#!/usr/bin/env bash
# run_issue.sh ISSUE-NNN [--opt systolic-opt] [--filecheck FileCheck]
#
# Re-run the reproducers of one recorded issue (failures/issues/ISSUE-NNN-*/)
# and record the actual output next to them as actual_<host>_<date>.txt:
#   variants.mlir      every variant through --test-affine-matmul-discovery
#   repro_*.mlir       the lit test's RUN lines, one by one, expectations checked
# The issue counts as fixed only when every reproducer shows the specified
# behaviour; the record (ISSUE-NNN.md) is then updated by hand.
set -uo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/.." && pwd)"
REPO="$(cd "$ROOT/../.." && pwd)"
ISSUE="${1:?usage: run_issue.sh ISSUE-NNN [--opt systolic-opt] [--filecheck FileCheck]}"
shift
OPT="${SYSTOLIC_OPT:-$REPO/build/bin/systolic-opt}"
FILECHECK="${FILECHECK:-$(command -v FileCheck || echo "${LLVM_BUILD:-$HOME/work/llvm-project/build}/bin/FileCheck")}"
while [ $# -gt 0 ]; do
  case "$1" in
    --opt) OPT="$2"; shift 2 ;;
    --filecheck) FILECHECK="$2"; shift 2 ;;
    *) echo "unknown option $1" >&2; exit 2 ;;
  esac
done
DIR="$(ls -d "$ROOT/failures/issues/$ISSUE"-* 2>/dev/null | head -1)"
[ -n "$DIR" ] || { echo "no directory for $ISSUE under $ROOT/failures/issues" >&2; exit 2; }
[ -x "$OPT" ] || { echo "missing $OPT (SYSTOLIC_OPT or --opt)" >&2; exit 2; }
[ -x "$FILECHECK" ] || { echo "missing FileCheck ($FILECHECK; --filecheck or LLVM_BUILD)" >&2; exit 2; }
OUT="$DIR/actual_$(hostname)_$(date +%Y%m%d-%H%M%S).txt"
{
  echo "issue: $ISSUE"
  echo "date: $(date -Is)   host: $(hostname)"
  echo "systolic-opt: $OPT"
  echo "compiler commit: $(git -C "$REPO" rev-parse --short HEAD 2>/dev/null)$(git -C "$REPO" status --porcelain -- lib include tools 2>/dev/null | grep -q . && echo ' (working tree modified)')"
  echo
  if [ -f "$DIR/variants.mlir" ]; then
    echo "== variants.mlir (discovery pass; one '-----' block per variant, in file order) =="
    "$OPT" "--systolic-declare-fpga-fleet=k-max-8x8=16 k-max-4x4=16" --test-affine-matmul-discovery \
      -split-input-file "$DIR/variants.mlir" -o /dev/null 2>&1 \
      | grep -E "recognition result|dense logical GEMM|fleet partition|unique logical" || true
    echo "(the first block is the file's leading comment: 0 candidates)"
    echo
  fi
  for f in "$DIR"/repro_*.mlir; do
    [ -f "$f" ] || continue
    echo "== $(basename "$f") =="
    python3 - "$f" <<'PY' | while IFS= read -r cmd; do
import re, sys
text = open(sys.argv[1]).read()
runs, cur = [], ""
for line in text.splitlines():
    m = re.match(r"^//\s*RUN:\s?(.*)$", line)
    if not m:
        continue
    part = m.group(1)
    if part.rstrip().endswith("\\"):
        cur += part.rstrip()[:-1] + " "
    else:
        runs.append((cur + part).strip())
        cur = ""
for r in runs:
    print(r)
PY
      cmd="${cmd//%s/$f}"
      cmd="${cmd//systolic-opt/$OPT}"
      cmd="${cmd//FileCheck/$FILECHECK}"
      echo "+ $cmd"
      if bash -c "$cmd" > /dev/null 2> "$DIR/.stderr"; then echo "  -> PASSED"; else echo "  -> FAILED"; head -4 "$DIR/.stderr" | sed 's/^/     /'; fi
    done
    rm -f "$DIR/.stderr"
    if grep -q "^// XFAIL" "$f"; then
      echo "  (marked XFAIL: FAILED above is the open issue reproducing; PASSED would mean it is fixed)"
    fi
    echo
  done
} | tee "$OUT"
echo "recorded in $OUT"
