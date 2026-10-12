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
#   --cosim-dir DIR   the co-simulation to run (default: BUILD/cosim)
#   --out DIR         where the cases and the summary go
#                     (default: BUILD/e2e/<backend>); it is emptied first,
#                     so an existing DIR must be one this script made
#   --quick           only the six required shapes x four value classes
#   --cases a,b,...   only these cases (names as gen_cases.py prints them)
#   --kmax-cases      instead of the fixed shapes, the cases that put K at the
#                     edges of the bitstream's K_MAX_8X8 and K_MAX_4X4
#                     (gen_cases.py --kmax)
#
# The bitstream's K_MAX_8X8 / K_MAX_4X4 reach the compiler and the runtime
# through SYSTOLIC_FPGA_CONFIG (runtime/fpga_matmul_tiled.h). On the
# co-simulation it is set to the fpga_config.txt build.sh wrote next to
# Vcosim (an exported SYSTOLIC_FPGA_CONFIG must name the same K_MAX values, or
# the run stops: the simulated RTL cannot be another bitstream); on the board,
# export it for the bitstream that is programmed (the archived one, 16 / 16,
# needs nothing). Before any case, malformed configurations must be refused
# by systolic-matmul before it touches the FPGA.
#
# Each case leaves build/e2e/<backend>/<case>/{input.txt,output.txt,ir/}; the
# summary is build/e2e/<backend>/summary.txt. On the co-simulation every
# case's DPTI bytes are also checked against the bytes the RTL consumed and
# sent, and one run with an injected write failure must be reported as an
# FPGA dispatch failure. Exit status 0 only if the configuration refusals,
# the runtime check, every case and (co-simulation) the failure check pass.
set -uo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/../.." && pwd)"

BACKEND=board
BUILD="$ROOT/build"
COSIM_DIR=""
OUT=""
QUICK=0
KMAX_CASES=0
ONLY=""
while [ $# -gt 0 ]; do
    case "$1" in
        --backend)    BACKEND="$2"; shift 2 ;;
        --build-dir)  BUILD="$(cd "$2" && pwd)"; shift 2 ;;
        --cosim-dir)  COSIM_DIR="$(cd "$2" && pwd)"; shift 2 ;;
        --out)        OUT="$2"; shift 2 ;;
        --quick)      QUICK=1; shift ;;
        --kmax-cases) KMAX_CASES=1; shift ;;
        --cases)      ONLY="$2"; shift 2 ;;
        -h|--help)    sed -n '3,36p' "$0"; exit 0 ;;
        *) echo "unknown argument: $1" >&2; exit 2 ;;
    esac
done
case "$BACKEND" in board|cosim) ;; *) echo "--backend board|cosim" >&2; exit 2 ;; esac

MATMUL="$BUILD/bin/systolic-matmul"
RTTEST="$BUILD/bin/runtime-dispatch-test"
for b in "$MATMUL" "$RTTEST"; do
    [ -x "$b" ] || { echo "missing $b -- build first (cmake --build $BUILD)" >&2; exit 2; }
done

[ -n "$COSIM_DIR" ] || COSIM_DIR="$BUILD/cosim"

# A K_MAX value of a configuration file, as the runtime reads it: plain
# decimal, no leading zeros (anything else the runtime refuses, and bash
# arithmetic below would read a leading 0 as octal). Empty if absent/invalid.
cfg_value_of() {
    sed -n -E "s/^[[:space:]]*$2[[:space:]]*=[[:space:]]*([1-9][0-9]*)[[:space:]]*(#.*)?$/\1/p" \
        "$1" | head -1
}
if [ "$BACKEND" = cosim ] && [ -f "$COSIM_DIR/fpga_config.txt" ]; then
    if [ -z "${SYSTOLIC_FPGA_CONFIG:-}" ]; then
        export SYSTOLIC_FPGA_CONFIG="$COSIM_DIR/fpga_config.txt"
    else
        # The co-simulation was built with fixed K_MAX values; a runtime told
        # anything else would send slabs the RTL refuses (err_mem_length) and
        # time out case after case.
        for key in K_MAX_8X8 K_MAX_4X4; do
            sim=$(cfg_value_of "$COSIM_DIR/fpga_config.txt" "$key")
            env=$(cfg_value_of "$SYSTOLIC_FPGA_CONFIG" "$key")
            if [ "$sim" != "$env" ]; then
                echo "SYSTOLIC_FPGA_CONFIG=$SYSTOLIC_FPGA_CONFIG says $key = ${env:-?}, but the co-simulation in $COSIM_DIR was built with $key = ${sim:-?}; unset it or point it at $COSIM_DIR/fpga_config.txt" >&2
                exit 2
            fi
        done
    fi
