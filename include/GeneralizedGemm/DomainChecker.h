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

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace mlir::generalized_gemm {

struct DomainLimits {
  uint64_t maxBox = 1u << 20; // Points of a box enumerated at one parameter point.
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
  /// For ConstantBox: μ as a 128-bit term over parameters.
  Term multiplicityTerm() const { return mu; }
  const Fact &cellKey() const { return key; }
  /// Proven: no two logical outputs share a cell. Refuted: a point where two do.
  const Fact &outputInjective() const { return collision; }
  const std::array<Fact, 3> &access() const { return accessFacts; }
  /// Proven: no cell has two contributions, so no order exists to change.
  const Fact &singleContribution() const { return single; }
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
  Fact key, collision, single, counter;
  std::array<Fact, 3> accessFacts;
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
