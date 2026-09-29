#!/usr/bin/env bash
# -----------------------------------------------------------------------------
# fix_prj_path.sh -- find (and optionally repoint) stale references to the
# MIG .prj behind:
#
#   CRITICAL WARNING: [Project 1-19] Could not find the file
#   '.../systolic-mlir/systolic-mlir/.../nexys_video_mig_axi128.prj'
#
# HISTORY OF THIS SCRIPT, because it matters
#   v1 looked only in the .xci files.  All three were already correct, so it
#      changed nothing and the warning stayed.
#   v2 searched every text file -- and compared each reference against the
#      absolute path AS A STRING.  Vivado deliberately stores project-relative
#      spellings ($PPRDIR/../../..., ../../../../../...) so a project can be
#      moved, and v2 called every one of them BAD.  Run with --write it would
#      have replaced correct relative paths with machine-specific absolute
#      ones: the project stops warning and stops being portable.  That is the
#      same class of change as setting ReferenceClock to "Use System Clock" --
#      the tool goes quiet and the thing gets worse.
#   v3 (this) RESOLVES each reference before judging it.  $PPRDIR, $PSRCDIR,
#      $PGENDIR are expanded, relative paths are tried against the containing
#      file's directory and the project root, and a reference is only wrong if
#      it resolves to nothing that exists.
#
# DEFAULT IS REPORT-ONLY.  Pass --write to change anything.
#
#   bash fix_prj_path.sh            # report      <- start, and usually finish, here
#   bash fix_prj_path.sh --write    # repoint unresolvable refs in generated files
#
# WHAT --write WILL NOT TOUCH
#   hand-written sources (mig_gen.tcl, the .sv, the .xdc, this file).  Their
#   mentions are comments and prose; editing those to silence a tool is not a
#   fix.
#
# THE HONEST RECOMMENDATION
#   In ddr3_bw.xpr the stale doubled path sits NEXT TO a correct one -- it is a
#   leftover duplicate entry, not a broken pointer.  Deleting the duplicate by
#   hand is more XML surgery than it is worth, and the whole project tree is
#   regenerated from scratch by
#       vivado -mode batch -source bw_build.tcl -tclargs all
#   which drops every stale entry, clears the [IP_Flow 19-12246] checksum
#   warning too, and re-measures -- so a second run either reproduces
#   beta_ceiling = 3.627 or tells you something you need to know.
#   That is the fix.  This script is the diagnostic that found the problem.
# -----------------------------------------------------------------------------
set -euo pipefail

WRITE=0
case "${1:-}" in
    --write)          WRITE=1 ;;
    ""|--dry-run)     WRITE=0 ;;
    *) echo "usage: bash fix_prj_path.sh [--write]" ; exit 2 ;;
esac

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PRJ="$HERE/ip/ddr3/nexys_video_mig_axi128.prj"

if [ ! -f "$PRJ" ]; then
    echo "the .prj is not where it should be either:"
    echo "  $PRJ"
    ls -1 "$HERE/ip/ddr3/" 2>/dev/null || echo "  (no such directory)"
    exit 1
fi
echo "target .prj: $PRJ"

ROOTS=("$HERE")
WRITABLE=()
for d in "$HOME/work/vivado/ddr3_bw" "$HOME/work/vivado/ddr3_calib" \
         "$HOME/work/vivado/mig_spike"; do
    [ -d "$d" ] && { ROOTS+=("$d"); WRITABLE+=("$d"); }
done
echo "searching:"
printf '  %s\n' "${ROOTS[@]}"
[ "$WRITE" = "1" ] && echo "mode: WRITE" || echo "mode: report only (--write to change files)"
echo

PRJ="$PRJ" WRITE="$WRITE" REPO="$HERE" \
WRITABLE="$(printf '%s\n' "${WRITABLE[@]+"${WRITABLE[@]}"}")" \
python3 - "${ROOTS[@]}" <<'PY'
import os, re, shutil, sys

