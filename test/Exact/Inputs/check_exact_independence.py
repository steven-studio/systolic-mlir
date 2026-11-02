#!/usr/bin/env python3
"""Checks, from the sources and the built files, that the exact solver depends
on no LLVM solver and that production has not moved to it. Everything is read
from outside the solver: this script trusts none of its claims about itself.
"""
import argparse
import hashlib
import json
import re
import subprocess
import sys
from pathlib import Path

# The only LLVM headers the solver may include, and where.
ALLOWED_INCLUDES = {
    "include/Systolic/Exact/Int.h": {"llvm/ADT/DynamicAPInt.h"},
    "lib/Systolic/Exact/Provenance.cpp": {"llvm/Config/llvm-config.h",
                                          "llvm/Support/VCSRevision.h"},
}
FORBIDDEN_CODE = re.compile(
    r"\b(mlir|presburger|Presburger\w*|Simplex\w*|LexSimplex\w*|GBRSimplex|"
    r"IntegerRelation|IntegerPolyhedron|PresburgerSet|FlatAffine\w*|"
    r"Z3_\w*|isl_\w*)\b")
FORBIDDEN_SYMBOL = re.compile(r"mlir::|presburger|Simplex|Z3_|\bisl_")
# DynamicAPInt is header code over these, which is all the solver may reach.
ALLOWED_LLVM_SYMBOL = re.compile(
    r"llvm::(detail::(SlowDynamicAPInt|floorDiv|ceilDiv|mod|gcd|lcm|abs|"
    r"operator|SlowDynamicAPInt::)|APInt|operator|DynamicAPInt|"
    r".*SlowDynamicAPInt)")
# The checker is the trusted core. It sees the data it checks, shares no
# algorithm with the code that produced that data, and no arithmetic beyond Int.
CHECKER_FILES = ("include/Systolic/Exact/Certificate.h",
                 "include/Systolic/Exact/Checker.h",
                 "lib/Systolic/Exact/Checker.cpp")
SEARCH_HEADERS = ("Systolic/Exact/Decide.h", "Systolic/Exact/Lattice.h",
                  "Systolic/Exact/Normalize.h")
# Everything the compiled checker may call in the solver.
CHECKER_CALLS = re.compile(
    r"systolic::exact::(contentOf|divCeil|divFloor|gcdOf|lcmOf|magnitude|modFloor|"
    r"detail::noteExecution)\(")


def fail(message):
    sys.exit("FAIL: " + message)


def run(*command, check=True):
    done = subprocess.run(command, capture_output=True, text=True, errors="replace")
    if check and done.returncode != 0:
        fail("command failed: " + " ".join(command) + "\n" + done.stderr)
    return done


def strip_comments(text):
    return re.sub(r"/\*.*?\*/", "", re.sub(r"//[^\n]*", "", text), flags=re.S)


def check_sources(root):
    files = sorted(
        list((root / "include/Systolic/Exact").glob("*.h"))
        + list((root / "lib/Systolic/Exact").glob("*.h"))
        + list((root / "lib/Systolic/Exact").glob("*.cpp")))
    if not files:
        fail("no solver sources found")
    for path in files:
        relative = str(path.relative_to(root))
        code = strip_comments(path.read_text())
        for header in re.findall(r'#include\s*[<"]([^">]+)[">]', code):
            if header.startswith(("mlir/", "z3", "isl/")):
                fail(f"{relative} includes {header}")
            if header.startswith("llvm/") and header not in ALLOWED_INCLUDES.get(
                    relative, ()):
                fail(f"{relative} includes {header}")
            if relative in CHECKER_FILES and header in SEARCH_HEADERS:
                fail(f"the checker file {relative} includes {header}")
        body = "\n".join(line for line in code.split("\n")
                         if not line.lstrip().startswith("#include"))
        found = FORBIDDEN_CODE.search(body)
        if found:
            fail(f"{relative} uses {found.group(0)}")
        if relative == CHECKER_FILES[-1] and re.search(
                r"\bRational\b",
                re.sub(r"const Rational &|std::vector<Rational>|Domain::Rational",
                       "", body)):
            fail("the checker uses Rational as more than data")
    print(f"sources: {len(files)} files, no forbidden dependency")


def check_archive(binary):
    # build/bin/x next to build/lib/Systolic/Exact/libSystolicExact.a
    archive = binary.parent.parent / "lib/Systolic/Exact/libSystolicExact.a"
    if not archive.exists():
        fail(f"no archive at {archive}")
    undefined = set()
    for line in run("nm", "-u", "-C", str(archive)).stdout.split("\n"):
        line = line.strip()
        if line.startswith("U "):
            undefined.add(line[2:])
    defined = set()
    for line in run("nm", "-C", "--defined-only", str(archive)).stdout.split("\n"):
        parts = line.split(" ", 2)
        if len(parts) == 3:
            defined.add(parts[2])
    external = undefined - defined
    for symbol in sorted(external):
        if FORBIDDEN_SYMBOL.search(symbol):
            fail(f"the archive needs {symbol}")
        if "llvm::" in symbol and not ALLOWED_LLVM_SYMBOL.search(symbol):
            fail(f"the archive needs an LLVM symbol it should not: {symbol}")
    print(f"archive: {len(external)} undefined symbols, none forbidden")


