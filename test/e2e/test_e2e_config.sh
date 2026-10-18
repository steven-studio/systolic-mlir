#!/usr/bin/env bash
#
# test_e2e_config.sh -- the E2E harness's handling of the bitstream
# configuration, on the host only. Nothing here opens the FPGA link or starts
# a co-simulation: runtime-dispatch-test is only asked for --print-config and
# --list (or refuses before the link), systolic-matmul only meets
# configurations it must refuse and a problem it refuses right after
# (M = 0), and run_e2e.sh only runs with --check-config or stops at the
# configuration.
#
#   test/e2e/test_e2e_config.sh [--build-dir DIR]
#   cmake --build build --target check-e2e-config
#
# Checks:
#   runtime layer     no SYSTOLIC_FPGA_CONFIG is refused, not read as 16 / 16;
#                     K_MAX_8X8 and K_MAX_4X4 are read independently
#                     (K8 != K4 both ways, 1 / 1, 16376); the cases follow
#                     each capacity (k = 1, K_MAX - 1, K_MAX, K_MAX + 1
#                     refused), K_MAX = 1 has no k = 0 case
#   refusals          systolic-matmul exits 4 with the runtime's reason for a
#                     missing or invalid K_MAX_8X8 or K_MAX_4X4, a line that
#                     is not KEY=VALUE, another fleet, no file, an empty file
#   harness           board without a configuration, with two, with an
#                     unknown profile, and with metadata that contradict the
#                     K_MAX values all stop (exit 2) before the output
#                     directory is touched; the cases a run would send follow
#                     the configuration (quick, full, 1 / 1, K beyond
#                     systolic-matmul's limit NOT RUN); --regression is the
#                     default and --quick cases, each once and unchanged,
#                     then its seven larger ones; the archived profile;
#                     the co-simulation's record against a named
#                     configuration
#   verdicts          e2e_verdict.py on made-up case directories: PASS,
#                     MISMATCH, CONFIG (a refused configuration, a runtime
#                     that fell back to 16 / 16, a compiler that folded by
#                     other k_max values, a tile deeper than its device, K
#                     not folded by K_MAX, a slab of another length),
#                     TRANSPORT, SUBMISSION, INPUT, COMPILE, FAIL
#   propagation       (Linux, with a co-simulation's libftdi_cosim.so; else
#                     SKIP) systolic-matmul compiled under 24 / 12 and 12 / 24
#                     with that stand-in for libftdi and no socket, so the
#                     first runtime call fails to open a link that does not
#                     exist: the IR folds K by each configuration's K_MAX,
#                     and the run is classified TRANSPORT, rc -3; the board
#                     backend behind the same stand-in stops after the
#                     runtime layer's transport failure (cases NOT RUN), or
#                     with --keep-going reports each case TRANSPORT
# Exit status 0 only if every check passes (SKIPs do not fail).
set -uo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/../.." && pwd)"
BUILD="$ROOT/build"
while [ $# -gt 0 ]; do
    case "$1" in
        --build-dir) BUILD="$(cd "$2" && pwd)" || exit 2; shift 2 ;;
        -h|--help)   awk 'NR > 2 && !/^#/ { exit } NR > 2 { sub(/^# ?/, ""); print }' "$0"; exit 0 ;;
        *) echo "unknown argument: $1" >&2; exit 2 ;;
    esac
done
MATMUL="$BUILD/bin/systolic-matmul"
RTTEST="$BUILD/bin/runtime-dispatch-test"
RUN="$HERE/run_e2e.sh"
VERDICT="$HERE/e2e_verdict.py"
for b in "$MATMUL" "$RTTEST"; do
    [ -x "$b" ] || { echo "missing $b -- build first (cmake --build $BUILD)" >&2; exit 2; }
done

WORK="$(mktemp -d "${TMPDIR:-/tmp}/test_e2e_config.XXXXXX")"
trap 'rm -rf "$WORK"' EXIT
unset SYSTOLIC_FPGA_CONFIG

passed=0; failed=0; skipped=0
ok()   { printf '   PASS  %s\n' "$1"; passed=$((passed + 1)); }
bad()  { printf '   FAIL  %s\n' "$1"; [ -n "${2:-}" ] && printf '%s\n' "$2" | head -8 | sed 's/^/         | /'; failed=$((failed + 1)); }
skip() { printf '   SKIP  %s\n' "$1"; skipped=$((skipped + 1)); }
has()  { grep -qF -- "$2" <<< "$1"; }
cfg()  { printf '%b' "$2" > "$WORK/$1"; printf '%s\n' "$WORK/$1"; }

# ---- runtime layer -----------------------------------------------------------
echo "== runtime-dispatch-test: the configuration"
out=$("$RTTEST" --print-config 2>&1); rc=$?
[ $rc -eq 2 ] && has "$out" "CONFIG_ERROR=SYSTOLIC_FPGA_CONFIG is not set" && ! has "$out" "K_MAX_8X8=16" \
    && ok "no SYSTOLIC_FPGA_CONFIG: --print-config refuses (exit 2), no 16 / 16" || bad "no SYSTOLIC_FPGA_CONFIG: --print-config" "$out"
out=$("$RTTEST" --list 2>&1); rc=$?
[ $rc -eq 2 ] && ! has "$out" "run " && ok "no SYSTOLIC_FPGA_CONFIG: --list refuses (exit 2)" || bad "no SYSTOLIC_FPGA_CONFIG: --list" "$out"
out=$("$RTTEST" 2>&1); rc=$?
[ $rc -eq 2 ] && has "$out" "RUNTIME DISPATCH: FAIL -- CONFIG" && ! has "$out" "rc=" \
    && ok "no SYSTOLIC_FPGA_CONFIG: a run stops before any invocation (exit 2)" || bad "no SYSTOLIC_FPGA_CONFIG: a run" "$out"
