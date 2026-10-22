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

/// The exact mapping of the task of 2026-10-17 (its Parts 2-6), computed for
/// the non-degenerate three-loop case and for the degenerate K = 1 case, and
/// REPORTED (the discovery pass prints it; nothing is decided from it -- the
/// decision is the role mapping of section 19.2, which is more general: C may
/// be a skewed view, a role may span several loops, batch coordinates). In
/// the task's convention the logical coordinates are C's own indices,
/// I = Ic(x), J = Jc(x), and K the reduction row q of the recognizer:
///   y = T x + d,  T = [cI; cJ; q] over the loops of two or more iterations,
///   lambda = r T^-1 for each index row r of A and B (exact rationals),
/// det T, whether T is unimodular, whether T diag(step) is a signed
/// permutation (then T(D) + d is a complete integer box), the primitive kernel
/// v of [cI; cJ] with the reductions' variation Ia.v, Ka.v, Kb.v, Jb.v, and
/// the role restrictions (A free of J, B free of I). K = 1 (no reduction
/// loop) is the documented degenerate path: a 2-D mapping [cI; cJ], no
/// reduction direction, the non-degenerate rule not applied (section 19.12).
struct ExactMapping {
  bool defined = false;
  std::string note;
  unsigned dims = 0;
  bool degenerateK = false;
  SmallVector<SmallVector<int64_t>> T;
  SmallVector<int64_t> d;
  int64_t det = 0;
  bool singular = false;
  bool unimodular = false;
  bool boxImage = false;
  SmallVector<int64_t> v;
  std::array<int64_t, 4> deltas{0, 0, 0, 0};
  bool aVaries = false, bVaries = false;
  SmallVector<lattice::RationalRow> lambdaA, lambdaB;
  bool aFreeOfJ = false, bFreeOfI = false;
  unsigned rankA = 0, rankB = 0, rankC = 0;
  /// The recognizer took the second factor as the row operand (C = B' A').
  bool rowsSwapped = false;
};

