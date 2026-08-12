#!/usr/bin/env bash
# sweep_kmax.sh -- k_max design-space sweep for the 8x8 FP32 fold array.
#
#   ./sweep_kmax.sh --sim-only   # WORKS TODAY: cycles + correctness
#   ./sweep_kmax.sh --sim-only 16 32 64 128
#   ./sweep_kmax.sh              # sim + synthesis (synthesis is gated, see below)
#
# ---------------------------------------------------------------------------
# STATUS
#
#   simulation  ENABLED   -- drives systolic_array_8x8_fold directly through
#                            tb_array_fold_kmax.sv. The array has no K
#                            parameter (K is just how long valid is held), so
#                            this needs nothing from the top level and gives
#                            real cycle counts at any K right now.
#
#   synthesis   GATED     -- systolic_uart_fold_top.sv is the fixed K=16
#                            baseline and has no capacity parameter, so only
#                            K_MAX=16 can be built. build_kmax.tcl refuses
#                            anything else rather than silently synthesising
#                            K=16 hardware and labelling it K_MAX=64.
#
# An earlier revision passed a top-level NFOLD generic. That baked the fold
# COUNT into the hardware; the fold count is a runtime scheduling quantity
# derived from k_dim, not a synthesis parameter. NFOLD is gone and must not
# come back under another name.
#
# TODO(K_MAX): once the RTL exposes `parameter int K_MAX`, drop the guard in
# build_kmax.tcl and the SYN_ALLOWED list below becomes "16 32 64 128".
# ---------------------------------------------------------------------------

set -uo pipefail

KMAX_LIST=(16 32 64 128)

# K_MAX values the RTL can currently be built at. Extend after the refactor.
SYN_ALLOWED=(16)

DO_SIM=1
DO_SYN=1

ARGS=()
for a in "$@"; do
    case "$a" in
        --sim-only) DO_SYN=0 ;;
        --syn-only) DO_SIM=0 ;;
        -h|--help)  sed -n '2,30p' "$0"; exit 0 ;;
        *)          ARGS+=("$a") ;;
    esac
done
[ ${#ARGS[@]} -gt 0 ] && KMAX_LIST=("${ARGS[@]}")

OUT=build_kmax
mkdir -p "$OUT/logs"

echo "=============================================="
echo " k_max sweep: ${KMAX_LIST[*]}"
echo " sim=$DO_SIM  syn=$DO_SYN"
echo " synthesis currently buildable at: ${SYN_ALLOWED[*]}"
echo "=============================================="

need() {
    command -v "$1" >/dev/null 2>&1 || {
        echo "ERROR: $1 not on PATH. Source your Vivado settings64.sh first."
        exit 1
    }
}

# ---------------------------------------------------------------------------
# Simulation: cycles + correctness, no synthesis and no top-level parameter
# ---------------------------------------------------------------------------
if [ "$DO_SIM" = 1 ]; then
    need vivado

    # fp_mul.sv / fp_add.sv instantiate Xilinx floating-point IP whose
    # sources are generated, gitignored, and need library mappings a bare
    # xelab call does not have. sim_kmax.tcl goes through a real project so
    # launch_simulation handles IP generation and compile order for us.
    #
    # XCI_DIR can be overridden if the IP lives somewhere else:
    #   XCI_DIR=/path/to/srcs/sources_1/ip ./sweep_kmax.sh --sim-only
    XCI_DIR="${XCI_DIR:-rtl_fp_pe_test/rtl_fp_pe_test.srcs/sources_1/ip}"

    if [ ! -f "$XCI_DIR/floating_point_add_0/floating_point_add_0.xci" ]; then
        echo
        echo "ERROR: floating-point IP not found under"
        echo "         $XCI_DIR"
        echo
        echo "  These are local Vivado artifacts, not in the repository."
        echo "  Find them with:"
        echo "     find ~ -name 'floating_point_add_0.xci' 2>/dev/null"
        echo "  then re-run with:"
        echo "     XCI_DIR=<dir containing floating_point_*_0/> \\"
        echo "       ./sweep_kmax.sh --sim-only"
        exit 1
    fi

    for K in "${KMAX_LIST[@]}"; do
        echo
        echo "--- sim K=$K ---"
        LOG="$OUT/logs/sim_k${K}.log"

        vivado -mode batch -nojournal -notrace \
               -log "$OUT/logs/vivado_sim_k${K}.log" \
               -source sim_kmax.tcl -tclargs "$K" "$XCI_DIR" \
               > "$LOG" 2>&1

        if grep -q "^KMAXCSV" "$LOG"; then
            grep "^KMAXCSV" "$LOG" | tail -1
            grep -E "^(PASS|FAIL)" "$LOG" | tail -1
        else
            echo "  FAILED at K=$K. Last 30 lines of $LOG:"
            echo "  ----------------------------------------"
            tail -30 "$LOG" | sed 's/^/  /'
            echo "  ----------------------------------------"
        fi
    done
fi

# ---------------------------------------------------------------------------
# Synthesis + implementation
# ---------------------------------------------------------------------------
if [ "$DO_SYN" = 1 ]; then
    need vivado

    for K in "${KMAX_LIST[@]}"; do
        echo
        echo "--- synth K_MAX=$K ---"

        allowed=0
        for ok in "${SYN_ALLOWED[@]}"; do
            [ "$K" = "$ok" ] && allowed=1
        done
        if [ "$allowed" = 0 ]; then
            echo "  SKIPPED: the RTL has no K_MAX parameter yet, so K_MAX=$K"
            echo "  cannot be built. Only ${SYN_ALLOWED[*]} is available."
            echo "  Cycle counts for K=$K are still available via --sim-only."
            continue
        fi

        if [ "$DO_SIM" = 1 ] && ! grep -q "^KMAXCSV" "$OUT/logs/sim_k${K}.log" 2>/dev/null; then
            echo "  SKIPPED: simulation did not pass at K=$K."
            echo "  Refusing to synthesise a configuration whose numerics"
            echo "  were not verified."
            continue
        fi

        vivado -mode batch -nojournal -notrace \
               -log "$OUT/logs/vivado_k${K}.log" \
               -source build_kmax.tcl -tclargs "$K" \
          || echo "  vivado returned non-zero for K_MAX=$K -- see $OUT/logs/vivado_k${K}.log"
    done
fi

echo
echo "=============================================="
echo " collecting results"
echo "=============================================="
python3 collect_kmax.py --dir "$OUT" --out "$OUT/kmax_sweep.csv"