#pragma once

// Phase 5.4.3d-2c-1
//
// Test-only interpreter for the generated MLIR subset.
//
// Supported:
//   affine.for with constant bounds
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
//
// Unsupported operations cause failure.
//
// This is NOT LLVM JIT execution.

#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/IR/Operation.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/BuiltinTypes.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMap.h"

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
  std::vector<std::int64_t> data;
};

struct MemoryEvent {
  bool isStore = false;
  mlir::Value memref;
  Point indices;
  std::int64_t value = 0;
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

  if (!outermost || !guardedIf || logicalIVs.empty())
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

  auto lookup = [&](Value value)
      -> std::optional<std::int64_t> {
    auto it = values.find(value);
    if (it == values.end())
      return std::nullopt;
    return it->second;
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
      if (!loop.hasConstantBounds()) {
        valid = false;
        return;
      }

      const std::int64_t lower = loop.getConstantLowerBound();
      const std::int64_t upper = loop.getConstantUpperBound();
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

    if (isa<memref::LoadOp, memref::StoreOp>(&op)) {

      // Memory operations are interpreted only against
      // an explicit memory model.

      if (!memory) {
        valid = false;
        return;
      }

      auto load = dyn_cast<memref::LoadOp>(&op);
      auto store = dyn_cast<memref::StoreOp>(&op);

      Value memref = load ? load.getMemRef() : store.getMemRef();
      ValueRange indices =
          load ? load.getIndices() : store.getIndices();

      auto bufferIt = memory->buffers.find(memref);

      if (bufferIt == memory->buffers.end()) {
        valid = false;
        return;
      }

      MemoryBuffer &buffer = bufferIt->second;

      auto type = dyn_cast<MemRefType>(memref.getType());

      if (!type || !type.hasStaticShape() ||
          !type.getElementType().isInteger(64) ||
          type.getShape() !=
              llvm::ArrayRef<std::int64_t>(buffer.shape) ||
          indices.size() != buffer.shape.size()) {
        valid = false;
        return;
      }

      Point point;
      point.reserve(indices.size());

      __int128 linear = 0;
      __int128 extent = 1;

      for (std::size_t d = 0; d < buffer.shape.size(); ++d) {
        auto coordinate = lookup(indices[d]);

        if (!coordinate || *coordinate < 0 ||
            *coordinate >= buffer.shape[d] ||
            buffer.shape[d] <= 0) {
          valid = false;
          return;
        }

        // The running extent never exceeds the buffer
        // size, so neither product below can overflow.

        if (extent > static_cast<__int128>(buffer.data.size()) /
                         buffer.shape[d]) {
          valid = false;
          return;
        }

        point.push_back(*coordinate);
        linear = linear * buffer.shape[d] + *coordinate;
        extent *= buffer.shape[d];
      }

      if (extent != static_cast<__int128>(buffer.data.size()) ||
          linear < 0 || linear >= extent) {
        valid = false;
        return;
      }

      const std::size_t offset = static_cast<std::size_t>(linear);

      if (load) {
        const std::int64_t loaded = buffer.data[offset];
        values[load.getResult()] = loaded;
        memory->trace.push_back(
            {false, memref, std::move(point), loaded});
        return;
      }

      auto stored = lookup(store.getValue());

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

      auto lhs = lookup(op.getOperand(0));
      auto rhs = lookup(op.getOperand(1));

      if (!lhs || !rhs) {
        valid = false;
        return;
      }

      std::optional<std::int64_t> output;

      if (isa<arith::AddIOp>(&op)) {
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
