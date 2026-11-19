//===- MatmulTileLowering.cpp - What the GEMM lowerings share --------------===//
//
// The alias gate, an operand's view, an operand gathered through a table, the hardware
// plan's tiles, their emission and the write-back: see Systolic/MatmulTileLowering.h. The
// alias gate, the operand's view and the offsets of a view were AffineMatmulToSystolic.cpp's
// own; the plan, the tiles and the write-back were written out once in the dense lowering
// and once in the lowering from records; the tables were the lowering from records' own.
//
//===----------------------------------------------------------------------===//

#include "Systolic/MatmulTileLowering.h"

#include "Systolic/AffineMatmulPartitioning.h"
#include "Systolic/SystolicOps.h"

#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Bufferization/IR/Bufferization.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/Interfaces/FunctionInterfaces.h"
#include "mlir/Interfaces/ViewLikeInterface.h"

#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/CheckedArithmetic.h"

#include <functional>

using namespace mlir;
using namespace mlir::systolic;

/// A function argument the function declares restrict (C `restrict`,
/// llvm.noalias): nothing else the function sees overlaps the memory it
/// writes through it.
static bool isNoAliasArgument(Value memref) {
  auto arg = dyn_cast<BlockArgument>(memref);
  if (!arg || !arg.getOwner()->isEntryBlock())
    return false;
  auto fn = dyn_cast<FunctionOpInterface>(arg.getOwner()->getParentOp());
  return fn && fn.getArgAttr(arg.getArgNumber(),
                             LLVM::LLVMDialect::getNoAliasAttrName());
}

static bool isFreshAllocation(Value memref) {
  return memref.getDefiningOp<memref::AllocOp>() ||
         memref.getDefiningOp<memref::AllocaOp>();
}

/// The buffer a memref is a view of (through subview, cast, reshapes, ...).
static Value getViewRoot(Value memref) {
  while (auto view = memref.getDefiningOp<ViewLikeOpInterface>())
    memref = view.getViewSource();
  return memref;
}

bool mlir::systolic::provablyDisjointFromOutput(Value input, Value output,
                                                bool assumeNoAlias) {
  if (input == output)
    return false;
  if (assumeNoAlias)
    return true;
  Value inputRoot = getViewRoot(input);
  Value outputRoot = getViewRoot(output);
  if (inputRoot == outputRoot)
    return false;
  if (isFreshAllocation(inputRoot) || isFreshAllocation(outputRoot))
    return true;
  return isNoAliasArgument(outputRoot) || isNoAliasArgument(inputRoot);
}

/// `expr` (over `symbols`, no dims) plus `constant`, as index arithmetic at
/// the builder's point: the run-time offset of a view dimension. A bare
/// symbol with no constant is the symbol's value itself.
static Value materializeOffset(OpBuilder &builder, Location loc,
                               AffineExpr expr, ArrayRef<Value> symbols,
                               int64_t constant) {
  std::function<Value(AffineExpr)> build = [&](AffineExpr e) -> Value {
    if (auto c = dyn_cast<AffineConstantExpr>(e))
      return arith::ConstantIndexOp::create(builder, loc, c.getValue());
    if (auto sym = dyn_cast<AffineSymbolExpr>(e))
      return symbols[sym.getPosition()];
    auto bin = cast<AffineBinaryOpExpr>(e);
    Value lhs = build(bin.getLHS()), rhs = build(bin.getRHS());
    switch (e.getKind()) {
    case AffineExprKind::Add:
      return arith::AddIOp::create(builder, loc, lhs, rhs);
    case AffineExprKind::Mul:
      return arith::MulIOp::create(builder, loc, lhs, rhs);
    default:
      // separateSymbols refuses floordiv, ceildiv and mod. Not
      // llvm_unreachable: in a build without assertions that is undefined
      // behaviour, which once lowered s0 floordiv 2 as s0 * 2.
      llvm::report_fatal_error(
          "materializeOffset: a symbolic offset that is not a sum of products");
    }
  };
  Value v = build(expr);
  if (constant != 0)
    v = arith::AddIOp::create(
        builder, loc, v, arith::ConstantIndexOp::create(builder, loc, constant));
  return v;
}

