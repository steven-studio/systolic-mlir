
# 2x 4x4 fixed-point arrays. Clone of build_project.tcl with:

#   top = matmul_top_dual (adds device-select byte to the UART protocol)

set proj_name "matmul_arty_4x4_dual"

set part_name "xc7a35ticsg324-1L"

create_project $proj_name ./build_dual -part $part_name -force

add_files -norecurse {

    ../rtl/matmul_top_dual.v

    ../rtl/matmul_iface.v

    ../rtl/uart_rx.v

    ../rtl/uart_tx.v

    ../rtl/clk_gen.v

}

add_files [glob ../work_fx_ii1/hls/impl/ip/hdl/verilog/*.v]

import_ip [glob ../work_fx_ii1/hls/impl/ip/hdl/ip/*/*.xci]

generate_target all [get_ips]

set_property GENERATE_SYNTH_CHECKPOINT true [get_files -all *.xci]

synth_ip [get_ips]

add_files -fileset constrs_1 -norecurse arty_a7.xdc

set_property top matmul_top_dual [current_fileset]

update_compile_order -fileset sources_1

launch_runs synth_1 -jobs 8

wait_on_run synth_1

if {[get_property PROGRESS [get_runs synth_1]] != "100%"} { puts "ERROR: synthesis 失敗"; exit 1 }

puts "INFO: synthesis 完成"

launch_runs impl_1 -to_step write_bitstream -jobs 8

wait_on_run impl_1

if {[get_property PROGRESS [get_runs impl_1]] != "100%"} { puts "ERROR: bitstream 失敗"; exit 1 }

puts "INFO: bitstream 產生完成"

