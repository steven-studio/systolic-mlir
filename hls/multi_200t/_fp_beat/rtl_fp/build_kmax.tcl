# build_kmax.tcl -- one synthesis+implementation run at a given K_MAX.
#
#   vivado -mode batch -source build_kmax.tcl -tclargs <K_MAX>
#
# K_MAX is the SYNTHESIS-TIME HARDWARE CAPACITY: the deepest reduction the
# on-chip operand buffers can hold. It is not the workload's K, and it is
# not a fold count.
#
#   K_MAX   synthesis-time hardware capacity     <- this script's argument
#   k_dim   runtime workload reduction length    <- supplied by software
#   folds   k_dim / 8, derived at runtime        <- not a hardware quantity
#
# =====================================================================
#  DISABLED -- waiting on the K_MAX parameter in the RTL
# =====================================================================
#
# An earlier revision of this script drove a top-level `NFOLD` generic.
# That was the wrong abstraction: it baked the fold COUNT into the
# hardware, when the fold count is a scheduling quantity software derives
# from k_dim. NFOLD has been removed from systolic_uart_fold_top.sv, and
# this script must not reintroduce it under a different name.
#
# systolic_uart_fold_top.sv currently has NO capacity parameter at all --
# it is the fixed K=16 baseline, with two 8-deep operand-buffer slots and
# a 1024-byte / 512-byte wire format.
#
# TODO(K_MAX): once the RTL exposes
#
#     parameter int K_MAX = 16
#
# and sizes A_buf/B_buf, rx_count, feed_t and the RX framing from it,
# delete the guard below and uncomment the -generic line in synth_design.
# Everything else in this file is already correct and reusable: the flow,
# the report set, and the summary.csv writer that reads results back out
# of Vivado's own APIs instead of scraping .rpt text.
#
# Until then the k_max sweep is simulation-only:
#
#     ./sweep_kmax.sh --sim-only
#
# which drives systolic_array_8x8_fold directly through
# tb_array_fold_kmax.sv and needs no top-level parameter, because the
# array has no notion of K -- K is only how long the feeder asserts valid.
# =====================================================================

if {$argc < 1} {
    error "usage: -tclargs <K_MAX>   (16, 32, 64, 128)"
}

set KMAX [lindex $argv 0]

if {$KMAX < 8 || ($KMAX % 8) != 0} {
    error "K_MAX must be a positive multiple of 8 (got $KMAX)"
}

# ---------------------------------------------------------------------
# Guard. Remove together with the -generic line below.
# ---------------------------------------------------------------------
# if {$KMAX != 16} {
#     puts ""
#     puts "======================================================="
#     puts " build_kmax.tcl is DISABLED for K_MAX != 16."
#     puts ""
#     puts " systolic_uart_fold_top.sv has no K_MAX parameter yet,"
#     puts " so this build would silently produce the fixed K=16"
#     puts " baseline and label it K_MAX=$KMAX. A mislabelled row is"
#     puts " worse than a missing one."
#     puts ""
#     puts " Requested : K_MAX = $KMAX"
#     puts " RTL can do: K_MAX = 16 (hardwired)"
#     puts ""
#     puts " Do the K_MAX refactor first, then delete this guard."
#     puts " For cycle counts today, use: ./sweep_kmax.sh --sim-only"
#     puts "======================================================="
#     puts ""
#     error "K_MAX=$KMAX not buildable yet (see message above)"
# }

set PART        "xc7a200tsbg484-1"
set TOP         "systolic_uart_fold_top"
set CLK_PERIOD  10.000
set OUT         "build_kmax/k${KMAX}"

file mkdir $OUT
file mkdir $OUT/reports

puts "========================================"
puts " k_max sweep point"
puts "   K_MAX = $KMAX"
puts "   out   = $OUT"
puts "========================================"

read_verilog -sv uart_rx.sv
read_verilog -sv uart_tx.sv
read_verilog -sv fp_mul.sv
read_verilog -sv fp_add.sv
read_verilog -sv fp_reduce16.sv
read_verilog -sv systolic_pe_fold.sv
read_verilog -sv systolic_array_8x8_fold.sv
read_verilog -sv systolic_uart_fold_top.sv

