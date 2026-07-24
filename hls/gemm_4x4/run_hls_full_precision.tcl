open_project systolic_proj
set_top matmul_4x4x4
add_files design.cpp
add_files -tb testbench.cpp
open_solution "solution1_full_precision" -flow_target vivado
set_part {xc7a35ticsg324-1L}
create_clock -period 10 -name default

# Pin the floating-point multiply and add units to "Full Usage" of DSP
# resources (impl=fulldsp), instead of Vitis HLS's default (impl=all, which
# lets the tool choose -- typically resolving to Medium/Low usage for
# basic ops), to test whether this is the source of the 7-30 ULP baseline
# error reported in Section eval-correctness. A separate solution name
# (solution1_full_precision, not solution1) keeps the original synthesis
# results/reports intact for a side-by-side comparison of resource
# utilization and measured ULP error.
config_op fmul -impl fulldsp
config_op fadd -impl fulldsp

csim_design
csynth_design
export_design -rtl verilog -format ip_catalog
exit
