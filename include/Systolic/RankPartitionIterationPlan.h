#pragma once

#include "Systolic/RankPartitionPresburgerSet.h"

#include "mlir/Analysis/Presburger/IntegerRelation.h"
#include "mlir/Analysis/Presburger/PresburgerRelation.h"

#include "llvm/ADT/SmallVector.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <utility>
#include <vector>

namespace systolic::logical {

// Phase 5.4.3c
//
// A symbolic exactly-once iteration plan.
//
// The candidate rectangular iteration space B is
// described by closed integer intervals.
//
// The exact logical domain Z is preserved separately.
//
// Construction requires:
//
//   Z \ B = empty
//
// Execution semantics:
//
//   for each z in B exactly once, in lexicographic order:
//     if z belongs to Z:
//       execute(z)
//
// Therefore each z in Z executes exactly once.
//
// IMPORTANT:
// This proves iteration coverage and multiplicity only.
// It does not prove memory dependence or lowering legality.
//
// No source-point enumeration.
// No convex approximation.
// No heuristic bounding-box estimation.

struct ClosedIntegerInterval {
  std::int64_t lower = 0;
  std::int64_t upper = 0;
};

struct SymbolicIterationPlan {
  mlir::presburger::PresburgerSet logicalDomain;
  std::vector<ClosedIntegerInterval> bounds;

  SymbolicIterationPlan(
      mlir::presburger::PresburgerSet domain,
      std::vector<ClosedIntegerInterval> intervals)
      : logicalDomain(std::move(domain)),
        bounds(std::move(intervals)) {}
};

inline std::optional<SymbolicIterationPlan>
buildSymbolicIterationPlan(
    const mlir::presburger::PresburgerSet &logicalDomain,
    const std::vector<ClosedIntegerInterval> &bounds) {

  using namespace mlir::presburger;

  const std::size_t n = bounds.size();

  if (n == 0 ||
      n > std::numeric_limits<unsigned>::max() - 1)
    return std::nullopt;

  if (logicalDomain.getNumDomainVars() != 0 ||
      logicalDomain.getNumRangeVars() != n ||
      logicalDomain.getNumSymbolVars() != 0)
    return std::nullopt;

  IntegerPolyhedron box(
      PresburgerSpace::getSetSpace(
          static_cast<unsigned>(n)));

  for (std::size_t i = 0; i < n; ++i) {
    const auto lower = bounds[i].lower;
    const auto upper = bounds[i].upper;

    if (lower > upper)
      return std::nullopt;

    // Avoid signed overflow when negating the lower
    // bound in the constraint constant.
    //
    // INT64_MIN is rejected conservatively.
    if (lower == std::numeric_limits<std::int64_t>::min())
      return std::nullopt;

    // z_i >= lower
    //
    // z_i - lower >= 0

    llvm::SmallVector<llvm::DynamicAPInt> lowerRow(
        n + 1, llvm::DynamicAPInt(0));

    lowerRow[i] = llvm::DynamicAPInt(1);
    lowerRow.back() = llvm::DynamicAPInt(-lower);

    box.addInequality(lowerRow);

    // z_i <= upper
    //
    // -z_i + upper >= 0

    llvm::SmallVector<llvm::DynamicAPInt> upperRow(
        n + 1, llvm::DynamicAPInt(0));

    upperRow[i] = llvm::DynamicAPInt(-1);
    upperRow.back() = llvm::DynamicAPInt(upper);

    box.addInequality(upperRow);
  }

  PresburgerSet boxSet(box);

  // The exact domain must be contained in the
  // proposed finite iteration box.
  //
  // This is an integer Presburger emptiness check,
  // not a source-point enumeration.
  //
  // The RHS box has no existential local variables.

  auto uncovered = logicalDomain.subtract(boxSet);

  if (!uncovered.isIntegerEmpty())
    return std::nullopt;

  return SymbolicIterationPlan(
      logicalDomain, bounds);
}

} // namespace systolic::logical
