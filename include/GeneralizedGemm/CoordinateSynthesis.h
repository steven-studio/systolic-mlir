//===- CoordinateSynthesis.h - Selector CEGIS -------------------*- C++ -*-===//
//
// Assigns atoms to the axes P, I, J, K with one selector per pair, so an axis is a
// tuple of any length. The solver proposes an assignment, the G3a facts judge it
// on the sampled domains, and each failure becomes a clause that every valid
// assignment satisfies (Theorem 6). So exhausting the clauses means only that no
// assignment of these atoms works: it says nothing about other coordinates.
//
// Preference orders the search. It never changes what a candidate is worth, and
// nothing here issues a status.
//
//===----------------------------------------------------------------------===//
#ifndef GENERALIZEDGEMM_COORDINATESYNTHESIS_H
#define GENERALIZEDGEMM_COORDINATESYNTHESIS_H

#include "GeneralizedGemm/CanonicalRecovery.h"
#include "GeneralizedGemm/CoordinateGrammar.h"
#include "GeneralizedGemm/SmtBackend.h"

#include <string>
#include <vector>

namespace mlir::generalized_gemm {

struct SynthesisLimits {
  unsigned maxRounds = 300, maxClauses = 4000, maxCandidates = 5;
  unsigned maxBatchAtoms = 3;
  SmtLimits solver;
};

/// A selector of `atom` on `axis`, or its negation.
struct SelectorLiteral {
  unsigned atom, axis;
  bool positive;
};
using SelectorClause = std::vector<SelectorLiteral>;

struct SynthesisOutcome {
  std::vector<CoordinateFamily> candidates;
  unsigned rounds = 0, clauses = 0;
  // Theorem 6, as learned: every assignment valid on the sampled domains satisfies
  // `always`, a unit one also `whenUnit`, and a box one also `whenBox`.
  std::vector<SelectorClause> always, whenUnit, whenBox;
  bool exhausted = false; // Every layer was searched to the end.
  std::string stop;       // Why the search ended early.
};

SynthesisOutcome synthesizeCoordinates(const SemanticProgram &program,
                                       const AtomBank &bank,
                                       llvm::ArrayRef<const AccessDomain *> domains,
                                       const SynthesisLimits &limits, SmtBackend &backend);

/// A lexicographic order that fits the samples, or nullopt. Only a guess: the
/// bounds may fail at other points, and then the checker says so.
std::optional<EnumerationOrder> proposeOrder(const SemanticProgram &program,
                                             const CoordinateFamily &family,
                                             llvm::ArrayRef<const AccessDomain *> domains);

} // namespace mlir::generalized_gemm
#endif
