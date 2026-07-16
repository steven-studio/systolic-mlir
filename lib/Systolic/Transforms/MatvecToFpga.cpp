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
// Lowers linalg.matvec (y[M] = A[M,K] @ x[K], static shape, f32) to a
// call into fpga_matvec_tiled_auto, which treats x as a Kx1 matrix and
// reuses fpga_matmul_tiled_auto (Section: tile-matmul-for-fpga) with
// N=1 -- the mirror image of VecmatToFpgaPattern's M=1. torch-mlir
// emits this op for an explicit A @ x matrix-vector product, as
// distinct from linalg.vecmat's x @ A vector-matrix product (operand
// order in ins() is swapped between the two).
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

struct MatvecToFpgaPattern : public OpRewritePattern<linalg::MatvecOp> {
  using OpRewritePattern::OpRewritePattern;

  LogicalResult matchAndRewrite(linalg::MatvecOp op,
                                 PatternRewriter &rewriter) const override {
    Value a = op.getInputs()[0]; // [M, K]
    Value x = op.getInputs()[1]; // [K]
    Value y = op.getOutputs()[0]; // [M]

    auto aTy = llvm::dyn_cast<RankedTensorType>(a.getType());
    auto xTy = llvm::dyn_cast<RankedTensorType>(x.getType());
    auto yTy = llvm::dyn_cast<RankedTensorType>(y.getType());
    if (!aTy || !xTy || !yTy || !aTy.hasStaticShape() ||
        !xTy.hasStaticShape() || !yTy.hasStaticShape())
      return rewriter.notifyMatchFailure(op, "只处理静态形状的 matvec");

    if (!aTy.getElementType().isF32())
      return rewriter.notifyMatchFailure(op, "目前只支援 f32");

    int64_t M = aTy.getShape()[0];
    int64_t K = aTy.getShape()[1];

    Location loc = op.getLoc();
    auto module = op->getParentOfType<ModuleOp>();

    auto ptrTy = LLVM::LLVMPointerType::get(rewriter.getContext());
    auto i32Ty = rewriter.getI32Type();
    StringRef fnName = "fpga_matvec_tiled_auto";
    auto fnTy = LLVM::LLVMFunctionType::get(
        i32Ty, {i32Ty, i32Ty, ptrTy, ptrTy, ptrTy}, /*isVarArg=*/false);

    auto matvecFunc = module.lookupSymbol<LLVM::LLVMFuncOp>(fnName);
    if (!matvecFunc) {
      OpBuilder::InsertionGuard guard(rewriter);
      rewriter.setInsertionPointToStart(module.getBody());
      matvecFunc =
          rewriter.create<LLVM::LLVMFuncOp>(module.getLoc(), fnName, fnTy);
    }

    Value aMemref = tensorToMemref(rewriter, loc, a, aTy);
    Value xMemref = tensorToMemref(rewriter, loc, x, xTy);
    Value yMemref = tensorToMemref(rewriter, loc, y, yTy);

    Value aPtr = memrefToLLVMPtr(rewriter, loc, aMemref);
    Value xPtr = memrefToLLVMPtr(rewriter, loc, xMemref);
    Value yPtr = memrefToLLVMPtr(rewriter, loc, yMemref);

    auto ci32 = [&](int64_t v) {
      return rewriter.create<arith::ConstantIntOp>(loc, v, 32);
    };

    // fpga_matvec_tiled_auto(M, K, A*, x*, y*)
    rewriter.create<LLVM::CallOp>(
        loc, matvecFunc, ValueRange{ci32(M), ci32(K), aPtr, xPtr, yPtr});

    Value resultTensor = rewriter.create<bufferization::ToTensorOp>(
        loc, yTy, yMemref, /*restrict=*/true, /*writable=*/true);
    rewriter.replaceOp(op, resultTensor);
    return success();
  }
};

struct MatvecToFpgaPass
    : public PassWrapper<MatvecToFpgaPass, OperationPass<ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(MatvecToFpgaPass)

  StringRef getArgument() const final { return "matvec-to-fpga"; }
  StringRef getDescription() const final {
    return "Lower linalg.matvec (static shape) to a call into "
           "fpga_matvec_tiled_auto, treating the input vector as a "
           "Kx1 matrix dispatched to the 4x4 matmul accelerator";
  }

  void getDependentDialects(DialectRegistry &registry) const override {
    registry.insert<linalg::LinalgDialect, bufferization::BufferizationDialect,
                     memref::MemRefDialect, LLVM::LLVMDialect,
                     arith::ArithDialect>();
  }

  void runOnOperation() override {
    RewritePatternSet patterns(&getContext());
    patterns.add<MatvecToFpgaPattern>(&getContext());
    if (failed(applyPatternsAndFoldGreedily(getOperation(),
                                             std::move(patterns))))
      signalPassFailure();
  }
};

} // namespace

std::unique_ptr<Pass> mlir::systolic::createMatvecToFpgaPass() {
  return std::make_unique<MatvecToFpgaPass>();
}

void mlir::systolic::registerMatvecToFpgaPass() {
  PassRegistration<MatvecToFpgaPass>();
}
