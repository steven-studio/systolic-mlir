#!/usr/bin/env bash
#
# run_board.sh COMMAND -- Board Mode: the Nexys Video programmed with the one
# bitstream this reproduction uses,
#   artifacts/fpga/tile_pipelined/1x8x8-3x4x4/2026-10-19-k8_16-k4_16/
#   systolic_dma_top_1x8x8_3x4x4.bit  (1 x 8x8 + 3 x 4x4, K8 = K4 = 16,
#   sha256 18329505a3c4c7961094c44c2b104d289dca30d95fb92bf511c98e4cedab705e)
# in the environment of env.board.sh (works from env -i; the caller's
# exported variables are not used).
#
#   check                 check_env.sh board: tools, build, configuration,
#                         artifact hashes, FT2232H on USB
#   verify-program        the artifact's .bit is there and its sha256 is the
#                         one above; programs nothing
#   program               program the FPGA with exactly that file
#                         (program_bit.tcl) -- only after verify-program
#                         found the hash; needs Vivado (PATH or XILINX_VIVADO)
#   e2e [ARGS...]         test/e2e/run_e2e.sh --backend board (default ARGS:
#                         --quick) with the artifact's fpga_config.txt
#   c FILE.c [OUTDIR]     compile FILE.c with systolic-cc for the board, run
#                         it, report where every GEMM ran (default OUTDIR:
#                         SYSTOLIC_BUILD/repro/board/<name>)
#
# e2e and c open the FT2232H link: run them only when nothing else is using
# the board. Nothing here rebuilds a bitstream.
set -uo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
. "$HERE/env.board.sh"
. "$HERE/c_program.sh"

# 'program' programs the artifact's .bit itself (program_bit.tcl), not the
# .bit of a Vivado project, so it needs no project and no project tag.
PROGRAM_TCL="scripts/reproducibility/program_bit.tcl"

verify_program() {
    local bit="$REPRO_ROOT/$REPRO_BOARD_BIT" got
    echo "program_bit.tcl $REPRO_BOARD_BIT"
    if [ ! -f "$bit" ]; then
        echo "  NOT VERIFIED: $REPRO_BOARD_BIT is not there (*.bit is not in git;"
        echo "  obtain the file and put it there, REPRODUCE.md section 6)"
        return 2
    fi
    got="$(sha256sum "$bit" | cut -d' ' -f1)"
    echo "  sha256:  $got"
    echo "  wanted:  $REPRO_BOARD_SHA256"
    if [ "$got" = "$REPRO_BOARD_SHA256" ]; then
        echo "  MATCH"
        return 0
    fi
    echo "  MISMATCH: that file is not the artifact's bitstream"
    return 1
}

cmd="${1:-}"
[ $# -gt 0 ] && shift
case "$cmd" in
    check)
        exec bash "$HERE/check_env.sh" board ;;
    verify-program)
        verify_program ;;
    program)
        verify_program || { echo "refusing to program: the bitstream is not the artifact's" >&2; exit 1; }
        command -v vivado >/dev/null 2>&1 || { echo "vivado not found: put it on PATH or set XILINX_VIVADO" >&2; exit 127; }
        cd "$REPRO_ROOT" && vivado -mode batch -nojournal -log "${TMPDIR:-/tmp}/systolic_program_bit.log" \
            -source "$PROGRAM_TCL" -tclargs "$REPRO_ROOT/$REPRO_BOARD_BIT" ;;
    e2e)
        repro_need_build || exit 2
        [ $# -gt 0 ] || set -- --quick
        cd "$REPRO_ROOT" && bash test/e2e/run_e2e.sh --backend board --build-dir "$SYSTOLIC_BUILD" "$@" ;;
    c)
        repro_need_build || exit 2
        [ $# -ge 1 ] || { echo "usage: $0 c FILE.c [OUTDIR]" >&2; exit 2; }
        src="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
        out="${2:-$SYSTOLIC_BUILD/repro/board/$(basename "$1" .c)}"
        repro_c_program board "$src" "$out" ;;
    *)
        sed -n '3,25p' "$0" | sed 's/^# \{0,1\}//'
        exit 2 ;;
esac
