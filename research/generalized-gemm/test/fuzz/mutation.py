#!/usr/bin/env python3
"""Mutation test of the G1 suite: each mutant must be caught.

Copies lib/GeneralizedGemm, applies one textual change, rebuilds the tests
against the copy (GG_LIB_DIR) and runs them. A mutant that passes everything
means the tests do not enforce that behaviour, and this script fails.
"""
import argparse
from pathlib import Path
import shutil
import subprocess
import sys

MUTANTS = [
    ("nsw checks unsigned", "SemanticImport.cpp", "value, /*isSigned=*/true));", "value, /*isSigned=*/false));"),
    ("nuw checks signed", "SemanticImport.cpp", "value, /*isSigned=*/false));", "value, /*isSigned=*/true));"),
    ("division by zero not undefined", "SemanticImport.cpp", 'side(SideKind::UndefinedBehavior, "division by zero", byZero);', ""),
    ("signed overflow not undefined", "SemanticImport.cpp", 'side(SideKind::UndefinedBehavior, "signed division overflow", overflow);', ""),
    ("bounds compare signed", "SemanticImport.cpp", "outside, terms.icmp(Predicate::Uge, i.value, extent(root, d)));", "outside, terms.icmp(Predicate::Sge, i.value, extent(root, d)));"),
    ("poison index allowed", "SemanticImport.cpp", 'side(SideKind::UndefinedBehavior, "poison memory index", poison);', ""),
    ("select takes both arms", "SemanticImport.cpp", "terms.logicalOr(c.poison, terms.ite(c.value, a.poison, b.poison))", "terms.logicalOr(c.poison, terms.logicalOr(a.poison, b.poison))"),
    ("branch on poison allowed", "SemanticImport.cpp", 'side(SideKind::UndefinedBehavior, "branch on poison", c.poison);', ""),
    ("shadow dropped on edges", "SemanticImport.cpp", "t.shadowAssignments.push_back({id, value.poison});", "t.shadowAssignments.push_back({id, terms.boolean(false)});"),
    ("poison store allowed", "SemanticImport.cpp", 'side(SideKind::ModelLimit, "poison reaches memory", stored.poison);', ""),
    ("NaN treated as determined", "SemanticImport.cpp", 'side(SideKind::Underspecified, "NaN result: payload is not determined",\n       terms.logicalAnd(live, terms.fclass(TermOp::FIsNaN, value)));', ""),
    ("exact division not poison", "SemanticImport.cpp", "  if (exact) {\n    TermOp remainder", "  if (false) {\n    TermOp remainder"),
    ("oversized shift not poison", "SemanticImport.cpp", "terms.logicalOr(terms.logicalOr(a.poison, b.poison), tooFar);", "terms.logicalOr(a.poison, b.poison);"),
    ("taint stops at state", "SemanticImport.cpp", "if (!tainted[id] && depends(term)) {", "if (false && depends(term)) {"),
    ("repeated switch case accepted", "SemanticImport.cpp", "if (!distinct.insert(value.getZExtValue()).second)", "if (false)"),
    ("alias pair called proven", "SemanticImport.cpp", "ProofStatus status = ProofStatus::assumed(name);", "ProofStatus status = ProofStatus::proven(name);"),
    ("index_cast zero extends", "SemanticImport.cpp", "  } else if (isa<arith::IndexCastOp>(op)) {\n    checkAttributes(&op, {});\n    widen(true);", "  } else if (isa<arith::IndexCastOp>(op)) {\n    checkAttributes(&op, {});\n    widen(false);"),
    ("unknown attributes ignored", "SemanticImport.cpp", "    if (std::find(allowed.begin(), allowed.end(), name) == allowed.end())\n      refuse(UnsupportedKind::UnsupportedAttribute,\n             op->getName().getStringRef().str() + \": \" + name.str());", ""),
    ("srem unsigned", "SemanticTerm.cpp", "return a.srem(b);", "return a.urem(b);"),
    ("rounding toward zero", "SemanticTerm.cpp", "auto rounding = llvm::RoundingMode::NearestTiesToEven;", "auto rounding = llvm::RoundingMode::TowardZero;"),
    ("ashr fills zero", "SemanticTerm.cpp", "return b.uge(w) ? (a.isNegative() ? APInt::getAllOnes(w) : APInt::getZero(w))\n                    : a.ashr(b);", "return b.uge(w) ? APInt::getZero(w) : a.ashr(b);"),
    ("store chain ignored", "ConcreteEvaluator.cpp", "    if (same)\n      return value(memory->args[1]);\n", ""),
    ("guards not partitioned", "ConcreteEvaluator.cpp", "      if (taken)\n        return stop(RunStatus::ModelLimit, \"guards overlap\");\n", "      if (taken)\n        continue;\n"),
    ("cycle detection on location only", "ConcreteEvaluator.cpp", "!seen.insert(configurationKey(*config)).second", "!seen.insert(std::to_string(config->location)).second"),
    ("evidence ignores memory dependence", "ConcreteEvaluator.cpp", "if (!program.controlOblivious || !program.addressOblivious) {", "if (false) {"),
    ("evidence from any ending", "ConcreteEvaluator.cpp", "if (run.status != RunStatus::Returned) {", "if (false) {"),
    ("coverage admits over-approximation", "ProofStatus.cpp", "    if (exact)\n      return {true, \"exact D supports coverage\"};", "    return {true, \"exact D supports coverage\"};"),
    ("assumption discharged freely", "ProofStatus.cpp", "case ProofStatus::Kind::Assumed: return authorized.count(status.detail()) != 0;", "case ProofStatus::Kind::Assumed: return true;"),
    ("assumption ranks as proof", "ProofStatus.cpp", "case ProofStatus::Kind::Assumed: return 1;", "case ProofStatus::Kind::Assumed: return 0;"),
    ("smt srem is smod", "RelationalEncoding.cpp", 'return binary("bvsrem");', 'return binary("bvsmod");'),
    ("smt rounding", "RelationalEncoding.cpp", '"(fp.mul RNE "', '"(fp.mul RTZ "'),
    ("smt frame loses memory", "RelationalEncoding.cpp", '(m == written.end() ? "m" + id : name(m->second))', '("m" + id)'),
    ("smt returns dropped", "RelationalEncoding.cpp", 'parts.push_back("(= r" + std::to_string(i) + " " + name(t.returns[i]) + ")");', ""),
    ("smt not stuck dropped", "RelationalEncoding.cpp", "isTrue(t.guard), notStuck[t.from],", "isTrue(t.guard),"),
    ("symbolic drops guard", "SymbolicEvaluator.cpp", "next.condition = terms.logicalAnd(path.condition, apply(t.guard));", "next.condition = path.condition;"),
    ("symbolic ignores memory update", "SymbolicEvaluator.cpp", "next.memory[root] = apply(term);", "(void)root;"),
]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", required=True, help="research/generalized-gemm")
    parser.add_argument("--scratch", required=True)
    parser.add_argument("--llvm-dir", required=True)
    parser.add_argument("--mlir-dir", required=True)
    parser.add_argument("--only", default="")
    args = parser.parse_args()

    source = Path(args.source).resolve()
    original = source.parent.parent / "lib" / "GeneralizedGemm"
    scratch = Path(args.scratch).resolve()
    copy = scratch / "lib"
    build = scratch / "build"
    if not copy.exists():
        shutil.copytree(original, copy)
    subprocess.run(["cmake", "-S", source, "-B", build, "-G", "Ninja", "-DCMAKE_BUILD_TYPE=Debug",
                    f"-DGG_LIB_DIR={copy}", f"-DLLVM_DIR={args.llvm_dir}", f"-DMLIR_DIR={args.mlir_dir}"],
                   check=True, capture_output=True)
    fuzz = source / "test" / "fuzz" / "differential.py"
    commands = [
        [build / "generalized-gemm-unit"],
        [sys.executable, fuzz, "--driver", build / "generalized-gemm-report", "--programs", "250", "--seed", "11"],
        [build / "generalized-gemm-relational"],
    ]
    # A kill only means something if the unmodified tree passes the same tests.
    for f in original.iterdir():
        shutil.copy(f, copy / f.name)
    subprocess.run(["ninja", "-C", build, "-j6", "generalized-gemm-unit", "generalized-gemm-report",
                    "generalized-gemm-relational"], check=True, capture_output=True)
    for command in commands:
        result = subprocess.run([str(c) for c in command], capture_output=True, text=True, timeout=300)
        if result.returncode:
            print(f"BASELINE FAILED: {command}\n{result.stdout[-2000:]}\n{result.stderr[-2000:]}")
            return 2
    print("baseline passes", flush=True)
    survivors = []
    for name, filename, old, new in MUTANTS:
        if args.only and args.only not in name:
            continue
        text = (original / filename).read_text()
        if text.count(old) < 1:
            print(f"STALE {name}: pattern not found")
            survivors.append(name)
            continue
        for f in original.iterdir():
            if (copy / f.name).read_text() != f.read_text():
                shutil.copy(f, copy / f.name)
        (copy / filename).write_text(text.replace(old, new, 1))
        built = subprocess.run(["ninja", "-C", build, "-j6", "generalized-gemm-unit", "generalized-gemm-report",
                                "generalized-gemm-relational"], capture_output=True, text=True)
        if built.returncode:
            print(f"NO-BUILD {name}\n{built.stdout[-800:]}")
            survivors.append(name)
            continue
        killer = None
        for command in commands:
            try:
                result = subprocess.run([str(c) for c in command], capture_output=True, text=True, timeout=300)
                if result.returncode:
                    killer = Path(str(command[0])).name if command[0] != sys.executable else "oracle fuzz"
                    break
            except subprocess.TimeoutExpired:
                killer = "timeout"
                break
        print(f"{'KILLED by ' + killer if killer else 'SURVIVED'}: {name}", flush=True)
        if not killer:
            survivors.append(name)
    print(f"{len(MUTANTS) - len(survivors)} of {len(MUTANTS)} killed; survivors: {survivors}")
    return 1 if survivors else 0


if __name__ == "__main__":
    sys.exit(main())
