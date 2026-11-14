#!/usr/bin/env python3
"""Mutation test of G3b: each mutant must be caught.

Works on a private snapshot, and only after the unmodified snapshot passes the
same tests. A mutant that passes everything, an edit that no longer applies and
an empty selection all fail this script.
"""
import argparse
import hashlib
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import shutil
import subprocess
import sys

F = "CoordinateFamily.cpp"
Y = "SymbolicDomain.cpp"
V = "CoverageSimulation.cpp"
C = "SymbolicCandidateChecker.cpp"
G = "CoordinateGrammar.cpp"
S = "CoordinateSynthesis.cpp"
A = "SynthesisAnalysis.cpp"
D = "FiniteDomain.cpp"
K = "CandidateChecker.cpp"
T = "SiteSets.cpp"  # G5b-M4-0: the group gate G3b, G4a and G5a share.
I = "IndexProgram.cpp"       # Statement-level index programs (thesis App. G, bit-vector adaptation).
L = "IndexSlice.cpp"         # Their specification and obligations.
Z = "ClosedFormSynthesis.cpp"
N = "LoopCounters.cpp"
MUTANTS = [
    # Definedness and evaluation.
    ("division ignored", F, "case TermOp::UDiv: case TermOp::URem: case TermOp::SDiv: case TermOp::SRem: {", "case TermOp::SDiv: case TermOp::SRem: {"),
    ("signed overflow ignored", F, "if (n->op == TermOp::SDiv || n->op == TermOp::SRem) {", "if (false) {"),
    ("shift ignored", F, "case TermOp::Shl: case TermOp::LShr: case TermOp::AShr: {", "case TermOp::AShr: {"),
    ("shift bound off by one", F, "terms.icmp(Predicate::Ult, n->args[1], terms.bv(APInt(width, width)))", "terms.icmp(Predicate::Ule, n->args[1], terms.bv(APInt(width, width)))"),
    ("inner terms unseen", F, "    for (Term arg : n->args)\n      visit(arg);\n", ""),
    ("undefined event accepted", F, "        if (ok->isZero()) {", "        if (false) {"),
    ("labels ignore the tuple", F, "        tuple.push_back(*value);\n", ""),
    # Reach.
    ("premise drops side conditions", Y, "      premise = terms.logicalAnd(premise, terms.logicalNot(side.violated));", "      (void)side;"),
    ("premise accepts memory guards", Y, "  if (readsMemory(path.condition) || containsFloat(path.condition))\n    return nullptr;\n", ""),
    ("arrivals stop at sites", Y, "        if (next.end != program.exitLocation() && !isCut[next.end])\n          walk(next);", "        if (next.end != program.exitLocation() && !isCut[next.end] && !llvm::any_of(sites, [&](unsigned id) { return program.sites[id].location == next.end; }))\n          walk(next);"),
    ("entry not walked", Y, "    walkFrom(program.entryLocation, terms.boolean(true));\n", ""),
    ("cuts not walked", Y, "    for (unsigned cut : cuts) {\n      auto found = invariants.find(cut);\n      walkFrom(cut, found == invariants.end() ? terms.boolean(true) : found->second);\n    }\n", ""),
    ("invariants dropped", Y, "walkFrom(cut, found == invariants.end() ? terms.boolean(true) : found->second);", "walkFrom(cut, terms.boolean(true));"),
    ("entry shadows free", Y, "    if (source == program.entryLocation)\n      begin.shadow.assign(program.state.size(), terms.boolean(false));\n", ""),
    ("arrival ignores the scope", Y, "arrival.premise = terms.logicalAnd(terms.logicalAnd(scope, start), premise);", "arrival.premise = terms.logicalAnd(start, premise);"),
    ("second copy shares state", Y, "      return terms.state(n->index + offset, n->sort);", "      return nullptr;"),
    ("liveness ignores guards", Y, "      changed = add(here, t.guard) || changed;\n", ""),
    ("liveness ignores definitions", Y, "        if (next.erase(id))\n          changed = add(here, term) || changed;\n", "        next.erase(id);\n"),
    # Obligations.
    ("V0 skipped", C, 'if (!require("V0", "definedness", terms.logicalAnd(a.premise, terms.logicalNot(defined))))\n        return false;', ""),
    ("V1h skipped", C, """if (!dependence("V1h", keyH, first.output->index, second.output->index) ||\n          !dependence("V1f",""", """if (!dependence("V1f","""),
    ("V1f skipped", C, """          !dependence("V1f", keyF, first.a.index, second.a.index) ||\n""", ""),
    ("V1g skipped", C, """ ||\n          !dependence("V1g", keyG, first.b.index, second.b.index))""", ")"),
    ("h key is f key", C, 'dependence("V1h", keyH,', 'dependence("V1h", keyF,'),
    ("f compares g", C, 'dependence("V1f", keyF, first.a.index, second.a.index)', 'dependence("V1f", keyF, first.b.index, second.b.index)'),
    ("different lengths agree", C, "  if (first.size() != second.size())\n    return terms.boolean(true);\n", "  if (first.size() != second.size())\n    return terms.boolean(false);\n"),
    ("V1 needs no equal key", C, "terms.logicalAnd(terms.logicalAnd(both, equalOn(key, a, b)),\n                                        differOn(left, right, a, b)));", "terms.logicalAnd(both, terms.boolean(false)));"),
    ("V2 ignored", C, "        injective = false;\n", "        ;\n"),
    ("V2 vacuous", C, "terms.logicalAnd(terms.logicalAnd(both, equalOn(all, a, b)), apart)))", "terms.boolean(false)))"),
    ("V4 skipped", C, 'if (!require("V4", "the update reads the cell it writes", terms.logicalAnd(a.premise, differs)))\n      return false;', "(void)differs;"),
    ("read root ignored by the checker", T, "&&\n                  site.outputRead->root == site.output->root;", ";"),
    ("tainted coordinate adopted", C, "        if (!independent[id]) {", "        if (false) {"),
    ("shadow coordinate adopted", C, "      if (readsShadow(kept, seen)) {", "      if (false) {"),
    ("every run always claimed", C, "  bool everyRun = outcome.verdict.kind() == TerminationVerdict::ProvenTerminating;", "  bool everyRun = true;"),
    ("no run ever claimed", C, "  bool everyRun = outcome.verdict.kind() == TerminationVerdict::ProvenTerminating;", "  bool everyRun = false;"),
    ("V3 skipped", C, 'if (!require("V3", "range", terms.logicalAnd(a.premise,\n                                                 terms.logicalNot(inOrderSet(program, order, values)))))\n      return false;', "(void)values;"),
    ("unknown discharges", C, "  SmtAnswer answer = ask(id, where, formula);\n  if (answer == SmtAnswer::Unsat)\n    return true;", "  SmtAnswer answer = ask(id, where, formula);\n  if (answer != SmtAnswer::Sat)\n    return true;"),
    ("sat discharges", C, "  SmtAnswer answer = ask(id, where, formula);\n  if (answer == SmtAnswer::Unsat)\n    return true;", "  SmtAnswer answer = ask(id, where, formula);\n  if (answer != SmtAnswer::Unknown)\n    return true;"),
    ("no query limit", C, "  if (queries >= options.limits.maxQueries)\n    return SmtAnswer::Unknown;\n", ""),
    ("no pair limit", C, "  if (n * n > options.limits.maxPairs) {", "  if (false) {"),
    ("definitions unchecked", C, "        targets.erase(targets.begin() + n);\n        changed = true;\n", ""),
    ("certificate result ignored", C, "  if (outcome.verdict.kind() != TerminationVerdict::ProvenTerminating &&\n      outcome.verdict.kind() != TerminationVerdict::ProvenTerminatingIfDefined)\n    return CheckedFamily::unknown(\"the certificate", "  if (false)\n    return CheckedFamily::unknown(\"the certificate"),
    ("wider scope not checked", C, "  check.scope = options.scope;\n  check.limits = options.termination;", "  check.limits = options.termination;"),
    ("non-oblivious admitted", C, 'if (!program.controlOblivious || !program.addressOblivious)\n    return CheckedFamily::unknown("the trace depends on memory contents");\n', ""),
    ("unsupported group admitted", T, '  if (!group.unsupported.empty()) {\n    why = group.unsupported;\n    return false;\n  }\n', ""),
    ("partial group admitted", T, '    if (same != (listed.count(site.id) != 0)) {\n      why = "the group is not the sites of its roots";', '    if (false) {\n      why = "the group is not the sites of its roots";'),
    ("empty group admitted", T, '  if (group.sites.empty()) {\n    why = "the group has no sites";\n    return false;\n  }\n', ""),
    ("group gate skipped", C, '  if (!wholeGroup(program, group, gate))\n    return CheckedFamily::unknown(gate);\n', ""),
    ("definition proofs lost", C, "  if (id != \"prune\")\n    records.push_back(std::move(record));", "  if (id != \"prune\" && id != \"I\")\n    records.push_back(std::move(record));"),
    ("no order accepted", C, 'if (!order || order->levels.empty())\n    return CheckedFamily::unknown("no enumeration order to bound the coordinates");\n', ""),
    ("bound may read state", C, 'if (!stateSupport(copy.lower).empty() || !stateSupport(copy.upper).empty())\n      return CheckedFamily::unknown("a bound reads state");\n', ""),
    ("bound may read inner levels", C, 'if (copy.upperOuter >= (int)levels.levels.size())\n      return CheckedFamily::unknown("a bound reads a level that is not outer");\n', ""),
    ("foreign level accepted", C, '    if (found == wanted.end())\n      return CheckedFamily::unknown("a level of the order is no coordinate");\n    wanted.erase(found);\n', "    if (found != wanted.end())\n      wanted.erase(found);\n"),
    ("omitted coordinate accepted", C, '  if (!wanted.empty())\n    return CheckedFamily::unknown("the order omits a coordinate");\n', ""),
    ("exact without coverage", C, "    bool exact = symbolic.dischargeCoverage(levels, why);\n", "    bool exact = symbolic.dischargeCoverage(levels, why) || true;\n"),
    ("V2 and coverage both open", C, "    if (!exact && !symbolic.injective) {", "    if (false) {"),
    ("box with any bound", C, "        if (level.upperOuter >= 0 && !batch.count(levels.levels[level.upperOuter].value))\n          box = false;\n", ""),
    ("shared block admitted", C, '  if (distinct.size() != locations.size()) {\n    why = "two sites of the group share a block";\n    return false;\n  }\n', ""),
    ("sample is the scope", C, "  bool complete = enumerationRanItsScope(enumeration);\n", "  bool complete = true;\n"),
    ("stopped point ignored", C, "      if (!point.reason.empty())\n        complete = false;\n", ""),
    ("refuted candidate passes", C, "    if (checked.kind() == RecoveryStatus::CandidateRefuted) {\n      // C0 is the one counterexample no family can change.\n      bool c0 = llvm::any_of(checked.report().counterexamples, [](const Counterexample &c) {\n        return c.kind == CounterexampleKind::OutputReadDiffers;\n      });\n      return refuted(checked.reason(), c0 ? Refutes::EveryFamily : Refutes::ThisFamily);\n    }\n    // G3a's unknown is no refutation: there is no counterexample to show.\n    if (checked.kind() != RecoveryStatus::CandidateFound)\n      return CheckedFamily::unknown(\"the G3a checker gives no verdict at a point: \" + checked.reason());\n", ""),
    ("memory term adopted", C, "      Term kept = adoptProofTerm(program, term, why);\n      if (!kept)\n        return std::nullopt;\n      DenseSet<Term> seen;", "      Term kept = term;\n      DenseSet<Term> seen;"),
    ("closed gate checked", C, '  if (enumeration.gate != GateStatus::Open)\n    return CheckedFamily::unknown("the domain gate is closed: " + enumeration.reason);\n', ""),
    ("no point checked", C, '  if (facts.empty())\n    return CheckedFamily::unknown("no point has events");\n', ""),
    # Simulation.
    ("S1 first unchecked", V, 'return mustHold("S1", "the first event", terms.logicalAnd(premise, terms.logicalNot(good)));', "(void)good;\n    return true;"),
    ("S1 exit unchecked", V, 'return mustHold("S1", "the entry reaches the exit",\n                      terms.logicalAnd(premise, firstInside));', "return true;"),
    ("S2 unchecked", V, 'return mustHold("S2", "the next event", terms.logicalAnd(premise, terms.logicalNot(good)));', "(void)good;\n      return true;"),
    ("S3 unchecked", V, 'return mustHold("S3", "an element with a successor reaches the exit",\n                        terms.logicalAnd(premise, more));', "return true;"),
    ("next event may leave the set", V, "Term good = terms.logicalAnd(more, terms.logicalAnd(order.equal(values, next),\n                                                          order.member(values)));", "Term good = terms.logicalAnd(more, order.equal(values, next));"),
    ("last element may continue", V, "Term good = terms.logicalAnd(more, terms.logicalAnd(order.equal(values, next),\n                                                          order.member(values)));", "Term good = terms.logicalAnd(order.equal(values, next), order.member(values));"),
    ("first may lie outside", V, "Term good = terms.logicalAnd(firstInside, order.equal(values, first));", "Term good = order.equal(values, first);"),
    ("the walk stops early", V, "      } else if (isSite.count(next.end)) {", "      } else if (true) {"),
    ("depth limit passes", V, '        return failed("the walk to the next event is longer than the limit");', "        continue;"),
    ("undecided prune passes", V, '      if (feasible != SmtAnswer::Sat)\n        return failed("a path could not be decided");\n', ""),
    ("prune drops feasible", V, "      if (feasible == SmtAnswer::Unsat)\n        continue;", "      if (feasible != SmtAnswer::Sat)\n        continue;"),
    ("successor keeps inner", V, "        bumped.push_back(levels[r].lower);", "        bumped.push_back(values[r]);"),
    ("successor prefers slow levels", V, "    for (unsigned m = 0; m < levels.size(); ++m) {\n      std::vector<Term> bumped", "    for (unsigned m = levels.size(); m-- > 0;) {\n      std::vector<Term> bumped"),
    ("upper ignores the outer level", V, "    if (levels[m].upperOuter >= 0)\n      base = terms.binary(TermOp::Add, base, values[levels[m].upperOuter]);\n", ""),
    ("member ignores the lower bound", V, "      all = terms.logicalAnd(all, terms.icmp(Predicate::Ule, levels[m].lower, values[m]));\n", ""),
    ("member ignores the upper bound", V, "      all = terms.logicalAnd(all, terms.icmp(Predicate::Ult, values[m], upper(m, values)));\n", ""),
    ("site in the entry block", V, '  if (std::find(locations.begin(), locations.end(), program.entryLocation) != locations.end()) {\n    why = "a site in the entry block";\n    return false;\n  }\n', ""),
    # Search: a lost template must show.
    ("no seed", S, "  seed();\n  // Preference", "  // Preference"),
    ("no selectors decoded", S, 'if (found != response.model.end() && found->second == "1")', 'if (false)'),
    ("box clause spans batches", S, "  for (unsigned atom : selection[AxisP])\n    keptBatch.push_back({atom, AxisP, false});\n", ""),
    ("box clause without its separator", S, "        separated(pair.ex, pair.ey, d, {AxisP}, clause);\n", ""),
    ("box fallback without its separator", S, "  separated(ei, ej, d, {AxisP}, clause);\n", ""),
    ("box fallback without its second separator", S, "  separated(ei, ek, d, {AxisP}, clause);\n", ""),
    ("h clause ignores the batch", S, "d, {AxisP, AxisI, AxisJ}, clause);", "d, {AxisI, AxisJ}, clause);"),
    ("f clause ignores the batch", S, "d, {AxisP, AxisI, AxisK}, clause);", "d, {AxisI, AxisK}, clause);"),
    ("g clause ignores the batch", S, "d, {AxisP, AxisK, AxisJ}, clause);", "d, {AxisK, AxisJ}, clause);"),
    ("unit clause ignores the batch", S, "d, {AxisP, AxisI, AxisJ, AxisK}, clause);", "d, {AxisI, AxisJ, AxisK}, clause);"),
    ("no blocking", S, "            blocked.push_back(std::move(block));\n", ""),
    ("no box clauses", S, "    if (mode.box && report.cartesian != CartesianForm::Box) {", "    if (false) {"),
    ("duplicate structures kept", S, "  if (!kernels.insert(kernelKey(selection)).second)\n    return;\n", ""),
    ("order from the wrong side", S, "[&](unsigned a, unsigned b) { return changes[a] < changes[b]; });", "[&](unsigned a, unsigned b) { return changes[a] > changes[b]; });"),
    ("no triangular fit", S, "        if (shifts.size() == 1) {", "        if (false) {"),
    ("no parametric fit", S, "      for (const Parameter &p : program.parameters) {\n        if (found || p.sort.kind", "      for (const Parameter &p : ArrayRef<Parameter>()) {\n        if (found || p.sort.kind"),
    ("no division in the grammar", G, "        if (spec.division && value > 1 && !binary(TermOp::UDiv, a, c))\n          return;\n", ""),
    ("no remainder in the grammar", G, "        if (spec.remainder && value > 1 && !binary(TermOp::URem, a, c))\n          return;\n", ""),
    ("no variable division in the grammar", G, "        if (spec.division && !binary(TermOp::UDiv, a, b))\n          return;\n", ""),
    ("no variable remainder in the grammar", G, "        if (spec.remainder && !binary(TermOp::URem, a, b))\n          return;\n", ""),
    ("no addition in the grammar", G, "        if (spec.addition && !binary(TermOp::Add, a, b))\n          return;\n", ""),
    ("no access subterms", G, "          subterms(index);\n", ""),
    # Pipeline.
    ("C0 not seen", A, "          if (event.h != event.hRead) {", "          if (false) {"),
    ("unverified kept", A, "    if (candidate.finite.status() == FamilyStatus::Verified)\n      out.candidates.push_back(std::move(candidate));", "    out.candidates.push_back(std::move(candidate));"),
    ("unsupported group proceeds", A, "  if (!result.enumeration.groups[g].unsupported.empty()) {\n    out.status", "  if (false) {\n    out.status"),
    ("full demanded nowhere", A, "    if (llvm::any_of(pool, fullSomewhere))\n      break;\n", "    break;\n"),
    ("live state off", A, "  out.liveStatePoints = true;\n", ""),
    # The additive G3a parts.
    ("structurally full needs injectivity", K, "out.structurallyFull = out.decompositionValid && box && unit && out.strictNonDegenerate;", "out.structurallyFull = out.genuine;"),
    ("structurally full without a box", K, "out.structurallyFull = out.decompositionValid && box && unit && out.strictNonDegenerate;", "out.structurallyFull = out.decompositionValid && unit && out.strictNonDegenerate;"),
    ("structurally full without strictness", K, "out.structurallyFull = out.decompositionValid && box && unit && out.strictNonDegenerate;", "out.structurallyFull = out.decompositionValid && box && unit;"),
    ("memory state kept live", D, "      if (independent[s.id] && s.sort.kind == Sort::BV)", "      if (s.sort.kind == Sort::BV)"),
    ("read root ignored by the enumerator", D, "    if (site.outputRead->root != site.output->root) {", "    if (false) {"),
    ("sample outside the scope", D, "      if (point.size() != program.parameters.size() ||\n          !parametersInScope(options.scope, point))", "      if (point.size() != program.parameters.size())"),
    ("sample not marked", D, "  out.sampled = !options.onlyPoints.empty();\n", "  bool sampling = !options.onlyPoints.empty();\n"),
    # Statement-level index programs (docs/GENERALIZED_GEMM_STATEMENT_BV.md): the G.5
    # rules, the two templates, the obligations, the counters.
    ("unwritten element reads as zero", I, "      if (found == arrays[x.index].elements.end())\n        throw NoRuleApplies", "      if (found == arrays[x.index].elements.end())\n        return APInt::getZero(x.width);\n      if (false)\n        throw NoRuleApplies"),
    ("array bounds unchecked", I, "      if (value.getActiveBits() > 64 || value.getZExtValue() >= table.extents[d])", "      if (false)"),
    ("counter template body may read the bound", I, '  if (inputs.count(out.bound))\n    return fail("S reads the bound m");\n', ""),
    ("table template takes any extent", I, '    if (!same)\n      return fail("the extent of T is not m + d");\n', ""),
    ("table template reads past the history", I, "!p.exprs.at(ix.b).value.isZero() && p.exprs.at(ix.b).value.ule(d);", "!p.exprs.at(ix.b).value.isZero();"),
    ("init not read through the entry definitions", L, "      Term here = defined(path->state[v]);", "      Term here = path->state[v];"),
    ("no rule counted as a cut run", L, "      if (run.status == IndexRunStatus::OutOfFuel) {", "      if (run.status != IndexRunStatus::Returned) {"),
    ("table proof without the no-wrap premise", L, "          stepPremise = terms.logicalAnd(stepPremise, noWrap);\n", ""),
    ("unmatched template proved", L, "    if (!shapeOk) {", "    if (false) {"),
    ("dependence fixpoint skipped", L, "      for (unsigned v : c.dependsOn) {", "      for (unsigned v : std::vector<unsigned>()) {"),
    ("free-leaf model called a refutation", L, "        satHasFree = !freed.empty()", "        satHasFree = false && !freed.empty()"),
    ("W-term skipped", L, '          obligations.push_back({"W-term"', '          if (false) obligations.push_back({"W-term"'),
    ("TT-bounds skipped", L, '          obligations.push_back({"TT-bounds"', '          if (false) obligations.push_back({"TT-bounds"'),
    ("TT-shift skipped", L, '            obligations.push_back({"TT-shift"', '            if (false) obligations.push_back({"TT-shift"'),
    ("model entry taken for a known init", Z, "      if (at != spec.carried.end() && !spec.init[at - spec.carried.begin()])", "      if (at != spec.carried.end())"),
    ("counter never takes over a kernel", G, "isCounter(atom.term) && !isCounter(out.atoms[slot->second].term)", "false && isCounter(atom.term) && !isCounter(out.atoms[slot->second].term)"),
    ("counter bumped on every edge", N, "        t.assignments.push_back({var.id, inLoop[t.from] ? bumped : zero});", "        t.assignments.push_back({var.id, bumped});"),
    ("replay ignores the counter", N, "      if (!value || *value != APInt(width, expected[c.header]", "      if (false && (!value || *value != APInt(width, expected[c.header]"),
]

