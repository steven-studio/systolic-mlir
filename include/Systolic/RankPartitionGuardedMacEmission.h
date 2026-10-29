#pragma once

#include "Systolic/RankPartitionGuardedMemoryEmission.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/MLIRContext.h"

#include "llvm/ADT/SmallPtrSet.h"

#include <cstddef>
#include <optional>

namespace systolic::logical {

// Phase 5.4.3d-3b-3:
//
// Guarded multiply-accumulate with exact reduction
// semantics.
//
// Completes the guarded statement of Phase 3b-2 inside the
// then-region of the exact Presburger scf.if:
//
//   %a = memref.load A[f_A(x)]      (Phase 3b-2)
//   %b = memref.load B[f_B(x)]      (Phase 3b-2)
//   %c = memref.load C[f_C(x)]      (Phase 3b-2)
//   %p = mul %a, %b                 arith.mulf / arith.muli
//   %s = add %c, %p                 arith.addf / arith.addi
//   memref.store %s, C[f_C(x)]      (Phase 3b-2)
//
// which implements the source statement
//
//   C[f_C(x)] = C[f_C(x)] + A[f_A(x)] * B[f_B(x)]
//
// Every check runs before any operation is created. The
// store is emitted by emitGuardedMemoryStore; if that call
// refuses, the two arithmetic operations are erased, so a
// refusal never leaves a partially emitted statement.
//
// Exact arithmetic and reduction semantics certified by a
// successful emission, in addition to C1-C4 of Phase 3b-2:
//
//   C5 same operations: the multiplication and the addition
//      are the source operation kinds on the source element
//      type, in the source operand order, with the source's
//      strict attributes: arith.mulf/addf without fast-math
//      flags and without a rounding-mode attribute (the
//      dialect's default, IEEE 754 round to nearest even
//      with signed zeros, infinities and NaNs as IEEE 754
//      specifies; no reassociation, no contraction), or
//      arith.muli/addi without overflow flags (two's
//      complement wrapping). Whatever a backend implements
//      for these attribute-free operations, it implements
//      identically for the source and the emitted ones.
//   C6 same operands: A and B are read-only and disjoint from
//      C (Phase 3b-2 V8 and the caller's alias certificate),
//      every C element receives its updates in source order
//      (Phase 3b-2 S3), and each update reads the running C
//      value stored by the previous one. Hence every emitted
//      operation sees exactly the operands its source
//      counterpart sees, and produces the identical result
//      bit for bit, including every rounding, overflow,
//      signed zero, infinity and NaN.
//
// No reduction is reassociated, tiled, parallelised or
// contracted into a fused multiply-add. This holds for the
// emitted IR; later lowerings must keep the arith semantics
// (the emitted operations carry no flag that would permit
// them to do otherwise).
//
// Refused (never approximated):
//   - element types other than f32, f64 and signless
//     integers of 2 to 64 bits (so f16, bf16, i1, index,
//     vectors and complex types);
//   - fast-math flags, a rounding-mode attribute or integer
//     overflow flags on the source operations (mirroring
//     them is deferred: it is exact for the emitted IR but
//     changes what later passes may do, which this phase
//     does not certify);
//   - every condition Phase 3b-2 refuses, including a
//     tampered loads struct and a nest with other effects.
//
// Caller obligations: those of Phase 3b-2, plus the
// statement shape: the source statement is one
// multiplication of the A and B loads and one addition of
// the product to the C load, nothing else.

struct GuardedMacSemantics {
  // Operand order of the source statement. IEEE 754 and
  // two's complement multiplication and addition are
  // commutative, but the source order is mirrored so the
  // emitted operations are structurally identical.
  bool aIsLeftFactor = true;           // A * B, not B * A
  bool accumulatorIsLeftAddend = true; // C + p, not p + C

