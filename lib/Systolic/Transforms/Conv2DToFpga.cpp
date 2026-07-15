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
// Lowers linalg.conv_2d_nhwc_hwcf (batch size 1, static shape) to a call
// into fpga_conv2d_im2col_auto, which performs im2col unfolding on the
// host and reuses fpga_matmul_tiled_auto (Section: tile-matmul-for-fpga)
// to dispatch the resulting matmul to the same 4x4 accelerator. No new
// hardware or UART protocol is required; only host-side data reshaping.
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

struct Conv2DToFpgaPattern
    : public OpRewritePattern<linalg::Conv2DNhwcHwcfOp> {
  using OpRewritePattern::OpRewritePattern;

  LogicalResult matchAndRewrite(linalg::Conv2DNhwcHwcfOp op,
                                 PatternRewriter &rewriter) const override {
    Value input = op.getInputs()[0];   // [N, H, W, Cin]
    Value filter = op.getInputs()[1];  // [Kh, Kw, Cin, Cout]
    Value output = op.getOutputs()[0]; // [N, Hout, Wout, Cout]

    auto inTy = llvm::dyn_cast<RankedTensorType>(input.getType());
    auto filTy = llvm::dyn_cast<RankedTensorType>(filter.getType());
    auto outTy = llvm::dyn_cast<RankedTensorType>(output.getType());
    if (!inTy || !filTy || !outTy || !inTy.hasStaticShape() ||
        !filTy.hasStaticShape() || !outTy.hasStaticShape())
      return rewriter.notifyMatchFailure(op, "only static-shape conv2d");

    if (!inTy.getElementType().isF32())
      return rewriter.notifyMatchFailure(op, "only f32 supported");

    // Batch size, asymmetric stride, and dilation are all supported
    // via fpga_conv2d_im2col_general_auto (Section: Extending to
    // Convolution). No shape or attribute constraint is imposed here
    // beyond static shape and f32 element type, checked above.
    int64_t Nb = inTy.getShape()[0];
    int64_t H = inTy.getShape()[1];
    int64_t W = inTy.getShape()[2];
    int64_t Cin = inTy.getShape()[3];
    int64_t Kh = filTy.getShape()[0];
    int64_t Kw = filTy.getShape()[1];
    int64_t Cout = filTy.getShape()[3];

    auto strides = op.getStrides();
    int64_t strideH = strides.getValues<int64_t>()[0];
    int64_t strideW = strides.getValues<int64_t>()[1];

    auto dilations = op.getDilations();
    int64_t dilationH = dilations.getValues<int64_t>()[0];
    int64_t dilationW = dilations.getValues<int64_t>()[1];

    Location loc = op.getLoc();
    auto module = op->getParentOfType<ModuleOp>();

    auto ptrTy = LLVM::LLVMPointerType::get(rewriter.getContext());
    auto i32Ty = rewriter.getI32Type();
    StringRef fnName = "fpga_conv2d_im2col_general_auto";
    auto fnTy = LLVM::LLVMFunctionType::get(
        i32Ty,
        {i32Ty, i32Ty, i32Ty, i32Ty, i32Ty, i32Ty, i32Ty,
         i32Ty, i32Ty, i32Ty, i32Ty, ptrTy, ptrTy, ptrTy},
        /*isVarArg=*/false);

    auto convFunc = module.lookupSymbol<LLVM::LLVMFuncOp>(fnName);
    if (!convFunc) {
      OpBuilder::InsertionGuard guard(rewriter);
      rewriter.setInsertionPointToStart(module.getBody());
      convFunc =
          rewriter.create<LLVM::LLVMFuncOp>(module.getLoc(), fnName, fnTy);
    }

    Value inMemref = tensorToMemref(rewriter, loc, input, inTy);
    Value filMemref = tensorToMemref(rewriter, loc, filter, filTy);
    Value outMemref = tensorToMemref(rewriter, loc, output, outTy);

    Value inPtr = memrefToLLVMPtr(rewriter, loc, inMemref);
    Value filPtr = memrefToLLVMPtr(rewriter, loc, filMemref);
    Value outPtr = memrefToLLVMPtr(rewriter, loc, outMemref);

    auto ci32 = [&](int64_t v) {
      return rewriter.create<arith::ConstantIntOp>(loc, v, 32);
    };

    rewriter.create<LLVM::CallOp>(
        loc, convFunc,
        ValueRange{ci32(Nb), ci32(H), ci32(W), ci32(Cin), ci32(Kh), ci32(Kw),
                   ci32(Cout), ci32(strideH), ci32(strideW),
                   ci32(dilationH), ci32(dilationW), inPtr, filPtr, outPtr});

    Value resultTensor = rewriter.create<bufferization::ToTensorOp>(
        loc, outTy, outMemref, /*restrict=*/true, /*writable=*/true);
    rewriter.replaceOp(op, resultTensor);
    return success();
  }
};

struct Conv2DToFpgaPass
    : public PassWrapper<Conv2DToFpgaPass, OperationPass<ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(Conv2DToFpgaPass)

  StringRef getArgument() const final { return "conv2d-to-fpga"; }
  StringRef getDescription() const final {
    return "Lower linalg.conv_2d_nhwc_hwcf (batch=1, static shape) to a "
           "call into an im2col-based runtime that reuses the 4x4 matmul "
           "accelerator";
  }

  void getDependentDialects(DialectRegistry &registry) const override {
    registry.insert<linalg::LinalgDialect, bufferization::BufferizationDialect,
                     memref::MemRefDialect, LLVM::LLVMDialect,
                     arith::ArithDialect>();
  }

  void runOnOperation() override {
    RewritePatternSet patterns(&getContext());
    patterns.add<Conv2DToFpgaPattern>(&getContext());
    if (failed(applyPatternsAndFoldGreedily(getOperation(),
                                             std::move(patterns))))
      signalPassFailure();
  }
};

} // namespace

std::unique_ptr<Pass> mlir::systolic::createConv2DToFpgaPass() {
  return std::make_unique<Conv2DToFpgaPass>();
}

void mlir::systolic::registerConv2DToFpgaPass() {
  PassRegistration<Conv2DToFpgaPass>();
}
