#pragma once

#include "RankPartitionExpansion.h"

#include <cstddef>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace systolic::logical {

struct PartitionCoordinateMap {
  // Integer matrix: z = T*x.
  IntMatrix sourceToLogical;

  // Exact rational matrix: x = T^{-1}*z.
  RatMatrix logicalToSource;
};

struct AffineInequality {
  // coefficients * x <= upperBound
  IntRow coefficients;
  BigInt upperBound;
};

struct AffineIntegerDomain {
  std::size_t sourceDimension = 0;
  std::vector<AffineInequality> inequalities;
};

namespace domain_detail {

inline std::optional<RatMatrix>
invertIntegerMatrix(const IntMatrix &input) {

  const std::size_t n = input.size();

  RatMatrix augmented(
      n, RatRow(2 * n, Rat(0)));

  for (std::size_t i = 0; i < n; ++i) {
    if (input[i].size() != n)
      return std::nullopt;

    for (std::size_t j = 0; j < n; ++j)
      augmented[i][j] = Rat(input[i][j]);

    augmented[i][n + i] = Rat(1);
  }

  // Exact Gauss-Jordan elimination over Q.
  for (std::size_t col = 0; col < n; ++col) {

    std::size_t pivotRow = col;

    while (pivotRow < n &&
           augmented[pivotRow][col] == 0)
      ++pivotRow;

    if (pivotRow == n)
      return std::nullopt;

    std::swap(
        augmented[col],
        augmented[pivotRow]);

    const Rat pivot = augmented[col][col];

    for (std::size_t j = 0; j < 2 * n; ++j)
      augmented[col][j] /= pivot;

    for (std::size_t row = 0; row < n; ++row) {
      if (row == col)
        continue;

      const Rat factor = augmented[row][col];

      if (factor == 0)
        continue;

      for (std::size_t j = 0; j < 2 * n; ++j)
        augmented[row][j] -=
            factor * augmented[col][j];
    }
  }

  RatMatrix inverse(
      n, RatRow(n, Rat(0)));

  for (std::size_t i = 0; i < n; ++i)
    for (std::size_t j = 0; j < n; ++j)
      inverse[i][j] = augmented[i][n + j];

  return inverse;
}

inline IntMatrix
collectCoordinateRows(
    const ExpandedRankPartition &partition) {

  IntMatrix rows;

  for (const auto &row : partition.i)
    rows.push_back(row);

  for (const auto &row : partition.j)
    rows.push_back(row);

  for (const auto &row : partition.k)
    rows.push_back(row);

  for (const auto &axis : partition.partitions)
    rows.push_back(axis.basis);

  return rows;
}

} // namespace domain_detail

inline std::optional<PartitionCoordinateMap>
buildPartitionCoordinateMap(
    const ExpandedRankPartition &partition) {

  PartitionCoordinateMap result;

  result.sourceToLogical =
      domain_detail::collectCoordinateRows(partition);

  if (result.sourceToLogical.size() !=
      partition.sourceRank)
    return std::nullopt;

  for (const auto &row : result.sourceToLogical)
    if (row.size() != partition.sourceRank)
      return std::nullopt;

  auto inverse =
      domain_detail::invertIntegerMatrix(
          result.sourceToLogical);

  if (!inverse)
    return std::nullopt;

  result.logicalToSource = std::move(*inverse);

  return result;
}

// Evaluate the forward integer coordinate map z = T*x.
inline std::optional<IntRow>
mapSourceIteration(
    const PartitionCoordinateMap &map,
    const IntRow &source) {

  const std::size_t n =
      map.sourceToLogical.size();

  if (source.size() != n)
    return std::nullopt;

  IntRow logical(n, BigInt(0));

  for (std::size_t i = 0; i < n; ++i) {
    if (map.sourceToLogical[i].size() != n)
      return std::nullopt;

    for (std::size_t j = 0; j < n; ++j)
      logical[i] +=
          map.sourceToLogical[i][j] * source[j];
  }

  return logical;
}

// Evaluate x = T^{-1}*z.
//
// Return nullopt if x has any non-integral coordinate.
inline std::optional<IntRow>
recoverSourceIteration(
    const PartitionCoordinateMap &map,
    const IntRow &logical) {

  const std::size_t n =
      map.logicalToSource.size();

  if (logical.size() != n)
    return std::nullopt;

  IntRow source(n, BigInt(0));

  for (std::size_t i = 0; i < n; ++i) {
    if (map.logicalToSource[i].size() != n)
      return std::nullopt;

    Rat value(0);

    for (std::size_t j = 0; j < n; ++j)
      value +=
          map.logicalToSource[i][j] * Rat(logical[j]);

    if (value.denominator() != 1)
      return std::nullopt;

    source[i] = value.numerator();
  }

  return source;
}

inline bool
containsSourcePoint(
    const AffineIntegerDomain &domain,
    const IntRow &source) {

  if (source.size() != domain.sourceDimension)
    return false;

  for (const auto &constraint : domain.inequalities) {

    if (constraint.coefficients.size() !=
        domain.sourceDimension)
      return false;

    BigInt value = 0;

    for (std::size_t i = 0;
         i < domain.sourceDimension; ++i)
      value += constraint.coefficients[i] * source[i];

    if (value > constraint.upperBound)
      return false;
  }

  return true;
}

// Exact membership predicate for transformed domain:
//
// z belongs to D' iff T^{-1} z is integer and belongs to D.
inline bool
containsLogicalPoint(
    const PartitionCoordinateMap &map,
    const AffineIntegerDomain &domain,
    const IntRow &logical) {

  if (map.sourceToLogical.size() !=
      domain.sourceDimension)
    return false;

  const auto source =
      recoverSourceIteration(map, logical);

  if (!source)
    return false;

  // Defensive forward-map check.
  const auto reconstructed =
      mapSourceIteration(map, *source);

  if (!reconstructed || *reconstructed != logical)
    return false;

  return containsSourcePoint(domain, *source);
}

} // namespace systolic::logical
