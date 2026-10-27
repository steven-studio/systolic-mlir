#pragma once

#include "RankPartitionPresburger.h"

#include "mlir/Analysis/Presburger/IntegerRelation.h"

#include "llvm/ADT/SmallVector.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <optional>

namespace systolic::logical {

// Phase 5.2.2
//
// Construct:
//
//   R_D = {(x,z) | x in D, z = T*x}
//
// Source domain:
//   [x0 ... x(n-1), q0 ... q(l-1), constant]
//
// Result relation:
//   [x0 ... x(n-1),
//    z0 ... z(n-1),
//    q0 ... q(l-1),
//    constant]
//
// Local variables remain existentially quantified.
//
// No source-point enumeration.

inline std::optional<mlir::presburger::IntegerRelation>
buildCoordinateRelationOnDomain(
    const PartitionCoordinateMap &map,
    const mlir::presburger::IntegerPolyhedron &sourceDomain) {

  using namespace mlir::presburger;

  const std::size_t n = map.sourceToLogical.size();

  if (n > std::numeric_limits<unsigned>::max() / 2)
    return std::nullopt;

  // This phase accepts a closed source universe.
  // Symbolic parameters will be handled separately.
  if (sourceDomain.getNumSymbolVars() != 0)
    return std::nullopt;

  if (sourceDomain.getNumDimVars() != n)
    return std::nullopt;

  const std::size_t locals = sourceDomain.getNumLocalVars();

  if (locals > std::numeric_limits<unsigned>::max() - 2 * n)
    return std::nullopt;

  // Phase 5.2.1 already validates T and its inverse,
  // and checks coefficient representability.
  auto graph = buildCoordinateRelation(map);

  if (!graph)
    return std::nullopt;

  const std::size_t total = 2 * n + locals + 1;

  IntegerRelation result(
      PresburgerSpace::getRelationSpace(
          static_cast<unsigned>(n),
          static_cast<unsigned>(n),
          0,
          static_cast<unsigned>(locals)));

  // --------------------------------------------------------
  // Step 1: copy z = T*x equalities.
  //
  // Original graph layout:
  //   [x, z, constant]
  //
  // New layout:
  //   [x, z, locals, constant]
  // --------------------------------------------------------

  for (unsigned i = 0; i < graph->getNumEqualities(); ++i) {

    auto original = graph->getEquality(i);

    if (original.size() != 2 * n + 1)
      return std::nullopt;

    llvm::SmallVector<llvm::DynamicAPInt> row(
        total, llvm::DynamicAPInt(0));

    for (std::size_t j = 0; j < 2 * n; ++j)
      row[j] = original[j];

    row.back() = original.back();

    result.addEquality(row);
  }

  // --------------------------------------------------------
  // Step 2: copy source-domain constraints.
  //
  // Original:
  //   [x, locals, constant]
  //
  // Destination:
  //   [x, z, locals, constant]
  //
  // Insert n zero coefficients for z.
  // --------------------------------------------------------

  auto embedSourceRow = [&](auto original)
      -> std::optional<llvm::SmallVector<llvm::DynamicAPInt>> {

    if (original.size() != n + locals + 1)
      return std::nullopt;

    llvm::SmallVector<llvm::DynamicAPInt> row(
        total, llvm::DynamicAPInt(0));

    // Source variables x.
    for (std::size_t j = 0; j < n; ++j)
      row[j] = original[j];

    // Local existential variables q.
    for (std::size_t j = 0; j < locals; ++j)
      row[2 * n + j] = original[n + j];

    // Constant.
    row.back() = original.back();

    return row;
  };

  // Source equalities.
  for (unsigned i = 0; i < sourceDomain.getNumEqualities(); ++i) {

    auto row = embedSourceRow(sourceDomain.getEquality(i));

    if (!row)
      return std::nullopt;

    result.addEquality(*row);
  }

  // Source inequalities.
  for (unsigned i = 0; i < sourceDomain.getNumInequalities(); ++i) {

    auto row = embedSourceRow(sourceDomain.getInequality(i));

    if (!row)
      return std::nullopt;

    result.addInequality(*row);
  }

  return result;
}

} // namespace systolic::logical
