#pragma once

#include "Systolic/RankPartitionSourceIVRecoveryPlan.h"
#include "Systolic/RankPartitionGuardedLoopNestEmission.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/IR/Builders.h"

#include "llvm/ADT/SmallVector.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace systolic::logical {

// Phase 5.4.3d-3a-2:
//
// Recover source coordinates from logical coordinates:
//
//     x = (M * z) / d
//
// All coefficient and intermediate arithmetic safety
// checks are delegated to the existing 3a-1 preflight.
//
// Operations are emitted inside the then-region of
// the exact Presburger scf.if.
//
// This phase does NOT:
//   - generate MACs;
//   - change the loop nest;
//   - modify the exact guard;
//   - prove arbitrary source/logical domain equality;
//   - validate a target with index width other than 64.

struct EmittedSourceIVRecovery {
  llvm::SmallVector<mlir::Value> sourceIVs;
};

inline std::optional<EmittedSourceIVRecovery>
emitSourceIVRecovery(
    mlir::OpBuilder &builder,
    mlir::Location loc,
    const PartitionCoordinateMap &map,
    const std::vector<ClosedIntegerInterval> &bounds,
    const EmittedGuardedAffineLoopNest &guarded) {

  using namespace mlir;

  // --------------------------------------------------
  // Preflight: no MLIR operation created yet.
  // --------------------------------------------------

  auto recoveryPlan = buildSourceIVRecoveryPlan(
      map, bounds);

  if (!recoveryPlan)
    return std::nullopt;

  const std::size_t n = recoveryPlan->dimension;

  if (n == 0 ||
      guarded.logicalIVs.size() != n ||
      guarded.logicalI64IVs.size() != n ||
      !guarded.outermost ||
      !guarded.guardIf ||
      !guarded.predicate)
    return std::nullopt;

  if (!guarded.predicate.getType().isInteger(1))
    return std::nullopt;

  // Copy the lightweight MLIR Op handle.
  // This does not clone the underlying Operation.
  scf::IfOp guardIf = guarded.guardIf;

  if (guardIf.getCondition() !=
      guarded.predicate)
    return std::nullopt;

  for (std::size_t j = 0; j < n; ++j) {
    if (!guarded.logicalIVs[j] ||
        !guarded.logicalIVs[j].getType().isIndex() ||
        !guarded.logicalI64IVs[j] ||
        !guarded.logicalI64IVs[j].getType().isInteger(64))
      return std::nullopt;
  }

  // Verify the guarded operation is nested in the
  // affine loop belonging to the final logical IV.

  Value finalLogicalIV = guarded.logicalIVs.back();

  Block *innerBlock =
      finalLogicalIV.getParentBlock();

  if (!innerBlock)
    return std::nullopt;

  auto innerLoop =
      dyn_cast<affine::AffineForOp>(
          innerBlock->getParentOp());

  if (!innerLoop ||
      guardIf->getParentOp() !=
          innerLoop.getOperation())
    return std::nullopt;

  // Every logical coordinate must be accessible from
  // the guarded operation. The expected input is the
  // output of the previously validated loop emitter.

  for (std::size_t j = 0; j < n; ++j) {
    Value logicalIV = guarded.logicalIVs[j];

    if (logicalIV.getParentBlock() == nullptr)
      return std::nullopt;

    auto definingCast =
        guarded.logicalI64IVs[j]
            .getDefiningOp<arith::IndexCastOp>();

    if (!definingCast ||
        definingCast.getIn() != guarded.logicalIVs[j] ||
        definingCast->getBlock() != innerBlock)
      return std::nullopt;

    if (!definingCast->isBeforeInBlock(
            guardIf.getOperation()))
      return std::nullopt;
  }

  if (guardIf.getThenRegion().empty())
    return std::nullopt;

  if (recoveryPlan->denominator <= 0 ||
      recoveryPlan->inverseNumerator.size() != n)
    return std::nullopt;

  for (const auto &row :
       recoveryPlan->inverseNumerator) {
    if (row.size() != n)
      return std::nullopt;
  }

  // --------------------------------------------------
  // All checks have passed.
  //
  // From here onward, emission has no expected
  // recoverable failure path.
  // --------------------------------------------------

  OpBuilder::InsertionGuard insertionGuard(builder);

  Block &thenBlock =
      guardIf.getThenRegion().front();

  builder.setInsertionPointToStart(&thenBlock);

  EmittedSourceIVRecovery result;

  for (std::size_t i = 0; i < n; ++i) {

    Value numerator = arith::ConstantIntOp::create(
        builder, loc, 0, 64);

    for (std::size_t j = 0; j < n; ++j) {

      const std::int64_t coefficient =
          recoveryPlan->inverseNumerator[i][j];

      Value coefficientValue =
          arith::ConstantIntOp::create(
              builder, loc, coefficient, 64);

      Value product = arith::MulIOp::create(
          builder,
          loc,
          coefficientValue,
          guarded.logicalI64IVs[j]);

      numerator = arith::AddIOp::create(
          builder, loc, numerator, product);
    }

    Value recovered = numerator;

    if (recoveryPlan->denominator != 1) {

      Value denominator =
          arith::ConstantIntOp::create(
              builder,
              loc,
              recoveryPlan->denominator,
              64);

      recovered = arith::DivSIOp::create(
          builder,
          loc,
          numerator,
          denominator);
    }

    result.sourceIVs.push_back(recovered);
  }

  return result;
}

} // namespace systolic::logical
