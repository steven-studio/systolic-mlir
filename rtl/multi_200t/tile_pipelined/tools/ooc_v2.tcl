# -----------------------------------------------------------------------------
# ooc_v2.tcl -- out-of-context cost of the beat-wide operand path (v2 vs v1).
#
#   vivado -mode batch -source tools/ooc_v2.tcl
#   vivado -mode batch -source tools/ooc_v2.tcl -tclargs synth     ;# faster, no P&R
#
# Produces the numbers for the "v2 writer + buffers (delta vs v1)" row of the
# FPGA'27 draft's resource table, module by module, on the same part and at
# the same clock the DMA top runs at (ui_clk, 100 MHz).  Same recipe as
# ooc_timing.tcl's check_module, plus -generic so the buffer can be built in
# both layouts:
#
#   systolic_operand_buffer      v1, N=8 K_MAX=256          (the baseline, x2)
#   systolic_operand_buffer_v2   LAYOUT_CYCLIC=1  (A side)  expected +24 RAMB18
#   systolic_operand_buffer_v2   LAYOUT_CYCLIC=0  (B side)  expected same RAM as v1
#   dma_operand_writer           v1
#   dma_operand_writer_v2
#   dma_wr_checksum_v2           (v1's checksum is two adders inside the top)
#
# Read the A-side number with the OOC caveat from ooc_timing.tcl in mind: the
# read data leaves through unpacked-array output pins with no output delay,
# so the 4:1 mux after the block RAM register is timed only to the pin.
# Whether it stays inside the read cycle IN CONTEXT is answered by the DMA
# top build (cyc_latched 125 vs 126), not here.  What this script answers is
# the block RAM, LUT and FF cost, and whether anything is hopeless.
# -----------------------------------------------------------------------------

set SCRIPT_DIR [file dirname [file normalize [info script]]]
set ROOT       [file dirname $SCRIPT_DIR]

set PART      xc7a200tsbg484-1
set UI_PERIOD 10.000
set RUN_IMPL  1
set N         8
set K_MAX     256
set OUTDIR    $ROOT/ooc_reports/v2
set IO_BUDGET 2.0      ;# ns charged at each pin, see ooc_cost

if {[info exists argv] && [lsearch -exact $argv synth] >= 0} { set RUN_IMPL 0 }

file mkdir $OUTDIR

# -----------------------------------------------------------------------------
# Count by REF_NAME (LUT6, FDCE, RAMB18E1 ...), which is stable across Vivado
# versions; the PRIMITIVE_TYPE strings are not -- 2026.1 matched none of the
# CLB.LUT.* / REGISTER.SDR.* patterns an earlier revision of this file used,
# and printed 0 LUTs for a module the synthesis report showed 256 LUT6 in.
proc count_prims {pattern} {
    return [llength [get_cells -hierarchical -filter "REF_NAME =~ $pattern"]]
}
proc grab_counts {r} {
    dict set r lut    [count_prims "LUT*"]
    dict set r ff     [count_prims "FD*"]
    dict set r ramb36 [count_prims "RAMB36*"]
    dict set r ramb18 [count_prims "RAMB18*"]
    return $r
}

# returns a dict: lut ff ramb36 ramb18 wns_synth wns_impl
proc ooc_cost {label top files generics period outdir run_impl part} {
    puts "\n=========================================================="
    puts "  $label   ($top)"
    puts "==========================================================\n"

    create_project -in_memory -part $part
    read_verilog -sv $files
    set gen_args {}
    foreach g $generics { lappend gen_args -generic $g }
    synth_design -top $top -part $part -mode out_of_context {*}$gen_args
    create_clock -name ui_clk -period $period [get_ports clk]
    # Out of context there is no reg-to-reg path inside the buffers (pin ->
    # BRAM -> mux -> pin), so without an I/O budget get_timing_paths returns
    # nothing and WNS prints blank.  Charge IO_BUDGET ns at each end: the
    # remaining slack is what the in-context neighbours (writer register on
    # the way in, feeder + PE input register on the way out) get to spend.
    set data_in [remove_from_collection [all_inputs] [get_ports clk]]
    set_input_delay  -clock ui_clk $::IO_BUDGET $data_in
    set_output_delay -clock ui_clk $::IO_BUDGET [all_outputs]

    set tag [string map {" " _ "=" _ "," _} $label]
    report_timing_summary -max_paths 5 -file $outdir/${tag}_synth_timing.rpt
    report_utilization            -file $outdir/${tag}_synth_util.rpt

    set r [grab_counts [dict create]]
    dict set r wns_synth [get_property SLACK [get_timing_paths -max_paths 1 -delay_type max]]
    dict set r wns_impl  "-"
    puts "  \[synth\] LUT [dict get $r lut]  FF [dict get $r ff]  RAMB36 [dict get $r ramb36]  RAMB18 [dict get $r ramb18]  WNS [dict get $r wns_synth] ns"

    if {$run_impl} {
        opt_design
        place_design
        phys_opt_design
        route_design
        report_timing_summary -max_paths 10 -file $outdir/${tag}_impl_timing.rpt
        report_utilization              -file $outdir/${tag}_impl_util.rpt
        set r [grab_counts $r]
        dict set r wns_impl [get_property SLACK [get_timing_paths -max_paths 1 -delay_type max]]
        puts "  \[impl \] LUT [dict get $r lut]  FF [dict get $r ff]  RAMB36 [dict get $r ramb36]  RAMB18 [dict get $r ramb18]  WNS [dict get $r wns_impl] ns   <-- believe this one"
    }

    close_project
    return $r
}

