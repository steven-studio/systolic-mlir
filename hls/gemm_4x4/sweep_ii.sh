#!/bin/sh
# sweep_ii.sh -- build both II design points and pull the numbers out.
#
#   sh sweep_ii.sh
#
# Verifies at the end that the two points actually DIFFER. A previous
# version of this sweep silently produced identical RTL twice, because
# the II was driven by a -D macro inside the pragma and Vitis does not
# macro-expand pragma text. Always confirm the achieved II changed
# before believing a sweep.
set -e

for II in 1 2; do
    cfg="hls_systolic.cfg"
    [ "$II" = "2" ] && cfg="hls_systolic_ii2.cfg"
    wd="work_ii${II}"

    echo "=============================================="
    echo "  TARGET II=${II}   (${cfg} -> ${wd})"
    echo "=============================================="
    v++ -c --mode hls --config "$cfg" --work_dir "$wd" > "${wd}_build.log" 2>&1 \
        || { echo "BUILD FAILED"; tail -30 "${wd}_build.log"; exit 1; }
    vitis-run --mode hls --cosim --config "$cfg" --work_dir "$wd" > "${wd}_cosim.log" 2>&1 \
        || { echo "COSIM FAILED"; tail -30 "${wd}_cosim.log"; exit 1; }

    grep -E "Pipelining result|Estimated Fmax" "${wd}_build.log" || true
    grep -E "co-simulation finished" "${wd}_cosim.log" || true
    grep -E "RTL Simulation : 1 / 1" "${wd}_cosim.log" || true
    echo
done

echo "=============================================="
echo "  sanity check: did the two points differ?"
echo "=============================================="
a=`grep -o "Final II = [0-9]*" work_ii1_build.log | head -1`
b=`grep -o "Final II = [0-9]*" work_ii2_build.log | head -1`
fa=`grep -o "Estimated Fmax: [0-9.]*" work_ii1_build.log | head -1`
fb=`grep -o "Estimated Fmax: [0-9.]*" work_ii2_build.log | head -1`
echo "  II=1 point : $a , $fa"
echo "  II=2 point : $b , $fb"
if [ "$a" = "$b" ] && [ "$fa" = "$fb" ]; then
    echo
    echo "  *** IDENTICAL -- the II directive did NOT take effect. ***"
    echo "  Check that syn.directive.pipeline names the loop correctly:"
    echo "      syn.directive.pipeline=<top>/<loop_label> II=<n>"
    echo "  The loop label in design.cpp is 'time_loop' and the top is"
    echo "  'matmul_4x4x4', so the path must be matmul_4x4x4/time_loop."
    exit 1
fi
echo "  OK -- the two design points are genuinely different."