/// Loop tiling as written (docs/AFFINE_GEMM_MAPPING.md section 26, S5.1): the
/// tile loops of a nest the program has already tiled, their point loops, the
/// tile sizes and the loop order -- RECORDED from the bounds as the program
/// writes them (before normalizeParallelotopeDomains rewrites a band), and
/// reported by the discovery pass. Nothing is decided from it: recognition,
/// the mapping and the lowering are those of the nest without this record,
/// and that the tiles cover the iteration space exactly once is not proven
/// here.
struct LoopTilingStructure {
  /// A loop's part in the tiling.
  enum class LoopRole { Plain, Tile, Point };
  /// How the program spells a tiling: a stepped tile loop whose counter only
  /// bounds its point loop (for t += S: for p in [t + c, t + c + S), or in
  /// [t + c, min(t + c + S, E))), or two box loops an access combines into one
  /// coordinate (y = S * t + p + c0, p in [0, S)).
  enum class Form { Bounds, Counter };
  /// The last tile: the extent is a multiple of the tile size (Divisible),
  /// the point loop's min clips the last tile (MinClipped), or, with no min,
  /// the last tile runs past the tile loop's upper bound (Unaligned).
  enum class Boundary { Divisible, MinClipped, Unaligned };
  struct Tiling {
    /// Band positions, outermost 0.
    unsigned tileLoop = 0;
    unsigned pointLoop = 0;
    int64_t tileSize = 0;
    Form form = Form::Bounds;
    Boundary boundary = Boundary::Divisible;
    /// The values the point counter (Bounds) or the coordinate (Counter)
    /// takes as written, [lower, upper), and the size of the last tile.
    int64_t lower = 0, upper = 0, lastTile = 0;
    /// The logical coordinate the point loop drives (0 = I, 1 = J, 2 = K),
    /// or -1: no recognized GEMM, or no single coordinate.
    int coordinate = -1;
    /// Bounds form: the point loop's lower bound is the tile counter plus
    /// this constant; the tile counter's value at the last tile.
    int64_t pointOffset = 0;
    int64_t lastTileCounter = 0;
    /// Bounds form: the values the point loop runs per tile as written, and
    /// the constant of its min, if any.
    int64_t width = 0;
    std::optional<int64_t> clip;
    /// The boundary tiles as proven by verifyLoopTilingCoverage (Bounds form):
    /// which tile counter values run fewer than tileSize points.
    std::string boundaryProof;
    bool boundaryProven = false;
  };
  /// The exact coverage of the iteration space (section 26.2): proven with
  /// the Presburger library on the band's exact iteration set as written --
  /// every point of the logical box run by exactly one iteration, none
  /// outside it -- or the counterexamples (lexicographically least points).
  struct Coverage {
    /// One logical dimension: the loop that runs it (a point loop, or an
    /// untiled loop) and the values it takes, [lower, upper) in steps of step.
    struct Dim {
      unsigned loop = 0;
      int coordinate = -1;
      int64_t lower = 0, upper = 0, step = 1;
    };
    bool decided = false;
    bool exact = false;
    /// Why the proof did not run (decided false).
    std::string note;
    /// Counterexamples, when not exact.
    std::optional<std::string> overlap, missing, outside;
    /// The logical iteration space, in the written order of its loops.
    SmallVector<Dim> dims;
  };
  /// The band's outermost loop (the same op after the normalization).
  Operation *outermost = nullptr;
  /// One per loop of the band, outermost first: the written loop order.
  SmallVector<LoopRole> roles;
  /// Per loop, the logical coordinate its counter drives (-1: none known).
  SmallVector<int> loopCoordinate;
  SmallVector<Tiling> tilings;
  /// Loops shaped like a tile loop that are not one, and why.
  SmallVector<std::string> rejected;
  /// Of those, the pairings whose bounds have the tiling's shape but whose
  /// numbers are not a tiling's (a point width other than the step, a min
  /// outside the last tile): their intended range is [lower, upper) with the
  /// tile step, and their coverage is decided with the tilings' (section
  /// 26.2), so that an overlap or a hole is shown by a counterexample.
  SmallVector<Tiling> candidates;
  /// The band's exact iteration set over its counters as written (one local
  /// per stepped loop), taken before any normalization; nullopt when it is
  /// not available (exactIterationSet).
  std::optional<presburger::IntegerPolyhedron> writtenDomain;
  /// Each loop's constant bounds as written, when they are constants.
  SmallVector<std::optional<std::pair<int64_t, int64_t>>> writtenRange;
  Coverage coverage;
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
  // permission or the pass's allow-fp-reassociation option. For an
  // assignment (below) the flag is read from the product's `arith.mulf`, the
  // nest's only floating-point operation.
  bool reductionMayReassociate = false;

  // True for C = C + A * B (the accumulation into C's prior value). False for
  // the assignment C = A * B: no accumulator load, every element of C written
  // exactly once on the iteration domain (the recognizer refuses a loop of two
  // or more iterations that C does not read; the lowering proves C's index
  // map injective on the whole domain), so the nest is the K = 1 GEMM into a
  // zeroed C, C = 0 + A * B, and is lowered as that: C's logical window is
  // zeroed before the tiles accumulate into it (docs/AFFINE_GEMM_MAPPING.md
  // §20).
  bool accumulates = true;

  // Dimension-intersection partitioning (docs/AFFINE_GEMM_MAPPING.md §21):
  // the loops (iteration dimensions, in the order of logicalMapping's
  // columns) the recognizer made batch loops beyond those read by all three
  // operands -- a loop read by C only, or a loop of a row or column role that
  // was not one dense coordinate (C[p][i][j] += A[p][i][k] * B[k][j]: p).
  // Each partitions the GEMM into independent slabs of C; the factor that
  // does not read it is shared by the partitions. Empty for a plain or a
  // canonically batched GEMM. Diagnostics only: the batch machinery treats
  // them as any other batch loop.
  SmallVector<unsigned> demotedLoops;
  // The demoted loops C does not read (reduction-batch partitioning, docs
  // section 23): reduction batch loops. For each of their values the nest is
  // a GEMM whose factor views are offset by them; all their values
  // accumulate into the same slab of C, in order. A subset of demotedLoops;
  // empty when the recognition ran without reductionBatches.
  SmallVector<unsigned> reductionBatchLoops;

