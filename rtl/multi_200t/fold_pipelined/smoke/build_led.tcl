# build_led.tcl -- 外接 LED 煙霧測試的 bitstream。
#
#   vivado -mode batch -source build_led.tcl
#   vivado -mode batch -source program_kmax.tcl -tclargs led
#
# 產物刻意放在 build_kmax/kled/,檔名照既有慣例,這樣可以直接
# 沿用已經驗證過的 program_kmax.tcl(含陳舊 bitstream 防護)。
# 它會讀同目錄的 summary.csv 檢查時序,所以這裡照樣寫一份。
#
# 這顆設計只有一個 26-bit 計數器,build 大約一兩分鐘。

set PART "xc7a200tsbg484-1"
set TOP  "led_test"
set OUT  "build_kmax/kled"

file mkdir $OUT
file mkdir $OUT/reports

# 陳舊 bitstream 是個陷阱:留下上一輪的 .bit 會被原封不動燒進板子,
# 症狀跟新設計壞掉一模一樣。開跑先刪。
file delete -force $OUT/systolic_uart_tile_top_kled.bit

puts "========================================"
puts " LED smoke test"
puts "   外接 : JB pin 4 (W7) 恆亮 / pin 10 (Y7) 心跳"
puts "   內建 : LD0 恆亮 / LD1 心跳"
puts "   out  = $OUT"
puts "========================================"

read_verilog -sv led_test.sv
read_xdc led_test.xdc

synth_design -top $TOP -part $PART

opt_design
place_design
route_design

report_timing_summary -file $OUT/reports/post_route_timing.rpt
report_drc            -file $OUT/reports/post_route_drc.rpt

set WNS [get_property SLACK [get_timing_paths -delay_type max -max_paths 1]]
set WHS [get_property SLACK [get_timing_paths -delay_type min -max_paths 1]]
set TIMING_OK [expr {$WNS >= 0 && $WHS >= 0}]

# program_kmax.tcl 會讀這一份來拒絕陳舊/未收斂的 bitstream。
set fh [open $OUT/summary.csv w]
puts $fh "k_max,lut,ff,bram,dsp,wns_ns,whs_ns,fmax_mhz,timing_met"
puts $fh [format "led,NA,NA,NA,NA,%.3f,%.3f,NA,%d" $WNS $WHS $TIMING_OK]
close $fh

puts "========================================"
puts " WNS=$WNS ns   WHS=$WHS ns"
if {$TIMING_OK} {
    # 檔名沿用 program_kmax.tcl 的慣例(它組的路徑是
    # build_kmax/k<TAG>/systolic_uart_tile_top_k<TAG>.bit)。
    write_bitstream -force $OUT/systolic_uart_tile_top_kled.bit
    puts " bitstream: $OUT/systolic_uart_tile_top_kled.bit"
    puts " program  : vivado -mode batch -source program_kmax.tcl -tclargs led"
} else {
    puts " bitstream SKIPPED (timing not met)"
}
puts "========================================"
