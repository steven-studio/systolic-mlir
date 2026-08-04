#!/usr/bin/env bash
# sweep_hls.sh -- cost-model sweep over (R, C, K_DIM) using the v++ HLS flow.
#
#   ./sweep_hls.sh csynth        # fast: resources + estimated latency
#   ./sweep_hls.sh cosim         # slow: exact RTL cycle counts
#   ./sweep_hls.sh csynth 6      # ...with -j 6
#   ./sweep_hls.sh one 4 4 8 1   # single point: R C K cosim(0|1)
#
# Generates one .cfg per point from the template below (same shape as the
# hand-written hls_systolic_8x8.cfg) and runs v++ against it. design.h
# guards R / C / K_DIM with #ifndef so -D on syn.cflags/tb.cflags is all
# that changes; no source edits needed.
#
# Each point runs in its own directory with its own TMPDIR: v++ leaves a
# lock in the work dir and uses unpredictable names under /tmp, so two
# concurrent points sharing either will collide.
#
# The limit is RAM, not cores. csynth is ~2-6 GB per process; cosim with
# XSIM is 8-16 GB. Pick -j from `free -g`, not `nproc`.

set -u

PART="${PART:-xc7a200tsbg484-1}"
CLOCK="${CLOCK:-10ns}"
TOP="${TOP:-matmul_4x4x4}"
TARGET_II="${TARGET_II:-1}"

SRC="$(cd "$(dirname "$0")" && pwd)"
OUT="$SRC/sweep_runs"

write_cfg() {
  local path=$1 R=$2 C=$3 K=$4
  cat > "$path" <<EOF
part=$PART

[hls]
syn.file=design.cpp
syn.top=$TOP
syn.cflags=-DR=$R -DC=$C -DK_DIM=$K
tb.file=testbench.cpp
tb.cflags=-DR=$R -DC=$C -DK_DIM=$K
clock=$CLOCK
clock_uncertainty=12.5%
syn.directive.pipeline=$TOP/time_loop II=$TARGET_II
EOF
}

run_point() {
  local R=$1 C=$2 K=$3 docosim=$4
  local tag="${R}x${C}x${K}"
  local dir="$OUT/$tag"

  rm -rf "$dir"
  mkdir -p "$dir/tmp"
  for f in design.cpp design.h testbench.cpp; do
    cp -f "$SRC/$f" "$dir/" 2>/dev/null
  done
  write_cfg "$dir/sweep.cfg" "$R" "$C" "$K"

  (
    cd "$dir" || exit 1
    export TMPDIR="$dir/tmp"
    {
      echo "### csynth $tag  (TIME_STEPS = $((K + R + C - 2)))"
      v++ -c --mode hls --config sweep.cfg --work_dir work || exit 1
      if [ "$docosim" = "1" ]; then
        echo "### cosim $tag"
        vitis-run --mode hls --cosim --config sweep.cfg --work_dir work || exit 1
      fi
    } > build.log 2>&1
  )

  if [ ! -d "$dir/work/hls/syn/report" ]; then
    echo "  FAIL $tag (csynth)  -> tail -30 $dir/build.log"
  elif [ "$docosim" = "1" ] && \
       [ ! -f "$dir/work/hls/sim/report/verilog/lat.rpt" ]; then
    echo "  FAIL $tag (cosim)   -> tail -30 $dir/build.log"
  else
    echo "  ok   $tag"
  fi
}
export -f run_point write_cfg
export OUT SRC PART CLOCK TOP TARGET_II

# Experiment A -- R=C=4, vary K. Slope of cycles vs K isolates II.
# Experiment B -- K=8, vary R=C. Tests the R+C-2 fill/drain term.
# Experiment C -- 4x8 vs 8x4: same R+C, so the model demands IDENTICAL
#                 cycle counts. A difference falsifies the model form.
POINTS="4 4 2
4 4 4
4 4 8
4 4 16
4 4 32
4 4 64
2 2 8
8 8 8
16 16 8
4 8 8
8 4 8
16 4 8
4 16 8
32 2 8
2 32 8"

MODE="${1:-csynth}"

if [ "$MODE" = "one" ]; then
  mkdir -p "$OUT"
  run_point "${2:-4}" "${3:-4}" "${4:-4}" "${5:-0}"
  exit 0
fi

case "$MODE" in
  csynth) DOCOSIM=0; DEFAULT_J=6 ;;
  cosim)  DOCOSIM=1; DEFAULT_J=2 ;;
  *) echo "usage: $0 {csynth|cosim|one R C K cosim} [jobs]" >&2; exit 1 ;;
esac
JOBS="${2:-$DEFAULT_J}"
export DOCOSIM

command -v v++ >/dev/null || {
  echo "v++ not on PATH -- source the Vitis settings64.sh first" >&2
  exit 1
}

mkdir -p "$OUT"
echo "mode=$MODE jobs=$JOBS part=$PART target_II=$TARGET_II"
echo "$POINTS" | xargs -P "$JOBS" -L1 bash -c 'run_point "$@" "$DOCOSIM"' _

echo
echo "done. parse with:  python3 parse_sweep.py $OUT"
