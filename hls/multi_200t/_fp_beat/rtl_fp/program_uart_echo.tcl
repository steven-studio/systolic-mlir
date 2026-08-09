open_hw_manager
connect_hw_server

set targets [get_hw_targets]

puts "TARGETS:"
foreach t $targets {
    puts "  $t"
}

set target ""

foreach t $targets {
    if {[string match "*210276C08FC0B" $t]} {
        set target $t
    }
}

if {$target eq ""} {
    error "FC0B target not found"
}

puts ""
puts "OPENING: $target"

open_hw_target $target

set devs [get_hw_devices]

puts "DEVICES: $devs"

if {[llength $devs] == 0} {
    error "FC0B opened but no FPGA device found"
}

set dev [lindex $devs 0]

current_hw_device $dev
refresh_hw_device $dev

puts "PROGRAMMING: $dev"

set_property PROGRAM.FILE \
    {uart_echo_build/uart_echo_top.bit} \
    $dev

program_hw_devices $dev
refresh_hw_device $dev

puts ""
puts "========================================"
puts " UART ECHO FPGA PROGRAMMED"
puts "========================================"

close_hw_manager
