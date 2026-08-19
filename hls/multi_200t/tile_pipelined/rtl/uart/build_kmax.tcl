# build_kmax.tcl -- one synthesis+implementation run at a given K_MAX.
#
#   vivado -mode batch -source build_kmax.tcl -tclargs <K_MAX>
#
# K_MAX is the SYNTHESIS-TIME HARDWARE CAPACITY: the deepest reduction the
# on-chip operand buffers can hold. It is not the workload's K, and it is
# not a fold count.
#
#   K_MAX   synthesis-time hardware capacity     <- this script's argument
#   k_dim   runtime workload reduction length    <- supplied by software
#   folds   k_dim / 8, derived at runtime        <- not a hardware quantity
#
# An earlier revision of this script drove a top-level `NFOLD` generic.
# That was the wrong abstraction: it baked the fold COUNT into the
# hardware, when the fold count is a scheduling quantity software derives
# from k_dim. Do not reintroduce it under a different name -- K_MAX is a
# capacity, and the operand buffers are indexed by absolute k, so no fold
# count appears in the RTL at all.
#
# RX framing scales with K_MAX (K_MAX * 64 bytes); TX does not (always
# 512 bytes, two accumulator contexts). So the host must send more for a
# deeper build, but always reads back the same two 8x8 matrices and adds
# them itself.
#
# NOTE: test_uart_fold8x8.py sends exactly 1024 bytes and is therefore a
# K_MAX=16 test. It needs updating before it can drive a deeper build.

if {$argc < 1} {
    error "usage: -tclargs <K_MAX> \[DEBUG_MARKERS\]   (16, 32, 64, 128)"
}

set KMAX [lindex $argv 0]

# 第二個參數 = DEBUG_MARKERS(預設 0)。開啟時輸出到 k<K>_dbg,
# 不覆蓋乾淨版;燒錄用 program_kmax.tcl -tclargs <K>_dbg。
# 板上判讀:host 收到的前幾個 byte 是 A1..A5 breadcrumb,
#   什麼都沒有       -> RX framing 從未接受 frame
#   只有 A1          -> 卡在 ST_FEED
#   A1 A2            -> 卡在 ST_WAIT_RESULT(PE 沒全部完成)
#   A1 A2 A3 A4 (A5) -> 結果有了,卡在 TX/ST_SEND
set DBG [expr {$argc >= 2 ? [lindex $argv 1] : 0}]

# The RTL requires K_MAX >= 16 and a multiple of 8. It has an elaboration
# assertion for both, but failing here is cheaper than failing in synth.
if {$KMAX < 16 || ($KMAX % 8) != 0} {
    error "K_MAX must be a multiple of 8 and at least 16 (got $KMAX)"
}


set PART        "xc7a200tsbg484-1"
set TOP         "systolic_uart_tile_top"
set CLK_PERIOD  10.000
set OUT         [expr {$DBG ? "build_kmax/k${KMAX}_dbg" : "build_kmax/k${KMAX}"}]
set BITTAG      [expr {$DBG ? "${KMAX}_dbg" : $KMAX}]

file mkdir $OUT
file mkdir $OUT/reports

# 陳舊 bitstream 是個陷阱:本腳本在時序未收斂時「跳過」write_bitstream,
# 而 program_kmax.tcl 只檢查路徑上有沒有檔案 -- 上一輪留下的舊 .bit 會被
# 原封不動燒進板子,症狀跟新設計壞掉一模一樣。開跑先刪,跑完後路徑上
# 若還有 .bit,就只可能是這一輪產生的。
file delete -force $OUT/${TOP}_k${BITTAG}.bit

puts "========================================"
puts " k_max sweep point"
puts "   K_MAX = $KMAX"
puts "   out   = $OUT"
puts "========================================"


read_verilog -sv uart_rx.sv
read_verilog -sv uart_tx.sv
read_verilog -sv fp_mul.sv
read_verilog -sv fp_add.sv
read_verilog -sv fp_reduce16.sv
read_verilog -sv systolic_pe_tile.sv
read_verilog -sv systolic_array_tile.sv
read_verilog -sv systolic_array_8x8_tile.sv
read_verilog -sv systolic_tile_feeder.sv
read_verilog -sv systolic_operand_buffer.sv
read_verilog -sv systolic_uart_tile_top.sv

