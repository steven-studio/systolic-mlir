#!/usr/bin/env bash
#
# run_e2e.sh -- the end-to-end regression: every case through
#
#   systolic-matmul (stdin) -> Linalg -> Systolic -> FPGA lowering -> JIT
#   -> fpga_matmul_tiled_auto_scheduled -> DPTI -> FPGA -> C -> CPU reference
#
#   test/e2e/run_e2e.sh --backend board    the Nexys Video, programmed with a
#                                          1x8x8 + 3x4x4 bitstream whose
#                                          configuration is named (below)
#   test/e2e/run_e2e.sh --backend cosim    the RTL co-simulation
#                                          (test/e2e/cosim/build.sh)
#
# The configuration -- K_MAX_8X8 of the 8x8 and K_MAX_4X4 of the 4x4s, two
# independent values, and the operand slab they imply -- is the file in
# SYSTOLIC_FPGA_CONFIG that the compiler and the runtime read. The harness
# takes all three from the runtime's own parser
# (runtime-dispatch-test --print-config) and derives every case and check
# from them. There is no default:
#   board   export SYSTOLIC_FPGA_CONFIG=<the fpga_config.txt dma_top_build.tcl
#           wrote next to the programmed .bit>, or --profile NAME for one
#           recorded in test/e2e/profiles/ (the archived bitstream:
#           --profile archived-1x8x8-3x4x4); one of the two, not both. With
#           neither the run stops before anything is sent.
#   cosim   the co-simulation's fpga_config.txt (build.sh records the RTL's
#           parameters there); an exported SYSTOLIC_FPGA_CONFIG or a
#           --profile must resolve to the same K_MAX_8X8, K_MAX_4X4 and slab.
# The file's metadata must agree with its K_MAX values: HOST_SLAB_BYTES with
# the slab, K_DIM (dma_top_build.tcl writes K_DIM = K_MAX_8X8) with
# K_MAX_8X8, BITSTREAM_SHA256 with the hash of BITSTREAM when that file is
# here (a relative BITSTREAM is relative to the configuration file); every
# key once. None of this shows what the board holds: the FPGA does not
# report its K_MAX (DPTI has no register read), so on the board the hardware
# configuration is not independently verified.
#
# Options:
#   --profile NAME    test/e2e/profiles/NAME.cfg (or a file) is the
#                     configuration
#   --check-config    the host side only: resolve and check the
#                     configuration, run the refusal checks, list what a run
#                     would send; nothing goes to the FPGA, no co-simulation
#                     starts
#   (default)         the 36 fixed cases, then the K_MAX boundary cases:
#                     12 x 12 x K for K = 1, K_MAX - 1, K_MAX, K_MAX + 1 of
#                     each capacity and 2 * max(K8, K4) + 3; 4x4xK4, 8x8xK8
#   --quick           the six required shapes x four value classes, then
#                     8x8x(K8 + 1) and 4x4x(K4 + 1)
#   --fixed-only      only the 36 fixed cases (nine shapes x four classes)
#   --kmax-cases      only the K_MAX boundary cases
#   --regression      every default and --quick case once, then larger
#                     GEMMs with mixed values: 64x64x64, 32x64x32,
#                     16x16x31, 32x32x65, 7x7x16, 15x15x16, 33x33x16
#   --extra-k K,K,..  with the boundary cases, also 12 x 12 x K for each K
#   --cases a,b,...   only these cases (names as gen_cases.py prints them)
#   --keep-going      run the cases even when the runtime layer could not
#                     reach the FPGA
#   --timing          time every case: the host wall-clock of the whole
#                     systolic-matmul run and the runtime's own per-invocation
#                     breakdown (SYSTOLIC_RUNTIME_TIMING=1 is exported to the
#                     cases; the runtime prints one `[fpga-runtime] timing`
#                     line at close, `[fpga-async] timing` with --mode async).
#                     The rows land in <out>/timing.tsv and
#                     tools/e2e_timing.py prints the table. Verdicts are not
#                     affected. Other SYSTOLIC_DPTI_* and SYSTOLIC_ASYNC_*
#                     variables (LATENCY_MS, SETTLE_US, STAGE, ...) pass
#                     through to the cases as exported.
#   --mode sync|async sync (default): every case through the synchronous
#                     runtime (systolic-matmul --sync), one
#                     fpga_matmul_tiled_auto_scheduled per tile, each waited
#                     for. async: systolic-matmul --async, the asynchronous runtime
#                     (fpga_gemm_begin / fpga_tile_submit / fpga_gemm_wait,
#                     docs/ASYNC.md); the runtime layer step also runs
#                     runtime-async-test, and the configuration must say
#                     COMPLETION_HEADER=1 (a bitstream with the completion
#                     record). The verdict checks that the lowered IR calls
#                     the runtime of the chosen mode.
#   --build-dir DIR   the CMake build directory (default: build)
#   --cosim-dir DIR   the co-simulation to run (default: BUILD/cosim)
#   --out DIR         where the cases and the summary go (default:
#                     BUILD/e2e/<backend>, with --check-config
#                     BUILD/e2e/check-config-<backend>); it is emptied
#                     first, so an existing DIR must be one this script made
#
# Steps: 0. malformed configurations must be refused by systolic-matmul
# before the link opens; 1. runtime-dispatch-test, the runtime layer on its
# own (k = 1, K_MAX - 1, K_MAX on each geometry, K_MAX + 1 refused);
# 2. every case through systolic-matmul, each classified by e2e_verdict.py:
# PASS, MISMATCH (numerical), CONFIG, TRANSPORT, SUBMISSION, INPUT, COMPILE,
# FAIL, with the raw exit status and runtime return code -- or NOT RUN (K
# beyond systolic-matmul's input limit; the runtime layer could not reach
# the FPGA); 3. co-simulation only: an injected write failure must be
# reported as a transport failure. Each case leaves
# <out>/<case>/{input.txt,output.txt,ir/}; the summary is <out>/summary.txt.
# Exit status: 0 everything that ran passed; 2 the configuration was refused
# (nothing was sent); 1 otherwise.
set -uo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/../.." && pwd)"

