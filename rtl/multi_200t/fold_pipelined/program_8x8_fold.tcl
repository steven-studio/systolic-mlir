open_hw_manager
connect_hw_server

set target [lindex [get_hw_targets *210276C08FC0B*] 0]

if {$target eq ""} {
    puts "ERROR: FC0B target not found"
    puts "Available targets:"
    puts [get_hw_targets]
    error "No correct JTAG target"
}

puts "OPENING: $target"
open_hw_target $target

set devs [get_hw_devices]
puts "DEVICES: $devs"

if {[llength $devs] == 0} {
    error "No FPGA device found"
}

set dev [lindex $devs 0]
current_hw_device $dev
refresh_hw_device $dev

puts "PROGRAMMING: $dev"

set_property PROGRAM.FILE \
    {build_8x8_fold/systolic_uart_fold_top.bit} \
    $dev

program_hw_devices $dev
refresh_hw_device $dev

puts "========================================"
puts " FPGA 8x8 FOLD PROGRAMMED SUCCESSFULLY"
puts "========================================"

close_hw_manager
