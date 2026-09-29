# ============================================================
# led_test -- Nexys Video 外接 LED 煙霧測試
# ============================================================
#
# 腳位取自 Digilent 官方 Nexys-Video-Master.xdc(不是憑記憶):
#   jb[3] = W7  = Pmod JB 實體 pin 4
#   jb[7] = Y7  = Pmod JB 實體 pin 10
#   led[0] = T14, led[1] = T15
#
# Pmod 12-pin 腳位定義(上排 1..6、下排 7..12):
#   1 2 3 4 訊號 | 5 GND | 6  3.3V
#   7 8 9 10 訊號 | 11 GND | 12 3.3V
#
# 所以可直接跨接 LED 的相鄰對只有兩組:pin4+pin5、pin10+pin11。
#
# 警告:pin 5(GND)與 pin 6(3.3V)也是相鄰的。方向認反把 LED
# 插進 5+6,等於直接跨在電源與地之間、毫無限流。通電前先用電表
# 確認哪一格是 3.3V。
# ============================================================

# 100 MHz 系統時脈
set_property -dict { PACKAGE_PIN R4 IOSTANDARD LVCMOS33 } [get_ports {clk}]
create_clock -period 10.000 -name clk [get_ports {clk}]

# ------------------------------------------------------------
# 外接 LED:Pmod JB
#
# DRIVE 4 是刻意的:即使 Pmod 腳位沒有板上串聯保護電阻、而你也
# 沒有在 LED 上串電阻,最低驅動強度的輸出阻抗也會把電流壓在
# 安全範圍內。有串電阻(220-470Ω)當然更好。
# ------------------------------------------------------------
set_property -dict { PACKAGE_PIN W7 IOSTANDARD LVCMOS33 DRIVE 4 SLEW SLOW } [get_ports {jb_led[0]}]
set_property -dict { PACKAGE_PIN Y7 IOSTANDARD LVCMOS33 DRIVE 4 SLEW SLOW } [get_ports {jb_led[1]}]

# ------------------------------------------------------------
# 板上內建 LD0 / LD1(對照組)
#
# 注意 IOSTANDARD 是 LVCMOS25,不是 33 -- 這排 LED 在 bank 13,
# VCCO = 2.5V。寫成 LVCMOS33 會在 implementation 直接報錯。
# ------------------------------------------------------------
set_property -dict { PACKAGE_PIN T14 IOSTANDARD LVCMOS25 } [get_ports {led[0]}]
set_property -dict { PACKAGE_PIN T15 IOSTANDARD LVCMOS25 } [get_ports {led[1]}]