# Changes that no test can see, and why. Not run: a survivor here would mean nothing.
EQUIVALENT = [
    ("definitions from any term", "the filter `terms_.size() == 1` in addDefinitions only saves "
     "queries: every proposed fact is proved by O3 and O4 or dropped, so a state with two "
     "definitions adds a query and no trust."),
]


def contents(root, parts):
    found = {}
    for part in parts:
        for path in (root / part).rglob("*"):
            if path.is_file() and "__pycache__" not in path.parts:
                found[path.relative_to(root)] = hashlib.sha256(path.read_bytes()).hexdigest()
    return found


def run(command, cwd=None, timeout=900):
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
        return run(["ninja", "-C", self.build, "-j3", "generalized-gemm-synthesis-test",
                    "generalized-gemm-synthesis-report"])

    def commands(self):
        fuzz = self.root / "research" / "generalized-gemm" / "test" / "fuzz"
        return [
            ("unit", [self.build / "generalized-gemm-synthesis-test"]),
            ("programs", [sys.executable, fuzz / "synthesis_oracle.py", "--driver",
                          self.build / "generalized-gemm-synthesis-report",
                          "--programs", "6", "--seed", "41", "--jobs", "3"]),
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

    names = [n for n in args.only.split(",") if n]
    unknown = sorted(set(names) - {m[0] for m in MUTANTS})
    if unknown:
        print(f"no such mutants: {unknown}")
        return 2
    chosen = [m for m in MUTANTS if not names or m[0] in names]
    if len({m[0] for m in MUTANTS}) != len(MUTANTS):
        print("two mutants share a name")
        return 2

    scratch = Path(args.scratch).resolve()
    scratch.mkdir(exist_ok=True)
    snapshot = scratch / "tree"
    source = Path(args.source).resolve()
    parts = ["include", "lib", "research"] + [
        f"test/{p.name}" for p in (source / "test").glob("polynomial*")]
    if not snapshot.exists():
        snapshot.mkdir()
        for part in parts:
            (snapshot / part).parent.mkdir(parents=True, exist_ok=True)
            shutil.copytree(source / part, snapshot / part)
    # A reused snapshot of an older tree would test the older tests, silently.
    elif contents(source, parts) != contents(snapshot, parts):
        print(f"{snapshot} is not a copy of {source}: use a new --scratch")
        return 2
    workers = [Worker(i, scratch, snapshot, args) for i in range(args.workers)]

    # A kill only means something if the unmodified tree passes the same tests.
    first = workers[0]
    first.restore()
    built = first.compile()
    if built.returncode:
        print(f"BASELINE NO-BUILD\n{built.stdout[-1500:]}")
        return 2
    for label, command in first.commands():
        result = run(command)
        if result.returncode:
            print(f"BASELINE FAILED: {label}\n{result.stdout[-1500:]}\n{result.stderr[-1500:]}")
            return 2
    print("baseline passes", flush=True)

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
