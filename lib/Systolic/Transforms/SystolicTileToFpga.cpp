#include "Systolic/Passes.h"
#include "Systolic/SystolicOps.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Bufferization/IR/Bufferization.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Transforms/GreedyPatternRewriteDriver.h"

#include "llvm/ADT/DenseMap.h"

using namespace mlir;
using namespace mlir::systolic;

namespace {

// -----------------------------------------------------------------------
// Lowers a *scheduled* systolic.matmul_tile -- one already carrying a
// device assignment from systolic-select-device -- into a single
// dispatch-runtime call, making the scheduling pipeline's output an
// executable program rather than an annotated task graph.
//
// This is deliberately separate from tile-matmul-for-fpga: that pass
// lowers a bare linalg.matmul directly, generating its own tile loops and
// bypassing the dialect's scheduling stages entirely. This one consumes
// what the scheduler produced, so the device each tile runs on is the
// device the cost model chose.
//
// Runtime contract (transport-neutral, as in tile-matmul-for-fpga):
//
//   int systolic_dispatch_open(void);
//   int systolic_dispatch_matmul4x4_dev(int handle, int device_id,
//         const float A[16], const float B[16],
//         const float C_init[16], float C_out[16]);
//
// device_id is the index of the systolic.device symbol in module
// declaration order, which is also the index the backend uses to address
// one of several physical arrays (on the UART backend, the device-select
// byte prefixed to each transaction).
//
// Tiles with m,n,k <= 4 are zero-padded into 4x4 scratch buffers. Because
// a tile's shape is a compile-time constant, the padding is fully
// unrolled -- no boundary predicates are needed, unlike the dynamic tile
// loops in tile-matmul-for-fpga. Larger tiles are left alone for a later
// pass (or a larger dispatch primitive) to handle.
// -----------------------------------------------------------------------

static Value memrefToLLVMPtr(PatternRewriter &rewriter, Location loc,
                             Value memref) {
  Value idx =
      rewriter.create<memref::ExtractAlignedPointerAsIndexOp>(loc, memref);
  Value idxAsI64 =
      rewriter.create<arith::IndexCastOp>(loc, rewriter.getI64Type(), idx);
  auto ptrTy = LLVM::LLVMPointerType::get(rewriter.getContext());
  return rewriter.create<LLVM::IntToPtrOp>(loc, ptrTy, idxAsI64);
}

struct TileToFpgaPattern : public OpRewritePattern<MatmulTileOp> {
  TileToFpgaPattern(MLIRContext *ctx, const llvm::DenseMap<StringRef, int> &ids)
      : OpRewritePattern<MatmulTileOp>(ctx), deviceIds(ids) {}

  const llvm::DenseMap<StringRef, int> &deviceIds;

  // Copy a small static m x n memref into the top-left of a 4x4 scratch
  // buffer, zero-filling the rest. All indices are compile-time constants.
  void emitPaddedFill(PatternRewriter &rewriter, Location loc, Value src,
                      int64_t rows, int64_t cols, Value dest,
                      Value zero) const {
    for (int64_t r = 0; r < 4; ++r) {
      Value cr = rewriter.create<arith::ConstantIndexOp>(loc, r);
      for (int64_t c = 0; c < 4; ++c) {
        Value cc = rewriter.create<arith::ConstantIndexOp>(loc, c);
        if (r < rows && c < cols) {
          Value v =
              rewriter.create<memref::LoadOp>(loc, src, ValueRange{cr, cc});
          rewriter.create<memref::StoreOp>(loc, v, dest, ValueRange{cr, cc});
        } else {
          rewriter.create<memref::StoreOp>(loc, zero, dest,
                                           ValueRange{cr, cc});
        }
      }
    }
  }

