# -----------------------------------------------------------------------------
# nexys_video_dma_top.xdc -- pins for systolic_dma_top (bring-up step 3b).
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