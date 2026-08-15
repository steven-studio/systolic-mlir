#!/usr/bin/env bash
# e2e_build_new.sh -- MLIR -> executable for the 8x8xK offload path.
#
# The repository never checked this pipeline in: README defers to "論文
# Implementation 章節", and gen_conv2d_sweep.py has a literal
# "<rest of your existing Section 4.3 toolchain>" placeholder. So this is a
# reconstruction, and the pass list below is the part most likely to need
# adjusting for your exact MLIR 18 build.
#
# Each stage writes its output to a separate file rather than piping, so a
# failure leaves the last good IR on disk to look at. Piping would make the
# whole thing fail as one opaque unit.
#
# Usage:  ./e2e_build_new.sh sim     # host CPU backend
#         ./e2e_build_new.sh uart    # drives the board

set -euo pipefail

BACKEND="${1:-sim}"
ROOT="${ROOT:-$HOME/systolic-mlir}"
OPT="$ROOT/build/bin/systolic-opt"
WORK="${WORK:-/tmp/e2e}"

MLIR_OPT="${MLIR_OPT:-mlir-opt-18}"
MLIR_TRANSLATE="${MLIR_TRANSLATE:-mlir-translate-18}"

# Fall back to unsuffixed names if the -18 ones are not on PATH.
command -v "$MLIR_OPT"       >/dev/null 2>&1 || MLIR_OPT=mlir-opt
command -v "$MLIR_TRANSLATE" >/dev/null 2>&1 || MLIR_TRANSLATE=mlir-translate

mkdir -p "$WORK"

echo "=== [1/5] systolic-opt --tile-matmul-for-fpga"
"$OPT" --tile-matmul-for-fpga \
    "$ROOT/runtime/e2e_gemm_new.mlir" \
    -o "$WORK/01_tiled.mlir"

# Bufferization has to cross the function boundary: the entry point still
# takes and returns tensors, and nothing downstream can lower a tensor.
# identity-layout-map keeps the resulting memrefs plain and contiguous, which
# is what the C driver's descriptor assumes -- a strided layout map here
# would still be correct MLIR but would not match the struct on the C side.
echo "=== [2/5] mlir-opt: bufferize"
"$MLIR_OPT" \
    --one-shot-bufferize="bufferize-function-boundaries function-boundary-type-conversion=identity-layout-map" \
    --canonicalize \
    "$WORK/01_tiled.mlir" \
    -o "$WORK/02_bufferized.mlir"

# scf -> cf must happen before the memref/arith/func conversions, since those
# only know how to rewrite ops inside plain blocks.
echo "=== [3/5] mlir-opt: lower to LLVM dialect"
"$MLIR_OPT" \
    --convert-scf-to-cf \
    --expand-strided-metadata \
    --finalize-memref-to-llvm \
    --convert-arith-to-llvm \
    --convert-cf-to-llvm \
    --convert-func-to-llvm \
    --reconcile-unrealized-casts \
    "$WORK/02_bufferized.mlir" \
    -o "$WORK/03_llvm.mlir"

echo "=== [4/5] mlir-translate --mlir-to-llvmir"
"$MLIR_TRANSLATE" --mlir-to-llvmir \
    "$WORK/03_llvm.mlir" \
    -o "$WORK/04_gemm.ll"

echo "=== [5/5] compile and link ($BACKEND backend)"
cd "$ROOT/runtime"
make "DISPATCH=$BACKEND" >/dev/null

clang -O2 -ffp-contract=off -c "$WORK/04_gemm.ll" -o "$WORK/gemm.o"
clang -O2 -ffp-contract=off \
    e2e_main_new.c "$WORK/gemm.o" lib/libfpgart.a \
    -o "$WORK/e2e_$BACKEND"

echo
echo "built: $WORK/e2e_$BACKEND"
