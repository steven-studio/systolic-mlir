//===- Verdict.cpp - The three answers of the solver ----------------------===//

#include "Systolic/Exact/Verdict.h"

namespace systolic::exact {

const char *verdictName(Verdict verdict) {
  switch (verdict) {
  case Verdict::Feasible:
    return "FEASIBLE";
  case Verdict::Infeasible:
    return "INFEASIBLE";
  case Verdict::Unknown:
    return "UNKNOWN";
  }
  return "INVALID";
}

const char *domainName(Domain domain) {
  switch (domain) {
  case Domain::Integer:
    return "integer";
  case Domain::Rational:
    return "rational";
  }
  return "invalid";
}

const char *unknownReasonName(UnknownReason reason) {
  switch (reason) {
  case UnknownReason::None:
    return "none";
  case UnknownReason::RequiresSearch:
    return "requires-search";
  case UnknownReason::UnsupportedConstruct:
    return "unsupported-construct";
  case UnknownReason::StepLimit:
    return "step-limit";
  case UnknownReason::RowLimit:
    return "row-limit";
  case UnknownReason::VariableLimit:
    return "variable-limit";
  case UnknownReason::BitLimit:
    return "bit-limit";
  case UnknownReason::Cancelled:
    return "cancelled";
  case UnknownReason::CertificateRejected:
    return "certificate-rejected";
  }
  return "invalid";
}

} // namespace systolic::exact
