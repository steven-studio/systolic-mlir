#!/usr/bin/env bash
#
# run_cosim_regressions.sh -- regression checks that need the RTL
# co-simulation (test/e2e/cosim/build.sh), for defects the 2026-10-20
# integration audit found on the host side of the DPTI link and in the
# co-simulation bench itself. Each section starts its own co-simulation on its
# own socket and stops it afterwards. Every program runs with
# libftdi_cosim.so preloaded, whose ftdi_usb_open only ever connects to that
# socket, so nothing here can reach a board, even one that is plugged in.
#
#   test/e2e/run_cosim_regressions.sh [--build-dir DIR] [--cosim-dir DIR]
#                                     [--out DIR] [--section NAME,...]
#   cmake --build build --target check-e2e-cosim-regressions
#
# Sections (all by default):
#   bench   the co-simulation keeps the board running after a host leaves
#           until the jobs it submitted have sent their completion record
#           and result, so nothing of them reaches the next session: a host
#           that leaves with jobs in flight (its commands complete, nothing
#           read back), the same after bytes that are no opcode (the RTL
#           consumes and skips them), and a descriptor the RTL refuses (a
#           16-byte record and no result: the session must close at once,
#           not after the drain's time limit). The hosts are replays of the
#           first jobs runtime-dispatch-test sends; after each one
#           runtime-dispatch-test runs again and must PASS.
#   modef   runtime-async-test's mode F needs a link failure injected by the
#           co-simulation's stand-in for libftdi, which the board's libftdi
#           never injects: with the co-simulation standing for the board
#           (SYSTOLIC_DPTI_COSIM_NO_FAULTS=1), run_e2e.sh --backend board
#           --mode async must PASS with F NOT APPLICABLE; on the
#           co-simulation F must run and PASS, and the harness must fail the
#           runtime layer if F does not PASS there; a wrong result (one read
#           byte flipped, SYSTOLIC_DPTI_COSIM_CORRUPT_READ) must still fail
#           the tool and the harness on the board model.
#   negoffset
#           test/SystolicCC/negoffset.c built by systolic-cc --runtime=board
#           --verify, both submissions: calls whose integer arguments put an
#           accessed row before an array argument's first element run on the
#           CPU and never reach the link, the others run on the FPGA and are
#           verified bitwise, all in one session, and the output equals the
#           program compiled by clang alone -- also when a link write fails
#           (each such run on a fresh co-simulation).
#   kmax    test/e2e/systolic_cc/kmax_mismatch.c built for K_MAX 32 / 32 and
#           run against this 16 / 16 co-simulation, both submissions, under
#           timeout -s KILL: every fold but the last is refused at submit;
#           the program must finish within the bound, with every GEMM on the
#           CPU and the output of the program compiled by clang alone (the
#           asynchronous runtime used to wait for ever for the refused folds).
#   views   test/SystolicCC/async_views.c (outputs written in reverse row
#           order, per batch, one row down a column, a forward view at an
#           offset, a reversed GEMM after a forward one) built by systolic-cc
#           --runtime=board --verify, asynchronous submission: every GEMM on
#           the FPGA, verified bitwise, no submit refused, output equal to
#           the program compiled by clang alone (such views used to be
#           submitted with ldc < n, refused, and run on the CPU).
#   header  the synchronous runtime with a configuration that does not say
#           COMPLETION_HEADER=1 against this RTL, which sends a 16-byte
#           completion record ahead of every result: runtime-dispatch-test
#           must refuse its first result (-9, the configuration named), never
#           read the record as tile data, twice in a row (the link stays in
#           step), and pass with the right configuration; a systolic-cc
#           program built and run without any configuration (the runtime's
#           built-in default) must run on the CPU with the program's output.
#           With --prerecord-cosim-dir (a co-simulation of RTL without the
#           record, built by that revision's own bench, e.g. git archive
#           5d6f01a test/e2e/cosim rtl, then its test/e2e/cosim/build.sh: this
#           tree's bench probes signals only the record RTL has), a
#           header-less configuration must PASS there (no false refusal);
#           else that check is SKIPPED.
#   jobid   job ids of different programs differ: both runtimes pick the
#           job_id epoch (bits 31..24) at random per process
#           (SYSTOLIC_DPTI_EPOCH fixes it, for tests). A completion record
#           an earlier program left on the link (its first result, injected
#           at the session's start with SYSTOLIC_DPTI_COSIM_INJECT_RX, as a
#           board can deliver one after the next session's purge) is never
#           taken for this program's result: the synchronous runtime refuses
#           it (-9), the asynchronous one ignores it and runs correctly; and
#           four programs started in a row do not all use one epoch.
#
# Exit status 0 only if every check passes (a SKIP is not a pass).
set -uo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/../.." && pwd)"
BUILD="$ROOT/build"
COSIM_DIR=""
OUT=""
SECTIONS="bench,modef,negoffset,kmax,views,header,jobid"
PRERECORD=""
while [ $# -gt 0 ]; do
    case "$1" in
        --build-dir) BUILD="$(cd "$2" && pwd)" || exit 2; shift 2 ;;
        --cosim-dir) COSIM_DIR="$(cd "$2" && pwd)" || exit 2; shift 2 ;;
        --prerecord-cosim-dir) PRERECORD="$(cd "$2" && pwd)" || exit 2; shift 2 ;;
        --out)       OUT="$2"; shift 2 ;;
        --section)   SECTIONS="$2"; shift 2 ;;
        -h|--help)   awk 'NR > 2 && !/^#/ { exit } NR > 2 { sub(/^# ?/, ""); print }' "$0"; exit 0 ;;
        *) echo "unknown argument: $1" >&2; exit 2 ;;
    esac
