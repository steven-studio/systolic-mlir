# -----------------------------------------------------------------------------
# nexys_video_dpti_timing.xdc -- FT2232H synchronous-FIFO I/O timing for
# systolic_dma_top.  Read after nexys_video_dma_top.xdc (pins, create_clock on
# dpti_clkout, clk_dpti, clock groups).
#
# Every number is tagged:
#   [DS]  FTDI DS_FT2232H, FT_000061 v2.10, Table 4.1
#   [ASM] assumption, not a measured or published value
#
# REFERENCE
#   All delays are relative to the dpti_clkout port clock: CLKOUT as it
#   arrives at the FPGA pin.  The FT2232H launches and samples on CLKOUT at
#   ITS pin, which is TC earlier.  Vivado propagates dpti_clkout through the
#   MMCM (dpti_clk_gen) to the IOB registers, so the MMCM's compensation, the
#   phase setting and the IBUF/BUFG/tree delays are all in the analysis; none
#   of them is assumed here.
#
#   TC  FT2232H CLKOUT pin -> FPGA pin (Y18) trace delay          [ASM]
#   TD  trace delay of every other DPTI signal, either direction  [ASM]
#       Digilent does not publish Nexys Video trace lengths.  Both are taken
#       as anywhere in 0 .. 0.5 ns (about 0 .. 7.5 cm of FR4), independently,
#       and each bound below uses the worst combination.
#
# DERIVATION (E = a CLKOUT edge at the FPGA pin; the chip sees it at E - TC)
#   Input (chip -> FPGA), data at the FPGA pin = E - TC + Tco + TD:
#     max = Tco_max + TD_max - TC_min = 7.15 + 0.5 - 0.0 = 7.65
#     min = Tco_min + TD_min - TC_max = 1.00 + 0.0 - 0.5 = 0.50
#   Output (FPGA -> chip), must reach the chip by (E_next - TC) - Tsu and not
#   before (E - TC) + Th:
#     max = Tsu + TD_max + TC_max      = 8.0  + 0.5 + 0.5 = 9.00
#     min = TD_min + TC_min - Th       = 0.0  + 0.0 - 0.0 = 0.00
#   Tco (t4/t5/t11) 1 .. 7.15, Tsu (t7/t9/t12/t14) 8, Th (t8/t10/t13/t15) 0.  [DS]
#
# NOT COVERED BY STA
#   * OE# -> read DATA valid (t6, 1 .. 7.15 [DS]) is a path through the chip;
#     dpti_sync245_phy never qualifies data on the edge after OE# falls (RD# is
#     only asserted one clock later), so it carries no requirement.
#   * OE# high -> chip releases D: not in Table 4.1; covered by the PHY's
#     TURN_CYCLES margin [ASM], not by a constraint.
# -----------------------------------------------------------------------------

set ft_tco_min  1.00 ;# [DS] t4/t5/t11 min
set ft_tco_max  7.15 ;# [DS] t4/t5/t11 max
set ft_tsu      8.00 ;# [DS] t7/t9/t12/t14
set ft_th       0.00 ;# [DS] t8/t10/t13/t15

set brd_tc_min  0.00 ;# [ASM]
set brd_tc_max  0.50 ;# [ASM]
set brd_td_min  0.00 ;# [ASM]
set brd_td_max  0.50 ;# [ASM]

# CLKOUT period jitter.  Table 4.1 gives none.  0.2 ns is an assumption so the
# analysis is not run with a perfect clock; replace it when a measured or
# published value exists.                                                [ASM]
set_input_jitter [get_clocks dpti_clkout] 0.200

# ---- chip -> FPGA ------------------------------------------------------------
set_input_delay -clock [get_clocks dpti_clkout] -max \
    [expr {$ft_tco_max + $brd_td_max - $brd_tc_min}] \
    [get_ports {dpti_d[*] dpti_rxf_n dpti_txe_n}]
set_input_delay -clock [get_clocks dpti_clkout] -min \
    [expr {$ft_tco_min + $brd_td_min - $brd_tc_max}] \
    [get_ports {dpti_d[*] dpti_rxf_n dpti_txe_n}]

# ---- FPGA -> chip ------------------------------------------------------------
# dpti_d[*] carries both its data and its output-enable (tristate) path.
set_output_delay -clock [get_clocks dpti_clkout] -max \
    [expr {$ft_tsu + $brd_td_max + $brd_tc_max}] \
    [get_ports {dpti_d[*] dpti_rd_n dpti_wr_n dpti_oe_n}]
set_output_delay -clock [get_clocks dpti_clkout] -min \
    [expr {$brd_td_min + $brd_tc_min - $ft_th}] \
    [get_ports {dpti_d[*] dpti_rd_n dpti_wr_n dpti_oe_n}]

# ---- I/O registers -----------------------------------------------------------
# Same intent as the IOB attributes in dpti_sync245_phy.sv, stated on the
# ports as well.  dma_top_build.tcl verifies after routing that every FT2232H
# port is registered in its own ILOGIC/OLOGIC and refuses the design if not.
set_property IOB TRUE [get_ports {dpti_d[*] dpti_rxf_n dpti_txe_n dpti_rd_n dpti_wr_n dpti_oe_n}]

# ---- safe pads while nothing drives them ------------------------------------
# During configuration and reconfiguration the FPGA's pins float.  Weak pull-
# ups hold RD#, WR#, OE# inactive (high) then.  SIWU# is an FT2232H input the
# design never drives (dpti_siwun is an FPGA input); the pull-up keeps it
# inactive instead of floating.  Whether the board already pulls these lines
# is not documented; a second pull-up is harmless.                     [ASM]
set_property PULLUP TRUE [get_ports {dpti_rd_n dpti_wr_n dpti_oe_n dpti_siwun}]

# ---- crossings into and out of dpti_clk_gen's lock controller ---------------
# The controller runs on clk_sys_100 (sys_clk_pin).  hold_mmcm comes from
# dpti_link_ctrl (ui_clk = clk_pll_i), and the MMCM reset count is sampled in
# ui_clk.  sys_clk_pin and clk_pll_i are related (the MIG derives one from
# the other), so without these Vivado would time two-flop synchronizers as
# synchronous paths.  Bound the data path to one destination period instead.
# (Patterns end in '*', not '[0]' / '[*]': inside -filter =~ a bracket is a
# glob character class, and 'reg[0]' would match no register at all.  Only the
# real crossings exist between these cells, so the wider match is exact.)
set_max_delay -datapath_only 10.000 \
    -from [get_cells -hierarchical -filter {NAME =~ *u_dpti_link_ctrl/hold_mmcm_reg}] \
    -to   [get_cells -hierarchical -filter {NAME =~ *u_dpti_clk_gen/hold_s_reg*}]
set_max_delay -datapath_only 10.000 \
    -from [get_cells -hierarchical -filter {NAME =~ *u_dpti_clk_gen/mmcm_reset_count_reg*}] \
    -to   [get_cells -hierarchical -filter {NAME =~ *dpti_mmcm_resets_meta_reg*}]