read_ip rtl_fp_pe_test/rtl_fp_pe_test.srcs/sources_1/ip/floating_point_add_0/floating_point_add_0.xci
read_ip rtl_fp_pe_test/rtl_fp_pe_test.srcs/sources_1/ip/floating_point_mul_0/floating_point_mul_0.xci


read_xdc nexys_video_uart.xdc

# ---------------------------------------------------------------------
# Synthesis
#
# DEBUG_MARKERS stays 0 for every sweep point. The breadcrumb bytes
# desynchronise a host reading exactly 512 bytes, and enabling them for
# some points but not others would put a constant-but-unequal offset into
# the LUT column.
#
# ---------------------------------------------------------------------
synth_design -top $TOP -part $PART \
    -generic K_MAX=$KMAX \
    -generic DEBUG_MARKERS=$DBG \
    -generic CYCLE_COUNTER=1

write_checkpoint -force $OUT/post_synth.dcp
report_utilization    -file $OUT/reports/post_synth_utilization.rpt
report_timing_summary -file $OUT/reports/post_synth_timing.rpt

opt_design
place_design
write_checkpoint -force $OUT/post_place.dcp
report_timing_summary -file $OUT/reports/post_place_timing.rpt

route_design
write_checkpoint -force $OUT/post_route.dcp
report_timing_summary -file $OUT/reports/post_route_timing.rpt
report_utilization    -file $OUT/reports/post_route_utilization.rpt
report_drc            -file $OUT/reports/post_route_drc.rpt

# ---------------------------------------------------------------------
# Machine-readable summary
# ---------------------------------------------------------------------

proc util_row {rpt name} {
    # Utilization rows look like:
    #   | Slice LUTs | 12345 | 0 | 0 | 134600 | 9.17 |
    # Used is the first numeric column. Block RAM Tile can be fractional.
    set pat "\\|\\s*[string map {( \\( ) \\)} $name]\\s*\\|\\s*(\[0-9\]+(?:\\.\[0-9\]+)?)\\s*\\|"
    if {[regexp $pat $rpt -> v]} { return $v }
    return "NA"
}

set urpt [report_utilization -return_string]

set LUT  [util_row $urpt "Slice LUTs"]
set FF   [util_row $urpt "Slice Registers"]
set BRAM [util_row $urpt "Block RAM Tile"]
set DSP  [util_row $urpt "DSPs"]

if {$LUT  eq "NA"} { set LUT  [util_row $urpt "CLB LUTs"] }
if {$FF   eq "NA"} { set FF   [util_row $urpt "CLB Registers"] }
if {$DSP  eq "NA"} { set DSP  [util_row $urpt "DSPs \\(DSP48E1\\)"] }

set WNS  [get_property SLACK [get_timing_paths -delay_type max -max_paths 1]]
set WHS  [get_property SLACK [get_timing_paths -delay_type min -max_paths 1]]
set FMAX [expr {1000.0 / ($CLK_PERIOD - $WNS)}]

set TIMING_OK [expr {$WNS >= 0 && $WHS >= 0}]

set fh [open $OUT/summary.csv w]
puts $fh "k_max,lut,ff,bram,dsp,wns_ns,whs_ns,fmax_mhz,timing_met"
puts $fh [format "%d,%s,%s,%s,%s,%.3f,%.3f,%.2f,%d" \
             $KMAX $LUT $FF $BRAM $DSP $WNS $WHS $FMAX $TIMING_OK]
close $fh

puts "========================================"
puts " K_MAX=$KMAX  LUT=$LUT  FF=$FF  BRAM=$BRAM  DSP=$DSP"
puts " WNS=$WNS ns   WHS=$WHS ns   Fmax=[format %.2f $FMAX] MHz"
if {$TIMING_OK} {
    puts " TIMING MET"
} else {
    puts " WARNING: TIMING NOT MET -- Fmax above is the achievable"
    puts " frequency, but this bitstream is not safe to run at 100 MHz."
}
puts " summary: $OUT/summary.csv"
puts "========================================"

# A bitstream is only meaningful if timing closed. Writing one regardless
# would invite programming a board with a design that fails setup.
if {$TIMING_OK} {
    write_bitstream -force $OUT/${TOP}_k${BITTAG}.bit
    puts " bitstream: $OUT/${TOP}_k${BITTAG}.bit"
    puts " program:   vivado -mode batch -source program_kmax.tcl -tclargs $BITTAG"
} else {
    puts " bitstream SKIPPED (timing not met)"
}