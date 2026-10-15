#!/usr/bin/env python3
"""
test_permissions.py -- checks that every path which creates a test source
(.c / .mlir) leaves it read-only (mode 0555) with unchanged content, and that
nothing else's permissions move: the Python tools, the directories, the JSON
and Markdown metadata.

    python3 scripts/test_permissions.py

Everything runs in temporary directories (a temporary registry and coverage
file for the generator), so the real showcase set, generated/ and
regression/ are not touched; the tools' and directories' modes under
test/gemm_recognition are snapshotted before and compared after.

Paths exercised:
  generator/gen_cases.py     --mode random | enumerate | boundary
  generator/make_showcase.py a fresh set, then the same set again (unchanged),
                             then a changed file without / with --force
  casespec.write_test_source the helper itself (unchanged / refuse / force)
  scripts/loop.py            write_failure() (the record's program and its
                             minimized variant) and add_to_corpus()

Exit status 0 when every check passed, 1 otherwise.  Nothing is reported
as passed without having run.
"""

import json
import os
import shutil
import stat
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, ".."))
REPO = os.path.abspath(os.path.join(ROOT, "..", ".."))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(ROOT, "generator"))
import casespec  # noqa: E402
import gen_cases  # noqa: E402
import loop  # noqa: E402
import stages  # noqa: E402

PY = sys.executable
GEN = os.path.join(ROOT, "generator", "gen_cases.py")
MAKE = os.path.join(ROOT, "generator", "make_showcase.py")
results = []


def check(name, ok, detail=""):
    results.append((name, bool(ok), detail))
    print("%-4s %s%s" % ("ok" if ok else "FAIL", name, (" -- " + detail) if detail else ""))
    return ok


def mode(path):
    return stat.S_IMODE(os.stat(path).st_mode)


def is_writable_mode(path):
    return bool(mode(path) & stat.S_IWUSR)


def snapshot_modes():
    """Modes of every tool file and every directory under test/gemm_recognition."""
    snap = {}
    for dirpath, dirnames, filenames in os.walk(ROOT):
        dirnames[:] = [d for d in dirnames if d != "__pycache__"]
        snap[dirpath] = mode(dirpath)
        for fn in filenames:
            if fn.endswith((".py", ".sh", ".cfg")):
                p = os.path.join(dirpath, fn)
                snap[p] = mode(p)
    return snap


def sources_in(d):
    out = []
    for dirpath, _, filenames in os.walk(d):
        for fn in filenames:
            if fn.endswith((".c", ".cpp", ".mlir")):
                out.append(os.path.join(dirpath, fn))
    return sorted(out)


def run(cmd, cwd=None):
    r = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True)
    return r.returncode, r.stdout, r.stderr


def test_generator(tmp):
    reg = os.path.join(tmp, "registry.json")
    cov = os.path.join(tmp, "coverage.json")
    for mode_name, seed, count in (("random", "perm", 12), ("enumerate", "perm", 12), ("boundary", "perm", 8)):
        out = os.path.join(tmp, "gen_" + mode_name)
        rc, so, se = run([PY, GEN, "--mode", mode_name, "--seed", seed, "--count", str(count), "--out", out,
                          "--registry", reg, "--coverage", cov])
        if not check("gen_cases --mode %s runs" % mode_name, rc == 0, se.strip()[-200:]):
            continue
        srcs = sources_in(out)
        check("gen_cases --mode %s wrote %d case files" % (mode_name, count), len(srcs) == count, "%d files" % len(srcs))
        bad = [p for p in srcs if mode(p) != 0o555]
        check("gen_cases --mode %s: every .c is mode 0555" % mode_name, not bad, "; ".join(os.path.basename(b) + " %o" % mode(b) for b in bad))
        meta = [os.path.join(out, "manifest.json"), os.path.join(out, "rejected.json"), reg, cov]
        check("gen_cases --mode %s: JSON metadata stays writable" % mode_name,
              all(os.path.exists(m) and is_writable_mode(m) for m in meta),
              "; ".join("%s %o" % (os.path.basename(m), mode(m)) for m in meta if os.path.exists(m)))
        check("gen_cases --mode %s: output directory mode is the default (not 0555)" % mode_name,
              is_writable_mode(out), "%o" % mode(out))
        # content: the file equals a fresh --reproduce (the chmod changed nothing)
        m = json.load(open(os.path.join(out, "manifest.json")))
        same = True
        for e in m["cases"][:2]:
            # the recorded command: python3 generator/gen_cases.py --family F --reproduce MODE:SEED:INDEX
            argv = e["generated_from"]["reproduce"].split()[2:]
            rc, so, se = run([PY, GEN] + argv)
            text = open(os.path.join(out, e["file"])).read()
            if rc != 0 or so != text:
                same = False
        check("gen_cases --mode %s: file content equals --reproduce output" % mode_name, same)
    # a second run into the same directory with the same indices: the files are
    # identical, so they are left alone (no PermissionError), still 0555
    out = os.path.join(tmp, "gen_random")
    rc, so, se = run([PY, GEN, "--mode", "random", "--seed", "perm", "--count", "12", "--out", out,
                      "--no-registry", "--coverage", cov])
    check("gen_cases regenerating identical files into the same directory succeeds", rc == 0, se.strip()[-200:])
    check("... and they are still mode 0555", all(mode(p) == 0o555 for p in sources_in(out)))


