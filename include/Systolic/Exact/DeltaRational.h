//===- DeltaRational.h - Rationals with an infinitesimal --------*- C++ -*-===//
//
// A value r + k * delta, where delta stands for an arbitrarily small positive
// rational. It is how a strict bound enters a tableau that only knows closed
// bounds: x > c becomes x >= c + delta, and x < c becomes x <= c - delta
// (Dutertre and de Moura, 2006, section 3.2).
//
// The order is lexicographic on (r, k). That is the order of the values for
// every small enough delta, so all the reasoning of the closed case carries
// over: these values form an ordered vector space over the rationals.
//
// Nothing here is approximate. A concrete value appears only at the end, when
// Session picks a positive rational for delta and the checker tests the result
// against the original strict rows.
//
//===----------------------------------------------------------------------===//

#ifndef SYSTOLIC_EXACT_DELTARATIONAL_H
#define SYSTOLIC_EXACT_DELTARATIONAL_H

#include "Systolic/Exact/Rational.h"

namespace systolic::exact {

class DeltaRational {
public:
  /// Zero.
  DeltaRational() = default;
  explicit DeltaRational(Rational real) : realPart(std::move(real)) {}
  DeltaRational(Rational real, Rational delta)
      : realPart(std::move(real)), deltaPart(std::move(delta)) {}

  const Rational &real() const { return realPart; }
  const Rational &delta() const { return deltaPart; }

  DeltaRational operator+(const DeltaRational &other) const;
  DeltaRational operator-(const DeltaRational &other) const;
  DeltaRational operator*(const Rational &factor) const;

  bool operator==(const DeltaRational &other) const;
  bool operator!=(const DeltaRational &other) const {
    return !(*this == other);
  }
  bool operator<(const DeltaRational &other) const;
  bool operator>(const DeltaRational &other) const { return other < *this; }
  bool operator<=(const DeltaRational &other) const {
    return !(other < *this);
  }
  bool operator>=(const DeltaRational &other) const {
    return !(*this < other);
  }

  /// The value with `epsilon` in place of delta.
  Rational at(const Rational &epsilon) const;

  /// "r" when the delta part is zero, otherwise "r+k*d".
  std::string toString() const;

private:
  Rational realPart;
  Rational deltaPart;
};

} // namespace systolic::exact

#endif // SYSTOLIC_EXACT_DELTARATIONAL_H
