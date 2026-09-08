# -----------------------------------------------------------------------------
# calib_build.tcl -- build and program the DDR3 bring-up gate.
#
#   vivado -mode batch -source calib_build.tcl -tclargs build
#   vivado -mode batch -source calib_build.tcl -tclargs program
#   vivado -mode batch -source calib_build.tcl -tclargs all      (both)
#
# Self-contained: creates its own project outside the repo from the .xci that
# mig_gen.tcl pinned, plus ddr3_calib_top.sv and nexys_video_calib.xdc.
# Nothing here touches the MIG configuration.
#
# The design is small (MIG plus a handful of flops), so expect a few minutes.
# -----------------------------------------------------------------------------

set PART      xc7a200tsbg484-1
# The .xci was customised in a project that had a board part set, so MIG needs
# the same board available here or it cannot recreate the IP instance
# ("ERROR: [Board 49-60] No current board set").
set BOARD_PART digilentinc.com:nexys_video:part0:1.2
set BOARD_REPO $::env(HOME)/work/vivado/vivado-boards/new/board_files
set PROJ_DIR  $::env(HOME)/work/vivado/ddr3_calib
set PROJ_NAME ddr3_calib
set TOP       ddr3_calib_top
set JOBS      8

set SCRIPT_DIR [file dirname [file normalize [info script]]]
set XCI        $SCRIPT_DIR/ip/ddr3/mig_7series_0.xci
set SRC        $SCRIPT_DIR/ddr3_calib_top.sv
set XDC        $SCRIPT_DIR/nexys_video_calib.xdc

set MODE [lindex $argv 0]
if {$MODE eq ""} { set MODE all }

