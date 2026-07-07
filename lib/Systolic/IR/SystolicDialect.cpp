#include "Systolic/SystolicDialect.h"
#include "Systolic/SystolicOps.h"

#include "mlir/IR/Builders.h"
#include "mlir/IR/OpImplementation.h"

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