out=$(SYSTOLIC_FPGA_CONFIG='' "$RTTEST" --print-config 2>&1); rc=$?
[ $rc -eq 2 ] && ok "SYSTOLIC_FPGA_CONFIG empty: refused like unset" || bad "SYSTOLIC_FPGA_CONFIG empty" "$out"

# K8 K4 slab: each value read from its own key.
while read -r k8 k4 slab; do
    f=$(cfg "k${k8}_${k4}.txt" "# a comment line\n K_MAX_4X4 = $k4 # trailing comment\nK_MAX_8X8=$k8\nBITSTREAM_SHA256=unknown\n")
    out=$(SYSTOLIC_FPGA_CONFIG="$f" "$RTTEST" --print-config 2>&1); rc=$?
    want="K_MAX_8X8=$k8
K_MAX_4X4=$k4
SLAB_BYTES=$slab
PIPELINE_DEPTH=1
COMPLETION_HEADER=0
SOURCE=$f"
    [ $rc -eq 0 ] && [ "$out" = "$want" ] && ok "K_MAX_8X8 = $k8, K_MAX_4X4 = $k4 -> slab $slab, pipeline depth 1 (not recorded)" \
        || bad "K_MAX_8X8 = $k8, K_MAX_4X4 = $k4" "$out"
done <<'EOF'
24 12 1536
12 24 1536
7 17 1536
17 7 1536
9 16 1024
1 1 512
16376 1 1048064
1 16376 1048064
16376 16376 1048064
512 512 32768
EOF

# PIPELINE_DEPTH: the context's job queue, read when recorded (the pipelined
# RTL's co-simulation and bitstream builds write it), 1 when not; malformed
# values are refused like the K_MAX ones.
f=$(cfg "pipe4.txt" "K_MAX_8X8=16\nK_MAX_4X4=16\nPIPELINE_DEPTH=4\n")
out=$(SYSTOLIC_FPGA_CONFIG="$f" "$RTTEST" --print-config 2>&1); rc=$?
[ $rc -eq 0 ] && has "$out" "PIPELINE_DEPTH=4" && ok "PIPELINE_DEPTH = 4 read" || bad "PIPELINE_DEPTH = 4" "$out"
for bad_depth in 0 65 04 x4; do
    f=$(cfg "pipe_bad_$bad_depth.txt" "K_MAX_8X8=16\nK_MAX_4X4=16\nPIPELINE_DEPTH=$bad_depth\n")
    out=$(SYSTOLIC_FPGA_CONFIG="$f" "$RTTEST" --print-config 2>&1); rc=$?
    [ $rc -ne 0 ] && has "$out" "PIPELINE_DEPTH = '$bad_depth' is not a decimal integer in 1..64" \
        && ok "PIPELINE_DEPTH = '$bad_depth' refused" || bad "PIPELINE_DEPTH = '$bad_depth'" "$out"
done

# COMPLETION_HEADER (the 16-byte completion record ahead of every result) and
# DDR_BYTES (the DDR the bitstream can address): recorded next to
# PIPELINE_DEPTH by the pipelined RTL's co-simulation and bitstream builds;
# read together, and COMPLETION_HEADER takes only 0 or 1.
f=$(cfg "pipe4_hdr.txt" "K_MAX_8X8=16\nK_MAX_4X4=16\nPIPELINE_DEPTH=4\nCOMPLETION_HEADER=1\nDDR_BYTES=1048576\n")
out=$(SYSTOLIC_FPGA_CONFIG="$f" "$RTTEST" --print-config 2>&1); rc=$?
[ $rc -eq 0 ] && has "$out" "PIPELINE_DEPTH=4" && has "$out" "COMPLETION_HEADER=1" && ok "PIPELINE_DEPTH = 4 with COMPLETION_HEADER = 1 and DDR_BYTES read" \
    || bad "PIPELINE_DEPTH = 4 with COMPLETION_HEADER = 1" "$out"
for bad_hdr in 2 x; do
    f=$(cfg "hdr_bad_$bad_hdr.txt" "K_MAX_8X8=16\nK_MAX_4X4=16\nCOMPLETION_HEADER=$bad_hdr\n")
    out=$(SYSTOLIC_FPGA_CONFIG="$f" "$RTTEST" --print-config 2>&1); rc=$?
    [ $rc -ne 0 ] && has "$out" "COMPLETION_HEADER = '$bad_hdr' is not 0 or 1" \
        && ok "COMPLETION_HEADER = '$bad_hdr' refused" || bad "COMPLETION_HEADER = '$bad_hdr'" "$out"
done
f=$(cfg "ddr_bad.txt" "K_MAX_8X8=16\nK_MAX_4X4=16\nDDR_BYTES=0\n")
out=$(SYSTOLIC_FPGA_CONFIG="$f" "$RTTEST" --print-config 2>&1); rc=$?
[ $rc -ne 0 ] && has "$out" "DDR_BYTES = '0' is not a decimal integer" \
    && ok "DDR_BYTES = '0' refused" || bad "DDR_BYTES = '0'" "$out"

