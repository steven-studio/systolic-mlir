//===- RankPartitionKernelCoset.h - General kernel-coset partition -*- C++ -*-===//
//
// C4a-sym General Integer Kernel-Coset Partition, milestones M0 and M1.
//
// A physical-C output fiber is a coset of the INTEGER kernel K_C = ker_Z(C_rows)
// intersected with the source domain D: two source iterations write the same
// physical C element iff their difference lies in K_C (row-major linearization
// is injective on the in-bounds index box, so a physical-address collision is a
// logical-coordinate collision). This header provides, ANALYSIS-ONLY and with
// NO source-point enumeration:
//
//   M0  buildIntegerKernelBasis -- the exact integer right kernel of C_rows via
//       the column Hermite normal form H = C_rows * U (U unimodular). The last
//       (n - rank) columns of U are a COMPLETE integer basis of K_C (not merely
//       a rational spanning set); det(U) = +/-1 certifies the whole basis is a
//       Z-basis of Z^n, so by rank-nullity the kernel columns generate the full
//       integer kernel. All conversions are i64-overflow checked.
//
//   M1  countKernelCosetCounts -- exact symbolic counts
//         sourceMACs   = |D|            (reuses M-count-2 countBlockSeparable)
//         outputFibers = |C_rows(D)|    (distinct physical C addresses)
//       For a RANK-ONE physical C the image is counted exactly by a proven
//       contiguity certificate over a box domain: never the convex projection,
//       never a bounding box. A projection that might contain holes returns
//       UNKNOWN. Rank >= 2 images, non-box domains, and overflow return UNKNOWN.
//
// This header is consumed only by the unit tests; it is NOT wired into any
// production pass (M2/M3/M4 are later milestones).
//
//===----------------------------------------------------------------------===//
#ifndef SYSTOLIC_RANK_PARTITION_KERNEL_COSET_H
#define SYSTOLIC_RANK_PARTITION_KERNEL_COSET_H

#include "Systolic/RankPartitionLatticeCount.h"
#include "Systolic/TensorLogicalSpaces.h"

#include "mlir/Analysis/Presburger/IntegerRelation.h"
#include "mlir/Analysis/Presburger/Matrix.h"

#include <algorithm>
#include <optional>
#include <utility>
#include <vector>

