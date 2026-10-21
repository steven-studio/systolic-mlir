#!/usr/bin/env bash
#
# check_env.sh [board|cosim|all] -- what a reproduction on this machine needs,
# checked from the environment the run scripts build (env.board.sh /
# env.cosim.sh), not from the caller's shell.
#
#   bash scripts/reproducibility/check_env.sh board
#   bash scripts/reproducibility/check_env.sh cosim
#   bash scripts/reproducibility/check_env.sh          (all: both)
#
# Each line is one requirement:
#   OK        present and usable
#   MISSING   a system package / tool that is not installed (not an
#             environment-variable problem: install it)
#   NOT BUILT a product of scripts/reproducibility/build.sh that is not there yet
#   FAIL      present but wrong (version, hash, refused configuration)
#   INFO      optional, or a fact this script cannot check
# Exit status: 0 everything required for the chosen mode is OK, 1 otherwise.
set -uo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MODE="${1:-all}"
case "$MODE" in board|cosim|all) ;; *) echo "usage: $0 [board|cosim|all]" >&2; exit 2 ;; esac

# What the caller's shell had exported, before the scripts replace it.
INHERITED=""
for v in SYSTOLIC_FPGA_CONFIG SYSTOLIC_CFRONT_CLANG SYSTOLIC_BUILD LLVM_BUILD LD_PRELOAD LD_LIBRARY_PATH SYSTOLIC_DPTI_COSIM_SOCKET COSIM_BUILD_DIR; do
    [ -n "${!v:-}" ] && INHERITED="$INHERITED $v"
done

status=0
ok()       { printf '  OK         %s\n' "$*"; }
missing()  { printf '  MISSING    %s\n' "$*"; status=1; }
notbuilt() { printf '  NOT BUILT  %s\n' "$*"; status=1; }
fail()     { printf '  FAIL       %s\n' "$*"; status=1; }
info()     { printf '  INFO       %s\n' "$*"; }

tool() {   # tool NAME [what for]
    if command -v "$1" >/dev/null 2>&1; then ok "$1: $(command -v "$1")"; else missing "$1${2:+ ($2)}"; fi
}

common_checks() {
    echo "== environment (env.common.sh)"
    if [ -n "$INHERITED" ]; then
        info "set by the caller:$INHERITED (SYSTOLIC_BUILD / LLVM_BUILD / SYSTOLIC_CFRONT_CLANG / COSIM_BUILD_DIR are honoured; the others are replaced or unset)"
    else
        info "nothing systolic-specific inherited from the caller"
    fi
    info "SYSTOLIC_BUILD=$SYSTOLIC_BUILD"
    info "LLVM_BUILD=${LLVM_BUILD:-<unknown>}"
    info "SYSTOLIC_CFRONT_CLANG=${SYSTOLIC_CFRONT_CLANG:-<unknown>}"
    info "PATH=$PATH"

    echo "== system tools and libraries"
    for t in bash cmake ninja gcc g++ pkg-config python3 sha256sum git sed awk; do tool "$t"; done
    if pkg-config --exists libftdi1 2>/dev/null; then
        ok "libftdi1 $(pkg-config --modversion libftdi1) (pkg-config: $(pkg-config --libs libftdi1))"
    else
        missing "libftdi1 development files (pkg-config libftdi1; Debian/Ubuntu: libftdi1-dev)"
    fi

    echo "== LLVM/MLIR 24 and clang 24 (from $SYSTOLIC_BUILD/CMakeCache.txt)"
    if [ -z "${LLVM_BUILD:-}" ]; then
        notbuilt "LLVM_BUILD unknown: configure with scripts/reproducibility/build.sh --llvm-build DIR --clang CLANG"
    else
        v="$("$LLVM_BUILD/bin/llvm-config" --version 2>/dev/null || true)"
        case "$v" in 24.*) ok "LLVM $v at $LLVM_BUILD" ;; '') missing "$LLVM_BUILD/bin/llvm-config" ;; *) fail "LLVM $v at $LLVM_BUILD (need 24)" ;; esac
        for b in mlir-translate llc; do
            [ -x "$LLVM_BUILD/bin/$b" ] && ok "$LLVM_BUILD/bin/$b" || missing "$LLVM_BUILD/bin/$b"
        done
        ls "$LLVM_BUILD"/lib/libmlir_c_runner_utils.so* >/dev/null 2>&1 \
            && ok "$LLVM_BUILD/lib/libmlir_c_runner_utils.so (linked with an rpath; no LD_LIBRARY_PATH needed)" \
            || missing "$LLVM_BUILD/lib/libmlir_c_runner_utils.so"
    fi
    if [ -z "${SYSTOLIC_CFRONT_CLANG:-}" ]; then
        notbuilt "SYSTOLIC_CFRONT_CLANG unknown: configure with scripts/reproducibility/build.sh --clang CLANG"
    else
        v="$("$SYSTOLIC_CFRONT_CLANG" --version 2>/dev/null | head -1 || true)"
        case "$v" in *"clang version 24."*) ok "$v" ;; '') missing "$SYSTOLIC_CFRONT_CLANG" ;; *) fail "$v (need clang 24)" ;; esac
    fi

    echo "== systolic-mlir build ($SYSTOLIC_BUILD)"
    for b in systolic-cc systolic-cfront systolic-opt systolic-matmul runtime-dispatch-test runtime-async-test; do
        [ -x "$SYSTOLIC_BUILD/bin/$b" ] && ok "bin/$b" || notbuilt "bin/$b (scripts/reproducibility/build.sh)"
    done
    [ -f "$SYSTOLIC_BUILD/runtime/libsystolic_fpga_runtime.a" ] && ok "runtime/libsystolic_fpga_runtime.a" \
        || notbuilt "runtime/libsystolic_fpga_runtime.a (scripts/reproducibility/build.sh)"
}

