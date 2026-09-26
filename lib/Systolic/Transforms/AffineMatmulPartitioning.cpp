#include "Systolic/AffineMatmulPartitioning.h"

using namespace mlir;
using namespace mlir::systolic;

FailureOr<SmallVector<LogicalPartition, 1>>
mlir::systolic::partitionLogicalMatmul(const LogicalMatmul &matmul) {
  if (!matmul.sourceDomain)
    return failure();

  SmallVector<LogicalPartition, 1> partitions;
  partitions.push_back(LogicalPartition{*matmul.sourceDomain});
  return partitions;
}
