#!/usr/bin/env bash
#
# build_bench.sh -- build tb_fleet_jobs (Verilator) for one bitstream
# configuration.  The RTL is the working tree's rtl/multi_200t/heterogeneous_fleet,
# with the source list and stand-ins of dma/tb/run_cost_phases.sh.
#
#   eval/cost_model_v2/build_bench.sh K_MAX_8X8 K_MAX_4X4 v1|v2 OUT_DIR
#
# Produces OUT_DIR/obj/tbrun.  Run it as  OUT_DIR/obj/tbrun +jobs=FILE +wb_region=4096
set -euo pipefail
[ $# -eq 4 ] || { echo "usage: $0 K_MAX_8X8 K_MAX_4X4 v1|v2 OUT_DIR"; exit 2; }
K8="$1"; K4="$2"; VARIANT="$3"; OUT="$4"
case "$VARIANT" in v1) USE_V2=0 ;; v2) USE_V2=1 ;; *) echo "variant must be v1 or v2"; exit 2 ;; esac
HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/../.." && pwd)"
RTL="$ROOT/rtl/multi_200t/heterogeneous_fleet"
DMA="$RTL/dma"
COSIM="$ROOT/test/e2e/cosim"
command -v verilator >/dev/null 2>&1 || { echo "verilator not found"; exit 127; }
mkdir -p "$OUT"
verilator --binary -Wno-fatal -Werror-USERERROR --timing --public-flat-rw -O2 \
    -j "${JOBS:-6}" \
    -GK_MAX_8X8="$K8" -GK_MAX_4X4="$K4" -GUSE_V2="$USE_V2" \
    -I"$DMA/tb" \
    --top-module tb_fleet_jobs -o tbrun --Mdir "$OUT/obj" \
    "$HERE/tb_fleet_jobs_r2.sv" \
    "$DMA/tb/xil_stubs.sv" \
    "$DMA/tb/mig_axi_model.sv" \
    "$COSIM/fp_ieee.sv" \
    "$DMA/systolic_dma_top.sv" \
    "$DMA/systolic_accel_context.sv" \
    "$DMA/dpti_host_rx.sv" \
    "$DMA/dpti_byte_tx.sv" \
    "$DMA/dpti_bus_arbiter.sv" \
    "$DMA/dpti_sync245_phy.sv" \
    "$DMA/dpti_sync245_fifo.sv" \
    "$DMA/dpti_clk_gen.sv" \
    "$DMA/dpti_link_ctrl.sv" \
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
    "$RTL/scheduler/systolic_hw_scheduler.sv" \
    "$RTL/scheduler/systolic_hw_scheduler_adapter.sv" \
    "$RTL/scheduler/systolic_job_ingress.sv" \
    "$DMA/dma_engine.sv" \
    "$DMA/dma_engine_multi.sv" \
    "$DMA/dma_seed_writer.sv" \
    "$DMA/dma_operand_writer.sv" \
    "$DMA/dma_operand_writer_v2.sv" \
    "$DMA/dma_wr_checksum_v2.sv" \
    "$DMA/dma_cdc_fifo.sv" \
    "$DMA/dma_result_reader.sv" \
    "$DMA/dma_writeback_engine.sv" \
    "$RTL/core/fp_reduce16.sv" \
    "$RTL/core/systolic_pe_bram.sv" \
    "$RTL/core/systolic_array.sv" \
    "$RTL/core/tile_feeder.sv" \
    "$RTL/core/operand_buffer.sv" \
    "$RTL/core/operand_buffer_v2.sv" \
    "$COSIM/dpti_cosim.cpp" \
    > "$OUT/build.log" 2>&1 || { tail -30 "$OUT/build.log"; exit 1; }
echo "built $OUT/obj/tbrun (K_MAX_8X8=$K8 K_MAX_4X4=$K4 $VARIANT)"
