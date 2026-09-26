#ifndef SYSTOLIC_AFFINE_MATMUL_PARTITIONING_H
#define SYSTOLIC_AFFINE_MATMUL_PARTITIONING_H

#include "Systolic/AffineMatmulAnalysis.h"

#include "mlir/Analysis/Presburger/IntegerRelation.h"
#include "mlir/Support/LLVM.h"

namespace mlir {
namespace systolic {

/// One exact execution region of a logical GEMM.
///
/// The iteration domain is expressed in source/execution coordinates x.
/// Logical coordinates (P*, I, J, K) remain defined by the LogicalMatmul
/// affine mapping y = T x + c.
struct LogicalPartition {
  affine::FlatAffineValueConstraints iterationDomain;
};

/// Partition an exact logical GEMM domain into logical regions.
///
/// This stage operates on logical-domain semantics only.  It does not perform
/// hardware tiling, device selection, or scheduling.
///
/// Failure means that the logical matmul does not currently admit a supported
/// partitioning.
FailureOr<SmallVector<LogicalPartition, 1>>
partitionLogicalMatmul(const LogicalMatmul &matmul);

} // namespace systolic
} // namespace mlir

#endif // SYSTOLIC_AFFINE_MATMUL_PARTITIONING_H
