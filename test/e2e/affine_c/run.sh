#!/usr/bin/env bash
#
# run.sh [M N K] -- the C GEMM through the affine path, stage by stage.
#
#   gemm.c                                     00-gemm.c (the source compiled)
#     -> clang -S -emit-llvm                     10-gemm.ll (LLVM IR, kept)
#     -> C front end: build/bin/systolic-cfront  20-gemm.affine.mlir
#        (clang 24 -ast-dump=json -> func/affine/arith/memref MLIR),
#        checked by systolic-opt (LLVM/MLIR 24)
#          FRONTEND=c (default)    the C source through the front end; needs
#                                  SYSTOLIC_CFRONT_CLANG = a clang 24 -- any
#                                  other clang is refused, never substituted
#          FRONTEND=fixture        the hand-written affine nest instead (the
#                                  input of the first hardware runs)
#     -> --systolic-declare-fpga-fleet           the 1x8x8 + 3x4x4 devices
#        --test-affine-matmul-discovery          30-analysis.txt
#     -> --affine-matmul-to-systolic             40-systolic.mlir
#        (recognition, exact affine partition over the fleet's plan, device
#        assignment and schedule of the binary-search optimizer)
#     -> --lower-systolic-matmul-tile-to-fpga    50-fpga.mlir
#     -> --systolic-runtime-to-llvm              60-llvm.mlir
#     -> mlir-translate, llc                     70-kernel.ll, 70-kernel.o
#     -> link main.c + gemm.c + a runtime       80-gemm-test, run: 90-run.txt
#          BACKEND=soft (default)  soft_runtime.c, a software model of the
#                                  runtime ABI -- NOT the FPGA
#          BACKEND=board           the real runtime (libsystolic_fpga_runtime):
#                                  DPTI to the programmed 1x8x8 + 3x4x4 board
#          SUBMISSION=auto (default) | sync | async
#                                  board only: async lowers the GEMM to the
#                                  asynchronous runtime (fpga_gemm_begin, one
#                                  fpga_tile_submit per tile, one
#                                  fpga_gemm_wait: every tile submitted
#                                  without waiting for the one before it);
#                                  auto chooses it when the configuration
#                                  says COMPLETION_HEADER=1, sync keeps one
#                                  fpga_matmul_tiled_auto_scheduled per tile
#
# Every intermediate is kept under build/test-output/affine_c_gemm/MxNxK
# (OUT= overrides). Each stage is reported PASS, FAIL, BLOCKED or NOT RUN;
# a stage after a failed one is NOT RUN. Exit status 0 only if no stage
# failed. With BACKEND=soft nothing talks to the board and the FPGA stage is
# NOT RUN.
#
# BACKEND=board needs SYSTOLIC_FPGA_CONFIG: the fpga_config.txt written next
# to the programmed .bit (as for test/e2e/run_e2e.sh --backend board). Its
# K_MAX_8X8 / K_MAX_4X4, as the runtime parses them, become the fleet's k_max
# and the FPGA lowering's phys-k-max (an explicit K_MAX_* that disagrees is
# refused), and its BITSTREAM_SHA256 is checked against its BITSTREAM. The
# FPGA does not report what it holds: that the board was programmed with
# that bitstream is trusted, not verified.
#
# Environment: LLVM_BUILD (the LLVM/MLIR 24 build; default
# ~/work/llvm-project/build), CLANG (default clang-18 / clang on PATH),
# K_MAX_8X8 / K_MAX_4X4 (the fleet's k_max; default 16 / 16), ORDER (the
# loop order, default ijk: with FRONTEND=c a C source with the loops in that
# order is written and compiled, the reference stays gemm.c), BACKEND (soft |
# board), FRONTEND (c | fixture), SYSTOLIC_CFRONT_CLANG (clang 24).

set -uo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/../../.." && pwd)"