fi

# The bitstream's K_MAX_8X8 / K_MAX_4X4, as the runtime will read them.
cfg_value() {
    [ -n "${SYSTOLIC_FPGA_CONFIG:-}" ] || { echo 16; return; }
    cfg_value_of "$SYSTOLIC_FPGA_CONFIG" "$1"
}
K8=$(cfg_value K_MAX_8X8)
K4=$(cfg_value K_MAX_4X4)
[ -n "$K8" ] && [ -n "$K4" ] || {
    echo "no valid K_MAX_8X8 / K_MAX_4X4 (decimal, no leading zeros) in SYSTOLIC_FPGA_CONFIG=$SYSTOLIC_FPGA_CONFIG" >&2; exit 2; }
# One MEM_WRITE carries the deepest geometry's slab: ceil(K_MAX / 8) windows
# of 512 bytes (1024 at 16 / 16).
SLAB8=$(( (K8 + 7) / 8 * 512 )); SLAB4=$(( (K4 + 7) / 8 * 512 ))
SLAB=$(( SLAB8 > SLAB4 ? SLAB8 : SLAB4 ))

[ -n "$OUT" ] || OUT="$BUILD/e2e/$BACKEND"
# OUT is emptied first. A directory given with --out is only emptied if this
# script made it (its marker is there) or it is empty: --out pointed at a
# source or build tree by mistake must not wipe it.
if [ -d "$OUT" ] && [ -n "$(ls -A "$OUT" 2>/dev/null)" ] &&
   [ "$OUT" != "$BUILD/e2e/$BACKEND" ] && [ ! -f "$OUT/.run_e2e_out" ]; then
    echo "--out $OUT is not empty and was not made by run_e2e.sh; not deleting it" >&2
    exit 2
fi
rm -rf "$OUT"
mkdir -p "$OUT/cases"
OUT="$(cd "$OUT" && pwd)"
: > "$OUT/.run_e2e_out"
if [ "$KMAX_CASES" = 1 ]; then
    python3 "$HERE/gen_cases.py" "$OUT/cases" --kmax "$K8" "$K4" > "$OUT/case_list.txt" || exit 2
else
    python3 "$HERE/gen_cases.py" "$OUT/cases" > "$OUT/case_list.txt" || exit 2
fi

COSIM_PID=""
cleanup() { [ -n "$COSIM_PID" ] && kill "$COSIM_PID" 2>/dev/null; }
trap cleanup EXIT

if [ "$BACKEND" = cosim ]; then
    VCOSIM="$COSIM_DIR/Vcosim"
    SHIM="$COSIM_DIR/libftdi_cosim.so"
    [ -x "$VCOSIM" ] && [ -f "$SHIM" ] || {
        echo "no co-simulation in $COSIM_DIR -- run test/e2e/cosim/build.sh" >&2; exit 2; }
    SOCK="$OUT/dpti.sock"
    # The RTL's own trace prints three lines per clock; keep the rest.
    "$VCOSIM" +socket="$SOCK" +max_sessions=0 \
        > >(grep --line-buffered -v -E '^(JOBFIREDBG|SCHEDHS|SCHTRACE|JOBHS) ' > "$OUT/cosim.log") 2>&1 &
    COSIM_PID=$!
    for _ in $(seq 100); do [ -S "$SOCK" ] && break; sleep 0.1; done
    export LD_PRELOAD="$SHIM"
    export SYSTOLIC_DPTI_COSIM_SOCKET="$SOCK"
    export SYSTOLIC_DPTI_TIMEOUT_MS="${SYSTOLIC_DPTI_TIMEOUT_MS:-600000}"
    echo "backend: RTL co-simulation of $(cat "$COSIM_DIR/RTL_SOURCE" 2>/dev/null)"
    echo "         (not the board: the FPGA is simulated by Verilator)"