usage() { awk 'NR > 2 && !/^#/ { exit } NR > 2 { sub(/^# ?/, ""); print }' "$0"; }
dir_arg() { (cd "$1" 2>/dev/null && pwd) || { echo "$2: no directory $1" >&2; exit 2; }; }

BACKEND=board
BUILD="$ROOT/build"
COSIM_DIR=""
OUT=""
MODE=full
MODES=0
EXTRA_K=""
ONLY=""
PROFILE=""
CHECK_ONLY=0
KEEP_GOING=0
TIMING=0
RTMODE=sync
while [ $# -gt 0 ]; do
    case "$1" in
        --backend)      BACKEND="$2"; shift 2 ;;
        --build-dir)    BUILD="$(dir_arg "$2" --build-dir)" || exit 2; shift 2 ;;
        --cosim-dir)    COSIM_DIR="$(dir_arg "$2" --cosim-dir)" || exit 2; shift 2 ;;
        --out)          OUT="$2"; shift 2 ;;
        --profile)      PROFILE="$2"; shift 2 ;;
        --check-config) CHECK_ONLY=1; shift ;;
        --quick)        MODE=quick; MODES=$((MODES + 1)); shift ;;
        --fixed-only)   MODE=fixed; MODES=$((MODES + 1)); shift ;;
        --kmax-cases)   MODE=kmax; MODES=$((MODES + 1)); shift ;;
        --regression)   MODE=regression; MODES=$((MODES + 1)); shift ;;
        --extra-k)      EXTRA_K="$2"; shift 2 ;;
        --cases)        ONLY="$2"; shift 2 ;;
        --keep-going)   KEEP_GOING=1; shift ;;
        --timing)       TIMING=1; shift ;;
        --mode)         RTMODE="$2"; shift 2 ;;
        -h|--help)      usage; exit 0 ;;
        *) echo "unknown argument: $1" >&2; exit 2 ;;
    esac
done
case "$BACKEND" in board|cosim) ;; *) echo "--backend board|cosim" >&2; exit 2 ;; esac
case "$RTMODE" in sync|async) ;; *) echo "--mode sync|async" >&2; exit 2 ;; esac
[ "$MODES" -le 1 ] || { echo "--quick, --fixed-only, --kmax-cases and --regression exclude each other" >&2; exit 2; }
if [ -n "$EXTRA_K" ] && { [ "$MODE" = quick ] || [ "$MODE" = fixed ] ||
                          ! [[ "$EXTRA_K" =~ ^[1-9][0-9]*(,[1-9][0-9]*)*$ ]]; }; then
    echo "--extra-k takes a comma-separated list of positive integers and goes with the K_MAX boundary cases (the default, --kmax-cases or --regression)" >&2
    exit 2
fi

MATMUL="$BUILD/bin/systolic-matmul"
RTTEST="$BUILD/bin/runtime-dispatch-test"
RTASYNC="$BUILD/bin/runtime-async-test"
VERDICT="$HERE/e2e_verdict.py"
# systolic-matmul chooses by itself (batch submission wherever the bitstream
# has the completion record); each mode asks for its runtime explicitly, so
# the sync mode keeps exercising the synchronous path on such a bitstream.
MATMUL_FLAGS=(--sync)
[ "$RTMODE" = async ] && MATMUL_FLAGS=(--async)
for b in "$MATMUL" "$RTTEST"; do
    [ -x "$b" ] || { echo "missing $b -- build first (cmake --build $BUILD)" >&2; exit 2; }
done
grep -q -- '--sync' "$MATMUL" || { echo "$MATMUL predates --sync -- rebuild it (cmake --build $BUILD)" >&2; exit 2; }
if [ "$RTMODE" = async ]; then
    [ -x "$RTASYNC" ] || { echo "missing $RTASYNC -- build first (cmake --build $BUILD)" >&2; exit 2; }
    grep -q -- '--async' "$MATMUL" || { echo "$MATMUL predates --async -- rebuild it (cmake --build $BUILD)" >&2; exit 2; }
fi
# An older runtime-dispatch-test has no --print-config and would run its
# cases -- open the link -- when asked for the configuration.
grep -q -- '--print-config' "$RTTEST" || {
    echo "$RTTEST predates --print-config -- rebuild it (cmake --build $BUILD) before running this script" >&2; exit 2; }
[ -n "$COSIM_DIR" ] || COSIM_DIR="$BUILD/cosim"

# ---- the configuration -------------------------------------------------------
# Every check below is built from it, and none of them may run against a
# configuration that is missing or inconsistent: stop before anything is
# sent, and before the previous results in --out are emptied.
fail_config() {
    echo "configuration failure: $*" >&2
    echo "E2E: FAIL -- CONFIGURATION (refused before anything was sent to the FPGA)"
    exit 2
}

abspath() { (cd "$(dirname "$1")" 2>/dev/null && printf '%s/%s\n' "$(pwd)" "$(basename "$1")"); }

profiles() { (cd "$HERE/profiles" 2>/dev/null && ls -- *.cfg 2>/dev/null | sed 's/\.cfg$//' | paste -s -d ' ' -); }

# K_MAX_8X8, K_MAX_4X4 and the slab of a configuration file, the way the
# runtime reads it: sets R8 R4 RSLAB, or RERR and returns 1.
resolve() {
    local out rc
    R8=""; R4=""; RSLAB=""; RERR=""
    out=$(SYSTOLIC_FPGA_CONFIG="$1" "$RTTEST" --print-config 2>&1)
    rc=$?
    if [ $rc -ne 0 ]; then
        RERR=$(sed -n 's/^CONFIG_ERROR=//p' <<< "$out" | head -1)
        [ -n "$RERR" ] || RERR="runtime-dispatch-test --print-config exited $rc: $(head -1 <<< "$out")"
        return 1
    fi
    R8=$(sed -n 's/^K_MAX_8X8=//p' <<< "$out")
    R4=$(sed -n 's/^K_MAX_4X4=//p' <<< "$out")
    RSLAB=$(sed -n 's/^SLAB_BYTES=//p' <<< "$out")
    for v in "$R8" "$R4" "$RSLAB"; do
        [[ "$v" =~ ^[1-9][0-9]*$ ]] || { RERR="unexpected runtime-dispatch-test --print-config output: $out"; return 1; }
    done
}

