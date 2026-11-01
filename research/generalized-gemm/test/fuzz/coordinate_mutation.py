#!/usr/bin/env python3
"""Mutation test of G3a: each mutant must be caught.

Works on a private snapshot, and only after the unmodified snapshot passes the
same tests. A mutant that passes everything, an edit that no longer applies and
an empty selection all fail this script.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import shutil
import subprocess
import sys

M = "CoordinateModel.cpp"
R = "CanonicalRecovery.cpp"
C = "CandidateChecker.cpp"
D = "FiniteDomain.cpp"
A = "CoordinateAnalysis.cpp"
MUTANTS = [
    # Tables.
    ("width not identity", M, "    key.push_back(value.getBitWidth());\n", ""),
    ("triples merged by h", M, "tripleIds.try_emplace({event.h, event.f, event.g},", "tripleIds.try_emplace({event.h, event.h, event.h},"),
    ("multiplicity not counted", M, "  ++counts[event.triple];\n", "  counts[event.triple] = 1;\n"),
    ("point ignores the site", M, "IndexTuple key{APInt(32, site)};", "IndexTuple key{APInt(32, 0)};"),
    # Joins.
    ("I ignores f", R, "out.rhoI = join({&out.kerH, &out.kerF});", "out.rhoI = join({&out.kerH});"),
    ("I ignores h", R, "out.rhoI = join({&out.kerH, &out.kerF});", "out.rhoI = join({&out.kerF});"),
    ("J ignores g", R, "out.rhoJ = join({&out.kerH, &out.kerG});", "out.rhoJ = join({&out.kerH});"),
    ("K ignores f", R, "out.rhoK = join({&out.kerF, &out.kerG});", "out.rhoK = join({&out.kerG});"),
    ("P ignores g", R, "out.rhoP = join({&out.kerH, &out.kerF, &out.kerG});", "out.rhoP = join({&out.kerH, &out.kerF});"),
    ("no union", R, "      sets.unite(t, slot->second);\n", "      (void)slot;\n"),
    ("hint always holds", R, "out.meetH = meet(out.rhoI, out.rhoJ) == out.kerH;", "out.meetH = true;"),
    ("candidate swaps I and J", R, "candidate.i.push_back(partitions.rhoI[event.triple]);", "candidate.i.push_back(partitions.rhoJ[event.triple]);"),
    ("candidate labels by event", R, "candidate.k.push_back(partitions.rhoK[event.triple]);", "candidate.k.push_back(partitions.rhoK[event.point % partitions.rhoK.size()]);"),
    # Validity.
    ("C0 unchecked", C, "if (events[e].h != events[e].hRead) {", "if (false) {"),
    ("C0 not in validity", C, "out.decompositionValid = out.outputReadMatches && out.functionOfPoint &&", "out.decompositionValid = out.functionOfPoint &&"),
    ("point function not in validity", C, "out.decompositionValid = out.outputReadMatches && out.functionOfPoint &&", "out.decompositionValid = out.outputReadMatches &&"),
    ("h dependence not in validity", C, "out.dependenceH && out.dependenceF && out.dependenceG;", "out.dependenceF && out.dependenceG;"),
    ("f dependence not in validity", C, "out.dependenceH && out.dependenceF && out.dependenceG;", "out.dependenceH && out.dependenceG;"),
    ("g dependence not in validity", C, "out.dependenceH && out.dependenceF && out.dependenceG;", "out.dependenceH && out.dependenceF;"),
    ("h key ignores J", C, "out.dependenceH = functional([&](uint64_t e) { return Key{c.p[e], c.i[e], c.j[e]}; },", "out.dependenceH = functional([&](uint64_t e) { return Key{c.p[e], c.i[e], 0}; },"),
    ("h key ignores P", C, "out.dependenceH = functional([&](uint64_t e) { return Key{c.p[e], c.i[e], c.j[e]}; },", "out.dependenceH = functional([&](uint64_t e) { return Key{0, c.i[e], c.j[e]}; },"),
    ("f key ignores K", C, "out.dependenceF = functional([&](uint64_t e) { return Key{c.p[e], c.i[e], c.k[e]}; },", "out.dependenceF = functional([&](uint64_t e) { return Key{c.p[e], c.i[e], 0}; },"),
    ("g key ignores K", C, "out.dependenceG = functional([&](uint64_t e) { return Key{c.p[e], c.k[e], c.j[e]}; },", "out.dependenceG = functional([&](uint64_t e) { return Key{c.p[e], 0, c.j[e]}; },"),
    ("functional always true", C, "    if (!added && value(slot->second) != value(e)) {", "    if (false) {"),
    ("counterexample misnames an event", C, "      add(kind, slot->second, e);\n", "      add(kind, e, e);\n"),
    # Injectivity.
    ("H injectivity ignores J", C, "out.injectiveH = functional([&](uint64_t e) { return events[e].h; },\n                              [&](uint64_t e) { return Value{c.p[e], c.i[e], c.j[e]}; },",
     "out.injectiveH = functional([&](uint64_t e) { return events[e].h; },\n                              [&](uint64_t e) { return Value{c.p[e], c.i[e], 0}; },"),
    ("F injectivity ignores K", C, "[&](uint64_t e) { return Value{c.p[e], c.i[e], c.k[e]}; },", "[&](uint64_t e) { return Value{c.p[e], c.i[e], 0}; },"),
    ("G injectivity ignores J", C, "[&](uint64_t e) { return Value{c.p[e], c.k[e], c.j[e]}; },", "[&](uint64_t e) { return Value{c.p[e], c.k[e], 0}; },"),
    ("shared views unseen", C, "        out.batchesShareViews = true;\n", "        ;\n"),
    # Domain structure.
    ("always a box", C, "out.cartesian = totalMissing.isZero() ? CartesianForm::Box : CartesianForm::Partial;", "out.cartesian = CartesianForm::Box;"),
    ("box from one axis", C, "APInt product = wide(batch.i.size()) * wide(batch.j.size()) * wide(batch.k.size());", "APInt product = wide(batch.i.size()) * wide(batch.j.size());"),
    ("missing not accumulated", C, "    totalMissing += missing;\n", "    totalMissing = missing;\n"),
    ("a batch's axes shared", C, "    Batch &batch = perBatch[c.p[e]];\n", "    Batch &batch = perBatch[0];\n"),
    ("missing point misreported", C, "          if (!image.count({label, i, j, k})) {", "          if (image.count({label, i, j, k})) {"),
    ("multiplicity by min", C, "out.multiplicity = high == 1      ? MultiplicityForm::Unit", "out.multiplicity = low == 1       ? MultiplicityForm::Unit"),
    ("nonuniform is uniform", C, "                     : low == high ? MultiplicityForm::Uniform", "                     : true ? MultiplicityForm::Uniform"),
    ("multiplicity witness dropped", C, "        add(CounterexampleKind::MultiplicityDiffers, 0, e);\n", ""),
    ("duplicates uncounted", C, "out.duplicateIterationPoints = events.size() - table.pointCount();", "out.duplicateIterationPoints = 0;"),
    # Classification.
    ("strict ignores I", C, "strict = strict && batch.i >= 2 && batch.j >= 2 && batch.k >= 2;", "strict = strict && batch.j >= 2 && batch.k >= 2;"),
    ("strict ignores J", C, "strict = strict && batch.i >= 2 && batch.j >= 2 && batch.k >= 2;", "strict = strict && batch.i >= 2 && batch.k >= 2;"),
    ("strict ignores K", C, "strict = strict && batch.i >= 2 && batch.j >= 2 && batch.k >= 2;", "strict = strict && batch.i >= 2 && batch.j >= 2;"),
    ("one good tile suffices", C, "  bool strict = true, same = true;", "  bool strict = false, same = true;"),
    ("genuine without validity", C, "out.genuine = out.decompositionValid && box && unit && injective &&", "out.genuine = box && unit && injective &&"),
    ("genuine without a box", C, "out.genuine = out.decompositionValid && box && unit && injective &&", "out.genuine = out.decompositionValid && unit && injective &&"),
    ("genuine without unit", C, "out.genuine = out.decompositionValid && box && unit && injective &&", "out.genuine = out.decompositionValid && box && injective &&"),
    ("genuine without injectivity", C, "out.genuine = out.decompositionValid && box && unit && injective &&", "out.genuine = out.decompositionValid && box && unit &&"),
    ("genuine without strictness", C, "unit && injective &&\n                out.strictNonDegenerate;", "unit && injective;"),
    ("unique without injectivity", C, "out.uniqueUpToRelabelling = out.decompositionValid && box && injective;", "out.uniqueUpToRelabelling = out.decompositionValid && box;"),
    ("degenerate shapes dropped", C, "  if (!out.decompositionValid)\n    return;\n  // A degenerate shape", "  if (!out.genuine)\n    return;\n  // A degenerate shape"),
    ("invalid stays eligible", C, "  if (!out.decompositionValid)\n    return;\n  // A degenerate shape", "  // A degenerate shape"),
    ("element-wise is scalar", C, "else if (*common == ShapeClass::Scalar && out.batches.size() > 1)", "else if (false)"),
    ("batches mixed silently", C, "  if (!same)\n    out.shape = ShapeClass::Mixed;\n  else if", "  if"),
    ("outer is scaled", C, "return i >= 2 && j >= 2 ? ShapeClass::Outer : ShapeClass::ScaledVector;", "return ShapeClass::ScaledVector;"),
    ("gemv is dot", C, "return i == 1 && j == 1 ? ShapeClass::Dot : ShapeClass::GEMV;", "return ShapeClass::Dot;"),
    ("short labels accepted", C, "  if (c.p.size() != size || c.i.size() != size || c.j.size() != size ||\n      c.k.size() != size) {", "  if (c.p.size() != size) {"),
    ("no events accepted", C, '  if (!size) {\n    out.malformed = "no events";\n    return out;\n  }\n', ""),
    ("invalid found", C, "  if (report.decompositionValid)\n    return CandidateAuthority::make(RecoveryStatus::CandidateFound,", "  if (true)\n    return CandidateAuthority::make(RecoveryStatus::CandidateFound,"),
    ("support empty when undetermined", C, "  if (p && i && j && k)\n    out.support = std::array<std::vector<unsigned>, 4>{*p, *i, *j, *k};", "  out.support = std::array<std::vector<unsigned>, 4>{p.value_or(std::vector<unsigned>{}), i.value_or(std::vector<unsigned>{}), j.value_or(std::vector<unsigned>{}), k.value_or(std::vector<unsigned>{})};"),
    ("support not conditioned on P", C, "auto p = smallest(c.p, nullptr), i = smallest(c.i, &c.p)", "auto p = smallest(c.p, nullptr), i = smallest(c.i, nullptr)"),
    ("support not smallest", C, "return __builtin_popcount(a) < __builtin_popcount(b);", "return __builtin_popcount(a) > __builtin_popcount(b);"),
    # Gates.
    ("unsupported program enumerated", D, 'return close(GateStatus::Unsupported,\n                 "the program is unsupported: " + program.unsupported->detail);', 'return close(GateStatus::Unknown, "");'),
    ("data-dependent traces admitted", D, "if (!program.controlOblivious || !program.addressOblivious)", "if (false)"),
    ("control dependence admitted", D, "if (!program.controlOblivious || !program.addressOblivious)", "if (!program.addressOblivious)"),
    ("address dependence admitted", D, "if (!program.controlOblivious || !program.addressOblivious)", "if (!program.controlOblivious)"),
    ("scalar gate open", D, "  if (!anySupported)\n    return close(GateStatus::Unsupported, out.groups[0].unsupported);\n", ""),
    ("no point limit", D, "if (ranges.points.ugt(options.limits.maxPoints))", "if (false)"),
    ("point limit off by one", D, "if (ranges.points.ugt(options.limits.maxPoints))", "if (ranges.points.uge(options.limits.maxPoints))"),
    ("certificate result ignored", D, "if (outcome.verdict.kind() != TerminationVerdict::ProvenTerminating &&\n      outcome.verdict.kind() != TerminationVerdict::ProvenTerminatingIfDefined)", "if (false)"),
    ("if-defined refused", D, "if (outcome.verdict.kind() != TerminationVerdict::ProvenTerminating &&\n      outcome.verdict.kind() != TerminationVerdict::ProvenTerminatingIfDefined)", "if (outcome.verdict.kind() != TerminationVerdict::ProvenTerminating)"),
    ("certificate checked over all parameters", D, "  check.scope = options.scope;\n", ""),
    ("assumptions not passed to the check", D, "  check.authorized = options.authorized;\n", ""),
    ("stopped run admitted", D, "if (!admission.admitted) {", "if (false) {"),
    ("no event limit", D, "} else if (run.events.size() > options.limits.maxEvents) {", "} else if (false) {"),
    ("event limit off by one", D, "} else if (run.events.size() > options.limits.maxEvents) {", "} else if (run.events.size() >= options.limits.maxEvents) {"),
    ("no cell limit", D, "      if (extent && cells > maxCells / extent)\n        return std::nullopt;\n", ""),
    ("negative extents run", D, "if (hasNegativeExtent(program, point)) {", "if (false) {"),
    ("low bound ignored", D, "low = std::max(low, range.lower.getSExtValue());", ";"),
    ("high bound ignored", D, "high = std::min(high, range.upper.getSExtValue());", ";"),
    ("groups ignore the output", D, "auto key = std::make_tuple(site.output->root, site.a.root, site.b.root);", "auto key = std::make_tuple(0u, site.a.root, site.b.root);"),
    ("groups ignore the first operand", D, "auto key = std::make_tuple(site.output->root, site.a.root, site.b.root);", "auto key = std::make_tuple(site.output->root, 0u, site.b.root);"),
    ("groups ignore the second operand", D, "auto key = std::make_tuple(site.output->root, site.a.root, site.b.root);", "auto key = std::make_tuple(site.output->root, site.a.root, 0u);"),
    ("site not recorded", D, "tables[g].add(event.site, event.step,", "tables[g].add(0, event.step,"),
    ("unsupported group gets a table", D, "if (out.groups[g].unsupported.empty())\n      tables[g].add", "tables[g].add"),
    # Analysis.
    ("canonical failure refutes", A, "if (checked.kind() == RecoveryStatus::CandidateFound || formRefuted) {", "if (true) {"),
    ("C0 only unknown", A, "if (checked.kind() == RecoveryStatus::CandidateFound || formRefuted) {", "if (checked.kind() == RecoveryStatus::CandidateFound) {"),
    ("hints dropped", A, "  out.hints = std::move(partitions);\n", ""),
    ("join memo keyed on h", A, "std::array<Partition, 3> key{kerH, kerF, kerG};", "std::array<Partition, 3> key{kerH, kerH, kerH};"),
    ("unsupported group proceeds", A, "if (!group.unsupported.empty()) {", "if (false) {"),
]


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
        return run(["ninja", "-C", self.build, "-j3", "generalized-gemm-coordinate-test",
                    "generalized-gemm-coordinate-report"])

    def commands(self):
        fuzz = self.root / "research" / "generalized-gemm" / "test" / "fuzz"
        driver = self.build / "generalized-gemm-coordinate-report"
        return [
            ("tables", [sys.executable, fuzz / "coordinate_bruteforce.py", "--driver", driver,
                        "--random", "4000", "--hidden", "300", "--subsets", "800", "--seed", "31"]),
            ("unit", [self.build / "generalized-gemm-coordinate-test"]),
            ("programs", [sys.executable, fuzz / "coordinate_oracle.py", "--driver", driver,
                          "--programs", "8", "--seed", "41", "--jobs", "2"]),
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
    if not snapshot.exists():
        source = Path(args.source).resolve()
        snapshot.mkdir()
        for part in ["include", "lib", "research"] + [
                f"test/{p.name}" for p in (source / "test").glob("polynomial*")]:
            (snapshot / part).parent.mkdir(parents=True, exist_ok=True)
            shutil.copytree(source / part, snapshot / part)
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
