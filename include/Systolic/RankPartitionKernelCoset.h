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
#include "mlir/Analysis/Presburger/PresburgerSpace.h"

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
  IntMatrix Uinv;                                 // n x n, exact integer inverse
  unsigned outputRank = 0;                        // r = rank(C_rows)
  unsigned kernelRank = 0;                        // kappa = n - r
  std::vector<std::vector<BigInt>> kernelColumns; // kappa columns, each length n
  std::vector<unsigned> kernelColIndices;         // columns of U that span K_C
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
  // determinant(&adj) stores the ADJUGATE (U*adj = det*I); for det = +/-1 the
  // exact integer inverse is U^{-1} = det * adj (since 1/det == det).
  mlir::presburger::IntMatrix adj(n, n);
  llvm::DynamicAPInt det = U.determinant(&adj);
  if (det != llvm::DynamicAPInt(1) && det != llvm::DynamicAPInt(-1))
    return std::nullopt;
  mlir::presburger::IntMatrix Uinv(n, n);
  for (unsigned i = 0; i < n; ++i)
    for (unsigned j = 0; j < n; ++j)
      Uinv.at(i, j) = det * adj.at(i, j);
  // Certificate 2b: U * Uinv == I and Uinv * U == I (exact integer inverse).
  for (unsigned i = 0; i < n; ++i)
    for (unsigned j = 0; j < n; ++j) {
      llvm::DynamicAPInt a(0), b(0);
      for (unsigned t = 0; t < n; ++t) {
        a += U.at(i, t) * Uinv.at(t, j);
        b += Uinv.at(i, t) * U.at(t, j);
      }
      llvm::DynamicAPInt want(i == j ? 1 : 0);
      if (a != want || b != want)
        return std::nullopt;
    }

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

  // Convert U and Uinv to systolic BigInt (i64-overflow checked).
  out.U.assign(n, IntRow(n, BigInt(0)));
  out.Uinv.assign(n, IntRow(n, BigInt(0)));
  for (unsigned i = 0; i < n; ++i)
    for (unsigned j = 0; j < n; ++j) {
      auto v = fromAPInt(U.at(i, j));
      auto w = fromAPInt(Uinv.at(i, j));
      if (!v || !w)
        return std::nullopt;
      out.U[i][j] = *v;
      out.Uinv[i][j] = *w;
    }
  out.kernelColIndices = kernelCols;
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

//===----------------------------------------------------------------------===//
// M2a: exact symbolic kernel-coset fiber representation (analysis-only).
//===----------------------------------------------------------------------===//

/// Exact reduced domain D' = { y | U*y in D }. Every constraint of D over x is
/// composed with x = U*y (a_x . x -> (a_x^T U) . y), preserving all equalities,
/// inequalities and existential locals. Because U is unimodular the integer
/// points of D' map bijectively to those of D (no lattice points gained/lost).
/// No enumeration and no bounding-box/convex-shadow approximation. nullopt on
/// i64 overflow or an arity mismatch.
inline std::optional<mlir::presburger::IntegerPolyhedron>
buildReducedDomain(const IntMatrix &U,
                   const mlir::presburger::IntegerPolyhedron &D, unsigned n) {
  if (U.size() != n || D.getNumDimVars() != n || D.getNumSymbolVars() != 0)
    return std::nullopt;
  const unsigned locals = D.getNumLocalVars();
  for (unsigned s = 0; s < n; ++s)
    if (U[s].size() != n)
      return std::nullopt;
  mlir::presburger::IntegerPolyhedron Dp(
      mlir::presburger::PresburgerSpace::getSetSpace(n, 0, locals));
  const unsigned cols = n + locals + 1;
  auto compose = [&](llvm::ArrayRef<llvm::DynamicAPInt> row,
                     llvm::SmallVector<std::int64_t> &out) -> bool {
    out.assign(cols, 0);
    for (unsigned t = 0; t < n; ++t) {
      BigInt acc = 0;
      for (unsigned s = 0; s < n; ++s) {
        auto a = lattice_count_detail::toI64(row[s]);
        if (!a)
          return false;
        acc += BigInt(*a) * U[s][t];
      }
      auto v = lattice_count_detail::toI64(acc);
      if (!v)
        return false;
      out[t] = *v;
    }
    for (unsigned l = 0; l < locals + 1; ++l) { // locals + constant pass through
      auto a = lattice_count_detail::toI64(row[n + l]);
      if (!a)
        return false;
      out[n + l] = *a;
    }
    return true;
  };
  for (unsigned r = 0; r < D.getNumInequalities(); ++r) {
    llvm::SmallVector<std::int64_t> row;
    if (!compose(D.getInequality(r), row))
      return std::nullopt;
    Dp.addInequality(row);
  }
  for (unsigned r = 0; r < D.getNumEqualities(); ++r) {
    llvm::SmallVector<std::int64_t> row;
    if (!compose(D.getEquality(r), row))
      return std::nullopt;
    Dp.addEquality(row);
  }
  return Dp;
}

