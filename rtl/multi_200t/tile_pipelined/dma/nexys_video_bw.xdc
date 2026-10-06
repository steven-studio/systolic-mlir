# -----------------------------------------------------------------------------
# nexys_video_bw.xdc -- pins for ddr3_bw_top.
#
# Same as nexys_video_calib.xdc plus the UART transmit pin.  Every DDR3 pin
# comes from the MIG IP's own XDC; do not add them here.
#
# Pins from Digilent's Nexys-Video-Master.xdc.
# -----------------------------------------------------------------------------

## 100 MHz system oscillator                    Sch=sysclk
set_property -dict { PACKAGE_PIN R4  IOSTANDARD LVCMOS33 } [get_ports { sys_clk_pin }]
create_clock -add -name sys_clk_pin -period 10.000 -waveform {0 5} [get_ports { sys_clk_pin }]

## reset button, active low                     Sch=cpu_resetn
set_property -dict { PACKAGE_PIN G4  IOSTANDARD LVCMOS15 } [get_ports { cpu_resetn }]

## UART: FPGA transmit -> USB bridge receive.
## ADAPT: take the pin and IOSTANDARD from the master XDC.  In Digilent's file
## the FPGA's transmit line is named from the BRIDGE's point of view, so the
## line you want is usually the one commented "uart_rx_out" -- confirm with
##     grep -iE "uart" Nexys-Video-Master.xdc
set_property -dict { PACKAGE_PIN AA19 IOSTANDARD LVCMOS33 } [get_ports { uart_tx_pin }]

## status LEDs
set_property -dict { PACKAGE_PIN T14 IOSTANDARD LVCMOS25 } [get_ports { led[0] }]  ;# init_calib_complete
set_property -dict { PACKAGE_PIN T15 IOSTANDARD LVCMOS25 } [get_ports { led[1] }]  ;# probe issuing
set_property -dict { PACKAGE_PIN T16 IOSTANDARD LVCMOS25 } [get_ports { led[2] }]  ;# probe finished
set_property -dict { PACKAGE_PIN U16 IOSTANDARD LVCMOS25 } [get_ports { led[3] }]  ;# our MMCM locked
set_property -dict { PACKAGE_PIN V15 IOSTANDARD LVCMOS25 } [get_ports { led[4] }]  ;# heartbeat 100 MHz
set_property -dict { PACKAGE_PIN W16 IOSTANDARD LVCMOS25 } [get_ports { led[5] }]  ;# heartbeat ui_clk

set_false_path -from [get_ports cpu_resetn]
set_false_path -to   [get_ports {led[*]}]
set_false_path -to   [get_ports uart_tx_pin]