  LogicalResult matchAndRewrite(MatmulTileOp op,
                                PatternRewriter &rewriter) const override {
    auto devAttr = op.getDeviceAttr();
    if (!devAttr)
      return rewriter.notifyMatchFailure(
          op, "tile has no device assignment; run systolic-select-device "
              "before this pass");

    auto it = deviceIds.find(devAttr.getValue());
    if (it == deviceIds.end())
      return rewriter.notifyMatchFailure(op, "device symbol not declared");
    const int devId = it->second;

    const int64_t m = static_cast<int64_t>(op.getM());
    const int64_t n = static_cast<int64_t>(op.getN());
    const int64_t k = static_cast<int64_t>(op.getK());
    if (m > 4 || n > 4 || k > 4)
      return rewriter.notifyMatchFailure(
          op, "executable path currently requires tile dims <= 4 "
              "(run systolic-tile-matmul with tile-m/n/k = 4)");

    auto aTy = cast<RankedTensorType>(op.getA().getType());
    auto bTy = cast<RankedTensorType>(op.getB().getType());
    auto cTy = cast<RankedTensorType>(op.getCIn().getType());
    if (!aTy.getElementType().isF32())
      return rewriter.notifyMatchFailure(op, "only f32 supported");

    Location loc = op.getLoc();
    auto module = op->getParentOfType<ModuleOp>();
    auto ptrTy = LLVM::LLVMPointerType::get(rewriter.getContext());
    auto i32Ty = rewriter.getI32Type();

    auto declareFn = [&](StringRef name, LLVM::LLVMFunctionType fnTy) {
      auto fn = module.lookupSymbol<LLVM::LLVMFuncOp>(name);
      if (!fn) {
        OpBuilder::InsertionGuard guard(rewriter);
        rewriter.setInsertionPointToStart(module.getBody());
        fn = rewriter.create<LLVM::LLVMFuncOp>(module.getLoc(), name, fnTy);
      }
      return fn;
    };

    auto openFn = declareFn("systolic_dispatch_open",
                            LLVM::LLVMFunctionType::get(i32Ty, {}, false));
    auto dispatchFn = declareFn(
        "systolic_dispatch_matmul4x4_dev",
        LLVM::LLVMFunctionType::get(
            i32Ty, {i32Ty, i32Ty, ptrTy, ptrTy, ptrTy, ptrTy}, false));

    // Materialize operands as memrefs.
    auto toMemref = [&](Value t, RankedTensorType ty) {
      auto mt = MemRefType::get(ty.getShape(), ty.getElementType());
      return rewriter.create<bufferization::ToMemrefOp>(loc, mt, t).getResult();
    };
    Value aMem = toMemref(op.getA(), aTy);
    Value bMem = toMemref(op.getB(), bTy);
    Value cMem = toMemref(op.getCIn(), cTy);

    auto f32Ty = rewriter.getF32Type();
    Value zero = rewriter.create<arith::ConstantOp>(
        loc, f32Ty, rewriter.getF32FloatAttr(0.0f));
    auto tile4x4Ty = MemRefType::get({4, 4}, f32Ty);
    Value aPad = rewriter.create<memref::AllocaOp>(loc, tile4x4Ty);
    Value bPad = rewriter.create<memref::AllocaOp>(loc, tile4x4Ty);
    Value cPad = rewriter.create<memref::AllocaOp>(loc, tile4x4Ty);
    Value outPad = rewriter.create<memref::AllocaOp>(loc, tile4x4Ty);

    emitPaddedFill(rewriter, loc, aMem, m, k, aPad, zero);
    emitPaddedFill(rewriter, loc, bMem, k, n, bPad, zero);
    emitPaddedFill(rewriter, loc, cMem, m, n, cPad, zero);

    Value handle =
        rewriter.create<LLVM::CallOp>(loc, openFn, ValueRange{}).getResult();
    Value devVal = rewriter.create<LLVM::ConstantOp>(
        loc, i32Ty, rewriter.getI32IntegerAttr(devId));

    rewriter.create<LLVM::CallOp>(
        loc, dispatchFn,
        ValueRange{handle, devVal, memrefToLLVMPtr(rewriter, loc, aPad),
                   memrefToLLVMPtr(rewriter, loc, bPad),
                   memrefToLLVMPtr(rewriter, loc, cPad),
                   memrefToLLVMPtr(rewriter, loc, outPad)});

    // Crop the 4x4 result back to the tile's true m x n shape.
    auto resMemTy = MemRefType::get({m, n}, f32Ty);
    Value resMem = rewriter.create<memref::AllocaOp>(loc, resMemTy);
    for (int64_t r = 0; r < m; ++r) {
      Value cr = rewriter.create<arith::ConstantIndexOp>(loc, r);
      for (int64_t c = 0; c < n; ++c) {
        Value cc = rewriter.create<arith::ConstantIndexOp>(loc, c);
        Value v =
            rewriter.create<memref::LoadOp>(loc, outPad, ValueRange{cr, cc});
        rewriter.create<memref::StoreOp>(loc, v, resMem, ValueRange{cr, cc});
      }
    }

    auto resTensor = rewriter.create<bufferization::ToTensorOp>(
        loc, op.getResult().getType(), resMem, /*restrict=*/true,
        /*writable=*/true);
    rewriter.replaceOp(op, resTensor.getResult());
    return success();
  }
};

struct SystolicTileToFpgaPass
    : public PassWrapper<SystolicTileToFpgaPass, OperationPass<ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(SystolicTileToFpgaPass)

  StringRef getArgument() const final { return "systolic-tile-to-fpga"; }
  StringRef getDescription() const final {
    return "Lower scheduled systolic.matmul_tile ops into dispatch-runtime "
           "calls, mapping each tile's assigned systolic.device to a device "
           "id, so that the scheduling pipeline's output is executable.";
  }

  void getDependentDialects(DialectRegistry &registry) const override {
    registry.insert<bufferization::BufferizationDialect, memref::MemRefDialect,
                    LLVM::LLVMDialect, arith::ArithDialect,
                    mlir::systolic::SystolicDialect>();
  }

  void runOnOperation() override {
    ModuleOp module = getOperation();

    // Device ids follow module declaration order.
    llvm::DenseMap<StringRef, int> deviceIds;
    int next = 0;
    module.walk([&](DeviceOp dev) { deviceIds[dev.getSymName()] = next++; });

    RewritePatternSet patterns(&getContext());
    patterns.add<TileToFpgaPattern>(&getContext(), deviceIds);
    if (failed(applyPatternsAndFoldGreedily(module, std::move(patterns))))
      signalPassFailure();
  }
};

} // namespace

std::unique_ptr<Pass> mlir::systolic::createSystolicTileToFpgaPass() {
  return std::make_unique<SystolicTileToFpgaPass>();
}

void mlir::systolic::registerSystolicTileToFpgaPass() {
  PassRegistration<SystolicTileToFpgaPass>();
}
