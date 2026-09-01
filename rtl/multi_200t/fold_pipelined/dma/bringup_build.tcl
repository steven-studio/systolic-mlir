# -----------------------------------------------------------------------------
# bringup_build.tcl -- build and program the DMA bring-up design (step 3a).
#
#   vivado -mode batch -source bringup_build.tcl -tclargs build
#   vivado -mode batch -source bringup_build.tcl -tclargs program
#   vivado -mode batch -source bringup_build.tcl -tclargs all      (both)
#   vivado -mode batch -source bringup_build.tcl -tclargs read
#
# Self-contained: creates its own project outside the repo from the .xci that
# mig_gen.tcl pinned, plus the DMA read path, the seed writer, the operand
# buffer from ../core/ and nexys_video_bringup.xdc.  Nothing here touches the
# MIG configuration.
#
# The design has no UART: everything is read over JTAG with
#   vivado -mode batch -source bringup_build.tcl -tclargs read
#
# The design is small (MIG plus a handful of flops), so expect a few minutes.
# -----------------------------------------------------------------------------

set PART      xc7a200tsbg484-1
# The .xci was customised in a project that had a board part set, so MIG needs
# the same board available here or it cannot recreate the IP instance
# ("ERROR: [Board 49-60] No current board set").
set BOARD_PART digilentinc.com:nexys_video:part0:1.2
set BOARD_REPO $::env(HOME)/work/vivado/vivado-boards/new/board_files
set PROJ_DIR  $::env(HOME)/work/vivado/dma_bringup
set PROJ_NAME dma_bringup
set TOP       dma_bringup_top
# Must match the EXPECT_CHK parameter in dma_bringup_top.sv.  It is a constant,
# so synthesis folds it away and there is no net for a probe to read -- the
# design still compares against it in hardware (chk_match), but the value has
# to be printed from here.
set EXPECT_CHK 387fdc00
set JOBS      8

set SCRIPT_DIR [file dirname [file normalize [info script]]]
set XCI        $SCRIPT_DIR/ip/ddr3/mig_7series_0.xci
set SRC        [list $SCRIPT_DIR/dma_bringup_top.sv \
                     $SCRIPT_DIR/dma_engine.sv \
                     $SCRIPT_DIR/dma_seed_writer.sv \
                     $SCRIPT_DIR/dma_operand_writer.sv \
                     $SCRIPT_DIR/../core/operand_buffer.sv]
set XDC        $SCRIPT_DIR/nexys_video_bringup.xdc

set MODE [lindex $argv 0]
if {$MODE eq ""} { set MODE all }

# -----------------------------------------------------------------------------
proc build {part proj_dir proj_name top xci src xdc jobs board_repo board_part} {
    foreach f [concat [list $xci $xdc] $src] {
        if {![file exists $f]} { error "missing: $f" }
    }

    # DO NOT DELETE A WORKING PROJECT BEFORE THE NEW ONE EXISTS.
    #
    # This used to open with 'file delete -force $proj_dir'.  When the build
    # then failed at import_ip, the previous project -- its bitstream and, far
    # worse, its .ltx probe map -- was already gone, and 'read' could no longer
    # name a single probe on a board that was still happily running the old
    # design.  A failed build must not cost you a working one.
    #
    # So: move the old project aside, and put it back if anything below fails.
    set prev $proj_dir.prev
    file delete -force $prev
    set had_prev 0
    if {[file isdirectory $proj_dir]} {
        file rename $proj_dir $prev
        set had_prev 1
        puts "previous project moved aside: $prev"
    }
    if {[catch {
        build_body $part $proj_dir $proj_name $top $xci $src $xdc $jobs \
                   $board_repo $board_part
    } msg]} {
        puts "\n=== BUILD FAILED =============================================="
        puts "  $msg"
        if {$had_prev} {
            file delete -force $proj_dir
            file rename $prev $proj_dir
            puts "\n  The previous project has been PUT BACK, bitstream and .ltx"
            puts "  intact.  The board was never reprogrammed, so"
            puts "    vivado -mode batch -source bringup_build.tcl -tclargs read"
            puts "  still reports the measurement it reported before."
        }
        error "build failed"
    }
    file delete -force $prev
}

