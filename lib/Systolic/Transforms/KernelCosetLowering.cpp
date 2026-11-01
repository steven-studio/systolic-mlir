//===- KernelCosetLowering.cpp - Experimental symbolic kernel-coset emit --===//
//
// Phase 5.4.3d M4b.2a. See KernelCosetLowering.h for the contract.
//
//===----------------------------------------------------------------------===//

#include "Systolic/KernelCosetLowering.h"

#include "Systolic/RankPartitionKernelCoset.h"
#include "Systolic/SystolicTilePlan.h"

#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Bufferization/IR/Bufferization.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/IR/AffineExpr.h"
#include "mlir/IR/AffineMap.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/IntegerSet.h"

#include <optional>

using namespace mlir;
using namespace mlir::systolic;

namespace {

namespace klogical = ::systolic::logical;
using klogical::BigInt;

/// Exact BigInt -> int64, nullopt when out of range (fail closed on overflow).
std::optional<int64_t> bigToI64(const BigInt &v) {
  static const BigInt lo(std::numeric_limits<int64_t>::min());
  static const BigInt hi(std::numeric_limits<int64_t>::max());
  if (v < lo || v > hi)
    return std::nullopt;
  return v.convert_to<int64_t>();
}

/// M = rows * U, all BigInt. `rows` is r x n, `U` is n x n, result is r x n.
klogical::IntMatrix composeWithU(const klogical::IntMatrix &rows,
                                 const klogical::IntMatrix &U, unsigned n) {
  klogical::IntMatrix M;
  M.reserve(rows.size());
  for (const auto &row : rows) {
    klogical::IntRow out(n, BigInt(0));
    for (unsigned t = 0; t < n; ++t) {
      BigInt acc(0);
      for (unsigned s = 0; s < n; ++s)
        acc += row[s] * U[s][t];
      out[t] = acc;
    }
    M.push_back(std::move(out));
  }
  return M;
}

} // namespace

