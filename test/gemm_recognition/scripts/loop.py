#!/usr/bin/env python3
"""
loop.py -- the automated test loop over generated cases:

    generate -> validate -> front end -> recognize -> partition -> lower ->
    compile -> execute -> compare with the independent references ->
    collect failures (seed, reproduction, stage, diagnosis, minimized case)
    -> regression corpus -> coverage -> generate more

    python3 scripts/loop.py [--iterations N] [--batch B] [--mode random|enumerate|boundary]
        [--seed S] [--start I] [--family mixed|positive|views|mutation]
        [--runtime soft|board] [--jobs J] [--build-dir DIR] [--llvm-build DIR]
        [--clang CLANG] [--clang-major N] [--no-minimize] [--max-extent E]

Every iteration generates --batch new cases (generator/gen_cases.py; the
registry under generated/ guarantees they are new), runs every stage on each
(scripts/stages.py), judges each against the specification and the current
implementation (scripts/verdict.py), and then:

  pass                  nothing to do
  known_false_negative  a recorded issue reproduces (e.g. ISSUE-001): appended
                        to failures/known_issue_log.jsonl (seed, index,
                        reproduce command), no new record
  anything else         a NEW finding: a failure record is written under
                        failures/<stamp>_<id>/ with the program, every stage's
                        output, the seed and the exact reproduction commands,
                        a diagnosis (which stage, expected vs actual), and --
                        unless --no-minimize -- a smaller program that still
                        fails the same way; the program is added to
                        regression/corpus/ so that it is re-run forever

Fixing the compiler is a separate, human-approved step (see README.md,
'The closed loop'); after a rebuild, scripts/regression.py re-runs the
original failing case, the whole showcase set, the corpus and the issue
reproducers, and compares with regression/baseline.json.

The loop never edits expectations, never deletes a case and never touches
the compiler.  It stops after --iterations (0: run until interrupted).

Every test source it keeps (the batch's cases, a failure record's program and
its minimized variant, a corpus entry) is read-only (mode 0555); only the
throw-away candidates of the minimizer in the work directory are not.
"""

import argparse
import copy
import datetime
import json
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, ".."))
REPO = os.path.abspath(os.path.join(ROOT, "..", ".."))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(ROOT, "generator"))
import stages  # noqa: E402
import verdict  # noqa: E402
import casespec  # noqa: E402
import gen_cases  # noqa: E402

FAILING = ("fail_runtime", "unexpected_stage", "false_positive", "test_bug", "tool_error", "fixed_issue")


def run_one(tools, entry, src, workdir, runtime, env):
    st = stages.run_case(tools, src, entry["kernel"], workdir, runtime=runtime, env=env)
    v, actual, mism, notes = verdict.classify(entry, st)
    return v, actual, mism, notes, st


