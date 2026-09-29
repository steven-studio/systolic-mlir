# -----------------------------------------------------------------------------
# mig_gen.tcl -- headless MIG generation for the DDR3 bandwidth spike.
#
# Lives in .../fold_pipelined/dma/ and resolves every path from its own
# location, like build_kmax.tcl and ooc_timing.tcl.
#
#   1. discovery (read-only, writes nothing, ~20 s):
#        vivado -mode batch -source mig_gen.tcl -tclargs discover
#
#   2. generate the IP and copy the .xci back into the repo:
#        vivado -mode batch -source mig_gen.tcl -tclargs gen
#
#   3. Xilinx's own traffic generator -- THE BRING-UP GATE, run this before
#      ddr3_bw_probe ever goes near the board:
#        vivado -mode batch -source mig_gen.tcl -tclargs example
#
#   4. synthesise, implement and write the bitstream (~20-40 min, unattended):
#        vivado -mode batch -source mig_gen.tcl -tclargs build
#
#   5. program the board over JTAG (needs the board powered and plugged into
#      THIS machine):
#        vivado -mode batch -source mig_gen.tcl -tclargs program
#
#   open_example_project only writes project files.  Nothing reaches the FPGA
#   until step 5, so no LED can light before then.
#
# UNTESTED -- there is no Vivado where this was written.  Read it, run it a
# step at a time.  Step 1 is read-only, so it costs nothing to try.
#
# The Vivado project is created OUTSIDE the repo ($HOME/work/vivado/mig_spike):
# it reaches several GB and it is disposable.  Two files are worth keeping and
# both end up in the repo:
#   ip/ddr3/nexys_video_mig.prj   the memory configuration (pin this by hand)
#   ip/ddr3/mig_7series_0.xci     what Vivado made from it (gen copies it)
#
# WHAT THE PINNED .prj SAYS  (Digilent nexys_video A.0 rev 1.2)
#   TimePeriod 2500 ps -> 400 MHz memory clock -> 800 MT/s
#   PHYRatio   4:1     -> ui_clk = 100 MHz
#   DataWidth  16      -> app_data_width = 2*4*16 = 128 bits = 4 fp32 words
#   => beta_ceiling = 4 operand words per 100 MHz array cycle.
#   800 MT/s is the Artix-7 -1 HR-bank limit, not a conservative choice, so
#   this ceiling cannot be tuned upward on this board.
#   The discover step recomputes all of this from whatever .prj is actually
#   configured, so the number follows the file rather than this comment.
# -----------------------------------------------------------------------------

set PART       xc7a200tsbg484-1
set PROJ_DIR   $::env(HOME)/work/vivado/mig_spike
set PROJ_NAME  mig_spike
set IP_NAME    mig_7series_0
set ARRAY_MHZ  100.0                       ;# the array's clock, for beta_ceiling

# Named explicitly, NOT auto-detected.  get_board_parts returns both
# nexys_video:part0:1.1 and :1.2, and picking whichever comes first silently
# pairs the wrong board revision with the pinned mig.prj.  This must match the
# revision directory the .prj was copied from (A.0/1.2 -> ...:part0:1.2).
set BOARD_PART digilentinc.com:nexys_video:part0:1.2

# Digilent board files, cloned with:
#   git clone --depth 1 https://github.com/Digilent/vivado-boards
set BOARD_REPO $::env(HOME)/work/vivado/vivado-boards/new/board_files

set SCRIPT_DIR [file dirname [file normalize [info script]]]
set REPO_IP    $SCRIPT_DIR/ip/ddr3
set MIG_PRJ    $REPO_IP/nexys_video_mig_axi128.prj

set MODE [lindex $argv 0]
if {$MODE eq ""} { set MODE discover }
# optional override: -tclargs gen /some/other/mig.prj
if {[llength $argv] > 1} { set MIG_PRJ [lindex $argv 1] }

# -----------------------------------------------------------------------------
# read one tag out of a MIG .prj (it is plain XML)
proc prj_get {file tag} {
    if {![file exists $file]} { return "" }
    set fh [open $file r]
    set data [read $fh]
    close $fh
    if {[regexp "<$tag>\\s*(\[^<\]*)\\s*</$tag>" $data -> v]} {
        return [string trim $v]
    }
    return ""
}

