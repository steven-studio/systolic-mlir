# table3.tcl -- the DMA rows of the FPGA paper's resource table, from the two
# routed runs that dma_top_build.tcl left under ~/work/vivado.
#
#   vivado -mode batch -source table3.tcl                 v1 and v2 at K_MAX=256
#   vivado -mode batch -source table3.tcl -tclargs v2 256 one variant
#
# For each project it writes
#   <proj>/table3_<v>_k<K>.txt   report_utilization -hierarchical, depth 2
#   <proj>/timing_<v>_k<K>.txt   report_timing_summary
# and prints, to the console, the utilization table, the three worst paths in
# the whole design, the worst path in every 10 ns clock group, and for each
# top-level instance the worst slack of any path that starts or ends inside
# it.  The last two are what decide whether the 0.913 ns WNS the bitstream
# reports is the MIG's own fixed path (it is 0.913 in the bandwidth-probe
# builds too) or something in the DMA logic.
#
# It then prints the three rows of the paper's resource table verbatim, ready
# to paste over the \tbd cells:
#
#   MIG + read engine + writer v1     u_mig_7series_0 + u_eng + u_wr   (v1 run)
#   Result reader + write-back        u_rdr + u_wb_fifo + u_wb         (v1 run)
#   DMA top, N=8, kmax=256, v2        everything                       (v2 run)
#
# Primitives are counted by REF_NAME, the same convention ooc_v2.tcl uses for
# the table's Delta row -- PRIMITIVE_TYPE strings are not stable across Vivado
# versions and 2026.1 silently matched none of them.  BRAM is in RAMB36 tiles
# (RAMB36 + RAMB18/2).  f_max is 1000/(10 - slack) from the worst path that
# starts or ends inside the row's cells, so it is what that block alone could
# clock, not the build's: the build's own number is printed beside it, with
# the block that owns the worst path named, because if that block is the MIG
# then no amount of work on the operand path moves it.

set variants {v1 v2}
set kmax     256
if {[info exists argv] && [llength $argv] >= 1} { set variants [lindex $argv 0] }
if {[info exists argv] && [llength $argv] >= 2} { set kmax     [lindex $argv 1] }

# Worst slack of paths that start inside / end inside the cells matching $pat.
proc blk_slack {pat} {
    set cells [get_cells -hierarchical -filter \
        "NAME =~ $pat && (IS_SEQUENTIAL || REF_NAME =~ RAMB* || REF_NAME =~ DSP*)"]
    if {[llength $cells] == 0} { return [format "  %-22s (no cells)" $pat] }
    set pf [get_timing_paths -from $cells -max_paths 1 -nworst 1]
    set pt [get_timing_paths -to   $cells -max_paths 1 -nworst 1]
    set sf [expr {[llength $pf] ? [format "%.3f" [get_property SLACK $pf]] : "n/a"}]
    set st [expr {[llength $pt] ? [format "%.3f" [get_property SLACK $pt]] : "n/a"}]
    return [format "  %-22s %6d seq cells   worst slack: from %7s   to %7s" \
            $pat [llength $cells] $sf $st]
}

