#include "Systolic/AffineMatmulPartitioning.h"

using namespace mlir;
using namespace mlir::systolic;

namespace {

/// Logical partitioning policy.
///
/// This layer deliberately does not know anything about systolic-array
/// geometry, device capacity, or hardware tile sizes.
///
/// A future policy may partition the logical iteration space using:
///
///   - affine loop structure,
///   - dependence regions,
///   - logical problem dimensions,
///   - compiler-selected partition boundaries.
///
/// Hardware-specific choices such as 4x4 or 8x8 belong to the
/// Systolic Tiling stage.
struct LogicalPartitionPolicy {
  FailureOr<SmallVector<LogicalPartition, 1>>
  apply(const LogicalMatmul &matmul) const {
    if (!matmul.sourceDomain)
      return failure();

    SmallVector<LogicalPartition, 1> partitions;

    // Initial policy: preserve the complete logical execution region.
    //
    // This is intentionally not hardware tiling.  The partition boundary
    // policy can be replaced independently once the intended affine
    // partition semantics are fixed.
    partitions.push_back(LogicalPartition{*matmul.sourceDomain});

    return partitions;
  }
};

} // namespace

FailureOr<SmallVector<LogicalPartition, 1>>
mlir::systolic::partitionLogicalMatmul(const LogicalMatmul &matmul) {
  LogicalPartitionPolicy policy;
  return policy.apply(matmul);
}