def test_make_showcase(tmp):
    out = os.path.join(tmp, "showcase")
    rc, so, se = run([PY, MAKE, "--out", out])
    if not check("make_showcase writes a fresh set", rc == 0, se.strip()[-200:]):
        return
    srcs = sources_in(out)
    # the number of case files the specs define (C cases + MLIR-level cases)
    sys.path.insert(0, os.path.join(ROOT, "generator"))
    import showcase_cases  # noqa: E402
    n_cases = len(showcase_cases.ALL) + len(showcase_cases.MLIR_CASES)
    check("make_showcase: %d case files (.c and .mlir)" % n_cases, len(srcs) == n_cases, "%d" % len(srcs))
    check("make_showcase: the MLIR-level cases are written too",
          sum(p.endswith(".mlir") for p in srcs) == len(showcase_cases.MLIR_CASES))
    check("make_showcase: every case file is mode 0555", all(mode(p) == 0o555 for p in srcs))
    check("make_showcase: manifest.json / CASES.md stay writable",
          is_writable_mode(os.path.join(out, "manifest.json")) and is_writable_mode(os.path.join(out, "CASES.md")))
    check("make_showcase: set directories keep the default mode",
          all(is_writable_mode(os.path.join(out, d)) for d in ("positive", "negative", "extended")))
    before = {p: open(p).read() for p in srcs}
    rc, so, se = run([PY, MAKE, "--out", out])
    check("make_showcase over the existing set: succeeds without --force", rc == 0, se.strip()[-200:])
    check("... reports every case file unchanged", "%d unchanged" % n_cases in so, so.strip()[-120:])
    check("... content untouched", all(open(p).read() == before[p] for p in srcs))
    check("... still mode 0555", all(mode(p) == 0o555 for p in srcs))
    # a case file whose content would change: refused without --force, replaced with it
    victim = os.path.join(out, "positive", "case_001.c")
    os.chmod(victim, 0o644)
    with open(victim, "a") as f:
        f.write("/* local edit */\n")
    os.chmod(victim, 0o555)
    rc, so, se = run([PY, MAKE, "--out", out])
    check("make_showcase refuses to replace a changed protected case file", rc != 0 and "protected test source" in se, se.strip()[-160:])
    check("... and left the file as it was", open(victim).read() == before[victim] + "/* local edit */\n")
    rc, so, se = run([PY, MAKE, "--out", out, "--force"])
    check("make_showcase --force replaces it", rc == 0 and open(victim).read() == before[victim], se.strip()[-160:])
    check("... and the replacement is mode 0555", mode(victim) == 0o555)


def test_helper(tmp):
    p = os.path.join(tmp, "helper.c")
    check("write_test_source: first write -> 'written', 0555",
          casespec.write_test_source(p, "int x;\n") == "written" and mode(p) == 0o555)
    os.chmod(p, 0o644)
    check("write_test_source: identical content -> 'unchanged' and mode restored to 0555",
          casespec.write_test_source(p, "int x;\n") == "unchanged" and mode(p) == 0o555)
    refused = False
    try:
        casespec.write_test_source(p, "int y;\n")
    except casespec.ProtectedTestSource:
        refused = True
    check("write_test_source: different content without force -> ProtectedTestSource", refused and open(p).read() == "int x;\n")
    check("write_test_source: force -> replaced, 0555",
          casespec.write_test_source(p, "int y;\n", force=True) == "written" and open(p).read() == "int y;\n" and mode(p) == 0o555)
    if os.geteuid() == 0:
        print("     (running as root: the kernel ignores mode bits for root, so 'cannot open for writing' is not checked here)")
    else:
        denied = False
        try:
            open(p, "w")
        except PermissionError:
            denied = True
        check("a 0555 test source cannot be opened for writing", denied)


