#!/usr/bin/env python3
"""Merge the k_max sweep's synthesis summaries and simulation cycle counts
into one table, and report the derived quantities the sweep exists to answer.

    python3 collect_kmax.py --dir build_kmax --out build_kmax/kmax_sweep.csv

Reads:
    build_kmax/k<K>/summary.csv     written by build_kmax.tcl (Vivado APIs)
    build_kmax/logs/sim_k<K>.log    KMAXCSV line from tb_array_fold_kmax

Derived columns, and why each is here rather than left to the reader:

  latency_us      total_cycles / fmax. Cycles alone cannot rank the points,
                  because a deeper k_max buys cycles-per-MAC while costing
                  Fmax. Only the product is comparable.

  macs            8*8*K useful multiply-accumulates per transaction.

  util_pct        macs / (64 * total_cycles). The fraction of the array's
                  peak the configuration actually reaches. The fixed ~32x
                  add-latency reduction tail is what keeps this low at small
                  k_max, and amortising it is the entire argument for a
                  larger one.

  macs_per_us     the throughput figure to rank on.

Knee detection is deliberately reported as marginal return per point rather
than asserted: the script prints the numbers and names the Pareto-efficient
set, but does not declare a winner. Which point to ship depends on how much
LUT/FF budget the rest of the design needs, which is not in this table.
"""

import argparse
import csv
import glob
import os
import re
import sys


def read_synth(d):
    """k -> dict of synthesis metrics, from each summary.csv."""
    out = {}
    for path in sorted(glob.glob(os.path.join(d, "k*", "summary.csv"))):
        try:
            with open(path) as f:
                rows = list(csv.DictReader(f))
            if not rows:
                print(f"  warn: {path} has no data row", file=sys.stderr)
                continue
            r = rows[0]
            out[int(r["k_max"])] = r
        except (OSError, KeyError, ValueError) as e:
            print(f"  warn: cannot parse {path}: {e}", file=sys.stderr)
    return out


def read_sim(d):
    """k -> dict of cycle counts, from the KMAXCSV line in each sim log."""
    out = {}
    pat = re.compile(r"^KMAXCSV,(\d+),(-?\d+),(-?\d+),(-?\d+),(\d+)", re.M)
    for path in sorted(glob.glob(os.path.join(d, "logs", "sim_k*.log"))):
        try:
            with open(path, errors="replace") as f:
                text = f.read()
        except OSError as e:
            print(f"  warn: cannot read {path}: {e}", file=sys.stderr)
            continue
        m = None
        for m in pat.finditer(text):
            pass                      # keep the last, in case of reruns
        if not m:
            print(f"  warn: no KMAXCSV line in {path}", file=sys.stderr)
            continue
        k, feed, drain, total, errors = (int(x) for x in m.groups())
        if errors:
            print(f"  warn: K={k} sim reported {errors} mismatches",
                  file=sys.stderr)
        out[k] = {"feed_cycles": feed, "drain_cycles": drain,
                  "total_cycles": total, "sim_errors": errors}
    return out


def fnum(v):
    try:
        return float(v)
    except (TypeError, ValueError):
        return None


