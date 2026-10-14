#!/usr/bin/env bash
#
# build.sh -- build the RTL co-simulation of the board's FPGA.
#
#   test/e2e/cosim/build.sh            -> build/cosim/{Vcosim,libftdi_cosim.so}
#
# The RTL is the working tree's by default on this branch
# (experiment/batch-gemm-parallelism changes the DPTI path, so the
# co-simulation has to follow the tree); a git revision can be named instead,
# so the simulated FPGA is a programmed bitstream's RTL even while rtl/ moves
# on:
#
#   RTL_DIR  a directory holding rtl/multi_200t/tile_pipelined (default: the
#            repository, i.e. the working tree; ignored when RTL_REV is set)
#   RTL_REV  a git revision to archive the RTL from instead, e.g.
#            tile-pipelined-1x8x8-3x4x4-20261009 (the 2026-10-09 bitstream's
#            tag; its RTL has one K_MAX and no completion record)
#   COSIM_BUILD_DIR  output directory (default build/cosim)
#   USE_V2   0 (default, the v1 operand path dma_top_build.tcl defaults to) or 1
#   K_MAX_8X8, K_MAX_4X4
#            the 8x8's and every 4x4's operand depth, independent, plain
#            decimal (default 16 and 16, the archived bitstream's). The
#            protocol takes 1..16376; this co-simulation takes 1..1024,
#            because its DDR model (tb/mig_axi_model.sv) holds 128 KiB and the
#            runtime stages every slab at 0x10000, so a slab of ceil(K / 8)
#            * 512 bytes must fit in 64 KiB (the board's DDR has no such
#            limit). Only an RTL with the split parameters (systolic_dma_top
#            K_MAX_8X8 / K_MAX_4X4, e.g. RTL_DIR=.) takes other values; the
#            default RTL_REV has one K_MAX and is built at 16.
#
# Besides Vcosim and the libftdi shim, OUT gets fpga_config.txt: the
# parameters this simulated bitstream was built with, in the form the
# runtime reads (SYSTOLIC_FPGA_CONFIG; run_e2e.sh points it there).
#
# The source list is dma_top_build.tcl's for that revision, with the two
# Xilinx floating-point IP wrappers (core/fp_mul.sv, core/fp_add.sv) replaced
# by fp_ieee.sv and the Xilinx primitives/MIG/VIO by the stand-ins
# run_top_sim.sh already uses (tb/xil_stubs.sv, tb/mig_axi_model.sv).
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/../../.." && pwd)"
OUT="${COSIM_BUILD_DIR:-$ROOT/build/cosim}"
RTL_REV="${RTL_REV:-}"
if [ -z "$RTL_REV" ] && [ -z "${RTL_DIR:-}" ]; then
    RTL_DIR="$ROOT"
fi
USE_V2="${USE_V2:-0}"
K_MAX_8X8="${K_MAX_8X8:-16}"
K_MAX_4X4="${K_MAX_4X4:-16}"
JOBS="${JOBS:-$(nproc 2>/dev/null || echo 2)}"

# Plain decimal only: bash arithmetic (the slab size below) and Tcl read a
# leading 0 as octal, Verilog and the runtime as decimal.
for v in K_MAX_8X8 K_MAX_4X4; do
    val="${!v}"
    case "$val" in
        ''|0*|*[!0-9]*|??????*)
            echo "$v must be a decimal integer in 1..16376 without leading zeros, got '$val'" >&2; exit 2 ;;
    esac
    if [ "$val" -gt 16376 ]; then
        echo "$v must be a decimal integer in 1..16376, got '$val'" >&2; exit 2
    fi
    if [ "$val" -gt 1024 ]; then
        echo "$v = $val: the protocol takes it (1..16376), but this co-simulation's DDR model holds 128 KiB and the runtime stages the slab at 0x10000, so its slabs end at K = 1024" >&2
        exit 2
    fi
done
case "$USE_V2" in
    0) ;;
    1) for v in K_MAX_8X8 K_MAX_4X4; do
           if [ $(( ${!v} % 8 )) -ne 0 ] || [ "${!v}" -lt 16 ]; then
               echo "USE_V2=1 needs K_MAX_8X8 and K_MAX_4X4 to be multiples of 8, at least 16 (got $K_MAX_8X8, $K_MAX_4X4)" >&2; exit 2
           fi
       done ;;
    *) echo "USE_V2 must be 0 or 1, got '$USE_V2'" >&2; exit 2 ;;
esac

