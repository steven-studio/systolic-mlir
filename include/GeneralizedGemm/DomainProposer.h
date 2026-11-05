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

#include <optional>
#include <string>

namespace mlir::generalized_gemm {

struct ProposalLimits {
  unsigned maxAtoms = 64, maxDisjuncts = 32, maxRepeatLevels = 2;
  uint64_t maxBox = 1u << 20;
};

/// A proposal, or nullopt with `why`. `coordinateOrder`, when given, orders the levels.
std::optional<DomainRequest> proposeDomain(const SemanticProgram &program,
                                           const DomainEnumeration &samples, unsigned group,
                                           const CoordinateFamily &family,
                                           const EnumerationOrder *coordinateOrder,
                                           const ProposalLimits &limits, std::string &why);

} // namespace mlir::generalized_gemm
#endif
