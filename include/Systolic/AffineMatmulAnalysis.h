#ifndef SYSTOLIC_AFFINE_MATMUL_ANALYSIS_H
#define SYSTOLIC_AFFINE_MATMUL_ANALYSIS_H

#include "mlir/Analysis/Presburger/IntegerRelation.h"
#include "mlir/Dialect/Affine/Analysis/AffineStructures.h"
#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/IR/AffineMap.h"
#include "mlir/IR/Operation.h"
#include "mlir/IR/Value.h"
#include "mlir/Support/LogicalResult.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"

#include <array>
#include <optional>
#include <string>

namespace mlir {
namespace systolic {

/// The classification of a candidate nest (docs/AFFINE_GEMM_MAPPING.md §9).
/// UNKNOWN is never treated as valid; a valid mapping is never called invalid
/// because a lowering component is missing.
enum class GemmMappingClass {
  /// A valid mapping exists and its views are representable.
  ProvenValid,
  /// A valid mathematical mapping exists, but a lowering component is missing.
  ValidButLoweringUnsupported,
  /// No valid mapping exists (the violated condition is in the reason).
  ProvenInvalid,
  /// Correctness cannot be established by the implemented analysis.
  Unknown,
};

inline const char *stringifyGemmMappingClass(GemmMappingClass value) {
  switch (value) {
  case GemmMappingClass::ProvenValid:
    return "PROVEN_VALID";
  case GemmMappingClass::ValidButLoweringUnsupported:
    return "VALID_BUT_LOWERING_UNSUPPORTED";
  case GemmMappingClass::ProvenInvalid:
    return "PROVEN_INVALID";
  case GemmMappingClass::Unknown:
    return "UNKNOWN";
  }
  return "UNKNOWN";
}

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

/// An operand's own view of a logical coordinate y: along that coordinate the
/// operand is subscripted by scale * y + offset.  (1, 0) when the operand's
/// subscript is the coordinate's expression itself; otherwise the operand
/// spells the shared coordinate differently from the other operand (C[i + 8][j]
/// with A[i][k]: C's row view has offset 8; C[2i][j] with A[i][k]: scale 2) and
/// the coordinate is the linear form both subscripts are affine images of.
///
/// The scale may be negative: the operand reads the coordinate backwards (a
/// negative-stride view, docs/AFFINE_GEMM_MAPPING.md §5); never zero.
struct OperandCoordinateView {
  int64_t scale = 1;
  int64_t offset = 0;
};

/// Where an operand's memref dimensions sit in the GEMM
/// (docs/AFFINE_GEMM_MAPPING.md §5). The operand's two GEMM roles, in GEMM
/// order -- A: (I, K), B: (K, J), C: (I, J) -- are read in dimensions
/// roleDim[0] and roleDim[1] of the memref, in either order (roleDim[0] >
/// roleDim[1] is a transposed operand), or roleDim[r] = -1 when the operand
/// has no dimension for role r (the role has extent 1 and the operand's rank
/// is lower). Every other dimension d has a fixed index constantIndex[d]: an
/// extent-one dimension, or a batch dimension (below).
///
/// Batch loops (docs/AFFINE_GEMM_MAPPING.md sections 15 and 16) are the loops
/// all three operands read. Dimension d of the operand is read at its role
/// view's index (role dimensions) or at constantIndex[d] (the others), plus
/// sum over the batch loops s of batchCoefficients[d][s] * x_s, x_s the batch
/// loop's counter: an affine offset per batch value. A batch dimension
/// (batchDim) reads batch loops only; a factor may also add batch terms to a
/// role dimension (A[4i - j][5k]: the row view moves with j), C never does.
struct OperandLayout {
  std::array<int, 2> roleDim{-1, -1};
  SmallVector<std::optional<int64_t>> constantIndex;
  // The dimensions that read batch loops only, in increasing order -- for C
  // one per batch coordinate. Their static index is constantIndex.
  SmallVector<int, 2> batchDim;
  // Per memref dimension, the coefficient of each loop of the nest (in the
  // order of the logical mapping's columns) that is a batch loop; zero for
  // every other loop. Empty when the operand reads no batch loop.
  SmallVector<SmallVector<int64_t>> batchCoefficients;

  bool hasBatchTerms(unsigned dim) const {
    return dim < batchCoefficients.size() &&
           llvm::any_of(batchCoefficients[dim],
                        [](int64_t c) { return c != 0; });
  }