command -v verilator >/dev/null 2>&1 || {
    echo "verilator not found (apt install verilator / brew install verilator; >= 5.0)" >&2
    exit 127
}

mkdir -p "$OUT"

if [ -z "$RTL_REV" ]; then
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

# The top's K_MAX parameters: split (K_MAX_8X8 / K_MAX_4X4) or one K_MAX.
KMAX_DEFS=()
if grep -q "K_MAX_8X8" "$D/systolic_dma_top.sv"; then
    KMAX_DEFS=(-DCOSIM_SPLIT_KMAX -GK_MAX_8X8="$K_MAX_8X8" -GK_MAX_4X4="$K_MAX_4X4"
               -I"$D/tb")   # reduction_probe.svh, the bench's step counters
elif [ "$K_MAX_8X8" != 16 ] || [ "$K_MAX_4X4" != 16 ]; then
    echo "the RTL in $RTL_DESC has one K_MAX (16); K_MAX_8X8=$K_MAX_8X8 K_MAX_4X4=$K_MAX_4X4 needs an RTL with the split parameters (RTL_DIR=.)" >&2
    exit 2
fi

# dma_top_build.tcl's SRC list, in its order. The per-accelerator contexts
# and the multi-slot read engine exist only in RTL after the 20261009 tag.
RTL_SRCS=(
    "$D/systolic_dma_top.sv"
)
for f in "$D/systolic_accel_context.sv" "$D/dma_engine_multi.sv" "$D/dpti_bus_arbiter.sv"; do
    [ -f "$f" ] && RTL_SRCS+=("$f")
done
RTL_SRCS+=(
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

echo "== cosim: RTL from $RTL_DESC (USE_V2=$USE_V2, K_MAX_8X8=$K_MAX_8X8, K_MAX_4X4=$K_MAX_4X4)"
echo "$RTL_DESC" > "$OUT/RTL_SOURCE"

rm -rf "$OUT/obj" "$OUT/fpga_config.txt"
verilator --binary -Wno-fatal --timing -j "$JOBS" -O3 \
    -GUSE_V2="$USE_V2" ${KMAX_DEFS[@]+"${KMAX_DEFS[@]}"} \
    -I"$HERE" \
    --top-module tb_dpti_cosim -o Vcosim --Mdir "$OUT/obj" \
    "$HERE/tb_dpti_cosim.sv" "${STANDINS[@]}" "${RTL_SRCS[@]}" \
    "$HERE/dpti_cosim.cpp" \
    > "$OUT/verilator.log" 2>&1 || { tail -40 "$OUT/verilator.log"; exit 1; }
cp "$OUT/obj/Vcosim" "$OUT/Vcosim"

cc -O2 -Wall -Wextra -shared -fPIC \
    $(pkg-config --cflags libftdi1 2>/dev/null || echo -I/usr/include/libftdi1) \
    -o "$OUT/libftdi_cosim.so" "$HERE/libftdi_cosim.c"

# The parameters of this simulated bitstream, for the host side
# (runtime/fpga_matmul_tiled.h: fpga_runtime_get_config). Written last, so a
# failed build leaves none behind.
slab() { echo $(( ($1 + 7) / 8 * 8 )); }
S8=$(slab "$K_MAX_8X8"); S4=$(slab "$K_MAX_4X4")
HOST_SLAB=$(( (S8 > S4 ? S8 : S4) * 64 ))
{
    echo "# fpga_config.txt -- written by test/e2e/cosim/build.sh with this co-simulation"
    echo "K_MAX_8X8=$K_MAX_8X8"
    echo "K_MAX_4X4=$K_MAX_4X4"
    echo "N=8"
    echo "NUM_8X8=1"
    echo "NUM_4X4=3"
    echo "VARIANT=$([ "$USE_V2" = 1 ] && echo v2 || echo v1)"
    echo "USE_V2=$USE_V2"
    echo "HOST_SLAB_BYTES=$HOST_SLAB"
    # RTL with the DPTI bus arbiter also sends the 16-byte completion record
    # ahead of every result (systolic_dma_top.sv, Oct 2026); the runtime reads
    # it only when told so here.
    echo "COMPLETION_HEADER=$([ -f "$D/dpti_bus_arbiter.sv" ] && echo 1 || echo 0)"
    echo "RTL_SOURCE=$RTL_DESC"
    echo "BUILT=$(date -Iseconds)"
} > "$OUT/fpga_config.txt"

echo "== cosim: built $OUT/Vcosim, $OUT/libftdi_cosim.so and $OUT/fpga_config.txt"
