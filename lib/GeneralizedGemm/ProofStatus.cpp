#include "GeneralizedGemm/ProofStatus.h"
#include "GeneralizedGemm/SemanticProgram.h"

using namespace mlir::generalized_gemm;

ProofStatus ProofStatus::proven(std::string certificate) {
  if (certificate.empty())
    return unknown("a proof needs a certificate");
  return ProofStatus(Kind::Proven, std::move(certificate));
}
ProofStatus ProofStatus::refuted(std::string witness) {
  if (witness.empty())
    return unknown("a refutation needs a witness");
  return ProofStatus(Kind::Refuted, std::move(witness));
}
ProofStatus ProofStatus::unknown(std::string reason) {
  return ProofStatus(Kind::Unknown, std::move(reason));
}
ProofStatus ProofStatus::assumed(std::string name) {
  if (name.empty())
    return unknown("an assumption needs a name");
  return ProofStatus(Kind::Assumed, std::move(name));
}
const char *ProofStatus::name() const {
  switch (statusKind) {
  case Kind::Proven: return "PROVEN";
  case Kind::Refuted: return "REFUTED";
  case Kind::Unknown: return "UNKNOWN";
  case Kind::Assumed: return "ASSUMED";
  }
  return "UNKNOWN";
}

namespace mlir::generalized_gemm {

ProofStatus conjoin(const ProofStatus &a, const ProofStatus &b) {
  auto rank = [](const ProofStatus &s) {
    switch (s.kind()) {
    case ProofStatus::Kind::Refuted: return 3;
    case ProofStatus::Kind::Unknown: return 2;
    case ProofStatus::Kind::Assumed: return 1;
    case ProofStatus::Kind::Proven: return 0;
    }
    return 3;
  };
  return rank(a) >= rank(b) ? a : b;
}

bool isDischarged(const ProofStatus &status, const Authorizations &authorized) {
  switch (status.kind()) {
  case ProofStatus::Kind::Proven: return true;
  case ProofStatus::Kind::Assumed: return authorized.count(status.detail()) != 0;
  case ProofStatus::Kind::Refuted:
  case ProofStatus::Kind::Unknown: return false;
  }
  return false;
}

const char *stringifyDomainForm(DomainForm form) {
  switch (form) {
  case DomainForm::ExactEnumerated: return "EXACT_ENUMERATED";
  case DomainForm::ExactPresburger: return "EXACT_PRESBURGER";
  case DomainForm::ExactClosedForm: return "EXACT_CLOSED_FORM";
  case DomainForm::OverApproximate: return "OVER_APPROXIMATE";
  case DomainForm::Unknown: return "UNKNOWN";
  }
  return "UNKNOWN";
}

DomainAdmission admitsClaim(const DomainEvidence &evidence, DomainClaim claim) {
  bool exact = evidence.form == DomainForm::ExactEnumerated ||
               evidence.form == DomainForm::ExactPresburger ||
               evidence.form == DomainForm::ExactClosedForm;
  if (evidence.form == DomainForm::Unknown)
    return {false, "the domain is unknown"};
  switch (claim) {
  case DomainClaim::UniversalOverDomain:
    return {true, "any superset of D supports a for-all claim"};
  case DomainClaim::Coverage:
    if (exact)
      return {true, "exact D supports coverage"};
    return {false, "coverage needs exact D; an over-approximation does not "
                   "establish that a point is executed"};
  case DomainClaim::RefutationWitness:
    if (evidence.form == DomainForm::ExactEnumerated)
      return {true, "the point was replayed concretely"};
    return {false, "a refutation needs a witness replayed by the concrete "
                   "evaluator"};
  }
  return {false, "unknown claim"};
}

bool scopeContains(const DomainEvidence &evidence,
                   const std::vector<llvm::APInt> &parameters) {
  if (evidence.scope.allParameters)
    return true;
  for (const ParameterRange &range : evidence.scope.ranges) {
    if (range.parameter >= parameters.size())
      return false;
    const llvm::APInt &value = parameters[range.parameter];
    if (value.getBitWidth() != range.lower.getBitWidth() ||
        value.slt(range.lower) || value.sgt(range.upper))
      return false;
  }
  return true;
}

DomainEvidence importedDomainEvidence() {
  DomainEvidence evidence;
  evidence.provenance = "import establishes nothing about D";
  return evidence;
}

const char *stringifySideKind(SideKind kind) {
  switch (kind) {
  case SideKind::UndefinedBehavior: return "UB";
  case SideKind::Underspecified: return "UNDERSPECIFIED";
  case SideKind::ModelLimit: return "MODEL_LIMIT";
  }
  return "UB";
}

const char *stringifyUnsupportedKind(UnsupportedKind kind) {
#define CASE(x) case UnsupportedKind::x: return #x;
  switch (kind) {
    CASE(NotVerified) CASE(ExternalFunction) CASE(NestedRegion)
    CASE(UnsupportedOperation) CASE(UnsupportedAttribute) CASE(UnsupportedType)
    CASE(UnsupportedTerminator) CASE(IndexWidthUnspecified)
    CASE(ConstantOutOfRange) CASE(NonIdentityLayout)
    CASE(NonDefaultMemorySpace) CASE(UnsupportedView) CASE(MemrefBlockArgument)
    CASE(MemrefReturn) CASE(AllocationInCycle) CASE(DynamicAllocation)
    CASE(DynamicDimensionIndex) CASE(RecursiveCall) CASE(ExternalCall)
    CASE(IndirectCall) CASE(CyclicCallee) CASE(LoweringFailed)
    CASE(ResourceLimit)
  }
#undef CASE
  return "UnsupportedOperation";
}

} // namespace mlir::generalized_gemm
