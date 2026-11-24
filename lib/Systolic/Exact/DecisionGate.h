//===- DecisionGate.h - The way into a Decision for Session -----*- C++ -*-===//
//
// Private to lib/Systolic/Exact. A Decision can only be made in Decide.cpp, and
// these functions are how Session reaches that code. A witness or a proof goes
// through the checker here, and a refused one comes back as Unknown with
// CertificateRejected, so Session cannot make a definite answer by itself.
//
//===----------------------------------------------------------------------===//

#ifndef SYSTOLIC_EXACT_DECISIONGATE_H
#define SYSTOLIC_EXACT_DECISIONGATE_H

#include "Systolic/Exact/Decide.h"

namespace systolic::exact::detail {

/// What the receipt of a Session query records.
struct TableauFacts {
  std::uint64_t steps = 0;
  std::uint64_t pivots = 0;
  bool ranTableau = false;
};

/// What the receipt of an integer query records.
struct SearchFacts {
  std::uint64_t steps = 0;
  std::uint64_t pivots = 0;
  std::uint64_t nodes = 0;
  bool ranTableau = false;
  bool ranSearch = false;
};

Decision gateWitness(const System &system, RationalWitness witness,
                     const TableauFacts &facts);
Decision gateProof(const System &system, InfeasibilityProof proof,
                   const TableauFacts &facts);
Decision gateUnknown(const System &system, UnknownReason reason,
                     const TableauFacts &facts);

Decision gateIntegerWitness(const System &system, IntegerWitness witness,
                            const SearchFacts &facts);
Decision gateBranchProof(const System &system, BranchProof proof,
                         const SearchFacts &facts);
Decision gateIntegerUnknown(const System &system, UnknownReason reason,
                            const SearchFacts &facts);

} // namespace systolic::exact::detail

#endif // SYSTOLIC_EXACT_DECISIONGATE_H