  // The task's exact mapping (reported by the discovery pass, not decided).
  ExactMapping exactMapping;

  // The run-time parts of the access functions. An access may add affine
  // symbols of the nest (a function's integer parameters: C[r0 + i][j]) to
  // its indices; those terms are separated off before recognition, so that
  // everything above is over the loops alone, and kept here: per operand, a
  // map of no dims and the access's symbols whose result r is the
  // symbol-only part of memref index r (the constant 0 where there is
  // none), with the symbol operands. The view of the operand the nest
  // touches is offset by these at run time (a "dynamic offset"). A null map
  // when the operand's access has no symbol.
  AffineMap symbolicA;
  AffineMap symbolicB;
  AffineMap symbolicC;
  SmallVector<Value> symbolsA;
  SmallVector<Value> symbolsB;
  SmallVector<Value> symbolsC;

  // The loop tiling the program writes (section 26, S5.1), when the nest is
  // tiled: recorded and reported, nothing is decided from it.
  std::optional<LoopTilingStructure> tiling;
};

/// Find Affine loop nests worth attempting to recognize.
///
/// This function does not claim that a candidate is a GEMM.
void collectAffineMatmulCandidates(
    Operation *root,
    SmallVectorImpl<AffineMatmulCandidate> &candidates);

/// Options of recognizeLogicalMatmul.
struct AffineMatmulRecognitionOptions {
  /// Reduction-batch partitioning (docs/AFFINE_GEMM_MAPPING.md section 23,
  /// on by default). On a box domain, the loops of a multiply-accumulate nest
  /// that C
  /// does not read become reduction batch loops when that makes the nest a
  /// dense GEMM: a loop read by one factor only (C[i][j] += A[i][k] * B[l][j]:
  /// k and l, with K = 1), and -- when the loops both factors read are not one
  /// dense GEMM coordinate -- all of those but the one that stays K (tried the
  /// longest first, ties innermost). Every iteration still runs exactly once:
  /// a reduction batch loop runs around the tiles, and its values accumulate
  /// into the same slab of C in order. When no such decomposition is a dense
  /// GEMM, the result, reason and class are those without the option.
  /// Assignments (C = A * B) never get reduction batch loops (section 20.4).
  bool reductionBatches = true;
  /// Whether the nest's counters are the ones as written (no domain
  /// normalization with U != I rewrote them). The ordered output batches of
  /// an accumulation whose C is not injective (section 25) are made only
  /// then.
  bool countersAsWritten = true;
};

/// Recover logical GEMM coordinates and access factorization.
///
/// Failure means "not recognized"; it is not a compiler error. On failure,
/// `why` (when given) receives the reason. The subscripts are compared as
/// functions on the iteration domain, not as expressions: a loop that runs
/// exactly once contributes a constant (docs/AFFINE_GEMM_MAPPING.md, §1, §4).
FailureOr<LogicalMatmul>
recognizeLogicalMatmul(AffineMatmulCandidate candidate,
                       std::string *why = nullptr,
                       GemmMappingClass *mappingClass = nullptr,
                       const AffineMatmulRecognitionOptions &options = {});

