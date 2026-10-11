#!/usr/bin/env bash
#
# run_e2e.sh -- the end-to-end regression: every case through
#
#   systolic-matmul (stdin) -> Linalg -> Systolic -> FPGA lowering -> JIT
#   -> fpga_matmul_tiled_auto_scheduled -> DPTI -> FPGA -> C -> CPU reference
#
#   test/e2e/run_e2e.sh --backend board    the Nexys Video, programmed with
#                                          the 1x8x8 + 3x4x4 bitstream
#   test/e2e/run_e2e.sh --backend cosim    the RTL co-simulation of that
#                                          bitstream (test/e2e/cosim/build.sh)
#
# Options:
#   --build-dir DIR   the CMake build directory (default: build)
#   --quick           only the six required shapes x four value classes
#   --cases a,b,...   only these cases (names as gen_cases.py prints them)
#
# Each case leaves build/e2e/<backend>/<case>/{input.txt,output.txt,ir/}; the
# summary is build/e2e/<backend>/summary.txt. Exit status 0 only if the
# runtime check and every case pass.
set -uo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/../.." && pwd)"

BACKEND=board
BUILD="$ROOT/build"
QUICK=0
ONLY=""
while [ $# -gt 0 ]; do
    case "$1" in
        --backend)   BACKEND="$2"; shift 2 ;;
        --build-dir) BUILD="$(cd "$2" && pwd)"; shift 2 ;;
        --quick)     QUICK=1; shift ;;
        --cases)     ONLY="$2"; shift 2 ;;
        -h|--help)   sed -n '3,20p' "$0"; exit 0 ;;
        *) echo "unknown argument: $1" >&2; exit 2 ;;
    esac
done
case "$BACKEND" in board|cosim) ;; *) echo "--backend board|cosim" >&2; exit 2 ;; esac

MATMUL="$BUILD/bin/systolic-matmul"
RTTEST="$BUILD/bin/runtime-dispatch-test"
for b in "$MATMUL" "$RTTEST"; do
    [ -x "$b" ] || { echo "missing $b -- build first (cmake --build $BUILD)" >&2; exit 2; }
done

OUT="$BUILD/e2e/$BACKEND"
rm -rf "$OUT"
mkdir -p "$OUT/cases"
python3 "$HERE/gen_cases.py" "$OUT/cases" > "$OUT/case_list.txt" || exit 2

COSIM_PID=""
cleanup() { [ -n "$COSIM_PID" ] && kill "$COSIM_PID" 2>/dev/null; }
trap cleanup EXIT

if [ "$BACKEND" = cosim ]; then
    VCOSIM="$BUILD/cosim/Vcosim"
    SHIM="$BUILD/cosim/libftdi_cosim.so"
    [ -x "$VCOSIM" ] && [ -f "$SHIM" ] || {
        echo "no co-simulation in $BUILD/cosim -- run test/e2e/cosim/build.sh" >&2; exit 2; }
    SOCK="$OUT/dpti.sock"
    # The RTL's own trace prints three lines per clock; keep the rest.
    "$VCOSIM" +socket="$SOCK" +max_sessions=0 \
        > >(grep --line-buffered -v -E '^(JOBFIREDBG|SCHEDHS|SCHTRACE|JOBHS) ' > "$OUT/cosim.log") 2>&1 &
    COSIM_PID=$!
    for _ in $(seq 100); do [ -S "$SOCK" ] && break; sleep 0.1; done
    export LD_PRELOAD="$SHIM"
    export SYSTOLIC_DPTI_COSIM_SOCKET="$SOCK"
    export SYSTOLIC_DPTI_TIMEOUT_MS="${SYSTOLIC_DPTI_TIMEOUT_MS:-600000}"
    echo "backend: RTL co-simulation of $(cat "$BUILD/cosim/RTL_SOURCE" 2>/dev/null)"
    echo "         (not the board: the FPGA is simulated by Verilator)"
else
    echo "backend: FPGA board over DPTI (FT2232H 0403:6010)"
    echo "         expects artifacts/fpga/tile_pipelined/1x8x8-3x4x4/systolic_dma_top_1x8x8_3x4x4.bit"
    echo "         (sha256 b9087bad05bd5efc9cad456736a3c0ebb757e62de64d824de0c79e691e7f3e0) on the board"
fi
echo

