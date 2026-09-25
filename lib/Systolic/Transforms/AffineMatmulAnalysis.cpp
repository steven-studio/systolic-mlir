#include "Systolic/AffineMatmulAnalysis.h"

#include "mlir/Dialect/Arith/IR/Arith.h"

#include <utility>

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

static SmallVector<Value>
collectAccessIterationIVs(affine::AffineLoadOp lhsLoad,
                          affine::AffineLoadOp rhsLoad,
                          affine::AffineLoadOp accLoad,
                          affine::AffineStoreOp store) {
  SmallVector<Value> usedIVs;

  auto collectUsedIVs = [&](ValueRange indices) {
    for (Value value : indices) {
      if (llvm::find(usedIVs, value) == usedIVs.end())
        usedIVs.push_back(value);
    }
  };

  collectUsedIVs(lhsLoad.getIndices());
  collectUsedIVs(rhsLoad.getIndices());
  collectUsedIVs(accLoad.getIndices());
  collectUsedIVs(store.getIndices());

  // Recover the enclosing affine.for chain around the MAC, initially from
  // inner to outer.
  SmallVector<affine::AffineForOp> enclosingLoops;
  for (Operation *op = store.getOperation()->getParentOp();
       op != nullptr;
       op = op->getParentOp()) {
    if (auto loop = dyn_cast<affine::AffineForOp>(op))
      enclosingLoops.push_back(loop);
  }

  // Canonicalize the iteration domain to lexical outer-to-inner order.
  std::reverse(enclosingLoops.begin(), enclosingLoops.end());

  SmallVector<Value> iterationIVs;
  for (affine::AffineForOp loop : enclosingLoops) {
    Value iv = loop.getInductionVar();
    if (llvm::find(usedIVs, iv) != usedIVs.end())
      iterationIVs.push_back(iv);
  }

  return iterationIVs;
}

static FailureOr<AffineMap>
normalizeAccessMap(AffineMap map,
                   ValueRange operands,
                   ValueRange iterationIVs) {
  MLIRContext *context = map.getContext();

  SmallVector<AffineExpr> dimReplacements;
  dimReplacements.reserve(operands.size());

  // Rewrite each map-local dimension into a common iteration domain:
  //
  //   iterationIVs[p] -> common dp
  //
  // The common domain may have any number of access-related loop IVs.
  for (Value operand : operands) {
    auto it = llvm::find(iterationIVs, operand);
    if (it == iterationIVs.end())
      return failure();

    unsigned position =
        static_cast<unsigned>(std::distance(iterationIVs.begin(), it));
    dimReplacements.push_back(getAffineDimExpr(position, context));
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
      /*numResultDims=*/iterationIVs.size(),
      /*numResultSyms=*/0);
}

struct LinearForm {
  SmallVector<int64_t> coefficients;
  int64_t constant = 0;
};

static FailureOr<LinearForm>
extractLinearForm(AffineExpr expr, unsigned numDims) {
  switch (expr.getKind()) {
  case AffineExprKind::DimId: {
    auto dim = cast<AffineDimExpr>(expr);
    if (dim.getPosition() >= numDims)
      return failure();

    LinearForm result;
    result.coefficients.assign(numDims, 0);
    result.coefficients[dim.getPosition()] = 1;
    return result;
  }

  case AffineExprKind::Constant: {
    LinearForm result;
    result.coefficients.assign(numDims, 0);
    result.constant = cast<AffineConstantExpr>(expr).getValue();
    return result;
  }

  case AffineExprKind::Add: {
    auto binary = cast<AffineBinaryOpExpr>(expr);

    auto lhs = extractLinearForm(binary.getLHS(), numDims);
    auto rhs = extractLinearForm(binary.getRHS(), numDims);
    if (failed(lhs) || failed(rhs))
      return failure();

    LinearForm result;
    result.coefficients.assign(numDims, 0);
    for (unsigned i = 0; i < numDims; ++i)
      result.coefficients[i] =
          lhs->coefficients[i] + rhs->coefficients[i];
    result.constant = lhs->constant + rhs->constant;
    return result;
  }

  case AffineExprKind::Mul: {
    auto binary = cast<AffineBinaryOpExpr>(expr);

    auto scale =
        dyn_cast<AffineConstantExpr>(binary.getRHS());
    if (!scale)
      return failure();

    auto lhs = extractLinearForm(binary.getLHS(), numDims);
    if (failed(lhs))
      return failure();

    LinearForm result;
    result.coefficients.assign(numDims, 0);
    int64_t factor = scale.getValue();

    for (unsigned i = 0; i < numDims; ++i)
      result.coefficients[i] =
          lhs->coefficients[i] * factor;
    result.constant = lhs->constant * factor;
    return result;
  }

  case AffineExprKind::SymbolId:
  case AffineExprKind::Mod:
  case AffineExprKind::FloorDiv:
  case AffineExprKind::CeilDiv:
    return failure();
  }

  return failure();
}