namespace systolic::logical {

namespace kernel_coset_detail {

inline BigInt bigAbs(const BigInt &v) { return v < 0 ? BigInt(-v) : v; }
inline BigInt bigGcd(BigInt a, BigInt b) {
  a = bigAbs(a);
  b = bigAbs(b);
  while (b != 0) {
    BigInt t = a % b;
    a = b;
    b = t;
  }
  return a;
}
// systolic BigInt -> MLIR DynamicAPInt, nullopt on i64 overflow (fail closed).
inline std::optional<llvm::DynamicAPInt> toAPInt(const BigInt &v) {
  auto i = lattice_count_detail::toI64(v);
  if (!i)
    return std::nullopt;
  return llvm::DynamicAPInt(*i);
}
// MLIR DynamicAPInt -> systolic BigInt, nullopt on i64 overflow (fail closed).
inline std::optional<BigInt> fromAPInt(const llvm::DynamicAPInt &v) {
  if (v < llvm::DynamicAPInt(std::numeric_limits<std::int64_t>::min()) ||
      v > llvm::DynamicAPInt(std::numeric_limits<std::int64_t>::max()))
    return std::nullopt;
  return BigInt(static_cast<std::int64_t>(v));
}

} // namespace kernel_coset_detail

/// M0: the exact integer right kernel of a physical access matrix.
/// `U` is unimodular (det +/-1) with C_rows * U = [H | 0]; `kernelColumns`
/// (kernelRank of them, each length n) are a COMPLETE integer basis of
/// ker_Z(C_rows). outputRank = rank(C_rows), kernelRank = n - outputRank.
struct IntegerKernelBasis {
  IntMatrix U;                                    // n x n, systolic BigInt
  unsigned outputRank = 0;                        // r = rank(C_rows)
  unsigned kernelRank = 0;                        // kappa = n - r
  std::vector<std::vector<BigInt>> kernelColumns; // kappa columns, each length n
};

inline std::optional<IntegerKernelBasis>
buildIntegerKernelBasis(const IntMatrix &Crows, unsigned n) {
  using namespace kernel_coset_detail;
  if (n == 0)
    return std::nullopt;
  const unsigned m = static_cast<unsigned>(Crows.size());
  if (m == 0)
    return std::nullopt;

  // Build the MLIR integer matrix (i64-overflow checked).
  mlir::presburger::IntMatrix C(m, n);
  for (unsigned i = 0; i < m; ++i) {
    if (Crows[i].size() != n)
      return std::nullopt;
    for (unsigned j = 0; j < n; ++j) {
      auto a = toAPInt(Crows[i][j]);
      if (!a)
        return std::nullopt;
      C.at(i, j) = *a;
    }
  }

  // Column Hermite normal form: H = C * U, U unimodular.
  auto hu = C.computeHermiteNormalForm();
  mlir::presburger::IntMatrix H = hu.first;
  mlir::presburger::IntMatrix U = hu.second;
  if (U.getNumRows() != n || U.getNumColumns() != n ||
      H.getNumRows() != m || H.getNumColumns() != n)
    return std::nullopt;

  // Certificate 1: C * U == H exactly.
  for (unsigned i = 0; i < m; ++i)
    for (unsigned j = 0; j < n; ++j) {
      llvm::DynamicAPInt s(0);
      for (unsigned t = 0; t < n; ++t)
        s += C.at(i, t) * U.at(t, j);
      if (s != H.at(i, j))
        return std::nullopt;
    }

  // Certificate 2: U unimodular (det +/-1) -> its columns are a Z-basis of Z^n,
  // so the kernel columns are a COMPLETE integer basis of K_C (not rational).
  llvm::DynamicAPInt det = U.determinant(nullptr);
  if (det != llvm::DynamicAPInt(1) && det != llvm::DynamicAPInt(-1))
    return std::nullopt;

  // Kernel columns = the all-zero columns of H (C * U_col = 0); the rest count
  // toward the rank. H is a column HNF, so its nonzero columns are the rank.
  IntegerKernelBasis out;
  unsigned r = 0;
  std::vector<unsigned> kernelCols;
  for (unsigned j = 0; j < n; ++j) {
    bool zero = true;
    for (unsigned i = 0; i < m; ++i)
      if (H.at(i, j) != llvm::DynamicAPInt(0)) {
        zero = false;
        break;
      }
    if (zero)
      kernelCols.push_back(j);
    else
      ++r;
  }
  out.outputRank = r;
  out.kernelRank = n - r;
  // Certificate 3 (rank-nullity): kernel column count == n - rank.
  if (static_cast<unsigned>(kernelCols.size()) != out.kernelRank)
    return std::nullopt;

  // Convert U to systolic BigInt (i64-overflow checked).
  out.U.assign(n, IntRow(n, BigInt(0)));
  for (unsigned i = 0; i < n; ++i)
    for (unsigned j = 0; j < n; ++j) {
      auto v = fromAPInt(U.at(i, j));
      if (!v)
        return std::nullopt;
      out.U[i][j] = *v;
    }
  for (unsigned col : kernelCols) {
    std::vector<BigInt> v(n, BigInt(0));
    for (unsigned i = 0; i < n; ++i)
      v[i] = out.U[i][col];
    out.kernelColumns.push_back(std::move(v));
  }
  return out;
}

/// M1 result.
struct KernelCosetCounts {
  BigInt sourceMACs = 0;
  BigInt outputFibers = 0;
};

/// Exact count of DISTINCT integer values of a single linear form `row` over an
/// axis-parallel box domain `D`, with a sound contiguity certificate: the image
/// of the primitive form is the contiguous interval [min, max] iff, processing
/// the axes by ascending |coefficient|, each coefficient is <= running_range+1
/// (so the shifted copies abut). Then the count is range+1. A coefficient that
/// fails the test may leave a hole, so the function returns nullopt (UNKNOWN)
/// rather than the (over-counting) convex-projection length. nullopt also when
/// D is not exactly the box, is unbounded, or carries locals. No enumeration.
inline std::optional<BigInt>
countRank1ImageContiguous(const IntRow &row,
                          const mlir::presburger::IntegerPolyhedron &D,
                          unsigned n) {
  using namespace kernel_coset_detail;
  using mlir::presburger::BoundType;
  if (row.size() != n)
    return std::nullopt;
  if (D.getNumDimVars() != n || D.getNumLocalVars() != 0 ||
      D.getNumSymbolVars() != 0)
    return std::nullopt;

  // Per-axis bounds, and require D to be EXACTLY the box they define (so each
  // axis contributes an independent span). A non-box (coupled/triangular) D is
  // out of this certificate's class -> UNKNOWN.
  std::vector<std::int64_t> lo(n), hi(n);
  BigInt boxCount(1);
  for (unsigned s = 0; s < n; ++s) {
    auto l = D.getConstantBound64(BoundType::LB, s);
    auto h = D.getConstantBound64(BoundType::UB, s);
    if (!l || !h || *h < *l)
      return std::nullopt;
    lo[s] = *l;
    hi[s] = *h;
    boxCount *= BigInt(*h - *l + 1);
  }
  auto dcount = countBlockSeparable(D);
  if (!dcount || *dcount != boxCount)
    return std::nullopt;

  // Primitive coefficients (divide the row by its gcd); the distinct-value
  // count is invariant under this scaling.
  BigInt g(0);
  for (unsigned s = 0; s < n; ++s)
    g = bigGcd(g, row[s]);
  if (g == 0)
    return BigInt(1); // all-zero form: a single image value.

  std::vector<std::pair<BigInt, BigInt>> terms; // (|primitive coeff|, span)
  for (unsigned s = 0; s < n; ++s) {
    if (row[s] == 0)
      continue;
    BigInt c = bigAbs(row[s] / g);
    BigInt w = BigInt(hi[s] - lo[s]);
    if (w == 0)
      continue; // pinned axis contributes no spread.
    terms.push_back({c, w});
  }
  std::sort(terms.begin(), terms.end(),
            [](const std::pair<BigInt, BigInt> &a,
               const std::pair<BigInt, BigInt> &b) { return a.first < b.first; });
  BigInt R(0);
  for (const auto &t : terms) {
    if (t.first > R + 1)
      return std::nullopt; // possible hole -> UNKNOWN (never the convex length).
    R += t.first * t.second;
  }
  return R + 1;
}

/// M1: exact sourceMACs = |D| and outputFibers = |C_rows(D)| for the kernel-
/// coset partition. Reuses M-count-2 for |D|. outputFibers is exact for a
/// rank-0 (constant) or rank-one physical C over a box domain; rank >= 2
/// images, non-box domains, holey projections and overflow return UNKNOWN.
inline std::optional<KernelCosetCounts>
countKernelCosetCounts(const IntMatrix &Crows,
                       const mlir::presburger::IntegerPolyhedron &D,
                       unsigned n) {
  KernelCosetCounts out;
  auto sm = countBlockSeparable(D);
  if (!sm)
    return std::nullopt; // D outside M-count-2's supported class.
  out.sourceMACs = *sm;
  if (out.sourceMACs == 0) {
    out.outputFibers = 0; // empty domain: no fibers.
    return out;
  }
  auto kb = buildIntegerKernelBasis(Crows, n);
  if (!kb)
    return std::nullopt;
  if (kb->outputRank == 0) {
    out.outputFibers = 1; // C is constant: every iteration one physical C.
    return out;
  }
  if (kb->outputRank == 1) {
    const IntRow *r0 = nullptr;
    for (const IntRow &r : Crows) {
      for (const BigInt &v : r)
        if (v != 0) {
          r0 = &r;
          break;
        }
      if (r0)
        break;
    }
    if (!r0)
      return std::nullopt;
    auto of = countRank1ImageContiguous(*r0, D, n);
    if (!of)
      return std::nullopt;
    out.outputFibers = *of;
    return out;
  }
  return std::nullopt; // rank >= 2 image: out of M1 scope -> UNKNOWN.
}

} // namespace systolic::logical

#endif // SYSTOLIC_RANK_PARTITION_KERNEL_COSET_H
