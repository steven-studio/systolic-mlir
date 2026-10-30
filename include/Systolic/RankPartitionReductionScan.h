//===- RankPartitionReductionScan.h - Option C C3c-3 M1 ----------*- C++ -*-===//
//
// Phase C3c-3 M1 (Option C): exact symbolic partitioning of a genuine
// two-dimensional, convex, NON-rectangular reduction domain (triangular,
// banded, trapezoidal, with rectangular as a regression control) by the
// peel-and-tile architecture. Analysis only: it emits no IR and produces a
// symbolic plan, not device operations.
//
// A two-axis reduction K = (k_outer, k_inner) is scanned as a loop nest:
//
//   for k_outer in [outerLo, outerHi]:            // peeled accumulation loop
//     for k_inner in fiber(output, k_outer):      // tiled by the C3 partitioner
//       C[output] += A[...] * B[...]              // all accumulate into one C
//
// The inner reduction is tiled by the EXISTING symbolic tile partitioner. To
// reuse it verbatim, the outer axis is appended to the output (C) access,
// turning the problem into a synthetic BATCHED GEMM whose batch is k_outer and
// whose single reduction is k_inner; buildSymbolicTilePartition then tiles the
// inner with the outer as an output coordinate, producing CONSTANT-length tile
// groups (constant device K per group) whose lengths may depend on k_outer and
// the real output -- exactly the non-rectangular structure (e.g. triangular ->
// group lengths 1,2,3,4). The appended C axis is an analysis device only: the
// REAL output address drops it, and the plan marks k_outer as an ACCUMULATION
// coordinate, never a physical output.
//
// Soundness obligations discharged symbolically (no source-point enumeration,
// no bounding box, no nonlinear ranking):
//   * the real output access is injective on the real outputs (M, N, genuine
//     batch) -- distinct physical outputs never alias -- rank(C) == n - r;
//   * the real output is constant along BOTH reduction axes, so accumulating
//     over (k_outer, k_inner) into one C element is valid;
//   * the inner reduction is tileable (buildSymbolicTilePartition succeeds),
//     yielding constant-K groups (so no dynamically-varying single tile is
//     assumed -- any varying inner K factors into constant-length groups);
//   * the outer axis has a constant (output-independent) integer range;
//   * every coefficient and bound is i64-representable.
// Any failure returns nullopt (UNKNOWN). This is M1: exactly two reduction
// axes, constant outer range, convex fiber. Higher reduction rank, batch-
// dependent outer ranges, congruence coupling, and non-convex fibers are later
// milestones and return UNKNOWN here.
//
// The flattened order is outer-then-inner. Whether it equals the SOURCE loop
// order is not recoverable from the access maps, so orderMatchesSourceForStrictFP
// stays false: under strict floating point the order is a caller obligation;
// integer reductions are order-independent. MAC-multiset equivalence is proven
// here regardless; strict-FP execution equivalence is explicitly NOT claimed.
//
#ifndef SYSTOLIC_RANK_PARTITION_REDUCTION_SCAN_H
#define SYSTOLIC_RANK_PARTITION_REDUCTION_SCAN_H

#include "Systolic/RankPartitionExpansion.h"
#include "Systolic/RankPartitionSymbolicTilePartition.h"

#include "mlir/Analysis/Presburger/IntegerRelation.h"
#include "mlir/Analysis/Presburger/PresburgerSpace.h"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace systolic::logical {

/// Symbolic plan for a peeled two-axis reduction. `outerCol`/`innerCol` are the
/// logical reduction columns; `outerLo..outerHi` is the peeled axis's constant
/// integer range; `innerStride` is the inner fiber stride (1 unless a one-local
/// congruence applies); `innerConstantTileLengths` are the constant device-K
/// group lengths the inner reduction factors into (its size may depend on the
/// outer coordinate and the output, but each group is a constant K).
/// `realOutputAccess` is the physical C access, which excludes the reduction
/// axes; `outerIsAccumulation` records that k_outer is summed into C, not a
/// physical output.
/// One affine bound on the peeled outer reduction axis, over the OUTPUT
/// coordinates: the bound value is `coeffs . z + constant`, with `coeffs` an
/// integer row over the logical coordinates that is zero on the two reduction
/// columns (so the value depends only on the output). A lower face means
/// k_outer >= value; an upper face means k_outer <= value. The exact per-output
/// outer range is [ max over lowerFaces, min over upperFaces ].
struct OuterBoundFace {
  IntRow coeffs; // length logicalRank; zero on outerCol and innerCol
  BigInt constant = 0;
};

