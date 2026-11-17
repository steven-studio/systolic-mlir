#!/usr/bin/env python3
"""Stage 2 mutation test (docs/GENERALIZED_GEMM_SCOPE.md): the production gate on records of
loop nests. Each mutant weakens one check of lib/Systolic/Transforms/NestRecordGate.cpp, of the
pass's gated path in AffineMatmulToSystolic.cpp, or of the binding in NestBinding.cpp, in a
copy of the production sources built in a scratch directory; the copy unmodified must pass
first. The tests are the production tests of the gate, test/Systolic/nest_records.mlir,
nest_records_refusals.mlir and nest_records_belts.mlir. Every result is one of:

  KILLED          a test failed by its own check (FileCheck), and nothing crashed.
  SURVIVED        both tests passed.
  EQUIVALENT      not run: listed below with the reason.
  HARNESS_ERROR   a stale pattern, a mutant that does not build, a crash.

Only KILLED and EQUIVALENT let a run pass. Nothing in the source tree is changed: the copy is
taken once, each mutant changes one file of it and is undone after. Results are appended to
--results with the hash of the files they are about; a run skips what that file already holds.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import signal
import subprocess
import sys

G = "lib/Systolic/Transforms/NestRecordGate.cpp"
P = "lib/Systolic/Transforms/AffineMatmulToSystolic.cpp"
B = "lib/Systolic/Transforms/NestBinding.cpp"

MUTANTS = [
    # Reading the file.
    ("the format not checked", G, "    if (!schema || *schema != kNestRecordSchema) {\n", "    if (!schema) {\n"),
    ("two objects of one function read", G, "    if (!out.functions.try_emplace(*name, function).second) {\n",
     "    if (!out.functions.try_emplace(*name, function).second && false) {\n"),
    ("a nest with no number read", G, "      if (!ordinal || *ordinal < 0) {\n", "      if (!ordinal && false) {\n"),
    ("two records of one nest read", G, "      if (!byOrdinal.try_emplace((unsigned)*ordinal, nest).second) {\n",
     "      if (!byOrdinal.try_emplace((unsigned)*ordinal, nest).second && false) {\n"),
    # The record: about this nest of this input.
    ("the number of roots not compared", G, "  if (recordRoots->size() != roots->roots.size())\n",
     "  if (recordRoots->size() < roots->roots.size())\n"),
    ("a root of the function not compared", G,
     "    if (!same)\n      return refuse(\"root \" + std::to_string(id) +",
     "    if (false)\n      return refuse(\"root \" + std::to_string(id) +"),
    ("a claim to authorize read", G, "  if (*functionAuthorizes || *authorizes)\n", "  if (false)\n"),
    ("the binding's scheme not compared", G, "  if (*scheme != kNestBindingScheme)\n", "  if (false)\n"),
    ("the binding not compared", G, "  if (*sha != nest->binding->sha256)\n", "  if (false)\n"),
    ("the index width not compared", G, "  if (*width != (int64_t)indexWidth)\n", "  if (false)\n"),
    # The record: complete and verified.
    ("the status not read", G, "  if (*status != \"VERIFIED\") {\n", "  if (false) {\n"),
    ("the ordered fold not read", G, "  if (!*orderedFold)\n", "  if (false)\n"),
    ("the refusals not read", G, "  if (!refusals->empty())\n    return refuse(\"the record has refusals\");\n",
     "  if (false)\n    return refuse(\"the record has refusals\");\n"),
    ("the scope not read", G, "  if (!scope->empty())\n", "  if (false)\n"),
    ("Theorem 13's tier not read", G, "  if (*t13Status != \"VERIFIED\" || *t13Tier != \"SYMBOLIC\" || *t13Logical != \"SYMBOLIC\")\n",
     "  if (*t13Status != \"VERIFIED\")\n"),
    ("the frame not read", G, "  if (*otherWrites != 0)\n", "  if (false)\n"),
    ("a domain that is no box read", G, "  if (!*box || *guard != \"true\")\n", "  if (false)\n"),
    # The record: in agreement with the recognizer.
    ("M, N and K not compared", G, "    if (*extent != dense->extent[axis])\n", "    if (false)\n"),
    ("the loop of a coordinate not compared", G, "    if (!level || *level != (int64_t)dense->sourceDim[axis])\n",
     "    if (!level)\n"),
    ("the number of levels not compared", G, "  if (levels->size() != 3)\n", "  if (levels->size() < 3)\n"),
    ("a level not compared", G,
     "    if (expected >= 3 || *axis != kAxisName[expected] || *descending ||\n"
     "        *lower != band[l].getConstantLowerBound() || *upper != band[l].getConstantUpperBound())\n",
     "    if (expected >= 3)\n"),
    ("an operand's root not compared", G, "    if (*rootNumber != (int64_t)rootOf[index])\n", "    if (false)\n"),
    ("a view not proved read", G, "    if (!*proved)\n", "    if (false)\n"),
    ("the number of dimensions not compared", G, "    if (dimensions->size() != 2)\n", "    if (dimensions->size() < 2)\n"),
    ("a dimension of a view not compared", G,
     "          *level != (int64_t)dense->sourceDim[operand.roles[role]] || *coefficient != mine.scale ||\n"
     "          *constant != mine.offset)\n",
     "          *level != (int64_t)dense->sourceDim[operand.roles[role]])\n"),
    ("the element type not compared", G, "  if (*element != myElement)\n", "  if (false)\n"),
    ("the update's operations not compared", G, "  if (*reduction != \"fadd\" || *multiply != \"fmul\" || *sites != 1 || !*sitesAgree)\n",
     "  if (false)\n"),
    ("the accumulator's place not compared", G, "  if (*first != accumulatorFirst)\n", "  if (false)\n"),
    ("the flags not compared", G, "  if (recordReduction != myReduction || recordMultiply != myMultiply)\n", "  if (false)\n"),
    ("a flag that is no string read", G, "  if (!wellFormed)\n    return refuse(\"the record is incomplete: a flag is no string\");\n",
     "  if (false)\n    return refuse(\"the record is incomplete: a flag is no string\");\n"),
    ("the source's flag not compared", G, "  if (!sourceFlag || *sourceFlag != reassoc)\n", "  if (!sourceFlag)\n"),
    ("a separation the record does not use read", G, "    if (!*used)\n      continue;\n", "    if (false)\n      continue;\n"),
    ("a proof between two arguments read", G, "      if (roots->roots[*a].argument && roots->roots[*b].argument)\n",
     "      if (false)\n"),
    ("a separation of no known kind read", G, "    if (*kind != \"assumption\")\n", "    if (false)\n"),
    ("an assumption the alias gate does not check read", G,
     "    if (!is(rootOf[0], rootOf[2]) && !is(rootOf[1], rootOf[2]))\n", "    if (false)\n"),
    # The record: about a nest of the shape it describes (nest_records_belts.mlir).
    ("batch loops read", G,
     "  if (dense->numBatchDims || !matmul.demotedLoops.empty() || !matmul.reductionBatchLoops.empty())\n",
     "  if (false)\n"),
    ("an assignment read", G, "  if (!matmul.accumulates)\n", "  if (false)\n"),
    ("a band of other than three loops read", G, "  if (band.size() != 3)\n", "  if (false)\n"),
    ("bounds that are no constants, or a step other than 1, read", G,
     "    if (!loop.hasConstantLowerBound() || !loop.hasConstantUpperBound() || loop.getStepAsInt() != 1)\n",
     "    if (false)\n"),
    ("a coordinate that is no loop's counter read", G, "    if (!unit)\n", "    if (false)\n"),
    ("a view the record does not compare read", G,
     "    if (layout.general || layout.roleDim[0] < 0 || layout.roleDim[1] < 0 || type.getRank() != 2 ||\n"
     "        layout.hasBatchTerms(0) || layout.hasBatchTerms(1) || operand.symbolic)\n",
     "    if (false)\n"),
    # The pass.
    ("the option not read", P, "    if (!nestRecords.empty()) {\n      runWithNestRecords();\n",
     "    if (false) {\n      runWithNestRecords();\n"),
    ("the gate's refusal ignored", P, "      if (!decision.agrees) {\n", "      if (false) {\n"),
    ("the other paths taken with records", P,
     "    if (generalAffine || kernelCosetOffload || certifiedOffloadFirst || commonFiberOffload || fpDistribute) {\n",
     "    if (false) {\n"),
    ("the index width taken as 16", P,
     "        DataLayout(module).getTypeSizeInBits(IndexType::get(module.getContext())).getFixedValue();\n",
     "        16;\n"),
    # The binding: what the production pass computes must be what the research tool computed.
    ("the function's name left out of the binding", B, "     << \"function: \" << function.getSymName() << \"\\n\"\n",
     "     << \"function: \\n\"\n"),
    ("a root's llvm.noalias left out of the binding", B,
     "      os << \"argument \" << root.argNumber << (root.noalias ? \", llvm.noalias\" : \"\");\n",
     "      os << \"argument \" << root.argNumber;\n"),
    ("locations in the binding", B, "  module.get()->print(os, OpPrintingFlags().printGenericOpForm());\n",
     "  module.get()->print(os, OpPrintingFlags().printGenericOpForm().enableDebugInfo());\n"),
]

# Not run: a check whose condition never holds once the checks before it have passed, for any
# input, records edited by hand included.
EQUIVALENT = [
    ("the lowering's own reassoc fact not compared", G, "  if (reassoc != matmul.reductionMayReassociate)\n",
     "with an accumulation, which a check before it requires, the recognizer's reductionMayReassociate is the "
     "reassoc bit of the flags of the addition the GEMM stores, the very arith.addf this gate reads, and "
     "flagNames lists that bit as reassoc whatever other bits are set (fast included): the two never differ"),
]

BUILD_SECONDS = 3600
TEST_SECONDS = 900
CRASH = ("Stack dump", "PLEASE submit a bug report", "Segmentation fault", "core dumped", "Assertion `",
         "terminate called", "exit status: -", "exit status: 134", "exit status: 139")


def run(command, cwd=None, timeout=TEST_SECONDS):
    process = subprocess.Popen([str(c) for c in command], cwd=cwd, stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT, text=True, start_new_session=True)
    try:
        text, _ = process.communicate(timeout=timeout)
        return process.returncode, text, False
    except subprocess.TimeoutExpired:
        try:
            os.killpg(process.pid, signal.SIGKILL)  # The group it started, by its id.
        except ProcessLookupError:
            pass
        text, _ = process.communicate()
        return None, text, True


def tree_hash(root, files):
    digest = hashlib.sha256()
    for name in files:
        digest.update(name.encode())
        digest.update((root / name).read_bytes())
    for path in sorted((root / "test/Systolic").glob("nest_records*")) + sorted((root / "test/Systolic/Inputs").glob("*nest_records*")):
        digest.update(str(path.relative_to(root)).encode())
        digest.update(path.read_bytes())
    return digest.hexdigest()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", required=True)
    parser.add_argument("--scratch", required=True)
    parser.add_argument("--llvm-dir", required=True)
    parser.add_argument("--mlir-dir", required=True)
    parser.add_argument("--lit", required=True)
    parser.add_argument("--results", required=True)
    parser.add_argument("--only", default="")
    args = parser.parse_args()
    root = Path(args.source).resolve()
    scratch = Path(args.scratch).resolve()
    copy, build = scratch / "src", scratch / "build"
    files = sorted({m[1] for m in MUTANTS} | {e[1] for e in EQUIVALENT})
    tree = tree_hash(root, files)
    results = Path(args.results)
    done = {}
    if results.exists():
        for line in results.read_text().splitlines():
            entry = json.loads(line)
            if entry["tree"] != tree:
                sys.exit(f"{results} is about other files ({entry['tree'][:12]})")
            done[entry["mutant"]] = entry
    names = [m[0] for m in MUTANTS] + [e[0] for e in EQUIVALENT]
    if len(set(names)) != len(names):
        sys.exit("two mutants have one name")
    chosen = [n for n in args.only.split(";") if n] or [m[0] for m in MUTANTS]

    def record(name, result, detail):
        entry = {"mutant": name, "result": result, "detail": detail, "tree": tree}
        done[name] = entry
        with results.open("a") as out:
            out.write(json.dumps(entry) + "\n")
        print(f"{result:14s} {name}" + (f"  [{detail}]" if detail else ""), flush=True)

    # The copy, once: what the production build reads.
    marker = scratch / "TREE"
    if not marker.exists():
        if copy.exists():
            sys.exit(f"{copy} exists and has no marker: refusing to use it")
        copy.mkdir(parents=True)
        for part in ("CMakeLists.txt", "include", "lib", "tools", "runtime", "test"):
            source = root / part
            if source.is_dir():
                shutil.copytree(source, copy / part, ignore=shutil.ignore_patterns("__pycache__", "logs", "*.o"))
            else:
                shutil.copy2(source, copy / part)
        marker.write_text(tree)
    elif marker.read_text() != tree:
        sys.exit(f"{scratch} holds a copy of other files")
    for _, file, _, _ in MUTANTS:
        if (copy / file).read_bytes() != (root / file).read_bytes():
            sys.exit(f"{copy / file} is not the tree's: a mutant was left in it")
    code, text, _ = run(["cmake", "-G", "Ninja", "-S", copy, "-B", build, "-DCMAKE_BUILD_TYPE=Release",
                         f"-DLLVM_DIR={args.llvm_dir}", f"-DMLIR_DIR={args.mlir_dir}", "-DCMAKE_C_COMPILER=/usr/bin/gcc",
                         "-DCMAKE_CXX_COMPILER=/usr/bin/g++", f"-DLLVM_EXTERNAL_LIT={args.lit}"], timeout=BUILD_SECONDS)
    if code != 0:
        sys.exit("cmake failed:\n" + text[-2000:])

    def build_tools():
        return run(["ninja", "-C", build, "-j4", "systolic-opt", "systolic-translate", "systolic-cost"],
                   timeout=BUILD_SECONDS)

    def tests():
        code, text, timed_out = run([args.lit, "-v", build / "test", "--filter", "nest_records"])
        if timed_out:
            return "HARNESS_ERROR", "the tests timed out"
        if code == 0:
            return "SURVIVED", ""
        if any(marker in text for marker in CRASH):
            return "HARNESS_ERROR", "a crash: " + next(m for m in CRASH if m in text)
        mismatch = re.search(r"error: ([A-Z]+): (expected string not found in input|is not on the line after "
                             r"the previous match|excluded string found in input|found here)", text)
        if mismatch:
            return "KILLED", f"{mismatch.group(1)}: {mismatch.group(2)}"
        return "HARNESS_ERROR", f"exit {code} with no check that failed"

    if "BASELINE" not in done:
        code, text, _ = build_tools()
        if code != 0:
            sys.exit("the copy does not build:\n" + text[-2000:])
        result, detail = tests()
        if result != "SURVIVED":
            sys.exit(f"the copy does not pass: {result} {detail}")
        record("BASELINE", "PASSED", "")
    for name, _, _, reason in [(e[0], e[1], e[2], e[3]) for e in EQUIVALENT]:
        if name not in done:
            record(name, "EQUIVALENT", reason)
    for name, file, pattern, replacement in MUTANTS:
        if name not in chosen or name in done:
            continue
        path = copy / file
        original = path.read_text()
        if original.count(pattern) != 1:
            record(name, "HARNESS_ERROR", f"the pattern occurs {original.count(pattern)} times")
            continue
        try:
            path.write_text(original.replace(pattern, replacement))
            code, text, _ = build_tools()
            if code != 0:
                record(name, "HARNESS_ERROR", "the mutant does not build: " + text[-300:].replace("\n", " "))
                continue
            record(name, *tests())
        finally:
            path.write_text(original)
    code, _, _ = build_tools()
    if code != 0:
        sys.exit("the copy does not build after the last mutant was undone")
    counts = {}
    for name in chosen + [e[0] for e in EQUIVALENT]:
        if name in done:
            counts[done[name]["result"]] = counts.get(done[name]["result"], 0) + 1
    print(", ".join(f"{n} {r}" for r, n in sorted(counts.items())))
    bad = [n for n in chosen if n not in done or done[n]["result"] not in ("KILLED", "EQUIVALENT")]
    for name in bad:
        print("NOT KILLED:", name, file=sys.stderr)
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
