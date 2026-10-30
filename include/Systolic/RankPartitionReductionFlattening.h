//===- RankPartitionReductionFlattening.h - Option C C3c-2 -------*- C++ -*-===//
//
// Phase C3c-2 (Option C): exact flattening of a genuine multi-dimensional
// reduction space into a single logical reduction ordinal, for the provably
// rectangular subset. This is an analysis helper only: it emits no IR and does
// not itself tile (that is C4). Its output is a certificate C4 can consume.
//
// A genuine multi-axis reduction K = (k_0, ..., k_{r-1}) (the active reduction
// axis plus every requiresReduction partition the recognizer kept, see
// RankPartitionExpansion.h) over a RECTANGULAR domain is flattened by a
// mixed-radix ordinal, with k_0 the most significant axis:
//
//   t = sum_p (k_p - lo_p) * R_p,     R_p = prod_{q>p} K_q,     0 <= t < prod K_q
//   k_p = lo_p + (t / R_p) mod K_p                              (exact inverse)
//
// This is accepted only when every obligation is discharged symbolically:
//   * each reduction axis is decoupled (appears only in its own per-axis bound
//     inequalities, never coupled with another variable), so the reduction
//     sub-domain is exactly the product box, not merely contained in it;
//   * no congruence (lattice) equality constrains any reduction axis, so the
//     axes are full integer intervals (no stepped/sublattice gaps);
//   * every extent K_p is strictly positive and i64-representable, and the
//     product and the radix weights do not overflow i64;
//   * the output access is injective on every output coordinate and constant
//     along every reduction axis (no two logical outputs alias one physical C
//     element) -- the C3b aliasing obligation, generalized.
// Otherwise buildFlattenedReduction returns nullopt (UNKNOWN) with a diagnostic.
//
// The mixed-radix map is a bijection between [0, prod K_q) and the product box
// by construction, so it preserves the exact MAC multiset. It does NOT by
// itself establish the physical summation order; order preservation under
// strict floating point is a separate obligation the caller must discharge
// (orderMatchesSourceForStrictFP is left false -- the source loop nesting is
// not recoverable from the access maps alone). Integer reductions are
// order-independent.
//
// Device addressability is classified per operand but not required here: an
// operand is "affine-strided in t" only when its reduction dependence lives in
// a single index component proportional to the radix weights; a genuine
// multi-index tensor (A[i][k1][k2]) is NOT -- its flattened address needs
// floor/mod, i.e. a memref collapse (contiguous layout) or packing, which is a
// C4 concern. No claim is made that an arbitrary layout becomes affine-strided.
//
#ifndef SYSTOLIC_RANK_PARTITION_REDUCTION_FLATTENING_H
#define SYSTOLIC_RANK_PARTITION_REDUCTION_FLATTENING_H

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

/// Certified flattening of a rectangular multi-dimensional reduction. The axes
/// are listed most-significant first; `extents[p]`, `lowerBounds[p]`, and
/// `radixWeights[p] = prod_{q>p} extents[q]` describe the mixed-radix ordinal
/// t in [0, length). `operandAffineStridedInT[op]` is true when operand op's
/// flattened reduction address is a single affine stride in t (Case A); false
/// means it needs floor/mod, i.e. a memref collapse or packing at emission time
/// (Case B). `empty` is true for an empty reduction domain (length 0, no MACs).
struct FlattenedReduction {
  std::vector<unsigned> reductionCols;   // logical z-columns, most significant first
  std::vector<std::int64_t> extents;     // K_p >= 1
  std::vector<std::int64_t> lowerBounds; // lo_p
  std::vector<BigInt> radixWeights;      // R_p
  BigInt length = 0;                     // prod K_p
  unsigned reductionRank = 0;            // r
  bool empty = false;
  bool orderMatchesSourceForStrictFP = false;
  std::array<bool, 2> operandAffineStridedInT = {false, false}; // A, B
  ReductionOrderDecision reductionOrder =
      ReductionOrderDecision::PreserveSourceOrder;
};