static FailureOr<int64_t>
recoverStaticIterationOrigin(Value iv) {
  auto blockArg = dyn_cast<BlockArgument>(iv);
  if (!blockArg)
    return failure();

  auto loop =
      dyn_cast<affine::AffineForOp>(blockArg.getOwner()->getParentOp());
  if (!loop || loop.getInductionVar() != iv)
    return failure();

  if (!loop.hasConstantLowerBound())
    return failure();

  return loop.getConstantLowerBound();
}

static bool
isInvertibleSquareMapping(
    const SmallVector<SmallVector<int64_t>> &transformation) {
  const unsigned size = transformation.size();
  if (size == 0)
    return false;

  for (const auto &row : transformation) {
    if (row.size() != size)
      return false;
  }

  // Fraction-free Gaussian elimination.  This tests full rank exactly over
  // the rationals without introducing floating-point tolerances.
  SmallVector<SmallVector<int64_t>> matrix = transformation;
  int64_t previousPivot = 1;

  for (unsigned column = 0; column + 1 < size; ++column) {
    unsigned pivotRow = column;
    while (pivotRow < size && matrix[pivotRow][column] == 0)
      ++pivotRow;

    if (pivotRow == size)
      return false;

    if (pivotRow != column)
      std::swap(matrix[pivotRow], matrix[column]);

    const int64_t pivot = matrix[column][column];

    for (unsigned row = column + 1; row < size; ++row) {
      for (unsigned j = column + 1; j < size; ++j) {
        const int64_t numerator =
            matrix[row][j] * pivot -
            matrix[row][column] * matrix[column][j];

        matrix[row][j] = numerator / previousPivot;
      }

      matrix[row][column] = 0;
    }

    previousPivot = pivot;
  }

  return matrix[size - 1][size - 1] != 0;
}