M="${1:-16}"
N="${2:-16}"
K="${3:-16}"
K_MAX_8X8_GIVEN="${K_MAX_8X8:-}"
K_MAX_4X4_GIVEN="${K_MAX_4X4:-}"
K_MAX_8X8="${K_MAX_8X8:-16}"
K_MAX_4X4="${K_MAX_4X4:-16}"
ORDER="${ORDER:-ijk}"
BACKEND="${BACKEND:-soft}"
FRONTEND="${FRONTEND:-c}"
SUBMISSION="${SUBMISSION:-auto}"
case "$SUBMISSION" in
  auto|sync|async) ;;
  *) echo "SUBMISSION must be auto, sync or async, got '$SUBMISSION'" >&2; exit 2 ;;
esac
case "$FRONTEND" in
  c|fixture) ;;
  *) echo "FRONTEND must be c or fixture, got '$FRONTEND'" >&2; exit 2 ;;
esac
CFRONT="$ROOT/build/bin/systolic-cfront"
case "$BACKEND" in
  soft|board) ;;
  *) echo "BACKEND must be soft or board, got '$BACKEND'" >&2; exit 2 ;;
esac

LLVM_BUILD="${LLVM_BUILD:-$HOME/work/llvm-project/build}"
CLANG="${CLANG:-$(command -v clang-18 || command -v clang || true)}"
SYSTOLIC_OPT="$ROOT/build/bin/systolic-opt"
MLIR_TRANSLATE="$LLVM_BUILD/bin/mlir-translate"
LLC="$LLVM_BUILD/bin/llc"

