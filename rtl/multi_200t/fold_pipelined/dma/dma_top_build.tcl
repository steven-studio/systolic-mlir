# -----------------------------------------------------------------------------
# dma_top_build.tcl -- build and program systolic_dma_top (bring-up step 3b).
#
#   vivado -mode batch -source dma_top_build.tcl -tclargs build   [v1|v2] [K_MAX]
#   vivado -mode batch -source dma_top_build.tcl -tclargs program [v1|v2] [K_MAX]
#   vivado -mode batch -source dma_top_build.tcl -tclargs all     [v1|v2] [K_MAX]   (both)
#   vivado -mode batch -source dma_top_build.tcl -tclargs read    [v1|v2] [K_MAX]
#
#   v1 (default) is the four-cycle operand writer into the single-port buffer;
#   v2 the beat-wide path (USE_V2=1 in systolic_dma_top).  K_MAX defaults to
#   16 (the 3b geometry); 256 is the paper's.  Each (variant, K_MAX) pair gets
#   its own project directory, so the v1 and v2 bitstreams of one geometry
#   coexist and 'read' opens the one you name.  The golden constants for each
#   K_MAX are tabulated below; add a row from seed_ref.py before building a
#   new one.
#
# This is bringup_build.tcl (3a) with the array added: the MIG and the DMA read
# path from that script, plus the floating-point IP and the thirteen RTL files
# that uart/build_kmax.tcl builds systolic_uart_top from.
#
# WHY A PROJECT FLOW AND NOT build_kmax.tcl's IN-MEMORY ONE
#   MIG's .xci was customised against a board part, and recreating it needs the
#   board available -- "ERROR: [Board 49-60] No current board set" otherwise.
#   An in-memory project has no board part, so build_kmax.tcl's flow cannot
#   instantiate this MIG.  Going the other way, a project flow synthesises the
#   floating-point IP as ordinary IP runs, which is what build_kmax.tcl's
#   explicit read_ip / synth_ip does by hand.  So the project flow can do both
#   jobs and the in-memory one cannot; that is the whole reason.
#
#   The one thing the project flow does NOT inherit is build_kmax.tcl's
#   set_clock_uncertainty -hold 0.150.  That is carried over deliberately, as a
#   pre-place hook -- see hold_margin.tcl, and read its header before removing
#   it.  It is not decoration; it is the fix for the 2026-08-19 placement
#   lottery, and this design instantiates the same array.
#
# READ THE UART PATH AS WELL AS THE PROBES
#   This bitstream has both.  'read' reports what the DMA-fed fold produced;
#   tools/uart_check.py then feeds the SAME operands over UART to the SAME
#   array and checks the answer independently.  Two ways in, one array, and the
#   golden that both are compared against was confirmed on hardware before any
#   of this was built.
# -----------------------------------------------------------------------------

set PART       xc7a200tsbg484-1
set BOARD_PART digilentinc.com:nexys_video:part0:1.2
set BOARD_REPO $::env(HOME)/work/vivado/vivado-boards/new/board_files
set PROJ_NAME  systolic_dma
set TOP        systolic_dma_top
set JOBS       8

# ---- arguments: mode [variant] [K_MAX] [n_inv] --------------------------
# n_inv is how many invocations the RUN makes, not anything about the build:
# one bitstream serves every count, because systolic_dma_top takes the number
# from vio_0's output probe.  'read' sets it and re-runs; 'sweep' walks
# 1..n_inv and writes one CSV row per point.  The build ignores it.
set MODE    [lindex $argv 0]
set VARIANT [lindex $argv 1]
set KARG    [lindex $argv 2]
set NARG    [lindex $argv 3]
if {$NARG eq ""} { set NARG 1 }
if {![string is integer -strict $NARG] || $NARG < 1 || $NARG > 15} {
    error "n_inv must be 1..15 (the probe is four bits), got '$NARG'"
}
if {$MODE eq ""}    { set MODE all }
if {$VARIANT eq ""} { set VARIANT v1 }
if {$VARIANT ne "v1" && $VARIANT ne "v2"} {
    error "variant must be v1 or v2, got '$VARIANT'"
}
set USE_V2 [expr {$VARIANT eq "v2" ? 1 : 0}]

# Geometry.  K_MAX = 16 is the 3b geometry: the golden confirmed on the board
# over UART, a 1 KiB payload, a build measured in minutes.  K_MAX = 256 is the
# paper's: 16 KiB, 1024 beats, the geometry every Table 4 number is quoted at.
# K_DIM = K_MAX: one fold of the full depth.
set NARR   8
set KMAX   [expr {$KARG eq "" ? 16 : $KARG}]
set KDIM   $KMAX