# Every value of KEY in a configuration file, one per line, read the way
# the runtime reads the file: '#' starts a comment, the line is KEY=VALUE,
# blanks around KEY and VALUE do not count.
cfg_values() {
    awk -v key="$2" '{ sub(/#.*/, ""); i = index($0, "="); if (i == 0) next
        k = substr($0, 1, i - 1); v = substr($0, i + 1)
        gsub(/^[ \t\r]+|[ \t\r]+$/, "", k); gsub(/^[ \t\r]+|[ \t\r]+$/, "", v)
        if (k == key) print v }' "$1"
}
cfg_count() { cfg_values "$1" "$2" | awk 'END { print NR }'; }

sha256_of() {
    if command -v sha256sum > /dev/null 2>&1; then sha256sum "$1" | cut -d' ' -f1
    elif command -v shasum > /dev/null 2>&1; then shasum -a 256 "$1" | cut -d' ' -f1
    fi
}

GIVEN=""
HOW=""
if [ -n "$PROFILE" ] && [ -n "${SYSTOLIC_FPGA_CONFIG:-}" ]; then
    fail_config "both SYSTOLIC_FPGA_CONFIG=$SYSTOLIC_FPGA_CONFIG and --profile $PROFILE name a configuration; give one"
fi
if [ -n "$PROFILE" ]; then
    case "$PROFILE" in
        */*|*.cfg|*.txt) p="$PROFILE" ;;
        *)               p="$HERE/profiles/$PROFILE.cfg" ;;
    esac
    [ -f "$p" ] || fail_config "no profile '$PROFILE' (test/e2e/profiles/: $(profiles))"
    GIVEN="$(abspath "$p")"
    HOW="--profile $PROFILE"
elif [ -n "${SYSTOLIC_FPGA_CONFIG:-}" ]; then
    GIVEN="$(abspath "$SYSTOLIC_FPGA_CONFIG")"
    [ -n "$GIVEN" ] || GIVEN="$SYSTOLIC_FPGA_CONFIG"   # the parser says why it cannot open it
    HOW="SYSTOLIC_FPGA_CONFIG"
fi

SIMCFG=""
if [ "$BACKEND" = cosim ]; then
    if [ -f "$COSIM_DIR/fpga_config.txt" ]; then
        # The simulated RTL was built with fixed K_MAX values; a runtime told
        # anything else would send slabs the RTL refuses (err_mem_length).
        SIMCFG="$(abspath "$COSIM_DIR/fpga_config.txt")"
        resolve "$SIMCFG" || fail_config "the co-simulation's record: $RERR"
        S8=$R8; S4=$R4; SS=$RSLAB
        if [ -n "$GIVEN" ]; then
            resolve "$GIVEN" || fail_config "$RERR"
            [ "$R8 $R4 $RSLAB" = "$S8 $S4 $SS" ] || fail_config "$HOW ($GIVEN) says K_MAX_8X8 = $R8, K_MAX_4X4 = $R4 ($RSLAB-byte slab), but the co-simulation in $COSIM_DIR was built with K_MAX_8X8 = $S8, K_MAX_4X4 = $S4 ($SS-byte slab); drop it or name $SIMCFG"
        else
            GIVEN="$SIMCFG"
            HOW="the co-simulation's build record"
        fi
    elif [ -z "$GIVEN" ]; then
        fail_config "the co-simulation in $COSIM_DIR has no fpga_config.txt (test/e2e/cosim/build.sh writes it), and no configuration was named: rebuild it, or name the K_MAX values it was built with (SYSTOLIC_FPGA_CONFIG or --profile)"
    fi
elif [ -z "$GIVEN" ]; then
    fail_config "the board backend needs the configuration of the bitstream on the FPGA: export SYSTOLIC_FPGA_CONFIG=<its fpga_config.txt> (dma_top_build.tcl writes it next to the .bit), or --profile NAME for a recorded one (test/e2e/profiles/: $(profiles)). There is no default: the FPGA does not report its K_MAX, and slabs of the wrong length are refused by the bitstream."
fi
resolve "$GIVEN" || fail_config "$RERR"
K8=$R8; K4=$R4; SLAB=$RSLAB
export SYSTOLIC_FPGA_CONFIG="$GIVEN"

# The metadata in the same file, against the K_MAX values.
for key in K_MAX_8X8 K_MAX_4X4 N NUM_8X8 NUM_4X4 K_DIM HOST_SLAB_BYTES BITSTREAM BITSTREAM_SHA256; do
    n=$(cfg_count "$GIVEN" "$key")
    [ "$n" -le 1 ] || fail_config "$GIVEN: $key is given $n times (the runtime would take the last one); a configuration names each value once"
done
META=()
v=$(cfg_values "$GIVEN" HOST_SLAB_BYTES)
if [ -n "$v" ]; then
    [ "$v" = "$SLAB" ] || fail_config "$GIVEN: HOST_SLAB_BYTES = $v, but K_MAX_8X8 = $K8 and K_MAX_4X4 = $K4 imply a $SLAB-byte slab (the larger of ceil(K_MAX / 8) x 512 B): the K_MAX values do not describe the bitstream these metadata belong to"
    META+=("HOST_SLAB_BYTES = $v: agrees with the K_MAX values")
else
    META+=("HOST_SLAB_BYTES: not recorded")
fi
v=$(cfg_values "$GIVEN" K_DIM)
if [ -n "$v" ]; then
    [ "$v" = "$K8" ] || fail_config "$GIVEN: K_DIM = $v, but K_MAX_8X8 = $K8 (dma_top_build.tcl writes K_DIM = K_MAX_8X8): the file has been edited or belongs to another build"
    META+=("K_DIM = $v: agrees with K_MAX_8X8")
fi
BIT=$(cfg_values "$GIVEN" BITSTREAM)
BITPATH=""
if [ -n "$BIT" ]; then
    case "$BIT" in /*) BITPATH="$BIT" ;; *) BITPATH="$(dirname "$GIVEN")/$BIT" ;; esac
    BITPATH="$(abspath "$BITPATH" || true)"
    [ -n "$BITPATH" ] || case "$BIT" in /*) BITPATH="$BIT" ;; *) BITPATH="$(dirname "$GIVEN")/$BIT" ;; esac
fi
SHA=$(cfg_values "$GIVEN" BITSTREAM_SHA256 | tr 'A-F' 'a-f')
if [ -n "$SHA" ] && [ "$SHA" != unknown ]; then
    [[ "$SHA" =~ ^[0-9a-f]{64}$ ]] || fail_config "$GIVEN: BITSTREAM_SHA256 = '$SHA' is not a sha256"
    if [ -n "$BITPATH" ] && [ -f "$BITPATH" ]; then
        got=$(sha256_of "$BITPATH")
        if [ -z "$got" ]; then
            META+=("BITSTREAM_SHA256 = $SHA: not checked (no sha256sum or shasum here)")
        elif [ "$got" != "$SHA" ]; then
            fail_config "$GIVEN: BITSTREAM_SHA256 = $SHA, but $BITPATH hashes to $got: the configuration is not that bitstream's"
        else
            META+=("BITSTREAM_SHA256 = $SHA: agrees with $BITPATH")
        fi
    else
        META+=("BITSTREAM_SHA256 = $SHA: not checked (BITSTREAM ${BIT:-not recorded}${BIT:+ is not here})")
    fi
else
    META+=("BITSTREAM_SHA256: not recorded${BIT:+ (BITSTREAM = $BIT)}")
fi
# COMPLETION_HEADER=1: every result the bitstream sends starts with the
# 16-byte completion record (docs/ASYNC.md phase 2); the verdict counts the
# bytes back accordingly, and --mode async needs it.
HEADER=$(cfg_values "$GIVEN" COMPLETION_HEADER | tail -1)
case "$HEADER" in
    1) META+=("COMPLETION_HEADER = 1: results carry the completion record") ;;
    ""|0) META+=("COMPLETION_HEADER: ${HEADER:-not recorded} (bare result tiles)"); HEADER=0 ;;
    *) fail_config "$GIVEN: COMPLETION_HEADER = '$HEADER' is not 0 or 1" ;;
esac
[ "$RTMODE" = async ] && [ "$HEADER" != 1 ] &&
    fail_config "--mode async needs a bitstream with the completion record, and $GIVEN does not say COMPLETION_HEADER=1"
# PIPELINE_DEPTH: the context's job queue, the asynchronous runtime's
# descriptor credit per array (docs/ASYNC.md section 9); 1 when not recorded.
DEPTH=$(cfg_values "$GIVEN" PIPELINE_DEPTH | tail -1)
[[ "$DEPTH" =~ ^[1-9][0-9]*$ ]] || DEPTH=1
rest=""
for key in VARIANT USE_V2 RTL_GIT_REV RTL_TREE_DIRTY RTL_SOURCE TIMING_MET WNS_NS WHS_NS BUILT; do
    v=$(cfg_values "$GIVEN" "$key" | tail -1)
    [ -n "$v" ] && rest="$rest${rest:+, }$key=$v"
done
[ -n "$rest" ] && META+=("$rest")
[ "$(cfg_values "$GIVEN" TIMING_MET | tail -1)" = 0 ] &&
    META+=("warning: TIMING_MET=0 -- the implementation did not meet timing")

if [ "$BACKEND" = board ]; then
    HW="Hardware configuration not independently verified: the FPGA does not report K_MAX_8X8 / K_MAX_4X4 (DPTI has no register read), so this run trusts that the board holds the bitstream described above"
elif [ -n "$SIMCFG" ]; then
    HW="the co-simulation in $COSIM_DIR, built with these K_MAX values (its fpga_config.txt, written by test/e2e/cosim/build.sh)"
else
    HW="the co-simulation in $COSIM_DIR has no build record: its K_MAX values are the named configuration's, not verified against the simulated RTL"
fi

# systolic-matmul's input limit on K; deeper cases are reported, not run.
KLIM=$("$MATMUL" --help 2>/dev/null | sed -nE 's/.*K <= ([0-9]+).*/\1/p' | head -1)

