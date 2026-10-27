#include "Systolic/RankPartitionPresburger.h"
#include <cstdint>
#include <iostream>
#include <limits>
#include "Systolic/RankPartitionPresburgerDomain.h"
#include "Systolic/RankPartitionWriteAddress.h"
#include "Systolic/RankPartitionCollision.h"
#include "Systolic/RankPartitionOrderedDependence.h"
#include "Systolic/RankPartitionDependenceLegality.h"
#include "Systolic/RankPartitionReductionSemantics.h"
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


  // --------------------------------------------------------
  // Phase 5.3.1: Exact symbolic output address.
  // --------------------------------------------------------
  {
    // Source order: (b, i, j, k)
    //
    // Logical order: (I, J, K, Batch) = (i, j, k, b)
    //
    // Original output:
    //
    //   C[b][b+i][j]
    //
    // Expected logical output:
    //
    //   C[Batch][Batch+I][J]

    PartitionCoordinateMap writeMap;

    writeMap.sourceToLogical = {
        {BigInt(0), BigInt(1), BigInt(0), BigInt(0)},
        {BigInt(0), BigInt(0), BigInt(1), BigInt(0)},
        {BigInt(0), BigInt(0), BigInt(0), BigInt(1)},
        {BigInt(1), BigInt(0), BigInt(0), BigInt(0)}
    };

    writeMap.logicalToSource = {
        {Rat(0), Rat(0), Rat(0), Rat(1)},
        {Rat(1), Rat(0), Rat(0), Rat(0)},
        {Rat(0), Rat(1), Rat(0), Rat(0)},
        {Rat(0), Rat(0), Rat(1), Rat(0)}
    };

    const std::vector<std::vector<BigInt>> outputRows = {
        {BigInt(1), BigInt(0), BigInt(0), BigInt(0)},
        {BigInt(1), BigInt(1), BigInt(0), BigInt(0)},
        {BigInt(0), BigInt(0), BigInt(1), BigInt(0)}
    };

    const std::vector<BigInt> outputOffsets = {
        BigInt(0), BigInt(0), BigInt(0)
    };

    auto writeAddress =
        buildSymbolicWriteAddress(
            writeMap, outputRows, outputOffsets);

    if (verify(writeAddress.has_value(),
               "symbolic write-address construction"))
      return 1;

    const std::vector<std::vector<Rat>> expected = {
        {Rat(0), Rat(0), Rat(0), Rat(1)},
        {Rat(1), Rat(0), Rat(0), Rat(1)},
        {Rat(0), Rat(1), Rat(0), Rat(0)}
    };

    if (verify(writeAddress->coefficients == expected,
               "exact transformed C access"))
      return 1;

    if (verify(
            writeAddress->offsets ==
                std::vector<Rat>{Rat(0), Rat(0), Rat(0)},
            "output offsets preserved"))
      return 1;

    // A malformed output access must not be accepted.
    const std::vector<std::vector<BigInt>> invalidRows = {
        {BigInt(1), BigInt(0)}
    };

    if (verify(
            !buildSymbolicWriteAddress(
                writeMap,
                invalidRows,
                {BigInt(0)}),
            "malformed output access rejected"))
      return 1;
  }


  // --------------------------------------------------------
  // Phase 5.3.2: Exact symbolic write collisions.
  // --------------------------------------------------------
  {
    using mlir::presburger::IntegerPolyhedron;
    using mlir::presburger::PresburgerSpace;

    // Source:
    //   x = (x0, x1)
    //
    // Logical:
    //   z = (x1, x0)
    //
    // Domain:
    //   0 <= x0 <= 2
    //   x1 = 2*q
    //
    // Output:
    //   C[x0]
    //
    // Therefore two iterations collide exactly when:
    //
    //   x0_first = x0_second
    //
    // Different x1 values may update the same C.

    PartitionCoordinateMap collisionMap;

    collisionMap.sourceToLogical = {
        {BigInt(0), BigInt(1)},
        {BigInt(1), BigInt(0)}
    };

    collisionMap.logicalToSource = {
        {Rat(0), Rat(1)},
        {Rat(1), Rat(0)}
    };

    IntegerPolyhedron collisionDomain(
        PresburgerSpace::getSetSpace(2, 0, 1));

    // [x0, x1, q, constant]
    collisionDomain.addInequality({1, 0, 0, 0});
    collisionDomain.addInequality({-1, 0, 0, 2});
    collisionDomain.addEquality({0, 1, -2, 0});

    const std::vector<std::vector<BigInt>> outputRows = {
        {BigInt(1), BigInt(0)}
    };

    const std::vector<BigInt> outputOffsets = {
        BigInt(5)
    };

    auto collisions =
        buildCoordinateCollisionRelation(
            collisionMap,
            collisionDomain,
            outputRows,
            outputOffsets);

    if (verify(collisions.has_value(),
               "symbolic collision relation construction"))
      return 1;

    if (verify(
            collisions->getNumDomainVars() == 4 &&
            collisions->getNumRangeVars() == 4 &&
            collisions->getNumLocalVars() == 2,
            "independent source and local variables preserved"))
      return 1;

    // 2 copies of:
    //   2 coordinate equalities
    //   1 source-domain equality
    //
    // Plus:
    //   1 collision equality
    //
    // Total: 7 equalities, 4 inequalities.

    if (verify(
            collisions->getNumEqualities() == 7 &&
            collisions->getNumInequalities() == 4,
            "both domains and collision equality preserved"))
      return 1;

    // Full relation layout:
    //
    // [x0_1, x1_1, z0_1, z1_1,
    //  x0_2, x1_2, z0_2, z1_2,
    //  q1, q2, constant]

    auto hasEquality = [&](std::initializer_list<int64_t> expected) {

      for (unsigned i = 0;
           i < collisions->getNumEqualities();
           ++i) {

        auto row = collisions->getEquality(i);

        if (row.size() != expected.size())
          continue;

        if (std::equal(
                row.begin(), row.end(), expected.begin(),
                [](const llvm::DynamicAPInt &actual,
                   int64_t value) {
                  return actual == llvm::DynamicAPInt(value);
                }))
          return true;
      }

      return false;
    };

    const bool exact =
        // Both iterations must write the same C[x0+5].
        hasEquality({
            1, 0, 0, 0,
           -1, 0, 0, 0,
            0, 0, 0
        }) &&

        // First iteration: x1_1 = 2*q1.
        hasEquality({
            0, 1, 0, 0,
            0, 0, 0, 0,
           -2, 0, 0
        }) &&

        // Second iteration: x1_2 = 2*q2.
        hasEquality({
            0, 0, 0, 0,
            0, 1, 0, 0,
            0, -2, 0
        });

    if (verify(exact,
               "exact collision equality and separate locals"))
      return 1;

    // Invalid C access dimensions must be rejected.
    if (verify(
            !buildCoordinateCollisionRelation(
                collisionMap,
                collisionDomain,
                {{BigInt(1)}},
                {BigInt(0)}),
            "malformed collision output access rejected"))
      return 1;

    // Reject coefficients whose negation is not
    // representable in signed int64.
    const BigInt minimum(
        std::numeric_limits<int64_t>::min());

    if (verify(
            !buildCoordinateCollisionRelation(
                collisionMap,
                collisionDomain,
                {{minimum, BigInt(0)}},
                {BigInt(0)}),
            "collision coefficient overflow rejected"))
      return 1;
  }


  // --------------------------------------------------------
  // Phase 5.3.3: ordered write-dependence as a disjunction.
  // --------------------------------------------------------
  {
    using mlir::presburger::IntegerPolyhedron;
    using mlir::presburger::PresburgerSpace;

    // x=(x0,x1), z=(x1,x0), C[x0+5].
    // Domain: 0<=x0<=2, x1=2*q (q is existential).
    PartitionCoordinateMap orderMap;
    orderMap.sourceToLogical = {
        {BigInt(0), BigInt(1)},
        {BigInt(1), BigInt(0)}
    };
    orderMap.logicalToSource = {
        {Rat(0), Rat(1)},
        {Rat(1), Rat(0)}
    };

    IntegerPolyhedron orderDomain(
        PresburgerSpace::getSetSpace(2, 0, 1));
    orderDomain.addInequality({1, 0, 0, 0});
    orderDomain.addInequality({-1, 0, 0, 2});
    orderDomain.addEquality({0, 1, -2, 0});

    auto ordered = buildOrderedWriteDependenceBranches(
        orderMap, orderDomain,
        {{BigInt(1), BigInt(0)}}, {BigInt(5)});

    if (verify(ordered.has_value() && ordered->size() == 2,
               "lexicographic disjunction has two branches"))
      return 1;

    // Layout: [x0_1,x1_1,z0_1,z1_1,
    //          x0_2,x1_2,z0_2,z1_2,q1,q2,constant].
    auto contains = [](const mlir::presburger::IntegerRelation &rel,
                       bool equality,
                       std::initializer_list<int64_t> expected) {
      const unsigned count = equality ? rel.getNumEqualities()
                                      : rel.getNumInequalities();
      for (unsigned i = 0; i < count; ++i) {
        auto row = equality ? rel.getEquality(i) : rel.getInequality(i);
        if (row.size() == expected.size() &&
            std::equal(row.begin(), row.end(), expected.begin(),
                       [](const llvm::DynamicAPInt &a, int64_t b) {
                         return a == llvm::DynamicAPInt(b);
                       }))
          return true;
      }
      return false;
    };

    const auto &first = (*ordered)[0];
    const auto &second = (*ordered)[1];

    // t=0: x0_2-x0_1-1 >= 0.
    if (verify(contains(first, false,
                        {-1,0,0,0, 1,0,0,0, 0,0,-1}),
               "first-difference branch at x0"))
      return 1;

    // t=1: x0_1=x0_2, x1_2-x1_1-1 >= 0.
    if (verify(contains(second, true,
                        {1,0,0,0, -1,0,0,0, 0,0,0}) &&
               contains(second, false,
                        {0,-1,0,0, 0,1,0,0, 0,0,-1}),
               "first-difference branch at x1"))
      return 1;

    // Both branches retain the two independent local variables
    // and the original collision equality x0_1 == x0_2.
    const bool retained =
        first.getNumLocalVars() == 2 &&
        second.getNumLocalVars() == 2 &&
        first.getNumEqualities() == 7 &&
        second.getNumEqualities() == 8 &&
        first.getNumInequalities() == 5 &&
        second.getNumInequalities() == 5 &&
        contains(first, true, {1,0,0,0, -1,0,0,0, 0,0,0}) &&
        contains(second, true, {0,1,0,0, 0,0,0,0, -2,0,0}) &&
        contains(second, true, {0,0,0,0, 0,1,0,0, 0,-2,0});
    if (verify(retained, "collision and distinct locals retained"))
      return 1;

    // Domain mismatch is a recoverable failure, not an exception.
    IntegerPolyhedron wrongDomain(PresburgerSpace::getSetSpace(1));
    if (verify(!buildOrderedWriteDependenceBranches(
                   orderMap, wrongDomain,
                   {{BigInt(1), BigInt(0)}}, {BigInt(5)}),
               "ordered dependence rejects mismatched domain"))
      return 1;
  }


  // --------------------------------------------------------
  // Phase 5.3.4: integer-exact feasible ordered conflicts.
  // --------------------------------------------------------
  {
    using mlir::presburger::IntegerPolyhedron;
    using mlir::presburger::PresburgerSpace;

    PartitionCoordinateMap identity;
    identity.sourceToLogical = {
        {BigInt(1), BigInt(0)}, {BigInt(0), BigInt(1)}};
    identity.logicalToSource = {
        {Rat(1), Rat(0)}, {Rat(0), Rat(1)}};

    // x0 in [0,2], x1 in [0,2]; C[x0] has conflicts along x1.
    IntegerPolyhedron domain(PresburgerSpace::getSetSpace(2));
    domain.addInequality({1, 0, 0});
    domain.addInequality({-1, 0, 2});
    domain.addInequality({0, 1, 0});
    domain.addInequality({0, -1, 2});

    auto analysis = analyzeOrderedWriteConflicts(
        identity, domain, {{BigInt(1), BigInt(0)}}, {BigInt(4)});
    if (verify(analysis.has_value(),
               "ordered conflict feasibility analysis"))
      return 1;
    // Branch t=0 contradicts the C[x0] equality. Branch t=1 is feasible.
    if (verify(analysis->testedBranches == 2 &&
               analysis->emptyBranches == 1 &&
               analysis->feasibleBranches.size() == 1,
               "integer-empty lexicographic branch removed"))
      return 1;

    auto floatingPolicy = classifyWriteExecution(
        *analysis, WriteUpdateKind::FloatingAdd);
    auto assignPolicy = classifyWriteExecution(
        *analysis, WriteUpdateKind::Assign);
    if (verify(!floatingPolicy.mayReorderWrites &&
               floatingPolicy.mustPreserveOriginalOrder &&
               floatingPolicy.requiresReductionProof &&
               !assignPolicy.mayReorderWrites &&
               assignPolicy.mustPreserveOriginalOrder &&
               !assignPolicy.requiresReductionProof,
               "colliding writes preserve original order"))
      return 1;

    // C[x0][x1] is injective over source iterations: no collisions.
    auto injective = analyzeOrderedWriteConflicts(
        identity, domain,
        {{BigInt(1), BigInt(0)}, {BigInt(0), BigInt(1)}},
        {BigInt(0), BigInt(0)});
    if (verify(injective.has_value() &&
               injective->feasibleBranches.empty() &&
               injective->emptyBranches == 2,
               "injective output access has no ordered conflicts"))
      return 1;
    auto noConflictPolicy = classifyWriteExecution(
        *injective, WriteUpdateKind::Unknown);
    if (verify(noConflictPolicy.mayReorderWrites &&
               !noConflictPolicy.mustPreserveOriginalOrder &&
               !noConflictPolicy.requiresReductionProof,
               "no write conflicts permits write reordering"))
      return 1;

    // Test exact integer feasibility, not merely rational feasibility:
    // x1 = 2*q and x1 = 1 cannot both hold for integer q.
    IntegerPolyhedron parity(PresburgerSpace::getSetSpace(2, 0, 1));
    parity.addEquality({0, 1, -2, 0});
    parity.addEquality({0, 1, 0, -1});
    auto parityResult = analyzeOrderedWriteConflicts(
        identity, parity, {{BigInt(1), BigInt(0)}}, {BigInt(0)});
    if (verify(parityResult.has_value() &&
               parityResult->feasibleBranches.empty() &&
               parityResult->emptyBranches == 2,
               "integer-infeasible source domain rejected as conflict"))
      return 1;
  }


  // --------------------------------------------------------
  // Phase 5.3.5: conservative operator-aware reduction policy.
  // This is not a proof that generated parallel code is legal.
  // --------------------------------------------------------
  {
    using systolic::logical::IntegerAddSemantics;
    using systolic::logical::ReductionOperatorEvidence;
    using systolic::logical::ReductionOrderDecision;
    using systolic::logical::ReductionWriteUpdateKind;
    using systolic::logical::classifyReductionOrder;

    ReductionOperatorEvidence evidence;
    const auto preserve = ReductionOrderDecision::PreserveSourceOrder;
    const auto candidate = ReductionOrderDecision::ReassociationCandidate;
    const auto noCollision = ReductionOrderDecision::NoOutputWriteCollision;

    if (verify(classifyReductionOrder(false, evidence) == noCollision,
               "no feasible ordered output-write collision"))
      return 1;

    if (verify(classifyReductionOrder(true, evidence) == preserve,
               "unknown update preserves source order"))
      return 1;

    evidence.updateIsRecognizedReduction = true;
    evidence.operandsIndependentOfDestination = true;
    evidence.noOtherObservableEffects = true;
    evidence.update = ReductionWriteUpdateKind::Assign;
    if (verify(classifyReductionOrder(true, evidence) == preserve,
               "overwrite preserves last-writer order"))
      return 1;

    evidence.update = ReductionWriteUpdateKind::IntegerAdd;
    evidence.integerSemantics = IntegerAddSemantics::MayOverflowOrUB;
    if (verify(classifyReductionOrder(true, evidence) == preserve,
               "unproven integer overflow preserves order"))
      return 1;

    evidence.integerSemantics = IntegerAddSemantics::ModularWrap;
    if (verify(classifyReductionOrder(true, evidence) == candidate,
               "proven wrapping integer reduction is candidate only"))
      return 1;

    evidence.operandsIndependentOfDestination = false;
    if (verify(classifyReductionOrder(true, evidence) == preserve,
               "destination-dependent operand forbids reassociation"))
      return 1;
    evidence.operandsIndependentOfDestination = true;

    evidence.update = ReductionWriteUpdateKind::FloatingAdd;
    evidence.floatingReassociationPermitted = false;
    if (verify(classifyReductionOrder(true, evidence) == preserve,
               "strict floating reduction preserves order"))
      return 1;

    evidence.floatingReassociationPermitted = true;
    if (verify(classifyReductionOrder(true, evidence) == candidate,
               "explicit FP reassociation yields candidate only"))
      return 1;

    evidence.noOtherObservableEffects = false;
    if (verify(classifyReductionOrder(true, evidence) == preserve,
               "observable effects forbid reduction reorder"))
      return 1;
  }

  std::cout << "All coordinate relation tests passed.\n";
  return 0;
}
