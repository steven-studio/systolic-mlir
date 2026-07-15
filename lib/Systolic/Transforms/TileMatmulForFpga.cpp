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
// 阶段 4:把「任意静态形状」的 linalg.matmul 转成一次呼叫已烧录好的
// 4x4 FPGA runtime (fpga_matmul_tiled_auto),而不是产生新硬件。
//
//   linalg.matmul ins(%A, %B) outs(%C) : tensor<MxKxf32>, tensor<KxNxf32>
//                                        -> tensor<MxNxf32>
// 转成:
//   %a_mem = bufferization.to_memref %A
//   %b_mem = bufferization.to_memref %B
//   %c_mem = bufferization.to_memref %C
//   %a_ptr = ... extract pointer, inttoptr ...
//   %b_ptr = ...
//   %c_ptr = ...
//   llvm.call @fpga_matmul_tiled_auto(%M, %K, %N, %a_ptr, %b_ptr, %c_ptr)
//   %result = bufferization.to_tensor %c_mem
//
// 实际的 4x4 tiling / zero-padding / UART 累加逻辑全部在 C runtime
// (runtime/fpga_matmul_tiled.c) 里完成,这个 pass 只负责产生呼叫。
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

struct TileMatmulForFpgaPattern : public OpRewritePattern<linalg::MatmulOp> {
  using OpRewritePattern::OpRewritePattern;

  LogicalResult matchAndRewrite(linalg::MatmulOp op,
                                 PatternRewriter &rewriter) const override {
    Value a = op.getInputs()[0];  // [M, K]
    Value b = op.getInputs()[1];  // [K, N]
    Value c = op.getOutputs()[0]; // [M, N]

    auto aTy = llvm::dyn_cast<RankedTensorType>(a.getType());
    auto bTy = llvm::dyn_cast<RankedTensorType>(b.getType());
    auto cTy = llvm::dyn_cast<RankedTensorType>(c.getType());
    if (!aTy || !bTy || !cTy || !aTy.hasStaticShape() ||
        !bTy.hasStaticShape() || !cTy.hasStaticShape())
      return rewriter.notifyMatchFailure(op, "只处理静态形状的 matmul");

    if (!aTy.getElementType().isF32())
      return rewriter.notifyMatchFailure(op, "目前只支援 f32");

    int64_t M = aTy.getShape()[0];
    int64_t K = aTy.getShape()[1];
    int64_t N = bTy.getShape()[1];

    Location loc = op.getLoc();
    auto module = op->getParentOfType<ModuleOp>();

    // 宣告外部 runtime 函式(如果还没宣告过)
    //   int fpga_matmul_tiled_auto(int M, int K, int N,
    //                               const float *A, const float *B, float *C);
    auto ptrTy = LLVM::LLVMPointerType::get(rewriter.getContext());
    auto i32Ty = rewriter.getI32Type();
    StringRef fnName = "fpga_matmul_tiled_auto";
    auto fnTy = LLVM::LLVMFunctionType::get(
        i32Ty, {i32Ty, i32Ty, i32Ty, ptrTy, ptrTy, ptrTy}, /*isVarArg=*/false);

    auto fpgaFunc = module.lookupSymbol<LLVM::LLVMFuncOp>(fnName);
    if (!fpgaFunc) {
      OpBuilder::InsertionGuard guard(rewriter);
      rewriter.setInsertionPointToStart(module.getBody());
      fpgaFunc =
          rewriter.create<LLVM::LLVMFuncOp>(module.getLoc(), fnName, fnTy);
    }

    Value aMemref = tensorToMemref(rewriter, loc, a, aTy);
    Value bMemref = tensorToMemref(rewriter, loc, b, bTy);
    Value cMemref = tensorToMemref(rewriter, loc, c, cTy);

    Value aPtr = memrefToLLVMPtr(rewriter, loc, aMemref);
    Value bPtr = memrefToLLVMPtr(rewriter, loc, bMemref);
    Value cPtr = memrefToLLVMPtr(rewriter, loc, cMemref);

    Value mVal = rewriter.create<arith::ConstantIntOp>(loc, M, 32);
    Value kVal = rewriter.create<arith::ConstantIntOp>(loc, K, 32);
    Value nVal = rewriter.create<arith::ConstantIntOp>(loc, N, 32);

    rewriter.create<LLVM::CallOp>(
        loc, fpgaFunc, ValueRange{mVal, kVal, nVal, aPtr, bPtr, cPtr});

    // c_mem 已经被外部呼叫原地写入结果,转回 tensor 顶替原本的 matmul 结果
    // restrict=true: 这个 memref 是我们刚从 to_memref 拿到的新值,
    // 保证没有其他别名指向同一块内存,One-Shot Bufferize 分析需要这个保证
    auto toTensorOp = rewriter.create<bufferization::ToTensorOp>(
        loc, cTy, cMemref, /*restrict=*/true, /*writable=*/true);
    rewriter.replaceOp(op, toTensorOp.getResult());
    return success();
  }
};

struct TileMatmulForFpgaPass
    : public PassWrapper<TileMatmulForFpgaPass, OperationPass<ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(TileMatmulForFpgaPass)

  StringRef getArgument() const final { return "tile-matmul-for-fpga"; }
  StringRef getDescription() const final {
    return "Lower arbitrary-shape linalg.matmul into calls to the FPGA "
           "runtime (fpga_matmul_tiled_auto), tiled into 4x4 blocks";
  }

  void getDependentDialects(DialectRegistry &registry) const override {
    registry.insert<linalg::LinalgDialect, bufferization::BufferizationDialect,
                     memref::MemRefDialect, LLVM::LLVMDialect,
                     arith::ArithDialect>();
  }

  void runOnOperation() override {
    RewritePatternSet patterns(&getContext());
    patterns.add<TileMatmulForFpgaPattern>(&getContext());
    if (failed(applyPatternsAndFoldGreedily(getOperation(),
                                             std::move(patterns))))
      signalPassFailure();
  }
};

} // namespace

std::unique_ptr<Pass> mlir::systolic::createTileMatmulForFpgaPass() {
  return std::make_unique<TileMatmulForFpgaPass>();
}

void mlir::systolic::registerTileMatmulForFpgaPass() {
  PassRegistration<TileMatmulForFpgaPass>();
}
