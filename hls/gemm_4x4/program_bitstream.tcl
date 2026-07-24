# program_bitstream.tcl
#
# Programs the connected FPGA over JTAG via Vivado's Hardware Manager,
# entirely from the command line (no GUI). Run with:
#
#   vivado -mode batch -source program_bitstream.tcl -tclargs <path-to-bit-file>
#
# Example, from the gemm_4x4 build directory:
#
#   vivado -mode batch -source program_bitstream.tcl \
#       -tclargs vivado/matmul_top.runs/impl_1/matmul_top.bit

if {$argc != 1} {
    puts "usage: vivado -mode batch -source program_bitstream.tcl -tclargs <bitfile.bit>"
    exit 1
}
set bitfile [lindex $argv 0]

if {![file exists $bitfile]} {
    puts "ERROR: bitstream file not found: $bitfile"
    exit 1
}

puts "Connecting to hw_server..."
open_hw_manager
connect_hw_server -allow_non_jtag

puts "Opening target..."
open_hw_target

set device [lindex [get_hw_devices] 0]
if {$device eq ""} {
    puts "ERROR: no hardware device found. Check the board is connected and powered."
    exit 1
}
puts "Found device: $device"

current_hw_device $device
refresh_hw_device -update_hw_probes false $device

set_property PROGRAM.FILE $bitfile $device

puts "Programming $device with $bitfile ..."
program_hw_devices $device

puts "Done. Refreshing device status..."
refresh_hw_device $device

close_hw_target
disconnect_hw_server
close_hw_manager

puts "=== Programming complete. Check the DONE LED on the board. ==="
