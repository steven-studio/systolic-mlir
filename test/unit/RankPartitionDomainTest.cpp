#include "RankPartitionDomain.h"

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

// Construct 0 <= x_i < bound for all coordinates.
static AffineIntegerDomain
makeBoxDomain(std::size_t n, int bound) {

  AffineIntegerDomain domain;
  domain.sourceDimension = n;

  for (std::size_t i = 0; i < n; ++i) {

    IntRow positive(n, BigInt(0));
    IntRow negative(n, BigInt(0));

    positive[i] = 1;
    negative[i] = -1;

    domain.inequalities.push_back({
        positive, BigInt(bound - 1)
    });

    domain.inequalities.push_back({
        negative, BigInt(0)
    });
  }

  return domain;
}

static PartitionCoordinateMap
makeMap(
    const IntMatrix &a,
    const IntMatrix &b,
    const IntMatrix &c,
    std::size_t n) {

  const auto partition =
      analyzeRankPartition(a, b, c, n);

  const auto expanded =
      expandRankPartition(partition);

  const auto map =
      buildPartitionCoordinateMap(expanded);

  require(
      map.has_value(),
      "Failed to construct invertible coordinate map");

  return *map;
}

static void checkRoundTrip(
    const std::string &name,
    const PartitionCoordinateMap &map,
    const IntRow &source) {

  auto logical = mapSourceIteration(map, source);

  require(
      logical.has_value(),
      name + ": forward map failed");

  auto recovered =
      recoverSourceIteration(map, *logical);

  require(
      recovered.has_value(),
      name + ": inverse map failed");

  require(
      *recovered == source,
      name + ": source round-trip mismatch");

  std::cout << "PASS: " << name << '\n';
}

int main() {
  try {
    // Test 1: standard GEMM, U=(i,j,k).
    {
      auto map = makeMap(
          {{1,0,0}, {0,0,1}},
          {{0,0,1}, {0,1,0}},
          {{1,0,0}, {0,1,0}},
          3);

      checkRoundTrip(
          "standard GEMM coordinate round-trip",
          map,
          {2,3,4});
    }

    // Test 2: k+l reduction, U=(i,j,k,l).
    {
      auto map = makeMap(
          {{1,0,0,0}, {0,0,1,1}},
          {{0,0,1,1}, {0,1,0,0}},
          {{1,0,0,0}, {0,1,0,0}},
          4);

      const IntRow source = {1,2,1,2};

      checkRoundTrip(
          "k+l coupled reduction round-trip",
          map,
          source);

      const auto logical =
          mapSourceIteration(map, source);

      require(
          logical.has_value(),
          "k+l forward map failed");

      const auto domain = makeBoxDomain(4, 4);

      require(
          containsLogicalPoint(map, domain, *logical),
          "Valid k+l source iteration rejected");

      // All source points must satisfy 0 <= x_i < 4.
      const auto outside =
          mapSourceIteration(map, {1,2,4,0});

      require(outside.has_value(),
              "Outside-domain forward map failed");

      require(
          !containsLogicalPoint(map, domain, *outside),
          "Outside-domain iteration incorrectly accepted");

      std::cout
          << "PASS: exact coupled-domain membership\n";
    }

    // Test 3: batched GEMM, U=(p,i,j,k).
    {
      auto map = makeMap(
          {{1,0,0,0}, {0,1,0,0}, {0,0,0,1}},
          {{1,0,0,0}, {0,0,1,0}, {0,0,0,1}},
          {{1,0,0,0}, {0,1,0,0}, {0,0,1,0}},
          4);

      checkRoundTrip(
          "batched GEMM coordinate round-trip",
          map,
          {3,1,2,4});
    }

    // Test 4: multiple I/J/K dimensions and original batch.
    // U=(i0,i1,j0,j1,k0,k1,p).
    {
      auto map = makeMap(
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
          7);

      checkRoundTrip(
          "expanded I2/J2/K2/P1 round-trip",
          map,
          {1,2,3,4,5,6,7});
    }

    // Test 5: non-unimodular coordinate map.
    //
    // z=(2*i,j), so z=(1,3) would imply i=1/2.
    {
      ExpandedRankPartition partition;
      partition.sourceRank = 2;
      partition.i = {{2,0}};
      partition.j = {{0,1}};

      const auto map =
          buildPartitionCoordinateMap(partition);

      require(
          map.has_value(),
          "Non-unimodular map incorrectly rejected");

      require(
          !recoverSourceIteration(*map, {1,3}).has_value(),
          "Non-integral source iteration accepted");

      const auto source =
          recoverSourceIteration(*map, {4,3});

      require(
          source.has_value() &&
          *source == IntRow({2,3}),
          "Valid non-unimodular inverse failed");

      std::cout
          << "PASS: non-unimodular integer-lattice check\n";
    }

    // Test 6: singular coordinate matrix must be rejected.
    {
      ExpandedRankPartition partition;
      partition.sourceRank = 2;
      partition.i = {{1,0}};
      partition.j = {{2,0}};

      require(
          !buildPartitionCoordinateMap(partition).has_value(),
          "Singular map incorrectly accepted");

      std::cout << "PASS: singular coordinate rejection\n";
    }

    // Test 7: domain without legal integer points.
    {
      auto map = makeMap(
          {{1,0,0}, {0,0,1}},
          {{0,0,1}, {0,1,0}},
          {{1,0,0}, {0,1,0}},
          3);

      AffineIntegerDomain empty;
      empty.sourceDimension = 3;

      // i <= 0 and i >= 1.
      empty.inequalities.push_back({
          {1,0,0}, BigInt(0)
      });

      empty.inequalities.push_back({
          {-1,0,0}, BigInt(-1)
      });

      const auto logical =
          mapSourceIteration(map, {0,0,0});

      require(logical.has_value(),
              "Empty-domain forward map failed");

      require(
          !containsLogicalPoint(map, empty, *logical),
          "Empty-domain point incorrectly accepted");

      std::cout << "PASS: empty-domain membership\n";
    }

    std::cout
        << "All exact coordinate transformation tests passed.\n";

    return 0;

  } catch (const std::exception &error) {
    std::cerr << "FAIL: " << error.what() << '\n';
    return 1;
  }
}
