# ============================================================
# Nexys Video - 8x8 Systolic UART
# ============================================================

# 100 MHz system clock
set_property -dict { PACKAGE_PIN R4 IOSTANDARD LVCMOS33 } [get_ports {clk}]

# Active-high reset: center push button BTNC
set_property -dict { PACKAGE_PIN B22 IOSTANDARD LVCMOS12 } [get_ports {rst}]

# ------------------------------------------------------------
# USB-UART (FT232R, connector J13)
#
# PC TXD -> FPGA uart_rx
# FPGA uart_tx -> PC RXD
# ------------------------------------------------------------

set_property -dict { PACKAGE_PIN V18  IOSTANDARD LVCMOS33 } [get_ports {uart_rx}]
set_property -dict { PACKAGE_PIN AA19 IOSTANDARD LVCMOS33 } [get_ports {uart_tx}]

# rst is a control signal, NOT a clock
set_property CLOCK_BUFFER_TYPE NONE [get_ports rst]
create_clock -period 10.000 -name clk [get_ports {clk}]