# The co-simulation numbers its sessions (one per program that opened the
# FTDI link) and logs the bytes each one moved; a run's session is the one
# that opened while it ran.
sessions_opened() { grep -c '^cosim: session [0-9]* opened' "$OUT/cosim.log" 2>/dev/null; }
sessions_closed() { grep -c '^cosim: session [0-9]* closed' "$OUT/cosim.log" 2>/dev/null; }
settle_sessions() {
    for _ in $(seq 400); do
        [ "$(sessions_closed)" = "$(sessions_opened)" ] && return 0
        sleep 0.05
    done
    return 1
}
cosim_bytes() {  # session -> "to back", as the RTL side counted them
    grep -m1 "^cosim: session $1 closed" "$OUT/cosim.log" 2>/dev/null \
        | sed -E 's/.*host->fpga ([0-9]+) B, fpga->host ([0-9]+) B.*/\1 \2/'
}

# ---- 1. the runtime layer on its own ---------------------------------------
echo "== runtime layer: host -> runtime -> DPTI -> FPGA (runtime-dispatch-test)"
"$RTTEST" > "$OUT/runtime_dispatch.txt" 2>&1
rt_rc=$?
[ "$BACKEND" = cosim ] && settle_sessions
sed 's/^/   /' "$OUT/runtime_dispatch.txt" | grep -v libftdi_cosim
echo

# ---- 2. every case through the whole pipeline ------------------------------
REQUIRED="1x1x1 2x3x4 4x4x4 8x8x8 8x4x16 16x8x4"
pass=0; fail=0; total=0
printf "%-22s %-8s %6s %6s %9s %9s %s\n" CASE RESULT TILES FPGA BYTES_TO BYTES_BK NOTE \
    | tee "$OUT/summary.txt"
while read -r name; do
    shape="${name%_*}"
    if [ -n "$ONLY" ]; then
        case ",$ONLY," in *",$name,"*) ;; *) continue ;; esac
    elif [ "$QUICK" = 1 ]; then
        case " $REQUIRED " in *" $shape "*) ;; *) continue ;; esac
    fi
    d="$OUT/$name"
    mkdir -p "$d/ir"
    cp "$OUT/cases/$name.txt" "$d/input.txt"
    [ "$BACKEND" = cosim ] && before=$(sessions_opened)
    "$MATMUL" --dump-ir="$d/ir" < "$d/input.txt" > "$d/output.txt" 2>&1
    rc=$?
    total=$((total + 1))

    tiles=$(grep -c 'systolic.matmul_tile' "$d/ir/10-systolic.mlir" 2>/dev/null || echo 0)
    inv=$(sed -nE 's/.*FPGA invocations = ([0-9]+).*/\1/p' "$d/output.txt")
    read -r to back < <(sed -nE 's/.*DPTI: ([0-9]+) B to the FPGA, ([0-9]+) B back.*/\1 \2/p' "$d/output.txt")
    note=""
    verdict=FAIL
    if [ $rc -eq 0 ] && grep -q '^Verification: PASS' "$d/output.txt"; then
        verdict=PASS
    fi
    # Every tile must have been an FPGA invocation.
    if [ "$verdict" = PASS ] && [ "${inv:-0}" != "$tiles" ]; then
        verdict=FAIL; note="tiles $tiles != invocations ${inv:-0}"
    fi
    if [ "$BACKEND" = cosim ] && [ "$verdict" = PASS ]; then
        # The runtime's byte counts must be the bytes the RTL received/sent.
        if settle_sessions && [ "$(sessions_opened)" -eq $((before + 1)) ]; then
            read -r cto cback < <(cosim_bytes "$((before + 1))")
            if [ "$cto" != "$to" ] || [ "$cback" != "$back" ]; then
                verdict=FAIL; note="runtime counted $to/$back B, RTL saw $cto/$cback B"
            else
                note="RTL saw the same $cto/$cback B (session $((before + 1)))"
            fi
        else
            verdict=FAIL; note="expected exactly one co-simulation session"
        fi
    fi
    [ "$verdict" = PASS ] && pass=$((pass + 1)) || fail=$((fail + 1))
    printf "%-22s %-8s %6s %6s %9s %9s %s\n" "$name" "$verdict" "$tiles" "${inv:--}" \
        "${to:--}" "${back:--}" "$note" | tee -a "$OUT/summary.txt"
done < "$OUT/case_list.txt"

echo | tee -a "$OUT/summary.txt"
if [ $rt_rc -eq 0 ]; then rt=PASS; else rt=FAIL; fi
echo "runtime layer: $rt" | tee -a "$OUT/summary.txt"
echo "E2E cases: $pass/$total PASS on backend $BACKEND" | tee -a "$OUT/summary.txt"
[ $rt_rc -eq 0 ] && [ $fail -eq 0 ] && [ $total -gt 0 ]
