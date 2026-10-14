#!/usr/bin/env python3
"""
make_showcase.py -- write the fixed showcase set: the 100 C files
(showcase/positive/case_001.c .. showcase/negative/case_100.c), the extended
cases (showcase/extended/ext_*.c), showcase/manifest.json and the per-case
expectation table that README.md includes.

    python3 generator/make_showcase.py [--out DIR] [--check]

--check regenerates into a temporary directory and reports whether the files
in --out are identical (the showcase set is meant to be stable: regenerate
only deliberately, and review the diff).

Every case is validated before it is written: the Python oracle evaluates it
(an out-of-bounds subscript is an error), its mathematical class is derived
from the subscripts and must agree with the author's claim, and the
expectation of each compiler stage is predicted from the compiler's rules.
Nothing here runs the compiler; that is scripts/run_showcase.py.
"""

import argparse
import filecmp
import json
import os
import shutil
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

import casespec  # noqa: E402
import showcase_cases  # noqa: E402

COMPILER_COMMIT = "6e25528"


def validate(case):
    """Raise if the case is not a valid, in-bounds program, if its claimed
    class contradicts the analysis, or if it sits in the wrong set.

    positive/  : the specification requires recognition (spec ACCEPT).  The
                 current implementation may still refuse one -- that is a
                 FALSE_NEGATIVE, kept in the positive set and recorded as an
                 issue; it is never moved out to make the suite green.
    negative/  : the current implementation refuses the nest (TRUE_NEGATIVE,
                 DOCUMENTED_LIMITATION, or a FALSE_NEGATIVE that documents the
                 limitation of an unsupported output/operand mapping).
    A FALSE_POSITIVE (spec REJECT, implementation offloads) is never allowed.
    """
    casespec.analyse(case)
    casespec.predict(case)
    casespec.spec_expect(case)
    cls = casespec.classify_case(case)
    casespec.evaluate(case)   # IndexError on an out-of-bounds subscript
    an = case.analysis
    if cls in ("FALSE_POSITIVE", "UNEXPECTED_ACCEPT"):
        raise ValueError("%s: the current implementation is predicted to offload a nest the "
                         "specification does not accept (%s)" % (case.id, cls))
    if case.set == "positive":
        if not an["is_gemm"]:
            raise ValueError("%s is in the positive set but is not a dense GEMM: %s"
                             % (case.id, an.get("not_dense_reasons")))
        if case.spec["recognition"] != "ACCEPT":
            raise ValueError("%s is in the positive set but the specification says %s (%s)"
                             % (case.id, case.spec["recognition"], case.spec["basis_key"]))
        if cls == "FALSE_NEGATIVE" and not case.issue:
            raise ValueError("%s is a predicted FALSE_NEGATIVE in the positive set: record it as an "
                             "issue (failures/issues/) and set case.issue" % case.id)
    if case.set == "negative" and case.expect["driver"] == "offloaded":
        raise ValueError("%s is in the negative set but the current implementation is predicted "
                         "to offload it" % case.id)


def subdir_of(case):
    return {"positive": "positive", "negative": "negative", "extended": "extended"}[case.set]