LogicalResult mlir::systolic::lowerKernelCosetOffload(
    const RecognizedMacStatement &stmt, const SystolicFleet &fleet,
    const SystolicOptimizerOptions &options, bool allowFpReassociation,
    bool operandsDisjointFromOutput, unsigned kdev,
    int64_t & /*nextGemmIndex*/, bool report, std::string &why) {
  // MLIR op handles are value types with non-const accessors; copy them out of
  // the const statement before use.
  std::array<affine::AffineLoadOp, 3> loads = stmt.loads;
  affine::AffineStoreOp store = stmt.store;
  arith::AddFOp addOp = stmt.add;
  affine::AffineForOp outerLoop = stmt.loops.front();

  // ---- Legality (fail closed; no source enumeration) -------------------
  if (!operandsDisjointFromOutput) {
    why = "general affine factors may alias the output";
    return failure();
  }
  if (fleet.resources.empty()) {
    why = "kernel-coset offload requires a declared fleet";
    return failure();
  }
  auto storeMemTy = cast<MemRefType>(store.getMemRef().getType());
  Type element = storeMemTy.getElementType();
  if (!element.isF32()) {
    why = "kernel-coset offload lowers only f32 operands in this phase";
    return failure();
  }
  // Floating-point reductions reassociate across the device's hardware order.
  // The M3 source-order certificate is NOT consulted here: it certifies the
  // source traversal, not the device reduction order. Strict FP declines.
  if (!allowFpReassociation &&
      !arith::bitEnumContainsAll(addOp.getFastmath(),
                                 arith::FastMathFlags::reassoc)) {
    why = "floating-point reassociation is not permitted for the kernel-coset "
          "reduction jobs";
    return failure();
  }

  const unsigned n = stmt.sourceDomain.getNumDimVars();
  if (n == 0) {
    why = "degenerate source rank";
    return failure();
  }
  if (stmt.sourceDomain.getNumSymbolVars() != 0 ||
      stmt.sourceDomain.getNumLocalVars() != 0) {
    why = "source domain carries symbols or congruence locals (out of M4b.2a "
          "scope)";
    return failure();
  }
  for (unsigned op = 0; op < 3; ++op)
    if (stmt.accessOffsets[op].size() != stmt.accessRows[op].size()) {
      why = "access offset/row arity mismatch";
      return failure();
    }

  // Convert the recognized int64 access rows/offsets to BigInt logical types.
  std::array<klogical::IntMatrix, 3> access;
  std::array<klogical::IntRow, 3> offset;
  for (unsigned op = 0; op < 3; ++op) {
    for (const auto &row : stmt.accessRows[op]) {
      if (row.size() != n) {
        why = "access row arity";
        return failure();
      }
      klogical::IntRow conv;
      conv.reserve(n);
      for (int64_t v : row)
        conv.push_back(BigInt(v));
      access[op].push_back(std::move(conv));
    }
    for (int64_t v : stmt.accessOffsets[op])
      offset[op].push_back(BigInt(v));
  }

  // ---- Kernel-coset plans (M0-M4b.1; no enumeration) -------------------
  auto scan =
      klogical::buildKernelCosetScanPlan(access[2], stmt.sourceDomain, n);
  if (!scan) {
    why = "kernel-coset scan plan UNKNOWN (kappa>2, non-contiguous image, "
          "rank-deficient reduction, or unbounded)";
    return failure();
  }
  if (scan->innerStride != 1) {
    why = "non-unit tile stride is out of M4b.2a scope";
    return failure();
  }
  auto tb = klogical::buildKernelCosetTileBounds(*scan);
  if (!tb || !tb->exact) {
    why = "exact kernel-coset tile bounds UNKNOWN (congruence, pinned tile, "
          "unbounded axis, or overflow)";
    return failure();
  }
  if (tb->innerStride != 1) {
    why = "non-unit inner tile stride is out of M4b.2a scope";
    return failure();
  }
  // Phase M5b-0 (finding F1): the output and peel loops below run over the
  // exact integer BOXES of their coordinates, which enforce a parameter-only
  // constraint of D' only when it involves a single coordinate. A constraint
  // coupling two coordinates (e.g. 0 <= y0 - y1 <= 3 from C[i+j]) would leave
  // chambers outside D' executing their tile interval -- MACs the source never
  // performs, reading A/B out of bounds. No IR can express it in this phase:
  // refuse, fail closed, source IR untouched.
  if (!tb->feasibilityBoxImplied) {
    why = "a parameter-only constraint of the reduced domain couples the output "
          "and peel coordinates (or is infeasible); the box-bounded peel loop "
          "cannot express it -- refused, fail closed (Phase M5b-0)";
    return failure();
  }

  const klogical::IntMatrix &U = scan->fiber.U;
  const std::vector<unsigned> &outputCols = scan->fiber.outputCols;
  const unsigned outputRank = scan->fiber.outputRank;
  const unsigned kappa = scan->fiber.kernelRank;
  if (outputCols.size() != outputRank || kappa == 0 || outputRank + kappa != n) {
    why = "kernel-coset dimension bookkeeping inconsistent";
    return failure();
  }

  // M_A, M_B, M_C = access * U  (physical address of A/B/C as an affine form of
  // the reduced coordinates y, since x = U*y).
  std::array<klogical::IntMatrix, 3> MU = {composeWithU(access[0], U, n),
                                           composeWithU(access[1], U, n),
                                           composeWithU(access[2], U, n)};

  // The C address must be independent of the reduction coordinates (it is, by
  // construction: C_rows*U is zero on the kernel columns). Verify and fail
  // closed otherwise.
  for (const auto &row : MU[2])
    for (unsigned c : scan->peelOrder)
      if (row[c] != 0) {
        why = "C address depends on a reduction coordinate (unexpected)";
        return failure();
      }

  // Validate every operand-address coefficient and offset up front (fail
  // closed on i64 overflow) so no IR is emitted before legality is decided.
  for (unsigned op = 0; op < 3; ++op) {
    for (const auto &row : MU[op])
      for (const BigInt &v : row)
        if (!bigToI64(v)) {
          why = "integer overflow in operand address coefficient";
          return failure();
        }
    for (const BigInt &v : offset[op])
      if (!bigToI64(v)) {
        why = "integer overflow in operand address offset";
        return failure();
      }
  }

  // ---- Device tiles ----------------------------------------------------
  // A static 1x1x1 tile is always needed (kdev == 1, and the kdev > 1
  // remainder); a 1x1xkdev tile is needed for the full blocks. Both must be
  // admitted by the fleet or the offload declines.
  if (kdev < 1) {
    why = "kernel-coset Kdev must be >= 1";
    return failure();
  }
  Operation *anchor = outerLoop.getOperation();
  auto plan = planSystolicMatmul(anchor, 1, 1, 1, fleet, options);
  if (failed(plan)) {
    why = "device does not admit the 1x1x1 reduction tile";
    return failure();
  }
  std::optional<SystolicTilePlan> planKdev;
  if (kdev > 1) {
    auto p = planSystolicMatmul(anchor, 1, 1, static_cast<int64_t>(kdev), fleet,
                                options);
    if (failed(p)) {
      why = "device does not admit the 1x1xKdev reduction tile";
      return failure();
    }
    planKdev = std::move(*p);
  }

  // ---- Build the affine nest -------------------------------------------
  OpBuilder builder(anchor);
  Location loc = anchor->getLoc();
  MLIRContext *ctx = builder.getContext();
  auto f32 = builder.getF32Type();

  // One affine map result = sum_j coeff[j]*d_j + c, over `numDims` dims; the
  // coefficients are taken from `coeffCols` of `row` (column i -> dim i). Fails
  // closed on i64 overflow.
  auto affineRow = [&](const klogical::IntRow &row,
                       ArrayRef<unsigned> coeffCols, const BigInt &c,
                       unsigned numDims,
                       AffineExpr &out) -> LogicalResult {
    auto cc = bigToI64(c);
    if (!cc)
      return failure();
    AffineExpr e = getAffineConstantExpr(*cc, ctx);
    for (auto [i, col] : llvm::enumerate(coeffCols)) {
      auto v = bigToI64(row[col]);
      if (!v)
        return failure();
      e = e + getAffineDimExpr(i, ctx) * (*v);
    }
    out = e;
    return success();
  };

  // y-column -> its induction variable, filled as loops are created.
  SmallVector<Value> yiv(n, Value());

  // Exact M4b.1 tile-axis bound faces, built up front so a face i64 overflow
  // declines before any IR is emitted (dim i <-> paramCols[i]). lbMap/ubMap are
  // the max/min affine.for bounds (ubMap half-open, floor + 1); they serve the
  // kdev==1 tile loop and the kdev>1 step-Kdev block-start loop alike. The raw
  // upper faces (tb->upperFloor) drive the kdev>1 full-block predicate and
  // remainder count directly.
  const unsigned numParams = tb->paramCols.size();
  SmallVector<unsigned> faceCols(numParams);
  for (unsigned i = 0; i < numParams; ++i)
    faceCols[i] = i;
  SmallVector<AffineExpr> lbExprs, ubExprs;
  for (const auto &f : tb->lowerCeil) {
    auto d = bigToI64(f.divisor);
    AffineExpr e;
    if (!d || *d <= 0 ||
        failed(affineRow(f.params, faceCols, f.constant, numParams, e))) {
      why = "integer overflow in tile lower bound";
      return failure();
    }
    lbExprs.push_back(e.ceilDiv(static_cast<uint64_t>(*d)));
  }
  for (const auto &f : tb->upperFloor) {
    auto d = bigToI64(f.divisor);
    AffineExpr e;
    if (!d || *d <= 0 ||
        failed(affineRow(f.params, faceCols, f.constant, numParams, e))) {
      why = "integer overflow in tile upper bound";
      return failure();
    }
    ubExprs.push_back(e.floorDiv(static_cast<uint64_t>(*d)) + 1);
  }
  AffineMap lbMap = AffineMap::get(numParams, 0, lbExprs, ctx);
  AffineMap ubMap = AffineMap::get(numParams, 0, ubExprs, ctx);

  // Pre-build the A and B physical-address maps over all n reduced coordinates
  // (dim j <-> y-column j), from the pre-validated access*U rows. Both emission
  // paths reuse them, so no address arithmetic can fail during emission.
  SmallVector<unsigned> allCols(n);
  for (unsigned i = 0; i < n; ++i)
    allCols[i] = i;
  std::array<SmallVector<AffineMap>, 2> abMaps;
  for (unsigned op = 0; op < 2; ++op)
    for (auto [r, row] : llvm::enumerate(MU[op])) {
      AffineExpr e;
      (void)affineRow(row, allCols, offset[op][r], n, e); // pre-validated above
      abMaps[op].push_back(AffineMap::get(n, 0, e, ctx));
    }

  // Output loops (paramCols[0 .. outputRank-1], constant box bounds).
  for (unsigned i = 0; i < outputRank; ++i) {
    auto lp = affine::AffineForOp::create(builder, loc, tb->outerLo[i],
                                          tb->outerHi[i] + 1, 1);
    yiv[tb->paramCols[i]] = lp.getInductionVar();
    builder.setInsertionPointToStart(lp.getBody());
  }

  // Physical C address (function of the output coordinates only) and the
  // accumulator slab, loaded with the current C value.
  Value zero = arith::ConstantIndexOp::create(builder, loc, 0);
  SmallVector<Value> outputIVs;
  for (unsigned c : outputCols)
    outputIVs.push_back(yiv[c]);
  SmallVector<Value> cIndex;
  for (auto [r, row] : llvm::enumerate(MU[2])) {
    AffineExpr e;
    if (failed(affineRow(row, outputCols, offset[2][r], outputRank, e))) {
      why = "integer overflow in C address";
      return failure();
    }
    auto m = AffineMap::get(outputRank, 0, e, ctx);
    cIndex.push_back(
        affine::AffineApplyOp::create(builder, loc, m, outputIVs).getResult());
  }
  Value cSlab =
      memref::AllocaOp::create(builder, loc, MemRefType::get({1, 1}, f32));
  Value cOld = memref::LoadOp::create(builder, loc, store.getMemRef(), cIndex);
  memref::StoreOp::create(builder, loc, cOld, cSlab, ValueRange{zero, zero});

  // Reduction nest: peel loops (paramCols[outputRank ..], constant box) and the
  // innermost tile loop (exact M4b.1 min/max affine faces).
  affine::AffineForOp redOuter;
  for (unsigned i = outputRank; i < tb->paramCols.size(); ++i) {
    auto lp = affine::AffineForOp::create(builder, loc, tb->outerLo[i],
                                          tb->outerHi[i] + 1, 1);
    if (i == outputRank)
      redOuter = lp;
    yiv[tb->paramCols[i]] = lp.getInductionVar();
    builder.setInsertionPointToStart(lp.getBody());
  }

  // The tile bound maps bind dim i to paramCols[i].
  SmallVector<Value> paramIVs;
  for (unsigned c : tb->paramCols)
    paramIVs.push_back(yiv[c]);
  auto toTensor = [&](Value buffer, ArrayRef<int64_t> shape,
                      bool writable) -> Value {
    return bufferization::ToTensorOp::create(
        builder, loc, RankedTensorType::get(shape, f32), buffer,
        /*restrict=*/true, writable);
  };

  if (kdev == 1) {
    // ----- Phase M4b.2a: one static 1x1x1 device tile per reduction point. --
    auto tileLoop = affine::AffineForOp::create(builder, loc, paramIVs, lbMap,
                                                paramIVs, ubMap, 1);
    if (!redOuter)
      redOuter = tileLoop; // kappa == 1: the tile loop is the outer reduction.
    yiv[tb->tileCol] = tileLoop.getInductionVar();
    builder.setInsertionPointToStart(tileLoop.getBody());
    SmallVector<Value> allIVs(yiv.begin(), yiv.end());
    auto gather = [&](unsigned op) -> Value {
      SmallVector<Value> idx;
      for (AffineMap m : abMaps[op])
        idx.push_back(
            affine::AffineApplyOp::create(builder, loc, m, allIVs).getResult());
      Value v = memref::LoadOp::create(builder, loc, loads[op].getMemRef(), idx);
      Value slab =
          memref::AllocaOp::create(builder, loc, MemRefType::get({1, 1}, f32));
      memref::StoreOp::create(builder, loc, v, slab, ValueRange{zero, zero});
      return slab;
    };
    Value aSlab = gather(0);
    Value bSlab = gather(1);
    Value result = emitSystolicTilePlan(
        builder, loc, toTensor(aSlab, {1, 1}, false),
        toTensor(bSlab, {1, 1}, false), toTensor(cSlab, {1, 1}, true), *plan,
        fleet, /*gemmIndex=*/-1);
    auto copy = bufferization::MaterializeInDestinationOp::create(builder, loc,
                                                                  result, cSlab);
    copy.setRestrict(true);
    copy.setWritable(true);
  } else {
    // ----- Phase M4b.2b-1: block the contiguous tile axis by Kdev, exact K=1
    // remainder. Gather K consecutive points from tile coordinate `base`
    // (base .. base+K-1) into 1xK / Kx1 slabs and accumulate one static
    // matmul_tile(1,1,K) into the single C slab.
    const int64_t K = static_cast<int64_t>(kdev);
    auto emitTile = [&](Value base, int64_t tileK, const SystolicTilePlan &pl) {
      Value aSlab = memref::AllocaOp::create(builder, loc,
                                             MemRefType::get({1, tileK}, f32));
      Value bSlab = memref::AllocaOp::create(builder, loc,
                                             MemRefType::get({tileK, 1}, f32));
      for (int64_t d = 0; d < tileK; ++d) {
        Value t = base;
        if (d != 0)
          t = affine::AffineApplyOp::create(
                  builder, loc,
                  AffineMap::get(1, 0, getAffineDimExpr(0, ctx) + d, ctx),
                  ValueRange{base});
        SmallVector<Value> ivs(yiv.begin(), yiv.end());
        ivs[tb->tileCol] = t;
        Value dIdx = arith::ConstantIndexOp::create(builder, loc, d);
        SmallVector<Value> aIdx, bIdx;
        for (AffineMap m : abMaps[0])
          aIdx.push_back(
              affine::AffineApplyOp::create(builder, loc, m, ivs).getResult());
        for (AffineMap m : abMaps[1])
          bIdx.push_back(
              affine::AffineApplyOp::create(builder, loc, m, ivs).getResult());
        Value av =
            memref::LoadOp::create(builder, loc, loads[0].getMemRef(), aIdx);
        Value bv =
            memref::LoadOp::create(builder, loc, loads[1].getMemRef(), bIdx);
        memref::StoreOp::create(builder, loc, av, aSlab, ValueRange{zero, dIdx});
        memref::StoreOp::create(builder, loc, bv, bSlab, ValueRange{dIdx, zero});
      }
      Value result = emitSystolicTilePlan(
          builder, loc, toTensor(aSlab, {1, tileK}, false),
          toTensor(bSlab, {tileK, 1}, false), toTensor(cSlab, {1, 1}, true), pl,
          fleet, /*gemmIndex=*/-1);
      auto copy = bufferization::MaterializeInDestinationOp::create(
          builder, loc, result, cSlab);
      copy.setRestrict(true);
      copy.setWritable(true);
    };

    // Step-Kdev loop over block starts %t in [max(lb), min(ub)) step Kdev --
    // the same bound maps as the kdev==1 tile loop (ubMap is the half-open
    // hi+1). An empty interval makes this loop run zero times (no MACs).
    auto blockLoop = affine::AffineForOp::create(builder, loc, paramIVs, lbMap,
                                                 paramIVs, ubMap, K);
    if (!redOuter)
      redOuter = blockLoop; // kappa == 1: C store after the step loop.
    OpBuilder::InsertionGuard g(builder);
    builder.setInsertionPointToStart(blockLoop.getBody());
    Value t = blockLoop.getInductionVar();

    // Build the "whole Kdev block fits" predicate and the remainder count, both
    // over the params (dims 0..numParams-1) and %t (dim numParams). For each
    // upper floor face tile <= floor(num_i/div_i): a full block needs
    //   floor(num_i/div_i) >= %t + Kdev - 1  <=>  num_i - div_i*%t - div_i*(Kdev-1) >= 0
    // (an exact affine inequality, no floordiv); the remainder covers
    //   [%t, hi] -> hi - %t + 1 = min_i(floor(num_i/div_i) - %t + 1) points.
    AffineExpr tDim = getAffineDimExpr(numParams, ctx);
    SmallVector<AffineExpr> fullConstraints, remResults;
    for (const auto &f : tb->upperFloor) {
      int64_t div = *bigToI64(f.divisor); // pre-validated above
      AffineExpr num = getAffineConstantExpr(*bigToI64(f.constant), ctx);
      for (unsigned j = 0; j < numParams; ++j)
        num = num + getAffineDimExpr(j, ctx) * (*bigToI64(f.params[j]));
      fullConstraints.push_back(num - tDim * div -
                                getAffineConstantExpr(div * (K - 1), ctx));
      remResults.push_back(num.floorDiv(static_cast<uint64_t>(div)) - tDim + 1);
    }
    SmallVector<bool> eqFlags(fullConstraints.size(), false); // all >= 0
    IntegerSet fullSet =
        IntegerSet::get(numParams + 1, 0, fullConstraints, eqFlags);
    SmallVector<Value> ifArgs(paramIVs.begin(), paramIVs.end());
    ifArgs.push_back(t);
    auto ifOp = affine::AffineIfOp::create(builder, loc, fullSet, ifArgs,
                                           /*withElseRegion=*/true);
    {
      // Full block: one static matmul_tile(1,1,Kdev) over [%t, %t+Kdev-1].
      OpBuilder::InsertionGuard g2(builder);
      builder.setInsertionPointToStart(ifOp.getThenBlock());
      emitTile(t, K, *planKdev);
    }
    {
      // Final partial block: a K=1 remainder loop over the remaining points.
      OpBuilder::InsertionGuard g2(builder);
      builder.setInsertionPointToStart(ifOp.getElseBlock());
      AffineMap remUbMap = AffineMap::get(numParams + 1, 0, remResults, ctx);
      AffineMap zeroMap = AffineMap::getConstantMap(0, ctx);
      auto remLoop = affine::AffineForOp::create(builder, loc, ValueRange{},
                                                 zeroMap, ifArgs, remUbMap, 1);
      OpBuilder::InsertionGuard g3(builder);
      builder.setInsertionPointToStart(remLoop.getBody());
      Value base = affine::AffineApplyOp::create(
          builder, loc,
          AffineMap::get(2, 0,
                         getAffineDimExpr(0, ctx) + getAffineDimExpr(1, ctx),
                         ctx),
          ValueRange{t, remLoop.getInductionVar()});
      emitTile(base, 1, *plan);
    }
  }

  // Write the accumulated fiber back to C, after the reduction nest.
  builder.setInsertionPointAfter(redOuter);
  Value cNew =
      memref::LoadOp::create(builder, loc, cSlab, ValueRange{zero, zero});
  memref::StoreOp::create(builder, loc, cNew, store.getMemRef(), cIndex);

  if (report)
    anchor->emitRemark()
        << "affine GEMM lowered by the experimental symbolic kernel-coset "
           "offload (Phase M4b.2): source MACs="
        << scan->fiber.sourceMACs.str()
        << "; output fibers=" << scan->fiber.outputFibers.str()
        << "; static K=" << kdev
        << " device tiles over exact affine tile bounds (K=1 remainder when "
           "K>1); no source enumeration";

  anchor->erase();
  return success();
}
