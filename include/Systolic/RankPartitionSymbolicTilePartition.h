//===- RankPartitionSymbolicTilePartition.h - Option C C3 --------*- C++ -*-===//
//
// Phase C3 (Option C): partition the exact logical contraction domain (from
// C2) into disjoint, device-representable reduction tiles grouped by a constant
// reduction length L, WITHOUT enumerating source iteration points.
//
// The logical image D (C2) is a lattice intersect a convex region:
//   image = Lambda  ∩  Convex,   Lambda = T*Z^n,   Convex = T(sourceDomain).
// Fix an output o (all logical dims except the single reduction dim k). The set
// of reduction coordinates { k : (o,k) ∈ image } is, whenever non-empty, a
// coset of g*Z intersected with a convex k-interval -- i.e. an ARITHMETIC
// PROGRESSION with a single, GLOBAL stride g:
//
//   g = min { delta > 0 : delta*e_k ∈ Lambda }
//     = lcm_i ( d / gcd(d, M[i][k]) ),   M = d*T^{-1}  (integer),  d > 0.
//
// g is proven positive (T is invertible, so column k of T^{-1} is non-zero).
// A contiguous fiber is simply the g = 1 case; a sublattice-punctured
// (non-contiguous) fiber is g > 1. No fiber is ever assumed contiguous from
// convexity: the stride is derived from the lattice.
//
// Each fiber is k = k0(o) + g*t, t = 0 .. L-1, with k0(o) the fiber minimum and
// L the point count. We partition the outputs by L. For a target length L the
// anchor set
//   Pi_L = { z (with k-slot = fiber minimum k0) :
//              (o,k0)        ∈ image        (base present, in the lattice)
//              (o,k0+g(L-1)) ∈ Convex       (top present; lattice automatic)
//              (o,k0-g)      ∉ Convex       (k0 is the minimum)
//              (o,k0+gL)     ∉ Convex       (top is the maximum) }
// Because base±(multiple of g) stays in Lambda, "∉ image" collapses to
// "∉ Convex", an affine test. "∉ Convex" is the disjunction over the convex
// faces that bound k; we materialise it as a disjoint union of convex pieces by
// selecting, in each piece, the FIRST violated lower face and FIRST violated
// upper face (a total order on faces). The construction uses only intersections
// (addInequality / addEquality) -- never set subtraction (so the LLVM-24
// unsafe >=2-equality-pinned-local subtraction path is never taken) and never
// parametric integer programming.
//
// Any step that is not provably exact and i64-representable returns nullopt
// (UNKNOWN): more than one reduction dimension, a reduction axis unbounded in
// the convex hull, an output pinned in k by an equality, or any overflow. The
// caller refuses rather than approximating.
//
// C3 produces a validated symbolic partition only. It emits no IR and performs
// no tile lowering (that is C4). buildRecoverableLogicalDomain, the guarded-MAC
// one-local path, and every existing test assertion are untouched.
//
#ifndef SYSTOLIC_RANK_PARTITION_SYMBOLIC_TILE_PARTITION_H
#define SYSTOLIC_RANK_PARTITION_SYMBOLIC_TILE_PARTITION_H

#include "Systolic/RankPartitionSymbolicContraction.h"

#include "mlir/Analysis/Presburger/IntegerRelation.h"
#include "mlir/Analysis/Presburger/PresburgerSpace.h"

#include "llvm/ADT/SmallVector.h"

#include <array>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace systolic::logical {

/// One group of reduction fibers that all share the same constant length.
/// `anchorPieces` is a DISJOINT union of convex integer polyhedra; their union
/// is the set of logical points whose k-slot is the fiber minimum and whose
/// fiber has exactly `length` points. Each piece is over [z(logicalRank),
/// u(congruenceLocals), 1] -- the same variable layout as the C2 image, the
/// u being the congruence (integrality) locals.
struct TileFiberGroup {
  std::int64_t length = 0;
  std::vector<mlir::presburger::IntegerPolyhedron> anchorPieces;
};