def write_all(out):
    cases = showcase_cases.ALL
    ids = [c.id for c in cases]
    assert len(ids) == len(set(ids)), "duplicate case ids"
    manifest = {
        "compiler_commit": COMPILER_COMMIT,
        "generator": "generator/make_showcase.py (specs in generator/showcase_cases.py)",
        "description": (
            "Fixed showcase set for GEMM recognition: 80 positive cases expected to be "
            "recognized, partitioned, lowered and offloaded by systolic-cc; 20 negative "
            "cases expected NOT to be accepted by the current compiler, each stating "
            "whether it is nevertheless a mathematically valid GEMM; plus extended "
            "cases outside the hundred. 'expected' records the prediction from the "
            "compiler's rules; 'tested' is always false here -- actual results are "
            "written by scripts/run_showcase.py to reports/, never into this file."),
        "stage_vocabulary": {
            "frontend": "systolic-cfront: translated | rejected (outside the affine C subset)",
            "recognition": "recognizeLogicalMatmul: accept | reject (hoisting_required: the lowering pass must first move a repetition loop outward)",
            "dense_partition": "getDenseLogicalGemm: supported | unsupported | not_reached",
            "lowering": "--affine-matmul-to-systolic (bounds, aliasing, rank, exact partition): supported | unsupported | not_reached",
            "driver": "systolic-cc: offloaded | stays_on_host | not_translated",
            "runtime": "what the compiled program's own check must show",
        },
        "showcase": [],
        "extended": [],
    }
    counts = {"positive": 0, "negative": 0, "extended": 0}
    for case in cases:
        validate(case)
        sub = subdir_of(case)
        d = os.path.join(out, sub)
        os.makedirs(d, exist_ok=True)
        text = casespec.c_file_text(case, COMPILER_COMMIT)
        with open(os.path.join(d, case.id + ".c"), "w") as f:
            f.write(text)
        entry = casespec.manifest_entry(case, sub)
        (manifest["extended"] if case.set == "extended" else manifest["showcase"]).append(entry)
        counts[case.set] += 1
    manifest["counts"] = counts
    with open(os.path.join(out, "manifest.json"), "w") as f:
        json.dump(manifest, f, indent=1)
        f.write("\n")
    with open(os.path.join(out, "CASES.md"), "w") as f:
        f.write(case_table(manifest))
    return counts


def case_table(manifest):
    lines = ["# Showcase cases: expectation table", "",
             "Generated by generator/make_showcase.py from generator/showcase_cases.py. "
             "'spec' is what the specification requires (set by the mathematics and the support "
             "specification); the 'impl:' columns are the behaviour predicted from reading commit %s; "
             "actual results live in reports/." % manifest["compiler_commit"], ""]
    for key, title in (("showcase", "The hundred"), ("extended", "Extended cases")):
        lines.append("## " + title)
        lines.append("")
        lines.append("| id | name | set / category | math class | GEMM | spec | classification | impl: front end | impl: recognition | impl: dense | impl: lowering | impl: driver | rejected at | issue |")
        lines.append("|---|---|---|---|---|---|---|---|---|---|---|---|---|---|")
        for e in manifest[key]:
            ex = e["current_implementation"]
            lines.append("| %s | %s | %s / %s | %s | %s | %s | %s | %s | %s%s | %s | %s | %s | %s | %s |" % (
                e["id"], e["name"], e["set"], e["category"], e["mathematical_class"],
                "yes" if e["is_valid_gemm"] else "no", e["expected"]["recognition"], e["classification"],
                ex["frontend"], ex["recognition"],
                " (after hoisting)" if ex["hoisting_required"] else "", ex["dense_partition"],
                ex["lowering"], ex["driver"], ex["reject_stage"] or "-", e.get("issue") or "-"))
        lines.append("")
    return "\n".join(lines)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default=os.path.join(HERE, "..", "showcase"))
    ap.add_argument("--check", action="store_true", help="compare a fresh generation with --out")
    a = ap.parse_args()
    out = os.path.abspath(a.out)
    if a.check:
        tmp = tempfile.mkdtemp(prefix="showcase-check-")
        try:
            write_all(tmp)
            diffs = []
            for root, _, files in os.walk(tmp):
                for fn in files:
                    p = os.path.join(root, fn)
                    q = os.path.join(out, os.path.relpath(p, tmp))
                    if not os.path.exists(q) or not filecmp.cmp(p, q, shallow=False):
                        diffs.append(os.path.relpath(p, tmp))
            if diffs:
                print("showcase set differs from a fresh generation:")
                for d in diffs:
                    print("  " + d)
                return 1
            print("showcase set is identical to a fresh generation")
            return 0
        finally:
            shutil.rmtree(tmp)
    counts = write_all(out)
    print("wrote %d positive, %d negative, %d extended cases + manifest.json + CASES.md to %s"
          % (counts["positive"], counts["negative"], counts["extended"], out))
    return 0


if __name__ == "__main__":
    sys.exit(main())
