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

#include <algorithm>
#include <cstdint>
#include <limits>
#include <optional>
#include <set>
#include <utility>
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

//===----------------------------------------------------------------------===//
// M-count-2: EXACT symbolic counting of certified NON-RECTANGULAR pieces.
//
// The separable-box counter above refuses (UNKNOWN) any source polytope with a
// coupling face (two nonzero source coefficients): triangular, trapezoidal,
// banded and simplicial pieces. M-count-2 counts those exactly and in closed
// form, still WITHOUT visiting individual lattice points, by decomposing the
// source polytope into independent variable blocks (the connected components of
// the "share a face" graph) and counting each block with a proven closed form:
//
//   * a 1-variable block  -> an integer interval (floor/ceil endpoints);
//   * a 2-variable block  -> the lower/upper envelope split into O(faces^2)
//        x-chambers, each a single affine band summed by Faulhaber -- this is
//        exactly the triangular / trapezoidal / banded class (unit coupling);
//   * a d-variable block  -> a standard simplex { x_i >= lo_i, sum x_i <= K },
//        counted by the binomial C(N+d, d), N = K - sum lo_i.
//
// Distinct blocks are independent, so the piece's count is the product of the
// per-block counts; a separable box is the all-1-D case, so this STRICTLY
// EXTENDS countSeparableBox. Any block outside the three proven forms -- a
// non-unit coupling coefficient, an unbounded axis, an unrecognised d-D shape,
// or a 2-D envelope that still has competing lines after chamber splitting --
// makes the whole count UNKNOWN, so the caller refuses rather than
// approximating. No per-point enumeration and no bounding box anywhere: every
// count is a finite sequence of exact integer operations whose length depends
// only on the number of faces and variables.
//
// These are ADDED entry points. The box-only countSeparableBox /
// countAnchorPieceFibers / countPartition above keep their exact prior
// semantics unchanged (they still return UNKNOWN on any coupling face).
//===----------------------------------------------------------------------===//

