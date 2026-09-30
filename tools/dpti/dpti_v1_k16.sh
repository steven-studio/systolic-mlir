#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
DMA_DIR="$ROOT/rtl/multi_200t/fold_pipelined/dma"
LOG="/tmp/dpti_v1_k16.$$.log"

EXPECTED_WR="3f880780"
EXPECTED_C="c74b2660"
EXPECTED_CYC="125"
EXPECTED_WORDS="256"

cleanup() {
    rm -f "$LOG"
}
trap cleanup EXIT

cd "$DMA_DIR"

echo "=============================================="
echo " DPTI — DMA Path Test / Integration"
echo "=============================================="
echo " variant : v1"
echo " N       : 8"
echo " K_MAX   : 16"
echo " K_DIM   : 16"
echo

vivado -mode batch \
    -source dma_top_build.tcl \
    -tclargs read v1 16 2>&1 | tee "$LOG"

echo
echo "=============================================="
echo " DPTI CHECK"
echo "=============================================="

fail=0

check() {
    local name="$1"
    local actual="$2"
    local expected="$3"

    if [[ "$actual" == "$expected" ]]; then
        printf " PASS  %-20s %s\n" "$name" "$actual"
    else
        printf " FAIL  %-20s actual=%s expected=%s\n" \
            "$name" "$actual" "$expected"
        fail=1
    fi
}

calib="$(grep -oP 'init_calib_complete = \K[01]+' "$LOG" | tail -1)"
seed="$(grep -oP 'seed written        = \K[01]+' "$LOG" | tail -1)"
desc="$(grep -oP 'descriptor complete = \K[01]+' "$LOG" | tail -1)"
fold="$(grep -oP 'fold complete       = \K[01]+' "$LOG" | tail -1)"
wb="$(grep -oP 'write-back complete = \K[01]+' "$LOG" | tail -1)"
words="$(grep -oP 'words written       = \K[0-9]+' "$LOG" | tail -1)"
chk_wr="$(grep -oP 'chk_wr  0x\K[0-9a-fA-F]+' "$LOG" | tail -1)"
chk_c="$(grep -oP 'chk_c   0x\K[0-9a-fA-F]+' "$LOG" | tail -1)"
cycles="$(grep -oP 'cycles  \K[0-9]+' "$LOG" | tail -1)"
err="$(grep -oP 'any error latched   = \K[01]+' "$LOG" | tail -1)"

check "DDR3 calibration" "$calib" "1"
check "seed complete" "$seed" "1"
check "descriptor complete" "$desc" "1"
check "fold complete" "$fold" "1"
check "write-back complete" "$wb" "1"
check "words written" "$words" "$EXPECTED_WORDS"
check "operand checksum" "$chk_wr" "$EXPECTED_WR"
check "result checksum" "$chk_c" "$EXPECTED_C"
check "compute cycles" "$cycles" "$EXPECTED_CYC"
check "error flag" "$err" "0"

echo

if [[ "$fail" -eq 0 ]]; then
    echo "=============================================="
    echo " DPTI: PASS"
    echo "=============================================="
    exit 0
else
    echo "=============================================="
    echo " DPTI: FAIL"
    echo "=============================================="
    exit 1
fi
