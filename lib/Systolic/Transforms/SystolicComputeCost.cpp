#include "Systolic/SystolicComputeCost.h"

namespace mlir {
namespace systolic {

FailureOr<int64_t> estimateSystolicTileComputeCycles(
    const SystolicTile &tile,
    int64_t K,
    const SystolicComputeCostParams &params) {

  // Current tiling scope requires positive reduction depth.
  if (K <= 0)
    return failure();

  // Validate cost-model parameters.
  if (params.kMax <= 0 ||
      params.implementationOverhead < 0)
    return failure();

  // A tile must be mapped to a valid square accelerator.
  if (tile.size <= 0 ||
      tile.acceleratorSize <= 0 ||
      tile.size != tile.acceleratorSize)
    return failure();

  const int64_t rows = tile.acceleratorSize;
  const int64_t cols = tile.acceleratorSize;

  // Number of reduction invocations within this spatial tile.
  //
  //   I = ceil(K / kMax)
  //
  // The final invocation may have a smaller k_i, but
  // sum(k_i) remains exactly K.
  const int64_t numInvocations =
      (K + params.kMax - 1) / params.kMax;

  // Sum of all k_i is exactly K.
  int64_t cycles = K;

  // Every invocation pays:
  //
  //   systolic fill/drain = rows + cols - 2
  //   implementation H    = per-invocation overhead
  //
  // Therefore H is charged numInvocations times.
  const int64_t perInvocationOverhead =
      rows + cols - 2 +
      params.implementationOverhead;

  cycles += numInvocations * perInvocationOverhead;

  return cycles;
}

} // namespace systolic
} // namespace mlir
