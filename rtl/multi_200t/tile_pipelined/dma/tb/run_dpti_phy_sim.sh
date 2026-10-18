#!/usr/bin/env bash
#
# run_dpti_phy_sim.sh -- standalone dpti_sync245_phy bench under Icarus.
#
#   ./run_dpti_phy_sim.sh            # full matrix
#   ./run_dpti_phy_sim.sh quick      # functional set only
#
# Output goes to $DPTI_PHY_SIM_OUT (default: tb/sim_out_dpti_phy).
#
# Four groups:
#   F  functional, nominal timing (board delays 0, IOB clock-to-out T_CO=5.0
#      [ASM], FPGA clock MMCM-aligned)                     -- must PASS
#   S  board-delay sweep TC x TD in {0,0.5,1.0,1.5}, plus jitter / duty /
#      chip clock-to-out corners                           -- must PASS
#   N  negative controls: timing deliberately outside the budget, to show the
#      checkers fire                                       -- must FAIL
#   L  the existing dpti_byte_rx + dpti_byte_tx (unmodified, via
#      dpti_legacy_phy_wrap) on the same bench             -- reported
#
# Every timing number here is an assumption or an STA-derived estimate; see the
# header of tb_dpti_sync245_phy.sv and ft2232h_sync245_model.sv.
set -u

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"   # .../dma/tb
DMA="$(dirname "$HERE")"
OUT="${DPTI_PHY_SIM_OUT:-$HERE/sim_out_dpti_phy}"
MODE="${1:-full}"
mkdir -p "$OUT"

command -v iverilog >/dev/null 2>&1 || { echo "iverilog not found"; exit 127; }

NEW_SRC="$HERE/tb_dpti_sync245_phy.sv $HERE/ft2232h_sync245_model.sv \
  $DMA/dpti_sync245_phy.sv $DMA/dpti_sync245_fifo.sv"
OLD_SRC="$HERE/tb_dpti_sync245_phy.sv $HERE/ft2232h_sync245_model.sv \
  $HERE/dpti_legacy_phy_wrap.sv $DMA/dpti_byte_rx.sv $DMA/dpti_byte_tx.sv"

iverilog -g2012 -o "$OUT/tb_new.vvp" $NEW_SRC || exit 1
iverilog -g2012 -DLEGACY -o "$OUT/tb_legacy.vvp" $OLD_SRC || exit 1

SUMMARY="$OUT/summary.txt"
: > "$SUMMARY"
BAD=0

# run <group> <name> <expect PASS|FAIL|REPORT> <vvp> <plusargs...>
run() {
  local grp="$1" name="$2" expect="$3" vvp="$4"; shift 4
  local log="$OUT/${grp}_${name}.log"
  timeout 1800 vvp -n "$OUT/$vvp" "$@" > "$log" 2>&1
  local res
  res="$(grep -m1 '^RESULT' "$log" | awk '{print $2}')"
  [ -z "$res" ] && res="NORESULT"
  local errs
  errs="$(grep -m1 '^ERRORS' "$log" | cut -d' ' -f2-)"
  local verdict="ok"
  if [ "$expect" = "PASS" ] && [ "$res" != "PASS" ]; then verdict="UNEXPECTED"; BAD=1; fi
  if [ "$expect" = "FAIL" ] && [ "$res" != "FAIL" ]; then verdict="UNEXPECTED"; BAD=1; fi
  printf '%-2s %-28s expect=%-6s got=%-8s %-10s %s\n' "$grp" "$name" "$expect" "$res" "$verdict" "$errs" | tee -a "$SUMMARY"
}

# ---------------- F: functional ----------------
run F reset            PASS tb_new.vvp +TEST=reset +SEED=1
run F rx_burst         PASS tb_new.vvp +TEST=rx +NRX=6000 +SEED=2
run F rx_backpressure  PASS tb_new.vvp +TEST=rx_bp +NRX=6000 +READY_PCT=30 +SEED=3
run F rx_trickle       PASS tb_new.vvp +TEST=rx +NRX=1500 +FEED=1 +READY_PCT=70 +SEED=4
run F rx_throughput    PASS tb_new.vvp +TEST=rx +NRX=4000 +FEED=2 +SEED=5
run F tx_basic         PASS tb_new.vvp +TEST=tx +NTX=6000 +SEED=6
run F tx_stall_glitch  PASS tb_new.vvp +TEST=tx +NTX=6000 +TXCAP=4 +DRAIN_PCT=30 +GLITCH=80 +TXV_PCT=80 +SEED=7
run F tx_throughput    PASS tb_new.vvp +TEST=tx +NTX=4000 +TXCAP=4096 +SEED=8
for s in 11 12 13 14 15; do
  run F "bidir_seed$s"  PASS tb_new.vvp +TEST=bidir +NRX=4000 +NTX=4000 +READY_PCT=60 +TXV_PCT=70 \
                             +TXCAP=8 +DRAIN_PCT=50 +GLITCH=20 +SEED=$s
