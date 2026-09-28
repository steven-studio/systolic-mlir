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

static void printSet(
    const char *name,
    const CountSet &set) {

  std::cout
      << name
      << " (" << set.size() << "):\n";

  for (const CountVector &counts : set) {
    std::cout
        << "  4x4=" << counts[0]
        << " 8x8=" << counts[1]
        << " 16x16=" << counts[2]
        << "\n";
  }
}

static bool checkCase(
    int64_t rows,
    int64_t columns) {

  const std::vector<int64_t> geometries{
      4,
      8,
      16,
  };

  const CountSet oldSet =
      toSet(
          enumerateSystolicGeometryMultisetsDP(
              rows,
              columns,
              geometries));

  const CountSet frontierSet =
      toSet(
          enumerateSystolicGeometryMultisetsFrontierDP(
              rows,
              columns,
              geometries));

  std::cout
      << rows << "x" << columns
      << ": old=" << oldSet.size()
      << " frontier=" << frontierSet.size();

  if (oldSet == frontierSet) {
    std::cout << " MATCH\n";
    return true;
  }

  std::cout << " DIFFER\n";

  printSet("old-only / old", oldSet);
  printSet("frontier", frontierSet);

  return false;
}

int main() {

  // ----------------------------------------------------------
  // B algorithm correctness gate.
  //
  // The first target is intentionally limited to 16x16.
  //
  // Old DP:
  //   rectangular-cut enumeration
  //
  // Frontier DP:
  //   canonical frontier-state enumeration
  //
  // The two implementations must produce exactly the same
  // geometry-count multisets before B can replace the old DP.
  // ----------------------------------------------------------

  bool allMatch = true;

  const std::vector<std::pair<int64_t, int64_t>> cases{
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

  for (const auto &[rows, columns] : cases) {
    if (!checkCase(rows, columns))
      allMatch = false;
  }

  assert(allMatch);

  std::cout
      << "\nFrontier DP matches old DP "
      << "for all 16x16-or-smaller cases.\n";

  return 0;
}
