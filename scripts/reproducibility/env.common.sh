# env.common.sh -- the compiler / runtime environment shared by the board and
# the co-simulation (REPRODUCE.md section 4).
#
# Sourced by env.board.sh and env.cosim.sh, which the run_*.sh scripts source
# in their own process; nothing here touches ~/.bashrc, ~/.profile or the
# terminal that started them. It works from an empty environment (env -i):
# every value comes from the repository, the build directory's CMakeCache.txt,
# or variables the caller names explicitly -- never from a developer's
# exported shell state.
#
# Exported (each one a variable the repository's tools already read):
#   SYSTOLIC_BUILD         the CMake build directory systolic-cc uses
#                          (default <repo>/build)
#   LLVM_BUILD             the LLVM/MLIR 24 build systolic-cc takes
#                          mlir-translate, llc and libmlir_c_runner_utils from;
#                          read from LLVM_DIR in SYSTOLIC_BUILD/CMakeCache.txt
#   SYSTOLIC_CFRONT_CLANG  the clang 24 of the C front end; read from
#                          SYSTOLIC_CFRONT_CLANG in SYSTOLIC_BUILD/CMakeCache.txt
#   PATH                   /usr/local/bin:/usr/bin:/bin plus the bin/ of the
#                          tools above; directories of other LLVM releases
#                          (llvm-18 and the like) are dropped
# Unset: LD_PRELOAD, LD_LIBRARY_PATH, SYSTOLIC_DPTI_COSIM_SOCKET,
# SYSTOLIC_FPGA_CONFIG (the programs need no LD_LIBRARY_PATH: libftdi1 is a
# system library and libmlir_c_runner_utils is linked with an rpath) -- the
# board / co-simulation file sets exactly the ones it needs.
#
# A value given by the caller (SYSTOLIC_BUILD, LLVM_BUILD,
# SYSTOLIC_CFRONT_CLANG on the command line) wins over CMakeCache.txt.

# The repository root, from this file's location.
REPRO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

: "${SYSTOLIC_BUILD:=$REPRO_ROOT/build}"
case "$SYSTOLIC_BUILD" in /*) ;; *) SYSTOLIC_BUILD="$REPRO_ROOT/$SYSTOLIC_BUILD" ;; esac
export SYSTOLIC_BUILD

# KEY:TYPE=VALUE of the build's CMakeCache.txt, or nothing.
repro_cmake_cache() {
    [ -f "$SYSTOLIC_BUILD/CMakeCache.txt" ] || return 0
    sed -n "s/^$1:[A-Z]*=//p" "$SYSTOLIC_BUILD/CMakeCache.txt" | head -1
}

if [ -z "${LLVM_BUILD:-}" ]; then
    llvm_dir="$(repro_cmake_cache LLVM_DIR)"
    case "$llvm_dir" in
        */lib/cmake/llvm) LLVM_BUILD="${llvm_dir%/lib/cmake/llvm}" ;;
    esac
    unset llvm_dir
fi
[ -n "${LLVM_BUILD:-}" ] && export LLVM_BUILD

if [ -z "${SYSTOLIC_CFRONT_CLANG:-}" ]; then
    SYSTOLIC_CFRONT_CLANG="$(repro_cmake_cache SYSTOLIC_CFRONT_CLANG)"
fi
if [ -n "${SYSTOLIC_CFRONT_CLANG:-}" ]; then export SYSTOLIC_CFRONT_CLANG; else unset SYSTOLIC_CFRONT_CLANG; fi

# PATH: the system directories, then this toolchain. A PATH inherited from a
# developer's shell (other LLVM releases, vendor tool bins) is not used.
PATH="/usr/local/bin:/usr/bin:/bin"
[ -n "${LLVM_BUILD:-}" ] && PATH="$LLVM_BUILD/bin:$PATH"
PATH="$SYSTOLIC_BUILD/bin:$PATH"
export PATH

unset LD_PRELOAD LD_LIBRARY_PATH SYSTOLIC_DPTI_COSIM_SOCKET SYSTOLIC_FPGA_CONFIG

SYSTOLIC_CC="$SYSTOLIC_BUILD/bin/systolic-cc"

# repro_need_build: stop with a message when systolic-cc is not built yet.
repro_need_build() {
    if [ ! -x "$SYSTOLIC_CC" ] || [ ! -x "$SYSTOLIC_BUILD/bin/runtime-dispatch-test" ]; then
        echo "no systolic-cc in $SYSTOLIC_BUILD: run scripts/reproducibility/build.sh first" >&2
        return 2
    fi
    if [ -z "${LLVM_BUILD:-}" ] || [ -z "${SYSTOLIC_CFRONT_CLANG:-}" ]; then
        echo "$SYSTOLIC_BUILD/CMakeCache.txt names no LLVM_DIR / SYSTOLIC_CFRONT_CLANG: reconfigure with scripts/reproducibility/build.sh --llvm-build DIR --clang CLANG" >&2
        return 2
    fi
}
