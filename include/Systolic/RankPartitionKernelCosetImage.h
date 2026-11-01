//===- RankPartitionKernelCosetImage.h - Exact symbolic output scan -*- C++ -*-===//
//
// Phase M5b-1 (Option C, Architecture D of the M5a design review): the EXACT
// symbolic output scan of a physical-C image of any rank, analysis-only.
//
// For the recognized MAC statement C[h_C(x)] += A[h_A(x)] * B[h_B(x)], x in D
// (D the integer points of a polytope over the n source loop variables), the
// physical outputs are S = h_C(D). With the column Hermite normal form
// H = C_rows * U (U unimodular, RankPartitionKernelCoset.h M0) and the reduced
// coordinates y = U^-1 x, D' = { y | U y in D }, the output address depends on
// the output columns alone, c = H_out * y_out + offset, and
//
//     S = H_out * O' + offset,   O' = proj_out(D') = { y_out | exists y_ker :
//                                                     (y_out, y_ker) in D' },
//
// BIJECTIVELY (H_out has full column rank). So a scan that visits every
// integer point of O' exactly once visits every physical output exactly once,
// and lattice holes / congruences of a non-unimodular C (C[2i][j], C[i+j][i-j],
// C[2i][3j], the det-9 skew) never need a guard: a c is generated only through
// its integer preimage coordinate y_out.
//
// O' is an integer projection. It is produced here as a NEST of exact
// ceil/floor faces (one level per output column, outermost first; the bounds of
// a level are affine in the enclosing levels) by iterated Fourier-Motzkin
// elimination of the kernel columns and then of the inner output columns,
// where EVERY elimination is certified integer-exact by the unit-coefficient
// criterion (the eliminated column has coefficient in {-1, 0, 1} in every
// constraint of the current system, so the rational shadow is the integer
// projection and a unit equality is an exact substitution). A step that cannot
// be certified makes the whole scan UNKNOWN -- the rational projection is never
// assumed to be the integer projection. For a rank-ONE image the M1 contiguity
// certificate (countKernelCosetCounts) is accepted as the alternative exact
// certificate of the single output level (the committed path's own).
//
// The reduction fibers are the existing kernel-coset machinery, unchanged: the
// M4b.1 tile faces with the M5b-0 parameter-only feasibility constraints
// (tileInterval is the exact slice of D' at every (output, peel)); the peel
// level gets exact faces when the elimination of the tile column is certified,
// otherwise its constant box (a valid over-approximation: a peel value outside
// the projection has an EMPTY exact tile interval, so no MAC is executed and
// the fiber is unchanged -- but then the tile faces alone are exact on every
// box chamber only when the feasibility constraints are box-implied;
// `fiberScanExact` records exactly that).
//
// Analysis only: no IR, no source-point or output-point enumeration, no
// cardinality budget, no bounding-box or convex-hull substitution, no Presburger
// set difference. Declines (UNKNOWN, with a reason): symbols/locals in D,
// replication (unused) source axes, pure box GEMM (unit-coordinate views over
// a box: owned by the clean box path), kappa == 0 or kappa > 2, rank-0 C, an
// empty domain, non-unit eliminated coefficients, unbounded levels, overflow.
//
// Consumed by the unit tests only; NOT wired into any pass (M5b-2 emission is a
// later milestone).
//
//===----------------------------------------------------------------------===//
#ifndef SYSTOLIC_RANK_PARTITION_KERNEL_COSET_IMAGE_H
#define SYSTOLIC_RANK_PARTITION_KERNEL_COSET_IMAGE_H

#include "Systolic/RankPartitionKernelCoset.h"
#include "Systolic/RankPartitionLatticeCount.h"

#include <algorithm>
#include <array>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace systolic::logical {

/// One scanned level of the nest: the reduced column `col`, bounded by exact
/// ceil/floor faces over the enclosing levels' columns `paramCols` (nest order,
/// outermost first; a face's `params` align with `paramCols`). `exact` is true
/// when the faces are the certified integer projection; false only for the
/// peel-level box fallback (a valid over-approximation, see the file comment).
struct KernelCosetScanLevel {
  unsigned col = 0;
  std::vector<unsigned> paramCols;
  std::vector<TileBoundFace> lowerCeil;  // col >= ceil((params.p + c) / d)
  std::vector<TileBoundFace> upperFloor; // col <= floor((params.p + c) / d)
  bool exact = false;
};