/// The domain normalization of docs/AFFINE_GEMM_MAPPING.md section 19.7 /
/// 19.12 (2026-10-18), applied to one perfect multiply-accumulate band whose
/// loop bounds form a constant-extent parallelotope: every loop's lower and
/// upper bound are the same linear form l_s of the outer counters plus
/// constants a_s < b_s (a box has every l_s = 0). The nest is rewritten in
/// place into the equivalent box nest
///
///   x = U x',   U lower unitriangular (unimodular),
///   x'_s runs a_s, a_s + step_s, ... < b_s,
///
/// with U[s][u] = delta_su + sum_{u <= t < s} l_s[t] U[t][u]: the loop bounds
/// become the constants a_s, b_s and every affine.load / affine.store map is
/// composed with U. U is a bijection of Z^n taking the original iteration
/// set onto the box and preserving the lexicographic order, and every access
/// reads F(x) = F(U x'), so the rewritten nest performs the same sequence of
/// memory operations as the original: everything a later stage proves about
/// the box nest holds for the original nest.
struct DomainNormalization {
  /// The band's outermost loop (the same op after the rewrite).
  Operation *outermost = nullptr;
  /// x = U x', one row per loop of the band, in nesting order.
  SmallVector<SmallVector<int64_t>> U;
  /// The shift of each loop as its bounds state it: l_s over the band
  /// positions (x_s = x_s' + l_s . x_<s); U composes these shifts.
  SmallVector<SmallVector<int64_t>> shift;
  /// The box: x'_s in [lower_s, upper_s) with the loop's step.
  SmallVector<int64_t> lower, upper, step;
  /// The loops whose bounds depended on outer counters (nonzero l_s).
  SmallVector<unsigned> shifted;
  /// The band was not in the constant-extent family as written (bounds with
  /// different linear parts at its two ends, a max / min of forms with
  /// different linear parts); its exact integer iteration set was found
  /// equal to the parallelotope above -- the Presburger library's
  /// integer-exact equality (docs section 19.13) -- and every loop's bounds
  /// were replaced by the box's.
  bool exact = false;
};

/// Normalizes every qualifying band under `root` (see DomainNormalization):
/// a constant-extent parallelotope as written (section 19.12), or a band
/// whose exact integer iteration set is one (section 19.13: bounds that do
/// not shift together, a max / min of forms with different linear parts),
/// or -- when the whole band is neither -- its sub-band below a prefix of
/// loops whose counters nothing below them reads. Bands outside -- an extent
/// that genuinely varies with an outer counter (a triangle), a bound with
/// floordiv/ceildiv/mod or a symbol, a stepped loop with a max of lower
/// bounds with different linear parts none of which is the max at every
/// outer point entering it, a counter used by anything but the body's loads
/// and stores and the inner bounds -- are left untouched, and so is every
/// band whose bounds are already constants. 64 bits: a written bound whose
/// products or value can leave 64 bits at an outer point entering its loop
/// (the program's set is then not the mathematical one), a composed
/// coefficient or a composed product over the box that does not fit, leave
/// the band untouched. Returns the normalizations applied, for the
/// diagnostics.
SmallVector<DomainNormalization, 1> normalizeParallelotopeDomains(Operation *root);

/// Whether one of `normalized` rewrote the counters (U != I) of the band that
/// holds `anchor`: the nest's counters are then not the ones as written, so
/// R8 (section 22) is not analysed and no reduction batch loop is made
/// (section 23: no integer basis transformation widens the recognition).
bool countersRewritten(ArrayRef<DomainNormalization> normalized,
                       Operation *anchor);

/// The Bounds-form loop tiling the perfect chain of loops from `outer` writes
/// (section 26, S5.1): a tile loop of step S >= 2 and constant bounds whose
/// counter nothing reads but both bounds of one inner loop of step 1, whose
/// lower bound is the tile counter plus a constant c and whose upper bound is
/// that plus S, or the min of that and one constant E falling in the last
/// tile. nullopt when the chain writes no tiling and no loop shaped like one.
/// Reads the bounds as written: call it before normalizeParallelotopeDomains.
std::optional<LoopTilingStructure>
analyzeWrittenLoopTiling(affine::AffineForOp outer);

/// Completes `tiling` (possibly empty) against `matmul`, recognized in the
/// band `tiling` records (or, with no record, the chain from its anchor --
/// for a Bounds tiling the recognized chain may be the point band inside the
/// record's band): the coordinate each loop drives, and the
/// Counter-form tilings its coordinate maps state (I = S * i0 + i1 with
/// i1 in [0, S)). `tiling` stays empty when the nest writes no tiling.
void resolveLoopTiling(std::optional<LoopTilingStructure> &tiling,
                       const LogicalMatmul &matmul);

