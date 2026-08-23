#!/usr/bin/env bash
# sweep_k_rtl8x8.sh -- 量 systolic_array_8x8_fold 在各 K 的 cycle 數,
#                      產出給成本模型擬合用的乾淨資料表。
#
# 放置位置:hls/multi_200t/fold_pipelined/rtl/
# (需要同目錄的 sim_kmax.tcl 與 tb_array_fold_kmax.sv)
#
#   ./sweep_k_rtl8x8.sh                     # 預設 8 16 24 32 48 64
#   ./sweep_k_rtl8x8.sh 8 16 24 32 48 64
#   XCI_DIR=/path/to/ip ./sweep_k_rtl8x8.sh
#
# ---------------------------------------------------------------------------
# 為什麼不是 sweep_k.sh
#
#   sweep_k.sh 走 Vitis HLS cosim,量的是 design.cpp 的 4x4 HLS 設計
#   (run_hls.tcl: set_top matmul_4x4x4;design.h 的 R/C 固定為 4),
#   而且 run_hls.tcl 的 PART 仍是未修改的 TODO 預設值 xcu250(Alveo U250)。
#   那條路徑與本專案要驗證的 8x8 RTL fold 陣列無關,兩者的數字不可比較。
#
# 為什麼不是 sweep_kmax.sh
#
#   sweep_kmax.sh 的主軸是 K_MAX 合成容量的設計空間掃描,輸出含
#   LUT/FF/BRAM/DSP/timing,是給硬體 trade-off 分析看的。本腳本只要
#   K -> cycles 的乾淨表,而且需要嚴格的資料驗證。兩者用途不同,不互相取代。
#
# ---------------------------------------------------------------------------
# 這支腳本刻意做的事(sweep_k.sh 沒做,因而產生過無聲的壞資料)
#
#   1. 前置檢查工具存在,而不是讓每個 K 各失敗一次
#   2. 解析結果後驗證數值,抓不到就 FAIL,絕不寫入空值
#   3. 檢查 testbench 自報的 sim_errors,非 0 即視為該點無效
#   4. 檢查 K 是 8 的倍數(fold = global_k >> 3,NF = K/8 為整數除法)
#   5. 寫 PROVENANCE.txt:git commit、vivado 版本、XCI_DIR、時間
#   6. 任一點失敗則整體 exit 非 0,不會假裝成功
# ---------------------------------------------------------------------------

set -uo pipefail

KS=("${@:-}")
if [ -z "${KS[0]:-}" ]; then
    KS=(8 16 24 32 48 64)
fi

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUT="$HERE/sweep_k_rtl8x8_out"
CSV="$OUT/sweep_results_8x8.csv"

# ---------------------------------------------------------------------------
# 前置檢查
# ---------------------------------------------------------------------------
if ! command -v vivado >/dev/null 2>&1; then
    echo "ERROR: vivado 不在 PATH。先 source 你的 settings64.sh:" >&2
    echo "  source /path/to/Xilinx/<ver>/Vivado/settings64.sh" >&2
    exit 1
fi

for f in sim_kmax.tcl tb_array_fold_kmax.sv systolic_array_8x8_fold.sv; do
    if [ ! -f "$HERE/$f" ]; then
        echo "ERROR: 找不到 $f。這支腳本必須放在 fold_pipelined/rtl/ 底下。" >&2
        exit 1
    fi
done

# 浮點 IP 是 gitignore 掉的本機 Vivado 產物,不在 repo 裡。
XCI_DIR="${XCI_DIR:-$HERE/rtl_fp_pe_test/rtl_fp_pe_test.srcs/sources_1/ip}"
if [ ! -f "$XCI_DIR/floating_point_add_0/floating_point_add_0.xci" ]; then
    echo "ERROR: 找不到浮點 IP:" >&2
    echo "         $XCI_DIR/floating_point_add_0/floating_point_add_0.xci" >&2
    echo >&2
    echo "  這些是本機 Vivado 產物,不在版控裡。找找看:" >&2
    echo "     find ~ -name 'floating_point_add_0.xci' 2>/dev/null" >&2
    echo "  然後:" >&2
    echo "     XCI_DIR=<含 floating_point_*_0/ 的目錄> $0" >&2
    exit 1
fi

# K 必須是 8 的倍數:tb 的 NF = K/8 是整數除法,預期校驗值會被截斷。
BAD_K=()
for K in "${KS[@]}"; do
    if [ "$K" -le 0 ] 2>/dev/null; then BAD_K+=("$K"); continue; fi
    if [ $(( K % 8 )) -ne 0 ]; then BAD_K+=("$K"); fi