/// The exact integer interval of a level at the given enclosing values:
/// [max over lower faces, min over upper faces]; lo > hi means empty. nullopt
/// on an arity mismatch or a level without a bound on one side.
inline std::optional<std::pair<BigInt, BigInt>>
scanLevelInterval(const KernelCosetScanLevel &lvl,
                  llvm::ArrayRef<std::int64_t> params) {
  if (lvl.lowerCeil.empty() || lvl.upperFloor.empty() ||
      params.size() != lvl.paramCols.size())
    return std::nullopt;
  std::optional<BigInt> lo, hi;
  for (const auto &f : lvl.lowerCeil) {
    auto v = evalTileFace(f, params, /*lower=*/true);
    if (!v)
      return std::nullopt;
    lo = lo ? std::max(*lo, *v) : *v;
  }
  for (const auto &f : lvl.upperFloor) {
    auto v = evalTileFace(f, params, /*lower=*/false);
    if (!v)
      return std::nullopt;
    hi = hi ? std::min(*hi, *v) : *v;
  }
  return std::make_pair(*lo, *hi);
}

/// The symbolic output scan. Holds no array of points or fibers.
struct KernelCosetOutputScan {
  /// U, Uinv, D', outputCols, reductionCols, peelOrder and the M3 verdict. Its
  /// counts (sourceMACs/outputFibers) are NOT computed here (left 0): no
  /// decision depends on a cardinality.
  KernelCosetScanPlan scan;
  /// H = C_rows * U: the physical C index is H[:, outputCols] * y_out + offset
  /// (H is zero on the kernel columns, verified).
  IntMatrix H;
  /// Output columns in nest order (outermost first) and their exact levels.
  std::vector<unsigned> outputOrder;
  std::vector<KernelCosetScanLevel> outputLevels;
  /// Peel levels (kappa - 1 of them), after the outputs in the nest.
  std::vector<KernelCosetScanLevel> peelLevels;
  bool peelExact = false;
  /// M4b.1 tile faces + M5b-0 feasibility constraints (params = the plan's
  /// outputCols ++ peel columns, in that stored order).
  KernelCosetTileBounds tile;
  /// peelExact || tile.feasibilityBoxImplied: the (peel, tile) nest executes
  /// exactly the fiber of every output without any further guard.
  bool fiberScanExact = false;
  /// "projection" (certified unit-coefficient eliminations) or "contiguity"
  /// (rank-one M1 certificate).
  const char *outputCertificate = "";
  /// |D| when M-count-2 can count it; informational only.
  std::optional<BigInt> sourceMACs;

  explicit KernelCosetOutputScan(KernelCosetScanPlan plan)
      : scan(std::move(plan)) {}
};

