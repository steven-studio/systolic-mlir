#include "Systolic/Passes.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Bufferization/IR/Bufferization.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/Dialect/Linalg/IR/Linalg.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Transforms/GreedyPatternRewriteDriver.h"

using namespace mlir;
using namespace mlir::systolic;

namespace {

// -----------------------------------------------------------------------
// torch-mlir does not lower torch.dot to linalg.dot; it lowers to a pair
// of linalg.generic ops -- an elementwise multiply followed by a
// reduction-to-scalar add -- with no named op in between that a
// type-based OpRewritePattern (as used by every other pattern in this
// file) can match on. This pattern instead structurally matches that
// specific two-op chain, verifying every step, and refuses to match
// anything else expressed via linalg.generic (e.g. softmax, which also
// uses linalg.generic for its reduction and elementwise steps but with
// a different body -- arith.maximumf instead of arith.addf, and no
// elementwise-multiply predecessor).
//
// This is deliberately conservative: any deviation from the exact
// expected shape -- wrong iterator_types, wrong region body op, wrong
// number of results, a producer that is not itself a matching
// elementwise-multiply linalg.generic -- causes the pattern to decline
// the match rather than guess. Declining is always safe here; the op
// is simply left unconverted, same as any other pattern in this file
// when its shape/type preconditions are not met.
// -----------------------------------------------------------------------

static Value tensorToMemref(PatternRewriter &rewriter, Location loc,
                             Value tensorVal, RankedTensorType ty) {
  auto memrefTy = MemRefType::get(ty.getShape(), ty.getElementType());
  return rewriter.create<bufferization::ToMemrefOp>(loc, memrefTy, tensorVal);
}

static Value memrefToLLVMPtr(PatternRewriter &rewriter, Location loc,
                              Value memref) {
  Value idx =
      rewriter.create<memref::ExtractAlignedPointerAsIndexOp>(loc, memref);
  Value idxAsI64 =
      rewriter.create<arith::IndexCastOp>(loc, rewriter.getI64Type(), idx);
  auto ptrTy = LLVM::LLVMPointerType::get(rewriter.getContext());
  return rewriter.create<LLVM::IntToPtrOp>(loc, ptrTy, idxAsI64);
}

// Checks that `op`'s single-block region body is exactly:
//   %r = <ExpectedArithOp>(%bbArg0, %bbArg1)
//   linalg.yield %r
// i.e. a single binary arithmetic op consuming the first two block
// arguments and nothing else.
template <typename ExpectedArithOp>
static bool hasSimpleBinaryBody(linalg::GenericOp op) {
  Block &body = op.getRegion().front();
  if (body.getNumArguments() < 2)
    return false;
  auto &ops = body.getOperations();
  // Expect exactly two ops: the arith op, then linalg.yield.
  if (ops.size() != 2)
    return false;
  auto arithOp = llvm::dyn_cast<ExpectedArithOp>(ops.front());
  if (!arithOp)
    return false;
  auto yieldOp = llvm::dyn_cast<linalg::YieldOp>(ops.back());
  if (!yieldOp || yieldOp.getNumOperands() != 1)
    return false;
  if (yieldOp.getOperand(0) != arithOp.getResult())
    return false;
  // Both arith operands must be the block's own arguments (not, e.g.,
  // some captured outer value), in either order.
  Value a = arithOp.getOperand(0), b = arithOp.getOperand(1);
  Value arg0 = body.getArgument(0), arg1 = body.getArgument(1);
  return (a == arg0 && b == arg1) || (a == arg1 && b == arg0);
}

struct DotGenericToFpgaPattern : public OpRewritePattern<linalg::GenericOp> {
  using OpRewritePattern::OpRewritePattern;

