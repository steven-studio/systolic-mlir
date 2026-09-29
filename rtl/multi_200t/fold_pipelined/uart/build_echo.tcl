# build_echo.tcl -- 序列鏈路診斷 bitstream,幾分鐘可建。
#
#   vivado -mode batch -source build_echo.tcl
#   vivado -mode batch -source program_kmax.tcl -tclargs echo
#
# 之後用任何序列終端看:
#   picocom -b 115200 /dev/ttyUSB2
#   'U' 每 ~1.3 秒出現一次(不用送任何東西);打 a 回 A。
#
# 約束是自帶的(只有 clk / rst / uart 四支腳 + 時脈),刻意不讀
# nexys_video_uart.xdc -- 那份裡的 multicycle 約束指向這個設計
# 沒有的 cell。腳位與主設計逐字相同。

set PART "xc7a200tsbg484-1"
set TOP  "uart_echo"
set OUT  "build_kmax/kecho"

file mkdir $OUT
file delete -force $OUT/systolic_uart_tile_top_kecho.bit

read_verilog -sv uart_rx.sv
read_verilog -sv uart_tx.sv
read_verilog -sv uart_echo.sv

# 與 nexys_video_uart.xdc 相同的四支腳。
set xdc $OUT/echo.xdc
set fh [open $xdc w]
puts $fh {set_property -dict { PACKAGE_PIN R4   IOSTANDARD LVCMOS33 } [get_ports {clk}]}
puts $fh {set_property -dict { PACKAGE_PIN B22  IOSTANDARD LVCMOS12 } [get_ports {rst}]}
puts $fh {set_property -dict { PACKAGE_PIN V18  IOSTANDARD LVCMOS33 } [get_ports {uart_rx}]}
puts $fh {set_property -dict { PACKAGE_PIN AA19 IOSTANDARD LVCMOS33 } [get_ports {uart_tx}]}
puts $fh {set_property CLOCK_BUFFER_TYPE NONE [get_ports rst]}
puts $fh {create_clock -period 10.000 -name clk [get_ports {clk}]}
close $fh
read_xdc $xdc

synth_design -top $TOP -part $PART
opt_design
place_design
route_design

set WNS [get_property SLACK [get_timing_paths -delay_type max -max_paths 1]]
puts "WNS = $WNS ns"
if {$WNS < 0} { error "echo 這種尺寸不該 fail timing -- 環境有問題" }

# 檔名配合 program_kmax.tcl 的 k${KMAX} 慣例:-tclargs echo 即可燒。
write_bitstream -force $OUT/systolic_uart_tile_top_kecho.bit
puts "bitstream: $OUT/systolic_uart_tile_top_kecho.bit"
puts "program:   vivado -mode batch -source program_kmax.tcl -tclargs echo"
