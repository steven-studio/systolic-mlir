#!/usr/bin/env bash
#
# run_sim.sh -- 不需要浮點 IP 的快速模擬,幾秒鐘一輪。
#
#   ./run_sim.sh              # 自動挑引擎(有 xvlog 用 xsim,否則 Verilator)
#   ./run_sim.sh verilator
#   ./run_sim.sh xsim
#
# 跑兩個 bench,對四個 K_MAX 設計點:
#
#   tb_operand_buffer_equiv  重構前後逐拍等價(golden = 舊的 inline generate)
#   tb_feeder_buffer         feeder + buffer 的契約
#
# 兩個都不碰 UART、不碰 floating_point_*_0,所以不需要 Vivado 專案,
# 也不需要生成 IP。全系統的 tb_uart_multi_invocation 需要 IP,走
# sim_kmax.tcl,不在這個腳本裡。
#
# 所有工具輸出都保留在 sim_out/ 底下,失敗時直接把 log 印出來。
# 之前這裡把 xvlog/xelab 的 stderr 導到 /dev/null,結果只看得到
# 「BUILD FAIL」而看不到原因 —— 吞掉錯誤訊息的腳本比沒有腳本更糟。
#
# 任何一點 FAIL 就以非零值結束,可以直接掛在 CI 或 pre-commit 上。

set -u

ENGINE="${1:-auto}"
KPOINTS=(16 32 64 256)

SRC_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUT="$SRC_DIR/sim_out"
RTL=("$SRC_DIR/systolic_operand_buffer.sv" "$SRC_DIR/systolic_tile_feeder.sv")
FAIL=0

if [ "$ENGINE" = "auto" ]; then
    if command -v xvlog >/dev/null 2>&1; then ENGINE=xsim; else ENGINE=verilator; fi
fi

NEED="$( [ "$ENGINE" = xsim ] && echo xvlog || echo verilator )"
if ! command -v "$NEED" >/dev/null 2>&1; then
    echo "找不到 $NEED。"
    echo "  Verilator : brew install verilator  /  apt install verilator"
    echo "  xsim      : 隨 Vivado 附帶,需先 source settings64.sh"
    exit 127
fi

rm -rf "$OUT"; mkdir -p "$OUT"
echo "engine: $ENGINE     logs: sim_out/"

show_log() {          # $1 = log 檔
    echo "  ---- $(basename "$1") ----"
    sed 's/^/  | /' "$1" | tail -25
    echo "  ------------------------"
}

run_one() {           # $1 = bench top, $2 = K_MAX(空字串 = 不帶參數)
    local top="$1" kmax="$2"
    local tag="$top${kmax:+ K_MAX=$kmax}"
    local d="$OUT/${top}${kmax:+_$kmax}"
    local out
    mkdir -p "$d"

    if [ "$ENGINE" = verilator ]; then
        local g=(); [ -n "$kmax" ] && g=(-GK_MAX="$kmax")
        if ! verilator --binary --timing -Wno-fatal "${g[@]}" \
                "${RTL[@]}" "$SRC_DIR/$top.sv" --top "$top" \
                -o "$top" --Mdir "$d/obj" >"$d/build.log" 2>&1; then
            echo "  BUILD FAIL  $tag"; show_log "$d/build.log"; return 1
        fi
        out=$("$d/obj/$top" 2>&1 | tee "$d/run.log")
    else
        local g=(); [ -n "$kmax" ] && g=(-generic_top "K_MAX=$kmax")
        if ! ( cd "$d" && xvlog -sv "${RTL[@]}" "$SRC_DIR/$top.sv" ) \
                >"$d/xvlog.log" 2>&1; then
            echo "  BUILD FAIL  $tag  (xvlog)"; show_log "$d/xvlog.log"; return 1
        fi
        if ! ( cd "$d" && xelab "$top" "${g[@]}" -s "sim_$top" ) \
                >"$d/xelab.log" 2>&1; then
            echo "  BUILD FAIL  $tag  (xelab)"; show_log "$d/xelab.log"; return 1
        fi
        out=$( cd "$d" && xsim "sim_$top" -R 2>&1 | tee run.log )
    fi

    if echo "$out" | grep -q "^PASS"; then
        echo "  PASS  $tag  $(echo "$out" | grep -oE '比對 [0-9]+ 次|checked = [0-9]+' | head -1)"
        return 0
    fi
    echo "  FAIL  $tag"
    echo "$out" | grep -E "FAIL|golden|期望|Error" | head -8 | sed 's/^/  | /'
    return 1
}

echo
echo "== 等價性:重構前後逐拍相同 =="
for k in "${KPOINTS[@]}"; do
    run_one tb_operand_buffer_equiv "$k" || FAIL=1
done

echo
echo "== 契約:feeder + buffer =="
run_one tb_feeder_buffer "" || FAIL=1

echo
if [ "$FAIL" -eq 0 ]; then
    echo "全部 PASS"
else
    echo "有 FAIL,完整 log 在 sim_out/"
fi
exit "$FAIL"