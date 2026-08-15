#!/usr/bin/env bash
# recover_originals.sh -- find surviving copies of the pre-overwrite sources.
#
# Context: `git checkout run_hls.tcl` failed with "did not match any file(s)
# known to git", which means that file was never tracked. The urgent
# question is whether design.h and design.cpp were also untracked -- if so,
# git cannot restore them either, and the originals only exist in whatever
# side copies happen to survive.
#
# Read-only. Copies candidates into ./_recovered/ and touches nothing else.
#
# Run from ~/systolic-mlir/hls/multi_200t

set -uo pipefail
REPO_ROOT=$(git rev-parse --show-toplevel 2>/dev/null || echo "$HOME/systolic-mlir")
HERE=$(pwd)
DEST="$HERE/_recovered"
mkdir -p "$DEST"

echo "############################################################"
echo "# 1. What does git actually track here?"
echo "############################################################"
# If design.h appears in this list, stop reading -- `git checkout` will fix
# everything and the rest of this script is unnecessary.
git ls-files "$HERE" 2>/dev/null || echo "(not a git repo / nothing tracked)"
echo
echo "--- untracked files in this directory ---"
git status --short --untracked-files=all "$HERE" 2>/dev/null | grep '^??' || echo "(none)"
echo
echo "--- any historical version of these files anywhere in history ---"
for f in design.h design.cpp run_hls.tcl testbench.cpp; do
  echo "== $f =="
  git log --all --oneline --name-only -- "*/$f" 2>/dev/null | head -20 || true
done

echo
echo "############################################################"
echo "# 2. Sibling project directories"
echo "############################################################"
# gemm_8x8 is the most likely place an earlier design.h survived -- same
# geometry, presumably forked from the same source.
for d in "$REPO_ROOT"/hls/gemm_4x4 "$REPO_ROOT"/hls/gemm_4x4_dual \
         "$REPO_ROOT"/hls/gemm_8x8; do
  [ -d "$d" ] || continue
  echo "--- $d ---"
  ls -la "$d" 2>/dev/null | grep -E "design|\.h$|\.cfg$|\.tcl$" || true
  for f in design.h design.cpp run_hls.tcl; do
    if [ -f "$d/$f" ]; then
      cp -n "$d/$f" "$DEST/$(basename "$d")__$f" 2>/dev/null || true
      echo "  saved: $(basename "$d")__$f"
    fi
  done
done

echo
echo "############################################################"
echo "# 3. Preprocessed sources inside the HLS work dirs"
echo "############################################################"
# This is the good one. Vitis HLS keeps a fully preprocessed snapshot of the
# kernel under .autopilot/db/. Macros are already expanded, so it will not
# give you the typedefs verbatim -- but it WILL tell you unambiguously what
# data_t and acc_t resolved to, which is the fact actually needed to
# reconstruct the header. Look for the accumulator's declared type.
find "$HERE" \( -path '*/.autopilot/db/*' -o -name '*.pragma.*.cpp' \
       -o -name 'a.o.*.cpp' -o -name 'apatb_*.cpp' \) -type f 2>/dev/null \
  | head -40 | while read -r p; do
      echo "  $p"
      cp -n "$p" "$DEST/db__$(echo "$p" | tr '/' '_')" 2>/dev/null || true
    done

echo
echo "--- grep those snapshots for the operand/accumulator types ---"
grep -rhoE "(ap_fixed|ap_int|ap_ufixed|half|double|float)[^;,)]{0,40}" \
     "$DEST" 2>/dev/null | sort | uniq -c | sort -rn | head -25 || true

echo
echo "############################################################"
echo "# 4. Directive / solution files (recover the old II + part)"
echo "############################################################"
find "$HERE" \( -name '*.directive' -o -name 'script.tcl' -o -name 'hls.app' \
       -o -name 'solution*.aps' -o -name 'vitis_hls.log' \) -type f 2>/dev/null \
  | head -20 | while read -r p; do
      echo "  $p"
      cp -n "$p" "$DEST/sol__$(echo "$p" | tr '/' '_')" 2>/dev/null || true
    done

echo
echo "--- cflags recorded in the HLS logs (shows the ORIGINAL -D flags) ---"
grep -rhoE "\-D[A-Za-z_][A-Za-z0-9_]*(=[^ ]*)?" "$HERE"/work_8x8*/ 2>/dev/null \
  | sort | uniq -c | sort -rn | head -25 || true

echo
echo "############################################################"
echo "# 5. The old numbers, for comparison"
echo "############################################################"
# These reports predate the rewrite, so they are the fixed-K_DIM baseline --
# i.e. the thing the 2.7x claim is measured against. Worth saving before
# anything overwrites them too.
find "$HERE" -name '*_csynth.rpt' -o -name '*cosim.rpt' 2>/dev/null \
  | head -20 | while read -r p; do
      echo "  $p"
      cp -n "$p" "$DEST/rpt__$(echo "$p" | tr '/' '_')" 2>/dev/null || true
    done

echo
echo "############################################################"
echo "# 6. Long shots"
echo "############################################################"
echo "--- editor backups / swap files ---"
find "$REPO_ROOT" \( -name '*design.h~' -o -name '.*design.h.sw?' \
     -o -name 'design.h.orig' -o -name 'design.h.bak' \) 2>/dev/null | head || echo "(none)"
echo "--- shell history mentions ---"
grep -hE "design\.h|run_hls|K_DIM|acc_t" ~/.bash_history ~/.zsh_history 2>/dev/null \
  | tail -20 || echo "(none)"

echo
echo "############################################################"
echo "Candidates copied to: $DEST"
echo "Nothing was modified or deleted."
echo "############################################################"