namespace kernel_coset_image_detail {

/// A projected system together with the ORIGINAL reduced column of each dim.
struct Projected {
  mlir::presburger::IntegerPolyhedron P;
  std::vector<unsigned> cols; // cols[pos] = original y column of dim `pos`
};

inline std::optional<unsigned> positionOf(const Projected &pr, unsigned col) {
  for (unsigned i = 0; i < pr.cols.size(); ++i)
    if (pr.cols[i] == col)
      return i;
  return std::nullopt;
}

/// Eliminate original column `col` from `pr` with the integer-exactness
/// certificate: in the CURRENT system every inequality and equality has
/// coefficient in {-1, 0, 1} on the column (Fourier-Motzkin with unit
/// coefficients is exact over Z; a unit equality is an exact substitution),
/// and the projection introduces no local. Otherwise false (UNKNOWN).
inline bool eliminateExact(Projected &pr, unsigned col, std::string &why) {
  auto pos = positionOf(pr, col);
  if (!pos) {
    why = "UNKNOWN: column to eliminate is not in the current system";
    return false;
  }
  const llvm::DynamicAPInt zero(0), one(1), minusOne(-1);
  auto unit = [&](llvm::ArrayRef<llvm::DynamicAPInt> row) {
    const llvm::DynamicAPInt &v = row[*pos];
    return v == zero || v == one || v == minusOne;
  };
  for (unsigned r = 0; r < pr.P.getNumInequalities(); ++r)
    if (!unit(pr.P.getInequality(r))) {
      why = "UNKNOWN: non-unit coefficient on an eliminated reduced column -- "
            "the projection is not certified integer-exact";
      return false;
    }
  for (unsigned r = 0; r < pr.P.getNumEqualities(); ++r)
    if (!unit(pr.P.getEquality(r))) {
      why = "UNKNOWN: non-unit equality coefficient on an eliminated reduced "
            "column -- the projection is not certified integer-exact";
      return false;
    }
  pr.P.projectOut(*pos, 1);
  if (pr.P.getNumLocalVars() != 0 || pr.P.getNumSymbolVars() != 0) {
    why = "UNKNOWN: the projection introduced locals";
    return false;
  }
  pr.cols.erase(pr.cols.begin() + *pos);
  return true;
}

/// Read the exact faces of original column `col` from the current system,
/// whose other columns must all be among `paramCols` (the enclosing levels, in
/// nest order). A row with zero coefficient on `col` constrains the enclosing
/// levels only and is enforced there (the enclosing systems are its exact
/// projections). An equality yields a lower and an upper face of the same
/// expression (empty when not divisible: exact). false (UNKNOWN) on overflow,
/// a stray column, or a level unbounded on one side.
inline bool readFaces(const Projected &pr, unsigned col,
                      const std::vector<unsigned> &paramCols,
                      KernelCosetScanLevel &lvl, std::string &why) {
  using lattice_count_detail::toI64;
  auto pos = positionOf(pr, col);
  if (!pos) {
    why = "UNKNOWN: level column is not in the current system";
    return false;
  }
  std::vector<std::optional<unsigned>> paramPos(paramCols.size());
  for (std::size_t i = 0; i < paramCols.size(); ++i)
    paramPos[i] = positionOf(pr, paramCols[i]);
  for (unsigned c : pr.cols)
    if (c != col &&
        std::find(paramCols.begin(), paramCols.end(), c) == paramCols.end()) {
      why = "UNKNOWN: a reduced column is neither the level nor an enclosing "
            "parameter";
      return false;
    }
  if (pr.P.getNumLocalVars() != 0) {
    why = "UNKNOWN: locals in the projected system";
    return false;
  }
  const unsigned constIdx = pr.P.getNumCols() - 1;
  lvl = KernelCosetScanLevel();
  lvl.col = col;
  lvl.paramCols = paramCols;
  auto readRow = [&](llvm::ArrayRef<llvm::DynamicAPInt> row,
                     bool equality) -> bool {
    auto a = toI64(row[*pos]);
    if (!a)
      return false;
    if (*a == 0)
      return true; // a constraint on the enclosing levels only.
    std::vector<BigInt> b(paramCols.size(), BigInt(0));
    for (std::size_t i = 0; i < paramCols.size(); ++i) {
      if (!paramPos[i])
        continue; // not in this system: coefficient zero.
      auto v = toI64(row[*paramPos[i]]);
      if (!v)
        return false;
      b[i] = BigInt(*v);
    }
    auto c = toI64(row[constIdx]);
    if (!c)
      return false;
    // a*col + b.p + c (>= | ==) 0.
    TileBoundFace f;
    f.params = b;
    if (*a > 0) { // col >= ceil((-b.p - c) / a); equality: also <= floor(.)
      for (auto &v : f.params)
        v = -v;
      f.constant = BigInt(-*c);
      f.divisor = BigInt(*a);
      lvl.lowerCeil.push_back(f);
      if (equality)
        lvl.upperFloor.push_back(f);
    } else { // col <= floor((b.p + c) / |a|); equality: also >= ceil(.)
      f.constant = BigInt(*c);
      f.divisor = BigInt(-*a);
      lvl.upperFloor.push_back(f);
      if (equality)
        lvl.lowerCeil.push_back(f);
    }
    return true;
  };
  for (unsigned r = 0; r < pr.P.getNumInequalities(); ++r)
    if (!readRow(pr.P.getInequality(r), /*equality=*/false)) {
      why = "UNKNOWN: i64 overflow in a level face";
      return false;
    }
  for (unsigned r = 0; r < pr.P.getNumEqualities(); ++r)
    if (!readRow(pr.P.getEquality(r), /*equality=*/true)) {
      why = "UNKNOWN: i64 overflow in a level face";
      return false;
    }
  if (lvl.lowerCeil.empty() || lvl.upperFloor.empty()) {
    why = "UNKNOWN: an output level is unbounded on one side";
    return false;
  }
  lvl.exact = true;
  return true;
}

} // namespace kernel_coset_image_detail

