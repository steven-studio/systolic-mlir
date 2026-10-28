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
#include "Systolic/RankPartitionScheduleLegality.h"
#include "Systolic/RankPartitionLogicalDomain.h"
#include "Systolic/RankPartitionPresburgerSet.h"
#include "Systolic/RankPartitionIterationPlan.h"
#include "Systolic/RankPartitionGuardGeneration.h"
#include "Systolic/RankPartitionGuardEmission.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Verifier.h"
#include "llvm/ADT/DenseMap.h"
#include <iterator>
#include <optional>
#include "Systolic/RankPartitionLoopNestEmission.h"
#include "Systolic/RankPartitionGuardedLoopNestEmission.h"
#include "RankPartitionLoopExecutionTest.h"
#include "Systolic/RankPartitionSourceIVRecoveryPlan.h"
#include "Systolic/RankPartitionSourceIVEmission.h"
#include "Systolic/RankPartitionLogicalDomainNormalization.h"
#include <functional>
#include "Systolic/RankPartitionSourceAccessEmission.h"
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


  // --------------------------------------------------------
  // Phase 5.4.2b: Symbolic schedule legality.
  // --------------------------------------------------------
  {
    using mlir::presburger::IntegerPolyhedron;
    using mlir::presburger::PresburgerSpace;

    // Source order:
    //   x = (b, i, j, k)
    //
    // Logical order:
    //   z = (i, j, k, b)
    //
    // Output:
    //   C[b][b+i][j]
    //
    // Same-output ordered conflicts:
    //
    //   b1=b2, i1=i2, j1=j2, k1<k2

    PartitionCoordinateMap scheduleMap;

    scheduleMap.sourceToLogical = {
        {BigInt(0), BigInt(1), BigInt(0), BigInt(0)},
        {BigInt(0), BigInt(0), BigInt(1), BigInt(0)},
        {BigInt(0), BigInt(0), BigInt(0), BigInt(1)},
        {BigInt(1), BigInt(0), BigInt(0), BigInt(0)}
    };

    scheduleMap.logicalToSource = {
        {Rat(0), Rat(0), Rat(0), Rat(1)},
        {Rat(1), Rat(0), Rat(0), Rat(0)},
        {Rat(0), Rat(1), Rat(0), Rat(0)},
        {Rat(0), Rat(0), Rat(1), Rat(0)}
    };

    IntegerPolyhedron domain(
        PresburgerSpace::getSetSpace(4));

    // [b, i, j, k, constant]
    // 0 <= b < 2
    // 0 <= i,j,k < 4

    for (unsigned d = 0; d < 4; ++d) {
      llvm::SmallVector<int64_t> lower(5, 0);
      llvm::SmallVector<int64_t> upper(5, 0);

      lower[d] = 1;
      upper[d] = -1;
      upper[4] = d == 0 ? 1 : 3;

      domain.addInequality(lower);
      domain.addInequality(upper);
    }

    const std::vector<std::vector<BigInt>> output = {
        {BigInt(1), BigInt(0), BigInt(0), BigInt(0)},
        {BigInt(1), BigInt(1), BigInt(0), BigInt(0)},
        {BigInt(0), BigInt(0), BigInt(1), BigInt(0)}
    };

    auto conflicts = analyzeOrderedWriteConflicts(
        scheduleMap,
        domain,
        output,
        {BigInt(0), BigInt(0), BigInt(0)});

    if (verify(
            conflicts.has_value() &&
            conflicts->feasibleBranches.size() == 1 &&
            conflicts->emptyBranches == 3,
            "schedule test has one feasible dependence branch"))
      return 1;

    // Identity: theta = (b,i,j,k)

    const std::vector<std::vector<BigInt>> identity = {
        {1, 0, 0, 0},
        {0, 1, 0, 0},
        {0, 0, 1, 0},
        {0, 0, 0, 1}
    };

    auto identityResult =
        checkOrderedWriteSchedule(*conflicts, identity, 4);

    if (verify(
            identityResult.has_value() &&
            identityResult->preservesOrderedWrites &&
            identityResult->feasibleViolationBranches == 0,
            "identity schedule preserves dependence order"))
      return 1;

    // Permute independent output dimensions:
    // theta = (i,j,b,k)
    //
    // k remains the final and increasing coordinate.

    const std::vector<std::vector<BigInt>> permuted = {
        {0, 1, 0, 0},
        {0, 0, 1, 0},
        {1, 0, 0, 0},
        {0, 0, 0, 1}
    };

    auto permutedResult =
        checkOrderedWriteSchedule(*conflicts, permuted, 4);

    if (verify(
            permutedResult.has_value() &&
            permutedResult->preservesOrderedWrites,
            "output-dimension permutation preserves write order"))
      return 1;

    // Reverse K:
    // theta = (-k,b,i,j)
    //
    // The same C receives updates in reverse order.

    const std::vector<std::vector<BigInt>> reversed = {
        {0, 0, 0, -1},
        {1, 0, 0, 0},
        {0, 1, 0, 0},
        {0, 0, 1, 0}
    };

    auto reversedResult =
        checkOrderedWriteSchedule(*conflicts, reversed, 4);

    if (verify(
            reversedResult.has_value() &&
            !reversedResult->preservesOrderedWrites &&
            reversedResult->feasibleViolationBranches > 0,
            "reversed reduction schedule rejected"))
      return 1;

    // Remove K:
    // theta = (b,i,j)
    //
    // Different k values become simultaneous timestamps.
    // This is not a valid sequential ordering certificate.

    const std::vector<std::vector<BigInt>> collapsed = {
        {1, 0, 0, 0},
        {0, 1, 0, 0},
        {0, 0, 1, 0}
    };

    auto collapsedResult =
        checkOrderedWriteSchedule(*conflicts, collapsed, 4);

    if (verify(
            collapsedResult.has_value() &&
            !collapsedResult->preservesOrderedWrites &&
            collapsedResult->feasibleViolationBranches > 0,
            "dependent iterations cannot share timestamp"))
      return 1;

    // Malformed schedules must be rejected.

    auto malformedResult =
        checkOrderedWriteSchedule(
            *conflicts,
            {{BigInt(1), BigInt(0)}},
            4);

    if (verify(!malformedResult.has_value(),
               "malformed schedule dimensions rejected"))
      return 1;

    // A coefficient whose negation exceeds int64
    // is rejected before constructing any relation.

    const BigInt minimum(
        std::numeric_limits<int64_t>::min());

    auto overflowResult =
        checkOrderedWriteSchedule(
            *conflicts,
            {{minimum, BigInt(0), BigInt(0), BigInt(0)}},
            4);

    if (verify(!overflowResult.has_value(),
               "unrepresentable schedule coefficient rejected"))
      return 1;

    // No ordered output-write dependences:
    // any well-formed schedule passes this particular check.
    //
    // This does NOT prove full lowering legality.

    OrderedWriteConflictAnalysis noConflicts;

    auto vacuousResult =
        checkOrderedWriteSchedule(noConflicts, identity, 4);

    if (verify(
            vacuousResult.has_value() &&
            vacuousResult->preservesOrderedWrites &&
            vacuousResult->testedDependenceBranches == 0,
            "empty write-dependence set accepted"))
      return 1;
  }


  // --------------------------------------------------------
  // Phase 5.4.3a: Exact Logical Domain Projection.
  // --------------------------------------------------------
  {
    using mlir::presburger::IntegerPolyhedron;
    using mlir::presburger::PresburgerSpace;

    auto contains = [](
        const IntegerPolyhedron &set,
        std::initializer_list<int64_t> coordinates) {

      llvm::SmallVector<llvm::DynamicAPInt> point;

      for (int64_t value : coordinates)
        point.push_back(llvm::DynamicAPInt(value));

      return set.containsPointNoLocal(point).has_value();
    };

    // ======================================================
    // T1: Triangle domain under coordinate permutation.
    //
    // D = { (i,j) | i>=0, j>=0, i+j<=5 }
    //
    // T(i,j) = (j,i)
    // ======================================================

    PartitionCoordinateMap swapMap;

    swapMap.sourceToLogical = {
        {BigInt(0), BigInt(1)},
        {BigInt(1), BigInt(0)}
    };

    swapMap.logicalToSource = {
        {Rat(0), Rat(1)},
        {Rat(1), Rat(0)}
    };

    IntegerPolyhedron triangle(
        PresburgerSpace::getSetSpace(2));

    triangle.addInequality(
        llvm::SmallVector<int64_t>{1, 0, 0});

    triangle.addInequality(
        llvm::SmallVector<int64_t>{0, 1, 0});

    triangle.addInequality(
        llvm::SmallVector<int64_t>{-1, -1, 5});

    auto projectedTriangle =
        buildLogicalDomain(swapMap, triangle);

    if (verify(
            projectedTriangle.has_value() &&
            contains(*projectedTriangle, {0, 0}) &&
            contains(*projectedTriangle, {2, 3}) &&
            contains(*projectedTriangle, {5, 0}) &&
            !contains(*projectedTriangle, {3, 3}) &&
            !contains(*projectedTriangle, {-1, 0}),
            "triangle logical domain projection"))
      return 1;

    // ======================================================
    // T2: Corner-cut triangle.
    //
    // D = {
    //   i>=0, j>=0,
    //   2<=i+j<=5
    // }
    //
    // This remains a convex integer polyhedron.
    // ======================================================

    IntegerPolyhedron cornerCut = triangle;

    cornerCut.addInequality(
        llvm::SmallVector<int64_t>{1, 1, -2});

    auto projectedCornerCut =
        buildLogicalDomain(swapMap, cornerCut);

    if (verify(
            projectedCornerCut.has_value() &&
            !contains(*projectedCornerCut, {0, 0}) &&
            !contains(*projectedCornerCut, {1, 0}) &&
            contains(*projectedCornerCut, {1, 1}) &&
            contains(*projectedCornerCut, {2, 3}) &&
            !contains(*projectedCornerCut, {3, 3}),
            "corner-cut triangle projection"))
      return 1;

    // ======================================================
    // T3: Non-unimodular transformation.
    //
    // z0=2*i
    // z1=j
    //
    // Legal z0 must be even.
    // ======================================================

    PartitionCoordinateMap scaleMap;

    scaleMap.sourceToLogical = {
        {BigInt(2), BigInt(0)},
        {BigInt(0), BigInt(1)}
    };

    scaleMap.logicalToSource = {
        {Rat(1, 2), Rat(0)},
        {Rat(0), Rat(1)}
    };

    auto projectedScaled =
        buildLogicalDomain(scaleMap, triangle);

    if (verify(
            projectedScaled.has_value() &&
            contains(*projectedScaled, {0, 0}) &&
            contains(*projectedScaled, {2, 3}) &&
            contains(*projectedScaled, {10, 0}) &&
            !contains(*projectedScaled, {1, 0}) &&
            !contains(*projectedScaled, {3, 0}) &&
            !contains(*projectedScaled, {12, 0}),
            "non-unimodular integer lattice preserved"))
      return 1;

    // ======================================================
    // T4: Empty integer source domain.
    //
    // i>=1 and i<=0.
    // ======================================================

    IntegerPolyhedron emptySource(
        PresburgerSpace::getSetSpace(2));

    emptySource.addInequality(
        llvm::SmallVector<int64_t>{1, 0, -1});

    emptySource.addInequality(
        llvm::SmallVector<int64_t>{-1, 0, 0});

    auto projectedEmpty =
        buildLogicalDomain(swapMap, emptySource);

    if (verify(
            projectedEmpty.has_value() &&
            projectedEmpty->isIntegerEmpty(),
            "empty integer domain remains empty"))
      return 1;

    // ======================================================
    // T5: Existential source local variable.
    //
    // i = 2*q, where q is integer.
    //
    // Under coordinate swap:
    //
    // z1 must remain even.
    // ======================================================

    IntegerPolyhedron localSource(
        PresburgerSpace::getSetSpace(2, 0, 1));

    // [i, j, q, constant]
    localSource.addEquality(
        llvm::SmallVector<int64_t>{1, 0, -2, 0});

    localSource.addInequality(
        llvm::SmallVector<int64_t>{1, 0, 0, 0});

    localSource.addInequality(
        llvm::SmallVector<int64_t>{-1, 0, 0, 4});

    localSource.addInequality(
        llvm::SmallVector<int64_t>{0, 1, 0, 0});

    localSource.addInequality(
        llvm::SmallVector<int64_t>{0, -1, 0, 4});

    auto projectedLocal =
        buildLogicalDomain(swapMap, localSource);

    if (verify(
            projectedLocal.has_value() &&
            contains(*projectedLocal, {1, 0}) &&
            contains(*projectedLocal, {1, 2}) &&
            contains(*projectedLocal, {1, 4}) &&
            !contains(*projectedLocal, {1, 1}) &&
            !contains(*projectedLocal, {1, 3}),
            "existential source locals preserve congruence"))
      return 1;

    // ======================================================
    // T6: Source dimension mismatch.
    // ======================================================

    IntegerPolyhedron wrongDimension(
        PresburgerSpace::getSetSpace(1));

    if (verify(
            !buildLogicalDomain(swapMap, wrongDimension),
            "logical projection rejects dimension mismatch"))
      return 1;

    // ======================================================
    // T7: Symbols unsupported by current source relation.
    //
    // Must reject instead of silently dropping symbols.
    // ======================================================

    IntegerPolyhedron symbolicSource(
        PresburgerSpace::getSetSpace(2, 1));

    if (verify(
            !buildLogicalDomain(swapMap, symbolicSource),
            "logical projection rejects unsupported symbols"))
      return 1;
  }


  // --------------------------------------------------------
  // Phase 5.4.3b:
  // Non-convex Presburger union and difference.
  // --------------------------------------------------------
  {
    using mlir::presburger::IntegerPolyhedron;
    using mlir::presburger::PresburgerSet;
    using mlir::presburger::PresburgerSpace;

    // Identity transformation:
    //
    // x=(i,j), z=(i,j)

    PartitionCoordinateMap identityMap;

    identityMap.sourceToLogical = {
        {BigInt(1), BigInt(0)},
        {BigInt(0), BigInt(1)}
    };

    identityMap.logicalToSource = {
        {Rat(1), Rat(0)},
        {Rat(0), Rat(1)}
    };

    // Outer triangle:
    //
    // i >= 0
    // j >= 0
    // i+j <= 7
    //
    // 36 integer points.

    IntegerPolyhedron outer(
        PresburgerSpace::getSetSpace(2));

    outer.addInequality(
        llvm::SmallVector<int64_t>{1, 0, 0});

    outer.addInequality(
        llvm::SmallVector<int64_t>{0, 1, 0});

    outer.addInequality(
        llvm::SmallVector<int64_t>{-1, -1, 7});

    // Hole:
    //
    // i>=2, j>=2, i+j<=5
    //
    // Three points: (2,2), (2,3), (3,2).

    IntegerPolyhedron hole(
        PresburgerSpace::getSetSpace(2));

    hole.addInequality(
        llvm::SmallVector<int64_t>{1, 0, -2});

    hole.addInequality(
        llvm::SmallVector<int64_t>{0, 1, -2});

    hole.addInequality(
        llvm::SmallVector<int64_t>{-1, -1, 5});

    PresburgerSet outerSet(outer);
    PresburgerSet holeSet(hole);

    // ======================================================
    // T1: Exact difference.
    // ======================================================

    auto cutSource =
        buildSourceDomainDifference(outerSet, holeSet);

    if (verify(
            cutSource.has_value() &&
            cutSource->containsPoint(
                llvm::SmallVector<int64_t>{0, 0}) &&
            cutSource->containsPoint(
                llvm::SmallVector<int64_t>{4, 2}) &&
            !cutSource->containsPoint(
                llvm::SmallVector<int64_t>{2, 2}) &&
            !cutSource->containsPoint(
                llvm::SmallVector<int64_t>{2, 3}) &&
            !cutSource->containsPoint(
                llvm::SmallVector<int64_t>{3, 2}) &&
            !cutSource->containsPoint(
                llvm::SmallVector<int64_t>{7, 7}),
            "triangle with hole exact difference"))
      return 1;

    // ======================================================
    // T2: Exact set identity.
    //
    // (outer \ hole) intersect hole = empty
    // ======================================================

    if (verify(
            cutSource->intersect(holeSet).isIntegerEmpty(),
            "removed hole has empty intersection"))
      return 1;

    // ======================================================
    // T3: Exact set reconstruction.
    //
    // (outer \ hole) union hole = outer
    //
    // In this example hole is contained in outer.
    // ======================================================

    auto reconstructed =
        buildSourceDomainUnion(*cutSource, holeSet);

    if (verify(
            reconstructed.has_value() &&
            reconstructed->isEqual(outerSet),
            "difference plus hole reconstructs outer set"))
      return 1;

    // ======================================================
    // T4: Logical image of non-convex source set.
    // ======================================================

    auto projected =
        buildLogicalDomain(identityMap, *cutSource);

    if (verify(
            projected.has_value() &&
            projected->containsPoint(
                llvm::SmallVector<int64_t>{0, 0}) &&
            projected->containsPoint(
                llvm::SmallVector<int64_t>{4, 2}) &&
            !projected->containsPoint(
                llvm::SmallVector<int64_t>{2, 2}) &&
            !projected->containsPoint(
                llvm::SmallVector<int64_t>{2, 3}) &&
            !projected->containsPoint(
                llvm::SmallVector<int64_t>{3, 2}),
            "non-convex logical-domain projection"))
      return 1;

    // ======================================================
    // T5: Set difference against empty domain.
    //
    // D \ empty = D
    // ======================================================

    PresburgerSet empty =
        PresburgerSet::getEmpty(
            PresburgerSpace::getSetSpace(2));

    auto unchanged =
        buildSourceDomainDifference(outerSet, empty);

    if (verify(
            unchanged.has_value() &&
            unchanged->isEqual(outerSet),
            "difference against empty set"))
      return 1;

    // ======================================================
    // T6: Union of overlapping regions.
    //
    // The set must not gain extra points.
    // Note: execution multiplicity is a later phase.
    // ======================================================

    auto overlappingUnion =
        buildSourceDomainUnion(outerSet, outerSet);

    if (verify(
            overlappingUnion.has_value() &&
            overlappingUnion->isEqual(outerSet),
            "overlapping union preserves set membership"))
      return 1;

    // ======================================================
    // T7: Non-unimodular image of a non-convex set.
    //
    // z0=2*i
    // z1=j
    //
    // Odd z0 must remain excluded.
    // ======================================================

    PartitionCoordinateMap scaledMap;

    scaledMap.sourceToLogical = {
        {BigInt(2), BigInt(0)},
        {BigInt(0), BigInt(1)}
    };

    scaledMap.logicalToSource = {
        {Rat(1, 2), Rat(0)},
        {Rat(0), Rat(1)}
    };

    auto scaled =
        buildLogicalDomain(scaledMap, *cutSource);

    if (verify(
            scaled.has_value() &&
            scaled->containsPoint(
                llvm::SmallVector<int64_t>{0, 0}) &&
            scaled->containsPoint(
                llvm::SmallVector<int64_t>{8, 2}) &&
            !scaled->containsPoint(
                llvm::SmallVector<int64_t>{4, 2}) &&
            !scaled->containsPoint(
                llvm::SmallVector<int64_t>{5, 2}),
            "non-convex non-unimodular projection"))
      return 1;

    // ======================================================
    // T8: Reject incompatible source dimensions.
    // ======================================================

    PresburgerSet wrongDimension(
        IntegerPolyhedron(
            PresburgerSpace::getSetSpace(1)));

    if (verify(
            !buildSourceDomainDifference(
                outerSet, wrongDimension) &&
            !buildSourceDomainUnion(
                outerSet, wrongDimension),
            "non-convex operations reject dimension mismatch"))
      return 1;

    // ======================================================
    // T9: Reject unresolved symbolic parameters.
    // ======================================================

    PresburgerSet symbolic(
        IntegerPolyhedron(
            PresburgerSpace::getSetSpace(2, 1)));

    if (verify(
            !buildSourceDomainDifference(
                outerSet, symbolic) &&
            !buildLogicalDomain(
                identityMap, symbolic),
            "non-convex operations reject unsupported symbols"))
      return 1;

    // ======================================================
    // T10: Reject unsupported existential locals
    // in the subtracted set.
    // ======================================================

    IntegerPolyhedron localHole(
        PresburgerSpace::getSetSpace(2, 0, 1));

    localHole.addEquality(
        llvm::SmallVector<int64_t>{1, 0, -2, 0});

    PresburgerSet localHoleSet(localHole);

    if (verify(
            !buildSourceDomainDifference(
                outerSet, localHoleSet),
            "difference rejects unsupported RHS locals"))
      return 1;

    // ======================================================
    // T11: Empty-set logical projection.
    // ======================================================

    auto emptyProjected =
        buildLogicalDomain(identityMap, empty);

    if (verify(
            emptyProjected.has_value() &&
            emptyProjected->isIntegerEmpty(),
            "empty PresburgerSet projects to empty set"))
      return 1;
  }


  // --------------------------------------------------------
  // Phase 5.4.3c:
  // Exactly-once symbolic iteration plan.
  // --------------------------------------------------------
  {
    using mlir::presburger::IntegerPolyhedron;
    using mlir::presburger::PresburgerSet;
    using mlir::presburger::PresburgerSpace;

    // Outer triangle:
    //
    // i>=0, j>=0, i+j<=7

    IntegerPolyhedron outer(
        PresburgerSpace::getSetSpace(2));

    outer.addInequality(
        llvm::SmallVector<int64_t>{1, 0, 0});

    outer.addInequality(
        llvm::SmallVector<int64_t>{0, 1, 0});

    outer.addInequality(
        llvm::SmallVector<int64_t>{-1, -1, 7});

    // Hole:
    //
    // i>=2, j>=2, i+j<=5

    IntegerPolyhedron hole(
        PresburgerSpace::getSetSpace(2));

    hole.addInequality(
        llvm::SmallVector<int64_t>{1, 0, -2});

    hole.addInequality(
        llvm::SmallVector<int64_t>{0, 1, -2});

    hole.addInequality(
        llvm::SmallVector<int64_t>{-1, -1, 5});

    PresburgerSet outerSet(outer);
    PresburgerSet holeSet(hole);

    auto cut = buildSourceDomainDifference(
        outerSet, holeSet);

    if (!cut)
      return 1;

    // ======================================================
    // T1: Triangle fits inside [0,7] x [0,7].
    // ======================================================

    auto trianglePlan = buildSymbolicIterationPlan(
        outerSet, {{0, 7}, {0, 7}});

    if (verify(
            trianglePlan.has_value() &&
            trianglePlan->bounds.size() == 2,
            "triangle symbolic iteration plan"))
      return 1;

    // ======================================================
    // T2: Triangle with hole fits in the same box.
    // ======================================================

    auto holePlan = buildSymbolicIterationPlan(
        *cut, {{0, 7}, {0, 7}});

    if (verify(
            holePlan.has_value() &&
            holePlan->logicalDomain.isEqual(*cut),
            "non-convex iteration plan preserves exact set"))
      return 1;

    // ======================================================
    // T3: Too-small bounds must fail.
    //
    // The source set contains (7,0).
    // ======================================================

    auto incomplete = buildSymbolicIterationPlan(
        outerSet, {{0, 6}, {0, 7}});

    if (verify(
            !incomplete.has_value(),
            "incomplete iteration bounds rejected"))
      return 1;

    // ======================================================
    // T4: Exact bounds are allowed.
    // ======================================================

    auto exactBounds = buildSymbolicIterationPlan(
        outerSet, {{0, 7}, {0, 7}});

    if (verify(
            exactBounds.has_value(),
            "complete finite iteration bounds accepted"))
      return 1;

    // ======================================================
    // T5: A box larger than the set is permitted.
    //
    // Extra candidate points are filtered by the
    // exact logical-domain guard.
    // ======================================================

    auto larger = buildSymbolicIterationPlan(
        *cut, {{-3, 10}, {-3, 10}});

    if (verify(
            larger.has_value() &&
            larger->logicalDomain.isEqual(*cut),
            "larger box preserves exact guarded domain"))
      return 1;

    // ======================================================
    // T6: Overlapping union is still one set.
    //
    // A single candidate box plus membership predicate
    // does not execute a coordinate once per disjunct.
    // ======================================================

    auto overlap = buildSourceDomainUnion(
        outerSet, outerSet);

    auto overlapPlan = overlap
        ? buildSymbolicIterationPlan(
              *overlap, {{0, 7}, {0, 7}})
        : std::nullopt;

    if (verify(
            overlapPlan.has_value() &&
            overlapPlan->logicalDomain.isEqual(outerSet),
            "overlapping union produces one guarded plan"))
      return 1;

    // ======================================================
    // T7: Non-unimodular logical lattice.
    //
    // z0=2*i, z1=j.
    //
    // Preserve even-coordinate constraints.
    // ======================================================

    PartitionCoordinateMap scaleMap;

    scaleMap.sourceToLogical = {
        {BigInt(2), BigInt(0)},
        {BigInt(0), BigInt(1)}
    };

    scaleMap.logicalToSource = {
        {Rat(1, 2), Rat(0)},
        {Rat(0), Rat(1)}
    };

    auto scaledDomain =
        buildLogicalDomain(scaleMap, *cut);

    auto scaledPlan = scaledDomain
        ? buildSymbolicIterationPlan(
              *scaledDomain, {{0, 14}, {0, 7}})
        : std::nullopt;

    if (verify(
            scaledPlan.has_value() &&
            !scaledPlan->logicalDomain.containsPoint(
                llvm::SmallVector<int64_t>{1, 0}) &&
            !scaledPlan->logicalDomain.containsPoint(
                llvm::SmallVector<int64_t>{4, 2}) &&
            scaledPlan->logicalDomain.containsPoint(
                llvm::SmallVector<int64_t>{8, 2}),
            "non-unimodular lattice iteration plan"))
      return 1;

    // ======================================================
    // T8: Invalid intervals are rejected.
    // ======================================================

    auto invalidInterval = buildSymbolicIterationPlan(
        outerSet, {{7, 0}, {0, 7}});

    if (verify(
            !invalidInterval.has_value(),
            "invalid iteration interval rejected"))
      return 1;

    // ======================================================
    // T9: Dimension mismatch is rejected.
    // ======================================================

    auto invalidDimension = buildSymbolicIterationPlan(
        outerSet, {{0, 7}});

    if (verify(
            !invalidDimension.has_value(),
            "iteration plan dimension mismatch rejected"))
      return 1;

    // ======================================================
    // T10: Empty domain.
    //
    // Any valid finite box covers the empty set.
    // ======================================================

    PresburgerSet empty =
        PresburgerSet::getEmpty(
            PresburgerSpace::getSetSpace(2));

    auto emptyPlan = buildSymbolicIterationPlan(
        empty, {{0, 0}, {0, 0}});

    if (verify(
            emptyPlan.has_value() &&
            emptyPlan->logicalDomain.isIntegerEmpty(),
            "empty logical domain iteration plan"))
      return 1;
  }


  // --------------------------------------------------------
  // Phase 5.4.3d-1a:
  // Exact guard representation and arithmetic safety.
  // --------------------------------------------------------
  {
    using mlir::presburger::IntegerPolyhedron;
    using mlir::presburger::PresburgerSet;
    using mlir::presburger::PresburgerSpace;

    // Triangle:
    //
    // i>=0, j>=0, i+j<=7.

    IntegerPolyhedron triangle(
        PresburgerSpace::getSetSpace(2));

    triangle.addInequality(
        llvm::SmallVector<int64_t>{1, 0, 0});

    triangle.addInequality(
        llvm::SmallVector<int64_t>{0, 1, 0});

    triangle.addInequality(
        llvm::SmallVector<int64_t>{-1, -1, 7});

    PresburgerSet triangleSet(triangle);

    auto trianglePlan = buildSymbolicIterationPlan(
        triangleSet, {{0, 7}, {0, 7}});

    if (!trianglePlan)
      return 1;

    // T1: Triangle guard representation.

    auto triangleGuard =
        buildExactGuardPlan(*trianglePlan);

    if (verify(
            triangleGuard.has_value() &&
            triangleGuard->dimension == 2 &&
            triangleGuard->disjuncts.size() == 1 &&
            triangleGuard->disjuncts[0].constraints.size() == 3,
            "triangle exact guard representation"))
      return 1;

    // T2: Empty domain gives FALSE.

    PresburgerSet empty =
        PresburgerSet::getEmpty(
            PresburgerSpace::getSetSpace(2));

    auto emptyPlan = buildSymbolicIterationPlan(
        empty, {{0, 7}, {0, 7}});

    auto emptyGuard = emptyPlan
        ? buildExactGuardPlan(*emptyPlan)
        : std::nullopt;

    if (verify(
            emptyGuard.has_value() &&
            emptyGuard->disjuncts.empty(),
            "empty domain produces false guard"))
      return 1;

    // T3: Unconstrained disjunct gives TRUE.
    //
    // The membership predicate is true everywhere
    // within the candidate iteration box.

    IntegerPolyhedron universe(
        PresburgerSpace::getSetSpace(2));

    PresburgerSet universeSet(universe);

    // The unbounded universe cannot fit into a
    // finite iteration box. Therefore the plan
    // builder must reject it.

    auto unboundedPlan = buildSymbolicIterationPlan(
        universeSet, {{0, 7}, {0, 7}});

    if (verify(
            !unboundedPlan.has_value(),
            "unbounded universe rejects finite plan"))
      return 1;

    // T4: Overlapping union remains OR, not
    // multiple independent execution bodies.

    auto overlapping = buildSourceDomainUnion(
        triangleSet, triangleSet);

    auto overlappingPlan = overlapping
        ? buildSymbolicIterationPlan(
              *overlapping, {{0, 7}, {0, 7}})
        : std::nullopt;

    auto overlappingGuard = overlappingPlan
        ? buildExactGuardPlan(*overlappingPlan)
        : std::nullopt;

    if (verify(
            overlappingGuard.has_value() &&
            overlappingGuard->disjuncts.size() >= 1,
            "overlapping union guard representation"))
      return 1;

    // T5: Triangle with a hole.

    IntegerPolyhedron hole(
        PresburgerSpace::getSetSpace(2));

    hole.addInequality(
        llvm::SmallVector<int64_t>{1, 0, -2});

    hole.addInequality(
        llvm::SmallVector<int64_t>{0, 1, -2});

    hole.addInequality(
        llvm::SmallVector<int64_t>{-1, -1, 5});

    auto cut = buildSourceDomainDifference(
        triangleSet, PresburgerSet(hole));

    auto cutPlan = cut
        ? buildSymbolicIterationPlan(
              *cut, {{0, 7}, {0, 7}})
        : std::nullopt;

    auto cutGuard = cutPlan
        ? buildExactGuardPlan(*cutPlan)
        : std::nullopt;

    if (verify(
            cutGuard.has_value() &&
            !cutGuard->disjuncts.empty(),
            "triangle with hole exact guard representation"))
      return 1;

    // T6: Equality constraint.

    IntegerPolyhedron equality(
        PresburgerSpace::getSetSpace(2));

    equality.addEquality(
        llvm::SmallVector<int64_t>{1, -1, 0});

    equality.addInequality(
        llvm::SmallVector<int64_t>{1, 0, 0});

    equality.addInequality(
        llvm::SmallVector<int64_t>{-1, 0, 7});

    PresburgerSet equalitySet(equality);

    auto equalityPlan = buildSymbolicIterationPlan(
        equalitySet, {{0, 7}, {0, 7}});

    auto equalityGuard = equalityPlan
        ? buildExactGuardPlan(*equalityPlan)
        : std::nullopt;

    if (verify(
            equalityGuard.has_value() &&
            equalityGuard->disjuncts.size() == 1 &&
            !equalityGuard->disjuncts[0].constraints.empty() &&
            equalityGuard->disjuncts[0].constraints[0].kind ==
                GuardConstraintKind::Equality,
            "equality guard representation"))
      return 1;

    // T7: A single equality-defined local is an exact congruence.

    IntegerPolyhedron parity(
        PresburgerSpace::getSetSpace(2, 0, 1));

    parity.addEquality(
        llvm::SmallVector<int64_t>{1, 0, -2, 0});

    parity.addInequality(
        llvm::SmallVector<int64_t>{1, 0, 0, 0});

    parity.addInequality(
        llvm::SmallVector<int64_t>{-1, 0, 0, 6});

    parity.addInequality(
        llvm::SmallVector<int64_t>{0, 1, 0, 0});

    parity.addInequality(
        llvm::SmallVector<int64_t>{0, -1, 0, 7});

    auto parityPlan = buildSymbolicIterationPlan(
        PresburgerSet(parity),
        {{0, 6}, {0, 7}});

    // i - 2*q == 0 with 0 <= i <= 6 and 0 <= j <= 7.
    // Existential q is equivalent to i divisible by 2.
    auto parityGuard = parityPlan
        ? buildExactGuardPlan(*parityPlan)
        : std::nullopt;
    if (verify(
            parityGuard.has_value() &&
            parityGuard->dimension == 2 &&
            parityGuard->disjuncts.size() == 1 &&
            parityGuard->disjuncts[0].constraints.size() == 5 &&
            parityGuard->disjuncts[0].constraints[0].kind ==
                GuardConstraintKind::Congruence &&
            parityGuard->disjuncts[0].constraints[0].modulus == 2 &&
            parityGuard->disjuncts[0].constraints[0].coefficients ==
                std::vector<int64_t>{1, 0} &&
            parityGuard->disjuncts[0].constraints[0].constant == 0,
            "existential parity local becomes exact congruence"))
      return 1;

    // T8: Reject multiplication overflow risk.

    IntegerPolyhedron overflow(
        PresburgerSpace::getSetSpace(1));

    const int64_t maximum =
        std::numeric_limits<int64_t>::max();

    overflow.addInequality(
        llvm::SmallVector<int64_t>{maximum, 0});

    // z>=0 and z<=2.
    overflow.addInequality(
        llvm::SmallVector<int64_t>{1, 0});

    overflow.addInequality(
        llvm::SmallVector<int64_t>{-1, 2});

    PresburgerSet overflowSet(overflow);

    auto overflowPlan = buildSymbolicIterationPlan(
        overflowSet, {{0, 2}});

    if (verify(
            overflowPlan.has_value() &&
            !buildExactGuardPlan(*overflowPlan),
            "guard multiplication overflow rejected"))
      return 1;

    // T9: Reject addition overflow risk.

    IntegerPolyhedron additionOverflow(
        PresburgerSpace::getSetSpace(1));

    additionOverflow.addInequality(
        llvm::SmallVector<int64_t>{1, maximum});

    additionOverflow.addInequality(
        llvm::SmallVector<int64_t>{1, 0});

    additionOverflow.addInequality(
        llvm::SmallVector<int64_t>{-1, 1});

    auto additionPlan = buildSymbolicIterationPlan(
        PresburgerSet(additionOverflow),
        {{0, 1}});

    if (verify(
            additionPlan.has_value() &&
            !buildExactGuardPlan(*additionPlan),
            "guard addition overflow rejected"))
      return 1;

    // T10: Reject unchecked manually constructed
    // iteration plans whose box omits valid points.

    SymbolicIterationPlan unchecked(
        triangleSet, {{0, 1}, {0, 1}});

    if (verify(
            !buildExactGuardPlan(unchecked),
            "unchecked guard plan rejected"))
      return 1;

    // T11: Reject malformed guard dimensions.

    SymbolicIterationPlan wrongDimension(
        triangleSet, {{0, 7}});

    if (verify(
            !buildExactGuardPlan(wrongDimension),
            "guard dimension mismatch rejected"))
      return 1;
  }


  // --------------------------------------------------------
  // Phase 5.4.3d-1b:
  // Emit actual MLIR SSA predicates.
  // --------------------------------------------------------
  {
    using mlir::presburger::IntegerPolyhedron;
    using mlir::presburger::PresburgerSet;
    using mlir::presburger::PresburgerSpace;

    mlir::MLIRContext context;
    context.loadDialect<
        mlir::arith::ArithDialect,
        mlir::func::FuncDialect>();

    mlir::OpBuilder builder(&context);

    auto location = builder.getUnknownLoc();

    // A bounded triangle:
    //
    // i >= 0
    // j >= 0
    // i + j <= 7

    IntegerPolyhedron triangle(
        PresburgerSpace::getSetSpace(2));

    triangle.addInequality(
        llvm::SmallVector<int64_t>{1, 0, 0});

    triangle.addInequality(
        llvm::SmallVector<int64_t>{0, 1, 0});

    triangle.addInequality(
        llvm::SmallVector<int64_t>{-1, -1, 7});

    PresburgerSet triangleSet(triangle);

    auto iterationPlan = buildSymbolicIterationPlan(
        triangleSet, {{0, 7}, {0, 7}});

    auto guardPlan = iterationPlan
        ? buildExactGuardPlan(*iterationPlan)
        : std::nullopt;

    if (verify(
            guardPlan.has_value(),
            "MLIR guard input plan constructed"))
      return 1;

    // Build:
    //
    // func.func @generated_triangle_guard(
    //     %i: i64, %j: i64) -> i1 {
    //   ...
    //   return %guard : i1
    // }

    auto module = mlir::ModuleOp::create(location);

    builder.setInsertionPointToStart(module.getBody());

    auto functionType = builder.getFunctionType(
        {builder.getI64Type(), builder.getI64Type()},
        {builder.getI1Type()});

    auto function = mlir::func::FuncOp::create(
        builder,
        location,
        "generated_triangle_guard",
        functionType);

    mlir::Block *entry = function.addEntryBlock();

    builder.setInsertionPointToStart(entry);

    auto guard = emitExactGuard(
        builder,
        location,
        *iterationPlan,
        entry->getArguments());

    if (verify(
            guard.has_value() &&
            (*guard).getType() == builder.getI1Type(),
            "MLIR triangle guard emitted as i1"))
      return 1;

    mlir::func::ReturnOp::create(
        builder, location, mlir::ValueRange{*guard});

    if (verify(
            mlir::succeeded(mlir::verify(module)),
            "generated triangle guard passes MLIR verifier"))
      return 1;

    // An empty PresburgerSet must emit FALSE.

    PresburgerSet empty =
        PresburgerSet::getEmpty(
            PresburgerSpace::getSetSpace(2));

    auto emptyIterationPlan = buildSymbolicIterationPlan(
        empty, {{0, 7}, {0, 7}});

    auto emptyGuardPlan = emptyIterationPlan
        ? buildExactGuardPlan(*emptyIterationPlan)
        : std::nullopt;

    if (!emptyGuardPlan)
      return 1;

    builder.setInsertionPointToStart(module.getBody());

    auto emptyFunction = mlir::func::FuncOp::create(
        builder,
        location,
        "generated_empty_guard",
        functionType);

    mlir::Block *emptyEntry = emptyFunction.addEntryBlock();

    builder.setInsertionPointToStart(emptyEntry);

    auto emptyGuard = emitExactGuard(
        builder,
        location,
        *emptyIterationPlan,
        emptyEntry->getArguments());

    if (verify(
            emptyGuard.has_value(),
            "empty PresburgerSet guard emitted"))
      return 1;

    mlir::func::ReturnOp::create(
        builder, location, mlir::ValueRange{*emptyGuard});

    if (verify(
            mlir::succeeded(mlir::verify(module)),
            "generated empty guard passes MLIR verifier"))
      return 1;

    // Reject mismatched coordinate types without
    // generating new operations.

    builder.setInsertionPoint(
        emptyEntry->getTerminator());

    auto badCoordinate = mlir::arith::ConstantIndexOp::create(
        builder, location, 0);

    auto badGuard = emitExactGuard(
        builder,
        location,
        *iterationPlan,
        mlir::ValueRange{badCoordinate, emptyEntry->getArgument(1)});

    if (verify(
            !badGuard.has_value(),
            "non-i64 guard coordinates rejected"))
      return 1;

    // Reject malformed coordinate counts.

    auto badRank = emitExactGuard(
        builder,
        location,
        *iterationPlan,
        mlir::ValueRange{emptyEntry->getArgument(0)});

    if (verify(
            !badRank.has_value(),
            "guard emission dimension mismatch rejected"))
      return 1;

    if (verify(
            mlir::succeeded(mlir::verify(module)),
            "guard rejection leaves valid MLIR"))
      return 1;
  }


  // Phase 5.4.3d-1c: interpret *emitted* MLIR SSA, not the source constraints.
  // Exhaustive finite-box enumeration is TEST-ONLY; production is symbolic.
  {
    using namespace mlir;
    using namespace mlir::presburger;
    MLIRContext context;
    context.loadDialect<arith::ArithDialect, func::FuncDialect>();
    OpBuilder builder(&context);
    Location loc = builder.getUnknownLoc();

    auto evaluate = [&](func::FuncOp function, int64_t x,
                        int64_t y) -> std::optional<bool> {
      Block &block = function.front();
      if (block.getNumArguments() != 2)
        return std::nullopt;
      llvm::DenseMap<Value, int64_t> values;
      values[block.getArgument(0)] = x;
      values[block.getArgument(1)] = y;
      auto get = [&](Value value) -> std::optional<int64_t> {
        auto it = values.find(value);
        if (it == values.end())
          return std::nullopt;
        return it->second;
      };
      for (Operation &operation : block) {
        if (auto constant = dyn_cast<arith::ConstantOp>(&operation)) {
          auto attribute = dyn_cast<IntegerAttr>(constant.getValue());
          if (!attribute)
            return std::nullopt;
          const llvm::APInt &integer = attribute.getValue();
          if (integer.getBitWidth() > 64)
            return std::nullopt;
          values[constant.getResult()] = integer.getBitWidth() == 1
              ? static_cast<int64_t>(integer.getZExtValue())
              : integer.getSExtValue();
          continue;
        }
        if (auto multiply = dyn_cast<arith::MulIOp>(&operation)) {
          auto a = get(multiply.getOperand(0));
          auto b = get(multiply.getOperand(1));
          if (!a || !b)
            return std::nullopt;
          __int128 result = static_cast<__int128>(*a) * *b;
          if (result < std::numeric_limits<int64_t>::min() ||
              result > std::numeric_limits<int64_t>::max())
            return std::nullopt;
          values[multiply.getResult()] = static_cast<int64_t>(result);
          continue;
        }
        if (auto addition = dyn_cast<arith::AddIOp>(&operation)) {
          auto a = get(addition.getOperand(0));
          auto b = get(addition.getOperand(1));
          if (!a || !b)
            return std::nullopt;
          __int128 result = static_cast<__int128>(*a) + *b;
          if (result < std::numeric_limits<int64_t>::min() ||
              result > std::numeric_limits<int64_t>::max())
            return std::nullopt;
          values[addition.getResult()] = static_cast<int64_t>(result);
          continue;
        }
        if (auto comparison = dyn_cast<arith::CmpIOp>(&operation)) {
          auto a = get(comparison.getOperand(0));
          auto b = get(comparison.getOperand(1));
          if (!a || !b)
            return std::nullopt;
          bool result;
          switch (comparison.getPredicate()) {
          case arith::CmpIPredicate::eq:  result = *a == *b; break;
          case arith::CmpIPredicate::sge: result = *a >= *b; break;
          default: return std::nullopt;
          }
          values[comparison.getResult()] = result ? 1 : 0;
          continue;
        }
        if (auto logicalAnd = dyn_cast<arith::AndIOp>(&operation)) {
          auto a = get(logicalAnd.getOperand(0));
          auto b = get(logicalAnd.getOperand(1));
          if (!a || !b)
            return std::nullopt;
          values[logicalAnd.getResult()] = (*a != 0 && *b != 0) ? 1 : 0;
          continue;
        }
        if (auto logicalOr = dyn_cast<arith::OrIOp>(&operation)) {
          auto a = get(logicalOr.getOperand(0));
          auto b = get(logicalOr.getOperand(1));
          if (!a || !b)
            return std::nullopt;
          values[logicalOr.getResult()] = (*a != 0 || *b != 0) ? 1 : 0;
          continue;
        }
        if (auto ret = dyn_cast<func::ReturnOp>(&operation)) {
          if (ret.getNumOperands() != 1 ||
              !ret.getOperand(0).getType().isInteger(1))
            return std::nullopt;
          auto answer = get(ret.getOperand(0));
          if (!answer)
            return std::nullopt;
          // A return must be the last operation in the function body.
          if (std::next(operation.getIterator()) != block.end())
            return std::nullopt;
          return *answer != 0;
        }
        return std::nullopt; // Unsupported operation: fail closed.
      }
      return std::nullopt;
    };

    auto checkDomain = [&](const PresburgerSet &domain,
                           const char *name) -> int {
      auto iterationPlan = buildSymbolicIterationPlan(domain, {{0, 7}, {0, 7}});
      if (!iterationPlan) {
        std::cerr << "FAIL: cannot build plan: " << name << "\n";
        return 1;
      }
      auto module = ModuleOp::create(loc);
      builder.setInsertionPointToStart(module.getBody());
      auto functionType = builder.getFunctionType(
          {builder.getI64Type(), builder.getI64Type()},
          {builder.getI1Type()});
      auto function = func::FuncOp::create(builder, loc, "test_guard", functionType);
      Block *entry = function.addEntryBlock();
      builder.setInsertionPointToStart(entry);
      auto guard = emitExactGuard(builder, loc, *iterationPlan,
                                  entry->getArguments());
      if (!guard) {
        std::cerr << "FAIL: cannot emit guard: " << name << "\n";
        return 1;
      }
      func::ReturnOp::create(builder, loc, ValueRange{*guard});
      if (failed(mlir::verify(module))) {
        std::cerr << "FAIL: invalid generated MLIR: " << name << "\n";
        return 1;
      }
      for (int64_t i = 0; i <= 7; ++i) {
        for (int64_t j = 0; j <= 7; ++j) {
          auto actual = evaluate(function, i, j);
          bool expected = domain.containsPoint(llvm::SmallVector<int64_t>{i, j});
          if (!actual || *actual != expected) {
            std::cerr << "FAIL: guard mismatch: " << name
                      << " at (" << i << "," << j << ") expected=" << expected
                      << " actual=";
            if (actual) std::cerr << *actual;
            else std::cerr << "UNKNOWN";
            std::cerr << "\n";
            return 1;
          }
        }
      }
      return verify(true, name);
    };

    IntegerPolyhedron triangle(PresburgerSpace::getSetSpace(2));
    triangle.addInequality(llvm::SmallVector<int64_t>{1, 0, 0});
    triangle.addInequality(llvm::SmallVector<int64_t>{0, 1, 0});
    triangle.addInequality(llvm::SmallVector<int64_t>{-1, -1, 7});
    PresburgerSet triangleSet(triangle);
    if (checkDomain(triangleSet, "executed triangle guard matches Presburger"))
      return 1;

    IntegerPolyhedron hole(PresburgerSpace::getSetSpace(2));
    hole.addInequality(llvm::SmallVector<int64_t>{1, 0, -2});
    hole.addInequality(llvm::SmallVector<int64_t>{0, 1, -2});
    hole.addInequality(llvm::SmallVector<int64_t>{-1, -1, 5});
    auto cut = buildSourceDomainDifference(triangleSet, PresburgerSet(hole));
    if (!cut || checkDomain(*cut, "executed hole guard matches Presburger"))
      return 1;

    auto overlap = buildSourceDomainUnion(triangleSet, triangleSet);
    if (!overlap || checkDomain(*overlap, "executed overlapping union matches Presburger"))
      return 1;

    IntegerPolyhedron diagonal(PresburgerSpace::getSetSpace(2));
    diagonal.addEquality(llvm::SmallVector<int64_t>{1, -1, 0});
    diagonal.addInequality(llvm::SmallVector<int64_t>{1, 0, 0});
    diagonal.addInequality(llvm::SmallVector<int64_t>{-1, 0, 7});
    if (checkDomain(PresburgerSet(diagonal),
                    "executed equality guard matches Presburger"))
      return 1;

    PresburgerSet empty =
        PresburgerSet::getEmpty(PresburgerSpace::getSetSpace(2));
    if (checkDomain(empty, "executed empty guard matches Presburger"))
      return 1;

    // An intentionally insufficient source box must be rejected BEFORE emission.
    SymbolicIterationPlan unchecked(triangleSet, {{0, 1}, {0, 1}});
    auto negativeModule = ModuleOp::create(loc);
    builder.setInsertionPointToStart(negativeModule.getBody());
    auto negativeType = builder.getFunctionType(
        {builder.getI64Type(), builder.getI64Type()},
        {builder.getI1Type()});
    auto negativeFunction =
        func::FuncOp::create(builder, loc, "negative_guard", negativeType);
    Block *negativeEntry = negativeFunction.addEntryBlock();
    builder.setInsertionPointToStart(negativeEntry);
    auto rejected = emitExactGuard(builder, loc, unchecked,
                                   negativeEntry->getArguments());
    if (verify(!rejected.has_value() && negativeEntry->empty(),
               "unsafe iteration plan rejected before IR emission"))
      return 1;
  }

  // Phase 5.4.3d-1d-a: exact existential congruence.
  {
    using namespace mlir;
    using namespace mlir::presburger;
    MLIRContext context;
    context.loadDialect<arith::ArithDialect, func::FuncDialect>();
    OpBuilder builder(&context);
    Location loc = builder.getUnknownLoc();

    auto runCongruence = [&](int64_t coefficient,
                             int64_t localCoefficient,
                             int64_t offset,
                             const char *label) -> int {
      IntegerPolyhedron poly(PresburgerSpace::getSetSpace(1));
      poly.appendVar(VarKind::Local);
      poly.addEquality(llvm::SmallVector<int64_t>{
          coefficient, localCoefficient, offset});
      // Restrict the domain to a finite box, using rows with no locals.
      poly.addInequality(llvm::SmallVector<int64_t>{1, 0, 8});
      poly.addInequality(llvm::SmallVector<int64_t>{-1, 0, 8});
      PresburgerSet domain(poly);
      auto plan = buildSymbolicIterationPlan(domain, {{-8, 8}});
      if (!plan) return verify(false, label);
      auto module = ModuleOp::create(loc);
      builder.setInsertionPointToStart(module.getBody());
      auto type = builder.getFunctionType({builder.getI64Type()},
                                          {builder.getI1Type()});
      auto function = func::FuncOp::create(builder, loc, "congruence", type);
      Block *entry = function.addEntryBlock();
      builder.setInsertionPointToStart(entry);
      auto guard = emitExactGuard(builder, loc, *plan, entry->getArguments());
      if (!guard) return verify(false, label);
      func::ReturnOp::create(builder, loc, ValueRange{*guard});
      if (failed(mlir::verify(module))) return verify(false, label);
      // Execute emitted SSA directly (the only novel operation is remsi).
      // This checks the emitted expression and checks against the oracle.
      for (int64_t i = -8; i <= 8; ++i) {
        llvm::DenseMap<Value, int64_t> values;
        values[entry->getArgument(0)] = i;
        bool returned = false, result = false;
        auto get = [&](Value v) -> std::optional<int64_t> {
          auto it = values.find(v);
          if (it == values.end()) return std::nullopt;
          return it->second;
        };
        for (Operation &op : *entry) {
          if (auto c = dyn_cast<arith::ConstantOp>(&op)) {
            auto attr = dyn_cast<IntegerAttr>(c.getValue());
            if (!attr || attr.getValue().getBitWidth() > 64)
              return verify(false, label);
            values[c.getResult()] = attr.getValue().getBitWidth() == 1
                ? static_cast<int64_t>(attr.getValue().getZExtValue())
                : attr.getValue().getSExtValue();
          } else if (auto mul = dyn_cast<arith::MulIOp>(&op)) {
            auto a=get(mul.getOperand(0)), b=get(mul.getOperand(1));
            if (!a || !b) return verify(false, label);
            __int128 v=static_cast<__int128>(*a)* *b;
            if (v < std::numeric_limits<int64_t>::min() ||
                v > std::numeric_limits<int64_t>::max())
              return verify(false, label);
            values[mul.getResult()]=static_cast<int64_t>(v);
          } else if (auto add = dyn_cast<arith::AddIOp>(&op)) {
            auto a=get(add.getOperand(0)), b=get(add.getOperand(1));
            if (!a || !b) return verify(false, label);
            __int128 v=static_cast<__int128>(*a)+ *b;
            if (v < std::numeric_limits<int64_t>::min() ||
                v > std::numeric_limits<int64_t>::max())
              return verify(false, label);
            values[add.getResult()]=static_cast<int64_t>(v);
          } else if (auto rem = dyn_cast<arith::RemSIOp>(&op)) {
            auto a=get(rem.getOperand(0)), b=get(rem.getOperand(1));
            if (!a || !b || *b <= 0) return verify(false, label);
            values[rem.getResult()]=*a % *b;
          } else if (auto cmp = dyn_cast<arith::CmpIOp>(&op)) {
            auto a=get(cmp.getOperand(0)), b=get(cmp.getOperand(1));
            if (!a || !b) return verify(false, label);
            bool v;
            switch(cmp.getPredicate()) {
              case arith::CmpIPredicate::eq: v=*a==*b; break;
              case arith::CmpIPredicate::sge: v=*a>=*b; break;
              default: return verify(false, label);
            }
            values[cmp.getResult()]=v;
          } else if (auto opAnd = dyn_cast<arith::AndIOp>(&op)) {
            auto a=get(opAnd.getOperand(0)), b=get(opAnd.getOperand(1));
            if (!a || !b) return verify(false, label);
            values[opAnd.getResult()]=(*a!=0 && *b!=0);
          } else if (auto opOr = dyn_cast<arith::OrIOp>(&op)) {
            auto a=get(opOr.getOperand(0)), b=get(opOr.getOperand(1));
            if (!a || !b) return verify(false, label);
            values[opOr.getResult()]=(*a!=0 || *b!=0);
          } else if (auto ret = dyn_cast<func::ReturnOp>(&op)) {
            auto v=get(ret.getOperand(0));
            if (!v) return verify(false, label);
            result=*v!=0;
            returned=true;
          } else return verify(false, label);
        }
        if (!returned || result != domain.containsPoint(
              llvm::SmallVector<int64_t>{i})) {
          std::cerr << "FAIL: " << label << " at " << i << "\n";
          return 1;
        }
      }
      return verify(true, label);
    };
    if (runCongruence(1, -2, 0,
            "executed even congruence guard matches Presburger")) return 1;
    if (runCongruence(1, 3, -1,
            "executed shifted mod-three guard matches Presburger")) return 1;
    if (runCongruence(-2, -3, 1,
            "executed negative coefficients congruence matches Presburger")) return 1;
    // Local used in an inequality: unsupported and must not emit any IR.
    IntegerPolyhedron unsupported(PresburgerSpace::getSetSpace(1));
    unsupported.appendVar(VarKind::Local);
    unsupported.addEquality(llvm::SmallVector<int64_t>{1, -2, 0});
    unsupported.addInequality(llvm::SmallVector<int64_t>{0, 1, 0});
    unsupported.addInequality(llvm::SmallVector<int64_t>{1, 0, 8});
    unsupported.addInequality(llvm::SmallVector<int64_t>{-1, 0, 8});
    auto unsafe = buildSymbolicIterationPlan(PresburgerSet(unsupported),
                                              {{-8, 8}});
    if (!unsafe) return verify(false, "unsupported local setup");
    auto rejectModule = ModuleOp::create(loc);
    builder.setInsertionPointToStart(rejectModule.getBody());
    auto rejectType = builder.getFunctionType({builder.getI64Type()},
                                               {builder.getI1Type()});
    auto rejectFn = func::FuncOp::create(builder, loc, "reject", rejectType);
    Block *rejectEntry = rejectFn.addEntryBlock();
    builder.setInsertionPointToStart(rejectEntry);
    auto rejected = emitExactGuard(builder, loc, *unsafe,
                                    rejectEntry->getArguments());
    if (verify(!rejected && rejectEntry->empty(),
            "unsupported existential inequality rejected without IR")) return 1;
  }

  // --------------------------------------------------------
  // Phase 5.4.3d-1d-b: actual floor-division SSA semantics.
  // Compare emitted operations against Presburger membership,
  // including negative numerators (floor != truncation).
  // --------------------------------------------------------
  {
    using namespace mlir;
    using namespace mlir::presburger;
    MLIRContext context;
    context.loadDialect<arith::ArithDialect, func::FuncDialect>();
    OpBuilder builder(&context);
    Location loc = builder.getUnknownLoc();

    auto checkFloor = [&](int64_t shift, int64_t divisor,
                          int64_t threshold, const char *name) -> int {
      // N = i+shift, q=floor(N/divisor), q>=threshold.
      IntegerPolyhedron poly(PresburgerSpace::getSetSpace(1, 0, 1));
      poly.addInequality(llvm::SmallVector<int64_t>{1, -divisor, shift});
      poly.addInequality(llvm::SmallVector<int64_t>{-1, divisor,
                                                     divisor - 1 - shift});
      poly.addInequality(llvm::SmallVector<int64_t>{0, 1, -threshold});
      poly.addInequality(llvm::SmallVector<int64_t>{1, 0, 7});
      poly.addInequality(llvm::SmallVector<int64_t>{-1, 0, 7});
      PresburgerSet domain(poly);
      auto plan = buildSymbolicIterationPlan(domain, {{-7, 7}});
      if (!plan) {
        std::cerr << "FAIL: floor plan " << name << "\n";
        return 1;
      }
      auto module = ModuleOp::create(loc);
      builder.setInsertionPointToStart(module.getBody());
      auto type = builder.getFunctionType({builder.getI64Type()},
                                           {builder.getI1Type()});
      auto function = func::FuncOp::create(builder, loc, "floor_guard", type);
      Block *entry = function.addEntryBlock();
      builder.setInsertionPointToStart(entry);
      auto guard = emitExactGuard(builder, loc, *plan, entry->getArguments());
      if (!guard) {
        std::cerr << "FAIL: floor guard emission " << name << "\n";
        return 1;
      }
      func::ReturnOp::create(builder, loc, ValueRange{*guard});
      if (failed(verify(module))) {
        std::cerr << "FAIL: floor MLIR verification " << name << "\n";
        return 1;
      }
      bool observedFloorOp = false;
      for (int64_t i = -7; i <= 7; ++i) {
        llvm::DenseMap<Value, int64_t> values;
        values[entry->getArgument(0)] = i;
        auto valueOf = [&](Value v) -> std::optional<int64_t> {
          auto it = values.find(v);
          if (it == values.end()) return std::nullopt;
          return it->second;
        };
        std::optional<bool> actual;
        for (Operation &op : *entry) {
          if (auto c = dyn_cast<arith::ConstantOp>(&op)) {
            auto a = dyn_cast<IntegerAttr>(c.getValue());
            if (!a || a.getValue().getBitWidth() > 64) return 1;
            values[c.getResult()] = a.getValue().getBitWidth() == 1
                ? static_cast<int64_t>(a.getValue().getZExtValue())
                : a.getValue().getSExtValue();
          } else if (auto mul = dyn_cast<arith::MulIOp>(&op)) {
            auto x = valueOf(mul.getOperand(0));
            auto y = valueOf(mul.getOperand(1));
            if (!x || !y) return 1;
            __int128 result = static_cast<__int128>(*x) * *y;
            if (result < INT64_MIN || result > INT64_MAX) return 1;
            values[mul.getResult()] = static_cast<int64_t>(result);
          } else if (auto add = dyn_cast<arith::AddIOp>(&op)) {
            auto x = valueOf(add.getOperand(0));
            auto y = valueOf(add.getOperand(1));
            if (!x || !y) return 1;
            __int128 result = static_cast<__int128>(*x) + *y;
            if (result < INT64_MIN || result > INT64_MAX) return 1;
            values[add.getResult()] = static_cast<int64_t>(result);
          } else if (auto div = dyn_cast<arith::FloorDivSIOp>(&op)) {
            observedFloorOp = true;
            auto x = valueOf(div.getOperand(0));
            auto y = valueOf(div.getOperand(1));
            if (!x || !y || *y == 0 ||
                (*x == INT64_MIN && *y == -1)) return 1;
            int64_t q = *x / *y;
            if ((*x % *y != 0) && ((*x < 0) != (*y < 0))) --q;
            values[div.getResult()] = q;
          } else if (auto cmp = dyn_cast<arith::CmpIOp>(&op)) {
            auto x = valueOf(cmp.getOperand(0));
            auto y = valueOf(cmp.getOperand(1));
            if (!x || !y) return 1;
            bool result;
            if (cmp.getPredicate() == arith::CmpIPredicate::eq)
              result = *x == *y;
            else if (cmp.getPredicate() == arith::CmpIPredicate::sge)
              result = *x >= *y;
            else return 1;
            values[cmp.getResult()] = result;
          } else if (auto andOp = dyn_cast<arith::AndIOp>(&op)) {
            auto x = valueOf(andOp.getOperand(0));
            auto y = valueOf(andOp.getOperand(1));
            if (!x || !y) return 1;
            values[andOp.getResult()] = (*x != 0 && *y != 0);
          } else if (auto orOp = dyn_cast<arith::OrIOp>(&op)) {
            auto x = valueOf(orOp.getOperand(0));
            auto y = valueOf(orOp.getOperand(1));
            if (!x || !y) return 1;
            values[orOp.getResult()] = (*x != 0 || *y != 0);
          } else if (auto ret = dyn_cast<func::ReturnOp>(&op)) {
            auto x = valueOf(ret.getOperand(0));
            if (!x) return 1;
            actual = *x != 0;
          } else return 1;
        }
        bool expected = domain.containsPoint(llvm::SmallVector<int64_t>{i});
        if (!actual || *actual != expected) {
          std::cerr << "FAIL: floor SSA mismatch " << name
                    << " at " << i << " expected=" << expected << "\n";
          return 1;
        }
      }
      if (!observedFloorOp) return 1;
      return verify(true, name);
    };
    if (checkFloor(0, 3, 1, "executed positive floor guard matches Presburger")) return 1;
    if (checkFloor(0, 3, -1, "executed negative floor guard matches Presburger")) return 1;
    if (checkFloor(2, 3, 0, "executed shifted floor guard matches Presburger")) return 1;

    // 1d-c regression: a non-floor local with finite two-sided bounds
    // is now supported by exact integer interval elimination.
    // x + 2*q >= 0, q <= 2, -7 <= x <= 7.
    // A witness exists exactly when ceil(-x/2) <= 2, i.e. x >= -4.
    IntegerPolyhedron unsupported(PresburgerSpace::getSetSpace(1, 0, 1));
    unsupported.addInequality(llvm::SmallVector<int64_t>{1, 2, 0});
    unsupported.addInequality(llvm::SmallVector<int64_t>{0, -1, 2});
    unsupported.addInequality(llvm::SmallVector<int64_t>{1, 0, 7});
    unsupported.addInequality(llvm::SmallVector<int64_t>{-1, 0, 7});
    auto badPlan = buildSymbolicIterationPlan(
        PresburgerSet(unsupported), {{-7, 7}});
    if (!badPlan) return 1;
    auto badModule = ModuleOp::create(loc);
    builder.setInsertionPointToStart(badModule.getBody());
    auto badType = builder.getFunctionType({builder.getI64Type()},
                                            {builder.getI1Type()});
    auto badFunction = func::FuncOp::create(builder, loc, "bad_floor", badType);
    Block *badEntry = badFunction.addEntryBlock();
    builder.setInsertionPointToStart(badEntry);
    auto accepted = emitExactGuard(builder, loc, *badPlan,
                                   badEntry->getArguments());
    if (!accepted) {
      std::cerr << "FAIL: interval local unexpectedly rejected\n";
      return 1;
    }
    func::ReturnOp::create(builder, loc, ValueRange{*accepted});
    if (verify(succeeded(mlir::verify(badModule)),
               "two-sided non-floor local emitted valid MLIR")) return 1;
  }


  // Phase 5.4.3d-1d-c: one-local two-sided interval elimination.
  // exists q: i-2q>=0, 3q-j>=0, q>=0.
  // For each point this equals floor(i/2)+floor(-j/3)>=0
  // and floor(0/1)+floor(i/2)>=0 (from q>=0).
  {
    using namespace mlir;
    using namespace mlir::presburger;
    IntegerPolyhedron bounded(PresburgerSpace::getSetSpace(2, 0, 1));
    bounded.addInequality(llvm::SmallVector<int64_t>{1, 0, -2, 0});
    bounded.addInequality(llvm::SmallVector<int64_t>{0, -1, 3, 0});
    bounded.addInequality(llvm::SmallVector<int64_t>{0, 0, 1, 0});

    // Phase 5.4.3d-1d-c:
    // Explicitly restrict the complete Presburger domain
    // to the certified candidate iteration box.
    //
    // 0 <= i <= 7
    // 0 <= j <= 7
    //
    // These constraints are part of the logical domain,
    // not merely bounds supplied to the iteration planner.

    bounded.addInequality(
        llvm::SmallVector<int64_t>{1, 0, 0, 0});

    bounded.addInequality(
        llvm::SmallVector<int64_t>{-1, 0, 0, 7});

    bounded.addInequality(
        llvm::SmallVector<int64_t>{0, 1, 0, 0});

    bounded.addInequality(
        llvm::SmallVector<int64_t>{0, -1, 0, 7});

    auto plan = buildSymbolicIterationPlan(PresburgerSet(bounded),
                                           {{0, 7}, {0, 7}});
    if (!plan) {
      std::cerr << "FAIL: 1d-c cannot construct symbolic plan\n";
      return 1;
    }
    auto guard = buildExactGuardPlan(*plan);
    // Phase 5.4.3d-1d-c:
    //
    // Three local-bearing inequalities:
    //
    //   i - 2q >= 0
    //   3q - j >= 0
    //   q >= 0
    //
    // produce two lower/upper bound comparisons.
    //
    // Four additional inequalities describe the
    // certified candidate box:
    //
    //   0 <= i <= 7
    //   0 <= j <= 7
    //
    // Expected:
    //   2 ExistentialBounds
    //   4 ordinary Inequalities
    //   6 constraints total.

    bool shape = guard && guard->disjuncts.size() == 1;

    if (shape) {
      const auto &constraints =
          guard->disjuncts[0].constraints;

      std::size_t existentialCount = 0;
      std::size_t inequalityCount = 0;

      for (const auto &c : constraints) {
        if (c.kind == GuardConstraintKind::ExistentialBounds) {
          ++existentialCount;

          if (c.floorDivisor <= 0 ||
              c.secondFloorDivisor <= 0)
            shape = false;

        } else if (c.kind == GuardConstraintKind::Inequality) {
          ++inequalityCount;

        } else {
          shape = false;
        }
      }

      shape = shape &&
              constraints.size() == 6 &&
              existentialCount == 2 &&
              inequalityCount == 4;
    }

    if (verify(
            shape,
            "two-sided existential local converted to floor comparisons"))
      return 1;
    MLIRContext context;
    context.loadDialect<arith::ArithDialect, func::FuncDialect>();
    OpBuilder builder(&context);
    auto loc = builder.getUnknownLoc();
    auto module = ModuleOp::create(loc);
    builder.setInsertionPointToStart(module.getBody());
    auto type = builder.getFunctionType(
        {builder.getI64Type(), builder.getI64Type()}, {builder.getI1Type()});
    auto function = func::FuncOp::create(builder, loc, "existential_guard", type);
    auto *entry = function.addEntryBlock();
    builder.setInsertionPointToStart(entry);
    auto predicate = emitExactGuard(builder, loc, *plan, entry->getArguments());
    if (!predicate) {
      std::cerr << "FAIL: existential interval guard emission\n";
      return 1;
    }
    func::ReturnOp::create(builder, loc, ValueRange{*predicate});
    if (verify(succeeded(mlir::verify(module)),
               "two-sided existential guard passes MLIR verifier"))
      return 1;

    // --------------------------------------------------------
    // Phase 5.4.3d-1d-c:
    // Generated MLIR SSA semantic validation.
    //
    // Execute the actual generated SSA operations.
    //
    // This interpreter does NOT reconstruct the original
    // Presburger inequalities.
    //
    // The independent oracle is:
    //
    //   PresburgerSet::containsPoint()
    //
    // Finite enumeration is TEST-ONLY.
    // Production Guard Generation remains symbolic.
    // --------------------------------------------------------

    PresburgerSet oracleDomain(bounded);

    auto evaluateGuard =
        [&](int64_t i, int64_t j) -> std::optional<bool> {

      Block &block = function.front();

      if (block.getNumArguments() != 2)
        return std::nullopt;

      llvm::DenseMap<Value, int64_t> values;

      values[block.getArgument(0)] = i;
      values[block.getArgument(1)] = j;

      auto getValue =
          [&](Value value) -> std::optional<int64_t> {

        auto it = values.find(value);

        if (it == values.end())
          return std::nullopt;

        return it->second;
      };

      auto fitsI64 = [](__int128 value) -> bool {
        return value >=
                   std::numeric_limits<int64_t>::min() &&
               value <=
                   std::numeric_limits<int64_t>::max();
      };

      bool returned = false;
      bool answer = false;

      for (Operation &operation : block) {

        if (auto constant =
                dyn_cast<arith::ConstantOp>(&operation)) {

          auto attribute =
              dyn_cast<IntegerAttr>(constant.getValue());

          if (!attribute)
            return std::nullopt;

          const llvm::APInt &integer =
              attribute.getValue();

          if (integer.getBitWidth() > 64)
            return std::nullopt;

          int64_t value =
              integer.getBitWidth() == 1
                  ? static_cast<int64_t>(
                        integer.getZExtValue())
                  : integer.getSExtValue();

          values[constant.getResult()] = value;
          continue;
        }

        if (auto addition =
                dyn_cast<arith::AddIOp>(&operation)) {

          auto lhs = getValue(addition.getOperand(0));
          auto rhs = getValue(addition.getOperand(1));

          if (!lhs || !rhs)
            return std::nullopt;

          __int128 result =
              static_cast<__int128>(*lhs) + *rhs;

          if (!fitsI64(result))
            return std::nullopt;

          values[addition.getResult()] =
              static_cast<int64_t>(result);

          continue;
        }

        if (auto multiply =
                dyn_cast<arith::MulIOp>(&operation)) {

          auto lhs = getValue(multiply.getOperand(0));
          auto rhs = getValue(multiply.getOperand(1));

          if (!lhs || !rhs)
            return std::nullopt;

          __int128 result =
              static_cast<__int128>(*lhs) * *rhs;

          if (!fitsI64(result))
            return std::nullopt;

          values[multiply.getResult()] =
              static_cast<int64_t>(result);

          continue;
        }

        if (auto division =
                dyn_cast<arith::FloorDivSIOp>(&operation)) {

          auto lhs = getValue(division.getOperand(0));
          auto rhs = getValue(division.getOperand(1));

          if (!lhs || !rhs || *rhs == 0)
            return std::nullopt;

          // Compute mathematical floor division
          // using widened signed arithmetic.
          //
          // Unlike truncating division:
          //
          //   floor(-1 / 3) == -1

          __int128 numerator = *lhs;
          __int128 denominator = *rhs;

          __int128 quotient =
              numerator / denominator;

          __int128 remainder =
              numerator % denominator;

          if (remainder != 0 &&
              ((remainder < 0) !=
               (denominator < 0))) {
            --quotient;
          }

          if (!fitsI64(quotient))
            return std::nullopt;

          values[division.getResult()] =
              static_cast<int64_t>(quotient);

          continue;
        }

        if (auto remainder =
                dyn_cast<arith::RemSIOp>(&operation)) {

          auto lhs = getValue(remainder.getOperand(0));
          auto rhs = getValue(remainder.getOperand(1));

          if (!lhs || !rhs || *rhs == 0)
            return std::nullopt;

          __int128 result =
              static_cast<__int128>(*lhs) % *rhs;

          if (!fitsI64(result))
            return std::nullopt;

          values[remainder.getResult()] =
              static_cast<int64_t>(result);

          continue;
        }

        if (auto comparison =
                dyn_cast<arith::CmpIOp>(&operation)) {

          auto lhs = getValue(comparison.getOperand(0));
          auto rhs = getValue(comparison.getOperand(1));

          if (!lhs || !rhs)
            return std::nullopt;

          bool result = false;

          switch (comparison.getPredicate()) {

          case arith::CmpIPredicate::eq:
            result = *lhs == *rhs;
            break;

          case arith::CmpIPredicate::ne:
            result = *lhs != *rhs;
            break;

          case arith::CmpIPredicate::sge:
            result = *lhs >= *rhs;
            break;

          case arith::CmpIPredicate::sgt:
            result = *lhs > *rhs;
            break;

          case arith::CmpIPredicate::sle:
            result = *lhs <= *rhs;
            break;

          case arith::CmpIPredicate::slt:
            result = *lhs < *rhs;
            break;

          default:
            return std::nullopt;
          }

          values[comparison.getResult()] =
              result ? 1 : 0;

          continue;
        }

        if (auto logicalAnd =
                dyn_cast<arith::AndIOp>(&operation)) {

          auto lhs = getValue(logicalAnd.getOperand(0));
          auto rhs = getValue(logicalAnd.getOperand(1));

          if (!lhs || !rhs)
            return std::nullopt;

          values[logicalAnd.getResult()] =
              (*lhs != 0 && *rhs != 0) ? 1 : 0;

          continue;
        }

        if (auto logicalOr =
                dyn_cast<arith::OrIOp>(&operation)) {

          auto lhs = getValue(logicalOr.getOperand(0));
          auto rhs = getValue(logicalOr.getOperand(1));

          if (!lhs || !rhs)
            return std::nullopt;

          values[logicalOr.getResult()] =
              (*lhs != 0 || *rhs != 0) ? 1 : 0;

          continue;
        }

        if (auto ret =
                dyn_cast<func::ReturnOp>(&operation)) {

          if (returned || ret.getNumOperands() != 1)
            return std::nullopt;

          auto value = getValue(ret.getOperand(0));

          if (!value)
            return std::nullopt;

          answer = *value != 0;
          returned = true;

          continue;
        }

        // Any unsupported SSA operation causes
        // semantic validation to fail.
        return std::nullopt;
      }

      if (!returned)
        return std::nullopt;

      return answer;
    };

    // --------------------------------------------------------
    // Exhaustive finite-box semantic comparison.
    //
    // The candidate box is:
    //
    //   0 <= i <= 7
    //   0 <= j <= 7
    //
    // Total: 8 * 8 = 64 membership comparisons.
    // --------------------------------------------------------

    std::size_t checkedPoints = 0;

    for (int64_t i = 0; i <= 7; ++i) {
      for (int64_t j = 0; j <= 7; ++j) {

        auto actual = evaluateGuard(i, j);

        const bool expected =
            oracleDomain.containsPoint(
                llvm::SmallVector<int64_t>{i, j});

        if (!actual || *actual != expected) {

          std::cerr
              << "FAIL: existential SSA semantic mismatch"
              << " at (" << i << "," << j << ")"
              << " expected=" << expected
              << " actual=";

          if (actual)
            std::cerr << *actual;
          else
            std::cerr << "UNKNOWN";

          std::cerr << "\n";

          return 1;
        }

        ++checkedPoints;
      }
    }

    if (verify(
            checkedPoints == 64,
            "two-sided existential SSA matches Presburger over 64 points"))
      return 1;

  }



  // --------------------------------------------------------
  // Phase 5.4.3d-2a:
  // Affine Loop Nest Construction.
  //
  // Validate real generated MLIR operations.
  // This is not yet an execution test.
  // --------------------------------------------------------
  {
    using namespace mlir;
    using namespace mlir::presburger;
    using namespace systolic::logical;

    MLIRContext context;

    context.loadDialect<
        affine::AffineDialect,
        func::FuncDialect>();

    OpBuilder builder(&context);
    Location loc = builder.getUnknownLoc();

    // Exact triangle domain:
    //
    // i >= 0
    // j >= 0
    // i + j <= 7
    //
    // Candidate iteration box:
    //
    // 0 <= i,j <= 7

    IntegerPolyhedron triangle(
        PresburgerSpace::getSetSpace(2));

    triangle.addInequality(
        llvm::SmallVector<int64_t>{1, 0, 0});

    triangle.addInequality(
        llvm::SmallVector<int64_t>{0, 1, 0});

    triangle.addInequality(
        llvm::SmallVector<int64_t>{-1, -1, 7});

    PresburgerSet domain(triangle);

    auto plan = buildSymbolicIterationPlan(
        domain,
        {{0, 7}, {0, 7}});

    if (verify(
            plan.has_value(),
            "2a triangle iteration plan constructed"))
      return 1;

    auto module = ModuleOp::create(loc);

    builder.setInsertionPointToStart(
        module.getBody());

    auto functionType = builder.getFunctionType(
        {},
        {});

    auto function = func::FuncOp::create(
        builder,
        loc,
        "test_affine_loop_nest",
        functionType);

    Block *entry = function.addEntryBlock();

    builder.setInsertionPointToStart(entry);

    func::ReturnOp::create(
        builder,
        loc);

    // Insert the generated loop nest before return.

    builder.setInsertionPoint(
        entry->getTerminator());

    auto emitted = emitAffineLoopNest(
        builder,
        loc,
        *plan);

    if (verify(
            emitted.has_value(),
            "2a affine loop nest emitted"))
      return 1;

    if (verify(
            emitted->logicalIVs.size() == 2 &&
            emitted->outermost != nullptr,
            "2a two logical induction variables"))
      return 1;

    bool correctIVTypes = true;

    for (Value iv : emitted->logicalIVs) {
      correctIVTypes &=
          iv.getType().isIndex();
    }

    if (verify(
            correctIVTypes,
            "2a affine induction variables are index"))
      return 1;

    std::size_t loopCount = 0;

    module.walk(
        [&](affine::AffineForOp) {
          ++loopCount;
        });

    if (verify(
            loopCount == 2,
            "2a exactly two affine loops generated"))
      return 1;

    if (verify(
            succeeded(mlir::verify(module)),
            "2a generated affine loop nest passes verifier"))
      return 1;

    // ------------------------------------------------------
    // Negative test:
    // An unsafe closed upper bound must be rejected
    // without creating any MLIR operation.
    // ------------------------------------------------------

    IntegerPolyhedron bounded(
        PresburgerSpace::getSetSpace(1));

    bounded.addInequality(
        llvm::SmallVector<int64_t>{1, 0});

    bounded.addInequality(
        llvm::SmallVector<int64_t>{-1, 7});

    PresburgerSet boundedDomain(bounded);

    SymbolicIterationPlan unsafe(
        boundedDomain,
        {{0, std::numeric_limits<int64_t>::max()}});

    auto negativeModule = ModuleOp::create(loc);

    builder.setInsertionPointToStart(
        negativeModule.getBody());

    auto negativeFunction = func::FuncOp::create(
        builder,
        loc,
        "test_unsafe_affine_nest",
        functionType);

    Block *negativeEntry =
        negativeFunction.addEntryBlock();

    builder.setInsertionPointToStart(
        negativeEntry);

    func::ReturnOp::create(
        builder,
        loc);

    builder.setInsertionPoint(
        negativeEntry->getTerminator());

    auto rejected = emitAffineLoopNest(
        builder,
        loc,
        unsafe);

    std::size_t negativeLoops = 0;

    negativeFunction.walk(
        [&](affine::AffineForOp) {
          ++negativeLoops;
        });

    if (verify(
            !rejected.has_value() &&
            negativeLoops == 0 &&
            succeeded(mlir::verify(negativeModule)),
            "2a unsafe upper bound rejected without IR"))
      return 1;
  }



  // --------------------------------------------------------
  // Phase 5.4.3d-2b:
  // Guarded Affine Loop Nest Integration.
  //
  // This tests generated MLIR structure and verification.
  // Actual loop execution belongs to Phase 2c.
  // --------------------------------------------------------
  {
    using namespace mlir;
    using namespace mlir::presburger;

    MLIRContext context;

    context.loadDialect<
        affine::AffineDialect,
        arith::ArithDialect,
        scf::SCFDialect,
        func::FuncDialect>();

    OpBuilder builder(&context);
    Location loc = builder.getUnknownLoc();

    IntegerPolyhedron triangle(
        PresburgerSpace::getSetSpace(2));

    triangle.addInequality(
        llvm::SmallVector<int64_t>{1, 0, 0});

    triangle.addInequality(
        llvm::SmallVector<int64_t>{0, 1, 0});

    triangle.addInequality(
        llvm::SmallVector<int64_t>{-1, -1, 7});

    PresburgerSet domain(triangle);

    auto plan = buildSymbolicIterationPlan(
        domain,
        {{0, 7}, {0, 7}});

    if (verify(
            plan.has_value(),
            "2b symbolic iteration plan constructed"))
      return 1;

    auto module = ModuleOp::create(loc);

    builder.setInsertionPointToStart(
        module.getBody());

    auto functionType = builder.getFunctionType(
        {}, {});

    auto function = func::FuncOp::create(
        builder,
        loc,
        "test_guarded_affine_nest",
        functionType);

    Block *entry = function.addEntryBlock();

    builder.setInsertionPointToStart(entry);

    func::ReturnOp::create(builder, loc);

    builder.setInsertionPoint(
        entry->getTerminator());

    auto emitted = emitGuardedAffineLoopNest(
        builder,
        loc,
        *plan);

    if (verify(
            emitted.has_value(),
            "2b guarded affine loop nest emitted"))
      return 1;

    bool structure =
        emitted->logicalIVs.size() == 2 &&
        emitted->logicalI64IVs.size() == 2 &&
        emitted->outermost &&
        emitted->guardIf &&
        emitted->predicate;

    if (verify(
            structure,
            "2b generated loop and guard structure"))
      return 1;

    std::size_t loops = 0;
    std::size_t ifs = 0;
    std::size_t indexCasts = 0;

    function.walk([&](affine::AffineForOp) {
      ++loops;
    });

    function.walk([&](scf::IfOp) {
      ++ifs;
    });

    function.walk([&](arith::IndexCastOp) {
      ++indexCasts;
    });

    if (verify(
            loops == 2 &&
            ifs == 1 &&
            indexCasts == 2,
            "2b two affine loops and one guarded region"))
      return 1;

    bool nesting = true;

    auto *innerBlock =
        emitted->logicalIVs.back()
            .getParentBlock();

    if (!innerBlock)
      nesting = false;

    if (nesting) {
      auto innerLoop =
          dyn_cast<affine::AffineForOp>(
              innerBlock->getParentOp());

      nesting =
          static_cast<bool>(innerLoop) &&
          emitted->guardIf->getParentOp() ==
              innerLoop.getOperation();
    }

    if (verify(
            nesting,
            "2b scf.if nested inside innermost affine.for"))
      return 1;

    if (verify(
            succeeded(mlir::verify(module)),
            "2b mixed dialect MLIR passes verifier"))
      return 1;

    // ------------------------------------------------------
    // Negative test:
    // An invalid plan must not generate partial IR.
    // ------------------------------------------------------

    IntegerPolyhedron bounded(
        PresburgerSpace::getSetSpace(1));

    bounded.addInequality(
        llvm::SmallVector<int64_t>{1, 0});

    bounded.addInequality(
        llvm::SmallVector<int64_t>{-1, 7});

    SymbolicIterationPlan unsafe(
        PresburgerSet(bounded),
        {{0, std::numeric_limits<int64_t>::max()}});

    auto negativeModule = ModuleOp::create(loc);

    builder.setInsertionPointToStart(
        negativeModule.getBody());

    auto negativeFunction = func::FuncOp::create(
        builder,
        loc,
        "test_guarded_nest_rejection",
        functionType);

    Block *negativeEntry =
        negativeFunction.addEntryBlock();

    builder.setInsertionPointToStart(negativeEntry);

    func::ReturnOp::create(builder, loc);

    builder.setInsertionPoint(
        negativeEntry->getTerminator());

    auto rejected = emitGuardedAffineLoopNest(
        builder,
        loc,
        unsafe);

    std::size_t rejectedLoops = 0;
    std::size_t rejectedIfs = 0;

    negativeFunction.walk(
        [&](affine::AffineForOp) {
          ++rejectedLoops;
        });

    negativeFunction.walk(
        [&](scf::IfOp) {
          ++rejectedIfs;
        });

    if (verify(
            !rejected &&
            rejectedLoops == 0 &&
            rejectedIfs == 0 &&
            succeeded(mlir::verify(negativeModule)),
            "2b unsafe plan rejected without residual IR"))
      return 1;
  }



  // --------------------------------------------------------
  // Phase 5.4.3d-2c-1:
  // Execute actual generated affine.for/scf.if MLIR.
  // --------------------------------------------------------
  {
    using namespace mlir;
    using namespace mlir::presburger;
    using namespace systolic::logical;
    using namespace systolic::logical::testing;

    MLIRContext context;

    context.loadDialect<
        affine::AffineDialect,
        arith::ArithDialect,
        scf::SCFDialect,
        func::FuncDialect>();

    OpBuilder builder(&context);
    Location loc = builder.getUnknownLoc();

    // D = {(i,j) : i >= 0, j >= 0, i+j <= 7}

    IntegerPolyhedron triangle(
        PresburgerSpace::getSetSpace(2));

    triangle.addInequality(
        llvm::SmallVector<int64_t>{1, 0, 0});

    triangle.addInequality(
        llvm::SmallVector<int64_t>{0, 1, 0});

    triangle.addInequality(
        llvm::SmallVector<int64_t>{-1, -1, 7});

    PresburgerSet domain(triangle);

    auto plan = buildSymbolicIterationPlan(
        domain, {{0, 7}, {0, 7}});

    if (verify(
            plan.has_value(),
            "2c triangle iteration plan"))
      return 1;

    auto module = ModuleOp::create(loc);

    builder.setInsertionPointToStart(
        module.getBody());

    auto function = func::FuncOp::create(
        builder,
        loc,
        "test_exact_execution",
        builder.getFunctionType({}, {}));

    Block *entry = function.addEntryBlock();

    builder.setInsertionPointToStart(entry);
    func::ReturnOp::create(builder, loc);

    builder.setInsertionPoint(entry->getTerminator());

    auto emitted = emitGuardedAffineLoopNest(
        builder, loc, *plan);

    if (verify(
            emitted.has_value(),
            "2c guarded affine nest constructed"))
      return 1;

    if (verify(
            succeeded(mlir::verify(module)),
            "2c executable-subset MLIR verifier"))
      return 1;

    // Interpret the actual emitted Operation Tree.

    auto execution = interpretGuardedLoopNest(
        emitted->outermost,
        emitted->guardIf,
        emitted->logicalIVs);

    if (verify(
            execution.has_value(),
            "2c generated affine/scf IR interpreted"))
      return 1;

    if (verify(
            execution->visitedPoints == 64,
            "2c all 64 candidate points visited"))
      return 1;

    if (verify(
            execution->acceptedPoints == 36,
            "2c exactly 36 triangle points executed"))
      return 1;

    bool exactlyOnce = true;

    for (int64_t i = 0; i < 8; ++i) {
      for (int64_t j = 0; j < 8; ++j) {

        Point point{i, j};

        auto it = execution->counts.find(point);

        const std::size_t actual =
            it == execution->counts.end()
                ? 0 : it->second;

        llvm::SmallVector<int64_t> coordinates{i, j};

        const bool expected =
            domain.containsPoint(coordinates);

        if (actual != (expected ? 1u : 0u))
          exactlyOnce = false;
      }
    }

    if (verify(
            exactlyOnce,
            "2c every Presburger point executed exactly once"))
      return 1;
  }



  // --------------------------------------------------------
  // Phase 5.4.3d-3a-1:
  // Source IV Recovery Arithmetic Preflight.
  // --------------------------------------------------------
  {
    using namespace systolic::logical;

    // Unimodular:
    //
    // z0 = i + j
    // z1 = j
    //
    // i = z0 - z1
    // j = z1

    PartitionCoordinateMap map;

    map.sourceToLogical = {
        {BigInt(1), BigInt(1)},
        {BigInt(0), BigInt(1)}
    };

    auto inverse =
        domain_detail::invertIntegerMatrix(
            map.sourceToLogical);

    if (verify(
            inverse.has_value(),
            "3a1 unimodular inverse constructed"))
      return 1;

    map.logicalToSource = *inverse;

    auto recovery = buildSourceIVRecoveryPlan(
        map, {{0, 7}, {0, 7}});

    if (verify(
            recovery.has_value() &&
            recovery->denominator == 1 &&
            recovery->dimension == 2 &&
            recovery->inverseNumerator[0][0] == 1 &&
            recovery->inverseNumerator[0][1] == -1 &&
            recovery->inverseNumerator[1][0] == 0 &&
            recovery->inverseNumerator[1][1] == 1,
            "3a1 unimodular recovery plan"))
      return 1;

    // Non-unimodular:
    //
    // z0 = 2*i
    // z1 = j
    //
    // i = z0/2
    // j = z1

    PartitionCoordinateMap nonUnimodular;

    nonUnimodular.sourceToLogical = {
        {BigInt(2), BigInt(0)},
        {BigInt(0), BigInt(1)}
    };

    auto nonUniInverse =
        domain_detail::invertIntegerMatrix(
            nonUnimodular.sourceToLogical);

    if (verify(
            nonUniInverse.has_value(),
            "3a1 non-unimodular inverse constructed"))
      return 1;

    nonUnimodular.logicalToSource = *nonUniInverse;

    auto nonUniRecovery =
        buildSourceIVRecoveryPlan(
            nonUnimodular,
            {{0, 6}, {0, 3}});

    if (verify(
            nonUniRecovery.has_value() &&
            nonUniRecovery->denominator == 2 &&
            nonUniRecovery->inverseNumerator[0][0] == 1 &&
            nonUniRecovery->inverseNumerator[0][1] == 0 &&
            nonUniRecovery->inverseNumerator[1][0] == 0 &&
            nonUniRecovery->inverseNumerator[1][1] == 2,
            "3a1 non-unimodular recovery plan"))
      return 1;

    // A malformed inverse must be rejected.

    PartitionCoordinateMap inconsistent = map;

    inconsistent.logicalToSource[0][0] = Rat(2);

    if (verify(
            !buildSourceIVRecoveryPlan(
                inconsistent,
                {{0, 7}, {0, 7}}),
            "3a1 inconsistent inverse rejected"))
      return 1;

    // An unsafe intermediate sum must be rejected.
    //
    // i = z0 - z1
    //
    // The interval z0 = positive large,
    // z1 = negative large can exceed i64.

    const int64_t large =
        std::numeric_limits<int64_t>::max() - 1;

    if (verify(
            !buildSourceIVRecoveryPlan(
                map,
                {{large, large}, {-large, -large}}),
            "3a1 unsafe intermediate arithmetic rejected"))
      return 1;

    // Invalid bounds must also be rejected.

    if (verify(
            !buildSourceIVRecoveryPlan(
                map,
                {{8, 0}, {0, 7}}),
            "3a1 invalid recovery bounds rejected"))
      return 1;
  }



  // --------------------------------------------------------
  // Phase 5.4.3d-3a-2:
  // Actual Source IV Recovery SSA Emission.
  //
  // Tests:
  //   - Unimodular inverse
  //   - Non-unimodular inverse
  //   - Recovery SSA inside scf.if
  //   - MLIR verification
  //
  // Arithmetic execution/oracle comparison follows
  // in Phase 3a-3.
  // --------------------------------------------------------
  {
    using namespace mlir;
    using namespace mlir::presburger;
    using namespace systolic::logical;

    bool allCasesPassed = true;

    for (int caseNumber = 0; caseNumber < 2;
         ++caseNumber) {

      const bool nonUnimodular =
          caseNumber == 1;

      PartitionCoordinateMap coordinateMap;

      if (nonUnimodular) {
        coordinateMap.sourceToLogical = {
            {BigInt(2), BigInt(0)},
            {BigInt(0), BigInt(1)}
        };
      } else {
        coordinateMap.sourceToLogical = {
            {BigInt(1), BigInt(1)},
            {BigInt(0), BigInt(1)}
        };
      }

      auto inverse =
          domain_detail::invertIntegerMatrix(
              coordinateMap.sourceToLogical);

      if (!inverse) {
        { std::cerr << "3a2 DEBUG: failure at line " << __LINE__ << ", caseNumber=" << caseNumber << "\n"; allCasesPassed = false; }
        break;
      }

      coordinateMap.logicalToSource = *inverse;

      // Source domain: 0 <= x0,x1 <= 3.
      //
      // Exact logical image is derived through
      // the existing Presburger projection API.

      IntegerPolyhedron source(
          PresburgerSpace::getSetSpace(2));

      source.addInequality(
          llvm::SmallVector<int64_t>{1, 0, 0});
      source.addInequality(
          llvm::SmallVector<int64_t>{-1, 0, 3});
      source.addInequality(
          llvm::SmallVector<int64_t>{0, 1, 0});
      source.addInequality(
          llvm::SmallVector<int64_t>{0, -1, 3});

      auto logicalDomain = buildLogicalDomain(
          coordinateMap, PresburgerSet(source));

      if (!logicalDomain) {
        { std::cerr << "3a2 DEBUG: failure at line " << __LINE__ << ", caseNumber=" << caseNumber << "\n"; allCasesPassed = false; }
        break;
      }

      const std::vector<ClosedIntegerInterval>
          bounds = {{0, 6}, {0, 3}};

      // Construct a guard-friendly exact logical image
      // independently from the general projection.

      AffineIntegerDomain sourceInequalities;
      sourceInequalities.sourceDimension = 2;

      // 0 <= x0 <= 3, 0 <= x1 <= 3.
      sourceInequalities.inequalities = {
          {{BigInt(-1), BigInt(0)}, BigInt(0)},
          {{BigInt(1), BigInt(0)}, BigInt(3)},
          {{BigInt(0), BigInt(-1)}, BigInt(0)},
          {{BigInt(0), BigInt(1)}, BigInt(3)}
      };

      auto normalized = buildRecoverableLogicalDomain(
          coordinateMap, sourceInequalities);

      if (!normalized) {
        std::cerr
            << "3a2: exact normalization failed, case="
            << caseNumber << "\\n";
        allCasesPassed = false;
        break;
      }

      // Independent symbolic equivalence check:
      //
      // normalized \\ projected = empty
      // projected \\ normalized = empty

      auto extra =
          normalized->subtract(*logicalDomain);

      auto missing =
          logicalDomain->subtract(*normalized);

      if (!extra.isIntegerEmpty() ||
          !missing.isIntegerEmpty()) {
        std::cerr
            << "3a2: domain equivalence failed, case="
            << caseNumber << "\\n";
        allCasesPassed = false;
        break;
      }

      auto plan = buildSymbolicIterationPlan(
          *normalized, bounds);

      if (!plan || !buildExactGuardPlan(*plan)) {
        std::cerr
            << "3a2: normalized guard rejected, case="
            << caseNumber << "\\n";
        allCasesPassed = false;
        break;
      }

      std::cerr
          << "3a2 DEBUG: caseNumber=" << caseNumber
          << ", disjuncts="
          << logicalDomain->getNumDisjuncts()
          << "\\n";

      for (const auto &disjunct :
           logicalDomain->getAllDisjuncts()) {
        std::cerr
            << "3a2 DEBUG: dimensions="
            << disjunct.getNumDimVars()
            << ", locals="
            << disjunct.getNumLocalVars()
            << ", equalities="
            << disjunct.getNumEqualities()
            << ", inequalities="
            << disjunct.getNumInequalities()
            << "\\n";
      }

      std::cerr
          << "3a2 DEBUG: exactGuardPlan="
          << (plan && buildExactGuardPlan(*plan)
                  ? "ACCEPT"
                  : "REJECT")
          << "\\n";

      if (!plan) {
        { std::cerr << "3a2 DEBUG: failure at line " << __LINE__ << ", caseNumber=" << caseNumber << "\n"; allCasesPassed = false; }
        break;
      }

      MLIRContext context;

      context.loadDialect<
          affine::AffineDialect,
          arith::ArithDialect,
          scf::SCFDialect,
          func::FuncDialect>();

      OpBuilder builder(&context);
      Location loc = builder.getUnknownLoc();

      auto module = ModuleOp::create(loc);

      builder.setInsertionPointToStart(
          module.getBody());

      auto function = func::FuncOp::create(
          builder,
          loc,
          nonUnimodular
              ? "test_nonunimodular_source_iv"
              : "test_unimodular_source_iv",
          builder.getFunctionType({}, {}));

      Block *entry = function.addEntryBlock();

      builder.setInsertionPointToStart(entry);

      func::ReturnOp::create(builder, loc);

      builder.setInsertionPoint(
          entry->getTerminator());

      auto guarded = emitGuardedAffineLoopNest(
          builder, loc, *plan);

      if (!guarded) {
        { std::cerr << "3a2 DEBUG: failure at line " << __LINE__ << ", caseNumber=" << caseNumber << "\n"; allCasesPassed = false; }
        break;
      }

      auto recovered = emitSourceIVRecovery(
          builder,
          loc,
          coordinateMap,
          bounds,
          *guarded);

      if (!recovered ||
          recovered->sourceIVs.size() != 2) {
        { std::cerr << "3a2 DEBUG: failure at line " << __LINE__ << ", caseNumber=" << caseNumber << "\n"; allCasesPassed = false; }
        break;
      }

      Block *thenBlock =
          &guarded->guardIf
               .getThenRegion().front();

      for (Value iv : recovered->sourceIVs) {
        if (!iv.getType().isInteger(64) ||
            !iv.getDefiningOp() ||
            iv.getDefiningOp()->getBlock() !=
                thenBlock) {
          { std::cerr << "3a2 DEBUG: failure at line " << __LINE__ << ", caseNumber=" << caseNumber << "\n"; allCasesPassed = false; }
        }
      }

      if (!allCasesPassed)
        break;


      // --------------------------------------------------
      // Phase 5.4.3d-3b-1a:
      //
      // Emit A/B/C source-space affine access SSA.
      //
      // This is SSA construction and structural
      // verification. Numeric access execution is
      // deferred to 3b-1b.
      // --------------------------------------------------

      const std::vector<std::vector<BigInt>>
          accessARows = {
              {BigInt(1), BigInt(0)},
              {BigInt(0), BigInt(1)}
          };

      const std::vector<std::vector<BigInt>>
          accessBRows = {
              {BigInt(0), BigInt(1)},
              {BigInt(1), BigInt(0)}
          };

      const std::vector<std::vector<BigInt>>
          accessCRows = {
              {BigInt(1), BigInt(1)}
          };

      auto accessA = emitSourceAffineAccess(
          builder, loc, coordinateMap, bounds,
          *guarded, *recovered,
          accessARows,
          {BigInt(0), BigInt(0)});

      auto accessB = emitSourceAffineAccess(
          builder, loc, coordinateMap, bounds,
          *guarded, *recovered,
          accessBRows,
          {BigInt(0), BigInt(0)});

      auto accessC = emitSourceAffineAccess(
          builder, loc, coordinateMap, bounds,
          *guarded, *recovered,
          accessCRows,
          {BigInt(0)});

      if (!accessA || !accessB || !accessC ||
          accessA->indices.size() != 2 ||
          accessB->indices.size() != 2 ||
          accessC->indices.size() != 1) {

        std::cerr
            << "3b1a: access SSA emission failed"
            << ", case=" << caseNumber << "\n";

        allCasesPassed = false;
        break;
      }

      Block *accessThenBlock =
          &guarded->guardIf.getThenRegion().front();

      auto checkAccess = [&](const EmittedSourceAccess &access) {
        for (Value index : access.indices) {
          if (!index ||
              !index.getType().isInteger(64) ||
              !index.getDefiningOp() ||
              index.getDefiningOp()->getBlock() !=
                  accessThenBlock)
            return false;
        }
        return true;
      };

      if (!checkAccess(*accessA) ||
          !checkAccess(*accessB) ||
          !checkAccess(*accessC)) {

        std::cerr
            << "3b1a: access indices outside guard"
            << ", case=" << caseNumber << "\n";

        allCasesPassed = false;
        break;
      }

      // Reject an unsafe coefficient without adding
      // further operations to the generated IR.

      const std::size_t operationCountBefore =
          std::distance(
              accessThenBlock->begin(),
              accessThenBlock->end());

      auto unsafeAccess = emitSourceAffineAccess(
          builder, loc, coordinateMap, bounds,
          *guarded, *recovered,
          {{BigInt(
              std::numeric_limits<int64_t>::max()),
            BigInt(1)}},
          {BigInt(0)});

      const std::size_t operationCountAfter =
          std::distance(
              accessThenBlock->begin(),
              accessThenBlock->end());

      if (unsafeAccess ||
          operationCountBefore != operationCountAfter) {

        std::cerr
            << "3b1a: unsafe access was not rejected"
            << ", case=" << caseNumber << "\n";

        allCasesPassed = false;
        break;
      }

      std::cout
          << "PASS: 3b1a "
          << (nonUnimodular
                  ? "non-unimodular"
                  : "unimodular")
          << " guarded A/B/C access SSA emitted\n";

      std::size_t divisionCount = 0;

      guarded->guardIf.walk(
          [&](arith::DivSIOp) {
            ++divisionCount;
          });

      if (nonUnimodular) {
        if (divisionCount != 2)
          { std::cerr << "3a2 DEBUG: failure at line " << __LINE__ << ", caseNumber=" << caseNumber << "\n"; allCasesPassed = false; }
      } else {
        if (divisionCount != 0)
          { std::cerr << "3a2 DEBUG: failure at line " << __LINE__ << ", caseNumber=" << caseNumber << "\n"; allCasesPassed = false; }
      }

      if (!allCasesPassed)
        break;

      if (failed(mlir::verify(module))) {
        { std::cerr << "3a2 DEBUG: failure at line " << __LINE__ << ", caseNumber=" << caseNumber << "\n"; allCasesPassed = false; }
        break;
      }



      // --------------------------------------------------
      // Phase 5.4.3d-3b-1b:
      //
      // Execute the generated A/B/C Access SSA using
      // the actual affine.for / scf.if interpreter.
      //
      // Compare every accepted logical point against
      // an independent BigInt source-space oracle.
      // --------------------------------------------------

      auto accessExecution =
          systolic::logical::testing::interpretGuardedLoopNest(
              guarded->outermost,
              guarded->guardIf,
              guarded->logicalIVs,
              recovered->sourceIVs,
              accessA->indices,
              accessB->indices,
              accessC->indices);

      if (!accessExecution ||
          accessExecution->visitedPoints != 28 ||
          accessExecution->acceptedPoints != 16 ||
          accessExecution->accessByLogical.size() != 16) {

        std::cerr
            << "3b1b: access interpreter/count failure"
            << ", case=" << caseNumber << "\n";

        allCasesPassed = false;
        break;
      }

      using AccessPoint =
          systolic::logical::testing::Point;

      std::map<AccessPoint, std::vector<AccessPoint>>
          expectedAccesses;

      auto toI64 = [](const BigInt &value)
          -> std::optional<int64_t> {

        if (value <
                BigInt(
                    std::numeric_limits<int64_t>::min()) ||
            value >
                BigInt(
                    std::numeric_limits<int64_t>::max()))
          return std::nullopt;

        return value.convert_to<int64_t>();
      };

      auto referenceAccess =
          [&](const std::vector<std::vector<BigInt>> &rows,
              const std::vector<BigInt> &offsets,
              const IntRow &sourceCoordinates)
              -> std::optional<AccessPoint> {

        if (rows.size() != offsets.size())
          return std::nullopt;

        AccessPoint result;

        for (std::size_t r = 0; r < rows.size(); ++r) {

          if (rows[r].size() != sourceCoordinates.size())
            return std::nullopt;

          BigInt address = offsets[r];

          for (std::size_t j = 0;
               j < sourceCoordinates.size(); ++j) {

            address +=
                rows[r][j] * sourceCoordinates[j];
          }

          auto coordinate = toI64(address);

          if (!coordinate)
            return std::nullopt;

          result.push_back(*coordinate);
        }

        return result;
      };

      for (int64_t x0 = 0; x0 <= 3; ++x0) {
        for (int64_t x1 = 0; x1 <= 3; ++x1) {

          IntRow sourcePoint = {
              BigInt(x0),
              BigInt(x1)
          };

          auto logicalPoint =
              mapSourceIteration(
                  coordinateMap, sourcePoint);

          if (!logicalPoint ||
              logicalPoint->size() != 2) {

            allCasesPassed = false;
            break;
          }

          AccessPoint logicalKey;

          for (const BigInt &coordinate :
               *logicalPoint) {

            auto value = toI64(coordinate);

            if (!value) {
              allCasesPassed = false;
              break;
            }

            logicalKey.push_back(*value);
          }

          if (!allCasesPassed)
            break;

          auto a = referenceAccess(
              accessARows,
              {BigInt(0), BigInt(0)},
              sourcePoint);

          auto b = referenceAccess(
              accessBRows,
              {BigInt(0), BigInt(0)},
              sourcePoint);

          auto c = referenceAccess(
              accessCRows,
              {BigInt(0)},
              sourcePoint);

          if (!a || !b || !c) {
            allCasesPassed = false;
            break;
          }

          auto inserted = expectedAccesses.emplace(
              logicalKey,
              std::vector<AccessPoint>{
                  *a, *b, *c
              });

          if (!inserted.second) {
            allCasesPassed = false;
            break;
          }
        }

        if (!allCasesPassed)
          break;
      }

      if (!allCasesPassed)
        break;

      if (accessExecution->accessByLogical !=
              expectedAccesses) {

        std::cerr
            << "3b1b: Access SSA / BigInt mismatch"
            << ", case=" << caseNumber << "\n";

        for (const auto &entry : expectedAccesses) {

          auto actual =
              accessExecution->accessByLogical.find(
                  entry.first);

          if (actual == accessExecution
                            ->accessByLogical.end() ||
              actual->second != entry.second) {

            std::cerr
                << "  mismatch at logical point (";

            for (std::size_t i = 0;
                 i < entry.first.size(); ++i) {

              if (i != 0)
                std::cerr << ",";

              std::cerr << entry.first[i];
            }

            std::cerr << ")\n";
            break;
          }
        }

        allCasesPassed = false;
        break;
      }

      std::cout
          << "PASS: 3b1b "
          << (nonUnimodular
                  ? "non-unimodular"
                  : "unimodular")
          << " A/B/C Access SSA matches BigInt"
          << " at 16 source points\n";

      // --------------------------------------------------
      // Phase 5.4.3d-3a-3b-2:
      //
      // Interpret the ACTUAL emitted affine.for tree,
      // exact scf.if predicate, and recovery SSA.
      //
      // The source-domain oracle is constructed
      // independently from expected source points.
      // --------------------------------------------------

      auto endToEnd = systolic::logical::testing::interpretGuardedLoopNest(
          guarded->outermost,
          guarded->guardIf,
          guarded->logicalIVs,
          recovered->sourceIVs);

      if (!endToEnd) {
        std::cerr
            << "3a3b2: operation-tree interpreter failed"
            << ", case=" << caseNumber << "\n";
        allCasesPassed = false;
        break;
      }

      // Generated affine.for loops must visit exactly
      // 7 * 4 candidate logical coordinates.

      if (endToEnd->visitedPoints != 28 ||
          endToEnd->acceptedPoints != 16) {

        std::cerr
            << "3a3b2: incorrect loop/guard counts"
            << ", case=" << caseNumber
            << ", visited=" << endToEnd->visitedPoints
            << ", accepted=" << endToEnd->acceptedPoints
            << "\n";

        allCasesPassed = false;
        break;
      }

      // Construct an independent expected map from
      // the original SOURCE domain, not from the
      // normalized logical domain or guard predicate.

      systolic::logical::testing::ExecutionCounts expectedLogicalCounts;
      systolic::logical::testing::ExecutionCounts expectedSourceCounts;

      for (int64_t x0 = 0; x0 <= 3; ++x0) {
        for (int64_t x1 = 0; x1 <= 3; ++x1) {

          IntRow sourcePoint = {
              BigInt(x0),
              BigInt(x1)
          };

          auto logicalPoint = mapSourceIteration(
              coordinateMap, sourcePoint);

          if (!logicalPoint ||
              logicalPoint->size() != 2) {
            allCasesPassed = false;
            break;
          }

          systolic::logical::testing::Point logicalKey;
          bool representable = true;

          for (const BigInt &coordinate :
               *logicalPoint) {

            if (coordinate <
                    BigInt(
                        std::numeric_limits<int64_t>::min()) ||
                coordinate >
                    BigInt(
                        std::numeric_limits<int64_t>::max())) {
              representable = false;
              break;
            }

            logicalKey.push_back(
                coordinate.convert_to<int64_t>());
          }

          if (!representable) {
            allCasesPassed = false;
            break;
          }

          ++expectedLogicalCounts[logicalKey];
          ++expectedSourceCounts[
              systolic::logical::testing::Point{x0, x1}];
        }

        if (!allCasesPassed)
          break;
      }

      if (!allCasesPassed)
        break;

      // Exact equality of maps establishes:
      //
      // - no missing logical/source points;
      // - no unexpected logical/source points;
      // - correct multiplicities;
      // - each expected source point executed once.

      if (endToEnd->counts != expectedLogicalCounts ||
          endToEnd->sourceCounts != expectedSourceCounts ||
          endToEnd->counts.size() != 16 ||
          endToEnd->sourceCounts.size() != 16) {

        std::cerr
            << "3a3b2: exactly-once mismatch"
            << ", case=" << caseNumber
            << ", logicalKeys="
            << endToEnd->counts.size()
            << ", sourceKeys="
            << endToEnd->sourceCounts.size()
            << "\n";

        allCasesPassed = false;
        break;
      }

      std::cout
          << "PASS: 3a3b2 "
          << (nonUnimodular
                  ? "non-unimodular"
                  : "unimodular")
          << " actual loop/guard/recovery exactly once"
          << " (28 visited, 16 accepted)\n";

      // --------------------------------------------------
      // Phase 5.4.3d-3a-3a:
      // Evaluate the emitted recovery SSA expression DAG.
      //
      // This is arithmetic SSA evaluation, not yet
      // end-to-end interpretation of affine.for/scf.if.
      // --------------------------------------------------

      std::size_t checkedPoints = 0;
      std::size_t outsidePoints = 0;

      for (int64_t z0 = bounds[0].lower;
           z0 <= bounds[0].upper; ++z0) {
        for (int64_t z1 = bounds[1].lower;
             z1 <= bounds[1].upper; ++z1) {

          IntRow logical = {
              BigInt(z0), BigInt(z1)
          };

          // Independent arbitrary-precision oracle.
          auto expected = recoverSourceIteration(
              coordinateMap, logical);

          bool inSourceDomain =
              expected.has_value();

          if (expected) {
            for (const BigInt &coordinate : *expected) {
              if (coordinate < 0 || coordinate > 3)
                inSourceDomain = false;
            }
          }


          // --------------------------------------------------
          // Phase 5.4.3d-3a-3b-1:
          // Evaluate the ACTUAL emitted Guard SSA.
          //
          // The BigInt oracle must not decide whether the
          // generated scf.if predicate is true.
          //
          // Arithmetic is checked, not allowed to wrap.
          // This is valid for the already preflighted
          // arithmetic ranges in these test cases.
          // --------------------------------------------------

          std::function<std::optional<int64_t>(Value)>
              evaluateGuard;

          evaluateGuard = [&](Value value)
              -> std::optional<int64_t> {

            if (value == guarded->logicalIVs[0])
              return z0;

            if (value == guarded->logicalIVs[1])
              return z1;

            if (auto cast =
                    value.getDefiningOp<arith::IndexCastOp>())
              return evaluateGuard(cast.getIn());

            if (auto constant =
                    value.getDefiningOp<arith::ConstantOp>()) {
              auto attr = llvm::dyn_cast<IntegerAttr>(
                  constant.getValue());

              if (!attr)
                return std::nullopt;

              const llvm::APInt integer = attr.getValue();

              if (!integer.isSignedIntN(64))
                return std::nullopt;

              return integer.getSExtValue();
            }

            if (auto add =
                    value.getDefiningOp<arith::AddIOp>()) {
              auto lhs = evaluateGuard(add.getLhs());
              auto rhs = evaluateGuard(add.getRhs());

              if (!lhs || !rhs)
                return std::nullopt;

              __int128 result =
                  static_cast<__int128>(*lhs) + *rhs;

              if (result <
                      std::numeric_limits<int64_t>::min() ||
                  result >
                      std::numeric_limits<int64_t>::max())
                return std::nullopt;

              return static_cast<int64_t>(result);
            }

            if (auto mul =
                    value.getDefiningOp<arith::MulIOp>()) {
              auto lhs = evaluateGuard(mul.getLhs());
              auto rhs = evaluateGuard(mul.getRhs());

              if (!lhs || !rhs)
                return std::nullopt;

              __int128 result =
                  static_cast<__int128>(*lhs) * *rhs;

              if (result <
                      std::numeric_limits<int64_t>::min() ||
                  result >
                      std::numeric_limits<int64_t>::max())
                return std::nullopt;

              return static_cast<int64_t>(result);
            }

            if (auto rem =
                    value.getDefiningOp<arith::RemSIOp>()) {
              auto lhs = evaluateGuard(rem.getLhs());
              auto rhs = evaluateGuard(rem.getRhs());

              if (!lhs || !rhs || *rhs == 0)
                return std::nullopt;

              if (*lhs ==
                      std::numeric_limits<int64_t>::min() &&
                  *rhs == -1)
                return std::nullopt;

              return *lhs % *rhs;
            }

            if (auto div =
                    value.getDefiningOp<arith::DivSIOp>()) {
              auto lhs = evaluateGuard(div.getLhs());
              auto rhs = evaluateGuard(div.getRhs());

              if (!lhs || !rhs || *rhs == 0)
                return std::nullopt;

              if (*lhs ==
                      std::numeric_limits<int64_t>::min() &&
                  *rhs == -1)
                return std::nullopt;

              return *lhs / *rhs;
            }

            if (auto floor =
                    value.getDefiningOp<arith::FloorDivSIOp>()) {
              auto lhs = evaluateGuard(floor.getLhs());
              auto rhs = evaluateGuard(floor.getRhs());

              if (!lhs || !rhs || *rhs == 0)
                return std::nullopt;

              __int128 a = *lhs;
              __int128 b = *rhs;

              __int128 q = a / b;
              __int128 r = a % b;

              if (r != 0 &&
                  ((r > 0) != (b > 0)))
                --q;

              if (q <
                      std::numeric_limits<int64_t>::min() ||
                  q >
                      std::numeric_limits<int64_t>::max())
                return std::nullopt;

              return static_cast<int64_t>(q);
            }

            if (auto cmp =
                    value.getDefiningOp<arith::CmpIOp>()) {
              auto lhs = evaluateGuard(cmp.getLhs());
              auto rhs = evaluateGuard(cmp.getRhs());

              if (!lhs || !rhs)
                return std::nullopt;

              switch (cmp.getPredicate()) {
              case arith::CmpIPredicate::eq:
                return *lhs == *rhs ? 1 : 0;
              case arith::CmpIPredicate::ne:
                return *lhs != *rhs ? 1 : 0;
              case arith::CmpIPredicate::sge:
                return *lhs >= *rhs ? 1 : 0;
              case arith::CmpIPredicate::sgt:
                return *lhs > *rhs ? 1 : 0;
              case arith::CmpIPredicate::sle:
                return *lhs <= *rhs ? 1 : 0;
              case arith::CmpIPredicate::slt:
                return *lhs < *rhs ? 1 : 0;
              default:
                return std::nullopt;
              }
            }

            if (auto op =
                    value.getDefiningOp<arith::AndIOp>()) {
              auto lhs = evaluateGuard(op.getLhs());
              auto rhs = evaluateGuard(op.getRhs());

              if (!lhs || !rhs)
                return std::nullopt;

              return *lhs & *rhs;
            }

            if (auto op =
                    value.getDefiningOp<arith::OrIOp>()) {
              auto lhs = evaluateGuard(op.getLhs());
              auto rhs = evaluateGuard(op.getRhs());

              if (!lhs || !rhs)
                return std::nullopt;

              return *lhs | *rhs;
            }

            return std::nullopt;
          };

          auto guardResult = evaluateGuard(
              guarded->guardIf.getCondition());

          if (!guardResult ||
              (*guardResult != 0 && *guardResult != 1)) {
            std::cerr
                << "3a3b1: guard SSA evaluation failed"
                << ", case=" << caseNumber
                << ", z=(" << z0 << "," << z1 << ")\n";

            allCasesPassed = false;
            break;
          }

          const bool guardAccepted = *guardResult == 1;

          // Independent reference:
          // exact integer recovery + source bounds.
          //
          // The reference is used ONLY to check the
          // emitted predicate, not to compute it.

          if (guardAccepted != inSourceDomain) {
            std::cerr
                << "3a3b1: guard/oracle mismatch"
                << ", case=" << caseNumber
                << ", z=(" << z0 << "," << z1 << ")"
                << ", guard=" << guardAccepted
                << ", oracle=" << inSourceDomain
                << "\n";

            allCasesPassed = false;
            break;
          }

          // Model scf.if entry using the emitted SSA
          // predicate, not the BigInt oracle.

          if (!guardAccepted) {
            ++outsidePoints;
            continue;
          }

          // Evaluate actual MLIR SSA definitions.
          //
          // Every sourceIV is a Value whose defining
          // operation was emitted by emitSourceIVRecovery.
          //
          // Evaluation is deliberately restricted to
          // the exact arithmetic subset this phase emits.

          std::function<std::optional<int64_t>(
              Value)> evaluate;

          evaluate = [&](Value value)
              -> std::optional<int64_t> {

            // Logical affine induction variables:
            // substitute the current test coordinates.

            if (value == guarded->logicalIVs[0])
              return z0;

            if (value == guarded->logicalIVs[1])
              return z1;

            if (auto cast =
                    value.getDefiningOp<
                        arith::IndexCastOp>()) {
              return evaluate(cast.getIn());
            }

            if (auto constant =
                    value.getDefiningOp<
                        arith::ConstantOp>()) {

              auto integerAttr =
                  llvm::dyn_cast<IntegerAttr>(
                      constant.getValue());

              if (!integerAttr)
                return std::nullopt;

              const llvm::APInt integer =
                  integerAttr.getValue();

              if (!integer.isSignedIntN(64))
                return std::nullopt;

              return integer.getSExtValue();
            }

            if (auto multiply =
                    value.getDefiningOp<
                        arith::MulIOp>()) {

              auto lhs = evaluate(multiply.getLhs());
              auto rhs = evaluate(multiply.getRhs());

              if (!lhs || !rhs)
                return std::nullopt;

              __int128 product =
                  static_cast<__int128>(*lhs) *
                  static_cast<__int128>(*rhs);

              if (product <
                      std::numeric_limits<int64_t>::min() ||
                  product >
                      std::numeric_limits<int64_t>::max())
                return std::nullopt;

              return static_cast<int64_t>(product);
            }

            if (auto addition =
                    value.getDefiningOp<
                        arith::AddIOp>()) {

              auto lhs = evaluate(addition.getLhs());
              auto rhs = evaluate(addition.getRhs());

              if (!lhs || !rhs)
                return std::nullopt;

              __int128 sum =
                  static_cast<__int128>(*lhs) +
                  static_cast<__int128>(*rhs);

              if (sum <
                      std::numeric_limits<int64_t>::min() ||
                  sum >
                      std::numeric_limits<int64_t>::max())
                return std::nullopt;

              return static_cast<int64_t>(sum);
            }

            if (auto division =
                    value.getDefiningOp<
                        arith::DivSIOp>()) {

              auto lhs = evaluate(division.getLhs());
              auto rhs = evaluate(division.getRhs());

              if (!lhs || !rhs || *rhs == 0)
                return std::nullopt;

              if (*lhs ==
                      std::numeric_limits<int64_t>::min() &&
                  *rhs == -1)
                return std::nullopt;

              return *lhs / *rhs;
            }

            // Unsupported operation or SSA value.
            return std::nullopt;
          };

          IntRow actual;
          bool evaluationSucceeded = true;

          for (Value sourceIV : recovered->sourceIVs) {

            auto value = evaluate(sourceIV);

            if (!value) {
              evaluationSucceeded = false;
              break;
            }

            actual.push_back(BigInt(*value));
          }

          if (!evaluationSucceeded ||
              actual != *expected) {

            std::cerr
                << "3a3a SSA mismatch: case="
                << caseNumber
                << ", z=(" << z0 << "," << z1 << ")"
                << "\n";

            allCasesPassed = false;
            break;
          }

          // Independent forward-map round trip.
          auto reconstructedLogical =
              mapSourceIteration(
                  coordinateMap, actual);

          if (!reconstructedLogical ||
              *reconstructedLogical != logical) {

            std::cerr
                << "3a3a round-trip mismatch: case="
                << caseNumber
                << ", z=(" << z0 << "," << z1 << ")"
                << "\n";

            allCasesPassed = false;
            break;
          }

          ++checkedPoints;
        }

        if (!allCasesPassed)
          break;
      }

      if (!allCasesPassed)
        break;

      if (checkedPoints != 16 ||
          outsidePoints != 12) {

        std::cerr
            << "3a3a unexpected point counts: case="
            << caseNumber
            << ", checked=" << checkedPoints
            << ", outside=" << outsidePoints
            << "\n";

        allCasesPassed = false;
        break;
      }

      std::cout
          << "PASS: 3a3a "
          << (nonUnimodular
                  ? "non-unimodular"
                  : "unimodular")
          << " emitted SSA matches BigInt at "
          << checkedPoints
          << " source points\n";

    }

    if (verify(
            allCasesPassed,
            "3a2 source recovery SSA emitted for both maps"))
      return 1;

    // A malformed coordinate map must fail preflight
    // without inserting recovery operations.

    PartitionCoordinateMap malformed;

    malformed.sourceToLogical = {
        {BigInt(1), BigInt(0)},
        {BigInt(0), BigInt(1)}
    };

    malformed.logicalToSource = {
        {Rat(2), Rat(0)},
        {Rat(0), Rat(1)}
    };

    if (verify(
            !buildSourceIVRecoveryPlan(
                malformed, {{0, 3}, {0, 3}}),
            "3a2 malformed inverse rejected before emission"))
      return 1;
  }

  std::cout << "All coordinate relation tests passed.\n";
  return 0;
}
