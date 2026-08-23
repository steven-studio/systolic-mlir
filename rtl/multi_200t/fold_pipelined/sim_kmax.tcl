# sim_kmax.tcl -- run a testbench through a real Vivado project so the
# floating-point IP is handled for us.
#
#   vivado -mode batch -source sim_kmax.tcl -tclargs <K>
#       shorthand: tb_array_fold_kmax at K=<K>
#
#   vivado -mode batch -source sim_kmax.tcl -tclargs <top> "<generics>" [xci_dir]
#       e.g. -tclargs tb_uart_multi_invocation "K_MAX=64 K=65"
#            -tclargs tb_array_fold_kmax      "K=128"
#
# WHY A PROJECT AND NOT PLAIN xvlog/xelab/xsim
#
# xelab does not compile source; it elaborates what xvlog already put in a
# library. Even after xvlog, fp_mul.sv and fp_add.sv instantiate
# floating_point_mul_0 / floating_point_add_0, whose VHDL sources are
# generated into the project tree and need a specific set of -L library
# mappings plus glbl. Reproducing that by hand is exactly what this script
# exists to avoid: launch_simulation generates the IP simulation targets,
# compiles everything in order with the right libraries, and runs.
#
# The project is created fresh under build_kmax/sim_<top>/ every time and
# is disposable; nothing outside build_kmax/ is touched.

if {$argc < 1} {
    error "usage: -tclargs <K>   |   -tclargs <top> \"<generics>\" \[xci_dir\]"
}

set arg0 [lindex $argv 0]

# Backward-compatible shorthand: a bare number means the array-level bench.
if {[string is integer -strict $arg0]} {
    set TOP      "tb_array_fold_kmax"
    set GENERICS "K=$arg0"
    set XCI_ARG  1
} else {
    set TOP      $arg0
    set GENERICS [expr {$argc >= 2 ? [lindex $argv 1] : ""}]
    set XCI_ARG  2
}

set XCI_DIR [expr {$argc > $XCI_ARG ? [lindex $argv $XCI_ARG] \
                       : "rtl_fp_pe_test/rtl_fp_pe_test.srcs/sources_1/ip"}]

set PART "xc7a200tsbg484-1"
set PROJ "sim_${TOP}"
set DIR  "build_kmax/$PROJ"

set ADD_XCI "$XCI_DIR/floating_point_add_0/floating_point_add_0.xci"
set MUL_XCI "$XCI_DIR/floating_point_mul_0/floating_point_mul_0.xci"

foreach f [list $ADD_XCI $MUL_XCI] {
    if {![file exists $f]} {
        puts ""
        puts "======================================================="
        puts " Cannot find the floating-point IP:"
        puts "   $f"
        puts ""
        puts " These are generated, gitignored artifacts -- they exist"
        puts " only in a local Vivado project, not in the repository."
        puts ""
        puts " Pass the right directory as the last argument."
        puts "======================================================="
        error "missing IP: $f"
    }
}

file mkdir build_kmax
file delete -force $DIR

create_project -force $PROJ $DIR -part $PART

# Every design file, regardless of which bench is top. Vivado elaborates
# only what the top actually instantiates, so the array-level bench is not
# slowed down by the UART sources being present.
add_files -norecurse [list \
    uart_rx.sv \
    uart_tx.sv \
    fp_mul.sv \
    fp_add.sv \
    systolic_pe_tile.sv \
    systolic_array_tile.sv \
    systolic_array_8x8_tile.sv \
    systolic_uart_tile_top.sv \
    systolic_tile_feeder.sv \
    systolic_operand_buffer.sv \
]

# Every bench goes into the simulation fileset so a syntax error in any of
# them is caught here rather than the next time someone switches top.
#
#   tb_uart_multi_invocation -- full system over UART, needs the FP IP
#   tb_feeder_buffer         -- feeder+buffer contract, no IP, seconds
#   tb_operand_buffer_equiv  -- refactor equivalence, no IP, seconds
add_files -fileset sim_1 -norecurse [list \
    tb_uart_multi_invocation.sv \
    tb_feeder_buffer.sv \
    tb_operand_buffer_equiv.sv \
]

set_property file_type SystemVerilog [get_files *.sv]

read_ip $ADD_XCI
read_ip $MUL_XCI
generate_target simulation [get_ips]

set_property top $TOP [get_filesets sim_1]
set_property top_lib xil_defaultlib [get_filesets sim_1]

# xelab's -generic_top takes exactly ONE assignment per occurrence. Passing
# "K_MAX=64 K=65" as a single option makes it read everything after the
# space as the value, so K_MAX silently became 0x4B3D3635 -- the ASCII of
# "K=65". Emit one flag per generic instead.
if {$GENERICS ne ""} {
    set gopts [list]
    foreach g $GENERICS {
        lappend gopts "-generic_top" $g
    }
    set_property -name {xsim.elaborate.xelab.more_options} \
                 -value [join $gopts " "] \
                 -objects [get_filesets sim_1]
    puts " xelab opts: [join $gopts { }]"
}

# Run to $finish rather than the default 1000 ns.
set_property -name {xsim.simulate.runtime} -value {all} \
             -objects [get_filesets sim_1]

puts "========================================"
puts " top      : $TOP"
puts " generics : $GENERICS"
puts " project  : $DIR"
puts "========================================"

launch_simulation

set simlog "$DIR/${PROJ}.sim/sim_1/behav/xsim/simulate.log"
if {[file exists $simlog]} {
    set fh [open $simlog r]
    set txt [read $fh]
    close $fh
    puts "---------- simulate.log ----------"
    puts $txt
    puts "----------------------------------"
} else {
    puts "WARNING: no simulate.log at $simlog"
}

close_sim
close_project