#include "Systolic/RankPartitionPresburger.h"
#include <cstdint>
#include <iostream>
#include <limits>
#include "Systolic/RankPartitionPresburgerDomain.h"
using namespace systolic::logical;
static int verify(bool ok, const char *name) {
  if (!ok) { std::cerr << "FAIL: " << name << "\n"; return 1; }
  std::cout << "PASS: " << name << "\n";
  return 0;
}
int main() {
  ExpandedRankPartition p;
  p.sourceRank = 4;
  p.i = {{0, 1, 0, 0}};
  p.j = {{0, 0, 1, 0}};
  p.k = {{0, 0, 0, 1}};
  p.partitions.push_back({{1, 0, 0, 0}, PartitionOrigin::OriginalBatch, false});
  auto map = buildPartitionCoordinateMap(p);
  if (verify(map.has_value(), "invertible partition coordinates")) return 1;
  auto graph = buildCoordinateRelation(*map);
  if (verify(graph && graph->getNumEqualities() == 4,
             "four-dimensional permutation relation")) return 1;
  auto inconsistent = *map;
  inconsistent.sourceToLogical[0][1] = 0;
  if (verify(!buildCoordinateRelation(inconsistent), "inconsistent inverse rejected")) return 1;
  const BigInt large = BigInt(std::numeric_limits<std::int64_t>::max()) + 1;
  PartitionCoordinateMap bigMap;
  bigMap.sourceToLogical = {{large}};
  bigMap.logicalToSource = {{Rat(1, large)}};
  if (verify(!buildCoordinateRelation(bigMap), "unrepresentable coefficient rejected")) return 1;
  const BigInt minimum(std::numeric_limits<std::int64_t>::min());
  PartitionCoordinateMap minMap;

  // det(T) = -1, so T is exactly invertible over the integers.
  // T = [minimum, 1; 1, 0]
  // T^-1 = [0, 1; 1, -minimum]
  //
  // Constructing the inverse requires no rational denominator
  // equal to INT64_MIN.
  minMap.sourceToLogical = {
      {minimum, BigInt(1)},
      {BigInt(1), BigInt(0)}
  };

  minMap.logicalToSource = {
      {Rat(0), Rat(1)},
      {Rat(1), Rat(-minimum)}
  };

  if (verify(!buildCoordinateRelation(minMap),
             "signed-negation overflow rejected"))
    return 1;

  // --------------------------------------------------------
  // Phase 5.2.2: Exact symbolic domain composition.
  // --------------------------------------------------------
  {
    using mlir::presburger::IntegerPolyhedron;
    using mlir::presburger::PresburgerSpace;

    // Source variables: x0, x1
    // Local variable: q
    //
    // Domain:
    //   0 <= x0 <= 3
    //   x1 = 2*q
    //
    // T:
    //   z0 = x1
    //   z1 = x0

    PartitionCoordinateMap domainMap;

    domainMap.sourceToLogical = {
        {BigInt(0), BigInt(1)},
        {BigInt(1), BigInt(0)}
    };

    domainMap.logicalToSource = {
        {Rat(0), Rat(1)},
        {Rat(1), Rat(0)}
    };

    IntegerPolyhedron source(
        PresburgerSpace::getSetSpace(2, 0, 1));

    // [x0, x1, q, constant]
    source.addInequality({1, 0, 0, 0});
    source.addInequality({-1, 0, 0, 3});
    source.addEquality({0, 1, -2, 0});

    auto combined =
        buildCoordinateRelationOnDomain(domainMap, source);

    if (verify(combined.has_value(),
               "symbolic source-domain composition"))
      return 1;

    if (verify(
            combined->getSpace().getNumDomainVars() == 2 &&
            combined->getSpace().getNumRangeVars() == 2 &&
            combined->getSpace().getNumLocalVars() == 1,
            "existential local variables preserved"))
      return 1;

    if (verify(
            combined->getNumEqualities() == 3 &&
            combined->getNumInequalities() == 2,
            "all symbolic constraints preserved"))
      return 1;

    // Verify exact coefficient placement independently.
    //
    // Result layout:
    // [x0, x1, z0, z1, q, constant]

    auto hasRow = [](const mlir::presburger::IntegerRelation &relation,
                     bool equality,
                     std::initializer_list<int64_t> expected) {

      const unsigned count = equality
          ? relation.getNumEqualities()
          : relation.getNumInequalities();

      for (unsigned i = 0; i < count; ++i) {
        auto row = equality
            ? relation.getEquality(i)
            : relation.getInequality(i);

        if (row.size() == expected.size() &&
            std::equal(
                row.begin(), row.end(), expected.begin(),
                [](const llvm::DynamicAPInt &actual,
                   int64_t expectedValue) {
                  return actual ==
                         llvm::DynamicAPInt(expectedValue);
                }))
          return true;
      }

      return false;
    };

    const bool correct =
        // z0 - x1 = 0
        hasRow(*combined, true, {0, -1, 1, 0, 0, 0}) &&
        // z1 - x0 = 0
        hasRow(*combined, true, {-1, 0, 0, 1, 0, 0}) &&
        // x1 - 2q = 0
        hasRow(*combined, true, {0, 1, 0, 0, -2, 0}) &&
        // x0 >= 0
        hasRow(*combined, false, {1, 0, 0, 0, 0, 0}) &&
        // 3 - x0 >= 0
        hasRow(*combined, false, {-1, 0, 0, 0, 0, 3});

    if (verify(correct, "exact equality and inequality coefficients"))
      return 1;

    // Wrong source dimension must be rejected.
    IntegerPolyhedron wrongDimension(
        PresburgerSpace::getSetSpace(3));

    if (verify(
            !buildCoordinateRelationOnDomain(domainMap, wrongDimension),
            "source dimension mismatch rejected"))
      return 1;

    // External symbols are not yet supported.
    IntegerPolyhedron symbolic(
        PresburgerSpace::getSetSpace(2, 1, 0));

    if (verify(
            !buildCoordinateRelationOnDomain(domainMap, symbolic),
            "unresolved symbol rejected"))
      return 1;
  }

  std::cout << "All coordinate relation tests passed.\n";
  return 0;
}
