//===- Decide.h - The feasibility decision ----------------------*- C++ -*-===//
//
// decide() asks whether a System has a point in a Domain. It answers only what
// equality elimination and row normalization settle. Anything that survives
// them is Unknown with reason RequiresSearch, which says nothing about
// feasibility. The rational and integer searches are in Session.h. They are
// opt-in, and decide() does not call them. decide() does not read strict rows,
// so a system with one is Unknown with reason UnsupportedConstruct.
//
// Contract:
//   Feasible    the Decision holds a witness in the domain, accepted by the
//               checker against the original rows.
//   Infeasible  the Decision holds a proof, accepted by the checker.
//   Unknown     nothing is asserted, and why() says why.
//
// A Decision is Feasible or Infeasible only through the checker: its
// constructor is private, and every path to one runs checkWitness or
// checkInfeasibility first. If the checker refuses, the answer is Unknown with
// reason CertificateRejected. Decision has no conversion to bool, so a call
// site has to say which answer it is testing for.
//
// decide() is a pure function of its arguments, apart from the cancellation
// token and the process-wide execution counters in Provenance.h. It uses no
// clock, no randomness and no address-dependent order, so the same system and
// limits give the same answer, proof and step count. It reports failure as a
// value and never throws or aborts.
//
// Ownership: a Decision owns everything it holds and refers to nothing in the
// System it came from, so it may outlive it. The System is read for the length
// of the call only.
//
//===----------------------------------------------------------------------===//

#ifndef SYSTOLIC_EXACT_DECIDE_H
#define SYSTOLIC_EXACT_DECIDE_H

#include "Systolic/Exact/Budget.h"
#include "Systolic/Exact/Certificate.h"
#include "Systolic/Exact/System.h"
#include "Systolic/Exact/Verdict.h"

#include <cstdint>

namespace systolic::exact {

/// Evidence that one call of decide() ran, bound to that call's input and
/// output. See Provenance.h for how it differs from a declaration.
struct ExecutionReceipt {
  /// FNV-1a 64 of receiptText() for this query. 0 when no query made this
  /// Decision.
  std::uint64_t digest = 0;
  /// Work charged by the query, in WorkMeter steps.
  std::uint64_t steps = 0;
  bool ranLattice = false;
  bool ranNormalize = false;
  bool ranChecker = false;
  /// Whether this query ran the tableau of the rational solver in Session.h.
  bool ranTableau = false;
  /// Pivots of that tableau in this query.
  std::uint64_t pivots = 0;
  /// Whether this query ran the integer search in Session.h.
  bool ranSearch = false;
  /// Nodes of that search in this query.
  std::uint64_t nodes = 0;
};

class [[nodiscard]] Decision {
public:
  Verdict verdict() const { return answer; }
  Domain domain() const { return where; }

  bool provedFeasible() const { return answer == Verdict::Feasible; }
  bool provedInfeasible() const { return answer == Verdict::Infeasible; }
  bool isUnknown() const { return answer == Verdict::Unknown; }

  /// UnknownReason::None unless the verdict is Unknown.
  UnknownReason why() const { return reason; }

  /// Set exactly when provedFeasible() in the integer domain.
  const IntegerWitness *integerWitness() const {
    return integerPoint ? &*integerPoint : nullptr;
  }
  /// Set exactly when provedFeasible() in the rational domain.
  const RationalWitness *rationalWitness() const {
    return rationalPoint ? &*rationalPoint : nullptr;
  }
  /// Set when provedInfeasible() by a derivation.
  const InfeasibilityProof *proof() const {
    return emptyProof ? &*emptyProof : nullptr;
  }
  /// Set when provedInfeasible() by cases. Exactly one of the two is set then.
  const BranchProof *branchProof() const {
    return branchTree ? &*branchTree : nullptr;
  }

  const ExecutionReceipt &receipt() const { return evidence; }

  /// Deleted so that `if (decision)` does not compile.
  explicit operator bool() const = delete;

private:
  friend struct DecisionMaker;
  Decision() = default;

  Verdict answer = Verdict::Unknown;
  Domain where = Domain::Integer;
  UnknownReason reason = UnknownReason::None;
  std::optional<IntegerWitness> integerPoint;
  std::optional<RationalWitness> rationalPoint;
  std::optional<InfeasibilityProof> emptyProof;
  std::optional<BranchProof> branchTree;
  ExecutionReceipt evidence;
};

/// Decides whether `system` has a point in `domain`. `cancellation` may be null;
/// otherwise it must outlive the call.
Decision decide(const System &system, Domain domain,
                const WorkLimits &limits = WorkLimits(),
                CancellationToken *cancellation = nullptr);

/// The way to turn a certificate from elsewhere into a Decision: the checker
/// runs, and a refused certificate gives Unknown with CertificateRejected.
Decision certifyFeasible(const System &system, IntegerWitness witness);
Decision certifyFeasible(const System &system, RationalWitness witness);
Decision certifyInfeasible(const System &system, Domain domain,
                           InfeasibilityProof proof);
Decision certifyInfeasible(const System &system, BranchProof proof);

/// The text whose FNV-1a 64 is ExecutionReceipt::digest. Exact format:
///   canonicalText(system), then the lines
///   domain=<name> verdict=<name> reason=<name> steps=<n>
///   stages=<lattice,normalize,tableau,search,checker, or the subset that ran>
/// and, when the tableau ran, the line pivots=<n>, and when the search ran, the
/// line nodes=<n>.
std::string receiptText(const System &system, const Decision &decision);

/// A text form of the answer: verdict, domain, reason, and the witness or proof.
/// Equal answers give equal text.
std::string canonicalText(const Decision &decision);

/// FNV-1a 64 of the bytes.
std::uint64_t fnv1a64(std::string_view bytes);

} // namespace systolic::exact

#endif // SYSTOLIC_EXACT_DECIDE_H
