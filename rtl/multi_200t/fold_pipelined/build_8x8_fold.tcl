set PART "xc7a200tsbg484-1"
set TOP  "systolic_uart_fold_top"

file mkdir build_8x8_fold
file mkdir build_8x8_fold/reports

puts "========================================"
puts " Reading RTL"
puts "========================================"

read_verilog -sv uart_rx.sv
read_verilog -sv uart_tx.sv

read_verilog -sv fp_mul.sv
read_verilog -sv fp_add.sv
read_verilog -sv fp_reduce16.sv

read_verilog -sv systolic_pe_fold.sv
read_verilog -sv systolic_array_8x8_fold.sv

read_verilog -sv systolic_uart_fold_top.sv

puts "========================================"
puts " Reading floating-point IP"
puts "========================================"

read_ip rtl_fp_pe_test/rtl_fp_pe_test.srcs/sources_1/ip/floating_point_add_0/floating_point_add_0.xci
read_ip rtl_fp_pe_test/rtl_fp_pe_test.srcs/sources_1/ip/floating_point_mul_0/floating_point_mul_0.xci

puts "========================================"
puts " Reading constraints"
puts "========================================"

read_xdc nexys_video_uart.xdc

puts "========================================"
puts " Synthesis"
puts "========================================"

synth_design \
    -top $TOP \
    -part $PART

write_checkpoint -force \
    build_8x8_fold/post_synth.dcp

report_utilization \
    -file build_8x8_fold/reports/post_synth_utilization.rpt

report_timing_summary \
    -file build_8x8_fold/reports/post_synth_timing.rpt

puts "========================================"
puts " Optimization"
puts "========================================"

opt_design

puts "========================================"
puts " Placement"
puts "========================================"

place_design

write_checkpoint -force \
    build_8x8_fold/post_place.dcp

report_timing_summary \
    -file build_8x8_fold/reports/post_place_timing.rpt

puts "========================================"
puts " Routing"
puts "========================================"

route_design

write_checkpoint -force \
    build_8x8_fold/post_route.dcp

report_timing_summary \
    -file build_8x8_fold/reports/post_route_timing.rpt

report_utilization \
    -file build_8x8_fold/reports/post_route_utilization.rpt

report_drc \
    -file build_8x8_fold/reports/post_route_drc.rpt

puts "========================================"
puts " Timing status"
puts "========================================"

set WNS [get_property SLACK \
    [get_timing_paths -delay_type max -max_paths 1]]

puts "FINAL WNS = $WNS ns"

if {$WNS < 0} {
    puts "WARNING: Timing is NOT met."
} else {
    puts "PASS: Timing met."
}

puts "========================================"
puts " Writing bitstream"
puts "========================================"

write_bitstream -force \
    build_8x8_fold/systolic_uart_fold_top.bit

puts ""
puts "========================================"
puts " BUILD COMPLETE"
puts " Bitstream:"
puts " build_8x8_fold/systolic_uart_fold_top.bit"
puts "========================================"