/// Decides `tiling.coverage` exactly (section 26.2): on the band's exact
/// iteration set D as written, with y(x) the logical point of iteration x
/// (a Bounds tile counter dropped, a Counter pair combined as S * t + p, every
/// other counter kept) and B the logical box (the pairings' ranges, the
/// untiled loops' constant ranges and steps): no point of B run by no
/// iteration (missing), no iteration outside B, and no two distinct
/// iterations of D with one y (overlap). Integer-exact and without set
/// differences: every claim is the emptiness (and the lexmin, for the least
/// counterexample) of one conjunctive integer set -- overlap and outside over
/// D itself, missing over each pairing's tiles on its line (D is the product
/// of its pairings' sets and its untiled ranges). No sampling, no bound on the
/// sizes but the 64-bit one of exactIterationSet. Also proves each Bounds
/// tiling's partial tiles. Recognition, mapping and lowering are unchanged.
void verifyLoopTilingCoverage(LoopTilingStructure &tiling);

/// The legality of untiling a band's written loop tiling (docs section 26.4,
/// S5.4): each claim the untiling of section 26.3 rests on, classified as in
/// section 9 (GemmMappingClass), and the untiling itself. These classify the
/// untiling -- a transformation -- not the nest's GEMM mapping: a nest whose
/// untiling is PROVEN_INVALID may still have another valid mapping (its
/// "not untiled" remark keeps the nest's class, UNKNOWN). nullopt: the layer
/// was not evaluated (an earlier one refused).
struct TilingLegality {
  /// Pattern: every tile-shaped loop is a tiling of the form 26.1 records.
  bool supportedForm = false;
  /// Coverage: the tiles run every point of the logical box exactly once
  /// (26.2) -- PROVEN_VALID; a counterexample -- PROVEN_INVALID; not decided
  /// -- UNKNOWN.
  std::optional<GemmMappingClass> coverage;
  /// Dependence: the untiled order reorders no dependent iterations; also the
  /// execution order (each element of C accumulates in the program's order).
  std::optional<GemmMappingClass> dependence;
  /// The dependence needed the exact test (a loop between a tile loop and its
  /// point loop); otherwise the order is kept by construction.
  bool orderTested = false;
  /// The untiling: PROVEN_VALID (the lowering pass performs it),
  /// PROVEN_INVALID (a layer is), VALID_BUT_LOWERING_UNSUPPORTED (legal, but
  /// of a form S5.3 does not untile), UNKNOWN.
  GemmMappingClass untiling = GemmMappingClass::Unknown;
  /// Why the untiling is not performed (the remark of section 26.3).
  std::string why;
  /// When performed: each tiling's point loop range, in tiling order.
  SmallVector<std::pair<int64_t, int64_t>> ranges;
};

/// One band's decision: its outermost loop (a sub-band's, below a prefix of
/// loops whose counters nothing below them reads), its written tiling with
/// the coverage decided, and the legality.
struct WrittenTilingDecision {
  Operation *band = nullptr;
  LoopTilingStructure tiling;
  TilingLegality legality;
};

/// The untiling decisions of section 26.3 for every perfect
/// multiply-accumulate band under `root` that writes a tiling S5.3 is about
/// (a min that clips a last tile or cuts a tile), decided without changing
/// the IR -- by --affine-matmul-to-systolic before it untiles, and by
/// --test-affine-matmul-discovery, which reports them. `assumeNoAlias` is the
/// lowering pass's option of the same name (the alias rule of the dependence
/// test).
SmallVector<WrittenTilingDecision, 1>
decideWrittenTilings(Operation *root, bool assumeNoAlias);

/// The layers of `legality`, one line, as both passes print them.
std::string describeTilingLegality(const TilingLegality &legality);