echo "== runtime-dispatch-test: the cases follow each capacity"
out=$(SYSTOLIC_FPGA_CONFIG="$WORK/k24_12.txt" "$RTTEST" --list 2>&1); rc=$?
miss=""
for want in "run     8x24x8 full tile, k = K_MAX, device 0 (8x8)" \
            "run     4x12x4 full tile, k = K_MAX, device 1 (4x4)" \
            "run     8x1x8 k = 1, device 0 (8x8)" "run     7x23x8 k = K_MAX - 1, device 0 (8x8)" \
            "run     8x24x7 k = K_MAX, device 0 (8x8)" "run     4x1x4 k = 1, device 1 (4x4)" \
            "run     3x11x4 k = K_MAX - 1, device 1 (4x4)" "run     4x12x3 k = K_MAX, device 1 (4x4)" \
            "run     3x11x4 k = K_MAX - 1, device 3 (4x4)" "run     4x12x4 with C_in != 0, device 3 (4x4)" \
            "refuse  4x13x4 k = K_MAX + 1, device 1 (4x4)" "refuse  8x25x8 k = K_MAX + 1, device 0 (8x8)"; do
    has "$out" "$want" || miss="$miss
missing: $want"
done
[ $rc -eq 0 ] && [ -z "$miss" ] && ! grep -qE 'x16x|x17x' <<< "$out" \
    && ok "24 / 12: k = 1, 23, 24 on the 8x8, 1, 11, 12 on the 4x4s, 25 and 13 refused; nothing at 16" \
    || bad "24 / 12 --list" "$miss
$out"
out=$(SYSTOLIC_FPGA_CONFIG="$WORK/k12_24.txt" "$RTTEST" --list 2>&1)
has "$out" "run     8x12x8 full tile, k = K_MAX, device 0 (8x8)" && has "$out" "run     4x24x4 full tile, k = K_MAX, device 1 (4x4)" \
    && has "$out" "refuse  8x13x8" && has "$out" "refuse  4x25x4" \
    && ok "12 / 24 (swapped): the 8x8 at 12, the 4x4s at 24" || bad "12 / 24 --list" "$out"
out=$(SYSTOLIC_FPGA_CONFIG="$WORK/k1_1.txt" "$RTTEST" --list 2>&1)
n0=$(grep -cE '^run .* (x0x|k = K_MAX - 1)' <<< "$out")
[ "$n0" = 0 ] && [ "$(grep -c '^skip    k = K_MAX - 1 = 0' <<< "$out")" = 3 ] && has "$out" "refuse  8x2x8" && has "$out" "refuse  4x2x4" \
    && ok "1 / 1: no k = 0 case (three noted as not a GEMM K), k = 2 refused" || bad "1 / 1 --list" "$out"
out=$(SYSTOLIC_FPGA_CONFIG="$WORK/k16376_16376.txt" "$RTTEST" --list 2>&1); rc=$?
[ $rc -eq 0 ] && has "$out" "run     7x16375x8 k = K_MAX - 1, device 0 (8x8)" && has "$out" "refuse  8x16377x8" \
    && has "$out" "1048064-byte operand slab" && ok "16376 / 16376: the deepest cases and their slab" || bad "16376 / 16376 --list" "$out"

# ---- refusals -----------------------------------------------------------------
echo "== systolic-matmul refuses a malformed configuration before reading the problem"
while IFS='|' read -r tag body want; do
    if [ "$tag" = no_file ]; then f="$WORK/no_such_file.txt"; else f=$(cfg "bad_$tag.txt" "$body"); fi
    out=$(printf '0 0 0\n' | SYSTOLIC_FPGA_CONFIG="$f" "$MATMUL" 2>&1); rc=$?
    out2=$(SYSTOLIC_FPGA_CONFIG="$f" "$RTTEST" --print-config 2>&1); rc2=$?
    [ $rc -eq 4 ] && has "$out" "$want" && [ $rc2 -eq 2 ] && has "$out2" "$want" \
        && ok "$tag: exit 4, '$want'" || bad "$tag" "systolic-matmul exit $rc: $out
runtime-dispatch-test exit $rc2: $out2"
done <<'EOF'
missing_8x8|K_MAX_4X4=12\n|K_MAX_8X8 and K_MAX_4X4 are both required
missing_4x4|K_MAX_8X8=24\n|K_MAX_8X8 and K_MAX_4X4 are both required
zero_8x8|K_MAX_8X8=0\nK_MAX_4X4=12\n|K_MAX_8X8 = '0' is not a decimal integer in 1..16376
leading_zero_8x8|K_MAX_8X8=024\nK_MAX_4X4=12\n|K_MAX_8X8 = '024' is not a decimal integer in 1..16376
signed_8x8|K_MAX_8X8=+24\nK_MAX_4X4=12\n|K_MAX_8X8 = '+24' is not a decimal integer in 1..16376
too_deep_8x8|K_MAX_8X8=16377\nK_MAX_4X4=12\n|K_MAX_8X8 = '16377' is not a decimal integer in 1..16376
word_8x8|K_MAX_8X8=sixteen\nK_MAX_4X4=12\n|K_MAX_8X8 = 'sixteen' is not a decimal integer in 1..16376
zero_4x4|K_MAX_8X8=24\nK_MAX_4X4=0\n|K_MAX_4X4 = '0' is not a decimal integer in 1..16376
negative_4x4|K_MAX_8X8=24\nK_MAX_4X4=-12\n|K_MAX_4X4 = '-12' is not a decimal integer in 1..16376
fraction_4x4|K_MAX_8X8=24\nK_MAX_4X4=7.5\n|K_MAX_4X4 = '7.5' is not a decimal integer in 1..16376
hex_4x4|K_MAX_8X8=24\nK_MAX_4X4=0x10\n|K_MAX_4X4 = '0x10' is not a decimal integer in 1..16376
empty_4x4|K_MAX_8X8=24\nK_MAX_4X4=\n|K_MAX_4X4 = '' is not a decimal integer in 1..16376
too_deep_4x4|K_MAX_8X8=24\nK_MAX_4X4=99999999999\n|K_MAX_4X4 = '99999999999' is not a decimal integer in 1..16376
not_key_value|K_MAX_8X8=24\nK_MAX_4X4 12\n|line 2: expected KEY=VALUE
other_fleet|K_MAX_8X8=24\nK_MAX_4X4=12\nNUM_4X4=2\n|this runtime drives the 1x8x8 + 3x4x4 fleet
other_n|K_MAX_8X8=24\nK_MAX_4X4=12\nN=16\n|this runtime drives the 1x8x8 + 3x4x4 fleet
empty_file||K_MAX_8X8 and K_MAX_4X4 are both required
no_file||cannot open
EOF