/// One peeled outer reduction level: the reduction axis `col` and its exact
/// piecewise-affine bounds, affine in the output and the more-significant outer
/// reduction axes (the enclosing levels). The range is
/// [ max over lowerFaces, min over upperFaces ].
struct ReductionPeelLevel {
  unsigned col = 0;
  std::vector<OuterBoundFace> lowerFaces;
  std::vector<OuterBoundFace> upperFaces;
};

struct ReductionScanPlan {
  // Ordered outer reduction levels, outermost first (one per peeled axis; the
  // innermost reduction axis is tiled, not peeled). For a two-axis reduction
  // this has a single level and mirrors the outer* fields below.
  std::vector<ReductionPeelLevel> peelLevels;
  unsigned reductionRank = 0; // total reduction axes r (peeled r-1 + 1 tiled)
  unsigned outerCol = 0;      // peelLevels.front().col (compat)
  unsigned innerCol = 0;      // the innermost, device-tiled axis
  std::int64_t outerLo = 0; // constant envelope of the outermost axis (superset)
  std::int64_t outerHi = 0;
  // Exact tight bounds (M2): piecewise-affine in the output. When
  // `outerRangeIsConstant`, the faces are constants equal to [outerLo, outerHi].
  bool outerRangeIsConstant = true;
  std::vector<OuterBoundFace> outerLowerFaces;
  std::vector<OuterBoundFace> outerUpperFaces;
  std::int64_t innerStride = 1;
  std::vector<std::int64_t> innerConstantTileLengths;
  bool empty = false;
  bool outerIsAccumulation = true;
  bool orderMatchesSourceForStrictFP = false;
  LogicalAccessMap realOutputAccess;
  ReductionOrderDecision reductionOrder =
      ReductionOrderDecision::PreserveSourceOrder;
};