FIELDS = ["k_max",
          "lut", "ff", "bram", "dsp",
          "wns_ns", "fmax_mhz", "timing_met",
          "feed_cycles", "drain_cycles", "total_cycles",
          "macs", "util_pct", "latency_us", "macs_per_us",
          "sim_errors"]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--dir", default="build_kmax")
    ap.add_argument("--out", default="build_kmax/kmax_sweep.csv")
    a = ap.parse_args()

    synth = read_synth(a.dir)
    sim = read_sim(a.dir)

    keys = sorted(set(synth) | set(sim))
    if not keys:
        print(f"no results found under {a.dir}/ -- nothing to collect")
        return 1

    rows = []
    for k in keys:
        s = synth.get(k, {})
        m = sim.get(k, {})
        row = {f: "" for f in FIELDS}
        row["k_max"] = k
        for f in ("lut", "ff", "bram", "dsp",
                  "wns_ns", "fmax_mhz", "timing_met"):
            row[f] = s.get(f, "")
        for f in ("feed_cycles", "drain_cycles", "total_cycles",
                  "sim_errors"):
            row[f] = m.get(f, "")

        total = m.get("total_cycles")
        fmax = fnum(s.get("fmax_mhz"))
        macs = 64 * k
        row["macs"] = macs
        if total:
            row["util_pct"] = round(100.0 * macs / (64.0 * total), 2)
            if fmax:
                lat_us = total / fmax          # cycles / MHz = microseconds
                row["latency_us"] = round(lat_us, 4)
                row["macs_per_us"] = round(macs / lat_us, 1)
        rows.append(row)

    os.makedirs(os.path.dirname(a.out) or ".", exist_ok=True)
    with open(a.out, "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=FIELDS)
        w.writeheader()
        w.writerows(rows)

    # ---------------- console table ----------------
    def cell(r, k, w, unit=""):
        v = r.get(k, "")
        return f"{str(v) + unit:>{w}}" if v != "" else f"{'--':>{w}}"

    print()
    print(f"{'k_max':>6} {'LUT':>8} {'FF':>8} {'BRAM':>6} {'DSP':>5} "
          f"{'Fmax':>8} {'cycles':>8} {'util%':>7} {'lat_us':>9} "
          f"{'MAC/us':>9} {'tmg':>4}")
    print("-" * 92)
    for r in rows:
        print(f"{r['k_max']:>6} {cell(r,'lut',8)} {cell(r,'ff',8)} "
              f"{cell(r,'bram',6)} {cell(r,'dsp',5)} {cell(r,'fmax_mhz',8)} "
              f"{cell(r,'total_cycles',8)} {cell(r,'util_pct',7)} "
              f"{cell(r,'latency_us',9)} {cell(r,'macs_per_us',9)} "
              f"{cell(r,'timing_met',4)}")

    # ---------------- sanity checks ----------------
    print()
    dsps = {r["dsp"] for r in rows if r["dsp"] not in ("", "NA")}
    if len(dsps) > 1:
        print(f"NOTE: DSP is NOT constant across k_max ({sorted(dsps)}).")
        print("      Expected 320 at every point (5 * R * C, k_max-invariant).")
        print("      A varying DSP count means something other than the")
        print("      operand buffers is scaling -- investigate before")
        print("      trusting the area trend.")
    elif dsps:
        print(f"DSP constant at {dsps.pop()} across all points, as expected.")

    bad = [r["k_max"] for r in rows
           if str(r.get("timing_met")) == "0"]
    if bad:
        print(f"TIMING NOT MET at k_max={bad}. Fmax for those points is the")
        print("achievable frequency, not a 100 MHz-safe result.")

    errs = [r["k_max"] for r in rows if r.get("sim_errors") not in ("", 0)]
    if errs:
        print(f"SIMULATION MISMATCHES at k_max={errs} -- area numbers for")
        print("those points describe hardware that computes wrong answers.")

    # ---------------- marginal return ----------------
    usable = [r for r in rows
              if r.get("macs_per_us") != "" and r.get("lut") not in ("", "NA")]
    if len(usable) >= 2:
        print()
        print("Marginal return per sweep step (throughput gained vs LUT spent):")
        print(f"{'step':>14} {'d_MAC/us':>10} {'d_LUT':>8} {'d_FF':>8} "
              f"{'MAC/us per kLUT':>17}")
        print("-" * 62)
        for prev, cur in zip(usable, usable[1:]):
            dt = cur["macs_per_us"] - prev["macs_per_us"]
            dl = fnum(cur["lut"]) - fnum(prev["lut"])
            df = fnum(cur["ff"]) - fnum(prev["ff"])
            eff = (dt / (dl / 1000.0)) if dl else float("inf")
            print(f"{str(prev['k_max']) + '->' + str(cur['k_max']):>14} "
                  f"{dt:>10.1f} {dl:>8.0f} {df:>8.0f} {eff:>17.1f}")
        print()
        print("The knee is the step where MAC/us per kLUT collapses. Read it")
        print("off the last column; this script does not pick a winner,")
        print("because the right choice also depends on how much of the")
        print("device the rest of your system needs.")

        # Pareto front on (throughput up, LUT down)
        front = []
        for r in usable:
            dominated = any(
                fnum(o["lut"]) <= fnum(r["lut"])
                and o["macs_per_us"] >= r["macs_per_us"]
                and (fnum(o["lut"]) < fnum(r["lut"])
                     or o["macs_per_us"] > r["macs_per_us"])
                for o in usable)
            if not dominated:
                front.append(r["k_max"])
        print(f"Pareto-efficient points (LUT vs throughput): {front}")

    print()
    print(f"wrote {a.out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
