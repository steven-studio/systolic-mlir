open_hw_manager
connect_hw_server
open_hw_target
set dev [lindex [get_hw_devices] 0]
set_property PROGRAM.FILE {./build/matmul_arty_4x4.runs/impl_1/matmul_top.bit} $dev
program_hw_devices $dev
close_hw_target
