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

# ---------------------------------------------------------------------------
# UART 載入路徑:多週期
#
# word_buf[23:0] 在 byte_pos 0/1/2 被寫入,在 byte_pos==3 才被讀去寫進
# A_buf/B_buf。兩者之間隔了完整的一個 byte 時間:115200 baud 下是
# 86.8 us,等於 8681 個 100 MHz 時脈。單週期是工具的預設,不是設計的需求。
#
# 這條路徑 logic levels = 0、route 佔 95%,是 WNS 的來源。word_buf 的
# 每個 bit 扇出到 256 顆 FF(A_buf 8x16 + B_buf 16x8),在 70% LUT
# 使用率下繞線繞不動。
#
# 保守取 4 拍(實際餘裕是 8681 拍)。setup N 必須配 hold N-1,
# 否則工具會反過來把 hold 檢查收緊。
# ---------------------------------------------------------------------------
set_multicycle_path -setup 4 \
    -from [get_cells {word_buf_reg[*]}] \
    -to   [get_cells {A_buf_reg[*][*][*] B_buf_reg[*][*][*]}]

set_multicycle_path -hold 3 \
    -from [get_cells {word_buf_reg[*]}] \
    -to   [get_cells {A_buf_reg[*][*][*] B_buf_reg[*][*][*]}]