# ---- harness ------------------------------------------------------------------
echo "== run_e2e.sh: which configuration, and what it sends (--check-config: nothing is sent)"
keep="$WORK/out_keep"; mkdir -p "$keep"; : > "$keep/.run_e2e_out"; : > "$keep/previous_result"
out=$(bash "$RUN" --backend board --build-dir "$BUILD" --out "$keep" 2>&1); rc=$?
[ $rc -eq 2 ] && has "$out" "the board backend needs the configuration" && has "$out" "E2E: FAIL -- CONFIGURATION" \
    && [ -f "$keep/previous_result" ] && ok "board, nothing named: exit 2 before --out is emptied" || bad "board, nothing named" "$out"
out=$(bash "$RUN" --backend board --build-dir "$BUILD" --check-config --out "$WORK/o1" 2>&1); rc=$?
[ $rc -eq 2 ] && ok "board --check-config, nothing named: exit 2" || bad "board --check-config, nothing named" "$out"
out=$(SYSTOLIC_FPGA_CONFIG="$WORK/k24_12.txt" bash "$RUN" --backend board --build-dir "$BUILD" \
      --profile archived-1x8x8-3x4x4 --check-config --out "$WORK/o2" 2>&1); rc=$?
[ $rc -eq 2 ] && has "$out" "give one" && ok "SYSTOLIC_FPGA_CONFIG and --profile together: exit 2" || bad "SYSTOLIC_FPGA_CONFIG and --profile" "$out"
out=$(bash "$RUN" --backend board --build-dir "$BUILD" --profile no-such-bitstream --check-config --out "$WORK/o3" 2>&1); rc=$?
[ $rc -eq 2 ] && has "$out" "no profile 'no-such-bitstream'" && ok "unknown --profile: exit 2" || bad "unknown --profile" "$out"
out=$(SYSTOLIC_FPGA_CONFIG="$WORK/bad_missing_4x4.txt" bash "$RUN" --backend board --build-dir "$BUILD" --check-config --out "$WORK/o4" 2>&1); rc=$?
[ $rc -eq 2 ] && has "$out" "K_MAX_8X8 and K_MAX_4X4 are both required" && ok "an invalid named configuration: exit 2 with the runtime's reason" \
    || bad "an invalid named configuration" "$out"

check_cases() {   # description, config file, extra run_e2e.sh args, then: + present, - absent, ! not run
    local what="$1" f="$2" args="$3"; shift 3
    local out rc miss="" o="$WORK/o_$passed$failed$skipped"
    # shellcheck disable=SC2086
    out=$(SYSTOLIC_FPGA_CONFIG="$f" bash "$RUN" --backend board --build-dir "$BUILD" --check-config --out "$o" $args 2>&1); rc=$?
    [ $rc -eq 0 ] || { bad "$what: --check-config exit $rc" "$out"; return; }
    for w in "$@"; do
        case "$w" in
            +*) grep -qx -- "${w#+}" "$o/case_list.txt" || miss="$miss ${w#+}(missing)" ;;
            -*) ! grep -qx -- "${w#-}" "$o/case_list.txt" || miss="$miss ${w#-}(present)" ;;
            !*) grep -q -- "^${w#!} " "$o/not_run.txt" || miss="$miss ${w#!}(not listed as not run)" ;;
        esac
    done
    [ -z "$miss" ] && ok "$what" || bad "$what" "$miss"
}
check_cases "24 / 12, full: the 36 fixed cases and the boundaries of both capacities" "$WORK/k24_12.txt" "" \
    +8x8x8_zero +32x32x32_mixed +12x12x1_mixed +12x12x11_mixed +12x12x12_mixed +12x12x13_mixed \
    +12x12x23_mixed +12x12x24_mixed +12x12x25_mixed +12x12x51_mixed +4x4x12_mixed +8x8x24_mixed \
    -12x12x15_mixed -12x12x16_mixed -12x12x17_mixed -12x12x33_mixed
check_cases "12 / 24 (swapped): the boundaries move with the values, not the keys' order" "$WORK/k12_24.txt" "" \
    +4x4x24_mixed +8x8x12_mixed -4x4x12_mixed -8x8x24_mixed +12x12x51_mixed
check_cases "17 / 7, --quick: the six required shapes and 8x8x18, 4x4x8" "$WORK/k17_7.txt" "--quick" \
    +1x1x1_zero +16x8x4_mixed +8x8x18_mixed +4x4x8_mixed -32x32x32_mixed -8x8x17_mixed -12x12x18_mixed
check_cases "17 / 7, --fixed-only: the 36 fixed cases, nothing from the capacities" "$WORK/k17_7.txt" "--fixed-only" \
    +17x9x33_mixed -12x12x18_mixed -8x8x18_mixed
check_cases "1 / 1: K = 1, 2 and 5, no K = 0" "$WORK/k1_1.txt" "--kmax-cases" \
    +12x12x1_mixed +12x12x2_mixed +12x12x5_mixed +4x4x1_mixed +8x8x1_mixed -12x12x0_mixed
