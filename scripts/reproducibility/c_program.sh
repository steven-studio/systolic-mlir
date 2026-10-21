# c_program.sh -- compile a user's C program with systolic-cc for the real
# runtime and run it; shared by run_board.sh and run_cosim.sh (sourced after
# their env file, so SYSTOLIC_FPGA_CONFIG names the target's configuration).
#
# repro_c_program BACKEND FILE.c OUTDIR [RUNNER...]
#   BACKEND  board | cosim (only for the report)
#   RUNNER   the command prefix the program runs under, e.g.
#            env LD_PRELOAD=<shim> SYSTOLIC_DPTI_COSIM_SOCKET=<socket>
# Leaves OUTDIR/{PROGRAM,compile.log,stdout.txt,run.log}. Returns the
# program's exit status, or 3 when no GEMM ran on the (real or simulated)
# FPGA, or 1 when the compilation failed.
repro_c_program() {
    local backend="$1" src="$2" out="$3"; shift 3
    local name; name="$(basename "$src" .c)"
    mkdir -p "$out"
    echo "== compile $src  (systolic-cc --runtime=board --fp-reassociation)"
    echo "   SYSTOLIC_FPGA_CONFIG=$SYSTOLIC_FPGA_CONFIG"
    if ! "$SYSTOLIC_CC" --runtime=board --fp-reassociation --explain \
            --keep-dir "$out/$name.systolic" "$src" -o "$out/$name" > "$out/compile.log" 2>&1; then
        cat "$out/compile.log" >&2
        echo "COMPILE FAILED: $src (log: $out/compile.log)" >&2
        return 1
    fi
    grep -E "is offloaded|stays on the host|wrote " "$out/compile.log" | sed 's/^/   /'
    echo "   libftdi the program loads by itself: $(ldd "$out/$name" | grep -o 'libftdi1[^ ]* => [^ ]*' || echo none)"
    echo "== run ($backend)"
    local rc
    "$@" env SYSTOLIC_RUNTIME_TRACE=1 SYSTOLIC_RUNTIME_TIMING=1 "$out/$name" \
        > "$out/stdout.txt" 2> "$out/run.log"
    rc=$?
    cat "$out/stdout.txt"
    local fpga cpu jobs
    fpga="$(grep -c '^\[systolic-cc\] FPGA Executed' "$out/stdout.txt")"
    cpu="$(grep -c '^\[systolic-cc\] CPU Executed' "$out/stdout.txt")"
    jobs="$(grep -c -E '^\[fpga-runtime\] job|^\[fpga-async\] accumulate' "$out/run.log")"
    echo "== result: exit $rc; GEMMs on the FPGA: $fpga, on the CPU: $cpu; runtime invocations traced: $jobs"
    grep -E '^\[fpga-(runtime|async)\] timing' "$out/run.log" | sed 's/^/   /'
    # The co-simulation's libftdi stand-in announces itself; the board's
    # libftdi1 prints nothing.
    if grep -q '^\[libftdi_cosim\] connected' "$out/run.log"; then
        echo "   link: $(grep -m1 '^\[libftdi_cosim\] connected' "$out/run.log")"
    else
        echo "   link: system libftdi1 (no [libftdi_cosim] line: not the co-simulation)"
    fi
    echo "   (stdout: $out/stdout.txt, runtime trace: $out/run.log)"
    if [ "$fpga" -eq 0 ]; then
        echo "NOT ON THE FPGA: no GEMM reported 'FPGA Executed' (see $out/run.log)" >&2
        [ $rc -eq 0 ] && return 3
    fi
    return $rc
}