# ---- the output directory ------------------------------------------------------
if [ -z "$OUT" ]; then
    [ "$CHECK_ONLY" = 1 ] && OUT="$BUILD/e2e/check-config-$BACKEND" || OUT="$BUILD/e2e/$BACKEND"
fi
# OUT is emptied first. A directory given with --out is only emptied if this
# script made it (its marker is there) or it is empty: --out pointed at a
# source or build tree by mistake must not wipe it.
if [ -d "$OUT" ] && [ -n "$(ls -A "$OUT" 2>/dev/null)" ] &&
   [ "$OUT" != "$BUILD/e2e/$BACKEND" ] && [ "$OUT" != "$BUILD/e2e/check-config-$BACKEND" ] &&
   [ ! -f "$OUT/.run_e2e_out" ]; then
    echo "--out $OUT is not empty and was not made by run_e2e.sh; not deleting it" >&2
    exit 2
fi
rm -rf "$OUT"
mkdir -p "$OUT/cases"
OUT="$(cd "$OUT" && pwd)"
: > "$OUT/.run_e2e_out"

{
    echo "configuration"
    echo "  host       K_MAX_8X8 = $K8, K_MAX_4X4 = $K4, $SLAB-byte operand slab"
    echo "             (the runtime's parser; SYSTOLIC_FPGA_CONFIG=$GIVEN, from $HOW)"
    echo "  runtime    $RTMODE$([ "$RTMODE" = async ] && echo ' (systolic-matmul --async: fpga_gemm_begin / fpga_tile_submit / fpga_gemm_wait)' || echo ' (systolic-matmul --sync: one fpga_matmul_tiled_auto_scheduled per tile)')"
    echo "  metadata   (the same file)"
    for m in ${META[@]+"${META[@]}"}; do echo "             $m"; done
    echo "  hardware   $HW"
    echo
} | tee "$OUT/configuration.txt"

