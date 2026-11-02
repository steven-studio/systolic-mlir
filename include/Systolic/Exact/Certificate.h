//===- Certificate.h - Witnesses and infeasibility proofs -------*- C++ -*-===//
//
// What a definite answer carries, so that it can be rechecked without trusting
// the code that found it. Checker.h does the rechecking.
//
// A witness is a point, one value per variable of the system.
//
// An infeasibility proof is a derivation of a false row. Each step derives one
// row over the system's variables, followed by one quotient per congruence,
// taken in order. It is a list of steps, so a row can be used twice, and each
// step reads only earlier ones. A row is either closed, as a . x + c >= 0, or
// strict, as a . x + c > 0. The last step must be a false row with no
// variables: c >= 0 for a negative c, or c > 0 for c <= 0. There are three
// rules:
//
//   Input     an inequality, a strict inequality, or an equality or congruence
//             with either sign. Congruence i contributes expr - m * q_i == 0.
//   Combine   a sum of earlier rows with positive integer multipliers, strict
//             when any row in it is strict. Valid over the rationals and the
//             integers.
//   Tighten   a . x + c >= 0 with g = gcd(a) > 0 becomes
//             (a / g) . x + floor(c / g) >= 0. Valid over the integers only:
//             a / g . x is an integer, so it may round. This is the step of
//             Gomory (1958) and Chvatal (1973). It takes closed rows only.
//
// Combine alone is the Farkas lemma, and it is what a conflict of the rational
// solver in Session.h is written in.
// Branching for the integer search is a planned extra rule and does not exist
// yet; a checker that met one would have to refuse it.
//
//===----------------------------------------------------------------------===//

#ifndef SYSTOLIC_EXACT_CERTIFICATE_H
#define SYSTOLIC_EXACT_CERTIFICATE_H

#include "Systolic/Exact/Rational.h"

#include <cstddef>

namespace systolic::exact {

struct IntegerWitness {
  std::vector<Int> values;
};

struct RationalWitness {
  std::vector<Rational> values;
};

/// A row of the system, by kind and index in that kind's list.
struct RowRef {
  enum class Kind { Inequality, Equality, Congruence, StrictInequality };
  Kind kind = Kind::Inequality;
  std::size_t index = 0;
  /// Use the row with its sign flipped. Refused for an inequality or a strict
  /// inequality, whose negation does not follow from it.
  bool negated = false;
};

struct ProofTerm {
  std::size_t step = 0;
  Int multiplier{1}; // > 0
};

struct ProofStep {
  enum class Rule { Input, Combine, Tighten };

  static ProofStep input(RowRef row);
  static ProofStep combine(std::vector<ProofTerm> terms);
  static ProofStep tighten(std::size_t source);

  Rule rule = Rule::Input;
  RowRef row;                   // Input
  std::vector<ProofTerm> terms; // Combine
  std::size_t source = 0;       // Tighten
};

struct InfeasibilityProof {
  std::vector<ProofStep> steps;
};

/// A text form that identifies the proof. Equal proofs give equal text.
std::string canonicalText(const InfeasibilityProof &proof);

} // namespace systolic::exact

#endif // SYSTOLIC_EXACT_CERTIFICATE_H