check_cases "16376 / 1: K beyond systolic-matmul's limit is NOT RUN, the 4x4s' boundaries run" "$WORK/k16376_1.txt" "--kmax-cases" \
    +12x12x1_mixed +12x12x2_mixed +4x4x1_mixed '!12x12x16375_mixed' '!12x12x16377_mixed' '!8x8x16376_mixed' '!12x12x32755_mixed'
check_cases "--extra-k with the boundary cases" "$WORK/k24_12.txt" "--kmax-cases --extra-k 7,48" +12x12x7_mixed +12x12x48_mixed

# --regression: every case of the default run and of --quick, once, with the
# same name and input file, in that order, then the seven larger ones, which
# no other mode runs.
NEW7=(64x64x64_mixed 32x64x32_mixed 16x16x31_mixed 32x32x65_mixed 7x7x16_mixed 15x15x16_mixed 33x33x16_mixed)
f16=$(cfg k16_16.txt 'K_MAX_8X8=16\nK_MAX_4X4=16\n')
check_cases "16 / 16, --regression: the default and --quick cases, then the seven larger ones" "$f16" "--regression" \
    +1x1x1_zero +16x8x4_mixed +32x32x32_zero +32x32x32_mixed +12x12x15_mixed +12x12x35_mixed \
    +8x8x16_mixed +8x8x17_mixed +4x4x17_mixed "${NEW7[@]/#/+}" -64x64x64_zero -16x16x16_mixed
check_cases "--extra-k with --regression" "$f16" "--regression --extra-k 7,48" +12x12x7_mixed +12x12x48_mixed +64x64x64_mixed
reg_ok=1; reg_why=""
for m in default quick regression; do
    args=(); [ "$m" = default ] || args=("--$m")
    SYSTOLIC_FPGA_CONFIG="$f16" bash "$RUN" --backend board --build-dir "$BUILD" --check-config \
        --out "$WORK/or_$m" ${args[@]+"${args[@]}"} > "$WORK/or_$m.log" 2>&1 \
        || { reg_ok=0; reg_why="$m --check-config: $(tail -1 "$WORK/or_$m.log")"; }
done
if [ "$reg_ok" = 1 ]; then
    want=$({ cat "$WORK/or_default/case_list.txt" "$WORK/or_quick/case_list.txt"; printf '%s\n' "${NEW7[@]}"; } | awk '!seen[$0]++')
    [ "$(cat "$WORK/or_regression/case_list.txt")" = "$want" ] \
        || { reg_ok=0; reg_why="the case list is not the default run's, then --quick's, then the seven, each once"; }
    for n in "${NEW7[@]}"; do
        ! grep -qx -- "$n" "$WORK/or_default/case_list.txt" "$WORK/or_quick/case_list.txt" \
            || { reg_ok=0; reg_why="$n is in the default or --quick run"; }
    done
    for m in default quick; do
        while read -r n; do
            cmp -s "$WORK/or_$m/cases/$n.txt" "$WORK/or_regression/cases/$n.txt" \
                || { reg_ok=0; reg_why="$n: the --regression input differs from the $m run's"; }
        done < "$WORK/or_$m/case_list.txt"
    done
fi
[ "$reg_ok" = 1 ] && ok "16 / 16: --regression = the default and --quick cases (same inputs, each once) + seven no other mode runs" \
    || bad "16 / 16 --regression case set" "$reg_why"
out=$(bash "$RUN" --backend board --build-dir "$BUILD" --quick --regression --check-config --out "$WORK/ox" 2>&1); rc=$?
[ $rc -eq 2 ] && has "$out" "exclude each other" && ok "--quick with --regression: exit 2" || bad "--quick with --regression" "$out"

out=$(SYSTOLIC_FPGA_CONFIG="$WORK/k24_12.txt" bash "$RUN" --backend board --build-dir "$BUILD" --check-config --out "$WORK/o5" 2>&1); rc=$?
[ $rc -eq 0 ] && has "$out" "K_MAX_8X8 = 24, K_MAX_4X4 = 12, 1536-byte operand slab" \
    && has "$out" "Hardware configuration not independently verified" \
    && has "$out" "configuration refusal: PASS" && has "$out" "nothing was sent to the FPGA" \
    && [ ! -e "$WORK/o5/runtime_dispatch.txt" ] && [ -z "$(find "$WORK/o5" -maxdepth 1 -type d -name '*_mixed')" ] \
    && ok "board --check-config 24 / 12: resolved, hardware not verified, refusals pass, no case run" \
    || bad "board --check-config 24 / 12" "$out"
bit="$ROOT/artifacts/fpga/tile_pipelined/1x8x8-3x4x4/systolic_dma_top_1x8x8_3x4x4.bit"
out=$(bash "$RUN" --backend board --build-dir "$BUILD" --profile archived-1x8x8-3x4x4 --check-config --out "$WORK/o6" 2>&1); rc=$?
if [ -f "$bit" ]; then shaok="BITSTREAM_SHA256 = 71b6847d5b4c30b23fc99c5ff30a24254552161702ddfd1fb79a3d5657edff7d: agrees with"
else shaok="not checked"; fi
[ $rc -eq 0 ] && has "$out" "K_MAX_8X8 = 16, K_MAX_4X4 = 16, 1024-byte operand slab" && has "$out" "from --profile archived-1x8x8-3x4x4" \
    && has "$out" "HOST_SLAB_BYTES = 1024: agrees" && has "$out" "$shaok" \
    && ok "--profile archived-1x8x8-3x4x4: 16 / 16, 1024 B, its metadata agree" || bad "--profile archived-1x8x8-3x4x4" "$out"

