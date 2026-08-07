#!/usr/bin/env bash
# sweep_k.sh -- run cosim at several K and fit cycles = II*(K+R+C-2) + depth.
#
# One cosim run per K, because cosim aggregates latency over all calls in
# the testbench and a multi-K testbench would give you a min/max range you
# cannot attribute. Slow, but it is the only honest way to get the model.
#
# Two points are enough to fit a straight line; more points are how you find
# out the line is not straight. If the residuals are not ~0, the cost model
# is wrong -- most likely because II > 1 at large K where the operand banks
# get deep, which is exactly the failure mode the partition rewrite was
# meant to avoid. Do not fit two points and declare victory.

set -euo pipefail

KS="${*:-4 8 16 32 48 64}"
OUT=sweep_results.csv
echo "K,cosim_cycles" > "$OUT"

for K in $KS; do
  echo "=== cosim K=$K ==="
  vitis_hls -f run_hls.tcl -tclargs "$K" cosim > "hls_k${K}.log" 2>&1 || {
    echo "K=$K FAILED -- see hls_k${K}.log"; continue; }

  # The cosim report prints a latency table; grab the max column. Adjust the
  # pattern if your Vitis version formats it differently -- verify against
  # the .rpt by eye the first time rather than trusting this grep.
  RPT="hls_k${K}/sol1/sim/report/matmul_4x4x4_cosim.rpt"
  CYC=$(awk '/\| *(verilog|Verilog)/ {for(i=1;i<=NF;i++) if($i ~ /^[0-9]+$/) last=$i} END{print last}' "$RPT")
  echo "$K,$CYC" >> "$OUT"
  echo "K=$K -> $CYC cycles"
done

echo
echo "--- $OUT ---"
cat "$OUT"
echo
echo "Fit with:  python3 fit_model.py $OUT"
