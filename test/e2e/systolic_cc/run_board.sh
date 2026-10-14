#!/usr/bin/env bash
#
# run_board.sh (--compile-only | --board-is-free) [PROGRAM.c ...]
#
# C programs compiled by systolic-cc for the FPGA (--runtime=board --verify)
# and run on it: every offloaded call goes through the real runtime, DPTI and
# the programmed 1x8x8 + 3x4x4 bitstream, and is checked against the same
# function compiled for the CPU. The default programs are test/SystolicCC's,
# whose compiler correctness lit already checks on the software runtime
# model; this script is the hardware run of the same binaries' code.
#
#   --compile-only    compile and link for the board; run nothing, open no link
#   --board-is-free   also run them -- only when nothing else is using the board
#                     and the FT2232H link
#
# Needs SYSTOLIC_FPGA_CONFIG (the fpga_config.txt of the bitstream on the
# board; its BITSTREAM_SHA256 is checked against its BITSTREAM) and
# SYSTOLIC_CFRONT_CLANG (clang 24).
#
# Each program is classified by its exit status, so that the link and the
# compiler are never blamed for each other:
#   PASS       0  every offloaded call equal to the CPU reference
#   TRANSPORT  3  an FPGA invocation failed (runtime / DPTI); the shim stops
#                 before using the incomplete result -- not a compiler result
#   MISMATCH   4  the FPGA path's result differs from the CPU reference
#   FAIL       anything else
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
                                       "$ROOT/test/SystolicCC/faults.c")

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
  if ! "$CC" --runtime=board --verify "$src" -o "$out/$name" \
      > "$out/compile.log" 2>&1; then
    echo "  COMPILE   $name  (see $out/compile.log)"
    status=1
    continue
  fi
  if [ "$MODE" = --compile-only ]; then
    echo "  BUILT     $name  ($(grep -c 'is offloaded' "$out/compile.log") offloaded function(s); not run)"
    continue
  fi
  SYSTOLIC_RUNTIME_TRACE=1 "$out/$name" > "$out/stdout.txt" 2> "$out/run.log"
  rc=$?
  case $rc in
    0) verdict=PASS ;;
    3) verdict=TRANSPORT; status=1 ;;
    4) verdict=MISMATCH; status=1 ;;
    *) verdict=FAIL; status=1 ;;
  esac
  printf '  %-9s %s  (exit %d; %s hardware invocations; %s)\n' "$verdict" "$name" \
    "$rc" "$(grep -c '^\[fpga-runtime\] job' "$out/run.log")" \
    "$(grep -c 'verified:' "$out/run.log") verified call(s)"
done
exit $status
