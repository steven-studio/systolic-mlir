#include "Systolic/Passes.h"
#include "Systolic/SystolicOps.h"

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
// 阶段 4:把已经完成 physical accelerator mapping 的
// systolic.matmul_tile lower 成对应的 FPGA runtime 呼叫。
//
//   systolic.matmul_tile
//       ... m = 4 n = 4 k = K on @acc_4x4_1
//                    |
//                    v
//   llvm.call @fpga_matmul_tiled_auto_scheduled(
//       4, K, 4, A_ptr, B_ptr, C_ptr)
//
// 本 pass 不负责 tile decomposition、device selection 或 scheduling。
// 未标注 `on @acc_*` 的 matmul_tile 不得进入 FPGA backend。
//
// 当前 FPGA runtime 实作的是 4x4 physical accelerator。
// K 维度可以大于 4；runtime 会沿 K 方向以 4-wide pieces
// 进行内部 decomposition / accumulation。
//
// 实际 UART、4x4 tiling、zero-padding 与累加逻辑全部在
// runtime/fpga_matmul_tiled.c 完成；本 pass 只负责将已经
// physical-mapped 的 systolic.matmul_tile 接到 runtime。
// -----------------------------------------------------------------------

static Value tensorToMemref(PatternRewriter &rewriter, Location loc,
                             Value tensorVal, RankedTensorType ty) {
  auto memrefTy = MemRefType::get(ty.getShape(), ty.getElementType());
  return bufferization::ToBufferOp::create(
      rewriter, loc, memrefTy, tensorVal);
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

struct LowerSystolicMatmulTileToFpgaPattern
    : public OpRewritePattern<MatmulTileOp> {
  using OpRewritePattern::OpRewritePattern;

  LogicalResult matchAndRewrite(
      MatmulTileOp op,
      PatternRewriter &rewriter) const override {

    // ----------------------------------------------------------
    // FPGA dispatch requires an explicit physical accelerator.
    //
    // An unassigned systolic.matmul_tile is still an intermediate
    // compiler IR and must NOT bypass device selection/scheduling.
    // ----------------------------------------------------------

    FlatSymbolRefAttr deviceRef = op.getDeviceAttr();

    if (!deviceRef) {
      return op.emitError(
          "DEBUG: systolic.matmul_tile has no assigned accelerator "
          "(missing 'on @acc_*)'");
    }

    auto module = op->getParentOfType<ModuleOp>();
    if (!module) {
      return op.emitError(
          "DEBUG: systolic.matmul_tile is not inside a module");
    }

    auto device =
        module.lookupSymbol<DeviceOp>(
            deviceRef.getRootReference());

    if (!device) {
      return op.emitError(
          "DEBUG: assigned accelerator does not resolve to a "
          "systolic.device");
    }

    // ----------------------------------------------------------
    // Scheduler boundary.
    //
    // FPGA lowering is allowed ONLY for a tile that has already
    // gone through decomposition / device assignment / scheduling.
    //
    // Do not silently lower an unscheduled tile.
    // ----------------------------------------------------------

    IntegerAttr estCyclesAttr = op.getEstCyclesAttr();
    IntegerAttr startCycleAttr = op.getStartCycleAttr();

    if (!estCyclesAttr || !startCycleAttr) {
      return op.emitError(
          "DEBUG: FPGA lowering requires a scheduled "
          "systolic.matmul_tile (missing est_cycles/start_cycle)");
    }

    const int64_t estCycles = estCyclesAttr.getInt();
    const int64_t startCycle = startCycleAttr.getInt();

    if (estCycles < 0 || startCycle < 0) {
      return op.emitError(
          "DEBUG: scheduled FPGA tile has invalid "
          "est_cycles/start_cycle");
    }

    // ----------------------------------------------------------
    // Registered physical FPGA geometries.
    //
    // A logical output tile must match the geometry of the physical
    // accelerator selected by the compiler.
    // ----------------------------------------------------------

    const int64_t rows = device.getRows();
    const int64_t cols = device.getCols();

    const bool is4x4Device = rows == 4 && cols == 4;
    const bool is8x8Device = rows == 8 && cols == 8;

    if (!is4x4Device && !is8x8Device) {
      return op.emitError(
          "DEBUG: no FPGA backend registered for accelerator geometry");
    }

    const int64_t M = op.getM();
    const int64_t K = op.getK();
    const int64_t N = op.getN();

    if (M != rows || N != cols) {
      return op.emitError(
          "DEBUG: logical tile geometry does not match assigned "
          "physical accelerator");
    }

    if (K != 16) {
      return op.emitError(
          "DEBUG: scheduled FPGA backend currently requires k=16");
    }

    Value a = op.getA();
    Value b = op.getB();
    Value c = op.getCIn();

    auto aTy = llvm::dyn_cast<RankedTensorType>(a.getType());
    auto bTy = llvm::dyn_cast<RankedTensorType>(b.getType());
    auto cTy = llvm::dyn_cast<RankedTensorType>(c.getType());

    if (!aTy || !bTy || !cTy ||
        !aTy.hasStaticShape() ||
        !bTy.hasStaticShape() ||
        !cTy.hasStaticShape()) {
      return op.emitError(
          "DEBUG: FPGA lowering currently requires static ranked tensors");
    }

    if (!aTy.getElementType().isF32() ||
        !bTy.getElementType().isF32() ||
        !cTy.getElementType().isF32()) {
      return op.emitError(
          "DEBUG: FPGA backend currently supports f32 tensors only");
    }

    Location loc = op.getLoc();

    // ----------------------------------------------------------
    // Declare:
    //
    //   int fpga_matmul_tiled_auto(
    //       int M, int K, int N,
    //       const float *A,
    //       const float *B,
    //       float *C);
    // ----------------------------------------------------------

    auto ptrTy = LLVM::LLVMPointerType::get(
        rewriter.getContext());
    auto i32Ty = rewriter.getI32Type();

    StringRef fnName = "fpga_matmul_tiled_auto_scheduled";

    auto fnTy = LLVM::LLVMFunctionType::get(
        i32Ty,
        {
            i32Ty, i32Ty, i32Ty,
            ptrTy, ptrTy, ptrTy,
            i32Ty, i32Ty, i32Ty
        },
        /*isVarArg=*/false);

    auto fpgaFunc =
        module.lookupSymbol<LLVM::LLVMFuncOp>(fnName);

    if (!fpgaFunc) {
      OpBuilder::InsertionGuard guard(rewriter);
      rewriter.setInsertionPointToStart(module.getBody());

      fpgaFunc =
          rewriter.create<LLVM::LLVMFuncOp>(
              module.getLoc(), fnName, fnTy);
    }

    // ----------------------------------------------------------
    // Tensor -> memref -> raw LLVM pointer.
    // ----------------------------------------------------------

    Value aMemref =
        tensorToMemref(rewriter, loc, a, aTy);

    Value bMemref =
        tensorToMemref(rewriter, loc, b, bTy);

    Value cMemref =
        tensorToMemref(rewriter, loc, c, cTy);

    Value aPtr =
        memrefToLLVMPtr(rewriter, loc, aMemref);

    Value bPtr =
        memrefToLLVMPtr(rewriter, loc, bMemref);

    Value cPtr =
        memrefToLLVMPtr(rewriter, loc, cMemref);

    Value mVal =
        rewriter.create<arith::ConstantIntOp>(loc, M, 32);

    Value kVal =
        rewriter.create<arith::ConstantIntOp>(loc, K, 32);

    Value nVal =
        rewriter.create<arith::ConstantIntOp>(loc, N, 32);

    // ----------------------------------------------------------
    // Preserve compiler-selected physical device identity.
    //
    // The MatmulTileOp already carries a FlatSymbolRefAttr naming
    // the exact systolic.device selected by decomposition/scheduling.
    // Map that symbol one-to-one onto the RTL physical device ID.
    //
    //   @acc_8x8_0 -> device_id 0
    //   @acc_4x4_0 -> device_id 1
    //   @acc_4x4_1 -> device_id 2
    //   @acc_4x4_2 -> device_id 3
    // ----------------------------------------------------------

    const StringRef deviceName = deviceRef.getValue();

    int64_t runtimeDeviceId = -1;

    if (deviceName == "acc_8x8_0" && is8x8Device)
      runtimeDeviceId = 0;
    else if (deviceName == "acc_4x4_0" && is4x4Device)
      runtimeDeviceId = 1;
    else if (deviceName == "acc_4x4_1" && is4x4Device)
      runtimeDeviceId = 2;
    else if (deviceName == "acc_4x4_2" && is4x4Device)
      runtimeDeviceId = 3;
    else {
      return op.emitError(
          "DEBUG: assigned systolic.device has no registered "
          "physical FPGA device ID");
    }

    auto deviceIdVal =
        rewriter.create<arith::ConstantIntOp>(
            loc, runtimeDeviceId, 32);

    auto startCycleVal =
        rewriter.create<arith::ConstantIntOp>(
            loc, startCycle, 32);

    auto estCyclesVal =
        rewriter.create<arith::ConstantIntOp>(
            loc, estCycles, 32);

    rewriter.create<LLVM::CallOp>(
        loc,
        fpgaFunc,
        ValueRange{
            mVal, kVal, nVal,
            aPtr, bPtr, cPtr,
            deviceIdVal,
            startCycleVal,
            estCyclesVal
        });

    // FPGA runtime writes the result into cMemref.
    auto toTensorOp =
        rewriter.create<bufferization::ToTensorOp>(
            loc,
            cTy,
            cMemref,
            /*restrict=*/true,
            /*writable=*/true);

    rewriter.replaceOp(op, toTensorOp.getResult());

    return success();
  }
};

struct LowerSystolicMatmulTileToFpgaPass
    : public PassWrapper<LowerSystolicMatmulTileToFpgaPass, OperationPass<ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(LowerSystolicMatmulTileToFpgaPass)

  StringRef getArgument() const final { return "lower-systolic-matmul-tile-to-fpga"; }
  StringRef getDescription() const final {
    return "Lower explicitly device-mapped systolic.matmul_tile "
           "operations into the FPGA runtime";
  }

  void getDependentDialects(DialectRegistry &registry) const override {
    registry.insert<linalg::LinalgDialect, bufferization::BufferizationDialect,
                     memref::MemRefDialect, LLVM::LLVMDialect,
                     arith::ArithDialect>();
  }

  void runOnOperation() override {
    ModuleOp module = getOperation();

    RewritePatternSet patterns(&getContext());
    patterns.add<LowerSystolicMatmulTileToFpgaPattern>(&getContext());

    if (failed(applyPatternsGreedily(module, std::move(patterns)))) {
      signalPassFailure();
      return;
    }

    // After every systolic.matmul_tile has been lowered to the FPGA
    // runtime ABI, systolic.device is compile-time metadata only.
    //
    // Keep devices if any matmul_tile remains so that partially lowered
    // IR never loses its physical-device information.
    bool hasRemainingMatmulTile = false;
    module.walk([&](MatmulTileOp) {
      hasRemainingMatmulTile = true;
    });

    if (hasRemainingMatmulTile)
      return;

    SmallVector<DeviceOp> devices;
    module.walk([&](DeviceOp device) {
      devices.push_back(device);
    });

    for (DeviceOp device : devices)
      device.erase();
  }
};

} // namespace

std::unique_ptr<Pass> mlir::systolic::createTileMatmulForFpgaPass() {
  return std::make_unique<LowerSystolicMatmulTileToFpgaPass>();
}

void mlir::systolic::registerTileMatmulForFpgaPass() {
  PassRegistration<LowerSystolicMatmulTileToFpgaPass>();
}
