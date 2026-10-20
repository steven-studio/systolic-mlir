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
#
# Exit status 0 only if every check passes.
set -uo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/../.." && pwd)"
BUILD="$ROOT/build"
COSIM_DIR=""
OUT=""
SECTIONS="bench"
while [ $# -gt 0 ]; do
    case "$1" in
        --build-dir) BUILD="$(cd "$2" && pwd)" || exit 2; shift 2 ;;
        --cosim-dir) COSIM_DIR="$(cd "$2" && pwd)" || exit 2; shift 2 ;;
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
        bench) ;;
        *) echo "unknown section: $s (sections: bench)" >&2; exit 2 ;;
    esac
done

VCOSIM="$COSIM_DIR/Vcosim"
SHIM="$COSIM_DIR/libftdi_cosim.so"
CFG="$COSIM_DIR/fpga_config.txt"
[ -x "$VCOSIM" ] && [ -f "$SHIM" ] && [ -f "$CFG" ] || {
    echo "no co-simulation in $COSIM_DIR -- run test/e2e/cosim/build.sh" >&2; exit 2; }
grep -qx 'COMPLETION_HEADER=1' "$CFG" || {
    echo "$CFG: this co-simulation's RTL sends no completion record; these checks need the working tree's RTL (test/e2e/cosim/build.sh)" >&2; exit 2; }
for t in runtime-dispatch-test; do
    [ -x "$BUILD/bin/$t" ] || { echo "no $BUILD/bin/$t -- build the project first" >&2; exit 2; }
done
command -v python3 >/dev/null || { echo "python3 not found" >&2; exit 2; }
mkdir -p "$OUT" || exit 2

passed=0; failed=0
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
cleanup() { stop_cosim; rmdir -- "${SOCKDIR:?}" 2>/dev/null; }
trap cleanup EXIT

# start_cosim DIR: a fresh co-simulation, logging to DIR/cosim.log.
start_cosim() {
    mkdir -p "$1"
    LOG="$1/cosim.log"
    "$VCOSIM" +socket="$SOCK" +max_sessions=0 \
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
# $SOCK.
host() {
    local t=$1; shift
    LD_PRELOAD="$SHIM" SYSTOLIC_DPTI_COSIM_SOCKET="$SOCK" SYSTOLIC_FPGA_CONFIG="$CFG" \
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

for s in $(tr ',' ' ' <<< "$SECTIONS"); do
    echo "== $s"
    "section_$s"
done

echo
echo "run_cosim_regressions: $passed passed, $failed failed"
[ $failed -eq 0 ]
