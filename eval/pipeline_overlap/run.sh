#!/usr/bin/env bash
#
# run.sh -- the pipelined-context evidence, end to end: the RTL bench
# (tb_pipeline_overlap.sv) on the K-folded chains in jobs/, pipelined and one
# at a time, on the v1 and the v2 operand path, and the cost model's
# per-accelerator timeline held against every stamp (compare_timeline.py).
#
#   eval/pipeline_overlap/run.sh [OUT_DIR]        (default build/pipeline_overlap)
#
# Needs: build/bin/systolic-cost (cmake --build build), Verilator >= 5.
# Exit status 0 only if every bench passes and every comparison has no
# mismatch.  The summary is OUT_DIR/summary.txt.
set -u
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/../.." && pwd)"
OUT="${1:-$ROOT/build/pipeline_overlap}"
COST="$ROOT/build/bin/systolic-cost"
SIM="$ROOT/rtl/multi_200t/tile_pipelined/dma/tb/run_pipeline_sim.sh"
[ -x "$COST" ] || { echo "no $COST -- build first" >&2; exit 2; }
mkdir -p "$OUT"
: > "$OUT/summary.txt"
fail=0
# The overlaps each pipelined run must show (measured on the 2026-10-18 RTL;
# a change that re-serializes a stage fails here).  On the v1 operand path
# the fill is the longest stage, so no fold starts before the previous
# publish: only the fill overlaps; on v2 the next fold starts while the
# previous tile reduces.
overlaps() {   # jobs variant -> plusargs
  case "$1 $2" in
    "dev0 v1")  echo "+min_fill_overlaps=10" ;;
    "dev1 v1")  echo "+min_fill_overlaps=10" ;;
    "chain v1") echo "+min_fill_overlaps=10" ;;
    "dev0 v2")  echo "+min_fill_overlaps=10 +min_feed_overlaps=3" ;;
    "dev1 v2")  echo "+min_fill_overlaps=10 +min_feed_overlaps=5" ;;
    "chain v2") echo "+min_fill_overlaps=10 +min_feed_overlaps=1" ;;
  esac
}
for v in v1 v2; do
  for jobs in dev0 dev1 chain; do
    for mode in pipelined serial; do
      d="$OUT/${jobs}_${v}_${mode}"
      plus=""; [ "$mode" = serial ] && plus="+serial=1"
      [ "$mode" = pipelined ] && plus="$(overlaps "$jobs" "$v")"
      if bash "$SIM" 16 16 "$HERE/jobs/$jobs.svh" "$d" "$v" $plus > "$d.out" 2>&1; then
        echo "bench   $jobs $v $mode: PASS" | tee -a "$OUT/summary.txt"
      else
        echo "bench   $jobs $v $mode: FAIL (see $d.out)" | tee -a "$OUT/summary.txt"; fail=1; continue
      fi
      grep -E '^(PDEV|PSUM)' "$d/run.log" | sed 's/^/        /' | tee -a "$OUT/summary.txt"
      v2flag=""; [ "$v" = v2 ] && v2flag="--v2"
      # One job at a time nothing waits on another context or on a readback,
      # so the model must match every stamp with no stall to explain.
      strict=""; [ "$mode" = serial ] && strict="--no-stalls"
      if python3 "$HERE/compare_timeline.py" --cost "$COST" --run "$d/run.log" $v2flag $strict > "$d.compare" 2>&1; then
        echo "model   $jobs $v $mode: $(tail -1 "$d.compare")" | tee -a "$OUT/summary.txt"
      else
        echo "model   $jobs $v $mode: $(tail -1 "$d.compare")" | tee -a "$OUT/summary.txt"; fail=1
      fi
    done
  done
done

# The quiet-gap rule where it is what releases the fold (jobs/gapbound.svh,
# 4x4 at K_MAX 32 on v2): PE(0,0) must see exactly FEED_GAP = 26 idle cycles,
# and with the gap shrunk below the PE's minimum the two tiles merge and the
# bench must FAIL -- the negative control that the boundary check sees it.
d="$OUT/gapbound_v2_pipelined"
if bash "$SIM" 32 32 "$HERE/jobs/gapbound.svh" "$d" v2 +expect_min_gap=26 +min_feed_overlaps=3 > "$d.out" 2>&1; then
  echo "bench   gapbound v2 pipelined: PASS (smallest PE gap = FEED_GAP)" | tee -a "$OUT/summary.txt"
else
  echo "bench   gapbound v2 pipelined: FAIL (see $d.out)" | tee -a "$OUT/summary.txt"; fail=1
fi
grep -E '^(PDEV|PSUM|  gap-bound)' "$d/run.log" | sed 's/^/        /' | tee -a "$OUT/summary.txt"
if python3 "$HERE/compare_timeline.py" --cost "$COST" --run "$d/run.log" --v2 --k8 32 --k4 32 > "$d.compare" 2>&1; then
  echo "model   gapbound v2 pipelined: $(tail -1 "$d.compare")" | tee -a "$OUT/summary.txt"
else
  echo "model   gapbound v2 pipelined: $(tail -1 "$d.compare")" | tee -a "$OUT/summary.txt"; fail=1
fi
d="$OUT/gapbound_v2_gap8"
if PE_QUIET_GAP=8 bash "$SIM" 32 32 "$HERE/jobs/gapbound.svh" "$d" v2 > "$d.out" 2>&1; then
  echo "control gapbound v2 PE_QUIET_GAP=8: PASSED BUT MUST FAIL (merged tiles not detected)" | tee -a "$OUT/summary.txt"; fail=1
else
  echo "control gapbound v2 PE_QUIET_GAP=8: FAIL as expected ($(grep -m1 -E '^  FAIL' "$d/run.log"))" | tee -a "$OUT/summary.txt"
fi
exit $fail
