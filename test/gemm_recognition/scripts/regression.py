#!/usr/bin/env python3
"""
regression.py -- the regression gate of the closed loop.

    python3 scripts/regression.py run  [--runtime soft|board] [--jobs J] [tool options]
    python3 scripts/regression.py save [--runtime soft|board]
    python3 scripts/regression.py diff [--runtime soft|board]

`run` executes, through every compiler stage (scripts/stages.py) and judges
(scripts/verdict.py):

  1. every showcase case (showcase/manifest.json: the hundred + extended);
  2. every case of regression/corpus/ (programs the loop found failing);
  3. every issue reproducer (failures/issues/ISSUE-*/variants.mlir and
     repro_*.mlir, through scripts/run_issue.sh);

and writes regression/last_run_<runtime>.json, then compares it with
regression/baseline_<runtime>.json: a case whose verdict got worse (pass ->
anything else, known_false_negative -> fail) is a REGRESSION; a case whose
verdict got better (known_false_negative -> pass, i.e. a fixed issue) is
reported as IMPROVED and needs the issue record updated.  Exit status 1 on
any regression or any failing verdict that is not in the baseline.

`save` copies the last run into the baseline -- only after a human has
looked at the diff.  `diff` prints the comparison again.

The existing lit suite is run separately (cmake --build build --target
check-systolic); its count is recorded here for the report when
--check-systolic is given.
"""

import argparse
import datetime
import json
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, ".."))
REPO = os.path.abspath(os.path.join(ROOT, "..", ".."))
sys.path.insert(0, HERE)
import stages  # noqa: E402
import verdict  # noqa: E402

RANK = {"pass": 0, "fixed_issue": 0, "known_false_negative": 1, "unexpected_stage": 2, "false_positive": 3,
        "fail_runtime": 3, "test_bug": 3, "tool_error": 4}


def load(path, default):
    try:
        with open(path) as f:
            return json.load(f)
    except (OSError, ValueError):
        return default


def collect_cases():
    out = []
    m = load(os.path.join(ROOT, "showcase", "manifest.json"), {"showcase": [], "extended": []})
    for e in m["showcase"] + m["extended"]:
        out.append(("showcase", e, os.path.join(ROOT, "showcase", e["file"])))
    c = load(os.path.join(ROOT, "regression", "corpus", "manifest.json"), {"cases": []})
    for e in c["cases"]:
        out.append(("corpus", e, os.path.join(ROOT, "regression", "corpus", e["file"])))
    return out


def run(a, tools, env):
    cases = collect_cases()
    work = a.work or tempfile.mkdtemp(prefix="gemm_regression-")
    results = {}
    for origin, e, src in cases:
        wd = os.path.join(work, e["id"])
        st = stages.run_case(tools, src, e["kernel"], wd, runtime=a.runtime, env=env)
        v, actual, mism, notes = verdict.classify(e, st)
        results[e["id"]] = {"origin": origin, "verdict": v, "classification": e.get("classification"),
                            "issue": e.get("issue"), "mismatches": mism, "notes": notes,
                            "actual": {k: actual.get(k) for k in ("driver", "recognition", "dense_partition",
                                                                  "lowering", "execute", "invocations")}}
        print("%-7s %-18s %-22s %s" % (origin, e["id"], v, "; ".join(mism)[:120]))
        sys.stdout.flush()
    issues = {}
    for d in sorted(os.listdir(os.path.join(ROOT, "failures", "issues"))) if os.path.isdir(os.path.join(ROOT, "failures", "issues")) else []:
        if not d.startswith("ISSUE-"):
            continue
        issue = d.split("-")[0] + "-" + d.split("-")[1]
        r = subprocess.run([os.path.join(HERE, "run_issue.sh"), issue, "--opt", tools.opt,
                            "--filecheck", os.path.join(tools.llvm_build, "bin", "FileCheck")],
                           capture_output=True, text=True)
        passed = r.stdout.count("-> PASSED")
        failed = r.stdout.count("-> FAILED")
        issues[issue] = {"repro_passed": passed, "repro_failed": failed, "rc": r.returncode,
                         "status": "still_open" if failed else ("fixed?" if passed else "no_repro")}
        print("%-7s %-18s repro passed=%d failed=%d" % ("issue", issue, passed, failed))
    out = {"kind": "regression_run", "runtime": a.runtime, "timestamp": datetime.datetime.now().strftime("%Y%m%d-%H%M%S"),
           "tools": tools.describe(), "cases": results, "issues": issues}
    if a.check_systolic:
        r = subprocess.run(["cmake", "--build", os.path.join(tools.build_dir), "--target", "check-systolic"],
                           capture_output=True, text=True)
        tail = (r.stdout + r.stderr).strip().splitlines()[-12:]
        out["check_systolic"] = {"rc": r.returncode, "tail": tail}
        print("check-systolic rc=%d" % r.returncode)
        for ln in tail:
            print("   " + ln)
    os.makedirs(os.path.join(ROOT, "regression"), exist_ok=True)
    with open(os.path.join(ROOT, "regression", "last_run_%s.json" % a.runtime), "w") as f:
        json.dump(out, f, indent=1)
    return out


