#!/usr/bin/env python3
"""kmax_transport_model.py -- the K_MAX sweep framework, analytical phase.

K_MAX is a synthesis-time parameter of each accelerator context (K_MAX_8X8
for the 8x8, K_MAX_4X4 for every 4x4).  A tile whose reduction length K
exceeds K_MAX runs as ceil(K / K_MAX) invocations, and every invocation pays
the per-invocation implementation overhead H again.  Whether that overhead is
visible depends on the operand transport: with the fill of invocation i+1
overlapped with the compute of invocation i, an invocation costs the larger of
its compute time and its DMA time, so below the compute/transport crossover
k* the H repeats show, and above it the DMA hides them.  K_MAX is therefore
not a BRAM/LUT/timing optimisation alone; it is chosen against beta.

The model (the thesis's, eq. costmodel + the transport term):

    T_compute(k, N)       = k + 2(N-1) + H                 cycles
    T_DMA(k, N, beta)     = 2*N*k / beta                   cycles  (A and B: 2*N*k words)
    T_invocation          = max(T_compute, T_DMA)          overlapped execution
    k*(N, beta)           = (2(N-1) + H) / (2N/beta - 1)   T_compute == T_DMA
                            (no crossover when 2N/beta <= 1: DMA is never the
                             longer side, every k is compute-bound)

For a tile of reduction length K on a context with K_MAX:

    n                 = ceil(K / K_MAX)
    k_i               = K_MAX for i < n, K - (n-1)*K_MAX for the last
    compute_cycles    = sum_i T_compute(k_i) = K + n*(2(N-1) + H)
    dma_cycles        = sum_i T_DMA(k_i)     = 2*N*K / beta
    overlapped_cycles = sum_i max(T_compute(k_i), T_DMA(k_i))
                        -- THE requested quantity: T_invocation summed over the
                           invocations.  It is what the Pareto screen and
                           sweep_plan.csv use.
    pipelined_time    = T_DMA(k_1) + sum_{i<n} max(T_compute(k_i), T_DMA(k_{i+1}))
                        + T_compute(k_n)
                        -- a SEPARATE, supplementary quantity: the pipelined
                           schedule as the RTL runs it (the first fill and the
                           last compute are exposed, every other fill hides
                           under the previous compute).  Reported beside
                           overlapped_cycles, never substituted for it.

The per-tile write-back (N*N words) is not in the model, as in the thesis:
in the RTL it runs in the write-back stage under the next fold and is exposed
once per job, independent of K_MAX.

H = 95 cycles per invocation (thesis, measured; H_4 = H_8).

PAYLOAD LANES.  The thesis model moves 2*N*k words for an N-wide tile.  The
CURRENT RTL does not: systolic_dma_top instantiates every context with
N_WIRE = N = 8 (both ACC_8X8 and ACC_4X4), the context's job_rx_bytes is
job_k * N_WIRE * 8, so a 4x4 invocation moves 2*8*k words and its buffers
keep lanes 0..3 (lane_in_array) -- the simulation shows words_written = 256
for a k=16 job on devices 1..3.  Therefore:

    --payload rtl          (DEFAULT)  N=8 -> 8 lanes, N=4 -> 8 lanes: the RTL
                                      as implemented.  k* for N=4 uses the
                                      moved width: (2(N-1)+H) / (2*8/beta - 1).
    --payload thesis-4lane            N=8 -> 8 lanes, N=4 -> 4 lanes: the
                                      thesis formula with N=4, a REFERENCE /
                                      sensitivity mode describing a 4-lane
                                      payload format the RTL does not have.

Every output row records payload_model and payload_lanes.

BETA (operand supply, words per array cycle).  The thesis regimes:

    0.99   effective fill of the v1 operand path (one 32-bit write port per
           context: a 128-bit beat lands in four cycles)       [declared]
    3.51   measured DDR3 supply, N=8 DMA path                  [measured, N=8]
    3.627  measured DDR3 supply, N=8 DMA path, 90.7 % of 4.00 [measured, N=8]
    11.20  fold-average demand 2sK/(K+2(s-1)+H), s=8, K=256   [demand, sensitivity]

3.51 and 3.627 were measured on the N=8 DMA path.  For N=4 they are declared
aggregate transport parameters used for sensitivity analysis only; there is
no N=4 bandwidth measurement.  Every row records beta_basis.

FLEET (1 x 8x8 + 3 x 4x4).  fleet.csv evaluates

    beta_ctx = min(writer_cap, beta_aggregate / n_active)

This is an ANALYTICAL CONTENTION ASSUMPTION, not a measured per-context beta:
the equal split follows from dma_engine_multi's round-robin single-beat
bursts (and matches simulation with an ideal 1-beat/cycle channel: 1, 2 and
4 concurrent fills each took 260 cycles for 256 words), the writer cap is the
declared v1 fill limit (0.99) or the beat-wide v2 limit (4.0), and the
aggregate values are the N=8 single-stream measurements reused -- the DDR3
controller's efficiency under single-beat bursts has not been measured.

Outputs (all under --out):
    analysis.csv     one row per (geometry, beta, K_MAX, K)
    pareto.csv       per (geometry, beta): which K_MAX are potentially
                     Pareto-optimal over {overlapped_cycles at every
                     representative K} x {operand memory}  (default metric;
                     --pareto-metric pipelined switches to pipelined_time)
    fleet.csv        beta_ctx and k* under 1/2/4 active contexts, both writers
    sweep_plan.csv   the proposed (K8, K4) x beta x geometry rows in the sweep
                     schema, Vivado columns empty, status = analysis_only

Numbers are kept at full precision internally and rounded once, when a CSV or
the console is written.

Nothing here runs Vivado, synthesis or implementation.  The script only
evaluates the model and writes the plan the Vivado phase will fill in.
"""
import argparse
import csv
import math
import os
import sys