def check_checker_object(binary):
    archive = binary.parent.parent / "lib/Systolic/Exact/libSystolicExact.a"
    calls = None
    member = ""
    for line in run("nm", "-u", "-C", str(archive)).stdout.split("\n"):
        if line.endswith(".o:"):
            member = line[:-1]
            if member == "Checker.cpp.o":
                calls = []
        elif member == "Checker.cpp.o" and "systolic::exact::" in line:
            calls.append(line.strip())
    if calls is None:
        fail("the archive has no checker object")
    for call in calls:
        if not CHECKER_CALLS.search(call):
            fail(f"the checker object calls {call}")
    print(f"checker: {len(calls)} calls into the solver, all Int helpers or its "
          "counter")


def check_test_binary(binary):
    names = run("nm", "-C", str(binary)).stdout
    if not names:
        fail("the test binary has no symbol table, so this check would prove nothing")
    if not re.search(r"(^|[^:\w])systolic::exact::decide", names, re.M):
        fail("the test binary does not contain the solver")
    for line in names.split("\n"):
        if FORBIDDEN_SYMBOL.search(line):
            fail(f"the test binary links {line.strip()}")
    print("test binary: no LLVM solver code linked")


def check_production(binary):
    names = run("nm", "-C", str(binary)).stdout
    if not names:
        fail("the production binary has no symbol table")
    # The namespace with its colons: mlir::systolic::exactIsSubset is not it.
    if re.search(r"(^|[^:\w])systolic::exact::", names, re.M):
        fail("production links the solver, which this milestone must not do")
    if "mlir::presburger::Simplex::restoreRow" not in names:
        fail("production no longer has LLVM Simplex: a cutover needs its own "
             "milestone, and this check must change with it")
    print("production: solver not linked, LLVM Simplex still in use")


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
    idle = json.loads(run(str(binary), "--print-provenance").stdout)
    used = json.loads(run(str(binary), "--print-provenance-after-query").stdout)
    declared = idle["compile_time_declaration"]

    # Printing the declaration is not execution; running a query is.
    if idle["runtime_evidence"]["decide_calls"] != 0:
        fail("evidence appeared without a query")
    if used["runtime_evidence"]["decide_calls"] != 1:
        fail("a query left no evidence")
    if used["runtime_evidence"]["checker_runs"] != 1:
        fail("a query did not reach the checker")

    if declared["solver"]["implementation"] != "PROJECT_OWNED":
        fail("wrong implementation")
    if declared["production"]["solver"] != "LLVM_UPSTREAM":
        fail("the declaration claims a cutover")
    # check_archive proves the symbols. This keeps the claim equal to them.
    if declared["solver"].get("llvm_dependency") != "ARITHMETIC_ONLY":
        fail("the declaration misstates what the solver takes from LLVM")
    if declared["solver"]["source_sha256"] != source_fingerprint(root):
        fail("the binary was not built from these sources")

    commit = declared["project"]["commit"]
    if commit != "UNKNOWN" and run("git", "-C", str(root), "cat-file", "-e",
                                   commit, check=False).returncode != 0:
        fail(f"the declared commit {commit} is not in this repository")
    if declared["project"]["tree"] not in ("CLEAN", "DIRTY", "UNKNOWN"):
        fail("bad tree state")

    # The patch status is reproduced from the same kind of evidence.
    llvm = declared["llvm"]
    status = llvm["patch_status"]
    if status not in ("NONE_VERIFIED", "PATCHED", "UNKNOWN"):
        fail("bad patch status")
    if status != "UNKNOWN":
        source = llvm["source_dir"]
        head = run("git", "-C", source, "rev-parse", "HEAD").stdout.strip()
        if head != llvm["revision"]:
            fail("a verified patch status needs the source at the built revision")
        changes = run("git", "--no-optional-locks", "-C", source, "status",
                      "--porcelain", "--", *llvm["patch_scope"].split()).stdout
        if (status == "PATCHED") != bool(changes.strip()):
            fail("the declared patch status does not match the source tree")
    print("provenance: declaration and runtime evidence verified")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--source-root", required=True, type=Path)
    parser.add_argument("--test-binary", required=True, type=Path)
    parser.add_argument("--production-binary", required=True, type=Path)
    args = parser.parse_args()
    root = args.source_root.resolve()
    binary = args.test_binary.resolve()
    check_sources(root)
    check_archive(binary)
    check_checker_object(binary)
    check_test_binary(binary)
    check_production(args.production_binary.resolve())
    check_provenance(root, binary)


main()
