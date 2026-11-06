//===- DomainAnalysis.h - G5a entry point -----------------------*- C++ -*-===//
//
// Runs the G3b search for candidates, enumerates the scope with live state, proposes a
// guarded order for each candidate, and hands everything to DomainChecker. The search
// and the proposer only propose. Nothing here authorizes a transformation.
//
//===----------------------------------------------------------------------===//
#ifndef GENERALIZEDGEMM_DOMAINANALYSIS_H
#define GENERALIZEDGEMM_DOMAINANALYSIS_H

#include "GeneralizedGemm/DomainChecker.h"
#include "GeneralizedGemm/DomainProposer.h"
#include "GeneralizedGemm/SynthesisAnalysis.h"

#include <string>
#include <vector>

namespace mlir::generalized_gemm {

struct DomainAnalysisRequest {
  SynthesisOptions synthesis;
  DomainOptions domain;
  ProposalLimits proposal;
  unsigned maxCandidates = 2; // Per group, in the search's order.
  // A family given from outside, in place of the search: one candidate per group, with
  // no coordinate order. For tests and oracles that want a repeat loop outside the
  // family. A proposal like any other: the checkers judge it.
  std::optional<CoordinateFamily> family;
};

struct CandidateDomain {
  CoordinateFamily family;
  std::optional<EnumerationOrder> coordinateOrder;
  std::optional<DomainRequest> request;
  std::string proposalReason;
  ReductionVerdict verdict = ReductionVerdict::unknown("not checked");
};

struct GroupDomain {
  unsigned group = 0;
  RecoveryStatus search = RecoveryStatus::Unknown;
  std::string searchReason;
  std::vector<CandidateDomain> candidates;
};

struct DomainResult {
  DomainEnumeration enumeration;
  std::vector<GroupDomain> groups;
};

DomainResult analyzeDomains(const SemanticProgram &program, const TerminationCertificate *certificate,
                            const DomainAnalysisRequest &request, SmtBackend &backend);

std::string toJSON(const SemanticProgram &program, const DomainResult &result, bool events);

} // namespace mlir::generalized_gemm
#endif
