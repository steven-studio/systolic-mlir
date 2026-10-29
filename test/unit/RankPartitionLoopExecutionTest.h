#pragma once

// Phase 5.4.3d-2c-1
//
// Test-only interpreter for the generated MLIR subset.
//
// Supported:
//   affine.for with constant bounds, or with affine-map
//     bounds over interpreted operands (Phase 3b-4)
//   affine.load / affine.store (Phase 3b-4, only with a
//     memory model; the map is evaluated exactly)
//   affine.yield
//   arith.constant
//   arith.index_cast
//   arith.addi
//   arith.muli
//   arith.floordivsi
//   arith.remsi
//   arith.cmpi
//   arith.andi
//   arith.ori
//   scf.if
//   scf.yield
//   memref.load / memref.store (only with a memory model)
//   arith.mulf / arith.addf on f32 and f64 (Phase 3b-3:
//     strict IEEE 754, one rounding per operation)
//   arith.addi / arith.muli on integers narrower than 64
//     bits wrap to their width (Phase 3b-3); 64-bit
//     integer arithmetic stays checked; overflow flags,
//     fast-math flags and rounding-mode attributes are
//     refused; other integer operations on types narrower
//     than 64 bits (except i1 logic) are refused
//
// Unsupported operations cause failure.
//
// This is NOT LLVM JIT execution.

#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/IR/AffineExpr.h"
#include "mlir/IR/AffineMap.h"
#include "mlir/IR/Operation.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/BuiltinTypes.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMap.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <optional>
#include <vector>

namespace systolic::logical::testing {

using Point = std::vector<std::int64_t>;
using ExecutionCounts = std::map<Point, std::size_t>;

struct ExecutionResult {
  ExecutionCounts counts;
  ExecutionCounts sourceCounts;

  // Logical point -> [A indices, B indices, C indices].
  //
  // Values are read after execution of the actual
  // guarded then-region.

  std::map<Point, std::vector<Point>> accessByLogical;

  std::size_t visitedPoints = 0;
  std::size_t acceptedPoints = 0;
};

// Phase 5.4.3d-3b-2:
//
// Optional memory model for memref.load / memref.store.
//
// Buffers are keyed by the memref SSA value. The element
// type must be i64, the static memref shape must equal
// the buffer shape and every access must be in bounds;
// anything else fails closed. Every executed memory
// operation is appended to `trace` in execution order.

struct MemoryBuffer {
  std::vector<std::int64_t> shape;

  // Integer elements (any width up to 64, sign extended).
  std::vector<std::int64_t> data;

  // Phase 3b-3: f32 / f64 elements. An f32 element is a
  // double holding an exactly representable float.
  bool isFloat = false;
  std::vector<double> floatData;
};

struct MemoryEvent {
  bool isStore = false;
  mlir::Value memref;
  Point indices;
  std::int64_t value = 0;