# From:  python3 tools/seed_ref.py --mode 1 --kmax <K_MAX>
# Passed down as generics so this script is the single source of truth; the
# defaults in systolic_dma_top.sv agree for 16, but only one place should
# decide.  These are folded constants in hardware with no net behind them, so
# they are NOT probed -- 'read' prints them from here.  3a shipped a bitstream
# that printed "expected 0x" because that lesson had not been learned yet.
# EXPECT_CYC = K_DIM + 2(N-1) + H with H = 95 measured on paper-hw-v1.
array set GOLDEN {
    16  {3f880780 c74b2660 125}
    32  {805c1f00 e906e560 141}
    256 {2dc7f800 54ccf610 365}
}
# The 32 row is the k_max split the cost model is tested on: eight invocations
# of k = 32 against one of k = 256, same K, same operands.  Its constants are
# PER INVOCATION.  chk_wr and chk_c are run totals, so what the design compares
# against -- and what 'read' prints -- is the constant times the number of
# invocations, mod 2^32.  seed_ref.py's own mode-0 self-check only reproduces
# the board's 0x387fdc00 at --kmax 16, where that constant was measured; a
# "self-check FAILED" line from any other geometry says nothing about this row.
if {![info exists GOLDEN($KMAX)]} {
    error "no golden constants tabulated for K_MAX=$KMAX -- run\n  python3 tools/seed_ref.py --mode 1 --kmax $KMAX\nand add a GOLDEN row"
}
lassign $GOLDEN($KMAX) EXPECT_WR_CHK EXPECT_C_CHK EXPECT_CYC

# One project per (variant, K_MAX) so bitstreams coexist and 'read' cannot open
# the wrong one.  The pre-Sep-10 3b build lives in plain systolic_dma/ and is
# left alone.
set PROJ_DIR   $::env(HOME)/work/vivado/systolic_dma_${VARIANT}_k${KMAX}

# Where the result tile is written back.  One page clear of the operand image,
# which is K_MAX*8*N bytes long (1 KiB at K_MAX = 16, 16 KiB at 256), so that
# an address error in either direction shows up as a wrong image rather than
# as one quietly overwriting the other.  4096 was fine for 3b; at K_MAX = 256
# it would land INSIDE the operand image, so it is computed from the geometry.
#
# It is no longer a generic.  The operand image is n_inv slabs long and n_inv is
# a run-time number, so systolic_dma_top computes the base from the n_inv it
# latched and this is only what the script PRINTS -- one place decides, and it
# is the one that knows how many slabs were seeded.
proc wb_base {n} {
    global KMAX NARR
    expr {$n * $KMAX * 8 * $NARR + 4096}
}
set WB_BASE       [expr {$KMAX * 8 * $NARR + 4096}]

set SCRIPT_DIR [file dirname [file normalize [info script]]]
set ROOT       [file normalize $SCRIPT_DIR/..]      ;# .../fold_pipelined

set XCI     $SCRIPT_DIR/ip/ddr3/mig_7series_0.xci
set FP_DIR  $ROOT/ip/fp32
set FP_IPS  [list floating_point_add_0 floating_point_mul_0]
set XDC     $SCRIPT_DIR/nexys_video_dma_top.xdc
set HOLD_TCL $SCRIPT_DIR/hold_margin.tcl

# The DMA side, then the compute core.
#
# NOTE WHAT IS ABSENT: uart/systolic_uart_top.sv, and everything that exists
# only to serve it -- uart_rx, uart_tx, uart_tx_streamer, core/status.sv,
# core/tx_source.sv.  systolic_dma_top instantiates the array, the feeder and
# the operand buffers directly, so the UART design is not part of this build in
# any form and is not edited by it.  That is deliberate: the file that carries
# the 48-configuration validation stays byte-identical.
#
# The seven core files below are uart/build_kmax.tcl's RTL_FILES minus the UART
# transport.  If that list gains a file the array needs, add it here too -- a 3b
# build made from a different set of sources is not exercising the same array.
set SRC [list \
    $SCRIPT_DIR/systolic_dma_top.sv \
    $SCRIPT_DIR/dma_engine.sv \
    $SCRIPT_DIR/dma_seed_writer.sv \
    $SCRIPT_DIR/dma_operand_writer.sv \
    $SCRIPT_DIR/dma_operand_writer_v2.sv \
    $SCRIPT_DIR/dma_wr_checksum_v2.sv \
    $SCRIPT_DIR/dma_result_reader.sv \
    $SCRIPT_DIR/dma_writeback_engine.sv \
    $SCRIPT_DIR/dma_cdc_fifo.sv \
    $ROOT/core/fp_mul.sv \
    $ROOT/core/fp_add.sv \
    $ROOT/core/fp_reduce16.sv \
    $ROOT/core/systolic_pe_bram.sv \
    $ROOT/core/systolic_array.sv \
    $ROOT/core/tile_feeder.sv \
    $ROOT/core/operand_buffer.sv \
    $ROOT/core/operand_buffer_v2.sv ]

