# fix_ip.tcl -- 把浮點 IP 重新鎖定到正確的 part。
#
#   vivado -mode batch -source fix_ip.tcl
#
# 現況(每次 build 都在印,但淹沒在 log 裡):
#
#   WARNING: [IP_Flow 19-2162] IP 'floating_point_add_0' is locked:
#   * Current project part 'xc7vx485tffg1157-1' and the part
#     'xc7a200tsbg484-1' used to customize the IP do not match.
#
# 這兩顆 xci 是為 xc7vx485t(Virtex-7)客製的,而板子是 xc7a200t
# (Artix-7)。IP 被「鎖住」代表 Vivado 不重新產生,直接拿舊 part 的
# 預產 netlist 塞進來用;同時 OOC 合成時脈是 100 ns,實際跑 10 ns:
#
#   WARNING: [Timing 38-316] Clock period '100.000' specified during
#   out-of-context synthesis ... is different from the actual clock
#   period '10.000'
#
# 這兩件事單獨都不必然致命(in-context STA 仍然會計時),但它們讓
# 每一次 placement 的餘裕都建立在錯誤前提上 -- 而這個設計的 WHS 只有
# 0.019 ns。重產到正確的 part,把前提修好。
#
# upgrade 之後 rtl_fp_pe_test 專案樹裡的 .gen 產物會更新;之後照常跑
# build_kmax.tcl 即可,不需要改它。

set PART "xc7a200tsbg484-1"
set IPS [list \
    rtl_fp_pe_test/rtl_fp_pe_test.srcs/sources_1/ip/floating_point_add_0/floating_point_add_0.xci \
    rtl_fp_pe_test/rtl_fp_pe_test.srcs/sources_1/ip/floating_point_mul_0/floating_point_mul_0.xci \
]

foreach f $IPS {
    if {![file exists $f]} { error "missing: $f" }
}

create_project -force fixip build_kmax/fixip -part $PART

foreach f $IPS { read_ip $f }

# 解鎖 + 重新客製到目前 part。同版本 IP 的 retarget 不會動任何組態
# 參數(latency、DSP 用量都保持原樣),只換目標矽片。
upgrade_ip [get_ips]

foreach ip [get_ips] {
    puts [format " %-24s part=%s  locked=%s" \
              [get_property NAME $ip] \
              [get_property PART $ip] \
              [get_property IS_LOCKED $ip]]
    if {[get_property IS_LOCKED $ip]} {
        error "[get_property NAME $ip] 仍然 locked -- 看上方 upgrade_ip 訊息"
    }
}

# 重產輸出物:合成/模擬目標 + OOC 合成的 DCP(build_kmax.tcl 的
# read_ip 流程吃的就是這顆)。
generate_target all [get_ips]
foreach ip [get_ips] { synth_ip $ip }

puts "========================================"
puts " IP 已 retarget 到 $PART 並重產 OOC netlist"
puts " 之後照常:vivado -mode batch -source build_kmax.tcl -tclargs 16"
puts " build log 裡不應再出現 IP_Flow 19-2162 locked 警告"
puts "========================================"

close_project
