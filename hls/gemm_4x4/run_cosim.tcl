# run_cosim.tcl -- 4x4 float32 systolic array: csim, csynth, cosim.
#
#   vitis_hls -f run_cosim.tcl
#
# The number to read is "Latency (cycles)" in the COSIM report:
#   systolic_proj_4x4/solution1/sim/report/verilog/*.rpt
# not the csynth estimate.
#
# Also read the achieved Initiation Interval for time_loop out of
# csynth.rpt. With a float accumulator II is bounded by the adder
# latency, so expect II=2 rather than 1, and
#     latency ~= II * (K + rows + cols - 2) + pipeline depth
#             = II * 10 + depth
# The geometry term (rows+cols-2 = 6) is unchanged by II; only the
# per-beat cost scales. Report the measured II in the paper rather than
# assuming II=1.

open_project -reset systolic_proj_4x4
set_top matmul_4x4x4

add_files design.cpp
add_files -tb testbench.cpp

open_solution -reset "solution1" -flow_target vivado

# Match your board. Arty A7-100T shown; use xc7a35ticsg324-1L for a 35T.
set_part {xc7a100tcsg324-1}
create_clock -period 10 -name default

csim_design
csynth_design
cosim_design -trace_level none -rtl verilog

exit