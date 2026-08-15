foreach f {fp_mul fp_add systolic_pe_fold} { read_verilog -sv $f.sv }
read_ip rtl_fp_pe_test/rtl_fp_pe_test.srcs/sources_1/ip/floating_point_add_0/floating_point_add_0.xci
read_ip rtl_fp_pe_test/rtl_fp_pe_test.srcs/sources_1/ip/floating_point_mul_0/floating_point_mul_0.xci
synth_design -top systolic_pe_fold -part xc7a200tsbg484-1
report_utilization -file /tmp/util_pe.rpt
