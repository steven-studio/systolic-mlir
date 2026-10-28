#pragma once

#include "Systolic/RankPartitionSymbolicDomain.h"
#include "Systolic/RankPartitionGuardGeneration.h"

#include "mlir/Analysis/Presburger/IntegerRelation.h"
#include "mlir/Analysis/Presburger/PresburgerRelation.h"

#include "llvm/ADT/SmallVector.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>

namespace systolic::logical {

// Phase 5.4.3d-3a-2a.
//
// Normalize the exact image of an inequality-defined
// source domain under an invertible integer matrix.
//
// Given:
//
//   z = T*x
//   x = M*z/d
//
// Construct:
//
//   A*M*z <= d*b
//   M*z == 0 (mod d)
//
// For this milestone, the output must be representable
// using at most one nontrivial existential local.
//
// This is not general Presburger quantifier elimination.
//
// No point enumeration.
// No convex approximation.
// No IR emission.

namespace normalization_detail {

inline bool fitsI64(const BigInt &value) {
  return value >=
             BigInt(std::numeric_limits<std::int64_t>::min()) &&
         value <=
             BigInt(std::numeric_limits<std::int64_t>::max());
}

inline std::optional<std::int64_t>
toI64(const BigInt &value) {
  if (!fitsI64(value))
    return std::nullopt;

  return value.convert_to<std::int64_t>();
}

} // namespace normalization_detail

inline std::optional<mlir::presburger::PresburgerSet>
buildRecoverableLogicalDomain(
    const PartitionCoordinateMap &map,
    const AffineIntegerDomain &sourceDomain) {

  using namespace mlir::presburger;

  const std::size_t n = sourceDomain.sourceDimension;

  if (n == 0 ||
      n > std::numeric_limits<unsigned>::max())
    return std::nullopt;

  // Reuse existing exact rational normalization.
  //
  // transformToSymbolicDomain validates both the
  // forward coordinate matrix and its inverse.

  auto symbolic = transformToSymbolicDomain(
      map, sourceDomain);

  if (!symbolic ||
      symbolic->dimension != n ||
      symbolic->denominator <= 0 ||
      symbolic->inverseNumerator.size() != n)
    return std::nullopt;

  const BigInt &d = symbolic->denominator;

  IntegerPolyhedron poly(
      PresburgerSpace::getSetSpace(
          static_cast<unsigned>(n)));

  // --------------------------------------------------
  // Source inequalities:
  //
  //   a*x <= b
  //
  // become:
  //
  //   -(a*M)*z + d*b >= 0
  // --------------------------------------------------

  for (const auto &constraint :
       symbolic->inequalities) {

    if (constraint.coefficients.size() != n)
      return std::nullopt;

    llvm::SmallVector<std::int64_t> row(
        n + 1, 0);

    for (std::size_t j = 0; j < n; ++j) {
      auto coefficient =
          normalization_detail::toI64(
              -constraint.coefficients[j]);

      if (!coefficient)
        return std::nullopt;

      row[j] = *coefficient;
    }

    auto constant =
        normalization_detail::toI64(
            constraint.upperBound);

    if (!constant)
      return std::nullopt;

    row[n] = *constant;

    poly.addInequality(row);
  }

  // --------------------------------------------------
  // Integer lattice constraints:
  //
  //   (M*z)_i == 0 (mod d)
  //
  // If every coefficient in a row is divisible by d,
  // the congruence is automatic.
  //
  // Otherwise introduce:
  //
  //   (M*z)_i - d*q == 0
  //
  // where q is an existential integer local.
  //
  // The current guard emitter supports a limited
  // one-local subset. Reject more complex cases.
  // --------------------------------------------------

  std::optional<std::size_t> congruenceRow;

  for (std::size_t i = 0; i < n; ++i) {

    const auto &row = symbolic->inverseNumerator[i];

    if (row.size() != n)
      return std::nullopt;

    bool automatic = true;

    for (const BigInt &coefficient : row) {
      if (coefficient % d != 0) {
        automatic = false;
        break;
      }
    }

    if (!automatic) {
      if (congruenceRow)
        return std::nullopt;

      congruenceRow = i;
    }
  }

  if (congruenceRow) {

    auto denominator =
        normalization_detail::toI64(d);

    if (!denominator ||
        *denominator <= 0)
      return std::nullopt;

    // A negative local coefficient must fit i64.
    auto negatedDenominator =
        normalization_detail::toI64(-d);

    if (!negatedDenominator)
      return std::nullopt;

    poly.appendVar(VarKind::Local);

    llvm::SmallVector<std::int64_t> equality(
        n + 2, 0);

    const auto &numerator =
        symbolic->inverseNumerator[*congruenceRow];

    for (std::size_t j = 0; j < n; ++j) {
      auto coefficient =
          normalization_detail::toI64(numerator[j]);

      if (!coefficient)
        return std::nullopt;

      equality[j] = *coefficient;
    }

    equality[n] = *negatedDenominator;
    equality[n + 1] = 0;

    poly.addEquality(equality);
  }

  return PresburgerSet(poly);
}

} // namespace systolic::logical
