//===- RankPartitionDeviceJobLowering.cpp - Certified GEMM to device jobs -===//
//
// Phase 5.4.3d-3b-6. See the header and docs/AFFINE_GEMM_MAPPING.md
// section 27.
//
//===----------------------------------------------------------------------===//

#include "Systolic/RankPartitionDeviceJobLowering.h"

#include "Systolic/CostModel.h"
#include "Systolic/RankPartitionCommonFiber.h"
#include "Systolic/RankPartitionExactInclusion.h"
#include "Systolic/RankPartitionGuardedLoopNestEmission.h"
#include "Systolic/SystolicOps.h"

#include "mlir/Analysis/Presburger/IntegerRelation.h"
#include "mlir/Analysis/Presburger/PresburgerSpace.h"
#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Bufferization/IR/Bufferization.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/Tensor/IR/Tensor.h"
#include "mlir/IR/AffineExpr.h"
#include "mlir/IR/AffineMap.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/Verifier.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/raw_ostream.h"

#include <limits>

using namespace mlir;

namespace mlir::systolic {

namespace {

using ::systolic::logical::BigInt;
using ::systolic::logical::GuardedMacOutcome;
using ::systolic::logical::GuardedMacPreflight;

std::optional<int64_t> toI64(const BigInt &value) {
  if (value < BigInt(std::numeric_limits<int64_t>::min()) ||
      value > BigInt(std::numeric_limits<int64_t>::max()))
    return std::nullopt;
  return value.convert_to<int64_t>();
}

const char *operandName(unsigned a) { return a == 0 ? "A" : a == 1 ? "B" : "C"; }

// J3 / J5: the plan's output blocks partition [0, M) x [0, N) -- each inside
// C, pairwise disjoint, total area M x N -- and each block's invocations
// cover [0, K) contiguously in ascending order.
bool planPartitionsBox(const SystolicTilePlan &plan, int64_t M, int64_t N,
                       int64_t K, std::string &why) {
  if (plan.M != M || plan.N != N || plan.K != K) {
    why = "the plan's extents are not the GEMM's";
    return false;
  }
  BigInt area = 0;
  for (const SystolicTileBlock &block : plan.blocks) {
    if (block.m <= 0 || block.n <= 0 || block.row < 0 || block.column < 0 ||
        block.row > M - block.m || block.column > N - block.n) {
      why = "an output block lies outside C";
      return false;
    }
    area += BigInt(block.m) * BigInt(block.n);
    BigInt next = 0;
    for (const SystolicTileInvocation &inv : block.invocations) {
      if (inv.k <= 0 || BigInt(inv.k0) != next) {
        why = "a block's invocations do not cover [0, K) contiguously in "
              "ascending order";
        return false;
      }
      next += BigInt(inv.k);
    }
    if (next != BigInt(K)) {
      why = "a block's invocations do not end at K";
      return false;
    }
  }
  if (area != BigInt(M) * BigInt(N)) {
    why = "the output blocks do not cover C exactly";
    return false;
  }
  for (size_t i = 0; i < plan.blocks.size(); ++i)
    for (size_t j = i + 1; j < plan.blocks.size(); ++j) {
      const SystolicTileBlock &p = plan.blocks[i];
      const SystolicTileBlock &q = plan.blocks[j];
      const bool rows = p.row < q.row + q.m && q.row < p.row + p.m;
      const bool columns =
          p.column < q.column + q.n && q.column < p.column + p.n;
      if (rows && columns) {
        why = "two output blocks overlap";
        return false;
      }
    }
  return true;
}

} // namespace

std::optional<DeviceJobPreflight>
prepareDeviceJobs(Operation *anchor, bool assumeNoAlias,
                  bool allowFpReassociation, bool commonFiberOffload,
                  GuardedMacOutcome &outcome, std::string &why) {
  using namespace mlir::presburger;

  auto unknown = [&](const Twine &message) -> std::optional<DeviceJobPreflight> {
    outcome = GuardedMacOutcome::Unknown;
    why = ("UNKNOWN: " + message).str();
    return std::nullopt;
  };

  // The shared preflight: recognition, statement shape, memory
  // disjointness (REFUSED when an input shares C's buffer), the exact
  // domain, its candidate box and the access functions.
  auto common = ::systolic::logical::prepareRecognizedGemm(
      anchor, assumeNoAlias, outcome, why);
  if (!common)
    return std::nullopt;

  DeviceJobPreflight result(std::move(*common));
  const GuardedMacPreflight &pf = result.common;

  // ------------------------------------------------------------------
  // J9 precision, J4 layout.
  // ------------------------------------------------------------------
  std::array<MemRefType, 3> types;
  for (unsigned a = 0; a < 3; ++a) {
    auto type = dyn_cast<MemRefType>(pf.memrefs[a].getType());
    if (!type)
      return unknown("an operand is not a memref");
    if (!type.getElementType().isF32()) {
      std::string text;
      llvm::raw_string_ostream(text) << type.getElementType();
      return unknown("the device datapath is binary32; the operands are " +
                     text);
    }
    if (type.getRank() != 2)
      return unknown(Twine("operand ") + operandName(a) + " has rank " +
                     Twine(type.getRank()) +
                     "; only rank-2 operands are lowered in this phase");
    if (!type.getLayout().isIdentity())
      return unknown(Twine("operand ") + operandName(a) +
                     " has a non-identity layout; only identity-layout "
                     "memrefs are lowered in this phase");
    if (type.isDynamicDim(1))
      return unknown(Twine("operand ") + operandName(a) +
                     " has a dynamic trailing dimension; only a dynamic "
                     "leading dimension is lowered in this phase");
    types[a] = type;
  }

  // ------------------------------------------------------------------
  // J10 arithmetic contract.
  // ------------------------------------------------------------------
  if (pf.roundingModeAttributes)
    return unknown("a rounding-mode attribute on the source statement: the "
                   "device rounds to nearest even only");
  result.permittedByFlag =
      arith::bitEnumContainsAll(pf.addFastMath, arith::FastMathFlags::reassoc);
  result.permittedByOption = allowFpReassociation;
  if (!result.permittedByFlag && !result.permittedByOption)
    return unknown(
        "strict floating-point semantics: the device sums each invocation "
        "from +0 in its own order (sixteen banks and a reduction tree, "
        "flush-to-zero) and the host adds the sum into C, so no bitwise "
        "equality with the loop can be proven (docs/AFFINE_GEMM_MAPPING.md "
        "sections 6 and 27); the offload needs the reassoc flag on the "
        "accumulation or allow-fp-reassociation");

  // ------------------------------------------------------------------
  // J1 the iteration domain: three coordinates, a unimodular map, and a
  // logical domain that is exactly its box.
  // ------------------------------------------------------------------
  const size_t n = pf.plan.bounds.size();
  if (n != 3)
    return unknown("the nest has " + Twine(n) +
                   " logical coordinates; only the three-coordinate GEMM "
                   "(no batch coordinate) is lowered in this phase");
  const auto &map = pf.coordinateMap;
  if (map.logicalToSource.size() != 3)
    return unknown("malformed coordinate map");
  // x_j = sum_i inverse[j][i] z_i
  std::array<std::array<BigInt, 3>, 3> inverse;
  for (unsigned j = 0; j < 3; ++j) {
    if (map.logicalToSource[j].size() != 3)
      return unknown("malformed coordinate map");
    for (unsigned i = 0; i < 3; ++i) {
      const auto &entry = map.logicalToSource[j][i];
      if (entry.denominator() != 1)
        return unknown("the coordinate map is not unimodular (its inverse "
                       "is not integral): a congruence domain is not "
                       "lowered in this phase");
      inverse[j][i] = entry.numerator();
    }
  }

  if (pf.plan.logicalDomain.getNumDisjuncts() != 1)
    return unknown("the logical domain is not one polyhedron");
  const IntegerRelation &logical = pf.plan.logicalDomain.getDisjunct(0);
  if (logical.getNumLocalVars() != 0 || logical.getNumSymbolVars() != 0 ||
      logical.getNumVars() != 3)
    return unknown("the logical domain carries locals or symbols");

  IntegerPolyhedron box(PresburgerSpace::getSetSpace(3));
  std::array<int64_t, 3> lower{}, extent{};
  for (unsigned i = 0; i < 3; ++i) {
    const auto &bound = pf.plan.bounds[i];
    const BigInt size = BigInt(bound.upper) - BigInt(bound.lower) + 1;
    auto fits = toI64(size);
    if (!fits || *fits <= 0)
      return unknown("the logical box is empty or does not fit i64");
    lower[i] = bound.lower;
    extent[i] = *fits;
    box.addBound(BoundType::LB, i, DynamicAPInt(bound.lower));
    box.addBound(BoundType::UB, i, DynamicAPInt(bound.upper));
  }
  auto inBox = ::systolic::logical::exactIntegerSetInclusion(logical, box);
  auto covers = ::systolic::logical::exactIntegerSetInclusion(box, logical);
  if (!inBox || !covers)
    return unknown("the equality of the logical domain and its box is "
                   "undecided");
  if (!*inBox)
    return unknown("the logical domain exceeds its candidate box");
  // Phase 3b-7: a non-box domain is lowered through an exact output mask
  // when it factors as O x BK (decided below, once the roles are known).
  const bool isBox = *covers;
  if (!isBox && logical.isIntegerEmpty())
    return unknown("the iteration domain is empty: nothing to offload");

  // ------------------------------------------------------------------
  // J2 the access functions in logical coordinates: one coordinate per
  // dimension with coefficient one, in the standard role order.
  // ------------------------------------------------------------------
  std::array<std::array<int, 2>, 3> coordinate{};
  std::array<std::array<BigInt, 2>, 3> offset;
  for (unsigned a = 0; a < 3; ++a) {
    if (pf.accessRows[a].size() != 2 || pf.accessOffsets[a].size() != 2)
      return unknown(Twine("the access of operand ") + operandName(a) +
                     " is not two-dimensional");
    for (unsigned d = 0; d < 2; ++d) {
      const auto &row = pf.accessRows[a][d];
      if (row.size() != 3)
        return unknown("malformed access row");
      int unit = -1;
      for (unsigned i = 0; i < 3; ++i) {
        BigInt coefficient = 0;
        for (unsigned j = 0; j < 3; ++j)
          coefficient += row[j] * inverse[j][i];
        if (coefficient == 0)
          continue;
        if (coefficient != 1 || unit != -1)
          return unknown(Twine("operand ") + operandName(a) + " reads its "
                         "dimension " + Twine(d) + " through a strided, "
                         "transposed, skewed or flattened form; only "
                         "unit-coefficient views are lowered in this phase");
        unit = static_cast<int>(i);
      }
      if (unit == -1)
        return unknown(Twine("operand ") + operandName(a) + " reads its "
                       "dimension " + Twine(d) + " at a constant; broadcast "
                       "and unit-extent views are not lowered in this phase");
      coordinate[a][d] = unit;
      offset[a][d] = pf.accessOffsets[a][d];
    }
  }
  // Roles: C reads (I, J); A must read (I, K); B must read (K, J).
  const int roleI = coordinate[2][0];
  const int roleJ = coordinate[2][1];
  if (roleI == roleJ)
    return unknown("C reads one coordinate in both dimensions");
  const int roleK = 3 - roleI - roleJ;
  if (coordinate[0][0] != roleI || coordinate[0][1] != roleK)
    return unknown("A is not read as (I, K): a transposed or role-swapped "
                   "factor view is not lowered in this phase");
  if (coordinate[1][0] != roleK || coordinate[1][1] != roleJ)
    return unknown("B is not read as (K, J): a transposed or role-swapped "
                   "factor view is not lowered in this phase");
  if (!isBox) {
    // The dense box: the candidate box (the interval-arithmetic image of
    // the loop bounds under T) tightened in every coordinate to the
    // domain's own constant bounds -- valid bounds of every iteration, so
    // the dense GEMM covers no more than the hull of the domain.
    auto tighten = [&](int coordinate) -> bool {
      if (auto tight = logical.getConstantBound64(BoundType::LB, coordinate);
          tight && *tight > lower[coordinate]) {
        extent[coordinate] -= *tight - lower[coordinate];
        lower[coordinate] = *tight;
      }
      if (auto tight = logical.getConstantBound64(BoundType::UB, coordinate);
          tight && *tight < lower[coordinate] + extent[coordinate] - 1)
        extent[coordinate] = *tight - lower[coordinate] + 1;
      return extent[coordinate] > 0;
    };
    for (int coordinate : {roleI, roleJ, roleK})
      if (!tighten(coordinate))
        return unknown("the domain's range in a coordinate is empty");
    const int64_t kLower = lower[roleK];
    const int64_t kUpper = lower[roleK] + extent[roleK] - 1;

    // The full-reduction-fiber property: for every (i, j, k) in D and
    // every k' in [kLower, kUpper], (i, j, k') is in D. Its negation is the
    // union, over the facets f of D, of the conjunctive sets
    //   { (z, k') : z in D, kLower <= k' <= kUpper, f violated at z[K := k'] }
    // (an equality is two facets); each is decided with integer emptiness,
    // no set difference and no projection (section 19.14).
    auto violated = [&](ArrayRef<DynamicAPInt> row, bool negate) -> bool {
      // row: a . z + c >= 0 over (z0, z1, z2, 1); violated at z[K := k']
      // means a . z' + c <= -1, i.e. -(a . z') - c - 1 >= 0.
      IntegerPolyhedron witness(logical);
      witness.appendVar(VarKind::SetDim, 1); // k' is variable 3
      witness.addBound(BoundType::LB, 3, DynamicAPInt(kLower));
      witness.addBound(BoundType::UB, 3, DynamicAPInt(kUpper));
      SmallVector<DynamicAPInt, 5> facet(5, DynamicAPInt(0));
      for (unsigned i = 0; i < 3; ++i) {
        const DynamicAPInt a = negate ? -row[i] : row[i];
        if (static_cast<int>(i) == roleK)
          facet[3] -= a;
        else
          facet[i] -= a;
      }
      facet[4] = -(negate ? -row[3] : row[3]) - 1;
      witness.addInequality(facet);
      return !witness.isIntegerEmpty();
    };
    const char *partialFibers = nullptr;
    for (unsigned r = 0; r < logical.getNumInequalities() && !partialFibers;
         ++r)
      if (violated(logical.getInequality(r), false))
        partialFibers = "the reduction fiber of an output coordinate is a "
                        "proper part of the K range (a K bound depends on i "
                        "or j): the output mask would skip contributions; "
                        "not lowered";
    for (unsigned r = 0; r < logical.getNumEqualities() && !partialFibers;
         ++r)
      if (violated(logical.getEquality(r), false) ||
          violated(logical.getEquality(r), true))
        partialFibers = "the reduction fiber of an output coordinate is a "
                        "proper part of the K range (an equality constrains "
                        "K): not lowered";
    if (partialFibers) {
      // Phase 3b-8 (section 29): under the option, the common fibers of the
      // output blocks go to the device and the rest to the host; the
      // blocks are the tile plan's, known at emission.
      if (!commonFiberOffload)
        return unknown(partialFibers);
      result.commonFiber = true;
    }
  }

  if (!isBox && !result.commonFiber) {
    const int64_t kLower = lower[roleK];
    const int64_t kUpper = lower[roleK] + extent[roleK] - 1;
    // Memory safety of the dense reads with a dynamic leading dimension
    // (J4 takes its upper bound as the program's own): the dense GEMM reads
    // every row of the box, so the box's last row must be a row the nest
    // itself reads -- an iteration with that coordinate value must exist
    // (exact: one equality on the domain, integer emptiness). A static
    // dimension is checked against the box below as before. For B the top
    // K value is attained whenever the fiber property holds and the domain
    // is nonempty; the check is kept for symmetry.
    auto attained = [&](int coordinate, int64_t value) -> bool {
      IntegerPolyhedron slice(logical);
      slice.addBound(BoundType::EQ, coordinate, DynamicAPInt(value));
      return !slice.isIntegerEmpty();
    };
    if (types[0].isDynamicDim(0) &&
        !attained(roleI, lower[roleI] + extent[roleI] - 1))
      return unknown("the last row of the dense box is not an iteration of "
                     "the nest: with a dynamic leading dimension of A the "
                     "dense GEMM could read past the array");
    if (types[1].isDynamicDim(0) && !attained(roleK, kUpper))
      return unknown("the last reduction row of the dense box is not an "
                     "iteration of the nest: with a dynamic leading "
                     "dimension of B the dense GEMM could read past the "
                     "array");

    // The output domain O = D at K = kLower, over (I, J), inside the box
    // BI x BJ; the exact guard of Phase 2b evaluates its membership.
    IntegerPolyhedron output(PresburgerSpace::getSetSpace(2));
    auto substitute = [&](ArrayRef<DynamicAPInt> row, bool equality) {
      SmallVector<DynamicAPInt, 3> reduced(3, DynamicAPInt(0));
      reduced[0] = row[roleI];
      reduced[1] = row[roleJ];
      reduced[2] = row[3] + row[roleK] * DynamicAPInt(kLower);
      if (equality)
        output.addEquality(reduced);
      else
        output.addInequality(reduced);
    };
    for (unsigned r = 0; r < logical.getNumInequalities(); ++r)
      substitute(logical.getInequality(r), false);
    for (unsigned r = 0; r < logical.getNumEqualities(); ++r)
      substitute(logical.getEquality(r), true);
    std::vector<::systolic::logical::ClosedIntegerInterval> maskBounds(2);
    maskBounds[0].lower = lower[roleI];
    maskBounds[0].upper = lower[roleI] + extent[roleI] - 1;
    maskBounds[1].lower = lower[roleJ];
    maskBounds[1].upper = lower[roleJ] + extent[roleJ] - 1;
    auto maskPlan = ::systolic::logical::buildSymbolicIterationPlan(
        PresburgerSet(output), maskBounds);
    if (!maskPlan)
      return unknown("the output domain is not representable over the I x "
                     "J box");
    if (!::systolic::logical::buildExactGuardPlan(*maskPlan))
      return unknown("the output mask is not representable in i64 "
                     "arithmetic over the I x J box");
    result.maskedOutput = true;
    result.maskPlan = std::move(*maskPlan);
    result.maskLower = {lower[roleI], lower[roleJ]};
  }

  result.M = extent[roleI];
  result.N = extent[roleJ];
  result.K = extent[roleK];
  result.roles = {roleI, roleJ, roleK};
  result.boxLower = lower;
  result.boxExtent = extent;

  // ------------------------------------------------------------------
  // J3 / J4 the views and their bounds, in checked arithmetic.
  // ------------------------------------------------------------------
  for (unsigned a = 0; a < 3; ++a) {
    DeviceJobView &view = result.views[a];
    view.memref = pf.memrefs[a];
    bool whole = true;
    for (unsigned d = 0; d < 2; ++d) {
      const int c = coordinate[a][d];
      const BigInt origin = offset[a][d] + BigInt(lower[c]);
      const int64_t size = extent[c];
      auto start = toI64(origin);
      if (!start || *start < 0)
        return unknown(Twine("the view of operand ") + operandName(a) +
                       " starts before its first element or does not fit "
                       "i64");
      const BigInt end = origin + BigInt(size); // exclusive
      if (types[a].isDynamicDim(d)) {
        whole = false; // the program's own bound; a static view is taken
      } else {
        const int64_t dim = types[a].getDimSize(d);
        if (end > BigInt(dim))
          return unknown(Twine("the view of operand ") + operandName(a) +
                         " exceeds its dimension " + Twine(d));
        if (*start != 0 || size != dim)
          whole = false;
      }
      view.origin[d] = *start;
      view.size[d] = size;
    }
    view.wholeMemref = whole;
  }
  result.sharedFactorView =
      pf.memrefs[0] == pf.memrefs[1] &&
      result.views[0].origin == result.views[1].origin &&
      result.views[0].size == result.views[1].size;

  outcome = GuardedMacOutcome::Proven;
  why = pf.diagnostic;
  return result;
}

std::optional<DeviceJobReplacement>
replaceGemmWithDeviceJobs(OpBuilder &builder, Operation *anchor,
                          const SystolicFleet &fleet,
                          const SystolicOptimizerOptions &options,
                          bool assumeNoAlias, bool allowFpReassociation,
                          bool commonFiberOffload, int64_t &nextGemmIndex,
                          GuardedMacOutcome &outcome, std::string &why,
                          bool &hardError) {
  using namespace mlir::presburger;
  hardError = false;
  auto pre = prepareDeviceJobs(anchor, assumeNoAlias, allowFpReassociation,
                               commonFiberOffload, outcome, why);
  if (!pre)
    return std::nullopt;

  auto unsupported =
      [&](const Twine &message) -> std::optional<DeviceJobReplacement> {
    outcome = GuardedMacOutcome::Unsupported;
    why = ("REFUSED: " + message).str();
    return std::nullopt;
  };
  auto unknown =
      [&](const Twine &message) -> std::optional<DeviceJobReplacement> {
    outcome = GuardedMacOutcome::Unknown;
    why = ("UNKNOWN: " + message).str();
    return std::nullopt;
  };

  Operation *source = pre->common.outermost.getOperation();
  Block *block = source->getBlock();
  if (!block || !block->getParentOp())
    return unsupported("the source nest is detached");
  Operation *parent = block->getParentOp();

  // The plan, and its re-verification against the box (J3, J5).
  std::optional<SystolicTilePlan> plan;
  if (!fleet.resources.empty()) {
    FailureOr<SystolicTilePlan> planned =
        planSystolicMatmul(source, pre->M, pre->N, pre->K, fleet, options);
    if (failed(planned)) {
      hardError = true;
      return unsupported("the fleet planner refused the GEMM (its "
                         "diagnostic precedes)");
    }
    std::string partitionWhy;
    if (!planPartitionsBox(*planned, pre->M, pre->N, pre->K, partitionWhy))
      return unsupported("the tile plan does not partition the logical "
                         "box: " + partitionWhy);
    plan = std::move(*planned);
  }

  // ------------------------------------------------------------------
  // Phase 3b-8: the common reduction fiber of every output block, the
  // device plan clipped to it, the exact counts, and the remainder sets.
  // Nothing is created yet.
  // ------------------------------------------------------------------
  struct FiberBlock {
    ::systolic::logical::OutputBlock block;       // logical I / J values
    ::systolic::logical::ReductionInterval fiber; // common fiber, or empty
    // M2b-1: the remainder's iteration plans, one per non-empty piece.
    std::vector<::systolic::logical::SymbolicIterationPlan> remainderPlans;
  };
  std::vector<FiberBlock> fiberBlocks;
  std::optional<SystolicTilePlan> trimmed; // the device plan (with a fleet)
  ::systolic::logical::FiberCounts counts;
  int64_t kLowerAbs = 0, kUpperAbs = -1;
  ::systolic::logical::FiberRoles roles;
  if (pre->commonFiber) {
    const IntegerRelation &logical =
        pre->common.plan.logicalDomain.getDisjunct(0);
    roles.i = static_cast<unsigned>(pre->roles[0]);
    roles.j = static_cast<unsigned>(pre->roles[1]);
    roles.k = static_cast<unsigned>(pre->roles[2]);
    const int64_t iLower = pre->boxLower[pre->roles[0]];
    const int64_t jLower = pre->boxLower[pre->roles[1]];
    kLowerAbs = pre->boxLower[pre->roles[2]];
    kUpperAbs = kLowerAbs + pre->K - 1;
    std::vector<::systolic::logical::OutputBlock> blocks;
    if (plan) {
      for (const SystolicTileBlock &block : plan->blocks)
        blocks.push_back({iLower + block.row, iLower + block.row + block.m - 1,
                          jLower + block.column,
                          jLower + block.column + block.n - 1});
    } else {
      blocks.push_back({iLower, iLower + pre->M - 1, jLower,
                        jLower + pre->N - 1});
    }
    // Every device box inside the source domain: for each facet of D' the
    // box points violating it form an empty set.
    auto insideDomain = [&](const ::systolic::logical::OutputBlock &block,
                            const ::systolic::logical::ReductionInterval &k) {
      for (unsigned r = 0; r < logical.getNumInequalities() +
                                   2 * logical.getNumEqualities();
           ++r) {
        IntegerPolyhedron box(PresburgerSpace::getSetSpace(3));
        box.addBound(BoundType::LB, roles.i, DynamicAPInt(block.rowLower));
        box.addBound(BoundType::UB, roles.i, DynamicAPInt(block.rowUpper));
        box.addBound(BoundType::LB, roles.j, DynamicAPInt(block.columnLower));
        box.addBound(BoundType::UB, roles.j,
                     DynamicAPInt(block.columnUpper));
        box.addBound(BoundType::LB, roles.k, DynamicAPInt(k.lower));
        box.addBound(BoundType::UB, roles.k, DynamicAPInt(k.upper));
        ArrayRef<DynamicAPInt> row;
        bool negate = false;
        if (r < logical.getNumInequalities()) {
          row = logical.getInequality(r);
        } else {
          const unsigned e = (r - logical.getNumInequalities()) / 2;
          negate = (r - logical.getNumInequalities()) % 2 == 1;
          row = logical.getEquality(e);
        }
        // violated: a . z + c <= -1  <=>  -(a . z) - c - 1 >= 0
        SmallVector<DynamicAPInt, 4> facet(4, DynamicAPInt(0));
        for (unsigned v = 0; v < 3; ++v)
          facet[v] = negate ? row[v] : -row[v];
        facet[3] = (negate ? row[3] : -row[3]) - 1;
        box.addInequality(facet);
        if (!box.isIntegerEmpty())
          return false;
      }
      return true;
    };
    for (const auto &block : blocks) {
      auto fiber = ::systolic::logical::commonReductionFiber(
          logical, roles, block, kLowerAbs, kUpperAbs);
      if (!fiber)
        return unknown("a common reduction fiber is not representable");
      if (!fiber->empty && !insideDomain(block, *fiber))
        return unsupported("a device box is not inside the source domain "
                           "(the common fiber certificate failed)");
      fiberBlocks.push_back({block, *fiber});
    }
    auto counted = ::systolic::logical::countReductionFibers(
        logical, roles, blocks, iLower, iLower + pre->M - 1, jLower,
        jLower + pre->N - 1, kLowerAbs, kUpperAbs);
    if (!counted)
      return unknown("the reduction fibers cannot be counted exactly");
    counts = *counted;
    if (counts.device == 0)
      return unknown("no output block has a common reduction fiber: "
                     "nothing for the device");
    if (plan) {
      // The device plan: each block's folds clipped to its common fiber
      // (folds outside dropped, boundary folds shortened), every clipped
      // invocation re-priced with the cost model it was priced with;
      // blocks without a fiber leave the plan (their cells are the host's).
      trimmed = *plan;
      trimmed->blocks.clear();
      for (size_t b = 0; b < plan->blocks.size(); ++b) {
        const SystolicTileBlock &block = plan->blocks[b];
        const auto &fiber = fiberBlocks[b].fiber;
        if (fiber.empty)
          continue;
        const int64_t first = fiber.lower - kLowerAbs; // relative to the box
        const int64_t last = fiber.upper - kLowerAbs;
        SystolicTileBlock clipped = block;
        clipped.invocations.clear();
        if (block.acceleratorId < 0 ||
            static_cast<size_t>(block.acceleratorId) >=
                fleet.resources.size())
          return unsupported("a block names no accelerator of the fleet");
        const ::systolic::ArrayConfig &config =
            fleet.resources[block.acceleratorId].config;
        int64_t next = first;
        for (const SystolicTileInvocation &inv : block.invocations) {
          const int64_t begin = std::max(inv.k0, first);
          const int64_t end = std::min(inv.k0 + inv.k - 1, last);
          if (begin > end)
            continue;
          if (begin != next)
            return unsupported("the clipped folds of a block are not "
                               "contiguous");
          SystolicTileInvocation cut = inv;
          cut.k0 = begin;
          cut.k = end - begin + 1;
          if (config.kMax > 0 && cut.k > config.kMax)
            return unsupported("a clipped invocation exceeds the "
                               "accelerator's k_max");
          const ::systolic::InvocationCost cost =
              ::systolic::estimateInvocationCost(cut.k, config);
          if (!cost.ok || cost.compute <= 0)
            return unsupported("the cost model cannot price a clipped "
                               "invocation");
          cut.estCycles = cost.compute;
          clipped.invocations.push_back(cut);
          next = end + 1;
        }
        if (next != last + 1 || clipped.invocations.empty())
          return unsupported("the clipped folds of a block do not cover "
                             "its common fiber");
        trimmed->blocks.push_back(std::move(clipped));
      }
      if (trimmed->blocks.empty())
        return unknown("no output block has a common reduction fiber: "
                       "nothing for the device");
    }
    // M2b-1 (section 29.8): the host remainder of every block as pieces
    // (below / above its common fiber, or the whole block without one),
    // each with the exact bounding box of its integer points, and the
    // certified iteration plan of each: the guard is the piece, the box is
    // the sweep. A piece without an establishable bound is UNKNOWN.
    for (FiberBlock &fb : fiberBlocks) {
      auto pieces = ::systolic::logical::remainderPieces(
          logical, roles, fb.block, fb.fiber, kLowerAbs, kUpperAbs);
      if (!pieces)
        return unknown("the exact bounds of a host remainder piece cannot "
                       "be established");
      for (::systolic::logical::RemainderPiece &piece : *pieces) {
        auto remainderPlan = ::systolic::logical::buildSymbolicIterationPlan(
            PresburgerSet(piece.set), piece.box);
        if (!remainderPlan ||
            !::systolic::logical::buildExactGuardPlan(*remainderPlan))
          return unknown("the host remainder of an output block is not "
                         "representable over its exact bounds");
        fb.remainderPlans.push_back(std::move(*remainderPlan));
      }
    }
  }

  // ------------------------------------------------------------------
  // Emission, immediately before the source nest. Everything created lies
  // between `before` and the source nest; a failure erases exactly that.
  // ------------------------------------------------------------------
  Operation *before = source->getPrevNode();
  auto rollback = [&]() {
    for (Operation *op = source->getPrevNode(); op != before;) {
      Operation *previous = op->getPrevNode();
      op->erase();
      op = previous;
    }
  };

  OpBuilder::InsertionGuard insertionGuard(builder);
  builder.setInsertionPoint(source);
  Location loc = source->getLoc();
  Type f32 = builder.getF32Type();

  auto viewOf = [&](const DeviceJobView &view) -> Value {
    if (view.wholeMemref)
      return view.memref;
    SmallVector<OpFoldResult> offsets{builder.getIndexAttr(view.origin[0]),
                                      builder.getIndexAttr(view.origin[1])};
    SmallVector<OpFoldResult> sizes{builder.getIndexAttr(view.size[0]),
                                    builder.getIndexAttr(view.size[1])};
    SmallVector<OpFoldResult> strides(2, builder.getIndexAttr(1));
    return memref::SubViewOp::create(builder, loc, view.memref, offsets,
                                     sizes, strides);
  };
  auto tensorOf = [&](Value view, const DeviceJobView &spec,
                      bool writable) -> Value {
    auto type = RankedTensorType::get({spec.size[0], spec.size[1]}, f32);
    return bufferization::ToTensorOp::create(builder, loc, type, view,
                                             /*restrict=*/true, writable);
  };

  Value aTensor = tensorOf(viewOf(pre->views[0]), pre->views[0],
                           /*writable=*/false);
  Value bTensor = pre->sharedFactorView
                      ? aTensor
                      : tensorOf(viewOf(pre->views[1]), pre->views[1],
                                 /*writable=*/false);
  // Phase 3b-7: the dense GEMM of a masked domain runs into a fresh
  // temporary T, zeroed first (as the fast path zeroes an assignment's C);
  // the pipeline's deallocation frees it with the function's other
  // allocations.
  Value temporary;
  Value cView;
  if (pre->maskedOutput) {
    temporary = memref::AllocOp::create(
        builder, loc, MemRefType::get({pre->M, pre->N}, f32));
    Value zero = arith::ConstantOp::create(builder, loc,
                                           builder.getZeroAttr(f32));
    {
      OpBuilder::InsertionGuard zeroGuard(builder);
      auto rows = affine::AffineForOp::create(builder, loc, 0, pre->M, 1);
      builder.setInsertionPointToStart(rows.getBody());
      auto columns = affine::AffineForOp::create(builder, loc, 0, pre->N, 1);
      builder.setInsertionPointToStart(columns.getBody());
      affine::AffineStoreOp::create(
          builder, loc, zero, temporary,
          ValueRange{rows.getInductionVar(), columns.getInductionVar()});
    }
    cView = temporary;
  } else {
    cView = viewOf(pre->views[2]);
  }
  DeviceJobView cSpec = pre->views[2];
  cSpec.size = {pre->M, pre->N};
  Value cTensor = tensorOf(cView, cSpec, /*writable=*/true);

  const int64_t gemmIndex = nextGemmIndex;
  Value result;
  if (pre->commonFiber && trimmed) {
    result = emitSystolicTilePlan(builder, loc, aTensor, bTensor, cTensor,
                                  *trimmed, fleet, gemmIndex);
  } else if (pre->commonFiber) {
    // No fleet: one tile over the whole box's common fiber, on the K slices
    // of A and B.
    const auto &fiber = fiberBlocks.front().fiber;
    const int64_t first = fiber.lower - kLowerAbs;
    const int64_t length = fiber.length();
    SmallVector<OpFoldResult> unit(2, builder.getIndexAttr(1));
    SmallVector<OpFoldResult> aOffsets{builder.getIndexAttr(0),
                                       builder.getIndexAttr(first)};
    SmallVector<OpFoldResult> aSizes{builder.getIndexAttr(pre->M),
                                     builder.getIndexAttr(length)};
    Value aSlice = tensor::ExtractSliceOp::create(builder, loc, aTensor,
                                                  aOffsets, aSizes, unit);
    SmallVector<OpFoldResult> bOffsets{builder.getIndexAttr(first),
                                       builder.getIndexAttr(0)};
    SmallVector<OpFoldResult> bSizes{builder.getIndexAttr(length),
                                     builder.getIndexAttr(pre->N)};
    Value bSlice = tensor::ExtractSliceOp::create(builder, loc, bTensor,
                                                  bOffsets, bSizes, unit);
    result = MatmulTileOp::create(
        builder, loc, cTensor.getType(), aSlice, bSlice, cTensor,
        builder.getI64IntegerAttr(pre->M), builder.getI64IntegerAttr(pre->N),
        builder.getI64IntegerAttr(length), /*device=*/FlatSymbolRefAttr(),
        /*est_cycles=*/IntegerAttr(), /*start_cycle=*/IntegerAttr());
  } else if (plan) {
    result = emitSystolicTilePlan(builder, loc, aTensor, bTensor, cTensor,
                                  *plan, fleet, gemmIndex);
  } else {
    result = MatmulTileOp::create(
        builder, loc, cTensor.getType(), aTensor, bTensor, cTensor,
        builder.getI64IntegerAttr(pre->M), builder.getI64IntegerAttr(pre->N),
        builder.getI64IntegerAttr(pre->K), /*device=*/FlatSymbolRefAttr(),
        /*est_cycles=*/IntegerAttr(), /*start_cycle=*/IntegerAttr());
  }
  auto writeBack =
      bufferization::MaterializeInDestinationOp::create(builder, loc, result,
                                                        cView);
  writeBack.setRestrict(true);
  writeBack.setWritable(true);

  // Phase 3b-7: C[i][j] += T[i][j] for (i, j) in O, under the exact guard;
  // nothing else of C is read or written.
  if (pre->maskedOutput) {
    auto nest = ::systolic::logical::emitGuardedAffineLoopNest(
        builder, loc, *pre->maskPlan);
    if (!nest) {
      rollback();
      return unsupported("the output mask nest could not be emitted");
    }
    Block *thenBlock = nest->guardIf.thenBlock();
    if (!thenBlock || nest->logicalIVs.size() != 2) {
      rollback();
      return unsupported("the output mask nest has an unexpected shape");
    }
    OpBuilder::InsertionGuard maskGuard(builder);
    builder.setInsertionPoint(thenBlock->getTerminator());
    MLIRContext *context = builder.getContext();
    AffineExpr i = getAffineDimExpr(0, context);
    AffineExpr j = getAffineDimExpr(1, context);
    const int64_t lowerI = pre->maskLower[0], lowerJ = pre->maskLower[1];
    const DeviceJobView &c = pre->views[2];
    AffineMap temporaryMap =
        AffineMap::get(2, 0, {i - lowerI, j - lowerJ}, context);
    AffineMap cMap = AffineMap::get(
        2, 0, {i - lowerI + c.origin[0], j - lowerJ + c.origin[1]}, context);
    Value t = affine::AffineLoadOp::create(builder, loc, temporary,
                                           temporaryMap, nest->logicalIVs);
    Value cValue = affine::AffineLoadOp::create(builder, loc, c.memref, cMap,
                                                nest->logicalIVs);
    Value sum = arith::AddFOp::create(builder, loc, cValue, t);
    affine::AffineStoreOp::create(builder, loc, sum, c.memref, cMap,
                                  nest->logicalIVs);
  }

  // Phase 3b-8: the host remainder, after the device write-back: per block
  // the contributions outside its common fiber (or the whole block when it
  // has none) as guarded MAC nests whose exact guard is D' intersected with
  // that set, with the source's own fast-math flags. M2b-1: one nest per
  // non-empty piece over the piece's exact bounding box (planned above).
  int64_t remainderNests = 0;
  for (const FiberBlock &fb : fiberBlocks) {
    for (const ::systolic::logical::SymbolicIterationPlan &remainderPlan :
         fb.remainderPlans) {
      ::systolic::logical::GuardedMacPreflight remainderPreflight = pre->common;
      remainderPreflight.plan = remainderPlan;
      remainderPreflight.partialDomain = true;
      remainderPreflight.semantics.mirrorFastMath = true;
      remainderPreflight.semantics.multiplyFastMath =
          pre->common.multiplyFastMath;
      remainderPreflight.semantics.addFastMath = pre->common.addFastMath;
      std::string nestWhy;
      auto statement = ::systolic::logical::emitGuardedMacStatement(
          builder, loc, remainderPreflight, pre->common.memrefs, nestWhy);
      if (!statement) {
        rollback();
        return unsupported("the host remainder nest of an output block: " +
                           nestWhy);
      }
      ++remainderNests;
    }
  }

  // Verify before the source goes.
  if (failed(mlir::verify(parent))) {
    rollback();
    return unsupported("the emitted device jobs do not verify");
  }

  DeviceJobReplacement replacement;
  replacement.M = pre->M;
  replacement.N = pre->N;
  replacement.K = pre->K;
  replacement.maskedOutput = pre->maskedOutput;
  replacement.commonFiber = pre->commonFiber;
  const SystolicTilePlan *emitted =
      trimmed ? &*trimmed : (plan ? &*plan : nullptr);
  if (emitted) {
    replacement.blocks = static_cast<int64_t>(emitted->blocks.size());
    for (const SystolicTileBlock &b : emitted->blocks)
      replacement.invocations += static_cast<int64_t>(b.invocations.size());
    // The makespan is the planner's for the plan it priced; a clipped plan
    // (Phase 3b-8) has none of its own, and the dense one is not reported.
    if (!pre->commonFiber)
      replacement.makespan =
          static_cast<int64_t>(emitted->optimization.makespan);
  } else {
    replacement.blocks = 1;
    replacement.invocations = 1;
  }
  if (pre->commonFiber) {
    replacement.deviceBlocks = replacement.blocks;
    replacement.remainderNests = remainderNests;
    replacement.totalMacs = counts.total.str();
    replacement.deviceMacs = counts.device.str();
    replacement.hostMacs = counts.remainder.str();
  }
  replacement.firstEmitted = before ? before->getNextNode() : &block->front();
  replacement.diagnostic = pre->common.diagnostic;

  source->erase();
  nextGemmIndex = gemmIndex + 1;

  outcome = GuardedMacOutcome::Proven;
  why = replacement.diagnostic;
  return replacement;
}

} // namespace mlir::systolic
