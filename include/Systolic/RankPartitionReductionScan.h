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
struct ReductionScanPlan {
  unsigned outerCol = 0;
  unsigned innerCol = 0;
  std::int64_t outerLo = 0;
  std::int64_t outerHi = 0;
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
  if (reductionCols.size() != 2) {
    why = "UNKNOWN: M1 supports exactly two reduction axes";
    return std::nullopt;
  }
  const unsigned outerCol = reductionCols[0];
  const unsigned innerCol = reductionCols[1];
  const unsigned r = 2;
  if (outerCol >= n || innerCol >= n) {
    why = "reduction column out of range";
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
    for (unsigned c : {outerCol, innerCol}) {
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
  synthF[2].push_back(coordinateMap->sourceToLogical[outerCol]);
  synthOff[2].push_back(BigInt(0));
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

  // 7. Outer axis constant integer range.
  auto oLo = D.getConstantBound64(BoundType::LB, outerCol);
  auto oHi = D.getConstantBound64(BoundType::UB, outerCol);
  if (!oLo || !oHi) {
    why = "UNKNOWN: peeled outer reduction axis is unbounded";
    return std::nullopt;
  }
  if (*oHi < *oLo) {
    why = "UNKNOWN: degenerate outer reduction range";
    return std::nullopt;
  }
  out.outerLo = *oLo;
  out.outerHi = *oHi;

  why = "reduction scan plan built";
  return out;
}

} // namespace systolic::logical

#endif // SYSTOLIC_RANK_PARTITION_REDUCTION_SCAN_H