echo "== run_e2e.sh: metadata that contradict the K_MAX values stop the run"
meta() {   # description, file contents, expected text
    local f out rc
    f=$(cfg "meta_$passed$failed.txt" "$2")
    out=$(SYSTOLIC_FPGA_CONFIG="$f" bash "$RUN" --backend board --build-dir "$BUILD" --check-config --out "$WORK/om_$passed$failed" 2>&1); rc=$?
    [ $rc -eq 2 ] && has "$out" "$3" && ok "$1" || bad "$1" "$out"
}
meta "HOST_SLAB_BYTES = 1024 with 24 / 12 (1536)" 'K_MAX_8X8=24\nK_MAX_4X4=12\nHOST_SLAB_BYTES=1024\n' "HOST_SLAB_BYTES = 1024, but K_MAX_8X8 = 24 and K_MAX_4X4 = 12 imply a 1536-byte slab"
meta "HOST_SLAB_BYTES = 1024 with 16 / 24 (1536): the 4x4s decide the slab" 'K_MAX_8X8=16\nK_MAX_4X4=24\nHOST_SLAB_BYTES=1024\n' "imply a 1536-byte slab"
meta "K_DIM = 16 with K_MAX_8X8 = 24" 'K_MAX_8X8=24\nK_MAX_4X4=12\nK_DIM=16\n' "K_DIM = 16, but K_MAX_8X8 = 24"
meta "K_MAX_4X4 given twice" 'K_MAX_8X8=24\nK_MAX_4X4=16\nK_MAX_4X4=12\n' "K_MAX_4X4 is given 2 times"
meta "BITSTREAM_SHA256 not a sha256" 'K_MAX_8X8=24\nK_MAX_4X4=12\nBITSTREAM_SHA256=1234\n' "is not a sha256"
printf 'not a bitstream\n' > "$WORK/fake.bit"
meta "BITSTREAM_SHA256 against the BITSTREAM file" "K_MAX_8X8=24\nK_MAX_4X4=12\nBITSTREAM=fake.bit\nBITSTREAM_SHA256=71b6847d5b4c30b23fc99c5ff30a24254552161702ddfd1fb79a3d5657edff7d\n" "hashes to"

echo "== run_e2e.sh: the co-simulation's record"
sim="$WORK/cosim"; mkdir -p "$sim"
out=$(bash "$RUN" --backend cosim --build-dir "$BUILD" --cosim-dir "$sim" --check-config --out "$WORK/o7" 2>&1); rc=$?
[ $rc -eq 2 ] && has "$out" "has no fpga_config.txt" && ok "co-simulation without a record, nothing named: exit 2" || bad "co-simulation without a record" "$out"
printf 'K_MAX_8X8=17\nK_MAX_4X4=7\nN=8\nNUM_8X8=1\nNUM_4X4=3\nHOST_SLAB_BYTES=1536\n' > "$sim/fpga_config.txt"
out=$(bash "$RUN" --backend cosim --build-dir "$BUILD" --cosim-dir "$sim" --check-config --out "$WORK/o8" 2>&1); rc=$?
[ $rc -eq 0 ] && has "$out" "K_MAX_8X8 = 17, K_MAX_4X4 = 7, 1536-byte operand slab" && has "$out" "from the co-simulation's build record" \
    && grep -qx 12x12x18_mixed "$WORK/o8/case_list.txt" \
    && ok "co-simulation record 17 / 7 used when nothing is named" || bad "co-simulation record 17 / 7" "$out"
out=$(SYSTOLIC_FPGA_CONFIG="$WORK/k24_12.txt" bash "$RUN" --backend cosim --build-dir "$BUILD" --cosim-dir "$sim" --check-config --out "$WORK/o9" 2>&1); rc=$?
[ $rc -eq 2 ] && has "$out" "was built with K_MAX_8X8 = 17, K_MAX_4X4 = 7" \
    && ok "SYSTOLIC_FPGA_CONFIG 24 / 12 against a 17 / 7 co-simulation (same slab): exit 2" || bad "24 / 12 against 17 / 7" "$out"
out=$(SYSTOLIC_FPGA_CONFIG="$WORK/k17_7.txt" bash "$RUN" --backend cosim --build-dir "$BUILD" --cosim-dir "$sim" --check-config --out "$WORK/o10" 2>&1); rc=$?
[ $rc -eq 0 ] && ok "SYSTOLIC_FPGA_CONFIG naming the co-simulation's values: accepted" || bad "matching SYSTOLIC_FPGA_CONFIG" "$out"

