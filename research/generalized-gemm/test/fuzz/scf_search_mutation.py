#!/usr/bin/env python3
"""Mutation test of the second search for scf.for nests (docs/GENERALIZED_GEMM_SCOPE.md, stage 3
on scf.for): GrammarSpec::exactLeaves in CoordinateGrammar.cpp and the search that turns it on
in SynthesisAnalysis.cpp. Each mutant weakens one thing, the unmodified tree must pass first,
and every result is one of:

  KILLED          a test failed by its own check (FileCheck), and nothing crashed.
  SURVIVED        every test ran to the end and passed.
  EQUIVALENT      not run: listed below with the reason.
  HARNESS_ERROR   a stale pattern, a mutant that does not build, a crash, a timeout.

Only KILLED and EQUIVALENT let a run pass. The tests are the research lit files that run a
driver holding the grammar (the coordinate search): nest_*, synth_*, dom_*, comp_*. They are run
from a copy of the tree in --scratch, built with --jobs jobs (two by default); the tree itself is
never changed.
Results are appended to --results as they finish.
"""
import argparse
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys

G = "lib/GeneralizedGemm/CoordinateGrammar.cpp"
A = "lib/GeneralizedGemm/SynthesisAnalysis.cpp"
T = "lib/GeneralizedGemm/TerminationProposers.cpp"

MUTANTS = [
    ("a state variable the sample lost not kept by the second search", G,
     "      if (spec.exactLeaves)\n        operands.push_back({term, size});\n",
     "      if (spec.exactLeaves)\n        (void)size;\n"),
    ("a state variable the sample lost not counted", G, "        ++out.lostLeaves;\n", "        (void)0;\n"),
    ("a state variable equal on every event taken for lost", G,
     "    if (mine && (!theirs || *mine != *theirs)) {\n", "    if (mine) {\n"),
    ("a state variable that differs at some event not taken for lost", G,
     "    if (mine && (!theirs || *mine != *theirs)) {\n", "    if (mine && !theirs) {\n"),
    ("the state variables offered without the comparison", G,
     "      offer(terms.state(s.id, s.sort), 1, /*leaf=*/true);\n",
     "      offer(terms.state(s.id, s.sort), 1, /*leaf=*/false);\n"),
    ("the lost state variables of a bank not summed", A, "      out.lostLeaves += bank.lostLeaves;\n",
     "      (void)bank.lostLeaves;\n"),
    ("the second search's grammar not told", A, "    spec.exactLeaves = exact;\n", "    (void)exact;\n"),
    ("the second search never run", A,
     "  if (sampling || out.lostLeaves == 0 || symbolic(out) || !result.enumeration.groups[g].unsupported.empty())\n",
     "  if (true)\n"),
    ("the second search run after a symbolic candidate", A,
     "  if (sampling || out.lostLeaves == 0 || symbolic(out) || !result.enumeration.groups[g].unsupported.empty())\n",
     "  if (sampling || out.lostLeaves == 0 || !result.enumeration.groups[g].unsupported.empty())\n"),
    ("the second search run on a sampled scope", A,
     "  if (sampling || out.lostLeaves == 0 || symbolic(out) || !result.enumeration.groups[g].unsupported.empty())\n",
     "  if (out.lostLeaves == 0 || symbolic(out) || !result.enumeration.groups[g].unsupported.empty())\n"),
    ("the second search's outcome taken whatever it is", A, "  if (symbolic(again))\n    out = std::move(again);\n",
     "  if (true)\n    out = std::move(again);\n"),
    ("a candidate taken as symbolic at any tier", A,
     "      return candidate.symbolic.tier() == EvidenceTier::SymbolicExact;\n",
     "      return candidate.symbolic.tier() != EvidenceTier::SymbolicExact;\n"),
    ("a counter's start through a constant state not proposed", T, "  if (!constantOf.empty())\n", "  if (false)\n"),
]

EQUIVALENT = [
    ("a value of other states rebuilt", T,
     "            llvm::any_of(stateSupport(value), [&](unsigned other) { return !constantOf.count(other); }))\n",
     "the check only spares the rebuilding of values that hold a state other than the constant ones: such a "
     "value stays open after the substitution and is dropped (closed(replaced) is false), so no proposal "
     "changes; what changes is the numbering of the terms made after it in the term context, which no "
     "verdict reads (term_records.mlir and dom_phase4.mlir printed the same with term numbers masked)"),
    ("the second search run though no state variable was lost", A,
     "  if (sampling || out.lostLeaves == 0 || symbolic(out) || !result.enumeration.groups[g].unsupported.empty())\n",
     "the second search differs from the first only in the state variables the first lost; with none lost, "
     "and on a scope enumerated whole (sampling is false there, so neither search adds a point to the "
     "driver's enumeration), it builds the same banks from the same domains and makes the same queries, so "
     "it verifies no candidate symbolically either and its outcome is not taken: it only costs time"),
]

CRASH = ("Stack dump", "PLEASE submit a bug report", "Segmentation fault", "core dumped", "Assertion `",
         "terminate called", "exit status: -", "exit status: 134", "exit status: 139", "AddressSanitizer")
TOOLS = ["generalized-gemm-nest-report", "generalized-gemm-synthesis-report", "generalized-gemm-domain-report",
         "generalized-gemm-computation-report", "generalized-gemm-report", "generalized-gemm-termination-report",
         "generalized-gemm-coordinate-report"]
FILTER = "nest_|synth_|dom_|comp_"
SECONDS = 3600


