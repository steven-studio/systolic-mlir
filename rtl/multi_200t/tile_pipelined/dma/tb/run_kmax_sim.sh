#!/usr/bin/env bash
#
# run_kmax_sim.sh -- systolic_dma_top at independent K_MAX_8X8 / K_MAX_4X4,
# with binary32 arithmetic and every result element checked (tb_kmax_hetero.sv).
#
#   ./run_kmax_sim.sh K_MAX_8X8 K_MAX_4X4 [v1|v2]   one configuration
#   ./run_kmax_sim.sh 17 7
#   ./run_kmax_sim.sh 24 16 v2
#
# Same top and sources as run_top_sim.sh (external scheduler, only the Xilinx
# IP replaced), except that fp_mul / fp_add are the co-simulation's IEEE
# binary32 model (test/e2e/cosim/fp_ieee.sv, its DPI half in dpti_cosim.cpp)
# instead of tb/fp_model.sv's integer one -- this bench compares numbers, not
# just where they went.  Each value is any 1..56 (the bench's address map),
# written in plain decimal; neither has to be a power of two.  v1 (default) is
# the operand path of the archived bitstream and the co-simulation; v2, the
# beat-wide path, needs multiples of 8 that are at least 16.
#
# Output: OUT_DIR (default <repo>/build/rtl_sim/kmax_<K8>_<K4>[_v2],
# gitignored). Exit status 0 only on "== PASS".
#
#   Verilator >= 5 (apt install verilator).
set -eu

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"   # .../dma/tb
DMA="$(dirname "$HERE")"
ROOT="$(dirname "$DMA")"                               # .../tile_pipelined
REPO="$(cd "$ROOT/../../.." && pwd)"
COSIM="$REPO/test/e2e/cosim"

[ $# -eq 2 ] || [ $# -eq 3 ] || { echo "usage: $0 K_MAX_8X8 K_MAX_4X4 [v1|v2]"; exit 2; }
K8="$1"; K4="$2"; VARIANT="${3:-v1}"
# Plain decimal only: a leading 0 would be octal to bash arithmetic.
for v in "$K8" "$K4"; do
  case "$v" in ''|0*|*[!0-9]*) echo "K_MAX must be a decimal integer in 1..56 (no leading zeros), got '$v'"; exit 2 ;; esac
  if [ "$v" -lt 1 ] || [ "$v" -gt 56 ]; then echo "K_MAX must be a decimal integer in 1..56, got '$v'"; exit 2; fi
done
case "$VARIANT" in
  v1) USE_V2=0; SUFFIX="" ;;
  v2) USE_V2=1; SUFFIX="_v2"
      for v in "$K8" "$K4"; do
        if [ $((v % 8)) -ne 0 ] || [ "$v" -lt 16 ]; then
          echo "the v2 operand path needs K_MAX multiples of 8, at least 16 (got $K8, $K4)"; exit 2
        fi
      done ;;
  *)  echo "variant must be v1 or v2, got '$VARIANT'"; exit 2 ;;
esac

command -v verilator >/dev/null 2>&1 || { echo "verilator not found"; exit 127; }

OUT="${OUT_DIR:-$REPO/build/rtl_sim/kmax_${K8}_${K4}${SUFFIX}}"
rm -rf "$OUT"
mkdir -p "$OUT"

echo "== build: tb_kmax_hetero K_MAX_8X8=$K8 K_MAX_4X4=$K4 $VARIANT -> $OUT =="
# -Werror-USERERROR: an elaboration-time $error (an unsupported K_MAX) stops
# the build instead of being one warning among many.
verilator --binary -Wno-fatal -Werror-USERERROR --timing --public-flat-rw -O2 \
    -GK_MAX_8X8="$K8" -GK_MAX_4X4="$K4" -GUSE_V2="$USE_V2" \
    --top-module tb_kmax_hetero -o tbrun --Mdir "$OUT/obj" \
    "$HERE/tb_kmax_hetero.sv" \
    "$HERE/xil_stubs.sv" \
    "$HERE/mig_axi_model.sv" \
    "$COSIM/fp_ieee.sv" \
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
    "$COSIM/dpti_cosim.cpp" \
    > "$OUT/build.log" 2>&1 || { tail -30 "$OUT/build.log"; exit 1; }

# The RTL's per-transition traces are long; keep them, print the verdicts.
"$OUT/obj/tbrun" +wb_region=4096 > "$OUT/run.log" 2>&1 || true
grep -E '^(==|--|  (job|device|COMPUTE|operand|all four|FAIL))' "$OUT/run.log" || true
grep -q '^== PASS' "$OUT/run.log"
