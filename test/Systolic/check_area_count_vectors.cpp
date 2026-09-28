#include "Systolic/SystolicDecompositionDP.h"

#include "llvm/ADT/SmallVector.h"

#include <cstdint>
#include <iostream>
#include <set>
#include <vector>

using namespace mlir;
using namespace mlir::systolic;

using CountVector = std::vector<int64_t>;

static std::set<CountVector>
enumerateAreaFeasible(
    int64_t rows,
    int64_t columns,
    const std::vector<int64_t> &geometries) {

  std::set<CountVector> result;

  if (geometries.size() != 3)
    return result;

  const int64_t totalArea =
      rows * columns;

  const int64_t area0 =
      geometries[0] * geometries[0];

  const int64_t area1 =
      geometries[1] * geometries[1];

  const int64_t area2 =
      geometries[2] * geometries[2];

  for (int64_t n2 = 0;
       n2 * area2 <= totalArea;
       ++n2) {

    for (int64_t n1 = 0;
         n2 * area2 +
                 n1 * area1 <=
             totalArea;
         ++n1) {

      const int64_t remaining =
          totalArea -
          n2 * area2 -
          n1 * area1;

      if (remaining < 0)
        continue;

      if (remaining % area0 != 0)
        continue;

      const int64_t n0 =
          remaining / area0;

      result.insert(
          CountVector{n0, n1, n2});
    }
  }

  return result;
}

static bool checkCase(
    int64_t rows,
    int64_t columns,
    const std::vector<int64_t> &geometries) {

  const auto areaFeasible =
      enumerateAreaFeasible(
          rows,
          columns,
          geometries);

  const auto dp =
      enumerateSystolicGeometryMultisetsDP(
          rows,
          columns,
          geometries);

  std::set<CountVector> dpSet;

  for (const auto &candidate : dp) {
    dpSet.insert(
        CountVector(
            candidate.counts.begin(),
            candidate.counts.end()));
  }

  std::cout
      << rows << "x" << columns
      << ": area=" << areaFeasible.size()
      << " dp=" << dpSet.size();

  if (areaFeasible == dpSet) {
    std::cout << " MATCH\n";
    return true;
  }

  std::cout << " DIFFER\n";

  int shown = 0;

  for (const CountVector &counts :
       areaFeasible) {

    if (dpSet.count(counts))
      continue;

    std::cout
        << "  area-only:"
        << " 4x4=" << counts[0]
        << " 8x8=" << counts[1]
        << " 16x16=" << counts[2]
        << "\n";

    if (++shown >= 10)
      break;
  }

  shown = 0;

  for (const CountVector &counts : dpSet) {

    if (areaFeasible.count(counts))
      continue;

    std::cout
        << "  dp-only:"
        << " 4x4=" << counts[0]
        << " 8x8=" << counts[1]
        << " 16x16=" << counts[2]
        << "\n";

    if (++shown >= 10)
      break;
  }

  return false;
}

int main() {
  const std::vector<int64_t> geometries{
      4,
      8,
      16,
  };

  const std::vector<std::pair<
      int64_t,
      int64_t>>
      cases{
          {16, 16},
          {16, 20},
          {16, 24},
          {20, 20},
          {20, 24},
          {24, 24},
          {24, 32},
          {28, 28},
          {32, 32},
          {32, 48},
          {40, 40},
          {48, 48},
          {64, 64},
      };

  bool allMatch = true;

  for (const auto &[rows, columns] :
       cases) {
    if (!checkCase(
            rows,
            columns,
            geometries))
      allMatch = false;
  }

  if (allMatch) {
    std::cout
        << "All area-feasible count sets "
        << "match decomposition DP.\n";
    return 0;
  }

  std::cout
      << "At least one area-feasible count "
      << "set differs from decomposition DP.\n";

  return 0;
}
