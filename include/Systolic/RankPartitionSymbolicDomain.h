#pragma once

#include "RankPartitionDomain.h"

#include <cstddef>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace systolic::logical {

// Exact symbolic image of an integer affine domain.
//
// Given:
//
//   z = T*x
//   x = T^{-1}*z
//
// Construct:
//
//   d = lcm(denominators(T^{-1}))
//   M = d*T^{-1}
//
// Then:
//
//   x = M*z/d
//
// z is legal iff:
//
//   (M*z)_i == 0 (mod d), for every i
//   A*M*z <= d*b
//
// No source-iteration enumeration is required.

struct SymbolicPartitionDomain {
  std::size_t dimension = 0;

  // Positive common denominator.
  BigInt denominator = 1;

  // M = denominator * T^{-1}.
  IntMatrix inverseNumerator;

  // Symbolically transformed inequalities.
  std::vector<AffineInequality> inequalities;
};

namespace symbolic_detail {

inline BigInt positiveMod(BigInt a, const BigInt &m) {
  BigInt r = a % m;

  if (r < 0)
    r += m;

  return r;
}

inline std::optional<BigInt> exactDot(
    const IntRow &a,
    const IntRow &b) {

  if (a.size() != b.size())
    return std::nullopt;

  BigInt result = 0;

  for (std::size_t i = 0; i < a.size(); ++i)
    result += a[i] * b[i];

  return result;
}

inline std::optional<IntRow> multiply(
    const IntMatrix &matrix, const IntRow &vector) {
  IntRow result;
  for (const auto &row : matrix) {
    auto value = exactDot(row, vector);
    if (!value) return std::nullopt;
    result.push_back(*value);
  }
  return result;
}


} // namespace symbolic_detail

// Construct symbolic integer-domain constraints.
//
// Returns nullopt for malformed coordinate maps or domains.
// All arithmetic uses arbitrary-precision integers.
inline std::optional<SymbolicPartitionDomain>
transformToSymbolicDomain(
    const PartitionCoordinateMap &map,
    const AffineIntegerDomain &sourceDomain) {

  const std::size_t n = sourceDomain.sourceDimension;

  if (map.sourceToLogical.size() != n ||
      map.logicalToSource.size() != n)
    return std::nullopt;

  for (const auto &row : map.sourceToLogical)
    if (row.size() != n)
      return std::nullopt;

  for (const auto &row : map.logicalToSource)
    if (row.size() != n)
      return std::nullopt;

  for (const auto &constraint : sourceDomain.inequalities)
    if (constraint.coefficients.size() != n)
      return std::nullopt;

  // Verify that the supplied maps are actual inverses.
  // This prevents malformed map objects from producing
  // incorrect symbolic domains.
  for (std::size_t i = 0; i < n; ++i) {
    for (std::size_t j = 0; j < n; ++j) {
      Rat value(0);

      for (std::size_t k = 0; k < n; ++k)
        value += Rat(map.sourceToLogical[i][k]) *
                 map.logicalToSource[k][j];

      if (value != Rat(i == j ? 1 : 0))
        return std::nullopt;
    }
  }

  SymbolicPartitionDomain result;
  result.dimension = n;

  // Step 1: common denominator.
  BigInt d = 1;

  for (const auto &row : map.logicalToSource)
    for (const auto &value : row)
      d = detail::lcm(d, value.denominator());

  if (d <= 0)
    return std::nullopt;

  result.denominator = d;

  // Step 2: M = d*T^{-1}.
  result.inverseNumerator.resize(
      n, IntRow(n, BigInt(0)));

  for (std::size_t i = 0; i < n; ++i) {
    for (std::size_t j = 0; j < n; ++j) {
      const Rat &value = map.logicalToSource[i][j];

      result.inverseNumerator[i][j] =
          value.numerator() * (d / value.denominator());
    }
  }

  // Step 3: A*M*z <= d*b.
  for (const auto &constraint : sourceDomain.inequalities) {

    IntRow transformed(n, BigInt(0));

    for (std::size_t j = 0; j < n; ++j) {
      for (std::size_t i = 0; i < n; ++i) {
        transformed[j] +=
            constraint.coefficients[i] *
            result.inverseNumerator[i][j];
      }
    }

    result.inequalities.push_back({
        std::move(transformed),
        d * constraint.upperBound
    });
  }

  return result;
}

// Membership in the transformed symbolic domain.
//
// This evaluates ONLY the transformed integer constraints.
// It does not call recoverSourceIteration() or
// containsLogicalPoint().
inline bool containsSymbolicPoint(
    const SymbolicPartitionDomain &domain,
    const IntRow &z) {

  if (z.size() != domain.dimension)
    return false;

  if (domain.denominator <= 0)
    return false;

  if (domain.inverseNumerator.size() != domain.dimension)
    return false;

  // Exact congruence constraints:
  //
  // M*z == 0 (mod d).
  auto numerators = symbolic_detail::multiply(domain.inverseNumerator, z);
  if (!numerators) return false;

  for (const BigInt &value : *numerators) {
    if (symbolic_detail::positiveMod(
            value, domain.denominator) != 0)
      return false;
  }

  // Exact transformed inequalities.
  for (const auto &constraint : domain.inequalities) {
    auto value = symbolic_detail::exactDot(constraint.coefficients, z);
    if (!value || *value > constraint.upperBound) return false;
  }

  return true;
}

} // namespace systolic::logical
