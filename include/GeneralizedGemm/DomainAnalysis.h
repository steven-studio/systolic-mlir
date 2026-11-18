//===- DomainAnalysis.h - G5a entry point -----------------------*- C++ -*-===//
//
// Runs the G3b search for candidates, enumerates the scope with live state, proposes a
// guarded order for each candidate, and hands everything to DomainChecker. The search
// and the proposer only propose. Nothing here authorizes a transformation.
//
//===----------------------------------------------------------------------===//
#ifndef GENERALIZEDGEMM_DOMAINANALYSIS_H
#define GENERALIZEDGEMM_DOMAINANALYSIS_H

#include "GeneralizedGemm/ClosedFormSynthesis.h"
#include "GeneralizedGemm/DomainChecker.h"
#include "GeneralizedGemm/DomainProposer.h"
#include "GeneralizedGemm/IndexInvariantSynthesis.h"
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
  // Phase 4 (I2, I3): run the closed-form synthesis of ClosedFormSynthesis.h over
  // the enumerated points and hand the proposer state identities: every PROVED
  // loop-free form of a loop whose header has an iteration counter
  // (LoopCounters.h), with m read as that counter, and every variable defined on
  // every path from the entry by one term over parameters. The proposer then
  // writes guards, bounds and views over levels; the checkers judge them as any
  // proposal. Off by default, and then nothing in the output changes.
  bool closedFormViews = false;
  ClosedFormOptions closedForms;
  // Section 14: identities from proved index invariants (IndexInvariantSynthesis.h),
  // handed to the proposer beside the ones above, so that a view can be written over
  // the levels where a subscript is a carried variable. Proposals: the domain checker
  // proves each view, under the certificate it is handed, or does not. The certificate
  // is the caller's to strengthen; nothing is added to it here for these.
  std::vector<StateIdentity> indexIdentities;
};

/// G5b-M4a: one set of groups, its request, and both routes' claims.
struct SetDomain {
  std::vector<unsigned> groups; // As listed.
  std::optional<DomainRequest> request;
  std::string proposalReason;
  CheckedSetDomain finite = CheckedSetDomain::unknown("not checked");
  CheckedSetDomain symbolic = CheckedSetDomain::unknown("not checked");
  // G5b-M4b-1: Theorem 17 over the set, whose domain components are the two above.
  SetReductionVerdict reduction = SetReductionVerdict::unknown("not checked");
};

struct CandidateDomain {
  CoordinateFamily family;
  std::optional<EnumerationOrder> coordinateOrder;
  std::optional<DomainRequest> request;
  std::string proposalReason;
  ReductionVerdict verdict = ReductionVerdict::unknown("not checked");
  // Phase 4: with closedFormViews, the termination certificate handed to the
  // checkers is the given one strengthened by the identities as invariants at
  // their cuts (v = f(c_h) at the loop header, the entry definitions everywhere)
  // and by the proposed bounds of every counter level, when the certificate
  // checker accepts the strengthened certificate; otherwise the given one. Either
  // way every checker re-checks the certificate it is handed.
  bool certificateStrengthened = false;
  std::vector<std::string> invariantsAdded;
  std::string certificateNote;
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
  // Phase 4, only with closedFormViews: what the proposer was handed and where it
  // came from. Proposals, not evidence.
  std::vector<StateIdentity> identities;
  std::optional<SliceSynthesisResult> closedForms;
  std::string identityReason; // Why some identity could not be formed.
  // Section 14: the request's indexIdentities, as handed to the proposer.
  std::vector<StateIdentity> indexIdentities;
};

/// Phase 4 (I3). Whether a term over level variables, parameters and constants is
/// affine: sums and differences of constant multiples of leaves and constants. An
/// `ite`, a remainder, a division, a product of two non-constants, a comparison or a
/// width change makes it non-affine. Definition 4.5 (V) of the thesis asks for an
/// affine view; this is the syntactic test the record reports, not a proof of
/// anything about the program.
bool affineOverLevels(Term term);

/// Section 14: the two things a search for index invariants gives the domain analysis,
/// handed over in one place. When the search has a strengthened certificate (one the
/// certificate checker accepted), its solved equalities are appended to
/// request.indexIdentities (and copied to `handed`, when given) and that certificate is
/// returned: it is the one to pass to analyzeDomains. Otherwise nothing is appended and
/// `given` is returned. The two travel together on purpose: an identity without the
/// invariant behind it proposes a view no checker can prove, and the invariant without
/// the identity leaves the view unproposed.
const TerminationCertificate *handIndexInvariants(const SemanticProgram &program,
                                                  const TerminationCertificate *given,
                                                  const IndexInvariantSearch &search,
                                                  DomainAnalysisRequest &request,
                                                  std::vector<IndexIdentity> *handed = nullptr);

DomainResult analyzeDomains(const SemanticProgram &program, const TerminationCertificate *certificate,
                            const DomainAnalysisRequest &request, SmtBackend &backend);

std::string toJSON(const SemanticProgram &program, const DomainResult &result, bool events);

} // namespace mlir::generalized_gemm
#endif
