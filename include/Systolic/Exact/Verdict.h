//===- Verdict.h - The three answers of the solver --------------*- C++ -*-===//
//
// Verdict and Domain are scoped enumerations, so neither converts to bool. A
// caller has to name the answer it is testing for, which is what keeps Unknown
// from being read as either of the other two.
//
//===----------------------------------------------------------------------===//

#ifndef SYSTOLIC_EXACT_VERDICT_H
#define SYSTOLIC_EXACT_VERDICT_H

namespace systolic::exact {

enum class Verdict {
  /// A point exists, and the Decision carries it.
  Feasible,
  /// No point exists, and the Decision carries the proof.
  Infeasible,
  /// Nothing is asserted either way.
  Unknown,
};

/// Where the variables range, which changes the question. { 2x = 1 } has a
/// rational point and no integer point. Rows may only be tightened by a gcd in
/// the integer domain, since that removes rational points.
enum class Domain { Integer, Rational };

/// Why a Decision is Unknown.
enum class UnknownReason {
  None,
  /// The system survived every implemented reduction. Deciding it needs a
  /// search that does not exist yet, so nothing can be said about it.
  RequiresSearch,
  /// The system uses something this domain does not define.
  UnsupportedConstruct,
  StepLimit,
  RowLimit,
  VariableLimit,
  BitLimit,
  Cancelled,
  /// The solver produced an answer and the checker refused it, or the lattice
  /// step failed its determinant check. Either is a solver bug, reported as
  /// Unknown because the answer cannot be trusted.
  CertificateRejected,
};

const char *verdictName(Verdict verdict);
const char *domainName(Domain domain);
const char *unknownReasonName(UnknownReason reason);

} // namespace systolic::exact

#endif // SYSTOLIC_EXACT_VERDICT_H
