open_hw_manager
connect_hw_server
open_hw_target
set dev [lindex [get_hw_devices] 0]
set_property PROGRAM.FILE {./baseline_40mhz/matmul_8x8x8_rk_40mhz_K1to64.bit} $dev
program_hw_devices $dev
close_hw_target
