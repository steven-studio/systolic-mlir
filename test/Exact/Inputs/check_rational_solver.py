#!/usr/bin/env python3
"""Checks, from the sources and the built files, that the rational solver is the
project's own, that a query really runs it, and that production does not reach
it. check_exact_independence.py covers the library as a whole. This adds what is
particular to the rational solver.
"""
import argparse
import hashlib
import json
import re
import subprocess
import sys
from pathlib import Path

# The checker must not see the code whose answers it checks.
CHECKER_FILES = ("include/Systolic/Exact/Certificate.h",
                 "include/Systolic/Exact/Checker.h",
                 "lib/Systolic/Exact/Checker.cpp")
SEARCH_HEADERS = ("Systolic/Exact/DeltaRational.h", "Systolic/Exact/Session.h",
                  "Systolic/Exact/Tableau.h")
# Names that show this solver is in a binary.
SOLVER_SYMBOLS = ("systolic::exact::Session::check", "systolic::exact::BoundsTableau::step")
FORBIDDEN_SYMBOL = re.compile(r"mlir::|presburger|Simplex|Z3_|\bisl_")


def fail(message):
    sys.exit("FAIL: " + message)


def run(*command):
    done = subprocess.run(command, capture_output=True, text=True, errors="replace")
    if done.returncode != 0:
        fail("command failed: " + " ".join(command) + "\n" + done.stderr)
    return done.stdout


def strip_comments(text):
    return re.sub(r"/\*.*?\*/", "", re.sub(r"//[^\n]*", "", text), flags=re.S)


def check_sources(root):
    for relative in CHECKER_FILES:
        code = strip_comments((root / relative).read_text())
        for header in re.findall(r'#include\s*[<"]([^">]+)[">]', code):
            if header in SEARCH_HEADERS:
                fail(f"the checker file {relative} includes {header}")
    # Only Decide.cpp may give a Decision its verdict.
    for path in sorted((root / "lib/Systolic/Exact").glob("*.cpp")):
        code = strip_comments(path.read_text())
        if path.name != "Decide.cpp" and re.search(r"\.answer\s*=[^=]", code):
            fail(f"{path.name} sets a verdict outside the gate")
    print("sources: the checker reads no search code, and one file sets verdicts")


def check_test_binary(binary):
    names = run("nm", "-C", str(binary))
    if not names:
        fail("the test binary has no symbol table, so this check would prove nothing")
    for symbol in SOLVER_SYMBOLS:
        if symbol not in names:
            fail(f"the test binary does not contain {symbol}")
    for line in names.split("\n"):
        if FORBIDDEN_SYMBOL.search(line):
            fail(f"the test binary links {line.strip()}")
    print("test binary: the rational solver is linked, and no LLVM solver code")


def check_production(binary):
    names = run("nm", "-C", str(binary))
    if not names:
        fail("the production binary has no symbol table")
    for symbol in SOLVER_SYMBOLS:
        if symbol in names:
            fail("production links the rational solver, which S2 must not do")
    if "mlir::presburger::Simplex::restoreRow" not in names:
        fail("production no longer has LLVM Simplex: a cutover needs its own "
             "milestone, and this check must change with it")
    print("production: rational solver not linked, LLVM Simplex still in use")


def source_fingerprint(root):
    paths = sorted(
        str(p.relative_to(root))
        for directory in ("include/Systolic/Exact", "lib/Systolic/Exact")
        for p in (root / directory).iterdir())
    lines = ""
    for path in paths:
        digest = hashlib.sha256((root / path).read_bytes()).hexdigest()
        lines += f"{digest}  {path}\n"
    return hashlib.sha256(lines.encode()).hexdigest()


def check_provenance(root, binary):
    idle = json.loads(run(str(binary), "--print-provenance"))
    used = json.loads(run(str(binary), "--print-provenance-after-rational-query"))
    declared = idle["compile_time_declaration"]
    if declared["solver"]["source_sha256"] != source_fingerprint(root):
        fail("the binary was not built from these sources")
    if declared["solver"]["implementation"] != "PROJECT_OWNED":
        fail("wrong implementation")
    if declared["solver"]["revision"] != "S2":
        fail("the declaration is not of S2")
    if declared["solver"]["llvm_dependency"] != "ARITHMETIC_ONLY":
        fail("the declaration misstates what the solver takes from LLVM")
    if declared["production"]["solver"] != "LLVM_UPSTREAM":
        fail("the declaration claims a cutover")

    before = idle["runtime_evidence"]
    after = used["runtime_evidence"]
    if before["rational_checks"] != 0 or before["tableau_pivots"] != 0:
        fail("evidence appeared without a query")
    if after["rational_checks"] != 1:
        fail("a query left no evidence")
    if after["tableau_pivots"] < 1:
        fail("a query that needs a pivot made none")
    if after["checker_runs"] != 1:
        fail("a query did not reach the checker")
    if after["decide_calls"] != 0 or after["lattice_runs"] != 0:
        fail("a rational query ran the other solver")
    print("provenance: a rational query ran the tableau and the checker")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--source-root", required=True, type=Path)
    parser.add_argument("--test-binary", required=True, type=Path)
    parser.add_argument("--production-binary", required=True, type=Path)
    args = parser.parse_args()
    root = args.source_root.resolve()
    binary = args.test_binary.resolve()
    check_sources(root)
    check_test_binary(binary)
    check_production(args.production_binary.resolve())
    check_provenance(root, binary)


main()