/// Source-order certificate status. M2a records Unverified; M3 discharges or
/// rejects it. An Unverified plan is a symbolic REPRESENTATION only -- never a
/// license to reorder a strict-FP reduction.
enum class SourceOrderStatus {
  Unverified,
  SourceLexEquivalent,
  ReassociationRequired
};

/// M2a: an exact symbolic representation of the integer kernel-coset fibers.
/// Holds NO array of source points or materialized fibers; a fiber is produced
/// on demand as the Presburger slice of `reducedDomain` at a fixed output
/// coordinate (fiberSlice). `outputCols`/`reductionCols` index the reduced
/// coordinates y (x = U*y); in y the kernel is axis-aligned, and the ragged
/// fiber boundary is carried by the output-dependent faces of `reducedDomain`.
struct KernelCosetFiberPlan {
  IntMatrix U, Uinv;
  unsigned outputRank = 0, kernelRank = 0;
  mlir::presburger::IntegerPolyhedron reducedDomain; // D'
  std::vector<unsigned> outputCols;                  // r output y-columns
  std::vector<unsigned> reductionCols;               // kappa reduction y-columns
  BigInt sourceMACs = 0;
  BigInt outputFibers = 0;
  SourceOrderStatus sourceOrder = SourceOrderStatus::Unverified;

  explicit KernelCosetFiberPlan(mlir::presburger::IntegerPolyhedron d)
      : reducedDomain(std::move(d)) {}
};

/// Build the M2a fiber plan from a physical-C access matrix and source domain.
/// Reuses M0 (kernel + unimodular U/Uinv) and M1 (exact counts). nullopt
/// (UNKNOWN) when M0 or M1 is UNKNOWN, or on overflow. The reduced domain is
/// exact; source order is left Unverified for M3. No enumeration.
inline std::optional<KernelCosetFiberPlan>
buildKernelCosetFiberPlan(const IntMatrix &Crows,
                          const mlir::presburger::IntegerPolyhedron &D,
                          unsigned n) {
  auto kb = buildIntegerKernelBasis(Crows, n);
  if (!kb)
    return std::nullopt;
  auto counts = countKernelCosetCounts(Crows, D, n);
  if (!counts)
    return std::nullopt;
  auto dp = buildReducedDomain(kb->U, D, n);
  if (!dp)
    return std::nullopt;
  KernelCosetFiberPlan plan(std::move(*dp));
  plan.U = kb->U;
  plan.Uinv = kb->Uinv;
  plan.outputRank = kb->outputRank;
  plan.kernelRank = kb->kernelRank;
  plan.reductionCols = kb->kernelColIndices;
  std::vector<bool> isKernel(n, false);
  for (unsigned c : kb->kernelColIndices)
    if (c < n)
      isKernel[c] = true;
  for (unsigned j = 0; j < n; ++j)
    if (!isKernel[j])
      plan.outputCols.push_back(j);
  plan.sourceMACs = counts->sourceMACs;
  plan.outputFibers = counts->outputFibers;
  plan.sourceOrder = SourceOrderStatus::Unverified;
  return plan;
}

/// One fiber as the exact Presburger slice of `reducedDomain` with each output
/// coordinate pinned to `yOut[i]`. Its integer points are the reduced
/// coordinates y of that fiber; x = U*y recovers the source iterations of one
/// physical-C address. Exact -- a sub-polytope, never a bounding box. nullopt
/// on an arity mismatch. No enumeration: the slice is symbolic.
inline std::optional<mlir::presburger::IntegerPolyhedron>
fiberSlice(const KernelCosetFiberPlan &plan, llvm::ArrayRef<std::int64_t> yOut) {
  if (yOut.size() != plan.outputCols.size())
    return std::nullopt;
  mlir::presburger::IntegerPolyhedron slice = plan.reducedDomain;
  const unsigned cols = slice.getNumCols();
  for (std::size_t i = 0; i < plan.outputCols.size(); ++i) {
    llvm::SmallVector<std::int64_t> eq(cols, 0);
    eq[plan.outputCols[i]] = 1;
    eq[cols - 1] = -yOut[i];
    slice.addEquality(eq);
  }
  return slice;
}