done
COSIM_DIR="${COSIM_DIR:-$BUILD/cosim}"
OUT="${OUT:-$BUILD/cosim-regressions}"
for s in $(tr ',' ' ' <<< "$SECTIONS"); do
    case "$s" in
        bench|modef|negoffset|kmax|views|header|jobid) ;;
        *) echo "unknown section: $s (sections: bench, modef, negoffset, kmax, views, header, jobid)" >&2; exit 2 ;;
    esac
done

VCOSIM="$COSIM_DIR/Vcosim"
SHIM="$COSIM_DIR/libftdi_cosim.so"
CFG="$COSIM_DIR/fpga_config.txt"
[ -x "$VCOSIM" ] && [ -f "$SHIM" ] && [ -f "$CFG" ] || {
    echo "no co-simulation in $COSIM_DIR -- run test/e2e/cosim/build.sh" >&2; exit 2; }
grep -qx 'COMPLETION_HEADER=1' "$CFG" || {
    echo "$CFG: this co-simulation's RTL sends no completion record; these checks need the working tree's RTL (test/e2e/cosim/build.sh)" >&2; exit 2; }
for t in runtime-dispatch-test runtime-async-test systolic-matmul systolic-cc; do
    [ -x "$BUILD/bin/$t" ] || { echo "no $BUILD/bin/$t -- build the project first" >&2; exit 2; }
done
# The C compiler and LLVM build systolic-cc uses (the build's own).
CLANG=$(sed -nE 's/^SYSTOLIC_CFRONT_CLANG:[A-Z]*=(.*)$/\1/p' "$BUILD/CMakeCache.txt")
LLVM_BUILD=$(sed -nE 's/^LLVM_DIR:[A-Z]*=(.*)\/lib\/cmake\/llvm$/\1/p' "$BUILD/CMakeCache.txt")
command -v python3 >/dev/null || { echo "python3 not found" >&2; exit 2; }
mkdir -p "$OUT" || exit 2

