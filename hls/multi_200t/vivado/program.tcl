open_hw_manager
connect_hw_server
open_hw_target
set dev [lindex [get_hw_devices] 0]
set_property PROGRAM.FILE {./build/matmul_nexys_8x8_dual.runs/impl_1/matmul_top_rk.bit} $dev
program_hw_devices $dev
close_hw_target