config_check() {   # the runtime's own parser on SYSTOLIC_FPGA_CONFIG
    if [ ! -f "$SYSTOLIC_FPGA_CONFIG" ]; then
        notbuilt "$SYSTOLIC_FPGA_CONFIG"
        return
    fi
    if [ ! -x "$SYSTOLIC_BUILD/bin/runtime-dispatch-test" ]; then
        info "runtime-dispatch-test not built: the configuration was not parsed"
        return
    fi
    out="$("$SYSTOLIC_BUILD/bin/runtime-dispatch-test" --print-config 2>&1)"
    if [ $? -eq 0 ]; then
        ok "the runtime accepts SYSTOLIC_FPGA_CONFIG: $(echo "$out" | grep -v '^SOURCE=' | tr '\n' ' ')"
    else
        fail "the runtime refuses SYSTOLIC_FPGA_CONFIG: $out"
    fi
}

board_checks() {
    (
    . "$HERE/env.board.sh"
    echo "== board mode (env.board.sh)"
    info "SYSTOLIC_FPGA_CONFIG=$SYSTOLIC_FPGA_CONFIG"
    config_check
    A="$REPRO_ROOT/$REPRO_BOARD_ARTIFACT"
    if [ -f "$A/SHA256SUMS" ]; then
        files_ok=1
        while read -r sum name; do
            [ -f "$A/$name" ] || continue
            [ "$(sha256sum "$A/$name" | cut -d' ' -f1)" = "$sum" ] || { fail "$REPRO_BOARD_ARTIFACT/$name does not match SHA256SUMS"; files_ok=0; }
        done < "$A/SHA256SUMS"
        [ $files_ok = 1 ] && ok "artifact files present match $REPRO_BOARD_ARTIFACT/SHA256SUMS"
    else
        fail "no $REPRO_BOARD_ARTIFACT/SHA256SUMS"
    fi
    if [ -f "$REPRO_ROOT/$REPRO_BOARD_BIT" ]; then
        got="$(sha256sum "$REPRO_ROOT/$REPRO_BOARD_BIT" | cut -d' ' -f1)"
        [ "$got" = "$REPRO_BOARD_SHA256" ] && ok "bitstream $REPRO_BOARD_BIT sha256 $got" \
            || fail "bitstream $REPRO_BOARD_BIT sha256 $got, expected $REPRO_BOARD_SHA256"
    else
        info "bitstream $REPRO_BOARD_BIT is not in this checkout (*.bit is not committed); only needed to program the board"
    fi
    found=""
    for d in /sys/bus/usb/devices/*; do
        [ -f "$d/idVendor" ] || continue
        [ "$(cat "$d/idVendor")" = 0403 ] && [ "$(cat "$d/idProduct")" = 6010 ] && found="$d"
    done
    if [ -n "$found" ]; then
        ok "FT2232H (0403:6010) on USB: $found ($(cat "$found/product" 2>/dev/null || echo '?'), serial $(cat "$found/serial" 2>/dev/null || echo '?'))"
    else
        missing "FT2232H (0403:6010) on USB: the Nexys Video is not connected or not powered"
    fi
    info "what the FPGA holds is not readable over DPTI; the board run trusts that $REPRO_BOARD_SHA256 was programmed"
    if command -v vivado >/dev/null 2>&1; then
        ok "vivado: $(command -v vivado) (only for 'run_board.sh program')"
    else
        info "vivado not found (PATH or XILINX_VIVADO): only 'run_board.sh program' needs it"
    fi
    exit $status
    ) || status=1
}

cosim_checks() {
    (
    . "$HERE/env.cosim.sh"
    echo "== co-simulation mode (env.cosim.sh)"
    info "SYSTOLIC_FPGA_CONFIG=$SYSTOLIC_FPGA_CONFIG"
    if command -v verilator >/dev/null 2>&1; then
        v="$(verilator --version 2>/dev/null | awk '{print $2}')"
        case "$v" in [5-9].*) ok "verilator $v" ;; *) fail "verilator $v (need >= 5.0)" ;; esac
    else
        missing "verilator >= 5.0 (Debian/Ubuntu: verilator)"
    fi
    for f in Vcosim libftdi_cosim.so; do
        [ -e "$COSIM_BUILD_DIR/$f" ] && ok "$COSIM_BUILD_DIR/$f" || notbuilt "$COSIM_BUILD_DIR/$f (scripts/reproducibility/build.sh --cosim)"
    done
    config_check
    [ -f "$COSIM_BUILD_DIR/RTL_SOURCE" ] && info "simulated RTL: $(cat "$COSIM_BUILD_DIR/RTL_SOURCE")"
    info "no FPGA, USB device or Vivado is needed in this mode"
    exit $status
    ) || status=1
}

. "$HERE/env.common.sh"
common_checks
case "$MODE" in
    board) board_checks ;;
    cosim) cosim_checks ;;
    all)   board_checks; cosim_checks ;;
esac
echo
[ $status = 0 ] && echo "check_env ($MODE): OK" || echo "check_env ($MODE): something required is missing (see above)"
exit $status
