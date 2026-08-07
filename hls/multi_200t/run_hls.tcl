# run_hls.tcl -- csim / csynth / cosim for the runtime-K systolic array.
#
#   vitis_hls -f run_hls.tcl                    ;# all three, K = TB_K default
#   vitis_hls -f run_hls.tcl -tclargs 32        ;# one K
#   vitis_hls -f run_hls.tcl -tclargs 32 csynth ;# stop after synthesis
#
# The three stages answer different questions and are worth keeping
# separate:
#
#   csim   -- is the schedule correct? Pure C++, no timing. Fast. If this
#             fails, nothing downstream means anything.
#   csynth -- what II did the tool ACHIEVE? This is the number the whole
#             partition rewrite was about. It is an estimate, but II and the
#             timing slack are reported honestly here.
#   cosim  -- what does the RTL actually take, in cycles, end to end? This
#             is the only stage that measures the +6 handshake/init/drain
#             overhead rather than assuming it.
#
# csynth latency is an ESTIMATE and for a variable-bound loop it is derived
# from the TRIPCOUNT pragma -- i.e. from numbers you supplied. It cannot
# confirm the cost model. Only cosim can. Do not quote csynth latency as a
# measurement.

set K_UNDER_TEST 64
set STOP_AFTER   "cosim"
if {$argc >= 1} { set K_UNDER_TEST [lindex $argv 0] }
if {$argc >= 2} { set STOP_AFTER   [lindex $argv 1] }

# ---- TODO: set these to your board/clock before the first run -----------
# The part and clock decide whether the operand-bank read lands in one cycle
# and whether II=1 closes timing. Numbers gathered under a different part
# are not comparable.
set PART   "xcu250-figd2104-2L-e"
set PERIOD 3.33
# ------------------------------------------------------------------------

open_project -reset hls_k${K_UNDER_TEST}
set_top matmul_4x4x4

add_files design.cpp -cflags "-std=c++14 -DTB_K=${K_UNDER_TEST}"
add_files -tb design_tb.cpp \
    -cflags "-std=c++14 -DTB_K=${K_UNDER_TEST} -DCSIM_ONLY -ffp-contract=off"

open_solution -reset "sol1" -flow_target vivado
set_part $PART
create_clock -period $PERIOD -name default

# csim builds the TB with -DCSIM_ONLY, so the edge cases and the fold check
# run here and only here.
csim_design -clean

if {$STOP_AFTER eq "csim"} { puts "stopped after csim"; exit }

csynth_design

if {$STOP_AFTER eq "csynth"} { puts "stopped after csynth"; exit }

# cosim rebuilds the TB WITHOUT -DCSIM_ONLY (see the cflags on the solution
# below) so exactly one kernel call happens and the reported latency is
# unambiguously the K under test.
#
# -trace_level none keeps it fast. Switch to "all" only when you need to
# open the waveform to see why II slipped -- it is dramatically slower and
# writes a large wdb.
cosim_design -trace_level none -rtl verilog

puts "=========================================================="
puts " K = ${K_UNDER_TEST}"
puts " csynth report : hls_k${K_UNDER_TEST}/sol1/syn/report/matmul_4x4x4_csynth.rpt"
puts "   -> read the time_loop row: 'Initiation Interval achieved'"
puts " cosim report  : hls_k${K_UNDER_TEST}/sol1/sim/report/matmul_4x4x4_cosim.rpt"
puts "   -> Latency min/avg/max, in cycles. This is the measurement."
puts "=========================================================="
