#pragma once

#include "Systolic/RankPartitionDomain.h"
#include "mlir/Analysis/Presburger/IntegerRelation.h"
#include "mlir/Analysis/Presburger/PresburgerSpace.h"

#include "llvm/ADT/SmallVector.h"

#include <cstdint>
#include <limits>
#include <optional>

namespace systolic::logical {

// Coordinates are [x_0 ... x_(n-1), z_0 ... z_(n-1), constant].
// The graph has exactly n equalities: z_i - sum_j T_ij*x_j == 0.
// No point enumeration or source-domain projection occurs here.
inline std::optional<mlir::presburger::IntegerRelation>
buildCoordinateRelation(const PartitionCoordinateMap &map) {
  const std::size_t n = map.sourceToLogical.size();

  // Preserve the dimension in MLIR's unsigned space and avoid size arithmetic
  // overflow before allocating rows.
  if (n > std::numeric_limits<unsigned>::max() ||
      n > (std::numeric_limits<std::size_t>::max() - 1) / 2)
    return std::nullopt;
  if (map.logicalToSource.size() != n)
    return std::nullopt;
  for (const auto &row : map.sourceToLogical)
    if (row.size() != n)
      return std::nullopt;
  for (const auto &row : map.logicalToSource)
    if (row.size() != n)
      return std::nullopt;

  // Check that the supplied inverse really is an inverse (over Q).
  // In particular, reject matrices constructed by hand with stale inverse.
  for (std::size_t i = 0; i < n; ++i)
    for (std::size_t j = 0; j < n; ++j) {
      Rat sum(0);
      for (std::size_t k = 0; k < n; ++k)
        sum += Rat(map.sourceToLogical[i][k]) * map.logicalToSource[k][j];
      if (sum != Rat(i == j ? 1 : 0))
        return std::nullopt;
    }

  const BigInt minCoefficient(std::numeric_limits<std::int64_t>::min());
  const BigInt maxCoefficient(std::numeric_limits<std::int64_t>::max());
  // Validate *all* coefficients before constructing the MLIR relation.
  for (const auto &row : map.sourceToLogical)
    for (const BigInt &value : row)
      if (value < minCoefficient || value > maxCoefficient)
        return std::nullopt;

  auto space = mlir::presburger::PresburgerSpace::getRelationSpace(
      static_cast<unsigned>(n), static_cast<unsigned>(n));
  mlir::presburger::IntegerRelation relation(space);
  for (std::size_t i = 0; i < n; ++i) {
    llvm::SmallVector<std::int64_t> eq(2 * n + 1, 0);
    for (std::size_t j = 0; j < n; ++j) {
      // Negating INT64_MIN would overflow; reject instead of constructing
      // an invalid 64-bit constraint. This does not affect mathematical ranks.
      if (map.sourceToLogical[i][j] == minCoefficient)
        return std::nullopt;
      eq[j] = (-map.sourceToLogical[i][j]).convert_to<std::int64_t>();
    }
    eq[n + i] = 1;
    relation.addEquality(eq);
  }
  return relation;
}

} // namespace systolic::logical
