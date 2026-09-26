#ifndef SYSTOLIC_AFFINE_MATMUL_ANALYSIS_H
#define SYSTOLIC_AFFINE_MATMUL_ANALYSIS_H

#include "mlir/Analysis/Presburger/IntegerRelation.h"
#include "mlir/Dialect/Affine/Analysis/AffineStructures.h"
#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/IR/AffineMap.h"
#include "mlir/IR/Operation.h"
#include "mlir/IR/Value.h"
#include "mlir/Support/LogicalResult.h"

#include "llvm/ADT/SmallVector.h"

#include <optional>

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

struct AffineLogicalMapping {
  // y = T x + c, where y = (P*, I, J, K).
  //
  // transformation[row][column] is the coefficient multiplying x[column]
  // in logical coordinate y[row].
  SmallVector<SmallVector<int64_t>> transformation;
  SmallVector<int64_t> offset;

  // Static lower bounds x0 of the source iteration coordinates.
  // Together with T and c, this permits zero-origin normalization
  // without losing the original affine access mapping y = T x + c.
  SmallVector<int64_t> sourceOrigin;
  bool hasStaticSourceOrigin = false;

  // True when T is square and full-rank, so the source iteration
  // coordinates can be recovered from the logical coordinates over Q.
  bool isInvertible = false;
};

struct LogicalMatmul {
  Operation *anchor = nullptr;
  Operation *outputStore = nullptr;

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

  // Affine coordinate transformation y = T x + c for
  // y = (P*, I, J, K).
  AffineLogicalMapping logicalMapping;

  // Exact source iteration domain D_S over the original iteration
  // coordinates x, in the same order as the columns of logicalMapping.
  std::optional<affine::FlatAffineValueConstraints> sourceDomain;

  // Exact integer logical iteration domain:
  //
  //   D_L = { y in Z^m | exists x in D_S : y = T x + c }.
  //
  // Source iteration coordinates may remain as existential local variables,
  // preserving lattice information for non-unimodular mappings.
  std::optional<presburger::IntegerPolyhedron> logicalDomain;

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