/// The exact integer iteration set of `loops` -- a chain of affine.for loops,
/// outermost first -- over their counters in that order (one local quotient
/// per stepped loop), when it is the set the program runs: every bound
/// result a linear form of the chain's outer counters (no symbol, no
/// floordiv / ceildiv / mod, no value from outside the chain), a stepped
/// loop's lower results folded per linear part or one of them the max at
/// every outer point entering it, and every product and every result of
/// every written bound within 64 bits at every outer point entering its loop
/// (docs/AFFINE_GEMM_MAPPING.md section 19.13.2). nullopt otherwise; an empty
/// set is returned as such.
std::optional<presburger::IntegerPolyhedron>
exactIterationSet(ArrayRef<affine::AffineForOp> loops);

/// Integer-exact a ⊆ b for two conjunctive integer sets of one space (no
/// symbols), without the Presburger library's set difference -- unsound in
/// the LLVM 24 build of this project when the sets carry two or more locals
/// pinned by equalities, the stepped loops' quotients
/// (docs/AFFINE_GEMM_MAPPING.md section 19.14). b's locals are removed
/// exactly: a local that an equality pins (c q + L = 0, no other remaining
/// local in L or |c| = 1) is eliminated -- q = -L / c, an integer iff |c|
/// divides L,
/// substituted into every other row with the positive scale |c| -- and every
/// remaining local must be a floor division of the variables
/// (d q <= L <= d q + d - 1, L over the variables), kept with its
/// definition. b is then a system P over the variables and those divisions
/// plus divisibility conditions, and a ⊆ b iff a meets no negated constraint
/// of P and no negated divisibility (L = d u + r, 1 <= r < d): each one
/// conjunctive set, decided by integer emptiness. nullopt when b has a local
/// that is neither (undecided: the callers refuse). When false and `witness`
/// is given, it receives a point of a that is not in b.
std::optional<bool>
exactIsSubset(const presburger::IntegerRelation &a,
              const presburger::IntegerRelation &b,
              SmallVectorImpl<llvm::DynamicAPInt> *witness = nullptr);

/// a = b by both inclusions (exactIsSubset): false as soon as one is
/// decided false, nullopt when neither is false and one is undecided.
std::optional<bool> exactIsEqual(const presburger::IntegerRelation &a,
                                 const presburger::IntegerRelation &b);

/// A bound of `loop` that is a constant as the recognizer reads it: one
/// constant result, or the max (a lower bound) / min (an upper bound) of
/// results that are all constants -- `min(1, 4)` is 1. nullopt when a result
/// is not a constant. The map's operands are not looked at: a constant map
/// may still list some.
std::optional<int64_t> constantAffineForBound(affine::AffineForOp loop,
                                              bool lower);

/// The rank of the stacked access map x -> (F_A(x), F_B(x), F_C(x)) of a
/// recognized nest over its iteration counters: n when the accesses tell
/// every two iterations apart. Exact (checked 64-bit arithmetic); nullopt
/// when a subscript is not a linear form of the counters or on overflow.
std::optional<unsigned> stackedAccessRank(const LogicalMatmul &matmul);

/// One line per loop of a normalization, for the diagnostics:
///   loop 1: x1 = x1' + 1*x0, box [0, 4) step 1
void describeDomainNormalization(const DomainNormalization &normalization,
                                 llvm::raw_ostream &os, StringRef indent);

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
  /// The same rows on the loop counters as written (not times the step):
  /// the coefficient vectors of R8 (section 22).
  std::array<lattice::IntMatrix, 3> written;

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
    /// The task statement's algorithm of 2026-10-18, line by line, on the
    /// same six vectors (docs/AFFINE_GEMM_MAPPING.md section 19.10). N is the
    /// integer kernel of [Ic; Jc] (its whole basis: "dot(v, N) == 0" means v
    /// is orthogonal to every basis vector):
    ///   rank(Ic, Jc) = 2 ................ a
    ///   rank(Ia, Ka) = 2 ................ d1
    ///   rank(Kb, Jb) = 2 ................ d2
    ///   rank(Ic, Jc, Ia, Ka, Kb, Jb) = 3  b
    ///   rank(Ia, Ka, Kb, Jb) = 3 ........ e
    ///   not (Ia . N = 0 and Ka . N = 0) . aVaries
    ///   not (Kb . N = 0 and Jb . N = 0) . bVaries
    ///   swap(Ia, Ka) iff Ia . N != 0 and Ka . N = 0 ... swapA
    ///   swap(Kb, Jb) iff Kb . N = 0 and Jb . N != 0 ... swapB
    /// `passes`: every "return error" line passes (the swaps are not errors).
    bool e = false, aVaries = false, bVaries = false, swapA = false,
         swapB = false, passes = false;
  };
  std::optional<SixVectorConditions> conditionsFull, conditionsRoles;
};

