//===- Checker.h - Independent checks of definite answers -------*- C++ -*-===//
//
// The trusted core. These functions recheck an answer from the definitions, and
// share only the data types with the code that produced it: they use nothing
// from Normalize, Lattice or Decide, so a defect there cannot hide here. They
// also do no rational arithmetic. A rational witness is read as numerators and
// denominators and evaluated in integers, so Int is all they have to trust.
//
// A check takes a System by reference for the length of the call and keeps
// nothing. It has no limits, because its work is linear in the certificate.
//
// A branch proof is walked with a stack of its own, so a deep proof cannot
// overflow the call stack, and every node has to be reached exactly once, so a
// proof cannot reuse a case or loop back on itself.
//
//===----------------------------------------------------------------------===//

#ifndef SYSTOLIC_EXACT_CHECKER_H
#define SYSTOLIC_EXACT_CHECKER_H

#include "Systolic/Exact/Certificate.h"
#include "Systolic/Exact/System.h"
#include "Systolic/Exact/Verdict.h"

namespace systolic::exact {

struct CheckResult {
  bool accepted = false;
  /// Why the certificate was refused. Empty when accepted.
  std::string why;
};

/// Every row holds at the point, over the integers.
CheckResult checkWitness(const System &system, const IntegerWitness &witness);

/// Every row holds at the point, over the rationals. Refused when the system
/// has a congruence, which is not defined there.
CheckResult checkWitness(const System &system, const RationalWitness &witness);

/// The steps derive a false row by the rules in Certificate.h. In the rational
/// domain Tighten and congruences are refused.
CheckResult checkInfeasibility(const System &system, Domain domain,
                               const InfeasibilityProof &proof);

/// The cases of the proof cover every integer point, and each leaf derives a
/// false row by those rules, from the system and the rows its path assumes.
CheckResult checkBranchProof(const System &system, const BranchProof &proof);

} // namespace systolic::exact

#endif // SYSTOLIC_EXACT_CHECKER_H
