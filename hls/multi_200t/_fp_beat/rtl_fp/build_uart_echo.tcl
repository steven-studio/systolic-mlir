create_project -force uart_echo_build ./uart_echo_build \
    -part xc7a200tsbg484-1

set_property target_language Verilog [current_project]

add_files [list \
    uart_rx.sv \
    uart_tx.sv \
    uart_echo_top.sv \
]

add_files -fileset constrs_1 uart_echo.xdc

set_property top uart_echo_top [current_fileset]

update_compile_order -fileset sources_1

puts "===== SYNTHESIS ====="

synth_design \
    -top uart_echo_top \
    -part xc7a200tsbg484-1

write_checkpoint -force uart_echo_build/post_synth.dcp

puts "===== OPT ====="
opt_design

puts "===== PLACE ====="
place_design

puts "===== ROUTE ====="
route_design

puts "===== TIMING ====="
report_timing_summary

puts "===== BITSTREAM ====="
write_bitstream -force uart_echo_build/uart_echo_top.bit

puts ""
puts "========================================"
puts " UART ECHO BUILD COMPLETE"
puts " uart_echo_build/uart_echo_top.bit"
puts "========================================"
