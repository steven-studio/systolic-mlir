#!/usr/bin/env bash
#
# run_board.sh (--compile-only | --board-is-free) [PROGRAM.c ...]
#
# C programs compiled by systolic-cc for the FPGA (--runtime=board --verify)
# and run on it: every offloaded call goes through the real runtime (the
# asynchronous one on a bitstream with the completion record: every tile
# submitted without waiting for the one before it, one wait per GEMM), DPTI
# and the programmed 1x8x8 + 3x4x4 bitstream, and is checked against the same
# function compiled for the CPU. With LD_PRELOAD=build/cosim/libftdi_cosim.so
# and SYSTOLIC_DPTI_COSIM_SOCKET set the same binaries run on the Verilator
# co-simulation instead (test/e2e/cosim). The default programs are test/SystolicCC's,
# whose compiler correctness lit already checks on the software runtime
# model; this script is the hardware run of the same binaries' code.
#
# Each program prints one "[systolic-cc] FPGA|CPU Executed: GEMM #N" line per
# GEMM. The lines a fully successful FPGA run would print are taken from the
# same program on the software model with SOFT_RUNTIME_MOCK_HARDWARE=1 (no
# board involved); the program's own output, without those lines, must equal
# that of the file compiled by clang alone.
#
#   --compile-only    compile and link for the board; run nothing, open no link
#   --board-is-free   also run them -- only when nothing else is using the board
#                     and the FT2232H link
#
# Needs SYSTOLIC_FPGA_CONFIG (the fpga_config.txt of the bitstream on the
# board; its BITSTREAM_SHA256 is checked against its BITSTREAM) and
# SYSTOLIC_CFRONT_CLANG (clang 24).
#
# Each program is classified by its exit status and its GEMM lines, so that
# the link and the compiler are never blamed for each other:
#   PASS       exit 0, every GEMM where the mock run put it, every offloaded
#              call equal to the CPU reference, output equal to clang's
#   FALLBACK   exit 0, output right, but some GEMM the FPGA should have run
#              reports CPU: an FPGA invocation failed (runtime / DPTI) and the
#              call was redone on the CPU -- not a compiler result
#   MISMATCH   exit 4: a result differs from the CPU reference
#   FAIL       anything else (another exit status, a wrong program output, a
#              GEMM line the mock run does not have)
# Artifacts and logs: build/test-output/systolic_cc_board/<program>/.

set -uo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/../../.." && pwd)"
CC="$ROOT/build/bin/systolic-cc"

MODE="${1:-}"
case "$MODE" in
  --compile-only|--board-is-free) shift ;;
  *) echo "usage: $0 --compile-only | --board-is-free [PROGRAM.c ...]" >&2
     exit 2 ;;
