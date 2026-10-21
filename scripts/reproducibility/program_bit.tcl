# program_bit.tcl -- program the Nexys Video's xc7a200t with one given .bit.
#
#   vivado -mode batch -nojournal -source scripts/reproducibility/program_bit.tcl \
#          -tclargs <file.bit>
#
# Unlike dma_top_build.tcl's 'program' mode, which programs the .bit of a
# Vivado project under ~/work/vivado chosen by geometry and SYSTOLIC_PROJ_TAG,
# this programs exactly the file it is given: run_board.sh hands it the
# archived artifact after checking its SHA256. It needs no Vivado project and
# changes nothing but the FPGA's configuration (no synthesis, no
# implementation, no bitstream generation).
#
# The JTAG target choice is dma_top_build.tcl's (proc pick_device): the
# FT2232H registers two JTAG targets and only one reaches the FPGA, so every
# target is scanned and the one whose chain holds the xc7a200t is used.
# Exits non-zero unless the device reports DONE after programming.

if {[llength $argv] != 1} {
    puts "usage: vivado -mode batch -source program_bit.tcl -tclargs <file.bit>"
    exit 2
}
set bit [file normalize [lindex $argv 0]]
if {![file isfile $bit]} {
    puts "no such bitstream: $bit"
    exit 2
}

proc pick_device {} {
    set names {}
    foreach t [get_hw_targets -quiet] { lappend names "$t" }
    if {[llength $names] == 0} {
        puts "\nNO JTAG TARGET.  In order:"
        puts "  1. board powered on"
        puts "  2. microUSB in the PROG port, into THIS machine"
        puts "  3. lsusb | grep -i -e digilent -e future"
        error "no hardware target"
    }
    puts "\n=== scanning [llength $names] JTAG target(s) ================="
    foreach n $names {
        catch {close_hw_target}
        set t [get_hw_targets -quiet $n]
        if {[llength $t] == 0} { puts "  $n -> no longer listed" ; continue }
        if {[catch {open_hw_target [lindex $t 0]}]} { puts "  $n -> cannot open" ; continue }
        set devs [get_hw_devices -quiet]
        if {[llength $devs] == 0} { puts "  $n -> chain empty" ; catch {close_hw_target} ; continue }
        puts "  $n -> [llength $devs] device(s): $devs"
        set dev ""
        foreach d $devs { if {[string match -nocase *xc7a200t* $d]} { set dev $d } }
        if {$dev eq ""} { puts "  $n -> no xc7a200t on this chain" ; catch {close_hw_target} ; continue }
        puts "  using: $n  device $dev"
        current_hw_device $dev
        return $dev
    }
    error "no JTAG target has the xc7a200t on its chain"
}

open_hw_manager
if {[catch {connect_hw_server} msg]} {
    puts "\ncannot reach hw_server: $msg"
    puts "board powered on and plugged into THIS machine?"
    exit 1
}
if {[catch {set dev [pick_device]} msg]} {
    puts $msg
    exit 1
}
puts "\n=== programming ================================================"
puts "  file:   $bit"
puts "  device: $dev"
set_property PROGRAM.FILE $bit $dev
if {[catch {program_hw_devices $dev} msg]} {
    puts "program_hw_devices failed: $msg"
    exit 1
}
refresh_hw_device -quiet $dev
set done [get_property REGISTER.IR.BIT5_DONE $dev]
puts "  DONE:   $done"
close_hw_manager
if {$done ne "1"} {
    puts "the device did not report DONE after programming"
    exit 1
}
puts "=== programmed ================================================="
exit 0
