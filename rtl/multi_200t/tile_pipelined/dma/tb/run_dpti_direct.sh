#!/usr/bin/env bash
#
# run_dpti_direct.sh -- build tb_dpti_direct.sv once, run one of its tests.
#
#   ./run_dpti_direct.sh basic                 # one job per device, synchronous
#   ./run_dpti_direct.sh multi                 # four jobs in flight
#   ./run_dpti_direct.sh refused               # a refused descriptor must not wedge the link
#   ./run_dpti_direct.sh collision             # MEM_WRITE inside a write-back burst (+w_stall=8)
#   ./run_dpti_direct.sh direction             # result out while a slab streams in
#   ./run_dpti_direct.sh hol                   # two descriptors to one device: documents the stall
#   ./run_dpti_direct.sh all                   # every verdict test (not hol)
#
# Environment:
#   K_MAX_8X8, K_MAX_4X4   the top's parameters (default 16 / 16)
#   COMPLETION_HEADER=1    the RTL emits the 16-byte completion record; the
#                          bench then attributes completions by it. Required
#                          for `multi` and `refused` to PASS; the RTL before
#                          phase 2 of docs/ASYNC.md has no header (default 0)
#   EXTRA_PLUSARGS         appended to the run (e.g. +dpti_check=0)
#   SIM_OUT                build directory (default <repo>/build/dpti_direct_k<8>_<4>,
#                          under the ignored build/ tree, unlike run_top_sim.sh's
#                          tracked sim_out_top_* directories)
#
# The sources are the working tree's, the list is test/e2e/cosim/build.sh's
# (dma_top_build.tcl's order) with fp_ieee.sv for the floating-point IP, so
# a job here computes real binary32 like the board.
#
# Exit status: 0 when every requested test printed PASS (hol: ran), 1 otherwise.
set -u
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"      # .../dma/tb
DMA="$(dirname "$HERE")"
ROOT="$(dirname "$DMA")"                                  # .../tile_pipelined
REPO="$(cd "$ROOT/../../.." && pwd)"
COSIM="$REPO/test/e2e/cosim"

K_MAX_8X8="${K_MAX_8X8:-16}"
K_MAX_4X4="${K_MAX_4X4:-16}"
COMPLETION_HEADER="${COMPLETION_HEADER:-0}"
EXTRA_PLUSARGS="${EXTRA_PLUSARGS:-}"
OUT="${SIM_OUT:-$REPO/build/dpti_direct_k${K_MAX_8X8}_${K_MAX_4X4}}"
JOBS="${JOBS:-$(nproc 2>/dev/null || echo 2)}"

command -v verilator >/dev/null 2>&1 || { echo "verilator not found (>= 5.0)"; exit 127; }