/// The validated symbolic tile partition. `stride` is the proven positive g
/// (1 for contiguous fibers). `access` echoes the C2 operand/output access maps
/// over [z, u]; `tStride[op][r]` is the proven-integer change in operand `op`'s
/// tensor index `r` when the reduction ordinal t advances by one. `groups` holds
/// one entry per realised length, ascending; their anchor sets are pairwise
/// disjoint and together cover every output exactly once.
struct SymbolicTilePartition {
  TileCoordinateFrame frame;
  std::int64_t stride = 1;
  unsigned kIndex = 0;
  bool hasReduction = true;
  unsigned congruenceLocals = 0;
  std::array<LogicalAccessMap, 3> access;
  std::array<std::vector<BigInt>, 3> tStride;
  ReductionOrderDecision reductionOrder =
      ReductionOrderDecision::PreserveSourceOrder;
  std::vector<TileFiberGroup> groups;

  // C4a-sym.0a: unused source iteration (replication) axes -- source axes in NO
  // operand access -- folded into the reduction. Empty for a clean partition.
  // Each group's `length` ALREADY includes the replication multiplicity
  // `replicationExtent` (= product of those axes' extents); the anchor pieces
  // pin each replication batch coordinate to its lower bound, so every anchor is
  // one physical output fiber. These axes are the outermost source dimensions
  // (certified at build time) so a replication-outer gather reproduces the
  // source accumulation order; otherwise the partition is refused under
  // PreserveSourceOrder.
  std::vector<unsigned> replicationSourceAxes;
  std::int64_t replicationExtent = 1;
};

namespace symbolic_tile_detail {

inline BigInt babs(BigInt v) { return v < 0 ? BigInt(-v) : v; }

inline BigInt bgcd(BigInt a, BigInt b) {
  a = babs(a);
  b = babs(b);
  while (b != 0) {
    BigInt t = a % b;
    a = b;
    b = t;
  }
  return a;
}

inline std::optional<BigInt> blcm(const BigInt &a, const BigInt &b) {
  if (a == 0 || b == 0)
    return BigInt(0);
  BigInt g = bgcd(a, b);
  return BigInt(babs(a) / g * babs(b));
}

inline std::optional<std::int64_t> toI64(const BigInt &value) {
  if (value < BigInt(std::numeric_limits<std::int64_t>::min()) ||
      value > BigInt(std::numeric_limits<std::int64_t>::max()))
    return std::nullopt;
  return value.convert_to<std::int64_t>();
}

/// A pure-z affine face: coefficients over z (length n) and a constant, with
/// a non-zero reduction-axis coefficient recorded for ordering.
struct ConvexFace {
  std::vector<std::int64_t> z; // length n
  std::int64_t constant = 0;
  std::int64_t kCoeff = 0;
};

/// Exact rank of an integer matrix over the rationals (fraction-aware Gaussian
/// elimination with exact Rat arithmetic). `ncols` columns are considered; rows
/// shorter than `ncols` are zero-extended. No overflow: Rat is arbitrary
/// precision.
inline unsigned rankOf(const IntMatrix &rows, unsigned ncols) {
  const unsigned nrows = static_cast<unsigned>(rows.size());
  std::vector<std::vector<Rat>> A(nrows, std::vector<Rat>(ncols, Rat(0)));
  for (unsigned i = 0; i < nrows; ++i)
    for (unsigned j = 0; j < ncols && j < rows[i].size(); ++j)
      A[i][j] = Rat(rows[i][j]);
  unsigned rank = 0;
  for (unsigned col = 0; col < ncols && rank < nrows; ++col) {
    unsigned piv = rank;
    while (piv < nrows && A[piv][col] == Rat(0))
      ++piv;
    if (piv == nrows)
      continue;
    std::swap(A[piv], A[rank]);
    for (unsigned r = 0; r < nrows; ++r)
      if (r != rank && A[r][col] != Rat(0)) {
        Rat f = A[r][col] / A[rank][col];
        for (unsigned c = col; c < ncols; ++c)
          A[r][c] = A[r][c] - f * A[rank][c];
      }
    ++rank;
  }
  return rank;
}

} // namespace symbolic_tile_detail

