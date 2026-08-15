foreach f {fp_mul fp_add systolic_pe_fold systolic_array_fold systolic_array_8x8_fold
           systolic_array_4x4_fold} { read_verilog -sv $f.sv }
read_verilog -sv /tmp/dual_probe.sv
read_ip rtl_fp_pe_test/rtl_fp_pe_test.srcs/sources_1/ip/floating_point_add_0/floating_point_add_0.xci
read_ip rtl_fp_pe_test/rtl_fp_pe_test.srcs/sources_1/ip/floating_point_mul_0/floating_point_mul_0.xci
synth_design -top dual_probe -part xc7a200tsbg484-1
report_utilization -file /tmp/util_dual.rpt
report_control_sets -file /tmp/csets_dual.rpt
