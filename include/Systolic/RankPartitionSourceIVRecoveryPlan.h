#pragma once

#include "Systolic/RankPartitionSymbolicDomain.h"
#include "Systolic/RankPartitionIterationPlan.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <vector>

namespace systolic::logical {

// Phase 5.4.3d-3a-1:
//
// Preflight for source IV recovery:
//
//   x = (M * z) / d
//
// Reuses the exact rational inverse already implemented
// in RankPartitionSymbolicDomain.h.
//
// All interval computations use arbitrary precision.
//
// This plan certifies arithmetic safety over the supplied
// rectangular bounds.
//
// It does NOT certify that a caller-supplied logical domain
// is the exact image of a particular source domain.
//
// It does NOT emit MLIR operations.

struct SourceIVRecoveryPlan {
  std::size_t dimension = 0;

  std::int64_t denominator = 1;

  std::vector<std::vector<std::int64_t>>
      inverseNumerator;
};

namespace source_iv_recovery_detail {

inline bool fitsI64(const BigInt &value) {
  static const BigInt minValue(
      std::numeric_limits<std::int64_t>::min());

  static const BigInt maxValue(
      std::numeric_limits<std::int64_t>::max());

  return value >= minValue && value <= maxValue;
}

} // namespace source_iv_recovery_detail

inline std::optional<SourceIVRecoveryPlan>
buildSourceIVRecoveryPlan(
    const PartitionCoordinateMap &map,
    const std::vector<ClosedIntegerInterval> &bounds) {

  const std::size_t n = bounds.size();

  if (n == 0 ||
      map.sourceToLogical.size() != n ||
      map.logicalToSource.size() != n)
    return std::nullopt;

  // Use the existing symbolic transformation to
  // construct the exact inverse numerator and denominator.
  //
  // Empty inequalities are intentional: this phase
  // validates the coordinate recovery arithmetic,
  // not the source-domain membership.

  AffineIntegerDomain unconstrainedSource;
  unconstrainedSource.sourceDimension = n;

  auto symbolic = transformToSymbolicDomain(
      map, unconstrainedSource);

  if (!symbolic ||
      symbolic->dimension != n ||
      symbolic->denominator <= 0 ||
      symbolic->inverseNumerator.size() != n)
    return std::nullopt;

  if (!source_iv_recovery_detail::fitsI64(
          symbolic->denominator))
    return std::nullopt;

  SourceIVRecoveryPlan result;

  result.dimension = n;
  result.denominator =
      symbolic->denominator.convert_to<std::int64_t>();

  result.inverseNumerator.resize(n);

  const BigInt minI64(
      std::numeric_limits<std::int64_t>::min());

  const BigInt maxI64(
      std::numeric_limits<std::int64_t>::max());

  for (const auto &bound : bounds) {
    if (bound.lower > bound.upper)
      return std::nullopt;

    // Match the currently supported affine loop
    // emission bounds.
    if (bound.lower ==
            std::numeric_limits<std::int64_t>::min() ||
        bound.upper ==
            std::numeric_limits<std::int64_t>::max())
      return std::nullopt;
  }

  for (std::size_t i = 0; i < n; ++i) {

    const auto &row =
        symbolic->inverseNumerator[i];

    if (row.size() != n)
      return std::nullopt;

    // Interval of the running numerator.
    //
    // Initial numerator is zero.

    BigInt cumulativeLower = 0;
    BigInt cumulativeUpper = 0;

    for (std::size_t j = 0; j < n; ++j) {

      const BigInt coefficient = row[j];

      if (!source_iv_recovery_detail::fitsI64(
              coefficient))
        return std::nullopt;

      result.inverseNumerator[i].push_back(
          coefficient.convert_to<std::int64_t>());

      const BigInt lower = bounds[j].lower;
      const BigInt upper = bounds[j].upper;

      const BigInt productA =
          coefficient * lower;

      const BigInt productB =
          coefficient * upper;

      const BigInt termLower =
          productA < productB ? productA : productB;

      const BigInt termUpper =
          productA > productB ? productA : productB;

      // Verify the multiplication itself.
      if (termLower < minI64 ||
          termUpper > maxI64)
        return std::nullopt;

      // Verify the intermediate addition.
      cumulativeLower += termLower;
      cumulativeUpper += termUpper;

      if (cumulativeLower < minI64 ||
          cumulativeUpper > maxI64)
        return std::nullopt;
    }

    // The denominator is positive.
    //
    // Exact divisibility is a separate semantic
    // obligation established by the logical domain.

    if (result.denominator <= 0)
      return std::nullopt;
  }

  return result;
}

} // namespace systolic::logical
