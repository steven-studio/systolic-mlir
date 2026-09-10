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
    close_project
}
