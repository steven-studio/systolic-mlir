# timing_fix_new.tcl -- attack the fo=2048 control net.
#
# Diagnosis this is based on (post-route, dual 8x8 float, 20 MHz):
#
#   Data Path Delay: 35.922ns  logic 0.715ns (2%)  route 35.207ns (98%)
#   Logic Levels:    1 (LUT6)
#   net (fo=2048, routed) 34.567ns
#     src u_iface1/u_matmul/first_iter_0_reg_10286_pp0_iter3_reg_reg[0]
#     dst .../fadd_32ns_32ns_32_2_full_dsp_1_U37/din0_buf1_reg[11]
#
# One net is 96% of the critical path. The logic is not the problem and
# neither is the float latency -- a single flip-flop is driving 2048 loads
# spread across the whole die, because 640 of 740 DSPs are occupied and the
# 128 PEs are therefore pinned across every DSP column on the part.
#
# Do these one at a time and re-measure between each. Stacking all three and
# reading one number tells you nothing about which one worked.
#
# Usage:
#   vivado -mode batch -source timing_fix_new.tcl

# create_project matmul_nexys_8x8_dual ./build  puts the .xpr directly in
# build/, not in a per-project subdirectory. Globbing rather than hardcoding
# so a renamed project does not silently fail here. Note that open_project
# does NOT expand "~" -- use $::env(HOME).
set BUILD_DIR $::env(HOME)/systolic-mlir/hls/multi_200t/vivado/build
set candidates [glob -nocomplain $BUILD_DIR/*.xpr]
if {[llength $candidates] != 1} {
    puts "ERROR: expected exactly one .xpr in $BUILD_DIR, found: $candidates"
    exit 1
}
set PROJ [lindex $candidates 0]
puts "INFO: opening $PROJ"
open_project $PROJ

# Property names for run steps move between Vivado releases -- 2026.1 has no
# STEPS.SYNTH_DESIGN.ARGS.FANOUT_LIMIT, for instance. Setting them blind
# aborts the script three seconds in. This wrapper reports and continues, so
# one missing property does not cost a whole run.
proc try_set {obj prop val} {
    if {[catch {set_property $prop $val $obj} err]} {
        puts "WARN: could not set '$prop' -- skipped. ($err)"
        return 0
    }
    puts "INFO: set '$prop' = $val"
    return 1
}

# ---------------------------------------------------------------------
# A1. Let synthesis replicate high-fanout drivers.
# ---------------------------------------------------------------------
# The default fanout limit lets a net reach ~10000 loads before synthesis
# considers replication. At 2048 loads across a fully-spread placement that
# is far too permissive. 64 is aggressive, but this design has 69% of its
# LUTs free -- the replicas are affordable.
#
# Passed via MORE OPTIONS rather than a dedicated property: that string goes
# straight to synth_design, so it does not depend on a property name that
# may not exist in this release.
try_set [get_runs synth_1] {STEPS.SYNTH_DESIGN.ARGS.MORE OPTIONS} \
        {-fanout_limit 64 -control_set_opt_threshold 2}

# ---------------------------------------------------------------------
# A2. Physical optimisation with forced replication.
# ---------------------------------------------------------------------
# Synthesis replicates before placement, so it is guessing about distance.
# phys_opt_design runs after placement and knows the actual geometry, which
# is what matters when the loads are spread over the whole die. This is the
# targeted fix for exactly this failure mode.
try_set [get_runs impl_1] STEPS.PHYS_OPT_DESIGN.IS_ENABLED true
try_set [get_runs impl_1] STEPS.PHYS_OPT_DESIGN.ARGS.DIRECTIVE AggressiveExplore
try_set [get_runs impl_1] STEPS.POST_ROUTE_PHYS_OPT_DESIGN.IS_ENABLED true
try_set [get_runs impl_1] STEPS.POST_ROUTE_PHYS_OPT_DESIGN.ARGS.DIRECTIVE AggressiveExplore

# ---------------------------------------------------------------------
# A3. Name the offending register explicitly.
# ---------------------------------------------------------------------
# Belt and braces: even with the global limit, HLS control registers
# sometimes survive replication because they sit behind a hierarchy
# boundary. This pins MAX_FANOUT on the specific family of registers.
#
# Wildcarded because the numeric suffix (10286) is regenerated on every
# synthesis run -- hardcoding it would silently stop matching.
set targets [get_cells -quiet -hier -filter {NAME =~ *first_iter*reg*}]
puts "INFO: MAX_FANOUT targets matched: [llength $targets]"
if {[llength $targets] > 0} {
    set_property MAX_FANOUT 32 $targets
} else {
    # Zero matches means the netlist names differ from the post-route report
    # -- worth knowing now rather than after a 40-minute run that changed
    # nothing.
    puts "WARN: no cells matched *first_iter*reg* -- A3 had no effect."
}

reset_run synth_1
launch_runs synth_1 -jobs 8
wait_on_run synth_1
if {[get_property PROGRESS [get_runs synth_1]] != "100%"} {
    puts "ERROR: synthesis failed"; exit 1
}

launch_runs impl_1 -to_step route_design -jobs 8
wait_on_run impl_1
open_run impl_1

report_utilization    -file post_route_util_fanoutfix.rpt
report_timing_summary -file post_route_timing_fanoutfix.rpt

# The number that matters, printed where you cannot miss it.
set wns [get_property SLACK [get_timing_paths -delay_type max]]
puts "############################################################"
puts "  WNS at 50ns period : $wns ns"
puts "  implied fmax       : [expr {1000.0/(50.0 - $wns)}] MHz"
puts ""
puts "  baseline was       : 13.873 ns  ->  27.7 MHz"
puts "############################################################"
puts ""
puts "If fmax barely moved, re-read the new worst path before trying"
puts "anything else -- a different net may now dominate, and the fix for"
puts "that one is probably not replication."