# -----------------------------------------------------------------------------
proc build {part proj_dir proj_name top xci src xdc jobs board_repo board_part} {
    foreach f [list $xci $src $xdc] {
        if {![file exists $f]} { error "missing: $f" }
    }

    file delete -force $proj_dir
    create_project -force $proj_name $proj_dir -part $part

    if {[file isdirectory $board_repo]} {
        set_property BOARD_PART_REPO_PATHS $board_repo [current_project]
    } else {
        puts "WARNING: board repo not found: $board_repo"
    }
    if {[lsearch -exact [get_board_parts -quiet] $board_part] >= 0} {
        set_property board_part $board_part [current_project]
        puts "board_part = $board_part"
    } else {
        puts "WARNING: board part '$board_part' unavailable; MIG will fail to"
        puts "  recreate the IP.  Present nexys_video parts:"
        foreach bp [get_board_parts -quiet] {
            if {[string match -nocase *nexys_video* $bp]} { puts "    $bp" }
        }
    }

    # import_ip, not add_files: add_files references the .xci in place and
    # writes this project's generated output next to it, inside the repo.
    # import_ip copies it in, so the pinned repo copy stays pristine.
    import_ip -files $xci
    add_files -norecurse $src
    add_files -fileset constrs_1 -norecurse $xdc
    set_property top $top [current_fileset]

    generate_target all [get_ips]
    update_compile_order -fileset sources_1

    launch_runs impl_1 -to_step write_bitstream -jobs $jobs
    wait_on_run impl_1

    if {[get_property PROGRESS [get_runs impl_1]] ne "100%"} {
        set d [get_property DIRECTORY [get_runs impl_1]]
        puts "\nimpl_1 did not finish: [get_property STATUS [get_runs impl_1]]"
        puts "  read: $d/runme.log"
        error "build failed"
    }

    set dir  [get_property DIRECTORY [get_runs impl_1]]
    set bits [glob -nocomplain $dir/*.bit]
    puts "\n=== bitstream ================================================="
    foreach b $bits { puts "  $b" }
    puts "  WNS = [get_property STATS.WNS [get_runs impl_1]] ns"
    puts "  WHS = [get_property STATS.WHS [get_runs impl_1]] ns"
    close_project
}

# -----------------------------------------------------------------------------
proc program {proj_dir proj_name} {
    open_project $proj_dir/$proj_name.xpr
    set dir  [get_property DIRECTORY [get_runs impl_1]]
    set bits [glob -nocomplain $dir/*.bit]
    if {[llength $bits] == 0} { error "no .bit in $dir  (run 'build' first)" }
    set bit [lindex $bits 0]

    open_hw_manager
    if {[catch {connect_hw_server} msg]} {
        puts "\ncannot reach hw_server: $msg"
        error "board powered on and plugged into THIS machine?"
    }
    set targets [get_hw_targets -quiet]
    if {[llength $targets] == 0} {
        puts "\nNO JTAG TARGET.  In order:"
        puts "  1. board powered on"
        puts "  2. microUSB in the PROG port, into THIS machine (you are on ssh)"
        puts "  3. lsusb | grep -i -e digilent -e future"
        puts "  4. sudo <Vivado>/data/xicom/cable_drivers/lin64/install_script/\\"
        puts "          install_drivers/install_drivers"
        error "no hardware target"
    }

    # The FT2232H on this board has TWO channels, and Digilent registers both
    # as JTAG targets (".../210276C08FC0" and ".../210276C08FC0B").  Only one
    # of them actually reaches the FPGA; the other scans empty.  Taking
    # [lindex $targets 0] picks whichever Vivado listed first and fails with
    # "No devices detected" about half the time -- which is exactly what it did.
    #
    # So: try each, keep the one whose chain is not empty.  A failed open
    # leaves the shared USB device half-claimed ("Target is already opened" on
    # the next one), so the server connection is reset between attempts.
    # Keep the NAMES as plain strings, not the objects: a hw_target handle does
    # not survive a server reconnect -- it comes back as "null" and
    # open_hw_target rejects it.  Re-query by name on each attempt instead, and
    # do not disconnect at all; closing the previous target is enough to stop
    # the shared USB device answering "Target is already opened".
    set names {}
    foreach t $targets { lappend names "$t" }

    puts "\n=== scanning [llength $names] JTAG target(s) ================="
    set chosen ""
    set dev    ""
    foreach n $names {
        catch {close_hw_target}
        set t [get_hw_targets -quiet $n]
        if {[llength $t] == 0} { puts "  $n -> no longer listed" ; continue }
        if {[catch {open_hw_target [lindex $t 0]} m]} {
            puts "  $n -> cannot open"
            continue
        }
        set devs [get_hw_devices -quiet]
        if {[llength $devs] == 0} {
            puts "  $n -> chain empty"
            catch {close_hw_target}
            continue
        }
        puts "  $n -> [llength $devs] device(s): $devs"
        set chosen $n
        foreach d $devs {
            if {[string match -nocase *xc7a200t* $d]} { set dev $d }
        }
        if {$dev eq ""} { set dev [lindex $devs 0] }
        break
    }
    if {$chosen eq ""} {
        error "every JTAG target scanned empty -- FPGA not on the chain"
    }
    puts "  using: $chosen  device $dev"
    current_hw_device $dev

    set_property PROGRAM.FILE $bit $dev
    program_hw_devices $dev
    refresh_hw_device -quiet $dev

    puts "\n=== programmed ================================================"
    puts "  LOOK AT THE BOARD.  LEDs, right to left:"
    puts "    led\[0\]  init_calib_complete   <-- THE GATE"
    puts "    led\[1\]  our 200 MHz MMCM locked"
    puts "    led\[2\]  MIG's MMCM locked"
    puts "    led\[3\]  MIG user interface out of reset"
    puts "    led\[4\]  heartbeat, 100 MHz    (blinks ~1.5 Hz)"
    puts "    led\[5\]  heartbeat, ui_clk     (blinks ~1.5 Hz)"
    puts ""
    puts "  led\[0\] lit          -> DDR3 CALIBRATED.  Gate passed."
    puts "  led\[1..5\] but not 0 -> clocks fine, calibration failed."
    puts "                          That is the 9/3 stop condition."
    puts "  only led\[4\]         -> our MMCM is dead"
    puts "  all dark            -> no board clock, or not programmed"
    close_hw_manager
    close_project
}

# -----------------------------------------------------------------------------
switch -- $MODE {
    build   { build $PART $PROJ_DIR $PROJ_NAME $TOP $XCI $SRC $XDC $JOBS \
                    $BOARD_REPO $BOARD_PART }
    program { program $PROJ_DIR $PROJ_NAME }
    all     { build $PART $PROJ_DIR $PROJ_NAME $TOP $XCI $SRC $XDC $JOBS \
                    $BOARD_REPO $BOARD_PART
              program $PROJ_DIR $PROJ_NAME }
    default { error "unknown mode '$MODE' (build | program | all)" }
}