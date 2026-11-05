//===- ComputationChecker.h - The G4a authority -----------------*- C++ -*-===//
//
// The only source of a verified computation claim. CheckedComputation has no public
// constructor for one, and this code links neither the grammar nor the synthesis. A
// candidate's coordinates are checked again here, because a CheckedFamily names no
// program, family or group that a caller could not have swapped.
//
// What is proved, for all initial memories whose run completes, is the replay
// relation of ComputationModel.h. What is not:
//
//   coordinate correctness   G3: reported beside this, and re-checked.
//   domain and order         G5: `domainAndOrder` stays Unknown.
//   transformation           G6, G7: `transformationEquivalence` stays Unknown.
//
// Two independent routes give a tier, and neither follows from the other:
//
//   Point, ScopeEnumerated   Herbrand replay of each exact point (Theorem 11).
//   Symbolic                 E, OP, R, W and F over the whole scope (Theorem 9).
//
// A solver's unknown, an exhausted limit, an unsupported form and a Herbrand
// mismatch all give Unknown. Refuted needs a concrete run, which is replayed.
// Separation of two roots comes from fresh allocation or from an authorized name,
// and every use is listed in the result. No result authorizes a lowering.
//
//===----------------------------------------------------------------------===//
#ifndef GENERALIZEDGEMM_COMPUTATIONCHECKER_H
#define GENERALIZEDGEMM_COMPUTATIONCHECKER_H

#include "GeneralizedGemm/ComputationModel.h"
#include "GeneralizedGemm/HerbrandReplay.h"
#include "GeneralizedGemm/SymbolicCandidateChecker.h"

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace mlir::generalized_gemm {

struct ComputationLimits {
  HerbrandLimits replay;
  unsigned maxQueries = 4000, maxPairs = 4000;
  // Concrete runs tried for a witness, after a mismatch. Only these use fills.
  unsigned witnessFills = 64;
  uint64_t witnessFuel = 1u << 22, witnessCells = 1u << 20;
};

struct ComputationOptions {
  ParameterScope scope;
  TerminationLimits termination;
  Authorizations authorized;
  ComputationLimits limits;
};

struct ComputationAuthority; // Defined only where the checker is implemented.

class CheckedComputation {
public:
  /// Anyone may say they do not know.
  static CheckedComputation unknown(std::string reason);

  ComputationTier tier() const { return evidenceTier; }
  ComputationStatus status() const { return claimStatus; }
  const std::string &reason() const { return text; }
  const std::vector<ObligationRecord> &records() const { return obligations; }
  const std::vector<SiteContract> &contracts() const { return siteContracts; }
  FoldForm fold() const { return foldForm; }
  /// Every use of the separation of two roots. Nothing is implied by their ids.
  const std::vector<SeparationUse> &separations() const { return separationUses; }
  /// Every memory effect outside the group. Nonempty means the group is not the
  /// whole program.
  const std::vector<ResidualEffect> &frame() const { return residual; }
  const std::set<MemoryDependentStop> &memoryStops() const { return stops; }
  const std::vector<ReplayFacts> &points() const { return facts; }
  /// True: every run of the claim's points. False: the runs that complete.
  bool holdsForEveryRun() const { return every; }
  /// A concrete entry at which the program and the replay differ. Set for Refuted.
  const std::optional<EntryState> &witness() const { return counterexample; }
  /// Parameters of a failed query. On an over-approximation: a lead, not a witness.
  const std::optional<std::vector<llvm::APInt>> &failingPoint() const { return failing; }
  /// The program, group, scope and authorizations this claim is about.
  const std::string &binding() const { return bound; }
  /// Always Unknown here: G5 owns these.
  const ProofStatus &domainAndOrder() const { return domainStatus; }
  /// Always Unknown here: G6 and G7 own these.
  const ProofStatus &transformationEquivalence() const { return transformationStatus; }

private:
  friend struct ComputationAuthority;
  CheckedComputation(ComputationTier tier, ComputationStatus status, std::string reason)
      : evidenceTier(tier), claimStatus(status), text(std::move(reason)) {}
  ComputationTier evidenceTier;
  ComputationStatus claimStatus;
  std::string text, bound;
  std::vector<ObligationRecord> obligations;
  std::vector<SiteContract> siteContracts;
  FoldForm foldForm = FoldForm::OrderedFloat;
  std::vector<SeparationUse> separationUses;
  std::vector<ResidualEffect> residual;
  std::set<MemoryDependentStop> stops;
  std::vector<ReplayFacts> facts;
  bool every = false;
  std::optional<EntryState> counterexample;
  std::optional<std::vector<llvm::APInt>> failing;
  ProofStatus domainStatus = ProofStatus::unknown("no domain or order is claimed before G5");
  ProofStatus transformationStatus =
      ProofStatus::unknown("no transformation is claimed before G6 and G7");
};

/// Whether `enumeration` ran every point of `scope` and no other: every point lies in
/// the scope, and their number, with those outside the domain, is the scope's. A
/// claim with fewer points is about those points, whatever scope it is given.
bool enumerationCoversScope(const SemanticProgram &program, const DomainEnumeration &enumeration,
                            const ParameterScope &scope);

/// Point or ScopeEnumerated. `group` indexes the enumeration's groups.
CheckedComputation checkComputationFinite(const SemanticProgram &program,
                                          const DomainEnumeration &enumeration, unsigned group,
                                          const ComputationOptions &options);

/// Symbolic. `certificate` is re-checked here.
CheckedComputation checkComputationSymbolic(const SemanticProgram &program,
                                            const TerminationCertificate *certificate,
                                            const SiteGroup &group,
                                            const ComputationOptions &options, SmtBackend &backend);

/// A concrete entry at which `run` of `entry` differs from the replay. Exposed so a
/// witness can be judged again by a test. Nullopt means no difference was shown,
/// which proves nothing. `prefix` is the replay's starting constants.
std::optional<std::string> replayDifference(const SemanticProgram &program,
                                            const SiteGroup &group, const EntryState &entry,
                                            const RunResult &run,
                                            const std::map<CellKey, llvm::APInt> &prefix);

enum class ViewInjectivity { Proven, NotProven };

/// One candidate: coordinates re-checked, and joined with the group's claims. A
/// tier here is the least of the two, at the points they share.
struct CandidateComputation {
  CoordinateFamily family;
  CheckedFamily coordinateFinite = CheckedFamily::unknown("not checked");
  CheckedFamily coordinateSymbolic = CheckedFamily::unknown("not checked");
  ComputationStatus status = ComputationStatus::Unknown;
  ComputationTier tier = ComputationTier::Proposed;
  std::vector<std::vector<llvm::APInt>> points; // For a finite tier.
  bool everyRun = false;
  std::string reason;
  // Facts, and no condition of validity. H, F, G over the whole scope.
  std::array<ViewInjectivity, 3> symbolicInjective{ViewInjectivity::NotProven,
                                                   ViewInjectivity::NotProven,
                                                   ViewInjectivity::NotProven};
};

CandidateComputation checkCandidateComputation(
    const SemanticProgram &program, const TerminationCertificate *certificate,
    const DomainEnumeration &enumeration, unsigned group, const CoordinateFamily &family,
    const EnumerationOrder *order, const CheckedComputation &finiteClaimOfGroup,
    const CheckedComputation &symbolicClaimOfGroup, const ComputationOptions &options,
    SmtBackend &backend);

} // namespace mlir::generalized_gemm
#endif
