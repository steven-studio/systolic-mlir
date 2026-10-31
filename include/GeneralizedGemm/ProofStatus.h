//===- ProofStatus.h - Proof states and domain polarity ---------*- C++ -*-===//
//
// Generalized GEMM recognition, Phase G1 (docs/GENERALIZED_GEMM_G0_G1.md).
//
// G1 issues no GEMM verdict. There is deliberately no ACCEPT/REJECT type here:
// this header only defines what later phases must carry.
//
//===----------------------------------------------------------------------===//
#ifndef GENERALIZEDGEMM_PROOFSTATUS_H
#define GENERALIZEDGEMM_PROOFSTATUS_H

#include "llvm/ADT/APInt.h"

#include <set>
#include <string>
#include <vector>

namespace mlir::generalized_gemm {

/// The status of one proof obligation (invariant I9). The only constructors
/// are the factories below, and no operation turns an Assumed status into a
/// Proven one. An empty certificate or witness yields Unknown.
class ProofStatus {
public:
  enum class Kind { Proven, Refuted, Unknown, Assumed };

  static ProofStatus proven(std::string certificate);
  static ProofStatus refuted(std::string witness);
  static ProofStatus unknown(std::string reason);
  static ProofStatus assumed(std::string name);

  Kind kind() const { return statusKind; }
  const std::string &detail() const { return text; }
  const char *name() const;

private:
  ProofStatus(Kind kind, std::string detail)
      : statusKind(kind), text(std::move(detail)) {}
  Kind statusKind;
  std::string text;
};

/// Conjunction: Refuted dominates, then Unknown, then Assumed, then Proven.
/// A conjunction with an assumption is never Proven.
ProofStatus conjoin(const ProofStatus &a, const ProofStatus &b);

/// Assumptions the user explicitly authorized by name, as `assume-no-alias`
/// is today. An authorization makes an Assumed obligation usable. It does not
/// change its kind: the status stays Assumed and stays reported.
using Authorizations = std::set<std::string>;
bool isDischarged(const ProofStatus &status,
                  const Authorizations &authorized = {});

/// How the iteration domain D is known (invariant I8).
enum class DomainForm {
  ExactEnumerated, // Complete concrete run, for its parameter scope only.
  ExactPresburger, // Reserved: G1 never constructs it.
  ExactClosedForm, // Reserved: G1 never constructs it.
  OverApproximate, // D-hat contains D.
  Unknown,
};
const char *stringifyDomainForm(DomainForm form);

/// The shape of a claim about D. Section 2.4 of the design:
///   UniversalOverDomain: a for-all claim. Any D-hat containing D suffices.
///   Coverage: surjectivity or Cartesian structure. Needs exact D.
///   RefutationWitness: a point claimed to lie in D. Needs a concrete replay.
enum class DomainClaim { UniversalOverDomain, Coverage, RefutationWitness };

/// Closed ranges for parameters (Decision 1: parameter scope). Parameters not
/// listed are unconstrained. Values are signed representatives at the
/// parameter's width.
struct ParameterRange {
  unsigned parameter = 0;
  llvm::APInt lower, upper;
};
struct ParameterScope {
  std::vector<ParameterRange> ranges;
  bool allParameters = true; // No restriction. False for a concrete run.
};

struct DomainEvidence {
  DomainForm form = DomainForm::Unknown;
  std::string provenance;
  ParameterScope scope;
};

struct DomainAdmission {
  bool admitted = false;
  std::string reason;
};

/// The single gate every later obligation must pass: whether `evidence` may
/// support a `claim`. Unknown forms support nothing.
DomainAdmission admitsClaim(const DomainEvidence &evidence, DomainClaim claim);

/// Whether a parameter assignment lies inside the evidence's scope. A claim
/// about any other parameter value is out of scope, even when admitted.
bool scopeContains(const DomainEvidence &evidence,
                   const std::vector<llvm::APInt> &parameters);

} // namespace mlir::generalized_gemm
#endif
