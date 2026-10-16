#ifndef SYSTOLIC_AFFINE_MATMUL_ANALYSIS_H
#define SYSTOLIC_AFFINE_MATMUL_ANALYSIS_H

#include "Systolic/IntegerLattice.h"

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

  /// A general affine view (docs/AFFINE_GEMM_MAPPING.md section 19), set by
  /// the readers analysis: per memref dimension d, the coefficients of the
  /// operand's two roles (in GEMM order: A (I, K), B (K, J), C (I, J)) and
  /// the constant -- the index is
  ///   dimConstants[d] + roleCoefficients[d][0] * y_0
  ///                   + roleCoefficients[d][1] * y_1 + the batch terms,
  /// y_r the value of the role's logical coordinate. Empty for the layouts of
  /// the positional and group analyses (one role per dimension, roleDim).
  SmallVector<std::array<int64_t, 2>> roleCoefficients;
  SmallVector<int64_t> dimConstants;
  /// A dimension combines both roles, or a role spans several dimensions: a
  /// skewed or merged view (A[i][i + k], C[4 i + j]), which no subview of the
  /// memref's own dimensions expresses. roleDim is then {-1, -1}.
  bool general = false;
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

/// The geometric screening of a multiply-accumulate nest
/// (docs/AFFINE_GEMM_MAPPING.md section 19, layer 1): the linear algebra of
/// its accesses on the integer lattice of its iterations. NECESSARY
/// conditions only -- nothing here decides that the nest is a GEMM; that is
/// recognizeLogicalMatmul's job, with the domain, the role decomposition,
/// C's injectivity per batch and the bounds. The discovery test pass prints
/// it (geometry=true).
///
/// All coefficients are on the loops' ordinals z_s (x_s = lower_s + step_s
/// z_s): an access coefficient times the loop's step, so that a direction is
/// a step of the actual iteration lattice. A loop of one iteration is a
/// constant on the domain (Lemma 0) and takes no part.
struct AffineGeometryScreening {
  /// The operands as the product writes them: 0 and 1 the factors, 2 C.
  enum Operand : unsigned { First = 0, Second = 1, Output = 2 };
  struct Loop {
    int64_t trip = 0;
    /// Bit X set iff some subscript of operand X has a nonzero ordinal
    /// coefficient on the loop (only for loops of two or more iterations).
    unsigned readers = 0;
    /// The loop's class by its readers: 'I' (first factor and C), 'J'
    /// (second factor and C), 'K' (both factors), 'P' (all three: a batch
    /// loop), '1' (one reader: a broadcast), '-' (no reader, or one
    /// iteration).
    char role = '-';
  };
  SmallVector<Loop> loops;
  /// Per operand, per memref dimension, the ordinal coefficients.
  std::array<lattice::IntMatrix, 3> ordinal;

  /// Ranks over Q of C's subscripts and of all subscripts, on every loop of
  /// two or more iterations ('full') and on the role loops only ('roles':
  /// the batch loops removed -- section 19, the role space).
  unsigned cRankFull = 0, cRankRoles = 0;
  unsigned allRankFull = 0, allRankRoles = 0;
  /// A basis (HNF) of the integer kernel of C's subscripts on the loops of
  /// two or more iterations, as full-length ordinal vectors: the directions
  /// along which C stays fixed.
  lattice::IntMatrix cKernel;
  /// The kernel basis vectors that join two points of the box (|delta_s| <
  /// trip_s) and are not combinations of reduction (K) loops: two
  /// iterations writing one C element that are not one reduction.
  lattice::IntMatrix cKernelBeyondReduction;
  /// The reduction loops ('K'): the axes of the reduction lattice.
  SmallVector<unsigned> reductionLoops;

  /// The six-vector conditions of the task statement, for operands of two
  /// dimensions each read as stored -- Ic, Jc = C's dimensions, Ia, Ka = the
  /// first factor's, Kb, Jb = the second's:
  ///   a: rank(cI, cJ) = 2;  b: rank of the six = 3;
  ///   c: some d with cI.d = cJ.d = 0, aK.d != 0, bK.d != 0 (the witness);
  ///   d1, d2, d3: rank(aI, aK) = 2, rank(bK, bJ) = 2, rank(aI, bJ) = 2.
  struct SixVectorConditions {
    bool a = false, b = false, c = false, d1 = false, d2 = false, d3 = false;
    std::optional<lattice::IntVector> reductionDirection;
  };
  std::optional<SixVectorConditions> conditionsFull, conditionsRoles;
};

FailureOr<AffineGeometryScreening>
screenAffineMatmulGeometry(AffineMatmulCandidate candidate,
                           std::string *why = nullptr);

} // namespace systolic
} // namespace mlir

#endif // SYSTOLIC_AFFINE_MATMUL_ANALYSIS_H
