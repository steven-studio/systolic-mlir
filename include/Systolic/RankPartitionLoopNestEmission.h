#pragma once

#include "Systolic/RankPartitionIterationPlan.h"

#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/IR/Builders.h"

#include "llvm/ADT/SmallVector.h"

#include <cstdint>
#include <limits>
#include <optional>

namespace systolic::logical {

// Phase 5.4.3d-2a:
//
// Emit an affine.for nest over the certified
// candidate rectangular iteration space.
//
// This phase does NOT emit the exact domain guard.
//
// Logical IVs are index-typed SSA values.
//
// Guard integration belongs to Phase 2b.
// Exactly-once execution validation belongs to Phase 2c.
//
// No source-point enumeration.
// No MAC operations.
// No existing lowering modification.

struct EmittedAffineLoopNest {
  mlir::affine::AffineForOp outermost;
  llvm::SmallVector<mlir::Value> logicalIVs;
};

inline std::optional<EmittedAffineLoopNest>
emitAffineLoopNest(
    mlir::OpBuilder &builder,
    mlir::Location loc,
    const SymbolicIterationPlan &sourcePlan) {

  const auto &bounds = sourcePlan.bounds;

  if (bounds.empty())
    return std::nullopt;

  // Revalidate coverage before creating any operation.
  //
  // SymbolicIterationPlan is publicly constructible,
  // so callers cannot be trusted to supply a
  // previously validated plan.

  auto verified = buildSymbolicIterationPlan(
      sourcePlan.logicalDomain,
      bounds);

  if (!verified)
    return std::nullopt;

  // Validate every dimension before modifying IR.
  //
  // affine.for uses:
  //
  //   [lower, upper)
  //
  // while SymbolicIterationPlan uses:
  //
  //   [lower, upper]
  //
  // Therefore upper+1 must be representable.

  for (const auto &bound : bounds) {

    if (bound.lower > bound.upper)
      return std::nullopt;

    if (bound.upper ==
        std::numeric_limits<std::int64_t>::max())
      return std::nullopt;
  }

  // All validation has completed.
  // IR creation starts below this point.

  mlir::OpBuilder::InsertionGuard insertionGuard(builder);

  EmittedAffineLoopNest result;

  for (std::size_t dimension = 0;
       dimension < bounds.size();
       ++dimension) {

    const auto &bound = bounds[dimension];

    const std::int64_t lower = bound.lower;
    const std::int64_t upper = bound.upper + 1;

    auto loop = mlir::affine::AffineForOp::create(
        builder,
        loc,
        lower,
        upper,
        1);

    if (dimension == 0)
      result.outermost = loop;

    result.logicalIVs.push_back(
        loop.getInductionVar());

    // Insert the next loop inside this loop,
    // before the affine.yield terminator.

    builder.setInsertionPointToStart(
        loop.getBody());
  }

  return result;
}

} // namespace systolic::logical
