#!/usr/bin/env python3
"""
run_showcase.py -- run the fixed showcase set (and the extended cases) through
the compiler, stage by stage, compare every stage with the expectation in
manifest.json, and write a report of what ACTUALLY happened.

    python3 scripts/run_showcase.py [--runtime soft|board] [--case ID ...]
        [--set positive|negative|extended] [--build-dir DIR] [--llvm-build DIR]
        [--clang CLANG] [--clang-major N] [--out reports/] [--jobs N]
        [--cpu-only] [--keep-output]

Per case the stages are (scripts/stages.py): the program compiled by an
ordinary C compiler (the kernel as written against its references); the C
front end on the kernel; --test-affine-matmul-discovery; --affine-matmul-
to-systolic=report=true; systolic-cc --explain --verify with the chosen
runtime; and the compiled program.  The verdicts:

  pass              every stage did what the manifest expects and the
                    program's own check passed (kernel == references)
  unexpected_stage  a stage did not do what was expected (the expectation or
                    the compiler is wrong -- recorded, never auto-resolved)
  fail_runtime      the compiled program's result differs from the references,
                    systolic-cc's --verify reported a mismatch, the runtime
                    reported failed invocations, or the program crashed
  test_bug          the program's references disagree among themselves
  tool_error        a tool is missing or crashed; the case was not tested

The report (reports/showcase_<runtime>_<timestamp>.json and .md, plus
reports/latest_<runtime>.json) is the only place results live; manifest.json
keeps 'tested: false' and the expectations.  --runtime board needs the board
(SYSTOLIC_FPGA_CONFIG) and is the authoritative run; soft is the software
model of the runtime ABI, NOT the FPGA.
"""

import argparse
import concurrent.futures
import datetime
import json
import os
import platform
import shutil
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, ".."))
REPO = os.path.abspath(os.path.join(ROOT, "..", ".."))
sys.path.insert(0, HERE)
import stages  # noqa: E402
from verdict import classify  # noqa: E402

def load_manifest():
    with open(os.path.join(ROOT, "showcase", "manifest.json")) as f:
        return json.load(f)


def select(manifest, ids, sets):
    entries = manifest["showcase"] + manifest["extended"]
    if ids:
        entries = [e for e in entries if e["id"] in ids]
    if sets:
        entries = [e for e in entries if e["set"] in sets]
    return entries


def classify_partial(entry, st, only_stages):
    """Judge only the stages that ran (e.g. front end + discovery on a machine
    without the full tool chain): each measured stage vs the prediction, and
    the recognition vs the specification.  Verdicts: partial_pass,
    known_false_negative, partial_unexpected, tool_error."""
    cur = entry["current_implementation"]
    actual, mism = {}, []
    fe = st.get("frontend")
    if fe:
        if fe.get("result") == "tool_error":
            return "tool_error", {"frontend": fe.get("reason")}, [fe.get("reason", "")], []
        actual["frontend"] = fe.get("result")
        if fe.get("result") != cur["frontend"]:
            mism.append("frontend: predicted %s, got %s (%s)" % (cur["frontend"], fe.get("result"), fe.get("reason", "")))
    di = st.get("discovery")
    if di:
        if di.get("result") == "tool_error":
            return "tool_error", {"discovery": di.get("reason")}, [di.get("reason", "")], []
        exp_rec = "reject" if cur.get("hoisting_required") else cur["recognition"]
        actual["discovery_recognition"] = di.get("recognition")
        actual["discovery_dense"] = di.get("dense")
        actual["discovery_dense_reason"] = di.get("dense_reason")
        actual["fleet_partition"] = di.get("fleet_partition")
        if di.get("recognition") != exp_rec:
            mism.append("discovery recognition: predicted %s, got %s" % (exp_rec, di.get("recognition")))
        elif di.get("recognition") == "accept" and di.get("dense") != cur["dense_partition"]:
            mism.append("discovery dense: predicted %s, got %s (%s)" % (cur["dense_partition"], di.get("dense"), di.get("dense_reason")))
    lo = st.get("lowering")
    if lo:
        actual["lowering_result"] = lo.get("result")
    notes = []
    spec = entry["expected"]
    if di and di.get("recognition") == "reject" and spec["recognition"] == "ACCEPT" and not cur.get("hoisting_required"):
        if not mism:
            return "known_false_negative", actual, ["specification: ACCEPT; discovery: reject (%s)" % (entry.get("issue") or "no issue recorded yet")], notes
    if mism:
        return "partial_unexpected", actual, mism, notes
    return "partial_pass", actual, mism, notes


