#ifndef SYSTOLIC_AFFINE_MATMUL_PARTITIONING_H
#define SYSTOLIC_AFFINE_MATMUL_PARTITIONING_H

#include "Systolic/AffineMatmulAnalysis.h"

#include "mlir/Analysis/Presburger/IntegerRelation.h"
#include "mlir/Support/LLVM.h"

#include <array>
#include <optional>
#include <string>

namespace mlir {
namespace systolic {

/// Logical GEMM coordinates, in the order LogicalBox and DenseLogicalGemm
/// index them.
enum LogicalGemmDim : unsigned { kLogicalI = 0, kLogicalJ = 1, kLogicalK = 2 };

/// A rectangular region of the dense logical GEMM space: the ordinals
/// lower[d] .. lower[d] + extent[d] - 1 of logical coordinate d (I, J, K).
struct LogicalBox {
  std::array<int64_t, 3> lower{};
  std::array<int64_t, 3> extent{};

  int64_t volume() const { return extent[0] * extent[1] * extent[2]; }
};

/// A recognized GEMM whose logical coordinates are a permutation of the
/// normalized loop ordinals z (LogicalMatmul::normalization), each with a
/// positive scale:
///
///   y_d = scale[d] * z_{sourceDim[d]} + offset[d],     d in (I, J, K)
///
/// so ordinal z_{sourceDim[d]} is the logical ordinal of coordinate d, and
/// the logical space is the dense box [0, extent[0]) x [0, extent[1]) x
/// [0, extent[2]) visited exactly once by the source loops. That is what
/// lets a box of logical ordinals be executed as a GEMM tile of matrices
/// whose rows/columns are those ordinals. Loops in any order, steps, nonzero
/// origins, and access offsets and strides are all of this form; batched
/// GEMMs (a partition prefix P*) and coupled or singular mappings (I = i+j,
/// two coordinates on one loop) are not.
struct DenseLogicalGemm {
  std::array<unsigned, 3> sourceDim{};
  std::array<int64_t, 3> scale{};
  std::array<int64_t, 3> offset{};
  std::array<int64_t, 3> extent{};
};

/// The dense view of `matmul`, or failure (with the reason in `why`) when
/// its logical coordinates are not of the form above.
FailureOr<DenseLogicalGemm> getDenseLogicalGemm(const LogicalMatmul &matmul,
                                                std::string *why = nullptr);

/// One exact execution region of a logical GEMM.
///
/// The iteration domain is expressed in source/execution coordinates x.
/// Logical coordinates (P*, I, J, K) remain defined by the LogicalMatmul
/// affine mapping y = T x + c.
struct LogicalPartition {
  affine::FlatAffineValueConstraints iterationDomain;

  /// The same region as a box of logical ordinals, for a dense GEMM.
  std::optional<LogicalBox> logicalBox;
};

/// Partition an exact logical GEMM domain into logical regions.
///
/// This stage operates on logical-domain semantics only.  It does not perform
/// hardware tiling, device selection, or scheduling.
///
/// The default policy keeps the whole domain as one region.
///
/// Failure means that the logical matmul does not currently admit a supported
/// partitioning.
FailureOr<SmallVector<LogicalPartition, 1>>
partitionLogicalMatmul(const LogicalMatmul &matmul);

/// Partition a dense logical GEMM into the given boxes -- the policy is the
/// caller's (for the systolic fleet: SystolicTilePlan.h) -- and prove that
/// they are an exact partition of its iteration domain before returning
/// them: every box lies inside the domain, together they cover it, and their
/// volumes sum to its volume, so every iteration point is executed by exactly
/// one region. Each region's iteration domain is the source domain restricted
/// to its box.
///
/// Failure (with the reason in `why`) means the boxes are not an exact
/// partition, or `matmul` is not dense; nothing that fails here may be
/// materialized.
FailureOr<SmallVector<LogicalPartition, 1>>
partitionLogicalMatmul(const LogicalMatmul &matmul,
                       ArrayRef<LogicalBox> boxes, std::string *why = nullptr);

} // namespace systolic
} // namespace mlir

#endif // SYSTOLIC_AFFINE_MATMUL_PARTITIONING_H
