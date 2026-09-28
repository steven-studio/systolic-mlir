#ifndef SYSTOLIC_DECOMPOSITION_DP_H
#define SYSTOLIC_DECOMPOSITION_DP_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"

#include <cstdint>

namespace mlir {
namespace systolic {

/// A geometry multiset represented by counts.
///
/// counts[i] is the number of logical tiles using
/// geometries[i].
///
/// Example:
///
///   geometries = {4, 8}
///   counts      = {2, 1}
///
/// represents:
///
///   2 x 4x4
///   1 x 8x8
struct SystolicGeometryMultiset {
  llvm::SmallVector<int64_t> counts;
};

/// Enumerate all unique geometry multisets that exactly cover an
/// R x C rectangular output region.
///
/// Current assumptions:
///
///   * every systolic geometry is square,
///   * every geometry size is a power of two,
///   * decomposition uses axis-aligned rectangular cuts,
///   * only the geometry multiset is retained; spatial placement
///     is intentionally discarded.
///
/// The returned multiset count vector follows the order of
/// `geometries`.
llvm::SmallVector<SystolicGeometryMultiset>
enumerateSystolicGeometryMultisetsDP(
    int64_t rows,
    int64_t columns,
    llvm::ArrayRef<int64_t> geometries);


/// Enumerate geometry multisets using an exact frontier-state DP.
///
/// Unlike enumerateSystolicGeometryMultisetsDP(), this algorithm
/// does not enumerate rectangular cut histories.  It scans the
/// normalized output grid through a canonical skyline frontier and
/// memoizes states of the form:
///
///   (column heights, remaining geometry counts)
///
/// Every transition places one square tile at the lowest-leftmost
/// frontier position.  This preserves a column-prefix occupancy
/// invariant and provides an exact tiling decision procedure.
///
/// This API is initially kept separate from the existing
/// rectangular-cut DP so that the two implementations can be
/// cross-checked before changing the production optimizer.
llvm::SmallVector<SystolicGeometryMultiset>
enumerateSystolicGeometryMultisetsFrontierDP(
    int64_t rows,
    int64_t columns,
    llvm::ArrayRef<int64_t> geometries);

/// Enumerate geometry multisets using an exact two-stage algorithm.
///
/// Stage 1 enumerates CountVectors satisfying the necessary area
/// equation:
///
///   sum_i counts[i] * geometries[i]^2 = rows * columns
///
/// Stage 2 performs an exact skyline tileability search for each
/// area-feasible CountVector.
///
/// No heuristic pruning is used: a CountVector is returned iff
/// there exists an exact square tiling using exactly those counts.
llvm::SmallVector<SystolicGeometryMultiset>
enumerateSystolicGeometryMultisetsAreaFirstDP(
    int64_t rows,
    int64_t columns,
    llvm::ArrayRef<int64_t> geometries);

} // namespace systolic
} // namespace mlir

#endif // SYSTOLIC_DECOMPOSITION_DP_H