# ---------------------------------------------------------------------------
# The board's configuration, checked before anything is compiled for it.
CONFIG_NOTE=""
if [ "$BACKEND" = board ]; then
  RTTEST="$ROOT/build/bin/runtime-dispatch-test"
  [ -n "${SYSTOLIC_FPGA_CONFIG:-}" ] || {
    echo "BACKEND=board needs SYSTOLIC_FPGA_CONFIG=<the fpga_config.txt next to the programmed .bit>" >&2
    exit 2; }
  [ -x "$RTTEST" ] || { echo "missing $RTTEST -- build first" >&2; exit 2; }
  cfg="$("$RTTEST" --print-config 2>&1)" || {
    echo "the runtime refuses SYSTOLIC_FPGA_CONFIG=$SYSTOLIC_FPGA_CONFIG: $cfg" >&2; exit 2; }
  hw8="$(sed -n 's/^K_MAX_8X8=//p' <<< "$cfg")"
  hw4="$(sed -n 's/^K_MAX_4X4=//p' <<< "$cfg")"
  hwhdr="$(sed -n 's/^COMPLETION_HEADER=//p' <<< "$cfg")"
  hwdepth="$(sed -n 's/^PIPELINE_DEPTH=//p' <<< "$cfg")"
  # The context's job queue (1 when the record has none): > 1 declares the
  # pipelined fleet to the compiler, as systolic-matmul does.
  [[ "$hwdepth" =~ ^[1-9][0-9]*$ ]] || hwdepth=1
  for pair in "K_MAX_8X8 ${K_MAX_8X8_GIVEN:-} $hw8" "K_MAX_4X4 ${K_MAX_4X4_GIVEN:-} $hw4"; do
    set -- $pair
    if [ $# = 3 ] && [ "$2" != "$3" ]; then
      echo "$1=$2 was given, but the board's configuration says $3" >&2; exit 2
    fi
  done
  K_MAX_8X8="$hw8"
  K_MAX_4X4="$hw4"
  cfgval() { awk -v key="$1" '{ sub(/#.*/, ""); i = index($0, "="); if (!i) next
      k = substr($0, 1, i - 1); v = substr($0, i + 1)
      gsub(/^[ \t\r]+|[ \t\r]+$/, "", k); gsub(/^[ \t\r]+|[ \t\r]+$/, "", v)
      if (k == key) print v }' "$SYSTOLIC_FPGA_CONFIG" | tail -1; }
  bit="$(cfgval BITSTREAM)"
  sha="$(cfgval BITSTREAM_SHA256 | tr 'A-F' 'a-f')"
  case "$bit" in ""|/*) ;; *) bit="$(dirname "$SYSTOLIC_FPGA_CONFIG")/$bit" ;; esac
  if [ -n "$sha" ] && [ -n "$bit" ] && [ -f "$bit" ]; then
    got="$(sha256sum "$bit" | cut -d' ' -f1)"
    [ "$got" = "$sha" ] || {
      echo "BITSTREAM_SHA256 = $sha, but $bit hashes to $got" >&2; exit 2; }
    CONFIG_NOTE="K_MAX_8X8=$hw8 K_MAX_4X4=$hw4 PIPELINE_DEPTH=$hwdepth, BITSTREAM_SHA256 matches $(basename "$bit"); board contents trusted, not read back"
  else
    CONFIG_NOTE="K_MAX_8X8=$hw8 K_MAX_4X4=$hw4 PIPELINE_DEPTH=$hwdepth, bitstream hash NOT checked (BITSTREAM or BITSTREAM_SHA256 missing)"
  fi
fi

# The lowering: batch submission wherever the bitstream reports completions.
ASYNC=0
if [ "$BACKEND" = board ]; then
  case "$SUBMISSION" in
    async) [ "${hwhdr:-0}" = 1 ] || {
             echo "SUBMISSION=async needs a configuration with COMPLETION_HEADER=1; $SYSTOLIC_FPGA_CONFIG says '${hwhdr:-nothing}'" >&2; exit 2; }
           ASYNC=1 ;;
    auto)  [ "${hwhdr:-0}" = 1 ] && ASYNC=1 ;;
  esac
elif [ "$SUBMISSION" = async ]; then
  echo "SUBMISSION=async needs BACKEND=board (the software model has only the synchronous entry point)" >&2; exit 2
fi
SUBMISSION_NOTE="$([ "$ASYNC" = 1 ] && echo "asynchronous: fpga_gemm_begin / fpga_tile_submit per tile / fpga_gemm_wait, PIPELINE_DEPTH=${hwdepth:-?} in flight per array" || echo "synchronous: one fpga_matmul_tiled_auto_scheduled per tile")"

OUT="${OUT:-$ROOT/build/test-output/affine_c_gemm/${M}x${N}x${K}$([ "$ORDER" = ijk ] || echo "-$ORDER")$([ "$BACKEND" = soft ] || echo "-$BACKEND")$([ "$FRONTEND" = c ] || echo "-$FRONTEND")}"
mkdir -p "$OUT"
: > "$OUT/commands.txt"

STAGES=()
STATUS=()
NOTES=()
BROKEN=0

record() { STAGES+=("$1"); STATUS+=("$2"); NOTES+=("$3"); }

# run NAME LOG CMD...: run a stage unless an earlier one failed.
run() {
  local name="$1" log="$2"
  shift 2
  if [ "$BROKEN" = 1 ]; then
    record "$name" "NOT RUN" "an earlier stage failed"
    return 1
  fi
  printf '%q ' "$@" >> "$OUT/commands.txt"
  printf '\n' >> "$OUT/commands.txt"
  "$@" > "$log" 2>&1
  local rc=$?
  [ "$rc" = 0 ] && return 0
  BROKEN=1
  record "$name" "FAIL" "exit status $rc, see $(basename "$log")"
  return 1
}

check() {
  local name="$1" ok="$2" note="$3"
  if [ "$ok" = 1 ]; then
    record "$name" "PASS" "$note"
  else
    BROKEN=1
    record "$name" "FAIL" "$note"
  fi
}

for tool in "$SYSTOLIC_OPT" "$MLIR_TRANSLATE" "$LLC" "$CLANG"; do
  if [ -z "$tool" ] || [ ! -x "$tool" ]; then
    echo "missing tool: '${tool}' (LLVM_BUILD=$LLVM_BUILD CLANG=$CLANG)" >&2
    exit 2
  fi
done

DEFS=(-DGEMM_M="$M" -DGEMM_N="$N" -DGEMM_K="$K")
case "$ORDER" in
  ijk|ikj|jik|jki|kij|kji) ;;
  *) echo "ORDER must be a permutation of ijk, got '$ORDER'" >&2; exit 2 ;;
esac
bound() { case "$1" in i) echo "GEMM_M" ;; j) echo "GEMM_N" ;; k) echo "GEMM_K" ;; esac; }

# The C source compiled: gemm.c itself, or for another ORDER the same
# function with its loops in that order (still ordinary C; the reference the
# result is compared against is gemm.c, unpermuted).
if [ "$ORDER" = ijk ]; then
  cp "$HERE/gemm.c" "$OUT/00-gemm.c"
else
  {
    echo "/* gemm.c with its loops in $ORDER order, written by run.sh. */"
    sed -n '/^#ifndef GEMM_M/,/^#endif/p; /^#ifndef GEMM_N/,/^#endif/p; /^#ifndef GEMM_K/,/^#endif/p' "$HERE/gemm.c"
    echo "void gemm(float A[restrict GEMM_M][GEMM_K], float B[restrict GEMM_K][GEMM_N],"
    echo "          float C[restrict GEMM_M][GEMM_N]) {"
    indent="  "
    for (( l = 0; l < 3; ++l )); do
      v="${ORDER:$l:1}"
      echo "${indent}for (int $v = 0; $v < $(bound "$v"); ++$v)"
      indent="$indent  "
    done
    echo "${indent}C[i][j] += A[i][k] * B[k][j];"
    echo "}"
  } > "$OUT/00-gemm.c"