def run(command, cwd=None, timeout=SECONDS):
    try:
        done = subprocess.run([str(c) for c in command], cwd=cwd, capture_output=True, text=True, timeout=timeout)
        return done.returncode, done.stdout + done.stderr, False
    except subprocess.TimeoutExpired as expired:
        return -1, str(expired.stdout or "") + str(expired.stderr or ""), True


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", required=True, help="the repository root")
    parser.add_argument("--scratch", required=True, help="a directory of this harness's own")
    parser.add_argument("--llvm-dir", required=True)
    parser.add_argument("--mlir-dir", required=True)
    parser.add_argument("--lit", required=True)
    parser.add_argument("--results", required=True)
    parser.add_argument("--only", default="", help="mutant names, separated by ';'")
    parser.add_argument("--jobs", type=int, default=2, help="build jobs")
    args = parser.parse_args()
    root, scratch = Path(args.source).resolve(), Path(args.scratch).resolve()
    names = [m[0] for m in MUTANTS]
    if len(set(names)) != len(names):
        sys.exit("two mutants share a name")
    for name, file, old, _ in MUTANTS:
        count = (root / file).read_text().count(old)
        if count != 1:
            sys.exit(f"{name}: the pattern occurs {count} times in {file}")
    chosen = [n.strip() for n in args.only.split(";") if n.strip()] or names
    unknown = [n for n in chosen if n not in names]
    if unknown:
        sys.exit(f"no such mutants: {unknown}")
    copy, build = scratch / "src", scratch / "build"
    parts = ["include", "lib", "research"] + [f"test/{p.name}" for p in (root / "test").glob("polynomial*")]
    if not copy.exists():
        copy.mkdir(parents=True)
        for part in parts:
            (copy / part).parent.mkdir(parents=True, exist_ok=True)
            shutil.copytree(root / part, copy / part)
    for name, file, _, _ in MUTANTS:
        if (copy / file).read_bytes() != (root / file).read_bytes():
            sys.exit(f"{copy / file} is not the tree's: use a new --scratch")
    code, text, _ = run(["cmake", "-G", "Ninja", "-S", copy / "research" / "generalized-gemm", "-B", build,
                         "-DCMAKE_BUILD_TYPE=Debug", "-DCMAKE_C_COMPILER=/usr/bin/gcc",
                         "-DCMAKE_CXX_COMPILER=/usr/bin/g++", f"-DLLVM_DIR={args.llvm_dir}",
                         f"-DMLIR_DIR={args.mlir_dir}"])
    if code != 0:
        sys.exit("cmake failed:\n" + text[-2000:])
    results = Path(args.results)
    done = {json.loads(line)["mutant"] for line in results.read_text().splitlines()} if results.exists() else set()

    def record(name, result, detail=""):
        with results.open("a") as out:
            out.write(json.dumps({"mutant": name, "result": result, "detail": detail}) + "\n")
        print(f"{result:14s} {name}  {detail[:160]}", flush=True)

    def tests():
        code, text, timed_out = run(["ninja", "-C", build, f"-j{args.jobs}"] + TOOLS)
        if code != 0:
            return "HARNESS_ERROR", "does not build: " + text[-600:].replace("\n", " | ")
        code, text, timed_out = run([args.lit, "-v", build / "lit", "--filter", FILTER])
        if timed_out:
            return "HARNESS_ERROR", "the tests timed out"
        if code == 0:
            return "SURVIVED", ""
        if any(marker in text for marker in CRASH):
            return "HARNESS_ERROR", "a crash: " + next(m for m in CRASH if m in text)
        mismatch = re.search(r"error: ([A-Z][A-Z0-9]*(?:-[A-Z]+(?:-[0-9]+)?)?): (expected string not found in input|"
                             r"is not on the line after the previous match|excluded string found in input|found here)",
                             text)
        if mismatch:
            failed = re.findall(r"^FAIL: generalized-gemm :: (\S+)", text, re.M)
            return "KILLED", f"{mismatch.group(1)}: {mismatch.group(2)} ({', '.join(failed)})"
        empty = re.search(r"FileCheck error: '<stdin>' is empty\.\s*(?:#\s*\|?\s*)*FileCheck command line: [^\n]*"
                          r"--check-prefix=([A-Z][A-Z0-9]*)", text)
        if empty:
            return "KILLED", f"{empty.group(1)}: the input is empty"
        return "HARNESS_ERROR", f"exit {code} with no check that failed: " + text[-1500:].replace("\n", " | ")

    if "BASELINE" not in done:
        result, detail = tests()
        if result != "SURVIVED":
            record("BASELINE", "HARNESS_ERROR", "the unmodified tree does not pass: " + detail)
            sys.exit(1)
        record("BASELINE", "PASSED")
    for name, file, old, reason in EQUIVALENT:
        if (root / file).read_text().count(old) != 1:
            sys.exit(f"{name}: the pattern of an equivalent mutant does not occur once in {file}")
        if name not in done:
            record(name, "EQUIVALENT", reason)
    for name, file, old, new in MUTANTS:
        if name not in chosen or name in done:
            continue
        path = copy / file
        original = path.read_text()
        path.write_text(original.replace(old, new, 1))
        try:
            result, detail = tests()
        finally:
            path.write_text(original)
        record(name, result, detail)
    rows = [json.loads(line) for line in results.read_text().splitlines()]
    bad = [r for r in rows if r["mutant"] in chosen and r["result"] not in ("KILLED", "EQUIVALENT")]
    print(f"{len(bad)} mutants neither killed nor equivalent")
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