namespace lattice_count_detail {

// gcd of |a|, |b| (0 is the additive identity: i64gcd(0,x) == |x|).
inline std::int64_t i64gcd(std::int64_t a, std::int64_t b) {
  a = a < 0 ? -a : a;
  b = b < 0 ? -b : b;
  while (b != 0) {
    std::int64_t t = a % b;
    a = b;
    b = t;
  }
  return a;
}

// Floor/ceil of p/q (q != 0) in exact big integers, toward -inf / +inf.
inline BigInt bFloorDiv(const BigInt &p, const BigInt &q) {
  BigInt d = p / q, r = p % q;
  if (r != 0 && ((r < 0) != (q < 0)))
    --d;
  return d;
}
inline BigInt bCeilDiv(const BigInt &p, const BigInt &q) {
  return -bFloorDiv(-p, q);
}

// One face a.x + c >= 0 of a block, restricted to that block's variable columns.
struct BlockFace {
  std::vector<std::int64_t> a;
  std::int64_t c;
};

// A 1-variable block: the integer interval cut by its faces. nullopt if the
// axis is unbounded; 0 if empty. No enumeration.
inline std::optional<BigInt>
count1DInterval(const std::vector<BlockFace> &faces) {
  std::int64_t lo = std::numeric_limits<std::int64_t>::min();
  std::int64_t hi = std::numeric_limits<std::int64_t>::max();
  for (const BlockFace &f : faces) {
    std::int64_t a = f.a[0], c = f.c;
    if (a == 0) {
      if (c < 0)
        return BigInt(0);
      continue;
    }
    if (a > 0) {
      std::int64_t b = ceilDiv(-c, a);
      if (b > lo)
        lo = b;
    } else {
      std::int64_t b = floorDiv(c, -a);
      if (b < hi)
        hi = b;
    }
  }
  if (lo == std::numeric_limits<std::int64_t>::min() ||
      hi == std::numeric_limits<std::int64_t>::max())
    return std::nullopt;
  if (hi < lo)
    return BigInt(0);
  return BigInt(hi) - BigInt(lo) + 1;
}

// A 2-variable block over (x = column 0, y = column 1). Supported iff every
// coupling face has unit y-coefficient (|b| == 1): the triangular / trapezoidal
// / banded class. The y-band is [max lower lines, min upper lines]; we split x
// at the integers bracketing every lower-lower and upper-upper crossing, so
// inside each chamber the active lower and upper line are each single and the
// per-x band width is one affine function, summed in closed form (Faulhaber).
// nullopt (UNKNOWN) on |b|>1, an unbounded axis, or a chamber that still has
// competing lines at its ends (never approximated). No point enumeration: the
// chamber count is O(faces^2) and each chamber is an O(1) sum.
inline std::optional<BigInt>
count2DChamber(const std::vector<BlockFace> &faces) {
  std::vector<std::pair<std::int64_t, std::int64_t>> upper; // y <= p*x + q
  std::vector<std::pair<std::int64_t, std::int64_t>> lower; // y >= p*x + q
  std::int64_t xlo = std::numeric_limits<std::int64_t>::min();
  std::int64_t xhi = std::numeric_limits<std::int64_t>::max();
  for (const BlockFace &f : faces) {
    std::int64_t a = f.a[0], b = f.a[1], c = f.c;
    if (b == 0) {
      if (a == 0) {
        if (c < 0)
          return BigInt(0);
        continue;
      }
      if (a > 0) {
        std::int64_t v = ceilDiv(-c, a);
        if (v > xlo)
          xlo = v;
      } else {
        std::int64_t v = floorDiv(c, -a);
        if (v < xhi)
          xhi = v;
      }
      continue;
    }
    if (b != 1 && b != -1)
      return std::nullopt; // non-unit coupling: outside the supported class
    if (b == -1)
      upper.push_back({a, c}); // a*x - y + c >= 0 -> y <= a*x + c
    else
      lower.push_back({-a, -c}); // a*x + y + c >= 0 -> y >= -a*x - c
  }
  if (upper.empty() || lower.empty())
    return std::nullopt; // y unbounded on a side
  if (xlo == std::numeric_limits<std::int64_t>::min() ||
      xhi == std::numeric_limits<std::int64_t>::max())
    return std::nullopt; // x unbounded
  if (xhi < xlo)
    return BigInt(0);

  // Breakpoints: integers bracketing each lower-lower and upper-upper crossing.
  std::set<std::int64_t> br;
  br.insert(xlo);
  auto cross = [&](std::pair<std::int64_t, std::int64_t> f,
                   std::pair<std::int64_t, std::int64_t> g) {
    if (f.first == g.first)
      return;
    BigInt num = BigInt(g.second) - f.second;
    BigInt den = BigInt(f.first) - g.first;
    BigInt fl = bFloorDiv(num, den);
    for (BigInt v = fl - 1; v <= fl + 2; ++v)
      if (v >= xlo && v <= xhi)
        br.insert(v.convert_to<std::int64_t>());
  };
  for (std::size_t i = 0; i < upper.size(); ++i)
    for (std::size_t j = i + 1; j < upper.size(); ++j)
      cross(upper[i], upper[j]);
  for (std::size_t i = 0; i < lower.size(); ++i)
    for (std::size_t j = i + 1; j < lower.size(); ++j)
      cross(lower[i], lower[j]);

  std::vector<std::int64_t> bounds(br.begin(), br.end());
  bounds.push_back(xhi);
  std::sort(bounds.begin(), bounds.end());
  bounds.erase(std::unique(bounds.begin(), bounds.end()), bounds.end());

  auto activeUpper = [&](std::int64_t x) {
    std::pair<std::int64_t, std::int64_t> best = upper[0];
    BigInt bestV = BigInt(upper[0].first) * x + upper[0].second;
    for (const auto &u : upper) {
      BigInt v = BigInt(u.first) * x + u.second;
      if (v < bestV) {
        bestV = v;
        best = u;
      }
    }
    return best;
  };
  auto activeLower = [&](std::int64_t x) {
    std::pair<std::int64_t, std::int64_t> best = lower[0];
    BigInt bestV = BigInt(lower[0].first) * x + lower[0].second;
    for (const auto &l : lower) {
      BigInt v = BigInt(l.first) * x + l.second;
      if (v > bestV) {
        bestV = v;
        best = l;
      }
    }
    return best;
  };

  BigInt total(0);
  for (std::size_t bi = 0; bi < bounds.size(); ++bi) {
    std::int64_t s = bounds[bi];
    std::int64_t e = (bi + 1 < bounds.size()) ? bounds[bi + 1] - 1 : xhi;
    if (bi + 1 < bounds.size() && bounds[bi + 1] == s)
      continue;
    if (s > e || s < xlo || s > xhi)
      continue;
    if (e > xhi)
      e = xhi;
    auto us = activeUpper(s), ue = activeUpper(e);
    auto ls = activeLower(s), le = activeLower(e);
    if (us != ue || ls != le)
      return std::nullopt; // competing envelope lines remain -> refuse
    BigInt p = BigInt(us.first) - ls.first;
    BigInt q = BigInt(us.second) - ls.second + 1; // band width = p*x + q
    BigInt a = s, b = e;
    BigInt gs = p * a + q, ge = p * b + q;
    if (gs < 0 && ge < 0)
      continue;
    if (p > 0) {
      BigInt z = bCeilDiv(-q, p);
      if (z > a)
        a = z;
    } else if (p < 0) {
      BigInt z = bFloorDiv(-q, p);
      if (z < b)
        b = z;
    } else if (q < 0) {
      continue;
    }
    if (a > b)
      continue;
    BigInt npts = b - a + 1;
    BigInt sumx = (a + b) * npts / 2;
    total += p * sumx + q * npts;
  }
  return total;
}

// A d-variable block that is a standard simplex: exactly d single-variable lower
// bounds x_i >= lo_i (positive leading coefficient) and one face with all
// coefficients -1, i.e. sum x_i <= K. Count = C(N + d, d), N = K - sum lo_i, by
// the exact multiplicative binomial. nullopt otherwise (e.g. a single-variable
// upper bound, a missing lower bound, or two simplex faces). No enumeration.
inline std::optional<BigInt>
countSimplexForm(const std::vector<BlockFace> &faces, unsigned d) {
  std::vector<std::int64_t> lo(d, std::numeric_limits<std::int64_t>::min());
  int simplexFace = -1;
  std::int64_t K = 0;
  for (std::size_t fi = 0; fi < faces.size(); ++fi) {
    const BlockFace &f = faces[fi];
    int nz = 0, ax = -1;
    for (unsigned s = 0; s < d; ++s)
      if (f.a[s] != 0) {
        ++nz;
        ax = static_cast<int>(s);
      }
    if (nz == 0) {
      if (f.c < 0)
        return BigInt(0);
      continue;
    }
    if (nz == 1) {
      std::int64_t a = f.a[ax], c = f.c;
      if (a <= 0)
        return std::nullopt; // a single-variable UPPER bound: not a simplex face
      std::int64_t v = ceilDiv(-c, a);
      if (v > lo[ax])
        lo[ax] = v;
    } else {
      bool allNeg1 = true;
      for (unsigned s = 0; s < d; ++s)
        if (f.a[s] != -1)
          allNeg1 = false;
      if (!allNeg1 || simplexFace != -1)
        return std::nullopt;
      simplexFace = static_cast<int>(fi);
      K = f.c;
    }
  }
  if (simplexFace < 0)
    return std::nullopt;
  for (unsigned s = 0; s < d; ++s)
    if (lo[s] == std::numeric_limits<std::int64_t>::min())
      return std::nullopt;
  BigInt N(K);
  for (unsigned s = 0; s < d; ++s)
    N -= lo[s];
  if (N < 0)
    return BigInt(0);
  BigInt num(1);
  for (unsigned i = 1; i <= d; ++i)
    num = num * (N + i) / i;
  return num;
}

} // namespace lattice_count_detail

