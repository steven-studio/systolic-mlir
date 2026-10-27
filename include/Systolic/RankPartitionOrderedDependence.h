#pragma once

#include "Systolic/RankPartitionCollision.h"

#include "llvm/ADT/SmallVector.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace systolic::logical {

// Phase 5.3.3: ordered write-collision candidates.
//
// Assumption: source IVs x[0],...,x[n-1] represent the actual
// sequential, ascending lexicographic execution order.
//
// D_ordered = C_ext intersect {x1 <_lex x2}.
//
// A strict lexicographic relation is a DISJUNCTION of n branches:
//   branch t:
//     x1[0..t-1] == x2[0..t-1]
//     x1[t] + 1 <= x2[t].
//
// Each IntegerRelation is one conjunction. Return a vector of
// branches representing the disjunction, without enumerating points.
// The same-iteration pair is excluded by strict inequality.
//
// Layout inherited from Phase 5.3.2:
//   [x1(n), z1(n), x2(n), z2(n), locals1(l), locals2(l), constant]
//
// This describes potential ordered conflicts; it does NOT prove
// that reassociation, parallel reduction, or overwrite lowering is legal.
inline std::optional<std::vector<mlir::presburger::IntegerRelation>>
buildOrderedWriteDependenceBranches(
    const PartitionCoordinateMap &map,
    const mlir::presburger::IntegerPolyhedron &sourceDomain,
    const std::vector<std::vector<BigInt>> &sourceOutputCoefficients,
    const std::vector<BigInt> &sourceOutputOffsets) {

  const std::size_t n = map.sourceToLogical.size();
  if (n == 0)
    return std::nullopt;

  auto collision = buildCoordinateCollisionRelation(
      map, sourceDomain, sourceOutputCoefficients, sourceOutputOffsets);
  if (!collision)
    return std::nullopt;

  const std::size_t locals = collision->getNumLocalVars();
  const std::size_t columns = 4 * n + locals + 1;

  std::vector<mlir::presburger::IntegerRelation> branches;
  branches.reserve(n);

  for (std::size_t t = 0; t < n; ++t) {
    // Copy the COMPLETE collision relation: both source domains,
    // distinct existential locals, and equality of output addresses.
    auto branch = *collision;

    // Equal source IV coordinates before the first difference.
    for (std::size_t j = 0; j < t; ++j) {
      llvm::SmallVector<llvm::DynamicAPInt> row(
          columns, llvm::DynamicAPInt(0));
      row[j] = llvm::DynamicAPInt(1);
      row[2 * n + j] = llvm::DynamicAPInt(-1);
      branch.addEquality(row);
    }

    // x1[t] < x2[t], over integers:
    // x2[t] - x1[t] - 1 >= 0.
    llvm::SmallVector<llvm::DynamicAPInt> row(
        columns, llvm::DynamicAPInt(0));
    row[t] = llvm::DynamicAPInt(-1);
    row[2 * n + t] = llvm::DynamicAPInt(1);
    row.back() = llvm::DynamicAPInt(-1);
    branch.addInequality(row);

    branches.push_back(std::move(branch));
  }

  return branches;
}

} // namespace systolic::logical