  // Attributes of the source operations. Only the strict
  // defaults are accepted in this phase: no fast-math
  // flags, no rounding-mode attribute, no integer overflow
  // flags.
  mlir::arith::FastMathFlags fastMath =
      mlir::arith::FastMathFlags::none;
  std::optional<mlir::arith::RoundingMode> roundingMode;
  mlir::arith::IntegerOverflowFlags integerOverflow =
      mlir::arith::IntegerOverflowFlags::none;

  // Phase 3b-8 (section 29): the host remainder of a hybrid
  // lowering runs under the program's own relaxed contract,
  // so the source statement's fast-math flags are MIRRORED
  // on the emitted operations (each operation its own
  // flags) instead of refused. A rounding-mode attribute is
  // still refused. Default false: the strict rule above.
  bool mirrorFastMath = false;
  mlir::arith::FastMathFlags multiplyFastMath =
      mlir::arith::FastMathFlags::none;
  mlir::arith::FastMathFlags addFastMath =
      mlir::arith::FastMathFlags::none;
};

struct EmittedGuardedMultiplyAccumulate {
  mlir::Operation *multiply = nullptr;
  mlir::Operation *add = nullptr;
  mlir::Value result;
  mlir::memref::StoreOp store;
};

namespace guarded_mac_detail {

enum class MacArithmetic { Float, Integer };

// Element types this phase emits with exactly the source
// semantics and validates numerically: IEEE binary32 and
// binary64, and signless integers of 2 to 64 bits.
inline std::optional<MacArithmetic>
classifyElementType(mlir::Type type) {
  if (!type)
    return std::nullopt;

  if (type.isF32() || type.isF64())
    return MacArithmetic::Float;

  if (type.isSignlessInteger()) {
    const unsigned width = type.getIntOrFloatBitWidth();

    if (width >= 2 && width <= 64)
      return MacArithmetic::Integer;
  }

  return std::nullopt;
}

} // namespace guarded_mac_detail

inline std::optional<EmittedGuardedMultiplyAccumulate>
emitGuardedMultiplyAccumulate(
    mlir::OpBuilder &builder,
    mlir::Location loc,
    const PartitionCoordinateMap &map,
    const SymbolicIterationPlan &plan,
    const mlir::presburger::IntegerPolyhedron &sourceDomain,
    const EmittedGuardedAffineLoopNest &guarded,
    const EmittedSourceIVRecovery &recovery,
    const GuardedMemoryOperand &a,
    const GuardedMemoryOperand &b,
    const GuardedMemoryOperand &c,
    const EmittedGuardedMemoryLoads &loads,
    const GuardedMacSemantics &semantics,
    bool partialDomain = false) {

  using namespace mlir;
  namespace memory = guarded_memory_detail;
  namespace mac = guarded_mac_detail;

  // --------------------------------------------------
  // Preflight: the complete Phase 3b-2 certificate is
  // re-established (nothing from the loads call is
  // trusted), then the arithmetic is classified.
  // No MLIR operation is created before this point.
  // --------------------------------------------------

  MLIRContext *context = builder.getContext();

  if (!context->getLoadedDialect<memref::MemRefDialect>() ||
      !context->getLoadedDialect<arith::ArithDialect>())
    return std::nullopt;

  Block *thenBlock = memory::validateGuardedStatementContext(
      map, plan, sourceDomain, guarded, recovery, partialDomain);

  if (!thenBlock)
    return std::nullopt;

  const std::size_t n = plan.bounds.size();

  auto elementType = memory::validateOperandTypes(a, b, c, guarded);

  if (!elementType)
    return std::nullopt;

  for (const GuardedMemoryOperand *operand : {&a, &b, &c}) {
    if (!memory::validateOperand(
            *operand, sourceDomain, recovery.sourceIVs, thenBlock,
            partialDomain))
      return std::nullopt;
  }

  memref::LoadOp loadA = loads.loadA;
  memref::LoadOp loadB = loads.loadB;
  memref::LoadOp loadC = loads.loadC;

  if (!memory::loadMatchesOperand(
          loadA, loads.indicesA, a, *elementType, thenBlock) ||
      !memory::loadMatchesOperand(
          loadB, loads.indicesB, b, *elementType, thenBlock) ||
      !memory::loadMatchesOperand(
          loadC, loads.indicesC, c, *elementType, thenBlock))
    return std::nullopt;

  // C5: supported arithmetic with strict flags only.

  auto arithmetic = mac::classifyElementType(*elementType);

  if (!arithmetic)
    return std::nullopt;

  if (*arithmetic == mac::MacArithmetic::Float &&
      ((!semantics.mirrorFastMath &&
        semantics.fastMath != arith::FastMathFlags::none) ||
       semantics.roundingMode.has_value()))
    return std::nullopt;

  if (semantics.roundingMode.has_value())
    return std::nullopt; // no rounding mode on integer operations

  if (*arithmetic == mac::MacArithmetic::Integer &&
      semantics.integerOverflow != arith::IntegerOverflowFlags::none)
    return std::nullopt;

  // C6: source order of every update of a C element.

  if (!memory::orderPreservedUnderLogicalSchedule(
          map, sourceDomain, c, n))
    return std::nullopt;

  // The nest holds nothing but the three loads: no other
  // effect, and in particular no earlier statement.

  llvm::SmallPtrSet<Operation *, 4> allowed;
  allowed.insert(loadA.getOperation());
  allowed.insert(loadB.getOperation());
  allowed.insert(loadC.getOperation());

  if (!memory::nestHasOnlyAllowedEffects(guarded, allowed))
    return std::nullopt;

  // --------------------------------------------------
  // Emission. The loads are defined in the then block,
  // so their results dominate the insertion point before
  // the terminator; the product dominates the sum, and the
  // sum dominates the store emitted after it.
  // --------------------------------------------------

  OpBuilder::InsertionGuard insertionGuard(builder);

  builder.setInsertionPoint(thenBlock->getTerminator());

  Value valueA = loadA.getResult();
  Value valueB = loadB.getResult();
  Value valueC = loadC.getResult();

  Value leftFactor = semantics.aIsLeftFactor ? valueA : valueB;
  Value rightFactor = semantics.aIsLeftFactor ? valueB : valueA;

  Operation *multiply = nullptr;
  Value product;

  if (*arithmetic == mac::MacArithmetic::Float) {
    auto op = arith::MulFOp::create(
        builder, loc, leftFactor, rightFactor);
    if (semantics.mirrorFastMath)
      op.setFastmathAttr(arith::FastMathFlagsAttr::get(
          context, semantics.multiplyFastMath));
    multiply = op.getOperation();
    product = op.getResult();
  } else {
    auto op = arith::MulIOp::create(
        builder, loc, leftFactor, rightFactor);
    multiply = op.getOperation();
    product = op.getResult();
  }

  Value leftAddend = semantics.accumulatorIsLeftAddend ? valueC : product;
  Value rightAddend = semantics.accumulatorIsLeftAddend ? product : valueC;

  Operation *add = nullptr;
  Value sum;

  if (*arithmetic == mac::MacArithmetic::Float) {
    auto op = arith::AddFOp::create(
        builder, loc, leftAddend, rightAddend);
    if (semantics.mirrorFastMath)
      op.setFastmathAttr(arith::FastMathFlagsAttr::get(
          context, semantics.addFastMath));
    add = op.getOperation();
    sum = op.getResult();
  } else {
    auto op = arith::AddIOp::create(
        builder, loc, leftAddend, rightAddend);
    add = op.getOperation();
    sum = op.getResult();
  }

  // The store re-establishes the Phase 3b-2 certificate
  // with the sum as the stored value; the two pure
  // operations above are what its effect walk sees.

  auto store = emitGuardedMemoryStore(
      builder, loc, map, plan, sourceDomain, guarded, recovery,
      a, b, c, loads, sum, partialDomain);

  if (!store) {
    add->erase();
    multiply->erase();
    return std::nullopt;
  }

  EmittedGuardedMultiplyAccumulate result;
  result.multiply = multiply;
  result.add = add;
  result.result = sum;
  result.store = store->store;

  return result;
}

} // namespace systolic::logical