//===----------------------------------------------------------------------===//
// M3: exact source-order certification (analysis-only).
//
// The canonical traversal of a KernelCosetFiberPlan is LEXICOGRAPHIC over the
// reduction coordinates y_red (reductionCols, outermost first) -- the natural
// nested order of a peel-and-tile over the reduced domain. The certificate asks
// whether that traversal accumulates every physical-C fiber in the ORIGINAL
// source order (lexicographic over the source iteration variables x). A
// unimodular change of basis need NOT preserve lexicographic order, so this is
// proved, not assumed.
//===----------------------------------------------------------------------===//

enum class OrderVerdict {
  SourceLexEquivalent,    // y_red-lex traversal == source x-lex on every fiber
  NotSourceLexEquivalent, // a same-fiber pair is reversed (witness provided)
  Unknown                 // overflow / malformed; legality is NEVER inferred
};

struct SourceOrderCertificate {
  OrderVerdict verdict = OrderVerdict::Unknown;
  // For NotSourceLexEquivalent: two distinct same-physical-C source points,
  // `earlier` strictly before `later` in source x-lex, but AFTER it in the
  // y_red-lex traversal (the reversal). Empty if a witness could not be read.
  std::vector<std::int64_t> earlier;
  std::vector<std::int64_t> later;
};

