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

# ------------------------------------------------------------
# 板上狀態 LED (systolic_status 驅動)
#
# 判讀:
#   LD0 frame 被接受(閃 42ms)   LD1 RX 進行中
#   LD2 IDLE  LD3 FEED  LD4 WAIT_RESULT  LD5 SEND   (one-hot)
#   LD6 c0_done                 LD7 c1_done
#
# IOSTANDARD 是 LVCMOS25 不是 33 -- 這排 LED 在 bank 13,
# VCCO = 2.5V。寫成 LVCMOS33 會在 implementation 直接報錯。
# 腳位取自 Digilent 官方 Nexys-Video-Master.xdc。
# ------------------------------------------------------------
set_property -dict { PACKAGE_PIN T14 IOSTANDARD LVCMOS25 } [get_ports {led[0]}]
set_property -dict { PACKAGE_PIN T15 IOSTANDARD LVCMOS25 } [get_ports {led[1]}]
set_property -dict { PACKAGE_PIN T16 IOSTANDARD LVCMOS25 } [get_ports {led[2]}]
set_property -dict { PACKAGE_PIN U16 IOSTANDARD LVCMOS25 } [get_ports {led[3]}]
set_property -dict { PACKAGE_PIN V15 IOSTANDARD LVCMOS25 } [get_ports {led[4]}]
set_property -dict { PACKAGE_PIN W16 IOSTANDARD LVCMOS25 } [get_ports {led[5]}]
set_property -dict { PACKAGE_PIN W15 IOSTANDARD LVCMOS25 } [get_ports {led[6]}]
set_property -dict { PACKAGE_PIN Y13 IOSTANDARD LVCMOS25 } [get_ports {led[7]}]

# ------------------------------------------------------------
# 外接 LED:Pmod JB
#
#   jb_led[0] = jb[3] = W7 = JB 實體 pin 4   心跳 ~1.5 Hz
#   jb_led[1] = jb[7] = Y7 = JB 實體 pin 10  busy (state != IDLE)
#
# LED 直接跨在 pin4+pin5 與 pin10+pin11(訊號腳與 GND 腳相鄰)。
# DRIVE 4 是刻意的:即使沒有串限流電阻,最低驅動強度的輸出阻抗
# 也會把電流壓在安全範圍。
#
# 注意 pin5(GND) 與 pin6(3.3V) 也相鄰 -- 插之前先用電表確認方向。
# ------------------------------------------------------------
set_property -dict { PACKAGE_PIN W7 IOSTANDARD LVCMOS33 DRIVE 4 SLEW SLOW } [get_ports {jb_led[0]}]
set_property -dict { PACKAGE_PIN Y7 IOSTANDARD LVCMOS33 DRIVE 4 SLEW SLOW } [get_ports {jb_led[1]}]

# ---------------------------------------------------------------------------
# 已移除:word_buf -> A_buf/B_buf 的 multicycle 約束
#
# 原本這裡有一組 set_multicycle_path,理由是 word_buf[23:0] 在 byte_pos
# 0/1/2 被寫入、在 byte_pos==3 才被讀走,兩者之間隔了完整一個 byte 時間
# (115200 baud 下 86.8us = 8681 個時脈)。那個理由本身仍然成立。
#
# 但它自 operand buffer 重構之後就已經失效:約束指名的 cell 是
# A_buf_reg[*][*][*] / B_buf_reg[*][*][*],那些是尚未抽成模組、
# 尚未推進 BRAM 之前的暫存器名稱。現在的目的地是
# u_a_buf/BANK[*].mem_reg,get_cells 回傳空集合,Vivado 只發一個
# CRITICAL WARNING 就跳過 -- 換句話說,2026-08 之後所有的時序數據
# 都是在「沒有這條約束」的情況下量到的。
#
# 既然 16 個設計點的 WNS 都在 0.1~1.26ns 之間收斂,這條約束並不需要。
# 留著一條靜默失效的約束比沒有更糟:它會讓後來的人以為某條路徑
# 受到保護。要重新啟用的話,先用 get_cells 確認新名稱真的匹配。
# ---------------------------------------------------------------------------
