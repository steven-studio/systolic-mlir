#!/usr/bin/env python3
"""Mutation test of the G2a checker: each mutant must be caught.

Works on a private snapshot of the sources, so editing the tree during a run
cannot change what is tested, and only after the unmodified snapshot passes the
same tests. A mutant that passes everything fails this script. A mutant stops at
its first killer, so the kill counts do not measure a suite alone.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import shutil
import subprocess
import sys

C = "TerminationCertificate.cpp"
O = "TerminationObligations.cpp"
P = "CutPaths.cpp"
G = "ControlStructure.cpp"
A = "TerminationAnalysis.cpp"
Z = "Z3Backend.cpp"
R = "TerminationProposers.cpp"
MUTANTS = [
    # Formulas.
    ("Cont negated", O, "      premise,\n      postTerm(program, path, continues(program, set, candidates, path.to)));",
     "      premise,\n      terms.logicalNot(postTerm(program, path, continues(program, set, candidates, path.to))));"),
    ("Cont dropped", O, "      premise,\n      postTerm(program, path, continues(program, set, candidates, path.to)));",
     "      premise,\n      terms.boolean(true));"),
    ("strict descent is weak", O, "strict ? Predicate::Ult : Predicate::Ule", "Predicate::Ule"),
    ("weak descent is strict", O, "strict ? Predicate::Ult : Predicate::Ule", "Predicate::Ult"),
    ("descent reversed", O, "Term decreased = terms.icmp(strict ? Predicate::Ult : Predicate::Ule, next,\n                              rankAtSource);",
     "Term decreased = terms.icmp(strict ? Predicate::Ugt : Predicate::Uge, next,\n                              rankAtSource);"),
    ("descent vacuous", O, "Term premise = terms.logicalAnd(terms.logicalAnd(scope, invariant), path.condition);\n  // A run",
     "Term premise = terms.boolean(false);\n  // A run"),
    ("descent ignores invariant", O, "Term premise = terms.logicalAnd(terms.logicalAnd(scope, invariant), path.condition);\n  // A run",
     "Term premise = terms.logicalAnd(scope, path.condition);\n  // A run"),
    ("rank read at source twice", O, "Term next = postTerm(program, path, rankAtTarget);", "Term next = rankAtTarget;"),
    ("consecution vacuous", O, "Term premise = terms.logicalAnd(terms.logicalAnd(scope, source), path.condition);\n  return terms.logicalAnd(premise,",
     "Term premise = terms.boolean(false);\n  return terms.logicalAnd(premise,"),
    ("invariant read at source", O, "terms.logicalNot(postTerm(program, path, target)));", "terms.logicalNot(target));"),
    ("stop check vacuous", O, "      terms.logicalAnd(terms.logicalAnd(scope, invariant), site.reached),\n      site.violated);",
     "      terms.boolean(false),\n      site.violated);"),
    ("guards: none ignored", O, "return found ? terms.logicalOr(none, overlap) : nullptr;", "return found ? overlap : nullptr;"),
    ("guards: overlap ignored", O, "return found ? terms.logicalOr(none, overlap) : nullptr;", "return found ? none : nullptr;"),
    ("scope upper bound ignored", O, "terms.icmp(Predicate::Sle, value, terms.bv(range.upper))", "terms.boolean(true)"),
    ("scope lower bound flipped", O, "terms.icmp(Predicate::Sge, value, terms.bv(range.lower))", "terms.icmp(Predicate::Sle, value, terms.bv(range.lower))"),
    ("plain script allows any command", O, 'else if (command != "declare-const" && command != "define-fun" &&\n             command != "assert") {', "else if (false) {"),
    ("proof terms may read memory", O, 'why = "a proof term may not read memory or floats";\n      return nullptr;', "return n;"),
    ("state leaves unchecked", O, "if (n->index >= program.state.size() ||\n          program.state[n->index].sort != n->sort) {", "if (false) {"),
    # Paths.
    ("stops left out of conditions", P, "        condition =\n            terms.logicalAnd(condition, terms.logicalNot(side.violated));", "        (void)condition;"),
    ("stops negated in conditions", P, "terms.logicalAnd(condition, terms.logicalNot(side.violated));", "terms.logicalAnd(condition, side.violated);"),
    ("float stops kept in conditions", P, "if (!containsFloat(side.violated))", "if (true)"),
    ("no stop sites recorded", P, "set.sites.push_back(std::move(site));", ""),
    ("entry shadows free", P, "start.shadow.assign(program.state.size(), terms.boolean(false));", ""),
    ("post ignores shadow", P, "      return path.shadow[n->index];\n    if (n->op == TermOp::MemVar)\n      return path.memory[n->index];\n    return nullptr;\n  };\n  return program.terms->substitute",
     "      return nullptr;\n    if (n->op == TermOp::MemVar)\n      return path.memory[n->index];\n    return nullptr;\n  };\n  return program.terms->substitute"),
    ("post ignores memory", P, "      return path.memory[n->index];\n    return nullptr;\n  };\n  return program.terms->substitute", "      return nullptr;\n    return nullptr;\n  };\n  return program.terms->substitute"),
    ("post ignores state", P, "      return path.state[n->index];\n    if (n->op == TermOp::Shadow)\n      return path.shadow[n->index];\n    if (n->op == TermOp::MemVar)\n      return path.memory[n->index];\n    return nullptr;\n  };\n  return program.terms->substitute",
     "      return nullptr;\n    if (n->op == TermOp::Shadow)\n      return path.shadow[n->index];\n    if (n->op == TermOp::MemVar)\n      return path.memory[n->index];\n    return nullptr;\n  };\n  return program.terms->substitute"),
    # Graphs.
    ("cycles past the cuts accepted", G, "if (!remaining.empty()) {", "if (false) {"),
    ("self loops are no cycle", G, "if (component.size() > 1 || selfEdge[node]) {", "if (component.size() > 1) {"),
    ("entry may be a cut", G, "if (cuts[i] == program.entryLocation) {", "if (false) {"),
    # Checker.
    ("unknown discharges", C, "bool wanted = response.answer == (record.expected == \"unsat\" ? SmtAnswer::Unsat\n                                                               : SmtAnswer::Sat);",
     "bool wanted = response.answer != (record.expected == \"unsat\" ? SmtAnswer::Sat\n                                                               : SmtAnswer::Unsat);"),
    ("verdict promoted", C, "      everyStopUnreachable\n          ? CheckerAuthority::proven(\"O0-O7 discharged\")", "      true\n          ? CheckerAuthority::proven(\"O0-O7 discharged\")"),
    ("fresh reads ignored", C, "if (effect.kind == MemoryEffect::Read &&\n          program.roots[effect.root].origin == MemoryRoot::FreshAllocation) {", "if (false) {"),
    ("fingerprint ignored", C, 'if (!structural("fingerprint", certificate.fingerprint == program.fingerprint,', 'if (!structural("fingerprint", true,'),
    ("cut check skipped", C, 'if (!structural("cut set", cutSetBreaksCycles(program, certificate.cuts, why), why))', 'if (false)'),
    ("empty strict accepted", C, "    if (strict.empty())\n      return structural", "    if (false)\n      return structural"),
    ("SCC count not compared", C, "if (components.size() != levels.size())", "if (false)"),
    ("unmatched SCC accepted", C, 'if (!match)\n      return structural("levels", false, "an SCC has no proof node");', 'if (!match)\n      continue;'),
    ("listed paths not compared", C, "if (listed != std::set<unsigned>(inside.begin(), inside.end()))", "if (false)"),
    ("children not checked", C, "if (!checkLevels(rest, level.children))", "if (false)"),
    ("children not discharged", C, "all = dischargeLevel(child) && all;", "(void)child;"),
    ("O3 and O4 skipped", C, "    if (path.to == program.exitLocation())\n      continue;\n    ObligationRecord record;\n    record.id = path.fromEntry ? \"O3\" : \"O4\";", "    if (true)\n      continue;\n    ObligationRecord record;\n    record.id = path.fromEntry ? \"O3\" : \"O4\";"),
    ("guards skipped", C, "proof = dischargeInvariants() && proof;", "dischargeGuards(); proof = dischargeInvariants();"),
    ("O5 and O6 failures ignored", C, "proof = dischargeLevel(level) && proof;", "dischargeLevel(level);"),
    ("domain not required", C, 'if (!dischargeDomain())\n      return stopWith("the parameter domain is empty or unknown");', "dischargeDomain();"),
    ("assumption gate open", C, "if (!isDischarged(assumption, options.authorized))\n        return stopWith", "if (false)\n        return stopWith"),
    ("assumption drops from status", C, "status = conjoin(status, assumption);", ""),
    ("stop memory unguarded", C, "if (usesMemory && !assumptionsAuthorized()) {", "if (false) {"),
    ("theorem memory untracked", C, 'if (record.id != "O7")\n    outcome.dependsOnMemory', 'if (false)\n    outcome.dependsOnMemory'),
    ("O7 failure counts as discharged", C, "    } else {\n      report.status = StopStatus::Unknown;\n      if (report.detail.empty())\n        report.detail = where;\n    }", "    } else {\n      ++report.discharged;\n    }"),
    ("configuration ignores memory", C, "a.shadow == b.shadow &&\n         sameCells(a.memory, b.memory);", "a.shadow == b.shadow;"),
    ("configuration ignores shadow", C, "a.state == b.state && a.shadow == b.shadow &&", "a.state == b.state &&"),
    ("configuration ignores state", C, "a.location == b.location && a.state == b.state &&", "a.location == b.location &&"),
    ("configuration ignores location", C, "return a.location == b.location && a.state == b.state", "return a.state == b.state"),
    ("witness needs no repeat", C, "if (!sameConfiguration(anchor, *config))", "if (false)"),
    ("witness period may be zero", C, "if (witness.period == 0)", "if (false)"),
    ("witness outside the scope", C, "if (!parametersInScope(scope, witness.entry.parameters))", "if (false)"),
    ("witness run may end", C, '    if (!advance())\n      return CheckedVerdict::unknown("the witness run ended in its period");', "    advance();"),
    # Analysis.
    ("no conflict on a cycle", A, "        if (proven) {\n          result.conflict = \"a proof and an exact cycle disagree\";", "        if (false) {\n          result.conflict = \"a proof and an exact cycle disagree\";"),
    ("negative extents in the domain", O, "    if (p.kind == Parameter::Dimension)\n      all = terms.logicalAnd(", "    if (false)\n      all = terms.logicalAnd("),
    ("scope on a float", O, "if (p.sort.kind != Sort::BV) {\n      why = \"a range is on", "if (false) {\n      why = \"a range is on"),
    ("lowered cycle refutes", C, "if (program.structuredLoweringApplied)\n    return CheckedVerdict::unknown(", "if (false)\n    return CheckedVerdict::unknown("),
    ("permissive float ignored", C, "if (permissive && containsFloat(site.violated)) {", "if (false) {"),
    ("no conflict on a stop", A, "if (proven && report.status == StopStatus::Unreachable) {", "if (false) {"),
    ("refutation not replayed", A, "if (refuted.kind() == TerminationVerdict::RefutedNonterminating) {", "if (true) {"),
    ("replay ignores scope", A, "if (points.size() >= limits.maxReplayEntries ||\n        !parametersInScope(scope, point))", "if (points.size() >= limits.maxReplayEntries)"),
    # Solver boundary.
    ("solver unknown is unsat", Z, "    case z3::unknown:\n      response.detail = solver.reason_unknown();\n      return response;", "    case z3::unknown:\n      response.answer = SmtAnswer::Unsat;\n      return response;"),
    ("scripts not screened", Z, "if (!isPlainScript(script, why)) {", "if (false) {"),
    # Coverage: a template that stops working must show.
    ("T1 removed", R, "tryAll(guardDistances(component))", "false"),
    ("T2 removed", R, "tryAll(modularDistances(component, inside))", "false"),
    ("T3 removed", R, "tryAll(affineRanks(component, inside))", "false"),
    ("T4 removed", R, "  if (!solve(rest, children))\n    return false;", "  if (!rest.empty())\n    return false;"),
    ("Houdini removed", R, "      for (const CutPath &path : in.set.paths)\n        if (path.to != program.exitLocation())\n          refine(path, active, changed);", "      active.clear();"),
]


def run(command, cwd=None, timeout=1500):
    return subprocess.run([str(c) for c in command], cwd=cwd, capture_output=True, text=True, timeout=timeout)


class Worker:
    def __init__(self, index, scratch, root, args):
        self.lib = scratch / f"lib{index}"
        self.build = scratch / f"build{index}"
        self.root, self.args = root, args
        if not self.lib.exists():
            shutil.copytree(root / "lib" / "GeneralizedGemm", self.lib)
        configured = run(["cmake", "-S", root / "research" / "generalized-gemm", "-B", self.build,
                          "-G", "Ninja", "-DCMAKE_BUILD_TYPE=Debug", f"-DGG_LIB_DIR={self.lib}",
                          f"-DLLVM_DIR={args.llvm_dir}", f"-DMLIR_DIR={args.mlir_dir}"])
        if configured.returncode:
            raise RuntimeError(f"cmake failed:\n{configured.stderr[-1500:]}")

    def restore(self):
        original = self.root / "lib" / "GeneralizedGemm"
        for f in original.iterdir():
            copy = self.lib / f.name
            if copy.read_text() != f.read_text():
                shutil.copy(f, copy)

    def compile(self):
        return run(["ninja", "-C", self.build, "-j3", "generalized-gemm-termination-test",
                    "generalized-gemm-termination-report", "generalized-gemm-loops"])

    def commands(self):
        fuzz = self.root / "research" / "generalized-gemm" / "test" / "fuzz" / "termination_exhaustive.py"
        catalogue = self.root / "research" / "generalized-gemm" / "test" / "lit" / "Inputs" / "gen_loops.py"
        return [
            ("oracle", [sys.executable, fuzz, "--driver", self.build / "generalized-gemm-termination-report",
                        "--programs", "120", "--seed", "21", "--jobs", "2", "--catalogue", catalogue]),
            ("unit", [self.build / "generalized-gemm-termination-test", self.build / "loops4.mlir",
                      self.build / "loops8.mlir"]),
        ]

    def trial(self, name, filename, old, new):
        self.restore()
        path = self.lib / filename
        text = path.read_text()
        if old not in text:
            return f"STALE {name}", False
        path.write_text(text.replace(old, new, 1))
        built = self.compile()
        if built.returncode:
            return f"NO-BUILD {name}\n{built.stdout[-600:]}", False
        for label, command in self.commands():
            try:
                if run(command).returncode:
                    return f"KILLED by {label}: {name}", True
            except subprocess.TimeoutExpired:
                return f"KILLED by timeout in {label}: {name}", True
        return f"SURVIVED: {name}", False


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", required=True, help="the repository root")
    parser.add_argument("--scratch", required=True)
    parser.add_argument("--llvm-dir", required=True)
    parser.add_argument("--mlir-dir", required=True)
    parser.add_argument("--only", default="", help="comma-separated mutant names")
    parser.add_argument("--workers", type=int, default=3)
    args = parser.parse_args()

    scratch = Path(args.scratch).resolve()
    scratch.mkdir(exist_ok=True)
    snapshot = scratch / "tree"
    if not snapshot.exists():
        source = Path(args.source).resolve()
        snapshot.mkdir()
        for part in ["include", "lib", "research"] + [
                f"test/{p.name}" for p in (source / "test").glob("polynomial*")]:
            (snapshot / part).parent.mkdir(parents=True, exist_ok=True)
            shutil.copytree(source / part, snapshot / part)
    workers = [Worker(i, scratch, snapshot, args) for i in range(args.workers)]

    # A kill only means something if the unmodified tree passes the same tests.
    def baseline(worker):
        worker.restore()
        built = worker.compile()
        if built.returncode:
            return f"BASELINE NO-BUILD\n{built.stdout[-1500:]}"
        for label, command in worker.commands():
            result = run(command)
            if result.returncode:
                return f"BASELINE FAILED: {label}\n{result.stdout[-1500:]}\n{result.stderr[-1500:]}"
        return None

    with ThreadPoolExecutor(args.workers) as pool:
        for failure in pool.map(baseline, workers[:1]):
            if failure:
                print(failure)
                return 2
    print("baseline passes", flush=True)

    names = [n for n in args.only.split(",") if n]
    unknown = sorted(set(names) - {m[0] for m in MUTANTS})
    if unknown:
        print(f"no such mutants: {unknown}")
        return 2
    chosen = [m for m in MUTANTS if not names or m[0] in names]
    queue = list(chosen)
    survivors = []

    def serve(worker):
        while True:
            try:
                mutant = queue.pop(0)
            except IndexError:
                return
            try:
                line, killed = worker.trial(*mutant)
            except Exception as error:  # A harness failure is no kill.
                line, killed = f"ERROR {mutant[0]}: {error}", False
            print(line, flush=True)
            if not killed:
                survivors.append(mutant[0])

    with ThreadPoolExecutor(args.workers) as pool:
        list(pool.map(serve, workers))
    print(f"{len(chosen) - len(survivors)} of {len(chosen)} killed; survivors: {survivors}")
    return 1 if survivors else 0


if __name__ == "__main__":
    sys.exit(main())