  bool isTransposed() const {
    return roleDim[0] >= 0 && roleDim[1] >= 0 && roleDim[0] > roleDim[1];
  }
};

struct AffineLogicalNormalization {
  // Dense normalized coordinates z represent the original execution
  // coordinates after removing their static origins.
  //
  //   x = S z + sourceOrigin
  //
  // The logical coordinates remain:
  //
  //   y = T x + c
  //
  // so downstream passes never need to pretend that T is identity.
  SmallVector<SmallVector<int64_t>> sourceTransformation;
  SmallVector<int64_t> sourceOffset;

  // Composed logical coordinates:
  //
  //   y = logicalTransformation * z + logicalOffset
  //
  // after composing the source-loop normalization with the recovered
  // logical mapping.
  SmallVector<SmallVector<int64_t>> logicalTransformation;
  SmallVector<int64_t> logicalOffset;

  // Exact normalized execution domain.
  //
  // This is deliberately kept separate from LogicalMatmul::logicalDomain:
  // logicalDomain describes y, while this domain describes normalized z.
  std::optional<affine::FlatAffineValueConstraints> domain;

  // True iff normalization is just x = z.
  bool isIdentity = false;
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

  // Normalized dense logical coordinates used by later tiling/lowering.
  AffineLogicalNormalization normalization;

  // True when no normalization could be computed because a checked
  // product, sum or trip count of it does not fit in 64 bits (and only then;
  // a bound that is not one constant leaves it false). getDenseLogicalGemm
  // reports the refusal as an overflow on this flag alone.
  bool normalizationOverflow = false;

  // True when no normalization could be computed because a loop of the nest
  // has constant bounds and no iteration (upper <= lower): an empty GEMM,
  // M, N or K = 0 (spec question SQ-2). Reported as such, not as "not static".
  bool normalizationEmptyLoop = false;

  // When no normalization could be computed because a loop bound is not one
  // constant: which bound, and why it is not (a symbol, a bound that depends
  // on another loop's counter -- the domain is not a box -- or the min/max of
  // non-constant values). Empty otherwise.
  std::string normalizationDetail;

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

  // Factor maps over the logical coordinates y = (P*, I, J, K):
  //
  //   F_A = phiA o (P, I, K)
  //   F_B = phiB o (P, K, J)
  //   F_C = phiC o (P, I, J)
  //
  // i.e. phiA(P*, I, J, K) = (P*, viewA[0](I), viewA[1](K)) and so on.
  AffineMap phiA;
  AffineMap phiB;
  AffineMap phiC;

  // Each operand's view of its two non-partition coordinates, in the
  // operand's subscript order: A = (I, K), B = (K, J), C = (I, J).  The
  // logical coordinate is one and the same for the two operands that share
  // it; the views are what differ when the operands spell it differently.
  std::array<OperandCoordinateView, 2> viewA;
  std::array<OperandCoordinateView, 2> viewB;
  std::array<OperandCoordinateView, 2> viewC;

  // Which memref dimension of each operand reads which role, and the fixed
  // index of every other dimension (OperandLayout). For the canonical rank-2
  // GEMM: roleDim = {0, 1}, no constant dimension.
  OperandLayout layoutA;
  OperandLayout layoutB;
  OperandLayout layoutC;

  // True when the accumulation `arith.addf` carries the `reassoc` fast-math
  // flag: the program permits reassociating the reduction
  // (docs/AFFINE_GEMM_MAPPING.md §6). The systolic execution sums each tile
  // from zero and adds the sum to C, so it is offloaded only with this
  // permission or the pass's allow-fp-reassociation option.
  bool reductionMayReassociate = false;
};

/// Find Affine loop nests worth attempting to recognize.
///
/// This function does not claim that a candidate is a GEMM.
void collectAffineMatmulCandidates(
    Operation *root,
    SmallVectorImpl<AffineMatmulCandidate> &candidates);

/// Recover logical GEMM coordinates and access factorization.
///
/// Failure means "not recognized"; it is not a compiler error. On failure,
/// `why` (when given) receives the reason. The subscripts are compared as
/// functions on the iteration domain, not as expressions: a loop that runs
/// exactly once contributes a constant (docs/AFFINE_GEMM_MAPPING.md, §1, §4).
FailureOr<LogicalMatmul>
recognizeLogicalMatmul(AffineMatmulCandidate candidate,
                       std::string *why = nullptr,
                       GemmMappingClass *mappingClass = nullptr);

} // namespace systolic
} // namespace mlir

#endif // SYSTOLIC_AFFINE_MATMUL_ANALYSIS_H