Value mlir::systolic::materializeOperandView(
    OpBuilder &builder, Location loc, const LogicalOperand &operand,
    ArrayRef<Value> batchIVs, ArrayRef<unsigned> batchLoops,
    llvm::DenseMap<AffineMap, Value> &applies) {
  if (operand.isWholeMemref())
    return operand.memref;
  auto sourceTy = cast<MemRefType>(operand.memref.getType());
  if (operand.explicitView) {
    MLIRContext *ctx = builder.getContext();
    OpFoldResult offset = builder.getIndexAttr(operand.elementOffset);
    int64_t layoutOffset = operand.elementOffset;
    AffineExpr expr = getAffineConstantExpr(operand.elementOffset, ctx);
    bool batched = false;
    for (auto [j, s] : llvm::enumerate(batchLoops)) {
      const int64_t coefficient =
          s < operand.elementBatchCoefficients.size()
              ? operand.elementBatchCoefficients[s]
              : 0;
      if (coefficient == 0)
        continue;
      expr = expr + getAffineDimExpr(j, ctx) * coefficient;
      batched = true;
    }
    if (batched) {
      AffineMap map = AffineMap::get(batchIVs.size(), /*symbolCount=*/0, expr);
      Value &index = applies[map];
      if (!index)
        index = affine::AffineApplyOp::create(builder, loc, map, batchIVs)
                    .getResult();
      offset = index;
      layoutOffset = ShapedType::kDynamic;
    }
    // The symbols' terms on the element offset: per dimension with a
    // run-time offset, its term times the memref's row-major stride.
    if (operand.hasSymbolicOffsets()) {
      SmallVector<int64_t> rowMajor(sourceTy.getRank(), 1);
      for (int64_t d = sourceTy.getRank() - 2; d >= 0; --d)
        rowMajor[d] = rowMajor[d + 1] * sourceTy.getDimSize(d + 1);
      Value sum = batched ? cast<Value>(offset)
                          : arith::ConstantIndexOp::create(
                                builder, loc, operand.elementOffset);
      for (int64_t d = 0; d < sourceTy.getRank(); ++d) {
        if (!operand.hasSymbolicOffset(d))
          continue;
        Value term = materializeOffset(builder, loc,
                                       operand.symbolic.getResult(d),
                                       operand.symbols, /*constant=*/0);
        if (rowMajor[d] != 1)
          term = arith::MulIOp::create(
              builder, loc, term,
              arith::ConstantIndexOp::create(builder, loc, rowMajor[d]));
        sum = arith::AddIOp::create(builder, loc, sum, term);
      }
      offset = sum;
      layoutOffset = ShapedType::kDynamic;
    }
    SmallVector<int64_t> viewShape(operand.shape.begin(), operand.shape.end());
    SmallVector<int64_t> viewStrides(operand.elementStrides.begin(),
                                     operand.elementStrides.end());
    auto viewTy = MemRefType::get(
        viewShape, sourceTy.getElementType(),
        StridedLayoutAttr::get(ctx, layoutOffset, viewStrides),
        sourceTy.getMemorySpace());
    SmallVector<OpFoldResult> sizes, strides;
    for (unsigned r = 0; r < 2; ++r) {
      sizes.push_back(builder.getIndexAttr(viewShape[r]));
      strides.push_back(builder.getIndexAttr(viewStrides[r]));
    }
    return memref::ReinterpretCastOp::create(builder, loc, viewTy,
                                             operand.memref, offset, sizes,
                                             strides);
  }
  // The subview keeps the role dimensions (in memref order), drops the rest.
  SmallVector<int64_t> keptShape;
  SmallVector<int> kept;
  for (int64_t d = 0; d < sourceTy.getRank(); ++d)
    if (d == operand.roleDim[0] || d == operand.roleDim[1]) {
      kept.push_back(d);
      keptShape.push_back(operand.sizes[d]);
    }
  SmallVector<OpFoldResult> offsets, sizes, strides;
  for (int64_t d = 0; d < sourceTy.getRank(); ++d) {
    offsets.push_back(builder.getIndexAttr(operand.offsets[d]));
    sizes.push_back(builder.getIndexAttr(operand.sizes[d]));
    strides.push_back(builder.getIndexAttr(operand.strides[d]));
  }
  for (int64_t d = 0; d < sourceTy.getRank(); ++d) {
    if (size_t(d) >= operand.batchCoefficients.size())
      continue;
    AffineExpr expr =
        getAffineConstantExpr(operand.offsets[d], builder.getContext());
    bool batched = false;
    for (auto [j, s] : llvm::enumerate(batchLoops)) {
      const int64_t coefficient = operand.batchCoefficients[d][s];
      if (coefficient == 0)
        continue;
      expr = expr + getAffineDimExpr(j, builder.getContext()) * coefficient;
      batched = true;
    }
    if (!batched)
      continue;
    // One dimension per batch loop, whether or not this offset uses it (the
    // map's dimension count must match the operands).
    // Operands that read the batch coordinate through the same expression
    // share one index (the batch loops' counters are the same values).
    AffineMap map = AffineMap::get(batchIVs.size(), /*symbolCount=*/0, expr);
    Value &index = applies[map];
    if (!index)
      index =
          affine::AffineApplyOp::create(builder, loc, map, batchIVs).getResult();
    offsets[d] = index;
  }
  // A dimension the nest's symbols offset: the static offset (or the batch
  // terms' affine.apply, which already holds it) plus the symbols' term,
  // as index arithmetic -- a dynamic offset of the subview (a bare symbol
  // with no static part is the symbol's value itself).
  for (int64_t d = 0; d < sourceTy.getRank(); ++d) {
    if (!operand.hasSymbolicOffset(d))
      continue;
    AffineExpr expr = operand.symbolic.getResult(d);
    if (auto base = dyn_cast<Value>(offsets[d])) {
      Value term = materializeOffset(builder, loc, expr, operand.symbols,
                                     /*constant=*/0);
      offsets[d] = arith::AddIOp::create(builder, loc, base, term).getResult();
    } else {
      offsets[d] = materializeOffset(builder, loc, expr, operand.symbols,
                                     operand.offsets[d]);
    }
  }
  MemRefType subviewTy = memref::SubViewOp::inferRankReducedResultType(
      keptShape, sourceTy, offsets, sizes, strides);
  Value view = memref::SubViewOp::create(builder, loc, subviewTy,
                                         operand.memref, offsets, sizes,
                                         strides);
  // The roles in the other order: swap the two dimensions.
  if (operand.isTransposed()) {
    AffineMap swap = AffineMap::getPermutationMap(ArrayRef<unsigned>{1, 0},
                                                  builder.getContext());
    view = memref::TransposeOp::create(builder, loc, view,
                                       AffineMapAttr::get(swap));
  }
  // A role the operand has no dimension for: a unit dimension in its place
  // (both roles: an operand of batch dimensions only, a rank-0 view, becomes
  // the 1 x 1 matrix).
  if (kept.size() < 2) {
    SmallVector<int64_t> shape(operand.shape.begin(), operand.shape.end());
    SmallVector<ReassociationIndices> reassociation;
    if (kept.size() == 1)
      reassociation.push_back(ReassociationIndices{0, 1});
    view = memref::ExpandShapeOp::create(builder, loc, shape, view,
                                         reassociation);
  }
  return view;
}