FailureOr<AffineGeometryScreening>
screenAffineMatmulGeometry(AffineMatmulCandidate candidate,
                           std::string *why = nullptr);

/// R8, the structural recognition relation (docs/AFFINE_GEMM_MAPPING.md
/// section 22): two subscripts intersect iff the dot product of their linear
/// coefficient vectors, on the loop counters as written, is nonzero (the
/// constants take no part). Every count is a rank over Q, never a number of
/// nonzero pairs. A loop of two or more iterations is classified by its
/// readers (the operands with a nonzero coefficient on it); the contraction
/// block S is the loops both factors read and C does not. A DIAGNOSTIC: the
/// recognizer does not use it yet.
struct AffineR8Structure {
  /// Per loop of the nest, bit X set iff operand X (0 the first factor, 1
  /// the second, 2 C) has a nonzero coefficient on it; 0 for a loop of fewer
  /// than two iterations (a constant).
  SmallVector<unsigned> readers;
  /// The reduction loops Z (read by a factor, not by C), the shared loops S
  /// of Z (read by both factors), and S's loops grouped by appearing
  /// together in one subscript of a factor.
  SmallVector<unsigned> reductionLoops;
  SmallVector<unsigned> contractionLoops;
  SmallVector<SmallVector<unsigned>> contractionGroups;
  /// On Z, with P and Q the factors' rows restricted to Z (the reduction
  /// index vectors): rank P, rank Q, rank [P; Q], and rank(P Q^T), the
  /// number of independent directions coupled by nonzero dot products.
  unsigned rankFirst = 0, rankSecond = 0, rankStacked = 0, coupling = 0;
  /// rank(P_b Q_b^T) on the loops all three operands read.
  unsigned batchCoupling = 0;
  /// The literal reading on every loop of two or more iterations:
  /// rank(M_X M_Y^T) for the pairs
  /// (first, second), (first, C), (second, C), and the number of nonzero
  /// (first, second) dot products (not a dimension).
  unsigned literalAB = 0, literalAC = 0, literalBC = 0, pairsAB = 0;
  /// The rank of each operand's rows on every loop; 1 is a
  /// one-dimensional operand (Phase 2), 0 a constant one.
  std::array<unsigned, 3> operandRank{};
  /// intersect(A, B): some reduction index vector a of A and b of B have
  /// a . b != 0 (equivalently P Q^T != 0). sharedLoops: some pair has
  /// sum_t |a_t| |b_t| > 0, i.e. a loop variable drives both (S is not
  /// empty) -- possible with a . b = 0 (x0 - x1 and x0 + x1). Neither is a
  /// row-space intersection, whose dimension is rank P + rank Q -
  /// rank [P; Q] (linear independence only).
  bool intersect = false, sharedLoops = false;
};

/// R8 on explicit rows: rows[X] holds operand X's subscripts' coefficients on
/// the n loops of `trips`. nullopt on a ragged row or on overflow.
std::optional<AffineR8Structure>
computeR8Structure(const std::array<lattice::IntMatrix, 3> &rows,
                   ArrayRef<int64_t> trips);

} // namespace systolic
} // namespace mlir

#endif // SYSTOLIC_AFFINE_MATMUL_ANALYSIS_H
