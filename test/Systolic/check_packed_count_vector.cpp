#include "Systolic/SystolicDecompositionDP.h"

#include "llvm/ADT/SmallVector.h"

#include <cstdint>
#include <iostream>
#include <vector>

using namespace mlir;
using namespace mlir::systolic;

int main() {

  // ----------------------------------------------------------
  // Four geometries are required to exercise geometry index 3.
  //
  // With 20 bits per count:
  //
  //   index 0 -> bits  0..19
  //   index 1 -> bits 20..39
  //   index 2 -> bits 40..59
  //   index 3 -> bits 60..79
  //
  // Therefore index 3 crosses a uint64_t word boundary.
  // ----------------------------------------------------------

  const llvm::SmallVector<int64_t> geometries{
      1,
      2,
      4,
      8,
  };

  // 8x8 can be tiled entirely by one 8x8 tile.
  const auto result =
      enumerateSystolicGeometryMultisetsFrontierDP(
          8,
          8,
          geometries);

  for (const auto &multiset : result) {

    if (multiset.counts.size() !=
        geometries.size()) {

      std::cerr
          << "FAIL: wrong count-vector size\n";

      return 1;
    }

    std::cout
        << "counts:";

    for (int64_t count :
         multiset.counts) {

      std::cout
          << " " << count;
    }

    std::cout
        << "\n";
  }

  std::cout
      << "PASS: packed count-vector "
      << "decode test completed.\n";

  return 0;
}