LogicalResult mlir::systolic::planMatmulTiles(
    Operation *anchor, int64_t M, int64_t N, int64_t K,
    const SystolicFleet &fleet, double sharedReadBytesPerCycle,
    StringRef costModel, StringRef operandPath,
    std::optional<SystolicTilePlan> &plan, SmallVectorImpl<LogicalBox> &boxes,
    std::string &why, bool &hardError) {
  if (!fleet.resources.empty()) {
    SystolicOptimizerOptions options;
    options.sharedReadBytesPerCycle = sharedReadBytesPerCycle;
    if (!parseCostModelVersion(costModel, options.costModel) ||
        (operandPath != "v1" && operandPath != "v2")) {
      why = "cost-model and operand-path must each be v1 or v2";
      hardError = true;
      return failure();
    }
    options.beatWideOperandPath = operandPath == "v2";
    FailureOr<SystolicTilePlan> planned =
        planSystolicMatmul(anchor, M, N, K, fleet, options);
    if (failed(planned)) {
      hardError = true;
      return failure();
    }
    plan = std::move(*planned);
    for (const SystolicTileBlock &block : plan->blocks)
      for (const SystolicTileInvocation &inv : block.invocations) {
        LogicalBox box;
        box.lower = {block.row, block.column, inv.k0};
        box.extent = {block.m, block.n, inv.k};
        boxes.push_back(box);
      }
  } else {
    LogicalBox whole;
    whole.extent = {M, N, K};
    boxes.push_back(whole);
  }
  return success();
}