prj      = os.environ["PRJ"]
write    = os.environ["WRITE"] == "1"
repo     = os.environ["REPO"]
writable = [w for w in os.environ["WRITABLE"].split("\n") if w]

pat  = re.compile(r'[^"\'<>\s()]*nexys_video_mig(?:_axi128)?\.prj')
skip = (".log", ".jou", ".str", ".bak", ".dcp", ".bit", ".ltx", ".pb",
        ".zip", ".rpt", ".debug", ".wdb", ".jgz", ".sh", ".md", ".py")

def proj_root(f):
    """nearest ancestor directory holding a .xpr"""
    d = os.path.dirname(f)
    while d and d != "/":
        try:
            if any(x.endswith(".xpr") for x in os.listdir(d)):
                return d
        except OSError:
            pass
        d = os.path.dirname(d)
    return None

def resolves(ref, f):
    """Does this reference name a file that exists?  Vivado stores several
    spellings; try each, and accept if ANY of them lands on a real file."""
    root = proj_root(f)
    name = os.path.basename(root) if root else ""
    subs = {"$PPRDIR": root or "",
            "$PSRCDIR": os.path.join(root, name + ".srcs") if root else "",
            "$PGENDIR": os.path.join(root, name + ".gen") if root else ""}
    r = ref
    for k, v in subs.items():
        if r.startswith(k):
            r = v + r[len(k):]
            break
    if os.path.isabs(r):
        return os.path.isfile(os.path.normpath(r))
    for base in (os.path.dirname(f), root or "", repo):
        if base and os.path.isfile(os.path.normpath(os.path.join(base, r))):
            return True
    return False

def may_write(f):
    if any(os.path.commonpath([f, w]) == w for w in writable):
        return True
    return f == os.path.join(repo, "ip", "ddr3", "mig_7series_0.xci")

seen = bad = held = touched = 0
for root in sys.argv[1:]:
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = [d for d in dirnames if d not in (".git", ".Xil")]
        for fn in sorted(filenames):
            if fn.endswith(skip):
                continue
            f = os.path.join(dirpath, fn)
            try:
                if os.path.getsize(f) > 40 * 1024 * 1024:
                    continue
                src = open(f, encoding="utf-8").read()
            except (OSError, UnicodeDecodeError):
                continue
            hits = sorted(set(pat.findall(src)))
            if not hits:
                continue
            dead = [h for h in hits if not resolves(h, f)]
            seen += 1
            if not dead:
                continue          # every reference here points at a real file
            bad += 1
            print(f"--- {f}")
            for h in hits:
                print(f"    {'ok  ' if h not in dead else 'DEAD'} {h}")
            if not may_write(f):
                print("    hand-written source -- reporting only")
                held += 1
                continue
            if not write:
                print("    (report only; --write to repoint)")
                continue
            shutil.copy2(f, f + ".bak")
            out = src
            for h in dead:
                out = out.replace(h, prj)
            open(f, "w", encoding="utf-8").write(out)
            print(f"    repointed -> {prj}   (backup {fn}.bak)")
            touched += 1

print()
print(f"files referencing the .prj: {seen}   with a reference that resolves "
      f"to nothing: {bad}" + (f"   (source, left alone: {held})" if held else "")
      + (f"   rewritten: {touched}" if write else ""))
if bad == 0:
    print("\nEvery reference resolves to a real file.  Nothing here explains the")
    print("warning any more -- if it persists, it is coming from a binary")
    print("artefact, and a rebuild is the only way out.")
else:
    print("\nThe clean fix for a generated project is not to edit it, but to")
    print("regenerate it -- that drops stale entries AND the 19-12246 checksum")
    print("warning, and re-measures beta_ceiling as a reproducibility check:")
    print("  vivado -mode batch -source bw_build.tcl -tclargs all")
PY