# ---- the cases ----------------------------------------------------------------
GEN=(python3 "$HERE/gen_cases.py" "$OUT/cases")
case "$MODE" in
    quick) GEN+=(--quick "$K8" "$K4") ;;
    kmax)  GEN+=(--kmax "$K8" "$K4") ;;
    full)  GEN+=(--full "$K8" "$K4") ;;
    regression) GEN+=(--regression "$K8" "$K4") ;;
esac
[ -n "$EXTRA_K" ] && GEN+=("$EXTRA_K")
if [ -n "$KLIM" ]; then
    GEN+=(--max-k "$KLIM")
else
    echo "note: systolic-matmul --help names no K limit; every case is run"
fi
"${GEN[@]}" > "$OUT/cases/generated.txt" || { echo "gen_cases.py failed" >&2; exit 2; }
: > "$OUT/case_list.txt"
: > "$OUT/not_run.txt"
[ "$TIMING" = 1 ] && printf 'case\texit\twall_us\truntime_timing\n' > "$OUT/timing.tsv"
while read -r name; do
    [ -n "$ONLY" ] && case ",$ONLY," in *",$name,"*) ;; *) continue ;; esac
    echo "$name" >> "$OUT/case_list.txt"
done < "$OUT/cases/generated.txt"
if [ -f "$OUT/cases/not_run.txt" ]; then
    while read -r name why; do
        [ -n "$ONLY" ] && case ",$ONLY," in *",$name,"*) ;; *) continue ;; esac
        echo "$name $why: beyond systolic-matmul's input limit" >> "$OUT/not_run.txt"
    done < "$OUT/cases/not_run.txt"
fi
if [ -n "$ONLY" ]; then
    for name in $(tr ',' ' ' <<< "$ONLY"); do
        grep -qx -- "$name" "$OUT/case_list.txt" || grep -q -- "^$name " "$OUT/not_run.txt" ||
            { echo "--cases: no case $name in this run's set" >&2; exit 2; }
    done
fi

COSIM_PID=""
cleanup() { [ -n "$COSIM_PID" ] && kill "$COSIM_PID" 2>/dev/null; }
trap cleanup EXIT

if [ "$CHECK_ONLY" = 1 ]; then
    echo "backend: $BACKEND -- check only: nothing is sent to the FPGA"
elif [ "$BACKEND" = cosim ]; then
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
    echo "         bitstream: ${BIT:-not recorded in the configuration}"
    echo "         Hardware configuration not independently verified (see above)"
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
# session -> "received consumed sent": the bytes the simulated FT2232H got
# from the host, the bytes the RTL took from it (RD# strobes), and the bytes
# the RTL sent back (WR# strobes).
cosim_bytes() {
    grep -m1 "^cosim: session $1 closed" "$OUT/cosim.log" 2>/dev/null \
        | sed -E 's/.*host->fpga ([0-9]+) B received, ([0-9]+) B consumed by the RTL, fpga->host ([0-9]+) B.*/\1 \2 \3/'
}
LIVE_SIM=0
[ "$BACKEND" = cosim ] && [ "$CHECK_ONLY" = 0 ] && LIVE_SIM=1

# ---- 0. a malformed configuration is refused before the FPGA is touched ---
# systolic-matmul reads SYSTOLIC_FPGA_CONFIG first and must exit 4 with the
# runtime's reason. On the running co-simulation the problem is a valid one
# and no session may open; everywhere else it is one systolic-matmul would
# refuse (M = 0) right after the configuration, so even a configuration that
# was wrongly accepted cannot reach a board.
echo "== configuration refusal: malformed SYSTOLIC_FPGA_CONFIG files"
cfg_ok=1
CFGDIR="$OUT/config_refusal"
mkdir -p "$CFGDIR"
if [ "$LIVE_SIM" = 1 ]; then
    PROBLEM='1 1 1\n1\n1\n'
    cfg_sessions_before=$(sessions_opened)
else
    PROBLEM='0 0 0\n'
fi
while IFS='|' read -r tag body want; do
    if [ "$tag" = no_file ]; then
        rm -f "$CFGDIR/$tag.txt"
    else
        printf "%b" "$body" > "$CFGDIR/$tag.txt"
    fi
    out=$(printf "%b" "$PROBLEM" | SYSTOLIC_FPGA_CONFIG="$CFGDIR/$tag.txt" "$MATMUL" 2>&1)
    rc=$?
    if [ $rc -eq 4 ] && grep -qF -- "$want" <<< "$out"; then
        v=PASS
    else
        v=FAIL; cfg_ok=0
    fi
    printf "   %-16s %s  (exit %s: %s)\n" "$tag" "$v" "$rc" \
        "$(head -1 <<< "$out" | sed -E 's/^systolic-matmul: SYSTOLIC_FPGA_CONFIG=[^:]*: //' | cut -c1-100)"
done <<'EOF'
leading_zero|K_MAX_8X8=016\nK_MAX_4X4=16\n|K_MAX_8X8 = '016' is not a decimal integer in 1..16376
zero|K_MAX_8X8=16\nK_MAX_4X4=0\n|K_MAX_4X4 = '0' is not a decimal integer in 1..16376
too_deep|K_MAX_8X8=16377\nK_MAX_4X4=16\n|K_MAX_8X8 = '16377' is not a decimal integer in 1..16376
not_a_number|K_MAX_8X8=16\nK_MAX_4X4=7.5\n|K_MAX_4X4 = '7.5' is not a decimal integer in 1..16376
missing_4x4|K_MAX_8X8=16\n|K_MAX_8X8 and K_MAX_4X4 are both required
other_fleet|K_MAX_8X8=16\nK_MAX_4X4=16\nNUM_4X4=2\n|this runtime drives the 1x8x8 + 3x4x4 fleet
no_file||cannot open
missing_8x8|K_MAX_4X4=12\n|K_MAX_8X8 and K_MAX_4X4 are both required
signed_8x8|K_MAX_8X8=+24\nK_MAX_4X4=12\n|K_MAX_8X8 = '+24' is not a decimal integer in 1..16376
empty_8x8|K_MAX_8X8=\nK_MAX_4X4=12\n|K_MAX_8X8 = '' is not a decimal integer in 1..16376
negative_4x4|K_MAX_8X8=24\nK_MAX_4X4=-12\n|K_MAX_4X4 = '-12' is not a decimal integer in 1..16376
hex_4x4|K_MAX_8X8=24\nK_MAX_4X4=0x10\n|K_MAX_4X4 = '0x10' is not a decimal integer in 1..16376
other_8x8_count|K_MAX_8X8=24\nK_MAX_4X4=12\nNUM_8X8=2\n|this runtime drives the 1x8x8 + 3x4x4 fleet
not_key_value|K_MAX_8X8=24\nK_MAX_4X4 12\n|line 2: expected KEY=VALUE
empty_file||K_MAX_8X8 and K_MAX_4X4 are both required
EOF
if [ "$LIVE_SIM" = 1 ] && [ "$(sessions_opened)" != "$cfg_sessions_before" ]; then
    echo "   FAIL: a refused configuration opened the FPGA link"; cfg_ok=0
