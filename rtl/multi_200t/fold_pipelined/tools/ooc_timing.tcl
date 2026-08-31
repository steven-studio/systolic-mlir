# -----------------------------------------------------------------------------
# ooc_timing.tcl -- out-of-context timing check for the DMA-side modules.
#
#   vivado -mode batch -source tools/ooc_timing.tcl
#
# Lives in tools/ and resolves every path from its own location, like
# build_kmax.tcl, so it can be run from any working directory.
#
# Answers one question: do dma_engine and the write side of dma_cdc_fifo close
# at the MIG user-interface clock (200 MHz for a 4:1 ratio on 16-bit DDR3)?
# Nothing here needs MIG, the board, or the array -- it is pure logic timing,
# so it can be run before any of the remaining modules exist.
#
# WHAT THIS DOES NOT ANSWER.  The array itself does not need 200 MHz and cannot
# reach it (fmax 112 MHz at N=8, measured).  That is not a problem to be fixed;
# it is the reason dma_cdc_fifo exists.  Running the whole design at one clock
# is not an option, so do not read a failing array-side number here as a blocker.
#
# Two passes:
#   SYNTH ONLY  -- fast, optimistic (no routing delay).  Use it to catch a path
#                  that is hopeless.  A positive WNS here is necessary, not
#                  sufficient.
#   FULL OOC    -- place and route out of context.  Slower, and the number you
#                  should actually believe.  Set RUN_IMPL to 1.
# -----------------------------------------------------------------------------

# Paths are resolved from this script's location: tools/ -> fold_pipelined/
set SCRIPT_DIR [file dirname [file normalize [info script]]]
set ROOT       [file dirname $SCRIPT_DIR]

set PART      xc7a200tsbg484-1
set UI_PERIOD 5.000    ;# 200 MHz, MIG ui_clk
set AR_PERIOD 10.000   ;# 100 MHz, array clock
set RUN_IMPL  1        ;# 0 = synthesis only (fast), 1 = also place and route
set OUTDIR    $ROOT/ooc_reports

file mkdir $OUTDIR

# -----------------------------------------------------------------------------
proc check_module {top files clk_specs async_groups outdir run_impl part} {
    puts "\n=========================================================="
    puts "  $top"
    puts "==========================================================\n"

    create_project -in_memory -part $part
    read_verilog -sv $files
    synth_design -top $top -part $part -mode out_of_context

    # clocks
    foreach spec $clk_specs {
        lassign $spec name port period
        create_clock -name $name -period $period [get_ports $port]
    }
    # asynchronous groups, so the tool does not time the CDC paths as if they
    # were synchronous and drown the report in false failures
    if {[llength $async_groups] == 2} {
        set_clock_groups -asynchronous \
            -group [get_clocks [lindex $async_groups 0]] \
            -group [get_clocks [lindex $async_groups 1]]
    }

    report_timing_summary -max_paths 5 -file $outdir/${top}_synth_timing.rpt
    report_utilization            -file $outdir/${top}_synth_util.rpt

    set wns [get_property SLACK [get_timing_paths -max_paths 1 -delay_type max]]
    puts "  \[synth\] WNS = $wns ns"

    if {$run_impl} {
        opt_design
        place_design
        phys_opt_design
        route_design
        report_timing_summary -max_paths 10 -file $outdir/${top}_impl_timing.rpt
        report_utilization              -file $outdir/${top}_impl_util.rpt
        set wns [get_property SLACK [get_timing_paths -max_paths 1 -delay_type max]]
        puts "  \[impl \] WNS = $wns ns   <-- believe this one"
    }

    close_project
}

# -----------------------------------------------------------------------------
# 1. the read engine: single clock, must close at ui_clk
check_module dma_engine \
    [list $ROOT/dma/dma_engine.sv] \
    [list [list ui_clk clk $UI_PERIOD]] \
    {} \
    $OUTDIR $RUN_IMPL $PART

# 2. the CDC FIFO: write side at ui_clk, read side at the array clock
check_module dma_cdc_fifo \
    [list $ROOT/dma/dma_cdc_fifo.sv] \
    [list [list ui_clk wclk $UI_PERIOD] [list ar_clk rclk $AR_PERIOD]] \
    {ui_clk ar_clk} \
    $OUTDIR $RUN_IMPL $PART

# 3. the throttle at the ARRAY clock -- it lives on the 100 MHz side, but the
#    32-bit adder and comparator it puts in the step-enable path is the one
#    place the insertion could cost timing, so check it explicitly.
if {[file exists $ROOT/core/operand_throttle.sv]} {
    check_module operand_throttle \
        [list $ROOT/core/operand_throttle.sv] \
        [list [list ar_clk clk $AR_PERIOD]] \
        {} \
        $OUTDIR $RUN_IMPL $PART
}

puts "\nReports in $OUTDIR"
