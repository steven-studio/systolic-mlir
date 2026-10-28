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
#include "Systolic/RankPartitionGuardedMacEmission.h"
#include "Systolic/RankPartitionGuardedMemoryEmission.h"
#include "Systolic/RankPartitionRecognizedMacAdapter.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/IR/OwningOpRef.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Pass/PassRegistry.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/Diagnostics.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/Support/raw_ostream.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <map>
#include <tuple>
using namespace systolic::logical;
// Phase 5.4.3d-3b-5: the production GEMM pass is registered through a
// separate translation unit (this file must not see mlir::systolic).
namespace systolic::logical::testing {
void registerProductionGemmPass();
}
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

  // --------------------------------------------------------
  // Phase 5.4.3d-3b-1b (completion):
  //
  // Execute the A/B/C Access SSA with nonzero offsets and
  // negative coefficients, for the unimodular and the
  // non-unimodular map, through the existing chain and the
  // existing interpreter only, against the BigInt source
  // oracle. Then refuse malformed access functions without
  // residual IR.
  // --------------------------------------------------------
  {
    using namespace mlir;
    using namespace mlir::presburger;
    using namespace ::systolic::logical;
    namespace exec = ::systolic::logical::testing;

    for (int caseNumber = 0; caseNumber < 2; ++caseNumber) {

      const bool nonUnimodular = caseNumber == 1;

      const char *label =
          nonUnimodular ? "non-unimodular" : "unimodular";

      bool ok = true;

      auto fail = [&](const char *what) {
        std::cerr << "3b1b completion (" << label << "): "
                  << what << "\n";
        ok = false;
      };

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

      auto inverse = domain_detail::invertIntegerMatrix(
          coordinateMap.sourceToLogical);

      if (!inverse) {
        fail("inverse");
      } else {
        coordinateMap.logicalToSource = *inverse;
      }

      // Source domain 0 <= x0, x1 <= 3.

      AffineIntegerDomain sourceInequalities;
      sourceInequalities.sourceDimension = 2;
      sourceInequalities.inequalities = {
          {{BigInt(-1), BigInt(0)}, BigInt(0)},
          {{BigInt(1), BigInt(0)}, BigInt(3)},
          {{BigInt(0), BigInt(-1)}, BigInt(0)},
          {{BigInt(0), BigInt(1)}, BigInt(3)}
      };

      const std::vector<ClosedIntegerInterval> bounds =
          {{0, 6}, {0, 3}};

      std::optional<SymbolicIterationPlan> plan;

      if (ok) {
        auto normalized = buildRecoverableLogicalDomain(
            coordinateMap, sourceInequalities);

        if (!normalized)
          fail("normalization");
        else
          plan = buildSymbolicIterationPlan(*normalized, bounds);

        if (ok && !plan)
          fail("plan");
      }

      // A[x0 + 1][x1], B[3 - x1][x0], C[3 - x0 + x1]:
      // nonzero offsets and negative coefficients.

      const std::array<std::vector<std::vector<BigInt>>, 3> rows = {
          std::vector<std::vector<BigInt>>{
              {BigInt(1), BigInt(0)}, {BigInt(0), BigInt(1)}},
          std::vector<std::vector<BigInt>>{
              {BigInt(0), BigInt(-1)}, {BigInt(1), BigInt(0)}},
          std::vector<std::vector<BigInt>>{
              {BigInt(-1), BigInt(1)}}};

      const std::array<std::vector<BigInt>, 3> offsets = {
          std::vector<BigInt>{BigInt(1), BigInt(0)},
          std::vector<BigInt>{BigInt(3), BigInt(0)},
          std::vector<BigInt>{BigInt(3)}};

      MLIRContext context;

      context.loadDialect<
          affine::AffineDialect,
          arith::ArithDialect,
          scf::SCFDialect,
          func::FuncDialect>();

      OpBuilder builder(&context);
      Location loc = builder.getUnknownLoc();

      auto module = ModuleOp::create(loc);

      builder.setInsertionPointToStart(module.getBody());

      auto function = func::FuncOp::create(
          builder, loc, "access_completion",
          builder.getFunctionType({}, {}));

      Block *entry = function.addEntryBlock();

      builder.setInsertionPointToStart(entry);

      func::ReturnOp::create(builder, loc);

      builder.setInsertionPoint(entry->getTerminator());

      std::optional<EmittedGuardedAffineLoopNest> guarded;
      std::optional<EmittedSourceIVRecovery> recovered;
      std::array<std::optional<EmittedSourceAccess>, 3> accesses;

      if (ok) {
        guarded = emitGuardedAffineLoopNest(builder, loc, *plan);

        if (!guarded)
          fail("guarded nest");
      }

      if (ok) {
        recovered = emitSourceIVRecovery(
            builder, loc, coordinateMap, bounds, *guarded);

        if (!recovered)
          fail("recovery");
      }

      for (unsigned i = 0; ok && i < 3; ++i) {
        accesses[i] = emitSourceAffineAccess(
            builder, loc, coordinateMap, bounds,
            *guarded, *recovered, rows[i], offsets[i]);

        if (!accesses[i])
          fail("access emission");
      }

      if (ok && failed(mlir::verify(module)))
        fail("verifier");

      std::optional<exec::ExecutionResult> execution;

      if (ok) {
        execution = exec::interpretGuardedLoopNest(
            guarded->outermost, guarded->guardIf,
            guarded->logicalIVs, recovered->sourceIVs,
            accesses[0]->indices, accesses[1]->indices,
            accesses[2]->indices);

        if (!execution ||
            execution->visitedPoints != 28 ||
            execution->acceptedPoints != 16 ||
            execution->accessByLogical.size() != 16)
          fail("interpreter counts");
      }

      if (ok) {
        std::map<exec::Point, std::vector<exec::Point>> expected;

        for (int64_t x0 = 0; x0 <= 3 && ok; ++x0) {
          for (int64_t x1 = 0; x1 <= 3 && ok; ++x1) {
            const IntRow x = {BigInt(x0), BigInt(x1)};

            auto logical = mapSourceIteration(coordinateMap, x);

            if (!logical || logical->size() != 2) {
              fail("forward map");
              break;
            }

            exec::Point key;

            for (const BigInt &coordinate : *logical)
              key.push_back(coordinate.convert_to<int64_t>());

            std::vector<exec::Point> indices;

            for (unsigned i = 0; i < 3; ++i) {
              exec::Point point;

              for (std::size_t r = 0; r < rows[i].size(); ++r) {
                BigInt address = offsets[i][r];

                for (std::size_t j = 0; j < 2; ++j)
                  address += rows[i][r][j] * x[j];

                point.push_back(address.convert_to<int64_t>());
              }

              indices.push_back(std::move(point));
            }

            if (!expected.emplace(key, std::move(indices)).second)
              fail("duplicate logical point");
          }
        }

        if (ok && execution->accessByLogical != expected)
          fail("access index mismatch against BigInt oracle");
      }

      if (ok) {
        Block *thenBlock =
            &guarded->guardIf.getThenRegion().front();

        const auto before =
            std::distance(thenBlock->begin(), thenBlock->end());

        // Two rows, one offset.
        auto sizeMismatch = emitSourceAffineAccess(
            builder, loc, coordinateMap, bounds,
            *guarded, *recovered, rows[0], {BigInt(0)});

        // A row with three coefficients.
        auto rankMismatch = emitSourceAffineAccess(
            builder, loc, coordinateMap, bounds,
            *guarded, *recovered,
            {{BigInt(1), BigInt(0), BigInt(0)}}, {BigInt(0)});

        const auto after =
            std::distance(thenBlock->begin(), thenBlock->end());

        if (sizeMismatch || rankMismatch || before != after ||
            failed(mlir::verify(module)))
          fail("malformed access not refused cleanly");
      }

      if (verify(ok, nonUnimodular
                         ? "3b1b non-unimodular access SSA with offsets and"
                           " negative coefficients matches BigInt at 16"
                           " source points; malformed accesses refused"
                         : "3b1b unimodular access SSA with offsets and"
                           " negative coefficients matches BigInt at 16"
                           " source points; malformed accesses refused"))
        return 1;
    }
  }

  // --------------------------------------------------------
  // Phase 5.4.3d-3b-2:
  // Guarded memory operations.
  //
  // The reference never evaluates emitted SSA. It
  // enumerates the SOURCE domain in source lexicographic
  // order with BigInt address arithmetic and replays the
  // statement on its own memory.
  //
  // The test-side update
  //
  //   new = 2*old + a + b
  //
  // is an arbitrary order-sensitive pure function, not a
  // MAC prototype: A holds multiples of 1000 and B values
  // below 1000, so a + b identifies the (a, b) pair and the
  // final value of an element depends on the order of its
  // updates. Every scenario with collisions additionally
  // proves that the reference evaluated in reversed source
  // order differs from the forward reference.
  // --------------------------------------------------------
  {
    using namespace mlir;
    using namespace mlir::presburger;
    using namespace ::systolic::logical;
    namespace exec = ::systolic::logical::testing;

    struct OperandSpec {
      std::vector<std::vector<BigInt>> rows;
      std::vector<BigInt> offsets;
      std::vector<int64_t> shape;
      bool isFloat = false;
      unsigned integerWidth = 64;
      unsigned floatWidth = 32; // Phase 3b-3: 16, 32 or 64
      std::vector<int64_t> strides; // empty: identity layout
    };

    struct FixtureSpec {
      std::vector<std::vector<BigInt>> T;
      std::vector<ClosedIntegerInterval> sourceBox;
      std::vector<ClosedIntegerInterval> bounds;
      std::array<OperandSpec, 3> operands;
      bool sharedAB = false;
      bool hasCollisions = false;

      // Further source constraints coefficients . x <= bound
      // cutting the box (a non-box source domain).
      std::vector<AffineInequality> extraInequalities;

      bool loadMemRefDialect = true;
    };

    struct Fixture {
      PartitionCoordinateMap map;
      AffineIntegerDomain inequalities;
      std::optional<IntegerPolyhedron> sourceDomain;
      std::optional<SymbolicIterationPlan> plan;
      MLIRContext context;
      OpBuilder builder{&context};
      Location loc = builder.getUnknownLoc();
      ModuleOp module;
      func::FuncOp function;
      Block *entry = nullptr;
      std::optional<EmittedGuardedAffineLoopNest> guarded;
      std::optional<EmittedSourceIVRecovery> recovered;
      std::array<GuardedMemoryOperand, 3> operands;
      Block *thenBlock = nullptr;
    };

    auto countOps = [](Block *block) {
      return std::distance(block->begin(), block->end());
    };

    // The source domain of a spec as inequalities over x:
    // the box plus the extra constraints.

    auto domainOf = [](const FixtureSpec &spec) {
      const std::size_t n = spec.sourceBox.size();

      AffineIntegerDomain domain;
      domain.sourceDimension = n;

      for (std::size_t i = 0; i < n; ++i) {
        IntRow lowRow(n, BigInt(0));
        IntRow highRow(n, BigInt(0));

        lowRow[i] = BigInt(-1);
        highRow[i] = BigInt(1);

        domain.inequalities.push_back(
            {lowRow, BigInt(-spec.sourceBox[i].lower)});
        domain.inequalities.push_back(
            {highRow, BigInt(spec.sourceBox[i].upper)});
      }

      for (const auto &extra : spec.extraInequalities)
        domain.inequalities.push_back(extra);

      return domain;
    };

    auto elementTypeOf = [](OpBuilder &builder,
                            const OperandSpec &spec) -> Type {
      if (spec.isFloat) {
        if (spec.floatWidth == 16)
          return builder.getF16Type();

        if (spec.floatWidth == 64)
          return builder.getF64Type();

        return builder.getF32Type();
      }

      return builder.getIntegerType(spec.integerWidth);
    };

    // Build the complete pre-3b-2 chain for one spec.

    auto buildFixture = [&](Fixture &f,
                            const FixtureSpec &spec) -> bool {
      const std::size_t n = spec.T.size();

      if (n == 0 || spec.sourceBox.size() != n ||
          spec.bounds.size() != n)
        return false;

      f.map.sourceToLogical = spec.T;

      auto inverse = domain_detail::invertIntegerMatrix(spec.T);

      if (!inverse)
        return false;

      f.map.logicalToSource = *inverse;

      IntegerPolyhedron source(
          PresburgerSpace::getSetSpace(static_cast<unsigned>(n)));

      f.inequalities = domainOf(spec);

      // coefficients . x <= bound  <=>  -coefficients . x + bound >= 0

      for (const auto &constraint : f.inequalities.inequalities) {
        if (constraint.coefficients.size() != n)
          return false;

        llvm::SmallVector<int64_t> row(n + 1, 0);

        for (std::size_t i = 0; i < n; ++i)
          row[i] = -constraint.coefficients[i].convert_to<int64_t>();

        row[n] = constraint.upperBound.convert_to<int64_t>();

        source.addInequality(row);
      }

      f.sourceDomain = source;

      auto normalized =
          buildRecoverableLogicalDomain(f.map, f.inequalities);

      if (!normalized)
        return false;

      f.plan = buildSymbolicIterationPlan(*normalized, spec.bounds);

      if (!f.plan)
        return false;

      f.context.loadDialect<
          affine::AffineDialect,
          arith::ArithDialect,
          scf::SCFDialect,
          func::FuncDialect>();

      if (spec.loadMemRefDialect)
        f.context.loadDialect<memref::MemRefDialect>();

      f.module = ModuleOp::create(f.loc);

      f.builder.setInsertionPointToStart(f.module.getBody());

      SmallVector<Type> argumentTypes;

      for (const auto &operand : spec.operands) {
        MemRefLayoutAttrInterface layout;

        if (!operand.strides.empty())
          layout = StridedLayoutAttr::get(
              &f.context, 0, operand.strides);

        argumentTypes.push_back(MemRefType::get(
            operand.shape, elementTypeOf(f.builder, operand),
            layout));
      }

      f.function = func::FuncOp::create(
          f.builder, f.loc, "guarded_memory",
          f.builder.getFunctionType(argumentTypes, {}));

      f.entry = f.function.addEntryBlock();

      f.builder.setInsertionPointToStart(f.entry);

      func::ReturnOp::create(f.builder, f.loc);

      f.builder.setInsertionPoint(f.entry->getTerminator());

      f.guarded = emitGuardedAffineLoopNest(
          f.builder, f.loc, *f.plan);

      if (!f.guarded)
        return false;

      f.recovered = emitSourceIVRecovery(
          f.builder, f.loc, f.map, spec.bounds, *f.guarded);

      if (!f.recovered)
        return false;

      f.thenBlock = &f.guarded->guardIf.getThenRegion().front();

      for (unsigned i = 0; i < 3; ++i) {
        auto access = emitSourceAffineAccess(
            f.builder, f.loc, f.map, spec.bounds,
            *f.guarded, *f.recovered,
            spec.operands[i].rows, spec.operands[i].offsets);

        if (!access)
          return false;

        Value memref = f.entry->getArgument(
            spec.sharedAB && i == 1 ? 0 : i);

        f.operands[i] = GuardedMemoryOperand{
            memref, spec.operands[i].rows,
            spec.operands[i].offsets, *access};
      }

      return true;
    };

    // Test-side update built between the two emitter
    // calls: new = 2*old + a + b.

    auto buildUpdate = [&](Fixture &f,
                           const EmittedGuardedMemoryLoads &loads)
        -> Value {
      memref::LoadOp loadA = loads.loadA;
      memref::LoadOp loadB = loads.loadB;
      memref::LoadOp loadC = loads.loadC;

      OpBuilder::InsertionGuard guard(f.builder);

      f.builder.setInsertionPoint(f.thenBlock->getTerminator());

      Value two = arith::ConstantIntOp::create(
          f.builder, f.loc, 2, 64);

      Value scaled = arith::MulIOp::create(
          f.builder, f.loc, two, loadC.getResult());

      Value sum = arith::AddIOp::create(
          f.builder, f.loc, scaled, loadA.getResult());

      return arith::AddIOp::create(
          f.builder, f.loc, sum, loadB.getResult());
    };

    // Independent reference.

    struct ReferenceEvent {
      bool isStore;
      unsigned operand;
      std::vector<int64_t> indices;
      int64_t value;

      bool operator<(const ReferenceEvent &other) const {
        return std::tie(operand, isStore, indices, value) <
               std::tie(other.operand, other.isStore,
                        other.indices, other.value);
      }

      bool operator==(const ReferenceEvent &other) const {
        return operand == other.operand &&
               isStore == other.isStore &&
               indices == other.indices &&
               value == other.value;
      }
    };

    auto bufferSize = [](const std::vector<int64_t> &shape) {
      std::size_t size = 1;

      for (int64_t extent : shape)
        size *= static_cast<std::size_t>(extent);

      return size;
    };

    auto initialBuffer = [&](const OperandSpec &spec,
                             unsigned operand) {
      exec::MemoryBuffer buffer;
      buffer.shape = spec.shape;

      const std::size_t size = bufferSize(spec.shape);

      for (std::size_t linear = 0; linear < size; ++linear) {
        int64_t value = 0;

        if (operand == 0)
          value = static_cast<int64_t>(1000 * (linear + 1));
        else if (operand == 1)
          value = static_cast<int64_t>(linear + 1);
        else
          value = static_cast<int64_t>(100 + linear);

        buffer.data.push_back(value);
      }

      return buffer;
    };

    auto linearIndex = [](const std::vector<int64_t> &shape,
                          const std::vector<int64_t> &indices)
        -> std::optional<std::size_t> {
      if (indices.size() != shape.size())
        return std::nullopt;

      std::size_t linear = 0;

      for (std::size_t d = 0; d < shape.size(); ++d) {
        if (indices[d] < 0 || indices[d] >= shape[d])
          return std::nullopt;

        linear = linear * static_cast<std::size_t>(shape[d]) +
                 static_cast<std::size_t>(indices[d]);
      }

      return linear;
    };

    // Source points in ascending (or descending)
    // lexicographic order.

    auto enumerateBox = [](const std::vector<ClosedIntegerInterval> &box,
                           bool descending) {
      std::vector<std::vector<int64_t>> points;
      std::vector<int64_t> point(box.size(), 0);

      std::function<void(std::size_t)> recurse =
          [&](std::size_t dim) {
            if (dim == box.size()) {
              points.push_back(point);
              return;
            }

            if (descending) {
              for (int64_t v = box[dim].upper;
                   v >= box[dim].lower; --v) {
                point[dim] = v;
                recurse(dim + 1);
              }
            } else {
              for (int64_t v = box[dim].lower;
                   v <= box[dim].upper; ++v) {
                point[dim] = v;
                recurse(dim + 1);
              }
            }
          };

      recurse(0);

      return points;
    };

    struct ReferenceRun {
      std::array<exec::MemoryBuffer, 3> buffers;
      std::vector<ReferenceEvent> trace;
    };

    auto referenceRun = [&](const FixtureSpec &spec,
                            bool reversed)
        -> std::optional<ReferenceRun> {
      ReferenceRun run;

      for (unsigned i = 0; i < 3; ++i)
        run.buffers[i] = initialBuffer(spec.operands[i], i);

      const AffineIntegerDomain domain = domainOf(spec);

      for (const auto &coordinates :
           enumerateBox(spec.sourceBox, reversed)) {

        IntRow x;

        for (int64_t coordinate : coordinates)
          x.emplace_back(coordinate);

        if (!containsSourcePoint(domain, x))
          continue;

        std::array<std::vector<int64_t>, 3> addresses;

        for (unsigned i = 0; i < 3; ++i) {
          const auto &operand = spec.operands[i];

          if (operand.rows.size() != operand.offsets.size())
            return std::nullopt;

          for (std::size_t r = 0; r < operand.rows.size(); ++r) {
            if (operand.rows[r].size() != x.size())
              return std::nullopt;

            BigInt address = operand.offsets[r];

            for (std::size_t j = 0; j < x.size(); ++j)
              address += operand.rows[r][j] * x[j];

            if (address <
                    BigInt(std::numeric_limits<int64_t>::min()) ||
                address >
                    BigInt(std::numeric_limits<int64_t>::max()))
              return std::nullopt;

            addresses[i].push_back(address.convert_to<int64_t>());
          }
        }

        std::array<int64_t, 3> loaded;

        for (unsigned i = 0; i < 3; ++i) {
          const unsigned buffer = spec.sharedAB && i == 1 ? 0 : i;

          auto linear = linearIndex(
              spec.operands[buffer].shape, addresses[i]);

          if (!linear)
            return std::nullopt;

          loaded[i] = run.buffers[buffer].data[*linear];

          run.trace.push_back(
              {false, buffer, addresses[i], loaded[i]});
        }

        const __int128 updated =
            static_cast<__int128>(2) * loaded[2] +
            static_cast<__int128>(loaded[0]) + loaded[1];

        if (updated < std::numeric_limits<int64_t>::min() ||
            updated > std::numeric_limits<int64_t>::max())
          return std::nullopt;

        auto linear = linearIndex(
            spec.operands[2].shape, addresses[2]);

        if (!linear)
          return std::nullopt;

        run.buffers[2].data[*linear] =
            static_cast<int64_t>(updated);

        run.trace.push_back(
            {true, 2, addresses[2], static_cast<int64_t>(updated)});
      }

      return run;
    };

    // The exact z-only image of the source domain must
    // agree with the BigInt membership oracle at every
    // point of the logical box (test-only enumeration).

    auto imageAgreesWithOracle = [&](Fixture &f,
                                     const FixtureSpec &spec,
                                     std::size_t expectedAccepted)
        -> bool {
      auto image = buildExactLogicalImage(f.map, *f.sourceDomain);

      if (!image)
        return false;

      std::size_t accepted = 0;

      for (const auto &coordinates : enumerateBox(spec.bounds, false)) {
        IntRow z;

        for (int64_t coordinate : coordinates)
          z.emplace_back(coordinate);

        const bool expected =
            containsLogicalPoint(f.map, f.inequalities, z);

        IntegerPolyhedron cut = *image;

        cut.setAndEliminate(0, llvm::ArrayRef<int64_t>(coordinates));

        const bool actual = !cut.isIntegerEmpty();

        if (actual != expected)
          return false;

        if (actual)
          ++accepted;
      }

      return accepted == expectedAccepted;
    };

    // Run one accepted scenario end to end and compare
    // with the reference.

    auto runAccepted = [&](const char *name,
                           const FixtureSpec &spec,
                           std::size_t expectedVisited,
                           std::size_t expectedAccepted) -> bool {
      Fixture f;

      if (!buildFixture(f, spec)) {
        std::cerr << "3b2 " << name << ": fixture failed\n";
        return false;
      }

      const auto before = countOps(f.thenBlock);

      auto loads = emitGuardedMemoryLoads(
          f.builder, f.loc, f.map, *f.plan, *f.sourceDomain,
          *f.guarded, *f.recovered,
          f.operands[0], f.operands[1], f.operands[2]);

      if (!loads) {
        std::cerr << "3b2 " << name << ": loads refused\n";
        return false;
      }

      // Three loads and one index cast per index.

      std::ptrdiff_t indexCount = 0;

      for (const auto &operand : f.operands)
        indexCount += static_cast<std::ptrdiff_t>(
            operand.access.indices.size());

      if (countOps(f.thenBlock) != before + 3 + indexCount) {
        std::cerr << "3b2 " << name << ": unexpected load ops\n";
        return false;
      }

      const bool isFloat = spec.operands[2].isFloat;

      for (memref::LoadOp load :
           {loads->loadA, loads->loadB, loads->loadC}) {
        if (!load || load->getBlock() != f.thenBlock ||
            (isFloat ? !load.getResult().getType().isF32()
                     : !load.getResult().getType().isInteger(64))) {
          std::cerr << "3b2 " << name << ": malformed load\n";
          return false;
        }

        for (Value index : load.getIndices()) {
          if (!index.getType().isIndex() ||
              !index.getDefiningOp<arith::IndexCastOp>() ||
              index.getDefiningOp()->getBlock() != f.thenBlock) {
            std::cerr << "3b2 " << name
                      << ": malformed load index\n";
            return false;
          }
        }
      }

      if (loads->loadA.getMemRef() != f.operands[0].memref ||
          loads->loadB.getMemRef() != f.operands[1].memref ||
          loads->loadC.getMemRef() != f.operands[2].memref) {
        std::cerr << "3b2 " << name << ": wrong memref\n";
        return false;
      }

      Value stored = isFloat ? loads->loadC.getResult()
                             : buildUpdate(f, *loads);

      const auto beforeStore = countOps(f.thenBlock);

      auto store = emitGuardedMemoryStore(
          f.builder, f.loc, f.map, *f.plan, *f.sourceDomain,
          *f.guarded, *f.recovered, f.operands[0], f.operands[1],
          f.operands[2], *loads, stored);

      if (!store || !store->store ||
          countOps(f.thenBlock) != beforeStore + 1 ||
          store->store->getBlock() != f.thenBlock ||
          store->store.getValue() != stored ||
          store->store.getMemRef() != f.operands[2].memref ||
          !llvm::equal(store->store.getIndices(),
                       loads->loadC.getIndices()) ||
          store->store->getNextNode() !=
              f.thenBlock->getTerminator()) {
        std::cerr << "3b2 " << name
                  << ": store refused or malformed\n";
        return false;
      }

      // A second store is one statement too many.

      auto secondStore = emitGuardedMemoryStore(
          f.builder, f.loc, f.map, *f.plan, *f.sourceDomain,
          *f.guarded, *f.recovered, f.operands[0], f.operands[1],
          f.operands[2], *loads, stored);

      if (secondStore || countOps(f.thenBlock) != beforeStore + 1) {
        std::cerr << "3b2 " << name << ": second store accepted\n";
        return false;
      }

      if (failed(mlir::verify(f.module))) {
        std::cerr << "3b2 " << name << ": verifier failed\n";
        return false;
      }

      if (std::getenv("RANK_PARTITION_DUMP_IR")) {
        llvm::errs() << "3b2 " << name << " IR:\n";
        f.module.print(llvm::errs());
        llvm::errs() << "\n";
      }

      if (!imageAgreesWithOracle(f, spec, expectedAccepted)) {
        std::cerr << "3b2 " << name
                  << ": exact logical image disagrees with the"
                     " BigInt oracle\n";
        return false;
      }

      if (isFloat)
        return true; // structural and verifier coverage only

      // Execute the actual operation tree with a memory
      // model, then compare against the reference.

      exec::MemoryState memory;

      for (unsigned i = 0; i < 3; ++i)
        memory.buffers[f.entry->getArgument(i)] =
            initialBuffer(spec.operands[i], i);

      auto execution = exec::interpretGuardedLoopNest(
          f.guarded->outermost, f.guarded->guardIf,
          f.guarded->logicalIVs, f.recovered->sourceIVs,
          f.operands[0].access.indices,
          f.operands[1].access.indices,
          f.operands[2].access.indices,
          &memory);

      if (!execution ||
          execution->visitedPoints != expectedVisited ||
          execution->acceptedPoints != expectedAccepted ||
          execution->accessByLogical.size() != expectedAccepted ||
          execution->sourceCounts.size() != expectedAccepted) {
        std::cerr << "3b2 " << name << ": interpreter failure\n";
        return false;
      }

      // Observed A/B/C indices at every accepted logical
      // point against the BigInt oracle.

      std::map<exec::Point, std::vector<exec::Point>> expectedAccess;

      for (const auto &coordinates : enumerateBox(spec.sourceBox, false)) {
        IntRow x;

        for (int64_t coordinate : coordinates)
          x.emplace_back(coordinate);

        if (!containsSourcePoint(f.inequalities, x))
          continue;

        auto logical = mapSourceIteration(f.map, x);

        if (!logical || logical->size() != x.size())
          return false;

        exec::Point key;

        for (const BigInt &coordinate : *logical)
          key.push_back(coordinate.convert_to<int64_t>());

        std::vector<exec::Point> indices;

        for (const auto &operand : spec.operands) {
          exec::Point point;

          for (std::size_t r = 0; r < operand.rows.size(); ++r) {
            BigInt address = operand.offsets[r];

            for (std::size_t j = 0; j < x.size(); ++j)
              address += operand.rows[r][j] * x[j];

            point.push_back(address.convert_to<int64_t>());
          }

          indices.push_back(std::move(point));
        }

        if (!expectedAccess.emplace(key, std::move(indices)).second)
          return false;
      }

      if (execution->accessByLogical != expectedAccess) {
        std::cerr << "3b2 " << name << ": access index mismatch\n";
        return false;
      }

      auto reference = referenceRun(spec, false);

      if (!reference) {
        std::cerr << "3b2 " << name << ": reference failed\n";
        return false;
      }

      // The reference itself must be order sensitive
      // exactly when the scenario has collisions.

      auto reversedReference = referenceRun(spec, true);

      if (!reversedReference ||
          (reversedReference->buffers[2].data ==
           reference->buffers[2].data) != !spec.hasCollisions) {
        std::cerr << "3b2 " << name
                  << ": reference order sensitivity check failed\n";
        return false;
      }

      // Final memory contents.

      for (unsigned i = 0; i < 3; ++i) {
        const auto &actual =
            memory.buffers[f.entry->getArgument(i)];

        if (actual.shape != reference->buffers[i].shape ||
            actual.data != reference->buffers[i].data) {
          std::cerr << "3b2 " << name
                    << ": memory contents differ, operand " << i
                    << "\n";
          return false;
        }
      }

      // Event multiset and per-element store order.

      std::vector<ReferenceEvent> observed;

      for (const auto &event : memory.trace) {
        std::optional<unsigned> operand;

        for (unsigned i = 0; i < 3; ++i)
          if (event.memref == f.entry->getArgument(i))
            operand = i;

        if (!operand)
          return false;

        observed.push_back(
            {event.isStore, *operand, event.indices, event.value});
      }

      if (observed.size() != reference->trace.size() ||
          observed.size() != 4 * expectedAccepted) {
        std::cerr << "3b2 " << name << ": event count "
                  << observed.size() << "\n";
        return false;
      }

      auto storeSequences =
          [](const std::vector<ReferenceEvent> &trace) {
            std::map<std::vector<int64_t>, std::vector<int64_t>>
                sequences;

            for (const auto &event : trace)
              if (event.isStore)
                sequences[event.indices].push_back(event.value);

            return sequences;
          };

      if (storeSequences(observed) !=
          storeSequences(reference->trace)) {
        std::cerr << "3b2 " << name
                  << ": per-element store order differs\n";
        return false;
      }

      std::vector<ReferenceEvent> sortedObserved = observed;
      std::vector<ReferenceEvent> sortedReference = reference->trace;

      std::sort(sortedObserved.begin(), sortedObserved.end());
      std::sort(sortedReference.begin(), sortedReference.end());

      if (sortedObserved != sortedReference) {
        std::cerr << "3b2 " << name << ": event multiset differs\n";
        return false;
      }

      return true;
    };

    const std::vector<std::vector<BigInt>> identityRows = {
        {BigInt(1), BigInt(0)},
        {BigInt(0), BigInt(1)}
    };

    const std::vector<std::vector<BigInt>> swapRows = {
        {BigInt(0), BigInt(1)},
        {BigInt(1), BigInt(0)}
    };

    const std::vector<BigInt> zero2 = {BigInt(0), BigInt(0)};

    const std::vector<ClosedIntegerInterval> unitBox2 =
        {{0, 3}, {0, 3}};

    const std::vector<std::vector<BigInt>> skewT = {
        {BigInt(1), BigInt(1)},
        {BigInt(0), BigInt(1)}
    };

    const std::vector<std::vector<BigInt>> stretchT = {
        {BigInt(2), BigInt(0)},
        {BigInt(0), BigInt(1)}
    };

    const std::vector<std::vector<BigInt>> swapT = {
        {BigInt(0), BigInt(1)},
        {BigInt(1), BigInt(0)}
    };

    // P1: unimodular skew; A[x0+1][x1], B[3-x1][x0],
    // C[x1] (collisions along x0, order preserved).

    FixtureSpec skewSpec{
        skewT, unitBox2, {{0, 6}, {0, 3}},
        {OperandSpec{identityRows, {BigInt(1), BigInt(0)}, {5, 4}},
         OperandSpec{{{BigInt(0), BigInt(-1)}, {BigInt(1), BigInt(0)}},
                     {BigInt(3), BigInt(0)}, {4, 4}},
         OperandSpec{{{BigInt(0), BigInt(1)}}, {BigInt(0)}, {4}}},
        false, true};

    if (verify(runAccepted("unimodular", skewSpec, 28, 16),
               "3b2 unimodular guarded loads/store match the"
               " source-order reference"))
      return 1;

    // P1': A and B read the same memref (read/read).

    {
      FixtureSpec shared = skewSpec;
      shared.sharedAB = true;
      shared.operands[1] = OperandSpec{swapRows, zero2, {5, 4}};

      if (verify(runAccepted("shared-ab", shared, 28, 16),
                 "3b2 shared A/B memref accepted and matches the"
                 " reference"))
        return 1;
    }

    // P2: non-unimodular stretch; A[x0+1][3-x1],
    // B[3-x1][x0+1], C[6-x0-x1] (anti-diagonal
    // collisions, order preserved; congruence guard and
    // divsi recovery).

    FixtureSpec stretchSpec{
        stretchT, unitBox2, {{0, 6}, {0, 3}},
        {OperandSpec{{{BigInt(1), BigInt(0)}, {BigInt(0), BigInt(-1)}},
                     {BigInt(1), BigInt(3)}, {5, 4}},
         OperandSpec{{{BigInt(0), BigInt(-1)}, {BigInt(1), BigInt(0)}},
                     {BigInt(3), BigInt(1)}, {4, 5}},
         OperandSpec{{{BigInt(-1), BigInt(-1)}}, {BigInt(6)}, {7}}},
        false, true};

    if (verify(runAccepted("non-unimodular", stretchSpec, 28, 16),
               "3b2 non-unimodular guarded loads/store match the"
               " source-order reference"))
      return 1;

    // P3: swapped loops with an injective C[3-x0][x1]:
    // no write collision, any order is legal.

    FixtureSpec swapInjectiveSpec{
        swapT, unitBox2, unitBox2,
        {OperandSpec{{{BigInt(-1), BigInt(0)}, {BigInt(0), BigInt(1)}},
                     {BigInt(3), BigInt(1)}, {4, 5}},
         OperandSpec{{{BigInt(0), BigInt(1)}, {BigInt(-1), BigInt(0)}},
                     {BigInt(0), BigInt(3)}, {4, 4}},
         OperandSpec{{{BigInt(-1), BigInt(0)}, {BigInt(0), BigInt(1)}},
                     {BigInt(3), BigInt(0)}, {4, 4}}},
        false, false};

    if (verify(runAccepted("swap-injective", swapInjectiveSpec, 16, 16),
               "3b2 reordered injective writes match the"
               " source-order reference"))
      return 1;

    // P4: unimodular T without a unit coefficient
    // (det 1); the z-only image makes the domain
    // equality decidable.

    FixtureSpec denseUnimodularSpec{
        {{BigInt(2), BigInt(3)}, {BigInt(3), BigInt(5)}},
        unitBox2, {{0, 15}, {0, 24}},
        {OperandSpec{identityRows, zero2, {4, 4}},
         OperandSpec{swapRows, zero2, {4, 4}},
         OperandSpec{{{BigInt(0), BigInt(1)}}, {BigInt(0)}, {4}}},
        false, true};

    if (verify(runAccepted("dense-unimodular", denseUnimodularSpec,
                           400, 16),
               "3b2 unimodular map without unit pivot accepted and"
               " matches the reference"))
      return 1;

    // P5: negative source and logical coordinates with
    // a negative partial sum in the C access.

    FixtureSpec negativeSpec{
        stretchT, {{-2, 1}, {2, 5}}, {{-4, 2}, {2, 5}},
        {OperandSpec{identityRows, {BigInt(2), BigInt(-2)}, {4, 4}},
         OperandSpec{swapRows, {BigInt(-2), BigInt(2)}, {4, 4}},
         OperandSpec{{{BigInt(1), BigInt(1)}}, {BigInt(0)}, {7}}},
        false, true};

    if (verify(runAccepted("negative-coordinates", negativeSpec, 28, 16),
               "3b2 negative coordinates accepted and match the"
               " reference"))
      return 1;

    // P6: non-diagonal non-unimodular T (det 2) with a
    // multi-term congruence guard.

    FixtureSpec shearedStretchSpec{
        {{BigInt(2), BigInt(1)}, {BigInt(0), BigInt(1)}},
        unitBox2, {{0, 9}, {0, 3}},
        {OperandSpec{identityRows, zero2, {4, 4}},
         OperandSpec{swapRows, zero2, {4, 4}},
         OperandSpec{{{BigInt(1), BigInt(1)}}, {BigInt(0)}, {7}}},
        false, true};

    if (verify(runAccepted("sheared-stretch", shearedStretchSpec, 40, 16),
               "3b2 non-diagonal non-unimodular map accepted and"
               " matches the reference"))
      return 1;

    // P7: three source loops, a rank-3 memref.

    FixtureSpec threeDimSpec{
        {{BigInt(1), BigInt(1), BigInt(0)},
         {BigInt(0), BigInt(1), BigInt(0)},
         {BigInt(0), BigInt(0), BigInt(1)}},
        {{0, 3}, {0, 3}, {0, 3}}, {{0, 6}, {0, 3}, {0, 3}},
        {OperandSpec{{{BigInt(1), BigInt(0), BigInt(0)},
                      {BigInt(0), BigInt(1), BigInt(0)},
                      {BigInt(0), BigInt(0), BigInt(1)}},
                     {BigInt(0), BigInt(0), BigInt(0)}, {4, 4, 4}},
         OperandSpec{{{BigInt(0), BigInt(0), BigInt(1)},
                      {BigInt(1), BigInt(0), BigInt(0)}},
                     zero2, {4, 4}},
         OperandSpec{{{BigInt(0), BigInt(1), BigInt(0)},
                      {BigInt(0), BigInt(0), BigInt(1)}},
                     zero2, {4, 4}}},
        false, true};

    if (verify(runAccepted("three-dimensional", threeDimSpec, 112, 64),
               "3b2 three-loop nest with a rank-3 memref matches the"
               " reference"))
      return 1;

    // P8: f32 element type (structural and verifier
    // coverage only; the interpreter executes i64).

    {
      FixtureSpec floatSpec = stretchSpec;

      for (auto &operand : floatSpec.operands)
        operand.isFloat = true;

      if (verify(runAccepted("f32", floatSpec, 28, 16),
                 "3b2 f32 memrefs accepted and verified"))
        return 1;
    }

    // P9: a non-box source domain, x1 <= x0 (10 points),
    // under the unimodular skew.

    {
      FixtureSpec triangle = skewSpec;
      triangle.extraInequalities = {
          {{BigInt(-1), BigInt(1)}, BigInt(0)}};

      if (verify(runAccepted("triangle", triangle, 28, 10),
                 "3b2 non-box source domain accepted and matches the"
                 " reference"))
        return 1;
    }

    // Rejections: each returns nullopt, creates no
    // operation, and leaves a verifiable module.

    auto expectLoadsRefused =
        [&](const char *name, const FixtureSpec &spec,
            std::function<void(Fixture &)> mutate) -> bool {
      Fixture f;

      if (!buildFixture(f, spec)) {
        std::cerr << "3b2 " << name << ": fixture failed\n";
        return false;
      }

      if (mutate)
        mutate(f);

      const auto before = countOps(f.thenBlock);

      auto loads = emitGuardedMemoryLoads(
          f.builder, f.loc, f.map, *f.plan, *f.sourceDomain,
          *f.guarded, *f.recovered,
          f.operands[0], f.operands[1], f.operands[2]);

      if (loads || countOps(f.thenBlock) != before ||
          failed(mlir::verify(f.module))) {
        std::cerr << "3b2 " << name << ": not refused\n";
        return false;
      }

      return true;
    };

    // R1: swapped loops with C[x0+x1]: the anti-diagonal
    // updates would be reversed. The whole statement is
    // refused before the first load exists.

    {
      FixtureSpec reversedSpec{
          swapT, unitBox2, unitBox2,
          {OperandSpec{identityRows, zero2, {4, 4}},
           OperandSpec{swapRows, zero2, {4, 4}},
           OperandSpec{{{BigInt(1), BigInt(1)}}, {BigInt(0)}, {7}}},
          false, true};

      if (verify(expectLoadsRefused("reversed-order", reversedSpec, nullptr),
                 "3b2 reversed reduction order refused before any"
                 " load (no speculative reordering)"))
        return 1;
    }

    FixtureSpec baseSpec = stretchSpec;

    // R2: rank mismatch (one C row for a rank-2 memref).

    {
      FixtureSpec spec = baseSpec;
      spec.operands[2].shape = {7, 7};

      if (verify(expectLoadsRefused("rank", spec, nullptr),
                 "3b2 access rank mismatch refused"))
        return 1;
    }

    // R3: out of bounds: high, tight (f == shape), low.

    {
      FixtureSpec spec = baseSpec;
      spec.operands[0].shape = {3, 4};

      if (verify(expectLoadsRefused("oob-high", spec, nullptr),
                 "3b2 out-of-bounds (high) access refused"))
        return 1;

      spec = baseSpec;
      spec.operands[0].shape = {4, 4}; // x0 + 1 reaches 4

      if (verify(expectLoadsRefused("oob-tight", spec, nullptr),
                 "3b2 out-of-bounds (f == extent) access refused"))
        return 1;

      spec = baseSpec;
      spec.operands[0].offsets = {BigInt(-1), BigInt(3)};

      if (verify(expectLoadsRefused("oob-low", spec, nullptr),
                 "3b2 out-of-bounds (low) access refused"))
        return 1;
    }

    // R4: dynamic shape.

    {
      FixtureSpec spec = baseSpec;
      spec.operands[0].shape = {ShapedType::kDynamic, 4};

      if (verify(expectLoadsRefused("dynamic", spec, nullptr),
                 "3b2 dynamic memref shape refused"))
        return 1;
    }

    // R5: element type mismatch.

    {
      FixtureSpec spec = baseSpec;
      spec.operands[0].integerWidth = 32;

      if (verify(expectLoadsRefused("element-type", spec, nullptr),
                 "3b2 element type mismatch refused"))
        return 1;
    }

    // R20: the MemRef dialect is not loaded.

    {
      FixtureSpec spec = baseSpec;
      spec.loadMemRefDialect = false;

      if (verify(expectLoadsRefused("memref-dialect", spec, nullptr),
                 "3b2 unloaded MemRef dialect refused"))
        return 1;
    }

    // R13: non-identity layout (strided<[1, 1]> maps
    // distinct indices to one element).

    {
      FixtureSpec spec = swapInjectiveSpec;
      spec.operands[2].strides = {1, 1};

      if (verify(expectLoadsRefused("layout", spec, nullptr),
                 "3b2 non-identity memref layout refused"))
        return 1;
    }

    // R6: A and C are the same memref value, or views of
    // the same buffer.

    if (verify(expectLoadsRefused(
                   "alias", baseSpec,
                   [](Fixture &f) {
                     f.operands[0].memref = f.operands[2].memref;
                   }),
               "3b2 provable A/C aliasing refused"))
      return 1;

    if (verify(expectLoadsRefused(
                   "alias-view", baseSpec,
                   [](Fixture &f) {
                     OpBuilder::InsertionGuard guard(f.builder);
                     f.builder.setInsertionPoint(f.guarded->outermost);
                     Value view = memref::CastOp::create(
                         f.builder, f.loc,
                         f.operands[2].memref.getType(),
                         f.operands[2].memref);
                     f.operands[1].memref = view;
                     // The view has C's shape; reuse C's rows.
                     f.operands[1].accessRows = f.operands[2].accessRows;
                     f.operands[1].accessOffsets =
                         f.operands[2].accessOffsets;
                     f.operands[1].access = f.operands[2].access;
                   }),
               "3b2 B/C aliasing through a view refused"))
      return 1;

    // R9: foreign memory effects inside the guard.

    if (verify(expectLoadsRefused(
                   "foreign-store", baseSpec,
                   [](Fixture &f) {
                     OpBuilder::InsertionGuard guard(f.builder);
                     f.builder.setInsertionPoint(
                         f.thenBlock->getTerminator());
                     Value zero = arith::ConstantIndexOp::create(
                         f.builder, f.loc, 0);
                     Value value = arith::ConstantIntOp::create(
                         f.builder, f.loc, 1, 64);
                     memref::StoreOp::create(
                         f.builder, f.loc, value,
                         f.operands[2].memref, ValueRange{zero});
                   }),
               "3b2 foreign memory store refused"))
      return 1;

    if (verify(expectLoadsRefused(
                   "foreign-load", baseSpec,
                   [](Fixture &f) {
                     OpBuilder::InsertionGuard guard(f.builder);
                     f.builder.setInsertionPoint(
                         f.thenBlock->getTerminator());
                     Value zero = arith::ConstantIndexOp::create(
                         f.builder, f.loc, 0);
                     memref::LoadOp::create(
                         f.builder, f.loc, f.operands[2].memref,
                         ValueRange{zero});
                   }),
               "3b2 foreign memory load refused"))
      return 1;

    if (verify(expectLoadsRefused(
                   "foreign-alloc", baseSpec,
                   [](Fixture &f) {
                     OpBuilder::InsertionGuard guard(f.builder);
                     f.builder.setInsertionPoint(
                         f.thenBlock->getTerminator());
                     memref::AllocOp::create(
                         f.builder, f.loc,
                         MemRefType::get({1}, f.builder.getI64Type()));
                   }),
               "3b2 foreign allocation refused"))
      return 1;

    // R11: a source domain inconsistent with the plan.

    if (verify(expectLoadsRefused(
                   "domain", baseSpec,
                   [](Fixture &f) {
                     IntegerPolyhedron smaller(
                         PresburgerSpace::getSetSpace(2));
                     smaller.addInequality(
                         llvm::SmallVector<int64_t>{1, 0, 0});
                     smaller.addInequality(
                         llvm::SmallVector<int64_t>{-1, 0, 2});
                     smaller.addInequality(
                         llvm::SmallVector<int64_t>{0, 1, 0});
                     smaller.addInequality(
                         llvm::SmallVector<int64_t>{0, -1, 2});
                     f.sourceDomain = smaller;
                   }),
               "3b2 source domain inconsistent with the plan refused"))
      return 1;

    // R12: access rows that differ from the emitted SSA.

    if (verify(expectLoadsRefused(
                   "stale-rows", baseSpec,
                   [](Fixture &f) {
                     f.operands[0].accessOffsets = {BigInt(0), BigInt(3)};
                   }),
               "3b2 access rows differing from the emitted SSA refused"))
      return 1;

    // R15 / R16: tampered loop-nest or recovery handles.

    {
      Fixture f;

      bool ok = buildFixture(f, baseSpec);

      if (ok) {
        const auto before = countOps(f.thenBlock);

        EmittedGuardedAffineLoopNest tamperedNest = *f.guarded;
        std::swap(tamperedNest.logicalIVs[0], tamperedNest.logicalIVs[1]);

        auto nestLoads = emitGuardedMemoryLoads(
            f.builder, f.loc, f.map, *f.plan, *f.sourceDomain,
            tamperedNest, *f.recovered,
            f.operands[0], f.operands[1], f.operands[2]);

        EmittedSourceIVRecovery tamperedRecovery = *f.recovered;
        std::swap(tamperedRecovery.sourceIVs[0],
                  tamperedRecovery.sourceIVs[1]);

        auto recoveryLoads = emitGuardedMemoryLoads(
            f.builder, f.loc, f.map, *f.plan, *f.sourceDomain,
            *f.guarded, tamperedRecovery,
            f.operands[0], f.operands[1], f.operands[2]);

        EmittedGuardedAffineLoopNest nulledNest = *f.guarded;
        nulledNest.logicalIVs[1] = Value();

        auto nulledLoads = emitGuardedMemoryLoads(
            f.builder, f.loc, f.map, *f.plan, *f.sourceDomain,
            nulledNest, *f.recovered,
            f.operands[0], f.operands[1], f.operands[2]);

        ok = !nestLoads && !recoveryLoads && !nulledLoads &&
             countOps(f.thenBlock) == before &&
             succeeded(mlir::verify(f.module));
      }

      if (verify(ok, "3b2 tampered loop-nest and recovery handles refused"))
        return 1;
    }

    // R23: a tampered loads struct is refused by the
    // store call.

    {
      Fixture f;

      bool ok = buildFixture(f, baseSpec);

      std::optional<EmittedGuardedMemoryLoads> loads;

      if (ok) {
        loads = emitGuardedMemoryLoads(
            f.builder, f.loc, f.map, *f.plan, *f.sourceDomain,
            *f.guarded, *f.recovered,
            f.operands[0], f.operands[1], f.operands[2]);
        ok = loads.has_value();
      }

      if (ok) {
        Value stored = buildUpdate(f, *loads);

        const auto before = countOps(f.thenBlock);

        EmittedGuardedMemoryLoads swapped = *loads;
        swapped.loadC = loads->loadA;

        EmittedGuardedMemoryLoads reindexed = *loads;
        reindexed.indicesC = loads->indicesB;

        EmittedGuardedMemoryLoads nulled = *loads;
        nulled.indicesC[0] = Value();

        for (const EmittedGuardedMemoryLoads *tampered :
             {&swapped, &reindexed, &nulled}) {
          auto store = emitGuardedMemoryStore(
              f.builder, f.loc, f.map, *f.plan, *f.sourceDomain,
              *f.guarded, *f.recovered, f.operands[0], f.operands[1],
              f.operands[2], *tampered, stored);

          if (store)
            ok = false;
        }

        ok = ok && countOps(f.thenBlock) == before &&
             succeeded(mlir::verify(f.module));
      }

      if (verify(ok, "3b2 tampered loads struct refused by the store"))
        return 1;
    }

    // R14 / R18: a plan whose box does not cover the
    // domain, and a two-disjunct plan.

    {
      Fixture f;

      bool ok = buildFixture(f, skewSpec);

      if (ok) {
        const auto before = countOps(f.thenBlock);

        SymbolicIterationPlan narrow(
            f.plan->logicalDomain, {{0, 5}, {0, 3}});

        auto narrowLoads = emitGuardedMemoryLoads(
            f.builder, f.loc, f.map, narrow, *f.sourceDomain,
            *f.guarded, *f.recovered,
            f.operands[0], f.operands[1], f.operands[2]);

        // The same point set described by two disjuncts:
        // P and its slice z1 <= 1. unionInPlace(const
        // IntegerRelation &) appends unconditionally; the
        // slice keeps the second disjunct a different
        // description of points already in P.

        IntegerRelation slice = f.plan->logicalDomain.getDisjunct(0);

        llvm::SmallVector<int64_t> sliceRow(slice.getNumCols(), 0);
        sliceRow[1] = -1;
        sliceRow.back() = 1;
        slice.addInequality(sliceRow);

        PresburgerSet twoDisjuncts = f.plan->logicalDomain;
        twoDisjuncts.unionInPlace(slice);

        SymbolicIterationPlan doubled(twoDisjuncts, f.plan->bounds);

        auto doubledLoads = emitGuardedMemoryLoads(
            f.builder, f.loc, f.map, doubled, *f.sourceDomain,
            *f.guarded, *f.recovered,
            f.operands[0], f.operands[1], f.operands[2]);

        ok = !narrowLoads && !doubledLoads &&
             doubled.logicalDomain.getNumDisjuncts() == 2 &&
             countOps(f.thenBlock) == before &&
             succeeded(mlir::verify(f.module));
      }

      if (verify(ok, "3b2 uncovering box and multi-disjunct plan refused"))
        return 1;
    }

    // R17: a memref that does not dominate the nest.

    if (verify(expectLoadsRefused(
                   "dominance", baseSpec,
                   [](Fixture &f) {
                     OpBuilder::InsertionGuard guard(f.builder);
                     f.builder.setInsertionPoint(
                         f.entry->getTerminator());
                     Value late = memref::AllocaOp::create(
                         f.builder, f.loc,
                         cast<MemRefType>(
                             f.operands[0].memref.getType()));
                     f.operands[0].memref = late;
                   }),
               "3b2 memref defined after the nest refused"))
      return 1;

    // R19: an access emitted under vacuous bounds whose
    // index arithmetic overflows i64 on the exact domain
    // (the overflow certificate V10 fires before the
    // tensor-bounds check V9 here).

    if (verify(expectLoadsRefused(
                   "oversized-access", baseSpec,
                   [](Fixture &f) {
                     const std::vector<ClosedIntegerInterval>
                         vacuous = {{0, 0}, {0, 0}};
                     const BigInt huge =
                         BigInt(std::int64_t(1) << 62);
                     auto access = emitSourceAffineAccess(
                         f.builder, f.loc, f.map, vacuous,
                         *f.guarded, *f.recovered,
                         {{huge, BigInt(0)}, {BigInt(0), BigInt(1)}},
                         {BigInt(0), BigInt(0)});
                     if (!access)
                       return;
                     f.operands[0].accessRows =
                         {{huge, BigInt(0)}, {BigInt(0), BigInt(1)}};
                     f.operands[0].accessOffsets = {BigInt(0), BigInt(0)};
                     f.operands[0].access = *access;
                   }),
               "3b2 access overflowing i64 on the exact domain refused"))
      return 1;

    // R21: an access emitted under vacuous bounds whose
    // arithmetic fits i64 but leaves the tensor on the
    // exact domain (V9 alone refuses: 8*x0 reaches 24
    // on memref<5x4>).

    if (verify(expectLoadsRefused(
                   "oversized-access", baseSpec,
                   [](Fixture &f) {
                     const std::vector<ClosedIntegerInterval>
                         vacuous = {{0, 0}, {0, 0}};
                     auto access = emitSourceAffineAccess(
                         f.builder, f.loc, f.map, vacuous,
                         *f.guarded, *f.recovered,
                         {{BigInt(8), BigInt(0)}, {BigInt(0), BigInt(1)}},
                         {BigInt(0), BigInt(0)});
                     if (!access)
                       return;
                     f.operands[0].accessRows =
                         {{BigInt(8), BigInt(0)}, {BigInt(0), BigInt(1)}};
                     f.operands[0].accessOffsets = {BigInt(0), BigInt(0)};
                     f.operands[0].access = *access;
                   }),
               "3b2 access leaving the tensor on the exact domain refused"))
      return 1;

    // R22: on the source domain x0 == x1 the access
    // 2^62*(x0 - x1) is 0 and in bounds, but its first
    // product overflows i64 at x0 >= 2: V10 alone refuses.

    {
      FixtureSpec diagonal{
          identityRows, unitBox2, unitBox2,
          {OperandSpec{identityRows, zero2, {4, 4}},
           OperandSpec{swapRows, zero2, {4, 4}},
           OperandSpec{identityRows, zero2, {4, 4}}},
          false, false};

      diagonal.extraInequalities = {
          {{BigInt(1), BigInt(-1)}, BigInt(0)},
          {{BigInt(-1), BigInt(1)}, BigInt(0)}};

      if (verify(expectLoadsRefused(
                     "overflow-only", diagonal,
                     [](Fixture &f) {
                       const std::vector<ClosedIntegerInterval>
                           vacuous = {{0, 0}, {0, 0}};
                       const BigInt huge =
                           BigInt(std::int64_t(1) << 62);
                       const std::vector<std::vector<BigInt>> rows =
                           {{huge, -huge}, {BigInt(0), BigInt(1)}};
                       auto access = emitSourceAffineAccess(
                           f.builder, f.loc, f.map, vacuous,
                           *f.guarded, *f.recovered, rows,
                           {BigInt(0), BigInt(0)});
                       if (!access)
                         return;
                       f.operands[0].accessRows = rows;
                       f.operands[0].accessOffsets = {BigInt(0), BigInt(0)};
                       f.operands[0].access = *access;
                     }),
                 "3b2 in-bounds access with overflowing i64 products"
                 " refused"))
        return 1;
    }

    // R7 / R8: store value of the wrong type, or not
    // dominating the store.

    {
      Fixture f;

      bool ok = buildFixture(f, baseSpec);

      std::optional<EmittedGuardedMemoryLoads> loads;

      if (ok) {
        loads = emitGuardedMemoryLoads(
            f.builder, f.loc, f.map, *f.plan, *f.sourceDomain,
            *f.guarded, *f.recovered,
            f.operands[0], f.operands[1], f.operands[2]);
        ok = loads.has_value();
      }

      if (ok) {
        OpBuilder::InsertionGuard guard(f.builder);
        f.builder.setInsertionPoint(f.thenBlock->getTerminator());

        Value narrow = arith::ConstantIntOp::create(
            f.builder, f.loc, 1, 32);

        Value truth = arith::ConstantIntOp::create(
            f.builder, f.loc, 1, 1);

        auto nested = scf::IfOp::create(
            f.builder, f.loc, TypeRange{}, truth, false);

        f.builder.setInsertionPointToStart(
            &nested.getThenRegion().front());

        Value hidden = arith::ConstantIntOp::create(
            f.builder, f.loc, 7, 64);

        const auto before = countOps(f.thenBlock);

        auto wrongType = emitGuardedMemoryStore(
            f.builder, f.loc, f.map, *f.plan, *f.sourceDomain,
            *f.guarded, *f.recovered, f.operands[0], f.operands[1],
            f.operands[2], *loads, narrow);

        auto notDominating = emitGuardedMemoryStore(
            f.builder, f.loc, f.map, *f.plan, *f.sourceDomain,
            *f.guarded, *f.recovered, f.operands[0], f.operands[1],
            f.operands[2], *loads, hidden);

        ok = !wrongType && !notDominating &&
             countOps(f.thenBlock) == before &&
             succeeded(mlir::verify(f.module));
      }

      if (verify(ok, "3b2 store value type and dominance validated"))
        return 1;
    }

    // ======================================================
    // Phase 5.4.3d-3b-3:
    // Guarded multiply-accumulate with exact reduction
    // semantics, on the Phase 3b-2 fixtures.
    //
    // The reference replays
    //
    //   C[fC(x)] = C[fC(x)] + A[fA(x)] * B[fB(x)]
    //
    // in source lexicographic order over the source domain
    // with the element type's own arithmetic: f32 / f64 with
    // one rounding per operation (volatile temporaries keep
    // the compiler from contracting), integers narrower than
    // 64 bits wrapping to their width, 64-bit integers
    // exact. The emitted operation tree is interpreted and
    // its final memory, event multiset and per-element store
    // sequences are compared bit for bit.
    //
    // Every float scenario with collisions must also show
    // that the reference evaluated in reversed source order
    // differs: the order of a floating-point reduction is
    // observable, and the emitter preserves it. Integer
    // reductions are order independent, so for them the
    // reversed reference must agree.
    // ======================================================

    // Canonical bits of a value: the integer itself, or the
    // bit pattern of the double (f32 values are exact
    // doubles). Neither IEEE 754 nor the arith dialect fixes
    // the sign or payload of a NaN an operation produces, so
    // every NaN maps to one canonical pattern: the comparison
    // requires both sides to produce a NaN, not the same NaN.

    auto bitsOf = [](bool isFloat, int64_t integer, double real)
        -> int64_t {
      if (!isFloat)
        return integer;

      if (std::isnan(real))
        real = std::numeric_limits<double>::quiet_NaN();

      int64_t bits = 0;
      static_assert(sizeof(bits) == sizeof(real));
      std::memcpy(&bits, &real, sizeof(bits));
      return bits;
    };

    auto wrapWidth = [](__int128 exact, unsigned width) -> int64_t {
      const __int128 modulus = static_cast<__int128>(1) << width;
      __int128 remainder = exact % modulus;

      if (remainder < 0)
        remainder += modulus;

      if (remainder >= modulus / 2)
        remainder -= modulus;

      return static_cast<int64_t>(remainder);
    };

    // Test data families (see macBuffer).
    enum class MacData { Cancellation, Rounding, Special };

    struct MacSpec {
      FixtureSpec fixture;
      GuardedMacSemantics semantics;
      MacData data = MacData::Cancellation;
    };

    // Typed initial buffers.
    //
    // Cancellation (float): along the first index, A cycles
    // through +M, 1, -M, 1 with M = 2^24 for f32 and M = 2^56
    // for f64, one bit beyond the significand; B is 1. A sum
    // of such products cancels the two large terms, and
    // whether the small terms survive depends on the order in
    // which they meet the large ones; evaluating f32 in a
    // wider format gives a different answer.
    //
    // Rounding (float, one update per element): even
    // elements multiply (1 + 2^-(p-1)) by (1 - 2^-(p-1)) and
    // add -1: the exact product 1 - 2^-(2p-2) rounds to 1, so
    // the result is 0 with one rounding per operation and
    // -2^-(2p-2) under a fused multiply-add or a wider
    // product. Odd elements add 1 to 2^p + 2: the exact sum
    // 2^p + 3 is a tie, resolved to the even 2^p + 4.
    //
    // Special (float, one update per element): signed zeros,
    // infinities, overflow, a subnormal product that rounds
    // to zero, and NaN production and propagation.
    //
    // Integers: narrow widths get products beyond their
    // width so that wrapping occurs; i64 stays small.

    auto macBuffer = [&](const OperandSpec &spec, unsigned operand,
                         MacData data) {
      exec::MemoryBuffer buffer;
      buffer.shape = spec.shape;

      const std::size_t size = bufferSize(spec.shape);

      if (spec.isFloat) {
        buffer.isFloat = true;

        const bool single = spec.floatWidth == 32;
        const int precision = single ? 24 : 53;

        const std::size_t innerExtent =
            size / static_cast<std::size_t>(spec.shape.front());

        for (std::size_t linear = 0; linear < size; ++linear) {
          double value = 0.0;

          if (data == MacData::Cancellation) {
            if (operand == 0) {
              const std::size_t first = linear / innerExtent;
              const double large = std::ldexp(1.0, precision);
              value = first % 4 == 0 ? large
                      : first % 4 == 2 ? -large
                                       : 1.0;
            } else if (operand == 1) {
              value = 1.0;
            } else {
              value = 0.3125 * static_cast<double>(linear + 1);
            }
          } else if (data == MacData::Rounding) {
            const double epsilon = std::ldexp(1.0, 1 - precision);

            if (linear % 2 == 0) {
              value = operand == 0 ? 1.0 + epsilon
                      : operand == 1 ? 1.0 - epsilon
                                     : -1.0;
            } else {
              value = operand == 0 ? std::ldexp(1.0, precision) + 2.0
                      : operand == 1 ? 1.0
                                     : 1.0;
            }
          } else {
            const double infinity = std::numeric_limits<double>::infinity();
            const double quietNaN = std::numeric_limits<double>::quiet_NaN();
            const double huge = single ? 3.0e38 : 1.0e308;
            const double tiny = single ? std::ldexp(1.0, -149)
                                       : std::ldexp(1.0, -1074);

            // (a, b, c) -> c + a * b
            switch (linear % 8) {
            case 0: // -0 + (-0 * 1) = -0
              value = operand == 0 ? -0.0 : operand == 1 ? 1.0 : -0.0;
              break;
            case 1: // -0 + (+0 * 1) = +0
              value = operand == 0 ? 0.0 : operand == 1 ? 1.0 : -0.0;
              break;
            case 2: // 5 + inf = inf
              value = operand == 0 ? infinity : operand == 1 ? 1.0 : 5.0;
              break;
            case 3: // inf + (inf * -1) = NaN
              value = operand == 0 ? infinity : operand == 1 ? -1.0 : infinity;
              break;
            case 4: // 1 + (inf * 0) = NaN
              value = operand == 0 ? infinity : operand == 1 ? 0.0 : 1.0;
              break;
            case 5: // 0 + huge * 4 overflows to inf
              value = operand == 0 ? huge : operand == 1 ? 4.0 : 0.0;
              break;
            case 6: // 0 + tiny * 0.5 rounds (tie) to +0
              value = operand == 0 ? tiny : operand == 1 ? 0.5 : 0.0;
              break;
            default: // NaN propagates
              value = operand == 0 ? quietNaN : operand == 1 ? 1.0 : 1.0;
              break;
            }
          }

          buffer.floatData.push_back(
              single ? static_cast<double>(static_cast<float>(value))
                     : value);
        }

        return buffer;
      }

      for (std::size_t linear = 0; linear < size; ++linear) {
        int64_t value = 0;

        if (spec.integerWidth < 64) {
          if (operand == 0)
            value = 100000 + 1000 * static_cast<int64_t>(linear);
          else if (operand == 1)
            value = 30000 + static_cast<int64_t>(linear);
          else
            value = 1000 + static_cast<int64_t>(linear);

          value = wrapWidth(value, spec.integerWidth);
        } else if (operand == 0) {
          value = 1000 * static_cast<int64_t>(linear + 1);
        } else if (operand == 1) {
          value = static_cast<int64_t>(linear + 1);
        } else {
          value = 100 + static_cast<int64_t>(linear);
        }

        buffer.data.push_back(value);
      }

      return buffer;
    };

    auto bufferBits = [&](const exec::MemoryBuffer &buffer) {
      std::vector<int64_t> bits;

      if (buffer.isFloat) {
        for (double value : buffer.floatData)
          bits.push_back(bitsOf(true, 0, value));
      } else {
        bits = buffer.data;
      }

      return bits;
    };

    struct MacEvent {
      bool isStore;
      unsigned operand;
      std::vector<int64_t> indices;
      int64_t bits;

      bool operator<(const MacEvent &other) const {
        return std::tie(operand, isStore, indices, bits) <
               std::tie(other.operand, other.isStore,
                        other.indices, other.bits);
      }

      bool operator==(const MacEvent &other) const {
        return operand == other.operand &&
               isStore == other.isStore &&
               indices == other.indices && bits == other.bits;
      }
    };

    struct MacRun {
      std::array<exec::MemoryBuffer, 3> buffers;
      std::vector<MacEvent> trace;
    };

    // Independent source-order reference of the MAC.

    auto macReference = [&](const MacSpec &mac, bool reversed)
        -> std::optional<MacRun> {
      const FixtureSpec &spec = mac.fixture;
      const GuardedMacSemantics &semantics = mac.semantics;

      MacRun run;

      for (unsigned i = 0; i < 3; ++i)
        run.buffers[i] = macBuffer(spec.operands[i], i, mac.data);

      const OperandSpec &element = spec.operands[2];
      const bool isFloat = element.isFloat;
      const unsigned width =
          isFloat ? element.floatWidth : element.integerWidth;

      const AffineIntegerDomain domain = domainOf(spec);

      for (const auto &coordinates :
           enumerateBox(spec.sourceBox, reversed)) {

        IntRow x;

        for (int64_t coordinate : coordinates)
          x.emplace_back(coordinate);

        if (!containsSourcePoint(domain, x))
          continue;

        std::array<std::size_t, 3> offsets{};
        std::array<std::vector<int64_t>, 3> addresses;
        std::array<unsigned, 3> bufferOf{};

        for (unsigned i = 0; i < 3; ++i) {
          const auto &operand = spec.operands[i];

          if (operand.rows.size() != operand.offsets.size())
            return std::nullopt;

          for (std::size_t r = 0; r < operand.rows.size(); ++r) {
            if (operand.rows[r].size() != x.size())
              return std::nullopt;

            BigInt address = operand.offsets[r];

            for (std::size_t j = 0; j < x.size(); ++j)
              address += operand.rows[r][j] * x[j];

            if (address <
                    BigInt(std::numeric_limits<int64_t>::min()) ||
                address >
                    BigInt(std::numeric_limits<int64_t>::max()))
              return std::nullopt;

            addresses[i].push_back(address.convert_to<int64_t>());
          }

          bufferOf[i] = spec.sharedAB && i == 1 ? 0 : i;

          auto linear = linearIndex(
              spec.operands[bufferOf[i]].shape, addresses[i]);

          if (!linear)
            return std::nullopt;

          offsets[i] = *linear;
        }

        if (isFloat) {
          const double a = run.buffers[bufferOf[0]].floatData[offsets[0]];
          const double b = run.buffers[bufferOf[1]].floatData[offsets[1]];
          const double old = run.buffers[2].floatData[offsets[2]];

          run.trace.push_back({false, bufferOf[0], addresses[0], bitsOf(true, 0, a)});
          run.trace.push_back({false, bufferOf[1], addresses[1], bitsOf(true, 0, b)});
          run.trace.push_back({false, 2, addresses[2], bitsOf(true, 0, old)});

          const double leftFactor = semantics.aIsLeftFactor ? a : b;
          const double rightFactor = semantics.aIsLeftFactor ? b : a;

          // IEEE 754 round to nearest even in software
          // (llvm::APFloat), independent of the host's
          // floating-point unit that the interpreter uses.

          const auto roundingMode = llvm::APFloat::rmNearestTiesToEven;

          double sum = 0.0;

          if (width == 32) {
            llvm::APFloat product(static_cast<float>(leftFactor));
            product.multiply(llvm::APFloat(static_cast<float>(rightFactor)),
                             roundingMode);

            llvm::APFloat accumulator(static_cast<float>(old));

            if (semantics.accumulatorIsLeftAddend) {
              accumulator.add(product, roundingMode);
              sum = static_cast<double>(accumulator.convertToFloat());
            } else {
              product.add(accumulator, roundingMode);
              sum = static_cast<double>(product.convertToFloat());
            }
          } else {
            llvm::APFloat product(leftFactor);
            product.multiply(llvm::APFloat(rightFactor), roundingMode);

            llvm::APFloat accumulator(old);

            if (semantics.accumulatorIsLeftAddend) {
              accumulator.add(product, roundingMode);
              sum = accumulator.convertToDouble();
            } else {
              product.add(accumulator, roundingMode);
              sum = product.convertToDouble();
            }
          }

          run.buffers[2].floatData[offsets[2]] = sum;
          run.trace.push_back({true, 2, addresses[2], bitsOf(true, 0, sum)});
          continue;
        }

        const int64_t a = run.buffers[bufferOf[0]].data[offsets[0]];
        const int64_t b = run.buffers[bufferOf[1]].data[offsets[1]];
        const int64_t old = run.buffers[2].data[offsets[2]];

        run.trace.push_back({false, bufferOf[0], addresses[0], a});
        run.trace.push_back({false, bufferOf[1], addresses[1], b});
        run.trace.push_back({false, 2, addresses[2], old});

        const __int128 leftFactor = semantics.aIsLeftFactor ? a : b;
        const __int128 rightFactor = semantics.aIsLeftFactor ? b : a;

        auto settle = [&](__int128 exact) -> std::optional<int64_t> {
          if (width < 64)
            return wrapWidth(exact, width);

          if (exact < std::numeric_limits<int64_t>::min() ||
              exact > std::numeric_limits<int64_t>::max())
            return std::nullopt; // mirrors the checked interpreter

          return static_cast<int64_t>(exact);
        };

        auto product = settle(leftFactor * rightFactor);

        if (!product)
          return std::nullopt;

        auto sum = settle(semantics.accumulatorIsLeftAddend
                              ? static_cast<__int128>(old) + *product
                              : static_cast<__int128>(*product) + old);

        if (!sum)
          return std::nullopt;

        run.buffers[2].data[offsets[2]] = *sum;
        run.trace.push_back({true, 2, addresses[2], *sum});
      }

      return run;
    };

    // Emit the MAC on a fixture, check its structure, execute
    // the actual operation tree and compare with the
    // reference.

    auto runMac = [&](const char *name, const MacSpec &mac,
                      std::size_t expectedVisited,
                      std::size_t expectedAccepted,
                      bool expectOrderSensitive) -> bool {
      const FixtureSpec &spec = mac.fixture;
      const GuardedMacSemantics &semantics = mac.semantics;

      Fixture f;

      if (!buildFixture(f, spec)) {
        std::cerr << "3b3 " << name << ": fixture failed\n";
        return false;
      }

      auto loads = emitGuardedMemoryLoads(
          f.builder, f.loc, f.map, *f.plan, *f.sourceDomain,
          *f.guarded, *f.recovered,
          f.operands[0], f.operands[1], f.operands[2]);

      if (!loads) {
        std::cerr << "3b3 " << name << ": loads refused\n";
        return false;
      }

      const auto before = countOps(f.thenBlock);

      auto emitted = emitGuardedMultiplyAccumulate(
          f.builder, f.loc, f.map, *f.plan, *f.sourceDomain,
          *f.guarded, *f.recovered,
          f.operands[0], f.operands[1], f.operands[2], *loads,
          semantics);

      if (!emitted) {
        std::cerr << "3b3 " << name << ": MAC refused\n";
        return false;
      }

      const bool isFloat = spec.operands[2].isFloat;

      Operation *multiply = emitted->multiply;
      Operation *add = emitted->add;
      memref::StoreOp store = emitted->store;

      // Structure: exactly mul, add, store appended in that
      // order before the terminator, source operation
      // kinds, source operand order, strict flags.

      bool structure =
          multiply && add && store &&
          countOps(f.thenBlock) == before + 3 &&
          multiply->getBlock() == f.thenBlock &&
          add->getBlock() == f.thenBlock &&
          store->getBlock() == f.thenBlock &&
          multiply->getNextNode() == add &&
          add->getNextNode() == store.getOperation() &&
          store->getNextNode() == f.thenBlock->getTerminator() &&
          multiply->getNumOperands() == 2 && add->getNumOperands() == 2 &&
          multiply->getNumResults() == 1 && add->getNumResults() == 1 &&
          emitted->result == add->getResult(0) &&
          store.getValue() == emitted->result &&
          store.getMemRef() == f.operands[2].memref;

      Value valueA = loads->loadA.getResult();
      Value valueB = loads->loadB.getResult();
      Value valueC = loads->loadC.getResult();

      if (structure) {
        structure =
            multiply->getOperand(0) ==
                (semantics.aIsLeftFactor ? valueA : valueB) &&
            multiply->getOperand(1) ==
                (semantics.aIsLeftFactor ? valueB : valueA) &&
            add->getOperand(0) ==
                (semantics.accumulatorIsLeftAddend
                     ? valueC : multiply->getResult(0)) &&
            add->getOperand(1) ==
                (semantics.accumulatorIsLeftAddend
                     ? multiply->getResult(0) : valueC);
      }

      if (structure) {
        if (isFloat) {
          auto mulf = dyn_cast<arith::MulFOp>(multiply);
          auto addf = dyn_cast<arith::AddFOp>(add);

          structure = mulf && addf &&
                      mulf.getFastmath() == arith::FastMathFlags::none &&
                      addf.getFastmath() == arith::FastMathFlags::none &&
                      !mulf.getRoundingmodeAttr() &&
                      !addf.getRoundingmodeAttr();
        } else {
          auto muli = dyn_cast<arith::MulIOp>(multiply);
          auto addi = dyn_cast<arith::AddIOp>(add);

          structure = muli && addi &&
                      muli.getOverflowFlags() ==
                          arith::IntegerOverflowFlags::none &&
                      addi.getOverflowFlags() ==
                          arith::IntegerOverflowFlags::none;
        }
      }

      if (!structure) {
        std::cerr << "3b3 " << name << ": malformed MAC structure\n";
        return false;
      }

      // One statement: a second MAC is refused and leaves
      // nothing behind.

      auto second = emitGuardedMultiplyAccumulate(
          f.builder, f.loc, f.map, *f.plan, *f.sourceDomain,
          *f.guarded, *f.recovered,
          f.operands[0], f.operands[1], f.operands[2], *loads,
          semantics);

      if (second || countOps(f.thenBlock) != before + 3) {
        std::cerr << "3b3 " << name << ": second MAC accepted\n";
        return false;
      }

      if (failed(mlir::verify(f.module))) {
        std::cerr << "3b3 " << name << ": verifier failed\n";
        return false;
      }

      if (std::getenv("RANK_PARTITION_DUMP_IR")) {
        llvm::errs() << "3b3 " << name << " IR:\n";
        f.module.print(llvm::errs());
        llvm::errs() << "\n";
      }

      if (!imageAgreesWithOracle(f, spec, expectedAccepted)) {
        std::cerr << "3b3 " << name
                  << ": exact logical image disagrees with the"
                     " BigInt oracle\n";
        return false;
      }

      // Execute the actual operation tree.

      exec::MemoryState memory;

      for (unsigned i = 0; i < 3; ++i)
        memory.buffers[f.entry->getArgument(i)] =
            macBuffer(spec.operands[i], i, mac.data);

      auto execution = exec::interpretGuardedLoopNest(
          f.guarded->outermost, f.guarded->guardIf,
          f.guarded->logicalIVs, f.recovered->sourceIVs,
          f.operands[0].access.indices,
          f.operands[1].access.indices,
          f.operands[2].access.indices,
          &memory);

      if (!execution ||
          execution->visitedPoints != expectedVisited ||
          execution->acceptedPoints != expectedAccepted ||
          execution->accessByLogical.size() != expectedAccepted ||
          execution->sourceCounts.size() != expectedAccepted) {
        std::cerr << "3b3 " << name << ": interpreter failure\n";
        return false;
      }

      auto reference = macReference(mac, false);
      auto reversedReference = macReference(mac, true);

      if (!reference || !reversedReference) {
        std::cerr << "3b3 " << name << ": reference failed\n";
        return false;
      }

      // Order sensitivity of the reference itself.

      const bool reversedAgrees =
          bufferBits(reversedReference->buffers[2]) ==
          bufferBits(reference->buffers[2]);

      if (reversedAgrees == expectOrderSensitive) {
        std::cerr << "3b3 " << name
                  << ": reference order sensitivity check failed\n";
        return false;
      }

      // Final memory, bit for bit.

      for (unsigned i = 0; i < 3; ++i) {
        const auto &actual =
            memory.buffers[f.entry->getArgument(i)];

        if (actual.shape != reference->buffers[i].shape ||
            actual.isFloat != reference->buffers[i].isFloat ||
            bufferBits(actual) != bufferBits(reference->buffers[i])) {
          std::cerr << "3b3 " << name
                    << ": memory contents differ, operand " << i
                    << "\n";
          return false;
        }
      }

      // Event multiset and per-element store order.

      std::vector<MacEvent> observed;

      for (const auto &event : memory.trace) {
        std::optional<unsigned> operand;

        for (unsigned i = 0; i < 3; ++i)
          if (event.memref == f.entry->getArgument(i))
            operand = i;

        if (!operand || event.isFloat != isFloat)
          return false;

        observed.push_back(
            {event.isStore, *operand, event.indices,
             bitsOf(event.isFloat, event.value, event.floatValue)});
      }

      if (observed.size() != reference->trace.size() ||
          observed.size() != 4 * expectedAccepted) {
        std::cerr << "3b3 " << name << ": event count "
                  << observed.size() << "\n";
        return false;
      }

      auto storeSequences =
          [](const std::vector<MacEvent> &trace) {
            std::map<std::vector<int64_t>, std::vector<int64_t>>
                sequences;

            for (const auto &event : trace)
              if (event.isStore)
                sequences[event.indices].push_back(event.bits);

            return sequences;
          };

      if (storeSequences(observed) !=
          storeSequences(reference->trace)) {
        std::cerr << "3b3 " << name
                  << ": per-element store order differs\n";
        return false;
      }

      std::vector<MacEvent> sortedObserved = observed;
      std::vector<MacEvent> sortedReference = reference->trace;

      std::sort(sortedObserved.begin(), sortedObserved.end());
      std::sort(sortedReference.begin(), sortedReference.end());

      if (sortedObserved != sortedReference) {
        std::cerr << "3b3 " << name << ": event multiset differs\n";
        return false;
      }

      return true;
    };

    auto floatVariant = [](FixtureSpec spec, unsigned width) {
      for (auto &operand : spec.operands) {
        operand.isFloat = true;
        operand.floatWidth = width;
      }

      return spec;
    };

    auto integerVariant = [](FixtureSpec spec, unsigned width) {
      for (auto &operand : spec.operands) {
        operand.isFloat = false;
        operand.integerWidth = width;
      }

      return spec;
    };

    GuardedMacSemantics strict;

    GuardedMacSemantics mirrored;
    mirrored.aIsLeftFactor = false;
    mirrored.accumulatorIsLeftAddend = false;

    FixtureSpec sharedMac = skewSpec;
    sharedMac.sharedAB = true;
    sharedMac.operands[1] = OperandSpec{swapRows, zero2, {5, 4}};

    FixtureSpec triangleMac = skewSpec;
    triangleMac.extraInequalities = {
        {{BigInt(-1), BigInt(1)}, BigInt(0)}};

    // Identity accesses under swapped loops: every element
    // receives one update from the A, B and C values at the
    // same linear position, so per-position data tables
    // describe each result directly.

    FixtureSpec identityAccess{
        swapT, unitBox2, unitBox2,
        {OperandSpec{identityRows, zero2, {4, 4}},
         OperandSpec{identityRows, zero2, {4, 4}},
         OperandSpec{identityRows, zero2, {4, 4}}},
        false, false};

    // Positive scenarios.

    if (verify(runMac("i64 unimodular", {skewSpec, strict}, 28, 16, false),
               "3b3 i64 MAC under the unimodular skew matches the"
               " source-order reference"))
      return 1;

    if (verify(runMac("f32 unimodular", {floatVariant(skewSpec, 32), strict},
                      28, 16, true),
               "3b3 f32 MAC under the unimodular skew matches the"
               " source-order reference bit for bit"))
      return 1;

    if (verify(runMac("f64 unimodular", {floatVariant(skewSpec, 64), strict},
                      28, 16, true),
               "3b3 f64 MAC under the unimodular skew matches the"
               " source-order reference bit for bit"))
      return 1;

    if (verify(runMac("i32 wrapping", {integerVariant(skewSpec, 32), strict},
                      28, 16, false),
               "3b3 i32 MAC wraps exactly like the source-order"
               " reference"))
      return 1;

    if (verify(runMac("f32 mirrored order",
                      {floatVariant(skewSpec, 32), mirrored}, 28, 16, true),
               "3b3 f32 MAC with mirrored operand order matches the"
               " reference"))
      return 1;

    if (verify(runMac("f32 rounding",
                      {floatVariant(identityAccess, 32), strict,
                       MacData::Rounding},
                      16, 16, false),
               "3b3 f32 MAC rounds each operation once (inexact"
               " products, ties to even) like the software reference"))
      return 1;

    if (verify(runMac("f64 rounding",
                      {floatVariant(identityAccess, 64), strict,
                       MacData::Rounding},
                      16, 16, false),
               "3b3 f64 MAC rounds each operation once (inexact"
               " products, ties to even) like the software reference"))
      return 1;

    if (verify(runMac("f32 special values",
                      {floatVariant(identityAccess, 32), strict,
                       MacData::Special},
                      16, 16, false),
               "3b3 f32 MAC with signed zeros, infinities, overflow,"
               " subnormal rounding and NaNs matches the software"
               " reference"))
      return 1;

    if (verify(runMac("f64 special values",
                      {floatVariant(identityAccess, 64), strict,
                       MacData::Special},
                      16, 16, false),
               "3b3 f64 MAC with signed zeros, infinities, overflow,"
               " subnormal rounding and NaNs matches the software"
               " reference"))
      return 1;

    if (verify(runMac("i64 shared A/B", {sharedMac, strict}, 28, 16, false),
               "3b3 i64 MAC with a shared A/B memref matches the"
               " reference"))
      return 1;

    if (verify(runMac("f32 non-unimodular",
                      {floatVariant(stretchSpec, 32), strict}, 28, 16, true),
               "3b3 f32 MAC under the non-unimodular stretch matches"
               " the reference bit for bit"))
      return 1;

    if (verify(runMac("i64 non-unimodular", {stretchSpec, strict}, 28, 16,
                      false),
               "3b3 i64 MAC under the non-unimodular stretch matches"
               " the reference"))
      return 1;

    if (verify(runMac("f32 swap injective",
                      {floatVariant(swapInjectiveSpec, 32), strict},
                      16, 16, false),
               "3b3 f32 MAC over reordered injective outputs matches"
               " the reference"))
      return 1;

    if (verify(runMac("i64 dense unimodular", {denseUnimodularSpec, strict},
                      400, 16, false),
               "3b3 i64 MAC under the unit-pivot-free map matches the"
               " reference"))
      return 1;

    if (verify(runMac("f32 negative coordinates",
                      {floatVariant(negativeSpec, 32), strict}, 28, 16, true),
               "3b3 f32 MAC with negative coordinates matches the"
               " reference bit for bit"))
      return 1;

    if (verify(runMac("f32 sheared stretch",
                      {floatVariant(shearedStretchSpec, 32), strict},
                      40, 16, true),
               "3b3 f32 MAC under the non-diagonal non-unimodular map"
               " matches the reference bit for bit"))
      return 1;

    if (verify(runMac("f32 three-dimensional",
                      {floatVariant(threeDimSpec, 32), strict}, 112, 64,
                      true),
               "3b3 f32 MAC over the three-loop nest matches the"
               " reference bit for bit"))
      return 1;

    if (verify(runMac("f32 triangle", {floatVariant(triangleMac, 32), strict},
                      28, 10, true),
               "3b3 f32 MAC over the non-box source domain matches the"
               " reference bit for bit"))
      return 1;

    // Negative scenarios: each refusal leaves the then block
    // unchanged (no multiply, add or store) and a verifying
    // module.

    auto expectMacRefused =
        [&](const char *name, const FixtureSpec &spec,
            const GuardedMacSemantics &semantics,
            std::function<void(Fixture &, EmittedGuardedMemoryLoads &,
                               GuardedMemoryOperand &)>
                mutate) -> bool {
      Fixture f;

      if (!buildFixture(f, spec)) {
        std::cerr << "3b3 " << name << ": fixture failed\n";
        return false;
      }

      auto loads = emitGuardedMemoryLoads(
          f.builder, f.loc, f.map, *f.plan, *f.sourceDomain,
          *f.guarded, *f.recovered,
          f.operands[0], f.operands[1], f.operands[2]);

      if (!loads) {
        std::cerr << "3b3 " << name << ": loads refused\n";
        return false;
      }

      EmittedGuardedMemoryLoads handles = *loads;
      GuardedMemoryOperand cOperand = f.operands[2];

      if (mutate)
        mutate(f, handles, cOperand);

      const auto before = countOps(f.thenBlock);

      auto emitted = emitGuardedMultiplyAccumulate(
          f.builder, f.loc, f.map, *f.plan, *f.sourceDomain,
          *f.guarded, *f.recovered,
          f.operands[0], f.operands[1], cOperand, handles, semantics);

      if (emitted || countOps(f.thenBlock) != before ||
          failed(mlir::verify(f.module))) {
        std::cerr << "3b3 " << name << ": not refused\n";
        return false;
      }

      return true;
    };

    if (verify(expectMacRefused("i1", integerVariant(baseSpec, 1), strict,
                                nullptr),
               "3b3 unsupported element type i1 refused"))
      return 1;

    if (verify(expectMacRefused("f16", floatVariant(baseSpec, 16), strict,
                                nullptr),
               "3b3 unsupported element type f16 refused"))
      return 1;

    {
      GuardedMacSemantics contracting;
      contracting.fastMath = arith::FastMathFlags::contract;

      if (verify(expectMacRefused("fast-math", floatVariant(baseSpec, 32),
                                  contracting, nullptr),
                 "3b3 fast-math flags on the source refused"))
        return 1;

      GuardedMacSemantics noSignedWrap;
      noSignedWrap.integerOverflow = arith::IntegerOverflowFlags::nsw;

      if (verify(expectMacRefused("overflow-flags", baseSpec, noSignedWrap,
                                  nullptr),
                 "3b3 integer overflow flags on the source refused"))
        return 1;

      GuardedMacSemantics directed;
      directed.roundingMode = arith::RoundingMode::upward;

      if (verify(expectMacRefused("rounding-mode", floatVariant(baseSpec, 32),
                                  directed, nullptr),
                 "3b3 rounding-mode attribute on the source refused"))
        return 1;
    }

    // Invalid operand type: a load of another buffer with
    // another element type in place of the A load.

    if (verify(expectMacRefused(
                   "operand-type", baseSpec, strict,
                   [](Fixture &f, EmittedGuardedMemoryLoads &handles,
                      GuardedMemoryOperand &) {
                     OpBuilder::InsertionGuard guard(f.builder);
                     f.builder.setInsertionPoint(f.guarded->outermost);
                     Value other = memref::AllocaOp::create(
                         f.builder, f.loc,
                         MemRefType::get({5, 4}, f.builder.getI32Type()));
                     f.builder.setInsertionPoint(
                         f.thenBlock->getTerminator());
                     memref::LoadOp replaced = handles.loadA;
                     handles.loadA = memref::LoadOp::create(
                         f.builder, f.loc, other, handles.indicesA);
                     replaced->erase(); // isolate the type check
                   }),
               "3b3 load of another element type refused"))
      return 1;

    // Invalid SSA dominance: the C load inside a nested
    // region does not dominate the statement.

    if (verify(expectMacRefused(
                   "dominance", baseSpec, strict,
                   [](Fixture &f, EmittedGuardedMemoryLoads &handles,
                      GuardedMemoryOperand &cOperand) {
                     OpBuilder::InsertionGuard guard(f.builder);
                     f.builder.setInsertionPoint(
                         f.thenBlock->getTerminator());
                     Value truth = arith::ConstantIntOp::create(
                         f.builder, f.loc, 1, 1);
                     auto nested = scf::IfOp::create(
                         f.builder, f.loc, TypeRange{}, truth, false);
                     f.builder.setInsertionPointToStart(
                         &nested.getThenRegion().front());
                     memref::LoadOp replaced = handles.loadC;
                     handles.loadC = memref::LoadOp::create(
                         f.builder, f.loc, cOperand.memref,
                         handles.indicesC);
                     replaced->erase(); // isolate the block check
                   }),
               "3b3 load that does not dominate the statement refused"))
      return 1;

    // Rejected memory access: the C operand claims another
    // access than the one its SSA computes.

    if (verify(expectMacRefused(
                   "memory-access", baseSpec, strict,
                   [](Fixture &, EmittedGuardedMemoryLoads &,
                      GuardedMemoryOperand &cOperand) {
                     cOperand.accessOffsets = {BigInt(1)};
                   }),
               "3b3 inconsistent C access refused"))
      return 1;

    // Tampered handles.

    if (verify(expectMacRefused(
                   "tampered-handles", baseSpec, strict,
                   [](Fixture &, EmittedGuardedMemoryLoads &handles,
                      GuardedMemoryOperand &) {
                     memref::LoadOp replaced = handles.loadC;
                     handles.loadC = handles.loadA;
                     replaced->erase(); // isolate the memref check
                   }),
               "3b3 tampered loads struct refused"))
      return 1;

    // Reversed reduction order: with loads forged for the
    // colliding access C'[x0 + x1] under swapped loops, the
    // MAC emitter itself refuses the statement.

    if (verify(expectMacRefused(
                   "reversed-order", swapInjectiveSpec, strict,
                   [](Fixture &f, EmittedGuardedMemoryLoads &handles,
                      GuardedMemoryOperand &cOperand) {
                     OpBuilder::InsertionGuard guard(f.builder);
                     f.builder.setInsertionPoint(f.guarded->outermost);
                     Value other = memref::AllocaOp::create(
                         f.builder, f.loc,
                         MemRefType::get({7}, f.builder.getI64Type()));
                     const std::vector<std::vector<BigInt>> rows =
                         {{BigInt(1), BigInt(1)}};
                     const std::vector<BigInt> offsets = {BigInt(0)};
                     auto access = emitSourceAffineAccess(
                         f.builder, f.loc, f.map, f.plan->bounds,
                         *f.guarded, *f.recovered, rows, offsets);
                     if (!access)
                       return;
                     f.builder.setInsertionPoint(
                         f.thenBlock->getTerminator());
                     Value index = arith::IndexCastOp::create(
                         f.builder, f.loc, f.builder.getIndexType(),
                         access->indices[0]);
                     memref::LoadOp forged = memref::LoadOp::create(
                         f.builder, f.loc, other, ValueRange{index});
                     handles.loadC->erase();
                     handles.loadC = forged;
                     handles.indicesC = {index};
                     cOperand = GuardedMemoryOperand{other, rows, offsets,
                                                     *access};
                   }),
               "3b3 reversed reduction order refused by the MAC emitter"))
      return 1;
  }

  // --------------------------------------------------------
  // Phase 5.4.3d-3b-4:
  // Recognized GEMM to guarded MAC twin, differentially.
  //
  // Each scenario is a real affine GEMM nest parsed from
  // text. The adapter recognizes it with the production
  // recognizer and emits the complete guarded statement into
  // a private, never-called twin function. The ORIGINAL nest
  // and the twin are then executed separately on identical,
  // independently initialised memory, and their final C
  // contents, memory event multisets and per-element store
  // sequences are compared bit for bit. The original nest's
  // own execution is the oracle: no lowering algorithm is
  // involved in producing the expected result.
  // --------------------------------------------------------
  {
    using namespace mlir;
    using namespace ::systolic::logical;
    namespace exec = ::systolic::logical::testing;

    // A GEMM nest as MLIR text. Arguments are C, A, B in that
    // order; `loops` opens the loop nest, `closes` closes it.

    struct GemmText {
      std::string name;
      std::string cType, aType, bType;
      std::string loops;
      int depth = 3;
      std::string cIndex, aIndex, bIndex;
      bool productFirst = false;
      std::string flags;          // e.g. " fastmath<reassoc>"
      std::string argumentAttrs = " {llvm.noalias}";
      std::string element = "f32";
      std::string prologue;       // extra ops before the nest
      bool aIsC = false;          // A reads C's memref
      std::string cMemref = "%C"; // the memref C is read and written through
    };

    auto gemmModule = [](const GemmText &g) {
      std::string closes;
      for (int i = 0; i < g.depth; ++i)
        closes += " }";

      std::string aMemref = g.aIsC ? g.cMemref : "%A";
      std::string aType = g.aIsC ? g.cType : g.aType;
      std::string mul = g.element == "f32" ? "arith.mulf" : "arith.muli";
      std::string add = g.element == "f32" ? "arith.addf" : "arith.addi";

      std::string text = "module {\n  func.func @" + g.name + "(%C: " +
                         g.cType + g.argumentAttrs + ", %A: " + g.aType +
                         g.argumentAttrs + ", %B: " + g.bType +
                         g.argumentAttrs + ") {\n" + g.prologue + "    " +
                         g.loops + "\n" +
                         "      %c = affine.load " + g.cMemref + "[" +
                         g.cIndex + "] : " + g.cType + "\n" +
                         "      %a = affine.load " + aMemref + "[" +
                         g.aIndex + "] : " + aType + "\n" +
                         "      %b = affine.load %B[" + g.bIndex + "] : " +
                         g.bType + "\n" +
                         "      %p = " + mul + " %a, %b" + g.flags + " : " +
                         g.element + "\n" +
                         (g.productFirst
                              ? "      %s = " + add + " %p, %c" + g.flags +
                                    " : " + g.element + "\n"
                              : "      %s = " + add + " %c, %p" + g.flags +
                                    " : " + g.element + "\n") +
                         "      affine.store %s, " + g.cMemref + "[" +
                         g.cIndex + "] : " + g.cType + "\n" + "    " +
                         closes + "\n" +
                         "    func.return\n  }\n}\n";
      return text;
    };

    const std::string loops3 =
        "affine.for %i = 0 to 4 { affine.for %j = 0 to 4 {"
        " affine.for %k = 0 to 4 {";

    // Data families (f32), per linear buffer position.

    enum class TwinData { Cancellation, Rounding, Special };

    auto bufferOf = [](MemRefType type, unsigned role, TwinData data) {
      // role: 0 = A, 1 = B, 2 = C
      exec::MemoryBuffer buffer;
      buffer.shape.assign(type.getShape().begin(), type.getShape().end());
      buffer.isFloat = true;

      std::size_t size = 1;
      for (int64_t extent : buffer.shape)
        size *= static_cast<std::size_t>(extent);

      const std::size_t lastExtent =
          static_cast<std::size_t>(buffer.shape.back());

      for (std::size_t linear = 0; linear < size; ++linear) {
        double value = 0.0;

        if (data == TwinData::Cancellation) {
          // +2^24, 1, -2^24, 1 along A's last index, the one the
          // reduction loop walks in every scenario, with B = 1:
          // the four products of one C element cancel their
          // large terms and keep the small ones or not depending
          // on the order in which they are added.
          if (role == 0) {
            const std::size_t last = linear % lastExtent;
            value = last % 4 == 0 ? 16777216.0
                    : last % 4 == 2 ? -16777216.0
                                    : 1.0;
          } else if (role == 1) {
            value = 1.0;
          } else {
            value = 0.3125 * static_cast<double>(linear + 1);
          }
        } else if (data == TwinData::Rounding) {
          // Inexact products and ties.
          const double epsilon = std::ldexp(1.0, -23);
          if (role == 0)
            value = linear % 2 == 0 ? 1.0 + epsilon
                                    : 16777218.0;
          else if (role == 1)
            value = linear % 2 == 0 ? 1.0 - epsilon : 1.0;
          else
            value = linear % 2 == 0 ? -1.0 : 1.0;
        } else {
          const double infinity = std::numeric_limits<double>::infinity();
          const double quietNaN = std::numeric_limits<double>::quiet_NaN();
          switch (linear % 6) {
          case 0:
            value = role == 2 ? -0.0 : role == 0 ? -0.0 : 1.0;
            break;
          case 1:
            value = role == 0 ? infinity : 1.0;
            break;
          case 2:
            value = role == 1 ? 0.0 : role == 0 ? infinity : 1.0;
            break;
          case 3:
            value = role == 0 ? 3.0e38 : role == 1 ? 4.0 : 0.0;
            break;
          case 4:
            value = role == 0 ? quietNaN : 1.0;
            break;
          default:
            value = role == 2 ? 0.0 : std::ldexp(1.0, -149);
            break;
          }
        }

        buffer.floatData.push_back(
            static_cast<double>(static_cast<float>(value)));
      }

      return buffer;
    };

    auto canonicalBits = [](double real) -> int64_t {
      if (std::isnan(real))
        real = std::numeric_limits<double>::quiet_NaN();
      int64_t bits = 0;
      std::memcpy(&bits, &real, sizeof(bits));
      return bits;
    };

    struct TwinEvent {
      bool isStore;
      unsigned role;
      std::vector<int64_t> indices;
      int64_t bits;

      bool operator<(const TwinEvent &other) const {
        return std::tie(role, isStore, indices, bits) <
               std::tie(other.role, other.isStore, other.indices,
                        other.bits);
      }

      bool operator==(const TwinEvent &other) const {
        return role == other.role && isStore == other.isStore &&
               indices == other.indices && bits == other.bits;
      }
    };

    // Build a context + module from text and find the source
    // function and its outermost loop.

    struct ParsedGemm {
      MLIRContext context;
      OwningOpRef<ModuleOp> module;
      func::FuncOp function;
      affine::AffineForOp outermost;
    };

    auto parseGemm = [](ParsedGemm &parsed, const std::string &text,
                        const std::string &name) -> bool {
      parsed.context.loadDialect<affine::AffineDialect, arith::ArithDialect,
                                 scf::SCFDialect, func::FuncDialect,
                                 memref::MemRefDialect, LLVM::LLVMDialect>();

      parsed.module = parseSourceString<ModuleOp>(text, &parsed.context);

      if (!parsed.module)
        return false;

      parsed.function = parsed.module->lookupSymbol<func::FuncOp>(name);

      if (!parsed.function)
        return false;

      for (Operation &op : parsed.function.getBody().front()) {
        if (auto loop = dyn_cast<affine::AffineForOp>(&op)) {
          parsed.outermost = loop;
          break;
        }
      }

      return static_cast<bool>(parsed.outermost);
    };

    auto moduleText = [](ModuleOp module) {
      std::string text;
      llvm::raw_string_ostream stream(text);
      module.print(stream);
      return text;
    };

    auto functionText = [](func::FuncOp function) {
      std::string text;
      llvm::raw_string_ostream stream(text);
      function.print(stream);
      return text;
    };

    // From the original nest's own trace (load C, load A,
    // load B, store C per iteration), replay every C element's
    // reduction forward and reversed with llvm::APFloat and
    // report whether some element's final value depends on the
    // order. The source loop order is what the twin must keep.

    auto orderMatters = [&](const exec::MemoryState &memory,
                            const exec::MemoryBuffer &initialC,
                            Value cMemref) -> std::optional<bool> {
      std::map<std::vector<int64_t>, std::vector<std::pair<double, double>>>
          factors;

      const auto &trace = memory.trace;

      if (trace.size() % 4 != 0)
        return std::nullopt;

      for (std::size_t i = 0; i < trace.size(); i += 4) {
        const auto &loadC = trace[i];
        const auto &loadA = trace[i + 1];
        const auto &loadB = trace[i + 2];
        const auto &storeC = trace[i + 3];

        if (loadC.isStore || loadA.isStore || loadB.isStore ||
            !storeC.isStore || loadC.memref != cMemref ||
            storeC.memref != cMemref || storeC.indices != loadC.indices)
          return std::nullopt;

        factors[storeC.indices].push_back(
            {loadA.floatValue, loadB.floatValue});
      }

      auto replay = [&](const std::vector<int64_t> &indices,
                        const std::vector<std::pair<double, double>> &pairs,
                        bool reversed) -> std::optional<float> {
        std::size_t linear = 0;
        for (std::size_t d = 0; d < indices.size(); ++d) {
          if (indices[d] < 0 || indices[d] >= initialC.shape[d])
            return std::nullopt;
          linear = linear * static_cast<std::size_t>(initialC.shape[d]) +
                   static_cast<std::size_t>(indices[d]);
        }

        llvm::APFloat accumulator(
            static_cast<float>(initialC.floatData[linear]));

        const auto mode = llvm::APFloat::rmNearestTiesToEven;

        for (std::size_t k = 0; k < pairs.size(); ++k) {
          const auto &pair = pairs[reversed ? pairs.size() - 1 - k : k];
          llvm::APFloat product(static_cast<float>(pair.first));
          product.multiply(llvm::APFloat(static_cast<float>(pair.second)),
                           mode);
          accumulator.add(product, mode);
        }

        return accumulator.convertToFloat();
      };

      bool differs = false;

      for (const auto &[indices, pairs] : factors) {
        auto forward = replay(indices, pairs, false);
        auto backward = replay(indices, pairs, true);

        if (!forward || !backward)
          return std::nullopt;

        int32_t forwardBits = 0, backwardBits = 0;
        std::memcpy(&forwardBits, &*forward, sizeof(forwardBits));
        std::memcpy(&backwardBits, &*backward, sizeof(backwardBits));

        if (forwardBits != backwardBits)
          differs = true;
      }

      return differs;
    };

    // The differential run.

    auto runTwin = [&](const GemmText &gemm, TwinData data,
                       std::size_t expectedVisited,
                       std::size_t expectedAccepted,
                       bool expectOrderSensitive) -> bool {
      const std::string label = gemm.name + "/" +
                                (data == TwinData::Cancellation ? "cancel"
                                 : data == TwinData::Rounding  ? "round"
                                                               : "special");

      ParsedGemm parsed;

      if (!parseGemm(parsed, gemmModule(gemm), gemm.name)) {
        std::cerr << "3b4 " << label << ": parse failed\n";
        return false;
      }

      const std::string sourceBefore = functionText(parsed.function);
      const std::size_t operationsBefore =
          std::distance(parsed.module->getBody()->begin(),
                        parsed.module->getBody()->end());

      OpBuilder builder(&parsed.context);
      std::string why;

      auto twin = emitGuardedMacTwin(builder, parsed.outermost,
                                     gemm.name + "_twin", why);

      if (!twin) {
        std::cerr << "3b4 " << label << ": " << why << "\n";
        return false;
      }

      // Exactly one new top-level operation: the twin.

      if (std::distance(parsed.module->getBody()->begin(),
                        parsed.module->getBody()->end()) !=
          static_cast<std::ptrdiff_t>(operationsBefore + 1)) {
        std::cerr << "3b4 " << label << ": unexpected module change\n";
        return false;
      }

      // The twin is private and unreferenced; the source
      // function is byte-for-byte unchanged.

      if (!twin->function || !twin->function.isPrivate() ||
          twin->function.getName() != gemm.name + "_twin" ||
          !twin->function.symbolKnownUseEmpty(*parsed.module) ||
          failed(mlir::verify(*parsed.module))) {
        std::cerr << "3b4 " << label << ": twin not isolated or invalid\n";
        return false;
      }

      if (functionText(parsed.function) != sourceBefore) {
        std::cerr << "3b4 " << label << ": source function changed\n";
        return false;
      }

      if (std::getenv("RANK_PARTITION_DUMP_IR")) {
        llvm::errs() << "3b4 " << label << " IR:\n";
        parsed.module->print(llvm::errs());
        llvm::errs() << "\n";
      }

      // Operand roles: the source statement's loads appear as
      // C, A, B (the text generator's order); the twin's
      // arguments are A, B, C.

      llvm::SmallVector<Value> sourceLoads;
      Value sourceStore;

      parsed.outermost.walk([&](Operation *op) {
        if (auto load = dyn_cast<affine::AffineLoadOp>(op))
          sourceLoads.push_back(load.getMemRef());
        else if (auto store = dyn_cast<affine::AffineStoreOp>(op))
          sourceStore = store.getMemRef();
      });

      if (sourceLoads.size() != 3 || sourceStore != sourceLoads[0]) {
        std::cerr << "3b4 " << label << ": unexpected source statement\n";
        return false;
      }

      std::array<Value, 3> sourceMemrefs = {sourceLoads[1], sourceLoads[2],
                                            sourceLoads[0]};

      std::array<Value, 3> twinMemrefs = {
          twin->function.getArgument(0), twin->function.getArgument(1),
          twin->function.getArgument(2)};

      exec::MemoryState sourceMemory;
      exec::MemoryState twinMemory;

      for (unsigned role = 0; role < 3; ++role) {
        auto type = cast<MemRefType>(sourceMemrefs[role].getType());
        sourceMemory.buffers[sourceMemrefs[role]] =
            bufferOf(type, role, data);
        twinMemory.buffers[twinMemrefs[role]] = bufferOf(type, role, data);
      }

      auto sourceRun = exec::interpretGuardedLoopNest(
          parsed.outermost, scf::IfOp(), ValueRange{}, ValueRange{},
          ValueRange{}, ValueRange{}, ValueRange{}, &sourceMemory);

      auto twinRun = exec::interpretGuardedLoopNest(
          twin->nest.outermost, twin->nest.guardIf, twin->nest.logicalIVs,
          twin->recovery.sourceIVs, twin->operands[0].access.indices,
          twin->operands[1].access.indices, twin->operands[2].access.indices,
          &twinMemory);

      if (!sourceRun || !twinRun) {
        std::cerr << "3b4 " << label << ": interpretation failed\n";
        return false;
      }

      // The order of the source reduction must be observable in
      // the order-sensitive family (and may be elsewhere).

      if (expectOrderSensitive) {
        auto sensitive = orderMatters(
            sourceMemory,
            bufferOf(cast<MemRefType>(sourceMemrefs[2].getType()), 2, data),
            sourceMemrefs[2]);

        if (!sensitive || !*sensitive) {
          std::cerr << "3b4 " << label
                    << ": reduction order is not observable in the"
                       " source data\n";
          return false;
        }
      }

      if (twinRun->visitedPoints != expectedVisited ||
          twinRun->acceptedPoints != expectedAccepted ||
          twinRun->sourceCounts.size() != expectedAccepted ||
          sourceMemory.trace.size() != 4 * expectedAccepted ||
          twinMemory.trace.size() != 4 * expectedAccepted) {
        std::cerr << "3b4 " << label << ": counts: visited "
                  << twinRun->visitedPoints << " accepted "
                  << twinRun->acceptedPoints << " source events "
                  << sourceMemory.trace.size() << "\n";
        return false;
      }

      // Final contents of every buffer, bit for bit.

      for (unsigned role = 0; role < 3; ++role) {
        const auto &source = sourceMemory.buffers[sourceMemrefs[role]];
        const auto &mirror = twinMemory.buffers[twinMemrefs[role]];

        if (source.floatData.size() != mirror.floatData.size())
          return false;

        for (std::size_t i = 0; i < source.floatData.size(); ++i) {
          if (canonicalBits(source.floatData[i]) !=
              canonicalBits(mirror.floatData[i])) {
            std::cerr << "3b4 " << label << ": buffer " << role
                      << " differs at " << i << "\n";
            return false;
          }
        }
      }

      // Memory events: same multiset of (role, address, value)
      // and the same per-element store sequence.

      auto events = [&](const exec::MemoryState &memory,
                        const std::array<Value, 3> &memrefs)
          -> std::optional<std::vector<TwinEvent>> {
        std::vector<TwinEvent> out;

        for (const auto &event : memory.trace) {
          std::optional<unsigned> role;

          for (unsigned r = 0; r < 3; ++r)
            if (event.memref == memrefs[r] && !role)
              role = r;

          if (!role || !event.isFloat)
            return std::nullopt;

          out.push_back({event.isStore, *role, event.indices,
                         canonicalBits(event.floatValue)});
        }

        return out;
      };

      auto sourceEvents = events(sourceMemory, sourceMemrefs);
      auto twinEvents = events(twinMemory, twinMemrefs);

      if (!sourceEvents || !twinEvents)
        return false;

      auto storeSequences = [](const std::vector<TwinEvent> &trace) {
        std::map<std::vector<int64_t>, std::vector<int64_t>> sequences;
        for (const auto &event : trace)
          if (event.isStore)
            sequences[event.indices].push_back(event.bits);
        return sequences;
      };

      if (storeSequences(*sourceEvents) != storeSequences(*twinEvents)) {
        std::cerr << "3b4 " << label
                  << ": per-element store order differs\n";
        return false;
      }

      std::sort(sourceEvents->begin(), sourceEvents->end());
      std::sort(twinEvents->begin(), twinEvents->end());

      if (*sourceEvents != *twinEvents) {
        std::cerr << "3b4 " << label << ": event multiset differs\n";
        return false;
      }

      return true;
    };

    // A refusal must leave the module unchanged and report
    // UNKNOWN or REFUSED.

    auto expectTwinRefused = [&](const GemmText &gemm,
                                 const char *expectedPrefix,
                                 const char *expectedFragment) -> bool {
      ParsedGemm parsed;

      if (!parseGemm(parsed, gemmModule(gemm), gemm.name)) {
        std::cerr << "3b4 " << gemm.name << ": parse failed\n";
        return false;
      }

      const std::string before = moduleText(*parsed.module);

      OpBuilder builder(&parsed.context);
      std::string why;

      auto twin = emitGuardedMacTwin(builder, parsed.outermost,
                                     gemm.name + "_twin", why);

      if (twin || moduleText(*parsed.module) != before ||
          why.rfind(expectedPrefix, 0) != 0 ||
          why.find(expectedFragment) == std::string::npos ||
          failed(mlir::verify(*parsed.module))) {
        std::cerr << "3b4 " << gemm.name << ": not refused as expected: "
                  << why << "\n";
        return false;
      }

      return true;
    };

    // ---- recognized nests ----

    GemmText plain{"plain", "memref<4x4xf32>", "memref<4x4xf32>",
                   "memref<4x4xf32>", loops3, 3, "%i, %j", "%i, %k",
                   "%k, %j"};

    GemmText skew{"skew", "memref<8x4xf32>", "memref<8x4xf32>",
                  "memref<4x4xf32>", loops3, 3, "%i + %j, %j",
                  "%i + %j, %k", "%k, %j"};

    GemmText stretch{"stretch", "memref<16x4xf32>", "memref<16x4xf32>",
                     "memref<4x4xf32>", loops3, 3, "2 * %i + %j, %j",
                     "2 * %i + %j, %k", "%k, %j"};

    GemmText stretchOffsets{"stretch_offsets", "memref<16x4xf32>",
                            "memref<16x8xf32>", "memref<8x4xf32>", loops3, 3,
                            "2 * %i + %j + 1, 3 - %j",
                            "2 * %i + %j + 1, %k + 2", "%k + 2, 3 - %j",
                            true};

    GemmText reversedLoops{"reversed_loops", "memref<4x4xf32>",
                           "memref<4x4xf32>", "memref<4x4xf32>",
                           "affine.for %k = 0 to 4 { affine.for %i = 0 to 4 {"
                           " affine.for %j = 0 to 4 {",
                           3, "%i, %j", "%i, %k", "%k, %j"};

    GemmText triangular{"triangular", "memref<4x4xf32>", "memref<4x4xf32>",
                        "memref<4x4xf32>",
                        "affine.for %i = 0 to 4 { affine.for %j = "
                        "affine_map<(d0) -> (d0)>(%i) to 4 {"
                        " affine.for %k = 0 to 4 {",
                        3, "%i, %j", "%i, %k", "%k, %j"};

    GemmText strideK{"stride_k", "memref<4x4xf32>", "memref<4x8xf32>",
                     "memref<8x4xf32>", loops3, 3, "%i, %j", "%i, 2 * %k",
                     "2 * %k, %j"};

    GemmText allocated{"allocated", "memref<4x4xf32>", "memref<4x4xf32>",
                       "memref<4x4xf32>", loops3, 3, "%i, %j", "%i, %k",
                       "%k, %j"};
    allocated.argumentAttrs = "";
    allocated.prologue = "    %Cbuf = memref.alloc() : memref<4x4xf32>\n";
    allocated.cMemref = "%Cbuf"; // A and B are plain arguments, C is fresh

    struct Case {
      GemmText gemm;
      std::size_t visited;
      std::size_t accepted;
      bool orderSensitive; // the cancellation family must expose the order
      const char *claim;
    };

    // stride_k reads A's columns 0, 2, 4, 6, where the pattern
    // is +M, -M, +M, -M: every order cancels exactly, so the
    // order check is not asserted for it.

    const std::vector<Case> cases = {
        {plain, 64, 64, true, "3b4 plain GEMM twin matches the source"},
        {skew, 7 * 4 * 4, 64, true,
         "3b4 skewed-access GEMM twin matches the source"},
        {stretch, 10 * 4 * 4, 64, true,
         "3b4 non-unimodular (det 2) GEMM twin matches the source"},
        {stretchOffsets, 10 * 4 * 4, 64, true,
         "3b4 non-unimodular GEMM with offsets, negative coefficients and"
         " product-first addition twin matches the source"},
        {reversedLoops, 64, 64, true,
         "3b4 GEMM with the reduction loop outermost twin matches the"
         " source order"},
        {triangular, 64, 40, true,
         "3b4 triangular (affine-bound) GEMM twin matches the source"},
        {strideK, 64, 64, false, "3b4 strided-K GEMM twin matches the source"},
    };

    for (const Case &c : cases) {
      for (TwinData data :
           {TwinData::Cancellation, TwinData::Rounding, TwinData::Special}) {
        std::string claim = c.claim;
        claim += data == TwinData::Cancellation
                     ? (c.orderSensitive ? " (order-sensitive data)"
                                         : " (cancellation data)")
                 : data == TwinData::Rounding ? " (rounding data)"
                                              : " (special values)";

        if (verify(runTwin(c.gemm, data, c.visited, c.accepted,
                           data == TwinData::Cancellation &&
                               c.orderSensitive),
                   claim.c_str()))
          return 1;
      }
    }

    if (verify(runTwin(allocated, TwinData::Cancellation, 64, 64, true),
               "3b4 GEMM into a fresh allocation (no llvm.noalias) twin"
               " matches the source (order-sensitive data)"))
      return 1;

    // ---- UNKNOWN / REFUSED ----

    GemmText det9{"det9", "memref<16x16xf32>", "memref<16x16xf32>",
                  "memref<16x16xf32>", loops3, 3,
                  "%i + %j - 2 * %k + 6, %i - 2 * %j + %k + 6",
                  "%i + %j - 2 * %k + 6, %i + %j + %k",
                  "%i + %j + %k, %i - 2 * %j + %k + 6"};

    if (verify(expectTwinRefused(det9, "UNKNOWN:",
                                 "more than one congruence local"),
               "3b4 coordinate map with several congruence locals (det 9)"
               " returns UNKNOWN"))
      return 1;

    GemmText relaxed = plain;
    relaxed.name = "relaxed";
    relaxed.flags = " fastmath<reassoc>";

    if (verify(expectTwinRefused(relaxed, "UNKNOWN:", "fast-math flags"),
               "3b4 fast-math flags on the source statement return UNKNOWN"))
      return 1;

    GemmText mayAlias = plain;
    mayAlias.name = "may_alias";
    mayAlias.argumentAttrs = "";

    if (verify(expectTwinRefused(mayAlias, "UNKNOWN:", "may alias C"),
               "3b4 arguments without llvm.noalias return UNKNOWN"))
      return 1;

    GemmText readsOutput = plain;
    readsOutput.name = "reads_output";
    readsOutput.aIsC = true;

    if (verify(expectTwinRefused(readsOutput, "REFUSED:",
                                 "shares its buffer with C"),
               "3b4 A reading the C buffer is refused"))
      return 1;

    GemmText integer = plain;
    integer.name = "integer";
    integer.element = "i32";
    integer.cType = integer.aType = integer.bType = "memref<4x4xi32>";

    if (verify(expectTwinRefused(integer, "UNKNOWN:",
                                 "not a recognized GEMM"),
               "3b4 integer MAC (not recognized) returns UNKNOWN"))
      return 1;

    GemmText stepped = plain;
    stepped.name = "stepped";
    stepped.loops = "affine.for %i = 0 to 4 { affine.for %j = 0 to 4 {"
                    " affine.for %k = 0 to 8 step 2 {";
    stepped.aType = "memref<4x8xf32>";
    stepped.bType = "memref<8x4xf32>";

    if (verify(expectTwinRefused(stepped, "UNKNOWN:",
                                 "not a recognized GEMM"),
               "3b4 non-unit loop step (not recognized) returns UNKNOWN"))
      return 1;

    {
      ParsedGemm parsed;
      bool ok = parseGemm(parsed, gemmModule(plain), "plain");
      std::string why;

      if (ok) {
        OpBuilder builder(&parsed.context);
        ok = emitGuardedMacTwin(builder, parsed.outermost, "plain", why)
                 .has_value() == false &&
             why.rfind("REFUSED:", 0) == 0 &&
             why.find("twin symbol already exists") != std::string::npos;
      }

      if (verify(ok, "3b4 twin name clashing with an existing symbol is"
                     " refused"))
        return 1;
    }

    // ======================================================
    // Phase 5.4.3d-3b-5:
    // The production pass (affine-matmul-to-systolic with
    // general-affine=true, as systolic-cc runs it) replaces a
    // proven nest in place. The transformed function and the
    // untouched original are executed on identical memory and
    // compared bit for bit; the original nest's execution is
    // the oracle.
    // ======================================================

    enum class ProductionExpectation { Replaced, Unchanged, FastPath };

    // `remarks`: fragments the pass must report on the nest (with
    // report=true). Without report=true the pass must stay silent.
    auto runProduction = [&](const GemmText &gemm, TwinData data,
                             const std::string &options,
                             ProductionExpectation expectation,
                             std::size_t expectedAccepted,
                             const std::vector<std::string> &remarks) -> bool {
      const std::string label =
          gemm.name + "/" +
          (data == TwinData::Cancellation ? "cancel"
           : data == TwinData::Rounding  ? "round"
                                         : "special") +
          "{" + options + "}";

      ParsedGemm original;
      ParsedGemm produced;

      if (!parseGemm(original, gemmModule(gemm), gemm.name) ||
          !parseGemm(produced, gemmModule(gemm), gemm.name)) {
        std::cerr << "3b5 " << label << ": parse failed\n";
        return false;
      }

      const std::string before = functionText(produced.function);
      const auto topLevelBefore =
          std::distance(produced.module->getBody()->begin(),
                        produced.module->getBody()->end());
      const FunctionType signatureBefore = produced.function.getFunctionType();

      ::systolic::logical::testing::registerProductionGemmPass();

      // The pass's remarks are the production diagnostics; keep them
      // for the failure report and for the decision checks.
      std::string diagnostics;
      ScopedDiagnosticHandler diagnosticHandler(
          &produced.context, [&](Diagnostic &diagnostic) {
            diagnostics += diagnostic.str() + "\n";
            return success();
          });

      // Anchored on the module, as systolic-opt anchors on the root
      // operation it parsed; the pass is then scheduled on this
      // module itself.
      PassManager manager(&produced.context, ModuleOp::getOperationName());

      if (failed(parsePassPipeline("affine-matmul-to-systolic{" + options +
                                       "}",
                                   manager, llvm::errs()))) {
        std::cerr << "3b5 " << label << ": pipeline parse failed\n";
        return false;
      }

      if (failed(manager.run(*produced.module))) {
        std::cerr << "3b5 " << label << ": the pass failed\n";
        return false;
      }

      if (failed(mlir::verify(*produced.module))) {
        std::cerr << "3b5 " << label << ": output does not verify\n";
        return false;
      }

      // No twin or other new top-level operation; the signature
      // is kept.

      if (std::distance(produced.module->getBody()->begin(),
                        produced.module->getBody()->end()) !=
              topLevelBefore ||
          produced.function.getFunctionType() != signatureBefore) {
        std::cerr << "3b5 " << label << ": module structure changed\n";
        return false;
      }

      // The pass's report. Without report=true the replacement says
      // nothing (the established fast path reports its own refusal
      // unconditionally, as before); with report=true the decision
      // on the nest is stated, and a replacement is reported as such
      // and never as "not applied".

      const bool reporting = options.find("report=true") != std::string::npos;
      const char *replacedRemark =
          "affine GEMM replaced by its exact guarded MAC nest";
      const char *notAppliedRemark = "guarded MAC replacement not applied: ";

      auto reported = [&](const std::string &fragment) {
        return diagnostics.find(fragment) != std::string::npos;
      };

      if (!reporting &&
          (reported(replacedRemark) || reported(notAppliedRemark))) {
        std::cerr << "3b5 " << label
                  << ": guarded MAC remark without report=true:\n"
                  << diagnostics;
        return false;
      }

      if (reporting) {
        const bool expectReplaced =
            expectation == ProductionExpectation::Replaced;

        if (reported(replacedRemark) != expectReplaced ||
            (expectation == ProductionExpectation::Unchanged &&
             !reported(notAppliedRemark))) {
          std::cerr << "3b5 " << label << ": unexpected report:\n"
                    << diagnostics;
          return false;
        }

        for (const std::string &fragment : remarks) {
          if (!reported(fragment)) {
            std::cerr << "3b5 " << label << ": report lacks \"" << fragment
                      << "\":\n"
                      << diagnostics;
            return false;
          }
        }
      }

      if (expectation == ProductionExpectation::Unchanged) {
        if (functionText(produced.function) != before) {
          std::cerr << "3b5 " << label << ": function changed\n";
          return false;
        }

        return true;
      }

      if (expectation == ProductionExpectation::FastPath) {
        // The established fast path owns the nest: tiles, no guard.
        bool tiles = false, guards = false;

        produced.function.walk([&](Operation *op) {
          if (op->getName().getStringRef() == "systolic.matmul_tile")
            tiles = true;
          if (isa<scf::IfOp>(op))
            guards = true;
        });

        if (!tiles || guards) {
          std::cerr << "3b5 " << label
                    << ": the fast path did not take the nest\n";
          return false;
        }

        return true;
      }

      // Replaced: no affine memory operation remains, the nest
      // carries the exact guard and memref operations, and the
      // function body is the prologue, one nest and the return.

      std::size_t affineMemory = 0, guards = 0, memrefLoads = 0,
                  memrefStores = 0, topLoops = 0;

      produced.function.walk([&](Operation *op) {
        if (isa<affine::AffineLoadOp, affine::AffineStoreOp>(op))
          ++affineMemory;
        if (isa<scf::IfOp>(op))
          ++guards;
        if (isa<memref::LoadOp>(op))
          ++memrefLoads;
        if (isa<memref::StoreOp>(op))
          ++memrefStores;
      });

      affine::AffineForOp producedNest;

      for (Operation &op : produced.function.getBody().front()) {
        if (auto loop = dyn_cast<affine::AffineForOp>(&op)) {
          ++topLoops;
          producedNest = loop;
        }
      }

      if (affineMemory != 0 || guards != 1 || memrefLoads != 3 ||
          memrefStores != 1 || topLoops != 1 || !producedNest) {
        std::cerr << "3b5 " << label << ": unexpected replacement shape:"
                  << " affine memory ops " << affineMemory << ", guards "
                  << guards << ", memref loads " << memrefLoads
                  << ", memref stores " << memrefStores << ", top loops "
                  << topLoops << "\ndiagnostics:\n" << diagnostics;
        return false;
      }

      if (std::getenv("RANK_PARTITION_DUMP_IR")) {
        llvm::errs() << "3b5 " << label << " IR:\n";
        produced.module->print(llvm::errs());
        llvm::errs() << "\n";
      }

      // Operand roles: the original statement's loads appear as
      // C, A, B; the replacement's loads as A, B, C.

      llvm::SmallVector<Value> sourceLoads, producedLoads;
      Value sourceStore, producedStore;

      original.outermost.walk([&](Operation *op) {
        if (auto load = dyn_cast<affine::AffineLoadOp>(op))
          sourceLoads.push_back(load.getMemRef());
        else if (auto store = dyn_cast<affine::AffineStoreOp>(op))
          sourceStore = store.getMemRef();
      });

      producedNest.walk([&](Operation *op) {
        if (auto load = dyn_cast<memref::LoadOp>(op))
          producedLoads.push_back(load.getMemRef());
        else if (auto store = dyn_cast<memref::StoreOp>(op))
          producedStore = store.getMemRef();
      });

      if (sourceLoads.size() != 3 || producedLoads.size() != 3 ||
          sourceStore != sourceLoads[0] || producedStore != producedLoads[2]) {
        std::cerr << "3b5 " << label << ": unexpected statement roles\n";
        return false;
      }

      std::array<Value, 3> sourceMemrefs = {sourceLoads[1], sourceLoads[2],
                                            sourceLoads[0]};
      std::array<Value, 3> producedMemrefs = {
          producedLoads[0], producedLoads[1], producedLoads[2]};

      // The replacement reads and writes the function's own
      // buffers: the same arguments (or the same allocation) as
      // the original, by position.

      for (unsigned role = 0; role < 3; ++role) {
        auto sourceArg = dyn_cast<BlockArgument>(sourceMemrefs[role]);
        auto producedArg = dyn_cast<BlockArgument>(producedMemrefs[role]);

        // The two functions live in different contexts: types are
        // compared by their text.
        std::string sourceType, producedType;
        llvm::raw_string_ostream(sourceType) << sourceMemrefs[role].getType();
        llvm::raw_string_ostream(producedType)
            << producedMemrefs[role].getType();

        if ((sourceArg == nullptr) != (producedArg == nullptr) ||
            (sourceArg && producedArg &&
             sourceArg.getArgNumber() != producedArg.getArgNumber()) ||
            sourceType != producedType) {
          std::cerr << "3b5 " << label << ": operand " << role
                    << " is not the original buffer\n";
          return false;
        }
      }

      exec::MemoryState sourceMemory;
      exec::MemoryState producedMemory;

      for (unsigned role = 0; role < 3; ++role) {
        auto type = cast<MemRefType>(sourceMemrefs[role].getType());
        sourceMemory.buffers[sourceMemrefs[role]] = bufferOf(type, role, data);
        producedMemory.buffers[producedMemrefs[role]] =
            bufferOf(type, role, data);
      }

      auto sourceRun = exec::interpretGuardedLoopNest(
          original.outermost, scf::IfOp(), ValueRange{}, ValueRange{},
          ValueRange{}, ValueRange{}, ValueRange{}, &sourceMemory);

      auto producedRun = exec::interpretGuardedLoopNest(
          producedNest, scf::IfOp(), ValueRange{}, ValueRange{},
          ValueRange{}, ValueRange{}, ValueRange{}, &producedMemory);

      if (!sourceRun || !producedRun ||
          sourceMemory.trace.size() != 4 * expectedAccepted ||
          producedMemory.trace.size() != 4 * expectedAccepted) {
        std::cerr << "3b5 " << label << ": interpretation failed or"
                  << " executed " << producedMemory.trace.size() / 4
                  << " statements\n";
        return false;
      }

      if (data == TwinData::Cancellation && gemm.name != "stride_k") {
        auto sensitive = orderMatters(
            sourceMemory,
            bufferOf(cast<MemRefType>(sourceMemrefs[2].getType()), 2, data),
            sourceMemrefs[2]);

        if (!sensitive || !*sensitive) {
          std::cerr << "3b5 " << label
                    << ": reduction order is not observable\n";
          return false;
        }
      }

      for (unsigned role = 0; role < 3; ++role) {
        const auto &source = sourceMemory.buffers[sourceMemrefs[role]];
        const auto &mirror = producedMemory.buffers[producedMemrefs[role]];

        if (source.floatData.size() != mirror.floatData.size())
          return false;

        for (std::size_t i = 0; i < source.floatData.size(); ++i) {
          if (canonicalBits(source.floatData[i]) !=
              canonicalBits(mirror.floatData[i])) {
            std::cerr << "3b5 " << label << ": buffer " << role
                      << " differs at " << i << "\n";
            return false;
          }
        }
      }

      auto events = [&](const exec::MemoryState &memory,
                        const std::array<Value, 3> &memrefs)
          -> std::optional<std::vector<TwinEvent>> {
        std::vector<TwinEvent> out;

        for (const auto &event : memory.trace) {
          std::optional<unsigned> role;

          for (unsigned r = 0; r < 3; ++r)
            if (event.memref == memrefs[r] && !role)
              role = r;

          if (!role || !event.isFloat)
            return std::nullopt;

          out.push_back({event.isStore, *role, event.indices,
                         canonicalBits(event.floatValue)});
        }

        return out;
      };

      auto sourceEvents = events(sourceMemory, sourceMemrefs);
      auto producedEvents = events(producedMemory, producedMemrefs);

      if (!sourceEvents || !producedEvents)
        return false;

      auto storeSequences = [](const std::vector<TwinEvent> &trace) {
        std::map<std::vector<int64_t>, std::vector<int64_t>> sequences;
        for (const auto &event : trace)
          if (event.isStore)
            sequences[event.indices].push_back(event.bits);
        return sequences;
      };

      if (storeSequences(*sourceEvents) != storeSequences(*producedEvents)) {
        std::cerr << "3b5 " << label
                  << ": per-element store order differs\n";
        return false;
      }

      std::sort(sourceEvents->begin(), sourceEvents->end());
      std::sort(producedEvents->begin(), producedEvents->end());

      if (*sourceEvents != *producedEvents) {
        std::cerr << "3b5 " << label << ": event multiset differs\n";
        return false;
      }

      return true;
    };

    const std::string strictOptions = "general-affine=true report=true";

    // In strict floating point the fast path refuses every nest
    // here (its tiles reassociate), so each one reaches the
    // guarded replacement.

    for (const Case &c : cases) {
      for (TwinData data :
           {TwinData::Cancellation, TwinData::Rounding, TwinData::Special}) {
        std::string claim = "3b5 production pass: ";
        claim += c.claim + 4; // drop the "3b4 " prefix
        // In production the nest is replaced in place, not twinned.
        for (std::size_t at = claim.find(" twin "); at != std::string::npos;
             at = claim.find(" twin ", at))
          claim.replace(at, 6, " replacement ");
        claim += data == TwinData::Cancellation
                     ? (c.orderSensitive ? " (order-sensitive data)"
                                         : " (cancellation data)")
                 : data == TwinData::Rounding ? " (rounding data)"
                                              : " (special values)";

        if (verify(runProduction(c.gemm, data, strictOptions,
                                 ProductionExpectation::Replaced,
                                 c.accepted, {}),
                   claim.c_str()))
          return 1;
      }
    }

    // The replacement does not depend on reporting: without
    // report=true the pass replaces the nest and says nothing about
    // it.

    if (verify(runProduction(plain, TwinData::Cancellation,
                             "general-affine=true",
                             ProductionExpectation::Replaced, 64, {}),
               "3b5 production pass: plain GEMM replaced without report=true,"
               " silently"))
      return 1;

    if (verify(runProduction(allocated, TwinData::Cancellation, strictOptions,
                             ProductionExpectation::Replaced, 64, {}),
               "3b5 production pass: GEMM into a fresh allocation replaced"
               " and matches the source"))
      return 1;

    // Unsupported nests stay exactly as written, and the pass states
    // why, with the mapping class of the decision.

    if (verify(runProduction(mayAlias, TwinData::Cancellation, strictOptions,
                             ProductionExpectation::Unchanged, 64,
                             {"UNKNOWN: A may alias C", "[UNKNOWN]"}),
               "3b5 production pass: arguments without llvm.noalias stay"
               " unchanged (UNKNOWN)"))
      return 1;

    if (verify(runProduction(mayAlias, TwinData::Cancellation,
                             "general-affine=true assume-no-alias=true",
                             ProductionExpectation::Replaced, 64, {}),
               "3b5 production pass: assume-no-alias option admits the"
               " replacement and it matches the source"))
      return 1;

    struct UnchangedCase {
      const GemmText *gemm;
      std::vector<std::string> remarks;
    };

    const std::vector<UnchangedCase> unchangedCases = {
        {&det9, {"UNKNOWN:", "[UNKNOWN]"}},
        {&readsOutput,
         {"REFUSED: A shares its buffer with C", "[PROVEN_INVALID]"}},
        {&integer, {"UNKNOWN: not a recognized GEMM", "[UNKNOWN]"}},
        {&stepped, {"UNKNOWN: not a recognized GEMM", "[UNKNOWN]"}},
    };

    for (const UnchangedCase &u : unchangedCases) {
      std::string claim =
          "3b5 production pass: " + u.gemm->name + " stays unchanged";

      if (verify(runProduction(*u.gemm, TwinData::Cancellation, strictOptions,
                               ProductionExpectation::Unchanged, 64,
                               u.remarks),
                 claim.c_str()))
        return 1;
    }

    // A repetition loop around the nest: the preflight accepts the
    // nest (the repetition is a batch coordinate), but the emitted
    // schedule orders the batch innermost, which would change the
    // order of the additions into each C element; the loads emitter
    // refuses at commit (Phase 3b-2 order preservation), and the
    // transaction leaves the source exactly as written.

    {
      GemmText repeated = plain;
      repeated.name = "repeated";
      // (%p names the product in the generated text.)
      repeated.loops = "affine.for %r = 0 to 2 { " + loops3;
      repeated.depth = 4;

      if (verify(runProduction(repeated, TwinData::Cancellation, strictOptions,
                               ProductionExpectation::Unchanged, 128,
                               {"REFUSED: guarded memory loads emission",
                                "[VALID_BUT_LOWERING_UNSUPPORTED]"}),
                 "3b5 production pass: a repetition loop the emitted schedule"
                 " would reorder is refused at commit and stays unchanged"))
        return 1;
    }

    // A dense nest whose statement carries fastmath<reassoc> is the
    // established fast path's: it is tiled, never guarded.

    if (verify(runProduction(relaxed, TwinData::Cancellation, strictOptions,
                             ProductionExpectation::FastPath, 64, {}),
               "3b5 production pass: a reassociable dense GEMM stays with"
               " the fast path"))
      return 1;

    // The same flags on a nest the fast path cannot lower (triangular
    // domain): neither path applies, the nest stays as written.

    {
      GemmText relaxedTriangular = triangular;
      relaxedTriangular.name = "relaxed_triangular";
      relaxedTriangular.flags = " fastmath<reassoc>";

      if (verify(runProduction(relaxedTriangular, TwinData::Cancellation,
                               strictOptions,
                               ProductionExpectation::Unchanged, 40,
                               {"UNKNOWN: fast-math flags", "[UNKNOWN]"}),
                 "3b5 production pass: fast-math flags on a nest only the"
                 " guarded path could take stay unchanged"))
        return 1;
    }
  }

  std::cout << "All coordinate relation tests passed.\n";
  return 0;
}
