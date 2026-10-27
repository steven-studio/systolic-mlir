#include "RankPartitionExpansion.h"

#include <iostream>
#include <stdexcept>
#include <string>

using namespace systolic::logical;

static void require(
    bool condition,
    const std::string &message) {

  if (!condition)
    throw std::runtime_error(message);
}

static void test(
    const std::string &name,
    const IntMatrix &a,
    const IntMatrix &b,
    const IntMatrix &c,
    std::size_t n,
    std::size_t expectedPartitions,
    std::size_t expectedReductionPartitions) {

  const RankPartition original =
      analyzeRankPartition(a, b, c, n);

  const ExpandedRankPartition expanded =
      expandRankPartition(original);

  require(expanded.i.size() <= 1,
          name + ": I rank exceeds one");

  require(expanded.j.size() <= 1,
          name + ": J rank exceeds one");

  require(expanded.k.size() <= 1,
          name + ": K rank exceeds one");

  require(expanded.partitions.size() ==
              expectedPartitions,
          name + ": unexpected partition count");

  std::size_t reductionPartitions = 0;

  for (const auto &axis : expanded.partitions) {
    if (axis.requiresReduction) {
      ++reductionPartitions;

      require(axis.origin == PartitionOrigin::K,
              name + ": incorrect reduction origin");
    }
  }

  require(reductionPartitions ==
              expectedReductionPartitions,
          name + ": incorrect reduction partition count");

  const std::size_t logicalAxes =
      expanded.i.size() +
      expanded.j.size() +
      expanded.k.size();

  require(logicalAxes +
              expanded.partitions.size() == n,
          name + ": source dimensions lost");

  std::cout << "PASS: " << name << '\n';
}

int main() {
  try {
    // U=(i,j,k)
    test(
        "standard GEMM",
        {{1,0,0}, {0,0,1}},
        {{0,0,1}, {0,1,0}},
        {{1,0,0}, {0,1,0}},
        3, 0, 0);

    // U=(p,i,j,k)
    test(
        "batched GEMM",
        {{1,0,0,0}, {0,1,0,0}, {0,0,0,1}},
        {{1,0,0,0}, {0,0,1,0}, {0,0,0,1}},
        {{1,0,0,0}, {0,1,0,0}, {0,0,1,0}},
        4, 1, 0);

    // U=(i0,i1,j0,j1,k0,k1,p)
    // I=2, J=2, K=2, P=1.
    //
    // After expansion:
    // I=1, J=1, K=1, partition axes=4.
    // One partition axis retains reduction semantics.
    test(
        "I2 J2 K2 P1",
        {
          {1,0,0,0,0,0,0},
          {0,1,0,0,0,0,0},
          {0,0,0,0,1,0,0},
          {0,0,0,0,0,1,0},
          {0,0,0,0,0,0,1}
        },
        {
          {0,0,1,0,0,0,0},
          {0,0,0,1,0,0,0},
          {0,0,0,0,1,0,0},
          {0,0,0,0,0,1,0},
          {0,0,0,0,0,0,1}
        },
        {
          {1,0,0,0,0,0,0},
          {0,1,0,0,0,0,0},
          {0,0,1,0,0,0,0},
          {0,0,0,1,0,0,0},
          {0,0,0,0,0,0,1}
        },
        7, 4, 1);

    // U=(i,j,k,l)
    // I=1, J=1, K=1, P=1.
    test(
        "coupled reduction k+l",
        {{1,0,0,0}, {0,0,1,1}},
        {{0,0,1,1}, {0,1,0,0}},
        {{1,0,0,0}, {0,1,0,0}},
        4, 1, 0);

    // U=(i,j): pairwise intersections vanish.
    // I/J/K remain zero-dimensional; both coordinates
    // are retained as partition axes.
    test(
        "essential three-way relation",
        {{1,0}},
        {{0,1}},
        {{1,1}},
        2, 2, 0);

    std::cout
        << "All partition expansion tests passed.\n";

    return 0;

  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
