#!/usr/bin/env bash
#
# run_kmax_matrix.sh -- independent K_MAX_8X8 / K_MAX_4X4, end to end, for a
# set of configurations, without a board.
#
#   test/e2e/run_kmax_matrix.sh                      the default matrix
#   test/e2e/run_kmax_matrix.sh 16:16 17:7           only these (K8:K4)
#   SKIP_RTL=1 / SKIP_COSIM=1 / SKIP_STANDARD=1      leave one part out
#
# For each configuration K8:K4:
#
#   rtl       rtl/multi_200t/tile_pipelined/dma/tb/run_kmax_sim.sh K8 K4
#             the top on its own, binary32 arithmetic, every result element
#             against an integer reference: the four arrays computing at once,
#             one job per accelerator at k = 1 / K_MAX - 1 / K_MAX, a smaller
#             k after a deeper one, a three-invocation partial sum, and
#             descriptors with k outside 1..K_MAX refused
#   cosim     test/e2e/cosim/build.sh (RTL_DIR=., K_MAX_8X8=K8, K_MAX_4X4=K4)
#   standard  run_e2e.sh --backend cosim --fixed-only: the 36 fixed cases
#             (nine shapes x four value classes, K = 1 .. 40) --
#             systolic-matmul (compiler with k_max = K8 / K4, FPGA lowering
#             checked against the same physical K_MAX) -> JIT -> runtime
#             (SYSTOLIC_FPGA_CONFIG = the co-simulation's fpga_config.txt) ->
#             DPTI -> the RTL, against the CPU reference; plus the
#             configuration refusals, the runtime layer on its own
#             (runtime-dispatch-test) and, where it applies, the broken-link
#             check
#   kmax      run_e2e.sh --backend cosim --kmax-cases: the same pipeline for
#             K = 1, K_MAX - 1, K_MAX, K_MAX + 1 and 2 * max(K8, K4) + 3 of
#             each capacity, on outputs that use the 8x8 and the 4x4s together
#
# The compiler build (build/bin/systolic-matmul, runtime-dispatch-test) must
# exist. Outputs go under build/kmax_matrix/; the summary is
# build/kmax_matrix/summary.txt. Exit status 0 only if every run passed.
set -uo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/../.." && pwd)"
BUILD="${BUILD_DIR:-$ROOT/build}"
OUT="$BUILD/kmax_matrix"
RTL_SIM="$ROOT/rtl/multi_200t/tile_pipelined/dma/tb/run_kmax_sim.sh"

CONFIGS=("$@")
[ ${#CONFIGS[@]} -gt 0 ] || CONFIGS=(16:16 16:8 8:16 24:12 17:7 1:1)

mkdir -p "$OUT"
SUMMARY="$OUT/summary.txt"
printf "%-8s %-6s %-6s %-9s %-6s %s\n" K8:K4 RTL COSIM STANDARD KMAX NOTE | tee "$SUMMARY"

# One run_e2e.sh pass; prints PASS/FAIL and leaves the log.
e2e() {   # tag cdir name [args...]
    local tag="$1" cdir="$2" name="$3"; shift 3
    if SYSTOLIC_FPGA_CONFIG="$cdir/fpga_config.txt" \
       bash "$HERE/run_e2e.sh" --backend cosim --build-dir "$BUILD" \
            --cosim-dir "$cdir" --out "$OUT/e2e/$tag/$name" "$@" \
            > "$OUT/e2e_${name}_$tag.log" 2>&1; then
        echo PASS
    else
        echo FAIL
    fi
}

all_ok=1
for cfg in "${CONFIGS[@]}"; do
    K8="${cfg%%:*}"; K4="${cfg##*:}"
    tag="k8_${K8}-k4_${K4}"
    rtl=SKIP; cosim=SKIP; std=SKIP; kmx=SKIP; note=""

    if [ "${SKIP_RTL:-0}" != 1 ]; then
        if OUT_DIR="$OUT/rtl/$tag" bash "$RTL_SIM" "$K8" "$K4" > "$OUT/rtl_$tag.log" 2>&1; then
            rtl=PASS
        else
            rtl=FAIL; all_ok=0
        fi
    fi

    if [ "${SKIP_COSIM:-0}" != 1 ]; then
        cdir="$OUT/cosim/$tag"
        if (cd "$ROOT" && COSIM_BUILD_DIR="$cdir" RTL_DIR=. K_MAX_8X8="$K8" K_MAX_4X4="$K4" \
                bash "$HERE/cosim/build.sh") > "$OUT/cosim_build_$tag.log" 2>&1; then
            cosim=PASS
            if [ "${SKIP_STANDARD:-0}" != 1 ]; then
                std=$(e2e "$tag" "$cdir" standard --fixed-only)
                [ "$std" = PASS ] || all_ok=0
                note="std: $(grep -E '^E2E cases:' "$OUT/e2e_standard_$tag.log" | tail -1 | sed 's/^E2E cases: //')"
            fi
            kmx=$(e2e "$tag" "$cdir" kmax --kmax-cases)
            [ "$kmx" = PASS ] || all_ok=0
            note="$note${note:+; }kmax: $(grep -E '^E2E cases:' "$OUT/e2e_kmax_$tag.log" | tail -1 | sed 's/^E2E cases: //')"
            df=$(grep -hE '^dispatch failure reported:' "$OUT"/e2e_*_"$tag".log 2>/dev/null | sort -u | sed 's/^dispatch failure reported: //' | tr '\n' ' ')
            [ -n "$df" ] && note="$note; broken link: $df"
        else
            cosim=FAIL; all_ok=0; note="co-simulation build failed"
        fi
    fi

    printf "%-8s %-6s %-6s %-9s %-6s %s\n" "$K8:$K4" "$rtl" "$cosim" "$std" "$kmx" "$note" | tee -a "$SUMMARY"
done

[ "$all_ok" = 1 ]