fi

# ---------------------------------------------------------------------------
# 1. C -> LLVM IR. Kept as the C compiler's view of the same source; the MLIR
#    path does not consume it (LLVM IR imports into MLIR only as the llvm
#    dialect, and nothing raises that to SCF/Affine).
IR_CLANG="$CLANG"
[ "$FRONTEND" = c ] && [ -n "${SYSTOLIC_CFRONT_CLANG:-}" ] && IR_CLANG="$SYSTOLIC_CFRONT_CLANG"
if run "C -> LLVM IR (clang)" "$OUT/10-clang.log" \
    "$IR_CLANG" -std=c11 -O0 -S -emit-llvm "${DEFS[@]}" "$OUT/00-gemm.c" \
    -o "$OUT/10-gemm.ll"; then
  record "C -> LLVM IR (clang)" "PASS" "$("$IR_CLANG" --version | head -1)"
fi

# ---------------------------------------------------------------------------
# 2. C -> MLIR (func / affine / arith / memref).
if [ "$FRONTEND" = c ]; then
  # The front end refuses a clang that is not clang 24 (exit 2); this stage
  # is then BLOCKED, and so is everything after it.
  if [ -z "${SYSTOLIC_CFRONT_CLANG:-}" ]; then
    BROKEN=1
    record "C -> MLIR (systolic-cfront)" "BLOCKED" \
      "SYSTOLIC_CFRONT_CLANG is not set (a clang 24, e.g. ~/work/clang24-build/bin/clang); no other clang is used in its place"
  elif run "C -> MLIR (systolic-cfront)" "$OUT/20-cfront.log" \
      "$CFRONT" --clang "$SYSTOLIC_CFRONT_CLANG" "${DEFS[@]}" \
      "$OUT/00-gemm.c" -o "$OUT/20-gemm.affine.mlir" &&
    run "C -> MLIR (systolic-cfront)" "$OUT/20-verify.log" \
      "$SYSTOLIC_OPT" "$OUT/20-gemm.affine.mlir" -o /dev/null; then
    loops="$(grep -c 'affine.for' "$OUT/20-gemm.affine.mlir")"
    record "C -> MLIR (systolic-cfront)" "PASS" \
      "$("$SYSTOLIC_CFRONT_CLANG" --version | head -1); $loops affine.for; verified by systolic-opt (MLIR 24)"
  fi
  KERNEL=gemm
