#!/usr/bin/env bash
# compile_torch_mlir.sh
#
# 把一份 torch-mlir 匯出的 .mlir(已經過 fix_torch_mlir_syntax.py 修正過語法、
# 改過函式名)一路編譯成 LLVM IR,套用 Systolic-MLIR 已知正確的 pass 順序:
#
#   1. systolic-opt 上的 offload pass(--tile-matmul-for-fpga /
#      --conv2d-to-fpga / --conv2d-nchw-to-fpga / --batch-matmul-to-fpga,
#      可疊加多個)
#   2. mlir-opt-18 標準 lowering,順序是今天實測過確實正確的:
#        --llvm-request-c-wrappers        (必須在 --convert-func-to-llvm 前)
#        --one-shot-bufferize
#        --convert-linalg-to-loops        (必須在 --convert-scf-to-cf 前,
#                                           否則 linalg.fill 展開的 scf.for
#                                           不會被後面的 pass 處理到)
#        --expand-strided-metadata        (必須在 --convert-scf-to-cf 前,
#                                           否則 memref.expand_shape 這類
#                                           reshape op 轉 LLVM 時會留下
#                                           unrealized_conversion_cast)
#        --convert-scf-to-cf
#        --convert-to-llvm
#        --reconcile-unrealized-casts
#   3. mlir-translate-18 轉成 .ll
#
# 用法:
#   ./compile_torch_mlir.sh <input.mlir> <output_prefix> [systolic-opt pass args...]
#
# 範例:
#   ./compile_torch_mlir.sh /tmp/torch_attention_fixed.mlir /tmp/torch_attention \
#       --batch-matmul-to-fpga
#
# 產物:
#   <output_prefix>_step1.mlir   -- systolic-opt 轉完的中間結果
#   <output_prefix>_llvm.mlir    -- 完整 lowering 到 LLVM dialect 後的結果
#   <output_prefix>.ll           -- LLVM IR,可直接 clang -c 編譯

set -euo pipefail

if [ $# -lt 2 ]; then
    echo "用法: $0 <input.mlir> <output_prefix> [systolic-opt pass args...]" >&2
    exit 1
fi

INPUT="$1"
PREFIX="$2"
shift 2
SYSTOLIC_OPT_ARGS=("$@")

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SYSTOLIC_OPT="${REPO_ROOT}/build/tools/systolic-opt/systolic-opt"

if [ ${#SYSTOLIC_OPT_ARGS[@]} -eq 0 ]; then
    echo "警告: 沒有指定任何 systolic-opt pass,將只跑標準 lowering" >&2
fi

echo "=== Step 1: systolic-opt (${SYSTOLIC_OPT_ARGS[*]:-<none>}) ==="
if [ ${#SYSTOLIC_OPT_ARGS[@]} -gt 0 ]; then
    "${SYSTOLIC_OPT}" "${INPUT}" "${SYSTOLIC_OPT_ARGS[@]}" -o "${PREFIX}_step1.mlir"
else
    cp "${INPUT}" "${PREFIX}_step1.mlir"
fi

echo "=== Step 2: mlir-opt-18 lowering ==="
mlir-opt-18 "${PREFIX}_step1.mlir" \
    --llvm-request-c-wrappers \
    --one-shot-bufferize="bufferize-function-boundaries" \
    --convert-linalg-to-loops \
    --expand-strided-metadata \
    --convert-scf-to-cf \
    --convert-to-llvm \
    --reconcile-unrealized-casts \
    -o "${PREFIX}_llvm.mlir"

echo "=== Step 3: mlir-translate-18 ==="
mlir-translate-18 --mlir-to-llvmir "${PREFIX}_llvm.mlir" -o "${PREFIX}.ll"

echo ""
echo "完成。C interface wrapper:"
grep "_mlir_ciface" "${PREFIX}_llvm.mlir" || echo "  (沒有找到 _mlir_ciface_* -- 檢查輸入函式是否有 llvm.emit_c_interface 屬性)"
echo ""
echo "產物:"
echo "  ${PREFIX}_step1.mlir"
echo "  ${PREFIX}_llvm.mlir"
echo "  ${PREFIX}.ll   <- 用這個接 clang-18 -O2 -c 編譯"