done
if [ ${#BAD_K[@]} -gt 0 ]; then
    echo "ERROR: K 必須是正的 8 的倍數,以下不合法: ${BAD_K[*]}" >&2
    echo "  8x8 fold 陣列每 8 個 k 為一折(fold = global_k >> 3)。" >&2
    echo "  tb_array_fold_kmax.sv 的 NF = K/8 是整數除法,K 非 8 倍數時" >&2
    echo "  預期校驗值會被截斷,correctness check 失去意義。" >&2
    exit 1
fi

mkdir -p "$OUT/logs"

# ---------------------------------------------------------------------------
# Provenance -- 沒有這個,產出的數字三個月後無法追溯
# ---------------------------------------------------------------------------
GIT_COMMIT=$(git -C "$HERE" rev-parse HEAD 2>/dev/null || echo "unknown")
GIT_DIRTY=$(git -C "$HERE" status --porcelain 2>/dev/null | head -1)
VIVADO_VER=$(vivado -version 2>/dev/null | head -1 || echo "unknown")

cat > "$OUT/PROVENANCE.txt" <<EOF
generated:    $(date -u +%Y-%m-%dT%H:%M:%SZ)
host:         $(hostname)
git_commit:   $GIT_COMMIT
git_dirty:    $([ -n "$GIT_DIRTY" ] && echo "YES -- working tree 有未提交的修改" || echo "no")
vivado:       $VIVADO_VER
xci_dir:      $XCI_DIR
dut:          systolic_array_8x8_fold
testbench:    tb_array_fold_kmax.sv
k_values:     ${KS[*]}
EOF

echo "=============================================="
echo " 8x8 fold 陣列 K sweep"
echo " K 值:    ${KS[*]}"
echo " commit:  ${GIT_COMMIT:0:12}$([ -n "$GIT_DIRTY" ] && echo " (dirty)")"
echo " 輸出:    $CSV"
echo "=============================================="

echo "K,folds,feed_cycles,drain_cycles,total_cycles,sim_errors" > "$CSV"

FAILED=()

for K in "${KS[@]}"; do
    echo
    echo "--- sim K=$K (NF=$((K / 8)) folds) ---"
    LOG="$OUT/logs/sim_k${K}.log"

    vivado -mode batch -nojournal -notrace \
           -log "$OUT/logs/vivado_sim_k${K}.log" \
           -source "$HERE/sim_kmax.tcl" -tclargs "$K" "$XCI_DIR" \
           > "$LOG" 2>&1

    # tb 印出:KMAXCSV,k,feed,drain,total,errors
    LINE=$(grep "^KMAXCSV," "$LOG" | tail -1)

    if [ -z "$LINE" ]; then
        echo "  FAILED: log 裡沒有 KMAXCSV 行。最後 30 行:"
        tail -30 "$LOG" | sed 's/^/    /'
        FAILED+=("$K")
        continue
    fi

    IFS=, read -r _tag k feed drain total errs <<< "$LINE"

    # 每個欄位都要是數字。sweep_k.sh 就是少了這一步才寫出空值。
    ok=1
    for v in "$k" "$feed" "$drain" "$total" "$errs"; do
        [[ "$v" =~ ^-?[0-9]+$ ]] || ok=0
    done
    if [ "$ok" = 0 ]; then
        echo "  FAILED: KMAXCSV 行解析不出數字: $LINE"
        FAILED+=("$K")
        continue
    fi

    if [ "$k" != "$K" ]; then
        echo "  FAILED: tb 回報 K=$k,但我們要求的是 K=$K"
        FAILED+=("$K")
        continue
    fi

    if [ "$errs" -ne 0 ]; then
        echo "  FAILED: testbench 回報 $errs 個數值 mismatch。"
        echo "          cycle 數即使看起來合理也不可採用。"
        FAILED+=("$K")
        continue
    fi

    echo "$K,$((K / 8)),$feed,$drain,$total,$errs" >> "$CSV"
    echo "  feed=$feed  drain=$drain  total=$total  errors=0"
done

echo
echo "--- $CSV ---"
cat "$CSV"
echo
cat "$OUT/PROVENANCE.txt"

if [ ${#FAILED[@]} -gt 0 ]; then
    echo
    echo "=============================================="
    echo " 失敗的 K: ${FAILED[*]}"
    echo " 資料不完整,不要拿去擬合。"
    echo "=============================================="
    exit 1
fi

echo
echo "全部成功。"
echo
echo "注意:與解析模型 II*(K+R+C-2)+depth 對照的是 total_cycles,"
echo "      不是 feed_cycles。feed_cycles = K+7 是純幾何量,不含 FP"
echo "      adder latency,拿它對照會得到虛假的完美吻合。"
