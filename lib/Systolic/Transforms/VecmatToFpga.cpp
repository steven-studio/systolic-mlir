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
// Lowers linalg.vecmat (y[N] = x[K] @ A[K,N], static shape, f32) to a
// call into fpga_vecmat_tiled_auto, which treats x as a 1xK matrix and
// reuses fpga_matmul_tiled_auto (Section: tile-matmul-for-fpga) with
// M=1. torch-mlir emits this op for nn.Linear applied to a rank-1,
// unbatched input, as distinct from linalg.batch_matmul (rank-3,
// batched inputs) or linalg.matmul (rank-2, unbatched but with an
// explicit leading dimension).
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

struct VecmatToFpgaPattern : public OpRewritePattern<linalg::VecmatOp> {
  using OpRewritePattern::OpRewritePattern;

  LogicalResult matchAndRewrite(linalg::VecmatOp op,
                                 PatternRewriter &rewriter) const override {
    Value x = op.getInputs()[0]; // [K]
    Value a = op.getInputs()[1]; // [K, N]
    Value y = op.getOutputs()[0]; // [N]

    auto xTy = llvm::dyn_cast<RankedTensorType>(x.getType());
    auto aTy = llvm::dyn_cast<RankedTensorType>(a.getType());
    auto yTy = llvm::dyn_cast<RankedTensorType>(y.getType());
    if (!xTy || !aTy || !yTy || !xTy.hasStaticShape() ||
        !aTy.hasStaticShape() || !yTy.hasStaticShape())
      return rewriter.notifyMatchFailure(op, "只处理静态形状的 vecmat");

    if (!xTy.getElementType().isF32())
      return rewriter.notifyMatchFailure(op, "目前只支援 f32");

    int64_t K = xTy.getShape()[0];
    int64_t N = aTy.getShape()[1];

    Location loc = op.getLoc();
    auto module = op->getParentOfType<ModuleOp>();

    auto ptrTy = LLVM::LLVMPointerType::get(rewriter.getContext());
    auto i32Ty = rewriter.getI32Type();
    StringRef fnName = "fpga_vecmat_tiled_auto";
    auto fnTy = LLVM::LLVMFunctionType::get(
        i32Ty, {i32Ty, i32Ty, ptrTy, ptrTy, ptrTy}, /*isVarArg=*/false);

    auto vecmatFunc = module.lookupSymbol<LLVM::LLVMFuncOp>(fnName);
    if (!vecmatFunc) {
      OpBuilder::InsertionGuard guard(rewriter);
      rewriter.setInsertionPointToStart(module.getBody());
      vecmatFunc =
          rewriter.create<LLVM::LLVMFuncOp>(module.getLoc(), fnName, fnTy);
    }

    Value xMemref = tensorToMemref(rewriter, loc, x, xTy);
    Value aMemref = tensorToMemref(rewriter, loc, a, aTy);
    Value yMemref = tensorToMemref(rewriter, loc, y, yTy);

    Value xPtr = memrefToLLVMPtr(rewriter, loc, xMemref);
    Value aPtr = memrefToLLVMPtr(rewriter, loc, aMemref);
    Value yPtr = memrefToLLVMPtr(rewriter, loc, yMemref);

    auto ci32 = [&](int64_t v) {
      return rewriter.create<arith::ConstantIntOp>(loc, v, 32);
    };

    // fpga_vecmat_tiled_auto(K, N, x*, A*, y*)
    rewriter.create<LLVM::CallOp>(
        loc, vecmatFunc, ValueRange{ci32(K), ci32(N), xPtr, aPtr, yPtr});

    Value resultTensor = rewriter.create<bufferization::ToTensorOp>(
        loc, yTy, yMemref, /*restrict=*/true, /*writable=*/true);
    rewriter.replaceOp(op, resultTensor);
    return success();
  }
};

struct VecmatToFpgaPass
    : public PassWrapper<VecmatToFpgaPass, OperationPass<ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(VecmatToFpgaPass)

  StringRef getArgument() const final { return "vecmat-to-fpga"; }
  StringRef getDescription() const final {
    return "Lower linalg.vecmat (static shape) to a call into "
           "fpga_vecmat_tiled_auto, treating the input vector as a "
           "1xK matrix dispatched to the 4x4 matmul accelerator";
  }

  void getDependentDialects(DialectRegistry &registry) const override {
    registry.insert<linalg::LinalgDialect, bufferization::BufferizationDialect,
                     memref::MemRefDialect, LLVM::LLVMDialect,
                     arith::ArithDialect>();
  }

  void runOnOperation() override {
    RewritePatternSet patterns(&getContext());
    patterns.add<VecmatToFpgaPattern>(&getContext());
    if (failed(applyPatternsAndFoldGreedily(getOperation(),
                                             std::move(patterns))))
      signalPassFailure();
  }
};

} // namespace

std::unique_ptr<Pass> mlir::systolic::createVecmatToFpgaPass() {
  return std::make_unique<VecmatToFpgaPass>();
}

void mlir::systolic::registerVecmatToFpgaPass() {
  PassRegistration<VecmatToFpgaPass>();
}
