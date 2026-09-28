#include "Systolic/SystolicComputeCost.h"

namespace mlir {
namespace systolic {

namespace {

/// Shared implementation for square systolic compute cost.
///
/// Both SystolicTile and SystolicExecutionTask carry the two
/// fields required by the current analytical model:
///
///   size
///   acceleratorSize
///
/// Keeping the formula here prevents the two public overloads
/// from drifting apart.
FailureOr<int64_t> estimateSquareComputeCycles(
    int64_t size,
    int64_t acceleratorSize,
    int64_t K,
    const SystolicComputeCostParams &params) {

  // Current tiling scope requires positive reduction depth.
  if (K <= 0)
    return failure();

  // Validate cost-model parameters.
  if (params.kMax <= 0 ||
      params.implementationOverhead < 0)
    return failure();

  // Current scope requires a square spatial task whose size
  // exactly matches its requested accelerator geometry.
  if (size <= 0 ||
      acceleratorSize <= 0 ||
      size != acceleratorSize)
    return failure();

  const int64_t rows = acceleratorSize;
  const int64_t cols = acceleratorSize;

  // Number of reduction invocations:
  //
  //   I = ceil(K / kMax)
  //
  const int64_t numInvocations =
      (K + params.kMax - 1) /
      params.kMax;

  // Since sum_i k_i = K, the total compute cost is:
  //
  //   T = K + I * (rows + cols - 2 + H)
  //
  int64_t cycles = K;

  const int64_t perInvocationOverhead =
      rows +
      cols -
      2 +
      params.implementationOverhead;

  cycles +=
      numInvocations *
      perInvocationOverhead;

  return cycles;
}

} // namespace

FailureOr<int64_t> estimateSystolicTileComputeCycles(
    const SystolicTile &tile,
    int64_t K,
    const SystolicComputeCostParams &params) {

  return estimateSquareComputeCycles(
      tile.size,
      tile.acceleratorSize,
      K,
      params);
}

FailureOr<int64_t> estimateSystolicTileComputeCycles(
    const SystolicExecutionTask &task,
    int64_t K,
    const SystolicComputeCostParams &params) {

  return estimateSquareComputeCycles(
      task.size,
      task.acceleratorSize,
      K,
      params);
}

FailureOr<int64_t> estimateSystolicTaskComputeCycles(
    const SystolicExecutionTask &task,
    const SystolicArrayResource &resource,
    int64_t K,
    const SystolicComputeCostParams &params) {

  if (K <= 0)
    return failure();

  if (params.kMax <= 0 ||
      params.implementationOverhead < 0)
    return failure();

  if (task.size <= 0 ||
      resource.arraySize <= 0)
    return failure();

  // A physical array cannot execute a logical spatial task
  // larger than its physical geometry.
  if (resource.arraySize < task.size)
    return failure();

  // IMPORTANT:
  //
  // The geometry term is determined by the PHYSICAL systolic
  // array, not by the logical task size.
  //
  // Example:
  //
  //   4x4 work on 4x4 physical array:
  //     geometry term = 4 + 4 - 2 = 6
  //
  //   4x4 work on 8x8 physical array:
  //     geometry term = 8 + 8 - 2 = 14
  //
  const int64_t rows = resource.arraySize;
  const int64_t cols = resource.arraySize;

  const int64_t numInvocations =
      (K + params.kMax - 1) /
      params.kMax;

  const int64_t perInvocationOverhead =
      rows +
      cols -
      2 +
      params.implementationOverhead;

  return K +
      numInvocations *
      perInvocationOverhead;
}

} // namespace systolic
} // namespace mlir
