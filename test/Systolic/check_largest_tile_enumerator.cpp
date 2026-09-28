#include "Systolic/SystolicDecompositionDP.h"

#include "llvm/ADT/SmallVector.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <set>
#include <vector>

using namespace mlir;
using namespace mlir::systolic;

using CountVector = std::vector<int64_t>;
using CountSet = std::set<CountVector>;

static CountSet toSet(
    const llvm::SmallVector<SystolicGeometryMultiset> &input) {

  CountSet result;

  for (const auto &multiset : input) {
    result.insert(
        CountVector(
            multiset.counts.begin(),
            multiset.counts.end()));
  }

  return result;
}

static bool checkCase(
    int64_t rows,
    int64_t columns,
    const llvm::SmallVector<int64_t> &geometries) {

  const auto oldResult =
      enumerateSystolicGeometryMultisetsDP(
          rows,
          columns,
          geometries);

  const auto frontierResult =
      enumerateSystolicGeometryMultisetsFrontierDP(
          rows,
          columns,
          geometries);

  const auto areaFirstResult =
      enumerateSystolicGeometryMultisetsAreaFirstDP(
          rows,
          columns,
          geometries);

  const CountSet oldSet =
      toSet(oldResult);

  const CountSet frontierSet =
      toSet(frontierResult);

  const CountSet areaFirstSet =
      toSet(areaFirstResult);

  std::cout
      << rows << "x" << columns
      << ": old=" << oldSet.size()
      << " frontier=" << frontierSet.size()
      << " area-first=" << areaFirstSet.size();

  if (oldSet == frontierSet &&
      oldSet == areaFirstSet) {

    std::cout << " MATCH\n";

    if (rows == 8 && columns == 16) {
      auto printSet = [](const char *name,
                         const CountSet &set) {
        std::cout << "\n  " << name << ":\n";

        for (const CountVector &counts : set) {
          std::cout
              << "    4x4=" << counts[0]
              << " 8x8=" << counts[1]
              << " 16x16=" << counts[2]
              << "\n";
        }
      };

      printSet("old", oldSet);
      printSet("frontier", frontierSet);
      printSet("area-first", areaFirstSet);
    }

    return true;
  }

  std::cout << " DIFFER\n";

  for (const CountVector &counts : oldSet) {

    if (!frontierSet.count(counts) ||
        !areaFirstSet.count(counts)) {

      std::cout
          << "  old-only:"
          << " 4x4=" << counts[0]
          << " 8x8=" << counts[1]
          << " 16x16=" << counts[2]
          << "\n";
    }
  }

  for (const CountVector &counts : areaFirstSet) {

    if (!oldSet.count(counts)) {

      std::cout
          << "  area-first-only:"
          << " 4x4=" << counts[0]
          << " 8x8=" << counts[1]
          << " 16x16=" << counts[2]
          << "\n";
    }
  }

  return false;
}

int main() {

  const llvm::SmallVector<int64_t> geometries{
      4,
      8,
      16,
  };

  const std::vector<std::pair<int64_t, int64_t>>
      cases{
          {4, 4},
          {8, 8},
          {8, 12},
          {8, 16},
          {12, 12},
          {12, 16},
          {16, 16},
          {20, 20},
          {24, 24},
          {28, 28},
          {32, 32},
      };

  bool allMatch = true;

  for (const auto &[rows, columns] : cases) {

    if (!checkCase(
            rows,
            columns,
            geometries)) {

      allMatch = false;
    }
  }

  if (!allMatch) {

    std::cout
        << "\nAreaFirstDP correctness "
        << "cross-check FAILED.\n";

    return 1;
  }

  std::cout
      << "\nOld DP == Frontier DP == "
      << "AreaFirstDP for all test cases.\n";

  return 0;
}
