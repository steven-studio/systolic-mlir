#pragma once

#include "TensorLogicalSpaces.h"

#include <cstddef>
#include <stdexcept>
#include <utility>
#include <vector>

namespace systolic::logical {

enum class PartitionOrigin {
  I,
  J,
  K,
  OriginalBatch
};

struct PartitionAxis {
  IntRow basis;
  PartitionOrigin origin;

  // True when this partition originated from K.
  // Such partitions contribute to the same output reduction.
  bool requiresReduction = false;
};

struct ExpandedRankPartition {
  IntMatrix i;
  IntMatrix j;
  IntMatrix k;

  std::vector<PartitionAxis> partitions;

  // A zero-rank logical role requires a virtual size-one axis
  // during future lowering; no source dimension is invented.
  bool virtualI = false;
  bool virtualJ = false;
  bool virtualK = false;

  std::size_t sourceRank = 0;
};

inline ExpandedRankPartition
expandRankPartition(const RankPartition &input) {

  ExpandedRankPartition result;
  result.sourceRank = input.sourceRank;

  auto split = [&](const IntMatrix &space,
                   IntMatrix &active,
                   PartitionOrigin origin,
                   bool reduction) {

    if (space.empty())
      return true;

    // Keep one logical dimension.
    active.push_back(space.front());

    // Move every additional dimension to partition axes.
    for (std::size_t index = 1;
         index < space.size(); ++index) {

      result.partitions.push_back({
          space[index],
          origin,
          reduction
      });
    }

    return false;
  };

  result.virtualI = split(
      input.i,
      result.i,
      PartitionOrigin::I,
      false);

  result.virtualJ = split(
      input.j,
      result.j,
      PartitionOrigin::J,
      false);

  result.virtualK = split(
      input.k,
      result.k,
      PartitionOrigin::K,
      true);

  // Every dimension from the original P becomes its
  // own partition axis.
  for (const auto &basis : input.batch) {
    result.partitions.push_back({
        basis,
        PartitionOrigin::OriginalBatch,
        false
    });
  }

  if (result.i.size() > 1 ||
      result.j.size() > 1 ||
      result.k.size() > 1)
    throw std::runtime_error(
        "Logical rank expansion failed");

  // Reconstruct all source covector directions.
  IntMatrix reconstructed;

  for (const auto &row : result.i)
    reconstructed.push_back(row);

  for (const auto &row : result.j)
    reconstructed.push_back(row);

  for (const auto &row : result.k)
    reconstructed.push_back(row);

  for (const auto &axis : result.partitions)
    reconstructed.push_back(axis.basis);

  if (reconstructed.size() != input.sourceRank)
    throw std::runtime_error(
        "Partition expansion lost source dimensions");

  if (detail::rank(
          reconstructed, input.sourceRank) !=
      input.sourceRank)
    throw std::runtime_error(
        "Partition expansion lost source rank");

  return result;
}

} // namespace systolic::logical
