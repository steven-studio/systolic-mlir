#!/usr/bin/env python3
r"""
parse_sweep.py -- read the v++ HLS sweep results and fit the cost model.

    python3 parse_sweep.py sweep_runs

Model under test:

    cycles = II * (K + R + C - 2) + depth
                   \___________/
                    TIME_STEPS

Checks, in increasing order of how much a failure would hurt:

  1. Overall fit of II and depth.
  2. Experiment A (R=C=4, K varying) -- the slope of cycles vs K gives II
     on its own, independent of the geometric term.
  3. Experiment C (4x8 vs 8x4) -- the model says only R+C enters, so these
     must match exactly. A difference falsifies the model FORM, not just
     the coefficients.

Also reports the II the tool actually achieved per point. The .cfg asks
for II=1; larger arrays may not get it, and a point where achieved II
differs from the target must not be pooled with the others in the fit.

Cosim latency (exact, from RTL simulation) is preferred. csynth latency
(a static estimate) is used as a fallback and flagged -- the two are not
interchangeable for a quantitative claim.
"""

import os
import re
import sys
import glob


def _find(point_dir, *parts):
    """Search both the v++ layout (work/hls/...) and any nested variant."""
    pat = os.path.join(point_dir, "**", *parts)
    hits = glob.glob(pat, recursive=True)
    return hits[0] if hits else None


def read_cosim_latency(point_dir):
    p = _find(point_dir, "sim", "report", "verilog", "lat.rpt")
    if not p:
        return None
    m = re.search(r'\$AVER_LATENCY\s*=\s*"(\d+)"', open(p).read())
    return int(m.group(1)) if m else None


def read_csynth(point_dir):
    """Pull total latency, achieved/target II and resources out of the rpt."""
    p = _find(point_dir, "syn", "report", "*_csynth.rpt")
    if not p:
        return {}
    lines = open(p).read().splitlines()
    out = {}

    # --- total latency: first all-numeric row after "+ Latency:" ---
    in_lat = False
    for ln in lines:
        if "+ Latency" in ln:
            in_lat = True
            continue
        if in_lat and ln.strip().startswith("|"):
            cells = [c.strip() for c in ln.strip().strip("|").split("|")]
            if cells and cells[0].isdigit():
                out["lat"] = int(cells[0])
                break

    # --- time_loop row: lat_min lat_max iter_lat II_ach II_tgt trip ---
    for ln in lines:
        if "time_loop" in ln and ln.strip().startswith("|"):
            nums = re.findall(r"\b\d+\b", ln)
            if len(nums) >= 6:
                out["loop_lat"] = int(nums[0])
                out["iter_lat"] = int(nums[2])
                out["ii_achieved"] = int(nums[3])
                out["ii_target"] = int(nums[4])
                out["trip"] = int(nums[5])
            break

    # --- utilization ---------------------------------------------
    # The table's first DATA row is literally named "DSP", so a header
    # test of "does this row mention DSP" matches it and clobbers the
    # real header. Anchor on the Name column instead.
    hdr, in_util = None, False
    want = {"DSP": "dsp", "DSP48E": "dsp", "LUT": "lut", "FF": "ff",
            "BRAM_18K": "bram", "URAM": "uram"}
    for ln in lines:
        if "Utilization Estimates" in ln:
            in_util = True
            continue
        if not in_util or not ln.strip().startswith("|"):
            continue
        cells = [c.strip() for c in ln.strip().strip("|").split("|")]
        up = [c.upper() for c in cells]
        if up and up[0] == "NAME":
            hdr = up
            continue
        if not hdr or not cells:
            continue
        row = cells[0].lower()
        suffix = ("" if row.startswith("total")
                  else "_avail" if row.startswith("available")
                  else "_pct" if row.startswith("utilization")
                  else None)
        if suffix is None:
            continue
        for name, val in zip(hdr, cells):
            key = want.get(name)
            if key and val.lstrip("-").isdigit():
                out[key + suffix] = int(val)
        if suffix == "_pct":
            break
    return out


def collect(root_dir):
    rows = []
    for d in sorted(glob.glob(os.path.join(root_dir, "*x*x*"))):
        tag = os.path.basename(d)
        try:
            R, C, K = (int(x) for x in tag.split("x"))
        except ValueError:
            continue
        syn = read_csynth(d)
        rows.append(dict(tag=tag, R=R, C=C, K=K, steps=K + R + C - 2,
                         cosim=read_cosim_latency(d), **syn))
    return rows


def lstsq(xs, ys):
    n = len(xs)
    if n < 2:
        return None, None, None
    mx, my = sum(xs) / n, sum(ys) / n
    sxx = sum((x - mx) ** 2 for x in xs)
    if sxx == 0:
        return None, None, None
    slope = sum((x - mx) * (y - my) for x, y in zip(xs, ys)) / sxx
    inter = my - slope * mx
    ss_tot = sum((y - my) ** 2 for y in ys)
    ss_res = sum((y - (slope * x + inter)) ** 2 for x, y in zip(xs, ys))
    return slope, inter, (1.0 if ss_tot == 0 else 1 - ss_res / ss_tot)