passed=0; failed=0; skipped=0
skip() { skipped=$((skipped + 1)); echo "SKIP: $1"; }
ok()  { passed=$((passed + 1)); echo "PASS: $1"; }
bad() { failed=$((failed + 1)); echo "FAIL: $1"; [ $# -gt 1 ] && [ -n "$2" ] && sed 's/^/      /' <<< "$2"; return 0; }
has() { grep -qF -- "$2" <<< "$1"; }

# A socket path must stay short (sun_path), so it lives under /tmp, not $OUT.
SOCKDIR=$(mktemp -d "${TMPDIR:-/tmp}/cosimreg.XXXXXX") || exit 2
SOCK="$SOCKDIR/dpti.sock"
COSIM_PID=""
LOG=""
stop_cosim() {
    if [ -n "$COSIM_PID" ]; then
        kill "$COSIM_PID" 2>/dev/null
        wait "$COSIM_PID" 2>/dev/null
        COSIM_PID=""
    fi
    rm -f -- "${SOCK:?}"
}
# (A run_e2e.sh record an interrupted modef section left in $SOCKDIR goes too.)
cleanup() {
    stop_cosim
    [ ! -f "$SOCKDIR/guard/.run_e2e_out" ] || rm -rf -- "${SOCKDIR:?}/guard"
    rmdir -- "${SOCKDIR:?}" 2>/dev/null
}
trap cleanup EXIT

# start_cosim DIR [VCOSIM]: a fresh co-simulation (this one's, or another
# build's Vcosim), logging to DIR/cosim.log.
start_cosim() {
    mkdir -p "$1"
    LOG="$1/cosim.log"
    "${2:-$VCOSIM}" +socket="$SOCK" +max_sessions=0 \
        > >(grep --line-buffered -v -E '^(JOBFIREDBG|SCHEDHS|SCHTRACE|JOBHS) ' > "$LOG") 2>&1 &
    COSIM_PID=$!
    for _ in $(seq 100); do [ -S "$SOCK" ] && return 0; sleep 0.1; done
    return 1
}
opened() { grep -c '^cosim: session [0-9]* opened' "$LOG" 2>/dev/null; }
closed() { grep -c '^cosim: session [0-9]* closed' "$LOG" 2>/dev/null; }
# settle N [SECONDS]: wait until N sessions have opened and closed.
settle() {
    local n=$1 limit=${2:-120} i
    for i in $(seq $((limit * 20))); do
        [ "$(opened)" -ge "$n" ] && [ "$(closed)" -ge "$n" ] && return 0
        sleep 0.05
    done
    return 1
}
close_line() { grep -m1 "^cosim: session $1 closed" "$LOG"; }

# host SECONDS CMD...: CMD as a host of this section's co-simulation, never a
# board: the stand-in for libftdi serves the link, and it connects only to
# $SOCK. The configuration is this co-simulation's, or HOSTCFG's (none: no
# SYSTOLIC_FPGA_CONFIG at all).
host() {
    local t=$1; shift
    local cfg=(SYSTOLIC_FPGA_CONFIG="${HOSTCFG:-$CFG}")
    [ "${HOSTCFG:-}" = none ] && cfg=()
    env -u SYSTOLIC_FPGA_CONFIG LD_PRELOAD="$SHIM" SYSTOLIC_DPTI_COSIM_SOCKET="$SOCK" "${cfg[@]}" \
        SYSTOLIC_DPTI_TIMEOUT_MS="${SYSTOLIC_DPTI_TIMEOUT_MS:-600000}" \
        timeout -s KILL "$t" "$@"
}
served() { has "$1" "[libftdi_cosim] connected to RTL co-simulation at $SOCK"; }

# replay CAPTURE JOBS ZEROS [K]: as a host, send ZEROS zero bytes, then the
# commands of the first JOBS jobs in CAPTURE (bytes a runtime wrote, each job
# ending with its CONTROL = submit), the last job's k replaced by K when
# given; then leave without reading anything back. Prints one line per job
# and "expect N" -- the bytes those jobs make the RTL send: a 16-byte
# completion record each, plus the result tile (8x8 or 4x4 fp32) unless its
# k is beyond the device's K_MAX.
replay() {
    python3 -I - "$SOCK" "$1" "$2" "$3" "${4:-}" "$CFG" <<'EOF'
import socket, sys, time
sock, cap, jobs, zeros, newk, cfg = sys.argv[1:7]
jobs, zeros = int(jobs), int(zeros)
kmax = {}
for line in open(cfg):
    key, _, val = line.strip().partition("=")
    if key in ("K_MAX_8X8", "K_MAX_4X4"):
        kmax[key] = int(val)
data = bytearray(open(cap, "rb").read())
i, seen, out, regs, expect = 0, 0, bytearray(), {}, 0
while seen < jobs:
    if i >= len(data):
        sys.exit("replay: %s holds only %d job(s)" % (cap, seen))
    op = data[i]
    if op == 0x01:
        cmd = data[i:i + 6]
        addr, val = cmd[1], int.from_bytes(cmd[2:6], "little")
        if addr == 0x14 and newk and seen == jobs - 1:
            val = int(newk)
            cmd[2:6] = val.to_bytes(4, "little")
        regs[addr] = val
        out += cmd
        i += 6
        if addr == 0x00 and (val & 1):
            dev, k = regs.get(0x08, 0), regs.get(0x14, 0)
            cap_k = kmax["K_MAX_8X8" if dev == 0 else "K_MAX_4X4"]
            payload = (256 if dev == 0 else 64) if 1 <= k <= cap_k else 0
            expect += 16 + payload
            print("job %d: device %d k %d -> %d B" % (seen, dev, k, 16 + payload))
            seen += 1
    elif op == 0x02:
        n = int.from_bytes(data[i + 5:i + 9], "little")
        out += data[i:i + 9 + n]
        i += 9 + n
    else:
        sys.exit("replay: byte 0x%02x at offset %d of %s is no opcode" % (op, i, cap))
s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
for _ in range(600):
    try:
        s.connect(sock)
        break
    except OSError:
        time.sleep(0.1)
else:
    sys.exit("replay: no co-simulation on " + sock)
s.sendall(bytes(zeros) + out)
s.close()
print("sent %d B (%d zero bytes, %d B of commands); expect %d" % (zeros + len(out), zeros, len(out), expect))
EOF
}

# dispatch N WHAT: runtime-dispatch-test as session N, which must PASS.
dispatch() {
    local out rc
    out=$(host 900 "$BUILD/bin/runtime-dispatch-test" 2>&1); rc=$?
    echo "$out" > "$D/dispatch_s$1.log"
    settle "$1"
    if [ $rc -eq 0 ] && has "$out" "RUNTIME DISPATCH: PASS" && served "$out"; then
        ok "bench: $2: runtime-dispatch-test PASS (session $1)"
    else
        bad "bench: $2: runtime-dispatch-test (session $1, exit $rc)" \
            "$(grep -E 'RUNTIME DISPATCH|rc=-|out of step|does not match' <<< "$out" | head -4)"
    fi
}

# left N CAPTURE JOBS ZEROS [K] WHAT: a replayed host as session N; the
# session must close at once with exactly what its jobs owe dropped.
left() {
    local n=$1 cap=$2 jobs=$3 zeros=$4 k=$5 what=$6 out want line
    out=$(replay "$cap" "$jobs" "$zeros" $k 2>&1) || { bad "bench: $what: replay" "$out"; return; }
    echo "$out" > "$D/replay_s$n.log"
    want=$(sed -nE 's/.*; expect ([0-9]+)$/\1/p' <<< "$out")
    if ! settle "$n" 30; then
        bad "bench: $what: session $n did not close within 30 s of the host leaving" "$(tail -2 "$LOG")"
        settle "$n" 900
    fi
    line=$(close_line "$n")
    if has "$line" "fpga->host $want B (" && has "$line" "after the host left, dropped)" && ! has "$line" "gave up"; then
        ok "bench: $what: session $n closed once the RTL had sent the $want B its jobs owed"
    else
        bad "bench: $what: session $n should drop exactly $want B" "$line"
    fi
}

section_bench() {
    D="$OUT/bench"
    start_cosim "$D" || { bad "bench: the co-simulation did not open $SOCK"; return; }
    local out rc kmax8
    # Session 1: a full run, every byte the runtime writes recorded.
    out=$(SYSTOLIC_DPTI_COSIM_CAPTURE="$D/capture.bin" host 900 "$BUILD/bin/runtime-dispatch-test" 2>&1); rc=$?
    echo "$out" > "$D/dispatch_s1.log"
    settle 1
    if [ $rc -eq 0 ] && has "$out" "RUNTIME DISPATCH: PASS" && served "$out" && [ -s "$D/capture.bin" ]; then
        ok "bench: runtime-dispatch-test PASS on a fresh co-simulation (session 1, its bytes recorded)"
    else
        bad "bench: runtime-dispatch-test on a fresh co-simulation (exit $rc)" "$(tail -3 <<< "$out")"
        stop_cosim; return
    fi
    left 2 "$D/capture.bin" 1 0 "" "a host left with one job in flight"
    dispatch 3 "after it"
    left 4 "$D/capture.bin" 3 0 "" "a host left with three jobs in flight (8x8 and 4x4)"
    dispatch 5 "after it"
    left 6 "$D/capture.bin" 1 64 "" "64 bytes that are no opcode, then one job in flight"
    dispatch 7 "after it"
    # runtime-dispatch-test's first job is a full 8x8 tile on device 0.
    kmax8=$(sed -nE 's/^K_MAX_8X8=([0-9]+)$/\1/p' "$CFG")
    left 8 "$D/capture.bin" 1 0 "$((kmax8 + 1))" "one job the RTL refuses (k = K_MAX + 1)"
    dispatch 9 "after it"
    stop_cosim
}

section_modef() {
    D="$OUT/modef"
    start_cosim "$D" || { bad "modef: the co-simulation did not open $SOCK"; return; }
    local out rc shadow
    # On the co-simulation (the default backend with the socket set) F runs
    # its injected link failure and PASSes.
    out=$(host 900 "$BUILD/bin/runtime-async-test" --mode F 2>&1); rc=$?
    echo "$out" > "$D/F_cosim.log"
    [ $rc -eq 0 ] && served "$out" && has "$out" "backend: cosim (SYSTOLIC_DPTI_COSIM_SOCKET is set)" \
        && grep -q '^mode F (.*): PASS$' <<< "$out" && has "$out" "RUNTIME ASYNC: PASS" \
        && ok "modef: on the co-simulation mode F runs its injected link failure and PASSes" \
        || bad "modef: mode F on the co-simulation (exit $rc)" "$(grep -E '^(backend|mode F|RUNTIME)' <<< "$out")"

    # The board's run, reproduced: the co-simulation standing for the board,
    # no write can be made to fail. Every other mode and case must PASS, F is
    # NOT APPLICABLE -- reported, never counted as a pass.
    out=$(SYSTOLIC_DPTI_COSIM_NO_FAULTS=1 host 1800 bash "$HERE/run_e2e.sh" --backend board --mode async \
              --quick --build-dir "$BUILD" --out "$D/board_model" 2>&1); rc=$?
    echo "$out" > "$D/board_model.log"
    [ $rc -eq 0 ] && grep -q '^   mode F (.*): NOT APPLICABLE -- ' <<< "$out" \
        && has "$out" "dispatch failure reported: NOT APPLICABLE on the board" \
        && grep -q '^E2E cases: 26/26 PASS on backend board (async runtime)$' <<< "$out" \
        && grep -q '^runtime layer: PASS (runtime-dispatch-test and runtime-async-test; mode F NOT APPLICABLE on the board' <<< "$out" \
        && grep -q '^E2E: PASS on backend board, async runtime .*; runtime-async-test mode F NOT APPLICABLE (co-simulation only)$' <<< "$out" \
        && ok "modef: board model (no fault injection): E2E PASS, 26/26, mode F NOT APPLICABLE" \
        || bad "modef: board model, run_e2e.sh --backend board --mode async --quick (exit $rc)" \
               "$(grep -E '^(runtime layer|E2E|dispatch failure)|^   mode F' <<< "$out")"

    # NOT APPLICABLE never hides a failure: one read byte flipped in every
    # session, on the board model.
    out=$(SYSTOLIC_DPTI_COSIM_CORRUPT_READ=40 host 900 "$BUILD/bin/runtime-async-test" --mode all --backend board 2>&1); rc=$?
    echo "$out" > "$D/corrupt_tool.log"
    [ $rc -ne 0 ] && [ $rc -ne 2 ] && grep -q '^RUNTIME ASYNC: FAIL -- .*; NOT APPLICABLE 1 (mode F: co-simulation only)$' <<< "$out" \
        && grep -q '^mode A (.*): MISMATCH' <<< "$out" \
        && ok "modef: a corrupted result on the board model: runtime-async-test FAIL (exit $rc), F's NOT APPLICABLE does not make it a pass" \
        || bad "modef: a corrupted result, runtime-async-test --backend board (exit $rc)" "$(grep -E '^(mode A|RUNTIME)' <<< "$out")"
    out=$(SYSTOLIC_DPTI_COSIM_CORRUPT_READ=40 host 1800 bash "$HERE/run_e2e.sh" --backend board --mode async \
              --quick --cases 4x4x4_mixed --build-dir "$BUILD" --out "$D/corrupt_harness" 2>&1); rc=$?
    echo "$out" > "$D/corrupt_harness.log"
    [ $rc -eq 1 ] && grep -q '^runtime layer: FAIL -- MISMATCH' <<< "$out" && grep -q '^E2E: FAIL -- runtime layer MISMATCH' <<< "$out" \
        && ok "modef: a corrupted result on the board model: runtime layer FAIL, E2E FAIL" \
        || bad "modef: a corrupted result, run_e2e.sh --backend board (exit $rc)" "$(grep -E '^(runtime layer|E2E)' <<< "$out")"
    stop_cosim

    # On the co-simulation the harness requires F to PASS: a runtime-async-test
    # that takes the co-simulation for the board (F NOT APPLICABLE there) must
    # fail the runtime layer. run_e2e.sh --backend cosim starts its own
    # co-simulation, on a socket in its --out directory: it runs in $SOCKDIR
    # (sun_path, as above), and its record is then kept in $D/guard.
    shadow="$D/shadow/bin"
    mkdir -p "$shadow"
    ln -sf "$BUILD/bin/systolic-matmul" "$shadow/systolic-matmul"
    ln -sf "$BUILD/bin/runtime-dispatch-test" "$shadow/runtime-dispatch-test"
    cat > "$shadow/runtime-async-test" <<EOF
#!/usr/bin/env bash
# A runtime-async-test that takes the co-simulation for the board:
# --backend cosim becomes --backend board.
args=()
for a in "\$@"; do [ "\$a" = cosim ] && a=board; args+=("\$a"); done
exec "$BUILD/bin/runtime-async-test" "\${args[@]}"
EOF
    chmod +x "$shadow/runtime-async-test"
    out=$(SYSTOLIC_DPTI_TIMEOUT_MS=600000 timeout -s KILL 1800 bash "$HERE/run_e2e.sh" --backend cosim --mode async \
              --quick --cases 4x4x4_mixed --build-dir "$D/shadow" --cosim-dir "$COSIM_DIR" --out "$SOCKDIR/guard" 2>&1); rc=$?
    echo "$out" > "$D/guard.log"
    [ ! -f "$D/guard/.run_e2e_out" ] || rm -rf -- "${D:?}/guard"
    mv -T -- "$SOCKDIR/guard" "$D/guard" || bad "modef: the guard run's record could not be moved from $SOCKDIR/guard to $D/guard"
    [ $rc -eq 1 ] && grep -q '^runtime layer: FAIL -- runtime-async-test FAIL (exit 0): mode F did not PASS on the co-simulation' <<< "$out" \
        && grep -q '^E2E: FAIL -- runtime layer FAIL' <<< "$out" \
        && ok "modef: on the co-simulation, mode F NOT APPLICABLE fails the runtime layer" \
        || bad "modef: run_e2e.sh --backend cosim with F NOT APPLICABLE (exit $rc)" "$(grep -E '^(runtime layer|E2E)' <<< "$out")"
}

# build_cc SRC OUT FLAGS...: systolic-cc for this co-simulation's bitstream.
build_cc() {
    local src=$1 out=$2; shift 2
    [ -x "$CLANG" ] && [ -d "$LLVM_BUILD" ] || { echo "no clang / LLVM build in $BUILD/CMakeCache.txt"; return 1; }
    "$BUILD/bin/systolic-cc" --runtime=board --fpga-config "$CFG" --clang "$CLANG" --build-dir "$BUILD" \
        --llvm-build "$LLVM_BUILD" "$@" "$src" -o "$out" > "$out.cc.log" 2>&1
}

section_negoffset() {
    D="$OUT/negoffset"
    mkdir -p "$D"
    local src="$ROOT/test/SystolicCC/negoffset.c" sub out rc n before
    "$CLANG" -std=c11 "$src" -o "$D/cpu" && "$D/cpu" > "$D/cpu.out" || { bad "negoffset: the clang build"; return; }
    for sub in sync async; do
        build_cc "$src" "$D/neg.$sub" --submission "$sub" --fp-reassociation --verify \
            || { bad "negoffset: systolic-cc --submission $sub" "$(tail -3 "$D/neg.$sub.cc.log")"; continue; }
        start_cosim "$D/run.$sub" || { bad "negoffset: the co-simulation did not open $SOCK"; continue; }
        out=$(host 900 "$D/neg.$sub" 2> "$D/neg.$sub.err"); rc=$?
        echo "$out" > "$D/neg.$sub.out"
        settle 1
        # Seven calls reach a row before an argument's first element, six do not
        # (seven GEMM executions, gemm_mixed running two).
        if [ $rc -eq 0 ] && grep -v '^\[systolic-cc\]' <<< "$out" | diff -q "$D/cpu.out" - > /dev/null \
           && [ "$(grep -c '^\[systolic-cc\] FPGA Executed' <<< "$out")" = 7 ] \
           && [ "$(grep -c '^\[systolic-cc\] CPU Executed' <<< "$out")" = 8 ] \
           && [ "$(grep -c ': verified: .* (bitwise)$' "$D/neg.$sub.err")" = 6 ] \
           && [ "$(grep -c ': ran on the CPU: ' "$D/neg.$sub.err")" = 7 ] \
           && ! grep -qi mismatch "$D/neg.$sub.err" && [ "$(opened)" = 1 ]; then
            ok "negoffset ($sub): 7 GEMMs on the FPGA verified bitwise, 8 on the CPU, one session, output equal to clang's"
        else
            bad "negoffset ($sub) (exit $rc, $(opened) session(s))" \
                "$(grep -E 'Executed' <<< "$out" | sort | uniq -c; tail -2 "$D/neg.$sub.err")"
        fi
        stop_cosim
        # A link write that fails: the calls from then on run on the CPU, and the
        # result is still the program's. A failed write can leave a command cut
        # in the RTL, so each such run has a co-simulation of its own.
        for n in 3 40; do
            start_cosim "$D/fail$n.$sub" || { bad "negoffset: the co-simulation did not open $SOCK"; continue; }
            out=$(SYSTOLIC_DPTI_COSIM_FAIL_WRITE=$n host 900 "$D/neg.$sub" 2> "$D/neg.$sub.fail$n.err"); rc=$?
            echo "$out" > "$D/neg.$sub.fail$n.out"
            if [ $rc -eq 0 ] && grep -v '^\[systolic-cc\]' <<< "$out" | diff -q "$D/cpu.out" - > /dev/null \
               && grep -q 'injected failure of write' "$D/neg.$sub.fail$n.err" \
               && ! grep -qi mismatch "$D/neg.$sub.fail$n.err"; then
                ok "negoffset ($sub): link write $n fails: output equal to clang's"
            else
                bad "negoffset ($sub): link write $n fails (exit $rc)" "$(tail -3 "$D/neg.$sub.fail$n.err")"
            fi
            stop_cosim
        done
    done
}

section_kmax() {
    D="$OUT/kmax"
    mkdir -p "$D"
    local src="$ROOT/test/e2e/systolic_cc/kmax_mismatch.c" sub out rc t0 secs
    sed -E 's/^K_MAX_8X8=.*/K_MAX_8X8=32/; s/^K_MAX_4X4=.*/K_MAX_4X4=32/' "$CFG" > "$D/fpga_config_k32.txt"
    "$CLANG" -std=c11 "$src" -o "$D/cpu" && "$D/cpu" > "$D/cpu.out" || { bad "kmax: the clang build"; return; }
    for sub in async sync; do
        "$BUILD/bin/systolic-cc" --runtime=board --fpga-config "$D/fpga_config_k32.txt" --clang "$CLANG" \
            --build-dir "$BUILD" --llvm-build "$LLVM_BUILD" --submission "$sub" --fp-reassociation \
            "$src" -o "$D/k32.$sub" > "$D/k32.$sub.cc.log" 2>&1 \
            || { bad "kmax: systolic-cc --submission $sub" "$(tail -3 "$D/k32.$sub.cc.log")"; continue; }
        grep -q 'k = 32' "$D/k32.$sub.systolic/40-systolic.mlir" 2>/dev/null \
            || grep -q 'k = 32' "$D/k32.$sub.systolic/20-systolic.mlir" \
            || { bad "kmax: the $sub build folds no k = 32"; continue; }
        start_cosim "$D/run.$sub" || { bad "kmax: the co-simulation did not open $SOCK"; continue; }
        t0=$(date +%s)
        # The run-time configuration is this co-simulation's: 16 / 16.
        out=$(host 180 "$D/k32.$sub" 2> "$D/k32.$sub.err"); rc=$?
        secs=$(( $(date +%s) - t0 ))
        echo "$out" > "$D/k32.$sub.out"
        if [ $rc -eq 0 ] && grep -v '^\[systolic-cc\]' <<< "$out" | diff -q "$D/cpu.out" - > /dev/null \
           && [ "$(grep -c '^\[systolic-cc\] CPU Executed' <<< "$out")" = 4 ] \
           && ! grep -q '^\[systolic-cc\] FPGA Executed' <<< "$out"; then
            ok "kmax ($sub): built for K_MAX 32, run on 16: four GEMMs on the CPU, output equal to clang's, ${secs} s"
        else
            bad "kmax ($sub): built for K_MAX 32, run on 16 (exit $rc$( [ $rc -eq 137 ] && echo ': killed at the 180 s bound'), ${secs} s)" \
                "$(grep 'Executed' <<< "$out"; tail -2 "$D/k32.$sub.err")"
        fi
        stop_cosim
    done
}

section_views() {
    D="$OUT/views"
    mkdir -p "$D"
    local src="$ROOT/test/SystolicCC/async_views.c" out rc
    "$CLANG" -std=c11 "$src" -o "$D/cpu" && "$D/cpu" > "$D/cpu.out" || { bad "views: the clang build"; return; }
    build_cc "$src" "$D/views" --fp-reassociation --verify \
        || { bad "views: systolic-cc" "$(tail -3 "$D/views.cc.log")"; return; }
    grep -q 'llvm.call @fpga_tile_submit' "$D/views.systolic/50-fpga.mlir" \
        || { bad "views: the build does not use the asynchronous runtime"; return; }
    start_cosim "$D/run" || { bad "views: the co-simulation did not open $SOCK"; return; }
    out=$(host 900 "$D/views" 2> "$D/views.err"); rc=$?
    echo "$out" > "$D/views.out"
    if [ $rc -eq 0 ] && grep -v '^\[systolic-cc\]' <<< "$out" | diff -q "$D/cpu.out" - > /dev/null \
       && [ "$(grep -c '^\[systolic-cc\] FPGA Executed' <<< "$out")" = 6 ] \
       && ! grep -q '^\[systolic-cc\] CPU Executed' <<< "$out" \
       && [ "$(grep -c ': verified: .* (bitwise)$' "$D/views.err")" = 5 ] \
       && ! grep -q 'refused at submit' "$D/views.err" && ! grep -qi mismatch "$D/views.err"; then
        ok "views: six GEMMs on the FPGA, five calls verified bitwise, no refusal, output equal to clang's"
    else
        bad "views (exit $rc)" "$(grep 'Executed' <<< "$out"; grep -E 'refused|verified|MISMATCH' "$D/views.err" | head -4)"
    fi
    stop_cosim
}

section_header() {
    D="$OUT/header"
    mkdir -p "$D"
    local out rc n first noh="$D/fpga_config_no_header.txt" src="$ROOT/test/SystolicCC/program.c"
    # This co-simulation's configuration without its COMPLETION_HEADER line.
    grep -v '^COMPLETION_HEADER=' "$CFG" > "$noh"
    start_cosim "$D/run" || { bad "header: the co-simulation did not open $SOCK"; return; }
    for n in 1 2; do
        out=$(HOSTCFG="$noh" host 900 "$BUILD/bin/runtime-dispatch-test" 2>&1); rc=$?
        echo "$out" > "$D/no_header_$n.log"
        settle "$n"
        first=$(grep -m1 -oE 'rc=-?[0-9]+' <<< "$out")
        if [ $rc -ne 0 ] && served "$out" && [ "$first" = "rc=-9" ] \
           && has "$out" "does not say COMPLETION_HEADER=1" && ! grep -qE 'rc=0 +MISMATCH' <<< "$out"; then
            ok "header: no COMPLETION_HEADER=1, run $n: the first result refused (rc -9, the configuration named), no record read as a tile"
        else
            bad "header: no COMPLETION_HEADER=1, run $n (exit $rc, first ${first:-none})" \
                "$(grep -E 'rc=|COMPLETION|RUNTIME DISPATCH' <<< "$out" | head -4)"
        fi
    done
    out=$(host 900 "$BUILD/bin/runtime-dispatch-test" 2>&1); rc=$?
    echo "$out" > "$D/with_header.log"
    settle 3
    [ $rc -eq 0 ] && has "$out" "RUNTIME DISPATCH: PASS" \
        && ok "header: then with the co-simulation's configuration: runtime-dispatch-test PASS (session 3)" \
        || bad "header: with the co-simulation's configuration (exit $rc)" "$(tail -2 <<< "$out")"
    # A systolic-cc program built and run without any configuration: the
    # built-in default of the compiler and of the synchronous runtime.
    "$CLANG" -std=c11 "$src" -o "$D/cpu" && "$D/cpu" > "$D/cpu.out" || { bad "header: the clang build"; stop_cosim; return; }
    "$BUILD/bin/systolic-cc" --runtime=board --clang "$CLANG" --build-dir "$BUILD" --llvm-build "$LLVM_BUILD" \
        --fp-reassociation "$src" -o "$D/program" > "$D/program.cc.log" 2>&1 \
        || { bad "header: systolic-cc without a configuration" "$(tail -3 "$D/program.cc.log")"; stop_cosim; return; }
    out=$(HOSTCFG=none host 900 "$D/program" 2> "$D/program.err"); rc=$?
    echo "$out" > "$D/program.out"
    settle 4
    if [ $rc -eq 0 ] && grep -v '^\[systolic-cc\]' <<< "$out" | diff -q "$D/cpu.out" - > /dev/null \
       && grep -q '^\[systolic-cc\] CPU Executed' <<< "$out" && ! grep -q '^\[systolic-cc\] FPGA Executed' <<< "$out" \
       && grep -q 'does not say COMPLETION_HEADER=1' "$D/program.err"; then
        ok "header: a systolic-cc program with no configuration: refused at its first result, run on the CPU, output equal to clang's"
    else
        bad "header: a systolic-cc program with no configuration (exit $rc)" \
            "$(grep 'Executed' <<< "$out" | head -3; grep -m2 -E 'COMPLETION|rc=' "$D/program.err")"
    fi
    stop_cosim
    if [ -z "$PRERECORD" ]; then
        skip "header: a header-less configuration on RTL without the record (no --prerecord-cosim-dir)"
        return
    fi
    if grep -qx 'COMPLETION_HEADER=1' "$PRERECORD/fpga_config.txt"; then
        bad "header: $PRERECORD is a co-simulation of RTL with the record"
        return
    fi
    start_cosim "$D/prerecord" "$PRERECORD/Vcosim" || { bad "header: the pre-record co-simulation did not open $SOCK"; return; }
    out=$(HOSTCFG="$PRERECORD/fpga_config.txt" host 900 "$BUILD/bin/runtime-dispatch-test" 2>&1); rc=$?
    echo "$out" > "$D/prerecord.log"
    settle 1
    [ $rc -eq 0 ] && has "$out" "RUNTIME DISPATCH: PASS" && ! has "$out" "does not say COMPLETION_HEADER=1" \
        && ok "header: RTL without the record, header-less configuration: runtime-dispatch-test PASS (no false refusal)" \
        || bad "header: RTL without the record (exit $rc)" "$(grep -E 'rc=-|RUNTIME DISPATCH' <<< "$out" | head -3)"
    stop_cosim
}

# first_job_id CAPTURE: the job_id of the first descriptor in the bytes a
# runtime wrote (WRITE32 to JOB_ID, 0x04), decimal.
first_job_id() {
    python3 -I - "$1" <<'EOF'
import sys
data = open(sys.argv[1], "rb").read()
i = 0
while i < len(data):
    if data[i] == 0x01:
        if data[i + 1] == 0x04:
            print(int.from_bytes(data[i + 2:i + 6], "little"))
            sys.exit(0)
        i += 6
    elif data[i] == 0x02:
        i += 9 + int.from_bytes(data[i + 5:i + 9], "little")
    else:
        sys.exit("capture: byte 0x%02x at %d is no opcode" % (data[i], i))
sys.exit("capture: no descriptor")
EOF
}

# stale_result JOB_ID: device 0's completion record for JOB_ID and its 8x8
# result (every element 2.0) -- what a program that left with that job in
# flight leaves on the link.
stale_result() {
    python3 -I - "$1" <<'EOF'
import struct, sys
job = int(sys.argv[1])
record = struct.pack("<IIBBHI", 0x59534A43, job, 0, 0, 256, 7)
sys.stdout.buffer.write(record + struct.pack("<64f", *([2.0] * 64)))
EOF
}

section_jobid() {
    D="$OUT/jobid"
    mkdir -p "$D"
    local out rc j first i epochs n
    start_cosim "$D/run" || { bad "jobid: the co-simulation did not open $SOCK"; return; }
    # The synchronous runtime: program 1 (epoch 5) runs; its first result,
    # left on the link, starts program 2's session (epoch 6).
    out=$(SYSTOLIC_DPTI_EPOCH=5 SYSTOLIC_DPTI_COSIM_CAPTURE="$D/sync1.bin" host 900 "$BUILD/bin/runtime-dispatch-test" 2>&1); rc=$?
    echo "$out" > "$D/sync1.log"
    settle 1
    j=$(first_job_id "$D/sync1.bin") && stale_result "$j" > "$D/stale_sync.bin" \
        || { bad "jobid: program 1 (exit $rc)" "$(tail -2 <<< "$out")"; stop_cosim; return; }
    out=$(SYSTOLIC_DPTI_EPOCH=6 SYSTOLIC_DPTI_COSIM_INJECT_RX="$D/stale_sync.bin" host 900 "$BUILD/bin/runtime-dispatch-test" 2>&1); rc=$?
    echo "$out" > "$D/sync2.log"
    settle 2
    first=$(grep -m1 -oE 'rc=-?[0-9]+' <<< "$out")
    if [ "$first" = "rc=-9" ] && has "$out" "does not match invocation" && ! grep -qE 'rc=0 +MISMATCH' <<< "$out"; then
        ok "jobid: synchronous: program 1's result (job_id $j) at program 2's start is refused (rc -9), not taken as its own"
    else
        bad "jobid: synchronous: program 1's result (job_id $j) at program 2's start (first ${first:-none})" \
            "$(grep -E 'rc=|does not match|RUNTIME' <<< "$out" | head -4)"
    fi
    # The asynchronous runtime, the same way.
    out=$(SYSTOLIC_DPTI_EPOCH=5 SYSTOLIC_DPTI_COSIM_CAPTURE="$D/async1.bin" host 900 "$BUILD/bin/runtime-async-test" --mode A 2>&1); rc=$?
    echo "$out" > "$D/async1.log"
    settle 3
    j=$(first_job_id "$D/async1.bin") && stale_result "$j" > "$D/stale_async.bin" \
        || { bad "jobid: asynchronous program 1 (exit $rc)" "$(tail -2 <<< "$out")"; stop_cosim; return; }
    out=$(SYSTOLIC_DPTI_EPOCH=6 SYSTOLIC_DPTI_COSIM_INJECT_RX="$D/stale_async.bin" host 900 "$BUILD/bin/runtime-async-test" --mode A 2>&1); rc=$?
    echo "$out" > "$D/async2.log"
    settle 4
    if [ $rc -eq 0 ] && grep -q '^mode A (.*): PASS$' <<< "$out" && has "$out" "injected bytes"; then
        printf -v j '0x%08x' "$j"
        ok "jobid: asynchronous: program 1's result (job_id $j) at program 2's start is ignored; program 2 PASS"
    else
        bad "jobid: asynchronous: program 1's result at program 2's start (exit $rc)" "$(grep -E '^mode A|RUNTIME|mismatch' <<< "$out" | head -3)"
    fi
    # Four programs without SYSTOLIC_DPTI_EPOCH.
    epochs=""
    for i in 1 2 3 4; do
        out=$(SYSTOLIC_DPTI_COSIM_CAPTURE="$D/random$i.bin" host 900 "$BUILD/bin/runtime-dispatch-test" 2>&1)
        settle $((4 + i))
        j=$(first_job_id "$D/random$i.bin") || j=-16777216
        epochs="$epochs $((j >> 24))"
    done
    n=$(tr ' ' '\n' <<< "$epochs" | grep -v '^$' | sort -u | wc -l)
    if [ "$n" -gt 1 ] && ! tr ' ' '\n' <<< "$epochs" | grep -qxE '0|255|-1'; then
        ok "jobid: four programs in a row: epochs$epochs (in 1..254, not all one)"
    else
        bad "jobid: four programs in a row: epochs$epochs (expected 1..254, not all one)"
    fi
    stop_cosim
}

for s in $(tr ',' ' ' <<< "$SECTIONS"); do
    echo "== $s"
    "section_$s"
done

echo
echo "run_cosim_regressions: $passed passed, $failed failed$( [ $skipped -gt 0 ] && echo ", $skipped skipped")"
[ $failed -eq 0 ]