read_ip rtl_fp_pe_test/rtl_fp_pe_test.srcs/sources_1/ip/floating_point_add_0/floating_point_add_0.xci
read_ip rtl_fp_pe_test/rtl_fp_pe_test.srcs/sources_1/ip/floating_point_mul_0/floating_point_mul_0.xci

read_xdc nexys_video_uart.xdc

# ---------------------------------------------------------------------
# Synthesis
#
# DEBUG_MARKERS stays 0 for every sweep point. The breadcrumb bytes
# desynchronise a host reading exactly 512 bytes, and enabling them for
# some points but not others would put a constant-but-unequal offset into
# the LUT column.
#
# TODO(K_MAX): add   -generic K_MAX=$KMAX   here.
# ---------------------------------------------------------------------
synth_design -top $TOP -part $PART \
    -generic DEBUG_MARKERS=0

write_checkpoint -force $OUT/post_synth.dcp
report_utilization    -file $OUT/reports/post_synth_utilization.rpt
report_timing_summary -file $OUT/reports/post_synth_timing.rpt

opt_design
place_design
write_checkpoint -force $OUT/post_place.dcp
report_timing_summary -file $OUT/reports/post_place_timing.rpt

route_design
write_checkpoint -force $OUT/post_route.dcp
report_timing_summary -file $OUT/reports/post_route_timing.rpt
report_utilization    -file $OUT/reports/post_route_utilization.rpt
report_drc            -file $OUT/reports/post_route_drc.rpt

# ---------------------------------------------------------------------
# Machine-readable summary
# ---------------------------------------------------------------------

proc util_row {rpt name} {
    # Utilization rows look like:
    #   | Slice LUTs | 12345 | 0 | 0 | 134600 | 9.17 |
    # Used is the first numeric column. Block RAM Tile can be fractional.
    set pat "\\|\\s*[string map {( \\( ) \\)} $name]\\s*\\|\\s*(\[0-9\]+(?:\\.\[0-9\]+)?)\\s*\\|"
    if {[regexp $pat $rpt -> v]} { return $v }
    return "NA"
}

set urpt [report_utilization -return_string]

set LUT  [util_row $urpt "Slice LUTs"]
set FF   [util_row $urpt "Slice Registers"]
set BRAM [util_row $urpt "Block RAM Tile"]
set DSP  [util_row $urpt "DSPs"]

if {$LUT  eq "NA"} { set LUT  [util_row $urpt "CLB LUTs"] }
if {$FF   eq "NA"} { set FF   [util_row $urpt "CLB Registers"] }
if {$DSP  eq "NA"} { set DSP  [util_row $urpt "DSPs \\(DSP48E1\\)"] }

set WNS  [get_property SLACK [get_timing_paths -delay_type max -max_paths 1]]
set WHS  [get_property SLACK [get_timing_paths -delay_type min -max_paths 1]]
set FMAX [expr {1000.0 / ($CLK_PERIOD - $WNS)}]

set TIMING_OK [expr {$WNS >= 0 && $WHS >= 0}]

set fh [open $OUT/summary.csv w]
puts $fh "k_max,lut,ff,bram,dsp,wns_ns,whs_ns,fmax_mhz,timing_met"
puts $fh [format "%d,%s,%s,%s,%s,%.3f,%.3f,%.2f,%d" \
             $KMAX $LUT $FF $BRAM $DSP $WNS $WHS $FMAX $TIMING_OK]
close $fh

puts "========================================"
puts " K_MAX=$KMAX  LUT=$LUT  FF=$FF  BRAM=$BRAM  DSP=$DSP"
puts " WNS=$WNS ns   WHS=$WHS ns   Fmax=[format %.2f $FMAX] MHz"
if {$TIMING_OK} {
    puts " TIMING MET"
} else {
    puts " WARNING: TIMING NOT MET -- Fmax above is the achievable"
    puts " frequency, but this bitstream is not safe to run at 100 MHz."
}
puts " summary: $OUT/summary.csv"
puts "========================================"

# A bitstream is only meaningful if timing closed. Writing one regardless
# would invite programming a board with a design that fails setup.
if {$TIMING_OK} {
    write_bitstream -force $OUT/${TOP}_k${KMAX}.bit
    puts " bitstream: $OUT/${TOP}_k${KMAX}.bit"
} else {
    puts " bitstream SKIPPED (timing not met)"
}
