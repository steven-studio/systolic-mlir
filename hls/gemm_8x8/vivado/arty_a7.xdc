## Arty A7-35T constraint file
## 只定義本專案實際用到的接腳:clock, reset button, UART TX/RX, LED

## 100MHz 系統時脈
set_property -dict { PACKAGE_PIN E3 IOSTANDARD LVCMOS33 } [get_ports clk]
create_clock -period 10.000 -name sys_clk_pin -waveform {0.000 5.000} -add [get_ports clk]

## 重置按鈕 btn0 (低電位有效,所以電路裡用 btn_rst 命名)
set_property -dict { PACKAGE_PIN D9 IOSTANDARD LVCMOS33 } [get_ports btn_rst]

## USB-UART bridge (FT2232H)
## 注意:FPGA 的 "TX" 接到 FTDI 的 RX,方向對 FPGA 來說是輸出
set_property -dict { PACKAGE_PIN D10 IOSTANDARD LVCMOS33 } [get_ports uart_tx_pin]
set_property -dict { PACKAGE_PIN A9  IOSTANDARD LVCMOS33 } [get_ports uart_rx_pin]

## LED0,完成指示燈
set_property -dict { PACKAGE_PIN H5 IOSTANDARD LVCMOS33 } [get_ports led_done]
