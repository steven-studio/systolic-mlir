# 100 MHz system clock
set_property -dict { PACKAGE_PIN R4 IOSTANDARD LVCMOS33 } [get_ports {clk}]
create_clock -period 10.000 -name clk [get_ports {clk}]

# BTNC active-high reset
set_property -dict { PACKAGE_PIN B22 IOSTANDARD LVCMOS12 } [get_ports {rst}]
set_property CLOCK_BUFFER_TYPE NONE [get_ports rst]

# FT232R:
# PC TXD -> FPGA RX
set_property -dict { PACKAGE_PIN V18 IOSTANDARD LVCMOS33 } [get_ports {uart_rx}]

# FPGA TX -> PC RXD
set_property -dict { PACKAGE_PIN AA19 IOSTANDARD LVCMOS33 } [get_ports {uart_tx}]