else
  record "C -> MLIR (systolic-cfront)" "NOT RUN" \
    "FRONTEND=fixture: the hand-written affine nest 20-gemm.affine.mlir"
  # The written-out affine nest of gemm.c: the same three loops (in ORDER),
  # the same accesses, and llvm.noalias for restrict.
  {
    echo "// The affine form of gemm.c (M = $M, N = $N, K = $K, loop order $ORDER),"
    echo "// written by run.sh in place of a C frontend. restrict -> llvm.noalias."
    echo "func.func @gemm_systolic(%A: memref<${M}x${K}xf32> {llvm.noalias},"
    echo "                         %B: memref<${K}x${N}xf32> {llvm.noalias},"
    echo "                         %C: memref<${M}x${N}xf32> {llvm.noalias}) {"
    indent="  "
    for (( l = 0; l < 3; ++l )); do
      v="${ORDER:$l:1}"
      case "$v" in i) ub=$M ;; j) ub=$N ;; k) ub=$K ;; esac
      echo "${indent}affine.for %$v = 0 to $ub {"
      indent="$indent  "
    done
    echo "${indent}%a = affine.load %A[%i, %k] : memref<${M}x${K}xf32>"
    echo "${indent}%b = affine.load %B[%k, %j] : memref<${K}x${N}xf32>"
    echo "${indent}%c = affine.load %C[%i, %j] : memref<${M}x${N}xf32>"
    echo "${indent}%p = arith.mulf %a, %b : f32"
    echo "${indent}%s = arith.addf %c, %p : f32"
    echo "${indent}affine.store %s, %C[%i, %j] : memref<${M}x${N}xf32>"
    for (( l = 2; l >= 0; --l )); do
      indent="${indent:2}"
      echo "${indent}}"
    done
    echo "  return"
    echo "}"
  } > "$OUT/20-gemm.affine.mlir"
  KERNEL=gemm_systolic
fi

FLEET="--systolic-declare-fpga-fleet=k-max-8x8=$K_MAX_8X8 k-max-4x4=$K_MAX_4X4 pipeline-depth=${hwdepth:-1}"
PHYS="--lower-systolic-matmul-tile-to-fpga=phys-k-max-8x8=$K_MAX_8X8 phys-k-max-4x4=$K_MAX_4X4$([ "$ASYNC" = 1 ] && echo ' async=true')"

# ---------------------------------------------------------------------------
# 3. Affine analysis and partition (report only; the IR is not changed).
if run "Affine analysis + partition" "$OUT/30-analysis.txt" \
    "$SYSTOLIC_OPT" "$FLEET" --test-affine-matmul-discovery \
    "$OUT/20-gemm.affine.mlir" -o /dev/null; then
  regions="$(sed -n 's/.*fleet partition: \([0-9]*\) regions, exact.*/\1/p' "$OUT/30-analysis.txt")"
  ok=0
  grep -q "recognition result: success" "$OUT/30-analysis.txt" &&
    grep -q "row extent: static $M" "$OUT/30-analysis.txt" &&
    grep -q "column extent: static $N" "$OUT/30-analysis.txt" &&
    grep -q "reduction extent: static $K" "$OUT/30-analysis.txt" &&
    [ -n "$regions" ] && ok=1
  check "Affine analysis + partition" "$ok" \
    "GEMM ${M}x${N}x${K} recognized; ${regions:-no} exact regions"
fi

# ---------------------------------------------------------------------------
# 4. Affine -> systolic.matmul_tile (partition, devices, schedule).
if run "Affine -> Systolic dialect" "$OUT/40-systolic.log" \
    "$SYSTOLIC_OPT" "$FLEET" "--affine-matmul-to-systolic=report=true" \
    "$OUT/20-gemm.affine.mlir" -o "$OUT/40-systolic.mlir"; then
  tiles="$(grep -c 'systolic.matmul_tile' "$OUT/40-systolic.mlir")"
  assigned="$(grep -c 'systolic.matmul_tile.* on @acc_.* est_cycles = [0-9]* start_cycle = [0-9]*' "$OUT/40-systolic.mlir")"
  loops="$(grep -c 'affine.for' "$OUT/40-systolic.mlir")"
  ok=0
  [ "$tiles" -gt 0 ] && [ "$tiles" = "$assigned" ] && [ "$loops" = 0 ] &&
    [ "$tiles" = "$regions" ] && ok=1
  devices="$(grep -o 'on @acc_[0-9x_]*' "$OUT/40-systolic.mlir" | sort | uniq -c | awk '{printf "%s%s x%s", sep, $3, $1; sep=", "}')"
  check "Affine -> Systolic dialect" "$ok" \
    "$tiles matmul_tile ($assigned scheduled, $loops affine.for left): $devices"
fi