# -----------------------------------------------------------------------------
proc build {} {
    global PART PROJ_DIR PROJ_NAME TOP XCI SRC XDC JOBS BOARD_REPO BOARD_PART

    foreach f [concat [list $XCI $XDC] $SRC] {
        if {![file exists $f]} { error "missing: $f" }
    }

    # DO NOT DELETE A WORKING PROJECT BEFORE THE NEW ONE EXISTS.  3a's script
    # opened with 'file delete -force', the build then failed at import_ip, and
    # the previous bitstream and .ltx were already gone -- 'read' could no
    # longer name a probe on a board still happily running the old design.
    set prev $PROJ_DIR.prev
    file delete -force $prev
    set had_prev 0
    if {[file isdirectory $PROJ_DIR]} {
        file rename $PROJ_DIR $prev
        set had_prev 1
        puts "previous project moved aside: $prev"
    }
    if {[catch {build_body} msg]} {
        puts "\n=== BUILD FAILED =============================================="
        puts "  $msg"
        if {$had_prev} {
            file delete -force $PROJ_DIR
            file rename $prev $PROJ_DIR
            puts "\n  The previous project has been PUT BACK, bitstream and .ltx"
            puts "  intact.  The board was never reprogrammed."
        }
        error "build failed"
    }
    file delete -force $prev
}

