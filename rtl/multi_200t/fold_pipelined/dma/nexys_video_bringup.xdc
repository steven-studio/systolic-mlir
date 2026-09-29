# -----------------------------------------------------------------------------
# nexys_video_bringup.xdc -- pins for dma_bringup_top.
#
# Only the non-DDR3 ports are here.  Every DDR3 pin comes from the MIG IP's own
# XDC, which is generated from the pinout inside nexys_video_mig_axi128.prj --
# do not add them here, and do not edit them.
#
# Pin assignments taken verbatim from Digilent's Nexys-Video-Master.xdc
# (github.com/Digilent/digilent-xdc), uncommented and renamed to match the
# ports of ddr3_calib_top.
# -----------------------------------------------------------------------------

## 100 MHz system oscillator                    Sch=sysclk
set_property -dict { PACKAGE_PIN R4  IOSTANDARD LVCMOS33 } [get_ports { sys_clk_pin }]
create_clock -add -name sys_clk_pin -period 10.000 -waveform {0 5} [get_ports { sys_clk_pin }]

## reset button, active low                     Sch=cpu_resetn
set_property -dict { PACKAGE_PIN G4  IOSTANDARD LVCMOS15 } [get_ports { cpu_resetn }]

## status LEDs
set_property -dict { PACKAGE_PIN T14 IOSTANDARD LVCMOS25 } [get_ports { led[0] }]  ;# init_calib_complete   memory alive
set_property -dict { PACKAGE_PIN T15 IOSTANDARD LVCMOS25 } [get_ports { led[1] }]  ;# seed written
set_property -dict { PACKAGE_PIN T16 IOSTANDARD LVCMOS25 } [get_ports { led[2] }]  ;# descriptor complete
set_property -dict { PACKAGE_PIN U16 IOSTANDARD LVCMOS25 } [get_ports { led[3] }]  ;# CHECKSUM MATCHES   <-- THE GATE
set_property -dict { PACKAGE_PIN V15 IOSTANDARD LVCMOS25 } [get_ports { led[4] }]  ;# any error latched
set_property -dict { PACKAGE_PIN W16 IOSTANDARD LVCMOS25 } [get_ports { led[5] }]  ;# heartbeat, ui_clk

## The reset button and the LEDs are asynchronous to everything; do not let
## them create false timing paths.
set_false_path -from [get_ports cpu_resetn]
set_false_path -to   [get_ports {led[*]}]