proc build_body {part proj_dir proj_name top xci src xdc jobs board_repo board_part} {
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

    # MIG re-runs its customisation Tcl when the IP is imported.  That Tcl cds
    # into the IP's generated-sources directory in order to resolve the .prj
    # path relative to it:
    #
    #   [IP_Flow 19-3484] Absolute path of file '.../nexys_video_mig_axi128.prj'
    #     provided.  It will be converted relative to IP Instance files
    #     '../../../../../../systolic-mlir/.../nexys_video_mig_axi128.prj'
    #   couldn't change working directory to
    #     '.../dma_bringup.gen/sources_1/ip/mig_7series_0': no such file or directory
    #   [IP_Flow 19-3475] Tcl error in ...updateAllModelParams
    #
    # At import_ip time that directory does not exist yet -- Vivado creates it
    # later, during generate_target.  Creating it up front costs nothing and
    # removes the ordering hazard.
    #
    # WHY THIS ONLY SHOWED UP NOW: while the .xci pointed at a .prj that was
    # not there, MIG never attempted to re-read it -- it warned [Project 1-19]
    # and used the parameters already stored in the .xci.  The dangling path
    # was, accidentally, what kept the build working.  Repair the path and MIG
    # starts doing the thing that was broken all along.
    file mkdir $proj_dir/$proj_name.gen/sources_1/ip/mig_7series_0

    # import_ip, not add_files: add_files references the .xci in place and
    # writes this project's generated output next to it, inside the repo.
    # import_ip copies it in, so the pinned repo copy stays pristine.
    if {[catch {import_ip -files $xci} m]} {
        puts "\nimport_ip failed: $m"
        puts ""
        puts "If it is still the working-directory error above, the IP output"
        puts "directory name did not match what MIG wanted.  The build has not"
        puts "touched the board -- whatever bitstream is on it still runs, and"
        puts "  vivado -mode batch -source bringup_build.tcl -tclargs read"
        puts "still reports the measurement."
        error "import_ip failed"
    }

    # VIO: read beats/cyc/sink over JTAG, so the measurement does not depend on
    # the UART pin working.  Two independent ways out of the chip; if they ever
    # disagree, the transport is at fault, not the counters.
    create_ip -name vio -vendor xilinx.com -library ip -version 3.0 \
              -module_name vio_0
    set_property -dict [list \
        CONFIG.C_NUM_PROBE_IN   {4} \
        CONFIG.C_NUM_PROBE_OUT  {0} \
        CONFIG.C_PROBE_IN0_WIDTH {32} \
        CONFIG.C_PROBE_IN1_WIDTH {32} \
        CONFIG.C_PROBE_IN2_WIDTH {32} \
        CONFIG.C_PROBE_IN3_WIDTH {8} \
    ] [get_ips vio_0]

    foreach f $src { add_files -norecurse $f }
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
    puts "    led\[0\]  init_calib_complete   memory alive"
    puts "    led\[1\]  seed written"
    puts "    led\[2\]  descriptor complete"
    puts "    led\[3\]  CHECKSUM MATCHES      <-- the gate"
    puts "    led\[4\]  any error latched"
    puts "    led\[5\]  heartbeat, ui_clk"
    puts ""
    puts "  Then read the numbers over JTAG:"
    puts "    vivado -mode batch -source bringup_build.tcl -tclargs read"
    close_hw_manager
    close_project
}

