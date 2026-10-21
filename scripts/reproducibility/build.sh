#!/usr/bin/env bash
#
# build.sh -- configure (once) and build systolic-cc, systolic-opt, the FPGA
# runtime and the test tools; optionally the RTL co-simulation.
#
#   bash scripts/reproducibility/build.sh --llvm-build DIR --clang CLANG   first time
#   bash scripts/reproducibility/build.sh                                  afterwards
#   bash scripts/reproducibility/build.sh --cosim                          + Vcosim
#
#   --llvm-build DIR  the LLVM/MLIR 24 build (or install) tree: the one with
#                     lib/cmake/llvm, lib/cmake/mlir, bin/mlir-translate,
#                     bin/llc. Needed only when SYSTOLIC_BUILD has no
#                     CMakeCache.txt yet (or to change it); LLVM_BUILD=DIR on
#                     the command line does the same.
#   --clang CLANG     clang 24, the C front end systolic-cc runs
#                     (-DSYSTOLIC_CFRONT_CLANG); SYSTOLIC_CFRONT_CLANG=CLANG
#                     on the command line does the same.
#   --cosim           also build the Verilator co-simulation of the working
#                     tree's RTL (test/e2e/cosim/build.sh) into
#                     COSIM_BUILD_DIR (default SYSTOLIC_BUILD/cosim)
#   --jobs N          parallel jobs (default: nproc)
#
# SYSTOLIC_BUILD=DIR selects the build directory (default <repo>/build).
# Nothing outside SYSTOLIC_BUILD is written.
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ARG_LLVM=""
ARG_CLANG=""
COSIM=0
JOBS="$(nproc 2>/dev/null || echo 2)"
while [ $# -gt 0 ]; do
    case "$1" in
        --llvm-build) ARG_LLVM="$2"; shift 2 ;;
        --clang)      ARG_CLANG="$2"; shift 2 ;;
        --cosim)      COSIM=1; shift ;;
        --jobs)       JOBS="$2"; shift 2 ;;
        -h|--help)    sed -n '3,25p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
        *) echo "unknown argument: $1" >&2; exit 2 ;;
    esac
done
[ -n "$ARG_LLVM" ] && export LLVM_BUILD="$ARG_LLVM"
[ -n "$ARG_CLANG" ] && export SYSTOLIC_CFRONT_CLANG="$ARG_CLANG"

. "$HERE/env.common.sh"

for tool in cmake ninja gcc g++ pkg-config; do
    command -v "$tool" >/dev/null 2>&1 || { echo "missing system tool: $tool (see REPRODUCIBILITY.md section 2)" >&2; exit 127; }
done
pkg-config --exists libftdi1 || { echo "missing system library: libftdi1 (pkg-config libftdi1; Debian/Ubuntu: libftdi1-dev)" >&2; exit 127; }

if [ -z "${LLVM_BUILD:-}" ] || [ -z "${SYSTOLIC_CFRONT_CLANG:-}" ]; then
    echo "no LLVM 24 build / clang 24 known for $SYSTOLIC_BUILD: give --llvm-build DIR --clang CLANG" >&2
    exit 2
fi
[ -f "$LLVM_BUILD/lib/cmake/llvm/LLVMConfig.cmake" ] && [ -f "$LLVM_BUILD/lib/cmake/mlir/MLIRConfig.cmake" ] || {
    echo "$LLVM_BUILD has no lib/cmake/llvm/LLVMConfig.cmake + lib/cmake/mlir/MLIRConfig.cmake" >&2; exit 2; }
llvm_version="$("$LLVM_BUILD/bin/llvm-config" --version 2>/dev/null || true)"
case "$llvm_version" in
    24.*) ;;
    *) echo "$LLVM_BUILD is LLVM '$llvm_version'; this tree needs LLVM/MLIR 24" >&2; exit 2 ;;
esac
clang_version="$("$SYSTOLIC_CFRONT_CLANG" --version 2>/dev/null | head -1 || true)"
case "$clang_version" in
    *"clang version 24."*) ;;
    *) echo "$SYSTOLIC_CFRONT_CLANG is '$clang_version'; systolic-cc needs clang 24" >&2; exit 2 ;;
esac

configured_llvm="$(repro_cmake_cache LLVM_DIR)"
configured_clang="$(repro_cmake_cache SYSTOLIC_CFRONT_CLANG)"
if [ ! -f "$SYSTOLIC_BUILD/build.ninja" ] ||
   [ "$configured_llvm" != "$LLVM_BUILD/lib/cmake/llvm" ] ||
   [ "$configured_clang" != "$SYSTOLIC_CFRONT_CLANG" ]; then
    echo "== configure $SYSTOLIC_BUILD (LLVM $llvm_version at $LLVM_BUILD, $SYSTOLIC_CFRONT_CLANG)"
    cmake -G Ninja -S "$REPRO_ROOT" -B "$SYSTOLIC_BUILD" \
        -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ \
        -DLLVM_DIR="$LLVM_BUILD/lib/cmake/llvm" \
        -DMLIR_DIR="$LLVM_BUILD/lib/cmake/mlir" \
        -DSYSTOLIC_CFRONT_CLANG="$SYSTOLIC_CFRONT_CLANG"
fi

echo "== build $SYSTOLIC_BUILD"
cmake --build "$SYSTOLIC_BUILD" -j "$JOBS"
for b in systolic-cc systolic-cfront systolic-opt systolic-matmul runtime-dispatch-test runtime-async-test; do
    [ -x "$SYSTOLIC_BUILD/bin/$b" ] || { echo "missing after the build: $SYSTOLIC_BUILD/bin/$b" >&2; exit 1; }
done
[ -f "$SYSTOLIC_BUILD/runtime/libsystolic_fpga_runtime.a" ] || { echo "missing after the build: $SYSTOLIC_BUILD/runtime/libsystolic_fpga_runtime.a" >&2; exit 1; }
echo "built: $SYSTOLIC_BUILD/bin/systolic-cc and the FPGA runtime"

if [ "$COSIM" = 1 ]; then
    . "$HERE/env.cosim.sh"
    echo "== co-simulation -> $COSIM_BUILD_DIR"
    JOBS="$JOBS" bash "$REPRO_ROOT/test/e2e/cosim/build.sh"
fi
