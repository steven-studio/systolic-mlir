//===- DomainChecker.h - The G5a authority ----------------------*- C++ -*-===//
//
// The only source of a verified domain or reduction claim. CheckedDomain has no public
// constructor for one, and this code links neither a proposer nor any Presburger or
// Simplex code. Every claim is bound to its program, group, scope and authorizations.
//
// Two routes give a domain tier, and neither follows from the other:
//
//   Point, ScopeEnumerated  each enumerated point: the run's events, in order, against
//                           S(v) computed exactly; multiplicities, the cell key, the
//                           collisions and the access forms from the same events.
//   Symbolic                D1 to D3 (Theorem 12), K1, C1, A1 and T1 over the scope.
//
// checkReduction re-runs the G3 and G4a checkers itself and joins them with these into
// Theorem 13. Solver unknowns, limits, unsupported forms and walks cut short give
// Unknown. Refuted needs a concrete point, which any run reproduces, or for a value
// fact a concrete memory. Transformation stays Unknown, and nothing authorizes lowering.
//
//===----------------------------------------------------------------------===//
#ifndef GENERALIZEDGEMM_DOMAINCHECKER_H
#define GENERALIZEDGEMM_DOMAINCHECKER_H

#include "GeneralizedGemm/ComputationChecker.h"
#include "GeneralizedGemm/DomainModel.h"
#include "GeneralizedGemm/GuardedOrbit.h"
#include "GeneralizedGemm/SiteSets.h"

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace mlir::generalized_gemm {

struct DomainLimits {
  uint64_t maxBox = 1u << 20; // Points of a box enumerated at one parameter point.
  EnumerationLimits events;   // The run that reads segment terms at each event (G5b).
  unsigned maxQueries = 6000, maxPairs = 4000, maxUnfolding = 64;
  unsigned witnessFills = 64;
  uint64_t witnessCells = 1u << 20;
};

struct DomainOptions {
  ParameterScope scope;
  TerminationLimits termination;
  Authorizations authorized;
  DomainLimits limits;
  ComputationLimits computation;
};

struct DomainAuthority; // Defined only where the checker is implemented.

class CheckedDomain {
public:
  /// Anyone may say they do not know.
  static CheckedDomain unknown(std::string reason);

  ComputationTier tier() const { return evidenceTier; }
  ComputationStatus status() const { return claimStatus; }
  const std::string &reason() const { return text; }
  const std::string &binding() const { return bound; }
  const std::vector<ObligationRecord> &records() const { return obligations; }
  const std::vector<DomainPointFacts> &points() const { return facts; }
  /// Theorem 12 at this tier: λ maps the events onto S(v), in order.
  bool exactOrder() const { return orbit; }
  DomainMultiplicity multiplicity() const { return form; }
  /// For ConstantBox: μ as a 128-bit term over parameters. For LogicalBox: over
  /// parameters and level variables, to be read at any event of the logical point; it
  /// reads no repeat level.
  Term multiplicityTerm() const { return mu; }
  const Fact &cellKey() const { return key; }
  /// Proven: no two logical outputs share a cell. Refuted: a point where two do.
  const Fact &outputInjective() const { return collision; }
  const std::array<Fact, 3> &access() const { return accessFacts; }
  /// Proven: no cell has two contributions, so no order exists to change.
  const Fact &singleContribution() const { return single; }
  /// G5b: one entry per segment of a segmented request, in execution order. Empty for a
  /// request with one order, whose facts are the ones above. For a segmented request the
  /// facts above are joined over the segments: a key or collision fact is Proven only
  /// when every segment's is, `multiplicity()` is Unit only when every segment's is and
  /// otherwise Unknown with no term, and the terms are here.
  const std::vector<SegmentFacts> &segments() const { return segmentList; }
  /// Proven: no cell is written by two segments. Refuted: a point where one is. Two
  /// segments may share a cell: this is a fact for G6, not a condition.
  const Fact &sharedCells() const { return shared; }
  /// For Refuted: the point and what is wrong there.
  const Fact &refutation() const { return counter; }
  const std::optional<std::vector<llvm::APInt>> &failingPoint() const { return failing; }

private:
  friend struct DomainAuthority;
  CheckedDomain(ComputationTier tier, ComputationStatus status, std::string reason)
      : evidenceTier(tier), claimStatus(status), text(std::move(reason)) {}
  ComputationTier evidenceTier;
  ComputationStatus claimStatus;
  std::string text, bound;
  std::vector<ObligationRecord> obligations;
  std::vector<DomainPointFacts> facts;
  bool orbit = false;
  DomainMultiplicity form = DomainMultiplicity::Unknown;
  Term mu = nullptr;
  Fact key, collision, single, counter, shared;
  std::array<Fact, 3> accessFacts;
  std::vector<SegmentFacts> segmentList;
  std::optional<std::vector<llvm::APInt>> failing;
};

/// Point or ScopeEnumerated. The enumeration must keep live state, so that every level
/// has a value at every event.
CheckedDomain checkDomainFinite(const SemanticProgram &program,
                                const DomainEnumeration &enumeration, unsigned group,
                                const CoordinateFamily &family, const DomainRequest &request,
                                const DomainOptions &options);

/// Symbolic. `certificate` is re-checked here.
CheckedDomain checkDomainSymbolic(const SemanticProgram &program,
                                  const TerminationCertificate *certificate,
                                  const SiteGroup &group, const CoordinateFamily &family,
                                  const DomainRequest &request, const DomainOptions &options,
                                  SmtBackend &backend);

/// G5b-M4a. One segmented domain claim over the union of several groups: Theorem 15
/// asked of every listed site at once, with M2's obligations (D1' to D5) and nothing new
/// about events. What a union adds is checked before any obligation:
///
///   XS  The set gate (uniteGroups): one to 64 whole groups, no root triple twice, no site
///       in two of them.
///   XP  The segments partition the union's sites, and each segment's sites belong to one
///       listed group: a segment never mixes groups.
///   CI  A cell is a root and an index. Cells of one root are compared by index. Cells of
///       two roots are compared only through rootRelation: apart by a fresh allocation, a
///       proof, or by an authorized name, an assumption; both are listed in rootPairs().
///       For roots that may alias, every fact that needs their cells apart (sharedCells,
///       singleContribution) stays Unknown, whatever the indices say.
///
/// Verified means Theorem 15 over the union at the tier given: the events of the listed
/// sites, in execution order, are segment 0's set S_0(v) in its order, then segment 1's,
/// and so on; no event of a listed site is left out and none is counted twice. The facts
/// are reported beside the claim and never raise it. Nothing here claims values (G4a, which
/// stays its own claim over a set), a fold (Theorem 17) or coordinates (Theorem 18).
struct RootPair {
  unsigned first = 0, second = 0; // Output roots of two segments, first < second.
  CellRelation relation = CellRelation::MayAlias;
  std::string basis;       // rootSeparation's: empty when the roots may alias.
  bool assumption = true;  // False only for a fresh allocation.
};

class CheckedSetDomain {
public:
  /// Anyone may say they do not know.
  static CheckedSetDomain unknown(std::string reason);

  ComputationTier tier() const { return evidenceTier; }
  ComputationStatus status() const { return claimStatus; }
  const std::string &reason() const { return text; }
  /// The program, the groups by their first site, the request, the scope and the names. No
  /// one-group claim has it.
  const std::string &binding() const { return bound; }
  /// The listed groups, by their first site: the listing's order changes nothing.
  const std::vector<SiteGroup> &groups() const { return groupList; }
  /// Per segment: the index in groups() of the group its sites belong to.
  const std::vector<unsigned> &segmentGroups() const { return segmentGroup; }
  const std::vector<ObligationRecord> &records() const { return obligations; }
  const std::vector<DomainPointFacts> &points() const { return facts; }
  /// Theorem 15 over the union, at this tier.
  bool exactOrder() const { return orbit; }
  /// One entry per segment, in execution order, each with its own key, collision, access
  /// and multiplicity facts.
  const std::vector<SegmentFacts> &segments() const { return segmentList; }
  /// Proven: no cell is written by two segments. Refuted: a point where one is, on one root.
  const Fact &sharedCells() const { return shared; }
  /// Proven: no cell has two contributions. Refuted: a point where one has, on one root.
  const Fact &singleContribution() const { return single; }
  /// Every two different output roots of the segments, and how they stand.
  const std::vector<RootPair> &rootPairs() const { return pairs; }
  /// For Refuted: the point and what is wrong there.
  const Fact &refutation() const { return counter; }
  const std::optional<std::vector<llvm::APInt>> &failingPoint() const { return failing; }

private:
  friend struct DomainAuthority;
  CheckedSetDomain(ComputationTier tier, ComputationStatus status, std::string reason)
      : evidenceTier(tier), claimStatus(status), text(std::move(reason)) {}
  ComputationTier evidenceTier;
  ComputationStatus claimStatus;
  std::string text, bound;
  std::vector<SiteGroup> groupList;
  std::vector<unsigned> segmentGroup;
  std::vector<ObligationRecord> obligations;
  std::vector<DomainPointFacts> facts;
  bool orbit = false;
  std::vector<SegmentFacts> segmentList;
  Fact shared, single, counter;
  std::vector<RootPair> pairs;
  std::optional<std::vector<llvm::APInt>> failing;
};

/// Point or ScopeEnumerated, at the points of an enumeration. `groups` index its groups.
CheckedSetDomain checkSetDomainFinite(const SemanticProgram &program,
                                      const DomainEnumeration &enumeration,
                                      llvm::ArrayRef<unsigned> groups, const DomainRequest &request,
                                      const DomainOptions &options);

/// Symbolic, over the scope. `certificate` is re-checked here.
CheckedSetDomain checkSetDomainSymbolic(const SemanticProgram &program,
                                        const TerminationCertificate *certificate,
                                        llvm::ArrayRef<SiteGroup> groups, const DomainRequest &request,
                                        const DomainOptions &options, SmtBackend &backend);

class ReductionVerdict;
ReductionVerdict checkReduction(const SemanticProgram &program,
                                const TerminationCertificate *certificate,
                                const DomainEnumeration &enumeration, unsigned group,
                                const CoordinateFamily &family,
                                const EnumerationOrder *coordinateOrder,
                                const DomainRequest &request, const DomainOptions &options,
                                SmtBackend &backend);

/// Theorem 13 and the reorder facts, from claims checkReduction makes itself. Only that
/// function sets anything here.
class ReductionVerdict {
public:
  static ReductionVerdict unknown(std::string reason) {
    ReductionVerdict out;
    out.why = std::move(reason);
    return out;
  }
  const CheckedFamily &coordinateFinite() const { return coordFinite; }
  const CheckedFamily &coordinateSymbolic() const { return coordSymbolic; }
  const CheckedComputation &computationFinite() const { return compFinite; }
  const CheckedComputation &computationSymbolic() const { return compSymbolic; }
  const CheckedDomain &domainFinite() const { return domFinite; }
  const CheckedDomain &domainSymbolic() const { return domSymbolic; }
  /// Theorem 13 over λ: C_final[c] is the fold of the contributions of c in the order
  /// of the cell key, from G4a and Theorem 12.
  ComputationStatus status() const { return claim; }
  ComputationTier tier() const { return claimTier; }
  const std::vector<std::vector<llvm::APInt>> &points() const { return at; } // Finite tiers.
  bool everyRun() const { return every; }
  /// The same over the logical coordinates (P, I, J, K), which needs G3 too.
  ComputationTier logicalTier() const { return logical; }
  const std::string &reason() const { return why; }
  /// Each carries its own tier, which may be lower than Theorem 13's.
  const Fact &trivialReorder() const { return trivial; }
  const Fact &integerReorder() const { return integer; }
  const Fact &floatReorder() const { return floating; }
  const ProofStatus &transformation() const { return transformationStatus; }

private:
  ReductionVerdict() = default;
  friend ReductionVerdict checkReduction(const SemanticProgram &, const TerminationCertificate *,
                                         const DomainEnumeration &, unsigned, const CoordinateFamily &,
                                         const EnumerationOrder *, const DomainRequest &,
                                         const DomainOptions &, SmtBackend &);
  CheckedFamily coordFinite = CheckedFamily::unknown("not checked");
  CheckedFamily coordSymbolic = CheckedFamily::unknown("not checked");
  CheckedComputation compFinite = CheckedComputation::unknown("not checked");
  CheckedComputation compSymbolic = CheckedComputation::unknown("not checked");
  CheckedDomain domFinite = CheckedDomain::unknown("not checked");
  CheckedDomain domSymbolic = CheckedDomain::unknown("not checked");
  ComputationStatus claim = ComputationStatus::Unknown;
  ComputationTier claimTier = ComputationTier::Proposed;
  std::vector<std::vector<llvm::APInt>> at;
  bool every = false;
  ComputationTier logical = ComputationTier::Proposed;
  std::string why;
  Fact trivial, integer, floating;
  ProofStatus transformationStatus =
      ProofStatus::unknown("no transformation is claimed before G6 and G7");
};

} // namespace mlir::generalized_gemm
#endif
