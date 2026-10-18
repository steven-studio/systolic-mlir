#!/usr/bin/env python3
"""e2e_timing.py -- the host-side timing of a `run_e2e.sh --timing` run.

    python3 tools/e2e_timing.py <out dir>            # the table, one row per case
    python3 tools/e2e_timing.py <out dir> --csv F    # also write the rows as CSV

<out dir> is run_e2e.sh's --out directory; the script reads its timing.tsv
(case, exit status, wall-clock of the whole systolic-matmul run in
microseconds, and the runtime's timing line: `[fpga-runtime] timing`, the
per-invocation breakdown the synchronous runtime measured with std::chrono
around the MEM_WRITE, the settle sleep, the descriptor and the result read;
or, from a `--mode async` run, `[fpga-async] timing`: the span from the
first submit to the last wait, the time inside the submits and the waits,
and how often the pump saw two or four arrays busy and a slab staged behind
a running job. run_e2e.sh writes the line with its source, `runtime` or
`async`, as the first word; the two kinds get their own tables).

Two clocks, kept apart on purpose:
  wall_ms        the whole run: reading the input, compiling, the JIT, every
                 invocation, the CPU reference, the comparison
  fpga_ms        the sum of the runtime's per-invocation intervals only
so `wall_ms - fpga_ms` is what the host spends outside the link.

Every number here is host wall-clock. FPGA-internal cycles are a different
measurement (the VIO counters on the board, FSMTRANS in the co-simulation)
and never appear in this table; the co-simulation's wall-clock is the
simulator's speed, not the board's, so a `--backend cosim` timing table says
nothing about the board.
"""
import argparse
import csv
import os
import statistics
import sys


def parse_runtime_line(text):
    out = {}
    toks = text.split()
    out["_kind"] = toks[0] if toks and "=" not in toks[0] else "runtime"
    for tok in toks:
        if "=" in tok:
            k, v = tok.split("=", 1)
            try:
                out[k] = int(v)
            except ValueError:
                out[k] = v
    return out


