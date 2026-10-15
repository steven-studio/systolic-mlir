#!/usr/bin/env bash
#
# run_dpti_top_replay.sh -- build and run tb_dpti_top_replay.sv (Level-2
# integration bench: systolic_dma_top + timing-aware FT2232H model replaying a
# captured host byte stream).
#
#   OUT=<dir> ./run_dpti_top_replay.sh build  sync245|legacy|legacy_param
#   OUT=<dir> ./run_dpti_top_replay.sh run    sync245|legacy|legacy_param NAME +PLUSARGS...
#
#   sync245       DPTI_PHY_SYNC245 = 1 (needs an RTL with that parameter)
#   legacy_param  DPTI_PHY_SYNC245 = 0
#   legacy        an RTL without the parameter (before the integration)
#
# OUT is required: nothing is written next to the sources.  The run prints the
# bench's CFG / COUNTS / ERRORS / RESULT lines; exit status 0 only on
# "RESULT PASS".  Every byte the FPGA wrote goes to OUT/NAME.tx.hex.
# K_MAX_8X8 = K_MAX_4X4 = 16, the co-simulation's default geometry.
set -u
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"   # .../dma/tb
DMA="$(dirname "$HERE")"
ROOT="$(dirname "$DMA")"                               # .../tile_pipelined
REPO="$(cd "$ROOT/../../.." && pwd)"
COSIM="$REPO/test/e2e/cosim"
: "${OUT:?set OUT to an output directory}"
CMD="${1:-}"; PHY="${2:-}"
case "$PHY" in
  sync245)      DEF="-DPHY_SYNC245" ;;
  legacy_param) DEF="-DPHY_LEGACY_PARAM" ;;
  legacy)       DEF="" ;;
  *) echo "usage: OUT=dir $0 build|run sync245|legacy|legacy_param [NAME +args]"; exit 2 ;;
esac
OBJ="$OUT/obj_$PHY"
mkdir -p "$OUT"

if [ "$CMD" = build ]; then
  command -v verilator >/dev/null 2>&1 || { echo "verilator not found"; exit 127; }
  EXTRA=()
  for f in "$DMA/dpti_sync245_phy.sv" "$DMA/dpti_sync245_fifo.sv" \
           "$DMA/dpti_clk_gen.sv" "$DMA/dpti_link_ctrl.sv"; do [ -f "$f" ] && EXTRA+=("$f"); done
  [ -f "$DMA/dpti_link_ctrl.sv" ] && DEF="$DEF -DHAS_LINK_CTRL"
  rm -rf "$OBJ"
  verilator --binary -Wno-fatal --timing --public-flat-rw -O2 $DEF \
    --top-module tb_dpti_top_replay -o tbrun --Mdir "$OBJ" \
    "$HERE/tb_dpti_top_replay.sv" \
    "$HERE/xil_stubs.sv" "$HERE/mig_axi_model.sv" "$COSIM/fp_ieee.sv" \
    "$DMA/systolic_dma_top.sv" "$DMA/systolic_accel_context.sv" "$DMA/dma_engine_multi.sv" \
    "$DMA/dpti_host_rx.sv" "$DMA/dpti_byte_rx.sv" "$DMA/dpti_write32_decoder.sv" \
    "$DMA/dpti_mem_write_decoder.sv" "$DMA/dpti_command_frontend.sv" "$DMA/dpti_async_fifo.sv" \
    "$DMA/dpti_output_cdc.sv" "$DMA/dpti_beat_to_byte.sv" "$DMA/dpti_byte_tx.sv" \
    ${EXTRA[@]+"${EXTRA[@]}"} \
    "$DMA/dpti_mem_write_cdc.sv" "$DMA/dpti_mem_write_engine.sv" "$DMA/dpti_mem_write_cdc_engine.sv" \
    "$DMA/dpti_cmd_async_fifo.sv" "$DMA/dpti_axi4lite_master.sv" "$DMA/axi4lite_dpti_bridge.sv" \
    "$DMA/dpti_descriptor_bridge.sv" "$ROOT/scheduler/systolic_job_ingress.sv" \
    "$DMA/dma_engine.sv" "$DMA/dma_seed_writer.sv" "$DMA/dma_operand_writer.sv" \
    "$DMA/dma_operand_writer_v2.sv" "$DMA/dma_wr_checksum_v2.sv" "$DMA/dma_result_reader.sv" \
    "$DMA/dma_writeback_engine.sv" "$DMA/dma_cdc_fifo.sv" \
    "$ROOT/core/fp_reduce16.sv" "$ROOT/core/systolic_pe_bram.sv" "$ROOT/core/systolic_array.sv" \
    "$ROOT/core/tile_feeder.sv" "$ROOT/core/operand_buffer.sv" "$ROOT/core/operand_buffer_v2.sv" \
    "$ROOT/scheduler/systolic_hw_scheduler.sv" "$ROOT/scheduler/systolic_hw_scheduler_adapter.sv" \
    "$COSIM/dpti_cosim.cpp" \
    > "$OUT/build_$PHY.log" 2>&1 || { tail -30 "$OUT/build_$PHY.log"; exit 1; }
  echo "built $OBJ/tbrun"
  exit 0
fi

[ "$CMD" = run ] || { echo "build|run"; exit 2; }
NAME="${3:?NAME}"; shift 3
[ -x "$OBJ/tbrun" ] || { echo "not built: $OBJ/tbrun"; exit 1; }
"$OBJ/tbrun" +NAME="$NAME" +TXOUT="$OUT/$NAME.tx.hex" "$@" > "$OUT/$NAME.run.log" 2>&1
grep -E '^(CFG|TB )' "$OUT/$NAME.run.log" | head -40
grep -E '^(RXCHECK|CHECKS|COUNTS|ERRORS|RESULT)' "$OUT/$NAME.run.log"
grep -q '^RESULT PASS' "$OUT/$NAME.run.log"
