//===- RankPartitionSymbolicContraction.h - Option C C2 ----------*- C++ -*-===//
//
// Phase C2 (Option C): recover a symbolic contraction representation and
// tile-compatible logical M/N/K coordinates from an already-recognized
// general affine MAC, WITHOUT enumerating source iteration points.
//
// Inputs are the pure-math facts the recognizer already produced: the three
// operand/output access maps over the source iteration variables (A, B, C as
// integer rows plus integer offsets), the exact integer source iteration
// domain, and its dimension. buildSymbolicContraction() returns:
//
//   * the logical M/N/K coordinate frame (which logical dims are output rows,
//     output columns, reduction, and residual batch axes), from the exact
//     rank partition;
//   * the exact N-local logical image D of the source domain under z = T*x,
//     reused verbatim from buildExactLogicalImage (validated in C1); and
//   * the operand/output access maps expressed over the logical image
//     variables [z, q], where q are the congruence locals -- integer-affine
//     and exact: evaluated at any image point they equal the source access at
//     the recovered source iteration x = T^{-1} z.
//
// No source-point or fiber enumeration. No approximation. Any step that cannot
// be shown exact and i64-representable returns nullopt (UNKNOWN); the caller
// refuses. This is the input the C3 symbolic tile partitioner will consume; it
// emits no IR and does not itself partition.
//
// buildRecoverableLogicalDomain (the guarded-MAC one-local path) is NOT used
// or modified here. buildExactLogicalImage is the N-local reconstruction.
//
#ifndef SYSTOLIC_RANK_PARTITION_SYMBOLIC_CONTRACTION_H
#define SYSTOLIC_RANK_PARTITION_SYMBOLIC_CONTRACTION_H

#include "Systolic/RankPartitionDomain.h"
#include "Systolic/RankPartitionExpansion.h"
#include "Systolic/RankPartitionGuardedMemoryEmission.h"
#include "Systolic/RankPartitionReductionSemantics.h"
#include "Systolic/RankPartitionSymbolicDomain.h"
#include "Systolic/TensorLogicalSpaces.h"

#include "mlir/Analysis/Presburger/IntegerRelation.h"

#include <array>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <vector>

namespace systolic::logical {

/// Which logical z-dims carry each GEMM role. The logical coordinate order is
/// [i.. , j.. , k.. , batch..] (collectCoordinateRows): the output rows (M)
/// come first, then output columns (N), then reduction (K), then any residual
/// batch/original axes. A role of extent zero is a virtual size-one axis
/// (no source dimension is invented).
struct TileCoordinateFrame {
  unsigned mDims = 0;
  unsigned nDims = 0;
  unsigned kDims = 0;
  unsigned batchDims = 0;
  bool virtualM = false;
  bool virtualN = false;
  bool virtualK = false;