# -----------------------------------------------------------------------------
set BUF1 $ROOT/core/operand_buffer.sv
set BUF2 $ROOT/core/operand_buffer_v2.sv
set WR1  $ROOT/dma/dma_operand_writer.sv
set WR2  $ROOT/dma/dma_operand_writer_v2.sv
set CHK2 $ROOT/dma/dma_wr_checksum_v2.sv

set G_BUF [list "K_MAX=$K_MAX" "N_BANKS=$N"]
set G_DMA [list "N=$N" "K_MAX=$K_MAX"]

set R(buf_v1)   [ooc_cost "buffer v1 N=$N K=$K_MAX"          systolic_operand_buffer    $BUF1 $G_BUF                          $UI_PERIOD $OUTDIR $RUN_IMPL $PART]
set R(buf_v2_A) [ooc_cost "buffer v2 A cyclic N=$N K=$K_MAX" systolic_operand_buffer_v2 $BUF2 [concat $G_BUF LAYOUT_CYCLIC=1] $UI_PERIOD $OUTDIR $RUN_IMPL $PART]
set R(buf_v2_B) [ooc_cost "buffer v2 B block N=$N K=$K_MAX"  systolic_operand_buffer_v2 $BUF2 [concat $G_BUF LAYOUT_CYCLIC=0] $UI_PERIOD $OUTDIR $RUN_IMPL $PART]
set R(wr_v1)    [ooc_cost "writer v1"                        dma_operand_writer         $WR1  $G_DMA                          $UI_PERIOD $OUTDIR $RUN_IMPL $PART]
set R(wr_v2)    [ooc_cost "writer v2"                        dma_operand_writer_v2      $WR2  $G_DMA                          $UI_PERIOD $OUTDIR $RUN_IMPL $PART]
set R(chk_v2)   [ooc_cost "checksum v2"                      dma_wr_checksum_v2         $CHK2 $G_DMA                          $UI_PERIOD $OUTDIR $RUN_IMPL $PART]

# -----------------------------------------------------------------------------
proc row {name r} {
    return [format "  %-34s %7d %7d %7d %7d   %8s %8s" $name \
        [dict get $r lut] [dict get $r ff] [dict get $r ramb36] [dict get $r ramb18] \
        [dict get $r wns_synth] [dict get $r wns_impl]]
}
proc sum {keys} {
    global R
    set s [dict create lut 0 ff 0 ramb36 0 ramb18 0 wns_synth "-" wns_impl "-"]
    foreach k $keys { foreach f {lut ff ramb36 ramb18} { dict incr s $f [dict get $R($k) $f] } }
    return $s
}
proc diff {a b} {
    set d [dict create wns_synth "-" wns_impl "-"]
    foreach f {lut ff ramb36 ramb18} { dict set d $f [expr {[dict get $a $f] - [dict get $b $f]}] }
    return $d
}

set v1_total [sum {buf_v1 buf_v1 wr_v1}]               ;# two v1 buffers (A and B) + writer
set v2_total [sum {buf_v2_A buf_v2_B wr_v2 chk_v2}]

puts "\n=========================================================="
puts "  OOC cost at N=$N K_MAX=$K_MAX, $PART, ui_clk $UI_PERIOD ns"
puts "  (impl numbers if RUN_IMPL=1; BRAM tiles = RAMB36 + RAMB18/2;"
puts "   WNS with $IO_BUDGET ns charged at every data pin -- pin paths, not fmax)"
puts "=========================================================="
puts [format "  %-34s %7s %7s %7s %7s   %8s %8s" "module" LUT FF RAMB36 RAMB18 WNSsyn WNSimpl]
puts [row "buffer v1 (one instance)"        $R(buf_v1)]
puts [row "buffer v2, A side (cyclic)"      $R(buf_v2_A)]
puts [row "buffer v2, B side (block)"       $R(buf_v2_B)]
puts [row "writer v1"                       $R(wr_v1)]
puts [row "writer v2"                       $R(wr_v2)]
puts [row "checksum v2 (4-term chk_wr)"     $R(chk_v2)]
puts "  ----------------------------------------------------------"
puts [row "v1 path: 2 buffers + writer"     $v1_total]
puts [row "v2 path: A + B + writer + chk"   $v2_total]
puts [row "DELTA v2 - v1  (Table 3 row)"    [diff $v2_total $v1_total]]
set d [diff $v2_total $v1_total]
puts [format "  delta in BRAM tiles = %.1f   (paper expects +12 at N=8: 24 RAMB18 on the A side)" \
    [expr {[dict get $d ramb36] + [dict get $d ramb18] / 2.0}]]
puts "\nReports in $OUTDIR"