fi
[ "$cfg_ok" = 1 ] && cfg=PASS || cfg=FAIL
echo "   configuration refusal: $cfg"
echo

if [ "$CHECK_ONLY" = 1 ]; then
    echo "== runtime layer: the invocations runtime-dispatch-test would send"
    "$RTTEST" --list > "$OUT/runtime_dispatch_plan.txt" 2>&1
    plan_rc=$?
    sed 's/^/   /' "$OUT/runtime_dispatch_plan.txt"
    echo
    if [ "$RTMODE" = async ]; then
        echo "== runtime layer: the modes runtime-async-test would run"
        "$RTASYNC" --list > "$OUT/runtime_async_plan.txt" 2>&1 || plan_rc=$?
        sed 's/^/   /' "$OUT/runtime_async_plan.txt"
        echo
    fi
    echo "== E2E cases this run would send ($(wc -l < "$OUT/case_list.txt" | tr -d ' ') case(s); $MODE)"
    tr '\n' ' ' < "$OUT/case_list.txt" | fold -s -w 96 | sed 's/^/   /'
    echo
    if [ -s "$OUT/not_run.txt" ]; then
        echo "   NOT RUN:"
        sed 's/^/     /' "$OUT/not_run.txt"
    fi
    echo
    if [ "$cfg_ok" = 1 ] && [ $plan_rc -eq 0 ]; then
        echo "check-config: PASS -- K_MAX_8X8 = $K8, K_MAX_4X4 = $K4, $SLAB-byte slab; nothing was sent to the FPGA"
        exit 0
    fi
    echo "check-config: FAIL -- configuration refusal $cfg, runtime-dispatch-test --list exit $plan_rc; nothing was sent to the FPGA"
    exit 1
fi

# ---- 1. the runtime layer on its own ---------------------------------------
echo "== runtime layer: host -> runtime -> DPTI -> FPGA (runtime-dispatch-test)"
"$RTTEST" > "$OUT/runtime_dispatch.txt" 2>&1
rt_rc=$?
[ "$LIVE_SIM" = 1 ] && settle_sessions
sed 's/^/   /' "$OUT/runtime_dispatch.txt" | grep -v libftdi_cosim
case $rt_rc in
    0) rt_class=PASS ;; 1) rt_class=MISMATCH ;; 2) rt_class=CONFIG ;;
    3) rt_class=TRANSPORT ;; 4) rt_class=SUBMISSION ;; *) rt_class=FAIL ;;
esac
# The configuration it ran with must be the one resolved above.
rt_seen=$(sed -nE 's/^configuration: K_MAX_8X8 = ([0-9]+), K_MAX_4X4 = ([0-9]+), ([0-9]+)-byte operand slab \((.*)\)$/\1 \2 \3 \4/p' \
              "$OUT/runtime_dispatch.txt" | head -1)
if [ $rt_rc -ne 2 ] && [ "$rt_seen" != "$K8 $K4 $SLAB $GIVEN" ]; then
    rt_class=CONFIG
    echo "   propagation: runtime-dispatch-test ran with '${rt_seen:-no configuration line}', not '$K8 $K4 $SLAB $GIVEN'"
fi
if [ "$rt_class" = PASS ]; then
    rt=PASS
else
    rt="FAIL -- $rt_class (exit $rt_rc): $(grep -E '^RUNTIME DISPATCH:' "$OUT/runtime_dispatch.txt" | tail -1 | sed -E 's/^RUNTIME DISPATCH: (FAIL -- )?//')"
fi
echo

# ---- 1b. the asynchronous runtime on its own (--mode async) -----------------
# runtime-async-test: one device, every device without and with the slab
# staged behind the running job, two GEMMs interleaved, the fold order
# checked bit for bit against the synchronous entry point, and the
# refusals; a 16x16x32 GEMM (20 tiles) unless SYSTOLIC_E2E_ASYNC_GEMM says
# otherwise. Its verdict joins the dispatch test's in "runtime layer".
if [ "$RTMODE" = async ] && [ "$rt_class" != TRANSPORT ] && [ "$rt_class" != CONFIG ]; then
    echo "== runtime layer: the asynchronous runtime (runtime-async-test --mode all)"
    "$RTASYNC" --mode all --gemm "${SYSTOLIC_E2E_ASYNC_GEMM:-16x16x32}" > "$OUT/runtime_async.txt" 2>&1
    ra_rc=$?
    [ "$LIVE_SIM" = 1 ] && settle_sessions
    sed 's/^/   /' "$OUT/runtime_async.txt" | grep -v -E 'libftdi_cosim|^   \[fpga-(async|runtime)\] (timing|submit|stage|descriptor|completion|accumulate|hold|gemm|session)'
    case $ra_rc in
        0) ra_class=PASS ;; 1) ra_class=MISMATCH ;; 2) ra_class=CONFIG ;;
        3) ra_class=TRANSPORT ;; 4) ra_class=SUBMISSION ;; *) ra_class=FAIL ;;
    esac
    if [ "$ra_class" != PASS ]; then
        rt_class=$ra_class
        rt="FAIL -- runtime-async-test $ra_class (exit $ra_rc): $(grep -E '^RUNTIME ASYNC:' "$OUT/runtime_async.txt" | tail -1 | sed -E 's/^RUNTIME ASYNC: (FAIL -- )?//')"
    else
        rt="PASS (runtime-dispatch-test and runtime-async-test)"
    fi
    echo