def diff(a, last=None):
    last = last or load(os.path.join(ROOT, "regression", "last_run_%s.json" % a.runtime), None)
    base = load(os.path.join(ROOT, "regression", "baseline_%s.json" % a.runtime), None)
    if not last:
        print("no last run for runtime %s" % a.runtime)
        return 2
    regressions, improvements, new_bad, same = [], [], [], 0
    for cid, r in last["cases"].items():
        v = r["verdict"]
        if base and cid in base["cases"]:
            b = base["cases"][cid]["verdict"]
            if RANK.get(v, 9) > RANK.get(b, 9):
                regressions.append((cid, b, v, "; ".join(r["mismatches"])[:160]))
            elif RANK.get(v, 9) < RANK.get(b, 9):
                improvements.append((cid, b, v))
            else:
                same += 1
        elif v != "pass":
            new_bad.append((cid, v, "; ".join(r["mismatches"])[:160]))
        else:
            same += 1
    if base:
        for issue, st in last.get("issues", {}).items():
            b = base.get("issues", {}).get(issue)
            if b and st["repro_failed"] > b["repro_failed"]:
                regressions.append((issue, "repro failed=%d" % b["repro_failed"], "repro failed=%d" % st["repro_failed"], ""))
            if b and st["repro_failed"] < b["repro_failed"]:
                improvements.append((issue, "repro failed=%d" % b["repro_failed"], "repro failed=%d" % st["repro_failed"]))
    print("baseline: %s; unchanged: %d" % ("none" if not base else base["timestamp"], same))
    for cid, b, v, m in regressions:
        print("REGRESSION  %-18s %s -> %s  %s" % (cid, b, v, m))
    for cid, v, m in new_bad:
        print("NEW FAILURE %-18s %s  %s" % (cid, v, m))
    for cid, b, v in improvements:
        print("IMPROVED    %-18s %s -> %s  (update the issue record, then `regression.py save`)" % (cid, b, v))
    if last.get("check_systolic"):
        print("check-systolic rc=%d" % last["check_systolic"]["rc"])
        if last["check_systolic"]["rc"] != 0:
            new_bad.append(("check-systolic", "failed", ""))
    return 1 if regressions or new_bad else 0


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("command", choices=["run", "save", "diff"])
    ap.add_argument("--runtime", default="soft", choices=["soft", "board"])
    ap.add_argument("--build-dir")
    ap.add_argument("--llvm-build")
    ap.add_argument("--clang")
    ap.add_argument("--clang-major", type=int)
    ap.add_argument("--work")
    ap.add_argument("--check-systolic", action="store_true", help="also run cmake --build build --target check-systolic")
    a = ap.parse_args()
    if a.command == "save":
        last = load(os.path.join(ROOT, "regression", "last_run_%s.json" % a.runtime), None)
        if not last:
            print("no last run to save", file=sys.stderr)
            return 2
        with open(os.path.join(ROOT, "regression", "baseline_%s.json" % a.runtime), "w") as f:
            json.dump(last, f, indent=1)
        print("baseline_%s.json saved from the run of %s" % (a.runtime, last["timestamp"]))
        return 0
    if a.command == "diff":
        return diff(a)
    tools = stages.Tools(REPO, a.build_dir, a.llvm_build, a.clang, a.clang_major)
    missing = tools.missing()
    if missing:
        print("missing tools: " + ", ".join(missing), file=sys.stderr)
        return 2
    env = dict(os.environ)
    if a.runtime == "board" and not env.get("SYSTOLIC_FPGA_CONFIG"):
        print("--runtime board needs SYSTOLIC_FPGA_CONFIG", file=sys.stderr)
        return 2
    last = run(a, tools, env)
    return diff(a, last)


if __name__ == "__main__":
    sys.exit(main())