LogicalResult mlir::systolic::planMatmulTiles(
    Operation *anchor, int64_t M, int64_t N, int64_t K,
    const SystolicFleet &fleet, double sharedReadBytesPerCycle,
    StringRef costModel, StringRef operandPath,
    std::optional<SystolicTilePlan> &plan, size_t &regions, std::string &why,
    bool &hardError) {
  SmallVector<LogicalBox> boxes;
  if (failed(planMatmulTiles(anchor, M, N, K, fleet, sharedReadBytesPerCycle,
                             costModel, operandPath, plan, boxes, why,
                             hardError)))
    return failure();
  // The tiles are an exact partition of the box: inside it, pairwise disjoint, and as many
  // points as it has (a domain counted from 0 along I, J and K).
  {
    const std::array<int64_t, 3> extent{M, N, K};
    std::optional<int64_t> volume = int64_t(0);
    for (auto [b, box] : llvm::enumerate(boxes)) {
      for (unsigned a = 0; a < 3; ++a)
        if (box.lower[a] < 0 || box.extent[a] <= 0 || box.lower[a] + box.extent[a] > extent[a]) {
          why = "the partition is not exact: a tile leaves the box";
          return failure();
        }
      for (unsigned o = 0; o < b; ++o) {
        bool apart = false;
        for (unsigned a = 0; a < 3; ++a)
          apart = apart || boxes[o].lower[a] + boxes[o].extent[a] <= box.lower[a] ||
                  box.lower[a] + box.extent[a] <= boxes[o].lower[a];
        if (!apart) {
          why = "the partition is not exact: two tiles overlap";
          return failure();
        }
      }
      std::optional<int64_t> v = llvm::checkedMul(box.extent[0], box.extent[1]);
      v = v ? llvm::checkedMul(*v, box.extent[2]) : std::nullopt;
      volume = volume && v ? llvm::checkedAdd(*volume, *v) : std::nullopt;
    }
    std::optional<int64_t> whole = llvm::checkedMul(M, N);
    whole = whole ? llvm::checkedMul(*whole, K) : std::nullopt;
    if (!volume || !whole || *volume != *whole) {
      why = "the partition is not exact: the tiles do not cover the box";
      return failure();
    }
  }
  regions = boxes.size();
  return success();
}

Value mlir::systolic::emitMatmulTiles(
    OpBuilder &builder, Location loc, Value aTensor, Value bTensor,
    Value cTensor, int64_t M, int64_t N, int64_t K,
    const std::optional<SystolicTilePlan> &plan, const SystolicFleet &fleet,
    int64_t &nextGemmIndex) {
  if (plan)
    return emitSystolicTilePlan(builder, loc, aTensor, bTensor, cTensor, *plan,
                                fleet, nextGemmIndex++);
  return MatmulTileOp::create(
      builder, loc, cTensor.getType(), aTensor, bTensor, cTensor,
      builder.getI64IntegerAttr(M), builder.getI64IntegerAttr(N),
      builder.getI64IntegerAttr(K), /*device=*/FlatSymbolRefAttr(),
      /*est_cycles=*/IntegerAttr(), /*start_cycle=*/IntegerAttr());
}

void mlir::systolic::writeBackMatmulResult(OpBuilder &builder, Location loc,
                                           Value result, Value destination) {
  auto writeBack = bufferization::MaterializeInDestinationOp::create(
      builder, loc, result, destination);
  writeBack.setRestrict(true);
  writeBack.setWritable(true);
}

