#include "Systolic/AffineMatmulAnalysis.h"

#include "mlir/Dialect/Arith/IR/Arith.h"

using namespace mlir;
using namespace mlir::systolic;

static StringRef classifyLoopIV(Value value,
                                affine::AffineForOp outerLoop,
                                affine::AffineForOp middleLoop,
                                affine::AffineForOp innerLoop) {
  if (value == outerLoop.getInductionVar())
    return "outer";
  if (value == middleLoop.getInductionVar())
    return "middle";
  if (value == innerLoop.getInductionVar())
    return "inner";
  return "other";
}

static FailureOr<AffineMap>
normalizeAccessMap(AffineMap map,
                   ValueRange operands,
                   affine::AffineForOp outerLoop,
                   affine::AffineForOp middleLoop,
                   affine::AffineForOp innerLoop) {
  MLIRContext *context = map.getContext();

  SmallVector<AffineExpr> dimReplacements;
  dimReplacements.reserve(operands.size());

  // Rewrite each map-local dimension into a common iteration domain:
  //
  //   d0 = outer loop IV
  //   d1 = middle loop IV
  //   d2 = inner loop IV
  //
  // For example, a map whose operands are [inner, middle] rewrites
  // its local dimensions as:
  //
  //   local d0 -> common d2
  //   local d1 -> common d1
  for (Value operand : operands) {
    if (operand == outerLoop.getInductionVar()) {
      dimReplacements.push_back(getAffineDimExpr(0, context));
    } else if (operand == middleLoop.getInductionVar()) {
      dimReplacements.push_back(getAffineDimExpr(1, context));
    } else if (operand == innerLoop.getInductionVar()) {
      dimReplacements.push_back(getAffineDimExpr(2, context));
    } else {
      return failure();
    }
  }

  // replaceDimsAndSymbols() requires one replacement for every map dim.
  if (dimReplacements.size() != map.getNumDims())
    return failure();

  // First checkpoint: handle dim-only affine accesses. Symbol support can
  // be added later without complicating the initial normalization logic.
  if (map.getNumSymbols() != 0)
    return failure();

  return map.replaceDimsAndSymbols(
      dimReplacements,
      /*symReplacements=*/{},
      /*numResultDims=*/3,
      /*numResultSyms=*/0);
}

void mlir::systolic::collectAffineMatmulCandidates(
    Operation *root,
    SmallVectorImpl<AffineMatmulCandidate> &candidates) {

  if (!root)
    return;

  root->walk([&](affine::AffineForOp outerLoop) {
    // Expect:
    //
    //   affine.for %i
    //     affine.for %j
    //       affine.for %k
    //
    // We deliberately keep discovery conservative here.

    affine::AffineForOp middleLoop;
    affine::AffineForOp innerLoop;

    // Find the unique affine.for directly nested in outerLoop.
    for (Operation &op : *outerLoop.getBody()) {
      if (auto loop = dyn_cast<affine::AffineForOp>(&op)) {
        if (middleLoop)
          return; // More than one candidate child loop.
        middleLoop = loop;
      }
    }

    if (!middleLoop)
      return;

    // Find the unique affine.for directly nested in middleLoop.
    for (Operation &op : *middleLoop.getBody()) {
      if (auto loop = dyn_cast<affine::AffineForOp>(&op)) {
        if (innerLoop)
          return;
        innerLoop = loop;
      }
    }

    if (!innerLoop)
      return;

    // Avoid discovering the same nest again starting from middleLoop/innerLoop.
    if (outerLoop->getParentOfType<affine::AffineForOp>())
      return;

    AffineMatmulCandidate candidate;
    candidate.anchor = outerLoop.getOperation();

    llvm::errs() << "Found affine matmul candidate:\n";
    llvm::errs() << "  outer IV: " << outerLoop.getInductionVar() << "\n";
    llvm::errs() << "  middle IV: " << middleLoop.getInductionVar() << "\n";
    llvm::errs() << "  inner IV: " << innerLoop.getInductionVar() << "\n";

    candidates.push_back(candidate);
  });
}

