#!/usr/bin/env bash
#
# run_v2_benches.sh -- the beat-wide operand path, every geometry the paper
# and the two k_max bitstreams need, under Icarus.
#
#   ./run_v2_benches.sh            # all geometries
#   ./run_v2_benches.sh quick      # N=8 K_MAX=64 only
#
# Two benches per geometry:
#   tb_dma_operand_writer_v2  writer beat decode == UART rx_count decode
#   tb_dma_path_v2            v2 image == v1 image == UART image, checksums
#                             agree, streaming read, two base addresses,
#                             seed-writer path
#
# N = 8 is the degenerate geometry (A and B lane/koff slices have the same
# width), so 4 and 16 are not optional.  K_MAX = 32 and 256 are the two
# bitstreams of the k_max decision; 64 is the bench default.
set -u

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"   # .../dma/tb
DMA="$(dirname "$HERE")"
ROOT="$(dirname "$DMA")"                               # .../fold_pipelined
OUT="${V2_SIM_OUT:-$HERE/sim_out_v2}"
mkdir -p "$OUT"

command -v iverilog >/dev/null 2>&1 || { echo "iverilog not found"; exit 127; }

WRITER_SRC="$HERE/tb_dma_operand_writer_v2.sv $DMA/dma_operand_writer_v2.sv"
PATH_SRC="$HERE/tb_dma_path_v2.sv \
  $DMA/dma_engine.sv $DMA/dma_seed_writer.sv \
  $DMA/dma_operand_writer.sv $DMA/dma_operand_writer_v2.sv $DMA/dma_wr_checksum_v2.sv \
  $ROOT/core/operand_buffer.sv $ROOT/core/operand_buffer_v2.sv"

if [ "${1:-}" = "quick" ]; then
  GEOMS="8:64:0"
else
  GEOMS="4:32:0 8:32:0 16:32:0  4:64:0 8:64:0 16:64:0  4:256:0 8:256:0 16:256:0  8:256:1 4:64:1"
fi

pass=0; failn=0; failed=""
for g in $GEOMS; do
  IFS=: read -r N K SEED <<<"$g"
  tag="n${N}_k${K}_s${SEED}"

  # writer decode
  if iverilog -g2012 -Ptb_dma_operand_writer_v2.N="$N" -Ptb_dma_operand_writer_v2.K_MAX="$K" \
       -o "$OUT/w_$tag.vvp" $WRITER_SRC >"$OUT/w_$tag.log" 2>&1 \
     && vvp "$OUT/w_$tag.vvp" >>"$OUT/w_$tag.log" 2>&1 \
     && grep -q "BEAT DECODE IS IDENTICAL" "$OUT/w_$tag.log"; then
    echo "PASS  writer  N=$N K_MAX=$K"; pass=$((pass+1))
  else
    echo "FAIL  writer  N=$N K_MAX=$K   (see $OUT/w_$tag.log)"; failn=$((failn+1)); failed="$failed writer:$tag"
  fi

  # image
  if iverilog -g2012 -Ptb_dma_path_v2.N="$N" -Ptb_dma_path_v2.K_MAX="$K" -Ptb_dma_path_v2.SEED_MODE="$SEED" \
       -o "$OUT/p_$tag.vvp" $PATH_SRC >"$OUT/p_$tag.log" 2>&1 \
     && vvp "$OUT/p_$tag.vvp" >>"$OUT/p_$tag.log" 2>&1 \
     && grep -q "V2 IMAGE == V1 IMAGE == UART PATH" "$OUT/p_$tag.log"; then
    chk=$(grep -o "board checksum for this geometry = 0x[0-9a-f]*" "$OUT/p_$tag.log" | tail -1 | sed 's/.*= //')
    fill=$(grep -o "fill v2 = [0-9]* cycles ([0-9.]* w/c)" "$OUT/p_$tag.log" | tail -1)
    echo "PASS  image   N=$N K_MAX=$K SEED_MODE=$SEED   chk=$chk   $fill"; pass=$((pass+1))
  else
    echo "FAIL  image   N=$N K_MAX=$K SEED_MODE=$SEED   (see $OUT/p_$tag.log)"; failn=$((failn+1)); failed="$failed image:$tag"
  fi
done

echo
echo "$pass passed, $failn failed${failed:+ :$failed}"
[ "$failn" -eq 0 ]