else
    echo "backend: FPGA board over DPTI (FT2232H 0403:6010)"
    echo "         expects artifacts/fpga/tile_pipelined/1x8x8-3x4x4/systolic_dma_top_1x8x8_3x4x4.bit"
    echo "         (sha256 b9087bade05bd5efc9cad456736a3c0ebb757e62de64d824de0c79e691e7f3e0) on the board"
fi
echo "bitstream parameters: K_MAX_8X8 = $K8, K_MAX_4X4 = $K4, $SLAB-byte operand slab"
echo "         (${SYSTOLIC_FPGA_CONFIG:-built-in default: the archived bitstream})"
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
# session -> "received consumed sent": the bytes the simulated FT2232H got
# from the host, the bytes the RTL took from it (RD# strobes), and the bytes
# the RTL sent back (WR# strobes).
cosim_bytes() {
    grep -m1 "^cosim: session $1 closed" "$OUT/cosim.log" 2>/dev/null \
        | sed -E 's/.*host->fpga ([0-9]+) B received, ([0-9]+) B consumed by the RTL, fpga->host ([0-9]+) B.*/\1 \2 \3/'
}

# ---- 0. a malformed configuration is refused before the FPGA is touched ---
# systolic-matmul reads SYSTOLIC_FPGA_CONFIG first and must exit 4 with the
# runtime's reason, without opening the link (no co-simulation session).
echo "== configuration refusal: malformed SYSTOLIC_FPGA_CONFIG files"
cfg_ok=1
CFGDIR="$OUT/config_refusal"
mkdir -p "$CFGDIR"
[ "$BACKEND" = cosim ] && cfg_sessions_before=$(sessions_opened)
while IFS='|' read -r tag body want; do
    if [ "$tag" = no_file ]; then
        rm -f "$CFGDIR/$tag.txt"
    else
        printf "%b" "$body" > "$CFGDIR/$tag.txt"
    fi
    out=$(printf '1 1 1\n1\n1\n' | SYSTOLIC_FPGA_CONFIG="$CFGDIR/$tag.txt" "$MATMUL" 2>&1)
    rc=$?
    if [ $rc -eq 4 ] && printf "%s" "$out" | grep -qF -- "$want"; then
        v=PASS
    else
        v=FAIL; cfg_ok=0
    fi
    printf "   %-16s %s  (exit %s: %s)\n" "$tag" "$v" "$rc" \
        "$(printf "%s" "$out" | head -1 | sed -E 's/^systolic-matmul: SYSTOLIC_FPGA_CONFIG=[^:]*: //' | cut -c1-100)"
done <<'EOF'
leading_zero|K_MAX_8X8=016\nK_MAX_4X4=16\n|K_MAX_8X8 = '016' is not a decimal integer in 1..16376
zero|K_MAX_8X8=16\nK_MAX_4X4=0\n|K_MAX_4X4 = '0' is not a decimal integer in 1..16376
too_deep|K_MAX_8X8=16377\nK_MAX_4X4=16\n|K_MAX_8X8 = '16377' is not a decimal integer in 1..16376
not_a_number|K_MAX_8X8=16\nK_MAX_4X4=7.5\n|K_MAX_4X4 = '7.5' is not a decimal integer in 1..16376
missing_4x4|K_MAX_8X8=16\n|K_MAX_8X8 and K_MAX_4X4 are both required
other_fleet|K_MAX_8X8=16\nK_MAX_4X4=16\nNUM_4X4=2\n|this runtime drives the 1x8x8 + 3x4x4 fleet
no_file||cannot open
EOF
if [ "$BACKEND" = cosim ] && [ "$(sessions_opened)" != "$cfg_sessions_before" ]; then
    echo "   FAIL: a refused configuration opened the FPGA link"; cfg_ok=0