/// Build the validated symbolic tile partition from the SAME raw recognizer
/// facts C2 consumes. Returns nullopt (UNKNOWN) on any inexactness.
inline std::optional<SymbolicTilePartition> buildSymbolicTilePartition(
    const std::array<IntMatrix, 3> &accessMatrices,
    const std::array<IntRow, 3> &offsets,
    const mlir::presburger::IntegerPolyhedron &sourceDomain,
    std::size_t sourceRank,
    ReductionOrderDecision reductionOrder,
    std::string &why) {

  using namespace mlir::presburger;
  using symbolic_tile_detail::bgcd;
  using symbolic_tile_detail::blcm;
  using symbolic_tile_detail::ConvexFace;
  using symbolic_tile_detail::rankOf;
  using symbolic_tile_detail::toI64;

  // 1. Reuse C2: recover frame, exact N-local image D over [z,u], access maps.
  auto contraction = buildSymbolicContraction(
      accessMatrices, offsets, sourceDomain, sourceRank, reductionOrder, why);
  if (!contraction)
    return std::nullopt; // why already set by C2.

  const TileCoordinateFrame frame = contraction->frame;
  const unsigned n = frame.logicalRank();
  const unsigned added = contraction->congruenceLocals;
  const IntegerPolyhedron &D = contraction->domain;

  if (D.getNumDimVars() != n || D.getNumSymbolVars() != 0 ||
      D.getNumLocalVars() != added || D.getNumCols() != n + added + 1) {
    why = "C2 image shape unexpected";
    return std::nullopt;
  }

  // More than one reduction dimension is not a single-stride progression.
  if (frame.kDims > 1) {
    why = "UNKNOWN: more than one reduction dimension (multi-dim contraction)";
    return std::nullopt;
  }

  const unsigned kIdx = frame.kBegin();
  const unsigned outputDims = n - frame.kDims; // M + N + batch logical dims

  // Recompute M = d*T^{-1}, d, and the congruence rows exactly as C2 /
  // buildExactLogicalImage do, so the stride, the ordinal strides, and the
  // batch-soundness test below are all consistent. Independent of the domain,
  // so it also validates empty and reduction-free contractions.
  const auto partition = analyzeRankPartition(
      accessMatrices[0], accessMatrices[1], accessMatrices[2], sourceRank);
  if (!partition) {
    why = "rank partition analysis failed";
    return std::nullopt;
  }
  const auto expanded = expandRankPartition(*partition);
  if (!expanded) {
    why = "rank partition expansion failed";
    return std::nullopt;
  }

  // Multi-dimensional reduction detection (C3c). expandRankPartition keeps only
  // ONE active reduction axis (k.front()) and pushes every further reduction
  // axis into `partitions` flagged requiresReduction. Those partitions are a
  // GENUINE multi-axis reduction K = (k1, k2, ...), NOT batch -- the recognizer
  // keeps the distinction in requiresReduction, which C2's frame.batchDims
  // (counting all partitions alike) drops. Count the true reduction rank from
  // the flag. Only a single reduction axis is tiled today; a genuine
  // multi-dimensional reduction requires exact flattening into one device K
  // dimension (a later stage), so refuse it conservatively here -- distinctly
  // from batch and from output aliasing -- rather than silently reinterpret a
  // reduction axis as a parallel axis.
  unsigned reductionAxes = static_cast<unsigned>(expanded->k.size());
  for (const auto &axis : expanded->partitions)
    if (axis.requiresReduction)
      ++reductionAxes;
  if (reductionAxes > 1) {
    why = "UNKNOWN: multi-dimensional reduction (K spans multiple axes); exact "
          "flattening into a single device K dimension is not yet implemented";
    return std::nullopt;
  }

  const auto coordinateMap = buildPartitionCoordinateMap(*expanded);
  if (!coordinateMap) {
    why = "coordinate map construction failed";
    return std::nullopt;
  }
  AffineIntegerDomain unconstrained;
  unconstrained.sourceDimension = n;
  const auto symbolic = transformToSymbolicDomain(*coordinateMap, unconstrained);
  if (!symbolic || symbolic->dimension != n || symbolic->denominator <= 0 ||
      symbolic->inverseNumerator.size() != n) {
    why = "symbolic transform of the coordinate map failed";
    return std::nullopt;
  }
  const BigInt &d = symbolic->denominator;
  const IntMatrix &M = symbolic->inverseNumerator;

  std::vector<unsigned> congruenceRows;
  for (unsigned i = 0; i < n; ++i) {
    if (M[i].size() != n) {
      why = "inverse-numerator row arity";
      return std::nullopt;
    }
    for (const BigInt &c : M[i])
      if (c % d != 0) {
        congruenceRows.push_back(i);
        break;
      }
  }
  if (static_cast<unsigned>(congruenceRows.size()) != added) {
    why = "congruence-local count disagrees with the exact image";
    return std::nullopt;
  }

  // Batch soundness (requirements 8-10). Zero or more independent batch axes
  // are supported: they are output logical coordinates that ride along with
  // M/N (the construction shifts only the reduction slot). A batch axis is
  // GENUINE iff the output (C) access is injective on EVERY output logical
  // coordinate -- distinct logical outputs never alias one physical C element,
  // so their reductions are independent. That holds exactly when
  //   rank(C_source_rows) == outputDims   (== n - kDims),
  // i.e. ker(C) is one-dimensional. A reduction axis the recognizer demoted to
  // "batch" (e.g. C[i][j] with a second contraction axis) leaves C
  // rank-deficient over the outputs, so distinct outputs alias and tiling them
  // as independent reductions would change semantics -> UNKNOWN.
  // Output injectivity. A clean single-reduction GEMM has C injective on the
  // output logical coordinates: rank(C_rows) == outputDims. C4a-sym.0a admits
  // ONE generalization -- UNUSED source iteration dimensions (replication): a
  // source axis in NO operand access. Such an axis becomes a batch logical
  // coordinate that C does not index; because every physical address is
  // independent of it, it introduces no distinct physical output, only a
  // reduction multiplicity. Any OTHER rank deficiency (a singular output map --
  // a real output axis C fails to index) is C4a-sym.0b and is refused here.
  std::vector<unsigned> replicationSourceAxes; // source axes in no access
  for (unsigned s = 0; s < sourceRank; ++s) {
    bool used = false;
    for (unsigned op = 0; op < 3 && !used; ++op)
      for (const IntRow &row : accessMatrices[op])
        if (s < row.size() && row[s] != 0) {
          used = true;
          break;
        }
    if (!used)
      replicationSourceAxes.push_back(s);
  }
  const unsigned rankC = rankOf(accessMatrices[2], n);
  const unsigned replCount = static_cast<unsigned>(replicationSourceAxes.size());
  if (rankC != outputDims) {
    // The deficiency must be EXACTLY the replication axes -- nothing else. A
    // singular output (rankC + replCount < outputDims) fails closed.
    if (replCount == 0 || rankC + replCount != outputDims) {
      why = "UNKNOWN: output access aliases distinct logical outputs "
            "(a batch/extra axis does not index C -- reductions not independent)";
      return std::nullopt;
    }
    // Strict-FP source-order obligation. A replication axis repeats every MAC
    // once per outer iteration; the fiber's source order equals "base fiber,
    // repeated per replication tuple" EXACTLY when every replication axis is
    // lexicographically outer to every reduction/output axis -- i.e. the
    // replication axes are the outermost source prefix {0,...,replCount-1}.
    // Then a replication-outer gather reproduces the source order. Otherwise
    // the summation order interleaves and is not proven source-equivalent, so
    // PreserveSourceOrder refuses (fail closed); reassociation accepts.
    bool outermostPrefix = true;
    for (unsigned t = 0; t < replCount; ++t)
      if (replicationSourceAxes[t] != t) {
        outermostPrefix = false;
        break;
      }
    // Fail closed: when the order is NOT proven source-equivalent (replication
    // not an outermost prefix), accept only if reassociation is explicitly
    // permitted; PreserveSourceOrder and the weaker NoOutputWriteCollision both
    // refuse.
    if (!outermostPrefix &&
        reductionOrder != ReductionOrderDecision::ReassociationCandidate) {
      why = "UNKNOWN: replication axis is not an outermost source dimension; the "
            "summation order is not proven source-equivalent without "
            "reassociation";
      return std::nullopt;
    }
  } else {
    replicationSourceAxes.clear(); // clean partition: nothing to fold.
  }
  // The output must also be constant along the reduction (C does not vary with
  // k): C_source_rows . (reduction direction M[:,kIdx]) == 0. Together with the
  // rank test this proves ker(C) == the reduction direction exactly, so batch
  // axes are parallel (never reinterpreted as reduction).
  if (frame.kDims == 1)
    for (const IntRow &crow : accessMatrices[2]) {
      if (crow.size() != n) {
        why = "output access row arity";
        return std::nullopt;
      }
      BigInt s = 0;
      for (unsigned j = 0; j < n; ++j)
        s += crow[j] * M[j][kIdx];
      if (s != 0) {
        why = "UNKNOWN: output varies along the reduction axis (not a reduction)";
        return std::nullopt;
      }
    }

  SymbolicTilePartition out;
  out.frame = frame;
  out.congruenceLocals = added;
  out.access = contraction->access;
  out.reductionOrder = reductionOrder;
  out.kIndex = kIdx;
  out.hasReduction = (frame.kDims == 1);

  // C4a-sym.0a replication fold. Pin each replication batch logical coordinate
  // (a batch column no operand indexes) to its lower bound -- selecting ONE
  // anchor per physical output fiber -- and multiply every realised fiber
  // length by R = product of those axes' extents. No-op for a clean partition
  // (replicationSourceAxes empty). Returns false (UNKNOWN) when the replication
  // extent is not a constant finite interval (coupled or unbounded), so the
  // multiplicity would not be identical across fibers.
  auto foldReplication = [&]() -> bool {
    if (replicationSourceAxes.empty())
      return true;
    std::vector<unsigned> replCols;
    for (unsigned col = frame.batchBegin(); col < n; ++col) {
      bool anyNonzero = false;
      for (unsigned op = 0; op < 3 && !anyNonzero; ++op)
        for (const auto &row : contraction->access[op].rows)
          if (col < row.size() && row[col] != 0) {
            anyNonzero = true;
            break;
          }
      if (!anyNonzero)
        replCols.push_back(col);
    }
    if (replCols.size() != replicationSourceAxes.size()) {
      why = "UNKNOWN: replication source/logical axis-count mismatch";
      return false;
    }
    BigInt R(1);
    std::vector<std::int64_t> pinLo(replCols.size());
    for (std::size_t idx = 0; idx < replCols.size(); ++idx) {
      const unsigned col = replCols[idx];
      auto lb = D.getConstantBound64(BoundType::LB, col);
      auto ub = D.getConstantBound64(BoundType::UB, col);
      if (!lb || !ub || *ub < *lb) {
        why = "UNKNOWN: replication axis extent not a constant finite interval";
        return false;
      }
      // Box-separability: no image constraint couples this column with another
      // variable, so its extent is identical for every fiber.
      auto coupled = [&](llvm::ArrayRef<llvm::DynamicAPInt> row) -> bool {
        if (row[col] == llvm::DynamicAPInt(0))
          return false;
        for (unsigned j = 0; j < n + added; ++j)
          if (j != col && row[j] != llvm::DynamicAPInt(0))
            return true;
        return false;
      };
      for (unsigned r = 0; r < D.getNumInequalities(); ++r)
        if (coupled(D.getInequality(r))) {
          why = "UNKNOWN: replication axis coupled in the image (non-constant "
                "extent)";
          return false;
        }
      for (unsigned r = 0; r < D.getNumEqualities(); ++r)
        if (coupled(D.getEquality(r))) {
          why = "UNKNOWN: replication axis coupled in the image";
          return false;
        }
      pinLo[idx] = *lb;
      R *= BigInt(*ub - *lb + 1);
    }
    auto R64 = toI64(R);
    if (!R64 || *R64 <= 0) {
      why = "UNKNOWN: replication multiplicity overflows i64";
      return false;
    }
    for (auto &group : out.groups) {
      BigInt scaled = BigInt(group.length) * R;
      auto s64 = toI64(scaled);
      if (!s64) {
        why = "UNKNOWN: folded fiber length overflows i64";
        return false;
      }
      group.length = *s64;
      for (auto &P : group.anchorPieces)
        for (std::size_t idx = 0; idx < replCols.size(); ++idx) {
          llvm::SmallVector<std::int64_t> eq(P.getNumCols(), 0);
          eq[replCols[idx]] = 1;
          eq[P.getNumCols() - 1] = -pinLo[idx];
          P.addEquality(eq);
        }
    }
    out.replicationSourceAxes = replicationSourceAxes;
    out.replicationExtent = *R64;
    return true;
  };

  // Empty domain: a valid partition with no fibers.
  if (D.isIntegerEmpty()) {
    out.stride = 1;
    for (unsigned op = 0; op < 3; ++op)
      out.tStride[op].assign(accessMatrices[op].size(), BigInt(0));
    return out;
  }

  // No reduction axis (K == 1 by construction): every image point is one MAC.
  // One length-1 group whose anchor is the whole image.
  if (!out.hasReduction) {
    out.stride = 1;
    for (unsigned op = 0; op < 3; ++op)
      out.tStride[op].assign(accessMatrices[op].size(), BigInt(0));
    TileFiberGroup grp;
    grp.length = 1;
    grp.anchorPieces.push_back(D);
    out.groups.push_back(std::move(grp));
    if (!foldReplication())
      return std::nullopt;
    return out;
  }

  // Stride g = lcm_i ( d / gcd(d, M[i][kIdx]) ). M[i][kIdx] is column kIdx.
  BigInt g(1);
  for (unsigned i = 0; i < n; ++i) {
    const BigInt &mik = M[i][kIdx];
    BigInt req = d / bgcd(d, mik); // delta must be a multiple of this.
    auto l = blcm(g, req);
    if (!l) {
      why = "stride lcm failed";
      return std::nullopt;
    }
    g = *l;
  }
  if (g <= 0) {
    why = "UNKNOWN: non-positive reduction stride";
    return std::nullopt;
  }
  auto g64 = toI64(g);
  if (!g64) {
    why = "UNKNOWN: reduction stride overflows i64";
    return std::nullopt;
  }
  out.stride = *g64;

  // 5. Per-operand ordinal (t) stride: as t advances, z_k += g and each
  //    congruence local u_l += M[cr_l][kIdx]*g/d (proven integer). The address
  //    change is access_z[kIdx]*g + sum_l access_u[l] * (M[cr_l][kIdx]*g/d).
  for (unsigned op = 0; op < 3; ++op) {
    const LogicalAccessMap &am = contraction->access[op];
    out.tStride[op].assign(am.rows.size(), BigInt(0));
    for (std::size_t r = 0; r < am.rows.size(); ++r) {
      if (am.rows[r].size() != n + added) {
        why = "access row arity";
        return std::nullopt;
      }
      BigInt delta = am.rows[r][kIdx] * g;
      for (unsigned l = 0; l < added; ++l) {
        const BigInt num = M[congruenceRows[l]][kIdx] * g;
        if (num % d != 0) {
          why = "UNKNOWN: ordinal stride not integral";
          return std::nullopt;
        }
        delta += am.rows[r][n + l] * (num / d);
      }
      if (!toI64(delta)) {
        why = "UNKNOWN: operand ordinal stride overflows i64";
        return std::nullopt;
      }
      out.tStride[op][r] = delta;
    }
  }

  // 6. Convex hull over z: D's inequalities are pure-z (u-columns zero); its
  //    equalities are either pure-z source equalities (u-columns zero) or the
  //    lattice congruence equalities (u-columns non-zero). Keep the pure-z
  //    constraints as ConvexZ; collect the k-bounding faces.
  auto readRow = [&](llvm::ArrayRef<llvm::DynamicAPInt> row,
                     std::vector<std::int64_t> &zOut, std::int64_t &cOut,
                     bool &uNonZero) -> bool {
    zOut.assign(n, 0);
    uNonZero = false;
    for (unsigned j = 0; j < n; ++j) {
      const llvm::DynamicAPInt &v = row[j];
      if (v < llvm::DynamicAPInt(std::numeric_limits<std::int64_t>::min()) ||
          v > llvm::DynamicAPInt(std::numeric_limits<std::int64_t>::max()))
        return false;
      zOut[j] = static_cast<std::int64_t>(v);
    }
    for (unsigned l = 0; l < added; ++l)
      if (row[n + l] != llvm::DynamicAPInt(0))
        uNonZero = true;
    const llvm::DynamicAPInt &c = row[n + added];
    if (c < llvm::DynamicAPInt(std::numeric_limits<std::int64_t>::min()) ||
        c > llvm::DynamicAPInt(std::numeric_limits<std::int64_t>::max()))
      return false;
    cOut = static_cast<std::int64_t>(c);
    return true;
  };

  IntegerPolyhedron convexZ(PresburgerSpace::getSetSpace(n, 0, 0));
  std::vector<ConvexFace> allConvexIneqs; // every pure-z inequality face
  std::vector<std::pair<std::vector<std::int64_t>, std::int64_t>> convexEqs;
  std::vector<ConvexFace> lowerFaces; // kCoeff > 0, lower-bound k
  std::vector<ConvexFace> upperFaces; // kCoeff < 0, upper-bound k

  for (unsigned r = 0; r < D.getNumInequalities(); ++r) {
    std::vector<std::int64_t> z;
    std::int64_t c;
    bool uNonZero;
    if (!readRow(D.getInequality(r), z, c, uNonZero)) {
      why = "UNKNOWN: convex constraint overflows i64";
      return std::nullopt;
    }
    if (uNonZero) {
      why = "UNKNOWN: image inequality references a congruence local";
      return std::nullopt;
    }
    llvm::SmallVector<std::int64_t> ineq(n + 1, 0);
    for (unsigned j = 0; j < n; ++j)
      ineq[j] = z[j];
    ineq[n] = c;
    convexZ.addInequality(ineq);
    allConvexIneqs.push_back({z, c, z[kIdx]});
    if (z[kIdx] > 0)
      lowerFaces.push_back({z, c, z[kIdx]});
    else if (z[kIdx] < 0)
      upperFaces.push_back({z, c, z[kIdx]});
  }
  for (unsigned r = 0; r < D.getNumEqualities(); ++r) {
    std::vector<std::int64_t> z;
    std::int64_t c;
    bool uNonZero;
    if (!readRow(D.getEquality(r), z, c, uNonZero)) {
      why = "UNKNOWN: convex constraint overflows i64";
      return std::nullopt;
    }
    if (uNonZero)
      continue; // lattice congruence equality -- not part of the convex hull.
    // A pure-z equality that pins the reduction axis makes the fiber geometry
    // degenerate under k-shift; refuse conservatively.
    if (z[kIdx] != 0) {
      why = "UNKNOWN: reduction axis constrained by an equality";
      return std::nullopt;
    }
    llvm::SmallVector<std::int64_t> eq(n + 1, 0);
    for (unsigned j = 0; j < n; ++j)
      eq[j] = z[j];
    eq[n] = c;
    convexZ.addEquality(eq);
    convexEqs.push_back({z, c});
  }

  // 7. Reduction axis must be bounded in the convex hull (finite fibers).
  auto ubK = convexZ.getConstantBound64(BoundType::UB, kIdx);
  auto lbK = convexZ.getConstantBound64(BoundType::LB, kIdx);
  if (!ubK || !lbK) {
    why = "UNKNOWN: reduction axis unbounded in the convex hull";
    return std::nullopt;
  }
  // A finite reduction extent must be bounded by at least one convex face on
  // each side; otherwise the fiber endpoints are not face-expressible.
  if (lowerFaces.empty() || upperFaces.empty()) {
    why = "UNKNOWN: reduction axis not bounded by a convex face";
    return std::nullopt;
  }
  // Max possible fiber length over the convex k-extent, with slack.
  const std::int64_t kSpan = *ubK - *lbK;
  if (kSpan < 0) {
    why = "UNKNOWN: degenerate reduction extent";
    return std::nullopt;
  }
  const std::int64_t gI = out.stride;
  // (span / g) + 2 bounds any fiber length; guard against absurd magnitudes.
  if (kSpan > (std::numeric_limits<std::int64_t>::max() - 2)) {
    why = "UNKNOWN: reduction extent overflows";
    return std::nullopt;
  }
  const std::int64_t capL = kSpan / gI + 2;
  if (capL <= 0 || capL > (std::int64_t(1) << 32)) {
    why = "UNKNOWN: unrepresentable reduction length cap";
    return std::nullopt;
  }

  const unsigned cols = n + added + 1; // anchor piece layout == D layout.

  // Helper: append `base` constraints (over [z,u,1]) of D into piece `P`.
  auto addDConstraints = [&](IntegerPolyhedron &P) -> bool {
    for (unsigned r = 0; r < D.getNumInequalities(); ++r)
      P.addInequality(D.getInequality(r));
    for (unsigned r = 0; r < D.getNumEqualities(); ++r)
      P.addEquality(D.getEquality(r));
    (void)cols;
    return true;
  };

  // Helper: add "convexZ face holds at z with k-slot shifted by `shift`"
  //   face.z . z + (face.constant + face.kCoeff*shift) >= 0
  // when `violated` is false, or the strict negation (<= -1) when true.
  auto addShiftedFace = [&](IntegerPolyhedron &P, const ConvexFace &face,
                            std::int64_t shift, bool violated) -> bool {
    // constant of the shifted face
    BigInt base = BigInt(face.constant) + BigInt(face.kCoeff) * BigInt(shift);
    llvm::SmallVector<std::int64_t> row(cols, 0);
    if (!violated) {
      for (unsigned j = 0; j < n; ++j)
        row[j] = face.z[j];
      auto cc = toI64(base);
      if (!cc)
        return false;
      row[n + added] = *cc;
    } else {
      for (unsigned j = 0; j < n; ++j)
        row[j] = -face.z[j];
      auto cc = toI64(BigInt(-1) - base);
      if (!cc)
        return false;
      row[n + added] = *cc;
    }
    P.addInequality(row);
    return true;
  };

  // Helper: add "top point (k-slot + shift) is inside ConvexZ".
  auto addTopInsideConvex = [&](IntegerPolyhedron &P,
                                std::int64_t shift) -> bool {
    for (const ConvexFace &face : allConvexIneqs) {
      BigInt base =
          BigInt(face.constant) + BigInt(face.kCoeff) * BigInt(shift);
      llvm::SmallVector<std::int64_t> row(cols, 0);
      for (unsigned j = 0; j < n; ++j)
        row[j] = face.z[j];
      auto cc = toI64(base);
      if (!cc)
        return false;
      row[n + added] = *cc;
      P.addInequality(row);
    }
    for (const auto &eq : convexEqs) {
      // Pure-z, k-independent equalities (kCoeff == 0 guaranteed by step 6).
      llvm::SmallVector<std::int64_t> row(cols, 0);
      for (unsigned j = 0; j < n; ++j)
        row[j] = eq.first[j];
      row[n + added] = eq.second;
      P.addEquality(row);
    }
    return true;
  };

  // 8. Build one group per realised length. For length L the fiber is
  //    [k0, k0+g(L-1)]; top = k0 + g(L-1); the "just below" point is k0-g and
  //    the "just above" point is k0+gL. Disjointness: in each piece a chosen
  //    lower face p is the FIRST violated one (faces before it hold) and a
  //    chosen upper face q is the FIRST violated one.
  for (std::int64_t L = 1; L <= capL; ++L) {
    // top shift = g*(L-1); above-top shift = g*L; below-base shift = -g.
    BigInt topShiftB = BigInt(gI) * BigInt(L - 1);
    BigInt aboveShiftB = BigInt(gI) * BigInt(L);
    auto topShift = toI64(topShiftB);
    auto aboveShift = toI64(aboveShiftB);
    if (!topShift || !aboveShift) {
      why = "UNKNOWN: fiber shift overflows i64";
      return std::nullopt;
    }

    TileFiberGroup group;
    group.length = L;

    for (std::size_t p = 0; p < lowerFaces.size(); ++p) {
      for (std::size_t q = 0; q < upperFaces.size(); ++q) {
        IntegerPolyhedron P(PresburgerSpace::getSetSpace(n, 0, added));
        if (!addDConstraints(P)) {
          why = "UNKNOWN: base-constraint overflow";
          return std::nullopt;
        }
        // top point present (inside convex; lattice automatic).
        if (!addTopInsideConvex(P, *topShift)) {
          why = "UNKNOWN: top-constraint overflow";
          return std::nullopt;
        }
        // lower face p is the first violated at the below-base point.
        bool ok = true;
        for (std::size_t pp = 0; pp < p; ++pp)
          ok &= addShiftedFace(P, lowerFaces[pp], -gI, /*violated=*/false);
        ok &= addShiftedFace(P, lowerFaces[p], -gI, /*violated=*/true);
        // upper face q is the first violated at the above-top point.
        for (std::size_t qq = 0; qq < q; ++qq)
          ok &= addShiftedFace(P, upperFaces[qq], *aboveShift,
                               /*violated=*/false);
        ok &= addShiftedFace(P, upperFaces[q], *aboveShift, /*violated=*/true);
        if (!ok) {
          why = "UNKNOWN: boundary-constraint overflow";
          return std::nullopt;
        }
        if (!P.isIntegerEmpty())
          group.anchorPieces.push_back(std::move(P));
      }
    }

    if (!group.anchorPieces.empty())
      out.groups.push_back(std::move(group));
  }

  if (!foldReplication())
    return std::nullopt;
  why = "symbolic tile partition built";
  return out;
}

} // namespace systolic::logical

#endif // SYSTOLIC_RANK_PARTITION_SYMBOLIC_TILE_PARTITION_H