SRCS=(
    "$DMA/systolic_dma_top.sv"
    "$DMA/systolic_accel_context.sv"
    "$DMA/dma_engine_multi.sv"
    "$DMA/dpti_host_rx.sv"
    "$DMA/dpti_byte_rx.sv"
    "$DMA/dpti_write32_decoder.sv"
    "$DMA/dpti_mem_write_decoder.sv"
    "$DMA/dpti_command_frontend.sv"
    "$DMA/dpti_async_fifo.sv"
    "$DMA/dpti_output_cdc.sv"
    "$DMA/dpti_beat_to_byte.sv"
    "$DMA/dpti_byte_tx.sv"
    "$DMA/dpti_mem_write_cdc.sv"
    "$DMA/dpti_mem_write_engine.sv"
    "$DMA/dpti_mem_write_cdc_engine.sv"
    "$DMA/dpti_cmd_async_fifo.sv"
    "$DMA/dpti_axi4lite_master.sv"
    "$DMA/axi4lite_dpti_bridge.sv"
    "$DMA/dpti_descriptor_bridge.sv"
    "$ROOT/scheduler/systolic_job_ingress.sv"
    "$DMA/dma_engine.sv"
    "$DMA/dma_seed_writer.sv"
    "$DMA/dma_operand_writer.sv"
    "$DMA/dma_operand_writer_v2.sv"
    "$DMA/dma_wr_checksum_v2.sv"
    "$DMA/dma_result_reader.sv"
    "$DMA/dma_writeback_engine.sv"
    "$DMA/dma_cdc_fifo.sv"
    "$ROOT/core/fp_reduce16.sv"
    "$ROOT/core/systolic_pe_bram.sv"
    "$ROOT/core/systolic_array.sv"
    "$ROOT/core/tile_feeder.sv"
    "$ROOT/core/operand_buffer.sv"
    "$ROOT/core/operand_buffer_v2.sv"
    "$ROOT/scheduler/systolic_hw_scheduler.sv"
    "$ROOT/scheduler/systolic_hw_scheduler_adapter.sv"
)
# Modules that exist only from phase 2 of docs/ASYNC.md on (the bus arbiter
# of the legacy pads path), and the registered FT2232H PHY with its clock
# and link controller (systolic_dma_top DPTI_PHY_SYNC245, the default from
# 29f3209 on): the generate branch references them whichever PHY is chosen.
for f in "$DMA/dpti_bus_arbiter.sv" "$DMA/dpti_sync245_phy.sv" "$DMA/dpti_sync245_fifo.sv" \
         "$DMA/dpti_clk_gen.sv" "$DMA/dpti_link_ctrl.sv"; do
    [ -f "$f" ] && SRCS+=("$f")
done
# PHY=sync245 (default: the registered PHY, as the board builds) or
# PHY=legacy (the pads path with dpti_bus_arbiter.sv).
PHY="${PHY:-sync245}"
case "$PHY" in
    sync245) PHY_G=1 ;;
    legacy)  PHY_G=0 ;;
    *) echo "PHY must be sync245 or legacy, got '$PHY'" >&2; exit 2 ;;
esac
OUT="${OUT}_${PHY}"

build() {
    mkdir -p "$OUT"
    verilator --binary -Wno-fatal --timing -j "$JOBS" -O2 --public-flat-rw \
        -GUSE_V2=0 -GK_MAX_8X8="$K_MAX_8X8" -GK_MAX_4X4="$K_MAX_4X4" -GDPTI_PHY_SYNC245="$PHY_G" \
        -I"$COSIM" -I"$HERE" \
        --top-module tb_dpti_direct -o tbdirect --Mdir "$OUT/obj" \
        "$HERE/tb_dpti_direct.sv" "$HERE/xil_stubs.sv" "$HERE/mig_axi_model.sv" \
        "$COSIM/fp_ieee.sv" "$COSIM/fp_ieee_dpi.cpp" "${SRCS[@]}" \
        > "$OUT/verilator.log" 2>&1 || { tail -40 "$OUT/verilator.log"; return 1; }
}

run_one() {
    local t="$1" args=""
    [ "$t" = collision ] && args="+w_stall=8"
    "$OUT/obj/tbdirect" "+test=$t" "+completion_header=$COMPLETION_HEADER" $args $EXTRA_PLUSARGS \
        > "$OUT/$t.log" 2>&1
    local line
    line=$(grep -m1 '^RESULT ' "$OUT/$t.log")
    echo "$line"
    case "$t" in
        hol) grep -q '^RESULT hol ' "$OUT/$t.log" ;;
        *)   grep -q "^RESULT $t PASS" "$OUT/$t.log" ;;
    esac
}

TESTS=("${@:-basic}")
[ "${TESTS[0]}" = all ] && TESTS=(basic multi refused collision direction)

echo "== tb_dpti_direct: K_MAX_8X8=$K_MAX_8X8 K_MAX_4X4=$K_MAX_4X4 COMPLETION_HEADER=$COMPLETION_HEADER =="
build || exit 1
rc=0
for t in "${TESTS[@]}"; do
    run_one "$t" || rc=1
done
exit $rc