# -----------------------------------------------------------------------------
proc read_vio {proj_dir proj_name exp} {
    open_project $proj_dir/$proj_name.xpr
    open_hw_manager
    connect_hw_server
    set names {}
    foreach t [get_hw_targets -quiet] { lappend names "$t" }
    set dev ""
    foreach n $names {
        catch {close_hw_target}
        set t [get_hw_targets -quiet $n]
        if {[llength $t] == 0} { continue }
        if {[catch {open_hw_target [lindex $t 0]}]} { continue }
        set devs [get_hw_devices -quiet]
        if {[llength $devs] == 0} { catch {close_hw_target} ; continue }
        foreach d $devs { if {[string match -nocase *xc7a200t* $d]} { set dev $d } }
        break
    }
    if {$dev eq ""} { error "no FPGA on any JTAG target" }
    current_hw_device $dev

    # the probes live in the .ltx written next to the bitstream
    set dir [get_property DIRECTORY [get_runs impl_1]]
    set ltx [glob -nocomplain $dir/*.ltx]
    if {[llength $ltx] > 0} {
        set_property PROBES.FILE [lindex $ltx 0] $dev
        set_property FULL_PROBES.FILE [lindex $ltx 0] $dev
    }
    refresh_hw_device -quiet $dev

    set vios [get_hw_vios -quiet]
    if {[llength $vios] == 0} {
        error "no VIO found -- was the design rebuilt with the VIO in it?"
    }
    set v [lindex $vios 0]
    refresh_hw_vio $v

    # Probe names are hierarchical (e.g. "u_vio/probe_in0"), so match on the
    # suffix rather than the bare name.  List them too: if a lookup ever comes
    # back empty, the names are the first thing to look at.
    set all_probes [get_hw_probes -of_objects $v -quiet]
    puts "\n=== probes found: [llength $all_probes] ==="
    foreach p $all_probes { puts "  [get_property NAME $p]" }

    # The .ltx does NOT name the probes probe_in0..3.  Vivado names each probe
    # after the NET that drives it, so probe_in0 shows up as "dbg_beats", and
    # the concatenation on probe_in3 is split into one probe per signal:
    #
    #   dbg_beats  dbg_cyc  dbg_sink
    #   init_calib_complete  probe_running  probe_reported  <const0>
    #
    # Match on the net names, and fall back to probe_inN for any bitstream
    # whose probe map does use the generic names.
    #
    # INPUT_VALUE is a STRING formatted according to INPUT_VALUE_RADIX, and the
    # default radix is not necessarily hex.  Pin it, or "0x$beats" is a lie.
    proc pval {probes radix args} {
        foreach name $args {
            foreach p $probes {
                if {[string match "*$name" [get_property NAME $p]]} {
                    catch {set_property INPUT_VALUE_RADIX $radix $p}
                    return [get_property INPUT_VALUE $p]
                }
            }
        }
        return ""
    }
    set chk   [pval $all_probes HEX    chk       probe_in0]
    set wrds  [pval $all_probes HEX    words_written probe_in1]

    set calib [pval $all_probes BINARY init_calib_complete]
    set sd    [pval $all_probes BINARY seed_done_sticky]
    set rd    [pval $all_probes BINARY read_done_sticky]
    set mt    [pval $all_probes BINARY chk_match]
    set er    [pval $all_probes BINARY any_err]

    if {$chk eq ""} {
        puts "\nNo probe matched.  Read the list printed above: those are the"
        puts "real names in the bitstream currently on the board."
    }

    puts "\n=== bring-up step 3a =========================================="
    puts "  init_calib_complete = $calib     memory alive"
    puts "  seed written        = $sd"
    puts "  descriptor complete = $rd"
    puts "  words written       0x$wrds"
    puts "  checksum            0x$chk"
    puts "  expected            0x$exp   (from the parameter, not a probe)"
    puts "  any error latched   = $er"
    puts ""
    if {$mt eq "1"} {
        puts "  CHECKSUM MATCHES.  The image survives"
        puts "  DDR3 -> MIG -> dma_engine -> dma_operand_writer -> the buffers."
        puts ""
        puts "  What that does and does not mean: placement correctness was"
        puts "  already proved in simulation, entry by entry, against a golden"
        puts "  model of the UART rx_count decode.  What this adds is that on"
        puts "  real silicon no beat was dropped, duplicated or misfiled."
        puts "  Step 3b can now blame the array alone."
    } else {
        puts "  CHECKSUM DOES NOT MATCH.  Read the flags in this order:"
        puts "    init_calib = 0       memory never came up; nothing below"
        puts "                         means anything yet"
        puts "    seed = 0             the AXI WRITE channel is stuck -- new"
        puts "                         territory, the read path was proved"
        puts "                         last night"
        puts "    descriptor = 0       reads were issued but never completed"
        puts "    any error = 1        alignment, response or range: the design"
        puts "                         refused work rather than moving garbage"
        puts "    all of those ok      beats were lost or misfiled; compare"
        puts "                         words written against K_MAX*2*N"
    }
    close_hw_manager
    close_project
}

# -----------------------------------------------------------------------------
switch -- $MODE {
    build   { build $PART $PROJ_DIR $PROJ_NAME $TOP $XCI $SRC $XDC $JOBS \
                    $BOARD_REPO $BOARD_PART }
    program { program $PROJ_DIR $PROJ_NAME }
    read    { read_vio $PROJ_DIR $PROJ_NAME $EXPECT_CHK }
    all     { build $PART $PROJ_DIR $PROJ_NAME $TOP $XCI $SRC $XDC $JOBS \
                    $BOARD_REPO $BOARD_PART
              program $PROJ_DIR $PROJ_NAME }
    default { error "unknown mode '$MODE' (build | program | all | read)" }
}