# ---------------------------------------------------------------------------
# 5. matmul_tile -> FPGA runtime calls.
if run "Systolic -> FPGA runtime calls" "$OUT/50-fpga.log" \
    "$SYSTOLIC_OPT" "$PHYS" "$OUT/40-systolic.mlir" -o "$OUT/50-fpga.mlir"; then
  ok=0
  if [ "$ASYNC" = 1 ]; then
    # One GEMM: one begin, a submit per tile, and the one wait after the
    # last submit (nothing between two submits reads a result).
    calls="$(grep -c 'llvm.call @fpga_tile_submit' "$OUT/50-fpga.mlir")"
    begins="$(grep -c 'llvm.call @fpga_gemm_begin' "$OUT/50-fpga.mlir")"
    waits="$(grep -c 'llvm.call @fpga_gemm_wait' "$OUT/50-fpga.mlir")"
    last_submit="$(grep -n 'llvm.call @fpga_tile_submit' "$OUT/50-fpga.mlir" | tail -1 | cut -d: -f1)"
    first_wait="$(grep -n 'llvm.call @fpga_gemm_wait' "$OUT/50-fpga.mlir" | head -1 | cut -d: -f1)"
    [ "$calls" = "$tiles" ] && [ "$begins" = 1 ] && [ "$waits" = 1 ] &&
      [ "${first_wait:-0}" -gt "${last_submit:-0}" ] &&
      ! grep -q 'systolic\.' "$OUT/50-fpga.mlir" && ok=1
    check "Systolic -> FPGA runtime calls" "$ok" \
      "$calls fpga_tile_submit between $begins fpga_gemm_begin and $waits fpga_gemm_wait (wait at line ${first_wait:-?}, last submit at line ${last_submit:-?})"
  else
    calls="$(grep -c 'llvm.call @fpga_matmul_tiled_auto_scheduled' "$OUT/50-fpga.mlir")"
    [ "$calls" = "$tiles" ] && ! grep -q 'systolic\.' "$OUT/50-fpga.mlir" && ok=1
    check "Systolic -> FPGA runtime calls" "$ok" "$calls runtime calls"
  fi
fi

# ---------------------------------------------------------------------------
# 6. -> LLVM dialect -> LLVM IR -> object.
if run "Bufferize + LLVM dialect" "$OUT/60-llvm.log" \
    "$SYSTOLIC_OPT" --systolic-runtime-to-llvm "$OUT/50-fpga.mlir" \
    -o "$OUT/60-llvm.mlir"; then
  record "Bufferize + LLVM dialect" "PASS" "60-llvm.mlir"
fi
if run "LLVM IR + object (mlir-translate, llc)" "$OUT/70-translate.log" \
    "$MLIR_TRANSLATE" --mlir-to-llvmir "$OUT/60-llvm.mlir" \
    -o "$OUT/70-kernel.ll" &&
  run "LLVM IR + object (mlir-translate, llc)" "$OUT/70-llc.log" \
    "$LLC" -O0 -filetype=obj -relocation-model=pic "$OUT/70-kernel.ll" \
    -o "$OUT/70-kernel.o"; then
  record "LLVM IR + object (mlir-translate, llc)" "PASS" "70-kernel.o"
fi

# ---------------------------------------------------------------------------
# 7. Link with a runtime and the C reference, run, compare.
#    mlir_c_runner_utils provides memrefCopy, which the lowered copies of
#    non-contiguous views call (systolic-matmul's JIT links it too).
if [ "$BACKEND" = board ]; then
  RUNTIME_SRC=("$ROOT/runtime/offload/board_stats.c" -I"$ROOT/runtime" -I"$ROOT/runtime/offload"
               "$ROOT/build/runtime/libsystolic_fpga_runtime.a"
               $(pkg-config --libs libftdi1) -lstdc++)
  RUN_ENV=(env SYSTOLIC_FPGA_CONFIG="$SYSTOLIC_FPGA_CONFIG"
           SYSTOLIC_RUNTIME_TRACE=1 SYSTOLIC_RUNTIME_TIMING=1)
  CHECK_NAME="Numerical check vs CPU reference (FPGA board over DPTI)"
  record "Board configuration" "PASS" "$CONFIG_NOTE"
  record "Submission" "PASS" "$SUBMISSION_NOTE"