esac
PROGRAMS=("$@")
[ ${#PROGRAMS[@]} -gt 0 ] || PROGRAMS=("$ROOT/test/SystolicCC/program.c"
                                       "$ROOT/test/SystolicCC/faults.c"
                                       "$ROOT/test/SystolicCC/fallback.c")

: "${SYSTOLIC_FPGA_CONFIG:?needs SYSTOLIC_FPGA_CONFIG (the fpga_config.txt of the programmed bitstream)}"
: "${SYSTOLIC_CFRONT_CLANG:?needs SYSTOLIC_CFRONT_CLANG (clang 24)}"

cfgval() { sed -n "s/^[[:space:]]*$1[[:space:]]*=[[:space:]]*//p" "$SYSTOLIC_FPGA_CONFIG" | sed 's/[[:space:]#].*//' | tail -1; }
bit="$(cfgval BITSTREAM)"
sha="$(cfgval BITSTREAM_SHA256 | tr 'A-F' 'a-f')"
case "$bit" in ""|/*) ;; *) bit="$(dirname "$SYSTOLIC_FPGA_CONFIG")/$bit" ;; esac
if [ -n "$bit" ] && [ -f "$bit" ] && [ -n "$sha" ]; then
  got="$(sha256sum "$bit" | cut -d' ' -f1)"
  [ "$got" = "$sha" ] || { echo "BITSTREAM_SHA256 = $sha, but $bit hashes to $got" >&2; exit 2; }
  echo "configuration: $SYSTOLIC_FPGA_CONFIG (bitstream hash matches; what the board holds is trusted, not read back)"
else
  echo "configuration: $SYSTOLIC_FPGA_CONFIG (bitstream hash NOT checked)"
fi

OUT_ROOT="$ROOT/build/test-output/systolic_cc_board"
status=0
for src in "${PROGRAMS[@]}"; do
  name="$(basename "$src" .c)"
  out="$OUT_ROOT/$name"
  mkdir -p "$out"
  if ! "$CC" --runtime=board --verify --explain "$src" -o "$out/$name" \
      > "$out/compile.log" 2>&1 ||
     ! "$CC" --runtime=soft "$src" -o "$out/$name.soft" \
      > "$out/compile.soft.log" 2>&1 ||
     ! "$SYSTOLIC_CFRONT_CLANG" -std=c11 "$src" -o "$out/$name.cpu" \
      > "$out/compile.cpu.log" 2>&1; then
    echo "  COMPILE   $name  (see $out/compile*.log)"
    status=1
    continue
  fi
  # What a fully successful FPGA run prints: the software model, claiming to
  # be the FPGA (no board involved), and clang's build of the file.
  env -u SOFT_RUNTIME_FAULT -u SOFT_RUNTIME_FAULT_AT SOFT_RUNTIME_MOCK_HARDWARE=1 \
      "$out/$name.soft" 2>/dev/null \
    | grep '^\[systolic-cc\]' > "$out/expected-gemms.txt"
  "$out/$name.cpu" > "$out/expected-output.txt" 2>/dev/null
  if [ "$MODE" = --compile-only ]; then
    echo "  BUILT     $name  ($(grep -c 'is offloaded' "$out/compile.log") offloaded function(s); not run)"
    continue
  fi
  SYSTOLIC_RUNTIME_TRACE=1 "$out/$name" > "$out/stdout.txt" 2> "$out/run.log"
  rc=$?
  grep '^\[systolic-cc\]' "$out/stdout.txt" > "$out/gemms.txt"
  output_ok=0
  grep -v '^\[systolic-cc\]' "$out/stdout.txt" | cmp -s - "$out/expected-output.txt" && output_ok=1
  # The same GEMMs in the same order, at most moved from the FPGA to the CPU.
  fell_back=0 lines_ok=1
  if [ "$(wc -l < "$out/gemms.txt")" -ne "$(wc -l < "$out/expected-gemms.txt")" ]; then
    lines_ok=0
  else
    while IFS=$'\t' read -r got want; do
      [ "$got" = "$want" ] && continue
      if [ "$got" = "${want/FPGA Executed/CPU Executed}" ]; then
        fell_back=$((fell_back + 1))
      else
        lines_ok=0
      fi
    done < <(paste "$out/gemms.txt" "$out/expected-gemms.txt")
  fi
  if [ $rc -eq 4 ]; then
    verdict=MISMATCH; status=1
  elif [ $rc -ne 0 ] || [ $output_ok -eq 0 ] || [ $lines_ok -eq 0 ]; then
    verdict=FAIL; status=1
  elif [ $fell_back -gt 0 ]; then
    verdict=FALLBACK; status=1
  else
    verdict=PASS
  fi
  printf '  %-9s %s  (exit %d; GEMMs: %d FPGA, %d CPU, %d fell back to the CPU; %s hardware invocations; %s)\n' \
    "$verdict" "$name" "$rc" "$(grep -c 'FPGA Executed' "$out/gemms.txt")" \
    "$(grep -c 'CPU Executed' "$out/gemms.txt")" "$fell_back" \
    "$(grep -c -E '^\[fpga-runtime\] job|^\[fpga-async\] accumulate' "$out/run.log")" \
    "$(grep -c 'verified:' "$out/run.log") verified call(s)"
done
exit $status
