# program_kmax.tcl -- program a K_MAX sweep bitstream onto the Nexys Video.
#
#   vivado -mode batch -source program_kmax.tcl -tclargs <K_MAX> [<jtag_glob>]
#
# e.g.  vivado -mode batch -source program_kmax.tcl -tclargs 64
#
# Kept separate from program_8x8_fold.tcl, which hardwires the old
# build_8x8_fold/ baseline bitstream. That script is left untouched so the
# original K=16 build stays reproducible.
#
# AFTER PROGRAMMING: press BTNC (reset) before the first transaction.
# k_dim is loaded with K_MAX on reset only, so straight out of configuration
# it is 0 and the feed FSM injects nothing -- the design will appear to hang
# waiting for results that never come.

if {$argc < 1} {
    error "usage: -tclargs <K_MAX> \[<jtag_target_glob>\]"
}

set KMAX   [lindex $argv 0]
set TGLOB  [expr {$argc >= 2 ? [lindex $argv 1] : "*210276C08FC0B*"}]

# ⚠ TOP 改名之後,舊的 bitstream(systolic_uart_tile_top_k*.bit)
#   這條路徑找不到。要燒 cgo-base 那一版就得用舊名字,或直接
#   git checkout 那個 tag 的腳本。
set BIT "build_kmax/k${KMAX}/systolic_uart_top_k${KMAX}.bit"

if {![file exists $BIT]} {
    puts ""
    puts "======================================================="
    puts " No bitstream at:"
    puts "   $BIT"
    puts ""
    puts " build_kmax.tcl only writes a bitstream when timing"
    puts " closed. Either the run has not happened yet, or that"
    puts " K_MAX did not meet timing / failed placement."
    puts ""
    puts " Available:"
    foreach f [glob -nocomplain build_kmax/k*/*.bit] { puts "   $f" }
    puts "======================================================="
    error "missing bitstream: $BIT"
}

# 燒錄前把 bitstream 的身分講清楚:多舊、當時時序有沒有收斂。
# 「燒了一顆舊的/壞的 .bit」與「新設計壞掉」在板上無法區分,
# 只能在這裡擋。
set SUM "build_kmax/k${KMAX}/summary.csv"
if {[file exists $SUM]} {
    set fh [open $SUM r]; set lines [split [read $fh] "\n"]; close $fh
    set row [lindex $lines 1]
    if {[llength [split $row ","]] >= 9} {
        set tmet [lindex [split $row ","] 8]
        set wns  [lindex [split $row ","] 5]
        if {$tmet != 1} {
            error "summary.csv 記錄此 K_MAX 時序未收斂 (WNS=$wns)。路徑上的 .bit 是更早一輪的殘留,拒絕燒錄。重跑 build_kmax.tcl。"
        }
        puts " summary : WNS=$wns ns, timing_met=$tmet"
    }
}

set age_s [expr {[clock seconds] - [file mtime $BIT]}]
puts "========================================"
puts " Programming K_MAX = $KMAX"
puts "   $BIT"
puts "   built [clock format [file mtime $BIT] -format {%Y-%m-%d %H:%M:%S}]  ([expr {$age_s / 60}] 分鐘前)"
if {$age_s > 6*3600} {
    puts ""
    puts " *** 警告:這顆 bitstream 超過 6 小時。確定是這次建出來的?"
    puts " *** build_kmax.tcl 在時序未收斂時不會覆寫 .bit。"
}
puts "========================================"

open_hw_manager
connect_hw_server

set target [lindex [get_hw_targets $TGLOB] 0]

if {$target eq ""} {
    puts "ERROR: JTAG target matching '$TGLOB' not found."
    puts "Available targets:"
    puts [get_hw_targets]
    error "no matching JTAG target"
}

puts "OPENING: $target"
open_hw_target $target

set devs [get_hw_devices]
if {[llength $devs] == 0} {
    error "No FPGA device found"
}

set dev [lindex $devs 0]
current_hw_device $dev
refresh_hw_device $dev

puts "PROGRAMMING: $dev"
set_property PROGRAM.FILE $BIT $dev
program_hw_devices $dev
refresh_hw_device $dev

puts "========================================"
puts " PROGRAMMED K_MAX = $KMAX"
if {[string is integer -strict $KMAX]} {
    puts ""
    puts " Wire format for this build:"
    puts "   RX  [expr {$KMAX * 64}] bytes   (A,B interleaved, [expr {$KMAX / 8}] windows of 8)"
    puts "   TX  512 bytes    (C_ctx0 then C_ctx1; host adds them)"
    puts ""
    puts " Then:  python3 test_uart_kmax.py --kmax $KMAX"
} else {
    # 非數字的 KMAX(echo、16_dbg 等)是診斷 bitstream。
    puts ""
    puts " Diagnostic bitstream. echo: picocom -b 115200 <port>,"
    puts " 'U' 每 ~1.3 s 一次,打 a 回 A。"
}
puts "========================================"

close_hw_manager