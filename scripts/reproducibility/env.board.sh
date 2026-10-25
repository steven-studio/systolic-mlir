# env.board.sh -- environment of a run on the Nexys Video board (Board Mode).
#
# Sourced by run_board.sh / check_env.sh in their own process. On top of
# env.common.sh it sets exactly:
#   SYSTOLIC_FPGA_CONFIG   the fpga_config.txt archived with the one bitstream
#                          this reproduction uses (repository-relative, see
#                          REPRO_BOARD_ARTIFACT); the runtime, systolic-cc and
#                          run_e2e.sh read K_MAX_8X8 / K_MAX_4X4 / slab /
#                          COMPLETION_HEADER / PIPELINE_DEPTH from it
# and makes sure the co-simulation's libftdi stand-in is NOT loaded
# (LD_PRELOAD and SYSTOLIC_DPTI_COSIM_SOCKET unset by env.common.sh), so the
# programs link the system libftdi1 and talk to the FT2232H over USB.
#
# Vivado (only for 'run_board.sh program'): taken from PATH, else from
# XILINX_VIVADO (the variable Vivado's settings64.sh sets), e.g.
#   XILINX_VIVADO=/opt/Xilinx/2026.1/Vivado bash scripts/reproducibility/run_board.sh program

. "$(dirname "${BASH_SOURCE[0]}")/env.common.sh"

REPRO_BOARD_ARTIFACT="artifacts/fpga/tile_pipelined/1x8x8-3x4x4/2026-10-25-k8_16-k4_16"
REPRO_BOARD_BIT="$REPRO_BOARD_ARTIFACT/systolic_dma_top_1x8x8_3x4x4.bit"
REPRO_BOARD_SHA256="bd8aba7212a24347eba1dc497204dd6eb5117e8a8ff77094fd94c496a41f3db5"

export SYSTOLIC_FPGA_CONFIG="$REPRO_ROOT/$REPRO_BOARD_ARTIFACT/fpga_config.txt"

if ! command -v vivado >/dev/null 2>&1 && [ -n "${XILINX_VIVADO:-}" ] && [ -x "$XILINX_VIVADO/bin/vivado" ]; then
    PATH="$PATH:$XILINX_VIVADO/bin"
fi