/// Count the integer points of a SOURCE-space convex polytope `src` (n dims, no
/// locals, no symbols) exactly and in closed form when it decomposes into
/// independent 1-D interval, 2-D unit-coupled (triangular / trapezoidal /
/// banded) and d-D simplex blocks. STRICTLY EXTENDS countSeparableBox (a box is
/// the all-1-D case). Returns nullopt (UNKNOWN) when any block is outside those
/// proven forms, or on i64 coefficient overflow. Zero when empty. No enumeration.
inline std::optional<BigInt>
countBlockSeparable(const mlir::presburger::IntegerPolyhedron &src) {
  using namespace lattice_count_detail;
  const unsigned n = src.getNumDimVars();
  if (src.getNumSymbolVars() != 0 || src.getNumLocalVars() != 0)
    return std::nullopt;
  if (n == 0)
    return BigInt(src.isIntegerEmpty() ? 0 : 1);

  // All faces as a.x + c >= 0 rows (an equality becomes two inequalities).
  std::vector<std::vector<std::int64_t>> rows;
  auto addRow = [&](llvm::ArrayRef<llvm::DynamicAPInt> r,
                    std::int64_t sign) -> bool {
    std::vector<std::int64_t> v(n + 1);
    for (unsigned j = 0; j <= n; ++j) {
      auto c = toI64(r[j]);
      if (!c)
        return false;
      v[j] = sign * *c;
    }
    rows.push_back(std::move(v));
    return true;
  };
  for (unsigned r = 0; r < src.getNumInequalities(); ++r)
    if (!addRow(src.getInequality(r), 1))
      return std::nullopt;
  for (unsigned r = 0; r < src.getNumEqualities(); ++r)
    if (!addRow(src.getEquality(r), 1) || !addRow(src.getEquality(r), -1))
      return std::nullopt;

  // Reduce every face to primitive integer form: divide the variable
  // coefficients by their gcd g and replace the constant c by floor(c/g). This
  // is count-preserving -- a.x + c >= 0 with g | a.x is equivalent to
  // (a/g).x + floor(c/g) >= 0 -- and is required so that unit-coupling and
  // simplex recognition survive a determinant-scaled composition (e.g. a face
  // 3y >= 0 or -3x-3y+9 >= 0 becomes y >= 0 or -x-y+3 >= 0). All-zero rows
  // (constant faces) are left untouched.
  for (auto &row : rows) {
    std::int64_t g = 0;
    for (unsigned j = 0; j < n; ++j)
      g = i64gcd(g, row[j]);
    if (g > 1) {
      for (unsigned j = 0; j < n; ++j)
        row[j] /= g;
      row[n] = floorDiv(row[n], g);
    }
  }

  // Union-find: couple variables that share a face (iterative, path-halving).
  std::vector<unsigned> uf(n);
  for (unsigned i = 0; i < n; ++i)
    uf[i] = i;
  auto find = [&](unsigned x) -> unsigned {
    while (uf[x] != x) {
      uf[x] = uf[uf[x]];
      x = uf[x];
    }
    return x;
  };
  for (const auto &row : rows) {
    int first = -1;
    for (unsigned j = 0; j < n; ++j)
      if (row[j] != 0) {
        if (first < 0)
          first = static_cast<int>(j);
        else
          uf[find(j)] = find(static_cast<unsigned>(first));
      }
  }

  std::vector<std::vector<unsigned>> blocks(n);
  for (unsigned j = 0; j < n; ++j)
    blocks[find(j)].push_back(j);

  BigInt product(1);
  for (unsigned b = 0; b < n; ++b) {
    if (blocks[b].empty())
      continue;
    const std::vector<unsigned> &vs = blocks[b];
    std::vector<BlockFace> bf;
    for (const auto &row : rows) {
      bool rel = false, other = false;
      for (unsigned j = 0; j < n; ++j)
        if (row[j] != 0) {
          if (std::find(vs.begin(), vs.end(), j) != vs.end())
            rel = true;
          else
            other = true;
        }
      if (rel && other)
        return std::nullopt; // a single face couples two blocks: inconsistent
      if (!rel) {
        // A face with no coefficient on this block's variables: either a
        // genuine all-zero constant face (feasibility check), or a face that
        // lives purely in ANOTHER block (skip -- counted when that block runs).
        if (!other && row[n] < 0)
          return BigInt(0);
        continue;
      }
      BlockFace f;
      f.a.resize(vs.size());
      for (std::size_t k = 0; k < vs.size(); ++k)
        f.a[k] = row[vs[k]];
      f.c = row[n];
      bf.push_back(std::move(f));
    }
    std::optional<BigInt> c;
    if (vs.size() == 1)
      c = count1DInterval(bf);
    else if (vs.size() == 2) {
      c = count2DChamber(bf);
      if (!c)
        c = countSimplexForm(bf, 2); // 2-D simplex with no explicit x bound
    } else
      c = countSimplexForm(bf, static_cast<unsigned>(vs.size()));
    if (!c)
      return std::nullopt;
    if (*c == 0)
      return BigInt(0);
    product *= *c;
  }
  return product;
}

