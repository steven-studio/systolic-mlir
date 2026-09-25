#ifndef SYSTOLIC_AFFINE_MATMUL_ANALYSIS_H
#define SYSTOLIC_AFFINE_MATMUL_ANALYSIS_H

#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/IR/AffineMap.h"
#include "mlir/IR/Operation.h"
#include "mlir/IR/Value.h"
#include "mlir/Support/LogicalResult.h"

#include "llvm/ADT/SmallVector.h"

namespace mlir {
namespace systolic {

/// A candidate Affine loop nest that may implement a matrix multiplication.
/// Discovery is intentionally separate from semantic recognition.
struct AffineMatmulCandidate {
  Operation *anchor = nullptr;
};

/// Logical matrix multiplication recovered from an Affine computation.
///
/// The original iteration vector is x.  Recognition attempts to recover
/// logical coordinates
///
///   P(x) : partition / batch coordinates
///   I(x) : output-row coordinate
///   J(x) : output-column coordinate
///   K(x) : reduction coordinate
///
/// together with the original operand access functions F_A/F_B/F_C and
/// factor maps phi_A/phi_B/phi_C.
struct LogicalExtent {
  enum class Kind {
    Unknown,
    Static,
    Symbolic,
  };

  Kind kind = Kind::Unknown;
  int64_t staticValue = 0;
  Value symbolicValue;
};

struct LogicalMatmul {
  Operation *anchor = nullptr;

  Value lhs;
  Value rhs;
  Value output;

  // Original Affine access functions F_A(x), F_B(x), F_C(x).
  AffineMap accessA;
  AffineMap accessB;
  AffineMap accessC;

  // Recovered logical coordinates.
  AffineMap partitionMap;
  AffineMap rowMap;
  AffineMap columnMap;
  AffineMap reductionMap;

  // Logical problem extents.  Extent recovery is deliberately separate from
  // GEMM recognition: an unknown extent does not make recognition fail.
  LogicalExtent rowExtent;
  LogicalExtent columnExtent;
  LogicalExtent reductionExtent;

  // Factor maps:
  //
  //   F_A = phiA o (P, I, K)
  //   F_B = phiB o (P, K, J)
  //   F_C = phiC o (P, I, J)
  AffineMap phiA;
  AffineMap phiB;
  AffineMap phiC;
};

/// Find Affine loop nests worth attempting to recognize.
///
/// This function does not claim that a candidate is a GEMM.
void collectAffineMatmulCandidates(
    Operation *root,
    SmallVectorImpl<AffineMatmulCandidate> &candidates);

/// Recover logical GEMM coordinates and access factorization.
///
/// Failure means "not recognized"; it is not a compiler error.
FailureOr<LogicalMatmul>
recognizeLogicalMatmul(AffineMatmulCandidate candidate);

} // namespace systolic
} // namespace mlir

#endif // SYSTOLIC_AFFINE_MATMUL_ANALYSIS_H
