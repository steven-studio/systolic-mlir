# sim_kmax.tcl -- run tb_array_fold_kmax at one K, through a real Vivado
# project so the floating-point IP is handled for us.
#
#   vivado -mode batch -source sim_kmax.tcl -tclargs <K> [<XCI_DIR>]
#
# WHY A PROJECT AND NOT PLAIN xvlog/xelab/xsim
#
# fp_mul.sv and fp_add.sv instantiate floating_point_mul_0 and
# floating_point_add_0. Those are Xilinx IP: their sources are generated
# into the project tree, are gitignored, and need library mappings that a
# bare xelab invocation does not have. Driving the simulator directly means
# reproducing all of that by hand. launch_simulation already does it --
# it generates the IP simulation targets, compiles them in the right order
# with the right -L flags, and runs.
#
# The project is created fresh under build_kmax/sim_k<K>/ every time and is
# disposable; nothing outside build_kmax/ is touched.

if {$argc < 1} {
    error "usage: -tclargs <K> \[<xci_dir>\]"
}

set K [lindex $argv 0]

# Where the two .xci live. Default matches build_8x8_fold.tcl.
set XCI_DIR [expr {$argc >= 2 ? [lindex $argv 1] \
                              : "rtl_fp_pe_test/rtl_fp_pe_test.srcs/sources_1/ip"}]

set PART "xc7a200tsbg484-1"
set PROJ "sim_k${K}"
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
        puts " Point this script at the right directory:"
        puts "   vivado -mode batch -source sim_kmax.tcl \\"
        puts "          -tclargs $K /path/to/srcs/sources_1/ip"
        puts ""
        puts " Or open the project that owns them and re-run from there."
        puts "======================================================="
        error "missing IP: $f"
    }
}

file mkdir build_kmax
file delete -force $DIR

create_project -force $PROJ $DIR -part $PART

add_files -norecurse [list \
    fp_mul.sv \
    fp_add.sv \
    systolic_pe_fold.sv \
    systolic_array_8x8_fold.sv \
]
set_property file_type SystemVerilog [get_files *.sv]

add_files -fileset sim_1 -norecurse tb_array_fold_kmax.sv
set_property file_type SystemVerilog [get_files tb_array_fold_kmax.sv]

read_ip $ADD_XCI
read_ip $MUL_XCI
generate_target simulation [get_ips]

set_property top tb_array_fold_kmax [get_filesets sim_1]
set_property top_lib xil_defaultlib  [get_filesets sim_1]

# K is a module-level parameter on the testbench; -generic_top overrides it
# at elaboration, so one source file covers every sweep point.
set_property -name {xsim.elaborate.xelab.more_options} \
             -value "-generic_top K=$K" \
             -objects [get_filesets sim_1]

# Run to $finish rather than the default 1000ns.
set_property -name {xsim.simulate.runtime} -value {all} \
             -objects [get_filesets sim_1]

puts "========================================"
puts " simulating tb_array_fold_kmax at K=$K"
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
