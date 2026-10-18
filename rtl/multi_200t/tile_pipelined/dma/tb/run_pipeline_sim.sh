#!/usr/bin/env bash
#
# run_pipeline_sim.sh -- tb_pipeline_overlap.sv for one bitstream configuration:
# consecutive invocations of one accelerator offered as fast as the design
# takes them (or, with +serial=1, one at a time), every stage stamped per job,
# every result checked.  Same top and sources as run_cost_phases.sh (external
# scheduler, only the Xilinx IP replaced, binary32 arithmetic from the
# co-simulation's fp_ieee.sv).
#
#   ./run_pipeline_sim.sh K_MAX_8X8 K_MAX_4X4 JOBS_SVH OUT_DIR [v1|v2] [+plusarg ...]
#
# JOBS_SVH is the job list (tb_cost_phases' format; see tb_pipeline_overlap.sv).
# Plusargs: +serial=1, +min_fill_overlaps=N, +min_feed_overlaps=N,
# +expect_min_gap=G.  Environment: PE_QUIET_GAP=<n> overrides the contexts'
# boundary gap (default 23) -- below the PE's minimum the bench must FAIL
# (merged tiles: fewer publishes than folds), which is the negative control
# eval/pipeline_overlap/run.sh runs.
#
# Output: OUT_DIR/run.log (one "PJOB ..." line per job, "PGEMM ..." per chained
# reduction, "PDEV ..." per device, "PSUM ...").  Exit status 0 only on "== PASS".
set -eu

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"   # .../dma/tb
DMA="$(dirname "$HERE")"
ROOT="$(dirname "$DMA")"                               # .../tile_pipelined
REPO="$(cd "$ROOT/../../.." && pwd)"
COSIM="${COSIM_DIR:-$REPO/test/e2e/cosim}"

[ $# -ge 4 ] || { echo "usage: $0 K_MAX_8X8 K_MAX_4X4 JOBS_SVH OUT_DIR [v1|v2] [+plusarg ...]"; exit 2; }
K8="$1"; K4="$2"; JOBS="$3"; OUT="$4"; shift 4
VARIANT="v1"
if [ $# -ge 1 ] && [ "${1#+}" = "$1" ]; then VARIANT="$1"; shift; fi
for v in "$K8" "$K4"; do
  case "$v" in ''|0*|*[!0-9]*) echo "K_MAX must be a decimal integer without leading zeros, got '$v'"; exit 2 ;; esac
done
case "$VARIANT" in v1) USE_V2=0 ;; v2) USE_V2=1 ;; *) echo "variant must be v1 or v2"; exit 2 ;; esac
[ -f "$JOBS" ] || { echo "no job list $JOBS"; exit 2; }
command -v verilator >/dev/null 2>&1 || { echo "verilator not found"; exit 127; }

rm -rf "$OUT"
mkdir -p "$OUT/inc"
cp "$JOBS" "$OUT/inc/pipe_jobs.svh"

echo "== build: tb_pipeline_overlap K_MAX_8X8=$K8 K_MAX_4X4=$K4 $VARIANT -> $OUT =="
GAP_GEN=""
[ -n "${PE_QUIET_GAP:-}" ] && GAP_GEN="-GPE_QUIET_GAP=$PE_QUIET_GAP"
verilator --binary -Wno-fatal -Werror-USERERROR --timing --public-flat-rw -O2 \
    -GK_MAX_8X8="$K8" -GK_MAX_4X4="$K4" -GUSE_V2="$USE_V2" $GAP_GEN \
    -I"$OUT/inc" -I"$HERE" \
    --top-module tb_pipeline_overlap -o tbrun --Mdir "$OUT/obj" \
    "$HERE/tb_pipeline_overlap.sv" \
    "$HERE/xil_stubs.sv" \
    "$HERE/mig_axi_model.sv" \
    "$COSIM/fp_ieee.sv" \
    "$DMA/systolic_dma_top.sv" \
    "$DMA/systolic_accel_context.sv" \
    "$DMA/dpti_host_rx.sv" \
    "$DMA/dpti_byte_tx.sv" \
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
    "$COSIM/dpti_cosim.cpp" \
    > "$OUT/build.log" 2>&1 || { tail -30 "$OUT/build.log"; exit 1; }

"$OUT/obj/tbrun" +wb_region=4096 "$@" > "$OUT/run.log" 2>&1 || true
grep -E '^(==|  FAIL|PDEV|PSUM)' "$OUT/run.log" || true
grep -q '^== PASS' "$OUT/run.log"
