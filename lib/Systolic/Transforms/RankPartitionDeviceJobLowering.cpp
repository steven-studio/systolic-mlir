//===- RankPartitionDeviceJobLowering.cpp - Certified GEMM to device jobs -===//
//
// Phase 5.4.3d-3b-6. See the header and docs/AFFINE_GEMM_MAPPING.md
// section 27.
//
//===----------------------------------------------------------------------===//

#include "Systolic/RankPartitionDeviceJobLowering.h"

#include "Systolic/RankPartitionExactInclusion.h"
#include "Systolic/SystolicOps.h"

#include "mlir/Analysis/Presburger/IntegerRelation.h"
#include "mlir/Analysis/Presburger/PresburgerSpace.h"
#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Bufferization/IR/Bufferization.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
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
                  bool allowFpReassociation, GuardedMacOutcome &outcome,
                  std::string &why) {
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
  if (!*covers)
    return unknown("the logical domain is not a box (an iteration of the "
                   "box is not an iteration of the nest): non-box domains "
                   "are not lowered in this phase");
  if (!*inBox)
    return unknown("the logical domain exceeds its candidate box");

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
  result.M = extent[roleI];
  result.N = extent[roleJ];
  result.K = extent[roleK];

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
                          int64_t &nextGemmIndex, GuardedMacOutcome &outcome,
                          std::string &why, bool &hardError) {
  hardError = false;
  auto pre = prepareDeviceJobs(anchor, assumeNoAlias, allowFpReassociation,
                               outcome, why);
  if (!pre)
    return std::nullopt;

  auto unsupported =
      [&](const Twine &message) -> std::optional<DeviceJobReplacement> {
    outcome = GuardedMacOutcome::Unsupported;
    why = ("REFUSED: " + message).str();
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
  Value cView = viewOf(pre->views[2]);
  Value cTensor = tensorOf(cView, pre->views[2], /*writable=*/true);

  const int64_t gemmIndex = nextGemmIndex;
  Value result;
  if (plan) {
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

  // Verify before the source goes.
  if (failed(mlir::verify(parent))) {
    rollback();
    return unsupported("the emitted device jobs do not verify");
  }

  DeviceJobReplacement replacement;
  replacement.M = pre->M;
  replacement.N = pre->N;
  replacement.K = pre->K;
  if (plan) {
    replacement.blocks = static_cast<int64_t>(plan->blocks.size());
    for (const SystolicTileBlock &b : plan->blocks)
      replacement.invocations += static_cast<int64_t>(b.invocations.size());
    replacement.makespan =
        static_cast<int64_t>(plan->optimization.makespan);
  } else {
    replacement.blocks = 1;
    replacement.invocations = 1;
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
