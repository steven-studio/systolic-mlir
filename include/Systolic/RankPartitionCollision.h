#pragma once

#include "Systolic/RankPartitionPresburgerDomain.h"

#include "llvm/ADT/SmallVector.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <vector>

namespace systolic::logical {

// Phase 5.3.2
//
// Extended symbolic write collision:
//
// C_ext = {
//   (x1,z1,x2,z2) |
//   (x1,z1) in R_D,
//   (x2,z2) in R_D,
//   F_C*x1 = F_C*x2
// }
//
// Variable order:
//
// [x1(n), z1(n), x2(n), z2(n),
//  locals1(l), locals2(l), constant]
//
// The local variables of the two source domains are
// independent.
//
// This function constructs one exact Presburger relation.
//
// No source-point enumeration.
// No output-independence requirement for Batch axes.

inline std::optional<mlir::presburger::IntegerRelation>
buildCoordinateCollisionRelation(
    const PartitionCoordinateMap &map,
    const mlir::presburger::IntegerPolyhedron &sourceDomain,
    const std::vector<std::vector<BigInt>> &sourceOutputCoefficients,
    const std::vector<BigInt> &sourceOutputOffsets) {

  using namespace mlir::presburger;

  const std::size_t n = map.sourceToLogical.size();

  if (n == 0 ||
      n > std::numeric_limits<unsigned>::max() / 4)
    return std::nullopt;

  // The same C access function is used for both iterations.
  // Its constant offset therefore cancels in the equality.

  if (sourceOutputCoefficients.size() !=
      sourceOutputOffsets.size())
    return std::nullopt;

  const BigInt maxCoefficient(
      std::numeric_limits<std::int64_t>::max());

  // This implementation requires signed int64-compatible
  // coefficients for the new collision equalities.
  //
  // Both +F_C and -F_C must be representable.

  for (const auto &row : sourceOutputCoefficients) {
    if (row.size() != n)
      return std::nullopt;

    for (const BigInt &coefficient : row) {
      if (coefficient > maxCoefficient ||
          coefficient < -maxCoefficient)
        return std::nullopt;
    }
  }

  auto sourceRelation =
      buildCoordinateRelationOnDomain(map, sourceDomain);

  if (!sourceRelation)
    return std::nullopt;

  if (sourceRelation->getNumDomainVars() != n ||
      sourceRelation->getNumRangeVars() != n ||
      sourceRelation->getNumSymbolVars() != 0)
    return std::nullopt;

  const std::size_t locals =
      sourceRelation->getNumLocalVars();

  if (locals >
      (std::numeric_limits<unsigned>::max() - 4 * n) / 2)
    return std::nullopt;

  const std::size_t total = 4 * n + 2 * locals + 1;

  // Domain: (x1,z1)
  // Range:  (x2,z2)
  // Locals: (q1,q2)

  IntegerRelation collision(
      PresburgerSpace::getRelationSpace(
          static_cast<unsigned>(2 * n),
          static_cast<unsigned>(2 * n),
          0,
          static_cast<unsigned>(2 * locals)));

  // Copy a source relation row into either the first
  // or the second independent iteration.
  //
  // Source row:
  //   [x(n), z(n), locals(l), constant]
  //
  // Collision row:
  //   [x1(n), z1(n), x2(n), z2(n),
  //    locals1(l), locals2(l), constant]

  auto embedRow = [&](auto original, bool second) {
    llvm::SmallVector<llvm::DynamicAPInt> row(
        total, llvm::DynamicAPInt(0));

    const std::size_t iterationOffset =
        second ? 2 * n : 0;

    const std::size_t localOffset =
        second ? 4 * n + locals : 4 * n;

    for (std::size_t i = 0; i < 2 * n; ++i)
      row[iterationOffset + i] = original[i];

    for (std::size_t i = 0; i < locals; ++i)
      row[localOffset + i] = original[2 * n + i];

    row.back() = original.back();

    return row;
  };

  // Duplicate the complete source-to-logical relation.
  // Each copy receives independent local variables.

  for (bool second : {false, true}) {

    for (unsigned i = 0;
         i < sourceRelation->getNumEqualities();
         ++i) {

      auto original = sourceRelation->getEquality(i);

      if (original.size() != 2 * n + locals + 1)
        return std::nullopt;

      collision.addEquality(embedRow(original, second));
    }

    for (unsigned i = 0;
         i < sourceRelation->getNumInequalities();
         ++i) {

      auto original = sourceRelation->getInequality(i);

      if (original.size() != 2 * n + locals + 1)
        return std::nullopt;

      collision.addInequality(embedRow(original, second));
    }
  }

  // For each output-address dimension:
  //
  //   F_C[d] * x1 - F_C[d] * x2 = 0
  //
  // Constant offsets cancel.

  for (const auto &sourceRow : sourceOutputCoefficients) {

    llvm::SmallVector<llvm::DynamicAPInt> equality(
        total, llvm::DynamicAPInt(0));

    for (std::size_t i = 0; i < n; ++i) {

      const std::int64_t coefficient =
          sourceRow[i].convert_to<std::int64_t>();

      equality[i] =
          llvm::DynamicAPInt(coefficient);

      equality[2 * n + i] =
          llvm::DynamicAPInt(-coefficient);
    }

    collision.addEquality(equality);
  }

  return collision;
}

} // namespace systolic::logical
