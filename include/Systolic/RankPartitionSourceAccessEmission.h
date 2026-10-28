#pragma once

#include "Systolic/RankPartitionSourceIVEmission.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/Builders.h"

#include "llvm/ADT/SmallVector.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <vector>

namespace systolic::logical {

// Phase 5.4.3d-3b-1a.
//
// Emit an integer affine access:
//
//   f(x) = A*x + b
//
// from previously emitted i64 Source IV SSA values.
//
// This phase does not emit memory operations.
//
// Arithmetic is conservatively checked over the full
// candidate logical bounding box.

struct EmittedSourceAccess {
  llvm::SmallVector<mlir::Value> indices;
};

namespace source_access_detail {

inline bool fitsI64(const BigInt &value) {
  return value >= BigInt(
             std::numeric_limits<std::int64_t>::min()) &&
         value <= BigInt(
             std::numeric_limits<std::int64_t>::max());
}

} // namespace source_access_detail

inline std::optional<EmittedSourceAccess>
emitSourceAffineAccess(
    mlir::OpBuilder &builder,
    mlir::Location loc,
    const PartitionCoordinateMap &map,
    const std::vector<ClosedIntegerInterval> &bounds,
    const EmittedGuardedAffineLoopNest &guarded,
    const EmittedSourceIVRecovery &recovery,
    const std::vector<std::vector<BigInt>> &accessRows,
    const std::vector<BigInt> &accessOffsets) {

  using namespace mlir;

  auto recoveryPlan =
      buildSourceIVRecoveryPlan(map, bounds);

  if (!recoveryPlan)
    return std::nullopt;

  const std::size_t n = recoveryPlan->dimension;

  if (n == 0 ||
      accessRows.empty() ||
      accessRows.size() != accessOffsets.size() ||
      recovery.sourceIVs.size() != n ||
      !guarded.guardIf)
    return std::nullopt;

  scf::IfOp guardIf = guarded.guardIf;

  if (guardIf.getThenRegion().empty())
    return std::nullopt;

  Block &thenBlock = guardIf.getThenRegion().front();

  // Every recovered coordinate must be an i64 value
  // defined inside the protected region.

  for (Value iv : recovery.sourceIVs) {
    if (!iv || !iv.getType().isInteger(64))
      return std::nullopt;

    Operation *definition = iv.getDefiningOp();

    if (!definition ||
        definition->getBlock() != &thenBlock)
      return std::nullopt;
  }

  // Derive conservative bounds for each recovered
  // source coordinate from the logical bounding box.
  //
  // Since denominator >= 1, the absolute recovered
  // integer is no greater than the absolute numerator.
  //
  // This may reject some otherwise-safe accesses.

  std::vector<BigInt> sourceAbsBounds(n, BigInt(0));

  for (std::size_t i = 0; i < n; ++i) {
    BigInt maximumNumerator = 0;

    for (std::size_t j = 0; j < n; ++j) {
      const BigInt coefficient(
          recoveryPlan->inverseNumerator[i][j]);

      const BigInt lowerProduct =
          coefficient * BigInt(bounds[j].lower);

      const BigInt upperProduct =
          coefficient * BigInt(bounds[j].upper);

      const BigInt lower =
          lowerProduct < upperProduct
              ? lowerProduct : upperProduct;

      const BigInt upper =
          lowerProduct > upperProduct
              ? lowerProduct : upperProduct;

      const BigInt absLower =
          lower < 0 ? -lower : lower;

      const BigInt absUpper =
          upper < 0 ? -upper : upper;

      maximumNumerator +=
          absLower > absUpper ? absLower : absUpper;
    }

    if (!source_access_detail::fitsI64(
            maximumNumerator))
      return std::nullopt;

    sourceAbsBounds[i] = maximumNumerator;
  }

  // Preflight all access arithmetic before emitting
  // any operation.
  //
  // The emitted sequence is:
  //
  //   accumulator = offset
  //   product_j   = coefficient_j * sourceIV_j
  //   accumulator = accumulator + product_j
  //
  // Both products and intermediate accumulators
  // must fit signed i64.

  for (std::size_t row = 0;
       row < accessRows.size(); ++row) {

    if (accessRows[row].size() != n ||
        !source_access_detail::fitsI64(
            accessOffsets[row]))
      return std::nullopt;

    BigInt minimum = accessOffsets[row];
    BigInt maximum = accessOffsets[row];

    for (std::size_t j = 0; j < n; ++j) {
      const BigInt &coefficient =
          accessRows[row][j];

      if (!source_access_detail::fitsI64(
              coefficient))
        return std::nullopt;

      const BigInt magnitude =
          coefficient < 0
              ? -coefficient : coefficient;

      const BigInt limit =
          magnitude * sourceAbsBounds[j];

      if (!source_access_detail::fitsI64(limit))
        return std::nullopt;

      minimum -= limit;
      maximum += limit;

      if (!source_access_detail::fitsI64(minimum) ||
          !source_access_detail::fitsI64(maximum))
        return std::nullopt;
    }
  }

  // All preflight checks passed.
  //
  // Insert after Recovery SSA definitions, but before
  // the terminator of the guarded then-region.
  //
  // This preserves SSA dominance and does not modify
  // the outer loop or guard structure.

  OpBuilder::InsertionGuard insertionGuard(builder);

  builder.setInsertionPoint(
      thenBlock.getTerminator());

  EmittedSourceAccess result;

  for (std::size_t row = 0;
       row < accessRows.size(); ++row) {

    Value accumulator =
        arith::ConstantIntOp::create(
            builder,
            loc,
            accessOffsets[row].convert_to<std::int64_t>(),
            64);

    for (std::size_t j = 0; j < n; ++j) {

      Value coefficient =
          arith::ConstantIntOp::create(
              builder,
              loc,
              accessRows[row][j]
                  .convert_to<std::int64_t>(),
              64);

      Value product =
          arith::MulIOp::create(
              builder,
              loc,
              coefficient,
              recovery.sourceIVs[j]);

      accumulator =
          arith::AddIOp::create(
              builder,
              loc,
              accumulator,
              product);
    }

    result.indices.push_back(accumulator);
  }

  return result;
}

} // namespace systolic::logical