proc report_prj {file array_mhz} {
    puts "\n=== memory configuration: $file"
    if {![file exists $file]} {
        puts "  MISSING.  Pin it first:"
        puts "    cp ~/work/vivado/vivado-boards/new/board_files/nexys_video/A.0/1.2/mig.prj \\"
        puts "       <repo>/rtl/multi_200t/fold_pipelined/dma/ip/ddr3/nexys_video_mig.prj"
        return
    }
    set dev   [prj_get $file MemoryDevice]
    set tp    [prj_get $file TimePeriod]
    set ratio [prj_get $file PHYRatio]
    set dw    [prj_get $file DataWidth]
    set inclk [prj_get $file InputClkFreq]
    puts "  MemoryDevice  $dev"
    puts "  TimePeriod    $tp ps"
    puts "  PHYRatio      $ratio"
    puts "  DataWidth     $dw"
    puts "  InputClkFreq  $inclk MHz"

    if {$tp eq "" || $ratio eq "" || $dw eq ""} {
        puts "  (could not parse enough to derive the ceiling)"
        return
    }
    set n     [lindex [split $ratio ":"] 0]      ;# 4 from "4:1"
    set memhz [expr {1.0e6 / $tp}]               ;# ps -> MHz
    set uihz  [expr {$memhz / $n}]
    set appw  [expr {2 * $n * $dw}]              ;# bits per UI beat
    set words [expr {$appw / 32.0}]              ;# fp32 words per UI beat
    set beta  [expr {$words * $uihz / $array_mhz}]

    puts "\n  derived:"
    puts [format "    memory clock      %.1f MHz  (%.0f MT/s)" $memhz [expr {$memhz*2}]]
    puts [format "    ui_clk            %.1f MHz" $uihz]
    puts [format "    app_data_width    %.0f bits = %.0f fp32 words per beat" $appw $words]
    puts [format "    BETA_CEILING      %.2f words per %.0f MHz array cycle" $beta $array_mhz]
    puts "    compare against the fold-average demand 2sK/(K+2(s-1)+H):"
    puts "      s=8, K=256, H=95  ->  4096/365 = 11.2 words/cycle"
    puts "    This is a CEILING derived from the config, not a measurement."
    puts "    Sequential reads only: no row misses, no refresh contention."
}

# -----------------------------------------------------------------------------
proc discover {part board_repo mig_prj array_mhz} {
    create_project -in_memory -part $part
    if {[file isdirectory $board_repo]} {
        set_property BOARD_PART_REPO_PATHS $board_repo [current_project]
        puts "board repo: $board_repo"
    } else {
        puts "board repo NOT FOUND: $board_repo"
        puts "  git clone --depth 1 https://github.com/Digilent/vivado-boards"
    }

    puts "\n=== board parts matching 'nexys' =============================="
    set found 0
    foreach bp [get_board_parts -quiet] {
        if {[string match -nocase *nexys* $bp]} { puts "  $bp" ; incr found }
    }
    if {!$found} { puts "  (none -- check the board repo path above)" }

    puts "\n=== mig_7series IP available =================================="
    foreach ip [get_ipdefs -quiet *mig_7series*] { puts "  $ip" }

    report_prj $mig_prj $array_mhz

    puts "\nIf the board part and the IP both showed up, run:"
    puts "  vivado -mode batch -source mig_gen.tcl -tclargs gen\n"
    close_project -quiet
}

# -----------------------------------------------------------------------------
proc gen {part proj_dir proj_name ip_name repo_ip mig_prj board_repo board_part array_mhz} {
    if {![file exists $mig_prj]} {
        error "mig .prj not found: $mig_prj  (run discover first)"
    }
    report_prj $mig_prj $array_mhz

    file mkdir $proj_dir
    create_project -force $proj_name $proj_dir -part $part

    if {[file isdirectory $board_repo]} {
        set_property BOARD_PART_REPO_PATHS $board_repo [current_project]
    }
    if {[lsearch -exact [get_board_parts -quiet] $board_part] >= 0} {
        set_property board_part $board_part [current_project]
        puts "\nboard_part = $board_part"
    } else {
        puts "\nWARNING: board part '$board_part' not available.  Present:"
        foreach bp [get_board_parts -quiet] {
            if {[string match -nocase *nexys_video* $bp]} { puts "    $bp" }
        }
        puts "  Fix BOARD_PART at the top of this script so it matches the"
        puts "  revision the pinned mig.prj came from, then re-run."
        puts "  Continuing on the .prj alone: it carries the DDR3 pinout, but"
        puts "  you will then need Digilent's master XDC for the UART pin, the"
        puts "  100 MHz clock and cpu_resetn."
    }

    create_ip -name mig_7series -vendor xilinx.com -library ip -module_name $ip_name
    # MIG is configured by an XML file, not by flat CONFIG.* properties.
    set_property -dict [list CONFIG.XML_INPUT_FILE $mig_prj] [get_ips $ip_name]

    generate_target all [get_ips $ip_name]
    synth_ip [get_ips $ip_name]

    set ipdir $proj_dir/$proj_name.srcs/sources_1/ip/$ip_name

    puts "\n=== instantiation template ===================================="
    foreach veo [glob -nocomplain $ipdir/*.veo] { puts "  $veo" }
    puts "  paste this over the mig_7series_0 block in ddr3_bw_top_SKELETON.sv,"
    puts "  and set APP_ADDR_W / APP_DATA_W there to match."

    file mkdir $repo_ip
    set xci $ipdir/$ip_name.xci
    if {[file exists $xci]} {
        file copy -force $xci $repo_ip/
        puts "\n=== kept ======================================================"
        puts "  $repo_ip/[file tail $xci]"
        puts "  git add -f [file join $repo_ip [file tail $xci]]"
        puts "  (the .xci plus the .prj are the whole answer to 'which memory"
        puts "   settings produced this number' -- the board may carry any of"
        puts "   four different DRAM parts, so beta belongs to the board)"
    } else {
        puts "\nWARNING: no .xci at $xci -- nothing copied back."
    }

    close_project
}

# -----------------------------------------------------------------------------
proc example {proj_dir proj_name ip_name} {
    if {![file exists $proj_dir/$proj_name.xpr]} {
        error "no project at $proj_dir/$proj_name.xpr  (run gen first)"
    }
    open_project $proj_dir/$proj_name.xpr
    open_example_project -force -dir $proj_dir/example [get_ips $ip_name]
    puts "\n=== example design ============================================"
    puts "  $proj_dir/example"
    puts "  Build it, program the board, watch the pass indicator.  This is"
    puts "  Xilinx's own traffic generator: it decides whether the memory is"
    puts "  alive BEFORE any of your RTL is in the picture."
    puts ""
    puts "  While you are in there, note what app_addr increments by per"
    puts "  command -- that is the real ADDR_STRIDE for ddr3_bw_probe."
    puts ""
    puts "  STOP CONDITION: if init_calib_complete is not high by end of 9/3,"
    puts "  stop and push this to October."
    close_project
}

# -----------------------------------------------------------------------------
proc ex_proj {proj_dir} {
    set hits [glob -nocomplain $proj_dir/example/*/*.xpr]
    if {[llength $hits] == 0} {
        error "no example project under $proj_dir/example  (run 'example' first)"
    }
    return [lindex $hits 0]
}

