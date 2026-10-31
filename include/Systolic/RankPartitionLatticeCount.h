//===- RankPartitionLatticeCount.h - M-count exact symbolic counting -*- C++ -*-===//
//
// M-count (Option C): EXACT, SYMBOLIC lattice-point counting of the C3 disjoint
// anchor pieces, with NO source-point enumeration. The diagnostic counts a
// general-affine GEMM lowering must report -- the number of output fibers and
// the number of source MACs -- are derived from the certified partition:
//
//   outputFibers = sum over groups g of   |pieces of g|
//   sourceMACs   = sum over groups g of L_g * |pieces of g|
//
// where |piece| is the number of reduction fibers the piece represents, i.e.
// the number of its integer points, and L_g is the group's certified constant
// reduction length. The anchor pieces are pairwise disjoint and every fiber's
// minimum lies in exactly one piece (C3's certificate), so the sums are exact.
//
// An anchor piece is an integer polyhedron over [z, u], z the logical tile
// coordinates and u the congruence (integrality) locals. Its integer points are
// exactly the pairs (z, u) with the lattice congruence z = T*u satisfied, and
// u ranges over the DENSE source iteration lattice (u is the source iteration).
// Composing the piece's pure-z faces with z = T*x (T = the recognizer's
// source->logical map) therefore yields a convex polytope over the DENSE source
// variables x whose integer points are exactly that piece's fibers -- the
// congruence sublattice disappears, counted in source coordinates.
//
// This header counts the SEPARABLE-BOX class exactly and in closed form: a
// source polytope all of whose faces are axis-parallel (one nonzero source
// coefficient each). Its count is the product of the per-axis integer-interval
// cardinalities -- O(dims * faces) integer operations, with NO loop proportional
// to the number of lattice points. Any piece that is not a separable box
// (triangular / simplicial / trapezoidal / a coupling face, or carrying a
// congruence local in an inequality) returns nullopt (UNKNOWN); the caller must
// refuse rather than approximate. Those shapes are a later M-count extension
// (exact simplex / iterated-single-face summation); never counted here by
// enumeration or bounding box.
//
//===----------------------------------------------------------------------===//
#ifndef SYSTOLIC_RANK_PARTITION_LATTICE_COUNT_H
#define SYSTOLIC_RANK_PARTITION_LATTICE_COUNT_H

#include "Systolic/RankPartitionSymbolicTilePartition.h"

#include "mlir/Analysis/Presburger/IntegerRelation.h"

#include <cstdint>
#include <limits>
#include <optional>
#include <vector>