def pct(part, whole):
    return 100.0 * part / whole if whole else 0.0


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("out_dir")
    ap.add_argument("--csv", help="write the rows to this CSV file as well")
    a = ap.parse_args()

    path = os.path.join(a.out_dir, "timing.tsv")
    if not os.path.exists(path):
        print(f"{path}: not found (run run_e2e.sh with --timing)", file=sys.stderr)
        return 2

    rows = []
    async_rows = []
    with open(path) as f:
        header = f.readline()
        for line in f:
            line = line.rstrip("\n")
            if not line:
                continue
            parts = line.split("\t")
            while len(parts) < 4:
                parts.append("")
            case, rc, wall_us, rt = parts[0], parts[1], parts[2], parts[3]
            r = parse_runtime_line(rt)
            if r["_kind"] == "async":
                span_us = r.get("span_us", 0)
                async_rows.append({
                    "case": case,
                    "exit": int(rc) if rc.lstrip("-").isdigit() else rc,
                    "tiles": r.get("completed", 0),
                    "wall_ms": int(wall_us) / 1000.0 if wall_us.isdigit() else float("nan"),
                    "span_ms": span_us / 1000.0,
                    "host_other_ms": (int(wall_us) - span_us) / 1000.0 if wall_us.isdigit() else float("nan"),
                    "tiles_per_s": 1e6 * r.get("completed", 0) / span_us if span_us else 0.0,
                    "us_per_tile": span_us / r["completed"] if r.get("completed") else 0.0,
                    "submit_ms": r.get("submit_us", 0) / 1000.0,
                    "wait_ms": r.get("wait_us", 0) / 1000.0,
                    "busy2_pct": pct(r.get("overlap2", 0), r.get("passes", 0)),
                    "busy4_pct": pct(r.get("overlap4", 0), r.get("passes", 0)),
                    "staged_pct": pct(r.get("staged_behind", 0), r.get("passes", 0)),
                    "reads": r.get("reads", 0),
                    "empty_reads": r.get("empty_reads", 0),
                    "max_queued": r.get("max_queued", 0),
                    "stage": r.get("stage", ""),
                    "read_policy": r.get("read_policy", ""),
                    "depth": r.get("depth", ""),
                    "max_inflight": r.get("max_inflight", ""),
                    "last_desc_ms": r.get("last_descriptor_us", 0) / 1000.0,
                    "first_cmpl_ms": r.get("first_completion_us", 0) / 1000.0,
                    "bytes_to_fpga": r.get("bytes_to_fpga", 0),
                    "bytes_from_fpga": r.get("bytes_from_fpga", 0),
                })
                continue
            inv = r.get("invocations", 0)
            fpga_us = r.get("total_us", 0)
            rows.append({
                "case": case,
                "exit": int(rc) if rc.lstrip("-").isdigit() else rc,
                "invocations": inv,
                "wall_ms": int(wall_us) / 1000.0 if wall_us.isdigit() else float("nan"),
                "fpga_ms": fpga_us / 1000.0,
                "host_other_ms": (int(wall_us) - fpga_us) / 1000.0 if wall_us.isdigit() else float("nan"),
                "inv_median_us": r.get("median_us", 0),
                "inv_p95_us": r.get("p95_us", 0),
                "inv_min_us": r.get("min_us", 0),
                "inv_max_us": r.get("max_us", 0),
                "mem_write_us": r.get("mem_write_us", 0),
                "settle_us": r.get("settle_us", 0),
                "submit_us": r.get("submit_us", 0),
                "read_us": r.get("read_us", 0),
                "settle_cfg_us": r.get("settle_cfg_us", ""),
                "bytes_to_fpga": r.get("bytes_to_fpga", 0),
                "bytes_from_fpga": r.get("bytes_from_fpga", 0),
            })

    if not rows and not async_rows:
        print("timing.tsv has no rows")
        return 1

    if async_rows:
        cols = ["case", "exit", "tiles", "wall_ms", "span_ms", "host_other_ms", "tiles_per_s",
                "us_per_tile", "submit_ms", "wait_ms", "busy2_pct", "busy4_pct", "staged_pct",
                "reads", "empty_reads", "max_queued", "stage", "read_policy", "depth",
                "max_inflight", "last_desc_ms", "first_cmpl_ms"]
        widths = {c: max(len(c), *(len(fmt(r[c])) for r in async_rows)) for c in cols}
        print("host timing, asynchronous runtime (wall-clock of the host; span = first submit to "
              "last wait; busy2/busy4 = pump passes with >= 2 / all 4 arrays in flight; "
              "staged = passes with a slab staged behind a running job)")
        print("  ".join(c.rjust(widths[c]) for c in cols))
        for r in async_rows:
            print("  ".join(fmt(r[c]).rjust(widths[c]) for c in cols))
        timed = [r for r in async_rows if r["tiles"] > 0]
        if timed:
            tiles = sum(r["tiles"] for r in timed)
            span = sum(r["span_ms"] for r in timed)
            print(f"cases with tiles: {len(timed)}; tiles: {tiles}; link span: {span:.1f} ms; "
                  f"mean per tile: {1000.0 * span / tiles:.1f} us; "
                  f"empty reads: {sum(r['empty_reads'] for r in timed)} of {sum(r['reads'] for r in timed)}")
        if not rows:
            if a.csv:
                with open(a.csv, "w", newline="") as f:
                    w = csv.DictWriter(f, fieldnames=list(async_rows[0].keys()))
                    w.writeheader()
                    w.writerows(async_rows)
                print(f"rows written to {a.csv}")
            return 0
        print()

    cols = ["case", "exit", "invocations", "wall_ms", "fpga_ms", "host_other_ms",
            "inv_median_us", "inv_p95_us", "inv_min_us", "inv_max_us",
            "mem_write_us", "settle_us", "submit_us", "read_us"]
    widths = {c: max(len(c), *(len(fmt(r[c])) for r in rows)) for c in cols}

    print("host timing (wall-clock of the host; FPGA-internal cycles are not in this table)")
    print("  ".join(c.rjust(widths[c]) for c in cols))
    for r in rows:
        print("  ".join(fmt(r[c]).rjust(widths[c]) for c in cols))

    timed = [r for r in rows if r["invocations"] > 0]
    if timed:
        total_inv = sum(r["invocations"] for r in timed)
        total_fpga_ms = sum(r["fpga_ms"] for r in timed)
        medians = [r["inv_median_us"] for r in timed]
        print(f"cases with invocations: {len(timed)}; invocations: {total_inv}; "
              f"link time: {total_fpga_ms:.1f} ms; "
              f"mean per invocation: {1000.0 * total_fpga_ms / total_inv:.1f} us; "
              f"median of the case medians: {statistics.median(medians):.0f} us")
        phases = {k: sum(r[k] for r in timed) for k in ("mem_write_us", "settle_us", "submit_us", "read_us")}
        tot = sum(phases.values()) or 1
        print("phase share of the link time: " + ", ".join(
            f"{k.replace('_us', '')} {100.0 * v / tot:.0f}%" for k, v in phases.items()))
        cfg = {r["settle_cfg_us"] for r in timed}
        print(f"settle sleep configured: {', '.join(str(c) for c in sorted(cfg, key=str))} us")

    if a.csv:
        with open(a.csv, "w", newline="") as f:
            w = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
            w.writeheader()
            w.writerows(rows)
        print(f"rows written to {a.csv}")
    return 0


def fmt(v):
    if isinstance(v, float):
        return f"{v:.1f}"
    return str(v)


if __name__ == "__main__":
    sys.exit(main())