/// Certify whether `plan`'s y_red-lex traversal preserves source x-lex order on
/// every physical-C fiber. Pure Presburger reasoning: for each (source level p,
/// traversal level q) it builds the exact system over two source points
/// (x1, x2) that are (a) both in the source domain, (b) in the same fiber
/// (equal output coordinates), (c) source-ordered x1 strictly before x2 at
/// level p, and (d) traversal-ordered x1 strictly AFTER x2 at level q; a single
/// satisfiable system is a witnessed order reversal. Empty for all (p, q) means
/// equivalent. No source-point enumeration: O(n * kappa) integer-emptiness
/// checks over affine systems, independent of the domain cardinality. Unknown
/// on i64 overflow or a malformed plan (legality is never inferred).
inline SourceOrderCertificate
certifyKernelCosetSourceOrder(const KernelCosetFiberPlan &plan) {
  using mlir::presburger::IntegerPolyhedron;
  using mlir::presburger::PresburgerSpace;
  SourceOrderCertificate cert;
  const unsigned n = static_cast<unsigned>(plan.U.size());
  if (n == 0 || plan.Uinv.size() != n)
    return cert; // Unknown
  for (unsigned i = 0; i < n; ++i)
    if (plan.Uinv[i].size() != n)
      return cert;

  // Reconstruct the source domain D = { x | Uinv*x in D' } (locals pass through).
  auto Dopt = buildReducedDomain(plan.Uinv, plan.reducedDomain, n);
  if (!Dopt)
    return cert; // Unknown (overflow / malformed)
  const IntegerPolyhedron &D = *Dopt;
  if (D.getNumDimVars() != n || D.getNumSymbolVars() != 0)
    return cert;
  const unsigned L = D.getNumLocalVars();
  const unsigned dims = 2 * n, locs = 2 * L, cols = dims + locs + 1;

  // Uinv row k (a y-coordinate) as i64 coefficients over x; nullopt on overflow.
  auto uinv = [&](unsigned k) -> std::optional<std::vector<std::int64_t>> {
    std::vector<std::int64_t> r(n, 0);
    for (unsigned t = 0; t < n; ++t) {
      auto v = lattice_count_detail::toI64(plan.Uinv[k][t]);
      if (!v)
        return std::nullopt;
      r[t] = *v;
    }
    return r;
  };

  const unsigned kappa = static_cast<unsigned>(plan.reductionCols.size());
  for (unsigned p = 0; p < n; ++p)
    for (unsigned q = 0; q < kappa; ++q) {
      IntegerPolyhedron S(PresburgerSpace::getSetSpace(dims, 0, locs));
      bool overflow = false;
      // Embed D onto (x @ xOff, locals @ locOff).
      auto embed = [&](unsigned xOff, unsigned locOff) {
        auto one = [&](llvm::ArrayRef<llvm::DynamicAPInt> row, bool eq) {
          llvm::SmallVector<std::int64_t> out(cols, 0);
          for (unsigned j = 0; j < n; ++j) {
            auto v = lattice_count_detail::toI64(row[j]);
            if (!v) { overflow = true; return; }
            out[xOff + j] = *v;
          }
          for (unsigned l = 0; l < L; ++l) {
            auto v = lattice_count_detail::toI64(row[n + l]);
            if (!v) { overflow = true; return; }
            out[dims + locOff + l] = *v;
          }
          auto c = lattice_count_detail::toI64(row[n + L]);
          if (!c) { overflow = true; return; }
          out[cols - 1] = *c;
          if (eq) S.addEquality(out);
          else S.addInequality(out);
        };
        for (unsigned r = 0; r < D.getNumInequalities(); ++r)
          one(D.getInequality(r), false);
        for (unsigned r = 0; r < D.getNumEqualities(); ++r)
          one(D.getEquality(r), true);
      };
      embed(0, 0);
      embed(n, L);
      if (overflow) {
        cert.verdict = OrderVerdict::Unknown;
        return cert;
      }
      // Same fiber: y_out(x1) == y_out(x2) for every output coordinate.
      for (unsigned oc : plan.outputCols) {
        auto u = uinv(oc);
        if (!u) { cert.verdict = OrderVerdict::Unknown; return cert; }
        llvm::SmallVector<std::int64_t> eq(cols, 0);
        for (unsigned t = 0; t < n; ++t) {
          eq[t] = (*u)[t];
          eq[n + t] = -(*u)[t];
        }
        S.addEquality(eq);
      }
      // Source level p: x1[s] == x2[s] for s < p; x1[p] <= x2[p] - 1.
      for (unsigned s = 0; s < p; ++s) {
        llvm::SmallVector<std::int64_t> eq(cols, 0);
        eq[s] = 1;
        eq[n + s] = -1;
        S.addEquality(eq);
      }
      {
        llvm::SmallVector<std::int64_t> ge(cols, 0);
        ge[p] = -1;      // -x1[p]
        ge[n + p] = 1;   // +x2[p]
        ge[cols - 1] = -1;
        S.addInequality(ge); // x2[p] - x1[p] - 1 >= 0
      }
      // Traversal level q (reversed): y_red(x1)[i] == y_red(x2)[i] for i < q;
      // y_red(x1)[q] >= y_red(x2)[q] + 1.
      bool ov = false;
      for (unsigned i = 0; i < q; ++i) {
        auto u = uinv(plan.reductionCols[i]);
        if (!u) { ov = true; break; }
        llvm::SmallVector<std::int64_t> eq(cols, 0);
        for (unsigned t = 0; t < n; ++t) {
          eq[t] = (*u)[t];
          eq[n + t] = -(*u)[t];
        }
        S.addEquality(eq);
      }
      if (ov) { cert.verdict = OrderVerdict::Unknown; return cert; }
      {
        auto u = uinv(plan.reductionCols[q]);
        if (!u) { cert.verdict = OrderVerdict::Unknown; return cert; }
        llvm::SmallVector<std::int64_t> ge(cols, 0);
        for (unsigned t = 0; t < n; ++t) {
          ge[t] = (*u)[t];       // +y_red(x1)[q]
          ge[n + t] = -(*u)[t];  // -y_red(x2)[q]
        }
        ge[cols - 1] = -1;
        S.addInequality(ge); // (y1q - y2q) - 1 >= 0
      }
      if (!S.isIntegerEmpty()) {
        cert.verdict = OrderVerdict::NotSourceLexEquivalent;
        if (auto s = S.findIntegerSample()) {
          cert.earlier.assign(n, 0);
          cert.later.assign(n, 0);
          bool ok = true;
          for (unsigned t = 0; t < n && ok; ++t) {
            auto a = lattice_count_detail::toI64((*s)[t]);
            auto b = lattice_count_detail::toI64((*s)[n + t]);
            if (!a || !b)
              ok = false;
            else {
              cert.earlier[t] = *a;
              cert.later[t] = *b;
            }
          }
          if (!ok) {
            cert.earlier.clear();
            cert.later.clear();
          }
        }
        return cert;
      }
    }
  cert.verdict = OrderVerdict::SourceLexEquivalent;
  return cert;
}

} // namespace systolic::logical

#endif // SYSTOLIC_RANK_PARTITION_KERNEL_COSET_H
