#!/usr/bin/env python3
"""Stage 1 mutation test (docs/GENERALIZED_GEMM_SCOPE.md): the copy of a nest and the record
read off the analyses. Each mutant weakens one thing a record relies on, in NestOutline.cpp
or NestRecord.cpp, and the unmodified tree must pass first. Every result is one of:

  KILLED          a test failed by its own assertion. Nothing else is a kill.
  SURVIVED        every test ran to the end and passed.
  EQUIVALENT      cannot be killed: listed below, with the reason. Not run.
  TIMEOUT         a test ran out of time and no test killed it.
  HARNESS_ERROR   a stale pattern, a mutant that does not build, or a crash that no
                  assertion explains.

Only KILLED and EQUIVALENT let a run pass. The tests are the ones stage 1 added, cheapest
first: the sections of the unit binary, the lit files nest_*, and the oracle with ten
programs and no floors. A test that stops by a signal, by an exit code that is no assertion's,
or because a driver it ran did so, is a crash and no kill. Each test runs in its own process
group under a wall-clock limit that kills the group by its id, and under an address-space
limit that its descendants inherit.

The existing analyses are not mutated here: their files are not stage 1's, and their own
harness (domain_mutation.py) is the one that mutates them.

Results are appended to --results as they finish, with the hash of the tree they are about.
A run skips mutants that file already holds for this tree, and refuses a file about another.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import resource
import shutil
import signal
import subprocess
import sys

O = "NestOutline.cpp"
N = "NestRecord.cpp"

MUTANTS = [
    # What the copy keeps.
    ("a store taken for a read", O, "    return index == 1 ? NestWrites : NestReads | NestWrites;\n", "    return NestReads;\n"),
    ("the array a loop is handed, taken for untouched", O,
     "          accessByRoot[found->second] |= op == loop ? NestReads | NestWrites : accessOf(op, i);\n",
     "          accessByRoot[found->second] |= op == loop ? 0 : accessOf(op, i);\n"),
    ("a value computed before the nest copied in", O, "      if (def && isa<arith::ConstantOp>(def))\n",
     "      if (def && !def->getNumRegions())\n"),
    ("the module's attributes left behind", O,
     "        module->getOperation()->setAttr(attr.getName(), attr.getValue());\n", "        (void)attr;\n"),
    ("the function's attributes left behind", O, "      copy->setAttr(attr.getName(), attr.getValue());\n",
     "      (void)attr;\n"),
    ("an argument's attributes left behind", O, "          copy.setArgAttrs(i, attrs);\n", "          (void)attrs;\n"),
    ("a copy that does not verify kept", O, "    if (!verifies) {\n", "    if (false) {\n"),
    # What is refused.
    ("a loop's value may leave", O, "      if (!result.use_empty()) {\n", "      if (false) {\n"),
    ("a parallel loop taken to run in order", O, "    if (!parallel.empty()) {\n", "    if (false) {\n"),
    ("a call is copied", O, "    if (!call.empty()) {\n", "    if (false) {\n"),
    ("a memref that is no root is copied", O, "    if (!foreignMemref.empty()) {\n", "    if (false) {\n"),
    ("a memref the nest makes is a root", O, "    if (!madeMemref.empty()) {\n", "    if (false) {\n"),
    ("a value that is no scalar enters", O, "    if (!refusal.empty()) {\n", "    if (false) {\n"),
    ("branches walked as one block", O, "    if (!function.getBody().hasOneBlock()) {\n", "    if (false) {\n"),
    ("a region of several blocks walked as one", O, "          if (!region.hasOneBlock())\n", "          if (false)\n"),
    # Separation, as it stands in the function.
    ("every pair apart by a proof", O,
     "    if (x.origin == FunctionRoot::FreshAllocation || y.origin == FunctionRoot::FreshAllocation) {\n",
     "    if (true) {\n"),
    ("one name authorizes every pair", O, "    } else if (authorized.count(s.name)) {\n",
     "    } else if (!authorized.empty()) {\n"),
    ("the hand-over under the function's names", O,
     "        s.outlinedName = separationName(nest.roots[a].outlined, nest.roots[b].outlined);\n",
     "        s.outlinedName = s.name;\n"),
    ("every pair handed over", N, "    if (separation.apart)\n      authorized.insert(separation.outlinedName);\n",
     "    authorized.insert(separation.outlinedName);\n"),
    # The order and how the nests meet.
    ("nests numbered backwards", O, "      nest.ordinal = entry.nest;\n",
     "      nest.ordinal = loops.size() - 1 - entry.nest;\n"),
    ("no nest has unknown effects", O,
     "    opaque.push_back(effects || !call.empty() || !foreignMemref.empty() || !madeMemref.empty());\n",
     "    opaque.push_back(false);\n"),
    ("a reader and a writer do not meet", O,
     "            if (!((a.access | b.access) & NestWrites) || !a.access || !b.access)\n",
     "            if (!(a.access & b.access & NestWrites))\n"),
    ("roots not known apart do not meet", O, "            if (!same && separation(a.root, b.root).apart)\n",
     "            if (!same)\n"),
    # What the record reads.
    ("A and B exchanged", N,
     "  r.views[1].root = functionRoot(siteGroup.first);\n  r.views[2].root = functionRoot(siteGroup.second);\n",
     "  r.views[1].root = functionRoot(siteGroup.second);\n  r.views[2].root = functionRoot(siteGroup.first);\n"),
    ("an extent one too long", N, "(uint64_t)(*level.upper - *level.lower) : 0;\n",
     "(uint64_t)(*level.upper - *level.lower + 1) : 0;\n"),
    ("the offset without the origin", N,
     "            offset += coefficient.getSExtValue() * *r.levels[level].lower;\n", "            offset += 0;\n"),
    ("M printed as N", N, "{\"M\", (int64_t)(*r.extents)[0]}", "{\"M\", (int64_t)(*r.extents)[1]}"),
    ("the scope dropped", N,
     "          scope.ranges.push_back({parameter.id, low.trunc(width), high.trunc(width)});\n",
     "          (void)low;\n"),
    # The ordered fold.
    ("the points run taken for the scope", N,
     "  bool overScope = verified && v.tier() == ComputationTier::Symbolic &&\n"
     "                   computation.status() == ComputationStatus::Verified &&\n"
     "                   computation.tier() == ComputationTier::Symbolic;\n",
     "  bool overScope = verified;\n"),
    ("the frame's writes ignored", N, "  r.orderedFoldVerified = overScope && !r.otherWrites;\n",
     "  r.orderedFoldVerified = overScope;\n"),
    ("two groups taken for one GEMM", N, "  if (r.groups != 1 || result.groups.size() != 1) {\n", "  if (false) {\n"),
    ("an operand that may overlap is not said", N,
     "      if (separation.name == separationName(c, operand) && !separation.apart)\n", "      if (false)\n"),
    # Whether the order may change.
    ("a fact at the points run is a fact over the scope", N,
     "  return fact.status == FactStatus::Proven && fact.tier == (int)ComputationTier::Symbolic;\n",
     "  return fact.status == FactStatus::Proven;\n"),
    ("the flag of the addition not read", N,
     "    reassoc = reassoc && (program.sites[site].reductionFlags & FlagReassoc);\n",
     "    reassoc = reassoc && true;\n"),
    ("integers reorder without the fact", N, "  } else if (!isFloat && provenOverScope(v.integerReorder())) {\n",
     "  } else if (!isFloat) {\n"),
    ("floats reorder without the flag", N, "  } else if (isFloat && reassoc) {\n", "  } else if (isFloat) {\n"),
    # Whether the record gives a box and three views.
    ("a candidate with no single order is mapped", N, "  if (!facts.singleOrder) {\n", "  if (false) {\n"),
    ("the logical tier not asked", N, "  if (!facts.logicalSymbolic)\n", "  if (false)\n"),
    ("the logical tier read off the wrong claim", N,
     "  facts.logicalSymbolic = v.logicalTier() == ComputationTier::Symbolic;\n",
     "  facts.logicalSymbolic = v.tier() == ComputationTier::Symbolic;\n"),
    ("an inexact order is no obstacle", N, "  if (!facts.exactOrder)\n", "  if (false)\n"),
    ("several events a point are no obstacle", N, "  if (!facts.unitMultiplicity)\n", "  if (false)\n"),
    ("a guard is no obstacle", N, "  if (r.guard != \"true\")\n", "  if (false)\n"),
    ("a level that is no coordinate is no obstacle", N, "  if (repeats)\n", "  if (false)\n"),
    ("a level on two axes is no obstacle", N, "  if (ambiguous)\n", "  if (false)\n"),
    ("a batch level is no obstacle", N, "  if (perAxis[0])\n", "  if (false)\n"),
    ("two levels on one axis are a box", N, "    if (perAxis[axis] != 1)\n", "    if (false)\n"),
    ("a symbolic lower bound is no obstacle", N, "    if (!level.lower)\n", "    if (false)\n"),
    ("a symbolic bound is no obstacle", N, "    if (!level.upper)\n", "    if (false)\n"),
    ("an empty level is no obstacle", N, "    if (level.extent && !*level.extent)\n", "    if (false)\n"),
    ("a descending level is mapped", N, "    if (level.descending)\n", "    if (false)\n"),
    ("a view on no root is a view", N, "    if (one.root < 0)\n", "    if (false)\n"),
    ("a view with no closed form is a view", N, "    if (!one.proposed) {\n", "    if (false) {\n"),
    ("a view not proved is a view", N, "    if (!one.proved)\n", "    if (false)\n"),
    ("a view that is not affine is a view", N,
     "      if (!dimension.affine) {\n        out.push_back(where + \" is not affine: \" + dimension.text);\n"
     "        continue;\n      }\n",
     "      if (!dimension.affine)\n        continue;\n"),
    ("a view with a symbolic offset is a view", N, "      if (!dimension.parameters.empty())\n", "      if (false)\n"),
    ("a view may read any level", N, "        if (!std::strchr(roles[view], axis))\n", "        if (false)\n"),
    ("shared cells are no obstacle", N, "  if (!facts.outputInjective)\n", "  if (false)\n"),
    ("shared cells taken for apart", N, "  facts.outputInjective = provenOverScope(domain.outputInjective());\n",
     "  facts.outputInjective = true;\n"),
    ("several events a point taken for one", N,
     "  facts.unitMultiplicity = domain.multiplicity() == DomainMultiplicity::Unit;\n",
     "  facts.unitMultiplicity = true;\n"),
    ("sites that disagree are one update", N, "  if (!r.flagsAgree)\n", "  if (false)\n"),
    ("verified is mapped", N, "             : mapped && r.refusals.empty() ? NestRecord::Verified\n",
     "             : true ? NestRecord::Verified\n"),
    # No record authorizes.
    ("a record authorizes", N, "                   {\"authorizes_lowering\", false}};\n",
     "                   {\"authorizes_lowering\", true}};\n"),
    ("a function's records authorize", N,
     "                   {\"authorizes_lowering\", false},\n                   {\"composition\",",
     "                   {\"authorizes_lowering\", true},\n                   {\"composition\","),
]

# Not run. Each is a check that no input reaches while the code above it is as it is.
EQUIVALENT = [
    ("the copy may read the function", O, "    if (escapes) {\n", "    if (false) {\n",
     "every value the nest reads and does not define is collected and mapped before the clone, so no operand "
     "of the copy is outside it: the check guards the collection itself"),
    ("an order taken for exact", N, "  facts.exactOrder = domain.exactOrder();\n", "  facts.exactOrder = true;\n",
     "Theorem 13 at the symbolic tier rests on Theorem 12, whose claim is the exact order, so the checkers "
     "give no verified fold with an inexact order; what the record does with an inexact order is asked of "
     "mappingRefusals on the data (the mutant 'an inexact order is no obstacle')"),
    ("a box with no constant extents is mapped", N, "  bool mapped = unmapped.empty() && r.extents.has_value();\n",
     "  bool mapped = unmapped.empty();\n",
     "the extents are unset only for a guard, a level that is no coordinate, a batch level, an axis without "
     "exactly one level or a bound that is no constant, and each of those is one of mappingRefusals' reasons"),
]

TEST_SECONDS = 1500
BUILD_SECONDS = 3600
MEMORY_LIMIT = 8 << 30


class Finished:
    def __init__(self, code, text, timed_out):
        self.code, self.text, self.timed_out = code, text, timed_out


def run(command, cwd=None, timeout=TEST_SECONDS, memory=MEMORY_LIMIT):
    def limit():
        if memory:
            resource.setrlimit(resource.RLIMIT_AS, (memory, memory))

    process = subprocess.Popen([str(part) for part in command], cwd=cwd, stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT, text=True, start_new_session=True, preexec_fn=limit)
    try:
        text, _ = process.communicate(timeout=timeout)
        return Finished(process.returncode, text, False)
    except subprocess.TimeoutExpired:
        try:
            os.killpg(process.pid, signal.SIGKILL)  # The group it started, by its id.
        except ProcessLookupError:
            pass
        text, _ = process.communicate()
        return Finished(None, text, True)


CRASH = ("Stack dump", "PLEASE submit a bug report", "core dumped", "Assertion `", "terminate called",
         "Segmentation fault", "std::bad_alloc", "' is empty", "exit status: -", "exit status: 134",
         "exit status: 139")


def verdict_of(kind, done):
    """'pass', 'assertion', 'timeout' or 'crash', and one line of evidence."""
    if done.timed_out:
        return "timeout", ""
    if done.code == 0:
        return "pass", ""
    lines = done.text.strip().splitlines()
    if kind == "unit":
        last = re.fullmatch(r"(\d+) checks, (\d+) failures", lines[-1]) if lines else None
        failed = [line for line in lines if line.startswith("FAIL ")]
        if done.code == 1 and last and int(last.group(2)) > 0 and failed:
            return "assertion", failed[0]
        return "crash", f"exit {done.code}"
    if kind == "lit":
        mismatch = [line for line in lines if re.search(r"error: [-A-Z]+: (expected string not found in input|"
                                                        r"is not on the line after the previous match|"
                                                        r"excluded string found in input)", line)]
        if done.code == 1 and "Failed Tests" in done.text and mismatch and not any(m in done.text for m in CRASH):
            return "assertion", mismatch[0].strip()
        return "crash", f"exit {done.code}"
    failed = [line for line in lines if line.startswith("FAIL: ")]
    real = [line for line in failed if not re.match(r"FAIL: (exit -?\d+:|timeout:)", line)]
    if done.code == 1 and real:
        return "assertion", real[0]
    return "crash", f"exit {done.code}: {failed[0] if failed else ''}"


def tree_hash(root):
    digest = hashlib.sha256()
    for part in ("include/GeneralizedGemm", "lib/GeneralizedGemm", "research/generalized-gemm"):
        for path in sorted((root / part).rglob("*")):
            if path.is_file() and "__pycache__" not in path.parts:
                digest.update(str(path.relative_to(root)).encode())
                digest.update(path.read_bytes())
    return digest.hexdigest()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", required=True, help="the repository root")
    parser.add_argument("--scratch", required=True)
    parser.add_argument("--llvm-dir", required=True)
    parser.add_argument("--mlir-dir", required=True)
    parser.add_argument("--lit", required=True, help="llvm-lit")
    parser.add_argument("--results", required=True, help="a JSON-lines file, appended to")
    parser.add_argument("--only", default="", help="mutant names, semicolon-separated")
    parser.add_argument("--oracle-programs", type=int, default=10)
    parser.add_argument("--jobs", type=int, default=4, help="driver runs at a time inside the oracle")
    args = parser.parse_args()
    root = Path(args.source).resolve()
    scratch = Path(args.scratch).resolve()
    library, build = scratch / "lib", scratch / "build"
    tree = tree_hash(root)
    results = Path(args.results)
    done = {}
    if results.exists():
        for line in results.read_text().splitlines():
            entry = json.loads(line)
            if entry["tree"] != tree:
                sys.exit(f"{results} is about another tree ({entry['tree'][:12]}), this one is {tree[:12]}")
            done[entry["mutant"]] = entry
    marker = scratch / "TREE"
    if marker.exists() and marker.read_text() != tree:
        sys.exit(f"{scratch} holds a snapshot of another tree")
    scratch.mkdir(parents=True, exist_ok=True)

    def record(name, result, detail):
        entry = {"mutant": name, "result": result, "detail": detail, "tree": tree}
        done[name] = entry
        with results.open("a") as out:
            out.write(json.dumps(entry) + "\n")
        print(f"{result:14s} {name}" + (f"  [{detail}]" if detail else ""), flush=True)

    names = [m[0] for m in MUTANTS] + [m[0] for m in EQUIVALENT]
    if len(set(names)) != len(names):
        sys.exit("two mutants have one name")
    chosen = [name.strip() for name in args.only.split(";") if name.strip()] or [m[0] for m in MUTANTS]
    for name in chosen:
        if name not in [m[0] for m in MUTANTS]:
            sys.exit(f"no mutant named '{name}'")

    # A snapshot of the library, built once; each mutant then changes one file of it.
    if not marker.exists():
        if library.exists():
            sys.exit(f"{library} exists and has no marker: refusing to use it")
        shutil.copytree(root / "lib" / "GeneralizedGemm", library)
        marker.write_text(tree)
    for _, file, _, _ in MUTANTS:
        if (library / file).read_bytes() != (root / "lib" / "GeneralizedGemm" / file).read_bytes():
            sys.exit(f"{library / file} is not the tree's: a mutant was left in it")
    configured = run(["cmake", "-S", root / "research" / "generalized-gemm", "-B", build, "-G", "Ninja",
                      "-DCMAKE_BUILD_TYPE=Debug", f"-DGG_LIB_DIR={library}", f"-DLLVM_DIR={args.llvm_dir}",
                      f"-DMLIR_DIR={args.mlir_dir}"], timeout=BUILD_SECONDS, memory=0)
    if configured.code != 0:
        sys.exit(f"cmake failed:\n{configured.text[-1500:]}")
    targets = ["generalized-gemm-nest-test", "generalized-gemm-nest-report", "generalized-gemm-domain-report"]

    def compile_all():
        return run(["ninja", "-C", build, "-j4"] + targets, timeout=BUILD_SECONDS, memory=0)

    unit = build / "generalized-gemm-nest-test"
    oracle = root / "research" / "generalized-gemm" / "test" / "fuzz" / "nest_oracle.py"
    tests = [("unit outline", "unit", [unit, "outline"]),
             ("unit mapping", "unit", [unit, "mapping"]),
             ("unit composition", "unit", [unit, "composition"]),
             ("lit nest_outline", "lit", [args.lit, "-v", build / "lit", "--filter", "nest_outline"]),
             ("unit product", "unit", [unit, "product"]),
             ("unit effects", "unit", [unit, "effects"]),
             ("unit refusals", "unit", [unit, "refusals"]),
             ("unit nests", "unit", [unit, "nests"]),
             ("lit nest records", "lit", [args.lit, "-v", build / "lit", "--filter",
                                          "nest_(product|shared|stored|reads|refusals)"]),
             ("oracle", "oracle", [sys.executable, oracle, "--driver", build / "generalized-gemm-nest-report",
                                   "--domain-driver", build / "generalized-gemm-domain-report", "--programs",
                                   str(args.oracle_programs), "--jobs", str(args.jobs), "--no-minimum"])]

    def run_tests():
        """The first assertion that fails, or what else went wrong, or nothing."""
        trouble = None
        for label, kind, command in tests:
            verdict, evidence = verdict_of(kind, run(command))
            if verdict == "assertion":
                return "KILLED", f"{label}: {evidence[:160]}"
            if verdict != "pass" and trouble is None:
                trouble = ("TIMEOUT" if verdict == "timeout" else "HARNESS_ERROR", f"{label}: {verdict} {evidence[:160]}")
        return trouble or ("SURVIVED", "")

    if "BASELINE" not in done:
        built = compile_all()
        if built.code != 0:
            sys.exit(f"the unmodified tree does not build:\n{built.text[-2000:]}")
        result, detail = run_tests()
        if result != "SURVIVED":
            sys.exit(f"the unmodified tree does not pass: {result} {detail}")
        record("BASELINE", "PASSED", "")
    for name, _, _, _, reason in EQUIVALENT:
        if name not in done:
            record(name, "EQUIVALENT", reason)

    for name, file, pattern, replacement in MUTANTS:
        if name not in chosen or name in done:
            continue
        path = library / file
        original = path.read_text()
        if original.count(pattern) != 1:
            record(name, "HARNESS_ERROR", f"the pattern occurs {original.count(pattern)} times")
            continue
        try:
            path.write_text(original.replace(pattern, replacement))
            built = compile_all()
            if built.timed_out or built.code != 0:
                record(name, "HARNESS_ERROR", "the mutant does not build: " + built.text[-300:].replace("\n", " "))
                continue
            record(name, *run_tests())
        finally:
            path.write_text(original)
    restored = compile_all()
    if restored.code != 0:
        sys.exit("the tree does not build after the last mutant was undone")

    counts = {}
    for name in chosen + [m[0] for m in EQUIVALENT]:
        if name in done:
            counts[done[name]["result"]] = counts.get(done[name]["result"], 0) + 1
    print(", ".join(f"{count} {result}" for result, count in sorted(counts.items())))
    bad = sorted(name for name in chosen if name not in done or done[name]["result"] not in ("KILLED", "EQUIVALENT"))
    for name in bad:
        print("NOT KILLED:", name, file=sys.stderr)
    sys.exit(1 if bad else 0)


if __name__ == "__main__":
    main()
