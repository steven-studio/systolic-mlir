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
#   verify-program        which .bit 'program' would load, and whether it is
#                         the artifact's (sha256); programs nothing
#   program               program the FPGA with dma_top_build.tcl's 'program'
#                         mode -- only after verify-program found the
#                         artifact's hash; needs Vivado (PATH or XILINX_VIVADO)
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

TCL="rtl/multi_200t/tile_pipelined/dma/dma_top_build.tcl"
PROGRAM_ARGS=(program v1 16 16 1 1 3)

# The .bit dma_top_build.tcl's 'program v1 16 16 1 1 3' opens: the first
# *.bit of the impl_1 run of the project directory it computes from
# HOME, the geometry and SYSTOLIC_PROJ_TAG (dma_top_build.tcl, PROJ_DIR and
# proc program).
program_source() {
    local proj="${HOME:?HOME is needed to find the Vivado project}/work/vivado/tile_pipelined/systolic_dma_v1_k8_16_k4_16_1x8x8_3x4x4"
    [ -n "${SYSTOLIC_PROJ_TAG:-}" ] && proj="${proj}_${SYSTOLIC_PROJ_TAG}"
    echo "$proj"
}

verify_program() {
    local proj bit got
    proj="$(program_source)"
    echo "dma_top_build.tcl ${PROGRAM_ARGS[*]}${SYSTOLIC_PROJ_TAG:+  (SYSTOLIC_PROJ_TAG=$SYSTOLIC_PROJ_TAG)}"
    echo "  project: $proj"
    bit="$(ls "$proj"/systolic_dma.runs/impl_1/*.bit 2>/dev/null | head -1)"
    if [ -z "$bit" ]; then
        echo "  NOT VERIFIED: no Vivado project with a bitstream there; the Tcl script can only program"
        echo "  a .bit from its own project, not the artifact file (REPRODUCIBILITY.md section 7)"
        return 2
    fi
    got="$(sha256sum "$bit" | cut -d' ' -f1)"
    echo "  loads:   $bit"
    echo "  sha256:  $got"
    echo "  wanted:  $REPRO_BOARD_SHA256 ($REPRO_BOARD_BIT)"
    if [ "$got" = "$REPRO_BOARD_SHA256" ]; then
        echo "  MATCH"
        return 0
    fi
    echo "  MISMATCH: this command would program a different bitstream"
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
        cd "$REPRO_ROOT" && vivado -mode batch -source "$TCL" -tclargs "${PROGRAM_ARGS[@]}" ;;
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
