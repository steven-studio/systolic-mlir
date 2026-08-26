# build_kmax.tcl -- one synthesis+implementation run at a given K_MAX.
#
#   vivado -mode batch -source uart/build_kmax.tcl -tclargs <K_MAX>
#
# Paths inside this script are resolved relative to the script's own
# location (fold_pipelined/uart/), so it can be invoked from any cwd.
#
# K_MAX is the SYNTHESIS-TIME HARDWARE CAPACITY: the deepest reduction the
# on-chip operand buffers can hold. It is not the workload's K, and it is
# not a fold count.
#
#   K_MAX   synthesis-time hardware capacity     <- this script's argument
#   k_dim   runtime workload reduction length    <- supplied by software
#   folds   k_dim / 8, derived at runtime        <- not a hardware quantity
#
# An earlier revision of this script drove a top-level `NFOLD` generic.
# That was the wrong abstraction: it baked the fold COUNT into the
# hardware, when the fold count is a scheduling quantity software derives
# from k_dim. Do not reintroduce it under a different name -- K_MAX is a
# capacity, and the operand buffers are indexed by absolute k, so no fold
# count appears in the RTL at all.
#
# RX framing scales with K_MAX (K_MAX * 64 bytes); TX does not (always
# 512 bytes, two accumulator contexts). So the host must send more for a
# deeper build, but always reads back the same two 8x8 matrices and adds
# them itself.
#
# NOTE: test_uart_fold8x8.py sends exactly 1024 bytes and is therefore a
# K_MAX=16 test. It needs updating before it can drive a deeper build.

if {$argc < 1} {
    error "usage: -tclargs <K_MAX> \[DEBUG_MARKERS\] \[PLACE_DIRECTIVE\] \[N\]"
}

set KMAX [lindex $argv 0]

# 第二個參數 = DEBUG_MARKERS(預設 0)。開啟時輸出到 k<K>_dbg,
# 不覆蓋乾淨版;燒錄用 program_kmax.tcl -tclargs <K>_dbg。
# 板上判讀:host 收到的前幾個 byte 是 A1..A5 breadcrumb,
#   什麼都沒有       -> RX framing 從未接受 frame
#   只有 A1          -> 卡在 ST_FEED
#   A1 A2            -> 卡在 ST_WAIT_RESULT(PE 沒全部完成)
#   A1 A2 A3 A4 (A5) -> 結果有了,卡在 TX/ST_SEND
set DBG [expr {$argc >= 2 ? [lindex $argv 1] : 0}]

# 第三個參數 = place_design directive(預設 Default;可用 Explore、
# ExtraTimingOpt、ExtraNetDelay_high 等)。
#
# 用途:同一個組態、換一個 placement,再擲一次硬幣。Vivado 是決定性
# 的 -- 同腳本重跑產出同一顆 bit,所以「同一顆 bit 每次燒都死」只
# 證明那顆 placement 壞,不證明組態壞。要分辨「參數導致失敗」與
# 「這次 placement 剛好踩雷」,唯一的方法是同組態多做幾個獨立
# placement。輸出目錄與 bit 名會帶 directive 後綴,不互相覆蓋。
set PDIR [expr {$argc >= 3 ? [lindex $argv 2] : "Default"}]

# 第四個參數 = 陣列邊長 N(預設 8)。N=4 輸出到 k<K>_n4,與 8x8 的
# 產物互不覆蓋。N 必須是 2 的冪(RTL elaboration 會再驗一次)。
set NARR [expr {$argc >= 4 ? [lindex $argv 3] : 8}]

# The RTL requires K_MAX >= 16 and a multiple of 8. It has an elaboration
# assertion for both, but failing here is cheaper than failing in synth.
if {$KMAX < 16 || ($KMAX % 8) != 0} {
    error "K_MAX must be a multiple of 8 and at least 16 (got $KMAX)"
}

# ---------------------------------------------------------------------
# Layout (fold_pipelined/):
#   core/    PE, array, feeder, operand buffer, status, tx source, fp wrappers
#   ip/fp32/<ip_name>/<ip_name>.xci   Xilinx floating-point IP, one dir each
#   uart/    UART transport: rx/tx, top, xdc, this script, program_kmax.tcl
#   build_kmax/  outputs (ignored by git)
# ---------------------------------------------------------------------
set HERE [file normalize [file dirname [info script]]]   ;# .../fold_pipelined/uart
set ROOT [file normalize [file join $HERE ..]]           ;# .../fold_pipelined

set PART        "xc7a200tsbg484-1"
set TOP         "systolic_uart_top"
set CLK_PERIOD  10.000

# in-memory project 的預設 part 是 xc7vx485t。不在 read_ip 之前把它設成
# 板子的 part,IP 會因「project part 與 IP 客製 part 不符」被 lock,之後
# generate_target 什麼都生不出來。舊註解裡「FP IP 鎖在 xc7vx485t」就是
# 這件事的誤診:鎖的是專案預設值,不是 IP。
create_project -in_memory -part $PART

set BITTAG $KMAX
if {$DBG} { append BITTAG "_dbg" }
if {$PDIR ne "Default"} { append BITTAG "_[string tolower $PDIR]" }
if {$NARR != 8} { append BITTAG "_n$NARR" }
set OUT "$ROOT/build_kmax/k${BITTAG}"

file mkdir $OUT
file mkdir $OUT/reports

