//===- Rational.h - Canonical exact rationals -------------------*- C++ -*-===//
//
// A Rational is always in lowest terms with a positive denominator, and zero
// is 0/1, so two rationals are equal exactly when their numerators and
// denominators are. The invariant is established by the only constructors and
// no operation can break it.
//
//===----------------------------------------------------------------------===//

#ifndef SYSTOLIC_EXACT_RATIONAL_H
#define SYSTOLIC_EXACT_RATIONAL_H

#include "Systolic/Exact/Int.h"

namespace systolic::exact {

class Rational {
public:
  /// Zero.
  Rational() = default;
  /// An integer.
  explicit Rational(Int integer) : num(std::move(integer)) {}

  /// numerator / denominator in canonical form. std::nullopt when the
  /// denominator is zero.
  static std::optional<Rational> of(const Int &numerator,
                                    const Int &denominator);

  const Int &numerator() const { return num; }
  /// Always positive.
  const Int &denominator() const { return den; }

  bool isInteger() const { return den == Int(1); }
  /// -1, 0 or 1.
  int sign() const;
  /// The largest integer not above this value.
  Int floor() const;
  /// The smallest integer not below this value.
  Int ceil() const;

  Rational operator-() const;
  Rational operator+(const Rational &other) const;
  Rational operator-(const Rational &other) const;
  Rational operator*(const Rational &other) const;
  /// std::nullopt when the divisor is zero.
  std::optional<Rational> dividedBy(const Rational &divisor) const;

  bool operator==(const Rational &other) const;
  bool operator!=(const Rational &other) const { return !(*this == other); }
  bool operator<(const Rational &other) const;
  bool operator>(const Rational &other) const { return other < *this; }
  bool operator<=(const Rational &other) const { return !(other < *this); }
  bool operator>=(const Rational &other) const { return !(*this < other); }

  /// "n" for an integer, otherwise "n/d".
  std::string toString() const;

private:
  Int num{0};
  Int den{1};
};

} // namespace systolic::exact

#endif // SYSTOLIC_EXACT_RATIONAL_H
