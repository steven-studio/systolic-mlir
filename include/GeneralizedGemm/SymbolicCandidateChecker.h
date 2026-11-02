//===- SymbolicCandidateChecker.h - G3b evidence tiers ----------*- C++ -*-===//
//
// The only authority for an evidence tier. CheckedFamily has no public
// constructor for a verified result, and this code links neither the grammar nor
// the synthesis. Each tier has its own obligations, and none follows from another.
//
//   Point            exact at each listed point: the G3a checker on exact domains.
//   ScopeEnumerated  the same, at every point of a finite scope.
//   SymbolicValid    V0 to V4 for every point of the scope, over an
//                    over-approximation. No coverage is claimed.
//   SymbolicExact    SymbolicValid and Theorem 8: the events are exactly the
//                    orbit of the declared order, each once, in that order.
//
// A sample proves nothing about another point. A solver's unknown, a limit and an
// unsupported form give Unknown, and a tier never rises from one. No result
// proves GEMM equivalence or authorizes a transformation.
//
//===----------------------------------------------------------------------===//
#ifndef GENERALIZEDGEMM_SYMBOLICCANDIDATECHECKER_H
#define GENERALIZEDGEMM_SYMBOLICCANDIDATECHECKER_H

#include "GeneralizedGemm/CandidateChecker.h"
#include "GeneralizedGemm/CoordinateFamily.h"
#include "GeneralizedGemm/FiniteDomain.h"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace mlir::generalized_gemm {

enum class EvidenceTier { Proposed, Point, ScopeEnumerated, SymbolicValid, SymbolicExact };
const char *stringifyEvidenceTier(EvidenceTier tier);

enum class FamilyStatus { Verified, Refuted, Unknown };
const char *stringifyFamilyStatus(FamilyStatus status);

struct PointFacts {
  std::vector<llvm::APInt> parameters;
  CandidateReport report;
};

struct FamilyAuthority; // Defined only where the checker is implemented.

class CheckedFamily {
public:
  static CheckedFamily unknown(std::string reason);

  EvidenceTier tier() const { return evidenceTier; }
  FamilyStatus status() const { return familyStatus; }
  const std::string &reason() const { return text; }
  const std::vector<ObligationRecord> &records() const { return obligations; }
  const std::vector<PointFacts> &points() const { return facts; }
  /// A replayable point for a refutation, or a parameter model of a failed query.
  const std::optional<std::vector<llvm::APInt>> &failingPoint() const { return failing; }
  /// Exact only for SymbolicExact.
  const DomainEvidence &coverage() const { return domain; }
  bool symbolicBox() const { return box; }
  /// What a symbolic tier quantifies over. True: every run of every point of the
  /// scope, as the program provably returns. False: the runs that complete. A
  /// point at which every run stops on undefined behaviour then satisfies the claim
  /// vacuously, so the stops of the certificate must be read with the tier.
  bool holdsForEveryRun() const { return every; }
  /// Always Unknown in G3b.
  const ProofStatus &equivalence() const { return equivalenceStatus; }

private:
  friend struct FamilyAuthority;
  CheckedFamily(EvidenceTier tier, FamilyStatus status, std::string reason)
      : evidenceTier(tier), familyStatus(status), text(std::move(reason)) {}
  EvidenceTier evidenceTier;
  FamilyStatus familyStatus;
  std::string text;
  std::vector<ObligationRecord> obligations;
  std::vector<PointFacts> facts;
  std::optional<std::vector<llvm::APInt>> failing;
  DomainEvidence domain;
  bool box = false, every = false;
  ProofStatus equivalenceStatus =
      ProofStatus::unknown("no equivalence is claimed before G4 to G7");
};

/// Point or ScopeEnumerated. `group` indexes the enumeration's groups.
CheckedFamily checkFamilyFinite(const SemanticProgram &program,
                                const DomainEnumeration &enumeration, unsigned group,
                                const CoordinateFamily &family);

struct SymbolicLimits2 {
  PathLimits paths;
  unsigned maxQueries = 2000, maxPairs = 400, maxUnfolding = 64;
};

struct SymbolicCheckOptions {
  ParameterScope scope;
  TerminationLimits termination;
  Authorizations authorized;
  SymbolicLimits2 limits;
};

/// SymbolicValid, then SymbolicExact when the order proves coverage. `certificate`
/// is re-checked here. `order` may be null: there is no tier without one, because
/// V3 needs the set the coordinates range over.
CheckedFamily checkFamilySymbolic(const SemanticProgram &program,
                                  const TerminationCertificate *certificate,
                                  const SiteGroup &group, const CoordinateFamily &family,
                                  const EnumerationOrder *order,
                                  const SymbolicCheckOptions &options, SmtBackend &backend);

} // namespace mlir::generalized_gemm
#endif