fi

# ---- 2. every case through the whole pipeline ------------------------------
SKIP_ALL=""
if [ "$KEEP_GOING" = 0 ] && { [ "$rt_class" = TRANSPORT ] || [ "$rt_class" = CONFIG ]; }; then
    SKIP_ALL="the runtime layer failed ($rt_class) -- --keep-going runs the cases anyway"
fi
pass=0; total=0
SEEN=""          # one "CLASS name" line per case that ran
: > "$OUT/coverage.txt"
row() { printf "%-22s %-10s %4s %5s %6s %6s %9s %9s %s\n" "$@"; }
row CASE RESULT EXIT RT_RC TILES FPGA BYTES_TO BYTES_BK NOTE | tee "$OUT/summary.txt"
while read -r name; do
    if [ -n "$SKIP_ALL" ]; then
        echo "$name $SKIP_ALL" >> "$OUT/not_run.txt"
        continue
    fi
    d="$OUT/$name"
    mkdir -p "$d/ir"
    cp "$OUT/cases/$name.txt" "$d/input.txt"
    [ "$LIVE_SIM" = 1 ] && before=$(sessions_opened)
    if [ "$TIMING" = 1 ]; then
        t0=$(date +%s%N)
        SYSTOLIC_RUNTIME_TIMING=1 "$MATMUL" ${MATMUL_FLAGS[@]+"${MATMUL_FLAGS[@]}"} --dump-ir="$d/ir" < "$d/input.txt" > "$d/output.txt" 2>&1
        rc=$?
        t1=$(date +%s%N)
        # case, exit, wall-clock of the whole run (compile + JIT + FPGA), the
        # runtime's own line (empty when the run never reached the close)
        printf '%s\t%s\t%s\t%s\n' "$name" "$rc" "$(( (t1 - t0) / 1000 ))" \
            "$(grep -m1 -E '^\[fpga-(runtime|async)\] timing ' "$d/output.txt" | sed -E 's/^\[fpga-(runtime|async)\] timing /\1 /')" \
            >> "$OUT/timing.tsv"
    else
        "$MATMUL" ${MATMUL_FLAGS[@]+"${MATMUL_FLAGS[@]}"} --dump-ir="$d/ir" < "$d/input.txt" > "$d/output.txt" 2>&1
        rc=$?
    fi
    total=$((total + 1))

    sim=()
    if [ "$LIVE_SIM" = 1 ]; then
        # Every byte the runtime counted out must have been consumed by the
        # RTL, and every byte it counted in must have been sent by the RTL.
        if settle_sessions && [ "$(sessions_opened)" -eq $((before + 1)) ]; then
            read -r crecv ccons csent < <(cosim_bytes "$((before + 1))")
            sim=(--cosim "${crecv:-0}" "${ccons:-0}" "${csent:-0}" "$((before + 1))")
        else
            sim=(--cosim-error "expected exactly one co-simulation session")
        fi
    fi
    IFS=$'\t' read -r cls rtrc tiles inv to back cov note < <(
        python3 "$VERDICT" "$d" --exit "$rc" --k8 "$K8" --k4 "$K4" --slab "$SLAB" \
            --source "$GIVEN" --mode "$RTMODE" --header "$HEADER" ${sim[@]+"${sim[@]}"})
    [ -n "${cls:-}" ] || { cls=FAIL; rtrc=-; tiles=-; inv=-; to=-; back=-; cov=-; note="e2e_verdict.py gave no verdict"; }
    SEEN="$SEEN$cls $name
"
    if [ "$cls" = PASS ]; then
        pass=$((pass + 1))
        printf '%s\t%s\n' "$name" "$cov" >> "$OUT/coverage.txt"
    fi
    row "$name" "$cls" "$rc" "$rtrc" "$tiles" "$inv" "$to" "$back" "$note" | tee -a "$OUT/summary.txt"
done < "$OUT/case_list.txt"
while read -r name why; do
    [ -n "$name" ] && row "$name" NOT_RUN - - - - - - "$why" | tee -a "$OUT/summary.txt"
done < "$OUT/not_run.txt"
not_run=$(awk 'END { print NR }' "$OUT/not_run.txt")

echo | tee -a "$OUT/summary.txt"

# ---- 3. a broken link is reported, not computed around (co-simulation) ---
# The co-simulation's libftdi fails write #7 of the session. Synchronous
# runtime: the 5th WRITE32 of the first of five descriptors (16x8x4) -- the
# MEM_WRITE (9 B and the slab) and four WRITE32s (4 x 6 B) are all that may
# reach the RTL -- no submit, so no result -- and systolic-matmul must exit
# 4 with the runtime's -6, a transport failure. Asynchronous runtime (burst
# submission, docs/ASYNC.md section 9: every slab a device has room for
# goes out before its descriptors, each MEM_WRITE as two writes, the 9-byte
# header and the slab): write #7 is the fourth slab's header, so three slabs
# reach the RTL and the runtime's -4 is reported; that order holds while
# the arrays have room for four slabs each (PIPELINE_DEPTH >= 3), else the
# check is skipped. 16x8x4 is five invocations as long as every K_MAX >= 4;
# with a shallower bitstream the count differs and the check is skipped.
df_ok=1
if [ "$RTMODE" = async ]; then
    DF_BYTES=$(( 3 * (9 + SLAB) )); DF_RC=-4
    DF_WHAT="the fourth slab's MEM_WRITE header (burst submission: slabs first)"
else
    DF_BYTES=$(( 9 + SLAB + 4 * 6 )); DF_RC=-6
    DF_WHAT="the 5th WRITE32 of the first descriptor"
fi
if [ "$BACKEND" = cosim ] && [ -n "$SKIP_ALL" ]; then
    echo "dispatch failure reported: NOT RUN ($SKIP_ALL)" | tee -a "$OUT/summary.txt"
elif [ "$BACKEND" = cosim ] && [ "$RTMODE" = async ] && [ "$DEPTH" -lt 3 ]; then
    echo "dispatch failure reported: NOT TESTED (PIPELINE_DEPTH = $DEPTH: the asynchronous runtime's write order differs)" \
        | tee -a "$OUT/summary.txt"