/// Build the peel-and-tile plan from the raw recognizer facts. Returns nullopt
/// (UNKNOWN) on any inexactness. M1 scope: exactly two reduction axes, convex
/// fiber, constant outer range.
inline std::optional<ReductionScanPlan> buildReductionScanPlan(
    const std::array<IntMatrix, 3> &accessMatrices,
    const std::array<IntRow, 3> &offsets,
    const mlir::presburger::IntegerPolyhedron &sourceDomain,
    std::size_t sourceRank,
    ReductionOrderDecision reductionOrder,
    std::string &why) {

  using namespace mlir::presburger;
  using symbolic_tile_detail::rankOf;

  // 1. Real contraction: real frame, real access (incl. the real C), real M.
  auto contraction = buildSymbolicContraction(
      accessMatrices, offsets, sourceDomain, sourceRank, reductionOrder, why);
  if (!contraction)
    return std::nullopt;
  const TileCoordinateFrame frame = contraction->frame;
  const unsigned n = frame.logicalRank();
  const IntegerPolyhedron &D = contraction->domain;

  // 2. Classify the reduction axes via requiresReduction.
  const auto partition = analyzeRankPartition(
      accessMatrices[0], accessMatrices[1], accessMatrices[2], sourceRank);
  if (!partition) { why = "rank partition analysis failed"; return std::nullopt; }
  const auto expanded = expandRankPartition(*partition);
  if (!expanded) { why = "rank partition expansion failed"; return std::nullopt; }
  const auto coordinateMap = buildPartitionCoordinateMap(*expanded);
  if (!coordinateMap) { why = "coordinate map construction failed"; return std::nullopt; }

  std::vector<unsigned> reductionCols;
  if (frame.kDims == 1)
    reductionCols.push_back(frame.kBegin());
  else if (frame.kDims > 1) {
    why = "unexpected multi-row active reduction axis";
    return std::nullopt;
  }
  const unsigned partitionBase = frame.batchBegin();
  for (unsigned p = 0; p < expanded->partitions.size(); ++p)
    if (expanded->partitions[p].requiresReduction)
      reductionCols.push_back(partitionBase + p);
  if (reductionCols.size() < 2) {
    why = "UNKNOWN: fewer than two reduction axes (single-axis path handles this)";
    return std::nullopt;
  }
  const unsigned r = static_cast<unsigned>(reductionCols.size());
  const unsigned outerCol = reductionCols[0];     // outermost peeled axis
  const unsigned innerCol = reductionCols[r - 1]; // innermost, device-tiled axis
  for (unsigned c : reductionCols)
    if (c >= n) {
      why = "reduction column out of range";
      return std::nullopt;
    }
  // The exact nested projection below requires a congruence-free (unit-stride)
  // lattice; a congruence-restricted reduction axis is refused conservatively.
  if (contraction->congruenceLocals != 0) {
    why = "UNKNOWN: congruence-restricted reduction axes";
    return std::nullopt;
  }

  // 3. Real M = d*T^{-1} for the accumulation obligations.
  AffineIntegerDomain unconstrained;
  unconstrained.sourceDimension = n;
  const auto symbolic = transformToSymbolicDomain(*coordinateMap, unconstrained);
  if (!symbolic || symbolic->dimension != n || symbolic->denominator <= 0 ||
      symbolic->inverseNumerator.size() != n) {
    why = "symbolic transform of the coordinate map failed";
    return std::nullopt;
  }
  const IntMatrix &M = symbolic->inverseNumerator;

  // 4. Real output must inject on the real outputs (M, N, genuine batch -- i.e.
  //    everything except the two reduction axes) and be constant along BOTH
  //    reduction axes, so accumulating over (k_outer, k_inner) into one C
  //    element is valid and distinct physical outputs never alias.
  const unsigned outputDims = n - r;
  if (rankOf(accessMatrices[2], n) != outputDims) {
    why = "UNKNOWN: real output aliases distinct logical outputs";
    return std::nullopt;
  }
  for (const IntRow &crow : accessMatrices[2]) {
    if (crow.size() != n) { why = "output access row arity"; return std::nullopt; }
    for (unsigned c : reductionCols) {
      BigInt s = 0;
      for (unsigned j = 0; j < n; ++j)
        s += crow[j] * M[j][c];
      if (s != 0) {
        why = "UNKNOWN: real output varies along a reduction axis";
        return std::nullopt;
      }
    }
  }

  ReductionScanPlan out;
  out.reductionRank = r;
  out.outerCol = outerCol;
  out.innerCol = innerCol;
  out.outerIsAccumulation = true;
  out.orderMatchesSourceForStrictFP = false;
  out.reductionOrder = reductionOrder;
  out.realOutputAccess = contraction->access[2];

  // 5. Empty domain: no MACs, no loop.
  if (D.isIntegerEmpty()) {
    out.empty = true;
    return out;
  }

  // 6. Tile the inner reduction via the EXISTING partitioner. Append the outer
  //    axis's source covector to C, turning the problem into a synthetic
  //    batched GEMM (batch = k_outer, single reduction = k_inner). The inner
  //    tiling, its constant-length groups, its disjointness, and its stride are
  //    all discharged by buildSymbolicTilePartition / C3b.
  if (coordinateMap->sourceToLogical.size() != n) {
    why = "coordinate map arity";
    return std::nullopt;
  }
  std::array<IntMatrix, 3> synthF = accessMatrices;
  std::array<IntRow, 3> synthOff = offsets;
  for (unsigned q = 0; q + 1 < r; ++q) { // append every outer reduction axis
    synthF[2].push_back(coordinateMap->sourceToLogical[reductionCols[q]]);
    synthOff[2].push_back(BigInt(0));
  }
  std::string innerWhy;
  auto inner = buildSymbolicTilePartition(synthF, synthOff, sourceDomain,
                                          sourceRank, reductionOrder, innerWhy);
  if (!inner) {
    why = "UNKNOWN: inner reduction not tileable (" + innerWhy + ")";
    return std::nullopt;
  }
  if (!inner->hasReduction) {
    why = "UNKNOWN: synthetic inner contraction lost its reduction axis";
    return std::nullopt;
  }
  out.innerStride = inner->stride;
  for (const TileFiberGroup &grp : inner->groups)
    out.innerConstantTileLengths.push_back(grp.length);

  // 7-8. (M2/M3) Exact nested outer bounds. For each peeled outer axis k_p,
  //      project the strictly-inner reduction axes {k_{p+1}..k_{r-1}} out of the
  //      domain EXACTLY -- Fourier-Motzkin elimination of unit-coefficient
  //      variables is exact over the integers -- and read k_p's bounding faces,
  //      which are affine in the output and the more-significant outer axes.
  //      The innermost axis is tiled above, not peeled.
  auto rd = [](const llvm::DynamicAPInt &v) -> std::optional<std::int64_t> {
    if (v < llvm::DynamicAPInt(std::numeric_limits<std::int64_t>::min()) ||
        v > llvm::DynamicAPInt(std::numeric_limits<std::int64_t>::max()))
      return std::nullopt;
    return static_cast<std::int64_t>(v);
  };
  auto extractAxisFaces =
      [&](unsigned axis, const std::vector<unsigned> &elim,
          std::vector<OuterBoundFace> &lower,
          std::vector<OuterBoundFace> &upper) -> bool {
    // Eliminated axes must be unit-coefficient and not equality-pinned in D.
    for (unsigned ri = 0; ri < D.getNumInequalities(); ++ri)
      for (unsigned ec : elim) {
        auto c = rd(D.getInequality(ri)[ec]);
        if (!c) { why = "UNKNOWN: inner coefficient overflows i64"; return false; }
        if (*c < -1 || *c > 1) {
          why = "UNKNOWN: non-unit inner reduction coefficient (projection not exact)";
          return false;
        }
      }
    for (unsigned re = 0; re < D.getNumEqualities(); ++re)
      for (unsigned ec : elim)
        if (D.getEquality(re)[ec] != llvm::DynamicAPInt(0)) {
          why = "UNKNOWN: inner reduction axis is equality-constrained";
          return false;
        }
    IntegerPolyhedron P = D;
    // elim is ascending (strictly-inner cols); project in descending order so
    // not-yet-projected columns keep their indices.
    for (int qi = static_cast<int>(elim.size()) - 1; qi >= 0; --qi)
      P.projectOut(elim[static_cast<unsigned>(qi)], 1);
    if (P.getNumLocalVars() != 0) {
      why = "UNKNOWN: outer-bound projection is not affine (introduced locals)";
      return false;
    }
    std::vector<unsigned> surviving;
    {
      std::vector<bool> drop(n, false);
      for (unsigned ec : elim)
        drop[ec] = true;
      for (unsigned c = 0; c < n; ++c)
        if (!drop[c])
          surviving.push_back(c);
    }
    const unsigned pdims = static_cast<unsigned>(surviving.size());
    if (P.getNumDimVars() != pdims || P.getNumSymbolVars() != 0) {
      why = "UNKNOWN: unexpected projected shape";
      return false;
    }
    unsigned axisPos = pdims;
    for (unsigned d = 0; d < pdims; ++d)
      if (surviving[d] == axis)
        axisPos = d;
    if (axisPos == pdims) { why = "UNKNOWN: projected axis missing"; return false; }
    for (unsigned re = 0; re < P.getNumEqualities(); ++re)
      if (P.getEquality(re)[axisPos] != llvm::DynamicAPInt(0)) {
        why = "UNKNOWN: projected outer axis is equality-constrained";
        return false;
      }
    for (unsigned ri = 0; ri < P.getNumInequalities(); ++ri) {
      auto row = P.getInequality(ri);
      auto ao = rd(row[axisPos]);
      if (!ao) { why = "UNKNOWN: outer coefficient overflows i64"; return false; }
      if (*ao == 0)
        continue; // not a bound on this axis
      if (*ao < -1 || *ao > 1) {
        why = "UNKNOWN: non-unit outer reduction coefficient";
        return false;
      }
      OuterBoundFace f;
      f.coeffs.assign(n, BigInt(0));
      for (unsigned d = 0; d < pdims; ++d) {
        if (d == axisPos)
          continue;
        auto cp = rd(row[d]);
        if (!cp) { why = "UNKNOWN: outer-face coefficient overflows i64"; return false; }
        f.coeffs[surviving[d]] = (*ao == 1) ? -BigInt(*cp) : BigInt(*cp);
      }
      auto cc = rd(row[pdims]);
      if (!cc) { why = "UNKNOWN: outer-face constant overflows i64"; return false; }
      f.constant = (*ao == 1) ? -BigInt(*cc) : BigInt(*cc);
      ((*ao == 1) ? lower : upper).push_back(std::move(f));
    }
    if (lower.empty() || upper.empty()) {
      why = "UNKNOWN: projected outer axis is unbounded on a side";
      return false;
    }
    return true;
  };

  for (unsigned p = 0; p + 1 < r; ++p) {
    std::vector<unsigned> elim;
    for (unsigned q = p + 1; q < r; ++q)
      elim.push_back(reductionCols[q]);
    ReductionPeelLevel level;
    level.col = reductionCols[p];
    if (!extractAxisFaces(reductionCols[p], elim, level.lowerFaces,
                          level.upperFaces))
      return std::nullopt;
    out.peelLevels.push_back(std::move(level));
  }

  // Compatibility mirror (outermost level) and the constant envelope.
  out.outerLowerFaces = out.peelLevels.front().lowerFaces;
  out.outerUpperFaces = out.peelLevels.front().upperFaces;
  {
    auto oLo = D.getConstantBound64(BoundType::LB, outerCol);
    auto oHi = D.getConstantBound64(BoundType::UB, outerCol);
    if (!oLo || !oHi) {
      why = "UNKNOWN: peeled outer reduction axis is unbounded";
      return std::nullopt;
    }
    out.outerLo = *oLo;
    out.outerHi = *oHi;
  }
  bool constant = true;
  for (const auto &lvl : out.peelLevels) {
    for (const auto &f : lvl.lowerFaces)
      for (const BigInt &c : f.coeffs)
        if (c != 0) constant = false;
    for (const auto &f : lvl.upperFaces)
      for (const BigInt &c : f.coeffs)
        if (c != 0) constant = false;
  }
  out.outerRangeIsConstant = constant;

  why = "reduction scan plan built";
  return out;
}

} // namespace systolic::logical

#endif // SYSTOLIC_RANK_PARTITION_REDUCTION_SCAN_H