/// Compose one anchor `piece` over [z(n), u(added)] into its DENSE source-space
/// polytope over x via z = T*x (the identical composition countAnchorPieceFibers
/// performs: pure-z faces only; congruence-local faces are the lattice relation
/// and drop out in source coordinates). nullopt on a non-pure-z face / overflow.
inline std::optional<mlir::presburger::IntegerPolyhedron>
composeAnchorPieceToSource(const mlir::presburger::IntegerPolyhedron &piece,
                           const IntMatrix &T, unsigned n, unsigned added) {
  using namespace lattice_count_detail;
  using mlir::presburger::IntegerPolyhedron;
  using mlir::presburger::PresburgerSpace;
  if (piece.getNumDimVars() != n || piece.getNumLocalVars() != added ||
      T.size() != n)
    return std::nullopt;
  IntegerPolyhedron src(PresburgerSpace::getSetSpace(n, 0, 0));
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
  return src;
}

/// As countAnchorPieceFibers, but counts the full M-count-2 class (block-
/// separable: 1-D / 2-D unit-coupled / d-D simplex) exactly. nullopt (UNKNOWN)
/// when the composed source polytope is outside that class.
inline std::optional<BigInt>
countAnchorPieceFibersBlocks(const mlir::presburger::IntegerPolyhedron &piece,
                             const IntMatrix &T, unsigned n, unsigned added) {
  auto src = composeAnchorPieceToSource(piece, T, n, added);
  if (!src)
    return std::nullopt;
  return countBlockSeparable(*src);
}

/// As countPartition, but using the M-count-2 block-separable counter, so
/// triangular / trapezoidal / banded / simplicial pieces are counted exactly
/// instead of refused. nullopt (UNKNOWN) if any piece is outside the supported
/// class. The box-only countPartition keeps its prior (stricter) semantics.
inline std::optional<PartitionCounts>
countPartitionBlocks(const SymbolicTilePartition &partition, const IntMatrix &T,
                     unsigned n) {
  PartitionCounts counts;
  const unsigned added = partition.congruenceLocals;
  for (const TileFiberGroup &group : partition.groups) {
    BigInt groupFibers = 0;
    for (const mlir::presburger::IntegerPolyhedron &piece : group.anchorPieces) {
      auto c = countAnchorPieceFibersBlocks(piece, T, n, added);
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