fi
[ "$cfg_ok" = 1 ] && cfg=PASS || cfg=FAIL
echo "   configuration refusal: $cfg"
echo

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
        # Every byte the runtime counted out must have been consumed by the
        # RTL, and every byte it counted in must have been sent by the RTL.
        if settle_sessions && [ "$(sessions_opened)" -eq $((before + 1)) ]; then
            read -r crecv ccons csent < <(cosim_bytes "$((before + 1))")
            if [ "$ccons" != "$to" ] || [ "$crecv" != "$ccons" ] || [ "$csent" != "$back" ]; then
                verdict=FAIL
                note="runtime counted $to/$back B; RTL received $crecv, consumed $ccons, sent $csent B"
            else
                note="RTL consumed $ccons B, sent $csent B (session $((before + 1)))"
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

# ---- 3. a broken link is reported, not computed around (co-simulation) ---
# The co-simulation's libftdi fails write #7 of the session: the 5th WRITE32
# of the first of five descriptors (16x8x4). The MEM_WRITE (9 + SLAB B, 1024
# at 16 / 16) and four WRITE32s (4 x 6 B) are all that may reach the RTL --
# no submit, so no result -- and systolic-matmul must exit 4 with the
# runtime's error. 16x8x4 is five invocations as long as every K_MAX >= 4;
# with a shallower bitstream the count differs and the check is skipped.
df_ok=1
DF_BYTES=$(( 9 + SLAB + 4 * 6 ))
if [ "$BACKEND" = cosim ] && [ "$K8" -ge 4 ] && [ "$K4" -ge 4 ]; then
    echo "== dispatch failure: write #7 of the session fails (5th WRITE32 of the first descriptor)"
    d="$OUT/dispatch_failure"
    mkdir -p "$d"
    python3 "$HERE/gen_cases.py" "$d/gen" > /dev/null
    cp "$d/gen/16x8x4_mixed.txt" "$d/input.txt"
    before=$(sessions_opened)
    SYSTOLIC_DPTI_COSIM_FAIL_WRITE=7 "$MATMUL" < "$d/input.txt" > "$d/output.txt" 2>&1
    frc=$?
    settle_sessions
    read -r frecv fcons fsent < <(cosim_bytes "$((before + 1))")
    sed -n -E 's/^ *(DPTI: .*)/   runtime: \1/p; s/^(FPGA dispatch: .*)/   \1/p' "$d/output.txt"
    echo "   exit $frc; RTL received ${frecv:--} B, consumed ${fcons:--} B, sent ${fsent:--} B"
    if [ $frc -eq 4 ] &&
       grep -q '^FPGA dispatch: FAIL -- 5 of 5 runtime call(s) failed (first error -6)' "$d/output.txt" &&
       grep -q "DPTI: $DF_BYTES B to the FPGA, 0 B back" "$d/output.txt" &&
       [ "$frecv" = "$DF_BYTES" ] && [ "$fcons" = "$DF_BYTES" ] && [ "$fsent" = 0 ]; then
        df=PASS
    else
        df=FAIL; df_ok=0
    fi
    echo "dispatch failure reported: $df" | tee -a "$OUT/summary.txt"
elif [ "$BACKEND" = cosim ]; then
    echo "dispatch failure reported: NOT TESTED (K_MAX < 4: 16x8x4 is not five invocations)" \
        | tee -a "$OUT/summary.txt"
fi

if [ $rt_rc -eq 0 ]; then rt=PASS; else rt=FAIL; fi
echo "configuration refusal: $cfg" | tee -a "$OUT/summary.txt"
echo "runtime layer: $rt" | tee -a "$OUT/summary.txt"
echo "E2E cases: $pass/$total PASS on backend $BACKEND" | tee -a "$OUT/summary.txt"
[ $rt_rc -eq 0 ] && [ $fail -eq 0 ] && [ $total -gt 0 ] && [ $df_ok -eq 1 ] && [ $cfg_ok -eq 1 ]
