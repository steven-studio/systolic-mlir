# clock_explore_new.tcl -- find what frequency this design can actually reach.
#
# WHY THIS AND NOT THE FANOUT FIX
#
# The fanout run changed nothing, and the log says why:
#
#   phys_opt_design: Design worst setup slack (WNS) is greater than or equal
#   to 0.000 ns. All physical synthesis setup optimizations will be skipped.
#
# Vivado optimises against the constraint it is given. The kernel clock is
# constrained at 50 ns (20 MHz), the placer hit that with 25 ns to spare, and
# every subsequent optimisation step correctly decided there was nothing to
# do. The fo=2048 net is 34.5 ns long because nothing asked it to be shorter
# -- not because it cannot be.
#
# So "fmax = 1/(50 - WNS) = 27.7 MHz" is wrong. That formula measures one
# particular relaxed placement, not the design's capability. To find the
# capability you have to constrain harder and see where it breaks.
#
# WHAT THIS DOES
#
# Overrides the auto-derived MMCM output clock with a tighter period, then
# re-runs place & route from the existing synthesis checkpoint. No RTL edit,
# no re-synthesis, and the project runs are left alone.
#
# *** THE RESULTING CHECKPOINT MUST NOT BE PROGRAMMED. ***
# clk_gen.v still has CLKOUT0_DIVIDE_F = 50.0, so the hardware would still
# produce 20 MHz. This constraint is a lie told to the timing engine on
# purpose, to measure headroom. Once you know the answer, change
# CLKOUT0_DIVIDE_F to match and rebuild properly.
#
# Usage:
#   vivado -mode batch -source clock_explore_new.tcl -tclargs 50   ;# try 50 MHz
#   vivado -mode batch -source clock_explore_new.tcl -tclargs 100  ;# try 100 MHz
#
# Suggested order: 50, then bisect. Jumping straight to 100 tells you only
# that it failed, not by how much that matters.

set TARGET_MHZ 50
if {$argc >= 1} { set TARGET_MHZ [lindex $argv 0] }

set BUILD $::env(HOME)/systolic-mlir/hls/multi_200t/vivado/build

# NOT the synth_1 checkpoint. The 256 fadd/fmul cores are synthesised
# out-of-context into their own .dcp files, so in synth_1/matmul_top_dual.dcp
# they are unresolved black boxes -- open_checkpoint does not pull them in
# (that is link_design's job, and it runs at the start of impl). Loading it
# directly gives 256 "DRC INBB-3 ... considered a black box" errors before
# opt_design can start.
#
# impl_1/matmul_top_dual_opt.dcp is the right starting point: link_design and
# opt_design have already run, so the IP is resolved, and nothing is placed
# yet -- which is exactly the state we want to re-place from.
set STARTDCP $BUILD/matmul_nexys_8x8_dual.runs/impl_1/matmul_top_dual_opt.dcp

if {![file exists $STARTDCP]} {
    puts "ERROR: no post-opt checkpoint at $STARTDCP"
    puts "       Run the normal impl flow once first (build_project.tcl)."
    exit 1
}

open_checkpoint $STARTDCP

if {[llength [get_cells -quiet -hier -filter {IS_BLACKBOX}]] > 0} {
    puts "ERROR: black boxes still present -- wrong checkpoint."
    exit 1
}

# The MMCM is VCO = 100 MHz * 10 / 1 = 1000 MHz, then / CLKOUT0_DIVIDE_F.
# Re-declaring the generated clock on the same output pin replaces the
# auto-derived one.
# create_generated_clock -divide_by only takes an integer, so the achievable
# targets are 1000/N MHz. Truncating silently (int()) is a trap: asking for 45
# gives 22.222 -> 22 -> an actual constraint of 45.45 MHz, while every message
# printed afterwards still says 45. That is conservative here, but for a target
# that rounds the other way it would over-report the achievable frequency.
#
# So: round, then report the frequency that was ACTUALLY constrained.
set DIV_EXACT [expr {1000.0 / $TARGET_MHZ}]
set DIV       [expr {round($DIV_EXACT)}]
set ACTUAL_MHZ [expr {1000.0 / $DIV}]

if {abs($ACTUAL_MHZ - $TARGET_MHZ) > 0.01} {
    puts "NOTE: ${TARGET_MHZ} MHz is not of the form 1000/N."
    puts "      Constraining ${ACTUAL_MHZ} MHz instead (divide_by $DIV)."
}
puts "INFO: kernel clock constrained to ${ACTUAL_MHZ} MHz (divide_by $DIV)"

create_generated_clock -name clk_kernel_explore \
    -source [get_pins u_clk_gen/u_mmcm/CLKIN1] \
    -multiply_by 10 -divide_by $DIV \
    [get_pins u_clk_gen/u_mmcm/CLKOUT0]

# opt_design is already baked into the checkpoint; re-running it here would
# just repeat work. Straight to placement, which is where the new constraint
# actually changes the outcome.
place_design
# This time phys_opt should actually engage -- if WNS is negative it has work
# to do, and high-fanout replication is one of the things it does. If it
# STILL reports "no setup violation", the target was too easy; go tighter.
phys_opt_design -directive AggressiveExplore
route_design
phys_opt_design -directive AggressiveExplore

set RPT explore_${TARGET_MHZ}mhz
report_timing_summary -file ${RPT}_timing.rpt
report_utilization    -file ${RPT}_util.rpt
report_timing -max_paths 3 -file ${RPT}_worst_paths.rpt

set period [expr {1000.0 / $ACTUAL_MHZ}]
set wns    [get_property SLACK [get_timing_paths -delay_type max]]

puts "############################################################"
puts "  constrained   : ${ACTUAL_MHZ} MHz  (period ${period} ns)"
puts "  WNS           : $wns ns"
if {$wns >= 0} {
    puts "  RESULT        : MET -- the design can do at least ${ACTUAL_MHZ} MHz."
    if {$wns < 0.5} {
        puts "  *** WARNING: margin is only ${wns} ns. This closed, but it closed"
        puts "      with nothing to spare -- any later change to the design will"
        puts "      break it. Do NOT pick this frequency for the shipping build."
    }
} else {
    puts "  RESULT        : MISSED by [expr {-$wns}] ns."
    puts "                  achievable here: [expr {1000.0/($period - $wns)}] MHz"
    puts "                  Read ${RPT}_worst_paths.rpt -- if route delay still"
    puts "                  dominates logic, fanout replication is now worth"
    puts "                  retrying, because phys_opt finally has a reason to run."
}
puts ""
puts "  baseline      : 20 MHz constrained, WNS 13.873"
puts "  DO NOT PROGRAM this checkpoint -- clk_gen.v still divides to 20 MHz."
puts "############################################################"
