#!/usr/bin/env bash
#
# run_kmax_elab.sh -- which K_MAX_8X8 / K_MAX_4X4 systolic_dma_top elaborates,
# and which it refuses, checked with Verilator's elaborator (--lint-only, a
# second per configuration; nothing is simulated).
#
#   ./run_kmax_elab.sh
#
# The RTL states its contract as elaboration-time $error checks
# (systolic_accel_context, dma_operand_writer): K_MAX any integer whose slab
# of ceil(K_MAX / 8) windows the 16-bit descriptor beat count can name
# (1..16376, independent per geometry, no power of two needed); the v2
# beat-wide path additionally a multiple of 8, at least 16.  Verilator reports
# such an $error as USERERROR.  Each configuration below must elaborate
# without one (ACCEPT) or produce one naming the reason (REFUSE).  K_DIM is
# set to K_MAX_8X8, as dma_top_build.tcl, the co-simulation and the benches
# set it.
set -u

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"   # .../dma/tb
DMA="$(dirname "$HERE")"
ROOT="$(dirname "$DMA")"                               # .../tile_pipelined
REPO="$(cd "$ROOT/../../.." && pwd)"

command -v verilator >/dev/null 2>&1 || { echo "verilator not found"; exit 127; }

SRCS=(
  "$HERE/xil_stubs.sv" "$HERE/mig_axi_model.sv" "$REPO/test/e2e/cosim/fp_ieee.sv"
  "$DMA/systolic_dma_top.sv" "$DMA/systolic_accel_context.sv" "$DMA/dma_engine_multi.sv"
  "$DMA/dpti_host_rx.sv" "$DMA/dpti_byte_rx.sv" "$DMA/dpti_write32_decoder.sv"
  "$DMA/dpti_mem_write_decoder.sv" "$DMA/dpti_command_frontend.sv" "$DMA/dpti_async_fifo.sv"
  "$DMA/dpti_output_cdc.sv" "$DMA/dpti_beat_to_byte.sv" "$DMA/dpti_byte_tx.sv"
  "$DMA/dpti_sync245_phy.sv" "$DMA/dpti_sync245_fifo.sv"
  "$DMA/dpti_clk_gen.sv" "$DMA/dpti_link_ctrl.sv"
  "$DMA/dpti_mem_write_cdc.sv" "$DMA/dpti_mem_write_engine.sv" "$DMA/dpti_mem_write_cdc_engine.sv"
  "$DMA/dpti_cmd_async_fifo.sv" "$DMA/dpti_axi4lite_master.sv" "$DMA/axi4lite_dpti_bridge.sv"
  "$DMA/dpti_descriptor_bridge.sv" "$ROOT/scheduler/systolic_job_ingress.sv"
  "$DMA/dma_engine.sv" "$DMA/dma_seed_writer.sv" "$DMA/dma_operand_writer.sv"
  "$DMA/dma_operand_writer_v2.sv" "$DMA/dma_wr_checksum_v2.sv" "$DMA/dma_result_reader.sv"
  "$DMA/dma_writeback_engine.sv" "$DMA/dma_cdc_fifo.sv"
  "$ROOT/core/fp_reduce16.sv" "$ROOT/core/systolic_pe_bram.sv" "$ROOT/core/systolic_array.sv"
  "$ROOT/core/tile_feeder.sv" "$ROOT/core/operand_buffer.sv" "$ROOT/core/operand_buffer_v2.sv"
  "$ROOT/scheduler/systolic_hw_scheduler.sv" "$ROOT/scheduler/systolic_hw_scheduler_adapter.sv"
)

# variant K8 K4 expectation [reason the refusal must name]
CASES=(
  "v1 16 16 ACCEPT"
  "v1 1 1 ACCEPT"
  "v1 1 16 ACCEPT"
  "v1 16 1 ACCEPT"
  "v1 17 7 ACCEPT"
  "v1 24 12 ACCEPT"
  "v1 100 3 ACCEPT"
  "v1 16376 16 ACCEPT"
  "v1 16 16376 ACCEPT"
  "v1 16377 16 REFUSE more beats than the BEAT_W-bit descriptor counts"
  "v1 16 16377 REFUSE more beats than the BEAT_W-bit descriptor counts"
  "v2 16 16 ACCEPT"
  "v2 24 16 ACCEPT"
  "v2 40 24 ACCEPT"
  "v2 16 12 REFUSE USE_V2 requires K_MAX to be a multiple of 8 and at least 16"
  "v2 24 8 REFUSE USE_V2 requires K_MAX to be a multiple of 8 and at least 16"
)

pass=0; fail=0
for c in "${CASES[@]}"; do
  read -r variant k8 k4 want reason <<<"$c"
  v2=0; [ "$variant" = v2 ] && v2=1
  out=$(verilator --lint-only -Wno-fatal -Wno-lint -Wno-style --timing \
          --top-module systolic_dma_top -GUSE_EXTERNAL_SCHEDULER=1 \
          -GUSE_V2="$v2" -GK_MAX_8X8="$k8" -GK_MAX_4X4="$k4" -GK_DIM="$k8" \
          "${SRCS[@]}" 2>&1)
  errs=$(printf "%s\n" "$out" | grep -E '^%(Warning|Error)-USERERROR' | sed -E 's/^[^:]*: [^ ]* //' | sort -u)
  if [ "$want" = ACCEPT ]; then
    if [ -z "$errs" ] && ! printf "%s\n" "$out" | grep -q '^%Error'; then r=PASS; else r=FAIL; fi
  else
    if printf "%s\n" "$errs" | grep -qF -- "$reason"; then r=PASS; else r=FAIL; fi
  fi
  [ "$r" = PASS ] && pass=$((pass + 1)) || fail=$((fail + 1))
  printf "%-4s %-4s K_MAX_8X8=%-6s K_MAX_4X4=%-6s %-7s %s\n" "$r" "$variant" "$k8" "$k4" "$want" \
    "$(printf "%s" "$errs" | head -1 | cut -c1-90)"
done
echo
echo "$pass passed, $fail failed"
[ "$fail" -eq 0 ]
