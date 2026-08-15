# run_cosim_new.tcl -- actually run cosim, via the classic vitis_hls flow.
#
# WHY NOT THE .cfg FLOW
#
# `cosim=1` in the [hls] section is rejected:
#     WARNING: [HLS 200-1921] Skipping unknown config ini [hls] entry 'cosim=1'
# and the run silently proceeds to csynth + export IP without simulating
# anything. `cosim.trace_level` IS accepted, so the namespace exists -- but a
# sub-option does not enable the step. That silent skip is the trap: the run
# looks successful and produces no cosim report.
#
# `cosim.enable=1` may be the correct key in this release. Rather than guess,
# this uses vitis_hls -f, where `cosim_design` is an explicit command that
# either runs or errors. No ambiguity about whether simulation happened.
#
# Usage (from ~/systolic-mlir/hls/multi_200t):
#   vitis_hls -f run_cosim_new.tcl
#
# Filename has a _new suffix so it cannot clobber anything in the directory.

# Mirrors hls_8x8.cfg exactly. If these drift apart the cosim number stops
# describing the design you actually build, so re-check them against the cfg
# whenever the cfg changes.
set PART      xc7a200tsbg484-1
set CLK       10
set CLK_UNC   12.5%
set TOP       matmul_4x4x4
set CFLAGS    "-DR=8 -DC=8 -DK_DIM=8"

open_project -reset cosim_prj_new
set_top $TOP
add_files design.cpp -cflags $CFLAGS
add_files -tb testbench.cpp -cflags $CFLAGS

open_solution -reset "sol1" -flow_target vivado
set_part $PART
create_clock -period $CLK -name default
set_clock_uncertainty $CLK_UNC

# Same directive the cfg applies. Kept here rather than in the source so the
# II can be swept without editing design.cpp.
set_directive_pipeline -II 1 "$TOP/time_loop"

csim_design
csynth_design

# -trace_level none keeps this fast; there is no waveform to look at yet and
# the trace files for a 128-PE design are large. Switch to `all` only if the
# cycle count comes out wrong and you need to see why.
cosim_design -trace_level none -rtl verilog

exit
