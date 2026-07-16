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
// Lowers linalg.batch_matmul (static shape, f32) to a call into
// fpga_batch_matmul_tiled_auto, which sequentially dispatches each
// batch element to fpga_matmul_tiled_auto (Section: tile-matmul-for-fpga).
// Structurally identical to TileMatmulForFpgaPattern, with an extra
// leading batch dimension threaded through as a runtime argument --
// no new hardware, UART protocol, or tiling logic required, since
// per-batch dispatch has no data dependency across batches.
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

struct BatchMatmulToFpgaPattern
    : public OpRewritePattern<linalg::BatchMatmulOp> {
  using OpRewritePattern::OpRewritePattern;

  LogicalResult matchAndRewrite(linalg::BatchMatmulOp op,
                                 PatternRewriter &rewriter) const override {
    Value a = op.getInputs()[0];  // [batch, M, K]
    Value b = op.getInputs()[1];  // [batch, K, N]
    Value c = op.getOutputs()[0]; // [batch, M, N]

    auto aTy = llvm::dyn_cast<RankedTensorType>(a.getType());
    auto bTy = llvm::dyn_cast<RankedTensorType>(b.getType());
    auto cTy = llvm::dyn_cast<RankedTensorType>(c.getType());
    if (!aTy || !bTy || !cTy || !aTy.hasStaticShape() ||
        !bTy.hasStaticShape() || !cTy.hasStaticShape())
      return rewriter.notifyMatchFailure(op, "只处理静态形状的 batch_matmul");

    if (!aTy.getElementType().isF32())
      return rewriter.notifyMatchFailure(op, "目前只支援 f32");

    int64_t Batch = aTy.getShape()[0];
    int64_t M = aTy.getShape()[1];
    int64_t K = aTy.getShape()[2];
    int64_t N = bTy.getShape()[2];

    Location loc = op.getLoc();
    auto module = op->getParentOfType<ModuleOp>();

    auto ptrTy = LLVM::LLVMPointerType::get(rewriter.getContext());
    auto i32Ty = rewriter.getI32Type();
    StringRef fnName = "fpga_batch_matmul_tiled_auto";
    auto fnTy = LLVM::LLVMFunctionType::get(
        i32Ty, {i32Ty, i32Ty, i32Ty, i32Ty, ptrTy, ptrTy, ptrTy},
        /*isVarArg=*/false);

    auto batchFunc = module.lookupSymbol<LLVM::LLVMFuncOp>(fnName);
    if (!batchFunc) {
      OpBuilder::InsertionGuard guard(rewriter);
      rewriter.setInsertionPointToStart(module.getBody());
      batchFunc =
          rewriter.create<LLVM::LLVMFuncOp>(module.getLoc(), fnName, fnTy);
    }

    Value aMemref = tensorToMemref(rewriter, loc, a, aTy);
    Value bMemref = tensorToMemref(rewriter, loc, b, bTy);
    Value cMemref = tensorToMemref(rewriter, loc, c, cTy);

    Value aPtr = memrefToLLVMPtr(rewriter, loc, aMemref);
    Value bPtr = memrefToLLVMPtr(rewriter, loc, bMemref);
    Value cPtr = memrefToLLVMPtr(rewriter, loc, cMemref);

    auto ci32 = [&](int64_t v) {
      return rewriter.create<arith::ConstantIntOp>(loc, v, 32);
    };

    // fpga_batch_matmul_tiled_auto(batch, M, K, N, A*, B*, C*)
    rewriter.create<LLVM::CallOp>(
        loc, batchFunc,
        ValueRange{ci32(Batch), ci32(M), ci32(K), ci32(N), aPtr, bPtr, cPtr});

    Value resultTensor = rewriter.create<bufferization::ToTensorOp>(
        loc, cTy, cMemref, /*restrict=*/true, /*writable=*/true);
    rewriter.replaceOp(op, resultTensor);
    return success();
  }
};

struct BatchMatmulToFpgaPass
    : public PassWrapper<BatchMatmulToFpgaPass, OperationPass<ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(BatchMatmulToFpgaPass)

  StringRef getArgument() const final { return "batch-matmul-to-fpga"; }
  StringRef getDescription() const final {
    return "Lower linalg.batch_matmul (static shape) to a call into "
           "fpga_batch_matmul_tiled_auto, dispatching each batch "
           "element to the 4x4 matmul accelerator sequentially";
  }

  void getDependentDialects(DialectRegistry &registry) const override {
    registry.insert<linalg::LinalgDialect, bufferization::BufferizationDialect,
                     memref::MemRefDialect, LLVM::LLVMDialect,
                     arith::ArithDialect>();
  }

  void runOnOperation() override {
    RewritePatternSet patterns(&getContext());
    patterns.add<BatchMatmulToFpgaPattern>(&getContext());
    if (failed(applyPatternsAndFoldGreedily(getOperation(),
                                             std::move(patterns))))
      signalPassFailure();
  }
};

} // namespace

std::unique_ptr<Pass> mlir::systolic::createBatchMatmulToFpgaPass() {
  return std::make_unique<BatchMatmulToFpgaPass>();
}

void mlir::systolic::registerBatchMatmulToFpgaPass() {
  PassRegistration<BatchMatmulToFpgaPass>();
}
