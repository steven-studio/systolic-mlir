#pragma once

#include "Systolic/RankPartitionOrderedDependence.h"

#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

namespace systolic::logical {

// Phase 5.3.4: exact emptiness of ordered write-collision branches.
//
// A nonempty branch proves existence of two distinct, originally ordered
// source iterations that update the same C address. It does not by itself
// classify the update operator or prove that a transformation is legal.
//
// Important assumptions inherited from Phase 5.3.3:
//   * ascending sequential lexicographic source-IV execution;
//   * a single affine source domain with no unresolved symbols;
//   * both accesses use the same output access function.
//
// `isIntegerEmpty()` considers integer feasibility including existential
// local variables. Do NOT substitute the weaker rational/polyhedral test.
struct OrderedWriteConflictAnalysis {
  // Only feasible branches are retained; all original variables and local
  // dimensions remain in the relations.
  std::vector<mlir::presburger::IntegerRelation> feasibleBranches;
  std::size_t testedBranches = 0;
  std::size_t emptyBranches = 0;

  bool hasOrderedWriteConflict() const { return !feasibleBranches.empty(); }
};

inline std::optional<OrderedWriteConflictAnalysis>
analyzeOrderedWriteConflicts(
    const PartitionCoordinateMap &map,
    const mlir::presburger::IntegerPolyhedron &sourceDomain,
    const std::vector<std::vector<BigInt>> &sourceOutputCoefficients,
    const std::vector<BigInt> &sourceOutputOffsets) {
  auto branches = buildOrderedWriteDependenceBranches(
      map, sourceDomain, sourceOutputCoefficients, sourceOutputOffsets);
  if (!branches)
    return std::nullopt;

  OrderedWriteConflictAnalysis result;
  result.testedBranches = branches->size();
  result.feasibleBranches.reserve(branches->size());
  for (auto &branch : *branches) {
    if (branch.isIntegerEmpty()) {
      ++result.emptyBranches;
      continue;
    }
    result.feasibleBranches.push_back(std::move(branch));
  }
  return result;
}

// Conservative policy only; this does not identify a reduction pattern.
// If writes collide, caller MUST preserve original write order unless a
// later, separate operator-specific legality proof grants more freedom.
// This deliberately does NOT infer parallel safety from Batch classification.
enum class WriteUpdateKind {
  Assign,
  IntegerAdd,
  FloatingAdd,
  Unknown
};

struct ConservativeWritePolicy {
  bool mayReorderWrites;
  bool mustPreserveOriginalOrder;
  bool requiresReductionProof;
};

inline ConservativeWritePolicy
classifyWriteExecution(const OrderedWriteConflictAnalysis &analysis,
                       WriteUpdateKind kind) {
  if (!analysis.hasOrderedWriteConflict())
    return {/*mayReorderWrites=*/true,
            /*mustPreserveOriginalOrder=*/false,
            /*requiresReductionProof=*/false};

  const bool potentialReduction =
      kind == WriteUpdateKind::IntegerAdd ||
      kind == WriteUpdateKind::FloatingAdd;
  return {/*mayReorderWrites=*/false,
          /*mustPreserveOriginalOrder=*/true,
          /*requiresReductionProof=*/potentialReduction};
}

} // namespace systolic::logical
