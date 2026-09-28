#include "Systolic/SystolicDecompositionDP.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <set>
#include <vector>

using namespace mlir::systolic;

namespace {

using Counts = std::vector<int64_t>;

std::set<Counts>
toSet(const llvm::SmallVector<SystolicGeometryMultiset> &results) {
  std::set<Counts> output;

  for (const auto &multiset : results) {
    output.insert(
        Counts(
            multiset.counts.begin(),
            multiset.counts.end()));
  }

  return output;
}

void printResults(
    int64_t rows,
    int64_t columns,
    const llvm::SmallVector<SystolicGeometryMultiset> &results) {

  std::cout
      << "DP[" << rows
      << "][" << columns
      << "]\n";

  for (const auto &multiset : results) {
    std::cout
        << "  4x4="
        << multiset.counts[0]
        << "  8x8="
        << multiset.counts[1]
        << "\n";
  }
}

} // namespace

int main() {
  llvm::SmallVector<int64_t> geometries = {
      4,
      8,
  };

  // ----------------------------------------------------------
  // DP[8][12]
  // ----------------------------------------------------------

  auto result8x12 =
      enumerateSystolicGeometryMultisetsDP(
          8,
          12,
          geometries);

  printResults(
      8,
      12,
      result8x12);

  std::set<Counts> expected8x12 = {
      {2, 1},
      {6, 0},
  };

  assert(toSet(result8x12) == expected8x12);

  // ----------------------------------------------------------
  // DP[16][16]
  // ----------------------------------------------------------

  auto result16x16 =
      enumerateSystolicGeometryMultisetsDP(
          16,
          16,
          geometries);

  printResults(
      16,
      16,
      result16x16);

  std::set<Counts> expected16x16 = {
      {0, 4},
      {4, 3},
      {8, 2},
      {12, 1},
      {16, 0},
  };

  assert(toSet(result16x16) == expected16x16);

  std::cout
      << "All decomposition DP tests passed.\n";

  return 0;
}
