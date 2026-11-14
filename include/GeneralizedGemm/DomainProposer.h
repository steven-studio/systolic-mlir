//===- DomainProposer.h - Untrusted guarded orders --------------*- C++ -*-===//
//
// Guesses a DomainRequest from the events of sample points. Nothing here is evidence:
// a proposal goes to DomainChecker, which proves it or does not.
//
//   levels   the family's coordinates, slowest first, each in the direction its values
//            move; then live state that tells repeated events apart, if any is needed.
//   bounds   a box around the samples: constants, or a parameter plus a constant.
//   guard    the program's own branch conditions over the levels, kept where every
//            sample event satisfies them; if their conjunction is not exact at the
//            samples, the disjunction of the patterns the events show.
//   key      the levels that change within a cell, if that orders each cell.
//   access   the index terms themselves, when they read only levels.
//
//===----------------------------------------------------------------------===//
#ifndef GENERALIZEDGEMM_DOMAINPROPOSER_H
#define GENERALIZEDGEMM_DOMAINPROPOSER_H

#include "GeneralizedGemm/CoordinateFamily.h"
#include "GeneralizedGemm/DomainModel.h"
#include "GeneralizedGemm/FiniteDomain.h"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace mlir::generalized_gemm {

/// Phase 4 (I2, I3). A proposed identity v = term at every event, with `term` over
/// state and parameters: a proved closed form of a loop variable with the iteration
/// count read as the loop's counter (LoopCounters.h), or a variable every path from
/// the entry defines by one term over parameters. The proposer substitutes it for v
/// in guards, bounds and views so that they read levels alone. A proposal, never
/// evidence: the checker proves or refutes whatever came of it.
struct StateIdentity {
  unsigned state = 0;
  Term term = nullptr;
  std::string origin;
  int cut = -1; // The cut (loop header) at which it holds, or -1 for every cut.
};

struct ProposalLimits {
  unsigned maxAtoms = 64, maxDisjuncts = 32, maxRepeatLevels = 2;
  uint64_t maxBox = 1u << 20;
  EnumerationLimits events; // The run that reads the state at each event, for segments.
  std::vector<StateIdentity> identities;
};

/// Untrusted, for tests of the proposer: the atoms a proposal's guard may use for `order`,
/// the program's conditions over the order's levels and the congruences of each level by
/// 2, 3 and 4 where the level's width holds the modulus. Every constant in an atom fits
/// its width. Nothing trusts an atom.
std::vector<Term> proposalConditionAtoms(const SemanticProgram &program, const GuardedOrder &order,
                                         const std::map<unsigned, llvm::APInt> &constants,
                                         unsigned limit);

/// A proposal, or nullopt with `why`. `coordinateOrder`, when given, orders the levels.
std::optional<DomainRequest> proposeDomain(const SemanticProgram &program,
                                           const DomainEnumeration &samples, unsigned group,
                                           const CoordinateFamily &family,
                                           const EnumerationOrder *coordinateOrder,
                                           const ProposalLimits &limits, std::string &why);

/// G5b: a segmented proposal. Sites whose events never interleave at the sample points
/// form segments, in program order; each gets the loop variables defined at all of its
/// events as coordinates, on axis I, and an order fitted to its events alone. With
/// `logical` given, only the loop variables it names are coordinates; the others are
/// repeat candidates, as in a flat proposal with a family.
std::optional<DomainRequest> proposeSegmentedDomain(const SemanticProgram &program,
                                                    const DomainEnumeration &samples, unsigned group,
                                                    const CoordinateFamily *logical,
                                                    const ProposalLimits &limits, std::string &why);

} // namespace mlir::generalized_gemm
#endif
