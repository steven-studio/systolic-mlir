#include "Systolic/SystolicDecompositionDP.h"
#include "Systolic/SystolicTiling.h"

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


std::set<Counts>
bruteForceMultisets(
    int64_t size,
    llvm::ArrayRef<SystolicArrayResource> fleet,
    llvm::ArrayRef<int64_t> geometries,
    size_t &spatialDecompositionCount) {

  SystolicTile tile;
  tile.row = 0;
  tile.column = 0;
  tile.size = size;
  tile.acceleratorSize = size;
  tile.acceleratorId = -1;

  auto decompositions =
      enumerateSystolicDecompositions(
          tile,
          fleet);

  spatialDecompositionCount =
      decompositions.size();

  std::set<Counts> output;

  for (const auto &decomposition :
       decompositions) {

    Counts counts(
        geometries.size(),
        0);

    for (const SystolicExecutionTask &task :
         decomposition) {

      bool foundGeometry = false;

      for (size_t geometryIndex = 0;
           geometryIndex < geometries.size();
           ++geometryIndex) {

        if (task.size !=
            geometries[geometryIndex])
          continue;

        ++counts[geometryIndex];
        foundGeometry = true;
        break;
      }

      assert(foundGeometry);
    }

    output.insert(
        std::move(counts));
  }

  return output;
}

void checkDPAgainstBruteForce(
    int64_t size,
    llvm::ArrayRef<SystolicArrayResource> fleet,
    llvm::ArrayRef<int64_t> geometries) {

  auto dp =
      enumerateSystolicGeometryMultisetsDP(
          size,
          size,
          geometries);

  std::set<Counts> dpSet =
      toSet(dp);

  size_t spatialCount = 0;

  std::set<Counts> bruteSet =
      bruteForceMultisets(
          size,
          fleet,
          geometries,
          spatialCount);

  std::cout
      << "cross-check "
      << size << "x" << size
      << ": spatial="
      << spatialCount
      << " unique-multisets="
      << bruteSet.size()
      << " dp="
      << dpSet.size()
      << "\n";

  assert(dpSet == bruteSet);
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

  // ----------------------------------------------------------
  // Cross-validation:
  //
  // Collapse the brute-force spatial decompositions into
  // geometry multisets and require them to match DP[R][C].
  // ----------------------------------------------------------

  llvm::SmallVector<SystolicArrayResource> fleet = {
      {4, 0},
      {8, 1},
  };

  checkDPAgainstBruteForce(
      4,
      fleet,
      geometries);

  checkDPAgainstBruteForce(
      8,
      fleet,
      geometries);

  checkDPAgainstBruteForce(
      12,
      fleet,
      geometries);

  checkDPAgainstBruteForce(
      16,
      fleet,
      geometries);

  std::cout
      << "DP multiset enumeration matches brute-force oracle.\n";


  // ----------------------------------------------------------
  // DP-only candidate growth.
  //
  // Do not run the brute-force spatial oracle here: the purpose
  // is to observe how the number of unique geometry multisets
  // grows for larger rectangles.
  // ----------------------------------------------------------

  std::cout
      << "DP candidate growth:\n";

  for (int64_t size :
       {4, 8, 12, 16, 20, 24, 28, 32}) {

    auto candidates =
        enumerateSystolicGeometryMultisetsDP(
            size,
            size,
            geometries);

    std::cout
        << "  "
        << size << "x" << size
        << ": "
        << candidates.size()
        << " unique multisets\n";
  }

  std::cout
      << "All decomposition DP tests passed.\n";

  return 0;
}
