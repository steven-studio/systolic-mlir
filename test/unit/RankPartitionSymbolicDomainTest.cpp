#include "RankPartitionSymbolicDomain.h"

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

static AffineIntegerDomain
box(std::size_t n, int upperExclusive) {

  AffineIntegerDomain result;
  result.sourceDimension = n;

  for (std::size_t i = 0; i < n; ++i) {

    IntRow positive(n, BigInt(0));
    IntRow negative(n, BigInt(0));

    positive[i] = 1;
    negative[i] = -1;

    result.inequalities.push_back({
        positive, BigInt(upperExclusive - 1)
    });

    result.inequalities.push_back({
        negative, BigInt(0)
    });
  }

  return result;
}

static PartitionCoordinateMap
coordinateMap(
    const IntMatrix &a,
    const IntMatrix &b,
    const IntMatrix &c,
    std::size_t n) {

  const auto p = analyzeRankPartition(a, b, c, n);
  const auto expanded = expandRankPartition(p);
  const auto map = buildPartitionCoordinateMap(expanded);

  require(map.has_value(), "Cannot construct coordinate map");

  return *map;
}

// Compare the independent symbolic predicate with
// the existing exact inverse/domain predicate.
//
// Finite samples provide regression coverage only;
// correctness of the transformation follows from
// the algebraic construction.
static void compare(
    const std::string &name,
    const PartitionCoordinateMap &map,
    const AffineIntegerDomain &domain,
    const IntRow &z) {

  auto symbolic = transformToSymbolicDomain(map, domain);

  require(symbolic.has_value(),
          name + ": symbolic construction failed");

  const bool expected =
      containsLogicalPoint(map, domain, z);

  const bool actual =
      containsSymbolicPoint(*symbolic, z);

  require(expected == actual,
          name + ": predicate mismatch");

  std::cout << "PASS: " << name << '\n';
}

int main() {
  try {

    // Test 1: standard GEMM.
    {
      auto map = coordinateMap(
          {{1,0,0}, {0,0,1}},
          {{0,0,1}, {0,1,0}},
          {{1,0,0}, {0,1,0}},
          3);

      auto domain = box(3, 4);

      auto z = mapSourceIteration(map, {1,2,3});

      require(z.has_value(), "Forward map failed");

      compare("standard GEMM", map, domain, *z);
    }

    // Test 2: coupled reduction k+l.
    {
      auto map = coordinateMap(
          {{1,0,0,0}, {0,0,1,1}},
          {{0,0,1,1}, {0,1,0,0}},
          {{1,0,0,0}, {0,1,0,0}},
          4);

      auto domain = box(4, 4);

      auto inside = mapSourceIteration(map, {1,2,1,2});
      auto outside = mapSourceIteration(map, {1,2,4,0});

      require(inside && outside,
              "Coupled forward mapping failed");

      compare("coupled domain valid point",
              map, domain, *inside);

      compare("coupled domain invalid point",
              map, domain, *outside);

      auto symbolic = transformToSymbolicDomain(map, domain);

      require(symbolic.has_value(),
              "Coupled symbolic transformation failed");

      require(containsSymbolicPoint(*symbolic, *inside),
              "Valid source point rejected");

      require(!containsSymbolicPoint(*symbolic, *outside),
              "Invalid source point accepted");
    }

    // Test 3: non-unimodular map.
    //
    // z=(2*i,j), so z0 must be even.
    {
      ExpandedRankPartition partition;
      partition.sourceRank = 2;
      partition.i = {{2,0}};
      partition.j = {{0,1}};

      const auto map = buildPartitionCoordinateMap(partition);

      require(map.has_value(), "Cannot construct non-unimodular map");

      const auto domain = box(2, 4);
      const auto symbolic =
          transformToSymbolicDomain(*map, domain);

      require(symbolic.has_value(),
              "Non-unimodular symbolic transform failed");

      require(symbolic->denominator == 2,
              "Unexpected common denominator");

      require(!containsSymbolicPoint(*symbolic, {1,1}),
              "Odd logical coordinate incorrectly accepted");

      require(containsSymbolicPoint(*symbolic, {4,1}),
              "Valid even logical coordinate rejected");

      require(!containsSymbolicPoint(*symbolic, {8,1}),
              "Outside-domain logical coordinate accepted");

      compare("non-unimodular congruence",
              *map, domain, {1,1});

      std::cout
          << "PASS: exact integer congruence constraints\n";
    }

    // Test 4: negative logical coordinates.
    {
      ExpandedRankPartition partition;
      partition.sourceRank = 2;
      partition.i = {{2,0}};
      partition.j = {{0,1}};

      const auto map = buildPartitionCoordinateMap(partition);

      require(map.has_value(), "Map construction failed");

      auto domain = box(2, 4);

      compare("negative logical coordinate",
              *map, domain, {-2,1});
    }

    // Test 5: symbolic empty domain.
    {
      auto map = coordinateMap(
          {{1,0,0}, {0,0,1}},
          {{0,0,1}, {0,1,0}},
          {{1,0,0}, {0,1,0}},
          3);

      AffineIntegerDomain empty;
      empty.sourceDimension = 3;

      empty.inequalities.push_back({
          {1,0,0}, BigInt(0)
      });

      empty.inequalities.push_back({
          {-1,0,0}, BigInt(-1)
      });

      auto symbolic = transformToSymbolicDomain(map, empty);

      require(symbolic.has_value(),
              "Empty domain construction failed");

      auto z = mapSourceIteration(map, {0,0,0});

      require(z.has_value(), "Forward map failed");

      require(!containsSymbolicPoint(*symbolic, *z),
              "Empty domain incorrectly accepted a point");

      std::cout << "PASS: symbolic empty domain\n";
    }

    // Test 6: large arbitrary-precision coefficients.
    {
      ExpandedRankPartition partition;
      partition.sourceRank = 2;

      const BigInt large =
          BigInt("100000000000000000000000000000000000000");

      partition.i = {{large,0}};
      partition.j = {{0,1}};

      const auto map = buildPartitionCoordinateMap(partition);

      require(map.has_value(),
              "Large coefficient map failed");

      auto symbolic =
          transformToSymbolicDomain(*map, box(2, 4));

      require(symbolic.has_value(),
              "Large coefficient transformation failed");

      require(
          containsSymbolicPoint(*symbolic, {large * 2, 1}),
          "Valid large-integer point rejected");

      require(
          !containsSymbolicPoint(*symbolic, {1,1}),
          "Invalid large-integer point accepted");

      std::cout
          << "PASS: arbitrary-precision integer constraints\n";
    }

    std::cout
        << "All symbolic integer-domain tests passed.\n";

    return 0;

  } catch (const std::exception &error) {
    std::cerr << "FAIL: " << error.what() << '\n';
    return 1;
  }
}