  // Phase 3b-3: set for float elements.
  bool isFloat = false;
  double floatValue = 0.0;
};

struct MemoryState {
  llvm::DenseMap<mlir::Value, MemoryBuffer> buffers;
  std::vector<MemoryEvent> trace;
};

inline std::optional<ExecutionResult>
interpretGuardedLoopNest(
    mlir::affine::AffineForOp outermost,
    mlir::scf::IfOp guardedIf,
    mlir::ValueRange logicalIVs,
    mlir::ValueRange sourceIVs = {},
    mlir::ValueRange accessA = {},
    mlir::ValueRange accessB = {},
    mlir::ValueRange accessC = {},
    MemoryState *memory = nullptr) {

  using namespace mlir;

  // Phase 3b-4: with no guard, a plain nest is executed
  // and only its memory model is observed.

  if (!outermost || (guardedIf && logicalIVs.empty()))
    return std::nullopt;

  const bool observingAccesses =
      !accessA.empty() ||
      !accessB.empty() ||
      !accessC.empty();

  if (observingAccesses &&
      (accessA.empty() ||
       accessB.empty() ||
       accessC.empty()))
    return std::nullopt;

  llvm::DenseMap<Value, std::int64_t> values;

  // Phase 3b-3: f32 values are stored as exactly
  // representable doubles; f64 values as themselves.
  llvm::DenseMap<Value, double> floatValues;

  ExecutionResult result;
  bool valid = true;

  // Evaluate the restricted signed arithmetic subset.
  //
  // The tests use small integer domains.
  // Reject arithmetic outside int64_t range.

  auto checked = [](__int128 number)
      -> std::optional<std::int64_t> {
    if (number < std::numeric_limits<std::int64_t>::min() ||
        number > std::numeric_limits<std::int64_t>::max())
      return std::nullopt;
    return static_cast<std::int64_t>(number);
  };

  // Phase 3b-7: a constant defined outside the executed nest (the
  // production lowering hoists its constants before its loops) is read
  // from its defining operation.
  auto lookup = [&](Value value)
      -> std::optional<std::int64_t> {
    auto it = values.find(value);
    if (it != values.end())
      return it->second;
    if (auto constant = value.getDefiningOp<arith::ConstantOp>())
      if (auto attribute = dyn_cast<IntegerAttr>(constant.getValue())) {
        const std::int64_t constantValue =
            attribute.getValue().getSExtValue();
        values[value] = constantValue;
        return constantValue;
      }
    return std::nullopt;
  };

  auto lookupFloat = [&](Value value)
      -> std::optional<double> {
    auto it = floatValues.find(value);
    if (it != floatValues.end())
      return it->second;
    if (auto constant = value.getDefiningOp<arith::ConstantOp>())
      if (auto attribute = dyn_cast<FloatAttr>(constant.getValue())) {
        Type type = constant.getResult().getType();
        if (!type.isF32() && !type.isF64())
          return std::nullopt;
        const double constantValue = attribute.getValueAsDouble();
        floatValues[value] = constantValue;
        return constantValue;
      }
    return std::nullopt;
  };

  // Two's complement wrap of an exact result to `width`
  // bits (Phase 3b-3: integers narrower than 64 bits).
  auto wrapToWidth = [](__int128 exact, unsigned width)
      -> std::int64_t {
    if (width < 1 || width > 63)
      return 0; // never reached: callers only pass 2..63

    const __int128 modulus = static_cast<__int128>(1) << width;
    __int128 remainder = exact % modulus;
    if (remainder < 0)
      remainder += modulus;
    if (remainder >= modulus / 2)
      remainder -= modulus;
    return static_cast<std::int64_t>(remainder);
  };

  // Phase 3b-4: exact evaluation of an affine map on
  // interpreted operand values (dims first, then symbols).

  std::function<std::optional<std::int64_t>(
      AffineExpr, llvm::ArrayRef<std::int64_t>, unsigned)>
      evaluateExpr;

  evaluateExpr = [&](AffineExpr expr, llvm::ArrayRef<std::int64_t> operands,
                     unsigned numDims) -> std::optional<std::int64_t> {
    if (auto constant = dyn_cast<AffineConstantExpr>(expr))
      return constant.getValue();

    if (auto dim = dyn_cast<AffineDimExpr>(expr)) {
      if (dim.getPosition() >= numDims)
        return std::nullopt;
      return operands[dim.getPosition()];
    }

    if (auto symbol = dyn_cast<AffineSymbolExpr>(expr)) {
      const std::size_t position = numDims + symbol.getPosition();
      if (position >= operands.size())
        return std::nullopt;
      return operands[position];
    }

    auto binary = dyn_cast<AffineBinaryOpExpr>(expr);

    if (!binary)
      return std::nullopt;

    auto lhs = evaluateExpr(binary.getLHS(), operands, numDims);
    auto rhs = evaluateExpr(binary.getRHS(), operands, numDims);

    if (!lhs || !rhs)
      return std::nullopt;

    const __int128 a = *lhs;
    const __int128 b = *rhs;

    switch (expr.getKind()) {
    case AffineExprKind::Add:
      return checked(a + b);
    case AffineExprKind::Mul:
      return checked(a * b);
    case AffineExprKind::FloorDiv: {
      if (b <= 0)
        return std::nullopt;
      __int128 q = a / b;
      if (a % b != 0 && a < 0)
        --q;
      return checked(q);
    }
    case AffineExprKind::CeilDiv: {
      if (b <= 0)
        return std::nullopt;
      __int128 q = a / b;
      if (a % b != 0 && a > 0)
        ++q;
      return checked(q);
    }
    case AffineExprKind::Mod: {
      if (b <= 0)
        return std::nullopt;
      __int128 r = a % b;
      if (r < 0)
        r += b;
      return checked(r);
    }
    default:
      return std::nullopt;
    }
  };

  auto evaluateMap = [&](AffineMap map, ValueRange mapOperands)
      -> std::optional<std::vector<std::int64_t>> {
    if (map.getNumInputs() != mapOperands.size())
      return std::nullopt;

    std::vector<std::int64_t> operands;
    operands.reserve(mapOperands.size());

    for (Value operand : mapOperands) {
      auto value = lookup(operand);
      if (!value)
        return std::nullopt;
      operands.push_back(*value);
    }

    std::vector<std::int64_t> results;

    for (AffineExpr expr : map.getResults()) {
      auto value = evaluateExpr(expr, operands, map.getNumDims());
      if (!value)
        return std::nullopt;
      results.push_back(*value);
    }

    return results;
  };

  std::function<void(Block &)> executeBlock;
  std::function<void(Operation &)> executeOp;

  executeBlock = [&](Block &block) {
    for (Operation &op : block) {
      if (!valid)
        return;
      executeOp(op);
    }
  };

  executeOp = [&](Operation &op) {
    if (!valid)
      return;

    if (auto loop = dyn_cast<affine::AffineForOp>(&op)) {
      std::int64_t lower = 0;
      std::int64_t upper = 0;

      if (loop.hasConstantBounds()) {
        lower = loop.getConstantLowerBound();
        upper = loop.getConstantUpperBound();
      } else {
        // affine.for: the lower bound is the maximum of its
        // map's results, the upper bound the minimum.

        auto lowerResults = evaluateMap(loop.getLowerBoundMap(),
                                        loop.getLowerBoundOperands());
        auto upperResults = evaluateMap(loop.getUpperBoundMap(),
                                        loop.getUpperBoundOperands());

        if (!lowerResults || !upperResults ||
            lowerResults->empty() || upperResults->empty()) {
          valid = false;
          return;
        }

        lower = *std::max_element(lowerResults->begin(),
                                  lowerResults->end());
        upper = *std::min_element(upperResults->begin(),
                                  upperResults->end());
      }

      const llvm::APInt stepValue = loop.getStep();

      if (!stepValue.isSignedIntN(64)) {
        valid = false;
        return;
      }

      const std::int64_t step = stepValue.getSExtValue();

      if (step <= 0) {
        valid = false;
        return;
      }

      Value iv = loop.getInductionVar();

      for (std::int64_t i = lower; i < upper;) {
        values[iv] = i;
        executeBlock(*loop.getBody());

        if (!valid)
          return;

        // Avoid signed overflow during increment.
        const __int128 next =
            static_cast<__int128>(i) + step;

        if (next >= upper)
          break;

        auto converted = checked(next);
        if (!converted) {
          valid = false;
          return;
        }

        i = *converted;
      }

      values.erase(iv);
      return;
    }

    if (isa<affine::AffineYieldOp, scf::YieldOp>(&op))
      return;

    if (auto constant = dyn_cast<arith::ConstantOp>(&op)) {
      if (auto floatAttribute =
              dyn_cast<FloatAttr>(constant.getValue())) {
        Type type = constant.getResult().getType();

        if (!type.isF32() && !type.isF64()) {
          valid = false;
          return;
        }

        floatValues[constant.getResult()] =
            floatAttribute.getValueAsDouble();
        return;
      }

      auto attribute =
          dyn_cast<IntegerAttr>(constant.getValue());

      if (!attribute) {
        valid = false;
        return;
      }

      values[constant.getResult()] =
          attribute.getValue().getSExtValue();
      return;
    }

    if (auto cast = dyn_cast<arith::IndexCastOp>(&op)) {
      auto operand = lookup(cast.getIn());
      if (!operand) {
        valid = false;
        return;
      }

      values[cast.getResult()] = *operand;
      return;
    }

    if (auto condition = dyn_cast<scf::IfOp>(&op)) {
      auto predicate = lookup(condition.getCondition());

      if (!predicate) {
        valid = false;
        return;
      }

      if (&op == guardedIf.getOperation()) {
        ++result.visitedPoints;

        if (*predicate != 0) {
          Point point;
          point.reserve(logicalIVs.size());

          for (Value iv : logicalIVs) {
            auto coordinate = lookup(iv);
            if (!coordinate) {
              valid = false;
              return;
            }
            point.push_back(*coordinate);
          }

          ++result.counts[point];
          ++result.acceptedPoints;
        }
      }

      if (*predicate != 0) {
        executeBlock(condition.getThenRegion().front());

        if (!valid)
          return;

        // Phase 3a-3b-2:
        // Read source coordinates produced by the
        // executed recovery SSA operations.
        //
        // These values are available only after
        // executing the guarded then-region.

        if (&op == guardedIf.getOperation() &&
            !sourceIVs.empty()) {

          Point sourcePoint;
          sourcePoint.reserve(sourceIVs.size());

          for (Value iv : sourceIVs) {
            auto coordinate = lookup(iv);

            if (!coordinate) {
              valid = false;
              return;
            }

            sourcePoint.push_back(*coordinate);
          }

          ++result.sourceCounts[sourcePoint];
        }

        // Phase 5.4.3d-3b-1b:
        //
        // Read the actual SSA values produced inside
        // the guarded region. Never compute access
        // indices from the reference matrices here.

        if (&op == guardedIf.getOperation() &&
            observingAccesses) {

          Point logicalPoint;
          logicalPoint.reserve(logicalIVs.size());

          for (Value iv : logicalIVs) {
            auto coordinate = lookup(iv);

            if (!coordinate) {
              valid = false;
              return;
            }

            logicalPoint.push_back(*coordinate);
          }

          std::vector<Point> observedAccesses;
          observedAccesses.reserve(3);

          for (ValueRange access :
               {accessA, accessB, accessC}) {

            Point indices;
            indices.reserve(access.size());

            for (Value index : access) {
              auto coordinate = lookup(index);

              if (!coordinate) {
                valid = false;
                return;
              }

              indices.push_back(*coordinate);
            }

            observedAccesses.push_back(
                std::move(indices));
          }

          auto insertion =
              result.accessByLogical.emplace(
                  std::move(logicalPoint),
                  std::move(observedAccesses));

          // Each logical coordinate should be
          // observed exactly once in this test.

          if (!insertion.second) {
            valid = false;
            return;
          }
        }

      } else if (!condition.getElseRegion().empty()) {
        executeBlock(condition.getElseRegion().front());
      }
      return;
    }

    if (isa<memref::LoadOp, memref::StoreOp, affine::AffineLoadOp,
            affine::AffineStoreOp>(&op)) {

      // Memory operations are interpreted only against
      // an explicit memory model.

      if (!memory) {
        valid = false;
        return;
      }

      Value memref;
      std::vector<std::int64_t> indexValues;
      bool isLoad = false;
      Value storedValue;
      Value loadedResult;

      if (auto load = dyn_cast<memref::LoadOp>(&op)) {
        memref = load.getMemRef();
        isLoad = true;
        loadedResult = load.getResult();
        for (Value index : load.getIndices()) {
          auto coordinate = lookup(index);
          if (!coordinate) {
            valid = false;
            return;
          }
          indexValues.push_back(*coordinate);
        }
      } else if (auto store = dyn_cast<memref::StoreOp>(&op)) {
        memref = store.getMemRef();
        storedValue = store.getValue();
        for (Value index : store.getIndices()) {
          auto coordinate = lookup(index);
          if (!coordinate) {
            valid = false;
            return;
          }
          indexValues.push_back(*coordinate);
        }
      } else if (auto load = dyn_cast<affine::AffineLoadOp>(&op)) {
        // Phase 3b-4: the source nest's own accesses.
        memref = load.getMemRef();
        isLoad = true;
        loadedResult = load.getResult();
        auto evaluated =
            evaluateMap(load.getAffineMap(), load.getMapOperands());
        if (!evaluated) {
          valid = false;
          return;
        }
        indexValues = std::move(*evaluated);
      } else {
        auto store = cast<affine::AffineStoreOp>(&op);
        memref = store.getMemRef();
        storedValue = store.getValueToStore();
        auto evaluated =
            evaluateMap(store.getAffineMap(), store.getMapOperands());
        if (!evaluated) {
          valid = false;
          return;
        }
        indexValues = std::move(*evaluated);
      }

      auto bufferIt = memory->buffers.find(memref);

      if (bufferIt == memory->buffers.end()) {
        valid = false;
        return;
      }

      MemoryBuffer &buffer = bufferIt->second;

      auto type = dyn_cast<MemRefType>(memref.getType());

      if (!type || !type.hasStaticShape() ||
          type.getShape() !=
              llvm::ArrayRef<std::int64_t>(buffer.shape) ||
          indexValues.size() != buffer.shape.size()) {
        valid = false;
        return;
      }

      // Element type: integers up to 64 bits in `data`,
      // f32 / f64 in `floatData`.

      Type elementType = type.getElementType();

      const bool floatElement =
          elementType.isF32() || elementType.isF64();

      const bool integerElement =
          elementType.isSignlessInteger() &&
          elementType.getIntOrFloatBitWidth() <= 64;

      if (!(floatElement || integerElement) ||
          buffer.isFloat != floatElement) {
        valid = false;
        return;
      }

      const std::size_t elementCount =
          floatElement ? buffer.floatData.size()
                       : buffer.data.size();

      Point point;
      point.reserve(indexValues.size());

      __int128 linear = 0;
      __int128 extent = 1;

      for (std::size_t d = 0; d < buffer.shape.size(); ++d) {
        const std::int64_t coordinate = indexValues[d];

        if (coordinate < 0 || coordinate >= buffer.shape[d] ||
            buffer.shape[d] <= 0) {
          valid = false;
          return;
        }

        // The running extent never exceeds the buffer
        // size, so neither product below can overflow.

        if (extent > static_cast<__int128>(elementCount) /
                         buffer.shape[d]) {
          valid = false;
          return;
        }

        point.push_back(coordinate);
        linear = linear * buffer.shape[d] + coordinate;
        extent *= buffer.shape[d];
      }

      if (extent != static_cast<__int128>(elementCount) ||
          linear < 0 || linear >= extent) {
        valid = false;
        return;
      }

      const std::size_t offset = static_cast<std::size_t>(linear);

      // An f32 element must be an exactly representable
      // float (NaN excepted).

      auto exactForWidth = [&](double value) {
        return !elementType.isF32() || std::isnan(value) ||
               static_cast<double>(static_cast<float>(value)) == value;
      };

      if (floatElement) {
        if (isLoad) {
          const double loaded = buffer.floatData[offset];

          if (!exactForWidth(loaded)) {
            valid = false;
            return;
          }

          floatValues[loadedResult] = loaded;
          memory->trace.push_back(
              {false, memref, std::move(point), 0, true, loaded});
          return;
        }

        auto stored = lookupFloat(storedValue);

        if (!stored || !exactForWidth(*stored)) {
          valid = false;
          return;
        }

        buffer.floatData[offset] = *stored;
        memory->trace.push_back(
            {true, memref, std::move(point), 0, true, *stored});
        return;
      }

      if (isLoad) {
        const std::int64_t loaded = buffer.data[offset];
        values[loadedResult] = loaded;
        memory->trace.push_back(
            {false, memref, std::move(point), loaded});
        return;
      }

      auto stored = lookup(storedValue);

      if (!stored) {
        valid = false;
        return;
      }

      buffer.data[offset] = *stored;
      memory->trace.push_back(
          {true, memref, std::move(point), *stored});
      return;
    }

    if (op.getNumOperands() == 2 &&
        op.getNumResults() == 1) {

      Type resultType = op.getResult(0).getType();

      // Phase 3b-3: strict IEEE 754 multiplication and
      // addition. Each operation rounds once; volatile
      // temporaries keep the compiler from contracting.

      if (resultType.isF32() || resultType.isF64()) {
        auto floatLhs = lookupFloat(op.getOperand(0));
        auto floatRhs = lookupFloat(op.getOperand(1));

        const bool multiply = isa<arith::MulFOp>(&op);
        const bool add = isa<arith::AddFOp>(&op);

        if (!floatLhs || !floatRhs || !(multiply || add)) {
          valid = false;
          return;
        }

        arith::FastMathFlags flags =
            multiply ? cast<arith::MulFOp>(&op).getFastmath()
                     : cast<arith::AddFOp>(&op).getFastmath();

        const bool roundingMode =
            multiply ? cast<arith::MulFOp>(&op).getRoundingmode().has_value()
                     : cast<arith::AddFOp>(&op).getRoundingmode().has_value();

        if (flags != arith::FastMathFlags::none || roundingMode) {
          valid = false;
          return;
        }

        double rounded = 0.0;

        if (resultType.isF32()) {
          volatile float left = static_cast<float>(*floatLhs);
          volatile float right = static_cast<float>(*floatRhs);
          volatile float out = multiply ? left * right : left + right;
          rounded = static_cast<double>(out);
        } else {
          volatile double left = *floatLhs;
          volatile double right = *floatRhs;
          volatile double out = multiply ? left * right : left + right;
          rounded = out;
        }

        floatValues[op.getResult(0)] = rounded;
        return;
      }

      auto lhs = lookup(op.getOperand(0));
      auto rhs = lookup(op.getOperand(1));

      if (!lhs || !rhs) {
        valid = false;
        return;
      }

      std::optional<std::int64_t> output;

      // Phase 3b-3: addi / muli on integers narrower than
      // 64 bits wrap to their width; 64-bit results stay
      // checked.

      if (auto addi = dyn_cast<arith::AddIOp>(&op);
          addi && addi.getOverflowFlags() !=
                      arith::IntegerOverflowFlags::none) {
        valid = false;
        return;
      }

      if (auto muli = dyn_cast<arith::MulIOp>(&op);
          muli && muli.getOverflowFlags() !=
                      arith::IntegerOverflowFlags::none) {
        valid = false;
        return;
      }

      auto narrowType = dyn_cast<IntegerType>(resultType);

      const bool narrowResult =
          narrowType && narrowType.getWidth() > 1 &&
          narrowType.getWidth() < 64;

      // Only addi / muli are modelled on narrow integers.

      if (narrowResult &&
          !isa<arith::AddIOp, arith::MulIOp>(&op)) {
        valid = false;
        return;
      }

      const bool narrow =
          narrowResult && narrowType.isSignless() &&
          isa<arith::AddIOp, arith::MulIOp>(&op);

      if (narrow) {
        const __int128 exact =
            isa<arith::AddIOp>(&op)
                ? static_cast<__int128>(*lhs) + *rhs
                : static_cast<__int128>(*lhs) * *rhs;

        output = wrapToWidth(exact, narrowType.getWidth());

      } else if (isa<arith::AddIOp>(&op)) {
        output = checked(
            static_cast<__int128>(*lhs) + *rhs);

      } else if (isa<arith::MulIOp>(&op)) {
        output = checked(
            static_cast<__int128>(*lhs) * *rhs);

      } else if (isa<arith::FloorDivSIOp>(&op)) {
        if (*rhs != 0) {
          __int128 a = *lhs;
          __int128 b = *rhs;
          __int128 q = a / b;
          __int128 r = a % b;

          if (r != 0 && ((r < 0) != (b < 0)))
            --q;

          output = checked(q);
        }

      } else if (isa<arith::RemSIOp>(&op)) {
        if (*rhs != 0) {
          output = checked(
              static_cast<__int128>(*lhs) % *rhs);
        }

      } else if (isa<arith::DivSIOp>(&op)) {
        if (*rhs != 0) {
          // Compute in wider arithmetic and reject
          // an unrepresentable signed result.
          output = checked(
              static_cast<__int128>(*lhs) /
              static_cast<__int128>(*rhs));
        }

      } else if (isa<arith::AndIOp>(&op)) {
        output = *lhs & *rhs;

      } else if (isa<arith::OrIOp>(&op)) {
        output = *lhs | *rhs;

      } else if (auto cmp = dyn_cast<arith::CmpIOp>(&op)) {
        bool comparison = false;

        switch (cmp.getPredicate()) {
        case arith::CmpIPredicate::eq:
          comparison = *lhs == *rhs;
          break;
        case arith::CmpIPredicate::ne:
          comparison = *lhs != *rhs;
          break;
        case arith::CmpIPredicate::slt:
          comparison = *lhs < *rhs;
          break;
        case arith::CmpIPredicate::sle:
          comparison = *lhs <= *rhs;
          break;
        case arith::CmpIPredicate::sgt:
          comparison = *lhs > *rhs;
          break;
        case arith::CmpIPredicate::sge:
          comparison = *lhs >= *rhs;
          break;
        default:
          valid = false;
          return;
        }

        output = comparison ? 1 : 0;
      }

      if (!output) {
        valid = false;
        return;
      }

      values[op.getResult(0)] = *output;
      return;
    }

    // Fail closed for unknown MLIR operations.
    valid = false;
  };

  executeOp(*outermost.getOperation());

  if (!valid)
    return std::nullopt;

  return result;
}

} // namespace systolic::logical::testing
