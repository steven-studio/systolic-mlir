#!/usr/bin/env bash
#
# run_top_sim.sh -- the whole of systolic_dma_top, end to end, in simulation.
#
#   ./run_top_sim.sh              # v1 operand path (four cycles per beat), K_MAX=16
#   ./run_top_sim.sh v2           # beat-wide v2 (USE_V2=1)
#   ./run_top_sim.sh v1 256       # the paper's geometry, either variant
#   ./run_top_sim.sh v2 32 8      # eight invocations of k=32: the k_max split
#   ./run_top_sim.sh v1 16 1 external 16
#                                  # external scheduler, K_MAX=16, JOB_K=16
#   ./run_top_sim.sh v1 32 1 external 16
#                                  # K_MAX=32 hardware, runtime JOB_K=16
#   ./run_top_sim.sh v1 32 1 external 32
#                                  # K_MAX=32 hardware, runtime JOB_K=32
#
# The third argument is the invocation count.  It is not a generic: the design
# takes it from vio_0's output probe, which xil_stubs drives from +n_inv, for
# the same reason the board takes it from JTAG -- the fold count is a
# scheduling quantity, and baking it into the hardware is the mistake
# uart/build_kmax.tcl records.
#
# What it runs is the real top with only the Xilinx IP replaced: the MIG by a
# behavioural AXI4 memory that also monitors the write channel, the primitives
# and the VIO by geometry-only stubs, and fp_mul / fp_add by tb/fp_model.sv --
# the same integer model the array's own benches use.
#
# Verilator, not Icarus.  The feeder and the array talk through unpacked array
# ports, and Icarus's always_comb sensitivity inference leaves those at X, so
# the fold never starts and the run hangs in ST_WAIT_RESULT.  That is a
# simulator limitation, not a design fault; the DMA benches in this directory
# stay on Icarus because none of them cross that boundary.
#
#   Verilator: brew install verilator  /  apt install verilator   (>= 5.x)
set -eu

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"   # .../dma/tb
DMA="$(dirname "$HERE")"
ROOT="$(dirname "$DMA")"                               # .../fold_pipelined
VARIANT="${1:-v1}"
KMAX="${2:-16}"
NINV="${3:-1}"

# Runtime reduction length.
#
# KMAX = compiled hardware capacity.
# JOB_K = K of the submitted scheduler job.
#
# Only external-scheduler mode consumes JOB_K.
# Legacy mode defaults it to KMAX and remains unchanged.
JOB_K="${5:-$KMAX}"

# Physical accelerator selected by the test descriptor:
#   0 -> 8x8 #0
#   1 -> 4x4 #0
#   2 -> 4x4 #1
#   3 -> 4x4 #2
TEST_DEVICE_ID="${6:-0}"

SCHEDULER_GEN=""
if [[ "${4:-}" == "external" ]]; then
  SCHEDULER_GEN="-GUSE_EXTERNAL_SCHEDULER=1"
elif [[ -n "${4:-}" ]]; then
  echo "usage: $0 [v1|v2] [K_MAX] [N_INV] [external]"
  exit 2
fi

case "$VARIANT" in
  v1) GEN="-GUSE_V2=0 -GK_MAX=$KMAX" ;;
  v2) GEN="-GUSE_V2=1 -GK_MAX=$KMAX" ;;
  *)  echo "usage: $0 [v1|v2] [K_MAX]"; exit 2 ;;
esac
OUT="$HERE/sim_out_top_${VARIANT}_k${KMAX}_jobk${JOB_K}_n${NINV}"

command -v verilator >/dev/null 2>&1 || {
    echo "verilator not found."
    echo "  brew install verilator   /   apt install verilator"
    exit 127
}

rm -rf "$OUT"
verilator --binary -Wno-fatal --timing --public-flat-rw $GEN $SCHEDULER_GEN \
    -GJOB_K=$JOB_K \
    -GTEST_DEVICE_ID=$TEST_DEVICE_ID \
    -DFP_MUL_LAT=8 -DFP_ADD_LAT=11 \
    --top-module tb_systolic_dma_top_8x8 -o tbrun --Mdir "$OUT" \
    "$HERE/tb_systolic_dma_top_8x8.sv" \
    "$HERE/xil_stubs.sv" \
    "$HERE/mig_axi_model.sv" \
    "$ROOT/tb/fp_model.sv" \
    "$ROOT/tb/fp32_dpi.cpp" \
    "$DMA/systolic_dma_top.sv" \
    "$DMA/dpti_host_rx.sv" \
    "$DMA/dpti_byte_rx.sv" \
    "$DMA/dpti_write32_decoder.sv" \
    "$DMA/dpti_mem_write_decoder.sv" \
    "$DMA/dpti_command_frontend.sv" \
    "$DMA/dpti_async_fifo.sv" \
    "$DMA/dpti_output_cdc.sv" \
    "$DMA/dpti_beat_to_byte.sv" \
    "$DMA/dpti_byte_tx.sv" \
    "$DMA/dpti_mem_write_cdc.sv" \
    "$DMA/dpti_mem_write_engine.sv" \
    "$DMA/dpti_mem_write_cdc_engine.sv" \
    "$DMA/dpti_cmd_async_fifo.sv" \
    "$DMA/dpti_axi4lite_master.sv" \
    "$DMA/axi4lite_dpti_bridge.sv" \
    "$DMA/dpti_descriptor_bridge.sv" \
    "$ROOT/scheduler/systolic_job_ingress.sv" \
    "$ROOT/scheduler/systolic_hw_scheduler.sv" \
    "$ROOT/scheduler/systolic_hw_scheduler_adapter.sv" \
    "$DMA/dma_engine.sv" \
    "$DMA/dma_seed_writer.sv" \
    "$DMA/dma_operand_writer.sv" \
    "$DMA/dma_operand_writer_v2.sv" \
    "$DMA/dma_wr_checksum_v2.sv" \
    "$DMA/dma_cdc_fifo.sv" \
    "$DMA/dma_result_reader.sv" \
    "$DMA/dma_writeback_engine.sv" \
    "$ROOT/core/fp_reduce16.sv" \
    "$ROOT/core/systolic_pe_bram.sv" \
    "$ROOT/core/systolic_array.sv" \
    "$ROOT/core/tile_feeder.sv" \
    "$ROOT/core/operand_buffer.sv" \
    "$ROOT/core/operand_buffer_v2.sv" \
    > "$HERE/build_top_${VARIANT}_k${KMAX}_jobk${JOB_K}.log" 2>&1 || {
    tail -30 "$HERE/build_top_${VARIANT}_k${KMAX}_jobk${JOB_K}.log"
    exit 1
}

if [[ "${4:-}" == "external" ]]; then
  EXTRA_ARGS=("${@:7}")
else
  EXTRA_ARGS=("${@:4}")
fi

if [[ "${4:-}" == "external" ]]; then

  # External scheduler owns the job's C/result region.

  # Keep synchronized with tb_systolic_dma_top.sv job_c_base.

  JOB_C_BASE_RUNTIME=$((0x3000))

  WB_REGION_RUNTIME=$JOB_C_BASE_RUNTIME

else

  # Legacy mode keeps the original run-level result region.

  WB_REGION_RUNTIME=$((NINV * KMAX * 8 * 8 + 4096))

fi



"$OUT/tbrun" +n_inv=$NINV +wb_region=$WB_REGION_RUNTIME "${EXTRA_ARGS[@]}"