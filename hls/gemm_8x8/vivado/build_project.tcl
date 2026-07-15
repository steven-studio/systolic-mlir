set proj_name "matmul_arty"
set part_name "xc7a35ticsg324-1L"

create_project $proj_name ./build -part $part_name -force

# 加入手寫的 RTL 跟 HLS 匯出的 Verilog(不含 fadd/fmul 的 .vhd 黑盒實作檔)
add_files -norecurse {
    ../rtl/matmul_top.v
    ../rtl/matmul_iface.v
    ../rtl/uart_rx.v
    ../rtl/uart_tx.v
}
add_files [glob ../systolic_proj/solution1/impl/ip/hdl/verilog/*.v]

# 正規方式匯入 IP:透過 .xci 檔案,讓 Vivado 走完整的 IP 管理流程
read_ip [glob ../systolic_proj/solution1/impl/ip/hdl/ip/*/*.xci]

# 產生 IP 的所有輸出檔(包含真正可綜合的內容,不只是黑盒宣告)
generate_target all [get_ips]

# 啟用 out-of-context synthesis checkpoint 產生
set_property GENERATE_SYNTH_CHECKPOINT true [get_files -all *.xci]

# 對每個 IP 各自跑 out-of-context synthesis,產生完整實作
synth_ip [get_ips]

# 加入 XDC constraint
add_files -fileset constrs_1 -norecurse arty_a7.xdc

set_property top matmul_top [current_fileset]
update_compile_order -fileset sources_1

launch_runs synth_1 -jobs 4
wait_on_run synth_1

if {[get_property PROGRESS [get_runs synth_1]] != "100%"} {
    puts "ERROR: synthesis 失敗"
    exit 1
}
puts "INFO: synthesis 完成"

launch_runs impl_1 -to_step write_bitstream -jobs 4
wait_on_run impl_1

if {[get_property PROGRESS [get_runs impl_1]] != "100%"} {
    puts "ERROR: implementation/bitstream 失敗"
    exit 1
}
puts "INFO: bitstream 產生完成"