# 陳舊 bitstream 是個陷阱:本腳本在時序未收斂時「跳過」write_bitstream,
# 而 program_kmax.tcl 只檢查路徑上有沒有檔案 -- 上一輪留下的舊 .bit 會被
# 原封不動燒進板子,症狀跟新設計壞掉一模一樣。開跑先刪,跑完後路徑上
# 若還有 .bit,就只可能是這一輪產生的。
file delete -force $OUT/${TOP}_k${BITTAG}.bit

puts "========================================"
puts " k_max sweep point"
puts "   K_MAX = $KMAX"
puts "   N     = $NARR"
puts "   root  = $ROOT"
puts "   out   = $OUT"
puts "========================================"


# ---------------------------------------------------------------------
# IP FIRST. synth_ip runs an out-of-context synth_design in this same
# session and leaves the current fileset pointing at the IP; anything
# read_xdc'd after it lands on the IP's constraint set instead of the
# design's, and the main synth then has no clocks. So: generate and
# synthesise the IP before a single RTL or XDC file is read.
# ---------------------------------------------------------------------
set IP_DIR [file join $ROOT ip fp32]
foreach xci {floating_point_add_0 floating_point_mul_0} {
    set p [file join $IP_DIR $xci ${xci}.xci]
    if {![file exists $p]} { set p [file join $IP_DIR ${xci}.xci] }
    if {![file exists $p]} { error "missing IP: $xci under $IP_DIR" }
    read_ip $p
}

set ips [get_ips]
foreach ip $ips {
    if {[get_property IS_LOCKED $ip]} {
        report_ip_status
        error "IP $ip is locked -- see report_ip_status above (part mismatch? shared output dir?)"
    }
}
# .xci 搬離原專案後生成產物不會跟著來,在這裡重生成;已是最新時
# Vivado 會跳過,所以每次跑都無害。產物落在 .xci 旁邊的目錄裡。
generate_target all $ips
synth_ip $ips        ;# OOC 合成,跟舊流程一致


# ---------------------------------------------------------------------
# RTL sources. Order is irrelevant to synth_design but is kept
# bottom-up for readability: transport primitives, fp wrappers, PE,
# array, buffers/feeder, side blocks, top.
# ---------------------------------------------------------------------
set RTL_FILES {
    uart/uart_rx.sv
    uart/uart_tx.sv
    core/fp_mul.sv
    core/fp_add.sv
    core/fp_reduce16.sv
    core/systolic_pe_bram.sv
    core/systolic_array.sv
    core/tile_feeder.sv
    core/operand_buffer.sv
    core/status.sv
    core/tx_source.sv
    uart/uart_tx_streamer.sv
    uart/systolic_uart_top.sv
}
foreach f $RTL_FILES {
    set p [file join $ROOT $f]
    if {![file exists $p]} { error "missing RTL source: $p" }
    read_verilog -sv $p
}

# Constraints, read last so they attach to the design fileset.
set XDC [file join $HERE nexys_video_uart.xdc]
if {![file exists $XDC]} { error "missing XDC: $XDC" }
read_xdc $XDC

# ---------------------------------------------------------------------
# Synthesis
#
# DEBUG_MARKERS stays 0 for every sweep point. The breadcrumb bytes
# desynchronise a host reading exactly 4*N*N bytes, and enabling them for
# some points but not others would put a constant-but-unequal offset into
# the LUT column.
#
# ---------------------------------------------------------------------
synth_design -top $TOP -part $PART \
    -generic K_MAX=$KMAX \
    -generic N=$NARR \
    -generic DEBUG_MARKERS=$DBG \
    -generic CYCLE_COUNTER=1

# Fail loudly if the XDC did not take: every later step assumes a clock.
if {[llength [get_clocks -quiet]] == 0} {
    error "no clocks after synth_design: create_clock in $XDC did not apply"
}

# ---------------------------------------------------------------------
# Hold 餘裕強化(2026-08-19 板上實驗結論)
#
# 同一份 clean 組態:Default placement 上板死(RX 0/512)、Explore
# placement 上板全對(bit-exact)、dbg placement 也活 -- 失敗只跟
# placement 相關,跟 RTL/組態無關。死掉那顆的 WHS 只有 0.02ns 級,
# 而 STA 的前提本身有瑕疵(OOC 時脈 100ns)。
# 這裡強制 implementation 多留 0.15ns 的 hold 餘裕:報表上的 WHS
# 已內含這份悲觀,summary 的 timing_met 閘門因此等於要求真實餘裕
# >= 0.15ns。從此不靠 placement 抽籤。
# (必須放在 synth_design 之後 -- 之前 design 未開、get_clocks 是空的。)
# ---------------------------------------------------------------------
set_clock_uncertainty -hold 0.150 [get_clocks]

write_checkpoint -force $OUT/post_synth.dcp
report_utilization    -file $OUT/reports/post_synth_utilization.rpt
report_timing_summary -file $OUT/reports/post_synth_timing.rpt

opt_design
place_design -directive $PDIR
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
    write_bitstream -force $OUT/${TOP}_k${BITTAG}.bit
    puts " bitstream: $OUT/${TOP}_k${BITTAG}.bit"
    puts " program:   vivado -mode batch -source $HERE/program_kmax.tcl -tclargs $BITTAG"
} else {
    puts " bitstream SKIPPED (timing not met)"
}