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
// NCHW/FCHW variant of Conv2DToFpgaPattern (see Conv2DToFpga.cpp), for
// linalg.conv_2d_nchw_fchw -- the layout PyTorch models lower to by
// default via torch-mlir, as opposed to the NHWC/HWCF layout
// Conv2DToFpgaPattern targets. Structurally identical three-step
// rewrite (buffer materialization, external call emission, result
// rematerialization); only the target runtime symbol differs, since
// the layout-specific im2col repacking (Kernel transpose, output
// transpose) lives entirely in fpga_conv2d_im2col_nchw_general_auto
// on the C side, not in this pattern.
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

struct Conv2DNchwToFpgaPattern
    : public OpRewritePattern<linalg::Conv2DNchwFchwOp> {
  using OpRewritePattern::OpRewritePattern;

  LogicalResult matchAndRewrite(linalg::Conv2DNchwFchwOp op,
                                 PatternRewriter &rewriter) const override {
    Value input = op.getInputs()[0];   // [N, Cin, H, W]
    Value filter = op.getInputs()[1];  // [Cout, Cin, Kh, Kw]
    Value output = op.getOutputs()[0]; // [N, Cout, Hout, Wout]

    auto inTy = llvm::dyn_cast<RankedTensorType>(input.getType());
    auto filTy = llvm::dyn_cast<RankedTensorType>(filter.getType());
    auto outTy = llvm::dyn_cast<RankedTensorType>(output.getType());
    if (!inTy || !filTy || !outTy || !inTy.hasStaticShape() ||
        !filTy.hasStaticShape() || !outTy.hasStaticShape())
      return rewriter.notifyMatchFailure(op, "only static-shape conv2d");

    if (!inTy.getElementType().isF32())
      return rewriter.notifyMatchFailure(op, "only f32 supported");

    int64_t Nb = inTy.getShape()[0];
    int64_t Cin = inTy.getShape()[1];
    int64_t H = inTy.getShape()[2];
    int64_t W = inTy.getShape()[3];
    int64_t Cout = filTy.getShape()[0];
    int64_t Kh = filTy.getShape()[2];
    int64_t Kw = filTy.getShape()[3];

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
    StringRef fnName = "fpga_conv2d_im2col_nchw_general_auto";
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

    // fpga_conv2d_im2col_nchw_general_auto(N, Cin, H, W, Cout, Kh, Kw,
    //     strideH, strideW, dilationH, dilationW, X*, Kernel*, Y*)
    rewriter.create<LLVM::CallOp>(
        loc, convFunc,
        ValueRange{ci32(Nb), ci32(Cin), ci32(H), ci32(W), ci32(Cout),
                   ci32(Kh), ci32(Kw), ci32(strideH), ci32(strideW),
                   ci32(dilationH), ci32(dilationW), inPtr, filPtr, outPtr});

    Value resultTensor = rewriter.create<bufferization::ToTensorOp>(
        loc, outTy, outMemref, /*restrict=*/true, /*writable=*/true);
    rewriter.replaceOp(op, resultTensor);
    return success();
  }
};

struct Conv2DNchwToFpgaPass
    : public PassWrapper<Conv2DNchwToFpgaPass, OperationPass<ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(Conv2DNchwToFpgaPass)

  StringRef getArgument() const final { return "conv2d-nchw-to-fpga"; }
  StringRef getDescription() const final {
    return "Lower linalg.conv_2d_nchw_fchw (static shape) to a call into "
           "an im2col-based runtime that reuses the 4x4 matmul "
           "accelerator, matching PyTorch's default NCHW/FCHW layout";
  }

  void getDependentDialects(DialectRegistry &registry) const override {
    registry.insert<linalg::LinalgDialect, bufferization::BufferizationDialect,
                     memref::MemRefDialect, LLVM::LLVMDialect,
                     arith::ArithDialect>();
  }

  void runOnOperation() override {
    RewritePatternSet patterns(&getContext());
    patterns.add<Conv2DNchwToFpgaPattern>(&getContext());
    if (failed(applyPatternsAndFoldGreedily(getOperation(),
                                             std::move(patterns))))
      signalPassFailure();
  }
};

} // namespace

std::unique_ptr<Pass> mlir::systolic::createConv2DNchwToFpgaPass() {
  return std::make_unique<Conv2DNchwToFpgaPass>();
}

void mlir::systolic::registerConv2DNchwToFpgaPass() {
  PassRegistration<Conv2DNchwToFpgaPass>();
}