static LogicalExtent
recoverLogicalExtent(AffineExpr expr,
                     ValueRange iterationIVs) {
  LogicalExtent result;

  // First checkpoint: only recover extents for a logical coordinate that is
  // exactly one iteration dimension.  Composed coordinates such as d0 + d1
  // remain valid GEMM coordinates, but their extent is currently unknown.
  auto dimExpr = dyn_cast<AffineDimExpr>(expr);
  if (!dimExpr)
    return result;

  unsigned position = dimExpr.getPosition();
  if (position >= iterationIVs.size())
    return result;

  Value iv = iterationIVs[position];

  auto blockArg = dyn_cast<BlockArgument>(iv);
  if (!blockArg)
    return result;

  auto loop =
      dyn_cast<affine::AffineForOp>(blockArg.getOwner()->getParentOp());
  if (!loop || loop.getInductionVar() != iv)
    return result;

  // Static affine.for trip count.
  if (loop.hasConstantLowerBound() &&
      loop.hasConstantUpperBound()) {
    int64_t lower = loop.getConstantLowerBound();
    int64_t upper = loop.getConstantUpperBound();
    int64_t step = loop.getStep().getSExtValue();

    if (step <= 0 || upper <= lower)
      return result;

    result.kind = LogicalExtent::Kind::Static;
    result.staticValue = (upper - lower + step - 1) / step;
    return result;
  }

  // First symbolic checkpoint:
  //
  //   affine.for %iv = 0 to %n
  //
  // is represented as:
  //
  //   LB: () -> (0)
  //   UB: ()[s0] -> (s0), operand = %n
  //
  // Keep this deliberately narrow.  More general affine bounds can be
  // supported later without weakening GEMM recognition.
  if (loop.getStep() != 1)
    return result;

  if (!loop.hasConstantLowerBound() ||
      loop.getConstantLowerBound() != 0)
    return result;

  AffineMap upperMap = loop.getUpperBoundMap();
  auto upperOperands = loop.getUpperBoundOperands();

  if (upperMap.getNumDims() != 0 ||
      upperMap.getNumSymbols() != 1 ||
      upperMap.getNumResults() != 1 ||
      upperOperands.size() != 1)
    return result;

  auto symbolExpr =
      dyn_cast<AffineSymbolExpr>(upperMap.getResult(0));
  if (!symbolExpr || symbolExpr.getPosition() != 0)
    return result;

  result.kind = LogicalExtent::Kind::Symbolic;
  result.symbolicValue = *upperOperands.begin();
  return result;
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

  auto iterationIVs =
      collectAccessIterationIVs(lhsLoad, rhsLoad, accLoad, store);

  llvm::errs() << "  access iteration IV count: "
               << iterationIVs.size() << "\n";

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
                         iterationIVs);
  auto normalizedRhs =
      normalizeAccessMap(rhsLoad.getMap(), rhsLoad.getIndices(),
                         iterationIVs);
  auto normalizedAcc =
      normalizeAccessMap(accLoad.getMap(), accLoad.getIndices(),
                         iterationIVs);

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

  // A logical GEMM may have a shared partition prefix P*:
  //
  //   A = (P*, I, K)
  //   B = (P*, K, J)
  //   C = (P*, I, J)
  //
  // Ordinary GEMM is the special case where P* is empty.
  unsigned numAccessResults = normalizedAcc->getNumResults();
  if (numAccessResults < 2 ||
      normalizedLhs->getNumResults() != numAccessResults ||
      normalizedRhs->getNumResults() != numAccessResults)
    return failure();

  unsigned numPartitionDims = numAccessResults - 2;

  // Multiplication is commutative, so the source operand order does not
  // determine which load is logical A=(P*,I,K) and which is B=(P*,K,J).
  affine::AffineLoadOp aLoad = lhsLoad;
  affine::AffineLoadOp bLoad = rhsLoad;
  AffineMap accessA = *normalizedLhs;
  AffineMap accessB = *normalizedRhs;

  auto matchesGemmAccesses =
      [&](AffineMap a, AffineMap b) {
        for (unsigned p = 0; p < numPartitionDims; ++p) {
          if (a.getResult(p) != normalizedAcc->getResult(p) ||
              b.getResult(p) != normalizedAcc->getResult(p))
            return false;
        }

        AffineExpr i = a.getResult(numPartitionDims);
        AffineExpr k = a.getResult(numPartitionDims + 1);
        AffineExpr j = b.getResult(numPartitionDims + 1);

        return i == normalizedAcc->getResult(numPartitionDims) &&
               j == normalizedAcc->getResult(numPartitionDims + 1) &&
               k == b.getResult(numPartitionDims) &&
               i != j && i != k && j != k;
      };

  if (!matchesGemmAccesses(accessA, accessB)) {
    if (!matchesGemmAccesses(accessB, accessA))
      return failure();

    std::swap(aLoad, bLoad);
    std::swap(accessA, accessB);
  }

  AffineExpr logicalI = accessA.getResult(numPartitionDims);
  AffineExpr logicalK = accessA.getResult(numPartitionDims + 1);
  AffineExpr logicalJ = accessB.getResult(numPartitionDims + 1);

  unsigned numIterationDims = accessA.getNumDims();

  auto linearI = extractLinearForm(logicalI, numIterationDims);
  auto linearJ = extractLinearForm(logicalJ, numIterationDims);
  auto linearK = extractLinearForm(logicalK, numIterationDims);

  if (failed(linearI) ||
      failed(linearJ) ||
      failed(linearK))
    return failure();

  llvm::errs() << "  affine logical mapping\n";
  if (succeeded(linearI) &&
      succeeded(linearJ) &&
      succeeded(linearK)) {
    auto printLinearForm = [](StringRef name,
                              const LinearForm &form) {
      llvm::errs() << "    " << name << " = [";
      for (unsigned i = 0; i < form.coefficients.size(); ++i) {
        if (i != 0)
          llvm::errs() << " ";
        llvm::errs() << form.coefficients[i];
      }
      llvm::errs() << " | " << form.constant << "]\n";
    };

    printLinearForm("I", *linearI);
    printLinearForm("J", *linearJ);
    printLinearForm("K", *linearK);
  } else {
    llvm::errs() << "    unsupported non-linear affine form\n";
  }

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
  result.outputStore = store.getOperation();

  SmallVector<int64_t> sourceOrigin;
  sourceOrigin.reserve(iterationIVs.size());

  bool hasStaticSourceOrigin = true;
  for (Value iv : iterationIVs) {
    auto origin = recoverStaticIterationOrigin(iv);
    if (failed(origin)) {
      hasStaticSourceOrigin = false;
      break;
    }

    sourceOrigin.push_back(*origin);
  }

  if (hasStaticSourceOrigin) {
    result.logicalMapping.sourceOrigin = std::move(sourceOrigin);
    result.logicalMapping.hasStaticSourceOrigin = true;
  }

  result.lhs = aLoad.getMemRef();
  result.rhs = bLoad.getMemRef();
  result.output = store.getMemRef();

  result.accessA = accessA;
  result.accessB = accessB;
  result.accessC = *normalizedAcc;

  SmallVector<AffineExpr> logicalPartitions;
  logicalPartitions.reserve(numPartitionDims);
  for (unsigned p = 0; p < numPartitionDims; ++p)
    logicalPartitions.push_back(accessA.getResult(p));

  // Materialize the complete affine coordinate transformation
  //
  //   y = T x + c
  //
  // where y = (P*, I, J, K).
  for (AffineExpr logicalPartition : logicalPartitions) {
    auto linearPartition =
        extractLinearForm(logicalPartition, numIterationDims);
    if (failed(linearPartition))
      return failure();

    result.logicalMapping.transformation.push_back(
        linearPartition->coefficients);
    result.logicalMapping.offset.push_back(
        linearPartition->constant);
  }

  result.logicalMapping.transformation.push_back(linearI->coefficients);
  result.logicalMapping.transformation.push_back(linearJ->coefficients);
  result.logicalMapping.transformation.push_back(linearK->coefficients);

  result.logicalMapping.offset.push_back(linearI->constant);
  result.logicalMapping.offset.push_back(linearJ->constant);
  result.logicalMapping.offset.push_back(linearK->constant);

  result.logicalMapping.isInvertible =
      isInvertibleSquareMapping(
          result.logicalMapping.transformation);

  result.partitionMap =
      AffineMap::get(/*dimCount=*/numIterationDims,
                     /*symbolCount=*/0,
                     logicalPartitions,
                     candidate.anchor->getContext());
  result.rowMap =
      AffineMap::get(/*dimCount=*/numIterationDims,
                     /*symbolCount=*/0,
                     logicalI,
                     candidate.anchor->getContext());
  result.columnMap =
      AffineMap::get(/*dimCount=*/numIterationDims,
                     /*symbolCount=*/0,
                     logicalJ,
                     candidate.anchor->getContext());
  result.reductionMap =
      AffineMap::get(/*dimCount=*/numIterationDims,
                     /*symbolCount=*/0,
                     logicalK,
                     candidate.anchor->getContext());

  result.rowExtent =
      recoverLogicalExtent(logicalI, iterationIVs);
  result.columnExtent =
      recoverLogicalExtent(logicalJ, iterationIVs);
  result.reductionExtent =
      recoverLogicalExtent(logicalK, iterationIVs);

  return result;
}