def shrink_candidates(case):
    """Smaller variants of a case that keep its structure: halve one loop's
    trip count (the coarse loop of a split coordinate), or drop one
    repetition loop.  Arrays keep their declared dimensions (they remain in
    bounds), so the feature under test is unchanged."""
    out = []
    for i, lp in enumerate(case.loops):
        if not lp.is_box() or any(lp.var in l.bound_vars() for l in case.loops):
            continue    # a loop with bounds over the enclosing counters, or one they read: kept as is
        if lp.role == "R":
            c = copy.deepcopy(case)
            del c.loops[i]
            out.append(("drop repetition loop %s" % lp.var, c))
            continue
        trip = lp.trip()
        if trip <= 1:
            continue
        # keep split coordinates dense: only the coarsest loop of a group may shrink
        group = [l for l in case.loops if l.role == lp.role]
        if len(group) > 1:
            # the coarsest loop is the one with the largest coefficient in C's/A's subscript
            def coef_of(l):
                best = 0
                for arr in case.arrays.values():
                    for f in arr.forms():
                        best = max(best, abs(f.coef.get(l.var, 0)))
                return best
            if coef_of(lp) != max(coef_of(l) for l in group):
                continue
        for new_trip in sorted({1, trip // 2, trip - 1}):
            if new_trip < 1 or new_trip >= trip:
                continue
            c = copy.deepcopy(case)
            l2 = c.loops[i]
            if l2.cmp == "<=":
                l2.ub = l2.lb + l2.step * (new_trip - 1)
            else:
                l2.ub = l2.lb + l2.step * new_trip
            out.append(("loop %s: %d -> %d iterations" % (lp.var, trip, new_trip), c))
    return out


def minimize(tools, case, entry, failing_verdict, failing_stage_key, workdir, runtime, env, budget=12):
    """Greedy shrinking: apply any candidate that keeps the same verdict and
    the same failing stage, until none does or the budget is spent."""
    current = case
    cur_entry = entry
    applied = []
    runs = 0
    while runs < budget:
        progressed = False
        for desc, cand in shrink_candidates(current):
            if runs >= budget:
                break
            try:
                casespec.analyse(cand)
                casespec.predict(cand)
                casespec.spec_expect(cand)
                casespec.classify_case(cand)
                casespec.evaluate(cand)
                text = casespec.c_file_text(cand)
            except Exception:  # noqa: BLE001  (an invalid shrink is skipped)
                continue
            wd = os.path.join(workdir, "min_%d" % runs)
            os.makedirs(wd, exist_ok=True)
            src = os.path.join(wd, cand.id + ".c")
            with open(src, "w") as f:
                f.write(text)
            e = casespec.manifest_entry(cand, "min")
            runs += 1
            v, actual, mism, notes, st = run_one(tools, e, src, wd, runtime, env)
            if v == failing_verdict and failing_stage(e, actual, v) == failing_stage_key:
                current, cur_entry = cand, e
                applied.append(desc)
                progressed = True
                break
        if not progressed:
            break
    return current, cur_entry, applied, runs


def failing_stage(entry, actual, v):
    """The first stage whose measured result deviates: the key the minimizer
    must preserve while shrinking."""
    if v in ("fail_runtime", "test_bug"):
        return "execute"
    cur = entry["current_implementation"]
    for k in ("frontend", "recognition", "dense_partition", "lowering", "driver"):
        if actual.get(k) != cur.get(k):
            return k
    if v == "false_positive":
        return "driver"
    return "stage"


def write_failure(root, stamp, case, entry, v, actual, mism, notes, st, src, minimized, runtime, tools):
    d = os.path.join(root, "failures", "%s_%s" % (stamp, case.id))
    os.makedirs(d, exist_ok=True)
    shutil.copy(src, os.path.join(d, case.id + ".c"))
    casespec.protect_test_source(os.path.join(d, case.id + ".c"))   # read-only test source
    with open(os.path.join(d, "manifest_entry.json"), "w") as f:
        json.dump(entry, f, indent=1)
    with open(os.path.join(d, "stages.json"), "w") as f:
        json.dump(stages.strip_runs(st, keep_output=True), f, indent=1)
    gen = entry.get("generated_from", {})
    lines = ["# Failure record: %s (%s)" % (case.id, v), "",
             "- date: %s" % stamp, "- runtime: %s" % runtime,
             "- compiler: %s" % json.dumps(tools.describe()),
             "- generated by: %s" % gen.get("reproduce", "(showcase/corpus case)"),
             "- family / mutation: %s / %s" % (gen.get("family"), gen.get("mutation")),
             "", "## Verdict", "", "- **%s**" % v]
    lines += ["- " + m for m in mism]
    lines += ["- note: " + n for n in notes]
    lines += ["", "## Diagnosis", "",
              "- specification: recognition %s (%s)" % (entry["expected"]["recognition"], entry["expected"]["basis"][:120]),
              "- current implementation predicted: frontend %s, recognition %s, dense %s, lowering %s, driver %s%s" % (
                  entry["current_implementation"]["frontend"], entry["current_implementation"]["recognition"],
                  entry["current_implementation"]["dense_partition"], entry["current_implementation"]["lowering"],
                  entry["current_implementation"]["driver"],
                  " (%s: %s)" % (entry["current_implementation"]["reject_stage"], entry["current_implementation"]["reject_reason"])
                  if entry["current_implementation"]["reject_stage"] else ""),
              "- measured: " + json.dumps({k: actual.get(k) for k in ("frontend", "recognition", "dense_partition", "lowering",
                                                                       "driver", "execute", "invocations", "planned_per_call")}),
              "", "## Reproduce", "",
              "    cc -std=c11 -O1 %s.c -o %s && ./%s        # the kernel as written vs its references" % (case.id, case.id, case.id),
              "    python3 scripts/run_showcase.py  # (showcase cases)",
              "    python3 scripts/stages.py %s %s/%s.c %s %s   # every stage, JSON" % (REPO, d, case.id, case.kernel, runtime),
              "    %s" % gen.get("reproduce", ""), ""]
    if minimized:
        mcase, mentry, applied, runs = minimized
        lines += ["## Minimized", "",
                  "- %d shrink step(s) applied in %d run(s): %s" % (len(applied), runs, "; ".join(applied) or "none kept the failure"),
                  "- minimized program: minimized/%s.c" % mcase.id, ""]
    with open(os.path.join(d, "REPORT.md"), "w") as f:
        f.write("\n".join(lines) + "\n")
    if minimized and minimized[2]:
        md = os.path.join(d, "minimized")
        os.makedirs(md, exist_ok=True)
        mcase, mentry = minimized[0], minimized[1]
        casespec.write_test_source(os.path.join(md, mcase.id + ".c"), casespec.c_file_text(mcase))
        with open(os.path.join(md, "manifest_entry.json"), "w") as f:
            json.dump(mentry, f, indent=1)
    return d


def add_to_corpus(root, case, entry, src, record_dir):
    cd = os.path.join(root, "regression", "corpus")
    os.makedirs(cd, exist_ok=True)
    dst = os.path.join(cd, case.id + ".c")
    if not os.path.exists(dst):
        shutil.copy(src, dst)
        casespec.protect_test_source(dst)   # read-only test source
    mpath = os.path.join(cd, "manifest.json")
    m = gen_cases.load_json(mpath, {"kind": "regression_corpus", "cases": []})
    if not any(e["id"] == case.id for e in m["cases"]):
        e = dict(entry)
        e["file"] = case.id + ".c"
        e["origin"] = os.path.relpath(record_dir, root)
        m["cases"].append(e)
        gen_cases.save_json(mpath, m)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--iterations", type=int, default=1, help="0 = until interrupted")
    ap.add_argument("--batch", type=int, default=20)
    ap.add_argument("--mode", default="random", choices=["random", "enumerate", "boundary"])
    ap.add_argument("--seed", default=datetime.datetime.now().strftime("%Y%m%d%H%M%S"))
    ap.add_argument("--start", type=int, default=None, help="first generator index (default: continue the registry)")
    ap.add_argument("--family", default="mixed",
                    choices=["mixed", "positive", "views", "mutation", "domains"])
    ap.add_argument("--runtime", default="soft", choices=["soft", "board"])
    ap.add_argument("--jobs", type=int, default=1)
    ap.add_argument("--build-dir")
    ap.add_argument("--llvm-build")
    ap.add_argument("--clang")
    ap.add_argument("--clang-major", type=int)
    ap.add_argument("--no-minimize", action="store_true")
    ap.add_argument("--max-extent", type=int, default=64)
    ap.add_argument("--max-loops", type=int, default=8)
    ap.add_argument("--work", help="where intermediates go (default: a temporary directory)")
    a = ap.parse_args()

    tools = stages.Tools(REPO, a.build_dir, a.llvm_build, a.clang, a.clang_major)
    missing = tools.missing()
    if missing:
        print("missing tools: " + ", ".join(missing), file=sys.stderr)
        return 2
    env = dict(os.environ)
    if a.runtime == "board" and not env.get("SYSTOLIC_FPGA_CONFIG"):
        print("--runtime board needs SYSTOLIC_FPGA_CONFIG", file=sys.stderr)
        return 2
    gen_root = os.path.join(ROOT, "generated")
    os.makedirs(gen_root, exist_ok=True)
    state_path = os.path.join(gen_root, "loop_state.json")
    state = gen_cases.load_json(state_path, {"next_index": {}})
    key = "%s:%s" % (a.mode, a.seed)
    index = a.start if a.start is not None else state["next_index"].get(key, 0)
    workroot = a.work or tempfile.mkdtemp(prefix="gemm_loop-")
    os.makedirs(os.path.join(ROOT, "failures"), exist_ok=True)
    log_path = os.path.join(ROOT, "failures", "known_issue_log.jsonl")
    totals = {}
    it = 0
    while a.iterations == 0 or it < a.iterations:
        it += 1
        stamp = datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
        batch_dir = os.path.join(gen_root, "batch_%s_%s_%d" % (a.mode, a.seed, index))
        cmd = [sys.executable, os.path.join(ROOT, "generator", "gen_cases.py"), "--mode", a.mode, "--seed", str(a.seed),
               "--count", str(a.batch), "--start", str(index), "--family", a.family, "--out", batch_dir,
               "--max-extent", str(a.max_extent), "--max-loops", str(a.max_loops)]
        r = subprocess.run(cmd, capture_output=True, text=True)
        print(r.stdout.strip())
        if r.returncode != 0:
            print(r.stderr, file=sys.stderr)
            return 1
        m = gen_cases.load_json(os.path.join(batch_dir, "manifest.json"), None)
        if not m:
            print("generator wrote no manifest", file=sys.stderr)
            return 1
        index = m["next_index"]
        state["next_index"][key] = index
        gen_cases.save_json(state_path, state)
        results = []
        for e in m["cases"]:
            src = os.path.join(batch_dir, e["file"])
            wd = os.path.join(workroot, e["id"])
            v, actual, mism, notes, st = run_one(tools, e, src, wd, a.runtime, env)
            totals[v] = totals.get(v, 0) + 1
            print("  %-18s %-22s %s" % (e["id"], v, "; ".join(mism)[:140]))
            sys.stdout.flush()
            results.append((e, src, wd, v, actual, mism, notes, st))
            if v == "known_false_negative":
                with open(log_path, "a") as f:
                    f.write(json.dumps({"date": stamp, "id": e["id"], "issue": e.get("issue"),
                                        "reproduce": e["generated_from"]["reproduce"], "runtime": a.runtime,
                                        "mismatch": mism}) + "\n")
            elif v in FAILING:
                # rebuild the Case object to minimize
                case, _, _, _ = gen_cases.build_one(e["generated_from"]["mode"], a.family, e["generated_from"]["seed"],
                                                    e["generated_from"]["index"], a.max_extent, a.max_loops)
                case.id = e["id"]
                gen_cases.finalize(case)
                minimized = None
                if not a.no_minimize and v != "tool_error":
                    stage_key = failing_stage(e, actual, v)
                    mcase, mentry, applied, runs = minimize(tools, case, e, v, stage_key, wd, a.runtime, env)
                    minimized = (mcase, mentry, applied, runs)
                d = write_failure(ROOT, stamp, case, e, v, actual, mism, notes, st, src, minimized, a.runtime, tools)
                add_to_corpus(ROOT, case, e, src, d)
                print("    -> recorded in %s" % os.path.relpath(d, ROOT))
        summary = {"kind": "loop_batch", "stamp": stamp, "mode": a.mode, "seed": a.seed, "runtime": a.runtime,
                   "batch_dir": os.path.relpath(batch_dir, ROOT), "tools": tools.describe(),
                   "verdicts": {}, "cases": []}
        for e, src, wd, v, actual, mism, notes, st in results:
            summary["verdicts"][v] = summary["verdicts"].get(v, 0) + 1
            summary["cases"].append({"id": e["id"], "classification": e["classification"], "verdict": v,
                                     "mismatches": mism, "notes": notes,
                                     "actual": {k: actual.get(k) for k in ("driver", "recognition", "dense_partition",
                                                                           "lowering", "execute", "invocations")}})
        with open(os.path.join(batch_dir, "results_%s_%s.json" % (a.runtime, stamp)), "w") as f:
            json.dump(summary, f, indent=1)
        print("iteration %d: %s" % (it, ", ".join("%s=%d" % kv for kv in sorted(summary["verdicts"].items()))))
    print("totals: " + ", ".join("%s=%d" % kv for kv in sorted(totals.items())))
    if not a.work:
        print("intermediates: %s" % workroot)
    bad = sum(v for k, v in totals.items() if k in FAILING)
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
