#include "Systolic/SystolicDialect.h"
#include "Systolic/SystolicOps.h"

#include "mlir/IR/Builders.h"
#include "mlir/IR/OpImplementation.h"

#include <algorithm>
#include <cmath>
#include <utility>

using namespace mlir;
using namespace mlir::systolic;

//===----------------------------------------------------------------------===//
// Dialect 初始化:注册所有 op
//===----------------------------------------------------------------------===//
#include "Systolic/SystolicOpsDialect.cpp.inc"

void SystolicDialect::initialize() {
  addOperations<
#define GET_OP_LIST
#include "Systolic/SystolicOps.cpp.inc"
      >();
}

#include "Systolic/SystolicOpsEnums.cpp.inc"

//===----------------------------------------------------------------------===//
// PEArrayOp 的 verifier
//===----------------------------------------------------------------------===//
LogicalResult PEArrayOp::verify() {
  int64_t rows = static_cast<int64_t>(getRows());
  int64_t cols = static_cast<int64_t>(getCols());

  if (rows <= 0 || cols <= 0)
    return emitOpError("pe_array 的 rows/cols 必须是正数,got ")
           << rows << "x" << cols;

  auto stationaryTy =
      llvm::cast<RankedTensorType>(getStationaryOperand().getType());
  auto movingTy = llvm::cast<RankedTensorType>(getMovingOperand().getType());
  if (stationaryTy.getRank() != 2 || movingTy.getRank() != 2)
    return emitOpError("stationary/moving operand 目前只支援 rank-2 (matmul) 的情况");

  // weight-stationary 时:
  //   weight (stationary_operand) 形状是 [K, N] —— K 是化约(时间)维度,
  //     N 是空间维度,应该等于 cols。
  //   moving_operand(A,经过 stream)形状是 [M, K] —— M 是空间维度,
  //     应该等于 rows;K 要跟 weight 的 K 对得上。
  if (getStationary() == StationaryKind::weight) {
    int64_t kFromWeight = stationaryTy.getShape()[0];
    int64_t nFromWeight = stationaryTy.getShape()[1];
    int64_t mFromMoving = movingTy.getShape()[0];
    int64_t kFromMoving = movingTy.getShape()[1];

    if (kFromWeight != kFromMoving)
      return emitOpError("weight 跟 moving operand 的化约维度 (K) 对不上: ")
             << "weight 的 K=" << kFromWeight << ", moving 的 K="
             << kFromMoving;
    if (mFromMoving != rows)
      return emitOpError("moving operand 的第 0 维 (M) 应该等于 rows,got M=")
             << mFromMoving << ", rows=" << rows;
    if (nFromWeight != cols)
      return emitOpError("weight 的第 1 维 (N) 应该等于 cols,got N=")
             << nFromWeight << ", cols=" << cols;
  }

  // acc_operand(如果有给)形状应该是 [rows, cols],跟 result 一致。
  if (Value acc = getAccOperand()) {
    auto accTy = llvm::cast<RankedTensorType>(acc.getType());
    if (accTy.getRank() != 2 || accTy.getShape()[0] != rows ||
        accTy.getShape()[1] != cols)
      return emitOpError("acc operand 形状应该是 [rows, cols] = [")
             << rows << ", " << cols << "]";
  }

  return success();
}

//===----------------------------------------------------------------------===//
// StreamOp 的 verifier
//===----------------------------------------------------------------------===//
LogicalResult StreamOp::verify() {
  if (static_cast<int64_t>(getSkew()) < 0)
    return emitOpError("skew 不能是负数,got ") << getSkew();

  if (getData().getType() != getResult().getType())
    return emitOpError("data 跟 result 的型别必须一致,got ")
           << getData().getType() << " vs " << getResult().getType();

  return success();
}

// 第二次 include 同一个 .cpp.inc,这次开 GET_OP_CLASSES,产生
// MacOp::print()/parse()/verifyInvariantsImpl() 这些实际函式定义
#define GET_OP_CLASSES
#include "Systolic/SystolicOps.cpp.inc"

//===----------------------------------------------------------------------===//
// DeviceOp verifier
//===----------------------------------------------------------------------===//

