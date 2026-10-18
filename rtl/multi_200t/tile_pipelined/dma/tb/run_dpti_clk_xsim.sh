#!/usr/bin/env bash
#
# run_dpti_clk_xsim.sh -- tb_dpti_clk_link.sv (dpti_clk_gen + dpti_link_ctrl)
# against Xilinx's MMCME2_ADV simulation model, in the Vivado simulator.
# Simulation only: no synthesis, no implementation, no hardware.
#
#   OUT=<dir> ./run_dpti_clk_xsim.sh       (default OUT: ./sim_out_dpti_clk)
#
# Needs xvlog/xelab/xsim on PATH (source Vivado's settings64.sh) or
# XILINX_VIVADO set.  Exit 0 only on "RESULT PASS".
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DMA="$(cd "$HERE/.." && pwd)"
OUT="${OUT:-$HERE/sim_out_dpti_clk}"

if [ -n "${XILINX_VIVADO:-}" ]; then PATH="$XILINX_VIVADO/bin:$PATH"; fi
command -v xvlog >/dev/null 2>&1 || { echo "xvlog not found (source Vivado settings64.sh or set XILINX_VIVADO)"; exit 127; }
VIV="$(cd "$(dirname "$(command -v xvlog)")/.." && pwd)"

mkdir -p "$OUT"
cd "$OUT"
rm -rf xsim.dir
xvlog -log xvlog.log --sv "$DMA/dpti_clk_gen.sv" "$DMA/dpti_link_ctrl.sv" "$HERE/tb_dpti_clk_link.sv" \
      "$VIV/data/verilog/src/glbl.v" > /dev/null
xelab -log xelab.log -L unisims_ver --timescale 1ns/1ps -s tb_dpti_clk_link \
      work.tb_dpti_clk_link work.glbl > /dev/null
xsim -log run.log tb_dpti_clk_link -R > /dev/null 2>&1 || true
grep -E "ok |ERROR|SUMMARY|RESULT|S[0-9]:" run.log || true
grep -q "^RESULT PASS" run.log