/// matrix[r][c] = the operand's element at the offset table[r][c] (the gather), or the
/// reverse (the scatter): two affine.for over the rows and the columns, the offset an i64
/// of the table cast to index, the operand read or written as a flat buffer.
static void copyByTable(OpBuilder &builder, Location loc, Value table, Value flat,
                        Value matrix, std::array<int64_t, 2> shape, bool scatter) {
  auto rows = affine::AffineForOp::create(builder, loc, 0, shape[0], 1);
  OpBuilder::InsertionGuard guard(builder);
  builder.setInsertionPointToStart(rows.getBody());
  auto columns = affine::AffineForOp::create(builder, loc, 0, shape[1], 1);
  builder.setInsertionPointToStart(columns.getBody());
  SmallVector<Value, 2> rc{rows.getInductionVar(), columns.getInductionVar()};
  Value offset64 = affine::AffineLoadOp::create(builder, loc, table, rc);
  Value offset = arith::IndexCastOp::create(builder, loc, builder.getIndexType(), offset64);
  if (!scatter) {
    Value v = memref::LoadOp::create(builder, loc, flat, ValueRange{offset});
    affine::AffineStoreOp::create(builder, loc, v, matrix, rc);
  } else {
    Value v = affine::AffineLoadOp::create(builder, loc, matrix, rc);
    memref::StoreOp::create(builder, loc, v, flat, ValueRange{offset});
  }
}

TableOperands mlir::systolic::materializeTableOperands(
    OpBuilder &builder, Location loc, ModuleOp module, StringRef label,
    const std::array<TableOperand, 3> &operands, Type elementType) {
  auto symbolName = [&](StringRef base) {
    std::string name = base.str();
    for (unsigned n = 1; SymbolTable::lookupSymbolIn(module, name); ++n)
      name = base.str() + "_" + std::to_string(n);
    return name;
  };
  std::string stem = ("__nest_table_" + label).str();
  for (char &ch : stem)
    if (!llvm::isAlnum(ch) && ch != '_')
      ch = '_';
  std::array<Value, 3> table, flat, matrix;
  static const char *const kName[] = {"A", "B", "C"};
  for (unsigned x = 0; x < 3; ++x) {
    auto tableTy = MemRefType::get(operands[x].shape, builder.getI64Type());
    auto values = DenseElementsAttr::get(RankedTensorType::get(operands[x].shape, builder.getI64Type()),
                                         ArrayRef<int64_t>(operands[x].table));
    std::string name = symbolName(stem + "_" + kName[x]);
    {
      OpBuilder::InsertionGuard guard(builder);
      builder.setInsertionPointToStart(module.getBody());
      memref::GlobalOp::create(builder, loc, name, builder.getStringAttr("private"), tableTy,
                               values, /*constant=*/true, /*alignment=*/IntegerAttr());
    }
    table[x] = memref::GetGlobalOp::create(builder, loc, tableTy, name);
    auto memTy = cast<MemRefType>(operands[x].memref.getType());
    const int64_t elements = memTy.getNumElements();
    auto flatTy = MemRefType::get({elements}, elementType);
    flat[x] = memref::ReinterpretCastOp::create(
        builder, loc, flatTy, operands[x].memref, /*offset=*/builder.getIndexAttr(0),
        ArrayRef<OpFoldResult>{builder.getIndexAttr(elements)},
        ArrayRef<OpFoldResult>{builder.getIndexAttr(1)});
    matrix[x] = memref::AllocOp::create(builder, loc, MemRefType::get(operands[x].shape, elementType));
  }
  // matrix[x][r][c] = operand x at table[x][r][c].
  for (unsigned x = 0; x < 3; ++x)
    copyByTable(builder, loc, table[x], flat[x], matrix[x], operands[x].shape, /*scatter=*/false);
  TableOperands made;
  made.matrix = matrix;
  made.table = table;
  made.flat = flat;
  for (unsigned x = 0; x < 3; ++x)
    made.shape[x] = operands[x].shape;
  return made;
}

void mlir::systolic::scatterTableResult(OpBuilder &builder, Location loc,
                                        const TableOperands &tables) {
  copyByTable(builder, loc, tables.table[2], tables.flat[2], tables.matrix[2], tables.shape[2],
              /*scatter=*/true);
}
