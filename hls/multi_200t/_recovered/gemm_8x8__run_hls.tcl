open_project systolic_proj
set_top matmul_8x8x8
add_files design.cpp
add_files -tb testbench.cpp
open_solution "solution1" -flow_target vivado
set_part {xc7a35ticsg324-1L}
create_clock -period 10 -name default

csim_design
csynth_design
export_design -rtl verilog -format ip_catalog
exit