def one(args):
    entry, tools, runtime, outdir, env, keep_output, cpu_only, only_stages = args
    src = os.path.join(ROOT, "showcase", entry["file"])
    workdir = os.path.join(outdir, entry["id"])
    if os.path.isdir(workdir):
        shutil.rmtree(workdir)
    if entry.get("kind") == "mlir":
        # an MLIR-level case: recognition, dense partition and lowering only;
        # --cpu-only has nothing to run for it, and a --stages subset applies
        # to those two stages at most
        if cpu_only:
            return entry["id"], "not_applicable", {"kind": "mlir"}, [], ["MLIR-level case: no C program to compile"], {}
        st = stages.run_case(tools, src, entry["kernel"], workdir, runtime=runtime, env=env, only=only_stages)
        if only_stages and not ({"discovery", "lowering"} <= set(only_stages)):
            return entry["id"], "not_applicable", {"kind": "mlir"}, [], ["MLIR-level case: needs the discovery and lowering stages"], stages.strip_runs(st, keep_output)
        verdict, actual, mism, notes = classify(entry, st)
        return entry["id"], verdict, actual, mism, notes, stages.strip_runs(st, keep_output)
    if cpu_only:
        os.makedirs(workdir, exist_ok=True)
        st = {"cpu": stages.stage_cpu(src, workdir)}
        verdict = "pass" if st["cpu"]["result"] == "pass" else "test_bug"
        return entry["id"], verdict, {"cpu": st["cpu"]["result"]}, [], [], stages.strip_runs(st, keep_output)
    st = stages.run_case(tools, src, entry["kernel"], workdir, runtime=runtime, env=env, only=only_stages,
                         extra_cc_flags=stages.verify_flags(entry))
    if only_stages and set(only_stages) < set(stages.ALL_STAGES):
        verdict, actual, mism, notes = classify_partial(entry, st, only_stages)
    else:
        verdict, actual, mism, notes = classify(entry, st)
    return entry["id"], verdict, actual, mism, notes, stages.strip_runs(st, keep_output)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--runtime", default="soft", choices=["soft", "board"])
    ap.add_argument("--case", action="append", default=[], help="case id (repeatable)")
    ap.add_argument("--set", action="append", default=[], choices=["positive", "negative", "extended"])
    ap.add_argument("--build-dir")
    ap.add_argument("--llvm-build")
    ap.add_argument("--clang")
    ap.add_argument("--clang-major", type=int)
    ap.add_argument("--out", default=os.path.join(ROOT, "reports"))
    ap.add_argument("--work", help="where intermediates go (default: a temporary directory)")
    ap.add_argument("--jobs", type=int, default=1)
    ap.add_argument("--cpu-only", action="store_true", help="only compile with cc and run (no compiler)")
    ap.add_argument("--stages", help="comma-separated subset of cpu,frontend,discovery,lowering,driver,execute "
                                     "(e.g. frontend,discovery on a machine with only the recognizer built)")
    ap.add_argument("--keep-output", action="store_true", help="keep every tool's full output in the report")
    ap.add_argument("--label", default="", help="free text recorded in the report (e.g. the machine)")
    ap.add_argument("--allow-spec-open", action="store_true",
                    help="exit 0 even when UNDETERMINED cases (verdict spec_open) are present; they are "
                         "still counted separately, never as pass")
    ap.add_argument("--allow-known-issues", action="store_true",
                    help="exit 0 even when recorded false negatives reproduce (regression gating)")
    a = ap.parse_args()

    manifest = load_manifest()
    entries = select(manifest, set(a.case), set(a.set))
    if not entries:
        print("no cases selected", file=sys.stderr)
        return 2
    tools = stages.Tools(REPO, a.build_dir, a.llvm_build, a.clang, a.clang_major)
    if not a.cpu_only:
        missing = tools.missing()
        if a.stages and "driver" not in a.stages:
            missing = [m for m in missing if m.split(" ")[0] in ("systolic-cfront", "systolic-opt", "clang")]
        if missing:
            print("missing tools: " + ", ".join(missing), file=sys.stderr)
            print("(build the compiler first; --build-dir / --llvm-build / --clang name the tool chain)", file=sys.stderr)
            return 2
    env = dict(os.environ)
    if a.runtime == "board" and not env.get("SYSTOLIC_FPGA_CONFIG"):
        print("--runtime board needs SYSTOLIC_FPGA_CONFIG (the fpga_config.txt of the programmed bitstream)",
              file=sys.stderr)
        return 2
    workroot = a.work or tempfile.mkdtemp(prefix="gemm_recognition-")
    os.makedirs(workroot, exist_ok=True)
    os.makedirs(a.out, exist_ok=True)
    stamp = datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
    mode = "cpu" if a.cpu_only else (a.runtime if not a.stages else "stages-" + a.stages.replace(",", "+"))

    results = []
    only_stages = [x.strip() for x in a.stages.split(",")] if a.stages else None
    jobs = [(e, tools, a.runtime, workroot, env, a.keep_output, a.cpu_only, only_stages) for e in entries]
    if a.jobs > 1 and a.runtime != "board":
        with concurrent.futures.ThreadPoolExecutor(max_workers=a.jobs) as pool:
            for r in pool.map(one, jobs):
                results.append(r)
                print("%-9s %-16s %s" % (r[0], r[1], "; ".join(r[3]) if r[3] else ""))
    else:
        for j in jobs:
            r = one(j)
            results.append(r)
            print("%-9s %-16s %s" % (r[0], r[1], "; ".join(r[3]) if r[3] else ""))
            sys.stdout.flush()

    by_id = {e["id"]: e for e in entries}
    counts = {}
    report_cases = []
    for cid, verdict, actual, mism, notes, st in results:
        counts[verdict] = counts.get(verdict, 0) + 1
        e = by_id[cid]
        report_cases.append({
            "id": cid, "kind": e.get("kind", "c"), "name": e["name"], "set": e["set"], "category": e["category"],
            "mathematical_class": e["mathematical_class"], "is_valid_gemm": e["is_valid_gemm"],
            "classification": e.get("classification"), "issue": e.get("issue"),
            "spec_question": e["expected"].get("spec_question"),
            "expected": e["expected"], "current_implementation": e["current_implementation"],
            "actual": actual, "verdict": verdict,
            "mismatches": mism, "notes": notes, "stages": st,
        })
    report = {
        "kind": "showcase_results",
        "mode": mode,
        "runtime": a.runtime if not a.cpu_only else None,
        "runtime_is_hardware": (a.runtime == "board"),
        "timestamp": stamp,
        "label": a.label,
        "host": platform.node(),
        "platform": platform.platform(),
        "tools": tools.describe() if not a.cpu_only else {"cc": "cc"},
        "manifest_compiler_commit": manifest.get("compiler_commit"),
        "cases_selected": len(entries),
        "counts": counts,
        "cases": report_cases,
    }
    base = os.path.join(a.out, "showcase_%s_%s" % (mode, stamp))
    with open(base + ".json", "w") as f:
        json.dump(report, f, indent=1)
    with open(base + ".md", "w") as f:
        f.write(markdown(report))
    with open(os.path.join(a.out, "latest_%s.json" % mode), "w") as f:
        json.dump(report, f, indent=1)
    print("\nverdicts: " + ", ".join("%s=%d" % kv for kv in sorted(counts.items())))
    print("report: %s.{json,md}" % base)
    if not a.work:
        print("intermediates: %s" % workroot)
    known = counts.get("known_false_negative", 0)
    if known:
        print("%d known false negative(s) reproduced (open issues in failures/issues/); "
              "they fail the run unless --allow-known-issues" % known)
    opens = counts.get("spec_open", 0)
    if opens:
        # UNDETERMINED is its own status: not a PASS (the specification has
        # not decided what the right outcome is) and not hidden either
        print("%d UNDETERMINED case(s) (verdict spec_open; spec questions, SPEC_QUESTIONS.md): neither "
              "PASS nor FAIL; they fail the run unless --allow-spec-open" % opens)
    refusals = counts.get("expected_refusal", 0)
    if refusals:
        # an open spec question the compiler answers conservatively (refused,
        # at the predicted stage, with the predicted reason): reported on its
        # own, never as a pass, and not a failure of the run
        print("%d EXPECTED_REFUSAL case(s) (open spec questions, SPEC_QUESTIONS.md, refused "
              "conservatively as predicted): not PASS, not blocking" % refusals)
    ok = ({"pass", "partial_pass", "not_applicable", "expected_refusal"}
          | ({"known_false_negative"} if a.allow_known_issues else set())
          | ({"spec_open"} if a.allow_spec_open else set()))
    return 0 if set(counts) <= ok else 1


