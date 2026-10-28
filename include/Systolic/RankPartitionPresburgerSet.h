#pragma once

#include "Systolic/RankPartitionLogicalDomain.h"

#include "mlir/Analysis/Presburger/PresburgerRelation.h"

#include <cstddef>
#include <limits>
#include <optional>

namespace systolic::logical {

// Phase 5.4.3b
//
// Exact Presburger union, difference and logical image.
//
// No source-point enumeration.
// No convex-hull approximation.
// No bounding-box approximation.
//
// Unsupported operations return std::nullopt.

namespace presburger_set_detail {

inline bool validSourceSet(
    const mlir::presburger::PresburgerSet &set,
    std::size_t n) {

  if (set.getNumDomainVars() != 0 ||
      set.getNumRangeVars() != n ||
      set.getNumSymbolVars() != 0)
    return false;

  return true;
}

} // namespace presburger_set_detail

// ---------------------------------------------------------
// Exact source-domain union.
//
// D = A union B
//
// Union is represented as a Presburger disjunction.
// Overlapping disjuncts do not duplicate set members.
//
// This does NOT by itself construct an exactly-once loop.
// ---------------------------------------------------------

inline std::optional<mlir::presburger::PresburgerSet>
buildSourceDomainUnion(
    const mlir::presburger::PresburgerSet &a,
    const mlir::presburger::PresburgerSet &b) {

  if (a.getNumDomainVars() != 0 ||
      b.getNumDomainVars() != 0 ||
      a.getNumRangeVars() != b.getNumRangeVars() ||
      a.getNumSymbolVars() != 0 ||
      b.getNumSymbolVars() != 0)
    return std::nullopt;

  return a.unionSet(b);
}

// ---------------------------------------------------------
// Exact source-domain difference.
//
// D = outer \ hole
//
// The MLIR subtraction implementation requires local
// variables of the subtracted set to have supported
// division representations.
//
// This conservative API accepts a local-free RHS.
//
// More general RHS existential locals are NOT silently
// approximated or discarded.
// ---------------------------------------------------------

inline std::optional<mlir::presburger::PresburgerSet>
buildSourceDomainDifference(
    const mlir::presburger::PresburgerSet &outer,
    const mlir::presburger::PresburgerSet &hole) {

  if (outer.getNumDomainVars() != 0 ||
      hole.getNumDomainVars() != 0 ||
      outer.getNumRangeVars() != hole.getNumRangeVars() ||
      outer.getNumSymbolVars() != 0 ||
      hole.getNumSymbolVars() != 0)
    return std::nullopt;

  // Fail closed for subtraction with existential locals
  // that have not been proved representable as divisions.
  //
  // In this milestone we require all RHS disjuncts
  // to be local-free.
  for (const auto &disjunct : hole.getAllDisjuncts()) {
    if (disjunct.getNumLocalVars() != 0)
      return std::nullopt;
  }

  return outer.subtract(hole);
}

// ---------------------------------------------------------
// Exact logical image of a PresburgerSet.
//
// Z_D = {z | exists x in D: z = T*x}
//
// The image of a union is the union of images:
//
// T(union_i D_i) = union_i T(D_i)
//
// This is valid even when T is non-unimodular.
//
// Each disjunct is projected with the already-tested
// Phase 5.4.3a buildLogicalDomain().
//
// No source-iteration enumeration.
// ---------------------------------------------------------

inline std::optional<mlir::presburger::PresburgerSet>
buildLogicalDomain(
    const PartitionCoordinateMap &map,
    const mlir::presburger::PresburgerSet &sourceSet) {

  using namespace mlir::presburger;

  const std::size_t n = map.sourceToLogical.size();

  if (n > std::numeric_limits<unsigned>::max())
    return std::nullopt;

  if (!presburger_set_detail::validSourceSet(
          sourceSet, n))
    return std::nullopt;

  // Validate the map even when the source set is empty.
  //
  // Build the coordinate relation independently of
  // source-domain disjuncts.
  if (!buildCoordinateRelation(map))
    return std::nullopt;

  PresburgerSet result = PresburgerSet::getEmpty(
      PresburgerSpace::getSetSpace(
          static_cast<unsigned>(n)));

  for (const auto &disjunct :
       sourceSet.getAllDisjuncts()) {

    auto polyhedron = IntegerPolyhedron(disjunct);

    auto projected =
        buildLogicalDomain(map, polyhedron);

    if (!projected)
      return std::nullopt;

    result.unionInPlace(*projected);
  }

  return result;
}

} // namespace systolic::logical
