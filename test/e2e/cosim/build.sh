#!/usr/bin/env bash
#
# build.sh -- build the RTL co-simulation of the board's FPGA.
#
#   test/e2e/cosim/build.sh            -> build/cosim/{Vcosim,libftdi_cosim.so}
#
# The RTL is taken from the git revision the bitstream on the board was built
# from, not from the working tree, so the simulated FPGA is the programmed
# one even while rtl/ moves on:
#
#   RTL_REV  (default tile-pipelined-1x8x8-3x4x4-20261009, the tag of
#            artifacts/fpga/tile_pipelined/1x8x8-3x4x4/, WNS +0.079 ns)
#   RTL_DIR  a directory holding rtl/multi_200t/tile_pipelined instead
#            (e.g. RTL_DIR=. for the working tree)
#   COSIM_BUILD_DIR  output directory (default build/cosim)
#   USE_V2   0 (default, the v1 operand path dma_top_build.tcl defaults to) or 1
#
# The source list is dma_top_build.tcl's for that revision, with the two
# Xilinx floating-point IP wrappers (core/fp_mul.sv, core/fp_add.sv) replaced
# by fp_ieee.sv and the Xilinx primitives/MIG/VIO by the stand-ins
# run_top_sim.sh already uses (tb/xil_stubs.sv, tb/mig_axi_model.sv).
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/../../.." && pwd)"
OUT="${COSIM_BUILD_DIR:-$ROOT/build/cosim}"
RTL_REV="${RTL_REV:-tile-pipelined-1x8x8-3x4x4-20261009}"
USE_V2="${USE_V2:-0}"
JOBS="${JOBS:-$(nproc 2>/dev/null || echo 2)}"

command -v verilator >/dev/null 2>&1 || {
    echo "verilator not found (apt install verilator / brew install verilator; >= 5.0)" >&2
    exit 127
}

mkdir -p "$OUT"

if [ -n "${RTL_DIR:-}" ]; then
    RTL_ROOT="$(cd "$RTL_DIR" && pwd)"
    RTL_DESC="directory $RTL_ROOT"
else
    RTL_ROOT="$OUT/rtl-src"
    rm -rf "$RTL_ROOT"
    mkdir -p "$RTL_ROOT"
    git -C "$ROOT" rev-parse --verify --quiet "$RTL_REV^{commit}" >/dev/null || {
        echo "git revision $RTL_REV not found (git fetch --tags?)" >&2
        exit 1
    }
    git -C "$ROOT" archive "$RTL_REV" rtl/multi_200t/tile_pipelined \
        | tar -x -C "$RTL_ROOT" --exclude='*/sim_out*'
    RTL_DESC="git $RTL_REV ($(git -C "$ROOT" rev-parse --short "$RTL_REV^{commit}"))"
fi

T="$RTL_ROOT/rtl/multi_200t/tile_pipelined"
D="$T/dma"
[ -f "$D/systolic_dma_top.sv" ] || { echo "no systolic_dma_top.sv under $T" >&2; exit 1; }

# dma_top_build.tcl's SRC list, in its order.
RTL_SRCS=(
    "$D/systolic_dma_top.sv"
    "$D/dpti_host_rx.sv"
    "$D/dpti_byte_rx.sv"
    "$D/dpti_write32_decoder.sv"
    "$D/dpti_mem_write_decoder.sv"
    "$D/dpti_command_frontend.sv"
    "$D/dpti_async_fifo.sv"
    "$D/dpti_output_cdc.sv"
    "$D/dpti_beat_to_byte.sv"
    "$D/dpti_byte_tx.sv"
    "$D/dpti_mem_write_cdc.sv"
    "$D/dpti_mem_write_engine.sv"
    "$D/dpti_mem_write_cdc_engine.sv"
    "$D/dpti_cmd_async_fifo.sv"
    "$D/dpti_axi4lite_master.sv"
    "$D/axi4lite_dpti_bridge.sv"
    "$D/dpti_descriptor_bridge.sv"
    "$T/scheduler/systolic_job_ingress.sv"
    "$D/dma_engine.sv"
    "$D/dma_seed_writer.sv"
    "$D/dma_operand_writer.sv"
    "$D/dma_operand_writer_v2.sv"
    "$D/dma_wr_checksum_v2.sv"
    "$D/dma_result_reader.sv"
    "$D/dma_writeback_engine.sv"
    "$D/dma_cdc_fifo.sv"
    "$T/core/fp_reduce16.sv"
    "$T/core/systolic_pe_bram.sv"
    "$T/core/systolic_array.sv"
    "$T/core/tile_feeder.sv"
    "$T/core/operand_buffer.sv"
    "$T/core/operand_buffer_v2.sv"
    "$T/scheduler/systolic_hw_scheduler.sv"
    "$T/scheduler/systolic_hw_scheduler_adapter.sv"
)
STANDINS=(
    "$D/tb/xil_stubs.sv"
    "$D/tb/mig_axi_model.sv"
    "$HERE/fp_ieee.sv"
)

echo "== cosim: RTL from $RTL_DESC (USE_V2=$USE_V2)"
echo "$RTL_DESC" > "$OUT/RTL_SOURCE"

rm -rf "$OUT/obj"
verilator --binary -Wno-fatal --timing -j "$JOBS" -O3 \
    -GUSE_V2="$USE_V2" \
    --top-module tb_dpti_cosim -o Vcosim --Mdir "$OUT/obj" \
    "$HERE/tb_dpti_cosim.sv" "${STANDINS[@]}" "${RTL_SRCS[@]}" \
    "$HERE/dpti_cosim.cpp" \
    > "$OUT/verilator.log" 2>&1 || { tail -40 "$OUT/verilator.log"; exit 1; }
cp "$OUT/obj/Vcosim" "$OUT/Vcosim"

cc -O2 -Wall -Wextra -shared -fPIC \
    $(pkg-config --cflags libftdi1 2>/dev/null || echo -I/usr/include/libftdi1) \
    -o "$OUT/libftdi_cosim.so" "$HERE/libftdi_cosim.c"

echo "== cosim: built $OUT/Vcosim and $OUT/libftdi_cosim.so"