  LogicalResult matchAndRewrite(linalg::GenericOp reduceOp,
                                 PatternRewriter &rewriter) const override {
    // --- Step 1: reduceOp must be a rank-0-result reduction over a
    //     single 1-D input, with body "acc + in". ---
    if (reduceOp.getNumDpsInputs() != 1 || reduceOp.getNumDpsInits() != 1)
      return rewriter.notifyMatchFailure(reduceOp, "not a single-input reduction");

    auto iterTypes = reduceOp.getIteratorTypesArray();
    if (iterTypes.size() != 1 ||
        iterTypes[0] != utils::IteratorType::reduction)
      return rewriter.notifyMatchFailure(reduceOp, "not a rank-1 reduction");

    if (!hasSimpleBinaryBody<arith::AddFOp>(reduceOp))
      return rewriter.notifyMatchFailure(
          reduceOp, "reduction body is not a simple addf (e.g. this is "
                     "softmax's reduce-max, not a dot-product sum)");

    Value reduceInput = reduceOp.getDpsInputOperand(0)->get();
    Value reduceInit = reduceOp.getDpsInitOperand(0)->get();

    auto resultTy =
        llvm::dyn_cast<RankedTensorType>(reduceOp.getResult(0).getType());
    if (!resultTy || resultTy.getRank() != 0 ||
        !resultTy.getElementType().isF32())
      return rewriter.notifyMatchFailure(
          reduceOp, "reduction result is not a static-shape f32 scalar");

    // The init must come from a linalg.fill with a zero constant --
    // otherwise this reduction has a nonzero starting accumulator and
    // is not a plain dot product.
    auto fillOp = reduceInit.getDefiningOp<linalg::FillOp>();
    if (!fillOp)
      return rewriter.notifyMatchFailure(
          reduceOp, "reduction init is not a linalg.fill (nonzero or "
                     "dynamic accumulator seed)");
    auto fillCst = fillOp.getInputs()[0].getDefiningOp<arith::ConstantOp>();
    if (!fillCst)
      return rewriter.notifyMatchFailure(reduceOp, "fill value is not constant");
    auto fillAttr = llvm::dyn_cast<FloatAttr>(fillCst.getValue());
    if (!fillAttr || !fillAttr.getValue().isZero())
      return rewriter.notifyMatchFailure(reduceOp, "fill value is not zero");

    // --- Step 2: reduceInput must itself be produced by a matching
    //     elementwise-multiply linalg.generic over two 1-D f32 tensors
    //     of the same static shape. ---
    auto mulOp = reduceInput.getDefiningOp<linalg::GenericOp>();
    if (!mulOp)
      return rewriter.notifyMatchFailure(
          reduceOp, "reduction input is not produced by a linalg.generic "
                     "(no elementwise-multiply predecessor found)");

    if (mulOp.getNumDpsInputs() != 2 || mulOp.getNumDpsInits() != 1)
      return rewriter.notifyMatchFailure(mulOp, "not a binary elementwise op");

    auto mulIterTypes = mulOp.getIteratorTypesArray();
    if (mulIterTypes.size() != 1 ||
        mulIterTypes[0] != utils::IteratorType::parallel)
      return rewriter.notifyMatchFailure(mulOp, "not a rank-1 elementwise op");

    if (!hasSimpleBinaryBody<arith::MulFOp>(mulOp))
      return rewriter.notifyMatchFailure(
          mulOp, "elementwise body is not a simple mulf");

    Value x = mulOp.getDpsInputOperand(0)->get();
    Value y = mulOp.getDpsInputOperand(1)->get();
    auto xTy = llvm::dyn_cast<RankedTensorType>(x.getType());
    auto yTy = llvm::dyn_cast<RankedTensorType>(y.getType());
    if (!xTy || !yTy || !xTy.hasStaticShape() || !yTy.hasStaticShape() ||
        xTy.getRank() != 1 || yTy.getRank() != 1 ||
        xTy.getShape() != yTy.getShape() || !xTy.getElementType().isF32())
      return rewriter.notifyMatchFailure(
          mulOp, "operands are not matching static-shape rank-1 f32 tensors");

    int64_t K = xTy.getShape()[0];

    // --- All checks passed: this is exactly x[K] . y[K]. Emit the call. ---
    Location loc = reduceOp.getLoc();
    auto module = reduceOp->getParentOfType<ModuleOp>();

    auto ptrTy = LLVM::LLVMPointerType::get(rewriter.getContext());
    auto i32Ty = rewriter.getI32Type();
    StringRef fnName = "fpga_dot_tiled_auto";
    auto fnTy = LLVM::LLVMFunctionType::get(
        i32Ty, {i32Ty, ptrTy, ptrTy, ptrTy}, /*isVarArg=*/false);

    auto dotFunc = module.lookupSymbol<LLVM::LLVMFuncOp>(fnName);
    if (!dotFunc) {
      OpBuilder::InsertionGuard guard(rewriter);
      rewriter.setInsertionPointToStart(module.getBody());
      dotFunc = rewriter.create<LLVM::LLVMFuncOp>(module.getLoc(), fnName, fnTy);
    }

    auto scalarMemrefTy = MemRefType::get({}, resultTy.getElementType());

    Value xMemref = tensorToMemref(rewriter, loc, x, xTy);
    Value yMemref = tensorToMemref(rewriter, loc, y, yTy);
    // The result is a rank-0 memref; reuse the reduction's own init
    // buffer (already zero-filled) as backing storage so this pattern
    // does not need to know rank-0 tensor.empty()'s exact syntax.
    // Bufferize reduceInit (the zero-filled linalg.fill output that is
    // reduceOp's "outs" operand) -- NOT reduceOp's own result. Using
    // reduceOp's result here would create a circular def-use chain: this
    // memref would depend on reduceOp's result, but reduceOp's result is
    // about to be replaced by a tensor derived from this very memref.
    Value resultMemref = rewriter.create<bufferization::ToMemrefOp>(
        loc, scalarMemrefTy, reduceInit);

    Value xPtr = memrefToLLVMPtr(rewriter, loc, xMemref);
    Value yPtr = memrefToLLVMPtr(rewriter, loc, yMemref);
    Value resultPtr = memrefToLLVMPtr(rewriter, loc, resultMemref);

    auto ci32 = [&](int64_t v) {
      return rewriter.create<arith::ConstantIntOp>(loc, v, 32);
    };

    // fpga_dot_tiled_auto(K, x*, y*, result*)
    rewriter.create<LLVM::CallOp>(
        loc, dotFunc, ValueRange{ci32(K), xPtr, yPtr, resultPtr});

    Value resultTensor = rewriter.create<bufferization::ToTensorOp>(
        loc, resultTy, resultMemref, /*restrict=*/true, /*writable=*/true);
    rewriter.replaceOp(reduceOp, resultTensor);
    // mulOp's only use was reduceOp's input, which is now gone. Erase it
    // explicitly rather than relying on the greedy driver's dead-code
    // pass to notice: leaving it around risks the driver revisiting this
    // now-orphaned linalg.generic, and this pattern matching it again on
    // a different (or even the same) code path, causing non-termination.
    rewriter.eraseOp(mulOp);
    return success();
  }
};

struct DotGenericToFpgaPass
    : public PassWrapper<DotGenericToFpgaPass, OperationPass<ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(DotGenericToFpgaPass)

  StringRef getArgument() const final { return "dot-generic-to-fpga"; }
  StringRef getDescription() const final {
    return "Structurally match the elementwise-multiply + "
           "reduction-to-scalar linalg.generic pair that torch-mlir "
           "emits for torch.dot, and lower it to a call into "
           "fpga_dot_tiled_auto. Declines to match any linalg.generic "
           "that does not exactly fit this shape (e.g. softmax).";
  }

  void getDependentDialects(DialectRegistry &registry) const override {
    registry.insert<linalg::LinalgDialect, bufferization::BufferizationDialect,
                     memref::MemRefDialect, LLVM::LLVMDialect,
                     arith::ArithDialect>();
  }

  void runOnOperation() override {
    RewritePatternSet patterns(&getContext());
    patterns.add<DotGenericToFpgaPattern>(&getContext());
    if (failed(applyPatternsAndFoldGreedily(getOperation(),
                                             std::move(patterns))))
      signalPassFailure();
  }
};

} // namespace

std::unique_ptr<Pass> mlir::systolic::createDotGenericToFpgaPass() {
  return std::make_unique<DotGenericToFpgaPass>();
}

void mlir::systolic::registerDotGenericToFpgaPass() {
  PassRegistration<DotGenericToFpgaPass>();
}