/// Build the exact symbolic output scan of the statement with access matrices
/// `access` = {A, B, C} rows over the n source loops and source domain `D`.
/// nullopt (UNKNOWN) with `why` on every decline listed in the file comment.
/// Complexity: O(n^3) HNF + at most (kappa + rank) projections of a system
/// with m constraints (each at worst O(m^2) new constraints) + O(rows * rank)
/// face extraction, over at most rank! output orders -- independent of |D|
/// and |S|. No enumeration anywhere.
inline std::optional<KernelCosetOutputScan>
buildKernelCosetOutputScan(const std::array<IntMatrix, 3> &access,
                           const mlir::presburger::IntegerPolyhedron &D,
                           unsigned n, std::string &why) {
  using namespace kernel_coset_image_detail;
  using mlir::presburger::BoundType;
  using mlir::presburger::IntegerPolyhedron;
  why.clear();

  // ---- Shape ---------------------------------------------------------
  if (n == 0) {
    why = "UNKNOWN: degenerate source rank";
    return std::nullopt;
  }
  if (D.getNumDimVars() != n || D.getNumSymbolVars() != 0 ||
      D.getNumLocalVars() != 0) {
    why = "UNKNOWN: the source domain carries symbols or locals (congruence) "
          "-- out of M5b-1 scope";
    return std::nullopt;
  }
  for (unsigned op = 0; op < 3; ++op)
    for (const IntRow &row : access[op])
      if (row.size() != n) {
        why = "UNKNOWN: access row arity";
        return std::nullopt;
      }
  if (access[2].empty()) {
    why = "UNKNOWN: no physical C access";
    return std::nullopt;
  }

  // ---- Policy: replication axes are refused (kept on the existing routes).
  for (unsigned s = 0; s < n; ++s) {
    bool unused = true;
    for (unsigned op = 0; op < 3 && unused; ++op)
      for (const IntRow &row : access[op])
        if (row[s] != 0) {
          unused = false;
          break;
        }
    if (unused) {
      why = "declined: source axis " + std::to_string(s) +
            " is a replication (unused) axis -- out of M5b-1 scope";
      return std::nullopt;
    }
  }
  // ---- Policy: a pure box GEMM (unit-coordinate views over a box domain) is
  // owned by the clean box path; never duplicate it here.
  {
    bool unitViews = true;
    for (unsigned op = 0; op < 3 && unitViews; ++op)
      for (const IntRow &row : access[op]) {
        unsigned nonzero = 0;
        bool unitEntry = true;
        for (const BigInt &v : row)
          if (v != 0) {
            ++nonzero;
            if (v != 1)
              unitEntry = false;
          }
        if (nonzero != 1 || !unitEntry) {
          unitViews = false;
          break;
        }
      }
    auto axisAligned = [&](llvm::ArrayRef<llvm::DynamicAPInt> row) {
      unsigned nonzero = 0;
      for (unsigned s = 0; s < n; ++s)
        if (row[s] != llvm::DynamicAPInt(0))
          ++nonzero;
      return nonzero <= 1;
    };
    bool axisBox = true;
    for (unsigned r = 0; r < D.getNumInequalities() && axisBox; ++r)
      axisBox = axisAligned(D.getInequality(r));
    for (unsigned r = 0; r < D.getNumEqualities() && axisBox; ++r)
      axisBox = axisAligned(D.getEquality(r));
    if (unitViews && axisBox) {
      why = "declined: pure box GEMM (unit-coordinate views over a box domain) "
            "is owned by the clean box path";
      return std::nullopt;
    }
  }

  // ---- M0: integer kernel, unimodular U --------------------------------
  auto kb = buildIntegerKernelBasis(access[2], n);
  if (!kb) {
    why = "UNKNOWN: integer kernel basis (overflow or malformed C rows)";
    return std::nullopt;
  }
  const unsigned rho = kb->outputRank, kappa = kb->kernelRank;
  if (rho == 0) {
    why = "UNKNOWN: constant physical C (rank 0) -- out of M5b-1 scope";
    return std::nullopt;
  }
  if (kappa == 0) {
    why = "UNKNOWN: no reduction coordinate (kappa == 0)";
    return std::nullopt;
  }
  if (kappa > 2) {
    why = "UNKNOWN: more than two reduction coordinates (kappa > 2) -- M4a "
          "scan scope";
    return std::nullopt;
  }

  // ---- D' and the count-free fiber/scan plan -----------------------------
  auto dp = buildReducedDomain(kb->U, D, n);
  if (!dp) {
    why = "UNKNOWN: reduced domain (i64 overflow)";
    return std::nullopt;
  }
  if (dp->isIntegerEmpty()) {
    why = "UNKNOWN: empty source domain";
    return std::nullopt;
  }
  KernelCosetFiberPlan fp(*dp);
  fp.U = kb->U;
  fp.Uinv = kb->Uinv;
  fp.outputRank = rho;
  fp.kernelRank = kappa;
  fp.reductionCols = kb->kernelColIndices;
  {
    std::vector<bool> isKernel(n, false);
    for (unsigned c : kb->kernelColIndices)
      if (c < n)
        isKernel[c] = true;
    for (unsigned j = 0; j < n; ++j)
      if (!isKernel[j])
        fp.outputCols.push_back(j);
  }
  if (fp.outputCols.size() != rho) {
    why = "UNKNOWN: kernel/output column bookkeeping";
    return std::nullopt;
  }
  auto cert = certifyKernelCosetSourceOrder(fp);
  KernelCosetScanPlan plan(std::move(fp));
  plan.peelOrder = plan.fiber.reductionCols; // outermost first; tile = last.
  plan.innerStride = 1;
  plan.sourceOrder = cert.verdict;
  plan.tileLegality = TileLegality::StaticKTileable;

  // ---- The tile: M4b.1 faces + M5b-0 feasibility ----------------------
  auto tb = buildKernelCosetTileBounds(plan);
  if (!tb || !tb->exact) {
    why = "UNKNOWN: exact tile bounds (pinned tile, unbounded axis, or "
          "overflow)";
    return std::nullopt;
  }

  // ---- H = C_rows * U, zero on the kernel columns --------------------------
  IntMatrix H;
  for (const IntRow &row : access[2]) {
    IntRow h(n, BigInt(0));
    for (unsigned t = 0; t < n; ++t)
      for (unsigned s = 0; s < n; ++s)
        h[t] += row[s] * kb->U[s][t];
    for (unsigned c : plan.peelOrder)
      if (h[c] != 0) {
        why = "UNKNOWN: physical C depends on a reduction coordinate";
        return std::nullopt;
      }
    H.push_back(std::move(h));
  }

  KernelCosetOutputScan out(std::move(plan));
  out.tile = *tb;
  out.H = std::move(H);
  const KernelCosetScanPlan &scan = out.scan;
  const unsigned tileCol = scan.peelOrder.back();

  // ---- Output projection: eliminate the kernel columns (either order) -----
  Projected base{*dp, {}};
  for (unsigned c = 0; c < n; ++c)
    base.cols.push_back(c);
  std::optional<Projected> outOnly;
  std::string kernelWhy;
  {
    std::vector<std::vector<unsigned>> orders;
    std::vector<unsigned> tileFirst;
    tileFirst.push_back(tileCol);
    for (unsigned i = 0; i + 1 < scan.peelOrder.size(); ++i)
      tileFirst.push_back(scan.peelOrder[i]);
    orders.push_back(tileFirst);
    if (kappa == 2)
      orders.push_back({scan.peelOrder[0], tileCol});
    for (const auto &order : orders) {
      Projected pr = base;
      bool ok = true;
      for (unsigned c : order)
        if (!eliminateExact(pr, c, kernelWhy)) {
          ok = false;
          break;
        }
      if (ok) {
        outOnly = std::move(pr);
        break;
      }
    }
  }

  // ---- Output levels ---------------------------------------------------
  if (outOnly) {
    std::vector<unsigned> perm = scan.fiber.outputCols; // ascending = identity
    std::sort(perm.begin(), perm.end());
    bool found = false;
    std::string lastWhy;
    do {
      Projected pr = *outOnly;
      std::vector<KernelCosetScanLevel> levels(rho);
      bool ok = true;
      for (unsigned j = rho; j-- > 1;) {
        std::vector<unsigned> params(perm.begin(), perm.begin() + j);
        if (!readFaces(pr, perm[j], params, levels[j], lastWhy) ||
            !eliminateExact(pr, perm[j], lastWhy)) {
          ok = false;
          break;
        }
      }
      if (ok && !readFaces(pr, perm[0], {}, levels[0], lastWhy))
        ok = false;
      if (ok) {
        out.outputOrder = perm;
        out.outputLevels = std::move(levels);
        out.outputCertificate = "projection";
        found = true;
        break;
      }
    } while (std::next_permutation(perm.begin(), perm.end()));
    if (!found) {
      why = lastWhy.empty()
                ? "UNKNOWN: no output loop order has a certified integer-exact "
                  "projection"
                : lastWhy + " (no output loop order is certified)";
      return std::nullopt;
    }
  } else if (rho == 1) {
    // Rank-one fallback: the committed M1 contiguity certificate -- the image
    // of the single output column is exactly the interval [lo, hi] of its
    // constant bounds, with hi - lo + 1 attained values (never the convex
    // length without the certificate).
    auto counts = countKernelCosetCounts(access[2], D, n);
    const unsigned col = scan.fiber.outputCols[0];
    auto lo = dp->getConstantBound64(BoundType::LB, col);
    auto hi = dp->getConstantBound64(BoundType::UB, col);
    if (!counts || !lo || !hi || *hi < *lo ||
        counts->outputFibers != BigInt(*hi - *lo + 1)) {
      why = kernelWhy + " (and the rank-one contiguity certificate does not "
                        "apply)";
      return std::nullopt;
    }
    KernelCosetScanLevel lvl;
    lvl.col = col;
    TileBoundFace lower, upper;
    lower.constant = BigInt(*lo);
    lower.divisor = BigInt(1);
    upper.constant = BigInt(*hi);
    upper.divisor = BigInt(1);
    lvl.lowerCeil.push_back(lower);
    lvl.upperFloor.push_back(upper);
    lvl.exact = true;
    out.outputOrder = {col};
    out.outputLevels = {lvl};
    out.outputCertificate = "contiguity";
    out.sourceMACs = counts->sourceMACs;
  } else {
    why = kernelWhy + " (rank >= 2 image: no alternative certificate)";
    return std::nullopt;
  }

  // ---- Peel level (kappa == 2): exact faces after a certified elimination of
  // the tile column; otherwise its constant box, a valid over-approximation
  // (an over-approximated peel value has an EMPTY exact tile interval).
  if (kappa == 2) {
    const unsigned peelCol = scan.peelOrder[0];
    Projected pt = base;
    std::string pw;
    KernelCosetScanLevel lvl;
    if (eliminateExact(pt, tileCol, pw) &&
        readFaces(pt, peelCol, out.outputOrder, lvl, pw)) {
      out.peelLevels.push_back(std::move(lvl));
      out.peelExact = true;
    } else {
      KernelCosetScanLevel box;
      box.col = peelCol;
      std::optional<std::size_t> idx;
      for (std::size_t i = 0; i < tb->paramCols.size(); ++i)
        if (tb->paramCols[i] == peelCol)
          idx = i;
      if (!idx) {
        why = "UNKNOWN: peel column bookkeeping";
        return std::nullopt;
      }
      TileBoundFace lower, upper;
      lower.constant = BigInt(tb->outerLo[*idx]);
      lower.divisor = BigInt(1);
      upper.constant = BigInt(tb->outerHi[*idx]);
      upper.divisor = BigInt(1);
      box.lowerCeil.push_back(lower);
      box.upperFloor.push_back(upper);
      box.exact = false;
      out.peelLevels.push_back(std::move(box));
      out.peelExact = false;
    }
  } else {
    out.peelExact = true; // no peel level.
  }
  out.fiberScanExact = out.peelExact || out.tile.feasibilityBoxImplied;
  if (!out.sourceMACs)
    out.sourceMACs = countBlockSeparable(D); // informational; may be nullopt.
  return out;
}

} // namespace systolic::logical

#endif // SYSTOLIC_RANK_PARTITION_KERNEL_COSET_IMAGE_H