# ---------------------------------------------------------------------------
# the model (unchanged)
# ---------------------------------------------------------------------------

def t_compute(k, n_edge, h):
    """T_compute(k, N) = k + 2(N-1) + H."""
    return k + 2 * (n_edge - 1) + h


def dma_words(k, lanes):
    """Operand words of one invocation: A (lanes x k) and B (k x lanes)."""
    return 2 * lanes * k


def t_dma(k, lanes, beta):
    """T_DMA(k, N, beta) = 2*N*k / beta, with N = the lanes actually moved."""
    return dma_words(k, lanes) / beta


def k_star(n_edge, lanes, beta, h):
    """k* = (2(N-1)+H) / (2*lanes/beta - 1); None when there is no crossover.
    lanes == n_edge gives the thesis formula verbatim."""
    denom = 2.0 * lanes / beta - 1.0
    if denom <= 0:
        return None
    return (2 * (n_edge - 1) + h) / denom


def split(k_total, k_max):
    """ceil(K/K_MAX) invocations: K_MAX each, the remainder last."""
    n = math.ceil(k_total / k_max)
    return [k_max] * (n - 1) + [k_total - (n - 1) * k_max]


def evaluate(k_total, k_max, n_edge, lanes, beta, h):
    """Full precision; nothing is rounded here."""
    ks = split(k_total, k_max)
    tc = [t_compute(k, n_edge, h) for k in ks]
    td = [t_dma(k, lanes, beta) for k in ks]
    overlapped = sum(max(a, b) for a, b in zip(tc, td))
    pipelined = td[0]
    for i in range(len(ks) - 1):
        pipelined += max(tc[i], td[i + 1])
    pipelined += tc[-1]
    # bottleneck of the full-size invocations (the first one is full size)
    if td[0] > tc[0] * 1.02:
        bottleneck = "transport"
    elif tc[0] > td[0] * 1.02:
        bottleneck = "compute"
    else:
        bottleneck = "balanced"
    return {
        "invocation_count": len(ks),
        "compute_cycles": sum(tc),
        "dma_cycles": sum(td),
        "overlapped_cycles": overlapped,
        "pipelined_time": pipelined,
        "bottleneck": bottleneck,
        "dma_words": sum(dma_words(k, lanes) for k in ks),
    }