FailureOr<LogicalMatmul>
mlir::systolic::recognizeLogicalMatmul(AffineMatmulCandidate candidate) {
  if (!candidate.anchor)
    return failure();

  auto outerLoop = dyn_cast<affine::AffineForOp>(candidate.anchor);
  if (!outerLoop)
    return failure();

  affine::AffineForOp middleLoop;
  affine::AffineForOp innerLoop;

  for (Operation &op : *outerLoop.getBody()) {
    if (auto loop = dyn_cast<affine::AffineForOp>(&op)) {
      if (middleLoop)
        return failure();
      middleLoop = loop;
    }
  }

  if (!middleLoop)
    return failure();

  for (Operation &op : *middleLoop.getBody()) {
    if (auto loop = dyn_cast<affine::AffineForOp>(&op)) {
      if (innerLoop)
        return failure();
      innerLoop = loop;
    }
  }

  if (!innerLoop)
    return failure();

  SmallVector<affine::AffineLoadOp> loads;
  SmallVector<affine::AffineStoreOp> stores;

  candidate.anchor->walk([&](Operation *op) {
    if (auto load = dyn_cast<affine::AffineLoadOp>(op))
      loads.push_back(load);
    else if (auto store = dyn_cast<affine::AffineStoreOp>(op))
      stores.push_back(store);
  });

  llvm::errs() << "Affine matmul recognition:\n";
  llvm::errs() << "  loads: " << loads.size() << "\n";
  llvm::errs() << "  stores: " << stores.size() << "\n";

  for (auto [index, load] : llvm::enumerate(loads)) {
    llvm::errs() << "  load[" << index << "] map: ";
    load.getMap().print(llvm::errs());
    llvm::errs() << "\n";

    llvm::errs() << "    operands:";
    for (Value operand : load.getIndices())
      llvm::errs() << " "
                   << classifyLoopIV(operand, outerLoop, middleLoop, innerLoop);
    llvm::errs() << "\n";
  }

  for (auto [index, store] : llvm::enumerate(stores)) {
    llvm::errs() << "  store[" << index << "] map: ";
    store.getMap().print(llvm::errs());
    llvm::errs() << "\n";

    llvm::errs() << "    operands:";
    for (Value operand : store.getIndices())
      llvm::errs() << " "
                   << classifyLoopIV(operand, outerLoop, middleLoop, innerLoop);
    llvm::errs() << "\n";
  }

  // Canonical GEMM currently expected to contain:
  //
  //   A load
  //   B load
  //   C load
  //   C store
  //
  // This is only the first recognition checkpoint.  We have not yet
  // established multiply-accumulate semantics or recovered logical I/J/K.
  if (loads.size() != 3 || stores.size() != 1)
    return failure();

  affine::AffineStoreOp store = stores.front();

  // Follow the SSA def-use chain backward from the stored value.
  auto add = store.getValue().getDefiningOp<arith::AddFOp>();
  if (!add)
    return failure();

  affine::AffineLoadOp accLoad;
  arith::MulFOp mul;

  // Accept either:
  //
  //   acc + lhs * rhs
  //
  // or, because addf is commutative:
  //
  //   lhs * rhs + acc
  if (auto load = add.getLhs().getDefiningOp<affine::AffineLoadOp>()) {
    if (auto candidateMul =
            add.getRhs().getDefiningOp<arith::MulFOp>()) {
      accLoad = load;
      mul = candidateMul;
    }
  }

  if (!accLoad || !mul) {
    if (auto load =
            add.getRhs().getDefiningOp<affine::AffineLoadOp>()) {
      if (auto candidateMul =
              add.getLhs().getDefiningOp<arith::MulFOp>()) {
        accLoad = load;
        mul = candidateMul;
      }
    }
  }

  if (!accLoad || !mul)
    return failure();

  auto lhsLoad =
      mul.getLhs().getDefiningOp<affine::AffineLoadOp>();
  auto rhsLoad =
      mul.getRhs().getDefiningOp<affine::AffineLoadOp>();

  if (!lhsLoad || !rhsLoad)
    return failure();

  // The accumulator must be read from and written back to the same
  // memref location.
  if (accLoad.getMemref() != store.getMemref())
    return failure();

  if (accLoad.getMap() != store.getMap())
    return failure();

  auto loadIndices = accLoad.getIndices();
  auto storeIndices = store.getIndices();

  if (loadIndices.size() != storeIndices.size())
    return failure();

  for (unsigned i = 0; i < loadIndices.size(); ++i) {
    if (loadIndices[i] != storeIndices[i])
      return failure();
  }

  // Recover canonical logical GEMM coordinates from access operands:
  //
  //   A = (I, K)
  //   B = (K, J)
  //   C = (I, J)
  //
  // This first checkpoint handles direct loop-IV operands only.
  auto normalizedLhs =
      normalizeAccessMap(lhsLoad.getMap(), lhsLoad.getIndices(),
                         outerLoop, middleLoop, innerLoop);
  auto normalizedRhs =
      normalizeAccessMap(rhsLoad.getMap(), rhsLoad.getIndices(),
                         outerLoop, middleLoop, innerLoop);
  auto normalizedAcc =
      normalizeAccessMap(accLoad.getMap(), accLoad.getIndices(),
                         outerLoop, middleLoop, innerLoop);

  if (failed(normalizedLhs) ||
      failed(normalizedRhs) ||
      failed(normalizedAcc))
    return failure();

  llvm::errs() << "  normalized A: ";
  normalizedLhs->print(llvm::errs());
  llvm::errs() << "\n";

  llvm::errs() << "  normalized B: ";
  normalizedRhs->print(llvm::errs());
  llvm::errs() << "\n";

  llvm::errs() << "  normalized C: ";
  normalizedAcc->print(llvm::errs());
  llvm::errs() << "\n";

  // A logical GEMM has the normalized access pattern:
  //
  //   A = (I, K)
  //   B = (K, J)
  //   C = (I, J)
  //
  // Unlike the previous direct-IV checkpoint, I/J/K are AffineExprs.
  // This allows coordinates such as I = d0 + d1.
  if (normalizedLhs->getNumResults() != 2 ||
      normalizedRhs->getNumResults() != 2 ||
      normalizedAcc->getNumResults() != 2)
    return failure();

  AffineExpr logicalI = normalizedLhs->getResult(0);
  AffineExpr logicalK = normalizedLhs->getResult(1);
  AffineExpr logicalJ = normalizedRhs->getResult(1);

  if (logicalI != normalizedAcc->getResult(0))
    return failure();

  if (logicalJ != normalizedAcc->getResult(1))
    return failure();

  if (logicalK != normalizedRhs->getResult(0))
    return failure();

  // Keep I, J, and K logically distinct.
  if (logicalI == logicalJ ||
      logicalI == logicalK ||
      logicalJ == logicalK)
    return failure();

  llvm::errs() << "  logical GEMM coordinates recovered\n";
  llvm::errs() << "    I: ";
  logicalI.print(llvm::errs());
  llvm::errs() << "\n";

  llvm::errs() << "    J: ";
  logicalJ.print(llvm::errs());
  llvm::errs() << "\n";

  llvm::errs() << "    K: ";
  logicalK.print(llvm::errs());
  llvm::errs() << "\n";

  llvm::errs() << "  accumulator read-modify-write verified\n";
  llvm::errs() << "  MAC dataflow recognized\n";
  llvm::errs() << "    accumulator load: ";
  accLoad.getOperation()->print(llvm::errs());
  llvm::errs() << "\n";

  llvm::errs() << "    lhs load: ";
  lhsLoad.getOperation()->print(llvm::errs());
  llvm::errs() << "\n";

  llvm::errs() << "    rhs load: ";
  rhsLoad.getOperation()->print(llvm::errs());
  llvm::errs() << "\n";

  LogicalMatmul result;
  result.anchor = candidate.anchor;

  result.lhs = lhsLoad.getMemRef();
  result.rhs = rhsLoad.getMemRef();
  result.output = store.getMemRef();

  result.accessA = *normalizedLhs;
  result.accessB = *normalizedRhs;
  result.accessC = *normalizedAcc;

  result.rowMap =
      AffineMap::get(/*dimCount=*/3, /*symbolCount=*/0,
                     logicalI, candidate.anchor->getContext());
  result.columnMap =
      AffineMap::get(/*dimCount=*/3, /*symbolCount=*/0,
                     logicalJ, candidate.anchor->getContext());
  result.reductionMap =
      AffineMap::get(/*dimCount=*/3, /*symbolCount=*/0,
                     logicalK, candidate.anchor->getContext());

  return result;
}
