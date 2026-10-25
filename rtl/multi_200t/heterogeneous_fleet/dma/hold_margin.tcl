# -----------------------------------------------------------------------------
# hold_margin.tcl -- pre-place hook: 0.15 ns of extra hold pessimism on ui_clk.
#
# WHY THIS EXISTS AT ALL
#   uart/build_kmax.tcl carries this, with the reason written out: on 2026-08-19
#   one clean configuration produced a Default placement that was dead on the
#   board (RX 0/512) and an Explore placement that was bit-exact.  Same RTL,
#   same parameters -- only the placement differed, and the dead one had WHS in
#   the 0.02 ns range against an STA whose premise was already flawed (the OOC
#   clock for the floating-point IP is 100 ns).  Forcing implementation to leave
#   0.15 ns of hold margin stopped the design depending on a placement lottery.
#
#   systolic_dma_top instantiates the SAME array through systolic_uart_top, so
#   it inherits the same exposure.  Dropping this because the DMA build happens
#   to use a project flow rather than an in-memory one would quietly reintroduce
#   a failure mode that took a day to diagnose.
#
# WHY ONLY ui_clk
#   build_kmax.tcl applies it to [get_clocks] because in that design there is
#   one clock.  Here there is a MIG, and its DDR3 interface clocks come with
#   constraints Xilinx tuned; adding pessimism to those is not a thing to do
#   casually.  Every flip-flop of the array, the DMA and the sequencer is on
#   ui_clk, so ui_clk is the whole of what needs the margin.
#
# It FAILS LOUDLY if it cannot find ui_clk.  A hook that silently does nothing
# is worse than no hook: the build would look identical and the protection
# would be gone.
# -----------------------------------------------------------------------------

set ui_nets [get_nets -quiet -hierarchical -filter {NAME =~ *ui_clk*}]
set ui_clks [get_clocks -quiet -of_objects $ui_nets]

# Fall back to matching the clock's own name, in case the net search comes back
# empty because of how MIG names things in this Vivado version.
if {[llength $ui_clks] == 0} {
    set ui_clks [get_clocks -quiet *ui_clk*]
}

if {[llength $ui_clks] == 0} {
    puts "\n=== hold_margin.tcl ==========================================="
    puts "  Could not find a ui_clk clock.  Clocks in this design:"
    foreach c [get_clocks -quiet] {
        puts [format "    %-28s period %s" $c [get_property PERIOD $c]]
    }
    puts "==============================================================="
    error "hold_margin.tcl: no ui_clk -- refusing to place without the margin"
}

set_clock_uncertainty -hold 0.150 $ui_clks

puts "\n=== hold_margin.tcl ==========================================="
puts "  0.150 ns hold uncertainty applied to:"
foreach c $ui_clks {
    puts [format "    %-28s period %s ns" $c [get_property PERIOD $c]]
}
puts "  Reported WHS therefore already contains this pessimism: a WHS of"
puts "  0.00 here means 0.150 ns of real margin, not none."
puts "==============================================================="
