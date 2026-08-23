# program_kmax.tcl -- program a K_MAX sweep bitstream onto the Nexys Video.
#
#   vivado -mode batch -source program_kmax.tcl -tclargs <K_MAX> [<jtag_glob>]
#
# e.g.  vivado -mode batch -source program_kmax.tcl -tclargs 64
#
# Kept separate from program_8x8_fold.tcl, which hardwires the old
# build_8x8_fold/ baseline bitstream. That script is left untouched so the
# original K=16 build stays reproducible.
#
# AFTER PROGRAMMING: press BTNC (reset) before the first transaction.
# k_dim is loaded with K_MAX on reset only, so straight out of configuration
# it is 0 and the feed FSM injects nothing -- the design will appear to hang
# waiting for results that never come.

if {$argc < 1} {
    error "usage: -tclargs <K_MAX> \[<jtag_target_glob>\]"
}

set KMAX   [lindex $argv 0]
set TGLOB  [expr {$argc >= 2 ? [lindex $argv 1] : "*210276C08FC0B*"}]

set BIT "build_kmax/k${KMAX}/systolic_uart_fold_top_k${KMAX}.bit"

if {![file exists $BIT]} {
    puts ""
    puts "======================================================="
    puts " No bitstream at:"
    puts "   $BIT"
    puts ""
    puts " build_kmax.tcl only writes a bitstream when timing"
    puts " closed. Either the run has not happened yet, or that"
    puts " K_MAX did not meet timing / failed placement."
    puts ""
    puts " Available:"
    foreach f [glob -nocomplain build_kmax/k*/*.bit] { puts "   $f" }
    puts "======================================================="
    error "missing bitstream: $BIT"
}

puts "========================================"
puts " Programming K_MAX = $KMAX"
puts "   $BIT"
puts "========================================"

open_hw_manager
connect_hw_server

set target [lindex [get_hw_targets $TGLOB] 0]

if {$target eq ""} {
    puts "ERROR: JTAG target matching '$TGLOB' not found."
    puts "Available targets:"
    puts [get_hw_targets]
    error "no matching JTAG target"
}

puts "OPENING: $target"
open_hw_target $target

set devs [get_hw_devices]
if {[llength $devs] == 0} {
    error "No FPGA device found"
}

set dev [lindex $devs 0]
current_hw_device $dev
refresh_hw_device $dev

puts "PROGRAMMING: $dev"
set_property PROGRAM.FILE $BIT $dev
program_hw_devices $dev
refresh_hw_device $dev

puts "========================================"
puts " PROGRAMMED K_MAX = $KMAX"
puts ""
puts " Wire format for this build:"
puts "   RX  [expr {$KMAX * 64}] bytes   (A,B interleaved, [expr {$KMAX / 8}] windows of 8)"
puts "   TX  512 bytes    (C_ctx0 then C_ctx1; host adds them)"
puts ""
puts " NOW PRESS BTNC (reset) before sending anything."
puts ""
puts " Then:  python3 test_uart_kmax.py --kmax $KMAX"
puts "========================================"

close_hw_manager