def main():
    root_dir = sys.argv[1] if len(sys.argv) > 1 else "sweep_runs"
    rows = collect(root_dir)
    if not rows:
        print(f"no sweep points found under {root_dir}")
        return 1

    g = lambda r, k: "-" if r.get(k) is None else str(r[k])

    def pct(r, key):
        v, a = r.get(key), r.get(key + "_avail")
        if v is None:
            return "-"
        if not a:
            return str(v)
        return f"{v}({100 * v // a}%)"

    print(f"{'point':>10} {'steps':>6} {'trip':>5} {'cosim':>7} {'csynth':>7} "
          f"{'II':>4} {'DSP':>12} {'LUT':>14} {'FF':>14}")
    print("-" * 92)
    for r in rows:
        print(f"{r['tag']:>10} {r['steps']:>6} {g(r,'trip'):>5} "
              f"{g(r,'cosim'):>7} {g(r,'lat'):>7} {g(r,'ii_achieved'):>4} "
              f"{pct(r,'dsp'):>12} {pct(r,'lut'):>14} {pct(r,'ff'):>14}")

    over = [r['tag'] for r in rows
            if r.get('dsp') and r.get('dsp_avail')
            and r['dsp'] > r['dsp_avail']]
    if over:
        print(f"\nover DSP budget on this part: {', '.join(over)} "
              f"-- cosim still valid (no place-and-route), but these will "
              f"not fit.")

    # trip count must equal TIME_STEPS -- if not, the loop bound in the
    # source is not what the model assumes and nothing below is meaningful.
    bad = [r['tag'] for r in rows
           if r.get('trip') is not None and r['trip'] != r['steps']]
    if bad:
        print(f"\n!! trip count != K+R+C-2 for: {', '.join(bad)}")

    cyc = lambda r: r['cosim'] if r.get('cosim') is not None else r.get('lat')
    usable = [r for r in rows if cyc(r) is not None]
    est = [r['tag'] for r in usable if r.get('cosim') is None]
    if est:
        print(f"\nNOTE: csynth ESTIMATES used for {', '.join(est)} -- "
              f"rerun with cosim before quoting these.")

    iis = {r.get('ii_achieved') for r in usable if r.get('ii_achieved')}
    if len(iis) > 1:
        print(f"\n!! achieved II is not constant across points: {sorted(iis)}")
        print("   Do not pool these in one fit -- group by achieved II.")

    print("\n=== fit: cycles = II * TIME_STEPS + depth ===")
    if len(usable) >= 2:
        II, depth, r2 = lstsq([r['steps'] for r in usable],
                              [cyc(r) for r in usable])
        print(f"II = {II:.3f}   depth = {depth:.3f}   R^2 = {r2:.5f}   "
              f"(n = {len(usable)})")
        print(f"\n{'point':>10} {'measured':>9} {'predicted':>10} {'err':>7}")
        for r in usable:
            pred = II * r['steps'] + depth
            print(f"{r['tag']:>10} {cyc(r):>9} {pred:>10.1f} "
                  f"{cyc(r) - pred:>7.1f}")
    else:
        print("need at least 2 points")

    a = [r for r in usable if r['R'] == 4 and r['C'] == 4]
    if len(a) >= 2:
        II_a, int_a, r2_a = lstsq([r['K'] for r in a], [cyc(r) for r in a])
        print(f"\n=== experiment A (R=C=4, K varying, n={len(a)}) ===")
        print(f"II = {II_a:.3f}   intercept = {int_a:.3f}   R^2 = {r2_a:.5f}")
        print(f"depth = intercept - II*(R+C-2) = {int_a - II_a * 6:.3f}")

    b = [r for r in usable if r['K'] == 8]
    if len(b) >= 2:
        s_b, i_b, r2_b = lstsq([r['R'] + r['C'] for r in b],
                               [cyc(r) for r in b])
        print(f"\n=== experiment B (K=8, R+C varying, n={len(b)}) ===")
        print(f"slope vs (R+C) = {s_b:.3f}   R^2 = {r2_b:.5f}")
        print("slope should equal II if the fill/drain term is R+C-2.")
        print(f"{'point':>10} {'PEs':>6} {'R+C':>5} {'cycles':>7}")
        for r in sorted(b, key=lambda r: r['R'] + r['C']):
            print(f"{r['tag']:>10} {r['R']*r['C']:>6} {r['R']+r['C']:>5} "
                  f"{cyc(r):>7}")

    # Experiment C: every (R,C) / (C,R) pair in the sweep. Transposed
    # arrays have the same R+C and the same PE count, so the model demands
    # identical cycle counts. A difference falsifies the model FORM.
    by = {(r['R'], r['C'], r['K']): cyc(r) for r in usable}
    pairs = sorted({tuple(sorted((R, C))) + (K,)
                    for (R, C, K) in by
                    if R != C and (C, R, K) in by})
    if pairs:
        print("\n=== experiment C (transposed arrays, same R+C) ===")
        for (lo, hi, K) in pairs:
            a_, b_ = by[(lo, hi, K)], by[(hi, lo, K)]
            verdict = "MATCH" if a_ == b_ else "DIFFER <-- model form wrong"
            print(f"  {lo}x{hi}x{K} = {a_:<5} {hi}x{lo}x{K} = {b_:<5} "
                  f"(R+C = {lo + hi})  {verdict}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
