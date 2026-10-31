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
#include "Systolic/RankPartitionLatticeCount.h"
#include "Systolic/RankPartitionKernelCoset.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Verifier.h"
#include "llvm/ADT/DenseMap.h"
#include <iterator>
#include <optional>
#include <set>
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
#include "Systolic/RankPartitionCommonFiber.h"
#include "Systolic/RankPartitionSymbolicContraction.h"
#include "Systolic/RankPartitionSymbolicTilePartition.h"
#include "Systolic/RankPartitionReductionFlattening.h"
#include "Systolic/RankPartitionReductionScan.h"

#include "mlir/Dialect/Bufferization/IR/Bufferization.h"
#include "mlir/Dialect/Tensor/IR/Tensor.h"
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
void loadProductionDialects(mlir::MLIRContext &context);
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
      std::string modulePrologue; // e.g. a systolic.device fleet (3b-6)
    };

    auto gemmModule = [](const GemmText &g) {
      std::string closes;
      for (int i = 0; i < g.depth; ++i)
        closes += " }";

      std::string aMemref = g.aIsC ? g.cMemref : "%A";
      std::string aType = g.aIsC ? g.cType : g.aType;
      const bool floating = g.element == "f32" || g.element == "f64";
      std::string mul = floating ? "arith.mulf" : "arith.muli";
      std::string add = floating ? "arith.addf" : "arith.addi";

      std::string text = "module {\n" + g.modulePrologue +
                         "  func.func @" + g.name + "(%C: " +
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
      ::systolic::logical::testing::loadProductionDialects(parsed.context);

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

    // The same flags on a nest no path can take: the fast path refuses the
    // non-box domain, the certified device lowering refuses the partial
    // reduction fibers (K bound depending on i, Phase 3b-7), the guarded
    // MAC nest refuses the flags; the nest stays as written. (Until Phase
    // 3b-7 a flagged triangle with complete fibers was this scenario; it is
    // now offloaded through the exact output mask.)

    {
      GemmText relaxedPartial = plain;
      relaxedPartial.name = "relaxed_partial_fibers";
      relaxedPartial.loops =
          "affine.for %i = 0 to 4 { affine.for %j = 0 to 4 {"
          " affine.for %k = 0 to affine_map<(d0) -> (d0 + 1)>(%i) {";
      relaxedPartial.flags = " fastmath<reassoc>";

      if (verify(runProduction(relaxedPartial, TwinData::Cancellation,
                               strictOptions,
                               ProductionExpectation::Unchanged, 40,
                               {"UNKNOWN: fast-math flags", "[UNKNOWN]"}),
                 "3b5 production pass: fast-math flags on a nest no path can"
                 " take stay unchanged"))
        return 1;
    }

    // ------------------------------------------------------------------
    // Phase 5.4.3d-3b-6: the certified device-job lowering through the
    // production pass (RankPartitionDeviceJobLowering.h, docs section 27).
    // The decision order of the guarded path is device jobs, guarded MAC,
    // source as written; certified-offload-first (test control) tries the
    // device jobs before the established fast path, so that the two
    // producers of the same tile plan can be compared byte for byte.
    // ------------------------------------------------------------------

    const std::string fleetText =
        "  systolic.device @acc_8x8_0 rows = 8 cols = 8 dataflow = "
        "output_stationary k_max = 16 tile_overhead = 95 "
        "read_bytes_per_cycle = 4.000000e+00 write_bytes_per_cycle = "
        "3.200000e+00 read_overhead = 11 write_overhead = 11 operand_lanes = "
        "8 k_window = 8\n"
        "  systolic.device @acc_4x4_0 rows = 4 cols = 4 dataflow = "
        "output_stationary k_max = 16 tile_overhead = 95 "
        "read_bytes_per_cycle = 4.000000e+00 write_bytes_per_cycle = "
        "3.200000e+00 read_overhead = 11 write_overhead = 11 operand_lanes = "
        "8 k_window = 8\n"
        "  systolic.device @acc_4x4_1 rows = 4 cols = 4 dataflow = "
        "output_stationary k_max = 16 tile_overhead = 95 "
        "read_bytes_per_cycle = 4.000000e+00 write_bytes_per_cycle = "
        "3.200000e+00 read_overhead = 11 write_overhead = 11 operand_lanes = "
        "8 k_window = 8\n"
        "  systolic.device @acc_4x4_2 rows = 4 cols = 4 dataflow = "
        "output_stationary k_max = 16 tile_overhead = 95 "
        "read_bytes_per_cycle = 4.000000e+00 write_bytes_per_cycle = "
        "3.200000e+00 read_overhead = 11 write_overhead = 11 operand_lanes = "
        "8 k_window = 8\n";

    enum class OffloadExpectation {
      Offloaded,
      FastPath,
      GuardedMac,
      Unchanged,
      Masked, // Phase 3b-7: dense GEMM into a temporary + exact output mask
      Hybrid  // Phase 3b-8: common fibers on the device, remainder on host
    };

    struct OffloadShape {
      int64_t M = 0, N = 0, K = 0;
      // The static origin of each operand's view: A, B, C.
      std::array<std::array<int64_t, 2>, 3> origins{};
      // False: the tiles are the fast path's for another decomposition
      // (a batched or general view), not this box's partition.
      bool checkPartition = true;
      // Phase 3b-8 (Hybrid): the expected common reduction fiber of an
      // output block, in logical values; null leaves the K ranges unchecked.
      std::function<std::optional<::systolic::logical::ReductionInterval>(
          const ::systolic::logical::OutputBlock &)>
          expectedFiber;
    };

    const char *certifiedRemark =
        "by the certified device-job lowering";
    const char *certifiedRefusal = "certified device offload not applied: ";

    // Runs the pass; checks the decision and, for a lowered nest, re-derives
    // from the IR that the tiles partition [0, M) x [0, N) x [0, K) exactly
    // and read the operands at the expected origins. `text` receives the
    // function after the pass.
    auto runOffload = [&](const GemmText &gemm, const std::string &options,
                          OffloadExpectation expectation,
                          const OffloadShape &shape,
                          const std::vector<std::string> &remarks,
                          std::string *text) -> bool {
      const std::string label = gemm.name + "{" + options + "}";

      ParsedGemm produced;
      if (!parseGemm(produced, gemmModule(gemm), gemm.name)) {
        std::cerr << "3b6 " << label << ": parse failed\n";
        return false;
      }
      const std::string before = functionText(produced.function);
      const FunctionType signatureBefore = produced.function.getFunctionType();

      ::systolic::logical::testing::registerProductionGemmPass();

      std::string diagnostics;
      ScopedDiagnosticHandler diagnosticHandler(
          &produced.context, [&](Diagnostic &diagnostic) {
            diagnostics += diagnostic.str() + "\n";
            return success();
          });

      PassManager manager(&produced.context, ModuleOp::getOperationName());
      if (failed(parsePassPipeline("affine-matmul-to-systolic{" + options +
                                       "}",
                                   manager, llvm::errs()))) {
        std::cerr << "3b6 " << label << ": pipeline parse failed\n";
        return false;
      }
      if (failed(manager.run(*produced.module))) {
        std::cerr << "3b6 " << label << ": the pass failed\n" << diagnostics;
        return false;
      }
      if (failed(mlir::verify(*produced.module))) {
        std::cerr << "3b6 " << label << ": output does not verify\n";
        return false;
      }
      if (produced.function.getFunctionType() != signatureBefore) {
        std::cerr << "3b6 " << label << ": the signature changed\n";
        return false;
      }
      if (text)
        *text = functionText(produced.function);

      auto reported = [&](const std::string &fragment) {
        return diagnostics.find(fragment) != std::string::npos;
      };
      for (const std::string &fragment : remarks)
        if (!reported(fragment)) {
          std::cerr << "3b6 " << label << ": report lacks \"" << fragment
                    << "\":\n"
                    << diagnostics;
          return false;
        }

      // The IR after the pass.
      unsigned affineOps = 0, guards = 0, tiles = 0;
      SmallVector<Operation *> tileOps;
      SmallVector<bufferization::ToTensorOp> tensors;
      produced.function.walk([&](Operation *op) {
        if (isa<affine::AffineForOp, affine::AffineLoadOp,
                affine::AffineStoreOp>(op))
          ++affineOps;
        if (isa<scf::IfOp>(op))
          ++guards;
        if (op->getName().getStringRef() == "systolic.matmul_tile") {
          ++tiles;
          tileOps.push_back(op);
        }
        if (auto toTensor = dyn_cast<bufferization::ToTensorOp>(op))
          tensors.push_back(toTensor);
      });

      const bool certified = reported(certifiedRemark);
      const bool masked = reported("with an exact output mask");
      unsigned allocations = 0;
      produced.function.walk([&](memref::AllocOp) { ++allocations; });

      const bool hybrid = reported("(section 29)");

      switch (expectation) {
      case OffloadExpectation::Hybrid:
        if (tiles == 0 || guards == 0 || !certified || !hybrid ||
            allocations != 0 || masked) {
          std::cerr << "3b8 " << label << ": expected the hybrid lowering"
                    << " (tiles " << tiles << ", guards " << guards
                    << ", allocations " << allocations << ")\n"
                    << diagnostics;
          return false;
        }
        break;
      case OffloadExpectation::Masked:
        if (tiles == 0 || guards != 1 || !certified || !masked ||
            allocations != 1) {
          std::cerr << "3b7 " << label << ": expected the masked device"
                    << " jobs (tiles " << tiles << ", guards " << guards
                    << ", allocations " << allocations << ")\n"
                    << diagnostics;
          return false;
        }
        break;
      case OffloadExpectation::Unchanged:
        if (functionText(produced.function) != before || certified) {
          std::cerr << "3b6 " << label << ": the function changed\n"
                    << diagnostics;
          return false;
        }
        return true;
      case OffloadExpectation::GuardedMac:
        if (tiles != 0 || guards != 1 || certified) {
          std::cerr << "3b6 " << label << ": expected the guarded MAC nest"
                    << " (tiles " << tiles << ", guards " << guards << ")\n"
                    << diagnostics;
          return false;
        }
        return true;
      case OffloadExpectation::FastPath:
        if (tiles == 0 || certified) {
          std::cerr << "3b6 " << label << ": expected the fast path's tiles"
                    << " (tiles " << tiles << ")\n"
                    << diagnostics;
          return false;
        }
        break;
      case OffloadExpectation::Offloaded:
        if (tiles == 0 || affineOps != 0 || guards != 0 || !certified ||
            masked || hybrid) {
          std::cerr << "3b6 " << label << ": expected the certified device"
                    << " jobs (tiles " << tiles << ", affine ops "
                    << affineOps << ", guards " << guards << ")\n"
                    << diagnostics;
          return false;
        }
        break;
      }

      if (!shape.checkPartition)
        return true;

      if (expectation == OffloadExpectation::Hybrid) {
        // Phase 3b-8: every tile's output block (its C slice) runs exactly
        // its common reduction fiber (its A slice's K range), the blocks'
        // folds contiguous and ascending; the device MACs of the tiles
        // equal the remark's count.
        struct BlockRange {
          int64_t row, column, m, n;
          int64_t first = -1, next = -1; // K range covered so far
        };
        std::vector<BlockRange> ranges;
        llvm::DenseMap<Operation *, size_t> blockOfTile;
        int64_t deviceMacs = 0;
        for (Operation *tile : tileOps) {
          auto attr = [&](StringRef name) {
            return tile->getAttrOfType<IntegerAttr>(name).getInt();
          };
          const int64_t m = attr("m"), n = attr("n"), k = attr("k");
          auto aSlice =
              tile->getOperand(0).getDefiningOp<tensor::ExtractSliceOp>();
          if (!aSlice || aSlice.getStaticOffsets().size() != 2) {
            std::cerr << "3b8 " << label << ": a tile's A operand is not a"
                      << " static slice\n";
            return false;
          }
          const int64_t k0 = aSlice.getStaticOffsets()[1];
          Value cIn = tile->getOperand(2);
          size_t index;
          if (auto slice = cIn.getDefiningOp<tensor::ExtractSliceOp>()) {
            auto offsets = slice.getStaticOffsets();
            ranges.push_back({offsets[0], offsets[1], m, n, k0, k0});
            index = ranges.size() - 1;
          } else if (Operation *previous = cIn.getDefiningOp();
                     previous && blockOfTile.count(previous)) {
            index = blockOfTile[previous];
          } else if (cIn.getDefiningOp<bufferization::ToTensorOp>()) {
            ranges.push_back({0, 0, m, n, k0, k0});
            index = ranges.size() - 1;
          } else {
            std::cerr << "3b8 " << label << ": a tile's c_in is unexpected\n";
            return false;
          }
          blockOfTile[tile] = index;
          if (ranges[index].next != k0) {
            std::cerr << "3b8 " << label << ": a block's folds are not"
                      << " contiguous\n";
            return false;
          }
          ranges[index].next = k0 + k;
          deviceMacs += m * n * k;
        }
        for (const BlockRange &r : ranges) {
          if (!shape.expectedFiber)
            break;
          auto fiber = shape.expectedFiber(
              {r.row, r.row + r.m - 1, r.column, r.column + r.n - 1});
          if (!fiber || fiber->empty || fiber->lower != r.first ||
              fiber->upper != r.next - 1) {
            std::cerr << "3b8 " << label << ": the block at (" << r.row
                      << ", " << r.column << ") runs K [" << r.first << ", "
                      << r.next - 1 << "), not its common fiber\n";
            return false;
          }
        }
        // "(<device> of <total> MACs)" in the remark.
        const std::string key = "on the device (";
        const auto at = diagnostics.find(key);
        if (at == std::string::npos) {
          std::cerr << "3b8 " << label << ": no MAC count in the remark\n";
          return false;
        }
        const int64_t reported =
            std::stoll(diagnostics.substr(at + key.size()));
        if (reported != deviceMacs) {
          std::cerr << "3b8 " << label << ": the remark counts " << reported
                    << " device MACs, the tiles " << deviceMacs << "\n";
          return false;
        }
        return true;
      }

      // The tiles partition the GEMM: each output block is an
      // extract_slice of C (static offsets and sizes), its K folds chain
      // through c_in in order and sum to K; the blocks lie inside C, are
      // pairwise disjoint and cover it.
      struct Block {
        int64_t row, column, m, n;
        int64_t k = 0;
        int64_t folds = 0;
      };
      std::vector<Block> blocks;
      llvm::DenseMap<Operation *, size_t> blockOfTile;
      for (Operation *tile : tileOps) {
        auto attr = [&](StringRef name) {
          auto value = tile->getAttrOfType<IntegerAttr>(name);
          return value ? value.getInt() : int64_t(-1);
        };
        const int64_t m = attr("m"), n = attr("n"), k = attr("k");
        Value cIn = tile->getOperand(2);
        size_t index;
        if (auto slice = cIn.getDefiningOp<tensor::ExtractSliceOp>()) {
          auto offsets = slice.getStaticOffsets();
          auto sizes = slice.getStaticSizes();
          if (offsets.size() != 2 || sizes.size() != 2 || sizes[0] != m ||
              sizes[1] != n) {
            std::cerr << "3b6 " << label << ": a tile's C slice is not its "
                      << "m x n block\n";
            return false;
          }
          blocks.push_back({offsets[0], offsets[1], m, n});
          index = blocks.size() - 1;
        } else if (Operation *previous = cIn.getDefiningOp();
                   previous && blockOfTile.count(previous)) {
          index = blockOfTile[previous];
          if (blocks[index].m != m || blocks[index].n != n) {
            std::cerr << "3b6 " << label << ": a K fold changes the block\n";
            return false;
          }
        } else if (cIn.getDefiningOp<bufferization::ToTensorOp>()) {
          blocks.push_back({0, 0, m, n}); // the whole GEMM, no fleet
          index = blocks.size() - 1;
        } else {
          std::cerr << "3b6 " << label << ": a tile's c_in is unexpected\n";
          return false;
        }
        blockOfTile[tile] = index;
        if (k <= 0) {
          std::cerr << "3b6 " << label << ": a tile has no reduction\n";
          return false;
        }
        if (auto kIndex = tile->getAttrOfType<IntegerAttr>("systolic.k_index");
            kIndex && kIndex.getInt() != blocks[index].folds) {
          std::cerr << "3b6 " << label << ": K folds out of order\n";
          return false;
        }
        blocks[index].k += k;
        ++blocks[index].folds;
      }
      int64_t area = 0;
      for (const Block &b : blocks) {
        if (b.row < 0 || b.column < 0 || b.m <= 0 || b.n <= 0 ||
            b.row + b.m > shape.M || b.column + b.n > shape.N ||
            b.k != shape.K) {
          std::cerr << "3b6 " << label << ": a block lies outside the GEMM"
                    << " or its folds do not sum to K\n";
          return false;
        }
        area += b.m * b.n;
      }
      for (size_t i = 0; i < blocks.size(); ++i)
        for (size_t j = i + 1; j < blocks.size(); ++j) {
          const Block &p = blocks[i], &q = blocks[j];
          if (p.row < q.row + q.m && q.row < p.row + p.m &&
              p.column < q.column + q.n && q.column < p.column + p.n) {
            std::cerr << "3b6 " << label << ": two blocks overlap\n";
            return false;
          }
        }
      if (area != shape.M * shape.N) {
        std::cerr << "3b6 " << label << ": the blocks cover " << area
                  << " of " << shape.M * shape.N << " elements\n";
        return false;
      }

      // The operand views: A, B, C tensors taken at the expected origins
      // (masked: C's tensor is the fresh temporary).
      if (expectation == OffloadExpectation::Offloaded ||
          expectation == OffloadExpectation::Masked) {
        if (tensors.size() != 3) {
          std::cerr << "3b6 " << label << ": " << tensors.size()
                    << " operand tensors\n";
          return false;
        }
        for (unsigned a = 0; a < 3; ++a) {
          Value source = tensors[a].getBuffer();
          if (a == 2 && expectation == OffloadExpectation::Masked) {
            if (!source.getDefiningOp<memref::AllocOp>()) {
              std::cerr << "3b7 " << label << ": the dense GEMM does not"
                        << " run into the temporary\n";
              return false;
            }
            continue;
          }
          std::array<int64_t, 2> origin{0, 0};
          if (auto view = source.getDefiningOp<memref::SubViewOp>()) {
            auto offsets = view.getStaticOffsets();
            if (offsets.size() != 2) {
              std::cerr << "3b6 " << label << ": a view is not rank 2\n";
              return false;
            }
            origin = {offsets[0], offsets[1]};
            source = view.getSource();
          }
          if (!isa<BlockArgument>(source) || origin != shape.origins[a]) {
            std::cerr << "3b6 " << label << ": operand " << a
                      << " is not read at its origin (" << origin[0] << ", "
                      << origin[1] << ")\n";
            return false;
          }
        }
      }
      return true;
    };

    const std::string certifiedOptions =
        "general-affine=true certified-offload-first=true "
        "allow-fp-reassociation=true report=true";
    const std::string fastOptions =
        "general-affine=true allow-fp-reassociation=true report=true";
    const std::string strictCertifiedOptions =
        "general-affine=true certified-offload-first=true report=true";

    // The two producers of one tile plan agree byte for byte.
    auto sameAsFastPath = [&](const GemmText &gemm, const OffloadShape &shape,
                              const char *claim) -> bool {
      std::string certifiedText, fastText;
      if (!runOffload(gemm, certifiedOptions, OffloadExpectation::Offloaded,
                      shape, {certifiedRemark}, &certifiedText) ||
          !runOffload(gemm, fastOptions, OffloadExpectation::FastPath, shape,
                      {" lowered as "}, &fastText))
        return false;
      if (certifiedText != fastText) {
        std::cerr << "3b6 " << gemm.name << ": the certified lowering and "
                  << "the fast path differ:\n--- certified ---\n"
                  << certifiedText << "\n--- fast path ---\n" << fastText;
        return false;
      }
      return verify(true, claim) == 0;
    };

    auto withFleet = [&](GemmText gemm, const char *name) {
      gemm.name = name;
      gemm.modulePrologue = fleetText;
      return gemm;
    };
    auto loopsOf = [](int M, int N, int K) {
      return "affine.for %i = 0 to " + std::to_string(M) +
             " { affine.for %j = 0 to " + std::to_string(N) +
             " { affine.for %k = 0 to " + std::to_string(K) + " {";
    };
    auto square = [](int M, int N, int K, const char *name) {
      return GemmText{name,
                      "memref<" + std::to_string(M) + "x" + std::to_string(N) +
                          "xf32>",
                      "memref<" + std::to_string(M) + "x" + std::to_string(K) +
                          "xf32>",
                      "memref<" + std::to_string(K) + "x" + std::to_string(N) +
                          "xf32>",
                      "", 3, "%i, %j", "%i, %k", "%k, %j"};
    };

    // Standard contiguous GEMMs: whole matrices, the fleet's plan.
    {
      GemmText small = withFleet(plain, "device_plain");
      if (!sameAsFastPath(small, {4, 4, 4, {}},
                          "3b6 device jobs: 4x4x4 GEMM offloaded, identical"
                          " to the fast path"))
        return 1;

      GemmText big = square(16, 16, 16, "device_16");
      big.loops = loopsOf(16, 16, 16);
      big = withFleet(big, "device_16");
      if (!sameAsFastPath(big, {16, 16, 16, {}},
                          "3b6 device jobs: 16x16x16 GEMM over the fleet"
                          " (10 blocks), identical to the fast path"))
        return 1;

      GemmText deep = square(8, 8, 40, "device_k40");
      deep.loops = loopsOf(8, 8, 40);
      deep = withFleet(deep, "device_k40");
      if (!sameAsFastPath(deep, {8, 8, 40, {}},
                          "3b6 device jobs: K = 40 folded by k_max in order,"
                          " identical to the fast path"))
        return 1;

      GemmText partial = square(6, 8, 8, "device_partial");
      partial.loops = loopsOf(6, 8, 8);
      partial = withFleet(partial, "device_partial");
      if (!sameAsFastPath(partial, {6, 8, 8, {}},
                          "3b6 device jobs: partial tile (M = 6), identical"
                          " to the fast path"))
        return 1;

      // The reduction loop outermost: the same jobs.
      GemmText kOuter = plain;
      kOuter.loops = "affine.for %k = 0 to 4 { affine.for %i = 0 to 4 {"
                     " affine.for %j = 0 to 4 {";
      kOuter = withFleet(kOuter, "device_k_outer");
      if (!sameAsFastPath(kOuter, {4, 4, 4, {}},
                          "3b6 device jobs: reduction loop outermost,"
                          " identical to the fast path"))
        return 1;

      // A sub-box with constant offsets: static subviews at the origins.
      GemmText offsets{"device_offsets", "memref<6x6xf32>", "memref<6x6xf32>",
                       "memref<6x6xf32>", loops3, 3, "%i + 1, %j + 2",
                       "%i + 1, %k", "%k, %j + 2"};
      offsets = withFleet(offsets, "device_offsets");
      if (!sameAsFastPath(offsets, {4, 4, 4, {{{1, 0}, {0, 2}, {1, 2}}}},
                          "3b6 device jobs: views with constant offsets,"
                          " identical to the fast path"))
        return 1;

      // No fleet: one unassigned tile over the whole GEMM.
      if (verify(runOffload(plain, certifiedOptions,
                            OffloadExpectation::Offloaded, {4, 4, 4, {}},
                            {certifiedRemark}, nullptr),
                 "3b6 device jobs: without a fleet one whole-GEMM tile"))
        return 1;

      // Permission by the reassoc flag alone.
      GemmText flagged = withFleet(relaxed, "device_reassoc");
      if (verify(runOffload(flagged, strictCertifiedOptions,
                            OffloadExpectation::Offloaded, {4, 4, 4, {}},
                            {certifiedRemark}, nullptr),
                 "3b6 device jobs: the reassoc flag permits the offload"))
        return 1;
    }

    // Strict semantics: no offload; the guarded MAC nest takes the nest.
    if (verify(runOffload(withFleet(plain, "strict_plain"),
                          strictCertifiedOptions,
                          OffloadExpectation::GuardedMac, {4, 4, 4, {}},
                          {std::string(certifiedRefusal) +
                               "UNKNOWN: strict floating-point semantics",
                           "replaced by its exact guarded MAC nest"},
                          nullptr),
               "3b6 device jobs: strict floating point is never offloaded;"
               " the guarded MAC nest follows"))
      return 1;

    // Outside the contract: UNKNOWN with the reason, the next decision
    // takes over (the fast path where it can, else the guarded MAC nest).
    // An f64 nest is refused upstream: the symbolic recognizer admits f32
    // accumulating MACs only (the device contract's J9 is then never
    // reached); the fast path lowers an f64 tile, which systolic-cc does
    // not offload.
    struct Refusal {
      GemmText gemm;
      OffloadExpectation next;
      OffloadShape shape;
      std::string reason;
      const char *claim;
    };
    GemmText f64 = plain;
    f64.name = "device_f64";
    f64.element = "f64";
    f64.cType = f64.aType = f64.bType = "memref<4x4xf64>";
    GemmText transposed = plain;
    transposed.name = "device_transposed";
    transposed.aIndex = "%k, %i";
    GemmText partialFibers = plain;
    partialFibers.name = "device_partial_fibers";
    partialFibers.loops =
        "affine.for %i = 0 to 4 { affine.for %j = 0 to 4 {"
        " affine.for %k = 0 to affine_map<(d0) -> (d0 + 1)>(%i) {";
    GemmText repeatedDevice = plain;
    repeatedDevice.name = "device_repeated";
    repeatedDevice.loops = "affine.for %r = 0 to 2 { " + loops3;
    repeatedDevice.depth = 4;

    const std::vector<Refusal> refusals = {
        {withFleet(f64, "device_f64"), OffloadExpectation::FastPath,
         {4, 4, 4, {}}, "",
         "3b6 device jobs: f64 is not a recognized GEMM (f32 only), the"
         " fast path's tile is not the certified lowering's"},
        // (Until Phase 3b-7 a triangle with complete reduction fibers was
        // the non-box refusal here; it is now offloaded through the exact
        // output mask, so the non-box domain no path can take is one whose
        // K bound depends on i: partial fibers.)
        {withFleet(partialFibers, "device_partial_fibers"),
         OffloadExpectation::GuardedMac, {4, 4, 4, {}},
         "UNKNOWN: the reduction fiber of an output coordinate is a proper "
         "part of the K range",
         "3b6 device jobs: a non-box domain with partial reduction fibers"
         " is UNKNOWN; the guarded MAC nest follows"},
        {withFleet(stretch, "device_stretch"), OffloadExpectation::FastPath,
         {4, 4, 4, {}}, "UNKNOWN: the coordinate map is not unimodular",
         "3b6 device jobs: a non-unimodular map is UNKNOWN"},
        {withFleet(transposed, "device_transposed"),
         OffloadExpectation::FastPath, {4, 4, 4, {}},
         "UNKNOWN: A is not read as (I, K)",
         "3b6 device jobs: a transposed factor is UNKNOWN"},
        {withFleet(repeatedDevice, "device_repeated"),
         OffloadExpectation::FastPath, {4, 4, 4, {}},
         "UNKNOWN: the nest has 4 logical coordinates",
         "3b6 device jobs: a repetition (batch) loop is UNKNOWN"},
    };
    for (const Refusal &r : refusals) {
      // The fast path's tiles for a batched or general view are not the
      // M x N x K partition of this nest's box: the IR check is skipped
      // for them by giving the fast path its own extents.
      OffloadShape shape = r.shape;
      if (r.next == OffloadExpectation::FastPath &&
          (r.gemm.name == "device_stretch" ||
           r.gemm.name == "device_repeated"))
        shape.checkPartition = false;
      std::vector<std::string> expected;
      if (!r.reason.empty())
        expected.push_back(std::string(certifiedRefusal) + r.reason);
      else
        expected.push_back("UNKNOWN: not a recognized GEMM: not one f32 "
                           "accumulating MAC");
      if (verify(runOffload(r.gemm, certifiedOptions, r.next, shape, expected,
                            nullptr),
                 r.claim))
        return 1;
    }

    // Strided K (A[i][2k]): a non-unit coefficient.
    {
      GemmText strided = plain;
      strided.name = "device_stride_k";
      strided.loops = "affine.for %i = 0 to 4 { affine.for %j = 0 to 4 {"
                      " affine.for %k = 0 to 4 {";
      strided.aType = "memref<4x8xf32>";
      strided.bType = "memref<8x4xf32>";
      strided.aIndex = "%i, 2 * %k";
      strided.bIndex = "2 * %k, %j";
      if (verify(runOffload(withFleet(strided, "device_stride_k"),
                            certifiedOptions, OffloadExpectation::FastPath,
                            {4, 4, 4, {}},
                            {std::string(certifiedRefusal) +
                             "UNKNOWN: operand A reads its dimension 1 "
                             "through a strided"},
                            nullptr),
                 "3b6 device jobs: a strided factor view is UNKNOWN"))
        return 1;
    }

    // Memory, decided by the shared preflight before either path: may-alias
    // arguments are UNKNOWN (nothing else takes the nest either);
    // assume-no-alias admits the jobs; an input reading C's buffer is
    // REFUSED.
    if (verify(runOffload(withFleet(mayAlias, "device_may_alias"),
                          certifiedOptions, OffloadExpectation::Unchanged,
                          {4, 4, 4, {}}, {"UNKNOWN: A may alias C"},
                          nullptr),
               "3b6 device jobs: arguments without llvm.noalias are UNKNOWN"
               " and stay unchanged"))
      return 1;
    if (verify(runOffload(withFleet(mayAlias, "device_assume"),
                          certifiedOptions + " assume-no-alias=true",
                          OffloadExpectation::Offloaded, {4, 4, 4, {}},
                          {certifiedRemark}, nullptr),
               "3b6 device jobs: assume-no-alias admits the jobs"))
      return 1;
    if (verify(runOffload(withFleet(readsOutput, "device_reads_output"),
                          certifiedOptions, OffloadExpectation::Unchanged,
                          {4, 4, 4, {}},
                          {"REFUSED: A shares its buffer with C",
                           "[PROVEN_INVALID]"},
                          nullptr),
               "3b6 device jobs: a factor reading C's buffer is REFUSED"))
      return 1;

    // ------------------------------------------------------------------
    // Phase 5.4.3d-3b-7: exact non-box domains through dense device jobs
    // and an exact output mask (docs section 28). The executed scenarios
    // run the pass without a fleet (one whole-box tile), execute the
    // produced function with the software runtime's arithmetic for the
    // tile, and compare with the original nest: selected outputs under the
    // declared contract, unselected outputs bitwise unchanged.
    // ------------------------------------------------------------------

    // Runs the pass on `produced`; the diagnostics are collected.
    auto runPass = [&](ParsedGemm &produced, const std::string &options,
                       std::string &diagnostics) -> bool {
      ::systolic::logical::testing::registerProductionGemmPass();
      ScopedDiagnosticHandler diagnosticHandler(
          &produced.context, [&](Diagnostic &diagnostic) {
            diagnostics += diagnostic.str() + "\n";
            return success();
          });
      PassManager manager(&produced.context, ModuleOp::getOperationName());
      if (failed(parsePassPipeline("affine-matmul-to-systolic{" + options +
                                       "}",
                                   manager, llvm::errs())))
        return false;
      if (failed(manager.run(*produced.module)))
        return false;
      return succeeded(mlir::verify(*produced.module));
    };

    // The buffer behind a tensor operand: the memref, or the base of a
    // static unit-stride subview with its origin.
    struct ViewRef {
      Value memref;
      std::array<int64_t, 2> origin{0, 0};
    };
    // The buffer behind a memref value: itself, or the base of a static
    // unit-stride subview with its origin.
    auto resolveMemref = [](Value memref) -> std::optional<ViewRef> {
      ViewRef ref;
      ref.memref = memref;
      if (auto view = memref.getDefiningOp<memref::SubViewOp>()) {
        auto offsets = view.getStaticOffsets();
        auto strides = view.getStaticStrides();
        if (offsets.size() != 2 || strides.size() != 2 || strides[0] != 1 ||
            strides[1] != 1)
          return std::nullopt;
        ref.origin = {offsets[0], offsets[1]};
        ref.memref = view.getSource();
      }
      return ref;
    };
    // The buffer behind a tensor operand: a to_tensor, possibly under static
    // unit-stride extract_slices (Phase 3b-8: the K slices of A and B).
    std::function<std::optional<ViewRef>(Value)> viewOf =
        [&](Value tensor) -> std::optional<ViewRef> {
      if (auto slice = tensor.getDefiningOp<tensor::ExtractSliceOp>()) {
        auto inner = viewOf(slice.getSource());
        auto offsets = slice.getStaticOffsets();
        auto strides = slice.getStaticStrides();
        if (!inner || offsets.size() != 2 || strides.size() != 2 ||
            strides[0] != 1 || strides[1] != 1)
          return std::nullopt;
        inner->origin[0] += offsets[0];
        inner->origin[1] += offsets[1];
        return inner;
      }
      auto toTensor = tensor.getDefiningOp<bufferization::ToTensorOp>();
      if (!toTensor)
        return std::nullopt;
      return resolveMemref(toTensor.getBuffer());
    };

    // Executes the function after the pass: affine nests with the
    // interpreter, memref.alloc as a fresh buffer (NaN-filled until
    // written), and the fleet-less dense GEMM (three to_tensor, one
    // whole-box matmul_tile, materialize_in_destination) with the software
    // runtime's arithmetic: per element a binary32 sum of binary32 products
    // from +0 in ascending k, then one add into the destination's value.
    auto executeOffloaded = [&](func::FuncOp function,
                                exec::MemoryState &memory) -> bool {
      auto bufferAt = [&](Value memref) -> exec::MemoryBuffer * {
        auto it = memory.buffers.find(memref);
        return it == memory.buffers.end() ? nullptr : &it->second;
      };
      for (Operation &op : function.getBody().front()) {
        if (auto loop = dyn_cast<affine::AffineForOp>(&op)) {
          auto run = exec::interpretGuardedLoopNest(
              loop, scf::IfOp(), ValueRange{}, ValueRange{}, ValueRange{},
              ValueRange{}, ValueRange{}, &memory);
          if (!run) {
            std::cerr << "3b7: a nest of the produced function did not "
                         "execute\n";
            return false;
          }
          continue;
        }
        if (auto alloc = dyn_cast<memref::AllocOp>(&op)) {
          auto type = alloc.getType();
          if (!type.hasStaticShape() || !type.getElementType().isF32())
            return false;
          exec::MemoryBuffer buffer;
          buffer.shape.assign(type.getShape().begin(), type.getShape().end());
          buffer.isFloat = true;
          buffer.floatData.assign(static_cast<std::size_t>(type.getNumElements()),
                                  std::numeric_limits<double>::quiet_NaN());
          memory.buffers[alloc.getResult()] = std::move(buffer);
          continue;
        }
        if (isa<arith::ConstantOp, bufferization::ToTensorOp,
                memref::SubViewOp, tensor::ExtractSliceOp>(&op) ||
            op.getName().getStringRef() == "systolic.matmul_tile")
          continue; // consumed by the write-back below
        if (auto writeBack =
                dyn_cast<bufferization::MaterializeInDestinationOp>(&op)) {
          Operation *tile = writeBack.getSource().getDefiningOp();
          if (!tile || tile->getName().getStringRef() != "systolic.matmul_tile" ||
              tile->hasAttr("device")) {
            std::cerr << "3b7: expected one fleet-less whole-box tile\n";
            return false;
          }
          auto attr = [&](StringRef name) {
            return tile->getAttrOfType<IntegerAttr>(name).getInt();
          };
          const int64_t m = attr("m"), n = attr("n"), k = attr("k");
          auto a = viewOf(tile->getOperand(0));
          auto b = viewOf(tile->getOperand(1));
          auto c = viewOf(tile->getOperand(2));
          auto destination = resolveMemref(writeBack.getDest());
          if (!a || !b || !c || !destination ||
              destination->memref != c->memref ||
              destination->origin != c->origin) {
            std::cerr << "3b7: the tile's operands are not plain views\n";
            return false;
          }
          exec::MemoryBuffer *bufA = bufferAt(a->memref);
          exec::MemoryBuffer *bufB = bufferAt(b->memref);
          exec::MemoryBuffer *bufC = bufferAt(c->memref);
          if (!bufA || !bufB || !bufC || !bufA->isFloat || !bufB->isFloat ||
              !bufC->isFloat || bufA->shape.size() != 2 ||
              bufB->shape.size() != 2 || bufC->shape.size() != 2 ||
              c->origin[0] + m > bufC->shape[0] ||
              c->origin[1] + n > bufC->shape[1] ||
              a->origin[0] + m > bufA->shape[0] ||
              a->origin[1] + k > bufA->shape[1] ||
              b->origin[0] + k > bufB->shape[0] ||
              b->origin[1] + n > bufB->shape[1]) {
            std::cerr << "3b7: the tile's views do not fit the buffers\n";
            return false;
          }
          for (int64_t r = 0; r < m; ++r)
            for (int64_t col = 0; col < n; ++col) {
              volatile float acc = 0.0f;
              for (int64_t kk = 0; kk < k; ++kk) {
                volatile float left = static_cast<float>(
                    bufA->floatData[(a->origin[0] + r) * bufA->shape[1] +
                                    a->origin[1] + kk]);
                volatile float right = static_cast<float>(
                    bufB->floatData[(b->origin[0] + kk) * bufB->shape[1] +
                                    b->origin[1] + col]);
                volatile float product = left * right;
                acc = acc + product;
              }
              const std::size_t cIndex =
                  (c->origin[0] + r) * bufC->shape[1] + c->origin[1] + col;
              volatile float prior = static_cast<float>(bufC->floatData[cIndex]);
              volatile float out = prior + acc;
              bufC->floatData[cIndex] = out;
            }
          continue;
        }
        if (isa<func::ReturnOp>(&op))
          break;
        std::cerr << "3b7: unexpected top-level operation "
                  << op.getName().getStringRef().str() << "\n";
        return false;
      }
      return true;
    };

    enum class MaskData { Integers, Special, Rounding };

    // A, B, C per family. Integers: nonzero small integers (every partial
    // sum exact: the relaxed contract is bitwise). Special: zeros and
    // negative factors (-0 products), NaN and -0 in C. Rounding: the
    // inexact products of the 3b-3 family (the contract is a tolerance).
    auto maskBuffer = [&](MemRefType type, unsigned role, MaskData data) {
      if (data == MaskData::Rounding)
        return bufferOf(type, role, TwinData::Rounding);
      exec::MemoryBuffer buffer;
      buffer.shape.assign(type.getShape().begin(), type.getShape().end());
      buffer.isFloat = true;
      const std::size_t columns = static_cast<std::size_t>(buffer.shape[1]);
      const std::size_t size =
          static_cast<std::size_t>(buffer.shape[0]) * columns;
      for (std::size_t linear = 0; linear < size; ++linear) {
        const std::size_t r = linear / columns, c = linear % columns;
        double value = 0.0;
        if (data == MaskData::Integers) {
          const int magnitude = static_cast<int>((r * 7 + c * 3 + role) % 9) + 1;
          value = ((r + c + role) % 2 == 0 ? 1.0 : -1.0) * magnitude;
        } else {
          if (role == 2) {
            switch (linear % 5) {
            case 0:
              value = std::numeric_limits<double>::quiet_NaN();
              break;
            case 1:
              value = -0.0;
              break;
            case 2:
              value = 0.0;
              break;
            default:
              value = static_cast<double>(static_cast<int>(linear % 7) - 3);
            }
          } else {
            // Rows of zeros and negative factors give -0 products.
            value = (r % 3 == 0) ? 0.0
                                 : static_cast<double>(
                                       static_cast<int>((r * 5 + c) % 5) - 2);
          }
        }
        buffer.floatData.push_back(value);
      }
      return buffer;
    };

    // Equality of two f32 values under the relaxed contract of section 6
    // on exact data: NaN matches NaN, a zero matches a zero of either
    // sign, everything else bitwise.
    auto contractEqual = [](double expected, double actual, MaskData data) {
      if (std::isnan(expected) || std::isnan(actual))
        return std::isnan(expected) && std::isnan(actual);
      if (data == MaskData::Rounding) {
        if (std::isinf(expected) || std::isinf(actual))
          return expected == actual;
        const double scale = std::max(std::fabs(expected), 1.0);
        return std::fabs(expected - actual) <= 1e-5 * scale;
      }
      if (expected == 0.0 && actual == 0.0)
        return true;
      return std::memcmp(&expected, &actual, sizeof(double)) == 0;
    };

    // The executed differential scenario.
    auto runMaskedExecution = [&](const GemmText &gemm, MaskData data,
                                  const std::string &options) -> bool {
      const std::string label =
          gemm.name + "/" +
          (data == MaskData::Integers ? "integers"
           : data == MaskData::Special ? "special"
                                       : "rounding") +
          "{" + options + "}";
      ParsedGemm original, produced;
      if (!parseGemm(original, gemmModule(gemm), gemm.name) ||
          !parseGemm(produced, gemmModule(gemm), gemm.name)) {
        std::cerr << "3b7 " << label << ": parse failed\n";
        return false;
      }
      std::string diagnostics;
      if (!runPass(produced, options, diagnostics)) {
        std::cerr << "3b7 " << label << ": the pass failed\n" << diagnostics;
        return false;
      }
      if (diagnostics.find("with an exact output mask") == std::string::npos) {
        std::cerr << "3b7 " << label << ": not lowered with the mask\n"
                  << diagnostics;
        return false;
      }

      // The same data behind both functions' arguments.
      exec::MemoryState sourceMemory, producedMemory;
      std::array<Value, 3> sourceArgs, producedArgs; // C, A, B
      for (unsigned i = 0; i < 3; ++i) {
        sourceArgs[i] = original.function.getArgument(i);
        producedArgs[i] = produced.function.getArgument(i);
      }
      // roles: argument 0 is C (role 2), 1 is A (role 0), 2 is B (role 1)
      const unsigned roleOf[3] = {2, 0, 1};
      for (unsigned i = 0; i < 3; ++i) {
        auto type = cast<MemRefType>(sourceArgs[i].getType());
        sourceMemory.buffers[sourceArgs[i]] = maskBuffer(type, roleOf[i], data);
        producedMemory.buffers[producedArgs[i]] =
            maskBuffer(type, roleOf[i], data);
      }
      const exec::MemoryBuffer initialC = sourceMemory.buffers[sourceArgs[0]];

      auto sourceRun = exec::interpretGuardedLoopNest(
          original.outermost, scf::IfOp(), ValueRange{}, ValueRange{},
          ValueRange{}, ValueRange{}, ValueRange{}, &sourceMemory);
      if (!sourceRun) {
        std::cerr << "3b7 " << label << ": the source nest did not execute\n";
        return false;
      }
      if (!executeOffloaded(produced.function, producedMemory))
        return false;

      // Selected: the elements the source nest stored.
      const exec::MemoryBuffer &sourceC = sourceMemory.buffers[sourceArgs[0]];
      const exec::MemoryBuffer &producedC =
          producedMemory.buffers[producedArgs[0]];
      std::vector<bool> selected(initialC.floatData.size(), false);
      std::size_t stores = 0;
      for (const exec::MemoryEvent &event : sourceMemory.trace) {
        if (!event.isStore || event.memref != sourceArgs[0])
          continue;
        const std::size_t linear =
            static_cast<std::size_t>(event.indices[0]) *
                static_cast<std::size_t>(initialC.shape[1]) +
            static_cast<std::size_t>(event.indices[1]);
        selected[linear] = true;
        ++stores;
      }
      std::size_t selectedCount = 0;
      for (std::size_t e = 0; e < selected.size(); ++e) {
        const double before = initialC.floatData[e];
        const double expected = sourceC.floatData[e];
        const double actual = producedC.floatData[e];
        if (!selected[e]) {
          if (std::memcmp(&before, &actual, sizeof(double)) != 0) {
            std::cerr << "3b7 " << label << ": unselected element " << e
                      << " changed from " << before << " to " << actual
                      << "\n";
            return false;
          }
          continue;
        }
        ++selectedCount;
        if (!contractEqual(expected, actual, data)) {
          std::cerr << "3b7 " << label << ": selected element " << e
                    << " is " << actual << ", the source gives " << expected
                    << "\n";
          return false;
        }
      }
      // A and B untouched.
      for (unsigned i = 1; i < 3; ++i) {
        const auto &before = sourceMemory.buffers[sourceArgs[i]].floatData;
        const auto &after = producedMemory.buffers[producedArgs[i]].floatData;
        if (before.size() != after.size() ||
            std::memcmp(before.data(), after.data(),
                        before.size() * sizeof(double)) != 0) {
          std::cerr << "3b7 " << label << ": a factor changed\n";
          return false;
        }
      }
      if (selectedCount == 0 || selectedCount == selected.size()) {
        std::cerr << "3b7 " << label << ": the mask selects " << selectedCount
                  << " of " << selected.size() << " elements\n";
        return false;
      }
      return true;
    };

    // The domains (C, A, B declared as in the 3b-4 fixtures: C is the
    // first argument).
    GemmText upper = plain;
    upper.name = "mask_upper";
    upper.loops = "affine.for %i = 0 to 4 { affine.for %j = "
                  "affine_map<(d0) -> (d0)>(%i) to 4 { affine.for %k = 0 to 4 {";
    GemmText lowerInclusive = plain;
    lowerInclusive.name = "mask_lower";
    lowerInclusive.loops =
        "affine.for %i = 0 to 4 { affine.for %j = 0 to "
        "affine_map<(d0) -> (d0 + 1)>(%i) { affine.for %k = 0 to 4 {";
    GemmText lowerStrict = plain;
    lowerStrict.name = "mask_lower_strict";
    lowerStrict.loops = "affine.for %i = 0 to 4 { affine.for %j = 0 to "
                        "affine_map<(d0) -> (d0)>(%i) { affine.for %k = 0 to 4 {";
    GemmText offsetsMask{"mask_offsets", "memref<8x8xf32>", "memref<8x8xf32>",
                         "memref<8x8xf32>",
                         "affine.for %i = 2 to 6 { affine.for %j = "
                         "affine_map<(d0) -> (d0)>(%i) to 7 { affine.for %k = "
                         "1 to 5 {",
                         3, "%i + 1, %j + 1", "%i + 1, %k", "%k, %j + 1"};
    GemmText hexagon{"mask_hexagon", "memref<8x8xf32>", "memref<8x8xf32>",
                     "memref<8x8xf32>",
                     "affine.for %i = 0 to 8 { affine.for %j = max "
                     "affine_map<(d0) -> (0, 3 - d0)>(%i) to min "
                     "affine_map<(d0) -> (8, 12 - d0)>(%i) { affine.for %k = "
                     "0 to 8 {",
                     3, "%i, %j", "%i, %k", "%k, %j"};
    GemmText kFromI = plain;
    kFromI.name = "mask_k_from_i";
    kFromI.loops = "affine.for %i = 0 to 4 { affine.for %j = 0 to 4 {"
                   " affine.for %k = 0 to affine_map<(d0) -> (d0 + 1)>(%i) {";
    GemmText kFromJ = plain;
    kFromJ.name = "mask_k_from_j";
    kFromJ.loops = "affine.for %i = 0 to 4 { affine.for %j = 0 to 4 {"
                   " affine.for %k = affine_map<(d0) -> (d0)>(%j) to 4 {";
    GemmText kEqualsI = plain;
    kEqualsI.name = "mask_k_equals_i";
    kEqualsI.loops = "affine.for %i = 0 to 4 { affine.for %j = 0 to 4 {"
                     " affine.for %k = affine_map<(d0) -> (d0)>(%i) to "
                     "affine_map<(d0) -> (d0 + 1)>(%i) {";
    GemmText emptyDomain = plain;
    emptyDomain.name = "mask_empty";
    emptyDomain.loops = "affine.for %i = 0 to 4 { affine.for %j = "
                        "affine_map<(d0) -> (d0 + 4)>(%i) to 4 { affine.for "
                        "%k = 0 to 4 {";

    // Executed: selected outputs under the contract, the rest untouched.
    struct Executed {
      const GemmText *gemm;
      MaskData data;
      const char *claim;
    };
    const std::vector<Executed> executed = {
        {&upper, MaskData::Integers,
         "3b7 masked offload: upper-triangular output, exact data, selected"
         " outputs bitwise, unselected untouched"},
        {&upper, MaskData::Special,
         "3b7 masked offload: upper-triangular output with NaN, -0 and zero"
         " products under the contract"},
        {&upper, MaskData::Rounding,
         "3b7 masked offload: upper-triangular output, inexact data within"
         " the contract's tolerance"},
        {&lowerInclusive, MaskData::Integers,
         "3b7 masked offload: lower-triangular output (j <= i)"},
        {&lowerStrict, MaskData::Integers,
         "3b7 masked offload: strictly lower-triangular output (j < i)"},
        {&lowerStrict, MaskData::Special,
         "3b7 masked offload: strictly lower-triangular output with NaN and"
         " -0 in C"},
        {&offsetsMask, MaskData::Integers,
         "3b7 masked offload: nonzero bounds and constant offsets"},
        {&offsetsMask, MaskData::Rounding,
         "3b7 masked offload: nonzero bounds and offsets, inexact data"},
        {&hexagon, MaskData::Integers,
         "3b7 masked offload: a hexagonal output domain (two cut corners)"},
        {&hexagon, MaskData::Special,
         "3b7 masked offload: a hexagonal output domain with NaN and -0"
         " in C"},
    };
    const std::string maskedOptions =
        "general-affine=true certified-offload-first=true "
        "allow-fp-reassociation=true report=true";
    for (const Executed &e : executed)
      if (verify(runMaskedExecution(*e.gemm, e.data, maskedOptions), e.claim))
        return 1;

    // A skewed triangle (C[i+j][j], j < 4 - i) with a dynamic leading
    // dimension of A: the candidate box's I range is the interval image
    // [0, 6] of the loop bounds; the dense box is tightened to the rows the
    // nest reads, [0, 3], and its last row is proven an iteration, so the
    // dense GEMM reads no row past the array (review finding, section 28.2).
    {
      GemmText skewDynamic{"mask_skew_dynamic", "memref<8x4xf32>",
                           "memref<?x4xf32>", "memref<4x4xf32>",
                           "affine.for %i = 0 to 4 { affine.for %j = 0 to "
                           "affine_map<(d0) -> (4 - d0)>(%i) { affine.for %k "
                           "= 0 to 4 {",
                           3, "%i + %j, %j", "%i + %j, %k", "%k, %j"};
      if (verify(runOffload(skewDynamic, maskedOptions,
                            OffloadExpectation::Masked, {4, 4, 4, {}},
                            {"with an exact output mask"}, nullptr),
                 "3b7 masked offload: a skewed triangle on a dynamic leading"
                 " dimension reads only the rows the nest reads"))
        return 1;
    }

    // Permission by the flag alone (structural: the interpreter does not
    // execute flagged statements, and the flag changes no arithmetic here).
    {
      GemmText flagged = upper;
      flagged.name = "mask_reassoc";
      flagged.flags = " fastmath<reassoc>";
      if (verify(runOffload(flagged, strictCertifiedOptions,
                            OffloadExpectation::Masked, {4, 4, 4, {}},
                            {"with an exact output mask"}, nullptr),
                 "3b7 masked offload: the reassoc flag permits the masked"
                 " offload"))
        return 1;
    }

    // With the fleet: partial tiles and K folds of the dense box, the mask
    // around them; the tiles partition the 6 x 8 x 40 box.
    {
      GemmText deep{"mask_deep_partial", "memref<6x8xf32>", "memref<6x40xf32>",
                    "memref<40x8xf32>",
                    "affine.for %i = 0 to 6 { affine.for %j = "
                    "affine_map<(d0) -> (d0)>(%i) to 8 { affine.for %k = 0 "
                    "to 40 {",
                    3, "%i, %j", "%i, %k", "%k, %j"};
      if (verify(runOffload(withFleet(deep, "mask_deep_partial"),
                            maskedOptions, OffloadExpectation::Masked,
                            {6, 8, 40, {}}, {"with an exact output mask"},
                            nullptr),
                 "3b7 masked offload: partial tile and K folds over the"
                 " fleet, the tiles partition the box"))
        return 1;
    }

    // Refused: partial reduction fibers (a K bound depending on i or j, K
    // pinned to i), the empty domain; strict semantics; may-alias
    // arguments. The next decision takes over each time.
    struct MaskRefusal {
      const GemmText *gemm;
      std::string options;
      OffloadExpectation next;
      std::string reason;
      const char *claim;
    };
    GemmText upperAlias = upper;
    upperAlias.name = "mask_may_alias";
    upperAlias.argumentAttrs = "";
    const std::vector<MaskRefusal> maskRefusals = {
        {&kFromI, maskedOptions, OffloadExpectation::GuardedMac,
         "UNKNOWN: the reduction fiber of an output coordinate is a proper "
         "part of the K range",
         "3b7 masked offload: a K bound depending on i is refused (partial"
         " fibers); the guarded MAC nest follows"},
        {&kFromJ, maskedOptions, OffloadExpectation::GuardedMac,
         "UNKNOWN: the reduction fiber of an output coordinate is a proper "
         "part of the K range",
         "3b7 masked offload: a K bound depending on j is refused"},
        // K pinned to i (k = i only): the certified lowering refuses the
        // mask (the fiber of every output is the single value i); the fast
        // path's established normalization (x = U x', section 19.12, since
        // 2026-10-18 and unchanged by Phases 3b-6/3b-7: the IR is the same
        // in both decision orders) lowers it as a batch loop over i of
        // 1 x 4 x 1 tiles, C[i][0..4) += A[i][i] x B[i][0..4): one product
        // per element, the elements the nest writes. Executed bitwise
        // against the CPU reference in test/SystolicCC/masked_offload.c
        // (k_equals_i, nonzero C). The remark below pins the lowering that
        // took the nest.
        {&kEqualsI, maskedOptions, OffloadExpectation::FastPath,
         "UNKNOWN: the reduction fiber of an output coordinate is a proper "
         "part of the K range",
         "3b7 masked offload: K pinned to i is refused; the fast path's"
         " batched 1x4x1 lowering takes it"},
        {&emptyDomain, maskedOptions, OffloadExpectation::GuardedMac,
         "UNKNOWN: the iteration domain is empty",
         "3b7 masked offload: an empty domain is not offloaded"},
        {&upper, strictCertifiedOptions, OffloadExpectation::GuardedMac,
         "UNKNOWN: strict floating-point semantics",
         "3b7 masked offload: strict semantics are never offloaded; the"
         " guarded MAC nest follows"},
        {&upperAlias, maskedOptions, OffloadExpectation::Unchanged,
         "UNKNOWN: A may alias C",
         "3b7 masked offload: may-alias arguments stay unchanged"},
    };
    for (const MaskRefusal &r : maskRefusals) {
      std::vector<std::string> expected = {r.reason};
      if (r.reason.rfind("UNKNOWN: A may alias", 0) != 0)
        expected[0] = std::string(certifiedRefusal) + r.reason;
      if (r.gemm == &kEqualsI) {
        expected.push_back("affine GEMM 1x4x1 lowered as 1 exact region(s)");
        expected.push_back("batched: 4 batch iteration(s) of 1 batch loop(s)");
      }
      OffloadShape shape{4, 4, 4, {}};
      shape.checkPartition = false;
      if (verify(runOffload(*r.gemm, r.options, r.next, shape, expected,
                            nullptr),
                 r.claim))
        return 1;
    }

    // ------------------------------------------------------------------
    // Phase 5.4.3d-3b-8 M1 (docs section 29), under the pass option
    // common-fiber-offload: a domain whose reduction fibers vary with the
    // output coordinate runs each output block's common fiber on the
    // device and the remaining contributions on the host.
    // ------------------------------------------------------------------

    // (1) The exact common-fiber arithmetic on the reference domains, with
    // 8 x 8 output blocks (the logical partition of the design report).
    {
      using namespace mlir::presburger;
      auto cube = [](int64_t n) {
        IntegerPolyhedron d(PresburgerSpace::getSetSpace(3));
        for (unsigned v = 0; v < 3; ++v) {
          d.addBound(BoundType::LB, v, DynamicAPInt(0));
          d.addBound(BoundType::UB, v, DynamicAPInt(n - 1));
        }
        return d;
      };
      auto row = [](int64_t a, int64_t b, int64_t c, int64_t d) {
        return SmallVector<DynamicAPInt, 4>{DynamicAPInt(a), DynamicAPInt(b),
                                           DynamicAPInt(c), DynamicAPInt(d)};
      };
      std::vector<::systolic::logical::OutputBlock> grid;
      for (int64_t bi = 0; bi < 8; ++bi)
        for (int64_t bj = 0; bj < 8; ++bj)
          grid.push_back({8 * bi, 8 * bi + 7, 8 * bj, 8 * bj + 7});
      const ::systolic::logical::FiberRoles roles{0, 1, 2};

      IntegerPolyhedron kLeI = cube(64);
      kLeI.addInequality(row(1, 0, -1, 0)); // i - k >= 0
      auto counts = ::systolic::logical::countReductionFibers(
          kLeI, roles, grid, 0, 63, 0, 63, 0, 63);
      if (verify(counts && counts->total == 133120 &&
                     counts->device == 118784 && counts->remainder == 14336,
                 "3b8 common fibers: k <= i on 64^3 with 8x8 blocks counts"
                 " 133120 = 118784 device + 14336 host MACs"))
        return 1;
      auto fiber = ::systolic::logical::commonReductionFiber(
          kLeI, roles, {8, 15, 0, 7}, 0, 63);
      if (verify(fiber && !fiber->empty && fiber->lower == 0 &&
                     fiber->upper == 8,
                 "3b8 common fibers: rows 8..15 of k <= i share K [0, 8]"))
        return 1;

      IntegerPolyhedron kLeIJ = cube(64);
      kLeIJ.addInequality(row(1, 1, -1, 0)); // i + j - k >= 0
      counts = ::systolic::logical::countReductionFibers(kLeIJ, roles, grid,
                                                         0, 63, 0, 63, 0, 63);
      if (verify(counts && counts->total == 218464 &&
                     counts->device == 203008 && counts->remainder == 15456,
                 "3b8 common fibers: k <= i + j on 64^3 counts 218464 ="
                 " 203008 device + 15456 host MACs"))
        return 1;

      IntegerPolyhedron between = cube(64);
      between.addInequality(row(1, 0, -1, 0));  // i - k >= 0
      between.addInequality(row(0, -1, 1, 0));  // k - j >= 0
      counts = ::systolic::logical::countReductionFibers(
          between, roles, grid, 0, 63, 0, 63, 0, 63);
      unsigned emptyBlocks = 0;
      for (const auto &block : grid) {
        auto f = ::systolic::logical::commonReductionFiber(between, roles,
                                                           block, 0, 63);
        if (!f)
          return verify(false, "3b8 common fibers: representable") ? 1 : 1;
        emptyBlocks += f->empty;
      }
      if (verify(counts && counts->total == 45760 &&
                     counts->device == 32256 && counts->remainder == 13504 &&
                     emptyBlocks == 36,
                 "3b8 common fibers: j <= k <= i on 64^3 counts 45760 ="
                 " 32256 device + 13504 host MACs, 36 blocks without a"
                 " common fiber"))
        return 1;

      // Coefficients beyond one (floor), a negative coefficient with an
      // offset (ceil), an equality, and a block that is one row.
      IntegerPolyhedron halfK = cube(64);
      halfK.addInequality(row(1, 0, -2, 0)); // i - 2k >= 0
      fiber = ::systolic::logical::commonReductionFiber(halfK, roles,
                                                        {8, 15, 0, 7}, 0, 63);
      if (verify(fiber && !fiber->empty && fiber->lower == 0 &&
                     fiber->upper == 4,
                 "3b8 common fibers: 2k <= i over rows 8..15 is K [0, 4]"))
        return 1;
      IntegerPolyhedron kGeJ2 = cube(64);
      kGeJ2.addInequality(row(0, -1, 1, -2)); // k - j - 2 >= 0
      fiber = ::systolic::logical::commonReductionFiber(kGeJ2, roles,
                                                        {0, 7, 8, 15}, 0, 63);
      if (verify(fiber && !fiber->empty && fiber->lower == 17 &&
                     fiber->upper == 63,
                 "3b8 common fibers: k >= j + 2 over columns 8..15 is K"
                 " [17, 63]"))
        return 1;
      IntegerPolyhedron kEqI = cube(64);
      kEqI.addEquality(row(1, 0, -1, 0)); // k = i
      auto multiRow = ::systolic::logical::commonReductionFiber(
          kEqI, roles, {8, 15, 0, 7}, 0, 63);
      auto oneRow = ::systolic::logical::commonReductionFiber(
          kEqI, roles, {5, 5, 0, 7}, 0, 63);
      if (verify(multiRow && multiRow->empty && oneRow && !oneRow->empty &&
                     oneRow->lower == 5 && oneRow->upper == 5,
                 "3b8 common fibers: k = i is empty over several rows and"
                 " {5} over row 5"))
        return 1;
      if (verify(!::systolic::logical::commonReductionFiber(
                     kLeI, roles, {8, 15, 0, 7}, 5, 4),
                 "3b8 common fibers: an empty K range is refused"))
        return 1;
    }

    // (2) Executed scenarios, no fleet: the whole output box is one block,
    // its common fiber runs as one tile, the rest as guarded host nests;
    // selected outputs under the relaxed contract, the others untouched.
    const std::string hybridOptions =
        "general-affine=true certified-offload-first=true "
        "allow-fp-reassociation=true common-fiber-offload=true report=true";
    auto runHybridExecution = [&](const GemmText &gemm, MaskData data,
                                  const std::string &options) -> bool {
      const std::string label =
          gemm.name + "/" +
          (data == MaskData::Integers ? "integers"
           : data == MaskData::Special ? "special"
                                       : "rounding") +
          "{" + options + "}";
      ParsedGemm original, produced;
      if (!parseGemm(original, gemmModule(gemm), gemm.name) ||
          !parseGemm(produced, gemmModule(gemm), gemm.name)) {
        std::cerr << "3b8 " << label << ": parse failed\n";
        return false;
      }
      std::string diagnostics;
      if (!runPass(produced, options, diagnostics)) {
        std::cerr << "3b8 " << label << ": the pass failed\n" << diagnostics;
        return false;
      }
      if (diagnostics.find("(section 29)") == std::string::npos) {
        std::cerr << "3b8 " << label << ": not lowered as a hybrid\n"
                  << diagnostics;
        return false;
      }
      exec::MemoryState sourceMemory, producedMemory;
      std::array<Value, 3> sourceArgs, producedArgs;
      for (unsigned i = 0; i < 3; ++i) {
        sourceArgs[i] = original.function.getArgument(i);
        producedArgs[i] = produced.function.getArgument(i);
      }
      const unsigned roleOf[3] = {2, 0, 1};
      for (unsigned i = 0; i < 3; ++i) {
        auto type = cast<MemRefType>(sourceArgs[i].getType());
        sourceMemory.buffers[sourceArgs[i]] = maskBuffer(type, roleOf[i], data);
        producedMemory.buffers[producedArgs[i]] =
            maskBuffer(type, roleOf[i], data);
      }
      const exec::MemoryBuffer initialC = sourceMemory.buffers[sourceArgs[0]];
      auto sourceRun = exec::interpretGuardedLoopNest(
          original.outermost, scf::IfOp(), ValueRange{}, ValueRange{},
          ValueRange{}, ValueRange{}, ValueRange{}, &sourceMemory);
      if (!sourceRun) {
        std::cerr << "3b8 " << label << ": the source nest did not execute\n";
        return false;
      }
      if (!executeOffloaded(produced.function, producedMemory))
        return false;
      const exec::MemoryBuffer &sourceC = sourceMemory.buffers[sourceArgs[0]];
      const exec::MemoryBuffer &producedC =
          producedMemory.buffers[producedArgs[0]];
      std::vector<bool> selected(initialC.floatData.size(), false);
      for (const exec::MemoryEvent &event : sourceMemory.trace) {
        if (!event.isStore || event.memref != sourceArgs[0])
          continue;
        selected[static_cast<std::size_t>(event.indices[0]) *
                     static_cast<std::size_t>(initialC.shape[1]) +
                 static_cast<std::size_t>(event.indices[1])] = true;
      }
      std::size_t selectedCount = 0;
      for (std::size_t e = 0; e < selected.size(); ++e) {
        const double before = initialC.floatData[e];
        const double expected = sourceC.floatData[e];
        const double actual = producedC.floatData[e];
        if (!selected[e]) {
          if (std::memcmp(&before, &actual, sizeof(double)) != 0) {
            std::cerr << "3b8 " << label << ": unselected element " << e
                      << " changed\n";
            return false;
          }
          continue;
        }
        ++selectedCount;
        if (!contractEqual(expected, actual, data)) {
          std::cerr << "3b8 " << label << ": selected element " << e
                    << " is " << actual << ", the source gives " << expected
                    << "\n";
          return false;
        }
      }
      for (unsigned i = 1; i < 3; ++i) {
        const auto &before = sourceMemory.buffers[sourceArgs[i]].floatData;
        const auto &after = producedMemory.buffers[producedArgs[i]].floatData;
        if (before.size() != after.size() ||
            std::memcmp(before.data(), after.data(),
                        before.size() * sizeof(double)) != 0) {
          std::cerr << "3b8 " << label << ": a factor changed\n";
          return false;
        }
      }
      if (selectedCount == 0) {
        std::cerr << "3b8 " << label << ": nothing selected\n";
        return false;
      }
      return true;
    };

    GemmText caseA = plain; // 0 <= k <= i
    caseA.name = "fiber_k_le_i";
    caseA.loops = "affine.for %i = 0 to 4 { affine.for %j = 0 to 4 {"
                  " affine.for %k = 0 to affine_map<(d0) -> (d0 + 1)>(%i) {";
    GemmText caseB = plain; // 0 <= k <= i + j, k < 4
    caseB.name = "fiber_k_le_i_plus_j";
    caseB.loops = "affine.for %i = 0 to 4 { affine.for %j = 0 to 4 {"
                  " affine.for %k = 0 to min affine_map<(d0, d1) -> (4, d0 + "
                  "d1 + 1)>(%i, %j) {";
    GemmText caseC = plain; // k >= j
    caseC.name = "fiber_k_ge_j";
    caseC.loops = "affine.for %i = 0 to 4 { affine.for %j = 0 to 4 {"
                  " affine.for %k = affine_map<(d0) -> (d0)>(%j) to 4 {";
    GemmText caseD = plain; // j <= k <= i + 3 (nonempty common fiber {3})
    caseD.name = "fiber_j_le_k_le_i3";
    caseD.aType = "memref<4x8xf32>";
    caseD.bType = "memref<8x4xf32>";
    caseD.loops = "affine.for %i = 0 to 4 { affine.for %j = 0 to 4 {"
                  " affine.for %k = affine_map<(d0) -> (d0)>(%j) to "
                  "affine_map<(d0) -> (d0 + 4)>(%i) {";
    GemmText shifted{"fiber_shifted", "memref<8x8xf32>", "memref<8x8xf32>",
                     "memref<8x8xf32>",
                     "affine.for %i = 1 to 4 { affine.for %j = 0 to 3 {"
                     " affine.for %k = 1 to affine_map<(d0) -> (d0 + 1)>(%i) {",
                     3, "%i + 1, %j + 2", "%i + 1, %k", "%k, %j + 2"};

    struct HybridExecuted {
      const GemmText *gemm;
      MaskData data;
      const char *claim;
    };
    const std::vector<HybridExecuted> hybridExecuted = {
        {&caseA, MaskData::Integers,
         "3b8 hybrid: k <= i, exact data: selected outputs bitwise, others"
         " untouched"},
        {&caseA, MaskData::Special,
         "3b8 hybrid: k <= i with NaN, -0 and zero products under the"
         " contract"},
        {&caseA, MaskData::Rounding,
         "3b8 hybrid: k <= i, inexact data within the contract's tolerance"},
        {&caseB, MaskData::Integers, "3b8 hybrid: k <= i + j, exact data"},
        {&caseB, MaskData::Rounding, "3b8 hybrid: k <= i + j, inexact data"},
        {&caseC, MaskData::Integers, "3b8 hybrid: k >= j, exact data"},
        {&caseC, MaskData::Special,
         "3b8 hybrid: k >= j with NaN and -0 in C"},
        {&caseD, MaskData::Integers,
         "3b8 hybrid: j <= k <= i + 3 (both bounds vary), exact data"},
        {&caseD, MaskData::Special,
         "3b8 hybrid: j <= k <= i + 3 with NaN and -0 in C"},
        {&shifted, MaskData::Integers,
         "3b8 hybrid: nonzero lower bounds and offsets, exact data"},
        {&shifted, MaskData::Rounding,
         "3b8 hybrid: nonzero lower bounds and offsets, inexact data"},
    };
    for (const HybridExecuted &e : hybridExecuted)
      if (verify(runHybridExecution(*e.gemm, e.data, hybridOptions), e.claim))
        return 1;

    // (3) With the fleet: every tile's block runs exactly its common fiber
    // (computed independently here), the folds contiguous, the device MACs
    // of the tiles equal the remark's count.
    {
      using namespace mlir::presburger;
      auto fiberOf = [](auto domainBuilder) {
        return [domainBuilder](const ::systolic::logical::OutputBlock &b) {
          IntegerPolyhedron d = domainBuilder();
          return ::systolic::logical::commonReductionFiber(
              d, ::systolic::logical::FiberRoles{0, 1, 2}, b, 0, 15);
        };
      };
      auto cube16 = []() {
        IntegerPolyhedron d(PresburgerSpace::getSetSpace(3));
        for (unsigned v = 0; v < 3; ++v) {
          d.addBound(BoundType::LB, v, DynamicAPInt(0));
          d.addBound(BoundType::UB, v, DynamicAPInt(15));
        }
        return d;
      };
      auto row = [](int64_t a, int64_t b, int64_t c, int64_t d) {
        return SmallVector<DynamicAPInt, 4>{DynamicAPInt(a), DynamicAPInt(b),
                                           DynamicAPInt(c), DynamicAPInt(d)};
      };
      GemmText big = square(16, 16, 16, "fiber_fleet_k_le_i");
      big.loops = "affine.for %i = 0 to 16 { affine.for %j = 0 to 16 {"
                  " affine.for %k = 0 to affine_map<(d0) -> (d0 + 1)>(%i) {";
      OffloadShape shape{16, 16, 16, {}};
      shape.expectedFiber = fiberOf([&]() {
        IntegerPolyhedron d = cube16();
        d.addInequality(row(1, 0, -1, 0));
        return d;
      });
      if (verify(runOffload(withFleet(big, "fiber_fleet_k_le_i"),
                            hybridOptions, OffloadExpectation::Hybrid, shape,
                            {"(section 29)"}, nullptr),
                 "3b8 hybrid with the fleet: k <= i, every block runs its"
                 " common fiber, the device MACs match the remark"))
        return 1;
      GemmText both = square(16, 16, 16, "fiber_fleet_between");
      both.loops = "affine.for %i = 0 to 16 { affine.for %j = 0 to 16 {"
                   " affine.for %k = affine_map<(d0) -> (d0)>(%j) to "
                   "affine_map<(d0) -> (d0 + 1)>(%i) {";
      shape.expectedFiber = fiberOf([&]() {
        IntegerPolyhedron d = cube16();
        d.addInequality(row(1, 0, -1, 0));
        d.addInequality(row(0, -1, 1, 0));
        return d;
      });
      if (verify(runOffload(withFleet(both, "fiber_fleet_between"),
                            hybridOptions, OffloadExpectation::Hybrid, shape,
                            {"(section 29)"}, nullptr),
                 "3b8 hybrid with the fleet: j <= k <= i, blocks without a"
                 " common fiber go to the host"))
        return 1;
    }

    // (4) Permission by the flag alone: the remainder mirrors the flags.
    {
      GemmText flagged = caseA;
      flagged.name = "fiber_reassoc";
      flagged.flags = " fastmath<reassoc>";
      ParsedGemm produced;
      std::string diagnostics;
      bool mirrored = false;
      if (parseGemm(produced, gemmModule(flagged), flagged.name) &&
          runPass(produced,
                  "general-affine=true certified-offload-first=true "
                  "common-fiber-offload=true report=true",
                  diagnostics) &&
          diagnostics.find("(section 29)") != std::string::npos) {
        mirrored = true;
        produced.function.walk([&](arith::AddFOp add) {
          if (add->getParentOfType<scf::IfOp>() &&
              !arith::bitEnumContainsAll(add.getFastmath(),
                                          arith::FastMathFlags::reassoc))
            mirrored = false;
        });
      }
      if (verify(mirrored,
                 "3b8 hybrid: the reassoc flag permits the offload and the"
                 " remainder's additions carry it"))
        return 1;
    }

    // (5) Refusals and fallbacks. Without the option the established
    // refusal of partial fibers stands (the Phase 3b-7 scenarios above
    // keep checking it); with it: strict semantics are never offloaded,
    // may-alias arguments stay, a box whose single block has no common
    // fiber (j <= k <= i, no fleet) is UNKNOWN and the guarded MAC nest
    // follows, and a stepped reduction is not recognized.
    {
      GemmText between4 = plain;
      between4.name = "fiber_between_no_fleet";
      between4.loops = "affine.for %i = 0 to 4 { affine.for %j = 0 to 4 {"
                       " affine.for %k = affine_map<(d0) -> (d0)>(%j) to "
                       "affine_map<(d0) -> (d0 + 1)>(%i) {";
      GemmText aliasA = caseA;
      aliasA.name = "fiber_may_alias";
      aliasA.argumentAttrs = "";
      GemmText steppedA = caseA;
      steppedA.name = "fiber_stepped";
      steppedA.aType = "memref<4x8xf32>";
      steppedA.bType = "memref<8x4xf32>";
      steppedA.loops = "affine.for %i = 0 to 4 { affine.for %j = 0 to 4 {"
                       " affine.for %k = 0 to affine_map<(d0) -> (2 * d0 + "
                       "2)>(%i) step 2 {";
      OffloadShape four{4, 4, 4, {}};
      four.checkPartition = false;
      if (verify(runOffload(caseA,
                            "general-affine=true certified-offload-first=true "
                            "common-fiber-offload=true report=true",
                            OffloadExpectation::GuardedMac, four,
                            {std::string(certifiedRefusal) +
                             "UNKNOWN: strict floating-point semantics"},
                            nullptr),
                 "3b8 hybrid: strict semantics are never offloaded; the"
                 " guarded MAC nest follows"))
        return 1;
      if (verify(runOffload(aliasA, hybridOptions,
                            OffloadExpectation::Unchanged, four,
                            {"UNKNOWN: A may alias C"}, nullptr),
                 "3b8 hybrid: may-alias arguments stay unchanged"))
        return 1;
      if (verify(runOffload(between4, hybridOptions,
                            OffloadExpectation::GuardedMac, four,
                            {std::string(certifiedRefusal) +
                             "UNKNOWN: no output block has a common reduction "
                             "fiber"},
                            nullptr),
                 "3b8 hybrid: a box whose only block has no common fiber is"
                 " UNKNOWN; the guarded MAC nest follows"))
        return 1;
      if (verify(runOffload(steppedA, hybridOptions,
                            OffloadExpectation::Unchanged, four,
                            {"UNKNOWN: not a recognized GEMM"}, nullptr),
                 "3b8 hybrid: a stepped reduction is not recognized and"
                 " stays unchanged"))
        return 1;
      // The option off: the established refusal, the guarded MAC follows.
      if (verify(runOffload(caseA, certifiedOptions,
                            OffloadExpectation::GuardedMac, four,
                            {std::string(certifiedRefusal) +
                             "UNKNOWN: the reduction fiber of an output "
                             "coordinate is a proper part of the K range"},
                            nullptr),
                 "3b8 hybrid: without the option the partial-fiber refusal"
                 " of Phase 3b-7 stands"))
        return 1;
    }

    // ------------------------------------------------------------------
    // Phase 5.4.3d-3b-8 M2b-1 (docs section 29.8): the host remainder in
    // pieces over exact bounding boxes. (6a) The piece arithmetic against
    // a brute-force oracle (membership of every box point); (6b) the
    // emitted nests, interpreted: the visited points are exactly the
    // boxes, the accepted points are exactly the remainder, each once,
    // none of the device's, none outside D', and every bound is attained
    // (the boxes are tight); (6c) the strict fallback is untouched; (6d)
    // unsupported inputs are refused.
    // ------------------------------------------------------------------
    {
      using namespace mlir::presburger;
      namespace fib = ::systolic::logical;
      using Point3 = std::array<int64_t, 3>;

      auto box3 = [](int64_t lo0, int64_t hi0, int64_t lo1, int64_t hi1,
                     int64_t lo2, int64_t hi2) {
        IntegerPolyhedron d(PresburgerSpace::getSetSpace(3));
        const int64_t lo[3] = {lo0, lo1, lo2}, hi[3] = {hi0, hi1, hi2};
        for (unsigned v = 0; v < 3; ++v) {
          d.addBound(BoundType::LB, v, DynamicAPInt(lo[v]));
          d.addBound(BoundType::UB, v, DynamicAPInt(hi[v]));
        }
        return d;
      };
      auto row = [](int64_t a, int64_t b, int64_t c, int64_t d) {
        return SmallVector<DynamicAPInt, 4>{DynamicAPInt(a), DynamicAPInt(b),
                                           DynamicAPInt(c), DynamicAPInt(d)};
      };
      const fib::FiberRoles ijk{0, 1, 2};

      // The oracle: the integer points of D' ∩ (block × [kFrom, kTo]) by
      // membership, with their bounding box.
      struct OraclePiece {
        std::set<Point3> points;
        Point3 lo{}, hi{};
      };
      auto enumerate = [&](const IntegerPolyhedron &d, fib::FiberRoles roles,
                           const fib::OutputBlock &block, int64_t kFrom,
                           int64_t kTo) {
        OraclePiece piece;
        bool first = true;
        for (int64_t i = block.rowLower; i <= block.rowUpper; ++i)
          for (int64_t j = block.columnLower; j <= block.columnUpper; ++j)
            for (int64_t k = kFrom; k <= kTo; ++k) {
              Point3 p{};
              p[roles.i] = i;
              p[roles.j] = j;
              p[roles.k] = k;
              if (!d.containsPoint(ArrayRef<int64_t>(p.data(), 3)))
                continue;
              piece.points.insert(p);
              for (unsigned v = 0; v < 3; ++v) {
                if (first || p[v] < piece.lo[v])
                  piece.lo[v] = p[v];
                if (first || p[v] > piece.hi[v])
                  piece.hi[v] = p[v];
              }
              first = false;
            }
        return piece;
      };
      // One block against the oracle: as many pieces as non-empty sides,
      // the oracle's boxes, and on every point of a box the same
      // membership (the guard set is exactly the oracle's piece). Returns
      // the sum of the box volumes (the guard evaluations), -1 on failure;
      // adds the remainder points to `points`.
      auto checkBlock = [&](const IntegerPolyhedron &d, fib::FiberRoles roles,
                            const fib::OutputBlock &block, int64_t kLo,
                            int64_t kHi, const std::string &label,
                            int64_t &points) -> int64_t {
        auto fiber = fib::commonReductionFiber(d, roles, block, kLo, kHi);
        if (!fiber) {
          std::cerr << "m2b1 " << label << ": the common fiber is refused\n";
          return -1;
        }
        auto pieces = fib::remainderPieces(d, roles, block, *fiber, kLo, kHi);
        if (!pieces) {
          std::cerr << "m2b1 " << label << ": the pieces are refused\n";
          return -1;
        }
        std::vector<OraclePiece> expected;
        auto side = [&](int64_t from, int64_t to) {
          OraclePiece piece = enumerate(d, roles, block, from, to);
          if (!piece.points.empty())
            expected.push_back(std::move(piece));
        };
        if (fiber->empty) {
          side(kLo, kHi);
        } else {
          if (fiber->lower > kLo)
            side(kLo, fiber->lower - 1);
          if (fiber->upper < kHi)
            side(fiber->upper + 1, kHi);
        }
        if (expected.size() != pieces->size()) {
          std::cerr << "m2b1 " << label << ": " << pieces->size()
                    << " piece(s), the oracle has " << expected.size() << "\n";
          return -1;
        }
        int64_t volume = 0;
        for (std::size_t p = 0; p < expected.size(); ++p) {
          const OraclePiece &oracle = expected[p];
          const fib::RemainderPiece &piece = (*pieces)[p];
          if (piece.box.size() != 3 || piece.set.getNumLocalVars() != 0) {
            std::cerr << "m2b1 " << label << ": piece " << p
                      << " is malformed\n";
            return -1;
          }
          int64_t pieceVolume = 1;
          for (unsigned v = 0; v < 3; ++v) {
            if (piece.box[v].lower != oracle.lo[v] ||
                piece.box[v].upper != oracle.hi[v]) {
              std::cerr << "m2b1 " << label << ": piece " << p
                        << " coordinate " << v << " spans ["
                        << piece.box[v].lower << ", " << piece.box[v].upper
                        << "], the oracle's points [" << oracle.lo[v] << ", "
                        << oracle.hi[v] << "]\n";
              return -1;
            }
            pieceVolume *= piece.box[v].upper - piece.box[v].lower + 1;
          }
          volume += pieceVolume;
          for (int64_t z0 = piece.box[0].lower; z0 <= piece.box[0].upper; ++z0)
            for (int64_t z1 = piece.box[1].lower; z1 <= piece.box[1].upper;
                 ++z1)
              for (int64_t z2 = piece.box[2].lower; z2 <= piece.box[2].upper;
                   ++z2) {
                const Point3 q{z0, z1, z2};
                const bool inSet =
                    piece.set.containsPoint(ArrayRef<int64_t>(q.data(), 3));
                if (inSet != (oracle.points.count(q) != 0)) {
                  std::cerr << "m2b1 " << label << ": piece " << p
                            << " and the oracle disagree at (" << z0 << ", "
                            << z1 << ", " << z2 << ")\n";
                  return -1;
                }
              }
          points += static_cast<int64_t>(oracle.points.size());
        }
        return volume;
      };
      // A grid of blocks: every block against the oracle, and the sum of
      // the remainder points against the exact count of section 29.
      auto gridOf = [](int64_t iLo, int64_t iHi, int64_t jLo, int64_t jHi,
                       int64_t size) {
        std::vector<fib::OutputBlock> blocks;
        for (int64_t i = iLo; i <= iHi; i += size)
          for (int64_t j = jLo; j <= jHi; j += size)
            blocks.push_back({i, std::min(i + size - 1, iHi), j,
                              std::min(j + size - 1, jHi)});
        return blocks;
      };
      auto checkGrid = [&](const IntegerPolyhedron &d,
                           const std::vector<fib::OutputBlock> &blocks,
                           int64_t iLo, int64_t iHi, int64_t jLo, int64_t jHi,
                           int64_t kLo, int64_t kHi, const char *label,
                           int64_t &sweep, int64_t &points) -> bool {
        sweep = 0;
        points = 0;
        for (const fib::OutputBlock &block : blocks) {
          const int64_t volume =
              checkBlock(d, ijk, block, kLo, kHi, label, points);
          if (volume < 0)
            return false;
          sweep += volume;
        }
        auto counts = fib::countReductionFibers(d, ijk, blocks, iLo, iHi, jLo,
                                                jHi, kLo, kHi);
        if (!counts || counts->remainder != BigInt(points)) {
          std::cerr << "m2b1 " << label << ": the pieces hold " << points
                    << " points, the remainder count differs\n";
          return false;
        }
        return true;
      };

      // (6a) The reference domains on 64^3 with 8 x 8 blocks.
      const std::vector<fib::OutputBlock> grid64 = gridOf(0, 63, 0, 63, 8);
      int64_t sweep = 0, points = 0;
      {
        IntegerPolyhedron kLeI = box3(0, 63, 0, 63, 0, 63);
        kLeI.addInequality(row(1, 0, -1, 0)); // i - k >= 0
        const bool ok = checkGrid(kLeI, grid64, 0, 63, 0, 63, 0, 63,
                                  "k <= i on 64^3", sweep, points);
        // Block rows [8b, 8b + 7]: the upper piece is i, k in [8b + 1,
        // 8b + 7] with k <= i, j the block's: a 7 x 8 x 7 box of 224
        // points per block; 64 blocks.
        if (verify(ok && points == 14336 && sweep == 64 * 392,
                   "m2b1 pieces: k <= i on 64^3 with 8x8 blocks: one upper"
                   " piece per block over its exact 7x8x7 box, 25088 guard"
                   " evaluations for the 14336 host MACs (the full K sweep"
                   " took 262144)"))
          return 1;
      }
      {
        IntegerPolyhedron kLeIJ = box3(0, 63, 0, 63, 0, 63);
        kLeIJ.addInequality(row(1, 1, -1, 0)); // i + j - k >= 0
        const bool ok = checkGrid(kLeIJ, grid64, 0, 63, 0, 63, 0, 63,
                                  "k <= i + j on 64^3", sweep, points);
        if (verify(ok && points == 15456 && sweep < 64 * 4096,
                   "m2b1 pieces: k <= i + j on 64^3: upper pieces over exact"
                   " boxes, 15456 host MACs"))
          return 1;
      }
      {
        IntegerPolyhedron kGeJ = box3(0, 63, 0, 63, 0, 63);
        kGeJ.addInequality(row(0, -1, 1, 0)); // k - j >= 0
        const bool ok = checkGrid(kGeJ, grid64, 0, 63, 0, 63, 0, 63,
                                  "k >= j on 64^3", sweep, points);
        // Block columns [8c, 8c + 7]: the lower piece is j, k in [8c,
        // 8c + 6] with k >= j, i the block's: an 8 x 7 x 7 box.
        if (verify(ok && points == 14336 && sweep == 64 * 392,
                   "m2b1 pieces: k >= j on 64^3: one lower piece per block"
                   " over its exact 8x7x7 box"))
          return 1;
      }
      {
        IntegerPolyhedron between = box3(0, 63, 0, 63, 0, 63);
        between.addInequality(row(1, 0, -1, 0)); // i - k >= 0
        between.addInequality(row(0, -1, 1, 0)); // k - j >= 0
        const bool ok = checkGrid(between, grid64, 0, 63, 0, 63, 0, 63,
                                  "j <= k <= i on 64^3", sweep, points);
        // Below the diagonal both pieces (28 blocks x 2), on it the whole
        // block (8 blocks, no common fiber), above it nothing.
        std::size_t pieceCount = 0;
        for (const fib::OutputBlock &block : grid64) {
          auto fiber = fib::commonReductionFiber(between, ijk, block, 0, 63);
          auto pieces = fiber ? fib::remainderPieces(between, ijk, block,
                                                     *fiber, 0, 63)
                              : std::nullopt;
          if (!pieces)
            return verify(false, "m2b1 pieces: j <= k <= i representable");
          pieceCount += pieces->size();
        }
        if (verify(ok && points == 13504 && pieceCount == 28 * 2 + 8,
                   "m2b1 pieces: j <= k <= i on 64^3: two pieces below the"
                   " diagonal, the whole block on it, nothing above; 13504"
                   " host MACs"))
          return 1;
      }
      // Shifted and negative bounds: i in [-3, 4], j in [-2, 1], k in
      // [-5, 6], j - 1 <= k <= i; two blocks of four rows.
      {
        IntegerPolyhedron shiftedD = box3(-3, 4, -2, 1, -5, 6);
        shiftedD.addInequality(row(1, 0, -1, 0)); // i - k >= 0
        shiftedD.addInequality(row(0, -1, 1, 1)); // k - j + 1 >= 0
        const std::vector<fib::OutputBlock> blocks{{-3, 0, -2, 1},
                                                   {1, 4, -2, 1}};
        const bool ok = checkGrid(shiftedD, blocks, -3, 4, -2, 1, -5, 6,
                                  "negative bounds", sweep, points);
        auto upper = fib::commonReductionFiber(shiftedD, ijk, blocks[1], -5, 6);
        auto pieces = upper ? fib::remainderPieces(shiftedD, ijk, blocks[1],
                                                   *upper, -5, 6)
                            : std::nullopt;
        // Rows [1, 4]: the common fiber is [0, 1]; below it k in [-3, -1]
        // with j in [-2, 0]; above it k in [2, 4] with i in [2, 4].
        if (verify(ok && pieces && pieces->size() == 2 &&
                       (*pieces)[0].box[0].lower == 1 &&
                       (*pieces)[0].box[0].upper == 4 &&
                       (*pieces)[0].box[1].lower == -2 &&
                       (*pieces)[0].box[1].upper == 0 &&
                       (*pieces)[0].box[2].lower == -3 &&
                       (*pieces)[0].box[2].upper == -1 &&
                       (*pieces)[1].box[0].lower == 2 &&
                       (*pieces)[1].box[0].upper == 4 &&
                       (*pieces)[1].box[1].lower == -2 &&
                       (*pieces)[1].box[1].upper == 1 &&
                       (*pieces)[1].box[2].lower == 2 &&
                       (*pieces)[1].box[2].upper == 4,
                   "m2b1 pieces: shifted and negative bounds, both pieces"
                   " over their exact boxes"))
          return 1;
      }
      // Degenerate intervals: a K range of one value, a block whose fiber
      // is the whole range (no remainder), a piece of one point.
      {
        IntegerPolyhedron kLeI = box3(0, 63, 0, 63, 0, 63);
        kLeI.addInequality(row(1, 0, -1, 0));
        int64_t one = 0;
        const int64_t whole = checkBlock(kLeI, ijk, {3, 8, 0, 7}, 5, 5,
                                         "K = {5}, rows 3..8", one);
        int64_t none = 0;
        const int64_t covered = checkBlock(kLeI, ijk, {5, 8, 0, 7}, 5, 5,
                                           "K = {5}, rows 5..8", none);
        int64_t single = 0;
        const int64_t point = checkBlock(kLeI, ijk, {0, 1, 0, 0}, 0, 63,
                                         "rows 0..1, one column", single);
        if (verify(whole == 32 && one == 32 && covered == 0 && none == 0 &&
                       point == 1 && single == 1,
                   "m2b1 pieces: degenerate intervals (a one-value K range"
                   " without and with a common fiber, a one-point piece)"))
          return 1;
      }
      // Partial output blocks: an 11 x 13 output under 8 x 8 blocks (edge
      // blocks of 3 x 8, 8 x 5 and 3 x 5), k <= i + j within [0, 15].
      {
        IntegerPolyhedron partial = box3(0, 10, 0, 12, 0, 15);
        partial.addInequality(row(1, 1, -1, 0));
        const bool ok = checkGrid(partial, gridOf(0, 10, 0, 12, 8), 0, 10, 0,
                                  12, 0, 15, "partial blocks", sweep, points);
        if (verify(ok && points > 0,
                   "m2b1 pieces: partial output blocks at the box's edge"))
          return 1;
      }
      // Exactness: the bounds are the integer points', not the rational
      // relaxation's. 3k = 2i + 2 on i in [0, 4] has the single integer
      // point (2, 0, 2); its rational hull spans i in [0, 4], k in [2/3,
      // 10/3].
      {
        IntegerPolyhedron sparse = box3(0, 4, 0, 0, 0, 10);
        sparse.addEquality(row(-2, 0, 3, -2)); // 3k - 2i - 2 = 0
        auto box = fib::exactBoundingBox(sparse, {{0, 4}, {0, 0}, {0, 10}});
        if (verify(box && (*box)[0].lower == 2 && (*box)[0].upper == 2 &&
                       (*box)[1].lower == 0 && (*box)[1].upper == 0 &&
                       (*box)[2].lower == 2 && (*box)[2].upper == 2,
                   "m2b1 pieces: the exact bounding box is the integer"
                   " points' (2, 0, 2), tighter than the rational hull"))
          return 1;
      }
      // (6d) Unsupported inputs are refused, not approximated.
      {
        IntegerPolyhedron kLeI = box3(0, 63, 0, 63, 0, 63);
        kLeI.addInequality(row(1, 0, -1, 0));
        const fib::OutputBlock block{8, 15, 0, 7};
        fib::ReductionInterval fiber;
        fiber.empty = false;
        fiber.lower = 0;
        fiber.upper = 8;
        IntegerPolyhedron withLocal = kLeI;
        withLocal.appendVar(VarKind::Local);
        withLocal.addEquality(
            {DynamicAPInt(0), DynamicAPInt(0), DynamicAPInt(1), DynamicAPInt(-2),
             DynamicAPInt(0)}); // k = 2q
        IntegerPolyhedron planar(PresburgerSpace::getSetSpace(2));
        fib::ReductionInterval outside;
        outside.empty = false;
        outside.lower = 70;
        outside.upper = 80;
        if (verify(!fib::remainderPieces(withLocal, ijk, block, fiber, 0, 63) &&
                       !fib::remainderPieces(planar, ijk, block, fiber, 0,
                                             63) &&
                       !fib::remainderPieces(kLeI, fib::FiberRoles{0, 0, 2},
                                             block, fiber, 0, 63) &&
                       !fib::remainderPieces(kLeI, ijk, {15, 8, 0, 7}, fiber,
                                             0, 63) &&
                       !fib::remainderPieces(kLeI, ijk, block, outside, 0,
                                             63) &&
                       !fib::remainderPieces(kLeI, ijk, block, fiber, 5, 4) &&
                       !fib::exactBoundingBox(kLeI, {{0, 63}, {0, 63}}),
                   "m2b1 pieces: a local, a planar domain, equal roles, an"
                   " inverted block, a fiber outside K, an empty K range"
                   " and a dimension mismatch are refused"))
          return 1;
      }

      // (6b) The emitted nests of the executed scenarios, interpreted.
      struct RemainderScenario {
        const GemmText *gemm;
        Point3 lo, hi; // the logical box (the source coordinates)
        std::function<bool(int64_t, int64_t, int64_t)> in; // D' membership
        const char *claim;
      };
      auto checkNests = [&](const RemainderScenario &s,
                            int64_t *sweepOut) -> bool {
        const std::string label = s.gemm->name;
        ParsedGemm produced;
        std::string diagnostics;
        if (!parseGemm(produced, gemmModule(*s.gemm), s.gemm->name) ||
            !runPass(produced, hybridOptions, diagnostics) ||
            diagnostics.find("(section 29)") == std::string::npos) {
          std::cerr << "m2b1 " << label << ": not lowered as a hybrid\n"
                    << diagnostics;
          return false;
        }
        const std::string key = "and the remainder on the host (";
        const auto at = diagnostics.find(key);
        if (at == std::string::npos) {
          std::cerr << "m2b1 " << label << ": no host count in the remark\n";
          return false;
        }
        const int64_t reportedHost =
            std::stoll(diagnostics.substr(at + key.size()));
        // The device's points from the tiles: block from the C slice (or
        // the tile it continues), K from the A slice.
        std::set<Point3> device;
        llvm::DenseMap<Operation *, std::array<int64_t, 2>> blockOfTile;
        bool deviceOk = true;
        produced.function.walk([&](Operation *tile) {
          if (tile->getName().getStringRef() != "systolic.matmul_tile" ||
              !deviceOk)
            return;
          auto attr = [&](StringRef name) {
            return tile->getAttrOfType<IntegerAttr>(name).getInt();
          };
          const int64_t m = attr("m"), n = attr("n"), k = attr("k");
          int64_t k0 = 0;
          if (auto aSlice =
                  tile->getOperand(0).getDefiningOp<tensor::ExtractSliceOp>())
            k0 = aSlice.getStaticOffsets()[1];
          std::array<int64_t, 2> origin{0, 0};
          Value cIn = tile->getOperand(2);
          if (auto cSlice = cIn.getDefiningOp<tensor::ExtractSliceOp>()) {
            origin = {cSlice.getStaticOffsets()[0],
                      cSlice.getStaticOffsets()[1]};
          } else if (Operation *previous = cIn.getDefiningOp();
                     previous && blockOfTile.count(previous)) {
            origin = blockOfTile[previous];
          }
          blockOfTile[tile] = origin;
          for (int64_t r = 0; r < m; ++r)
            for (int64_t c = 0; c < n; ++c)
              for (int64_t kk = 0; kk < k; ++kk) {
                const Point3 p{s.lo[0] + origin[0] + r, s.lo[1] + origin[1] + c,
                               s.lo[2] + k0 + kk};
                if (!s.in(p[0], p[1], p[2]) || !device.insert(p).second) {
                  std::cerr << "m2b1 " << label << ": the device point ("
                            << p[0] << ", " << p[1] << ", " << p[2]
                            << ") is outside D' or runs twice\n";
                  deviceOk = false;
                }
              }
        });
        if (!deviceOk)
          return false;
        // The remainder nests: three constant-bound loops and one guard.
        exec::MemoryState memory;
        const unsigned roleOf[3] = {2, 0, 1};
        for (unsigned a = 0; a < 3; ++a) {
          Value argument = produced.function.getArgument(a);
          memory.buffers[argument] = maskBuffer(
              cast<MemRefType>(argument.getType()), roleOf[a],
              MaskData::Integers);
        }
        std::set<Point3> host;
        int64_t sweep = 0, accepted = 0, nests = 0;
        for (Operation &op : produced.function.getBody().front()) {
          auto loop = dyn_cast<affine::AffineForOp>(&op);
          if (!loop)
            continue;
          ++nests;
          SmallVector<Value, 3> ivs;
          Point3 lo{}, hi{};
          affine::AffineForOp level = loop;
          for (unsigned d = 0; d < 3; ++d) {
            if (!level || !level.hasConstantBounds()) {
              std::cerr << "m2b1 " << label << ": a nest without constant"
                        << " bounds\n";
              return false;
            }
            ivs.push_back(level.getInductionVar());
            lo[d] = level.getConstantLowerBound();
            hi[d] = level.getConstantUpperBound() - 1;
            if (d == 2)
              break;
            affine::AffineForOp next;
            unsigned inner = 0;
            for (Operation &child : level.getBody()->getOperations())
              if (auto f = dyn_cast<affine::AffineForOp>(&child)) {
                next = f;
                ++inner;
              }
            if (inner != 1) {
              std::cerr << "m2b1 " << label << ": a nest is not a chain of"
                        << " three loops\n";
              return false;
            }
            level = next;
          }
          scf::IfOp guard;
          unsigned guards = 0;
          level.getBody()->walk([&](scf::IfOp ifOp) {
            guard = ifOp;
            ++guards;
          });
          if (guards != 1) {
            std::cerr << "m2b1 " << label << ": a nest has " << guards
                      << " guards\n";
            return false;
          }
          auto run = exec::interpretGuardedLoopNest(
              loop, guard, ValueRange(ivs), ValueRange{}, ValueRange{},
              ValueRange{}, ValueRange{}, &memory);
          if (!run) {
            std::cerr << "m2b1 " << label << ": a nest did not execute\n";
            return false;
          }
          int64_t volume = 1;
          for (unsigned d = 0; d < 3; ++d)
            volume *= hi[d] - lo[d] + 1;
          if (static_cast<int64_t>(run->visitedPoints) != volume ||
              run->acceptedPoints == 0) {
            std::cerr << "m2b1 " << label << ": a nest visited "
                      << run->visitedPoints << " points of a box of "
                      << volume << " and accepted " << run->acceptedPoints
                      << "\n";
            return false;
          }
          Point3 seenLo{}, seenHi{};
          bool first = true;
          for (const auto &entry : run->counts) {
            const Point3 p{entry.first[0], entry.first[1], entry.first[2]};
            if (entry.second != 1 || !s.in(p[0], p[1], p[2]) ||
                device.count(p) || !host.insert(p).second) {
              std::cerr << "m2b1 " << label << ": the host point (" << p[0]
                        << ", " << p[1] << ", " << p[2] << ") runs "
                        << entry.second << " time(s), is outside D', or is"
                        << " the device's\n";
              return false;
            }
            for (unsigned d = 0; d < 3; ++d) {
              if (first || p[d] < seenLo[d])
                seenLo[d] = p[d];
              if (first || p[d] > seenHi[d])
                seenHi[d] = p[d];
            }
            first = false;
          }
          for (unsigned d = 0; d < 3; ++d)
            if (seenLo[d] != lo[d] || seenHi[d] != hi[d]) {
              std::cerr << "m2b1 " << label << ": loop " << d << " spans ["
                        << lo[d] << ", " << hi[d] << "] but its points ["
                        << seenLo[d] << ", " << seenHi[d]
                        << "]: the bounds are not tight\n";
              return false;
            }
          sweep += volume;
          accepted += static_cast<int64_t>(run->acceptedPoints);
        }
        // The partition: every point of D' is the device's or the host's,
        // and the remark counts the host's.
        int64_t total = 0;
        for (int64_t i = s.lo[0]; i <= s.hi[0]; ++i)
          for (int64_t j = s.lo[1]; j <= s.hi[1]; ++j)
            for (int64_t k = s.lo[2]; k <= s.hi[2]; ++k) {
              if (!s.in(i, j, k))
                continue;
              ++total;
              const Point3 p{i, j, k};
              if (!host.count(p) && !device.count(p)) {
                std::cerr << "m2b1 " << label << ": the point (" << i << ", "
                          << j << ", " << k << ") of D' runs nowhere\n";
                return false;
              }
            }
        if (static_cast<int64_t>(host.size() + device.size()) != total ||
            accepted != reportedHost || nests == 0) {
          std::cerr << "m2b1 " << label << ": " << host.size() << " host + "
                    << device.size() << " device points for " << total
                    << " of D'; the remark says " << reportedHost
                    << " host MACs, the nests accepted " << accepted << "\n";
          return false;
        }
        if (sweepOut)
          *sweepOut = sweep;
        return true;
      };
      GemmText bigFleet = withFleet(square(16, 16, 16, "m2b1_fleet_k_le_i"),
                                    "m2b1_fleet_k_le_i");
      bigFleet.loops = "affine.for %i = 0 to 16 { affine.for %j = 0 to 16 {"
                       " affine.for %k = 0 to affine_map<(d0) -> (d0 + 1)>(%i) {";
      GemmText bothFleet = withFleet(square(16, 16, 16, "m2b1_fleet_between"),
                                     "m2b1_fleet_between");
      bothFleet.loops = "affine.for %i = 0 to 16 { affine.for %j = 0 to 16 {"
                        " affine.for %k = affine_map<(d0) -> (d0)>(%j) to "
                        "affine_map<(d0) -> (d0 + 1)>(%i) {";
      const std::vector<RemainderScenario> scenarios = {
          {&caseA, {0, 0, 0}, {3, 3, 3},
           [](int64_t i, int64_t, int64_t k) { return k <= i; },
           "m2b1 nests: k <= i without a fleet: the nests sweep exactly"
           " their boxes, execute exactly the remainder once, bounds"
           " attained"},
          {&caseB, {0, 0, 0}, {3, 3, 3},
           [](int64_t i, int64_t j, int64_t k) { return k <= i + j; },
           "m2b1 nests: k <= i + j without a fleet"},
          {&caseC, {0, 0, 0}, {3, 3, 3},
           [](int64_t, int64_t j, int64_t k) { return k >= j; },
           "m2b1 nests: k >= j without a fleet"},
          {&caseD, {0, 0, 0}, {3, 3, 7},
           [](int64_t i, int64_t j, int64_t k) { return j <= k && k <= i + 3; },
           "m2b1 nests: j <= k <= i + 3 without a fleet (pieces on both"
           " sides)"},
          {&shifted, {1, 0, 1}, {3, 2, 3},
           [](int64_t i, int64_t, int64_t k) { return k <= i; },
           "m2b1 nests: nonzero lower bounds and offsets"},
          {&bigFleet, {0, 0, 0}, {15, 15, 15},
           [](int64_t i, int64_t, int64_t k) { return k <= i; },
           "m2b1 nests: k <= i with the fleet: every block's nest over its"
           " exact box, the partition exact"},
          {&bothFleet, {0, 0, 0}, {15, 15, 15},
           [](int64_t i, int64_t j, int64_t k) { return j <= k && k <= i; },
           "m2b1 nests: j <= k <= i with the fleet: two pieces per block"
           " below the diagonal, whole blocks on it, the partition exact"},
      };
      for (const RemainderScenario &s : scenarios) {
        int64_t sweptPoints = 0;
        if (verify(checkNests(s, &sweptPoints), s.claim))
          return 1;
        if (s.gemm == &bigFleet &&
            verify(sweptPoints == 2 * 392 + 8 * 36,
                   "m2b1 nests: k <= i on 16^3 with the fleet sweeps 1072"
                   " points (two 7x8x7 and eight 3x4x3 boxes) for its 640"
                   " host MACs; the full K sweep took 4096"))
          return 1;
      }
      // (6c) The strict fallback is untouched: the Phase 3b-5 guarded MAC
      // nest over the whole box.
      {
        ParsedGemm produced;
        std::string diagnostics;
        bool wholeBox = false;
        if (parseGemm(produced, gemmModule(caseA), caseA.name) &&
            runPass(produced,
                    "general-affine=true certified-offload-first=true "
                    "common-fiber-offload=true report=true",
                    diagnostics) &&
            diagnostics.find("UNKNOWN: strict floating-point semantics") !=
                std::string::npos) {
          wholeBox = true;
          unsigned loops = 0;
          produced.function.walk([&](affine::AffineForOp loop) {
            ++loops;
            if (!loop.hasConstantBounds() ||
                loop.getConstantLowerBound() != 0 ||
                loop.getConstantUpperBound() != 4)
              wholeBox = false;
          });
          wholeBox = wholeBox && loops == 3;
        }
        if (verify(wholeBox,
                   "m2b1 nests: the strict fallback still sweeps the whole"
                   " 4x4x4 box"))
          return 1;
      }
    }
  }

  // --------------------------------------------------------
  // Phase C1 (Option C): the symbolic tile-path domain foundation. The exact
  // N-local logical image is reconstructed by buildExactLogicalImage (the
  // same function the coverage check already uses) -- NOT by the guarded-MAC
  // one-local buildRecoverableLogicalDomain, which stays as it is. Validated
  // here against an independent brute-force membership oracle over small
  // bounded domains (test only; the production reconstruction is symbolic).
  // --------------------------------------------------------
  {
    using mlir::presburger::IntegerPolyhedron;
    using mlir::presburger::PresburgerSpace;

    // A source box [0,hi]^2 as an IntegerPolyhedron over (x0, x1).
    auto box2 = [](int hi) {
      IntegerPolyhedron s(PresburgerSpace::getSetSpace(2, 0, 0));
      s.addInequality({1, 0, 0});    //  x0 >= 0
      s.addInequality({-1, 0, hi});  //  hi - x0 >= 0
      s.addInequality({0, 1, 0});    //  x1 >= 0
      s.addInequality({0, -1, hi});  //  hi - x1 >= 0
      return s;
    };

    // Integer membership of (z0, z1) in a 2-D image with existential locals:
    // pin the two dims by equalities and test integer-feasibility. This is the
    // sound local-handling path (isIntegerEmpty projects the existentials); it
    // never subtracts two local-bearing sets.
    auto contains = [](const IntegerPolyhedron &img, int64_t z0, int64_t z1) {
      IntegerPolyhedron p = img;
      const unsigned cols = p.getNumCols();
      llvm::SmallVector<llvm::DynamicAPInt> e0(cols, llvm::DynamicAPInt(0));
      llvm::SmallVector<llvm::DynamicAPInt> e1(cols, llvm::DynamicAPInt(0));
      e0[0] = llvm::DynamicAPInt(1);
      e0[cols - 1] = llvm::DynamicAPInt(-z0);
      e1[1] = llvm::DynamicAPInt(1);
      e1[cols - 1] = llvm::DynamicAPInt(-z1);
      p.addEquality(e0);
      p.addEquality(e1);
      return !p.isIntegerEmpty();
    };

    // Brute-force image of [0,hi]^2 under an integer 2x2 T (test oracle only).
    auto imageOf = [](int64_t t00, int64_t t01, int64_t t10, int64_t t11,
                      int hi) {
      std::set<std::pair<int64_t, int64_t>> image;
      for (int x0 = 0; x0 <= hi; ++x0)
        for (int x1 = 0; x1 <= hi; ++x1)
          image.insert({t00 * x0 + t01 * x1, t10 * x0 + t11 * x1});
      return image;
    };

    // Every image point is in, every other lattice point in the box is out.
    auto matchesImage = [&](const IntegerPolyhedron &img,
                            const std::set<std::pair<int64_t, int64_t>> &image,
                            int64_t lo0, int64_t hi0, int64_t lo1, int64_t hi1) {
      for (int64_t z0 = lo0; z0 <= hi0; ++z0)
        for (int64_t z1 = lo1; z1 <= hi1; ++z1)
          if (contains(img, z0, z1) != (image.count({z0, z1}) != 0))
            return false;
      return true;
    };

    // (1) CASE 1's shape: T = [[1,1],[1,-1]], det = -2. Both inverse rows are
    // odd, so d = 2 and BOTH rows are non-automatic -> two existential locals.
    // This is the multi-local domain the guarded-MAC path refuses (3b-4 det9),
    // reconstructed here on the tile-path function instead.
    {
      PartitionCoordinateMap map;
      map.sourceToLogical = {{BigInt(1), BigInt(1)}, {BigInt(1), BigInt(-1)}};
      map.logicalToSource = {{Rat(1, 2), Rat(1, 2)}, {Rat(1, 2), Rat(-1, 2)}};
      auto img = buildExactLogicalImage(map, box2(3));
      if (verify(img.has_value(),
                 "C1: two-local non-unimodular image reconstructed"))
        return 1;
      if (verify(img->getNumLocalVars() == 2,
                 "C1: exactly two existential congruence locals"))
        return 1;
      // z0 = x0+x1 in [0,6], z1 = x0-x1 in [-3,3].
      if (verify(matchesImage(*img, imageOf(1, 1, 1, -1, 3), 0, 6, -3, 3),
                 "C1: two-local image equals the exact T-image (oracle)"))
        return 1;
    }

    // (2) One congruence local: T = [[1,0],[0,2]]. Row 0 is automatic, row 1
    // is not -> one local.
    {
      PartitionCoordinateMap map;
      map.sourceToLogical = {{BigInt(1), BigInt(0)}, {BigInt(0), BigInt(2)}};
      map.logicalToSource = {{Rat(1), Rat(0)}, {Rat(0), Rat(1, 2)}};
      auto img = buildExactLogicalImage(map, box2(3));
      if (verify(img.has_value() && img->getNumLocalVars() == 1,
                 "C1: one congruence local image reconstructed"))
        return 1;
      // z0 in [0,3], z1 in {0,2,4,6}.
      if (verify(matchesImage(*img, imageOf(1, 0, 0, 2, 3), 0, 3, 0, 6),
                 "C1: one-local image equals the exact T-image (oracle)"))
        return 1;
    }

    // (3) Unimodular map: T = [[0,1],[1,0]] -> no local.
    {
      PartitionCoordinateMap map;
      map.sourceToLogical = {{BigInt(0), BigInt(1)}, {BigInt(1), BigInt(0)}};
      map.logicalToSource = {{Rat(0), Rat(1)}, {Rat(1), Rat(0)}};
      auto img = buildExactLogicalImage(map, box2(3));
      if (verify(img.has_value() && img->getNumLocalVars() == 0,
                 "C1: unimodular image has no local"))
        return 1;
      if (verify(matchesImage(*img, imageOf(0, 1, 1, 0, 3), 0, 3, 0, 3),
                 "C1: unimodular image equals the exact T-image (oracle)"))
        return 1;
    }

    // (4) Empty source domain -> integer-empty image.
    {
      PartitionCoordinateMap map;
      map.sourceToLogical = {{BigInt(1), BigInt(1)}, {BigInt(1), BigInt(-1)}};
      map.logicalToSource = {{Rat(1, 2), Rat(1, 2)}, {Rat(1, 2), Rat(-1, 2)}};
      IntegerPolyhedron empty(PresburgerSpace::getSetSpace(2, 0, 0));
      empty.addInequality({-1, 0, 3}); //  x0 <= 3
      empty.addInequality({1, 0, -5}); //  x0 >= 5  (contradiction)
      empty.addInequality({0, 1, 0});
      empty.addInequality({0, -1, 3});
      auto img = buildExactLogicalImage(map, empty);
      if (verify(img.has_value() && img->isIntegerEmpty(),
                 "C1: empty source domain reconstructs integer-empty"))
        return 1;
    }

    // (5) Unrepresentable coefficient -> conservative refusal (nullopt).
    {
      const BigInt large = BigInt(std::numeric_limits<std::int64_t>::max()) + 1;
      PartitionCoordinateMap map;
      map.sourceToLogical = {{large, BigInt(0)}, {BigInt(0), BigInt(1)}};
      map.logicalToSource = {{Rat(1, large), Rat(0)}, {Rat(0), Rat(1)}};
      auto img = buildExactLogicalImage(map, box2(3));
      if (verify(!img.has_value(),
                 "C1: unrepresentable transform refused (nullopt)"))
        return 1;
    }

    std::cout << "Phase C1 symbolic domain foundation tests passed.\n";
  }

  // --------------------------------------------------------
  // Phase C2 (Option C): symbolic contraction recovery. buildSymbolicContraction
  // recovers the logical M/N/K frame, the exact N-local image, and the A/B/C
  // access maps over [z, q] -- no source-point enumeration. Validated by a
  // differential: the recovered logical access maps, evaluated at the image
  // point of each source iteration, must equal the independently evaluated
  // source access maps. (The source box is enumerated here, in the test
  // oracle only; the production path is symbolic.)
  // --------------------------------------------------------
  {
    using mlir::presburger::IntegerPolyhedron;
    using mlir::presburger::PresburgerSpace;

    // Evaluate buildSymbolicContraction on a case, then differentially check
    // its logical access maps against the source access maps over [0,hi]^rank.
    auto runContraction =
        [&](const char *name, const std::array<IntMatrix, 3> &F,
            const std::array<IntRow, 3> &off, unsigned rank, int hi,
            unsigned wantM, unsigned wantN, unsigned wantK,
            unsigned wantLocals) -> bool {
      IntegerPolyhedron src(PresburgerSpace::getSetSpace(rank, 0, 0));
      for (unsigned a = 0; a < rank; ++a) {
        llvm::SmallVector<llvm::DynamicAPInt> lo(rank + 1, llvm::DynamicAPInt(0));
        llvm::SmallVector<llvm::DynamicAPInt> up(rank + 1, llvm::DynamicAPInt(0));
        lo[a] = llvm::DynamicAPInt(1);                 //  x_a >= 0
        up[a] = llvm::DynamicAPInt(-1);                // -x_a + hi >= 0
        up[rank] = llvm::DynamicAPInt(hi);
        src.addInequality(lo);
        src.addInequality(up);
      }
      std::string why;
      auto c = buildSymbolicContraction(
          F, off, src, rank,
          systolic::logical::ReductionOrderDecision::PreserveSourceOrder, why);
      std::string base = name;
      if (verify(c.has_value(), (base + ": symbolic contraction recovered").c_str()))
        return false;
      if (verify(c->frame.mDims == wantM && c->frame.nDims == wantN &&
                     c->frame.kDims == wantK && c->frame.batchDims == 0,
                 (base + ": frame M/N/K counts").c_str()))
        return false;
      if (verify(c->congruenceLocals == wantLocals,
                 (base + ": congruence-local count").c_str()))
        return false;

      // Independent oracle: recover T and the congruence rows via public APIs.
      auto part = analyzeRankPartition(F[0], F[1], F[2], rank);
      if (verify(part.has_value(), (base + ": oracle rank partition").c_str()))
        return false;
      auto exp = expandRankPartition(*part);
      if (verify(exp.has_value(), (base + ": oracle expansion").c_str()))
        return false;
      auto map = buildPartitionCoordinateMap(*exp);
      if (verify(map.has_value(), (base + ": oracle coordinate map").c_str()))
        return false;
      AffineIntegerDomain unc;
      unc.sourceDimension = rank;
      auto sym = transformToSymbolicDomain(*map, unc);
      if (verify(sym.has_value(), (base + ": oracle symbolic transform").c_str()))
        return false;
      const BigInt &d = sym->denominator;
      const IntMatrix &Mi = sym->inverseNumerator;
      std::vector<unsigned> cr;
      for (unsigned i = 0; i < rank; ++i)
        for (const BigInt &x : Mi[i])
          if (x % d != 0) { cr.push_back(i); break; }
      if (verify(cr.size() == wantLocals,
                 (base + ": oracle congruence-row count").c_str()))
        return false;

      // Differential over the whole source box (test oracle enumeration).
      std::vector<int> x(rank, 0);
      std::function<bool(unsigned)> sweep = [&](unsigned dim) -> bool {
        if (dim == rank) {
          IntRow z(rank, BigInt(0));
          for (unsigned a = 0; a < rank; ++a)
            for (unsigned b = 0; b < rank; ++b)
              z[a] += map->sourceToLogical[a][b] * BigInt(x[b]);
          IntRow q(cr.size(), BigInt(0));
          for (unsigned l = 0; l < cr.size(); ++l)
            q[l] = BigInt(x[cr[l]]);
          for (unsigned op = 0; op < 3; ++op)
            for (unsigned r = 0; r < c->access[op].rows.size(); ++r) {
              BigInt logical = c->access[op].offsets[r];
              for (unsigned a = 0; a < rank; ++a)
                logical += c->access[op].rows[r][a] * z[a];
              for (unsigned l = 0; l < cr.size(); ++l)
                logical += c->access[op].rows[r][rank + l] * q[l];
              BigInt source = off[op][r];
              for (unsigned a = 0; a < rank; ++a)
                source += F[op][r][a] * BigInt(x[a]);
              if (logical != source)
                return false;
            }
          return true;
        }
        for (x[dim] = 0; x[dim] <= hi; ++x[dim])
          if (!sweep(dim + 1))
            return false;
        return true;
      };
      if (verify(sweep(0),
                 (base + ": logical access maps reproduce source addresses "
                         "(differential over the box, A/B/C)").c_str()))
        return false;
      return true;
    };

    // CASE 1: C[i+j-2k+6][i-2j+k+6] += A[i+j-2k+6][i+j+k] * B[i+j+k][i-2j+k+6].
    // det(T) = -9, denominator d = 3; all three inverse rows are non-divisible,
    // refuses (3b-4 det9) -- recovered here through the tile-path foundation.
    {
      std::array<IntMatrix, 3> F = {
          IntMatrix{{BigInt(1), BigInt(1), BigInt(-2)},
                    {BigInt(1), BigInt(1), BigInt(1)}}, // A rows
          IntMatrix{{BigInt(1), BigInt(1), BigInt(1)},
                    {BigInt(1), BigInt(-2), BigInt(1)}}, // B rows
          IntMatrix{{BigInt(1), BigInt(1), BigInt(-2)},
                    {BigInt(1), BigInt(-2), BigInt(1)}}, // C rows
      };
      std::array<IntRow, 3> off = {IntRow{BigInt(6), BigInt(0)},
                                   IntRow{BigInt(0), BigInt(6)},
                                   IntRow{BigInt(6), BigInt(6)}};
      if (!runContraction("C2 CASE1", F, off, 3, 3, 1, 1, 1, 3))
        return 1;
    }

    // Plain GEMM: C[i][j] += A[i][k] * B[k][j]. Unimodular (T = I), no locals.
    {
      std::array<IntMatrix, 3> F = {
          IntMatrix{{BigInt(1), BigInt(0), BigInt(0)},
                    {BigInt(0), BigInt(0), BigInt(1)}}, // A[i][k]
          IntMatrix{{BigInt(0), BigInt(0), BigInt(1)},
                    {BigInt(0), BigInt(1), BigInt(0)}}, // B[k][j]
          IntMatrix{{BigInt(1), BigInt(0), BigInt(0)},
                    {BigInt(0), BigInt(1), BigInt(0)}}, // C[i][j]
      };
      std::array<IntRow, 3> off = {IntRow{BigInt(0), BigInt(0)},
                                   IntRow{BigInt(0), BigInt(0)},
                                   IntRow{BigInt(0), BigInt(0)}};
      if (!runContraction("C2 plain", F, off, 3, 3, 1, 1, 1, 0))
        return 1;
    }

    std::cout << "Phase C2 symbolic contraction tests passed.\n";
  }

  // --------------------------------------------------------
  // Phase C3 (Option C): symbolic tile partition. buildSymbolicTilePartition
  // partitions the exact logical domain into disjoint reduction tiles grouped
  // by constant length L with a single proven stride g (g == 1 contiguous,
  // g > 1 sublattice-punctured / non-contiguous). Validated by a brute-force
  // oracle over the bounded source domain: the enumeration lives in the test
  // oracle only -- the production partitioner is symbolic. The oracle checks
  // that every fiber is an arithmetic progression with the global stride g,
  // that the minimum anchor of each fiber lies in exactly one group of the
  // right length (coverage + cross-length disjointness + the min-anchor graph
  // property), that non-minimum fiber points are never anchors (multiplicity),
  // and that each operand/output address advances by the reported t-stride.
  // --------------------------------------------------------
  {
    using mlir::presburger::IntegerPolyhedron;
    using mlir::presburger::PresburgerSpace;

    auto box = [](unsigned rank, int hi) {
      IntegerPolyhedron s(PresburgerSpace::getSetSpace(rank, 0, 0));
      for (unsigned a = 0; a < rank; ++a) {
        llvm::SmallVector<std::int64_t> lo(rank + 1, 0), up(rank + 1, 0);
        lo[a] = 1;
        s.addInequality(lo); // x_a >= 0
        up[a] = -1;
        up[rank] = hi;
        s.addInequality(up); // hi - x_a >= 0
      }
      return s;
    };

    auto inDomain = [](const IntegerPolyhedron &dom,
                       const std::vector<int> &x) -> bool {
      IntegerPolyhedron P = dom;
      const unsigned cols = P.getNumCols();
      for (std::size_t j = 0; j < x.size(); ++j) {
        llvm::SmallVector<std::int64_t> eq(cols, 0);
        eq[j] = 1;
        eq[cols - 1] = -static_cast<std::int64_t>(x[j]);
        P.addEquality(eq);
      }
      return !P.isIntegerEmpty();
    };

    auto inGroup = [](const systolic::logical::TileFiberGroup &grp,
                      const std::vector<std::int64_t> &za) -> bool {
      for (const auto &piece : grp.anchorPieces) {
        IntegerPolyhedron P = piece;
        const unsigned cols = P.getNumCols();
        for (std::size_t j = 0; j < za.size(); ++j) {
          llvm::SmallVector<std::int64_t> eq(cols, 0);
          eq[j] = 1;
          eq[cols - 1] = -za[j];
          P.addEquality(eq);
        }
        if (!P.isIntegerEmpty())
          return true;
      }
      return false;
    };

    auto runPartition =
        [&](const char *name, const std::array<IntMatrix, 3> &F,
            const std::array<IntRow, 3> &off, unsigned rank,
            const IntegerPolyhedron &domain, int hi, long expStride,
            long expGroups, long expFibers, long expMacs,
            std::map<std::int64_t, long> &lenHist) -> bool {
      lenHist.clear();
      std::string why;
      auto part = buildSymbolicTilePartition(
          F, off, domain, rank,
          systolic::logical::ReductionOrderDecision::PreserveSourceOrder, why);
      std::string b = name;
      if (verify(part.has_value(), (b + ": partition built").c_str()))
        return false;
      if (expStride >= 0 &&
          verify(part->stride == expStride, (b + ": stride g").c_str()))
        return false;

      auto p2 = analyzeRankPartition(F[0], F[1], F[2], rank);
      auto e2 = p2 ? expandRankPartition(*p2) : std::nullopt;
      auto m2 = e2 ? buildPartitionCoordinateMap(*e2) : std::nullopt;
      if (verify(p2 && e2 && m2, (b + ": oracle coordinate map").c_str()))
        return false;
      const unsigned n = rank;
      const unsigned kIdx = part->kIndex;

      auto zOf = [&](const std::vector<int> &x) {
        std::vector<std::int64_t> z(n, 0);
        for (unsigned a = 0; a < n; ++a) {
          BigInt acc = 0;
          for (unsigned c = 0; c < n; ++c)
            acc += m2->sourceToLogical[a][c] * BigInt(x[c]);
          z[a] = acc.convert_to<std::int64_t>();
        }
        return z;
      };
      auto okeyOf = [&](const std::vector<std::int64_t> &z) {
        std::vector<std::int64_t> o;
        for (unsigned a = 0; a < n; ++a)
          if (a != kIdx)
            o.push_back(z[a]);
        return o;
      };
      auto zFromOkey = [&](const std::vector<std::int64_t> &o, std::int64_t k) {
        std::vector<std::int64_t> z(n, 0);
        unsigned oi = 0;
        for (unsigned a = 0; a < n; ++a)
          z[a] = (a == kIdx) ? k : o[oi++];
        return z;
      };

      std::map<std::vector<std::int64_t>, std::map<std::int64_t, std::vector<int>>>
          fibers;
      std::vector<int> x(n, 0);
      std::function<void(unsigned)> sweep = [&](unsigned dim) {
        if (dim == n) {
          if (!inDomain(domain, x))
            return;
          auto z = zOf(x);
          fibers[okeyOf(z)].emplace(z[kIdx], x);
          return;
        }
        for (x[dim] = 0; x[dim] <= hi; ++x[dim])
          sweep(dim + 1);
      };
      sweep(0);

      const std::int64_t g = part->stride;
      long macs = 0;
      std::set<std::int64_t> distinctLens;
      bool good = true;

      for (const auto &kv : fibers) {
        std::vector<std::int64_t> ks;
        for (const auto &e : kv.second)
          ks.push_back(e.first);
        const std::int64_t L = (std::int64_t)ks.size();
        macs += L;
        distinctLens.insert(L);
        ++lenHist[L];
        for (std::size_t t = 1; t < ks.size(); ++t)
          good &= (ks[t] - ks[t - 1] == g);
        const std::int64_t k0 = ks.front();
        auto za = zFromOkey(kv.first, k0);

        int hits = 0;
        std::int64_t hitLen = -1;
        for (const auto &grp : part->groups)
          if (inGroup(grp, za)) {
            ++hits;
            hitLen = grp.length;
          }
        good &= (hits == 1) && (hitLen == L);

        if (ks.size() >= 2) {
          auto zb = zFromOkey(kv.first, k0 + g);
          for (const auto &grp : part->groups)
            good &= !inGroup(grp, zb);
        }

        for (std::size_t t = 0; t + 1 < ks.size(); ++t) {
          const auto &xa = kv.second.at(ks[t]);
          const auto &xb = kv.second.at(ks[t + 1]);
          for (unsigned op = 0; op < 3; ++op)
            for (std::size_t r = 0; r < F[op].size(); ++r) {
              BigInt aa = off[op][r], bb = off[op][r];
              for (unsigned c = 0; c < n; ++c) {
                aa += F[op][r][c] * BigInt(xa[c]);
                bb += F[op][r][c] * BigInt(xb[c]);
              }
              good &= ((bb - aa) == part->tStride[op][r]);
            }
        }
      }

      if (verify(good,
                 (b + ": fibers are stride-g APs; min-anchor unique and "
                      "correct length; multiplicity and address/t-stride exact")
                     .c_str()))
        return false;

      // Within each group the anchor pieces are pairwise disjoint (both share
      // the same congruence-local definition from D, so adding one piece's
      // constraints to the other tests z-projection overlap exactly).
      bool disjoint = true;
      for (const auto &grp : part->groups)
        for (std::size_t a = 0; a < grp.anchorPieces.size(); ++a)
          for (std::size_t c = a + 1; c < grp.anchorPieces.size(); ++c) {
            IntegerPolyhedron inter = grp.anchorPieces[a];
            const auto &pc = grp.anchorPieces[c];
            for (unsigned r = 0; r < pc.getNumInequalities(); ++r)
              inter.addInequality(pc.getInequality(r));
            for (unsigned r = 0; r < pc.getNumEqualities(); ++r)
              inter.addEquality(pc.getEquality(r));
            disjoint &= inter.isIntegerEmpty();
          }
      if (verify(disjoint, (b + ": anchor pieces pairwise disjoint").c_str()))
        return false;

      // Absolute access equivalence: the stored logical access maps over [z,u]
      // reproduce every source operand/output address, including the
      // batch-dependent terms (u_l = x[congruenceRow_l]).
      {
        AffineIntegerDomain unc;
        unc.sourceDimension = n;
        auto sym = transformToSymbolicDomain(*m2, unc);
        std::vector<unsigned> cr;
        if (sym)
          for (unsigned i = 0; i < n; ++i)
            for (const BigInt &v : sym->inverseNumerator[i])
              if (v % sym->denominator != 0) { cr.push_back(i); break; }
        bool addrOk = sym.has_value() && cr.size() == part->congruenceLocals;
        for (const auto &kv : fibers)
          for (const auto &e : kv.second) {
            const auto &xx = e.second;
            std::vector<std::int64_t> z = zOf(xx);
            for (unsigned op = 0; op < 3 && addrOk; ++op)
              for (std::size_t r = 0; r < part->access[op].rows.size(); ++r) {
                BigInt pred = part->access[op].offsets[r];
                for (unsigned c = 0; c < n; ++c)
                  pred += part->access[op].rows[r][c] * BigInt(z[c]);
                for (unsigned l = 0; l < cr.size(); ++l)
                  pred += part->access[op].rows[r][n + l] * BigInt(xx[cr[l]]);
                BigInt src = off[op][r];
                for (unsigned c = 0; c < n; ++c)
                  src += F[op][r][c] * BigInt(xx[c]);
                if (pred != src)
                  addrOk = false;
              }
          }
        if (verify(addrOk, (b + ": logical access maps reproduce "
                                "batch-dependent source addresses").c_str()))
          return false;
      }

      if (verify(part->groups.size() == distinctLens.size(),
                 (b + ": one group per realised length").c_str()))
        return false;
      for (const auto &grp : part->groups)
        if (verify(distinctLens.count(grp.length) == 1,
                   (b + ": group length is realised").c_str()))
          return false;
      if (expGroups >= 0 &&
          verify((long)part->groups.size() == expGroups,
                 (b + ": group count").c_str()))
        return false;
      if (expFibers >= 0 &&
          verify((long)fibers.size() == expFibers,
                 (b + ": output fiber count").c_str()))
        return false;
      if (expMacs >= 0 &&
          verify(macs == expMacs, (b + ": source MAC count").c_str()))
        return false;
      return true;
    };

    std::map<std::int64_t, long> hist;

    // Plain GEMM C[i][j] += A[i][k]*B[k][j]: contiguous (g == 1), one length.
    {
      std::array<IntMatrix, 3> F = {
          IntMatrix{{BigInt(1), BigInt(0), BigInt(0)},
                    {BigInt(0), BigInt(0), BigInt(1)}},
          IntMatrix{{BigInt(0), BigInt(0), BigInt(1)},
                    {BigInt(0), BigInt(1), BigInt(0)}},
          IntMatrix{{BigInt(1), BigInt(0), BigInt(0)},
                    {BigInt(0), BigInt(1), BigInt(0)}}};
      std::array<IntRow, 3> off = {IntRow{BigInt(0), BigInt(0)},
                                   IntRow{BigInt(0), BigInt(0)},
                                   IntRow{BigInt(0), BigInt(0)}};
      if (!runPartition("C3 plain", F, off, 3, box(3, 3), 3, 1, 1, 16, 64, hist))
        return 1;
    }

    // CASE 1: det(T) == -9, stride g == 3, non-contiguous fibers. 37 output
    // fibers, 64 source MACs, length histogram {1:18, 2:12, 3:6, 4:1}.
    {
      std::array<IntMatrix, 3> F = {
          IntMatrix{{BigInt(1), BigInt(1), BigInt(-2)},
                    {BigInt(1), BigInt(1), BigInt(1)}},
          IntMatrix{{BigInt(1), BigInt(1), BigInt(1)},
                    {BigInt(1), BigInt(-2), BigInt(1)}},
          IntMatrix{{BigInt(1), BigInt(1), BigInt(-2)},
                    {BigInt(1), BigInt(-2), BigInt(1)}}};
      std::array<IntRow, 3> off = {IntRow{BigInt(6), BigInt(0)},
                                   IntRow{BigInt(0), BigInt(6)},
                                   IntRow{BigInt(6), BigInt(6)}};
      if (!runPartition("C3 CASE1", F, off, 3, box(3, 3), 3, 3, 4, 37, 64, hist))
        return 1;
      bool distOk = hist.size() == 4 && hist[1] == 18 && hist[2] == 12 &&
                    hist[3] == 6 && hist[4] == 1;
      if (verify(distOk, "C3 CASE1: fiber-length histogram {1:18,2:12,3:6,4:1}"))
        return 1;
    }

    // Piecewise-affine bounds / variable fiber lengths: plain GEMM over a
    // banded source domain i + k <= 3, so L(i) = 4 - i (lengths 4,3,2,1).
    {
      std::array<IntMatrix, 3> F = {
          IntMatrix{{BigInt(1), BigInt(0), BigInt(0)},
                    {BigInt(0), BigInt(0), BigInt(1)}},
          IntMatrix{{BigInt(0), BigInt(0), BigInt(1)},
                    {BigInt(0), BigInt(1), BigInt(0)}},
          IntMatrix{{BigInt(1), BigInt(0), BigInt(0)},
                    {BigInt(0), BigInt(1), BigInt(0)}}};
      std::array<IntRow, 3> off = {IntRow{BigInt(0), BigInt(0)},
                                   IntRow{BigInt(0), BigInt(0)},
                                   IntRow{BigInt(0), BigInt(0)}};
      IntegerPolyhedron tri = box(3, 3);
      tri.addInequality({-1, 0, -1, 3}); // 3 - x0 - x2 >= 0  (i + k <= 3)
      if (!runPartition("C3 banded", F, off, 3, tri, 3, 1, 4, 16, 40, hist))
        return 1;
    }

    // Empty source domain: a valid partition with no fibers.
    {
      std::array<IntMatrix, 3> F = {
          IntMatrix{{BigInt(1), BigInt(0), BigInt(0)},
                    {BigInt(0), BigInt(0), BigInt(1)}},
          IntMatrix{{BigInt(0), BigInt(0), BigInt(1)},
                    {BigInt(0), BigInt(1), BigInt(0)}},
          IntMatrix{{BigInt(1), BigInt(0), BigInt(0)},
                    {BigInt(0), BigInt(1), BigInt(0)}}};
      std::array<IntRow, 3> off = {IntRow{BigInt(0), BigInt(0)},
                                   IntRow{BigInt(0), BigInt(0)},
                                   IntRow{BigInt(0), BigInt(0)}};
      IntegerPolyhedron empty = box(3, 3);
      empty.addInequality({1, 0, 0, -5}); // x0 >= 5 contradicts x0 <= 3
      if (!runPartition("C3 empty", F, off, 3, empty, 3, 1, 0, 0, 0, hist))
        return 1;
    }

    // Standard batched GEMM, one batch dim: C[b][i][j] += A[b][i][k]*B[b][k][j];
    // source x = (b,i,j,k). Genuine parallel batch (C indexed by b): operand and
    // output addresses depend on b, g == 1, one length group.
    {
      std::array<IntMatrix, 3> F = {
          IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1)}},
          IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1)},{BigInt(0),BigInt(0),BigInt(1),BigInt(0)}},
          IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(1),BigInt(0)}}};
      std::array<IntRow, 3> off = {IntRow{BigInt(0),BigInt(0),BigInt(0)},IntRow{BigInt(0),BigInt(0),BigInt(0)},IntRow{BigInt(0),BigInt(0),BigInt(0)}};
      if (!runPartition("C3 batched1", F, off, 4, box(4,3), 3, 1, 1, 64, 256, hist))
        return 1;
    }

    // Batched GEMM, two batch dims: C[b0][b1][i][j] += A[b0][b1][i][k]*B[b0][b1][k][j];
    // source x = (b0,b1,i,j,k).
    {
      std::array<IntMatrix, 3> F = {
          IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(1),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(1)}},
          IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(1)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1),BigInt(0)}},
          IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(1),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1),BigInt(0)}}};
      std::array<IntRow, 3> off = {IntRow{BigInt(0),BigInt(0),BigInt(0),BigInt(0)},IntRow{BigInt(0),BigInt(0),BigInt(0),BigInt(0)},IntRow{BigInt(0),BigInt(0),BigInt(0),BigInt(0)}};
      if (!runPartition("C3 batched2", F, off, 5, box(5,2), 2, 1, 1, 81, 243, hist))
        return 1;
    }

    // Batch-dependent K bounds: batched GEMM over k <= b, so L(b) = b + 1
    // (lengths 1,2,3,4 across the batch coordinate).
    {
      std::array<IntMatrix, 3> F = {
          IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1)}},
          IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1)},{BigInt(0),BigInt(0),BigInt(1),BigInt(0)}},
          IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(1),BigInt(0)}}};
      std::array<IntRow, 3> off = {IntRow{BigInt(0),BigInt(0),BigInt(0)},IntRow{BigInt(0),BigInt(0),BigInt(0)},IntRow{BigInt(0),BigInt(0),BigInt(0)}};
      IntegerPolyhedron dom = box(4, 3);
      dom.addInequality({1, 0, 0, -1, 0}); // b - k >= 0  (k <= b)
      if (!runPartition("C3 batchK", F, off, 4, dom, 3, 1, 4, 64, 160, hist))
        return 1;
    }

    // Batched + stride-3 non-contiguous skew: batched CASE 1. Genuine batch,
    // batch-dependent skewed operand/output addresses, g == 3, variable length.
    {
      std::array<IntMatrix, 3> F = {
          IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(1),BigInt(1),BigInt(-2)},{BigInt(0),BigInt(1),BigInt(1),BigInt(1)}},
          IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(1),BigInt(1),BigInt(1)},{BigInt(0),BigInt(1),BigInt(-2),BigInt(1)}},
          IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(1),BigInt(1),BigInt(-2)},{BigInt(0),BigInt(1),BigInt(-2),BigInt(1)}}};
      std::array<IntRow, 3> off = {IntRow{BigInt(0),BigInt(6),BigInt(0)},IntRow{BigInt(0),BigInt(0),BigInt(6)},IntRow{BigInt(0),BigInt(6),BigInt(6)}};
      if (!runPartition("C3 batchedCase1", F, off, 4, box(4,3), 3, 3, 4, 148, 256, hist))
        return 1;
    }

    // Empty batched domain: a valid partition with no fibers.
    {
      std::array<IntMatrix, 3> F = {
          IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1)}},
          IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1)},{BigInt(0),BigInt(0),BigInt(1),BigInt(0)}},
          IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(1),BigInt(0)}}};
      std::array<IntRow, 3> off = {IntRow{BigInt(0),BigInt(0),BigInt(0)},IntRow{BigInt(0),BigInt(0),BigInt(0)},IntRow{BigInt(0),BigInt(0),BigInt(0)}};
      IntegerPolyhedron dom = box(4, 3);
      dom.addInequality({1, 0, 0, 0, -5}); // b >= 5 contradicts b <= 3
      if (!runPartition("C3 batchEmpty", F, off, 4, dom, 3, 1, 0, 0, 0, hist))
        return 1;
    }

    // Batch coordinates do NOT distinguish physical output addresses: a double
    // reduction C[i][j] += A[i][k1][k2]*B[k1][k2][j]. The recognizer demotes the
    // extra contraction axis to "batch", but C is not indexed by it, so distinct
    // logical outputs alias one physical C element. Reductions are not
    // independent -> refuse conservatively with UNKNOWN (requirement 10).
    {
      std::array<IntMatrix, 3> F = {
          IntMatrix{{BigInt(1), BigInt(0), BigInt(0), BigInt(0)},
                    {BigInt(0), BigInt(0), BigInt(1), BigInt(0)},
                    {BigInt(0), BigInt(0), BigInt(0), BigInt(1)}},
          IntMatrix{{BigInt(0), BigInt(0), BigInt(1), BigInt(0)},
                    {BigInt(0), BigInt(0), BigInt(0), BigInt(1)},
                    {BigInt(0), BigInt(1), BigInt(0), BigInt(0)}},
          IntMatrix{{BigInt(1), BigInt(0), BigInt(0), BigInt(0)},
                    {BigInt(0), BigInt(1), BigInt(0), BigInt(0)}}};
      std::array<IntRow, 3> off = {IntRow{BigInt(0), BigInt(0), BigInt(0)},
                                   IntRow{BigInt(0), BigInt(0), BigInt(0)},
                                   IntRow{BigInt(0), BigInt(0)}};
      std::string why;
      auto part = buildSymbolicTilePartition(
          F, off, box(4, 3), 4,
          systolic::logical::ReductionOrderDecision::PreserveSourceOrder, why);
      if (verify(!part.has_value(),
                 "C3 multi-axis contraction: refused (UNKNOWN)"))
        return 1;
      if (verify(why.find("UNKNOWN") != std::string::npos &&
                     (why.find("batch") != std::string::npos ||
                      why.find("reduction") != std::string::npos),
                 "C3 multi-axis contraction: reason names the unsupported axis"))
        return 1;
    }

    // Phase C3c (classification). The recognizer keeps genuine extra reduction
    // axes as requiresReduction partitions, NOT batch. buildSymbolicTilePartition
    // now counts the true reduction rank and refuses a genuine multi-dimensional
    // reduction with a distinct UNKNOWN (flattening into one device K is a later
    // stage), never conflating it with batch or with output aliasing. The
    // reduction rank is detected even when a real batch is also present. The
    // two-dimensional reduction above is now reported as a multi-dimensional
    // reduction; the cases below add higher reduction rank, a batched second
    // reduction, and a single-reduction output-aliasing control.
    auto expectUnknown = [&](const char *name, const std::array<IntMatrix, 3> &F,
                             const std::array<IntRow, 3> &off, unsigned rank,
                             const char *needle) -> bool {
      std::string why;
      auto part = buildSymbolicTilePartition(
          F, off, box(rank, 3), rank,
          systolic::logical::ReductionOrderDecision::PreserveSourceOrder, why);
      std::string base = name;
      if (verify(!part.has_value(), (base + ": refused (UNKNOWN)").c_str()))
        return false;
      if (verify(why.find("UNKNOWN") != std::string::npos &&
                     why.find(needle) != std::string::npos,
                 (base + ": reason is correct").c_str()))
        return false;
      return true;
    };

    // Two-dimensional reduction, now checked for the precise reason.
    if (!expectUnknown("C3c 2D-reduction",
          {IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1)}},
           IntMatrix{{BigInt(0),BigInt(0),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0)}},
           IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0)}}},
          {IntRow{BigInt(0),BigInt(0),BigInt(0)},IntRow{BigInt(0),BigInt(0),BigInt(0)},IntRow{BigInt(0),BigInt(0)}},
          4, "multi-dimensional reduction"))
      return 1;

    // Three-dimensional reduction: C[i][j] += A[i][k1][k2][k3]*B[k1][k2][k3][j].
    if (!expectUnknown("C3c 3D-reduction",
          {IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(1),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(1)}},
           IntMatrix{{BigInt(0),BigInt(0),BigInt(1),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(1)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0),BigInt(0)}},
           IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0),BigInt(0)}}},
          {IntRow{BigInt(0),BigInt(0),BigInt(0),BigInt(0)},IntRow{BigInt(0),BigInt(0),BigInt(0),BigInt(0)},IntRow{BigInt(0),BigInt(0)}},
          5, "multi-dimensional reduction"))
      return 1;

    // Genuine batch AND a genuine second reduction: C[b][i][j] +=
    // A[b][i][k1][k2]*B[b][k1][k2][j]. Reduction rank 2 is detected even though a
    // real batch b is present -> UNKNOWN (reduction), never reinterpreted as batch.
    if (!expectUnknown("C3c batched-2reduction",
          {IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(1)}},
           IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(1)},{BigInt(0),BigInt(0),BigInt(1),BigInt(0),BigInt(0)}},
           IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(1),BigInt(0),BigInt(0)}}},
          {IntRow{BigInt(0),BigInt(0),BigInt(0),BigInt(0)},IntRow{BigInt(0),BigInt(0),BigInt(0),BigInt(0)},IntRow{BigInt(0),BigInt(0),BigInt(0)}},
          5, "multi-dimensional reduction"))
      return 1;

    // Output aliasing with a SINGLE reduction: C[i][j] += A[i][k]*B[k][j][p];
    // p (in B only) is batch-classified but C does not index it, so distinct
    // logical outputs alias one C element. Reduction rank is 1, so this reaches
    // the aliasing gate (not the multi-dimensional gate) -> UNKNOWN (aliasing).
    if (!expectUnknown("C3c output-aliasing",
          {IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(1),BigInt(0)}},
           IntMatrix{{BigInt(0),BigInt(0),BigInt(1),BigInt(0)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1)}},
           IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0)}}},
          {IntRow{BigInt(0),BigInt(0)},IntRow{BigInt(0),BigInt(0),BigInt(0)},IntRow{BigInt(0),BigInt(0)}},
          4, "aliases"))
      return 1;

    // Overflow / unrepresentable: a coefficient outside i64 is refused.
    {
      const BigInt big = BigInt(std::numeric_limits<std::int64_t>::max()) + 1;
      std::array<IntMatrix, 3> F = {
          IntMatrix{{big, BigInt(0), BigInt(0)},
                    {BigInt(0), BigInt(0), BigInt(1)}},
          IntMatrix{{BigInt(0), BigInt(0), BigInt(1)},
                    {BigInt(0), BigInt(1), BigInt(0)}},
          IntMatrix{{big, BigInt(0), BigInt(0)},
                    {BigInt(0), BigInt(1), BigInt(0)}}};
      std::array<IntRow, 3> off = {IntRow{BigInt(0), BigInt(0)},
                                   IntRow{BigInt(0), BigInt(0)},
                                   IntRow{BigInt(0), BigInt(0)}};
      std::string why;
      auto part = buildSymbolicTilePartition(
          F, off, box(3, 3), 3,
          systolic::logical::ReductionOrderDecision::PreserveSourceOrder, why);
      if (verify(!part.has_value(), "C3 overflow: refused (UNKNOWN)"))
        return 1;
    }

    std::cout << "Phase C3 symbolic tile partition tests passed.\n";
  }

  // --------------------------------------------------------
  // Phase C3c-2 (Option C): exact multi-dimensional reduction flattening.
  // buildFlattenedReduction certifies a mixed-radix flattening of a genuine
  // rectangular multi-axis reduction into one ordinal, reusing C2/C3. Validated
  // by a brute-force oracle over the bounded domain (enumeration is test-only):
  // every output's reduction set is the exact product box (output-independent),
  // the mixed-radix map is a bijection onto [0, L) whose inverse recovers the
  // exact source reduction coordinates (so C4 reads the exact operand element),
  // the flattened order is the lexicographic most-significant-axis-first order,
  // the MAC multiplicity is exact, and batch axes are excluded from K. No tile
  // is emitted (that is C4); genuine multi-index operands are reported Case B
  // (floor/mod -> needs collapse/packing at emission).
  // --------------------------------------------------------
  {
    using mlir::presburger::IntegerPolyhedron;
    using mlir::presburger::PresburgerSpace;

    auto box = [](unsigned rank, int hi) {
      IntegerPolyhedron s(PresburgerSpace::getSetSpace(rank, 0, 0));
      for (unsigned a = 0; a < rank; ++a) {
        llvm::SmallVector<std::int64_t> lo(rank + 1, 0), up(rank + 1, 0);
        lo[a] = 1;
        s.addInequality(lo);
        up[a] = -1;
        up[rank] = hi;
        s.addInequality(up);
      }
      return s;
    };
    auto inDomain = [](const IntegerPolyhedron &dom,
                       const std::vector<int> &x) -> bool {
      IntegerPolyhedron P = dom;
      const unsigned cols = P.getNumCols();
      for (std::size_t j = 0; j < x.size(); ++j) {
        llvm::SmallVector<std::int64_t> eq(cols, 0);
        eq[j] = 1;
        eq[cols - 1] = -static_cast<std::int64_t>(x[j]);
        P.addEquality(eq);
      }
      return !P.isIntegerEmpty();
    };

    auto runFlatten = [&](const char *name, const std::array<IntMatrix, 3> &F,
                          const std::array<IntRow, 3> &off, unsigned rank,
                          const IntegerPolyhedron &dom, int hi, unsigned expR,
                          long expLen, bool expEmpty, bool expCaseA,
                          bool expCaseB) -> bool {
      std::string why;
      auto f = buildFlattenedReduction(
          F, off, dom, rank,
          systolic::logical::ReductionOrderDecision::PreserveSourceOrder, why);
      std::string b = name;
      if (verify(f.has_value(), (b + ": certified").c_str()))
        return false;
      if (verify(f->reductionRank == expR, (b + ": reduction rank").c_str()))
        return false;
      if (verify(f->length.convert_to<long>() == expLen,
                 (b + ": flattened length").c_str()))
        return false;
      if (verify(f->empty == expEmpty, (b + ": emptiness").c_str()))
        return false;
      if (verify(f->operandAffineStridedInT[0] == expCaseA &&
                     f->operandAffineStridedInT[1] == expCaseB,
                 (b + ": operand addressability class").c_str()))
        return false;
      if (verify(!f->orderMatchesSourceForStrictFP,
                 (b + ": strict-FP source order not claimed").c_str()))
        return false;

      auto p2 = analyzeRankPartition(F[0], F[1], F[2], rank);
      auto e2 = p2 ? expandRankPartition(*p2) : std::nullopt;
      auto m2 = e2 ? buildPartitionCoordinateMap(*e2) : std::nullopt;
      if (verify(p2 && e2 && m2, (b + ": oracle coordinate map").c_str()))
        return false;
      const unsigned n = rank;
      auto zOf = [&](const std::vector<int> &x) {
        std::vector<std::int64_t> z(n, 0);
        for (unsigned a = 0; a < n; ++a) {
          BigInt acc = 0;
          for (unsigned c = 0; c < n; ++c)
            acc += m2->sourceToLogical[a][c] * BigInt(x[c]);
          z[a] = acc.convert_to<std::int64_t>();
        }
        return z;
      };
      std::vector<bool> isRed(n, false);
      for (unsigned c : f->reductionCols)
        isRed[c] = true;

      std::map<std::vector<std::int64_t>, std::set<std::vector<std::int64_t>>>
          byOut;
      long macs = 0;
      std::vector<int> x(n, 0);
      std::function<void(unsigned)> sweep = [&](unsigned dim) {
        if (dim == n) {
          if (!inDomain(dom, x))
            return;
          auto z = zOf(x);
          std::vector<std::int64_t> okey, tup;
          for (unsigned a = 0; a < n; ++a)
            (isRed[a] ? tup : okey).push_back(z[a]);
          byOut[okey].insert(tup);
          ++macs;
          return;
        }
        for (x[dim] = 0; x[dim] <= hi; ++x[dim])
          sweep(dim + 1);
      };
      sweep(0);

      if (expEmpty) {
        if (verify(macs == 0 && byOut.empty(),
                   (b + ": empty domain has no MACs").c_str()))
          return false;
        return true;
      }

      // The product box the certificate claims.
      std::set<std::vector<std::int64_t>> boxSet;
      std::function<void(unsigned, std::vector<std::int64_t> &)> grow =
          [&](unsigned p, std::vector<std::int64_t> &cur) {
            if (p == expR) {
              boxSet.insert(cur);
              return;
            }
            for (std::int64_t v = 0; v < f->extents[p]; ++v) {
              cur.push_back(f->lowerBounds[p] + v);
              grow(p + 1, cur);
              cur.pop_back();
            }
          };
      {
        std::vector<std::int64_t> cur;
        grow(0, cur);
      }

      bool ok = (long)boxSet.size() == expLen &&
                (long)byOut.size() * (long)boxSet.size() == macs;
      // every output's reduction set equals the box (output-independent)
      for (auto &kv : byOut)
        ok &= (kv.second == boxSet);

      // mixed-radix bijection, exact inverse, lexicographic order
      std::map<long, std::vector<std::int64_t>> byT;
      for (auto &tup : boxSet) {
        BigInt t = 0;
        for (unsigned p = 0; p < expR; ++p)
          t += BigInt(tup[p] - f->lowerBounds[p]) * f->radixWeights[p];
        long tv = t.convert_to<long>();
        ok &= (tv >= 0 && tv < expLen);
        for (unsigned p = 0; p < expR; ++p) {
          long kp = f->lowerBounds[p] +
                    (tv / f->radixWeights[p].convert_to<long>()) % f->extents[p];
          ok &= (kp == tup[p]);
        }
        byT[tv] = tup;
      }
      ok &= ((long)byT.size() == expLen);
      {
        std::vector<std::vector<std::int64_t>> lex(boxSet.begin(), boxSet.end());
        long t = 0;
        for (auto &kv : byT) {
          ok &= (kv.first == t) && (kv.second == lex[t]);
          ++t;
        }
      }
      if (verify(ok, (b + ": reduction box = product, mixed-radix bijection, "
                          "exact inverse, lexicographic order, MAC multiplicity")
                         .c_str()))
        return false;
      return true;
    };

    auto runFlattenUnknown = [&](const char *name,
                                 const std::array<IntMatrix, 3> &F,
                                 const std::array<IntRow, 3> &off, unsigned rank,
                                 const IntegerPolyhedron &dom) -> bool {
      std::string why;
      auto f = buildFlattenedReduction(
          F, off, dom, rank,
          systolic::logical::ReductionOrderDecision::PreserveSourceOrder, why);
      std::string b = name;
      if (verify(!f.has_value(), (b + ": UNKNOWN (conservative refusal)").c_str()))
        return false;
      return true;
    };

    // 2-D rectangular reduction: C[i][j] += A[i][k1][k2]*B[k1][k2][j].
    if (!runFlatten("C3c2 2D",
          {IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1)}},
           IntMatrix{{BigInt(0),BigInt(0),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0)}},
           IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0)}}},
          {IntRow{BigInt(0),BigInt(0),BigInt(0)},IntRow{BigInt(0),BigInt(0),BigInt(0)},IntRow{BigInt(0),BigInt(0)}},
          4, box(4,3), 3, 2, 16, false, false, false))
      return 1;

    // 3-D rectangular reduction.
    if (!runFlatten("C3c2 3D",
          {IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(1),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(1)}},
           IntMatrix{{BigInt(0),BigInt(0),BigInt(1),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(1)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0),BigInt(0)}},
           IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0),BigInt(0)}}},
          {IntRow{BigInt(0),BigInt(0),BigInt(0),BigInt(0)},IntRow{BigInt(0),BigInt(0),BigInt(0),BigInt(0)},IntRow{BigInt(0),BigInt(0)}},
          5, box(5,2), 2, 3, 27, false, false, false))
      return 1;

    // Batched GEMM with two reduction dims: C[b][i][j] += A[b][i][k1][k2]*B[b][k1][k2][j].
    if (!runFlatten("C3c2 batched-2reduction",
          {IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(1)}},
           IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(1)},{BigInt(0),BigInt(0),BigInt(1),BigInt(0),BigInt(0)}},
           IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(1),BigInt(0),BigInt(0)}}},
          {IntRow{BigInt(0),BigInt(0),BigInt(0),BigInt(0)},IntRow{BigInt(0),BigInt(0),BigInt(0),BigInt(0)},IntRow{BigInt(0),BigInt(0),BigInt(0)}},
          5, box(5,2), 2, 2, 9, false, false, false))
      return 1;

    // Batched GEMM with three reduction dims: C[b][i][j] += A[b][i][k1][k2][k3]*B[b][k1][k2][k3][j].
    if (!runFlatten("C3c2 batched-3reduction",
          {IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(1)}},
           IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(1)},{BigInt(0),BigInt(0),BigInt(1),BigInt(0),BigInt(0),BigInt(0)}},
           IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(1),BigInt(0),BigInt(0),BigInt(0)}}},
          {IntRow{BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(0)},IntRow{BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(0)},IntRow{BigInt(0),BigInt(0),BigInt(0)}},
          6, box(6,1), 1, 3, 8, false, false, false))
      return 1;

    // Nonzero reduction lower bound: k1 >= 1 over the 2-D reduction.
    {
      IntegerPolyhedron dom = box(4, 3);
      dom.addInequality({0, 0, 1, 0, -1}); // x2 - 1 >= 0  (k1 >= 1)
      if (!runFlatten("C3c2 nonzero-lb",
            {IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1)}},
             IntMatrix{{BigInt(0),BigInt(0),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0)}},
             IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0)}}},
            {IntRow{BigInt(0),BigInt(0),BigInt(0)},IntRow{BigInt(0),BigInt(0),BigInt(0)},IntRow{BigInt(0),BigInt(0)}},
            4, dom, 3, 2, 12, false, false, false))
        return 1;
    }

    // Empty reduction domain: no MACs invented.
    {
      IntegerPolyhedron dom = box(4, 3);
      dom.addInequality({0, 0, 1, 0, -5}); // k1 >= 5 contradicts k1 <= 3
      if (!runFlatten("C3c2 empty",
            {IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1)}},
             IntMatrix{{BigInt(0),BigInt(0),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0)}},
             IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0)}}},
            {IntRow{BigInt(0),BigInt(0),BigInt(0)},IntRow{BigInt(0),BigInt(0),BigInt(0)},IntRow{BigInt(0),BigInt(0)}},
            4, dom, 3, 2, 0, true, false, false))
        return 1;
    }

    // Non-rectangular reduction domain: k1 + k2 <= 3 couples the axes -> UNKNOWN.
    {
      IntegerPolyhedron dom = box(4, 3);
      dom.addInequality({0, 0, -1, -1, 3}); // 3 - k1 - k2 >= 0
      if (!runFlattenUnknown("C3c2 nonrectangular",
            {IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1)}},
             IntMatrix{{BigInt(0),BigInt(0),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0)}},
             IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0)}}},
            {IntRow{BigInt(0),BigInt(0),BigInt(0)},IntRow{BigInt(0),BigInt(0),BigInt(0)},IntRow{BigInt(0),BigInt(0)}},
            4, dom))
        return 1;
    }

    // Congruence/equality-restricted reduction: k1 == k2 -> UNKNOWN.
    {
      IntegerPolyhedron dom = box(4, 3);
      dom.addEquality({0, 0, 1, -1, 0}); // k1 - k2 == 0
      if (!runFlattenUnknown("C3c2 congruence-restricted",
            {IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1)}},
             IntMatrix{{BigInt(0),BigInt(0),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0)}},
             IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0)}}},
            {IntRow{BigInt(0),BigInt(0),BigInt(0)},IntRow{BigInt(0),BigInt(0),BigInt(0)},IntRow{BigInt(0),BigInt(0)}},
            4, dom))
        return 1;
    }

    // Mixed-radix overflow: huge reduction extents -> UNKNOWN.
    {
      const std::int64_t BIG = std::int64_t(1) << 40;
      IntegerPolyhedron dom(PresburgerSpace::getSetSpace(4, 0, 0));
      dom.addInequality({1, 0, 0, 0, 0});
      dom.addInequality({-1, 0, 0, 0, 1}); // i in [0,1]
      dom.addInequality({0, 1, 0, 0, 0});
      dom.addInequality({0, -1, 0, 0, 1}); // j in [0,1]
      dom.addInequality({0, 0, 1, 0, 0});
      dom.addInequality({0, 0, -1, 0, BIG}); // k1 in [0,BIG]
      dom.addInequality({0, 0, 0, 1, 0});
      dom.addInequality({0, 0, 0, -1, BIG}); // k2 in [0,BIG]
      if (!runFlattenUnknown("C3c2 overflow",
            {IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1)}},
             IntMatrix{{BigInt(0),BigInt(0),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0)}},
             IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0)}}},
            {IntRow{BigInt(0),BigInt(0),BigInt(0)},IntRow{BigInt(0),BigInt(0),BigInt(0)},IntRow{BigInt(0),BigInt(0)}},
            4, dom))
        return 1;
    }

    // Output aliasing: 2-D reduction with p in B only (not indexed by C) -> UNKNOWN.
    if (!runFlattenUnknown("C3c2 output-aliasing",
          {IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(1),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1),BigInt(0)}},
           IntMatrix{{BigInt(0),BigInt(0),BigInt(1),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1),BigInt(0)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(1)}},
           IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0),BigInt(0)}}},
          {IntRow{BigInt(0),BigInt(0),BigInt(0)},IntRow{BigInt(0),BigInt(0),BigInt(0),BigInt(0)},IntRow{BigInt(0),BigInt(0)}},
          5, box(5,2)))
      return 1;

    // Single-axis reduction (plain GEMM): not this helper's job -> UNKNOWN.
    if (!runFlattenUnknown("C3c2 single-reduction",
          {IntMatrix{{BigInt(1),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(1)}},
           IntMatrix{{BigInt(0),BigInt(0),BigInt(1)},{BigInt(0),BigInt(1),BigInt(0)}},
           IntMatrix{{BigInt(1),BigInt(0),BigInt(0)},{BigInt(0),BigInt(1),BigInt(0)}}},
          {IntRow{BigInt(0),BigInt(0)},IntRow{BigInt(0),BigInt(0)},IntRow{BigInt(0),BigInt(0)}},
          3, box(3,3)))
      return 1;

    std::cout << "Phase C3c-2 reduction flattening tests passed.\n";
  }

  // --------------------------------------------------------
  // Phase C3c-3 M1 (Option C): exact two-axis non-rectangular reduction by
  // peel-and-tile. buildReductionScanPlan peels the outer reduction axis as an
  // accumulation loop and tiles the inner axis with the existing partitioner.
  // Validated by a self-contained brute-force oracle: the exact 2-D reduction
  // fiber decomposes as (outer in [outerLo, outerHi]) x (inner a stride-g
  // contiguous interval) whose lengths are exactly the plan's constant-K group
  // lengths; every iteration accumulates into one physical C element (output
  // constant along both reduction axes); and the MAC multiset is exact. No tile
  // is emitted. MAC-multiset equivalence is proven here; strict-FP execution
  // order is NOT claimed (orderMatchesSourceForStrictFP stays false).
  // --------------------------------------------------------
  {
    using mlir::presburger::IntegerPolyhedron;
    using mlir::presburger::PresburgerSpace;

    auto box = [](unsigned rank, int hi) {
      IntegerPolyhedron s(PresburgerSpace::getSetSpace(rank, 0, 0));
      for (unsigned a = 0; a < rank; ++a) {
        llvm::SmallVector<std::int64_t> lo(rank + 1, 0), up(rank + 1, 0);
        lo[a] = 1;
        s.addInequality(lo);
        up[a] = -1;
        up[rank] = hi;
        s.addInequality(up);
      }
      return s;
    };
    auto inDomain = [](const IntegerPolyhedron &dom,
                       const std::vector<int> &x) -> bool {
      IntegerPolyhedron P = dom;
      const unsigned cols = P.getNumCols();
      for (std::size_t j = 0; j < x.size(); ++j) {
        llvm::SmallVector<std::int64_t> eq(cols, 0);
        eq[j] = 1;
        eq[cols - 1] = -static_cast<std::int64_t>(x[j]);
        P.addEquality(eq);
      }
      return !P.isIntegerEmpty();
    };

    auto runScan = [&](const char *name, const std::array<IntMatrix, 3> &F,
                       const std::array<IntRow, 3> &off, unsigned rank,
                       const IntegerPolyhedron &dom, int hi, long expOuterLo,
                       long expOuterHi, std::set<long> expInnerLengths,
                       long expStride, bool expEmpty) -> bool {
      std::string why;
      auto plan = buildReductionScanPlan(
          F, off, dom, rank,
          systolic::logical::ReductionOrderDecision::PreserveSourceOrder, why);
      std::string b = name;
      if (verify(plan.has_value(), (b + ": scan plan built").c_str()))
        return false;
      if (verify(plan->empty == expEmpty, (b + ": emptiness").c_str()))
        return false;
      if (verify(plan->outerIsAccumulation &&
                     !plan->orderMatchesSourceForStrictFP,
                 (b + ": accumulation and strict-FP order flags").c_str()))
        return false;

      auto p2 = analyzeRankPartition(F[0], F[1], F[2], rank);
      auto e2 = p2 ? expandRankPartition(*p2) : std::nullopt;
      auto m2 = e2 ? buildPartitionCoordinateMap(*e2) : std::nullopt;
      if (verify(p2 && e2 && m2, (b + ": oracle coordinate map").c_str()))
        return false;
      const unsigned n = rank;
      auto zOf = [&](const std::vector<int> &x) {
        std::vector<std::int64_t> z(n, 0);
        for (unsigned a = 0; a < n; ++a) {
          BigInt acc = 0;
          for (unsigned c = 0; c < n; ++c)
            acc += m2->sourceToLogical[a][c] * BigInt(x[c]);
          z[a] = acc.convert_to<std::int64_t>();
        }
        return z;
      };
      const unsigned oc = plan->outerCol, ic = plan->innerCol;

      // realOut -> outer -> set of inner ; and realOut -> real C address.
      std::map<std::vector<std::int64_t>,
               std::map<std::int64_t, std::set<std::int64_t>>>
          fib;
      std::map<std::vector<std::int64_t>, std::vector<BigInt>> realC;
      long macs = 0;
      bool accOk = true;
      std::vector<int> x(n, 0);
      std::function<void(unsigned)> sweep = [&](unsigned dim) {
        if (dim == n) {
          if (!inDomain(dom, x))
            return;
          auto z = zOf(x);
          std::vector<std::int64_t> ro;
          for (unsigned a = 0; a < n; ++a)
            if (a != oc && a != ic)
              ro.push_back(z[a]);
          std::vector<BigInt> caddr;
          for (std::size_t rr = 0; rr < F[2].size(); ++rr) {
            BigInt v = off[2][rr];
            for (unsigned c = 0; c < n; ++c)
              v += F[2][rr][c] * BigInt(x[c]);
            caddr.push_back(v);
          }
          auto it = realC.find(ro);
          if (it == realC.end())
            realC[ro] = caddr;
          else if (it->second != caddr)
            accOk = false; // real C must not depend on the reduction axes
          fib[ro][z[oc]].insert(z[ic]);
          ++macs;
          return;
        }
        for (x[dim] = 0; x[dim] <= hi; ++x[dim])
          sweep(dim + 1);
      };
      sweep(0);

      if (expEmpty) {
        if (verify(macs == 0, (b + ": empty domain has no MACs").c_str()))
          return false;
        return true;
      }

      bool ok = true;
      long total = 0;
      std::set<long> lensSeen;
      for (auto &kv : fib)
        for (auto &ov : kv.second) {
          ok &= (ov.first >= plan->outerLo && ov.first <= plan->outerHi);
          std::vector<std::int64_t> ks(ov.second.begin(), ov.second.end());
          for (std::size_t t = 1; t < ks.size(); ++t)
            ok &= (ks[t] - ks[t - 1] == plan->innerStride);
          lensSeen.insert((long)ks.size());
          total += (long)ks.size();
        }
      ok &= (total == macs);
      ok &= accOk;
      std::set<long> planLens(plan->innerConstantTileLengths.begin(),
                              plan->innerConstantTileLengths.end());
      ok &= (planLens == lensSeen);
      if (verify(ok, (b + ": peel exact -- outer in range, inner stride-g "
                          "contiguous, grouped by constant-K lengths, single-C "
                          "accumulation, exact MAC multiplicity")
                         .c_str()))
        return false;
      if (verify(plan->outerLo == expOuterLo && plan->outerHi == expOuterHi,
                 (b + ": outer reduction range").c_str()))
        return false;
      if (verify(plan->innerStride == expStride, (b + ": inner stride").c_str()))
        return false;
      if (verify(planLens == expInnerLengths,
                 (b + ": inner constant-K group lengths").c_str()))
        return false;
      return true;
    };

    auto runScanUnknown = [&](const char *name,
                              const std::array<IntMatrix, 3> &F,
                              const std::array<IntRow, 3> &off, unsigned rank,
                              const IntegerPolyhedron &dom) -> bool {
      std::string why;
      auto plan = buildReductionScanPlan(
          F, off, dom, rank,
          systolic::logical::ReductionOrderDecision::PreserveSourceOrder, why);
      std::string b = name;
      if (verify(!plan.has_value(), (b + ": UNKNOWN (conservative refusal)").c_str()))
        return false;
      return true;
    };

    // 2-D access shared by the convex-fiber cases: C[i][j] += A[i][k1][k2]*B[k1][k2][j].
    std::array<IntMatrix, 3> F2 = {
        IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1)}},
        IntMatrix{{BigInt(0),BigInt(0),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0)}},
        IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0)}}};
    std::array<IntRow, 3> O2 = {IntRow{BigInt(0),BigInt(0),BigInt(0)},
                                IntRow{BigInt(0),BigInt(0),BigInt(0)},
                                IntRow{BigInt(0),BigInt(0)}};

    // Triangular: 0 <= k2 <= k1 <= 3  (x2=k1, x3=k2).
    {
      IntegerPolyhedron dom = box(4, 3);
      dom.addInequality({0, 0, 1, -1, 0}); // k1 - k2 >= 0
      if (!runScan("C3c3 triangular", F2, O2, 4, dom, 3, 0, 3, {1, 2, 3, 4}, 1, false))
        return 1;
    }

    // Banded: |k1 - k2| <= 1.
    {
      IntegerPolyhedron dom = box(4, 3);
      dom.addInequality({0, 0, 1, -1, 1});  // k1 - k2 >= -1
      dom.addInequality({0, 0, -1, 1, 1});  // k1 - k2 <= 1
      if (!runScan("C3c3 banded", F2, O2, 4, dom, 3, 0, 3, {2, 3}, 1, false))
        return 1;
    }

    // Trapezoidal: k1 <= k2 <= k1 + 1.
    {
      IntegerPolyhedron dom = box(4, 3);
      dom.addInequality({0, 0, -1, 1, 0}); // k2 - k1 >= 0
      dom.addInequality({0, 0, 1, -1, 1}); // k1 - k2 >= -1  (k2 <= k1+1)
      if (!runScan("C3c3 trapezoidal", F2, O2, 4, dom, 3, 0, 3, {1, 2}, 1, false))
        return 1;
    }

    // Rectangular regression: full box, peeled as outer x inner.
    if (!runScan("C3c3 rectangular", F2, O2, 4, box(4, 3), 3, 0, 3, {4}, 1, false))
      return 1;

    // Nonzero lower bounds: 1 <= k2 <= k1 <= 3 (outer range [1,3]).
    {
      IntegerPolyhedron dom = box(4, 3);
      dom.addInequality({0, 0, 1, -1, 0}); // k2 <= k1
      dom.addInequality({0, 0, 0, 1, -1}); // k2 >= 1
      if (!runScan("C3c3 nonzero-lb", F2, O2, 4, dom, 3, 1, 3, {1, 2, 3}, 1, false))
        return 1;
    }

    // Empty reduction domain.
    {
      IntegerPolyhedron dom = box(4, 3);
      dom.addInequality({0, 0, 1, 0, -5}); // k1 >= 5 contradicts k1 <= 3
      if (!runScan("C3c3 empty", F2, O2, 4, dom, 3, 0, 0, {}, 1, true))
        return 1;
    }

    // Overflow: huge reduction extents -> inner not tileable -> UNKNOWN.
    {
      const std::int64_t BIG = std::int64_t(1) << 40;
      IntegerPolyhedron dom(PresburgerSpace::getSetSpace(4, 0, 0));
      dom.addInequality({1, 0, 0, 0, 0});
      dom.addInequality({-1, 0, 0, 0, 1}); // i in [0,1]
      dom.addInequality({0, 1, 0, 0, 0});
      dom.addInequality({0, -1, 0, 0, 1}); // j in [0,1]
      dom.addInequality({0, 0, 1, 0, 0});
      dom.addInequality({0, 0, -1, 0, BIG}); // k1 in [0,BIG]
      dom.addInequality({0, 0, 0, 1, 0});
      dom.addInequality({0, 0, 0, -1, BIG}); // k2 in [0,BIG]
      if (!runScanUnknown("C3c3 overflow", F2, O2, 4, dom))
        return 1;
    }

    // Former M1 feature boundary: a third reduction axis was UNSUPPORTED by the
    // two-axis M1 scan and returned UNKNOWN. Phase C3c-3 M3 generalizes the peel
    // to r >= 3, so this exact rectangular three-reduction input is now
    // SUPPORTED. The former UNKNOWN expectation is therefore obsolete; the input
    // and its regression coverage are preserved and the expectation is replaced
    // with a positive correctness assertion. The full brute-force oracle
    // verification of this same input (coverage, nested bounds, MAC
    // multiplicity, operand/output addresses, accumulation) is the
    // "C3c3m3 former-M1-boundary" case in the C3c-3 M3 block below.
    {
      std::array<IntMatrix, 3> F3 = {
          IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(1),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(1)}},
          IntMatrix{{BigInt(0),BigInt(0),BigInt(1),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(1)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0),BigInt(0)}},
          IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0),BigInt(0)}}};
      std::array<IntRow, 3> O3 = {IntRow{BigInt(0),BigInt(0),BigInt(0),BigInt(0)},IntRow{BigInt(0),BigInt(0),BigInt(0),BigInt(0)},IntRow{BigInt(0),BigInt(0)}};
      std::string why3;
      auto plan3 = buildReductionScanPlan(
          F3, O3, box(5, 2), 5,
          systolic::logical::ReductionOrderDecision::PreserveSourceOrder, why3);
      if (verify(plan3.has_value() && plan3->reductionRank == 3 &&
                     !plan3->empty,
                 "C3c3 three-reduction: now supported (reduction rank 3)"))
        return 1;
    }

    // Unsupported: a coupled axis in B only, not indexed by C -> output aliasing -> UNKNOWN.
    if (!runScanUnknown("C3c3 output-aliasing",
          {IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(1),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1),BigInt(0)}},
           IntMatrix{{BigInt(0),BigInt(0),BigInt(1),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1),BigInt(0)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(1)}},
           IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0),BigInt(0)}}},
          {IntRow{BigInt(0),BigInt(0),BigInt(0)},IntRow{BigInt(0),BigInt(0),BigInt(0),BigInt(0)},IntRow{BigInt(0),BigInt(0)}},
          5, box(5, 2)))
      return 1;

    std::cout << "Phase C3c-3 M1 reduction scan tests passed.\n";
  }

  // --------------------------------------------------------
  // Phase C3c-3 M2 (Option C): exact output-dependent outer reduction bounds.
  // buildReductionScanPlan now derives tight piecewise-affine outer bounds over
  // the output coordinates (including batch) by exact projection of the inner
  // axis. Validated by a self-contained brute-force oracle: for every output,
  // the plan's outer faces evaluate to the exact [min,max] of the true outer
  // range, the inner is a stride-g contiguous interval grouped by constant-K
  // lengths, every iteration accumulates into one physical C element, and the
  // MAC multiset is exact. No tile is emitted. Strict-FP order not claimed.
  // --------------------------------------------------------
  {
    using mlir::presburger::IntegerPolyhedron;
    using mlir::presburger::PresburgerSpace;
    auto box = [](unsigned rank, int hi) {
      IntegerPolyhedron s(PresburgerSpace::getSetSpace(rank, 0, 0));
      for (unsigned a = 0; a < rank; ++a) {
        llvm::SmallVector<std::int64_t> lo(rank + 1, 0), up(rank + 1, 0);
        lo[a] = 1;
        s.addInequality(lo);
        up[a] = -1;
        up[rank] = hi;
        s.addInequality(up);
      }
      return s;
    };
    auto inDomain = [](const IntegerPolyhedron &dom,
                       const std::vector<int> &x) -> bool {
      IntegerPolyhedron P = dom;
      const unsigned cols = P.getNumCols();
      for (std::size_t j = 0; j < x.size(); ++j) {
        llvm::SmallVector<std::int64_t> eq(cols, 0);
        eq[j] = 1;
        eq[cols - 1] = -static_cast<std::int64_t>(x[j]);
        P.addEquality(eq);
      }
      return !P.isIntegerEmpty();
    };

    auto runScanM2 = [&](const char *name, const std::array<IntMatrix, 3> &F,
                         const std::array<IntRow, 3> &off, unsigned rank,
                         const IntegerPolyhedron &dom, int hi, bool expConstant,
                         std::set<long> expInnerLengths, long expStride,
                         bool expEmpty) -> bool {
      std::string why;
      auto plan = buildReductionScanPlan(
          F, off, dom, rank,
          systolic::logical::ReductionOrderDecision::PreserveSourceOrder, why);
      std::string b = name;
      if (verify(plan.has_value(), (b + ": scan plan built").c_str()))
        return false;
      if (verify(plan->empty == expEmpty, (b + ": emptiness").c_str()))
        return false;
      if (verify(plan->outerRangeIsConstant == expConstant,
                 (b + ": outer-range constancy classification").c_str()))
        return false;
      if (verify(plan->outerIsAccumulation &&
                     !plan->orderMatchesSourceForStrictFP,
                 (b + ": accumulation and strict-FP order flags").c_str()))
        return false;
      if (expEmpty)
        return true;

      auto p2 = analyzeRankPartition(F[0], F[1], F[2], rank);
      auto e2 = p2 ? expandRankPartition(*p2) : std::nullopt;
      auto m2 = e2 ? buildPartitionCoordinateMap(*e2) : std::nullopt;
      if (verify(p2 && e2 && m2, (b + ": oracle coordinate map").c_str()))
        return false;
      const unsigned n = rank;
      auto zOf = [&](const std::vector<int> &x) {
        std::vector<std::int64_t> z(n, 0);
        for (unsigned a = 0; a < n; ++a) {
          BigInt acc = 0;
          for (unsigned c = 0; c < n; ++c)
            acc += m2->sourceToLogical[a][c] * BigInt(x[c]);
          z[a] = acc.convert_to<std::int64_t>();
        }
        return z;
      };
      const unsigned oc = plan->outerCol, ic = plan->innerCol;
      auto faceVal = [&](const systolic::logical::OuterBoundFace &f,
                         const std::vector<std::int64_t> &z) -> long {
        BigInt v = f.constant;
        for (unsigned c = 0; c < n; ++c)
          v += f.coeffs[c] * BigInt(z[c]);
        return v.convert_to<long>();
      };

      std::map<std::vector<std::int64_t>, std::vector<std::int64_t>> repZ;
      std::map<std::vector<std::int64_t>,
               std::map<std::int64_t, std::set<std::int64_t>>>
          fib;
      std::map<std::vector<std::int64_t>, std::vector<BigInt>> realC;
      long macs = 0;
      bool accOk = true;
      std::vector<int> x(n, 0);
      std::function<void(unsigned)> sweep = [&](unsigned dim) {
        if (dim == n) {
          if (!inDomain(dom, x))
            return;
          auto z = zOf(x);
          std::vector<std::int64_t> ro;
          for (unsigned a = 0; a < n; ++a)
            if (a != oc && a != ic)
              ro.push_back(z[a]);
          if (!repZ.count(ro))
            repZ[ro] = z;
          std::vector<BigInt> caddr;
          for (std::size_t rr = 0; rr < F[2].size(); ++rr) {
            BigInt v = off[2][rr];
            for (unsigned c = 0; c < n; ++c)
              v += F[2][rr][c] * BigInt(x[c]);
            caddr.push_back(v);
          }
          auto it = realC.find(ro);
          if (it == realC.end())
            realC[ro] = caddr;
          else if (it->second != caddr)
            accOk = false;
          fib[ro][z[oc]].insert(z[ic]);
          ++macs;
          return;
        }
        for (x[dim] = 0; x[dim] <= hi; ++x[dim])
          sweep(dim + 1);
      };
      sweep(0);

      bool ok = true;
      long total = 0;
      std::set<long> lensSeen;
      for (auto &kv : fib) {
        const auto &z = repZ[kv.first];
        // tight outer bounds: plan faces give exactly the true [min,max]
        long planLo = std::numeric_limits<long>::min(),
             planHi = std::numeric_limits<long>::max();
        for (auto &f : plan->outerLowerFaces)
          planLo = std::max(planLo, faceVal(f, z));
        for (auto &f : plan->outerUpperFaces)
          planHi = std::min(planHi, faceVal(f, z));
        long trueMin = kv.second.begin()->first;
        long trueMax = kv.second.rbegin()->first;
        ok &= (planLo == trueMin && planHi == trueMax);
        for (auto &ov : kv.second) {
          std::vector<std::int64_t> ks(ov.second.begin(), ov.second.end());
          for (std::size_t t = 1; t < ks.size(); ++t)
            ok &= (ks[t] - ks[t - 1] == plan->innerStride);
          lensSeen.insert((long)ks.size());
          total += (long)ks.size();
        }
      }
      ok &= (total == macs);
      ok &= accOk;
      std::set<long> planLens(plan->innerConstantTileLengths.begin(),
                              plan->innerConstantTileLengths.end());
      ok &= (planLens == lensSeen);
      if (verify(ok, (b + ": outer faces give exact per-output [min,max]; inner "
                          "stride-g contiguous grouped by constant-K lengths; "
                          "single-C accumulation; exact MAC multiplicity")
                         .c_str()))
        return false;
      if (verify(planLens == expInnerLengths,
                 (b + ": inner constant-K group lengths").c_str()))
        return false;
      if (verify(plan->innerStride == expStride, (b + ": inner stride").c_str()))
        return false;
      return true;
    };

    auto runScanM2Unknown = [&](const char *name,
                                const std::array<IntMatrix, 3> &F,
                                const std::array<IntRow, 3> &off, unsigned rank,
                                const IntegerPolyhedron &dom) -> bool {
      std::string why;
      auto plan = buildReductionScanPlan(
          F, off, dom, rank,
          systolic::logical::ReductionOrderDecision::PreserveSourceOrder, why);
      std::string b = name;
      if (verify(!plan.has_value(), (b + ": UNKNOWN (conservative refusal)").c_str()))
        return false;
      return true;
    };

    // Batched 2-axis access: C[b][i][j] += A[b][i][k1][k2]*B[b][k1][k2][j];
    // source x = (b,i,j,k1,k2).
    std::array<IntMatrix, 3> FB = {
        IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(1)}},
        IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(1)},{BigInt(0),BigInt(0),BigInt(1),BigInt(0),BigInt(0)}},
        IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(1),BigInt(0),BigInt(0)}}};
    std::array<IntRow, 3> OB = {IntRow{BigInt(0),BigInt(0),BigInt(0),BigInt(0)},
                                IntRow{BigInt(0),BigInt(0),BigInt(0),BigInt(0)},
                                IntRow{BigInt(0),BigInt(0),BigInt(0)}};

    // Batch-dependent triangular: k1 <= b, k2 <= k1.
    {
      IntegerPolyhedron dom = box(5, 3);
      dom.addInequality({1, 0, 0, -1, 0, 0}); // b - k1 >= 0
      dom.addInequality({0, 0, 0, 1, -1, 0}); // k1 - k2 >= 0
      if (!runScanM2("C3c3m2 batch-triangular", FB, OB, 5, dom, 3, false, {1, 2, 3, 4}, 1, false))
        return 1;
    }
    // Batch-dependent banded: k1 <= b, |k1 - k2| <= 1.
    {
      IntegerPolyhedron dom = box(5, 3);
      dom.addInequality({1, 0, 0, -1, 0, 0});  // k1 <= b
      dom.addInequality({0, 0, 0, 1, -1, 1});  // k1 - k2 >= -1
      dom.addInequality({0, 0, 0, -1, 1, 1});  // k1 - k2 <= 1
      if (!runScanM2("C3c3m2 batch-banded", FB, OB, 5, dom, 3, false, {2, 3}, 1, false))
        return 1;
    }
    // Batch-dependent trapezoidal: k1 <= b, k1 <= k2 <= k1 + 1.
    {
      IntegerPolyhedron dom = box(5, 3);
      dom.addInequality({1, 0, 0, -1, 0, 0});  // k1 <= b
      dom.addInequality({0, 0, 0, -1, 1, 0});  // k2 - k1 >= 0
      dom.addInequality({0, 0, 0, 1, -1, 1});  // k2 <= k1 + 1
      if (!runScanM2("C3c3m2 batch-trapezoidal", FB, OB, 5, dom, 3, false, {1, 2}, 1, false))
        return 1;
    }

    // 2-axis access without batch: C[i][j] += A[i][k1][k2]*B[k1][k2][j];
    // source x = (i,j,k1,k2).
    std::array<IntMatrix, 3> FN = {
        IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1)}},
        IntMatrix{{BigInt(0),BigInt(0),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0)}},
        IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0)}}};
    std::array<IntRow, 3> ON = {IntRow{BigInt(0),BigInt(0),BigInt(0)},
                                IntRow{BigInt(0),BigInt(0),BigInt(0)},
                                IntRow{BigInt(0),BigInt(0)}};

    // Output-dependent on i: k1 <= i, k2 <= k1.
    {
      IntegerPolyhedron dom = box(4, 3);
      dom.addInequality({1, 0, -1, 0, 0}); // i - k1 >= 0
      dom.addInequality({0, 0, 1, -1, 0}); // k1 - k2 >= 0
      if (!runScanM2("C3c3m2 output-dependent-i", FN, ON, 4, dom, 3, false, {1, 2, 3, 4}, 1, false))
        return 1;
    }
    // Constant-bound M1 regression: plain triangular, outer constant [0,3].
    {
      IntegerPolyhedron dom = box(4, 3);
      dom.addInequality({0, 0, 1, -1, 0}); // k2 <= k1
      if (!runScanM2("C3c3m2 constant-triangular", FN, ON, 4, dom, 3, true, {1, 2, 3, 4}, 1, false))
        return 1;
    }
    // Nonzero lower bound, output-dependent: 1 <= k1 <= i, k2 <= k1.
    {
      IntegerPolyhedron dom = box(4, 3);
      dom.addInequality({0, 0, 1, 0, -1}); // k1 >= 1
      dom.addInequality({1, 0, -1, 0, 0}); // k1 <= i
      dom.addInequality({0, 0, 1, -1, 0}); // k2 <= k1
      if (!runScanM2("C3c3m2 nonzero-lb", FN, ON, 4, dom, 3, false, {2, 3, 4}, 1, false))
        return 1;
    }
    // Empty.
    {
      IntegerPolyhedron dom = box(4, 3);
      dom.addInequality({0, 0, 1, 0, -5}); // k1 >= 5 contradicts k1 <= 3
      if (!runScanM2("C3c3m2 empty", FN, ON, 4, dom, 3, true, {}, 1, true))
        return 1;
    }

    // Non-unit outer coefficient: 2*k1 <= b -> UNKNOWN.
    {
      IntegerPolyhedron dom = box(5, 3);
      dom.addInequality({1, 0, 0, -2, 0, 0}); // b - 2*k1 >= 0
      dom.addInequality({0, 0, 0, 1, -1, 0}); // k2 <= k1
      if (!runScanM2Unknown("C3c3m2 non-unit-coefficient", FB, OB, 5, dom))
        return 1;
    }
    // Output aliasing: p in B only, not indexed by C -> UNKNOWN.
    if (!runScanM2Unknown("C3c3m2 output-aliasing",
          {IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(1),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1),BigInt(0)}},
           IntMatrix{{BigInt(0),BigInt(0),BigInt(1),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1),BigInt(0)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(1)}},
           IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0),BigInt(0)}}},
          {IntRow{BigInt(0),BigInt(0),BigInt(0)},IntRow{BigInt(0),BigInt(0),BigInt(0),BigInt(0)},IntRow{BigInt(0),BigInt(0)}},
          5, box(5, 2)))
      return 1;
    // Integer overflow: huge reduction extents -> UNKNOWN.
    {
      const std::int64_t BIG = std::int64_t(1) << 40;
      IntegerPolyhedron dom(PresburgerSpace::getSetSpace(5, 0, 0));
      dom.addInequality({1, 0, 0, 0, 0, 0});
      dom.addInequality({-1, 0, 0, 0, 0, 3});   // b in [0,3]
      dom.addInequality({0, 1, 0, 0, 0, 0});
      dom.addInequality({0, -1, 0, 0, 0, 1});   // i in [0,1]
      dom.addInequality({0, 0, 1, 0, 0, 0});
      dom.addInequality({0, 0, -1, 0, 0, 1});   // j in [0,1]
      dom.addInequality({0, 0, 0, 1, 0, 0});
      dom.addInequality({0, 0, 0, -1, 0, BIG}); // k1 in [0,BIG]
      dom.addInequality({0, 0, 0, 0, 1, 0});
      dom.addInequality({0, 0, 0, 0, -1, BIG}); // k2 in [0,BIG]
      if (!runScanM2Unknown("C3c3m2 overflow", FB, OB, 5, dom))
        return 1;
    }

    std::cout << "Phase C3c-3 M2 reduction scan tests passed.\n";
  }

  // --------------------------------------------------------
  // Phase C3c-3 M3 (Option C): exact peel-and-tile for three (or more) genuine
  // reduction dimensions over convex Presburger domains. buildReductionScanPlan
  // peels r-1 nested outer reduction axes (each with piecewise-affine bounds
  // over the output and the enclosing outer axes) and tiles the innermost.
  // Validated by a self-contained brute-force oracle: at every nesting level
  // the plan's faces evaluate to the exact [min,max] of that axis given the
  // enclosing coordinates; the innermost axis is a stride-g contiguous interval
  // grouped by constant-K lengths; each MAC is covered exactly once and
  // accumulates into one physical C element. No tile is emitted. Strict-FP
  // order not claimed.
  // --------------------------------------------------------
  {
    using mlir::presburger::IntegerPolyhedron;
    using mlir::presburger::PresburgerSpace;
    auto box = [](unsigned rank, int hi) {
      IntegerPolyhedron s(PresburgerSpace::getSetSpace(rank, 0, 0));
      for (unsigned a = 0; a < rank; ++a) {
        llvm::SmallVector<std::int64_t> lo(rank + 1, 0), up(rank + 1, 0);
        lo[a] = 1;
        s.addInequality(lo);
        up[a] = -1;
        up[rank] = hi;
        s.addInequality(up);
      }
      return s;
    };
    auto inDomain = [](const IntegerPolyhedron &dom,
                       const std::vector<int> &x) -> bool {
      IntegerPolyhedron P = dom;
      const unsigned cols = P.getNumCols();
      for (std::size_t j = 0; j < x.size(); ++j) {
        llvm::SmallVector<std::int64_t> eq(cols, 0);
        eq[j] = 1;
        eq[cols - 1] = -static_cast<std::int64_t>(x[j]);
        P.addEquality(eq);
      }
      return !P.isIntegerEmpty();
    };

    auto runScanM3 = [&](const char *name, const std::array<IntMatrix, 3> &F,
                         const std::array<IntRow, 3> &off, unsigned rank,
                         const IntegerPolyhedron &dom, int hi, unsigned expR,
                         std::set<long> expInnerLengths, bool expConstant,
                         bool expEmpty) -> bool {
      std::string why;
      auto plan = buildReductionScanPlan(
          F, off, dom, rank,
          systolic::logical::ReductionOrderDecision::PreserveSourceOrder, why);
      std::string b = name;
      if (verify(plan.has_value(), (b + ": scan plan built").c_str()))
        return false;
      if (verify(plan->reductionRank == expR, (b + ": reduction rank").c_str()))
        return false;
      if (verify(plan->empty == expEmpty, (b + ": emptiness").c_str()))
        return false;
      if (verify(plan->outerIsAccumulation &&
                     !plan->orderMatchesSourceForStrictFP,
                 (b + ": accumulation and strict-FP order flags").c_str()))
        return false;
      if (expEmpty)
        return true;
      if (verify(plan->outerRangeIsConstant == expConstant,
                 (b + ": outer-range constancy").c_str()))
        return false;
      if (verify(plan->peelLevels.size() == expR - 1,
                 (b + ": peel-level count").c_str()))
        return false;

      auto p2 = analyzeRankPartition(F[0], F[1], F[2], rank);
      auto e2 = p2 ? expandRankPartition(*p2) : std::nullopt;
      auto m2 = e2 ? buildPartitionCoordinateMap(*e2) : std::nullopt;
      if (verify(p2 && e2 && m2, (b + ": oracle coordinate map").c_str()))
        return false;
      const unsigned n = rank;
      auto zOf = [&](const std::vector<int> &x) {
        std::vector<std::int64_t> z(n, 0);
        for (unsigned a = 0; a < n; ++a) {
          BigInt acc = 0;
          for (unsigned c = 0; c < n; ++c)
            acc += m2->sourceToLogical[a][c] * BigInt(x[c]);
          z[a] = acc.convert_to<std::int64_t>();
        }
        return z;
      };
      auto faceVal = [&](const systolic::logical::OuterBoundFace &f,
                         const std::vector<std::int64_t> &z) -> long {
        BigInt v = f.constant;
        for (unsigned c = 0; c < n; ++c)
          v += f.coeffs[c] * BigInt(z[c]);
        return v.convert_to<long>();
      };
      // reduction columns in peel order, innermost last.
      std::vector<unsigned> rc;
      for (const auto &lvl : plan->peelLevels)
        rc.push_back(lvl.col);
      rc.push_back(plan->innerCol);
      std::vector<bool> isRed(n, false);
      for (unsigned c : rc)
        isRed[c] = true;

      struct Pt {
        std::vector<std::int64_t> o;
        std::vector<std::int64_t> rt; // reduction values in peel order + inner
        std::vector<std::int64_t> z;
      };
      std::vector<Pt> pts;
      std::map<std::vector<std::int64_t>, std::vector<BigInt>> realC;
      std::set<std::pair<std::vector<std::int64_t>, std::vector<std::int64_t>>> seen;
      bool accOk = true, dupOk = true;
      std::vector<int> x(n, 0);
      std::function<void(unsigned)> sweep = [&](unsigned dim) {
        if (dim == n) {
          if (!inDomain(dom, x))
            return;
          auto z = zOf(x);
          Pt pt;
          pt.z = z;
          for (unsigned a = 0; a < n; ++a)
            if (!isRed[a])
              pt.o.push_back(z[a]);
          for (unsigned c : rc)
            pt.rt.push_back(z[c]);
          std::vector<BigInt> caddr;
          for (std::size_t r = 0; r < F[2].size(); ++r) {
            BigInt v = off[2][r];
            for (unsigned c = 0; c < n; ++c)
              v += F[2][r][c] * BigInt(x[c]);
            caddr.push_back(v);
          }
          auto it = realC.find(pt.o);
          if (it == realC.end())
            realC[pt.o] = caddr;
          else if (it->second != caddr)
            accOk = false;
          if (!seen.insert({pt.o, pt.rt}).second)
            dupOk = false;
          pts.push_back(std::move(pt));
          return;
        }
        for (x[dim] = 0; x[dim] <= hi; ++x[dim])
          sweep(dim + 1);
      };
      sweep(0);

      bool ok = accOk && dupOk;
      // Per-level nested bounds: for each level p, grouping by the output and
      // the more-significant reduction values, the faces give exact [min,max].
      for (unsigned p = 0; p + 1 < expR && ok; ++p) {
        std::map<std::vector<std::int64_t>,
                 std::pair<std::pair<long, long>, std::vector<std::int64_t>>>
            grp;
        for (const auto &pt : pts) {
          std::vector<std::int64_t> key = pt.o;
          for (unsigned q = 0; q < p; ++q)
            key.push_back(pt.rt[q]);
          long v = pt.rt[p];
          auto it = grp.find(key);
          if (it == grp.end())
            grp[key] = {{v, v}, pt.z};
          else {
            it->second.first.first = std::min(it->second.first.first, v);
            it->second.first.second = std::max(it->second.first.second, v);
          }
        }
        for (auto &kv : grp) {
          long planLo = std::numeric_limits<long>::min(),
               planHi = std::numeric_limits<long>::max();
          for (auto &f : plan->peelLevels[p].lowerFaces)
            planLo = std::max(planLo, faceVal(f, kv.second.second));
          for (auto &f : plan->peelLevels[p].upperFaces)
            planHi = std::min(planHi, faceVal(f, kv.second.second));
          ok &= (planLo == kv.second.first.first &&
                 planHi == kv.second.first.second);
        }
      }
      // Innermost: stride-g contiguous grouped by constant-K lengths.
      std::set<long> lensSeen;
      {
        std::map<std::vector<std::int64_t>, std::set<std::int64_t>> gi;
        for (const auto &pt : pts) {
          std::vector<std::int64_t> key = pt.o;
          for (unsigned q = 0; q + 1 < expR; ++q)
            key.push_back(pt.rt[q]);
          gi[key].insert(pt.rt[expR - 1]);
        }
        for (auto &kv : gi) {
          std::vector<std::int64_t> ks(kv.second.begin(), kv.second.end());
          for (std::size_t t = 1; t < ks.size(); ++t)
            ok &= (ks[t] - ks[t - 1] == plan->innerStride);
          lensSeen.insert((long)ks.size());
        }
      }
      std::set<long> planLens(plan->innerConstantTileLengths.begin(),
                              plan->innerConstantTileLengths.end());
      ok &= (planLens == lensSeen);
      if (verify(ok, (b + ": nested per-level faces give exact [min,max]; inner "
                          "stride-g contiguous grouped by constant-K lengths; "
                          "single-C accumulation; exact MAC multiplicity")
                         .c_str()))
        return false;
      if (verify(planLens == expInnerLengths,
                 (b + ": inner constant-K group lengths").c_str()))
        return false;
      return true;
    };

    auto runScanM3Unknown = [&](const char *name,
                                const std::array<IntMatrix, 3> &F,
                                const std::array<IntRow, 3> &off, unsigned rank,
                                const IntegerPolyhedron &dom) -> bool {
      std::string why;
      auto plan = buildReductionScanPlan(
          F, off, dom, rank,
          systolic::logical::ReductionOrderDecision::PreserveSourceOrder, why);
      std::string b = name;
      if (verify(!plan.has_value(), (b + ": UNKNOWN (conservative refusal)").c_str()))
        return false;
      return true;
    };

    // 3-reduction access without batch: C[i][j] += A[i][k1][k2][k3]*B[k1][k2][k3][j];
    // source x = (i,j,k1,k2,k3).
    std::array<IntMatrix, 3> F3 = {
        IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(1),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(1)}},
        IntMatrix{{BigInt(0),BigInt(0),BigInt(1),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(1)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0),BigInt(0)}},
        IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0),BigInt(0)}}};
    std::array<IntRow, 3> O3 = {IntRow{BigInt(0),BigInt(0),BigInt(0),BigInt(0)},
                                IntRow{BigInt(0),BigInt(0),BigInt(0),BigInt(0)},
                                IntRow{BigInt(0),BigInt(0)}};

    // 3-D simplex: 0 <= k3 <= k2 <= k1 <= 3.
    {
      IntegerPolyhedron dom = box(5, 3);
      dom.addInequality({0, 0, 1, -1, 0, 0}); // k1 - k2 >= 0
      dom.addInequality({0, 0, 0, 1, -1, 0}); // k2 - k3 >= 0
      if (!runScanM3("C3c3m3 simplex", F3, O3, 5, dom, 3, 3, {1, 2, 3, 4}, false, false))
        return 1;
    }
    // 3-D triangular: k2 <= k1, k3 free (rectangular in k3).
    {
      IntegerPolyhedron dom = box(5, 3);
      dom.addInequality({0, 0, 1, -1, 0, 0}); // k1 - k2 >= 0
      if (!runScanM3("C3c3m3 triangular", F3, O3, 5, dom, 3, 3, {4}, false, false))
        return 1;
    }
    // Rectangular 3-D regression (box only): all levels constant.
    if (!runScanM3("C3c3m3 rectangular", F3, O3, 5, box(5, 3), 3, 3, {4}, true, false))
      return 1;
    // The exact former-M1-boundary input (rectangular three-reduction over
    // box extent 2), now fully oracle-verified as supported instead of UNKNOWN.
    if (!runScanM3("C3c3m3 former-M1-boundary", F3, O3, 5, box(5, 2), 2, 3, {3}, true, false))
      return 1;

    // Batched GEMM with three reduction dims: C[b][i][j] += A[b][i][k1][k2][k3]*
    // B[b][k1][k2][k3][j]; source x = (b,i,j,k1,k2,k3).
    {
      std::array<IntMatrix, 3> FB3 = {
          IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(1)}},
          IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(1)},{BigInt(0),BigInt(0),BigInt(1),BigInt(0),BigInt(0),BigInt(0)}},
          IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(1),BigInt(0),BigInt(0),BigInt(0)}}};
      std::array<IntRow, 3> OB3 = {IntRow{BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(0)},IntRow{BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(0)},IntRow{BigInt(0),BigInt(0),BigInt(0)}};
      if (!runScanM3("C3c3m3 batched-3reduction", FB3, OB3, 6, box(6, 1), 1, 3, {2}, true, false))
        return 1;
    }

    // Empty reduction domain.
    {
      IntegerPolyhedron dom = box(5, 3);
      dom.addInequality({0, 0, 1, 0, 0, -5}); // k1 >= 5 contradicts k1 <= 3
      if (!runScanM3("C3c3m3 empty", F3, O3, 5, dom, 3, 3, {}, true, true))
        return 1;
    }

    // Overflow: huge reduction extents -> UNKNOWN.
    {
      const std::int64_t BIG = std::int64_t(1) << 40;
      IntegerPolyhedron dom(PresburgerSpace::getSetSpace(5, 0, 0));
      dom.addInequality({1, 0, 0, 0, 0, 0});
      dom.addInequality({-1, 0, 0, 0, 0, 1}); // i in [0,1]
      dom.addInequality({0, 1, 0, 0, 0, 0});
      dom.addInequality({0, -1, 0, 0, 0, 1}); // j in [0,1]
      dom.addInequality({0, 0, 1, 0, 0, 0});
      dom.addInequality({0, 0, -1, 0, 0, BIG}); // k1 in [0,BIG]
      dom.addInequality({0, 0, 0, 1, 0, 0});
      dom.addInequality({0, 0, 0, -1, 0, BIG}); // k2 in [0,BIG]
      dom.addInequality({0, 0, 0, 0, 1, 0});
      dom.addInequality({0, 0, 0, 0, -1, BIG}); // k3 in [0,BIG]
      if (!runScanM3Unknown("C3c3m3 overflow", F3, O3, 5, dom))
        return 1;
    }
    // Output aliasing: p in B only, not indexed by C -> UNKNOWN. Source
    // x = (i,j,k1,k2,k3,p); A[i][k1][k2][k3], B[k1][k2][k3][j][p], C[i][j].
    if (!runScanM3Unknown("C3c3m3 output-aliasing",
          {IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(1),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(1),BigInt(0)}},
           IntMatrix{{BigInt(0),BigInt(0),BigInt(1),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(1),BigInt(0)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(1)}},
           IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0),BigInt(0),BigInt(0)}}},
          {IntRow{BigInt(0),BigInt(0),BigInt(0),BigInt(0)},IntRow{BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(0)},IntRow{BigInt(0),BigInt(0)}},
          6, box(6, 1)))
      return 1;
    // Congruence/coupling-restricted reduction: an equality couples two
    // reduction axes (k1 == k2) -> UNKNOWN. (Genuine lattice congruence on a
    // reduction axis is normalized to a primitive basis by the recognizer, so
    // it does not arise as such; a coupled/equality-restricted reduction is the
    // representable restricted case and is refused.)
    {
      std::array<IntMatrix, 3> FC = {
          IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1)}},
          IntMatrix{{BigInt(0),BigInt(0),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0)}},
          IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0)}}};
      std::array<IntRow, 3> OC = {IntRow{BigInt(0),BigInt(0),BigInt(0)},IntRow{BigInt(0),BigInt(0),BigInt(0)},IntRow{BigInt(0),BigInt(0)}};
      IntegerPolyhedron dom = box(4, 3);
      dom.addEquality({0, 0, 1, -1, 0}); // k1 - k2 == 0
      if (!runScanM3Unknown("C3c3m3 coupled-reduction", FC, OC, 4, dom))
        return 1;
    }

    std::cout << "Phase C3c-3 M3 reduction scan tests passed.\n";
  }

  // --------------------------------------------------------
  // Phase C3c-3 M4-box (Option C): CONGRUENCE-RESTRICTED multi-dimensional
  // reductions. A genuine reduction of rank >= 2 may carry exactly one
  // congruence-coupled axis (a stride-g, sublattice-punctured fiber, as in
  // CASE 1 / det-9); buildReductionScanPlan routes that axis to the innermost
  // tiled position (C3's stride-g partitioner discharges the congruence) and
  // peels the remaining congruence-free axes with constant, independent ranges.
  //
  // Each accepted case is validated by an independent brute-force oracle over
  // the bounded source domain (enumeration lives in the TEST only; production
  // is symbolic). The oracle establishes D_generated == D_original as a MULTISET
  // of source MACs -- not merely equal counts -- by running the proven C3
  // coverage oracle on the SYNTHETIC inner problem (the plan's peeled axes
  // appended to C): every source MAC is a stride-g fiber point under a UNIQUE
  // min-anchor of the correct length (coverage + multiplicity), operand/output
  // addresses advance by the reported t-stride, anchor pieces are disjoint, and
  // the generated MAC count equals the enumerated source count. Independently,
  // in the original frame, the constant peel ranges equal the enumerated axis
  // extents and every genuine reduction (coupled AND clean) accumulates into one
  // physical C element. Because MAC emission is anchor-gated, a convex-hull
  // outer coordinate with an EMPTY inner fiber contributes zero MACs -- shown
  // directly by the empty-fiber case. Strict-FP order is not claimed.
  // --------------------------------------------------------
  {
    using mlir::presburger::IntegerPolyhedron;
    using mlir::presburger::PresburgerSpace;
    auto box = [](unsigned rank, int hi) {
      IntegerPolyhedron s(PresburgerSpace::getSetSpace(rank, 0, 0));
      for (unsigned a = 0; a < rank; ++a) {
        llvm::SmallVector<std::int64_t> lo(rank + 1, 0), up(rank + 1, 0);
        lo[a] = 1;
        s.addInequality(lo);
        up[a] = -1;
        up[rank] = hi;
        s.addInequality(up);
      }
      return s;
    };
    auto inDomain = [](const IntegerPolyhedron &dom,
                       const std::vector<int> &x) -> bool {
      IntegerPolyhedron P = dom;
      const unsigned cols = P.getNumCols();
      for (std::size_t j = 0; j < x.size(); ++j) {
        llvm::SmallVector<std::int64_t> eq(cols, 0);
        eq[j] = 1;
        eq[cols - 1] = -static_cast<std::int64_t>(x[j]);
        P.addEquality(eq);
      }
      return !P.isIntegerEmpty();
    };
    auto inGroup = [](const systolic::logical::TileFiberGroup &grp,
                      const std::vector<std::int64_t> &za) -> bool {
      for (const auto &piece : grp.anchorPieces) {
        IntegerPolyhedron P = piece;
        const unsigned cols = P.getNumCols();
        for (std::size_t j = 0; j < za.size(); ++j) {
          llvm::SmallVector<std::int64_t> eq(cols, 0);
          eq[j] = 1;
          eq[cols - 1] = -za[j];
          P.addEquality(eq);
        }
        if (!P.isIntegerEmpty())
          return true;
      }
      return false;
    };

    // Comprehensive M4 oracle. expR: reduction rank; expStride: inner fiber
    // stride g; expInnerLengths: constant-K group lengths; expEmpty: empty plan.
    // wantEmptyFiberWitness: assert the convex envelope contains >= 1
    // (output,clean) combo with an EMPTY coupled fiber (so the generated set is
    // strictly anchor-gated, never the envelope product).
    auto runScanM4 = [&](const char *name, const std::array<IntMatrix, 3> &F,
                         const std::array<IntRow, 3> &off, unsigned rank,
                         const IntegerPolyhedron &dom, int hi, unsigned expR,
                         std::int64_t expStride, std::set<long> expInnerLengths,
                         bool expEmpty, bool wantEmptyFiberWitness) -> bool {
      std::string why;
      auto plan = buildReductionScanPlan(
          F, off, dom, rank,
          systolic::logical::ReductionOrderDecision::PreserveSourceOrder, why);
      std::string b = name;
      if (verify(plan.has_value(), (b + ": scan plan built").c_str()))
        return false;
      if (verify(plan->reductionRank == expR, (b + ": reduction rank").c_str()))
        return false;
      if (verify(plan->empty == expEmpty, (b + ": emptiness").c_str()))
        return false;
      if (verify(plan->outerIsAccumulation &&
                     !plan->orderMatchesSourceForStrictFP,
                 (b + ": accumulation and strict-FP order flags").c_str()))
        return false;
      if (expEmpty)
        return true;
      // The outerRangeIsConstant flag must agree with the actual peel faces
      // (constant iff every face has zero coefficients) -- it may be false for
      // triangular/simplex/banded/trapezoidal clean dependencies.
      {
        bool allConst = true;
        for (const auto &lvl : plan->peelLevels) {
          for (const auto &f : lvl.lowerFaces)
            for (const BigInt &c : f.coeffs)
              if (c != 0) allConst = false;
          for (const auto &f : lvl.upperFaces)
            for (const BigInt &c : f.coeffs)
              if (c != 0) allConst = false;
        }
        if (verify(plan->outerRangeIsConstant == allConst,
                   (b + ": outer-range constancy flag consistent").c_str()))
          return false;
      }
      if (verify(plan->peelLevels.size() == expR - 1,
                 (b + ": peel-level count").c_str()))
        return false;
      if (verify((long)plan->innerStride == (long)expStride,
                 (b + ": inner stride g").c_str()))
        return false;
      std::set<long> planLens(plan->innerConstantTileLengths.begin(),
                              plan->innerConstantTileLengths.end());
      if (verify(planLens == expInnerLengths,
                 (b + ": inner constant-K group lengths").c_str()))
        return false;

      // Original-frame coordinate map + enumeration.
      auto p2 = analyzeRankPartition(F[0], F[1], F[2], rank);
      auto e2 = p2 ? expandRankPartition(*p2) : std::nullopt;
      auto m2 = e2 ? buildPartitionCoordinateMap(*e2) : std::nullopt;
      if (verify(p2 && e2 && m2, (b + ": oracle coordinate map").c_str()))
        return false;
      const unsigned n = rank;
      auto zOf = [&](const std::vector<int> &x) {
        std::vector<std::int64_t> z(n, 0);
        for (unsigned a = 0; a < n; ++a) {
          BigInt acc = 0;
          for (unsigned c = 0; c < n; ++c)
            acc += m2->sourceToLogical[a][c] * BigInt(x[c]);
          z[a] = acc.convert_to<std::int64_t>();
        }
        return z;
      };
      std::vector<unsigned> peel;
      for (const auto &l : plan->peelLevels)
        peel.push_back(l.col);
      const unsigned inner = plan->innerCol;
      std::vector<bool> isRed(n, false);
      for (unsigned c : peel)
        isRed[c] = true;
      isRed[inner] = true;

      // Accumulation (same original output -> same real C address) and the full
      // set of enumerated logical points (for the exact nested peel-bound check).
      auto faceVal = [&](const systolic::logical::OuterBoundFace &f,
                         const std::vector<std::int64_t> &z) -> long {
        BigInt v = f.constant;
        for (unsigned c = 0; c < n; ++c)
          v += f.coeffs[c] * BigInt(z[c]);
        return v.convert_to<long>();
      };
      std::map<std::vector<std::int64_t>, std::vector<BigInt>> realC;
      std::vector<std::vector<std::int64_t>> zpts;
      bool accOk = true;
      long srcMacs = 0;
      std::vector<int> x(n, 0);
      std::function<void(unsigned)> sweep = [&](unsigned d) {
        if (d == n) {
          if (!inDomain(dom, x))
            return;
          auto z = zOf(x);
          ++srcMacs;
          std::vector<std::int64_t> o;
          for (unsigned a = 0; a < n; ++a)
            if (!isRed[a])
              o.push_back(z[a]);
          std::vector<BigInt> c;
          for (std::size_t r = 0; r < F[2].size(); ++r) {
            BigInt v = off[2][r];
            for (unsigned cc = 0; cc < n; ++cc)
              v += F[2][r][cc] * BigInt(x[cc]);
            c.push_back(v);
          }
          auto it = realC.find(o);
          if (it == realC.end())
            realC[o] = c;
          else if (it->second != c)
            accOk = false;
          zpts.push_back(z);
          return;
        }
        for (x[d] = 0; x[d] <= hi; ++x[d])
          sweep(d + 1);
      };
      sweep(0);
      if (verify(accOk, (b + ": every genuine reduction (coupled and clean) "
                             "accumulates into one physical C element")
                            .c_str()))
        return false;
      // Exact nested peel bounds: at each level, grouping by the output and the
      // enclosing (more-significant) peel values, the plan's faces evaluate to
      // the enumerated [min,max] of that axis -- affine (triangular/simplex/
      // banded) or constant.
      bool peelOk = true;
      for (std::size_t p = 0; p < peel.size() && peelOk; ++p) {
        std::map<std::vector<std::int64_t>,
                 std::pair<std::pair<long, long>, std::vector<std::int64_t>>>
            grp;
        for (const auto &z : zpts) {
          std::vector<std::int64_t> key;
          for (unsigned a = 0; a < n; ++a)
            if (!isRed[a])
              key.push_back(z[a]);
          for (std::size_t q = 0; q < p; ++q)
            key.push_back(z[peel[q]]);
          long v = z[peel[p]];
          auto it = grp.find(key);
          if (it == grp.end())
            grp[key] = {{v, v}, z};
          else {
            it->second.first.first = std::min(it->second.first.first, v);
            it->second.first.second = std::max(it->second.first.second, v);
          }
        }
        for (auto &kv : grp) {
          long planLo = std::numeric_limits<long>::min(),
               planHi = std::numeric_limits<long>::max();
          for (auto &f : plan->peelLevels[p].lowerFaces)
            planLo = std::max(planLo, faceVal(f, kv.second.second));
          for (auto &f : plan->peelLevels[p].upperFaces)
            planHi = std::min(planHi, faceVal(f, kv.second.second));
          peelOk &= (planLo == kv.second.first.first &&
                     planHi == kv.second.first.second);
        }
      }
      if (verify(peelOk,
                 (b + ": exact nested peel bounds equal enumerated extents")
                     .c_str()))
        return false;

      // Inner MAC exactness via the proven C3 coverage oracle on the SYNTHETIC
      // problem (peeled axes appended to C). This is D_generated == D_original.
      std::array<IntMatrix, 3> sF = F;
      std::array<IntRow, 3> sOff = off;
      for (unsigned c : peel) {
        sF[2].push_back(m2->sourceToLogical[c]);
        sOff[2].push_back(BigInt(0));
      }
      std::string sWhy;
      auto part = buildSymbolicTilePartition(
          sF, sOff, dom, rank,
          systolic::logical::ReductionOrderDecision::PreserveSourceOrder, sWhy);
      if (verify(part.has_value(),
                 (b + ": synthetic inner partition built").c_str()))
        return false;
      if (verify((long)part->stride == (long)expStride,
                 (b + ": synthetic inner stride").c_str()))
        return false;
      auto sp2 = analyzeRankPartition(sF[0], sF[1], sF[2], rank);
      auto se2 = sp2 ? expandRankPartition(*sp2) : std::nullopt;
      auto sm2 = se2 ? buildPartitionCoordinateMap(*se2) : std::nullopt;
      if (verify(sp2 && se2 && sm2,
                 (b + ": synthetic coordinate map").c_str()))
        return false;
      const unsigned kIdx = part->kIndex;
      auto zOfS = [&](const std::vector<int> &xx) {
        std::vector<std::int64_t> z(n, 0);
        for (unsigned a = 0; a < n; ++a) {
          BigInt acc = 0;
          for (unsigned c = 0; c < n; ++c)
            acc += sm2->sourceToLogical[a][c] * BigInt(xx[c]);
          z[a] = acc.convert_to<std::int64_t>();
        }
        return z;
      };
      auto okeyOfS = [&](const std::vector<std::int64_t> &z) {
        std::vector<std::int64_t> o;
        for (unsigned a = 0; a < n; ++a)
          if (a != kIdx)
            o.push_back(z[a]);
        return o;
      };
      auto zFromOkeyS = [&](const std::vector<std::int64_t> &o,
                            std::int64_t k) {
        std::vector<std::int64_t> z(n, 0);
        unsigned oi = 0;
        for (unsigned a = 0; a < n; ++a)
          z[a] = (a == kIdx) ? k : o[oi++];
        return z;
      };
      std::map<std::vector<std::int64_t>, std::map<std::int64_t, std::vector<int>>>
          fibers;
      std::fill(x.begin(), x.end(), 0);
      std::function<void(unsigned)> sweep2 = [&](unsigned d) {
        if (d == n) {
          if (!inDomain(dom, x))
            return;
          auto z = zOfS(x);
          fibers[okeyOfS(z)].emplace(z[kIdx], x);
          return;
        }
        for (x[d] = 0; x[d] <= hi; ++x[d])
          sweep2(d + 1);
      };
      sweep2(0);
      const std::int64_t g = part->stride;
      long genMacs = 0;
      std::set<long> lensSeen;
      bool good = true;
      for (const auto &kv : fibers) {
        std::vector<std::int64_t> ks;
        for (const auto &e : kv.second)
          ks.push_back(e.first);
        const std::int64_t L = (std::int64_t)ks.size();
        genMacs += L;
        lensSeen.insert((long)L);
        for (std::size_t t = 1; t < ks.size(); ++t)
          good &= (ks[t] - ks[t - 1] == g);
        auto za = zFromOkeyS(kv.first, ks.front());
        int hits = 0;
        std::int64_t hl = -1;
        for (const auto &grp : part->groups)
          if (inGroup(grp, za)) {
            ++hits;
            hl = grp.length;
          }
        good &= (hits == 1) && (hl == L);
        if (ks.size() >= 2) {
          auto zb = zFromOkeyS(kv.first, ks.front() + g);
          for (const auto &grp : part->groups)
            good &= !inGroup(grp, zb);
        }
        for (std::size_t t = 0; t + 1 < ks.size(); ++t) {
          const auto &xa = kv.second.at(ks[t]);
          const auto &xb = kv.second.at(ks[t + 1]);
          for (unsigned op = 0; op < 3; ++op)
            for (std::size_t r = 0; r < sF[op].size(); ++r) {
              BigInt aa = sOff[op][r], bb = sOff[op][r];
              for (unsigned c = 0; c < n; ++c) {
                aa += sF[op][r][c] * BigInt(xa[c]);
                bb += sF[op][r][c] * BigInt(xb[c]);
              }
              good &= ((bb - aa) == part->tStride[op][r]);
            }
        }
      }
      if (verify(good,
                 (b + ": inner fibers stride-g; unique min-anchor of correct "
                      "length; multiplicity and operand/output t-stride exact "
                      "(D_generated == D_original)")
                     .c_str()))
        return false;
      if (verify(genMacs == srcMacs,
                 (b + ": generated MAC count equals enumerated source count")
                     .c_str()))
        return false;
      if (verify(lensSeen == expInnerLengths,
                 (b + ": enumerated inner fiber lengths match plan").c_str()))
        return false;
      // Anchor pieces pairwise disjoint within each group.
      bool disjoint = true;
      for (const auto &grp : part->groups)
        for (std::size_t a = 0; a < grp.anchorPieces.size(); ++a)
          for (std::size_t c = a + 1; c < grp.anchorPieces.size(); ++c) {
            IntegerPolyhedron inter = grp.anchorPieces[a];
            const auto &pc = grp.anchorPieces[c];
            for (unsigned r = 0; r < pc.getNumInequalities(); ++r)
              inter.addInequality(pc.getInequality(r));
            for (unsigned r = 0; r < pc.getNumEqualities(); ++r)
              inter.addEquality(pc.getEquality(r));
            disjoint &= inter.isIntegerEmpty();
          }
      if (verify(disjoint, (b + ": anchor pieces pairwise disjoint").c_str()))
        return false;
      // Empty-fiber witness: the convex hull of the logical image strictly
      // contains the image (CASE 1's det-9 sublattice leaves lattice holes), so
      // there EXIST convex-hull coordinates with an empty inner reduction fiber.
      // Since the generated MAC count equals the source count (checked above),
      // the plan emits ZERO at every such hole -- convex-hull overshoot never
      // introduces a MAC. Build the pure-z convex hull from the synthetic image
      // (dropping the congruence equalities) and count its integer points over
      // the hull's own tight per-dimension bounds.
      if (wantEmptyFiberWitness) {
        std::string cWhy;
        auto con = buildSymbolicContraction(
            sF, sOff, dom, rank,
            systolic::logical::ReductionOrderDecision::PreserveSourceOrder,
            cWhy);
        bool witnessOk = con.has_value();
        long hull = 0;
        if (con) {
          const auto &DD = con->domain;
          const unsigned add2 = con->congruenceLocals;
          IntegerPolyhedron cz(PresburgerSpace::getSetSpace(n, 0, 0));
          auto pureZ = [&](llvm::ArrayRef<llvm::DynamicAPInt> row,
                           llvm::SmallVector<std::int64_t> &out) -> bool {
            for (unsigned l = 0; l < add2; ++l)
              if (row[n + l] != llvm::DynamicAPInt(0))
                return false;
            out.assign(n + 1, 0);
            for (unsigned j = 0; j < n; ++j)
              out[j] = static_cast<std::int64_t>(row[j]);
            out[n] = static_cast<std::int64_t>(row[n + add2]);
            return true;
          };
          for (unsigned r = 0; r < DD.getNumInequalities(); ++r) {
            llvm::SmallVector<std::int64_t> e;
            if (pureZ(DD.getInequality(r), e))
              cz.addInequality(e);
          }
          for (unsigned r = 0; r < DD.getNumEqualities(); ++r) {
            llvm::SmallVector<std::int64_t> e;
            if (pureZ(DD.getEquality(r), e))
              cz.addEquality(e);
          }
          std::vector<std::int64_t> lo(n), up(n);
          bool bounded = true;
          for (unsigned d = 0; d < n && bounded; ++d) {
            auto a = cz.getConstantBound64(
                mlir::presburger::BoundType::LB, d);
            auto bb = cz.getConstantBound64(
                mlir::presburger::BoundType::UB, d);
            if (!a || !bb)
              bounded = false;
            else {
              lo[d] = *a;
              up[d] = *bb;
            }
          }
          witnessOk = bounded;
          if (bounded) {
            std::vector<std::int64_t> z(n, 0);
            std::function<void(unsigned)> cw = [&](unsigned d) {
              if (d == n) {
                IntegerPolyhedron P = cz;
                const unsigned cols = P.getNumCols();
                for (unsigned j = 0; j < n; ++j) {
                  llvm::SmallVector<std::int64_t> e(cols, 0);
                  e[j] = 1;
                  e[cols - 1] = -z[j];
                  P.addEquality(e);
                }
                if (!P.isIntegerEmpty())
                  ++hull;
                return;
              }
              for (z[d] = lo[d]; z[d] <= up[d]; ++z[d])
                cw(d + 1);
            };
            cw(0);
          }
        }
        if (verify(witnessOk && hull > srcMacs,
                   (b + ": convex hull strictly contains the image -- "
                        "empty-fiber coordinates exist and emit zero MACs")
                       .c_str()))
          return false;
      }
      return true;
    };

    auto runScanM4Unknown = [&](const char *name,
                                const std::array<IntMatrix, 3> &F,
                                const std::array<IntRow, 3> &off, unsigned rank,
                                const IntegerPolyhedron &dom) -> bool {
      std::string why;
      auto plan = buildReductionScanPlan(
          F, off, dom, rank,
          systolic::logical::ReductionOrderDecision::PreserveSourceOrder, why);
      std::string b = name;
      if (verify(!plan.has_value(),
                 (b + ": UNKNOWN (conservative refusal)").c_str()))
        return false;
      return true;
    };

    // CASE 1 (det-9, d=3, stride-3) over source (i,j,k1): C[i+j-2k1+6][i-2j+k1+6]
    // += A[i+j-2k1+6][i+j+k1] * B[i+j+k1][i-2j+k1+6]. The congruence-coupled
    // reduction axis is k1; clean reduction axes are appended as k2, k3.

    // (A) Two-dimensional reduction, one congruence-coupled axis (probe A).
    //     source (i,j,k1,k2); k2 clean.
    {
      std::array<IntMatrix, 3> F = {
          IntMatrix{{BigInt(1),BigInt(1),BigInt(-2),BigInt(0)},{BigInt(1),BigInt(1),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1)}},
          IntMatrix{{BigInt(1),BigInt(1),BigInt(1),BigInt(0)},{BigInt(1),BigInt(-2),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1)}},
          IntMatrix{{BigInt(1),BigInt(1),BigInt(-2),BigInt(0)},{BigInt(1),BigInt(-2),BigInt(1),BigInt(0)}}};
      std::array<IntRow, 3> O = {IntRow{BigInt(6),BigInt(0),BigInt(0)},
                                 IntRow{BigInt(0),BigInt(6),BigInt(0)},
                                 IntRow{BigInt(6),BigInt(6)}};
      if (!runScanM4("C3c3m4 2D-one-coupled", F, O, 4, box(4, 3), 3, 2, 3,
                     {1, 2, 3, 4}, false, false))
        return 1;
    }

    // (B) Three-dimensional reduction, one congruence-coupled axis (probe E).
    //     source (i,j,k1,k2,k3); k2,k3 clean (multiple clean peels, case D too).
    {
      std::array<IntMatrix, 3> F = {
          IntMatrix{{BigInt(1),BigInt(1),BigInt(-2),BigInt(0),BigInt(0)},{BigInt(1),BigInt(1),BigInt(1),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(1)}},
          IntMatrix{{BigInt(1),BigInt(1),BigInt(1),BigInt(0),BigInt(0)},{BigInt(1),BigInt(-2),BigInt(1),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(1)}},
          IntMatrix{{BigInt(1),BigInt(1),BigInt(-2),BigInt(0),BigInt(0)},{BigInt(1),BigInt(-2),BigInt(1),BigInt(0),BigInt(0)}}};
      std::array<IntRow, 3> O = {IntRow{BigInt(6),BigInt(0),BigInt(0),BigInt(0)},
                                 IntRow{BigInt(0),BigInt(6),BigInt(0),BigInt(0)},
                                 IntRow{BigInt(6),BigInt(6)}};
      if (!runScanM4("C3c3m4 3D-one-coupled", F, O, 5, box(5, 3), 3, 3, 3,
                     {1, 2, 3, 4}, false, false))
        return 1;
    }

    // Case C of the spec (congruence stride of 3, reduction ordinals and
    // operand addresses) is discharged inside every runScanM4 call above: the
    // "inner stride g" assertion fixes g == 3 and the "operand/output t-stride
    // exact" assertion checks A/B/C addresses advance correctly along the
    // stride-3 fiber. Case D (multiple clean peeled axes with constant bounds)
    // is the 3D-one-coupled case above (two clean peels k2, k3).

    // (E) Genuine batched GEMM combined with a congruence-restricted reduction:
    //     source (bt,i,j,k1,k2); bt genuine batch, k1 coupled, k2 clean.
    //     C[bt][i+j-2k1+6][i-2j+k1+6] += A[bt][i+j-2k1+6][i+j+k1][k2]
    //                                   * B[bt][i+j+k1][i-2j+k1+6][k2].
    {
      std::array<IntMatrix, 3> F = {
          IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(1),BigInt(1),BigInt(-2),BigInt(0)},{BigInt(0),BigInt(1),BigInt(1),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(1)}},
          IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(1),BigInt(1),BigInt(1),BigInt(0)},{BigInt(0),BigInt(1),BigInt(-2),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(1)}},
          IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(1),BigInt(1),BigInt(-2),BigInt(0)},{BigInt(0),BigInt(1),BigInt(-2),BigInt(1),BigInt(0)}}};
      std::array<IntRow, 3> O = {IntRow{BigInt(0),BigInt(6),BigInt(0),BigInt(0)},
                                 IntRow{BigInt(0),BigInt(0),BigInt(6),BigInt(0)},
                                 IntRow{BigInt(0),BigInt(6),BigInt(6)}};
      if (!runScanM4("C3c3m4 batch+congruence", F, O, 5, box(5, 2), 2, 2, 3,
                     {1, 2, 3}, false, false))
        return 1;
    }

    // (F) Nonzero reduction lower bound: the clean peeled axis k2 lies in [1,3].
    {
      std::array<IntMatrix, 3> F = {
          IntMatrix{{BigInt(1),BigInt(1),BigInt(-2),BigInt(0)},{BigInt(1),BigInt(1),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1)}},
          IntMatrix{{BigInt(1),BigInt(1),BigInt(1),BigInt(0)},{BigInt(1),BigInt(-2),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1)}},
          IntMatrix{{BigInt(1),BigInt(1),BigInt(-2),BigInt(0)},{BigInt(1),BigInt(-2),BigInt(1),BigInt(0)}}};
      std::array<IntRow, 3> O = {IntRow{BigInt(6),BigInt(0),BigInt(0)},
                                 IntRow{BigInt(0),BigInt(6),BigInt(0)},
                                 IntRow{BigInt(6),BigInt(6)}};
      IntegerPolyhedron dom = box(4, 3);
      dom.addInequality({0, 0, 0, 1, -1}); // k2 >= 1 (nonzero lower bound)
      if (!runScanM4("C3c3m4 nonzero-lb", F, O, 4, dom, 3, 2, 3, {1, 2, 3, 4},
                     false, false))
        return 1;
    }

    // (G) Empty inner fiber: CASE 1's det-9 sublattice leaves lattice holes, so
    //     the convex hull of the logical image strictly contains the image --
    //     there exist convex-hull coordinates whose inner reduction fiber is
    //     empty. The oracle's generated-count == source-count assertion proves
    //     the plan emits ZERO MACs for those coordinates, and the empty-fiber
    //     witness confirms such coordinates exist (convex hull > image).
    {
      std::array<IntMatrix, 3> F = {
          IntMatrix{{BigInt(1),BigInt(1),BigInt(-2),BigInt(0)},{BigInt(1),BigInt(1),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1)}},
          IntMatrix{{BigInt(1),BigInt(1),BigInt(1),BigInt(0)},{BigInt(1),BigInt(-2),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1)}},
          IntMatrix{{BigInt(1),BigInt(1),BigInt(-2),BigInt(0)},{BigInt(1),BigInt(-2),BigInt(1),BigInt(0)}}};
      std::array<IntRow, 3> O = {IntRow{BigInt(6),BigInt(0),BigInt(0)},
                                 IntRow{BigInt(0),BigInt(6),BigInt(0)},
                                 IntRow{BigInt(6),BigInt(6)}};
      if (!runScanM4("C3c3m4 empty-inner-fiber", F, O, 4, box(4, 3), 3, 2, 3,
                     {1, 2, 3, 4}, false, /*wantEmptyFiberWitness=*/true))
        return 1;
    }

    // (H) Empty reduction domain: contradictory constraint -> empty plan.
    {
      std::array<IntMatrix, 3> F = {
          IntMatrix{{BigInt(1),BigInt(1),BigInt(-2),BigInt(0)},{BigInt(1),BigInt(1),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1)}},
          IntMatrix{{BigInt(1),BigInt(1),BigInt(1),BigInt(0)},{BigInt(1),BigInt(-2),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1)}},
          IntMatrix{{BigInt(1),BigInt(1),BigInt(-2),BigInt(0)},{BigInt(1),BigInt(-2),BigInt(1),BigInt(0)}}};
      std::array<IntRow, 3> O = {IntRow{BigInt(6),BigInt(0),BigInt(0)},
                                 IntRow{BigInt(0),BigInt(6),BigInt(0)},
                                 IntRow{BigInt(6),BigInt(6)}};
      IntegerPolyhedron dom = box(4, 3);
      dom.addInequality({0, 0, 1, 0, -5}); // k1 >= 5 contradicts k1 <= 3
      if (!runScanM4("C3c3m4 empty-domain", F, O, 4, dom, 3, 2, 3, {}, true,
                     false))
        return 1;
    }

    // (I) Regression control: a congruence-FREE multi-reduction still flows
    //     through the same oracle (locals == 0 path, stride 1), confirming the
    //     M4-box changes did not disturb the M1/M2/M3 behaviour. The full
    //     M1/M2/M3 blocks above remain the primary regression controls.
    {
      std::array<IntMatrix, 3> F = {
          IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1)}},
          IntMatrix{{BigInt(0),BigInt(0),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0)}},
          IntMatrix{{BigInt(1),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(1),BigInt(0),BigInt(0)}}};
      std::array<IntRow, 3> O = {IntRow{BigInt(0),BigInt(0),BigInt(0)},
                                 IntRow{BigInt(0),BigInt(0),BigInt(0)},
                                 IntRow{BigInt(0),BigInt(0)}};
      if (!runScanM4("C3c3m4 congruence-free-control", F, O, 4, box(4, 3), 3, 2,
                     1, {4}, false, false))
        return 1;
    }

    // ---- M4-tri: non-rectangular, clean-axis-dependent peel bounds, with one
    //      congruence-coupled inner axis (stride 3). The clean dependency shapes
    //      the peel ranges (triangular/simplex/banded/trapezoidal) while the
    //      inner CASE 1 fiber structure is unchanged (lengths {1,2,3,4}). ----
    // Reusable r=3 accesses: CASE1(i,j,k1 coupled) + clean k2 (col3), k3 (col4).
    auto triF = []() {
      return std::array<IntMatrix, 3>{
          IntMatrix{{BigInt(1),BigInt(1),BigInt(-2),BigInt(0),BigInt(0)},{BigInt(1),BigInt(1),BigInt(1),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(1)}},
          IntMatrix{{BigInt(1),BigInt(1),BigInt(1),BigInt(0),BigInt(0)},{BigInt(1),BigInt(-2),BigInt(1),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(1)}},
          IntMatrix{{BigInt(1),BigInt(1),BigInt(-2),BigInt(0),BigInt(0)},{BigInt(1),BigInt(-2),BigInt(1),BigInt(0),BigInt(0)}}};
    };
    std::array<IntRow, 3> triO = {IntRow{BigInt(6),BigInt(0),BigInt(0),BigInt(0)},
                                  IntRow{BigInt(0),BigInt(6),BigInt(0),BigInt(0)},
                                  IntRow{BigInt(6),BigInt(6)}};
    // Triangular: k3 <= k2.
    {
      IntegerPolyhedron dom = box(5, 3);
      dom.addInequality({0, 0, 0, 1, -1, 0}); // k2 - k3 >= 0
      if (!runScanM4("C3c3m4 tri-triangular", triF(), triO, 5, dom, 3, 3, 3,
                     {1, 2, 3, 4}, false, false))
        return 1;
    }
    // Banded: k2 - 1 <= k3 <= k2.
    {
      IntegerPolyhedron dom = box(5, 3);
      dom.addInequality({0, 0, 0, 1, -1, 0});  // k2 - k3 >= 0
      dom.addInequality({0, 0, 0, -1, 1, 1});  // k3 - k2 + 1 >= 0
      if (!runScanM4("C3c3m4 tri-banded", triF(), triO, 5, dom, 3, 3, 3,
                     {1, 2, 3, 4}, false, false))
        return 1;
    }
    // Trapezoidal: k3 <= min(k2, 2).
    {
      IntegerPolyhedron dom = box(5, 3);
      dom.addInequality({0, 0, 0, 1, -1, 0});  // k2 - k3 >= 0
      dom.addInequality({0, 0, 0, 0, -1, 2});  // 2 - k3 >= 0
      if (!runScanM4("C3c3m4 tri-trapezoidal", triF(), triO, 5, dom, 3, 3, 3,
                     {1, 2, 3, 4}, false, false))
        return 1;
    }
    // Simplex over THREE clean axes (r=4): 0 <= k4 <= k3 <= k2 <= 3, with the
    // congruence-coupled k1 tiled. source (i,j,k1,k2,k3,k4).
    {
      std::array<IntMatrix, 3> F = {
          IntMatrix{{BigInt(1),BigInt(1),BigInt(-2),BigInt(0),BigInt(0),BigInt(0)},{BigInt(1),BigInt(1),BigInt(1),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(1)}},
          IntMatrix{{BigInt(1),BigInt(1),BigInt(1),BigInt(0),BigInt(0),BigInt(0)},{BigInt(1),BigInt(-2),BigInt(1),BigInt(0),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(1)}},
          IntMatrix{{BigInt(1),BigInt(1),BigInt(-2),BigInt(0),BigInt(0),BigInt(0)},{BigInt(1),BigInt(-2),BigInt(1),BigInt(0),BigInt(0),BigInt(0)}}};
      std::array<IntRow, 3> O = {IntRow{BigInt(6),BigInt(0),BigInt(0),BigInt(0),BigInt(0)},
                                 IntRow{BigInt(0),BigInt(6),BigInt(0),BigInt(0),BigInt(0)},
                                 IntRow{BigInt(6),BigInt(6)}};
      IntegerPolyhedron dom = box(6, 3);
      dom.addInequality({0, 0, 0, 1, -1, 0, 0}); // k2 - k3 >= 0
      dom.addInequality({0, 0, 0, 0, 1, -1, 0}); // k3 - k4 >= 0
      if (!runScanM4("C3c3m4 tri-simplex-3clean", F, O, 6, dom, 3, 4, 3,
                     {1, 2, 3, 4}, false, false))
        return 1;
    }

    // ---- Negative tests: conservative UNKNOWN (safety must not weaken) ----
    auto caseF = []() {
      return std::array<IntMatrix, 3>{
          IntMatrix{{BigInt(1),BigInt(1),BigInt(-2),BigInt(0)},{BigInt(1),BigInt(1),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1)}},
          IntMatrix{{BigInt(1),BigInt(1),BigInt(1),BigInt(0)},{BigInt(1),BigInt(-2),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1)}},
          IntMatrix{{BigInt(1),BigInt(1),BigInt(-2),BigInt(0)},{BigInt(1),BigInt(-2),BigInt(1),BigInt(0)}}};
    };
    std::array<IntRow, 3> caseFO = {IntRow{BigInt(6),BigInt(0),BigInt(0)},
                                    IntRow{BigInt(0),BigInt(6),BigInt(0)},
                                    IntRow{BigInt(6),BigInt(6)}};
    // (N1) Clean peel coupled to the inner congruence axis (k2 <= k1).
    {
      IntegerPolyhedron dom = box(4, 3);
      dom.addInequality({0, 0, 1, -1, 0}); // k1 - k2 >= 0
      if (!runScanM4Unknown("C3c3m4 neg coupled-inner-dependence", caseF(),
                            caseFO, 4, dom))
        return 1;
    }
    // (N2) Output-dependent clean peel range (k2 <= i).
    {
      IntegerPolyhedron dom = box(4, 3);
      dom.addInequality({1, 0, 0, -1, 0}); // i - k2 >= 0
      if (!runScanM4Unknown("C3c3m4 neg output-dependent-peel", caseF(), caseFO,
                            4, dom))
        return 1;
    }
    // (N3) Zero congruence-coupled reduction axes (output-only skew, det-2,
    //      clean reductions) -> congruence couples != 1 -> UNKNOWN.
    {
      std::array<IntMatrix, 3> FZ = {
          IntMatrix{{BigInt(1),BigInt(1),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1)}},
          IntMatrix{{BigInt(0),BigInt(0),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1)},{BigInt(1),BigInt(-1),BigInt(0),BigInt(0)}},
          IntMatrix{{BigInt(1),BigInt(1),BigInt(0),BigInt(0)},{BigInt(1),BigInt(-1),BigInt(0),BigInt(0)}}};
      std::array<IntRow, 3> OZ = {IntRow{BigInt(6),BigInt(0),BigInt(0)},
                                  IntRow{BigInt(0),BigInt(0),BigInt(6)},
                                  IntRow{BigInt(6),BigInt(6)}};
      if (!runScanM4Unknown("C3c3m4 neg zero-coupled-axes", FZ, OZ, 4,
                            box(4, 3)))
        return 1;
    }
    // (N4) Overflow: an enormous coupled-axis extent -> UNKNOWN.
    {
      const std::int64_t BIG = std::int64_t(1) << 40;
      IntegerPolyhedron dom(PresburgerSpace::getSetSpace(4, 0, 0));
      dom.addInequality({1, 0, 0, 0, 0});
      dom.addInequality({-1, 0, 0, 0, 3});
      dom.addInequality({0, 1, 0, 0, 0});
      dom.addInequality({0, -1, 0, 0, 3});
      dom.addInequality({0, 0, 1, 0, 0});
      dom.addInequality({0, 0, -1, 0, BIG});
      dom.addInequality({0, 0, 0, 1, 0});
      dom.addInequality({0, 0, 0, -1, 3});
      if (!runScanM4Unknown("C3c3m4 neg overflow", caseF(), caseFO, 4, dom))
        return 1;
    }
    // (N5) Output aliasing with a congruence: an extra axis p indexes B only
    //      (not C). source (i,j,k1,k2,p).
    {
      std::array<IntMatrix, 3> FA = {
          IntMatrix{{BigInt(1),BigInt(1),BigInt(-2),BigInt(0),BigInt(0)},{BigInt(1),BigInt(1),BigInt(1),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1),BigInt(0)}},
          IntMatrix{{BigInt(1),BigInt(1),BigInt(1),BigInt(0),BigInt(0)},{BigInt(1),BigInt(-2),BigInt(1),BigInt(0),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(1),BigInt(0)},{BigInt(0),BigInt(0),BigInt(0),BigInt(0),BigInt(1)}},
          IntMatrix{{BigInt(1),BigInt(1),BigInt(-2),BigInt(0),BigInt(0)},{BigInt(1),BigInt(-2),BigInt(1),BigInt(0),BigInt(0)}}};
      std::array<IntRow, 3> OA = {IntRow{BigInt(6),BigInt(0),BigInt(0)},
                                  IntRow{BigInt(0),BigInt(6),BigInt(0),BigInt(0)},
                                  IntRow{BigInt(6),BigInt(6)}};
      if (!runScanM4Unknown("C3c3m4 neg output-aliasing", FA, OA, 5, box(5, 3)))
        return 1;
    }
    // (N6) M4-tri must still refuse a peel bound coupled to the inner congruence
    //      axis (k2 <= k1) -- an outer-depends-on-inner inversion.
    {
      IntegerPolyhedron dom = box(5, 3);
      dom.addInequality({0, 0, 1, -1, 0, 0}); // k1 - k2 >= 0
      if (!runScanM4Unknown("C3c3m4 neg tri-coupled-inner", triF(), triO, 5, dom))
        return 1;
    }
    // (N7) M4-tri must still refuse a congruence-skewed output-dependent peel
    //      bound (k2 <= i): the skewed output drags the coupled column in.
    {
      IntegerPolyhedron dom = box(5, 3);
      dom.addInequality({1, 0, 0, -1, 0, 0}); // i - k2 >= 0
      if (!runScanM4Unknown("C3c3m4 neg tri-output-skew", triF(), triO, 5, dom))
        return 1;
    }

    std::cout << "Phase C3c-3 M4-box/M4-tri reduction scan tests passed.\n";
  }

  // --------------------------------------------------------
  // M-count (Option C): EXACT, SYMBOLIC lattice-point counting of the C3 anchor
  // pieces for the general-affine lowering diagnostic, with NO source-point
  // enumeration in the counter. Validated against an independent brute-force
  // oracle (enumeration confined to this test). countSeparableBox counts an
  // axis-parallel source polytope as a product of per-axis integer intervals;
  // countAnchorPieceFibers composes a piece's pure-z faces with z = T*x into the
  // dense source space; countPartition sums fibers and MACs over the certified
  // groups. Non-box pieces (triangular/simplicial/coupling, e.g. a guarded
  // domain) return UNKNOWN -- never an approximation.
  // --------------------------------------------------------
  {
    using mlir::presburger::IntegerPolyhedron;
    using mlir::presburger::PresburgerSpace;
    using systolic::logical::countSeparableBox;
    using systolic::logical::countAnchorPieceFibers;
    using systolic::logical::countPartition;

    // Independent brute-force oracle: integer points of P over its DIM vars
    // (congruence locals existential). TEST ONLY.
    auto brute = [](const IntegerPolyhedron &P) -> long {
      unsigned nd = P.getNumDimVars();
      std::vector<long> lo(nd), hi(nd);
      for (unsigned d = 0; d < nd; ++d) {
        auto a = P.getConstantBound64(mlir::presburger::BoundType::LB, d);
        auto b = P.getConstantBound64(mlir::presburger::BoundType::UB, d);
        if (!a || !b)
          return -1;
        lo[d] = *a;
        hi[d] = *b;
      }
      long cnt = 0;
      std::vector<long> x(nd);
      std::function<void(unsigned)> rec = [&](unsigned d) {
        if (d == nd) {
          IntegerPolyhedron Q = P;
          unsigned cols = Q.getNumCols();
          for (unsigned j = 0; j < nd; ++j) {
            llvm::SmallVector<std::int64_t> e(cols, 0);
            e[j] = 1;
            e[cols - 1] = -x[j];
            Q.addEquality(e);
          }
          if (!Q.isIntegerEmpty())
            ++cnt;
          return;
        }
        for (x[d] = lo[d]; x[d] <= hi[d]; ++x[d])
          rec(d + 1);
      };
      rec(0);
      return cnt;
    };
    auto asLong = [](const std::optional<systolic::logical::BigInt> &c) -> long {
      return c ? (long)c->template convert_to<std::int64_t>() : -1;
    };

    // Synthetic separable-box primitives.
    {
      // [0,4) x [0,4) x [0,4) -> 64.
      IntegerPolyhedron b(PresburgerSpace::getSetSpace(3, 0, 0));
      for (unsigned a = 0; a < 3; ++a) {
        llvm::SmallVector<std::int64_t> lo(4, 0), up(4, 0);
        lo[a] = 1;
        b.addInequality(lo);
        up[a] = -1;
        up[3] = 3;
        b.addInequality(up);
      }
      if (verify(asLong(countSeparableBox(b)) == 64,
                 "M-count box [0,4)^3 == 64"))
        return 1;
    }
    {
      // [1,3] x [0,5] = 3 * 6 = 18.
      IntegerPolyhedron b(PresburgerSpace::getSetSpace(2, 0, 0));
      b.addInequality({1, 0, -1});   // x0 >= 1
      b.addInequality({-1, 0, 3});   // x0 <= 3
      b.addInequality({0, 1, 0});    // x1 >= 0
      b.addInequality({0, -1, 5});   // x1 <= 5
      if (verify(asLong(countSeparableBox(b)) == 18,
                 "M-count box [1,3]x[0,5] == 18"))
        return 1;
    }
    {
      // Empty box: x0 >= 2 and x0 <= 1 -> 0.
      IntegerPolyhedron b(PresburgerSpace::getSetSpace(1, 0, 0));
      b.addInequality({1, -2});
      b.addInequality({-1, 1});
      if (verify(asLong(countSeparableBox(b)) == 0, "M-count empty box == 0"))
        return 1;
    }
    {
      // Coupling face x0 + x1 <= 3 (triangular) -> UNKNOWN (not a separable box).
      IntegerPolyhedron t(PresburgerSpace::getSetSpace(2, 0, 0));
      t.addInequality({1, 0, 0});
      t.addInequality({0, 1, 0});
      t.addInequality({-1, -1, 3});
      if (verify(!countSeparableBox(t).has_value(),
                 "M-count triangular/coupling -> UNKNOWN"))
        return 1;
    }
    {
      // Unbounded axis -> UNKNOWN.
      IntegerPolyhedron u(PresburgerSpace::getSetSpace(1, 0, 0));
      u.addInequality({1, 0}); // x0 >= 0, no upper bound
      if (verify(!countSeparableBox(u).has_value(),
                 "M-count unbounded axis -> UNKNOWN"))
        return 1;
    }

    // CASE 1 partition: exact fiber/MAC counts via the certified anchor pieces.
    auto sourceToLogicalMatrix =
        [&](const std::array<IntMatrix, 3> &F,
            unsigned rank) -> std::optional<IntMatrix> {
      auto p = analyzeRankPartition(F[0], F[1], F[2], rank);
      auto e = p ? expandRankPartition(*p) : std::nullopt;
      auto m = e ? buildPartitionCoordinateMap(*e) : std::nullopt;
      if (!m || m->sourceToLogical.size() != rank)
        return std::nullopt;
      IntMatrix T(rank, IntRow(rank, BigInt(0)));
      for (unsigned j = 0; j < rank; ++j) {
        if (m->sourceToLogical[j].size() != rank)
          return std::nullopt;
        for (unsigned s = 0; s < rank; ++s)
          T[j][s] = m->sourceToLogical[j][s];
      }
      return T;
    };
    auto box = [&](unsigned rank, int hi) {
      IntegerPolyhedron s(PresburgerSpace::getSetSpace(rank, 0, 0));
      for (unsigned a = 0; a < rank; ++a) {
        llvm::SmallVector<std::int64_t> lo(rank + 1, 0), up(rank + 1, 0);
        lo[a] = 1;
        s.addInequality(lo);
        up[a] = -1;
        up[rank] = hi;
        s.addInequality(up);
      }
      return s;
    };
    std::array<IntMatrix, 3> C1F = {
        IntMatrix{{BigInt(1),BigInt(1),BigInt(-2)},{BigInt(1),BigInt(1),BigInt(1)}},
        IntMatrix{{BigInt(1),BigInt(1),BigInt(1)},{BigInt(1),BigInt(-2),BigInt(1)}},
        IntMatrix{{BigInt(1),BigInt(1),BigInt(-2)},{BigInt(1),BigInt(-2),BigInt(1)}}};
    std::array<IntRow, 3> C1O = {IntRow{BigInt(6),BigInt(0)},
                                 IntRow{BigInt(0),BigInt(6)},
                                 IntRow{BigInt(6),BigInt(6)}};
    {
      auto T = sourceToLogicalMatrix(C1F, 3);
      std::string why;
      auto part = buildSymbolicTilePartition(
          C1F, C1O, box(3, 3), 3,
          systolic::logical::ReductionOrderDecision::PreserveSourceOrder, why);
      if (verify(T.has_value() && part.has_value(),
                 "M-count CASE1 partition + coordinate map"))
        return 1;
      auto counts = countPartition(*part, *T, 3);
      if (verify(counts.has_value(), "M-count CASE1 counted exactly"))
        return 1;
      if (verify(counts->outputFibers == BigInt(37),
                 "M-count CASE1 outputFibers == 37"))
        return 1;
      if (verify(counts->sourceMACs == BigInt(64),
                 "M-count CASE1 sourceMACs == 64"))
        return 1;
      // Per-group histogram 18/12/6/1, cross-checked against brute per piece.
      std::map<long, long> hist;
      bool pieceOk = true;
      for (const auto &g : part->groups) {
        long gf = 0;
        for (const auto &P : g.anchorPieces) {
          auto c = countAnchorPieceFibers(P, *T, 3, part->congruenceLocals);
          long b = brute(P);
          pieceOk &= (c.has_value() && (long)c->convert_to<std::int64_t>() == b);
          gf += b;
        }
        hist[(long)g.length] += gf;
      }
      if (verify(pieceOk,
                 "M-count CASE1 per-piece counter matches brute oracle"))
        return 1;
      if (verify(hist[1] == 18 && hist[2] == 12 && hist[3] == 6 && hist[4] == 1,
                 "M-count CASE1 length histogram 18/12/6/1"))
        return 1;
    }

    // Guarded domain (box intersect i+j<=3): pieces are trapezoidal -> the
    // separable-box counter returns UNKNOWN (never an approximation). The
    // brute oracle records the true answer (not yet produced symbolically).
    {
      auto T = sourceToLogicalMatrix(C1F, 3);
      IntegerPolyhedron dom = box(3, 3);
      dom.addInequality({-1, -1, 0, 3}); // i + j <= 3
      std::string why;
      auto part = buildSymbolicTilePartition(
          C1F, C1O, dom, 3,
          systolic::logical::ReductionOrderDecision::PreserveSourceOrder, why);
      if (verify(T.has_value() && part.has_value(),
                 "M-count guard partition built"))
        return 1;
      if (verify(!countPartition(*part, *T, 3).has_value(),
                 "M-count guard (trapezoidal pieces) -> UNKNOWN (conservative)"))
        return 1;
    }

    // A different non-unimodular frame (det T = -14), box domain: the counter
    // is not CASE 1-specific. Output map injective -> 64 length-1 fibers.
    {
      std::array<IntMatrix, 3> FF = {
          IntMatrix{{BigInt(2),BigInt(1),BigInt(-1)},{BigInt(1),BigInt(1),BigInt(2)}},
          IntMatrix{{BigInt(1),BigInt(1),BigInt(2)},{BigInt(1),BigInt(-2),BigInt(1)}},
          IntMatrix{{BigInt(2),BigInt(1),BigInt(-1)},{BigInt(1),BigInt(-2),BigInt(1)}}};
      std::array<IntRow, 3> FO = {IntRow{BigInt(13),BigInt(12)},
                                  IntRow{BigInt(14),BigInt(15)},
                                  IntRow{BigInt(12),BigInt(12)}};
      auto T = sourceToLogicalMatrix(FF, 3);
      std::string why;
      auto part = buildSymbolicTilePartition(
          FF, FO, box(3, 3), 3,
          systolic::logical::ReductionOrderDecision::PreserveSourceOrder, why);
      if (verify(T.has_value() && part.has_value(),
                 "M-count frame-0 partition + map"))
        return 1;
      auto counts = countPartition(*part, *T, 3);
      if (verify(counts.has_value() && counts->outputFibers == BigInt(64) &&
                     counts->sourceMACs == BigInt(64),
                 "M-count frame-0 fibers=64 macs=64"))
        return 1;
    }

    // C4a-sym.0a: a pure replication loop (an IV in NO access) is now CERTIFIED
    // by buildSymbolicTilePartition as an additional reduction MULTIPLICITY --
    // not refused. The replication axis folds into every fiber: the physical
    // output-fiber count is unchanged (the axis does not index C), each fiber's
    // length is multiplied by the axis extent, and the source-MAC multiplicity
    // is preserved. Cross-checked against an independent brute oracle that groups
    // the source box by PHYSICAL C address (enumeration confined to the test).
    //
    // (This block replaces the former replication-refusal feature-boundary test.
    // Its RF/RO/dom input is identical; only the expected verdict is stronger --
    // from "refused" to the exact certified counts.)
    {
      using systolic::logical::countPartitionBlocks;
      using systolic::logical::ReductionOrderDecision;
      // Independent brute oracle: distinct physical C addresses (fibers) and the
      // total source-MAC count over a box domain, grouped by physical C.
      auto bruteByC = [](const std::array<IntMatrix, 3> &F,
                         const std::array<IntRow, 3> &O,
                         const std::vector<std::pair<long, long>> &boxr, long N,
                         long &fibers, long &macs) {
        const unsigned r = static_cast<unsigned>(boxr.size());
        std::vector<long> x(r);
        std::map<long, long> perC;
        long total = 0;
        std::function<void(unsigned)> rec = [&](unsigned d) {
          if (d == r) {
            long c0 = (long)O[2][0].convert_to<std::int64_t>();
            long c1 = (long)O[2][1].convert_to<std::int64_t>();
            for (unsigned s = 0; s < r; ++s) {
              c0 += (long)F[2][0][s].convert_to<std::int64_t>() * x[s];
              c1 += (long)F[2][1][s].convert_to<std::int64_t>() * x[s];
            }
            perC[c0 * N + c1]++;
            ++total;
            return;
          }
          for (x[d] = boxr[d].first; x[d] <= boxr[d].second; ++x[d])
            rec(d + 1);
        };
        rec(0);
        fibers = (long)perC.size();
        macs = total;
      };
      // Build a box domain over `boxr` (rank = boxr.size()).
      auto boxDom = [](const std::vector<std::pair<long, long>> &boxr) {
        const unsigned r = static_cast<unsigned>(boxr.size());
        IntegerPolyhedron d(PresburgerSpace::getSetSpace(r, 0, 0));
        for (unsigned a = 0; a < r; ++a) {
          llvm::SmallVector<std::int64_t> lo(r + 1, 0), up(r + 1, 0);
          lo[a] = 1;
          lo[r] = -boxr[a].first;
          d.addInequality(lo);
          up[a] = -1;
          up[r] = boxr[a].second;
          d.addInequality(up);
        }
        return d;
      };

      // --- Converted test: identical RF/RO/dom (p = source axis 0, extent 2). ---
      std::array<IntMatrix, 3> RF = {
          IntMatrix{{BigInt(0),BigInt(1),BigInt(1),BigInt(-2)},{BigInt(0),BigInt(1),BigInt(1),BigInt(1)}},
          IntMatrix{{BigInt(0),BigInt(1),BigInt(1),BigInt(1)},{BigInt(0),BigInt(1),BigInt(-2),BigInt(1)}},
          IntMatrix{{BigInt(0),BigInt(1),BigInt(1),BigInt(-2)},{BigInt(0),BigInt(1),BigInt(-2),BigInt(1)}}};
      std::array<IntRow, 3> RO = {IntRow{BigInt(6),BigInt(0)},
                                  IntRow{BigInt(0),BigInt(6)},
                                  IntRow{BigInt(6),BigInt(6)}};
      IntegerPolyhedron dom(PresburgerSpace::getSetSpace(4, 0, 0));
      dom.addInequality({1, 0, 0, 0, 0});   // p >= 0
      dom.addInequality({-1, 0, 0, 0, 1});  // p <= 1
      for (unsigned a = 1; a < 4; ++a) {
        llvm::SmallVector<std::int64_t> lo(5, 0), up(5, 0);
        lo[a] = 1;
        dom.addInequality(lo);
        up[a] = -1;
        up[4] = 3;
        dom.addInequality(up);
      }
      {
        auto T = sourceToLogicalMatrix(RF, 4);
        std::string why;
        auto part = buildSymbolicTilePartition(
            RF, RO, dom, 4, ReductionOrderDecision::PreserveSourceOrder, why);
        if (verify(T.has_value() && part.has_value(),
                   "C4a-sym.0a replication certified (was the refusal test)"))
          return 1;
        if (verify(part->replicationSourceAxes.size() == 1 &&
                       part->replicationSourceAxes[0] == 0 &&
                       part->replicationExtent == 2,
                   "C4a-sym.0a replication axis 0 folded, extent 2"))
          return 1;
        long bf, bm;
        bruteByC(RF, RO, {{0,1},{0,3},{0,3},{0,3}}, 16, bf, bm);
        auto counts = countPartitionBlocks(*part, *T, 4);
        if (verify(counts.has_value() && counts->outputFibers == BigInt(37) &&
                       counts->sourceMACs == BigInt(128),
                   "C4a-sym.0a replication R=2: 37 fibers / 128 MACs"))
          return 1;
        if (verify(counts->outputFibers == BigInt(bf) &&
                       counts->sourceMACs == BigInt(bm),
                   "C4a-sym.0a replication R=2 matches brute-by-physical-C"))
          return 1;
      }

      // --- Replication extent 3 (same axis, same frame). ---
      {
        IntegerPolyhedron dom3 = boxDom({{0,2},{0,3},{0,3},{0,3}});
        auto T = sourceToLogicalMatrix(RF, 4);
        std::string why;
        auto part = buildSymbolicTilePartition(
            RF, RO, dom3, 4, ReductionOrderDecision::PreserveSourceOrder, why);
        long bf, bm;
        bruteByC(RF, RO, {{0,2},{0,3},{0,3},{0,3}}, 16, bf, bm);
        auto counts = part ? countPartitionBlocks(*part, *T, 4) : std::nullopt;
        if (verify(part.has_value() && part->replicationExtent == 3 &&
                       counts.has_value() &&
                       counts->outputFibers == BigInt(37) &&
                       counts->sourceMACs == BigInt(192) &&
                       counts->outputFibers == BigInt(bf) &&
                       counts->sourceMACs == BigInt(bm),
                   "C4a-sym.0a replication R=3: 37/192 == brute"))
          return 1;
      }

      // --- Replication axis in a NON-outermost position (source axis 1). ---
      // source (i, p, j, k): zero column at index 1.
      std::array<IntMatrix, 3> PF = {
          IntMatrix{{BigInt(1),BigInt(0),BigInt(1),BigInt(-2)},{BigInt(1),BigInt(0),BigInt(1),BigInt(1)}},
          IntMatrix{{BigInt(1),BigInt(0),BigInt(1),BigInt(1)},{BigInt(1),BigInt(0),BigInt(-2),BigInt(1)}},
          IntMatrix{{BigInt(1),BigInt(0),BigInt(1),BigInt(-2)},{BigInt(1),BigInt(0),BigInt(-2),BigInt(1)}}};
      IntegerPolyhedron pdom = boxDom({{0,3},{0,1},{0,3},{0,3}});
      {
        // Strict FP (PreserveSourceOrder): order not source-equivalent -> UNKNOWN.
        std::string why;
        auto part = buildSymbolicTilePartition(
            PF, RO, pdom, 4, ReductionOrderDecision::PreserveSourceOrder, why);
        if (verify(!part.has_value(),
                   "C4a-sym.0a non-outermost replication refused under strict FP"))
          return 1;
      }
      {
        // Reassociation permitted: accepted, counts exact.
        auto T = sourceToLogicalMatrix(PF, 4);
        std::string why;
        auto part = buildSymbolicTilePartition(
            PF, RO, pdom, 4, ReductionOrderDecision::ReassociationCandidate, why);
        long bf, bm;
        bruteByC(PF, RO, {{0,3},{0,1},{0,3},{0,3}}, 16, bf, bm);
        auto counts = part ? countPartitionBlocks(*part, *T, 4) : std::nullopt;
        if (verify(part.has_value() && counts.has_value() &&
                       counts->outputFibers == BigInt(37) &&
                       counts->sourceMACs == BigInt(128) &&
                       counts->outputFibers == BigInt(bf) &&
                       counts->sourceMACs == BigInt(bm),
                   "C4a-sym.0a non-outermost replication under reassoc: 37/128 "
                   "== brute"))
          return 1;
      }

      // --- Two independent replication axes (source axes 0,1), R = 4. ---
      std::array<IntMatrix, 3> TF = {
          IntMatrix{{BigInt(0),BigInt(0),BigInt(1),BigInt(1),BigInt(-2)},{BigInt(0),BigInt(0),BigInt(1),BigInt(1),BigInt(1)}},
          IntMatrix{{BigInt(0),BigInt(0),BigInt(1),BigInt(1),BigInt(1)},{BigInt(0),BigInt(0),BigInt(1),BigInt(-2),BigInt(1)}},
          IntMatrix{{BigInt(0),BigInt(0),BigInt(1),BigInt(1),BigInt(-2)},{BigInt(0),BigInt(0),BigInt(1),BigInt(-2),BigInt(1)}}};
      {
        IntegerPolyhedron tdom = boxDom({{0,1},{0,1},{0,3},{0,3},{0,3}});
        auto T = sourceToLogicalMatrix(TF, 5);
        std::string why;
        auto part = buildSymbolicTilePartition(
            TF, RO, tdom, 5, ReductionOrderDecision::PreserveSourceOrder, why);
        long bf, bm;
        bruteByC(TF, RO, {{0,1},{0,1},{0,3},{0,3},{0,3}}, 16, bf, bm);
        auto counts = part ? countPartitionBlocks(*part, *T, 5) : std::nullopt;
        if (verify(part.has_value() && part->replicationSourceAxes.size() == 2 &&
                       part->replicationExtent == 4 && counts.has_value() &&
                       counts->outputFibers == BigInt(37) &&
                       counts->sourceMACs == BigInt(256) &&
                       counts->outputFibers == BigInt(bf) &&
                       counts->sourceMACs == BigInt(bm),
                   "C4a-sym.0a two replication axes R=4: 37/256 == brute"))
          return 1;
      }
      std::cout << "C4a-sym.0a replication partition tests passed.\n";
    }

    // ========================================================
    // M-count-2: EXACT, SYMBOLIC counting of certified NON-RECTANGULAR pieces
    // (triangular / trapezoidal / banded / simplicial) via the block-separable
    // counter -- countBlockSeparable / countAnchorPieceFibersBlocks /
    // countPartitionBlocks. The counter never enumerates a lattice point; the
    // brute oracle above is TEST ONLY. The box-only entry points keep their
    // prior semantics (the guard -> UNKNOWN assertion above still holds).
    // ========================================================
    {
      using systolic::logical::countBlockSeparable;
      using systolic::logical::countAnchorPieceFibersBlocks;
      using systolic::logical::countPartitionBlocks;
      auto cntB = [&](const IntegerPolyhedron &P) -> long {
        auto c = countBlockSeparable(P);
        return c ? (long)c->template convert_to<std::int64_t>() : -1;
      };

      // (A) Triangular 0<=y<=x<=4 == 15.
      {
        IntegerPolyhedron t(PresburgerSpace::getSetSpace(2, 0, 0));
        t.addInequality({0, 1, 0});  // y >= 0
        t.addInequality({1, -1, 0}); // y <= x
        t.addInequality({-1, 0, 4}); // x <= 4
        t.addInequality({1, 0, 0});  // x >= 0
        if (verify(cntB(t) == 15 && brute(t) == 15,
                   "M-count-2 triangular 0<=y<=x<=4 == 15"))
          return 1;
      }
      // (B) Trapezoidal 0<=y<=min(x,2), 0<=x<=4 == 12.
      {
        IntegerPolyhedron t(PresburgerSpace::getSetSpace(2, 0, 0));
        t.addInequality({0, 1, 0});
        t.addInequality({1, -1, 0});
        t.addInequality({0, -1, 2});
        t.addInequality({1, 0, 0});
        t.addInequality({-1, 0, 4});
        if (verify(cntB(t) == 12 && brute(t) == 12,
                   "M-count-2 trapezoid min(x,2) == 12"))
          return 1;
      }
      // (C) Banded |x-y|<=1, 0<=x,y<=4 == 13.
      {
        IntegerPolyhedron t(PresburgerSpace::getSetSpace(2, 0, 0));
        t.addInequality({1, 0, 0});
        t.addInequality({-1, 0, 4});
        t.addInequality({0, 1, 0});
        t.addInequality({0, -1, 4});
        t.addInequality({1, -1, 1});
        t.addInequality({-1, 1, 1});
        if (verify(cntB(t) == 13 && brute(t) == 13,
                   "M-count-2 banded |x-y|<=1 == 13"))
          return 1;
      }
      // (D) 3D simplex x,y,z>=0, x+y+z<=3 == C(6,3) == 20.
      {
        IntegerPolyhedron s(PresburgerSpace::getSetSpace(3, 0, 0));
        s.addInequality({1, 0, 0, 0});
        s.addInequality({0, 1, 0, 0});
        s.addInequality({0, 0, 1, 0});
        s.addInequality({-1, -1, -1, 3});
        if (verify(cntB(s) == 20 && brute(s) == 20,
                   "M-count-2 3D simplex sum<=3 == 20"))
          return 1;
      }
      // (D) 4D simplex sum<=2 == C(6,4) == 15.
      {
        IntegerPolyhedron s(PresburgerSpace::getSetSpace(4, 0, 0));
        for (unsigned a = 0; a < 4; ++a) {
          llvm::SmallVector<std::int64_t> lo(5, 0);
          lo[a] = 1;
          s.addInequality(lo);
        }
        s.addInequality({-1, -1, -1, -1, 2});
        if (verify(cntB(s) == 15 && brute(s) == 15,
                   "M-count-2 4D simplex sum<=2 == 15"))
          return 1;
      }
      // 2D simplex with NO explicit x upper bound -> simplex-form fallback == 10.
      {
        IntegerPolyhedron s(PresburgerSpace::getSetSpace(2, 0, 0));
        s.addInequality({1, 0, 0});
        s.addInequality({0, 1, 0});
        s.addInequality({-1, -1, 3});
        if (verify(cntB(s) == 10 && brute(s) == 10,
                   "M-count-2 2D simplex sum<=3 == 10"))
          return 1;
      }
      // Nonzero lower bounds across independent blocks: x in [2,5], y in [0,3].
      // (Exercises the block-local feasibility rule: an other-block lower bound
      // with a negative constant must NOT be read as global emptiness.)
      {
        IntegerPolyhedron b(PresburgerSpace::getSetSpace(2, 0, 0));
        b.addInequality({1, 0, -2}); // x >= 2
        b.addInequality({-1, 0, 5}); // x <= 5
        b.addInequality({0, 1, 0});  // y >= 0
        b.addInequality({0, -1, 3}); // y <= 3
        if (verify(cntB(b) == 16 && brute(b) == 16,
                   "M-count-2 nonzero-lb box x[2,5]y[0,3] == 16"))
          return 1;
      }
      // Separable-box regression: the block counter agrees with countSeparableBox.
      {
        IntegerPolyhedron b(PresburgerSpace::getSetSpace(3, 0, 0));
        for (unsigned a = 0; a < 3; ++a) {
          llvm::SmallVector<std::int64_t> lo(4, 0), up(4, 0);
          lo[a] = 1;
          b.addInequality(lo);
          up[a] = -1;
          up[3] = 3;
          b.addInequality(up);
        }
        if (verify(cntB(b) == 64 &&
                       countBlockSeparable(b) == countSeparableBox(b),
                   "M-count-2 box regression (blocks == separable-box)"))
          return 1;
      }
      // Empty triangular region (x>=0 and x<=-1) -> 0.
      {
        IntegerPolyhedron t(PresburgerSpace::getSetSpace(2, 0, 0));
        t.addInequality({0, 1, 0});   // y >= 0
        t.addInequality({1, -1, 0});  // y <= x
        t.addInequality({-1, 0, -1}); // x <= -1
        t.addInequality({1, 0, 0});   // x >= 0
        if (verify(cntB(t) == 0, "M-count-2 empty coupled region == 0"))
          return 1;
      }
      // Degenerate pinned axis x==2 inside 0<=y<=x -> y in {0,1,2} == 3.
      {
        IntegerPolyhedron t(PresburgerSpace::getSetSpace(2, 0, 0));
        t.addEquality({1, 0, -2}); // x == 2
        t.addInequality({0, 1, 0});
        t.addInequality({1, -1, 0});
        if (verify(cntB(t) == 3 && brute(t) == 3,
                   "M-count-2 degenerate pinned x==2, 0<=y<=x == 3"))
          return 1;
      }
      // Negative-orthant triangular -2<=y<=x<=0 (negative coefficients/coords).
      {
        IntegerPolyhedron t(PresburgerSpace::getSetSpace(2, 0, 0));
        t.addInequality({-1, 0, 0}); // x <= 0
        t.addInequality({1, 0, 2});  // x >= -2
        t.addInequality({1, -1, 0}); // y <= x
        t.addInequality({0, 1, 2});  // y >= -2
        if (verify(cntB(t) == brute(t) && brute(t) == 6,
                   "M-count-2 negative-orthant triangular == 6"))
          return 1;
      }
      // Large bounds vs CLOSED-FORM identity (no brute): triangular N=1e6.
      {
        long N = 1000000;
        IntegerPolyhedron t(PresburgerSpace::getSetSpace(2, 0, 0));
        t.addInequality({0, 1, 0});
        t.addInequality({1, -1, 0});
        t.addInequality({-1, 0, N});
        t.addInequality({1, 0, 0});
        BigInt ident = (BigInt(N) + 1) * (BigInt(N) + 2) / 2;
        if (verify(countBlockSeparable(t) == ident,
                   "M-count-2 large triangular N=1e6 == (N+1)(N+2)/2"))
          return 1;
      }
      // Large simplex vs binomial identity (no brute): 3D sum<=1000 == C(1003,3).
      {
        long M = 1000;
        IntegerPolyhedron s(PresburgerSpace::getSetSpace(3, 0, 0));
        s.addInequality({1, 0, 0, 0});
        s.addInequality({0, 1, 0, 0});
        s.addInequality({0, 0, 1, 0});
        s.addInequality({-1, -1, -1, M});
        BigInt ident = (BigInt(M) + 1) * (BigInt(M) + 2) * (BigInt(M) + 3) / 6;
        if (verify(countBlockSeparable(s) == ident,
                   "M-count-2 large 3D simplex sum<=1000 == C(1003,3)"))
          return 1;
      }
      // Overflow: a coefficient exceeding i64 -> UNKNOWN (never a wrong count).
      {
        IntegerPolyhedron b(PresburgerSpace::getSetSpace(1, 0, 0));
        llvm::DynamicAPInt huge(1000000000);
        huge = huge * huge * huge; // 1e27 > i64 max
        llvm::SmallVector<llvm::DynamicAPInt> row;
        row.push_back(huge);  // huge * x
        row.push_back(-huge); // - huge  (=> x >= 1)
        b.addInequality(row);
        b.addInequality({-1, 100}); // x <= 100
        if (verify(!countBlockSeparable(b).has_value(),
                   "M-count-2 i64-overflow coefficient -> UNKNOWN"))
          return 1;
      }
      // Non-unit coupling (|b|=2) -> outside the class -> UNKNOWN.
      {
        IntegerPolyhedron t(PresburgerSpace::getSetSpace(2, 0, 0));
        t.addInequality({1, 0, 0});  // x >= 0
        t.addInequality({-1, 0, 8}); // x <= 8
        t.addInequality({0, 1, 0});  // y >= 0
        t.addInequality({1, -2, 0}); // x - 2y >= 0  (non-unit coupling)
        if (verify(!countBlockSeparable(t).has_value(),
                   "M-count-2 non-unit coupling x-2y>=0 -> UNKNOWN"))
          return 1;
      }
      // 3-variable coupled block that is NOT a simplex -> UNKNOWN.
      {
        IntegerPolyhedron t(PresburgerSpace::getSetSpace(3, 0, 0));
        t.addInequality({1, 0, 0, 0});
        t.addInequality({0, 1, 0, 0});
        t.addInequality({0, 0, 1, 0});
        t.addInequality({-1, -1, 0, 3}); // x+y<=3 (couples x,y)
        t.addInequality({0, -1, -1, 3}); // y+z<=3 (couples y,z) -> not a simplex
        if (verify(!countBlockSeparable(t).has_value(),
                   "M-count-2 3-var non-simplex coupled block -> UNKNOWN"))
          return 1;
      }
      // Congruence-restricted guard partition: counted EXACTLY by the block
      // counter (box-only countPartition above returns UNKNOWN), cross-checked
      // against the brute-per-piece oracle.
      {
        auto Tm = sourceToLogicalMatrix(C1F, 3);
        IntegerPolyhedron dom = box(3, 3);
        dom.addInequality({-1, -1, 0, 3}); // i + j <= 3
        std::string why;
        auto part = buildSymbolicTilePartition(
            C1F, C1O, dom, 3,
            systolic::logical::ReductionOrderDecision::PreserveSourceOrder, why);
        if (verify(Tm.has_value() && part.has_value(),
                   "M-count-2 guard partition built"))
          return 1;
        long bruteFibers = 0, bruteMacs = 0;
        bool pieceOk = true;
        for (const auto &g : part->groups)
          for (const auto &P : g.anchorPieces) {
            auto c =
                countAnchorPieceFibersBlocks(P, *Tm, 3, part->congruenceLocals);
            long bb = brute(P);
            pieceOk &=
                (c.has_value() && (long)c->convert_to<std::int64_t>() == bb);
            bruteFibers += bb;
            bruteMacs += (long)g.length * bb;
          }
        if (verify(pieceOk,
                   "M-count-2 guard per-piece counter matches brute oracle"))
          return 1;
        auto counts = countPartitionBlocks(*part, *Tm, 3);
        if (verify(counts.has_value() &&
                       counts->outputFibers == BigInt(bruteFibers) &&
                       counts->sourceMACs == BigInt(bruteMacs),
                   "M-count-2 guard partition counted exactly (== brute)"))
          return 1;
        std::cout << "  M-count-2 guard: fibers=" << bruteFibers
                  << " macs=" << bruteMacs << "\n";
      }
      // Regression: on the CASE1 BOX partition the block path agrees with the
      // old box-only path (37 fibers / 64 MACs).
      {
        auto Tm = sourceToLogicalMatrix(C1F, 3);
        std::string why;
        auto part = buildSymbolicTilePartition(
            C1F, C1O, box(3, 3), 3,
            systolic::logical::ReductionOrderDecision::PreserveSourceOrder, why);
        if (verify(Tm.has_value() && part.has_value(),
                   "M-count-2 CASE1 partition built"))
          return 1;
        auto counts = countPartitionBlocks(*part, *Tm, 3);
        if (verify(counts.has_value() && counts->outputFibers == BigInt(37) &&
                       counts->sourceMACs == BigInt(64),
                   "M-count-2 CASE1 blocks path == 37/64 (box regression)"))
          return 1;
      }
      std::cout << "M-count-2 non-rectangular lattice-count tests passed.\n";
    }

    std::cout << "M-count symbolic lattice-count tests passed.\n";
  }

  // ========================================================
  // C4a-sym General Kernel-Coset Partition, M0 (exact integer kernel) + M1
  // (exact sourceMACs = |D| and outputFibers = |C_rows(D)|). Analysis-only; no
  // source-point enumeration in the counters. The brute oracles here are TEST
  // ONLY. Target: C[i+j-2k] += A[i][k]*B[k][j], 0<=i,j,k<4 -> 64 MACs/13 fibers.
  // ========================================================
  {
    using mlir::presburger::IntegerPolyhedron;
    using mlir::presburger::PresburgerSpace;
    using systolic::logical::buildIntegerKernelBasis;
    using systolic::logical::countKernelCosetCounts;
    auto box = [](std::vector<std::pair<long, long>> b) {
      unsigned r = static_cast<unsigned>(b.size());
      IntegerPolyhedron D(PresburgerSpace::getSetSpace(r, 0, 0));
      for (unsigned a = 0; a < r; ++a) {
        llvm::SmallVector<std::int64_t> lo(r + 1, 0), up(r + 1, 0);
        lo[a] = 1;
        lo[r] = -b[a].first;
        D.addInequality(lo);
        up[a] = -1;
        up[r] = b[a].second;
        D.addInequality(up);
      }
      return D;
    };
    // brute: distinct physical-C vectors (fibers) + total points (MACs).
    auto bruteCounts = [](const IntMatrix &C,
                          const std::vector<std::pair<long, long>> &b, long &fib,
                          long &mac) {
      unsigned r = static_cast<unsigned>(b.size());
      std::map<std::vector<long>, long> cs;
      long tot = 0;
      std::vector<long> x(r);
      std::function<void(unsigned)> rec = [&](unsigned d) {
        if (d == r) {
          std::vector<long> cv;
          for (const auto &row : C) {
            long s = 0;
            for (unsigned t = 0; t < r; ++t)
              s += (long)row[t].convert_to<std::int64_t>() * x[t];
            cv.push_back(s);
          }
          cs[cv]++;
          ++tot;
          return;
        }
        for (x[d] = b[d].first; x[d] <= b[d].second; ++x[d])
          rec(d + 1);
      };
      rec(0);
      fib = (long)cs.size();
      mac = tot;
    };
    // brute: the kernel columns generate the COMPLETE integer kernel over a box
    // (every integer v with C*v==0 is an integer combination of the basis).
    auto kernelComplete = [](const IntMatrix &C,
                             const std::vector<std::vector<BigInt>> &basis,
                             unsigned n) -> bool {
      long N = 4, M = 10;
      std::vector<long> v(n);
      std::function<bool(unsigned)> rec = [&](unsigned d) -> bool {
        if (d == n) {
          for (const auto &row : C) {
            long s = 0;
            for (unsigned t = 0; t < n; ++t)
              s += (long)row[t].convert_to<std::int64_t>() * v[t];
            if (s != 0)
              return true; // not a kernel vector: nothing to represent.
          }
          if (basis.empty()) {
            for (unsigned t = 0; t < n; ++t)
              if (v[t] != 0)
                return false;
            return true;
          }
          if (basis.size() == 1) {
            for (long a = -M; a <= M; ++a) {
              bool ok = true;
              for (unsigned t = 0; t < n; ++t)
                if ((long)basis[0][t].convert_to<std::int64_t>() * a != v[t])
                  ok = false;
              if (ok)
                return true;
            }
            return false;
          }
          if (basis.size() == 2) {
            for (long a = -M; a <= M; ++a)
              for (long c = -M; c <= M; ++c) {
                bool ok = true;
                for (unsigned t = 0; t < n; ++t) {
                  long s = (long)basis[0][t].convert_to<std::int64_t>() * a +
                           (long)basis[1][t].convert_to<std::int64_t>() * c;
                  if (s != v[t])
                    ok = false;
                }
                if (ok)
                  return true;
              }
            return false;
          }
          return true; // >2 basis columns not exercised here.
        }
        for (v[d] = -N; v[d] <= N; ++v[d])
          if (!rec(d + 1))
            return false;
        return true;
      };
      return rec(0);
    };

    // ---- M0: exact integer kernel foundation ----
    {
      IntMatrix C = {{BigInt(1), BigInt(1), BigInt(-2)}};
      auto kb = buildIntegerKernelBasis(C, 3);
      if (verify(kb.has_value() && kb->outputRank == 1 && kb->kernelRank == 2 &&
                     kb->kernelColumns.size() == 2,
                 "M0 kernel of [1,1,-2]: rank 1, kappa 2"))
        return 1;
      if (verify(kernelComplete(C, kb->kernelColumns, 3),
                 "M0 kernel basis is the COMPLETE integer kernel (brute)"))
        return 1;
    }
    {
      IntMatrix C = {{BigInt(1), BigInt(0), BigInt(0)},
                     {BigInt(0), BigInt(1), BigInt(0)}};
      auto kb = buildIntegerKernelBasis(C, 3);
      if (verify(kb.has_value() && kb->outputRank == 2 && kb->kernelRank == 1 &&
                     kernelComplete(C, kb->kernelColumns, 3),
                 "M0 rank-2 map: rank 2, kappa 1, complete"))
        return 1;
    }
    {
      IntMatrix C = {{BigInt(0), BigInt(0), BigInt(0)}};
      auto kb = buildIntegerKernelBasis(C, 3);
      if (verify(kb.has_value() && kb->outputRank == 0 && kb->kernelRank == 3,
                 "M0 zero map: rank 0, kappa 3"))
        return 1;
    }
    {
      BigInt huge = BigInt(1000000000) * BigInt(1000000000) * BigInt(1000000000);
      IntMatrix C = {{huge, BigInt(1), BigInt(-2)}};
      if (verify(!buildIntegerKernelBasis(C, 3).has_value(),
                 "M0 i64-overflow coefficient -> UNKNOWN"))
        return 1;
    }

    // ---- M1: exact counts vs brute-by-physical-C ----
    auto check = [&](const char *nm, IntMatrix C,
                     std::vector<std::pair<long, long>> b, long expMac,
                     long expFib) -> int {
      auto D = box(b);
      auto c = countKernelCosetCounts(C, D, static_cast<unsigned>(b.size()));
      long bf, bm;
      bruteCounts(C, b, bf, bm);
      return verify(c.has_value() && c->sourceMACs == BigInt(expMac) &&
                        c->outputFibers == BigInt(expFib) &&
                        c->sourceMACs == BigInt(bm) &&
                        c->outputFibers == BigInt(bf),
                    nm);
    };
    if (check("M1 target C[i+j-2k]: 64 MACs / 13 fibers == brute",
              {{BigInt(1), BigInt(1), BigInt(-2)}}, {{0, 3}, {0, 3}, {0, 3}}, 64,
              13))
      return 1;
    if (check("M1 singular 2-row C: 64 / 13 == brute",
              {{BigInt(1), BigInt(1), BigInt(-2)},
               {BigInt(0), BigInt(0), BigInt(0)}},
              {{0, 3}, {0, 3}, {0, 3}}, 64, 13))
      return 1;
    if (check("M1 nonprimitive 2*[1,1,-2]: 64 / 13 == brute",
              {{BigInt(2), BigInt(2), BigInt(-4)}}, {{0, 3}, {0, 3}, {0, 3}}, 64,
              13))
      return 1;
    if (check("M1 rank-0 constant C: 64 / 1 == brute",
              {{BigInt(0), BigInt(0), BigInt(0)}}, {{0, 3}, {0, 3}, {0, 3}}, 64,
              1))
      return 1;

    // ---- M1 negative cases: UNKNOWN / fail closed ----
    {
      auto D = box({{0, 1}, {0, 1}}); // image of [1,3] = {0,1,3,4}: hole at 2.
      if (verify(
              !countKernelCosetCounts({{BigInt(1), BigInt(3)}}, D, 2).has_value(),
              "M1 holey image [1,3] -> UNKNOWN (not the convex length 5)"))
        return 1;
    }
    {
      auto D = box({{0, 3}, {0, 3}, {0, 3}});
      if (verify(!countKernelCosetCounts({{BigInt(1), BigInt(0), BigInt(0)},
                                          {BigInt(0), BigInt(1), BigInt(0)}},
                                         D, 3)
                      .has_value(),
                 "M1 rank-2 image -> UNKNOWN"))
        return 1;
    }
    {
      // non-box (triangular) domain -> image certificate fails closed.
      IntegerPolyhedron T(PresburgerSpace::getSetSpace(2, 0, 0));
      T.addInequality({1, 0, 0});
      T.addInequality({0, 1, 0});
      T.addInequality({-1, -1, 3});
      if (verify(
              !countKernelCosetCounts({{BigInt(1), BigInt(1)}}, T, 2).has_value(),
              "M1 non-box (triangular) domain -> UNKNOWN"))
        return 1;
    }
    {
      IntegerPolyhedron D(PresburgerSpace::getSetSpace(2, 0, 0));
      D.addInequality({1, 0, -5}); // x >= 5
      D.addInequality({-1, 0, 0}); // x <= 0  -> empty
      D.addInequality({0, 1, 0});
      D.addInequality({0, -1, 3});
      auto c = countKernelCosetCounts({{BigInt(1), BigInt(1)}}, D, 2);
      if (verify(c.has_value() && c->sourceMACs == BigInt(0) &&
                     c->outputFibers == BigInt(0),
                 "M1 empty domain -> 0 / 0"))
        return 1;
    }
    {
      BigInt huge = BigInt(1000000000) * BigInt(1000000000) * BigInt(1000000000);
      auto D = box({{0, 3}, {0, 3}, {0, 3}});
      if (verify(!countKernelCosetCounts({{huge, BigInt(1), BigInt(-2)}}, D, 3)
                      .has_value(),
                 "M1 overflow coefficient -> UNKNOWN"))
        return 1;
    }
    std::cout << "C4a-sym kernel-coset M0/M1 tests passed.\n";
  }

  // ========================================================
  // C4a-sym Kernel-Coset Partition M2a: exact symbolic fiber representation
  // (KernelCosetFiberPlan over the reduced domain D' = {y | U*y in D}).
  // Analysis-only; no source-point enumeration in production. The brute-by-
  // physical-C oracle here is TEST ONLY. Target: C[i+j-2k] += A[i][k]*B[k][j],
  // 0<=i,j,k<4 -> 64 MACs / 13 fibers, kernel rank 2, ragged fibers.
  // ========================================================
  {
    using mlir::presburger::BoundType;
    using mlir::presburger::IntegerPolyhedron;
    using mlir::presburger::PresburgerSpace;
    using systolic::logical::buildKernelCosetFiberPlan;
    using systolic::logical::countKernelCosetCounts;
    using systolic::logical::fiberSlice;
    using systolic::logical::SourceOrderStatus;

    auto box3 = []() {
      IntegerPolyhedron D(PresburgerSpace::getSetSpace(3, 0, 0));
      for (unsigned a = 0; a < 3; ++a) {
        llvm::SmallVector<std::int64_t> lo(4, 0), up(4, 0);
        lo[a] = 1;
        D.addInequality(lo);
        up[a] = -1;
        up[3] = 3;
        D.addInequality(up);
      }
      return D;
    };
    // map x = U*y (row i of U dotted with y).
    auto Ux = [](const IntMatrix &U, unsigned i, const std::vector<long> &y) {
      long s = 0;
      for (unsigned t = 0; t < y.size(); ++t)
        s += (long)U[i][t].convert_to<std::int64_t>() * y[t];
      return s;
    };
    // membership of an integer assignment in a 0-local polyhedron.
    auto inPoly = [](const IntegerPolyhedron &P, const std::vector<long> &y) {
      unsigned nd = P.getNumDimVars();
      if (P.getNumLocalVars())
        return false;
      for (unsigned r = 0; r < P.getNumInequalities(); ++r) {
        auto row = P.getInequality(r);
        long s = (long)(std::int64_t)row[nd];
        for (unsigned j = 0; j < nd; ++j)
          s += (long)(std::int64_t)row[j] * y[j];
        if (s < 0)
          return false;
      }
      for (unsigned r = 0; r < P.getNumEqualities(); ++r) {
        auto row = P.getEquality(r);
        long s = (long)(std::int64_t)row[nd];
        for (unsigned j = 0; j < nd; ++j)
          s += (long)(std::int64_t)row[j] * y[j];
        if (s != 0)
          return false;
      }
      return true;
    };

    IntMatrix C = {{BigInt(1), BigInt(1), BigInt(-2)}};
    IntegerPolyhedron D = box3();

    // independent brute oracle: group box points by physical C = i+j-2k.
    std::map<long, std::set<std::vector<long>>> bruteFib;
    long bruteMac = 0;
    for (long i = 0; i < 4; ++i)
      for (long j = 0; j < 4; ++j)
        for (long k = 0; k < 4; ++k) {
          bruteFib[i + j - 2 * k].insert({i, j, k});
          ++bruteMac;
        }

    auto plan = buildKernelCosetFiberPlan(C, D, 3);
    if (verify(plan.has_value() && plan->outputRank == 1 &&
                   plan->kernelRank == 2 &&
                   plan->sourceMACs == BigInt(64) &&
                   plan->outputFibers == BigInt(13) &&
                   plan->sourceOrder == SourceOrderStatus::Unverified,
               "M2a plan: rank 1 / kappa 2 / 64 MACs / 13 fibers / order "
               "Unverified"))
      return 1;
    if (verify(plan->outputCols.size() == 1 && plan->outputCols[0] == 0 &&
                   plan->reductionCols.size() == 2 &&
                   plan->reductionCols[0] == 1 && plan->reductionCols[1] == 2,
               "M2a plan output/reduction column split"))
      return 1;
    // U * Uinv == I (exact integer inverse).
    {
      bool ok = true;
      for (unsigned i = 0; i < 3 && ok; ++i)
        for (unsigned j = 0; j < 3; ++j) {
          long s = 0;
          for (unsigned t = 0; t < 3; ++t)
            s += (long)plan->U[i][t].convert_to<std::int64_t>() *
                 (long)plan->Uinv[t][j].convert_to<std::int64_t>();
          if (s != (i == j ? 1 : 0))
            ok = false;
        }
      if (verify(ok, "M2a U * Uinv == I"))
        return 1;
    }

    // y-bounding box of D'.
    const IntegerPolyhedron &Dp = plan->reducedDomain;
    std::vector<long> ylo(3), yhi(3);
    {
      bool bounded = true;
      for (unsigned d = 0; d < 3; ++d) {
        auto l = Dp.getConstantBound64(BoundType::LB, d);
        auto h = Dp.getConstantBound64(BoundType::UB, d);
        if (!l || !h)
          bounded = false;
        else {
          ylo[d] = *l;
          yhi[d] = *h;
        }
      }
      if (verify(bounded, "M2a reduced domain D' is bounded"))
        return 1;
    }

    // D' bijection + physical-C grouping == brute (coverage/disjointness/
    // multiplicity of the whole partition).
    {
      std::map<long, std::set<std::vector<long>>> symFib;
      long symMac = 0;
      std::vector<long> y(3);
      for (y[0] = ylo[0]; y[0] <= yhi[0]; ++y[0])
        for (y[1] = ylo[1]; y[1] <= yhi[1]; ++y[1])
          for (y[2] = ylo[2]; y[2] <= yhi[2]; ++y[2]) {
            if (!inPoly(Dp, y))
              continue;
            std::vector<long> x = {Ux(plan->U, 0, y), Ux(plan->U, 1, y),
                                   Ux(plan->U, 2, y)};
            symFib[x[0] + x[1] - 2 * x[2]].insert(x);
            ++symMac;
          }
      if (verify(symMac == bruteMac && symFib == bruteFib,
                 "M2a D' bijection + physical-C grouping == brute (64/13)"))
        return 1;
    }

    // fiberSlice: each pinned output coordinate reconstructs exactly one brute
    // fiber (per-fiber cardinality, reconstruction, unique physical-C owner).
    {
      long nonEmpty = 0;
      bool sliceOk = true;
      auto y0l = Dp.getConstantBound64(BoundType::LB, plan->outputCols[0]);
      auto y0h = Dp.getConstantBound64(BoundType::UB, plan->outputCols[0]);
      for (long y0 = *y0l; y0 <= *y0h && sliceOk; ++y0) {
        std::int64_t yo[1] = {(std::int64_t)y0};
        auto sl = fiberSlice(*plan, llvm::ArrayRef<std::int64_t>(yo, 1));
        if (!sl) {
          sliceOk = false;
          break;
        }
        std::set<std::vector<long>> pts;
        long cphys = 0;
        bool any = false;
        std::vector<long> yy(3);
        for (yy[0] = ylo[0]; yy[0] <= yhi[0]; ++yy[0])
          for (yy[1] = ylo[1]; yy[1] <= yhi[1]; ++yy[1])
            for (yy[2] = ylo[2]; yy[2] <= yhi[2]; ++yy[2]) {
              if (!inPoly(*sl, yy))
                continue;
              std::vector<long> x = {Ux(plan->U, 0, yy), Ux(plan->U, 1, yy),
                                     Ux(plan->U, 2, yy)};
              cphys = x[0] + x[1] - 2 * x[2];
              pts.insert(x);
              any = true;
            }
        if (!any)
          continue;
        ++nonEmpty;
        if (bruteFib.find(cphys) == bruteFib.end() || bruteFib[cphys] != pts)
          sliceOk = false;
      }
      if (verify(sliceOk && nonEmpty == 13,
                 "M2a fiberSlice reconstructs all 13 fibers exactly == brute"))
        return 1;
    }

    // Negatives: fail closed to UNKNOWN (no plan).
    {
      // holey image [1,3] over [0,1]^2 -> M1 image UNKNOWN -> no plan.
      IntegerPolyhedron H(PresburgerSpace::getSetSpace(2, 0, 0));
      H.addInequality({1, 0, 0});
      H.addInequality({-1, 0, 1});
      H.addInequality({0, 1, 0});
      H.addInequality({0, -1, 1});
      if (verify(!buildKernelCosetFiberPlan({{BigInt(1), BigInt(3)}}, H, 2)
                      .has_value(),
                 "M2a holey image -> UNKNOWN (no plan)"))
        return 1;
    }
    {
      // rank-2 output image -> UNKNOWN.
      IntegerPolyhedron b = box3();
      if (verify(!buildKernelCosetFiberPlan({{BigInt(1), BigInt(0), BigInt(0)},
                                             {BigInt(0), BigInt(1), BigInt(0)}},
                                            b, 3)
                      .has_value(),
                 "M2a rank-2 image -> UNKNOWN (no plan)"))
        return 1;
    }
    {
      // non-box (triangular) domain -> M1 image certificate UNKNOWN -> no plan.
      IntegerPolyhedron T(PresburgerSpace::getSetSpace(2, 0, 0));
      T.addInequality({1, 0, 0});
      T.addInequality({0, 1, 0});
      T.addInequality({-1, -1, 3});
      if (verify(
              !buildKernelCosetFiberPlan({{BigInt(1), BigInt(1)}}, T, 2)
                   .has_value(),
              "M2a non-box domain -> UNKNOWN (no plan)"))
        return 1;
    }
    {
      // i64-overflow coefficient -> UNKNOWN (kernel/inverse rejected).
      BigInt huge = BigInt(1000000000) * BigInt(1000000000) * BigInt(1000000000);
      IntegerPolyhedron b = box3();
      if (verify(!buildKernelCosetFiberPlan({{huge, BigInt(1), BigInt(-2)}}, b, 3)
                      .has_value(),
                 "M2a overflow coefficient -> UNKNOWN (no plan)"))
        return 1;
    }
    std::cout << "C4a-sym kernel-coset M2a fiber-representation tests passed.\n";
  }

  // ========================================================
  // C4a-sym Kernel-Coset Partition M3: exact source-order certification.
  // Certifies whether the plan's y_red-lex traversal accumulates every
  // physical-C fiber in the original source (x-lex) order. Analysis-only; the
  // brute same-fiber reversal oracle here is TEST ONLY. The certificate never
  // consults fastmath, so NotSourceLexEquivalent is strict-FP-illegal and is
  // never silently reassociated.
  // ========================================================
  {
    using mlir::presburger::IntegerPolyhedron;
    using mlir::presburger::PresburgerSpace;
    using systolic::logical::buildKernelCosetFiberPlan;
    using systolic::logical::certifyKernelCosetSourceOrder;
    using systolic::logical::KernelCosetFiberPlan;
    using systolic::logical::OrderVerdict;

    auto boxR = [](int r, int hi) {
      IntegerPolyhedron D(PresburgerSpace::getSetSpace(r, 0, 0));
      for (int a = 0; a < r; ++a) {
        llvm::SmallVector<std::int64_t> l(r + 1, 0), u(r + 1, 0);
        l[a] = 1;
        D.addInequality(l);
        u[a] = -1;
        u[r] = hi;
        D.addInequality(u);
      }
      return D;
    };
    // physical-C key of x (full C vector).
    auto ckey = [](const IntMatrix &C, const std::vector<long> &x) {
      std::vector<long> k;
      for (const auto &row : C) {
        long s = 0;
        for (size_t t = 0; t < x.size(); ++t)
          s += (long)row[t].convert_to<std::int64_t>() * x[t];
        k.push_back(s);
      }
      return k;
    };
    // y_red-lex comparison of a vs b: -1/0/1.
    auto ylex = [](const IntMatrix &Uinv, const std::vector<unsigned> &rc,
                   const std::vector<long> &a, const std::vector<long> &b) {
      for (unsigned k : rc) {
        long ya = 0, yb = 0;
        for (size_t t = 0; t < a.size(); ++t) {
          ya += (long)Uinv[k][t].convert_to<std::int64_t>() * a[t];
          yb += (long)Uinv[k][t].convert_to<std::int64_t>() * b[t];
        }
        if (ya < yb)
          return -1;
        if (ya > yb)
          return 1;
      }
      return 0;
    };
    auto xlex = [](const std::vector<long> &a, const std::vector<long> &b) {
      for (size_t t = 0; t < a.size(); ++t) {
        if (a[t] < b[t])
          return -1;
        if (a[t] > b[t])
          return 1;
      }
      return 0;
    };
    // brute: does a SAME-fiber pair reverse (x-lex before, y_red-lex after)?
    auto bruteSameFiberReversal =
        [&](const IntMatrix &C, const IntMatrix &Uinv,
            const std::vector<unsigned> &rc, int r, int hi) {
          std::map<std::vector<long>, std::vector<std::vector<long>>> fib;
          std::vector<long> x(r);
          std::function<void(int)> rec = [&](int d) {
            if (d == r) {
              fib[ckey(C, x)].push_back(x);
              return;
            }
            for (x[d] = 0; x[d] <= hi; ++x[d])
              rec(d + 1);
          };
          rec(0);
          for (auto &kv : fib)
            for (auto &a : kv.second)
              for (auto &b : kv.second)
                if (xlex(a, b) < 0 && ylex(Uinv, rc, a, b) > 0)
                  return true;
          return false;
        };

    // Verify a plan's certificate matches the brute same-fiber reversal oracle,
    // and (when NotEquivalent) that the witness is a genuine same-fiber reversal.
    auto checkC = [&](const char *nm, IntMatrix C, OrderVerdict expect) -> int {
      int r = (int)C[0].size();
      IntegerPolyhedron D = boxR(r, 3);
      auto plan = buildKernelCosetFiberPlan(C, D, r);
      if (verify(plan.has_value(), nm))
        return 1;
      auto cert = certifyKernelCosetSourceOrder(*plan);
      bool rev =
          bruteSameFiberReversal(C, plan->Uinv, plan->reductionCols, r, 3);
      bool verdictOk =
          (cert.verdict == OrderVerdict::SourceLexEquivalent && !rev) ||
          (cert.verdict == OrderVerdict::NotSourceLexEquivalent && rev);
      if (verify(verdictOk && cert.verdict == expect, nm))
        return 1;
      if (cert.verdict == OrderVerdict::NotSourceLexEquivalent) {
        // witness: distinct, same physical C, source-earlier, traversal-later.
        bool w = !cert.earlier.empty() && !cert.later.empty() &&
                 cert.earlier != cert.later &&
                 ckey(C, cert.earlier) == ckey(C, cert.later) &&
                 xlex(cert.earlier, cert.later) < 0 &&
                 ylex(plan->Uinv, plan->reductionCols, cert.earlier,
                      cert.later) > 0;
        if (verify(w, nm))
          return 1;
      }
      return 0;
    };

    // Identity transformation preserves source order.
    if (checkC("M3 identity [1,0,0] -> SourceLexEquivalent",
               {{BigInt(1), BigInt(0), BigInt(0)}},
               OrderVerdict::SourceLexEquivalent))
      return 1;
    // A lexicographically compatible NON-identity unimodular U preserves order.
    if (checkC("M3 lex-compatible non-identity [0,1,0] -> SourceLexEquivalent",
               {{BigInt(0), BigInt(1), BigInt(0)}},
               OrderVerdict::SourceLexEquivalent))
      return 1;
    // A unimodular U that reverses source order is rejected (with a witness).
    if (checkC("M3 order-reversing [1,-1] -> NotSourceLexEquivalent",
               {{BigInt(1), BigInt(-1)}},
               OrderVerdict::NotSourceLexEquivalent))
      return 1;
    // The skew singular-output target against its exact fiber traversal.
    if (checkC("M3 skew target [1,1,-2] -> NotSourceLexEquivalent + witness",
               {{BigInt(1), BigInt(1), BigInt(-2)}},
               OrderVerdict::NotSourceLexEquivalent))
      return 1;

    // Distinct physical-C fibers are not incorrectly constrained against one
    // another: identity [1,0,0] has CROSS-fiber reversals, yet the certificate
    // (same-fiber only) is SourceLexEquivalent.
    {
      IntMatrix C = {{BigInt(1), BigInt(0), BigInt(0)}};
      auto plan = buildKernelCosetFiberPlan(C, boxR(3, 3), 3);
      // brute cross-fiber reversal: a pair in DIFFERENT fibers, x-lex vs y_red.
      bool crossRev = false;
      std::vector<std::vector<long>> all;
      for (long i = 0; i < 4; ++i)
        for (long j = 0; j < 4; ++j)
          for (long k = 0; k < 4; ++k)
            all.push_back({i, j, k});
      for (auto &a : all)
        for (auto &b : all)
          if (ckey(C, a) != ckey(C, b) && xlex(a, b) < 0 &&
              ylex(plan->Uinv, plan->reductionCols, a, b) > 0)
            crossRev = true;
      auto cert = certifyKernelCosetSourceOrder(*plan);
      if (verify(crossRev &&
                     cert.verdict == OrderVerdict::SourceLexEquivalent,
                 "M3 distinct fibers not cross-constrained (cross-reversal "
                 "exists but same-fiber is clean)"))
        return 1;
    }

    // Congruence-restricted domain handled exactly: D' has a congruence local
    // (y1 even). The traversal (y1-lex) matches source order -> Equivalent, and
    // the violation system carries the locals through isIntegerEmpty.
    {
      IntegerPolyhedron Dp(PresburgerSpace::getSetSpace(2, 0, 1)); // +1 local q
      Dp.addInequality({1, 0, 0, 0});  // y0 >= 0
      Dp.addInequality({-1, 0, 0, 3}); // y0 <= 3
      Dp.addInequality({0, 1, 0, 0});  // y1 >= 0
      Dp.addInequality({0, -1, 0, 5}); // y1 <= 5
      Dp.addEquality({0, 1, -2, 0});   // y1 == 2q  (y1 even)
      KernelCosetFiberPlan plan(std::move(Dp));
      plan.U = {{BigInt(1), BigInt(0)}, {BigInt(0), BigInt(1)}};
      plan.Uinv = {{BigInt(1), BigInt(0)}, {BigInt(0), BigInt(1)}};
      plan.outputRank = 1;
      plan.kernelRank = 1;
      plan.outputCols = {0};
      plan.reductionCols = {1};
      auto cert = certifyKernelCosetSourceOrder(plan);
      // brute: y0 in [0,3], y1 in {0,2,4}, fiber = y0; (y0,y1)-lex vs y1-lex.
      bool rev = false;
      for (int y0 = 0; y0 <= 3; ++y0)
        for (int a = 0; a <= 5; a += 2)
          for (int b = 0; b <= 5; b += 2)
            if (std::make_pair(y0, a) < std::make_pair(y0, b) && a > b)
              rev = true;
      if (verify(cert.verdict == OrderVerdict::SourceLexEquivalent && !rev,
                 "M3 congruence-restricted domain (y1 even) -> Equivalent"))
        return 1;
    }

    // Unprovable/overflow -> UNKNOWN (legality never inferred).
    {
      BigInt huge = BigInt(1000000000) * BigInt(1000000000) * BigInt(1000000000);
      IntegerPolyhedron Dp = boxR(2, 3);
      KernelCosetFiberPlan plan(std::move(Dp));
      plan.U = {{BigInt(1), BigInt(0)}, {BigInt(0), BigInt(1)}};
      plan.Uinv = {{huge, BigInt(0)}, {BigInt(0), BigInt(1)}};
      plan.outputRank = 1;
      plan.kernelRank = 1;
      plan.outputCols = {0};
      plan.reductionCols = {1};
      if (verify(certifyKernelCosetSourceOrder(plan).verdict ==
                     OrderVerdict::Unknown,
                 "M3 i64-overflow -> UNKNOWN (never inferred legal)"))
        return 1;
    }

    // Strict FP is never silently reassociated: the skew target's verdict is
    // NotSourceLexEquivalent regardless of any flag (the certificate consults
    // no fastmath), so a strict-FP reordered reduction stays illegal.
    {
      auto plan = buildKernelCosetFiberPlan(
          {{BigInt(1), BigInt(1), BigInt(-2)}}, boxR(3, 3), 3);
      if (verify(certifyKernelCosetSourceOrder(*plan).verdict ==
                     OrderVerdict::NotSourceLexEquivalent,
                 "M3 strict-FP: skew target stays NotSourceLexEquivalent"))
        return 1;
    }
    std::cout << "C4a-sym kernel-coset M3 source-order tests passed.\n";
  }

  // ========================================================
  // C4a-sym Kernel-Coset Partition M4a: symbolic peel-and-tile scan plan.
  // Analysis-only; no IR emission. The ragged convex fiber is represented via
  // D' + the peel/tile ordering, and a TILE is the slice of D' with the output
  // and peel coordinates pinned (the tile axis free -> a contiguous interval).
  // Validated vs a brute peel-and-tile reconstruction (TEST ONLY). Target
  // C[i+j-2k]: 13 fibers / 64 MACs, kernel rank 2, static-K tileable, source
  // order NotSourceLexEquivalent (recorded, never used to reorder).
  // ========================================================
  {
    using mlir::presburger::BoundType;
    using mlir::presburger::IntegerPolyhedron;
    using mlir::presburger::PresburgerSpace;
    using systolic::logical::buildKernelCosetScanPlan;
    using systolic::logical::fiberSlice;
    using systolic::logical::OrderVerdict;
    using systolic::logical::scanTileSlice;
    using systolic::logical::TileLegality;

    auto boxR = [](int r, int hi) {
      IntegerPolyhedron D(PresburgerSpace::getSetSpace(r, 0, 0));
      for (int a = 0; a < r; ++a) {
        llvm::SmallVector<std::int64_t> l(r + 1, 0), u(r + 1, 0);
        l[a] = 1;
        D.addInequality(l);
        u[a] = -1;
        u[r] = hi;
        D.addInequality(u);
      }
      return D;
    };
    auto Ux = [](const IntMatrix &U, unsigned i, const std::vector<long> &y) {
      long s = 0;
      for (unsigned t = 0; t < y.size(); ++t)
        s += (long)U[i][t].convert_to<std::int64_t>() * y[t];
      return s;
    };
    auto inPoly = [](const IntegerPolyhedron &P, const std::vector<long> &y) {
      unsigned nd = P.getNumDimVars();
      if (P.getNumLocalVars())
        return false;
      for (unsigned r = 0; r < P.getNumInequalities(); ++r) {
        auto row = P.getInequality(r);
        long s = (long)(std::int64_t)row[nd];
        for (unsigned j = 0; j < nd; ++j)
          s += (long)(std::int64_t)row[j] * y[j];
        if (s < 0)
          return false;
      }
      for (unsigned r = 0; r < P.getNumEqualities(); ++r) {
        auto row = P.getEquality(r);
        long s = (long)(std::int64_t)row[nd];
        for (unsigned j = 0; j < nd; ++j)
          s += (long)(std::int64_t)row[j] * y[j];
        if (s != 0)
          return false;
      }
      return true;
    };

    IntMatrix C = {{BigInt(1), BigInt(1), BigInt(-2)}};
    auto plan = buildKernelCosetScanPlan(C, boxR(3, 3), 3);
    if (verify(plan.has_value() && plan->fiber.kernelRank == 2 &&
                   plan->peelOrder.size() == 2 && plan->peelOrder[0] == 1 &&
                   plan->peelOrder[1] == 2 && plan->innerStride == 1 &&
                   plan->sourceOrder == OrderVerdict::NotSourceLexEquivalent &&
                   plan->tileLegality == TileLegality::StaticKTileable &&
                   plan->fiber.sourceMACs == BigInt(64) &&
                   plan->fiber.outputFibers == BigInt(13),
               "M4a scan plan: kappa 2, peel/tile [1,2], static-K, NotSourceLex, "
               "64/13"))
      return 1;

    // Independent brute fibers (box grouped by physical C = i+j-2k).
    std::map<long, std::set<std::vector<long>>> brute;
    for (long i = 0; i < 4; ++i)
      for (long j = 0; j < 4; ++j)
        for (long k = 0; k < 4; ++k)
          brute[i + j - 2 * k].insert({i, j, k});

    const IntegerPolyhedron &Dp = plan->fiber.reducedDomain;
    std::vector<long> ylo(3), yhi(3);
    {
      bool bounded = true;
      for (unsigned d = 0; d < 3; ++d) {
        auto l = Dp.getConstantBound64(BoundType::LB, d);
        auto h = Dp.getConstantBound64(BoundType::UB, d);
        if (!l || !h)
          bounded = false;
        else {
          ylo[d] = *l;
          yhi[d] = *h;
        }
      }
      if (verify(bounded, "M4a reduced domain bounded"))
        return 1;
    }
    unsigned oc = plan->fiber.outputCols[0];
    unsigned peel = plan->peelOrder[0];
    unsigned tile = plan->peelOrder[1];

    // Peel-and-tile reconstruction: iterate output, peel; the tile axis is the
    // free interval of scanTileSlice. Verify tiles are contiguous intervals,
    // pairwise disjoint, and reconstruct every brute fiber exactly.
    std::map<long, std::set<std::vector<long>>> rebuilt;
    std::set<std::vector<long>> seen;
    long rebuiltMac = 0;
    bool intervals = true, disjoint = true, sliceOk = true;
    for (long c = ylo[oc]; c <= yhi[oc] && sliceOk; ++c) {
      std::int64_t ov[1] = {(std::int64_t)c};
      auto fib = fiberSlice(plan->fiber, llvm::ArrayRef<std::int64_t>(ov, 1));
      if (!fib) {
        sliceOk = false;
        break;
      }
      auto pl = fib->getConstantBound64(BoundType::LB, peel);
      auto ph = fib->getConstantBound64(BoundType::UB, peel);
      if (!pl || !ph)
        continue;
      for (long p = *pl; p <= *ph; ++p) {
        std::int64_t pv[1] = {(std::int64_t)p};
        auto ts = scanTileSlice(*plan, llvm::ArrayRef<std::int64_t>(ov, 1),
                                llvm::ArrayRef<std::int64_t>(pv, 1));
        if (!ts) {
          sliceOk = false;
          break;
        }
        auto tl = ts->getConstantBound64(BoundType::LB, tile);
        auto th = ts->getConstantBound64(BoundType::UB, tile);
        if (!tl || !th || *th < *tl)
          continue;
        for (long t = *tl; t <= *th; ++t) {
          std::vector<long> y(3, 0);
          y[oc] = c;
          y[peel] = p;
          y[tile] = t;
          if (!inPoly(*ts, y)) {
            intervals = false; // a gap inside the tile interval
            continue;
          }
          std::vector<long> x = {Ux(plan->fiber.U, 0, y), Ux(plan->fiber.U, 1, y),
                                 Ux(plan->fiber.U, 2, y)};
          if (seen.count(x))
            disjoint = false;
          seen.insert(x);
          rebuilt[x[0] + x[1] - 2 * x[2]].insert(x);
          ++rebuiltMac;
        }
      }
    }
    if (verify(sliceOk && intervals && disjoint && rebuilt == brute &&
                   rebuiltMac == 64 && rebuilt.size() == 13,
               "M4a peel-and-tile reconstructs 13 fibers / 64 MACs exactly == "
               "brute (contiguous disjoint tiles)"))
      return 1;

    // Negatives -> no scan plan (UNKNOWN / fail closed).
    if (verify(!buildKernelCosetScanPlan(
                    {{BigInt(1), BigInt(0), BigInt(0), BigInt(0)}}, boxR(4, 3), 4)
                    .has_value(),
               "M4a kappa>2 (rank-1 C over 4D) -> UNKNOWN"))
      return 1;
    if (verify(!buildKernelCosetScanPlan({{BigInt(1), BigInt(3)}}, boxR(2, 1), 2)
                    .has_value(),
               "M4a holey image -> UNKNOWN"))
      return 1;
    if (verify(!buildKernelCosetScanPlan({{BigInt(1), BigInt(0), BigInt(0)},
                                          {BigInt(0), BigInt(1), BigInt(0)}},
                                         boxR(3, 3), 3)
                    .has_value(),
               "M4a rank-2 image -> UNKNOWN"))
      return 1;

    // A lex-compatible kernel (identity [1,0,0]) yields a scan plan whose
    // recorded source order IS SourceLexEquivalent (so M4 could offload it under
    // strict FP); the skew target above stays NotSourceLexEquivalent.
    {
      auto id = buildKernelCosetScanPlan({{BigInt(1), BigInt(0), BigInt(0)}},
                                         boxR(3, 3), 3);
      if (verify(id.has_value() &&
                     id->sourceOrder == OrderVerdict::SourceLexEquivalent &&
                     id->tileLegality == TileLegality::StaticKTileable,
                 "M4a identity kernel: SourceLexEquivalent + static-K"))
        return 1;
    }
    std::cout << "C4a-sym kernel-coset M4a scan-plan tests passed.\n";
  }

  std::cout << "All coordinate relation tests passed.\n";
  return 0;
}
