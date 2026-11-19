//===- TerminationObligations.h - Formulas of the proof rule ----*- C++ -*-===//
//
// Each obligation is one BV1 term over the configuration at the start of a
// path. It is the NEGATION of what must hold, so `unsat` discharges it, and it
// is ordinary G1 term language: evaluateTerm can read it on a concrete
// configuration, which is how the tests check the formulas themselves.
//
// O1 domain nonempty (sat expected), O2 exactly one guard, O3 initiation,
// O4 consecution, O5 strict descent, O6 weak descent, O7 no stop.
//
//===----------------------------------------------------------------------===//
#ifndef GENERALIZEDGEMM_TERMINATIONOBLIGATIONS_H
#define GENERALIZEDGEMM_TERMINATIONOBLIGATIONS_H

#include "GeneralizedGemm/CutPaths.h"
#include "GeneralizedGemm/ProofStatus.h"

#include <string>
#include <utility>
#include <vector>

namespace mlir::generalized_gemm {

/// The parameter domain as a term. A malformed scope gives nullptr and `why`.
Term scopeFormula(const SemanticProgram &program, const ParameterScope &scope,
                  std::string &why);

/// Whether concrete parameters lie in a well-formed scope.
bool parametersInScope(const ParameterScope &scope,
                       const std::vector<llvm::APInt> &parameters);

/// Cont: some path of `candidates` leaving `location` is enabled.
Term continues(const SemanticProgram &program, const PathSet &set,
               llvm::ArrayRef<unsigned> candidates, unsigned location);

/// All negated: O2 at one location. Null when it has no transition.
Term guardViolation(const SemanticProgram &program, unsigned location);
/// O3 and O4: a path from a state satisfying `source` misses `target` at its
/// end. For a path from the entry, pass the term `true` as `source`.
Term invariantViolation(const SemanticProgram &program, Term scope,
                        const CutPath &path, Term source, Term target);
/// O5 (strict) and O6: a path of the current SCC that an infinite run could
/// take next fails to decrease. `candidates` are the paths of that SCC.
Term descentViolation(const SemanticProgram &program, const PathSet &set,
                      Term scope, const CutPath &path, Term invariant,
                      llvm::ArrayRef<unsigned> candidates, Term rankAtSource,
                      Term rankAtTarget, bool strict);
/// O7: a side condition can be violated where the walk reaches it.
Term stopViolation(const SemanticProgram &program, Term scope,
                   const StopSite &site, Term invariant);

/// A complete script: declarations, definitions, one assertion, check-sat.
/// `aux` adds named constants equal to terms, so a model can report them.
std::string obligationScript(
    const std::string &comment, Term formula,
    llvm::ArrayRef<std::pair<std::string, Term>> aux = {});

/// Every leaf is a parameter, a state or a shadow that exists in the program
/// with this sort, and the term holds no memory or float node. Rebuilds the
/// term inside the program's context; nullptr when refused.
Term adoptProofTerm(const SemanticProgram &program, Term term, std::string &why);

/// Whether a node below `term` reads memory.
bool readsMemory(Term term);

} // namespace mlir::generalized_gemm
#endif