  unsigned mBegin() const { return 0; }
  unsigned nBegin() const { return mDims; }
  unsigned kBegin() const { return mDims + nDims; }
  unsigned batchBegin() const { return mDims + nDims + kDims; }
  unsigned logicalRank() const { return mDims + nDims + kDims + batchDims; }
};

/// One affine tensor access over the logical image variables [z, q]: `rows`
/// has one row per tensor index, each with logicalRank + congruenceLocals
/// integer coefficients (z first, then the congruence locals q), plus an
/// integer offset. Every coefficient and offset fits in i64.
struct LogicalAccessMap {
  IntMatrix rows;    // tensorRank x (logicalRank + congruenceLocals)
  IntRow offsets;    // tensorRank
};

/// The enumeration-free symbolic contraction: the logical frame, the exact
/// N-local image domain over [z, q], the A/B/C access maps over [z, q], the
/// number of congruence locals, and the reduction-order decision. The domain
/// together with the three access maps is the multiplicity witness: every
/// physical MAC-address triple is determined exactly from them (the exactly-
/// once certificate is discharged later, in C5). C2 adds no partition.
struct SymbolicContraction {
  TileCoordinateFrame frame;
  mlir::presburger::IntegerPolyhedron domain; // over [z(logicalRank), q(locals), 1]
  unsigned congruenceLocals = 0;
  std::array<LogicalAccessMap, 3> access;     // A, B, C over [z, q]
  ReductionOrderDecision reductionOrder = ReductionOrderDecision::PreserveSourceOrder;
};

namespace symbolic_contraction_detail {

inline std::optional<std::int64_t> toI64(const BigInt &value) {
  if (value < BigInt(std::numeric_limits<std::int64_t>::min()) ||
      value > BigInt(std::numeric_limits<std::int64_t>::max()))
    return std::nullopt;
  return value.convert_to<std::int64_t>();
}

} // namespace symbolic_contraction_detail

/// Recover the symbolic contraction. `accessMatrices` and `offsets` are the
/// source-coordinate access rows and offsets for A, B, C; `sourceDomain` is
/// the exact integer source iteration domain over the source IVs (no
/// existential locals in C2); `sourceRank` is the number of source IVs.
/// `reductionOrder` is carried through from the recognizer. Returns nullopt
/// (UNKNOWN) on any representability, integrality, overflow, or consistency
/// failure. No enumeration.
inline std::optional<SymbolicContraction> buildSymbolicContraction(
    const std::array<IntMatrix, 3> &accessMatrices,
    const std::array<IntRow, 3> &offsets,
    const mlir::presburger::IntegerPolyhedron &sourceDomain,
    std::size_t sourceRank,
    ReductionOrderDecision reductionOrder,
    std::string &why) {

  using symbolic_contraction_detail::toI64;

  if (sourceRank == 0 ||
      sourceRank > std::numeric_limits<unsigned>::max() - 1) {
    why = "degenerate source rank";
    return std::nullopt;
  }
  const unsigned n = static_cast<unsigned>(sourceRank);

  // C2 recovers a convex source domain with no pre-existing existential
  // locals; a domain that already carries locals is refused here.
  if (sourceDomain.getNumDimVars() != n ||
      sourceDomain.getNumSymbolVars() != 0 ||
      sourceDomain.getNumLocalVars() != 0) {
    why = "source domain shape is not a local-free rank-n polyhedron";
    return std::nullopt;
  }

  // 1. Exact rank partition -> logical M/N/K frame.
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

  TileCoordinateFrame frame;
  frame.mDims = static_cast<unsigned>(expanded->i.size());
  frame.nDims = static_cast<unsigned>(expanded->j.size());
  frame.kDims = static_cast<unsigned>(expanded->k.size());
  frame.batchDims = static_cast<unsigned>(expanded->partitions.size());
  frame.virtualM = expanded->virtualI;
  frame.virtualN = expanded->virtualJ;
  frame.virtualK = expanded->virtualK;
  if (frame.logicalRank() != n) {
    why = "logical frame rank does not cover the source rank";
    return std::nullopt;
  }

  const auto coordinateMap = buildPartitionCoordinateMap(*expanded);
  if (!coordinateMap) {
    why = "coordinate map construction failed";
    return std::nullopt;
  }

  // 2. Exact N-local logical image (C1-validated). Reused verbatim.
  auto image = buildExactLogicalImage(*coordinateMap, sourceDomain);
  if (!image) {
    why = "exact logical image unrepresentable or not integral";
    return std::nullopt;
  }
  const unsigned added = image->getNumLocalVars();

  // 3. Recovery x = T^{-1} z, integral on the image via the congruence locals:
  //    M = d * T^{-1}. For a row i of M all divisible by d (automatic),
  //    x_i = (M[i]/d) . z. Otherwise x_i is a congruence local q_l. Recover M,
  //    d and the congruence rows exactly as buildExactLogicalImage does, so
  //    the local indexing agrees with `image`.
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
  std::vector<int> localOf(n, -1);
  for (unsigned l = 0; l < added; ++l)
    localOf[congruenceRows[l]] = static_cast<int>(l);

  // 4. Compose each operand/output access over [z(n), q(added)]:
  //    f(x) = F . x + b  with  x = R_z z + R_q q  gives
  //    f = (F R_z) z + (F R_q) q + b, integer and i64-checked.
  // IntegerPolyhedron has no default constructor, so SymbolicContraction is
  // not default-constructible: assemble the members as locals and aggregate-
  // initialize the result at the return.
  std::array<LogicalAccessMap, 3> accessMaps;
  for (unsigned op = 0; op < 3; ++op) {
    const IntMatrix &F = accessMatrices[op];
    LogicalAccessMap lam;
    lam.rows.resize(F.size());
    lam.offsets.resize(F.size());
    for (std::size_t r = 0; r < F.size(); ++r) {
      if (F[r].size() != n) {
        why = "access row arity";
        return std::nullopt;
      }
      IntRow row(n + added, BigInt(0));
      // z columns: only automatic rows contribute, via M[i][j]/d.
      for (unsigned j = 0; j < n; ++j) {
        BigInt acc = 0;
        for (unsigned i = 0; i < n; ++i) {
          if (localOf[i] != -1)
            continue; // non-automatic: handled by a q column
          acc += F[r][i] * (M[i][j] / d);
        }
        if (!toI64(acc)) {
          why = "logical access coefficient overflows i64";
          return std::nullopt;
        }
        row[j] = acc;
      }
      // q columns: coefficient of local l is F[r][congruenceRows[l]].
      for (unsigned l = 0; l < added; ++l) {
        const BigInt &coeff = F[r][congruenceRows[l]];
        if (!toI64(coeff)) {
          why = "logical access local coefficient overflows i64";
          return std::nullopt;
        }
        row[n + l] = coeff;
      }
      if (r >= offsets[op].size()) {
        why = "access offset arity";
        return std::nullopt;
      }
      if (!toI64(offsets[op][r])) {
        why = "logical access offset overflows i64";
        return std::nullopt;
      }
      lam.rows[r] = std::move(row);
      lam.offsets[r] = offsets[op][r];
    }
    accessMaps[op] = std::move(lam);
  }

  why = "symbolic contraction recovered";
  return SymbolicContraction{std::move(frame), std::move(*image), added,
                             std::move(accessMaps), reductionOrder};
}

} // namespace systolic::logical

#endif // SYSTOLIC_RANK_PARTITION_SYMBOLIC_CONTRACTION_H
