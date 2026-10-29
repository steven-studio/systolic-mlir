#pragma once

#include "Systolic/PolynomialSummation.h"

namespace systolic::polynomial {

// P2a: sparse parameter monomials with P0 binomial-basis polynomials in t.
// No IR semantics are implied by this rational algebra.
class SymbolicPolynomial {
public:
  using Monomial = std::map<std::string, unsigned>;
  using Terms = std::map<Monomial, Polynomial>;
  using Assignment = std::map<std::string, Integer>;
  using Ranges = std::map<std::string, std::pair<Integer, Integer>>;
  static constexpr unsigned MaxParameters = 16, MaxParameterDegree = 16;
  static constexpr unsigned MaxTerms = 256, MaxCoefficientBits = 16384;

  SymbolicPolynomial() = default;
  explicit SymbolicPolynomial(const Polynomial &coefficient);
  static SymbolicPolynomial constant(const Rational &value);
  static SymbolicPolynomial parameter(const std::string &name);
  const Terms &terms() const { return data; }
  bool isZero() const { return data.empty(); }
  bool isIntegerValued() const;
  int iterationDegree() const;
  SymbolicPolynomial operator+(const SymbolicPolynomial &other) const;
  SymbolicPolynomial operator-(const SymbolicPolynomial &other) const;
  SymbolicPolynomial operator*(const SymbolicPolynomial &other) const;
  SymbolicPolynomial scale(const Rational &factor) const;
  SymbolicPolynomial shift(const Integer &offset) const;
  SymbolicPolynomial difference() const;
  SymbolicPolynomial integrate() const; // zero initial polynomial
  SymbolicPolynomial atIteration(const Integer &iteration) const;
  Polynomial substitute(const Assignment &values) const;
  Rational evaluate(const Integer &iteration, const Assignment &values) const;
  // Sound rectangular enclosure, t in [0,last], parameters in closed ranges.
  std::pair<Rational, Rational> bounds(const Integer &last, const Ranges &ranges) const;
  bool operator==(const SymbolicPolynomial &other) const { return data == other.data; }
  bool operator!=(const SymbolicPolynomial &other) const { return !(*this == other); }

private:
  explicit SymbolicPolynomial(Terms terms);
  void normalize();
  Terms data;
};

// Bounded substitution of the existing immutable P0 expression DAG.
SymbolicPolynomial substituteSymbolic(
    const Expression &expression, const std::map<std::string, SymbolicPolynomial> &values);

} // namespace systolic::polynomial
