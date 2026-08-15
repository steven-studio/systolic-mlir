#!/usr/bin/env python3
"""Fit cosim cycles against  cycles = II*(K + R + C - 2) + depth.

Reports the residuals, not just the fit. A least-squares line through
anything looks plausible; what tells you the model is right is that every
point sits on it. If residuals grow with K, II is not constant -- the deep
operand banks are costing you something the model does not express, and the
headline speedup is not the one you will get in hardware.

Usage:  python3 fit_model.py sweep_results.csv [--rows 8] [--cols 8]
"""
import csv, sys, argparse

p = argparse.ArgumentParser()
p.add_argument("csvfile")
p.add_argument("--rows", type=int, default=8)
p.add_argument("--cols", type=int, default=8)
a = p.parse_args()

pts = []
with open(a.csvfile) as f:
    for row in csv.DictReader(f):
        try:
            pts.append((int(row["K"]), int(row["cosim_cycles"])))
        except (ValueError, TypeError, KeyError):
            print(f"skipping unparseable row: {row}", file=sys.stderr)

if len(pts) < 2:
    sys.exit("need at least 2 usable points")

pts.sort()
geom = a.rows + a.cols - 2
xs = [k + geom for k, _ in pts]      # beats
ys = [c for _, c in pts]

n = len(xs)
mx, my = sum(xs) / n, sum(ys) / n
sxx = sum((x - mx) ** 2 for x in xs)
if sxx == 0:
    sys.exit("all points at the same K -- sweep a range")
ii = sum((x - mx) * (y - my) for x, y in zip(xs, ys)) / sxx
depth = my - ii * mx

print(f"geometry     : {a.rows}x{a.cols}, fill/drain = R+C-2 = {geom}")
print(f"fitted II    : {ii:.4f}")
print(f"fitted depth : {depth:.2f} cycles  (per-call init/drain + handshake)")
print()
print(f"{'K':>5} {'beats':>7} {'measured':>9} {'model':>9} {'resid':>7} {'util%':>7}")

worst = 0.0
for (k, c), x in zip(pts, xs):
    m = ii * x + depth
    r = c - m
    worst = max(worst, abs(r))
    ideal = k * 1.0                     # one beat per k is the floor
    util = 100.0 * (ideal + geom) / c if c else 0.0
    print(f"{k:>5} {x:>7} {c:>9} {m:>9.1f} {r:>7.1f} {util:>7.1f}")

print()
print(f"worst |residual| = {worst:.2f} cycles")
if worst > 1.5:
    print("  -> model does NOT hold. Check achieved II in the csynth report")
    print("     at each K; a drifting II means the operand-bank depth is")
    print("     showing up in the schedule.")
elif abs(ii - 1.0) > 0.05:
    print(f"  -> line is straight but II is {ii:.2f}, not 1. The loop-carried")
    print("     fadd on acc[i][j] is the first thing to look at.")
else:
    print("  -> II=1 and the model holds across the sweep.")