def operand_words(n_edge, k_max):
    """Operand memory of one context: A and B, ping-pong, N banks x K_MAX."""
    return 2 * 2 * n_edge * k_max


def operand_bram18k_est(n_edge, k_max):
    """ESTIMATE ONLY, v1 buffers (USE_V2=0): each of the 4*N banks is a
    32-bit, 1W1R block RAM of K_MAX words -> one 18 Kb block (512 x 36) up to
    512 deep, one more per further 512.  Operand buffers only; the v2 CYCLIC
    layout and everything else in the fleet are not covered.  Vivado's BRAM
    column replaces this."""
    return 4 * n_edge * math.ceil(k_max / 512)


# ---------------------------------------------------------------------------
# labels
# ---------------------------------------------------------------------------

PAYLOAD_LANES = {
    "rtl":          {8: 8, 4: 8},   # the current RTL: N_WIRE = 8 for every context
    "thesis-4lane": {8: 8, 4: 4},   # the thesis formula with N = 4 (reference only)
}

MEASURED_N8 = (3.51, 3.627)


def beta_basis(beta, n_edge):
    if beta in MEASURED_N8:
        return ("measured_N8_path" if n_edge == 8
                else "declared_for_N4_(N8_measurement_reused)")
    if beta == 0.99:
        return "declared_v1_fill_limit"
    if beta == 11.2:
        return "demand_sensitivity_case"
    return "user_declared"


FLEET_BASIS = "analytical_contention_assumption_min(cap,beta_agg/n_active)"

SWEEP_SCHEMA = ["K8", "K4", "beta", "geometry", "k_star",
                "BRAM", "LUT", "FF", "DSP", "WNS", "TNS", "WHS", "THS",
                "invocation_count", "compute_cycles", "dma_cycles",
                "overlapped_cycles", "status",
                "payload_model", "payload_lanes", "beta_basis"]

DEFAULT_PAIRS = "16:16,32:16,32:32,128:128,256:256,512:512"
BRAM_CLIFF_PAIR = (1024, 1024)


# ---------------------------------------------------------------------------
# helpers
# ---------------------------------------------------------------------------

def r2(x):
    """Round once, for a CSV cell."""
    if x is None:
        return ""
    if isinstance(x, float):
        return round(x, 2)
    return x


def fmt1(x):
    """Round once, for the console."""
    if x is None:
        return "none"
    if isinstance(x, float):
        return f"{x:.1f}"
    return str(x)


def parse_list(s, cast):
    return [cast(v) for v in s.split(",") if v.strip()]


def pareto_front(rows, cost_key, time_keys):
    """A row is dominated if another row is <= on every objective and < on at
    least one.  Rows equal on every objective are mutually non-dominated."""
    front = []
    for r in rows:
        dominated = False
        for o in rows:
            if o is r:
                continue
            le = o[cost_key] <= r[cost_key] and all(o[t] <= r[t] for t in time_keys)
            lt = o[cost_key] < r[cost_key] or any(o[t] < r[t] for t in time_keys)
            if le and lt:
                dominated = True
                break
        front.append(not dominated)
    return front


