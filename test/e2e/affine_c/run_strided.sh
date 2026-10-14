#!/usr/bin/env bash
#
# run_strided.sh -- strided_permuted.mlir (loops k, i, j; operands at offsets
# and strides inside larger memrefs) through the same compiler stages as
# run.sh, linked with the software runtime model (NOT the FPGA) and checked
# element by element against the same loops in C (strided_main.c).
#
# Artifacts: build/test-output/affine_c_gemm/strided_permuted (OUT=
# overrides). Exit 0 on PASS. Environment as for run.sh (LLVM_BUILD, CLANG).

set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/../../.." && pwd)"
LLVM_BUILD="${LLVM_BUILD:-$HOME/work/llvm-project/build}"
CLANG="${CLANG:-$(command -v clang-18 || command -v clang)}"
SYSTOLIC_OPT="$ROOT/build/bin/systolic-opt"
OUT="${OUT:-$ROOT/build/test-output/affine_c_gemm/strided_permuted}"
mkdir -p "$OUT"

set -x
"$SYSTOLIC_OPT" --systolic-declare-fpga-fleet \
  "--affine-matmul-to-systolic=report=true" "$HERE/strided_permuted.mlir" \
  -o "$OUT/40-systolic.mlir"
"$SYSTOLIC_OPT" --lower-systolic-matmul-tile-to-fpga "$OUT/40-systolic.mlir" \
  -o "$OUT/50-fpga.mlir"
"$SYSTOLIC_OPT" --systolic-runtime-to-llvm "$OUT/50-fpga.mlir" \
  -o "$OUT/60-llvm.mlir"
"$LLVM_BUILD/bin/mlir-translate" --mlir-to-llvmir "$OUT/60-llvm.mlir" \
  -o "$OUT/70-kernel.ll"
"$LLVM_BUILD/bin/llc" -O0 -filetype=obj -relocation-model=pic \
  "$OUT/70-kernel.ll" -o "$OUT/70-kernel.o"
"$CLANG" -std=c11 -O0 "$HERE/strided_main.c" "$HERE/soft_runtime.c" \
  "$OUT/70-kernel.o" -L"$LLVM_BUILD/lib" -lmlir_c_runner_utils \
  -Wl,-rpath,"$LLVM_BUILD/lib" -o "$OUT/80-strided-test"
"$OUT/80-strided-test" | tee "$OUT/90-run.txt"