proc build {proj_dir jobs} {
    set xpr [ex_proj $proj_dir]
    puts "example project: $xpr"
    open_project $xpr

    launch_runs impl_1 -to_step write_bitstream -jobs $jobs
    wait_on_run impl_1

    if {[get_property PROGRESS [get_runs impl_1]] ne "100%"} {
        puts "\nimpl_1 did not finish: [get_property STATUS [get_runs impl_1]]"
        error "build failed -- read the run log under [get_property DIRECTORY [get_runs impl_1]]"
    }

    set dir [get_property DIRECTORY [get_runs impl_1]]
    set bits [glob -nocomplain $dir/*.bit]
    puts "\n=== bitstream ================================================="
    foreach b $bits { puts "  $b" }
    set wns [get_property STATS.WNS [get_runs impl_1]]
    puts "  impl WNS = $wns ns"
    puts "\n  next:  vivado -mode batch -source mig_gen.tcl -tclargs program"
    close_project
}

proc program {proj_dir} {
    set xpr [ex_proj $proj_dir]
    open_project $xpr
    set dir  [get_property DIRECTORY [get_runs impl_1]]
    set bits [glob -nocomplain $dir/*.bit]
    if {[llength $bits] == 0} {
        error "no .bit in $dir  (run 'build' first)"
    }
    set bit [lindex $bits 0]
    puts "bitstream: $bit"

    open_hw_manager
    if {[catch {connect_hw_server} msg]} {
        puts "\ncannot reach hw_server: $msg"
        error "is the board powered on and plugged into THIS machine?"
    }
    set targets [get_hw_targets -quiet]
    if {[llength $targets] == 0} {
        puts "\nNO JTAG TARGET FOUND.  Check, in this order:"
        puts "  1. board powered on (the PROG LED near the barrel jack)"
        puts "  2. microUSB in the PROG port, and into THIS machine --"
        puts "     you are on a remote box over ssh, so a board plugged into"
        puts "     your laptop is invisible here"
        puts "  3. cable drivers installed:"
        puts "     sudo <Vivado>/data/xicom/cable_drivers/lin64/install_script/\\"
        puts "          install_drivers/install_drivers"
        puts "  4. lsusb | grep -i -e digilent -e futurelec"
        error "no hardware target"
    }
    open_hw_target [lindex $targets 0]

    set dev ""
    foreach d [get_hw_devices] {
        if {[string match -nocase *xc7a200t* $d]} { set dev $d }
    }
    if {$dev eq ""} { set dev [lindex [get_hw_devices] 0] }
    current_hw_device $dev
    puts "device: $dev"

    set_property PROGRAM.FILE $bit $dev
    program_hw_devices $dev
    refresh_hw_device $dev

    puts "\n=== programmed ================================================"
    puts "  Now LOOK AT THE BOARD.  The MIG example design drives its status"
    puts "  onto the LEDs; init_calib_complete is the one that matters."
    puts "  Lit  -> DDR3 calibrated, the memory is alive.  Gate passed."
    puts "  Dark -> calibration failed.  That is the 9/3 stop condition."
    close_hw_manager
    close_project
}

# -----------------------------------------------------------------------------
switch -- $MODE {
    discover { discover $PART $BOARD_REPO $MIG_PRJ $ARRAY_MHZ }
    gen      { gen $PART $PROJ_DIR $PROJ_NAME $IP_NAME $REPO_IP $MIG_PRJ \
                   $BOARD_REPO $BOARD_PART $ARRAY_MHZ }
    example  { example $PROJ_DIR $PROJ_NAME $IP_NAME }
    build    { build $PROJ_DIR 8 }
    program  { program $PROJ_DIR }
    default  { error "unknown mode '$MODE' (discover | gen | example | build | program)" }
}