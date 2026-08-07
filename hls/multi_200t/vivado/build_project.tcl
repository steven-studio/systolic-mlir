set proj_name "matmul_nexys_8x8_dual"
set part_name "xc7a200tsbg484-1"
set hls_ip "../work_systolic/hls/impl/ip"

create_project $proj_name ./build -part $part_name -force

add_files -norecurse {
    ../rtl/matmul_top_rk.v
    ../rtl/matmul_iface_rk.v
    ../rtl/uart_rx.v
    ../rtl/uart_tx.v
    ../rtl/clk_gen.v
}
add_files [glob $hls_ip/hdl/verilog/*.v]

import_ip [glob $hls_ip/hdl/ip/*/*.xci]
generate_target all [get_ips]
set_property GENERATE_SYNTH_CHECKPOINT true [get_files -all *.xci]
synth_ip [get_ips]

add_files -fileset constrs_1 -norecurse nexys_video.xdc

set_property top matmul_top_rk [current_fileset]
update_compile_order -fileset sources_1

launch_runs synth_1 -jobs 4
wait_on_run synth_1
if {[get_property PROGRESS [get_runs synth_1]] != "100%"} {
    puts "ERROR: synthesis 失敗"; exit 1
}
puts "INFO: synthesis 完成"

launch_runs impl_1 -to_step write_bitstream -jobs 4
wait_on_run impl_1
if {[get_property PROGRESS [get_runs impl_1]] != "100%"} {
    puts "ERROR: implementation/bitstream 失敗"; exit 1
}
puts "INFO: bitstream 產生完成"