foreach v $variants {
    set proj $::env(HOME)/work/vivado/systolic_dma_${v}_k${kmax}
    if {![file exists $proj/systolic_dma.xpr]} { error "no project: $proj" }
    open_project $proj/systolic_dma.xpr
    open_run impl_1

    set util $proj/table3_${v}_k${kmax}.txt
    report_utilization -hierarchical -hierarchical_depth 2 -file $util
    puts "\n=== $v K_MAX=$kmax : utilization (post-route, hierarchical) -> $util"
    set fh [open $util r]
    puts [read $fh]
    close $fh

    report_timing_summary -max_paths 1 -no_header -file $proj/timing_${v}_k${kmax}.txt
    puts "=== $v K_MAX=$kmax : three worst paths in the design"
    foreach p [get_timing_paths -max_paths 3 -nworst 1 -sort_by slack] {
        puts [format "  slack %7.3f  req %6.3f  %s" \
              [get_property SLACK $p] [get_property REQUIREMENT $p] [get_property GROUP $p]]
        puts "      from  [get_property STARTPOINT_PIN $p]"
        puts "      to    [get_property ENDPOINT_PIN $p]"
    }

    puts "=== $v K_MAX=$kmax : worst path per 10 ns clock"
    foreach c [get_clocks -filter {PERIOD > 9.9 && PERIOD < 10.1}] {
        set p [get_timing_paths -group $c -max_paths 1 -nworst 1]
        if {[llength $p] == 0} { continue }
        puts [format "  %-28s slack %7.3f" $c [get_property SLACK $p]]
        puts "      from  [get_property STARTPOINT_PIN $p]"
        puts "      to    [get_property ENDPOINT_PIN $p]"
    }

    puts "=== $v K_MAX=$kmax : worst slack touching each block"
    foreach pat {*u_mig_7series_0/* *u_eng/* *u_wr/* *u_chk_wr/* *u_a_buf/* *u_b_buf/*
                 *u_seed/* *u_rdr/* *u_wb_fifo/* *u_wb/* *u_feeder/* *u_array/* *u_vio/*} {
        puts [blk_slack $pat]
    }
    paper_rows $v $kmax
    close_project
}

# -----------------------------------------------------------------------------
# The paper's rows.
proc name_or {pats} {
    set t {}
    foreach p $pats { lappend t "NAME =~ $p" }
    return "([join $t { || }])"
}

proc count_prims {pats ref} {
    return [llength [get_cells -hierarchical -quiet \
        -filter "[name_or $pats] && REF_NAME =~ $ref"]]
}

# Worst slack (ns) of any path that starts or ends inside $pats, or "" if the
# patterns match no sequential cell.
proc worst_slack {pats} {
    set cells [get_cells -hierarchical -quiet -filter \
        "[name_or $pats] && (IS_SEQUENTIAL || REF_NAME =~ RAMB* || REF_NAME =~ DSP*)"]
    if {[llength $cells] == 0} { return "" }
    set worst ""
    foreach dir {-from -to} {
        set pth [get_timing_paths -quiet {*}[list $dir $cells] -max_paths 1 -nworst 1]
        if {[llength $pth] == 0} { continue }
        set sl [get_property SLACK $pth]
        if {$worst eq "" || $sl < $worst} { set worst $sl }
    }
    return $worst
}

proc paper_rows {v kmax} {
    global ROW
    set defs [list \
        [list mig  "MIG + read engine + writer v1" {*u_mig_7series_0/* *u_eng/* *u_wr/*}] \
        [list wb   "Result reader + write-back"    {*u_rdr/* *u_wb_fifo/* *u_wb/*}] \
        [list top  "DMA top"                       {*}] ]
    foreach d $defs {
        lassign $d key label pats
        set lut [count_prims $pats "LUT*"]
        set ff  [count_prims $pats "FD*"]
        set b36 [count_prims $pats "RAMB36*"]
        set b18 [count_prims $pats "RAMB18*"]
        set dsp [count_prims $pats "DSP*"]
        set sl  [worst_slack $pats]
        set fmx [expr {$sl eq "" ? "" : 1000.0 / (10.0 - $sl)}]
        set ROW($v,$key) [list $label $lut $ff [expr {$b36 + $b18 / 2.0}] $dsp $sl $fmx]
    }
    # who owns the build's worst path -- the question the f_max column hangs on
    set p [get_timing_paths -quiet -max_paths 1 -nworst 1 -sort_by slack]
    if {[llength $p]} {
        set ROW($v,wns)  [get_property SLACK $p]
        set ROW($v,path) [get_property ENDPOINT_PIN $p]
    } else {
        set ROW($v,wns) "" ; set ROW($v,path) "(none)"
    }
}

# -----------------------------------------------------------------------------
# The array rows print 51.2k / 13.1k, so anything at that scale gets the same
# treatment and anything smaller stays an integer -- 0.3k for 312 LUTs would
# be a worse number than 312.
proc kfmt {n} {
    if {$n >= 1000} { return [format "%.1fk" [expr {$n/1000.0}]] }
    return [format "%d" $n]
}
proc bfmt {b} { return [expr {$b == int($b) ? int($b) : $b}] }

proc fmt_row {r} {
    lassign $r label lut ff bram dsp sl fmx
    set fmx_s [expr {$fmx eq "" ? "--" : [format "%.0f" $fmx]}]
    return [format "%-34s & %6s & %6s & %5s & %3d & %4s \\\\" \
            $label [kfmt $lut] [kfmt $ff] [bfmt $bram] $dsp $fmx_s]
}

puts "\n=============================================================="
puts "  tab:resources -- paste these over the \\tbd cells"
puts "  LUT/FF/RAMB counted by REF_NAME (ooc_v2.tcl's convention);"
puts "  BRAM in RAMB36 tiles; f_max = 1000/(10 - block worst slack)."
puts "=============================================================="
foreach {v key} {v1 mig v1 wb v2 top} {
    if {![info exists ROW($v,$key)]} {
        puts "  (missing: $v $key -- that variant was not in this run)"
        continue
    }
    set r $ROW($v,$key)
    if {$key eq "top"} { lset r 0 "DMA top, \$N{=}8\$, \$\\kmax{=}256\$, v2" }
    puts "  [fmt_row $r]"
    puts [format "      raw: LUT %d  FF %d  BRAM %s tiles  DSP %d  block slack %s ns" \
          [lindex $r 1] [lindex $r 2] [bfmt [lindex $r 3]] [lindex $r 4] [lindex $r 5]]
}
puts ""
foreach v $variants {
    if {[info exists ROW($v,wns)]} {
        puts [format "  %s build WNS %s ns  ->  %.0f MHz   worst path ends at:" \
              $v $ROW($v,wns) [expr {1000.0/(10.0 - $ROW($v,wns))}]]
        puts "      $ROW($v,path)"
        if {[string match "*mig_7series*" $ROW($v,path)]} {
            puts "      ^ inside the MIG: the build's f_max is the controller's, not the operand path's."
        } else {
            puts "      ^ NOT in the MIG -- say so in the caption; the operand path owns the critical path."
        }
    }
}
puts ""