def markdown(report):
    L = []
    L.append("# GEMM recognition showcase: actual results")
    L.append("")
    L.append("- mode: **%s**%s" % (report["mode"], " (the software runtime model, NOT the FPGA)" if report["mode"] == "soft" else
                                   " (the FPGA board)" if report["mode"] == "board" else " (plain C compiler, no systolic-cc)"))
    L.append("- timestamp: %s, host: %s (%s)%s" % (report["timestamp"], report["host"], report["platform"],
                                                   ", label: " + report["label"] if report["label"] else ""))
    t = report["tools"]
    if "compiler_commit" in t:
        L.append("- compiler: commit %s%s, clang: %s, build: %s, LLVM: %s" % (
            t.get("compiler_commit"), " (working tree has uncommitted compiler changes)" if t.get("compiler_dirty") else "",
            t.get("clang_version"), t.get("build_dir"), t.get("llvm_build")))
    L.append("- cases: %d; verdicts: %s" % (report["cases_selected"], ", ".join("%s=%d" % kv for kv in sorted(report["counts"].items()))))
    L.append("")
    L.append("| id | kind | set | spec | impl. predicted | actual driver | recognition | dense | lowering | planned inv. | runtime inv. | program | verdict | notes |")
    L.append("|---|---|---|---|---|---|---|---|---|---|---|---|---|---|")
    for c in report["cases"]:
        a = c["actual"]
        L.append("| %s | %s | %s | %s | %s | %s | %s | %s | %s | %s | %s | %s | **%s** | %s |" % (
            c["id"], c.get("kind", "c"), c["set"], c["expected"]["recognition"], c["current_implementation"]["driver"], a.get("driver"),
            a.get("recognition"), a.get("dense_partition"),
            a.get("lowering"), a.get("planned_per_call", ""), a.get("invocations", ""), a.get("execute"),
            c["verdict"], "; ".join(c["mismatches"] + c["notes"]).replace("|", "\\|")))
    L.append("")
    refused = [c for c in report["cases"] if c["verdict"] == "expected_refusal"]
    if refused:
        L.append("## Open spec questions refused conservatively (expected_refusal; not a pass, not blocking)")
        L.append("")
        for c in refused:
            L.append("- %s (%s): %s -- compiler: recognition %s, dense %s, lowering %s, driver %s, program %s; "
                     "reason: %s" % (
                c["id"], c["name"], c.get("spec_question"), c["actual"].get("recognition"),
                c["actual"].get("dense_partition"), c["actual"].get("lowering"), c["actual"].get("driver"),
                c["actual"].get("execute"), c["actual"].get("dense_reason") or c["actual"].get("lowering_reason")
                or c["actual"].get("driver_reason") or "(recognizer refusal; no reason stated)"))
        L.append("")
    opens = [c for c in report["cases"] if c["verdict"] == "spec_open"]
    if opens:
        L.append("## Cases the specification does not decide (spec questions; recorded, not judged)")
        L.append("")
        for c in opens:
            L.append("- %s (%s): %s -- compiler: recognition %s, dense %s, lowering %s, driver %s, program %s" % (
                c["id"], c["name"], c.get("spec_question"), c["actual"].get("recognition"),
                c["actual"].get("dense_partition"), c["actual"].get("lowering"), c["actual"].get("driver"),
                c["actual"].get("execute")))
        L.append("")
    known = [c for c in report["cases"] if c["verdict"] == "known_false_negative"]
    if report["mode"].startswith("stages-"):
        L.insert(2, "- **partial run**: only the stages %s were executed; 'partial_pass' means those stages "
                    "behaved as predicted, nothing is claimed about the others" % report["mode"][7:].replace("+", ", "))
    if known:
        L.append("## Known false negatives reproduced (open issues)")
        L.append("")
        for c in known:
            L.append("- %s (%s): %s" % (c["id"], c["name"], "; ".join(c["mismatches"])))
        L.append("")
    bad = [c for c in report["cases"] if c["verdict"] not in ("pass", "partial_pass", "known_false_negative", "spec_open", "expected_refusal", "not_applicable")]
    if bad:
        L.append("## Cases that are not 'pass'")
        L.append("")
        for c in bad:
            L.append("### %s (%s): %s" % (c["id"], c["name"], c["verdict"]))
            for m in c["mismatches"]:
                L.append("- " + m)
            for n in c["notes"]:
                L.append("- note: " + n)
            L.append("")
    return "\n".join(L) + "\n"


if __name__ == "__main__":
    sys.exit(main())
