#pragma once

#include "Systolic/RankPartitionPresburger.h"

#include <cstddef>
#include <optional>
#include <vector>

namespace systolic::logical {

// Phase 5.3.1
//
// Source output access:
//
//   C_address(x) = F_C * x + c_C
//
// Coordinate transformation:
//
//   z = T * x
//
// Symbolic logical output access:
//
//   W(z) = F_C * T^{-1} * z + c_C
//
// Coefficients are exact rational numbers.
//
// Integer legality is enforced by the existing
// source-to-logical Presburger relation.
//
// This structure does NOT assume that Batch axes are
// independent output dimensions.

struct SymbolicWriteAddress {
  std::vector<std::vector<Rat>> coefficients;
  std::vector<Rat> offsets;
};

inline std::optional<SymbolicWriteAddress>
buildSymbolicWriteAddress(
    const PartitionCoordinateMap &map,
    const std::vector<std::vector<BigInt>> &sourceCoefficients,
    const std::vector<BigInt> &sourceOffsets) {

  const std::size_t n = map.sourceToLogical.size();

  // Reuse the Phase 5.2.1 validation of the
  // coordinate map and its inverse.
  if (!buildCoordinateRelation(map))
    return std::nullopt;

  if (sourceCoefficients.size() != sourceOffsets.size())
    return std::nullopt;

  if (map.logicalToSource.size() != n)
    return std::nullopt;

  for (const auto &row : map.logicalToSource) {
    if (row.size() != n)
      return std::nullopt;
  }

  SymbolicWriteAddress result;

  result.coefficients.reserve(sourceCoefficients.size());
  result.offsets.reserve(sourceOffsets.size());

  for (std::size_t address = 0;
       address < sourceCoefficients.size();
       ++address) {

    const auto &sourceRow = sourceCoefficients[address];

    if (sourceRow.size() != n)
      return std::nullopt;

    std::vector<Rat> logicalRow(n, Rat(0));

    // G[address][j]
    //
    //   = sum_i F_C[address][i] * T_inverse[i][j]

    for (std::size_t j = 0; j < n; ++j) {
      Rat sum(0);

      for (std::size_t i = 0; i < n; ++i) {
        sum += Rat(sourceRow[i]) *
               map.logicalToSource[i][j];
      }

      logicalRow[j] = sum;
    }

    result.coefficients.push_back(std::move(logicalRow));
    result.offsets.emplace_back(sourceOffsets[address]);
  }

  return result;
}

} // namespace systolic::logical
