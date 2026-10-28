#pragma once

#include "Systolic/RankPartitionPresburgerDomain.h"

#include "mlir/Analysis/Presburger/IntegerRelation.h"

#include <optional>

namespace systolic::logical {

// Phase 5.4.3a
//
// Exact integer projection:
//
//   R_D = {(x,z) | x in D, z = T*x}
//
//   Z_D = {z | exists integer x: (x,z) in R_D}
//
// Source dimensions become existential local variables.
//
// This preserves integer lattice constraints, including
// congruences induced by non-unimodular transforms.
//
// No source iteration enumeration.
// No rational relaxation.
// No bounding-box approximation.

inline std::optional<mlir::presburger::IntegerPolyhedron>
buildLogicalDomain(
    const PartitionCoordinateMap &map,
    const mlir::presburger::IntegerPolyhedron &sourceDomain) {

  auto relation =
      buildCoordinateRelationOnDomain(map, sourceDomain);

  if (!relation)
    return std::nullopt;

  const std::size_t n = map.sourceToLogical.size();

  if (relation->getNumDomainVars() != n ||
      relation->getNumRangeVars() != n ||
      relation->getNumSymbolVars() != 0)
    return std::nullopt;

  // Exact existential integer projection.
  //
  // Original relation:
  //
  //   [x, z, source-locals, constant]
  //
  // Projected set:
  //
  //   [z, existential-locals, constant]
  //
  // The x variables are existentially quantified,
  // not substituted using a rational approximation.

  auto logicalDomain = relation->getRangeSet();

  if (logicalDomain.getNumDimVars() != n ||
      logicalDomain.getNumSymbolVars() != 0)
    return std::nullopt;

  return logicalDomain;
}

} // namespace systolic::logical