# ---- verdicts -------------------------------------------------------------------
echo "== e2e_verdict.py: one class per outcome, with the raw return code"
SRC="$WORK/k24_12.txt"
mkcase() {   # dir name, output.txt, IR tiles ("m n k device", ;-separated)
    local d="$WORK/v/$1" t m n k dev
    rm -rf "$d"; mkdir -p "$d/ir"
    printf '%b' "$2" > "$d/output.txt"
    : > "$d/ir/10-systolic.mlir"
    while IFS= read -r t; do
        [ -n "$t" ] || continue
        read -r m n k dev <<< "$t"
        printf '    %%0 = systolic.matmul_tile %%a, %%b, %%c m = %s n = %s k = %s on @%s est_cycles = 1 start_cycle = 0\n' \
            "$m" "$n" "$k" "$dev" >> "$d/ir/10-systolic.mlir"
    done < <(tr ';' '\n' <<< "$3")
    printf '%s\n' "$d"
}
header() { printf 'systolic-matmul: C[8][8] = A[8][25] x B[25][8] on the 1x8x8 + 3x4x4 FPGA\n  K_MAX_8X8 = %s, K_MAX_4X4 = %s (%s)\n\nPipeline\n  [2] Systolic   2 systolic.matmul_tile (binary-search decomposition, K folded by k_max = %s on the 8x8, %s on the 4x4s)\n' "$1" "$2" "$3" "${4:-$1}" "${5:-$2}"; }
TILES8='8 8 24 acc_8x8_0;8 8 1 acc_8x8_0'
runtime_ok() { printf '  [5] Runtime    2 FPGA invocation(s): device 0 (8x8) 2, device 1 0, device 2 0, device 3 0 (4x4)\n                 DPTI: %s B to the FPGA, 512 B back, 0.010 s\n\nM N K = 8 8 25   FPGA invocations = 2   mismatches = %s / 64\n' "${1:-3258}" "${2:-0}"; }
verdict() {   # description, expected class, expected rt_rc, case dir, exit, [extra args]
    local what="$1" cls="$2" rrc="$3" d="$4" ex="$5"; shift 5
    local line got gotrc
    line=$(python3 "$VERDICT" "$d" --exit "$ex" --k8 24 --k4 12 --slab 1536 --source "$SRC" "$@" 2>&1)
    got=$(cut -f1 <<< "$line"); gotrc=$(cut -f2 <<< "$line")
    [ "$got" = "$cls" ] && [ "$gotrc" = "$rrc" ] && ok "$what -> $cls (rc $rrc)" || bad "$what: expected $cls / $rrc" "$line"
}
d=$(mkcase 8x8x25_mixed "$(header 24 12 "$SRC")\n$(runtime_ok)\nVerification: PASS\n" "$TILES8")
verdict "a passing run (board)" PASS 0 "$d" 0
verdict "a passing run, the RTL moved what the runtime counted" PASS 0 "$d" 0 --cosim 3258 3258 512 7
verdict "the RTL consumed other bytes than the runtime sent" TRANSPORT 0 "$d" 0 --cosim 3258 3000 512 7
verdict "no co-simulation session" FAIL 0 "$d" 0 --cosim-error "expected exactly one co-simulation session"
d=$(mkcase 8x8x25_mixed "$(header 16 16 "built-in default (artifacts/fpga/tile_pipelined/1x8x8-3x4x4: K_MAX_8X8 = K_MAX_4X4 = 16)")\n$(runtime_ok)\nVerification: PASS\n" "$TILES8")
verdict "the runtime fell back to the built-in 16 / 16" CONFIG 0 "$d" 0
d=$(mkcase 8x8x25_mixed "$(header 24 12 "/elsewhere/fpga_config.txt")\n$(runtime_ok)\nVerification: PASS\n" "$TILES8")
verdict "the runtime read another file" CONFIG 0 "$d" 0
d=$(mkcase 8x8x25_mixed "$(header 24 12 "$SRC" 16 16)\n$(runtime_ok)\nVerification: PASS\n" "$TILES8")
verdict "the compiler folded K by k_max 16 / 16" CONFIG 0 "$d" 0
d=$(mkcase 8x8x25_mixed "$(header 24 12 "$SRC")\n$(runtime_ok)\nVerification: PASS\n" '8 8 16 acc_8x8_0;8 8 9 acc_8x8_0')
verdict "K folded 16 + 9 on an 8x8 of K_MAX 24" CONFIG 0 "$d" 0
d=$(mkcase 4x4x13_mixed "$(header 24 12 "$SRC")\n$(runtime_ok 3258)\nVerification: PASS\n" '4 4 13 acc_4x4_1')
verdict "a 4x4 tile with k = 13 > K_MAX_4X4 = 12" CONFIG 0 "$d" 0
d=$(mkcase 8x8x25_mixed "$(header 24 12 "$SRC")\n$(runtime_ok 2234)\nVerification: PASS\n" "$TILES8")
verdict "invocations that carried a 1024-byte slab, not 1536" CONFIG 0 "$d" 0
d=$(mkcase 8x8x25_mixed "$(header 24 12 "$SRC")\n$(runtime_ok)\nVerification: PASS\n" '8 8 24 acc_8x8_0')
verdict "a tile missing (the tiles do not cover the GEMM)" FAIL 0 "$d" 0
d=$(mkcase 8x8x25_mixed "$(header 24 12 "$SRC")\n$(runtime_ok 3258 3)\nfirst mismatch: i=0 j=1  FPGA=5  CPU=4\nVerification: FAIL\n" "$TILES8")
verdict "a numerical mismatch" MISMATCH 0 "$d" 1
d=$(mkcase 8x8x25_mixed "systolic-matmul: SYSTOLIC_FPGA_CONFIG=$SRC: K_MAX_8X8 and K_MAX_4X4 are both required\n" "")
verdict "systolic-matmul refused the configuration" CONFIG - "$d" 4
for pair in -3:TRANSPORT -4:TRANSPORT -6:TRANSPORT -7:TRANSPORT -8:TRANSPORT -14:TRANSPORT \
            -5:SUBMISSION -10:SUBMISSION -11:SUBMISSION -12:SUBMISSION -13:SUBMISSION -15:SUBMISSION -16:CONFIG -99:FAIL; do
    r=${pair%%:*}; c=${pair#*:}
    d=$(mkcase 8x8x25_mixed "$(header 24 12 "$SRC")\n  [5] Runtime    0 FPGA invocation(s): device 0 (8x8) 0, device 1 0, device 2 0, device 3 0 (4x4)\n                 DPTI: 0 B to the FPGA, 0 B back, 0.001 s\n\nFPGA dispatch: FAIL -- 2 of 2 runtime call(s) failed (first error $r); C is not valid\nVerification: FAIL\n" "$TILES8")
    verdict "runtime first error $r" "$c" "$r" "$d" 4
done
d=$(mkcase 8x8x25_mixed "systolic-matmul: K = 5000 is outside 1..4096\n" "")
verdict "systolic-matmul refused the input" INPUT - "$d" 2
d=$(mkcase 8x8x25_mixed "$(header 24 12 "$SRC")\nsystolic-matmul: Linalg -> Systolic (binary-search tiling) failed\n" "")
verdict "the compiler failed" COMPILE - "$d" 3
d=$(mkcase 8x8x25_mixed "$(header 24 12 "$SRC")" "$TILES8")
verdict "killed by a signal" FAIL - "$d" 139

# ---- propagation through the compiler --------------------------------------------
echo "== propagation: the compiler folds K by the configured K_MAX values (no link)"
SHIM=""
if [ "$(uname -s)" = Linux ]; then
    for s in "$BUILD/cosim/libftdi_cosim.so" "$BUILD"/kmax_matrix/cosim/*/libftdi_cosim.so; do
        [ -f "$s" ] && { SHIM="$s"; break; }
    done
fi
if [ -z "$SHIM" ]; then
    skip "no libftdi_cosim.so under $BUILD (test/e2e/cosim/build.sh builds one; Linux only)"
else
    python3 "$HERE/gen_cases.py" "$WORK/pcases" --kmax 24 12 > /dev/null
    for pair in "24 12" "12 24"; do
        set -- $pair
        f="$WORK/k$1_$2.txt"
        dd="$WORK/p/$1_$2/12x12x25_mixed"; mkdir -p "$dd/ir"
        # libftdi_cosim.so with no socket: ftdi_usb_open fails, so the first
        # runtime call returns -3 and nothing is sent anywhere.
        env -u SYSTOLIC_DPTI_COSIM_SOCKET LD_PRELOAD="$SHIM" SYSTOLIC_FPGA_CONFIG="$f" \
            "$MATMUL" --dump-ir="$dd/ir" < "$WORK/pcases/12x12x25_mixed.txt" > "$dd/output.txt" 2>&1
        rc=$?
        line=$(python3 "$VERDICT" "$dd" --exit "$rc" --k8 "$1" --k4 "$2" --slab 1536 --source "$f")
        k8max=$(sed -nE 's/.* k = ([0-9]+) on @acc_8x8_0.*/\1/p' "$dd/ir/10-systolic.mlir" | sort -n | tail -1)
        k4max=$(sed -nE 's/.* k = ([0-9]+) on @acc_4x4_.*/\1/p' "$dd/ir/10-systolic.mlir" | sort -n | tail -1)
        [ $rc -eq 4 ] && [ "$(cut -f1 <<< "$line")" = TRANSPORT ] && [ "$(cut -f2 <<< "$line")" = -3 ] \
            && [ "${k8max:-0}" = "$1" ] && [ "${k4max:-0}" = "$2" ] \
            && has "$(cat "$dd/output.txt")" "K_MAX_8X8 = $1, K_MAX_4X4 = $2 ($f)" \
            && ok "$1 / $2: 12x12x25 folded with k <= $1 on the 8x8, <= $2 on the 4x4s; TRANSPORT rc -3 without a link" \
            || bad "$1 / $2 propagation (exit $rc, 8x8 max k ${k8max:-none}, 4x4 max k ${k4max:-none})" "$line"
    done

    # The board backend end to end, with that stand-in in front of every
    # program: no link opens, so the runtime layer is a transport failure,
    # the cases are not run, and with --keep-going each one is TRANSPORT -3.
    out=$(env -u SYSTOLIC_DPTI_COSIM_SOCKET LD_PRELOAD="$SHIM" SYSTOLIC_FPGA_CONFIG="$WORK/k24_12.txt" \
          bash "$RUN" --backend board --build-dir "$BUILD" --quick --out "$WORK/dead" 2>&1); rc=$?
    [ $rc -eq 1 ] && has "$out" "runtime layer: FAIL -- TRANSPORT (exit 3): TRANSPORT 1 (first: rc=-3" \
        && has "$out" "E2E cases: 0/0 PASS on backend board; NOT RUN 26" && has "$out" "E2E: FAIL -- runtime layer TRANSPORT" \
        && ok "board, no link: runtime layer TRANSPORT (rc -3, the rest -14 NOT RUN), the 26 cases NOT RUN" \
        || bad "board, no link" "$(tail -5 <<< "$out")"
    out=$(env -u SYSTOLIC_DPTI_COSIM_SOCKET LD_PRELOAD="$SHIM" SYSTOLIC_FPGA_CONFIG="$WORK/k24_12.txt" \
          bash "$RUN" --backend board --build-dir "$BUILD" --quick --keep-going \
          --cases 8x8x25_mixed,4x4x13_mixed --out "$WORK/dead2" 2>&1); rc=$?
    [ $rc -eq 1 ] && [ "$(grep -cE '^(8x8x25|4x4x13)_mixed +TRANSPORT +4 +-3 ' <<< "$out")" = 2 ] \
        && has "$out" "E2E cases: 0/2 PASS on backend board; TRANSPORT 2" \
        && ok "board, no link, --keep-going: each case TRANSPORT, exit 4, runtime rc -3" \
        || bad "board, no link, --keep-going" "$(grep -E '_mixed|E2E' <<< "$out")"
fi

echo
echo "test_e2e_config: $passed passed, $failed failed, $skipped skipped"
[ $failed -eq 0 ]
