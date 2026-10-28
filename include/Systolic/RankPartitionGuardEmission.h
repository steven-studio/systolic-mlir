#pragma once

#include "Systolic/RankPartitionGuardGeneration.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/Builders.h"

#include <cstddef>
#include <cstdint>
#include <optional>

namespace systolic::logical {

// Phase 5.4.3d-1b.
//
// Lower an already validated ExactGuardPlan to MLIR SSA.
//
// The result is exactly one i1 value representing:
//
//   OR_t (AND_r constraint[t][r](z))
//
// Coordinates must be i64.
//
// Arithmetic safety and the absence of existential locals
// must have been established by buildExactGuardPlan().
//
// No loops, memory writes or MAC operations are generated.
// No source-point enumeration.

inline std::optional<mlir::Value>
emitExactGuard(
    mlir::OpBuilder &builder,
    mlir::Location loc,
    const SymbolicIterationPlan &sourcePlan,
    mlir::ValueRange logicalIVs) {

  using namespace mlir;

  // Re-prove arithmetic safety from the source plan before any IR creation.
  auto checkedPlan = buildExactGuardPlan(sourcePlan);
  if (!checkedPlan)
    return std::nullopt;
  const ExactGuardPlan &plan = *checkedPlan;

  if (plan.dimension == 0 ||
      logicalIVs.size() != plan.dimension)
    return std::nullopt;

  Type i64Type = builder.getI64Type();

  for (Value value : logicalIVs) {
    if (!value || value.getType() != i64Type)
      return std::nullopt;
  }

  // Validate the complete structure before modifying IR.
  //
  // ExactGuardPlan is expected to originate from
  // buildExactGuardPlan(), which performed the necessary
  // arithmetic-range proofs.

  for (const auto &disjunct : plan.disjuncts) {
    for (const auto &constraint : disjunct.constraints) {
      if (constraint.coefficients.size() != plan.dimension ||
          (constraint.kind == GuardConstraintKind::Congruence &&
           constraint.modulus <= 0) ||
          (constraint.floorDivisor != 0 &&
           (constraint.floorDivisor <= 0 ||
            constraint.floorCoefficients.size() != plan.dimension ||
            constraint.kind == GuardConstraintKind::Congruence)) ||
          (constraint.floorDivisor == 0 &&
           constraint.floorCoefficient != 0) ||
          (constraint.secondFloorDivisor != 0 &&
           (constraint.kind != GuardConstraintKind::ExistentialBounds ||
            constraint.secondFloorDivisor <= 0 ||
            constraint.secondFloorCoefficients.size() != plan.dimension ||
            constraint.floorDivisor <= 0)) ||
          (constraint.kind == GuardConstraintKind::ExistentialBounds &&
           (constraint.secondFloorDivisor <= 0 ||
            constraint.floorCoefficient != 1)))
        return std::nullopt;
    }
  }

  auto constant = [&](std::int64_t value) -> Value {
    return arith::ConstantIntOp::create(
        builder, loc, value, 64);
  };

  auto boolean = [&](bool value) -> Value {
    return arith::ConstantIntOp::create(
        builder, loc, value ? 1 : 0, 1);
  };

  auto emitAffine =
      [&](const std::vector<std::int64_t> &coefficients,
          std::int64_t affineConstant) -> Value {

    Value accumulator = constant(affineConstant);

    for (std::size_t i = 0;
         i < coefficients.size(); ++i) {

      const std::int64_t coefficient = coefficients[i];

      if (coefficient == 0)
        continue;

      Value term = logicalIVs[i];

      if (coefficient != 1) {
        Value factor = constant(coefficient);

        term = arith::MulIOp::create(
            builder, loc, factor, term);
      }

      accumulator = arith::AddIOp::create(
          builder, loc, accumulator, term);
    }

    return accumulator;
  };

  auto emitLinearExpression =
      [&](const GuardConstraint &constraint) -> Value {
    Value accumulator = emitAffine(constraint.coefficients,
                                   constraint.constant);
    if (constraint.floorDivisor > 0) {
      Value numerator = emitAffine(constraint.floorCoefficients,
                                    constraint.floorConstant);
      Value divisor = constant(constraint.floorDivisor);
      Value quotient = arith::FloorDivSIOp::create(
          builder, loc, numerator, divisor);
      if (constraint.floorCoefficient != 1) {
        Value factor = constant(constraint.floorCoefficient);
        quotient = arith::MulIOp::create(builder, loc, factor, quotient);
      }
      accumulator = arith::AddIOp::create(
          builder, loc, accumulator, quotient);
    }
    if (constraint.secondFloorDivisor > 0) {
      Value numerator = emitAffine(constraint.secondFloorCoefficients,
                                    constraint.secondFloorConstant);
      Value divisor = constant(constraint.secondFloorDivisor);
      Value quotient = arith::FloorDivSIOp::create(
          builder, loc, numerator, divisor);
      accumulator = arith::AddIOp::create(
          builder, loc, accumulator, quotient);
    }
    return accumulator;
  };

  Value falseValue = boolean(false);
  Value trueValue = boolean(true);
  Value zero = constant(0);

  Value result = falseValue;

  for (const auto &disjunct : plan.disjuncts) {

    Value conjunction = trueValue;

    for (const auto &constraint : disjunct.constraints) {

      Value expression = emitLinearExpression(constraint);

      Value comparison;
      if (constraint.kind == GuardConstraintKind::Congruence) {
        // A positive divisor avoids signed remainder overflow.
        Value divisor = constant(constraint.modulus);
        Value remainder = arith::RemSIOp::create(
            builder, loc, expression, divisor);
        comparison = arith::CmpIOp::create(
            builder, loc, arith::CmpIPredicate::eq, remainder, zero);
      } else {
        arith::CmpIPredicate predicate =
            constraint.kind == GuardConstraintKind::Equality
                ? arith::CmpIPredicate::eq
                : arith::CmpIPredicate::sge;
        comparison = arith::CmpIOp::create(
            builder, loc, predicate, expression, zero);
      }

      conjunction = arith::AndIOp::create(
          builder, loc, conjunction, comparison);
    }

    result = arith::OrIOp::create(
        builder, loc, result, conjunction);
  }

  return result;
}

} // namespace systolic::logical
