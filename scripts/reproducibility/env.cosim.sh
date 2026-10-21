# env.cosim.sh -- environment of a run on the RTL co-simulation (no FPGA).
#
# Sourced by run_cosim.sh / check_env.sh in their own process. On top of
# env.common.sh it sets:
#   SYSTOLIC_FPGA_CONFIG   COSIM_BUILD_DIR/fpga_config.txt, which
#                          test/e2e/cosim/build.sh writes with the simulated
#                          bitstream's parameters (K_MAX 16 / 16, completion
#                          record, PIPELINE_DEPTH 4 -- like the board's -- but
#                          DDR_BYTES = 1 MiB, the DDR model's size, which the
#                          asynchronous runtime fits its buffers to; so the
#                          board's fpga_config.txt is NOT used here)
#   COSIM_BUILD_DIR        where build.sh puts Vcosim, libftdi_cosim.so and
#                          that fpga_config.txt (default SYSTOLIC_BUILD/cosim;
#                          build.sh's own variable)
# LD_PRELOAD=COSIM_BUILD_DIR/libftdi_cosim.so and SYSTOLIC_DPTI_COSIM_SOCKET
# are NOT exported here: run_cosim.sh sets them on the one program it runs
# against a live Vcosim, so no compiler or shell process ever loads the shim.

. "$(dirname "${BASH_SOURCE[0]}")/env.common.sh"

: "${COSIM_BUILD_DIR:=$SYSTOLIC_BUILD/cosim}"
case "$COSIM_BUILD_DIR" in /*) ;; *) COSIM_BUILD_DIR="$REPRO_ROOT/$COSIM_BUILD_DIR" ;; esac
export COSIM_BUILD_DIR
export SYSTOLIC_FPGA_CONFIG="$COSIM_BUILD_DIR/fpga_config.txt"
