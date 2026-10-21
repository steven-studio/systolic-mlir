#!/usr/bin/env bash
#
# run_cosim.sh COMMAND -- Co-simulation Mode: no FPGA. The same host binaries
# as on the board (systolic-cc output, the unmodified runtime) talk DPTI to a
# Verilator simulation of the bitstream's RTL instead of the FT2232H:
# libftdi_cosim.so is preloaded in front of the system libftdi1 and carries
# the link's bytes over a UNIX socket to Vcosim (test/e2e/cosim), which runs
# systolic_dma_top with the Xilinx IP replaced (fp_ieee.sv for the FP cores,
# tb/mig_axi_model.sv for the MIG/DDR3, tb/xil_stubs.sv for the primitives).
# Environment: env.cosim.sh (works from env -i).
#
#   check                 check_env.sh cosim (no board, USB or Vivado needed)
#   build                 test/e2e/cosim/build.sh -> COSIM_BUILD_DIR
#                         (Vcosim, libftdi_cosim.so, fpga_config.txt)
#   e2e [ARGS...]         test/e2e/run_e2e.sh --backend cosim (default ARGS:
#                         --quick)
#   c FILE.c [OUTDIR]     start Vcosim, compile FILE.c with systolic-cc for the
#                         board runtime, run it against the simulation, stop
#                         Vcosim (default OUTDIR: SYSTOLIC_BUILD/repro/cosim/<name>)
#
# It is a simulation: correctness of the RTL behind the unmodified host
# stack, not evidence about a board or its speed.
set -uo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
. "$HERE/env.cosim.sh"
. "$HERE/c_program.sh"

need_cosim() {
    [ -x "$COSIM_BUILD_DIR/Vcosim" ] && [ -f "$COSIM_BUILD_DIR/libftdi_cosim.so" ] && [ -f "$SYSTOLIC_FPGA_CONFIG" ] || {
        echo "no co-simulation in $COSIM_BUILD_DIR: run scripts/reproducibility/run_cosim.sh build" >&2; return 2; }
}

# A short directory for the socket: AF_UNIX paths end at ~107 bytes.
short_dir() { mktemp -d "/tmp/systolic-cosim.XXXXXX"; }

cmd="${1:-}"
[ $# -gt 0 ] && shift
case "$cmd" in
    check)
        exec bash "$HERE/check_env.sh" cosim ;;
    build)
        command -v verilator >/dev/null 2>&1 || { echo "verilator not installed (>= 5.0; Debian/Ubuntu: apt install verilator)" >&2; exit 127; }
        bash "$REPRO_ROOT/test/e2e/cosim/build.sh" ;;
    e2e)
        repro_need_build || exit 2
        need_cosim || exit 2
        [ $# -gt 0 ] || set -- --quick
        # run_e2e.sh puts its socket at OUT/dpti.sock; keep OUT short.
        out="$SYSTOLIC_BUILD/e2e/cosim"
        if [ ${#out} -gt 90 ]; then out="$(short_dir)/e2e"; fi
        cd "$REPRO_ROOT" && bash test/e2e/run_e2e.sh --backend cosim --build-dir "$SYSTOLIC_BUILD" \
            --cosim-dir "$COSIM_BUILD_DIR" --out "$out" "$@" ;;
    c)
        repro_need_build || exit 2
        need_cosim || exit 2
        [ $# -ge 1 ] || { echo "usage: $0 c FILE.c [OUTDIR]" >&2; exit 2; }
        src="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
        out="${2:-$SYSTOLIC_BUILD/repro/cosim/$(basename "$1" .c)}"
        mkdir -p "$out"
        sockdir="$(short_dir)"
        sock="$sockdir/dpti.sock"
        "$COSIM_BUILD_DIR/Vcosim" +socket="$sock" +max_sessions=0 \
            > >(grep --line-buffered -v -E '^(JOBFIREDBG|SCHEDHS|SCHTRACE|JOBHS) ' > "$out/cosim.log") 2>&1 &
        vpid=$!
        trap 'kill "$vpid" 2>/dev/null; rm -f "$sock"; rmdir "$sockdir" 2>/dev/null' EXIT
        for _ in $(seq 100); do [ -S "$sock" ] && break; sleep 0.1; done
        [ -S "$sock" ] || { echo "Vcosim did not open $sock (log: $out/cosim.log)" >&2; exit 1; }
        echo "Vcosim (pid $vpid) simulating $(cat "$COSIM_BUILD_DIR/RTL_SOURCE" 2>/dev/null), socket $sock"
        repro_c_program cosim "$src" "$out" \
            env LD_PRELOAD="$COSIM_BUILD_DIR/libftdi_cosim.so" SYSTOLIC_DPTI_COSIM_SOCKET="$sock" \
                SYSTOLIC_DPTI_TIMEOUT_MS=600000
        rc=$?
        sleep 0.5
        echo "== simulated FPGA (Vcosim, $out/cosim.log)"
        grep -E '^cosim: session [0-9]+ (opened|closed)' "$out/cosim.log" | sed 's/^/   /'
        exit $rc ;;
    *)
        sed -n '3,22p' "$0" | sed 's/^# \{0,1\}//'
        exit 2 ;;
esac