def test_loop_paths(tmp):
    root = os.path.join(tmp, "root")
    os.makedirs(os.path.join(root, "failures"))
    os.makedirs(os.path.join(root, "regression"))
    case, fam, mut, tag = gen_cases.build_one("random", "mixed", "perm-loop", 3, 64, 8)
    case.id = "gen_" + gen_cases.signature(case)
    gen_cases.finalize(case)
    batch = os.path.join(tmp, "batch")
    os.makedirs(batch)
    src = os.path.join(batch, case.id + ".c")
    casespec.write_test_source(src, casespec.c_file_text(case))
    entry = casespec.manifest_entry(case, "batch")
    entry["file"] = case.id + ".c"
    entry["generated_from"] = {"mode": "random", "seed": "perm-loop", "index": 3, "family": fam, "mutation": mut,
                               "reproduce": "python3 generator/gen_cases.py --reproduce " + tag}
    # a minimized variant: the first shrink candidate
    cands = loop.shrink_candidates(case)
    mcase = cands[0][1] if cands else case
    casespec.analyse(mcase); casespec.predict(mcase); casespec.spec_expect(mcase); casespec.classify_case(mcase)
    mentry = casespec.manifest_entry(mcase, "min")
    tools = stages.Tools(REPO)
    d = loop.write_failure(root, "20260101-000000", case, entry, "fail_runtime", {"driver": "offloaded"}, ["x"], [],
                           {"source": src}, src, (mcase, mentry, ["shrink"], 1), "soft", tools)
    rec = os.path.join(d, case.id + ".c")
    mini = os.path.join(d, "minimized", mcase.id + ".c")
    check("loop.write_failure: the record's program is mode 0555", os.path.exists(rec) and mode(rec) == 0o555)
    check("loop.write_failure: the record's program equals the batch file", open(rec).read() == open(src).read())
    check("loop.write_failure: the minimized program is mode 0555", os.path.exists(mini) and mode(mini) == 0o555)
    check("loop.write_failure: REPORT.md / JSON stay writable",
          all(is_writable_mode(os.path.join(d, f)) for f in ("REPORT.md", "manifest_entry.json", "stages.json")))
    check("loop.write_failure: the record directory keeps the default mode", is_writable_mode(d))
    loop.add_to_corpus(root, case, entry, src, d)
    cp = os.path.join(root, "regression", "corpus", case.id + ".c")
    check("loop.add_to_corpus: the corpus file is mode 0555", os.path.exists(cp) and mode(cp) == 0o555)
    check("loop.add_to_corpus: corpus manifest.json stays writable",
          is_writable_mode(os.path.join(root, "regression", "corpus", "manifest.json")))
    try:
        loop.add_to_corpus(root, case, entry, src, d)
        again = True
    except Exception as e:  # noqa: BLE001
        again = False
        print("     add_to_corpus second call raised: %s" % e)
    check("loop.add_to_corpus: adding the same case again does not fail (existing file kept)", again and mode(cp) == 0o555)


def main():
    before = snapshot_modes()
    tmp = tempfile.mkdtemp(prefix="gemm_perm_test-")
    try:
        test_helper(tmp)
        test_generator(tmp)
        test_make_showcase(tmp)
        test_loop_paths(tmp)
    finally:
        shutil.rmtree(tmp, ignore_errors=True)
    after = snapshot_modes()
    changed = sorted(p for p in before if p in after and before[p] != after[p])
    missing = sorted(p for p in before if p not in after)
    check("Python / shell tools and every directory under test/gemm_recognition keep their modes",
          not changed and not missing, "; ".join(changed + missing)[:300])
    nfail = sum(1 for _, ok, _ in results if not ok)
    print("\n%d check(s), %d failed" % (len(results), nfail))
    return 1 if nfail else 0


if __name__ == "__main__":
    sys.exit(main())