elif [ "$BACKEND" = cosim ] && [ "$K8" -ge 4 ] && [ "$K4" -ge 4 ]; then
    echo "== dispatch failure: write #7 of the session fails ($DF_WHAT)"
    d="$OUT/dispatch_failure"
    mkdir -p "$d"
    python3 "$HERE/gen_cases.py" "$d/gen" > /dev/null
    cp "$d/gen/16x8x4_mixed.txt" "$d/input.txt"
    before=$(sessions_opened)
    SYSTOLIC_DPTI_COSIM_FAIL_WRITE=7 "$MATMUL" ${MATMUL_FLAGS[@]+"${MATMUL_FLAGS[@]}"} < "$d/input.txt" > "$d/output.txt" 2>&1
    frc=$?
    settle_sessions
    read -r frecv fcons fsent < <(cosim_bytes "$((before + 1))")
    sed -n -E 's/^ *(DPTI: .*)/   runtime: \1/p; s/^(FPGA dispatch: .*)/   \1/p' "$d/output.txt"
    echo "   exit $frc; RTL received ${frecv:--} B, consumed ${fcons:--} B, sent ${fsent:--} B"
    if [ $frc -eq 4 ] &&
       grep -q "^FPGA dispatch: FAIL -- 5 of 5 runtime call(s) failed (first error $DF_RC)" "$d/output.txt" &&
       grep -q "DPTI: $DF_BYTES B to the FPGA, 0 B back" "$d/output.txt" &&
       [ "$frecv" = "$DF_BYTES" ] && [ "$fcons" = "$DF_BYTES" ] && [ "$fsent" = 0 ]; then
        df="PASS (TRANSPORT, rc=$DF_RC, as injected)"
    else
        df=FAIL; df_ok=0
    fi
    echo "dispatch failure reported: $df" | tee -a "$OUT/summary.txt"
elif [ "$BACKEND" = cosim ]; then
    echo "dispatch failure reported: NOT TESTED (K_MAX < 4: 16x8x4 is not five invocations)" \
        | tee -a "$OUT/summary.txt"
fi

# ---- summary ------------------------------------------------------------------
count() { printf '%s' "$SEEN" | grep -c "^$1 "; }
first() { printf '%s' "$SEEN" | grep -m1 "^$1 " | cut -d' ' -f2; }
fails=""
for c in MISMATCH CONFIG TRANSPORT SUBMISSION INPUT COMPILE FAIL; do
    n=$(count "$c")
    [ "$n" -gt 0 ] && fails="$fails${fails:+, }$c $n (first: $(first "$c"))"
done
cases_line="E2E cases: $pass/$total PASS on backend $BACKEND"
[ "$RTMODE" = async ] && cases_line="$cases_line (async runtime)"
[ -n "$fails" ] && cases_line="$cases_line; $fails"
[ "$not_run" -gt 0 ] && cases_line="$cases_line; NOT RUN $not_run (see the NOT_RUN rows)"

coverage=$(awk -F'\t' -v k8="$K8" -v k4="$K4" '
    { n = split($2, kv, ",")
      for (i = 1; i <= n; i++) { split(kv[i], p, "="); s[p[1]] += p[2] } }
    END { printf "tiles on device 0 (8x8) / 1 / 2 / 3 (4x4) = %d / %d / %d / %d; edge tiles %d; ",
                 s["d0"], s["d1"], s["d2"], s["d3"], s["edge"]
          printf "K folded on the 8x8: %s, on a 4x4: %s; k = K_MAX_8X8 (%s) run: %s, k = K_MAX_4X4 (%s) run: %s\n",
                 s["fold8"] ? "yes" : "no", s["fold4"] ? "yes" : "no",
                 k8, s["full8"] ? "yes" : "no", k4, s["full4"] ? "yes" : "no" }' "$OUT/coverage.txt")

{
    echo "configuration: K_MAX_8X8 = $K8, K_MAX_4X4 = $K4, $SLAB-byte operand slab ($GIVEN, from $HOW)"
    echo "hardware: $HW"
    echo "runtime mode: $RTMODE (COMPLETION_HEADER = $HEADER)"
    echo "configuration refusal: $cfg"
    echo "runtime layer: $rt"
    echo "$cases_line"
    [ "$pass" -gt 0 ] && echo "coverage (passing cases): $coverage"
} | tee -a "$OUT/summary.txt"

ok=1
[ "$cfg_ok" = 1 ] || ok=0
[ "$rt_class" = PASS ] || ok=0
[ "$pass" = "$total" ] && [ "$total" -gt 0 ] || ok=0
[ "$df_ok" = 1 ] || ok=0
if [ "$ok" = 1 ]; then
    verdict="E2E: PASS on backend $BACKEND (K_MAX_8X8 = $K8, K_MAX_4X4 = $K4)"
    [ "$RTMODE" = async ] && verdict="E2E: PASS on backend $BACKEND, async runtime (K_MAX_8X8 = $K8, K_MAX_4X4 = $K4)"
    [ "$BACKEND" = board ] && verdict="$verdict -- hardware configuration not independently verified"
    [ "$not_run" -gt 0 ] && verdict="$verdict; $not_run case(s) NOT RUN"
else
    why=""
    [ "$cfg_ok" = 1 ] || why="configuration refusal FAIL"
    [ "$rt_class" = PASS ] || why="$why${why:+; }runtime layer $rt_class"
    [ -n "$fails" ] && why="$why${why:+; }cases: $fails"
    [ "$total" -gt 0 ] || why="$why${why:+; }no case ran"
    [ "$df_ok" = 1 ] || why="$why${why:+; }dispatch failure not reported as injected"
    verdict="E2E: FAIL -- $why"
fi
echo "$verdict" | tee -a "$OUT/summary.txt"
if [ "$TIMING" = 1 ] && [ -s "$OUT/timing.tsv" ]; then
    echo | tee -a "$OUT/summary.txt"
    python3 "$ROOT/tools/e2e_timing.py" "$OUT" | tee -a "$OUT/summary.txt"
fi
[ "$ok" = 1 ]
