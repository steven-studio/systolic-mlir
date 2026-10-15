# -----------------------------------------------------------------------------
# nexys_video_dma_top.xdc -- pins for systolic_dma_top (bring-up step 3b).

# ============================================================================
# Nexys Video FT2232H DPTI
#
# DPTI is the USB-to-FPGA synchronous FIFO interface exposed by the FT2232H.
# These are the official Nexys Video FT2232H DPTI pins.
# ============================================================================

set_property -dict { PACKAGE_PIN U20 IOSTANDARD LVCMOS33 } [get_ports { dpti_d[0] }]
set_property -dict { PACKAGE_PIN P14 IOSTANDARD LVCMOS33 } [get_ports { dpti_d[1] }]
set_property -dict { PACKAGE_PIN P15 IOSTANDARD LVCMOS33 } [get_ports { dpti_d[2] }]
set_property -dict { PACKAGE_PIN U17 IOSTANDARD LVCMOS33 } [get_ports { dpti_d[3] }]
set_property -dict { PACKAGE_PIN R17 IOSTANDARD LVCMOS33 } [get_ports { dpti_d[4] }]
set_property -dict { PACKAGE_PIN P16 IOSTANDARD LVCMOS33 } [get_ports { dpti_d[5] }]
set_property -dict { PACKAGE_PIN R18 IOSTANDARD LVCMOS33 } [get_ports { dpti_d[6] }]
set_property -dict { PACKAGE_PIN N14 IOSTANDARD LVCMOS33 } [get_ports { dpti_d[7] }]

set_property -dict { PACKAGE_PIN N17 IOSTANDARD LVCMOS33 } [get_ports { dpti_rxf_n }]
set_property -dict { PACKAGE_PIN Y19 IOSTANDARD LVCMOS33 } [get_ports { dpti_txe_n }]
set_property -dict { PACKAGE_PIN P19 IOSTANDARD LVCMOS33 } [get_ports { dpti_rd_n }]
set_property -dict { PACKAGE_PIN R19 IOSTANDARD LVCMOS33 } [get_ports { dpti_wr_n }]
set_property -dict { PACKAGE_PIN V17 IOSTANDARD LVCMOS33 } [get_ports { dpti_oe_n }]
set_property -dict { PACKAGE_PIN P17 IOSTANDARD LVCMOS33 } [get_ports { dpti_siwun }]
set_property -dict { PACKAGE_PIN Y18 IOSTANDARD LVCMOS33 } [get_ports { dpti_clkout }]

# FT2232H synchronous FIFO interface clock.
# CLKOUT is a 60 MHz source clock for the DPTI interface.  It drives only the
# MMCM in dpti_clk_gen; the DPTI logic runs on that MMCM's output, named
# clk_dpti here (the rename form of create_generated_clock: Vivado derives the
# clock, including the MMCM's compensation and phase, and only the name is
# set).  I/O timing: nexys_video_dpti_timing.xdc.
create_clock -add -name dpti_clkout -period 16.667 \
  -waveform {0 8.3335} [get_ports { dpti_clkout }]
create_generated_clock -name clk_dpti \
  [get_pins -hierarchical -filter {NAME =~ *u_dpti_clk_gen/u_mmcm/CLKOUT0}]

#
#
# This is nexys_video_bringup.xdc with two more LEDs.  There is no UART in this
# design -- systolic_dma_top instantiates the array directly rather than through
# systolic_uart_top, so there is nothing on V18/AA19 to constrain, and the
# board's UART belongs to whatever build_kmax bitstream you program next.
#
# led[6] and led[7] are Digilent's LD6/LD7, taken from uart/nexys_video_uart.xdc
# rather than from a datasheet, so both designs light the same physical LEDs.
#
# Every DDR3 pin comes from the MIG IP's own XDC, generated from the pinout
# inside nexys_video_mig_axi128.prj.  Do not add them here and do not edit them.
# -----------------------------------------------------------------------------

## 100 MHz system oscillator                    Sch=sysclk
set_property -dict { PACKAGE_PIN R4  IOSTANDARD LVCMOS33 } [get_ports { sys_clk_pin }]
create_clock -add -name sys_clk_pin -period 10.000 -waveform {0 5} [get_ports { sys_clk_pin }]

# DPTI clock and the MIG ui_clk-derived clock are independent clock domains.
# CDC between them is handled explicitly by dpti_mem_write_cdc /
# dpti_async_fifo / dpti_cmd_async_fifo and dpti_link_ctrl's synchronizers.
# Do not time these paths as synchronous inter-clock paths.
#
# Both groups take their generated clocks: the DPTI logic is on clk_dpti (the
# MMCM output), not on dpti_clkout itself, and naming only dpti_clkout would
# leave clk_dpti <-> clk_pll_i timed as synchronous.  sys_clk_pin's family is
# the board oscillator and everything derived from it, clk_pll_i included.
set_clock_groups -asynchronous \
  -group [get_clocks -include_generated_clocks dpti_clkout] \
  -group [get_clocks -include_generated_clocks sys_clk_pin]

## reset button, active low                     Sch=cpu_resetn
set_property -dict { PACKAGE_PIN G4  IOSTANDARD LVCMOS15 } [get_ports { cpu_resetn }]

## status LEDs
set_property -dict { PACKAGE_PIN T14 IOSTANDARD LVCMOS25 } [get_ports { led[0] }]  ;# init_calib_complete   memory alive
set_property -dict { PACKAGE_PIN T15 IOSTANDARD LVCMOS25 } [get_ports { led[1] }]  ;# seed written
set_property -dict { PACKAGE_PIN T16 IOSTANDARD LVCMOS25 } [get_ports { led[2] }]  ;# descriptor complete
set_property -dict { PACKAGE_PIN U16 IOSTANDARD LVCMOS25 } [get_ports { led[3] }]  ;# fold complete
set_property -dict { PACKAGE_PIN V15 IOSTANDARD LVCMOS25 } [get_ports { led[4] }]  ;# chk_wr matches -- operands arrived
set_property -dict { PACKAGE_PIN W16 IOSTANDARD LVCMOS25 } [get_ports { led[5] }]  ;# chk_c  matches <-- THE GATE
set_property -dict { PACKAGE_PIN W15 IOSTANDARD LVCMOS25 } [get_ports { led[6] }]  ;# any error latched
set_property -dict { PACKAGE_PIN Y13 IOSTANDARD LVCMOS25 } [get_ports { led[7] }]  ;# heartbeat, ui_clk

## Asynchronous to everything; do not let them create false timing paths.
set_false_path -from [get_ports cpu_resetn]
set_false_path -to   [get_ports {led[*]}]