// k_max is a reduction capacity: every consumer divides by it (the cost
// model, the K-folding of the binary-search pass, the FPGA lowering). A
// non-positive value used to be caught by some of them and silently costed
// as zero cycles by others, so it is rejected once, here.
::mlir::LogicalResult mlir::systolic::DeviceOp::verify() {
  if (::mlir::IntegerAttr kMax = getKMaxAttr())
    if (kMax.getInt() < 1)
      return emitOpError("k_max must be a positive integer, got ")
             << kMax.getInt();

  // The read / write terms of the cost model (CostModel.h). A bandwidth is
  // a divisor, a lane count or window a multiplier of the bytes moved, and
  // a negative overhead would make a transfer cheaper than free.
  for (auto [name, attr] :
       {std::make_pair("read_bytes_per_cycle", getReadBytesPerCycleAttr()),
        std::make_pair("write_bytes_per_cycle", getWriteBytesPerCycleAttr())})
    if (attr) {
      const double v = attr.getValueAsDouble();
      if (!std::isfinite(v) || v <= 0.0)
        return emitOpError() << name << " must be positive and finite, got "
                             << v;
    }
  for (auto [name, attr] :
       {std::make_pair("read_overhead", getReadOverheadAttr()),
        std::make_pair("write_overhead", getWriteOverheadAttr())})
    if (attr && attr.getInt() < 0)
      return emitOpError() << name << " must be non-negative, got "
                           << attr.getInt();
  if (::mlir::IntegerAttr window = getKWindowAttr())
    if (window.getInt() < 1)
      return emitOpError("k_window must be a positive integer, got ")
             << window.getInt();
  if (::mlir::IntegerAttr lanes = getOperandLanesAttr()) {
    const int64_t widest = std::max<int64_t>(getRows(), getCols());
    if (lanes.getInt() < widest)
      return emitOpError("operand_lanes must cover the array's ")
             << widest << " lanes, got " << lanes.getInt();
  }

  // The pipelined per-accelerator timeline (CostModel.h). The three
  // attributes describe one piece of hardware -- how deep the context's job
  // queue is, which part of tile_overhead is the serial reduction, and the
  // idle gap the PE needs between two invocations' operands -- so a device
  // declares all of them or none: a depth without a reduce term would overlap
  // the whole of H, which the PE cannot do, and a reduce term without a depth
  // would describe a pipeline the context does not have.
  ::mlir::IntegerAttr depth = getPipelineDepthAttr();
  ::mlir::IntegerAttr reduce = getReduceCyclesAttr();
  ::mlir::IntegerAttr gap = getFeedGapAttr();
  if (depth || reduce || gap) {
    if (!depth || !reduce || !gap)
      return emitOpError("pipeline_depth, reduce_cycles and feed_gap "
                         "describe one pipelined accelerator and must be "
                         "declared together");
    if (depth.getInt() < 1)
      return emitOpError("pipeline_depth must be a positive integer, got ")
             << depth.getInt();
    if (reduce.getInt() < 0)
      return emitOpError("reduce_cycles must be non-negative, got ")
             << reduce.getInt();
    if (gap.getInt() < 0)
      return emitOpError("feed_gap must be non-negative, got ") << gap.getInt();
    const int64_t overhead =
        getTileOverheadAttr() ? getTileOverheadAttr().getInt() : 0;
    if (reduce.getInt() > overhead)
      return emitOpError("reduce_cycles is the serial part of tile_overhead "
                         "and cannot exceed it: ")
             << reduce.getInt() << " > " << overhead;
  }
  return ::mlir::success();
}

//===----------------------------------------------------------------------===//
// MatmulTileOp verifier
//===----------------------------------------------------------------------===//

::mlir::LogicalResult mlir::systolic::MatmulTileOp::verify() {
  auto aTy = ::llvm::dyn_cast<::mlir::RankedTensorType>(getA().getType());
  auto bTy = ::llvm::dyn_cast<::mlir::RankedTensorType>(getB().getType());
  auto cTy = ::llvm::dyn_cast<::mlir::RankedTensorType>(getCIn().getType());
  if (!aTy || !bTy || !cTy)
    return emitOpError("operands must be ranked tensors");
  if (aTy.getRank() != 2 || bTy.getRank() != 2 || cTy.getRank() != 2)
    return emitOpError("operands must be rank-2 tensors");

  const int64_t m = static_cast<int64_t>(getM());
  const int64_t n = static_cast<int64_t>(getN());
  const int64_t k = static_cast<int64_t>(getK());

  if (aTy.getShape()[0] != m || aTy.getShape()[1] != k)
    return emitOpError("operand 'a' shape does not match (m, k)");
  if (bTy.getShape()[0] != k || bTy.getShape()[1] != n)
    return emitOpError("operand 'b' shape does not match (k, n)");
  if (cTy.getShape()[0] != m || cTy.getShape()[1] != n)
    return emitOpError("operand 'c_in' shape does not match (m, n)");
  if (aTy.getElementType() != bTy.getElementType() ||
      aTy.getElementType() != cTy.getElementType())
    return emitOpError("all operands must have the same element type");

  return ::mlir::success();
}
