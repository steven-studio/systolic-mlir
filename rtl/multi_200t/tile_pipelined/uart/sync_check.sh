#!/usr/bin/env bash
# sync_check.sh -- 檢查 uart/ 工作區的工具檔與 RTL 是否為最新交付版。
#
#   ./sync_check.sh          # 在 Ubuntu 的 rtl/uart/ 目錄下跑
#
# 原理:不比對 Mac(構不到),改比對「新版檔案必有的指紋字串」--
# 每次交付的檔案都有只在新版存在的特徵行。缺任何一個指紋就報出來。
# 這已經是第五次因為 Ubuntu 上留著舊版檔案而燒掉 debug 時間:
#   1. build_kmax.tcl 沒有 PDIR(Explore 參數被靜默吃掉)
#   2. fix_ip.tcl 根本不存在(git add pathspec 失敗)
#   3. program_kmax.tcl 不認非數字 tag(手動無害,sweep 全滅)
#   4. tb_rx_contract.sv 不存在(git add 全體失敗)
#   5. program_kmax.tcl 又一次(sweep N=4 PROGRAM FAIL)
# build 或 sweep 之前跑這個,十秒換一晚。

set -u
fails=0

need() {  # need <file> <fingerprint> <說明>
    local f="$1" pat="$2" why="$3"
    if [ ! -f "$f" ]; then
        echo "MISSING : $f  ($why)"
        fails=$((fails + 1))
    elif ! grep -q "$pat" "$f"; then
        echo "STALE   : $f  (缺指紋: $why)"
        fails=$((fails + 1))
    fi
}

need build_kmax.tcl        "set PDIR"                 "PLACE_DIRECTIVE 第三參數"
need build_kmax.tcl        "set NARR"                 "N 第四參數"
need build_kmax.tcl        "set_clock_uncertainty"    "hold 餘裕強化"
need build_kmax.tcl        "systolic_tx_source"       "TX 模組進 read_verilog 清單"
need program_kmax.tcl      "string is integer"        "非數字 tag(16_n4 等)防護"
need sweep_kmax.sh         'NARR=8'                   "-n 參數支援"
need test_uart_kmax.py     'add_argument("--n"'       "N 參數"
need test_uart_kmax.py     "2 \* (n - 1) + 105"       "幾何化 cycle 模型"
need systolic_uart_tile_top.sv    "parameter int N = 8"     "N generic"
need systolic_operand_buffer.sv   "N_BANKS"                 "bank 數參數"
need systolic_tile_feeder.sv      "parameter int N"         "N 參數"
need systolic_tx_source.sv        "parameter int N"         "N 參數"
need uart_tx_streamer.sv          "module uart_tx_streamer" "TX streamer 存在"
need systolic_status.sv           "module systolic_status"  "板上狀態顯示模組"
need nexys_video_uart.xdc         "jb_led"                  "LED 腳位約束"
need build_kmax.tcl               "systolic_status"         "status 進 read_verilog 清單"
need tb_nparam_equiv.sv           "tx_equiv_pair\|golden"   "N 等價 tb 存在"
need tb_rx_contract.sv            "rx_contract_one"         "RX 契約 tb 存在"
need golden_operand_buffer.sv     "golden_operand_buffer"   "golden 副本存在"
need golden_tile_feeder.sv        "golden_tile_feeder"      "golden 副本存在"
need tb_uart_multi_invocation.sv  "parameter int N"         "full-system tb 的 N 參數"
need fix_ip.tcl                   "upgrade_ip"              "FP IP retarget 腳本存在"

echo ""
if [ $fails -eq 0 ]; then
    echo "SYNC OK: 全部檔案都是最新交付版"
else
    echo "SYNC FAIL: $fails 個檔案缺失或過期 -- 從 Mac 重新 scp 上列檔案"
    exit 1
fi
