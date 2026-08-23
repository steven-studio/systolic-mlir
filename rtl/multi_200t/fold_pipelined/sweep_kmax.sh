#!/usr/bin/env bash
# sweep_kmax.sh -- 一次跑完整個 K_MAX sweep:每一點 build -> 燒錄 -> 板上實測。
#
#   ./sweep_kmax.sh              # N=8,預設 16 32 64 128 256
#   ./sweep_kmax.sh 16 32        # N=8,只跑指定的點
#   ./sweep_kmax.sh -n 4 16 32   # 4x4 陣列的 sweep
#
# 每一點的流程:
#   1. build_kmax.tcl -tclargs K            (Default placement)
#   2. 若 timing 沒過(沒產出 .bit),自動改用 Explore directive 重試一次
#   3. program_kmax.tcl 燒錄(它自己會拒絕 timing 沒過的陳舊 bit)
#   4. test_uart_kmax.py --kmax K 板上實測,以輸出中的 "PASS:" 為準
#
# 結果彙整在 build_kmax/sweep_results.csv(直接就是論文表格的原料),
# 逐點完整 log 在 build_kmax/sweep_logs/。任何一點失敗不會中斷 sweep,
# 會記錄下來繼續跑下一點,最後印總表。
#
# 注意:一個點的 build 要幾十分鐘到幾小時,整趟可能跑一整天。
# 用 tmux 或 nohup 跑,不要裸奔在 ssh session 裡:
#   nohup ./sweep_kmax.sh > sweep.out 2>&1 &
#   tail -f sweep.out

set -u

NARR=8
if [ "${1:-}" = "-n" ]; then
    NARR="$2"; shift 2
fi
NSUF=""
[ "$NARR" != 8 ] && NSUF="_n${NARR}"

KS=(16 32 64 128 256)
[ $# -gt 0 ] && KS=("$@")

TOP=systolic_uart_tile_top
LOGDIR=build_kmax/sweep_logs
RESULTS=build_kmax/sweep_results.csv
mkdir -p "$LOGDIR"

# ---------------------------------------------------------------------
# 開跑前的防呆:上次就是舊腳本默默吃掉參數,同一個坑不跌第二次。
# ---------------------------------------------------------------------
if ! grep -q "set PDIR" build_kmax.tcl; then
    echo "FATAL: build_kmax.tcl 是舊版(沒有 PDIR 支援),先從 Mac 同步新版" >&2
    exit 1
fi
if ! grep -q "set_clock_uncertainty" build_kmax.tcl; then
    echo "FATAL: build_kmax.tcl 沒有 hold 餘裕強化,先從 Mac 同步新版" >&2
    exit 1
fi
if ! command -v vivado >/dev/null; then
    echo "FATAL: 找不到 vivado(source settings64.sh 了嗎?)" >&2
    exit 1
fi

# 彙整檔改為 append:已存在就不重寫表頭,單點補跑不再蓋掉舊數據。
if [ ! -f "$RESULTS" ]; then
    echo "n,k_max,directive,lut,ff,bram,dsp,wns_ns,whs_ns,fmax_mhz,timing_met,board_test" > "$RESULTS"
fi

# 總表用:每點一行的人類可讀摘要
declare -a SUMMARY=()

for K in "${KS[@]}"; do
    echo ""
    echo "======================================================"
    echo " N = $NARR   K_MAX = $K    ($(date '+%H:%M:%S'))"
    echo "======================================================"

    # ---- 1. build(Default;失敗自動 fallback 到 Explore) ----------
    TAG="${K}${NSUF}"
    DIRECTIVE="Default"
    BIT="build_kmax/k${TAG}/${TOP}_k${TAG}.bit"

    vivado -mode batch -source build_kmax.tcl -tclargs "$K" 0 Default "$NARR" \
        > "$LOGDIR/build_k${K}${NSUF}_default.log" 2>&1
    if [ ! -f "$BIT" ]; then
        echo " Default placement timing 未收斂,改用 Explore 重試..."
        TAG="${K}_explore${NSUF}"
        DIRECTIVE="Explore"
        BIT="build_kmax/k${TAG}/${TOP}_k${TAG}.bit"
        vivado -mode batch -source build_kmax.tcl -tclargs "$K" 0 Explore "$NARR" \
            > "$LOGDIR/build_k${K}${NSUF}_explore.log" 2>&1
    fi

    if [ ! -f "$BIT" ]; then
        echo " BUILD FAIL: Default 與 Explore 都沒收斂,跳過這一點"
        echo " log: $LOGDIR/build_k${K}${NSUF}_default.log / _explore.log"
        echo "$NARR,$K,none,NA,NA,NA,NA,NA,NA,NA,0,BUILD_FAIL" >> "$RESULTS"
        SUMMARY+=("N=$NARR K=$K  BUILD_FAIL(兩個 directive 都沒收斂)")
        continue
    fi

    # summary.csv 第二行 = 這一點的數據
    ROW=$(sed -n 2p "build_kmax/k${TAG}/summary.csv")
    echo " build OK ($DIRECTIVE): $ROW"

    # ---- 2. 燒錄 --------------------------------------------------
    if ! vivado -mode batch -source program_kmax.tcl -tclargs "$TAG" \
            > "$LOGDIR/program_k${TAG}.log" 2>&1; then
        echo " PROGRAM FAIL,跳過這一點(log: $LOGDIR/program_k${TAG}.log)"
        echo "$ROW" | \
            awk -F, -v n="$NARR" -v d="$DIRECTIVE" 'BEGIN{OFS=","}{print n,$1,d,$2,$3,$4,$5,$6,$7,$8,$9,"PROGRAM_FAIL"}' \
            >> "$RESULTS"
        SUMMARY+=("N=$NARR K=$K  PROGRAM_FAIL")
        continue
    fi
    sleep 2   # 板子 configuration 後喘口氣再打 serial

    # ---- 3. 板上實測(重試一次) ------------------------------------
    # 單次失敗不算數:request 中一個 byte 毛刺會讓整個 frame 被 END
    # 檢查丟棄,症狀 RX 0(k16_n4 2026-08-20 16:39 實例:單次失敗後
    # 13/13 全過)。重試仍失敗才記 FAIL,確定性失敗不受影響。
    # PASS(retry) 會如實寫進 CSV,毛刺發生率留有紀錄。
    TESTLOG="$LOGDIR/test_k${TAG}.log"
    BOARD=FAIL
    for attempt in 1 2; do
        python3 test_uart_kmax.py --kmax "$K" --n "$NARR" > "$TESTLOG" 2>&1
        if grep -q "^PASS:" "$TESTLOG"; then
            BOARD=PASS
            [ "$attempt" -eq 2 ] && BOARD="PASS_retry"
            break
        fi
        sleep 3
    done
    echo " board test: $BOARD  (log: $TESTLOG)"

    echo "$ROW" | \
        awk -F, -v n="$NARR" -v d="$DIRECTIVE" -v b="$BOARD" 'BEGIN{OFS=","}{print n,$1,d,$2,$3,$4,$5,$6,$7,$8,$9,b}' \
        >> "$RESULTS"
    SUMMARY+=("N=$NARR K=$K  $DIRECTIVE  board=$BOARD")
done

echo ""
echo "======================================================"
echo " SWEEP DONE  ($(date '+%H:%M:%S'))"
echo "======================================================"
for line in "${SUMMARY[@]}"; do echo "  $line"; done
echo ""
echo " 彙整表: $RESULTS"
column -s, -t "$RESULTS"