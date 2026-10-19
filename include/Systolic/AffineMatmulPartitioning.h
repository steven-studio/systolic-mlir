#ifndef SYSTOLIC_AFFINE_MATMUL_PARTITIONING_H
#define SYSTOLIC_AFFINE_MATMUL_PARTITIONING_H

#include "Systolic/AffineMatmulAnalysis.h"

#include "mlir/Analysis/Presburger/IntegerRelation.h"
#include "mlir/Support/LLVM.h"
#include "llvm/ADT/SmallVector.h"

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

/// A recognized GEMM whose logical coordinates are each driven by their own
/// set of normalized loop ordinals z (LogicalMatmul::normalization), any
/// number of loops per coordinate, at any depth of the nest:
///
///   y_d = offset[d] + scale[d] * ordinal_d,   ordinal_d = sum over the
///         loops s of coordinate d of (|T[d][s]| / scale[d]) * z'_s,
///
/// with z'_s = z_s for a loop along which y_d increases and z'_s = n_s - 1 -
/// z_s for one along which it decreases (T[d][s] < 0: a counting-down C loop
/// or a reversed subscript; offset[d] is then the smallest value),
/// where the loops of a coordinate number it densely (mixed radix: sorted by
/// scale, each scale is the previous one times the previous loop's extent),
/// so ordinal_d runs over [0, extent[d]) exactly once and the logical space
/// is the dense box [0, extent[0]) x [0, extent[1]) x [0, extent[2]). That
/// is what lets a box of logical ordinals be executed as a GEMM tile of
/// matrices whose rows/columns are those ordinals. Loops in any order,
/// steps, nonzero origins, access offsets and strides, coordinates split
/// over several loops (tiled nests) and coordinates enumerated downward are
/// all of this form; batched GEMMs (a
/// partition prefix P*) and coupled or singular mappings (I = i+j, one loop
/// driving two coordinates) are not. A loop that runs exactly once (n_s = 1)
/// is constant on the domain and constrains nothing: it is left out of every
/// coordinate that a loop of two or more iterations drives. For loops of two
/// or more iterations the form above is necessary as well as sufficient for
/// the coordinates to be a bijection onto a box (docs/AFFINE_GEMM_MAPPING.md,
/// Theorem 3).
///
/// A batched GEMM (a partition prefix P*: coordinates every operand reads,
/// docs/AFFINE_GEMM_MAPPING.md section 2) is dense in I, J, K for every value
/// of the batch coordinates; the batch loops -- the loops with a nonzero
/// coefficient in a P row, which then drive no I, J or K -- are not required
/// to enumerate anything densely: the lowering re-emits them as they are
/// around the tiles, one GEMM per batch iteration, exactly the original
/// iteration domain (ISSUE-006).
struct DenseLogicalGemm {
  /// The finest (smallest-scale) loop of each coordinate.
  std::array<unsigned, 3> sourceDim{};
  /// All loops of each coordinate, finest first.
  std::array<SmallVector<unsigned, 2>, 3> sourceDims{};
  std::array<int64_t, 3> scale{};
  std::array<int64_t, 3> offset{};
  std::array<int64_t, 3> extent{};

  /// The batch coordinates P* (0 for a plain GEMM): the source loops of two
  /// or more iterations with a nonzero coefficient in any P row (outermost
  /// first; a loop that runs once is a constant, folded into the batch index
  /// by the lowering), and per coordinate
  /// its smallest and largest value on the iteration domain (for the bounds
  /// of the operands' batch dimensions) and the product of the batch loops'
  /// trip counts (how often the tiles run).
  unsigned numBatchDims = 0;
  SmallVector<unsigned, 2> batchLoops;
  SmallVector<std::pair<int64_t, int64_t>, 2> batchRange;
  int64_t batchTrips = 1;
};

/// The dense view of `matmul`, or failure (with the reason in `why`) when
/// its logical coordinates are not of the form above.
FailureOr<DenseLogicalGemm>
getDenseLogicalGemm(const LogicalMatmul &matmul, std::string *why = nullptr,
                    GemmMappingClass *mappingClass = nullptr);

/// The smallest and largest value, on the nest's iteration domain, of
/// constant + sum over the loops s of coefficients[s] * x_s, x_s the counter
/// of loop s (the logical mapping's columns): exact on the box of the
/// normalized domain (x = S z + origin, each term at an end of its loop's
/// range). Failure when the domain has no static normalized form or the
/// arithmetic overflows 64 bits.
FailureOr<std::pair<int64_t, int64_t>>
rangeOnDomain(const LogicalMatmul &matmul, ArrayRef<int64_t> coefficients,
              int64_t constant = 0);

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

/// The logical mapping as an exact bijection (docs section 19.13), checked
/// independently of the mixed-radix argument that built it: the source
/// iteration domain is the image of the box of its loops' ordinals z under
/// x = S z + origin (Presburger integer-set equality); the ordinals o of the
/// batch loops and of I, J, K (y_d = offset_d + scale_d o_d) are an integer
/// map of z; their ranges lie in the box batch x [0, M) x [0, N) x [0, K);
/// the map is injective on the z box (no nonzero bounded integer kernel
/// vector, Presburger integer emptiness); and the two boxes have as many
/// points -- so no logical iteration is missing, out of range or
/// duplicated. `why` on failure.
LogicalResult verifyLogicalBijection(const LogicalMatmul &matmul,
                                     const DenseLogicalGemm &dense,
                                     std::string *why);

/// For an assignment C = A * B (matmul.accumulates false): C's full index
/// map, as an integer map of the box of its loops' ordinals z (x = S z +
/// origin, the box verifyLogicalBijection proved the domain equal to), is
/// injective -- no nonzero difference delta of two points of the box with the
/// same C indices (Presburger integer emptiness, the argument of
/// verifyLogicalBijection step 4) -- so every element of C is written exactly
/// once and the nest is the K = 1 GEMM into a zeroed C (docs section 20).
/// Call after verifyLogicalBijection succeeded. `why` on failure.
LogicalResult verifyOutputWrittenOnce(const LogicalMatmul &matmul,
                                      std::string *why);

/// Partition a dense logical GEMM into the given boxes -- the policy is the
/// caller's (for the systolic fleet: SystolicTilePlan.h) -- and prove that
/// they are an exact partition of its iteration domain before returning
/// them: the logical mapping is an exact bijection (verifyLogicalBijection),
/// every box lies inside the domain, together they cover it, and their
/// volumes sum to its volume, so every iteration point is executed by exactly
/// one region. Each region's iteration domain is the source domain restricted
/// to its box.
///
/// Failure (with the reason in `why`) means the bijection is not verified,
/// the boxes are not an exact partition, or `matmul` is not dense; nothing
/// that fails here may be materialized.
FailureOr<SmallVector<LogicalPartition, 1>>
partitionLogicalMatmul(const LogicalMatmul &matmul,
                       ArrayRef<LogicalBox> boxes, std::string *why = nullptr);

} // namespace systolic
} // namespace mlir

#endif // SYSTOLIC_AFFINE_MATMUL_PARTITIONING_H
