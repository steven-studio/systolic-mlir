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

} // namespace systolic
} // namespace mlir

#endif // SYSTOLIC_DECOMPOSITION_DP_H