namespace systolic::logical {

namespace lattice_count_detail {

inline std::optional<std::int64_t> toI64(const llvm::DynamicAPInt &v) {
  if (v < llvm::DynamicAPInt(std::numeric_limits<std::int64_t>::min()) ||
      v > llvm::DynamicAPInt(std::numeric_limits<std::int64_t>::max()))
    return std::nullopt;
  return static_cast<std::int64_t>(v);
}
inline std::optional<std::int64_t> toI64(const BigInt &v) {
  if (v < BigInt(std::numeric_limits<std::int64_t>::min()) ||
      v > BigInt(std::numeric_limits<std::int64_t>::max()))
    return std::nullopt;
  return v.convert_to<std::int64_t>();
}
// Exact floor/ceil of p/q, q != 0, rounding toward -inf / +inf.
inline std::int64_t floorDiv(std::int64_t p, std::int64_t q) {
  std::int64_t d = p / q, r = p % q;
  if (r != 0 && ((r < 0) != (q < 0)))
    --d;
  return d;
}
inline std::int64_t ceilDiv(std::int64_t p, std::int64_t q) {
  return -floorDiv(-p, q);
}

} // namespace lattice_count_detail

/// Count the integer points of a SOURCE-space convex polytope `src` (n dims, no
/// locals, no symbols) exactly, in closed form, IF it is a separable box: every
/// inequality and equality is axis-parallel (a single nonzero coordinate). The
/// count is the product of the per-axis integer-interval cardinalities. Returns
/// nullopt (UNKNOWN) on a non-axis-parallel face, an unbounded axis, or i64
/// overflow. Zero when the box is empty. No enumeration of lattice points.
inline std::optional<BigInt>
countSeparableBox(const mlir::presburger::IntegerPolyhedron &src) {
  using namespace lattice_count_detail;
  const unsigned n = src.getNumDimVars();
  if (src.getNumSymbolVars() != 0 || src.getNumLocalVars() != 0)
    return std::nullopt;
  if (n == 0)
    return BigInt(src.isIntegerEmpty() ? 0 : 1);

  std::vector<std::int64_t> lo(n, std::numeric_limits<std::int64_t>::min());
  std::vector<std::int64_t> hi(n, std::numeric_limits<std::int64_t>::max());

  // Returns the single nonzero axis of `row` (over the n dims), -2 on multiple
  // nonzero axes (non-separable), -1 when all-zero (a constant constraint).
  auto onlyAxis = [&](llvm::ArrayRef<llvm::DynamicAPInt> row,
                      std::int64_t &coeff, std::int64_t &constant) -> int {
    int axis = -1;
    coeff = 0;
    for (unsigned s = 0; s < n; ++s) {
      auto c = toI64(row[s]);
      if (!c)
        return -3; // overflow sentinel
      if (*c != 0) {
        if (axis != -1)
          return -2;
        axis = static_cast<int>(s);
        coeff = *c;
      }
    }
    auto k = toI64(row[n]);
    if (!k)
      return -3;
    constant = *k;
    return axis;
  };

  for (unsigned r = 0; r < src.getNumInequalities(); ++r) {
    std::int64_t a, k;
    int axis = onlyAxis(src.getInequality(r), a, k);
    if (axis == -3)
      return std::nullopt;
    if (axis == -2)
      return std::nullopt; // coupling face: not a separable box
    if (axis == -1) {       // constant: k >= 0, else empty
      if (k < 0)
        return BigInt(0);
      continue;
    }
    // a*x + k >= 0 : a>0 -> x >= ceil(-k/a); a<0 -> x <= floor(k/-a).
    if (a > 0) {
      std::int64_t b = ceilDiv(-k, a);
      if (b > lo[axis])
        lo[axis] = b;
    } else {
      std::int64_t b = floorDiv(k, -a);
      if (b < hi[axis])
        hi[axis] = b;
    }
  }
  for (unsigned r = 0; r < src.getNumEqualities(); ++r) {
    std::int64_t a, k;
    int axis = onlyAxis(src.getEquality(r), a, k);
    if (axis == -3)
      return std::nullopt;
    if (axis == -2)
      return std::nullopt;
    if (axis == -1) {
      if (k != 0)
        return BigInt(0);
      continue;
    }
    // a*x + k == 0 : x = -k/a (integral), else empty; pins the axis.
    if ((-k) % a != 0)
      return BigInt(0);
    std::int64_t v = (-k) / a;
    if (v > lo[axis])
      lo[axis] = v;
    if (v < hi[axis])
      hi[axis] = v;
  }

  BigInt product(1);
  for (unsigned s = 0; s < n; ++s) {
    if (lo[s] == std::numeric_limits<std::int64_t>::min() ||
        hi[s] == std::numeric_limits<std::int64_t>::max())
      return std::nullopt; // unbounded axis
    if (hi[s] < lo[s])
      return BigInt(0);
    product *= BigInt(hi[s] - lo[s] + 1);
  }
  return product;
}

/// Count the fibers (integer points) of one anchor `piece` over [z(n), u(added)]
/// by composing its pure-z faces with z = T*x (T[j][s] = source->logical, so
/// z_j = sum_s T[j][s]*x_s) into a DENSE source-space polytope, then counting
/// that as a separable box. A face that references a congruence local (nonzero
/// u-coefficient) is not pure-z and makes the piece unsupported here. Returns
/// nullopt (UNKNOWN) when not a separable box; no enumeration.
inline std::optional<BigInt>
countAnchorPieceFibers(const mlir::presburger::IntegerPolyhedron &piece,
                       const IntMatrix &T, unsigned n, unsigned added) {
  using namespace lattice_count_detail;
  using mlir::presburger::IntegerPolyhedron;
  using mlir::presburger::PresburgerSpace;
  if (piece.getNumDimVars() != n || piece.getNumLocalVars() != added ||
      T.size() != n)
    return std::nullopt;

  IntegerPolyhedron src(PresburgerSpace::getSetSpace(n, 0, 0));
  // Compose one [z,u] row into a source row over x (n dims + constant).
  auto compose = [&](llvm::ArrayRef<llvm::DynamicAPInt> row,
                     llvm::SmallVector<std::int64_t> &out) -> bool {
    for (unsigned l = 0; l < added; ++l) {
      auto c = toI64(row[n + l]);
      if (!c)
        return false;
      if (*c != 0)
        return false; // references a congruence local -> not pure-z
    }
    out.assign(n + 1, 0);
    for (unsigned s = 0; s < n; ++s) {
      BigInt acc = 0;
      for (unsigned j = 0; j < n; ++j) {
        auto cz = toI64(row[j]);
        if (!cz)
          return false;
        if (T[j].size() != n)
          return false;
        acc += BigInt(*cz) * T[j][s];
      }
      auto v = toI64(acc);
      if (!v)
        return false;
      out[s] = *v;
    }
    auto k = toI64(row[n + added]);
    if (!k)
      return false;
    out[n] = *k;
    return true;
  };
  for (unsigned r = 0; r < piece.getNumInequalities(); ++r) {
    llvm::SmallVector<std::int64_t> row;
    if (!compose(piece.getInequality(r), row))
      return std::nullopt;
    src.addInequality(row);
  }
  for (unsigned r = 0; r < piece.getNumEqualities(); ++r) {
    auto raw = piece.getEquality(r);
    bool referencesLocal = false;
    for (unsigned l = 0; l < added; ++l)
      if (raw[n + l] != llvm::DynamicAPInt(0))
        referencesLocal = true;
    if (referencesLocal)
      continue; // the lattice congruence z = T*u: automatic in source coords.
    llvm::SmallVector<std::int64_t> row;
    if (!compose(raw, row))
      return std::nullopt;
    src.addEquality(row);
  }
  return countSeparableBox(src);
}

/// The exact diagnostic counts of a certified single-reduction tile partition:
/// {outputFibers, sourceMACs}. `T` is the source->logical map (z = T*x).
/// Returns nullopt (UNKNOWN) if any anchor piece is outside the supported
/// counting class, so the caller refuses rather than reporting an approximation.
struct PartitionCounts {
  BigInt outputFibers = 0;
  BigInt sourceMACs = 0;
};
inline std::optional<PartitionCounts>
countPartition(const SymbolicTilePartition &partition, const IntMatrix &T,
               unsigned n) {
  PartitionCounts counts;
  const unsigned added = partition.congruenceLocals;
  for (const TileFiberGroup &group : partition.groups) {
    BigInt groupFibers = 0;
    for (const mlir::presburger::IntegerPolyhedron &piece : group.anchorPieces) {
      auto c = countAnchorPieceFibers(piece, T, n, added);
      if (!c)
        return std::nullopt;
      groupFibers += *c;
    }
    counts.outputFibers += groupFibers;
    counts.sourceMACs += BigInt(group.length) * groupFibers;
  }
  return counts;
}

} // namespace systolic::logical

#endif // SYSTOLIC_RANK_PARTITION_LATTICE_COUNT_H