/// Build the flattening certificate from the SAME raw recognizer facts the
/// other Option C entry points consume. Returns nullopt (UNKNOWN) on any
/// inexactness. Intended for a genuine multi-axis reduction (reductionRank >= 2);
/// a single-axis reduction is handled by buildSymbolicTilePartition instead.
inline std::optional<FlattenedReduction> buildFlattenedReduction(
    const std::array<IntMatrix, 3> &accessMatrices,
    const std::array<IntRow, 3> &offsets,
    const mlir::presburger::IntegerPolyhedron &sourceDomain,
    std::size_t sourceRank,
    ReductionOrderDecision reductionOrder,
    std::string &why) {

  using namespace mlir::presburger;
  using symbolic_tile_detail::rankOf;
  using symbolic_tile_detail::toI64;

  // 1. C2 recovery (frame, exact image D over [z,u], access maps).
  auto contraction = buildSymbolicContraction(
      accessMatrices, offsets, sourceDomain, sourceRank, reductionOrder, why);
  if (!contraction)
    return std::nullopt;
  const TileCoordinateFrame frame = contraction->frame;
  const unsigned n = frame.logicalRank();
  const unsigned added = contraction->congruenceLocals;
  const IntegerPolyhedron &D = contraction->domain;
  if (D.getNumDimVars() != n || D.getNumLocalVars() != added ||
      D.getNumCols() != n + added + 1) {
    why = "C2 image shape unexpected";
    return std::nullopt;
  }

  // 2. Recompute the rank partition so requiresReduction is visible, plus
  //    M = d*T^{-1} for the reduction directions.
  const auto partition = analyzeRankPartition(
      accessMatrices[0], accessMatrices[1], accessMatrices[2], sourceRank);
  if (!partition) { why = "rank partition analysis failed"; return std::nullopt; }
  const auto expanded = expandRankPartition(*partition);
  if (!expanded) { why = "rank partition expansion failed"; return std::nullopt; }
  const auto coordinateMap = buildPartitionCoordinateMap(*expanded);
  if (!coordinateMap) { why = "coordinate map construction failed"; return std::nullopt; }
  AffineIntegerDomain unconstrained;
  unconstrained.sourceDimension = n;
  const auto symbolic = transformToSymbolicDomain(*coordinateMap, unconstrained);
  if (!symbolic || symbolic->dimension != n || symbolic->denominator <= 0 ||
      symbolic->inverseNumerator.size() != n) {
    why = "symbolic transform of the coordinate map failed";
    return std::nullopt;
  }
  const IntMatrix &M = symbolic->inverseNumerator;

  // 3. Ordered reduction columns: the active axis (k.front()) first, then each
  //    requiresReduction partition in order (most significant first).
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
  const unsigned r = static_cast<unsigned>(reductionCols.size());
  if (r < 2) {
    why = "not a multi-dimensional reduction (single-axis path handles this)";
    return std::nullopt;
  }
  for (unsigned c : reductionCols)
    if (c >= n) { why = "reduction column out of range"; return std::nullopt; }

  std::vector<bool> isReductionCol(n, false);
  for (unsigned c : reductionCols)
    isReductionCol[c] = true;

  const unsigned outputDims = n - r;

  // 4. Output aliasing / constancy (generalized C3b obligation). The output
  //    access must be injective on all output coordinates and constant along
  //    every reduction axis, so distinct logical outputs never alias one C
  //    element and the reductions are independent.
  if (rankOf(accessMatrices[2], n) != outputDims) {
    why = "UNKNOWN: output access aliases distinct logical outputs "
          "(reductions not independent)";
    return std::nullopt;
  }
  for (const IntRow &crow : accessMatrices[2]) {
    if (crow.size() != n) { why = "output access row arity"; return std::nullopt; }
    for (unsigned c : reductionCols) {
      BigInt s = 0;
      for (unsigned j = 0; j < n; ++j)
        s += crow[j] * M[j][c];
      if (s != 0) {
        why = "UNKNOWN: output varies along a reduction axis (not a reduction)";
        return std::nullopt;
      }
    }
  }

  FlattenedReduction out;
  out.reductionCols = reductionCols;
  out.reductionRank = r;
  out.reductionOrder = reductionOrder;
  out.orderMatchesSourceForStrictFP = false; // not provable from access maps

  // Empty reduction domain: no MAC operations.
  if (D.isIntegerEmpty()) {
    out.empty = true;
    out.length = 0;
    out.extents.assign(r, 0);
    out.lowerBounds.assign(r, 0);
    out.radixWeights.assign(r, BigInt(0));
    return out;
  }

  // 5. Rectangularity: every reduction axis must be DECOUPLED. An inequality
  //    that touches a reduction axis may touch only that one axis (a pure
  //    per-axis bound); no equality may touch any reduction axis (so the
  //    lattice leaves the reduction axes as full integer intervals).
  auto rowTouchesOnlyCol = [&](llvm::ArrayRef<llvm::DynamicAPInt> row,
                               unsigned col) -> bool {
    for (unsigned j = 0; j < n + added; ++j) {
      if (j == col)
        continue;
      if (row[j] != llvm::DynamicAPInt(0))
        return false;
    }
    return true;
  };
  for (unsigned ri = 0; ri < D.getNumInequalities(); ++ri) {
    auto row = D.getInequality(ri);
    unsigned touched = 0, which = 0;
    for (unsigned c : reductionCols)
      if (row[c] != llvm::DynamicAPInt(0)) { ++touched; which = c; }
    if (touched == 0)
      continue;
    if (touched > 1 || !rowTouchesOnlyCol(row, which)) {
      why = "UNKNOWN: reduction axis is coupled (non-rectangular reduction "
            "domain)";
      return std::nullopt;
    }
  }
  for (unsigned re = 0; re < D.getNumEqualities(); ++re) {
    auto row = D.getEquality(re);
    for (unsigned c : reductionCols)
      if (row[c] != llvm::DynamicAPInt(0)) {
        why = "UNKNOWN: reduction axis is congruence/equality constrained "
              "(stepped or coupled reduction)";
        return std::nullopt;
      }
  }

  // 6. Per-axis constant extents from the decoupled bounds.
  out.extents.resize(r);
  out.lowerBounds.resize(r);
  for (unsigned p = 0; p < r; ++p) {
    auto lb = D.getConstantBound64(BoundType::LB, reductionCols[p]);
    auto ub = D.getConstantBound64(BoundType::UB, reductionCols[p]);
    if (!lb || !ub) {
      why = "UNKNOWN: reduction axis unbounded";
      return std::nullopt;
    }
    if (*ub < *lb) {
      why = "UNKNOWN: degenerate reduction extent";
      return std::nullopt;
    }
    out.lowerBounds[p] = *lb;
    // K_p = ub - lb + 1, i64-checked.
    BigInt ext = BigInt(*ub) - BigInt(*lb) + 1;
    auto e64 = toI64(ext);
    if (!e64 || *e64 <= 0) {
      why = "UNKNOWN: reduction extent overflow or non-positive";
      return std::nullopt;
    }
    out.extents[p] = *e64;
  }

  // 7. Radix weights R_p = prod_{q>p} K_q and length = prod K_q, i64-checked.
  out.radixWeights.assign(r, BigInt(1));
  for (unsigned p = 0; p + 1 < r; ++p) {
    BigInt w = 1;
    for (unsigned q = p + 1; q < r; ++q) {
      w *= BigInt(out.extents[q]);
      if (!toI64(w)) {
        why = "UNKNOWN: mixed-radix weight overflows i64";
        return std::nullopt;
      }
    }
    out.radixWeights[p] = w;
  }
  BigInt len = 1;
  for (unsigned p = 0; p < r; ++p) {
    len *= BigInt(out.extents[p]);
    if (!toI64(len)) {
      why = "UNKNOWN: flattened reduction length overflows i64";
      return std::nullopt;
    }
  }
  out.length = len;

  // 8. Device addressability per operand (A, B). The operand's flattened
  //    reduction address is a single affine stride in t iff every index
  //    component that depends on the reduction axes has reduction coefficients
  //    proportional to the radix weights. A genuine multi-index tensor is not.
  for (unsigned op = 0; op < 2; ++op) {
    const IntMatrix &F = accessMatrices[op];
    bool affine = true;
    for (const IntRow &frow : F) {
      if (frow.size() != n) { why = "operand access row arity"; return std::nullopt; }
      // reduction coefficient vector for this index component
      bool anyNonZero = false;
      for (unsigned c : reductionCols)
        if (frow[c] != 0) { anyNonZero = true; break; }
      if (!anyNonZero)
        continue;
      // require frow[reductionCols[p]] == s * R_p for a single scalar s
      // determined by the first axis with a non-zero weight.
      bool haveS = false;
      BigInt sNum = 0, sDen = 1; // s = sNum/sDen
      for (unsigned p = 0; p < r; ++p) {
        const BigInt &coeff = frow[reductionCols[p]];
        const BigInt &weight = out.radixWeights[p];
        if (!haveS) {
          sNum = coeff;
          sDen = weight; // weight >= 1
          haveS = true;
        } else {
          // coeff/weight == sNum/sDen  <=>  coeff*sDen == sNum*weight
          if (coeff * sDen != sNum * weight) { affine = false; break; }
        }
      }
      if (!affine)
        break;
    }
    out.operandAffineStridedInT[op] = affine;
  }

  why = "flattened reduction certified";
  return out;
}

} // namespace systolic::logical

#endif // SYSTOLIC_RANK_PARTITION_REDUCTION_FLATTENING_H
