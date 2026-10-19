#!/usr/bin/env bash
#
# run_strided.sh -- a strided, permuted GEMM (loops k, i, j; operands at
# offsets and strides inside larger arrays) through the same compiler stages
# as run.sh, checked element by element against the same loops in C
# (strided_main.c).
#
#   FRONTEND=c (default)  strided_permuted.c through build/bin/systolic-cfront;
#                         needs SYSTOLIC_CFRONT_CLANG = a clang 24 (no other
#                         clang is used in its place)
#   FRONTEND=fixture      strided_permuted.mlir, the same nest by hand
#   BACKEND=soft (default) the software model of the runtime, NOT the FPGA
#   BACKEND=board         the real runtime and board; needs
#                         SYSTOLIC_FPGA_CONFIG (as for run.sh)
#
# Artifacts: build/test-output/affine_c_gemm/strided_permuted[-fixture]
# [-board] (OUT= overrides). Exit 0 on PASS. Environment as for run.sh
# (LLVM_BUILD, CLANG).

set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/../../.." && pwd)"
LLVM_BUILD="${LLVM_BUILD:-$HOME/work/llvm-project/build}"
CLANG="${CLANG:-$(command -v clang-18 || command -v clang)}"
SYSTOLIC_OPT="$ROOT/build/bin/systolic-opt"
FRONTEND="${FRONTEND:-c}"
BACKEND="${BACKEND:-soft}"
OUT="${OUT:-$ROOT/build/test-output/affine_c_gemm/strided_permuted$([ "$FRONTEND" = c ] || echo "-$FRONTEND")$([ "$BACKEND" = soft ] || echo "-$BACKEND")}"
mkdir -p "$OUT"

case "$FRONTEND" in
  c)
    if [ -z "${SYSTOLIC_CFRONT_CLANG:-}" ]; then
      echo "BLOCKED: FRONTEND=c needs SYSTOLIC_CFRONT_CLANG (a clang 24)" >&2
      exit 1
    fi
    cp "$HERE/strided_permuted.c" "$OUT/00-strided_permuted.c"
    "$ROOT/build/bin/systolic-cfront" \
      --clang "$SYSTOLIC_CFRONT_CLANG" "$OUT/00-strided_permuted.c" \
      -o "$OUT/20-affine.mlir"
    ;;
  fixture) cp "$HERE/strided_permuted.mlir" "$OUT/20-affine.mlir" ;;
  *) echo "FRONTEND must be c or fixture" >&2; exit 2 ;;
esac

case "$BACKEND" in
  soft)
    RUNTIME=("$ROOT/runtime/offload/soft_runtime.c" -I"$ROOT/runtime/offload")
    PHYS=--lower-systolic-matmul-tile-to-fpga
    FLEET=--systolic-declare-fpga-fleet ;;
  board)
    : "${SYSTOLIC_FPGA_CONFIG:?BACKEND=board needs SYSTOLIC_FPGA_CONFIG}"
    cfg="$("$ROOT/build/bin/runtime-dispatch-test" --print-config)"
    k8="$(sed -n 's/^K_MAX_8X8=//p' <<< "$cfg")"
    k4="$(sed -n 's/^K_MAX_4X4=//p' <<< "$cfg")"
    depth="$(sed -n 's/^PIPELINE_DEPTH=//p' <<< "$cfg")"
    [[ "$depth" =~ ^[1-9][0-9]*$ ]] || depth=1
    RUNTIME=("$ROOT/runtime/offload/board_stats.c" -I"$ROOT/runtime" -I"$ROOT/runtime/offload"
             "$ROOT/build/runtime/libsystolic_fpga_runtime.a"
             $(pkg-config --libs libftdi1) -lstdc++)
    PHYS="--lower-systolic-matmul-tile-to-fpga=phys-k-max-8x8=$k8 phys-k-max-4x4=$k4"
    FLEET="--systolic-declare-fpga-fleet=k-max-8x8=$k8 k-max-4x4=$k4 pipeline-depth=$depth" ;;
  *) echo "BACKEND must be soft or board" >&2; exit 2 ;;
esac

set -x
"$SYSTOLIC_OPT" "$OUT/20-affine.mlir" -o /dev/null
"$SYSTOLIC_OPT" "$FLEET" "--affine-matmul-to-systolic=report=true" \
  "$OUT/20-affine.mlir" -o "$OUT/40-systolic.mlir"
"$SYSTOLIC_OPT" "$PHYS" "$OUT/40-systolic.mlir" -o "$OUT/50-fpga.mlir"
"$SYSTOLIC_OPT" --systolic-runtime-to-llvm "$OUT/50-fpga.mlir" \
  -o "$OUT/60-llvm.mlir"
"$LLVM_BUILD/bin/mlir-translate" --mlir-to-llvmir "$OUT/60-llvm.mlir" \
  -o "$OUT/70-kernel.ll"
"$LLVM_BUILD/bin/llc" -O0 -filetype=obj -relocation-model=pic \
  "$OUT/70-kernel.ll" -o "$OUT/70-kernel.o"
"$CLANG" -std=c11 -O0 "$HERE/strided_main.c" "$OUT/70-kernel.o" \
  "${RUNTIME[@]}" -L"$LLVM_BUILD/lib" -lmlir_c_runner_utils \
  -Wl,-rpath,"$LLVM_BUILD/lib" -o "$OUT/80-strided-test"
"$OUT/80-strided-test" | tee "$OUT/90-run.txt"