done
run F bidir_trickle    PASS tb_new.vvp +TEST=bidir +NRX=1500 +NTX=1500 +FEED=1 +TXCAP=2 +DRAIN_PCT=20 +GLITCH=50 +SEED=16

[ "$MODE" = "quick" ] && { echo "BAD=$BAD"; exit $BAD; }

# ---------------- S: board-delay sweep and corners ----------------
STRESS="+TEST=bidir +NRX=2500 +NTX=2500 +READY_PCT=60 +TXV_PCT=70 +TXCAP=8 +DRAIN_PCT=50 +GLITCH=20"
for tc in 0.0 0.5 1.0 1.5; do
  for td in 0.0 0.5 1.0 1.5; do
    run S "tc${tc}_td${td}" PASS tb_new.vvp $STRESS +TC=$tc +TD=$td +SEED=21
  done
done
run S tcomax_tc1.5_td1.5   PASS tb_new.vvp $STRESS +TC=1.5 +TD=1.5 +TCO_MODE=1 +SEED=22
run S tcomin_tc1.5_td0     PASS tb_new.vvp $STRESS +TC=1.5 +TD=0.0 +TCO_MODE=2 +SEED=23
run S jit0.2_tc1.5_td1.5   PASS tb_new.vvp $STRESS +TC=1.5 +TD=1.5 +JIT=0.2 +SEED=24
run S duty45               PASS tb_new.vvp $STRESS +DUTY=0.45 +TC=1.0 +TD=1.0 +SEED=25
run S duty55               PASS tb_new.vvp $STRESS +DUTY=0.55 +TC=1.0 +TD=1.0 +SEED=26
run S tco6.0_tc1.0_td1.0   PASS tb_new.vvp $STRESS +T_CO=6.0 +TC=1.0 +TD=1.0 +SEED=27
run S turn_toff_40         PASS tb_new.vvp $STRESS +TOFF=40.0 +SEED=28

# ---------------- N: negative controls (checkers must fire) ----------------
run N tco8.0_outputs_late   FAIL tb_new.vvp $STRESS +T_CO=8.0 +TC=0.5 +TD=0.5 +SEED=31
run N late_clock_input_hold FAIL tb_new.vvp $STRESS +T_INS=2.5 +TCO_MODE=2 +SEED=32
run N toff_beyond_turn      FAIL tb_new.vvp $STRESS +TOFF=60.0 +SEED=33

# ---------------- L: existing PHY on the same bench ----------------
# ideal: every delay small -- functional / protocol behaviour only.  T_INS is
# 0.3, not 0, so that the FPGA edge never coincides with CLKOUT in simulation.
IDEAL="+CLK_TRACK=1 +T_INS=0.3 +D_IN=0.2 +T_CO=0.5"
run L ideal_rx          REPORT tb_legacy.vvp +TEST=rx    +NRX=3000 $IDEAL +SEED=41
run L ideal_tx          REPORT tb_legacy.vvp +TEST=tx    +NTX=3000 $IDEAL +SEED=42
run L ideal_bidir       REPORT tb_legacy.vvp $STRESS $IDEAL +SEED=43
run L ideal_reset       REPORT tb_legacy.vvp +TEST=reset $IDEAL +SEED=44
# STA-derived (XDC-0, board delay 0, slow corner): BUFG insertion T_INS, and
# per-port register/pin -> pad delays.  Input capture is not checked here
# (CHK_FPGA_IN=0): STA showed it passing at board delay 0, and D_IN is set
# T_INS + 0.5 so the bench captures as that STA result implies.
K16="+CLK_TRACK=1 +T_INS=5.417 +D_IN=5.917 +D_RD=10.146 +D_OE=11.462 +D_WR=8.181 +D_DAT=10.400 +CHK_FPGA_IN=0"
K512="+CLK_TRACK=1 +T_INS=5.442 +D_IN=5.942 +D_RD=12.704 +D_OE=10.369 +D_WR=11.235 +D_DAT=13.326 +CHK_FPGA_IN=0"
run L k16_rx            REPORT tb_legacy.vvp +TEST=rx +NRX=3000 $K16 +SEED=45
run L k16_bidir         REPORT tb_legacy.vvp $STRESS $K16 +SEED=46
run L k512_rx           REPORT tb_legacy.vvp +TEST=rx +NRX=3000 $K512 +SEED=47
run L k512_bidir        REPORT tb_legacy.vvp $STRESS $K512 +SEED=48

echo "BAD=$BAD  (summary: $SUMMARY)"
exit $BAD