proc build_body {} {
    global PART PROJ_DIR PROJ_NAME TOP XCI SRC XDC JOBS BOARD_REPO BOARD_PART
    global FP_DIR FP_IPS HOLD_TCL NARR KMAX KDIM EXPECT_WR_CHK EXPECT_C_CHK
    global WB_BASE USE_V2 VARIANT

    create_project -force $PROJ_NAME $PROJ_DIR -part $PART

    if {[file isdirectory $BOARD_REPO]} {
        set_property BOARD_PART_REPO_PATHS $BOARD_REPO [current_project]
    } else {
        puts "WARNING: board repo not found: $BOARD_REPO"
    }
    if {[lsearch -exact [get_board_parts -quiet] $BOARD_PART] >= 0} {
        set_property board_part $BOARD_PART [current_project]
        puts "board_part = $BOARD_PART"
    } else {
        puts "WARNING: board part '$BOARD_PART' unavailable; MIG will fail to"
        puts "  recreate the IP.  Present nexys_video parts:"
        foreach bp [get_board_parts -quiet] {
            if {[string match -nocase *nexys_video* $bp]} { puts "    $bp" }
        }
    }

    # MIG re-runs its customisation Tcl at import and cds into the IP's
    # generated-sources directory to resolve the .prj path relative to it.  At
    # import_ip time Vivado has not created that directory yet.  Creating it up
    # front costs nothing and removes the ordering hazard -- 3a lost a working
    # build to exactly this.
    file mkdir $PROJ_DIR/$PROJ_NAME.gen/sources_1/ip/mig_7series_0

    if {[catch {import_ip -files $XCI} m]} {
        puts "\nimport_ip failed (MIG): $m"
        error "import_ip failed"
    }

    # Floating-point IP: the same two cores uart/build_kmax.tcl reads.  IMPORTED
    # rather than referenced, so this project writes its generated output into
    # its own tree instead of next to the pinned .xci in the repo.
    foreach ip $FP_IPS {
        set p [file join $FP_DIR $ip ${ip}.xci]
        if {![file exists $p]} { set p [file join $FP_DIR ${ip}.xci] }
        if {![file exists $p]} { error "missing floating-point IP: $ip under $FP_DIR" }
        if {[catch {import_ip -files $p} m]} {
            puts "\nimport_ip failed ($ip): $m"
            error "import_ip failed"
        }
    }

    foreach ip [get_ips] {
        if {[get_property IS_LOCKED $ip]} {
            report_ip_status
            error "IP $ip is locked -- see report_ip_status above (part mismatch?)"
        }
    }

    # VIO: FOURTEEN probes -- 3b's five plus the nine operand-path counters
    # (probe_in5..13, all 32-bit).  3a's vio_0 had probe_in3 at 8 bits; reusing
    # a stale configuration silently truncates a 32-bit probe, and a truncated
    # count still looks like a count.  So the widths are all spelled out.
    create_ip -name vio -vendor xilinx.com -library ip -version 3.0 \
              -module_name vio_0
    # 17 in, 2 out.  The three new inputs are t_span, cyc_total and the
    # n_inv/folds_done status word; the outputs are n_inv itself and the re-run
    # request.  n_inv's INIT is 1, so a freshly programmed board runs exactly
    # what it ran before the invocation loop existed -- the single-invocation
    # numbers in the paper are reproducible without touching a probe.
    set vio_cfg [list CONFIG.C_NUM_PROBE_IN {17} CONFIG.C_NUM_PROBE_OUT {2}]
    for {set i 0} {$i < 17} {incr i} {
        lappend vio_cfg CONFIG.C_PROBE_IN${i}_WIDTH [expr {$i == 4 ? 8 : 32}]
    }
    lappend vio_cfg CONFIG.C_PROBE_OUT0_WIDTH {4} CONFIG.C_PROBE_OUT0_INIT_VAL {0x1}
    lappend vio_cfg CONFIG.C_PROBE_OUT1_WIDTH {1} CONFIG.C_PROBE_OUT1_INIT_VAL {0x0}
    set_property -dict $vio_cfg [get_ips vio_0]

    foreach f $SRC { add_files -norecurse $f }
    add_files -fileset constrs_1 -norecurse $XDC
    set_property top $TOP [current_fileset]

    # Geometry and the golden constants come from this script.  systolic_dma_top
    # passes DMA_PORT / CYCLE_COUNTER / DEBUG_MARKERS down to systolic_uart_top
    # itself, so those are not generics here -- they are structural.
    set_property generic [list \
        N=$NARR K_MAX=$KMAX K_DIM=$KDIM \
        USE_V2=1'b$USE_V2 \
        EXPECT_WR_CHK=32'h$EXPECT_WR_CHK \
        EXPECT_C_CHK=32'h$EXPECT_C_CHK ] [current_fileset]
    puts "building $VARIANT at N=$NARR K_MAX=$KMAX K_DIM=$KDIM (USE_V2=$USE_V2), project $PROJ_DIR"

    generate_target all [get_ips]
    update_compile_order -fileset sources_1

    # Carry over build_kmax.tcl's hold margin.  See hold_margin.tcl's header.
    # Added to utils_1 first: a hook script that is not a fileset member still
    # RUNS, but Vivado warns that it is outside the project (Runs 36-537), and a
    # project archived without it would place with no hold margin and give no
    # sign of it.
    if {![file exists $HOLD_TCL]} { error "missing: $HOLD_TCL" }
    add_files -fileset utils_1 -norecurse $HOLD_TCL
    set_property STEPS.PLACE_DESIGN.TCL.PRE $HOLD_TCL [get_runs impl_1]

    launch_runs impl_1 -to_step write_bitstream -jobs $JOBS
    wait_on_run impl_1

    if {[get_property PROGRESS [get_runs impl_1]] ne "100%"} {
        set d [get_property DIRECTORY [get_runs impl_1]]
        puts "\nimpl_1 did not finish: [get_property STATUS [get_runs impl_1]]"
        puts "  read: $d/runme.log"
        error "build failed"
    }

    set dir  [get_property DIRECTORY [get_runs impl_1]]
    set wns  [get_property STATS.WNS [get_runs impl_1]]
    set whs  [get_property STATS.WHS [get_runs impl_1]]
    puts "\n=== bitstream ================================================="
    puts "  $VARIANT  N=$NARR K_MAX=$KMAX"
    foreach b [glob -nocomplain $dir/*.bit] { puts "  $b" }
    puts "  WNS = $wns ns"
    puts "  WHS = $whs ns   (already includes 0.150 ns of forced hold pessimism)"
    if {$wns < 0 || $whs < 0} {
        puts ""
        puts "  TIMING NOT MET.  Do not quote any number this bitstream reports."
        puts "  3a shipped a checksum from a bitstream with WNS -1.011 ns; the"
        puts "  number happened to be right, which is not the same as being"
        puts "  trustworthy.  Rebuild before reading."
    }
    close_project
}

# -----------------------------------------------------------------------------
proc program {} {
    global PROJ_DIR PROJ_NAME VARIANT KMAX
    # Re-point the board repo before opening: without it Vivado prints
    # "Board part ... is not found. BoardPart property will be unset", which is
    # harmless here (nothing is regenerated) but is exactly the kind of warning
    # that teaches you to skim past warnings.
    global BOARD_REPO
    if {[file isdirectory $BOARD_REPO]} {
        set_param board.repoPaths $BOARD_REPO
    }
    open_project $PROJ_DIR/$PROJ_NAME.xpr
    set dir  [get_property DIRECTORY [get_runs impl_1]]
    set bits [glob -nocomplain $dir/*.bit]
    if {[llength $bits] == 0} { error "no .bit in $dir  (run 'build' first)" }
    set bit [lindex $bits 0]

    open_hw_manager
    if {[catch {connect_hw_server} msg]} {
        puts "\ncannot reach hw_server: $msg"
        error "board powered on and plugged into THIS machine?"
    }
    set dev [pick_device]
    set_property PROGRAM.FILE $bit $dev
    program_hw_devices $dev
    refresh_hw_device -quiet $dev

    puts "\n=== programmed ================================================"
    puts "  LOOK AT THE BOARD.  LEDs, right to left:"
    puts "    led\[0\]  init_calib_complete   memory alive"
    puts "    led\[1\]  seed written"
    puts "    led\[2\]  descriptor complete"
    puts "    led\[3\]  fold complete"
    puts "    led\[4\]  chk_wr MATCHES        operands arrived correctly"
    puts "    led\[5\]  chk_c  MATCHES        <-- THE GATE"
    puts "    led\[6\]  any error latched"
    puts "    led\[7\]  heartbeat, ui_clk"
    puts ""
    puts "  The write-back has no LED -- all eight are spoken for, and"
    puts "  renumbering them would invalidate every photograph of a previous"
    puts "  run.  It is in the JTAG flag word; a write-back fault raises"
    puts "  led\[6\] like any other fault."
    puts ""
    puts "  Then:"
    puts "    vivado -mode batch -source dma_top_build.tcl -tclargs read $VARIANT $KMAX"
    puts ""
    puts "  This bitstream has NO UART -- uart_check.py will time out against it."
    puts "  The independent reference lives in its own bitstream: program a"
    puts "  build_kmax build and run uart_check.py there, before or after."
    close_hw_manager
    close_project
}

# -----------------------------------------------------------------------------
# The FT2232H registers TWO JTAG targets and only one reaches the FPGA; the
# other scans empty.  Taking [lindex $targets 0] fails about half the time.
# Keep the NAMES as strings: a hw_target handle does not survive a reconnect.
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
        set dev [lindex $devs 0]
        foreach d $devs { if {[string match -nocase *xc7a200t* $d]} { set dev $d } }
        puts "  using: $n  device $dev"
        current_hw_device $dev
        return $dev
    }
    error "every JTAG target scanned empty -- FPGA not on the chain"
}

# -----------------------------------------------------------------------------
# Put the board on a given invocation count and start a fresh run.  The design
# latches n_inv on the way out of P_CALIB and clears every counter there, so the
# order matters: write n_inv first, then edge the re-run probe.  The request is
# only honoured in P_DONE, which is where a finished board sits.
proc arm_run {v n} {
    set pn [get_hw_probes -of_objects $v -quiet]
    set probe_n ""
    set probe_r ""
    foreach p $pn {
        set nm [get_property NAME $p]
        if {[string match "*n_inv_probe*" $nm]} { set probe_n $p }
        if {[string match "*rerun_probe*" $nm]} { set probe_r $p }
    }
    if {$probe_n eq "" || $probe_r eq ""} {
        # An older bitstream -- the k_max = 256 pair, say, built before the
        # invocation loop.  It still runs, and it still runs exactly one
        # invocation, so reading it is meaningful; only asking it for a
        # different n_inv is not.  Warn and read what is there rather than
        # refusing, but refuse to pretend a request was honoured.
        if {$n != 1} {
            error "this bitstream has no n_inv probe (it predates the invocation\n\
                   loop) and cannot run n_inv = $n.  Rebuild it, or read it at n_inv = 1."
        }
        puts "  NOTE: no n_inv/rerun probe -- pre-loop bitstream, reading its one run as it stands."
        return
    }
    catch {set_property OUTPUT_VALUE_RADIX UNSIGNED $probe_n}
    catch {set_property OUTPUT_VALUE_RADIX UNSIGNED $probe_r}
    set_property OUTPUT_VALUE $n $probe_n
    commit_hw_vio $probe_n
    set_property OUTPUT_VALUE 1 $probe_r
    commit_hw_vio $probe_r
    after 200
    set_property OUTPUT_VALUE 0 $probe_r
    commit_hw_vio $probe_r
    # The run itself is tens of microseconds at 100 MHz; this is JTAG latency.
    after 500
    refresh_hw_vio $v
}

proc read_vio {} {
    global PROJ_DIR PROJ_NAME EXPECT_WR_CHK EXPECT_C_CHK EXPECT_CYC KMAX NARR KDIM
    global WB_BASE VARIANT USE_V2
    # Re-point the board repo before opening: without it Vivado prints
    # "Board part ... is not found. BoardPart property will be unset", which is
    # harmless here (nothing is regenerated) but is exactly the kind of warning
    # that teaches you to skim past warnings.
    global BOARD_REPO
    if {[file isdirectory $BOARD_REPO]} {
        set_param board.repoPaths $BOARD_REPO
    }
    open_project $PROJ_DIR/$PROJ_NAME.xpr
    open_hw_manager
    connect_hw_server
    set dev [pick_device]

    set dir [get_property DIRECTORY [get_runs impl_1]]
    set ltx [glob -nocomplain $dir/*.ltx]
    if {[llength $ltx] > 0} {
        set_property PROBES.FILE      [lindex $ltx 0] $dev
        set_property FULL_PROBES.FILE [lindex $ltx 0] $dev
    }
    refresh_hw_device -quiet $dev

    set vios [get_hw_vios -quiet]
    if {[llength $vios] == 0} { error "no VIO found -- was this design rebuilt with it?" }
    set v [lindex $vios 0]
    refresh_hw_vio $v
    global NARG
    arm_run $v $NARG

    # Vivado names each probe after the NET that drives it, not probe_inN, and
    # it splits a concatenation into one probe per signal.  Match on net names
    # with a probe_inN fallback, and list them: if a lookup comes back empty the
    # names are the first thing to look at.
    set all_probes [get_hw_probes -of_objects $v -quiet]
    puts "\n=== probes found: [llength $all_probes] ==="
    foreach p $all_probes { puts "  [get_property NAME $p]" }

    # INPUT_VALUE is a STRING formatted by INPUT_VALUE_RADIX, and the default
    # radix is not necessarily hex.  Pin it, or "0x$chk" is a lie.
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

    # The probe is named after the NET, and the net is cyc_latched -- there is no
    # dma_cyc in the standalone top, that name belonged to the version that went
    # through systolic_uart_top's observation port.  Asking for the wrong name
    # returned "" and the cycle count printed blank, which is how a check can go
    # missing without anything looking wrong.
    set wchk  [pval $all_probes HEX      chk_wr        probe_in0]
    set cchk  [pval $all_probes HEX      chk_c         probe_in1]
    set cyc   [pval $all_probes UNSIGNED cyc_latched   probe_in2]
    set wrds  [pval $all_probes UNSIGNED words_written probe_in3]

    set calib [pval $all_probes BINARY init_calib_complete]
    set sd    [pval $all_probes BINARY seed_done_sticky]
    set rd    [pval $all_probes BINARY read_done_sticky]
    set fd    [pval $all_probes BINARY fold_done_sticky]
    set wm    [pval $all_probes BINARY wr_match]
    set cm    [pval $all_probes BINARY c_match]
    set er    [pval $all_probes BINARY any_err]
    set wb    [pval $all_probes BINARY wb_done_sticky]

    # the operand-path counters (probe_in5..13)
    set fill    [pval $all_probes UNSIGNED fill_cycles          probe_in5]
    set ebusy   [pval $all_probes UNSIGNED eng_busy_cycles      probe_in6]
    set erdy    [pval $all_probes UNSIGNED eng_rdy_stall_cycles probe_in7]
    set erst    [pval $all_probes UNSIGNED eng_r_stall_cycles   probe_in8]
    set wbcyc   [pval $all_probes UNSIGNED wb_cycles            probe_in9]
    set wbbusy  [pval $all_probes UNSIGNED wb_busy_cycles       probe_in10]
    set wbaw    [pval $all_probes UNSIGNED wb_aw_stall_cycles   probe_in11]
    set wbw     [pval $all_probes UNSIGNED wb_w_stall_cycles    probe_in12]
    set wbstv   [pval $all_probes UNSIGNED wb_src_starve_cycles probe_in13]

    # the invocation loop's three (probe_in14..16)
    set tspan [pval $all_probes UNSIGNED t_span     probe_in14]
    set ctot  [pval $all_probes UNSIGNED cyc_total  probe_in15]
    set nrun  [pval $all_probes UNSIGNED n_inv]
    set fdone [pval $all_probes UNSIGNED folds_done]
    if {$nrun eq "" || $nrun == 0} { set nrun 1 }

    if {$wchk eq "" || $cyc eq ""} {
        puts "\nA probe lookup came back EMPTY.  Read the list above: those are"
        puts "the real names in the bitstream on the board, and one of them is"
        puts "not what this script asked for.  Do not read past this point --"
        puts "a blank value is an absent check, not a passing one."
    }

    # Every expectation below is a RUN total: $nrun invocations, each seeding
    # and reading the same slab pattern into the same buffers.
    set want_words [expr {$nrun * $KMAX * 2 * $NARR}]
    # 0x$VAR does not substitute inside a braced expr -- Tcl parses the
    # expression before the variable is there.  Make the string first.
    set wr_hex 0x$EXPECT_WR_CHK
    set c_hex  0x$EXPECT_C_CHK
    set want_wchk  [format %08x [expr {($nrun * $wr_hex) & 0xffffffff}]]
    set want_cchk  [format %08x [expr {($nrun * $c_hex)  & 0xffffffff}]]

    puts "\n=== bring-up step 3b =========================================="
    puts "  $VARIANT operand path   N = $NARR   K_MAX = $KMAX   k_dim = $KDIM"
    puts ""
    puts "  init_calib_complete = $calib     memory alive"
    puts "  seed written        = $sd"
    puts "  descriptor complete = $rd"
    puts "  fold complete       = $fd"
    puts "  write-back complete = $wb     $nrun result tile(s) from 0x[format %x [wb_base $nrun]]"
    puts "    (that the writes were ACCEPTED and every response returned.  That"
    puts "     the right bytes are in memory is proved in simulation by"
    puts "     tb/run_top_sim.sh; reading the image back on the board is the"
    puts "     next step and is not in this bitstream.)"
    puts "  invocations         = $nrun     (folds_done $fdone)"
    puts "  words written       = $wrds     (expect $want_words = n_inv*K_MAX*2*N)"
    puts ""
    set cyc_ok [expr {$cyc ne "" && $cyc == $EXPECT_CYC}]

    puts "  chk_wr  0x$wchk     expected 0x$want_wchk    operands ($nrun x 0x$EXPECT_WR_CHK)"
    puts "  chk_c   0x$cchk     expected 0x$want_cchk    result   ($nrun x 0x$EXPECT_C_CHK)"
    puts "  cycles  $cyc              expected $EXPECT_CYC    control-FSM equivalence (125 vs 126 is the v2 read-mux question)"
    puts ""
    puts "  any error latched   = $er"
    puts ""

    # ---- the operand path, stage by stage (Table 4 / Sec. 5.2 of the draft) --
    set beats [expr {$want_words / 4}]
    puts "=== operand path ($VARIANT) ==================================="
    if {$fill ne "" && $fill > 0} {
        puts [format "  fill        %6s cycles   %5.2f words/cycle   (%d words, %d beats)" \
              $fill [expr {double($want_words) / $fill}] $want_words $beats]
    } else {
        puts "  fill        $fill cycles   <-- empty or zero: probe missing or fill never completed"
    }
    puts "  read engine busy $ebusy   rdy_stall $erdy   r_stall $erst"
    if {$fill ne "" && $fill > 0 && $erst ne ""} {
        puts [format "              r_stall / fill = %.2f   (v1: ~0.75, the port; v2: ~0, the port is gone)" \
              [expr {double($erst) / $fill}]]
    }
    puts "  compute     $cyc cycles   (cyc_latched; expected $EXPECT_CYC on both paths)"
    puts "  write-back  $wbcyc cycles   wb engine busy $wbbusy   aw_stall $wbaw   w_stall $wbw   src_starve $wbstv"
    # T is fill + compute + write-back, summed over the run's invocations -- the
    # same three counters at n_inv = 1, where cyc_total is one invocation.  It is
    # NOT the wall clock: t_span is, and the difference is P_GO, the hand-offs
    # and n_inv scans of C, which are instrumentation.  Both are printed because
    # a total that quietly included the checksum scan would be a slower design
    # than the one the paper describes.
    if {$fill ne "" && $ctot ne "" && $wbcyc ne "" && $fill > 0 && $ctot > 0 && $wbcyc > 0} {
        set total [expr {$fill + $ctot + $wbcyc}]
        puts [format "  total       %d cycles, serial   MAC utilisation K/T = %.1f%%" \
              $total [expr {100.0 * $nrun * $KDIM / $total}]]
        if {$nrun > 1} {
            puts [format "              %d invocations of k = %d: %.1f cycles each (fill %.1f, compute %.1f, wb %.1f)" \
                  $nrun $KDIM [expr {double($total)/$nrun}] \
                  [expr {double($fill)/$nrun}] [expr {double($ctot)/$nrun}] \
                  [expr {double($wbcyc)/$nrun}]]
        }
        if {$tspan ne "" && $tspan > 0} {
            puts [format "              t_span %d (first AR to last B); %d of it is the C scan and the hand-offs" \
                  $tspan [expr {$tspan - $total}]]
        }
    }
    if {$USE_V2} {
        puts "  v2 expectation at K=256: fill ~1129 (controller-bound, 3.63 w/c), r_stall ~0."
        puts "  If r_stall is still ~3/4 of fill the v2 writer is not the one in the bitstream."
    } else {
        puts "  v1 expectation at K=256: fill ~4096 (port-bound, 1.00 w/c), r_stall ~3/4 of fill."
    }
    puts ""

    # The cycle count is part of the verdict, not a footnote.  systolic_dma_top
    # duplicates systolic_uart_top's control FSM rather than instantiating it,
    # and this is the only check that the duplicate is faithful: the checksums
    # would still match if the copy had drifted by a cycle and happened to
    # produce the right answer anyway.  A verdict that ignored it would be
    # declaring something it had not tested.
    if {$cm eq "1" && $wm eq "1" && $cyc_ok} {
        puts "  ALL THREE MATCH: operands, result, and cycle count."
        puts "  The array computed the right answer from operands that arrived"
        puts "  over DDR3, with no host involved, in the same number of cycles"
        puts "  systolic_uart_top takes -- so the copied control FSM is faithful."
        puts ""
        puts "  Step 3b is done.  The independent reference is the UART build:"
        puts "    vivado -mode batch -source ../uart/program_kmax.tcl -tclargs $KMAX"
        puts "    python3 ../tools/uart_check.py --port /dev/ttyUSB2 --kmax $KMAX"
        puts "  feeds the SAME operands to the SAME array over UART.  If that"
        puts "  also passes, two disjoint input paths agree with one model that"
        puts "  was confirmed on hardware beforehand."
    } elseif {$cm eq "1" && $wm eq "1"} {
        puts "  CHECKSUMS MATCH BUT THE CYCLE COUNT DOES NOT ($cyc, expected $EXPECT_CYC)."
        puts ""
        puts "  This is NOT a DMA failure -- the operands arrived correctly and"
        puts "  the array produced the right answer.  It means systolic_dma_top's"
        puts "  copy of systolic_uart_top's control FSM has drifted: same result,"
        puts "  different timing.  The right answer from the wrong machine is"
        puts "  still the wrong machine, and 3b's claim is that this design"
        puts "  exercises the SAME array behaviour as the validated one."
        puts ""
        puts "  Compare the two side by side: the ST_FEED exit condition, when"
        puts "  cyc_running arms, and whether c_valid_out is sampled on the same"
        puts "  edge.  Do not adjust EXPECT_CYC to match what came back."
    } elseif {$wm eq "1"} {
        puts "  OPERANDS ARRIVED, THE RESULT IS WRONG."
        puts "  chk_wr matching means every word reached the right bank and"
        puts "  address, so the DMA path is not at fault.  Look at the array"
        puts "  side: was k_dim what you meant ($KDIM)?  Did the fold start"
        puts "  before the last operand landed?  Compare cycles against"
        puts "  $EXPECT_CYC -- a short count means the fold began early."
    } else {
        puts "  OPERANDS DID NOT ARRIVE CORRECTLY.  Read the flags in order:"
        puts "    init_calib = 0      memory never came up; nothing below means"
        puts "                        anything yet"
        puts "    seed = 0            the AXI write channel is stuck"
        puts "    descriptor = 0      reads issued but never completed"
        puts "    collision = 1       UART and DMA wrote the buffers at once --"
        puts "                        something sent a frame during bring-up"
        puts "    any error = 1       alignment, response or range: the design"
        puts "                        refused work rather than moving garbage"
        puts "    words != $want_words   beats lost or misfiled"
    }
    # One row per point, appended: the sweep is eight runs and the interesting
    # number is the SLOPE through them -- how many cycles each extra invocation
    # costs -- which is the per-descriptor start-up measured rather than
    # inferred from one fill's excess over beta_mem.
    global CSV
    set csv [open $CSV a]
    if {[file size $CSV] == 0} {
        puts $csv "variant,k_max,k_dim,n_inv,fill,compute,wb,total,t_span,eng_busy,rdy_stall,r_stall,chk_wr,chk_c,wr_match,c_match,any_err"
    }
    puts $csv "$VARIANT,$KMAX,$KDIM,$nrun,$fill,$ctot,$wbcyc,[expr {$fill+$ctot+$wbcyc}],$tspan,$ebusy,$erdy,$erst,$wchk,$cchk,$wm,$cm,$er"
    close $csv
    puts "  appended to $CSV"

    close_hw_manager
    close_project
}

# Walk n_inv = 1..NARG on the board that is already programmed.  Each point is a
# fresh run: the design clears every counter in P_CALIB, which the bench proves
# by re-running and demanding the identical numbers, so the rows are comparable
# without reprogramming between them.
proc sweep_vio {} {
    global NARG
    set top $NARG
    for {set n 1} {$n <= $top} {incr n} {
        set NARG $n
        puts "\n########## n_inv = $n ##########"
        read_vio
    }
    set NARG $top
}

# -----------------------------------------------------------------------------
set CSV $PROJ_DIR/ninv_sweep.csv

switch -- $MODE {
    build   { build }
    program { program }
    read    { read_vio }
    sweep   { sweep_vio }
    all     { build ; program }
    default { error "unknown mode '$MODE' (build | program | all | read | sweep)" }
}