def write_csv(path, rows):
    with open(path, "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
        w.writeheader()
        w.writerows(rows)


# ---------------------------------------------------------------------------
# main
# ---------------------------------------------------------------------------

def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--H", type=int, default=95,
                    help="per-invocation implementation overhead, cycles (thesis: 95)")
    ap.add_argument("--betas", default="0.99,3.51,3.627,11.20",
                    help="operand supply regimes, words/cycle")
    ap.add_argument("--kmax8", default="16,32,64,128,256,512",
                    help="candidate K_MAX_8X8 values (legal: powers of two >= 16)")
    ap.add_argument("--kmax4", default="16,32,64,128,256,512",
                    help="candidate K_MAX_4X4 values (legal: powers of two >= 16)")
    ap.add_argument("--K", default="16,64,256,1024",
                    help="representative tile reduction lengths K")
    ap.add_argument("--K-rep", type=int, default=256,
                    help="the K shown in the console tables and used for sweep_plan.csv")
    ap.add_argument("--payload", choices=sorted(PAYLOAD_LANES), default="rtl",
                    help="rtl (default): the current RTL moves the 8-lane payload to every "
                         "context; thesis-4lane: the thesis formula with N=4 (reference only)")
    ap.add_argument("--as-implemented", action="store_true",
                    help="alias of --payload rtl (the default)")
    ap.add_argument("--writer-caps", default="v1:0.99,v2:4.0",
                    help="declared per-context fill limits for the fleet assumption")
    ap.add_argument("--fleet-active", default="1,2,4",
                    help="numbers of contexts filling at the same time")
    ap.add_argument("--cost", choices=["words", "bram18k"], default="words",
                    help="memory cost for the Pareto screen: operand words, or the 18 Kb "
                         "block ESTIMATE (flat up to 512 deep per bank)")
    ap.add_argument("--pareto-metric", choices=["overlapped", "pipelined"], default="overlapped",
                    help="time objective for the Pareto screen: overlapped_cycles (default, "
                         "the thesis quantity) or pipelined_time")
    ap.add_argument("--pairs", default=DEFAULT_PAIRS,
                    help=f"(K8:K4) pairs for sweep_plan.csv (default {DEFAULT_PAIRS})")
    ap.add_argument("--bram-cliff", action="store_true",
                    help="also add the optional 1024:1024 BRAM-cliff experiment to the plan")
    ap.add_argument("--out", default="out", help="output directory")
    args = ap.parse_args()

    if args.as_implemented:
        args.payload = "rtl"
    h = args.H
    betas = parse_list(args.betas, float)
    kmax = {8: parse_list(args.kmax8, int), 4: parse_list(args.kmax4, int)}
    ks_rep = parse_list(args.K, int)
    lanes_of = PAYLOAD_LANES[args.payload]
    time_key = "overlapped_cycles" if args.pareto_metric == "overlapped" else "pipelined_time"
    cost_key = "operand_words" if args.cost == "words" else "operand_bram18k_est"
    os.makedirs(args.out, exist_ok=True)

    print(f"model: T_compute = k + 2(N-1) + H,  T_DMA = 2*lanes*k/beta,  "
          f"T_invocation = max(.,.),  H = {h}")
    print(f"payload: {args.payload}  ->  N=8 moves {lanes_of[8]} lanes, N=4 moves {lanes_of[4]} lanes"
          + ("   (the current RTL: N_WIRE = 8 for every context)" if args.payload == "rtl"
             else "   (REFERENCE ONLY: thesis N=4 formula, a payload format the RTL does not have)"))
    print(f"betas: {betas} words/cycle -- 3.51 / 3.627 measured on the N=8 path, declared"
          f" aggregate sensitivity parameters for N=4; 0.99 declared v1 fill; 11.2 demand case")
    print(f"time columns: overlapped_cycles = sum max(T_compute, T_DMA) (the requested model);"
          f" pipelined_time = first fill + last compute exposed (supplementary)")
    print(f"Pareto screen: {time_key} at K in {ks_rep}  vs  {cost_key}")
    print()

    # ---- analysis.csv ---------------------------------------------------------
    analysis_rows = []
    for n_edge in (8, 4):
        lanes = lanes_of[n_edge]
        for beta in betas:
            ks = k_star(n_edge, lanes, beta, h)
            for km in kmax[n_edge]:
                for K in ks_rep:
                    e = evaluate(K, km, n_edge, lanes, beta, h)
                    analysis_rows.append({
                        "geometry": f"{n_edge}x{n_edge}", "N": n_edge,
                        "payload_model": args.payload, "payload_lanes": lanes,
                        "beta": beta, "beta_basis": beta_basis(beta, n_edge), "H": h,
                        "k_star": r2(ks),
                        "K_MAX": km, "K": K,
                        "operand_words": operand_words(n_edge, km),
                        "operand_bram18k_est": operand_bram18k_est(n_edge, km),
                        "invocation_count": e["invocation_count"],
                        "compute_cycles": e["compute_cycles"],
                        "dma_cycles": r2(e["dma_cycles"]),
                        "overlapped_cycles": r2(e["overlapped_cycles"]),
                        "pipelined_time": r2(e["pipelined_time"]),
                        "bottleneck": e["bottleneck"],
                        "dma_words": e["dma_words"],
                    })
    write_csv(os.path.join(args.out, "analysis.csv"), analysis_rows)

    # ---- console tables at K_rep + pareto.csv --------------------------------
    pareto_rows = []
    for n_edge in (8, 4):
        lanes = lanes_of[n_edge]
        print(f"=== N = {n_edge}  (payload {args.payload}: {lanes} lanes moved per invocation;"
              f" T_compute = k + {2*(n_edge-1)} + {h} = k + {2*(n_edge-1)+h}) ===")
        for beta in betas:
            ks = k_star(n_edge, lanes, beta, h)
            print(f"\n  beta = {beta:<6} [{beta_basis(beta, n_edge)}]  k* = {fmt1(ks)}"
                  + ("" if ks is not None else
                     f"   (2*lanes/beta = {2*lanes/beta:.3f} <= 1: no crossover, compute-bound at every k)"))
            print(f"  {'K_MAX':>6} {'K':>5} {'n_inv':>5} {'compute':>9} {'dma':>9} "
                  f"{'overlapped':>10} {'pipelined':>9}  {'bottleneck':<10} {'op_words':>8} {'bram18k~':>8}")
            cand = []
            for km in kmax[n_edge]:
                evs = {K: evaluate(K, km, n_edge, lanes, beta, h) for K in ks_rep}
                e = evs[args.K_rep]
                print(f"  {km:>6} {args.K_rep:>5} {e['invocation_count']:>5} "
                      f"{e['compute_cycles']:>9} {fmt1(e['dma_cycles']):>9} "
                      f"{fmt1(e['overlapped_cycles']):>10} {fmt1(e['pipelined_time']):>9}  "
                      f"{e['bottleneck']:<10} {operand_words(n_edge, km):>8} "
                      f"{operand_bram18k_est(n_edge, km):>8}")
                row = {"geometry": f"{n_edge}x{n_edge}",
                       "payload_model": args.payload, "payload_lanes": lanes,
                       "beta": beta, "beta_basis": beta_basis(beta, n_edge),
                       "k_star": r2(ks), "K_MAX": km,
                       "operand_words": operand_words(n_edge, km),
                       "operand_bram18k_est": operand_bram18k_est(n_edge, km),
                       "pareto_metric": time_key, "pareto_cost": cost_key}
                exact = {}
                for K in ks_rep:
                    exact[f"time_K{K}"] = evs[K][time_key]
                    row[f"time_K{K}"] = r2(evs[K][time_key])
                    row[f"ninv_K{K}"] = evs[K]["invocation_count"]
                row["bottleneck_at_Krep"] = e["bottleneck"]
                row["_exact"] = exact
                cand.append(row)
            # dominance on the full-precision values
            screen = [{cost_key: r[cost_key], **r["_exact"]} for r in cand]
            front = pareto_front(screen, cost_key, [f"time_K{K}" for K in ks_rep])
            for r, on in zip(cand, front):
                r["pareto_candidate"] = int(on)
                del r["_exact"]
                pareto_rows.append(r)
            print(f"  potentially Pareto-optimal K_MAX ({time_key} at K in {ks_rep} vs {cost_key}): "
                  + ", ".join(str(r["K_MAX"]) for r, on in zip(cand, front) if on))
        print()
    write_csv(os.path.join(args.out, "pareto.csv"), pareto_rows)

    # ---- fleet.csv: the contention ASSUMPTION ---------------------------------
    caps = {}
    for item in args.writer_caps.split(","):
        name, val = item.split(":")
        caps[name.strip()] = float(val)
    actives = parse_list(args.fleet_active, int)
    fleet_rows = []
    print("=== fleet 1x8x8 + 3x4x4: per-context beta under concurrent DMA ===")
    print("    ANALYTICAL CONTENTION ASSUMPTION, not a measured per-context beta:")
    print("    beta_ctx = min(writer_cap, beta_aggregate / n_active); k* evaluated at beta_ctx")
    print("    (equal split = dma_engine_multi round-robin single-beat bursts, matches the ideal-"
          "channel simulation; aggregate values = N=8 single-stream measurements reused)")
    print(f"  {'writer':<6} {'cap':>5} {'beta_agg':>8} {'n_act':>5} {'beta_ctx':>8} "
          f"{'k*_8x8':>8} {'k*_4x4':>8}  limited_by")
    for wname, cap in caps.items():
        for beta in betas:
            for n_act in actives:
                beta_ctx = min(cap, beta / n_act)
                k8 = k_star(8, lanes_of[8], beta_ctx, h)
                k4 = k_star(4, lanes_of[4], beta_ctx, h)
                lim = "writer_cap" if cap <= beta / n_act else "memory_system_share"
                print(f"  {wname:<6} {cap:>5} {beta:>8} {n_act:>5} {beta_ctx:>8.3f} "
                      f"{fmt1(k8):>8} {fmt1(k4):>8}  {lim}")
                fleet_rows.append({"writer": wname, "writer_cap": cap,
                                   "beta_aggregate": beta,
                                   "beta_aggregate_basis": beta_basis(beta, 8),
                                   "n_active": n_act, "beta_ctx": r2(beta_ctx),
                                   "payload_model": args.payload,
                                   "payload_lanes_8x8": lanes_of[8],
                                   "payload_lanes_4x4": lanes_of[4],
                                   "k_star_8x8": r2(k8), "k_star_4x4": r2(k4),
                                   "limited_by": lim, "basis": FLEET_BASIS})
    write_csv(os.path.join(args.out, "fleet.csv"), fleet_rows)
    print()

    # ---- sweep_plan.csv: the sweep schema, Vivado columns empty --------------
    pairs = [tuple(int(v) for v in p.split(":")) for p in parse_list(args.pairs, str)]
    if args.bram_cliff and BRAM_CLIFF_PAIR not in pairs:
        pairs.append(BRAM_CLIFF_PAIR)
    plan_rows = []
    for k8, k4 in pairs:
        for beta in betas:
            for n_edge, km in ((8, k8), (4, k4)):
                lanes = lanes_of[n_edge]
                ks = k_star(n_edge, lanes, beta, h)
                e = evaluate(args.K_rep, km, n_edge, lanes, beta, h)
                plan_rows.append({
                    "K8": k8, "K4": k4, "beta": beta, "geometry": f"{n_edge}x{n_edge}",
                    "k_star": r2(ks),
                    "BRAM": "", "LUT": "", "FF": "", "DSP": "",
                    "WNS": "", "TNS": "", "WHS": "", "THS": "",
                    "invocation_count": e["invocation_count"],
                    "compute_cycles": e["compute_cycles"],
                    "dma_cycles": r2(e["dma_cycles"]),
                    "overlapped_cycles": r2(e["overlapped_cycles"]),
                    "status": "analysis_only",
                    "payload_model": args.payload, "payload_lanes": lanes,
                    "beta_basis": beta_basis(beta, n_edge),
                })
    with open(os.path.join(args.out, "sweep_plan.csv"), "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=SWEEP_SCHEMA)
        w.writeheader()
        w.writerows(plan_rows)
    print(f"=== sweep plan: {len(pairs)} (K8, K4) pairs, analytical columns at K_rep = {args.K_rep} ===")
    print("  " + ", ".join(f"({a},{b})" for a, b in pairs)
          + ("" if args.bram_cliff else "   (+ optional 1024:1024 BRAM-cliff experiment with --bram-cliff)"))
    print(f"  sweep_plan.csv: {len(plan_rows)} rows = {len(pairs)} pairs x {len(betas)} betas x 2 geometries")
    print(f"  schema: {','.join(SWEEP_SCHEMA)}")
    print(f"\nwritten: {args.out}/analysis.csv  pareto.csv  fleet.csv  sweep_plan.csv")
    print("Vivado was NOT launched; this phase is analytical only.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