else
  RUNTIME_SRC=("$ROOT/runtime/offload/soft_runtime.c" -I"$ROOT/runtime/offload")
  RUN_ENV=(env SOFT_K_MAX_8X8="$K_MAX_8X8" SOFT_K_MAX_4X4="$K_MAX_4X4")
  CHECK_NAME="Numerical check vs CPU reference (software runtime model)"
fi
# The reference is gemm.c (ijk) compiled by the host compiler on its own,
# renamed gemm_reference, with -ffp-contract=off: the front end gives every C
# operator its own MLIR op, which is that semantics.
if run "Link test binary" "$OUT/80-link.log" \
    "$CLANG" -std=c11 -O0 -ffp-contract=off "${DEFS[@]}" -Dgemm=gemm_reference \
    -c "$HERE/gemm.c" -o "$OUT/75-reference.o" &&
  run "Link test binary" "$OUT/80-link.log" \
    "$CLANG" -std=c11 -O0 "${DEFS[@]}" -DGEMM_KERNEL="$KERNEL" \
    "$HERE/main.c" "$OUT/75-reference.o" "$OUT/70-kernel.o" "${RUNTIME_SRC[@]}" \
    -L"$LLVM_BUILD/lib" -lmlir_c_runner_utils -Wl,-rpath,"$LLVM_BUILD/lib" \
    -o "$OUT/80-gemm-test"; then
  record "Link test binary" "PASS" "80-gemm-test ($BACKEND runtime)"
fi
if run "$CHECK_NAME" "$OUT/90-run.txt" "${RUN_ENV[@]}" "$OUT/80-gemm-test"; then
  record "$CHECK_NAME" "PASS" \
    "$(grep 'invocations per device' "$OUT/90-run.txt")"
fi

if [ "$BACKEND" = board ]; then
  if [ "$ASYNC" = 1 ]; then
    hw_calls="$(grep -c '^\[fpga-async\] accumulate' "$OUT/90-run.txt" 2>/dev/null || true)"
    batch="$(grep -o 'before_first_completion=[0-9]* behind_inflight=[0-9]* credit_waits=[0-9]* submits_blocked=[0-9]*' "$OUT/90-run.txt" | tail -1)"
    hw_note="$hw_calls tile(s) accumulated from completion records (runtime trace); ${batch:-no timing line}"
  else
    hw_calls="$(grep -c '^\[fpga-runtime\] job' "$OUT/90-run.txt" 2>/dev/null || true)"
    hw_note="$hw_calls invocations completed on the board (runtime trace)"
  fi
  if grep -q 'Verification (FPGA hardware): PASS' "$OUT/90-run.txt" 2>/dev/null; then
    record "FPGA execution" "PASS" \
      "$hw_note; $(grep 'bytes to / from' "$OUT/90-run.txt")"
  elif [ -f "$OUT/90-run.txt" ] && grep -q 'runtime backend = FPGA' "$OUT/90-run.txt"; then
    BROKEN=1
    record "FPGA execution" "FAIL" \
      "$(grep -E 'failed invocations|mismatching' "$OUT/90-run.txt" | paste -s -d ';' -)"
  else
    record "FPGA execution" "NOT RUN" "the test binary did not run"
  fi
else
  record "FPGA execution" "NOT RUN" \
    "BACKEND=soft never opens the board link (BACKEND=board does)"
fi

# ---------------------------------------------------------------------------
printf '\nC GEMM %sx%sx%s (loop order %s, %s runtime, %s submission) through the affine path  (artifacts: %s)\n\n' \
  "$M" "$N" "$K" "$ORDER" "$BACKEND" "$([ "$ASYNC" = 1 ] && echo asynchronous || echo synchronous)" "$OUT"
for i in "${!STAGES[@]}"; do
  printf '  %-8s %-58s %s\n' "${STATUS[$i]}" "${STAGES[$i]}" "${NOTES[$i]}"
done | tee "$OUT/summary.txt"

[ "$BROKEN" = 0 ]
