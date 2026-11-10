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

/// G5b-M4a: one set of groups to claim over: by index, or every supported group, in
/// increasing or decreasing order.
struct DomainSetRequest {
  std::vector<unsigned> groups;
  bool all = false, reversed = false;
};

struct DomainAnalysisRequest {
  SynthesisOptions synthesis;
  DomainOptions domain;
  ProposalLimits proposal;
  unsigned maxCandidates = 2; // Per group, in the search's order.
  // A family given from outside, in place of the search: one candidate per group, with
  // no coordinate order. For tests and oracles that want a repeat loop outside the
  // family. A proposal like any other: the checkers judge it.
  std::optional<CoordinateFamily> family;
  // G5b: propose segments by site instead of one order over the search's family. One
  // candidate per group, with an empty family; the segments carry theirs.
  bool segmented = false;
  // G5b-M4a: one claim over each set of groups, by their indices. The request over a set is
  // each listed group's own segmented proposal, the segments then in program order by their
  // first site: untrusted, for the checker to judge. A proposer over unions is not here.
  std::vector<DomainSetRequest> groupSets;
};

/// G5b-M4a: one set of groups, its request, and both routes' claims.
struct SetDomain {
  std::vector<unsigned> groups; // As listed.
  std::optional<DomainRequest> request;
  std::string proposalReason;
  CheckedSetDomain finite = CheckedSetDomain::unknown("not checked");
  CheckedSetDomain symbolic = CheckedSetDomain::unknown("not checked");
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
  std::vector<SetDomain> sets; // G5b-M4a, one per listed set.
};

DomainResult analyzeDomains(const SemanticProgram &program, const TerminationCertificate *certificate,
                            const DomainAnalysisRequest &request, SmtBackend &backend);

std::string toJSON(const SemanticProgram &program, const DomainResult &result, bool events);

} // namespace mlir::generalized_gemm
#endif
