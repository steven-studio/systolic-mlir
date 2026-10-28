#pragma once

#include "Systolic/RankPartitionLoopNestEmission.h"
#include "Systolic/RankPartitionGuardEmission.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/IR/Builders.h"

#include "llvm/ADT/SmallVector.h"

#include <cstdint>
#include <limits>
#include <optional>

namespace systolic::logical {

// Phase 5.4.3d-2b:
//
// Generate:
//
//   affine.for ...
//     affine.for ...
//       index_cast logical IVs to i64
//       exact Presburger guard
//       scf.if guard
//
// No MAC operations are generated.
//
// The scf.if then-region is deliberately empty.
// Phase 3 will populate the protected execution body.
//
// Failure must not leave partially generated IR.

struct EmittedGuardedAffineLoopNest {
  mlir::affine::AffineForOp outermost;
  mlir::scf::IfOp guardIf;
  mlir::Value predicate;

  llvm::SmallVector<mlir::Value> logicalIVs;
  llvm::SmallVector<mlir::Value> logicalI64IVs;
};

inline std::optional<EmittedGuardedAffineLoopNest>
emitGuardedAffineLoopNest(
    mlir::OpBuilder &builder,
    mlir::Location loc,
    const SymbolicIterationPlan &sourcePlan) {

  using namespace mlir;

  // --------------------------------------------------
  // Preflight validation.
  //
  // No IR operation may be created before this point.
  // --------------------------------------------------

  if (sourcePlan.bounds.empty())
    return std::nullopt;

  for (const auto &bound : sourcePlan.bounds) {
    if (bound.lower > bound.upper)
      return std::nullopt;

    if (bound.upper ==
        std::numeric_limits<std::int64_t>::max())
      return std::nullopt;
  }

  auto verifiedIterationPlan =
      buildSymbolicIterationPlan(
          sourcePlan.logicalDomain,
          sourcePlan.bounds);

  if (!verifiedIterationPlan)
    return std::nullopt;

  auto verifiedGuardPlan =
      buildExactGuardPlan(*verifiedIterationPlan);

  if (!verifiedGuardPlan)
    return std::nullopt;

  if (verifiedGuardPlan->dimension !=
      sourcePlan.bounds.size())
    return std::nullopt;

  // All expected validation failures have been
  // checked before IR creation.
  //
  // The builder insertion point is restored on exit.

  OpBuilder::InsertionGuard insertionGuard(builder);

  auto loops = emitAffineLoopNest(
      builder, loc, *verifiedIterationPlan);

  if (!loops)
    return std::nullopt;

  EmittedGuardedAffineLoopNest result;
  result.outermost = loops->outermost;
  result.logicalIVs = loops->logicalIVs;

  // emitAffineLoopNest() uses OpBuilder::InsertionGuard.
  //
  // Therefore its insertion point is restored when it
  // returns. Explicitly enter the innermost affine.for
  // body before emitting index casts and exact guards.
  //
  // The induction variable is a block argument of
  // the corresponding affine.for body.

  if (result.logicalIVs.empty() ||
      !result.logicalIVs.back().getParentBlock()) {
    result.outermost->erase();
    return std::nullopt;
  }

  builder.setInsertionPointToStart(
      result.logicalIVs.back().getParentBlock());

  for (Value iv : result.logicalIVs) {

    if (!iv || !iv.getType().isIndex()) {
      result.outermost->erase();
      return std::nullopt;
    }

    Value converted = arith::IndexCastOp::create(
        builder,
        loc,
        builder.getI64Type(),
        iv);

    result.logicalI64IVs.push_back(converted);
  }

  // Reuses the already validated 1d Guard Emitter.
  //
  // The emitter revalidates the source plan itself.

  auto predicate = emitExactGuard(
      builder,
      loc,
      *verifiedIterationPlan,
      result.logicalI64IVs);

  if (!predicate) {
    result.outermost->erase();
    return std::nullopt;
  }

  if (!(*predicate).getType().isInteger(1)) {
    result.outermost->erase();
    return std::nullopt;
  }

  result.predicate = *predicate;

  // Protect future computation using the exact
  // Presburger membership predicate.
  //
  // Empty then-region is intentional for Phase 2b.

  result.guardIf = scf::IfOp::create(
      builder,
      loc,
      TypeRange{},
      result.predicate,
      false);

  return result;
}

} // namespace systolic::logical
