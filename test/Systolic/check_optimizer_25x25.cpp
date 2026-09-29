#include "Systolic/SystolicOptimizer.h"

#include <cstdint>
#include <iostream>
#include <map>

using namespace mlir::systolic;

int main() {
  SystolicArrayResource fleet[] = {
      {16, 0},
      {8, 0},
      {8, 1},
      {4, 0},
      {4, 1},
      {4, 2},
  };

  auto result =
      optimizeSystolicGreedy(
          25,
          25,
          fleet);

  if (failed(result)) {
    std::cerr
        << "FAIL: optimizeSystolicGreedy() returned failure\n";
    return 1;
  }

  std::map<int64_t, int64_t> counts;

  for (const auto &task : result->decomposition)
    ++counts[task.size];

  std::cout << "Decomposition:\n";

  for (const auto &[size, count] : counts) {
    std::cout
        << "  "
        << count
        << " x "
        << size
        << "x"
        << size
        << "\n";
  }

  std::cout
      << "\nMakespan = "
      << result->makespan
      << "\n";

  if (counts[16] != 1 ||
      counts[8] != 5 ||
      counts[4] != 13) {
    std::cerr
        << "FAIL: unexpected decomposition\n";
    return 1;
  }

  if (result->makespan != 42) {
    std::cerr
        << "FAIL: expected makespan 42\n";
    return 1;
  }

  std::cout
      << "PASS: optimizer 25x25\n";

  return 0;
}
