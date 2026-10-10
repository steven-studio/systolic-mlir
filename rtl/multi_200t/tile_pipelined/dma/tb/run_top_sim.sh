#!/usr/bin/env bash
#
# run_top_sim.sh -- the whole of systolic_dma_top, end to end, in simulation.
#
#   ./run_top_sim.sh              # v1 operand path (four cycles per beat), K_MAX=16
#   ./run_top_sim.sh v2           # beat-wide v2 (USE_V2=1)
#   ./run_top_sim.sh v1 256       # the paper's geometry, either variant
#   ./run_top_sim.sh v2 32 8      # eight invocations of k=32: the k_max split
#
#   ./run_top_sim.sh v1 16 1 1 32 16 hetero_concurrent
#                                 # heterogeneous concurrent-execution test:
#                                 # one 8x8 job (device 0) + one 4x4 job
#                                 # (device 1) back to back through the
#                                 # external scheduler; PASS only if the two
#                                 # arrays' COMPUTE intervals overlap in time
#
# The seventh argument selects the test: "regress" (default -- the existing
# flow, unchanged) or "hetero_concurrent".  It is a run-time plusarg to the
# same bench; the build is identical.
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
ROOT="$(dirname "$DMA")"                               # .../tile_pipelined
VARIANT="${1:-v1}"
KMAX="${2:-16}"
NINV="${3:-1}"
SCHED="${4:-0}"
KMAX_8X8="${5:-32}"
KMAX_4X4="${6:-16}"
TEST="${7:-regress}"
case "$VARIANT" in
  v1) GEN="-GUSE_V2=0 -GK_MAX_8X8=$KMAX_8X8 -GK_MAX_4X4=$KMAX_4X4 -GK_DIM=$KMAX" ;;
  v2) GEN="-GUSE_V2=1 -GK_MAX_8X8=$KMAX_8X8 -GK_MAX_4X4=$KMAX_4X4 -GK_DIM=$KMAX" ;;
  *)  echo "usage: $0 [v1|v2] [K_MAX] [NINV] [SCHED] [K_MAX_8X8] [K_MAX_4X4] [regress|hetero_concurrent]"; exit 2 ;;
esac

case "$SCHED" in
  0|1) ;;
  *)  echo "SCHED must be 0 or 1"; exit 2 ;;
esac

# The test selector: the default adds nothing, so the regression invocations
# run exactly as before.  hetero_concurrent needs the external scheduler.
case "$TEST" in
  regress)           TEST_ARGS="" ;;
  hetero_concurrent) TEST_ARGS="+hetero_concurrent"
                     [ "$SCHED" -eq 1 ] || { echo "hetero_concurrent requires SCHED=1"; exit 2; } ;;
  *)  echo "TEST must be regress or hetero_concurrent"; exit 2 ;;
esac

GEN="$GEN -GUSE_EXTERNAL_SCHEDULER=$SCHED"

OUT="$HERE/sim_out_top_${VARIANT}_k${KMAX}_n${NINV}_sched${SCHED}"
[ "$TEST" = "regress" ] || OUT="${OUT}_${TEST}"

command -v verilator >/dev/null 2>&1 || {
    echo "verilator not found."
    echo "  brew install verilator   /   apt install verilator"
    exit 127
}

rm -rf "$OUT"

echo "== build: variant=$VARIANT K_MAX=$KMAX n_inv=$NINV external_scheduler=$SCHED K_MAX_8X8=$KMAX_8X8 K_MAX_4X4=$KMAX_4X4 test=$TEST =="

verilator --binary -Wno-fatal --timing --public-flat-rw $GEN \
    --top-module tb_systolic_dma_top -o tbrun --Mdir "$OUT" \
    "$HERE/tb_systolic_dma_top.sv" \
    "$HERE/xil_stubs.sv" \
    "$HERE/mig_axi_model.sv" \
    "$ROOT/tb/fp_model.sv" \
    "$DMA/systolic_dma_top.sv" \
    "$DMA/systolic_accel_context.sv" \
    "$DMA/dpti_host_rx.sv" \
    "$DMA/dpti_byte_tx.sv" \
    "$DMA/dpti_write32_decoder.sv" \
    "$DMA/dpti_axi4lite_master.sv" \
    "$DMA/dpti_byte_rx.sv" \
    "$DMA/dpti_command_frontend.sv" \
    "$DMA/dpti_output_cdc.sv" \
    "$DMA/dpti_mem_write_decoder.sv" \
    "$DMA/dpti_descriptor_bridge.sv" \
    "$DMA/dpti_beat_to_byte.sv" \
    "$DMA/dpti_async_fifo.sv" \
    "$DMA/axi4lite_dpti_bridge.sv" \
    "$DMA/dpti_mem_write_cdc.sv" \
    "$DMA/dpti_mem_write_cdc_engine.sv" \
    "$DMA/dpti_mem_write_engine.sv" \
    "$DMA/dpti_cmd_async_fifo.sv" \
    "$ROOT/scheduler/systolic_hw_scheduler.sv" \
    "$ROOT/scheduler/systolic_hw_scheduler_adapter.sv" \
    "$ROOT/scheduler/systolic_job_ingress.sv" \
    "$DMA/dma_engine.sv" \
    "$DMA/dma_engine_multi.sv" \
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
    > "$HERE/build_top_${VARIANT}_k${KMAX}.log" 2>&1 || { tail -30 "$HERE/build_top_${VARIANT}_k${KMAX}.log"; exit 1; }

if [ "$SCHED" -eq 1 ]; then
    WB_REGION=4096
else
    WB_REGION=$((NINV * KMAX * 8 * 8 + 4096))
fi

"$OUT/tbrun" +n_inv=$NINV +wb_region=$WB_REGION $TEST_ARGS
