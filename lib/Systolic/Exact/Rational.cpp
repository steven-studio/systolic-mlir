//===- Rational.cpp - Canonical exact rationals ---------------------------===//

#include "Systolic/Exact/Rational.h"

namespace systolic::exact {

std::optional<Rational> Rational::of(const Int &numerator,
                                     const Int &denominator) {
  if (denominator == Int(0))
    return std::nullopt;
  // The gcd is at least |denominator|'s divisor 1, so these divide exactly.
  const Int divisor = gcdOf(numerator, denominator);
  Rational result;
  result.num = numerator / divisor;
  result.den = denominator / divisor;
  if (result.den < Int(0)) {
    result.num = -result.num;
    result.den = -result.den;
  }
  return result;
}

int Rational::sign() const {
  if (num == Int(0))
    return 0;
  return num < Int(0) ? -1 : 1;
}

// A denominator is positive, so neither division can fail.
Int Rational::floor() const { return *divFloor(num, den); }
Int Rational::ceil() const { return *divCeil(num, den); }

Rational Rational::operator-() const {
  Rational result = *this;
  result.num = -result.num;
  return result;
}

Rational Rational::operator+(const Rational &other) const {
  return *of(num * other.den + other.num * den, den * other.den);
}

Rational Rational::operator-(const Rational &other) const {
  return *this + (-other);
}

Rational Rational::operator*(const Rational &other) const {
  return *of(num * other.num, den * other.den);
}

std::optional<Rational> Rational::dividedBy(const Rational &divisor) const {
  return of(num * divisor.den, den * divisor.num);
}

bool Rational::operator==(const Rational &other) const {
  return num == other.num && den == other.den;
}

bool Rational::operator<(const Rational &other) const {
  // Both denominators are positive, so cross multiplying keeps the order.
  return num * other.den < other.num * den;
}

std::string Rational::toString() const {
  if (isInteger())
    return toDecimal(num);
  return toDecimal(num) + "/" + toDecimal(den);
}

} // namespace systolic::exact
