# compare_netlists.tcl -- 檢驗「DEBUG_MARKERS=0 時合成把必要邏輯剪掉」
#
#   vivado -mode batch -source compare_netlists.tcl
#
# 不重建任何東西:直接打開 clean(k16)與 dbg(k16_dbg)兩顆
# post_route.dcp,逐項清點主控制鏈的 cell 存活狀況。
#
# 判讀:
#   clean 有任何一項是 0 而 dbg 不是 0(debug_* 除外,clean 剪掉
#   debug_* 是設計上的預期)-> 剪除假說成立,而且直接看到剪掉了誰。
#   兩邊主控制鏈都齊全 -> 剪除假說出局,netlist 功能等價,
#   分歧必在實體層(placement / hold),回到 Explore 實驗。
#
# 注意:名稱可能被合併/改名,單一項 0 不急著下結論,看整條鏈。
# uart_tx 的 tx/busy、TX FSM、c0/c1_done、matrices_ready 這幾項
# 是絕對不允許消失的 -- 消失一個,板上就是 RX 0/512。

set PAIRS {
    clean build_kmax/k16/post_route.dcp
    dbg   build_kmax/k16_dbg/post_route.dcp
}

# 主控制鏈,按資料流順序:RX -> framing -> 主 FSM -> 陣列完成 -> TX
set CHECKS {
    {u_uart_rx 整個 RX 模組}          {NAME =~ *u_uart_rx/*}
    {rx_state  RX framing FSM}        {NAME =~ *rx_state_reg*}
    {sync_sr   frame marker 滑窗}     {NAME =~ *sync_sr_reg*}
    {matr_rdy  matrices_ready}        {NAME =~ *matrices_ready*}
    {k_dim     k_dim 暫存器}          {NAME =~ *k_dim_reg*}
    {state     主 FSM}                {NAME =~ *state_reg*}
    {feed_t    feed 計數器}           {NAME =~ *feed_t_reg*}
    {a_buf     A 側運算元緩衝}        {NAME =~ *u_a_buf/*}
    {b_buf     B 側運算元緩衝}        {NAME =~ *u_b_buf/*}
    {c0_done   ctx0 完成旗標}         {NAME =~ *c0_done*}
    {c1_done   ctx1 完成旗標}         {NAME =~ *c1_done*}
    {C0_store  C0 結果暫存}           {NAME =~ *C0_reg*}
    {C1_store  C1 結果暫存}           {NAME =~ *C1_reg*}
    {tx_state  TX FSM}                {NAME =~ *tx_state_reg*}
    {tx_count  TX byte 計數}          {NAME =~ *tx_count_reg*}
    {tx_start  tx_start 暫存器}       {NAME =~ *tx_start_reg*}
    {tx_snt    tx_send_started}       {NAME =~ *tx_send_started*}
    {u_uart_tx 整個 TX 模組}          {NAME =~ *u_uart_tx/*}
    {cyc_lat   cycle counter 鎖存}    {NAME =~ *cyc_latched_reg*}
    {debug_p   debug_pending(僅 dbg 應有)} {NAME =~ *debug_pending*}
    {por       POR 移位暫存器}        {NAME =~ *por_sr*}
}

foreach {tag dcp} $PAIRS {
    if {![file exists $dcp]} { error "missing: $dcp" }

    puts ""
    puts "======================================================"
    puts " $tag  --  $dcp"
    puts "======================================================"
    open_checkpoint $dcp

    foreach {label filt} $CHECKS {
        set n [llength [get_cells -quiet -hierarchical -filter $filt]]
        puts [format "  %-34s %6d cells" $label $n]
    }

    # 頂層四支腳有沒有真的接到邏輯(而不是被優化成常數)
    foreach p {uart_tx uart_rx clk rst} {
        set net [get_nets -quiet -of [get_ports -quiet $p]]
        puts [format "  port %-8s nets=%d" $p [llength $net]]
    }

    close_design
}

puts ""
puts "把兩段輸出並排看:clean 缺了 dbg 有的任何主鏈項目,剪除假說成立。"
