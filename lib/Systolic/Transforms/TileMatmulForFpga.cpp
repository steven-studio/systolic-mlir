#include "Systolic/Passes.h"
#include "Systolic/SystolicOps.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Bufferization/IR/Bufferization.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/Dialect/Linalg/IR/Linalg.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/Pass/Pass.h"

#include "llvm/ADT/STLExtras.h"

#include <limits>

using namespace mlir;
using namespace mlir::systolic;

namespace {

// -----------------------------------------------------------------------
// 阶段 4:把已经完成 physical accelerator mapping 与 scheduling 的
// systolic.matmul_tile lower 成对应的 FPGA runtime 呼叫。
//
//   systolic.matmul_tile %a, %b, %c
//       m = 2 n = 3 k = 4 on @acc_4x4_1 est_cycles = E start_cycle = S
//                    |
//                    v
//   llvm.call @fpga_matmul_tiled_auto_scheduled(
//       m, k, n, A_ptr, B_ptr, C_ptr, device_id, S, E)
//
// 本 pass 不负责 tile decomposition、device selection、scheduling 或
// K-folding;它们都在 --systolic-binary-search-tile-matmul 完成。未标注
// `on @acc_*`、或没有 est_cycles/start_cycle 的 matmul_tile 不得进入
// FPGA backend。
//
// 一个 matmul_tile 就是该 device 的一次 invocation,所以这里检查的是
// 这次 invocation 放不放得进那颗硬体:
//
//   m <= rows, n <= cols    边界 tile 可以比阵列小;runtime 补零到阵列
//                           的实体几何,只写回有效的 m x n
//   1 <= k <= k_max         operand buffer 的容量;更深的 reduction 应该
//                           已经被 binary-search pass 依 k_max 折开
//
// Runtime 的语意是 C_out = C_in + A x B(linalg.matmul / matmul_tile 的
// 语意):一次 invocation 的乘积由 FPGA 算,runtime 把它加回 C 所在的
// buffer,所以依 c_in 串起来的 K invocation 会正确累加。
// -----------------------------------------------------------------------

// Physical device IDs of the 1x8x8 + 3x4x4 bitstream (tag
// tile-pipelined-1x8x8-3x4x4-20261009, systolic_dma_top's device table).
// The compiler-visible systolic.device symbol is mapped one-to-one onto
// the RTL's physical instance.
static int64_t lookupPhysicalDeviceId(StringRef name, int64_t rows,
                                      int64_t cols) {
  const bool is8x8 = rows == 8 && cols == 8;
  const bool is4x4 = rows == 4 && cols == 4;

  if (name == "acc_8x8_0" && is8x8)
    return 0;
  if (name == "acc_4x4_0" && is4x4)
    return 1;
  if (name == "acc_4x4_1" && is4x4)
    return 2;
  if (name == "acc_4x4_2" && is4x4)
    return 3;
  return -1;
}

static Value tensorToMemref(OpBuilder &builder, Location loc,
                            Value tensorVal, RankedTensorType ty,
                            bool readOnly) {
  auto memrefTy = MemRefType::get(ty.getShape(), ty.getElementType());
  return bufferization::ToBufferOp::create(builder, loc, memrefTy, tensorVal,
                                           readOnly);
}

static Value memrefToLLVMPtr(OpBuilder &builder, Location loc,
                             Value memref) {
  Value idx =
      memref::ExtractAlignedPointerAsIndexOp::create(builder, loc, memref);
  Value idxAsI64 = arith::IndexCastOp::create(builder, loc,
                                              builder.getI64Type(), idx);
  auto ptrTy = LLVM::LLVMPointerType::get(builder.getContext());
  return LLVM::IntToPtrOp::create(builder, loc, ptrTy, idxAsI64);
}

/// Everything the runtime call needs, checked before anything is rewritten.
struct FpgaInvocation {
  int64_t m, n, k;
  int64_t deviceId;
  int64_t startCycle, estCycles;
  RankedTensorType aTy, bTy, cTy;
};

static FailureOr<FpgaInvocation> checkInvocation(MatmulTileOp op,
                                                 ModuleOp module) {
  FlatSymbolRefAttr deviceRef = op.getDeviceAttr();
  if (!deviceRef)
    return op.emitError("FPGA lowering requires an assigned accelerator "
                        "(missing 'on @acc_*'); run device selection first");

  auto device = module.lookupSymbol<DeviceOp>(deviceRef.getRootReference());
  if (!device)
    return op.emitError("assigned accelerator ")
           << deviceRef << " does not resolve to a systolic.device";

  IntegerAttr estCyclesAttr = op.getEstCyclesAttr();
  IntegerAttr startCycleAttr = op.getStartCycleAttr();
  if (!estCyclesAttr || !startCycleAttr)
    return op.emitError("FPGA lowering requires a scheduled "
                        "systolic.matmul_tile (missing est_cycles/start_cycle)");

  FpgaInvocation inv;
  inv.m = op.getM();
  inv.n = op.getN();
  inv.k = op.getK();
  inv.estCycles = estCyclesAttr.getInt();
  inv.startCycle = startCycleAttr.getInt();

  if (inv.estCycles <= 0 || inv.startCycle < 0 ||
      inv.estCycles > std::numeric_limits<int32_t>::max() ||
      inv.startCycle > std::numeric_limits<int32_t>::max())
    return op.emitError("scheduled tile has est_cycles = ")
           << inv.estCycles << ", start_cycle = " << inv.startCycle
           << "; the runtime descriptor needs est_cycles in [1, 2^31) "
              "and start_cycle in [0, 2^31)";

  const int64_t rows = device.getRows();
  const int64_t cols = device.getCols();

  inv.deviceId = lookupPhysicalDeviceId(device.getSymName(), rows, cols);
  if (inv.deviceId < 0)
    return op.emitError("systolic.device @")
           << device.getSymName() << " (" << rows << "x" << cols
           << ") has no physical FPGA device ID; the bitstream provides "
              "@acc_8x8_0 (8x8) and @acc_4x4_0..2 (4x4)";

  if (inv.m < 1 || inv.n < 1 || inv.m > rows || inv.n > cols)
    return op.emitError("tile ")
           << inv.m << "x" << inv.n << " does not fit the assigned "
           << rows << "x" << cols << " accelerator @" << device.getSymName();

  IntegerAttr kMaxAttr = device.getKMaxAttr();
  if (!kMaxAttr)
    return op.emitError("FPGA lowering needs the operand-buffer capacity of "
                        "@")
           << device.getSymName() << "; declare k_max on the systolic.device";

  const int64_t kMax = kMaxAttr.getInt();
  if (inv.k < 1 || inv.k > kMax)
    return op.emitError("tile reduction depth k = ")
           << inv.k << " exceeds k_max = " << kMax << " of @"
           << device.getSymName()
           << "; fold K before lowering (--systolic-binary-search-tile-matmul "
              "folds by the device's k_max)";

  inv.aTy = dyn_cast<RankedTensorType>(op.getA().getType());
  inv.bTy = dyn_cast<RankedTensorType>(op.getB().getType());
  inv.cTy = dyn_cast<RankedTensorType>(op.getCIn().getType());

  if (!inv.aTy || !inv.bTy || !inv.cTy || !inv.aTy.hasStaticShape() ||
      !inv.bTy.hasStaticShape() || !inv.cTy.hasStaticShape())
    return op.emitError("FPGA lowering requires static ranked tensors");

  if (!inv.aTy.getElementType().isF32() ||
      !inv.bTy.getElementType().isF32() ||
      !inv.cTy.getElementType().isF32())
    return op.emitError("the FPGA datapath is fp32; got element types ")
           << inv.aTy.getElementType() << ", " << inv.bTy.getElementType()
           << ", " << inv.cTy.getElementType();

  return inv;
}

static LLVM::LLVMFuncOp getOrDeclareRuntime(ModuleOp module,
                                            OpBuilder &builder) {
  // int fpga_matmul_tiled_auto_scheduled(
  //     int M, int K, int N,
  //     const float *A, const float *B, float *C,
  //     unsigned device_id, unsigned start_cycle, unsigned est_cycles);
  constexpr StringLiteral fnName = "fpga_matmul_tiled_auto_scheduled";

  if (auto fn = module.lookupSymbol<LLVM::LLVMFuncOp>(fnName))
    return fn;

  auto ptrTy = LLVM::LLVMPointerType::get(builder.getContext());
  auto i32Ty = builder.getI32Type();
  auto fnTy = LLVM::LLVMFunctionType::get(
      i32Ty, {i32Ty, i32Ty, i32Ty, ptrTy, ptrTy, ptrTy, i32Ty, i32Ty, i32Ty},
      /*isVarArg=*/false);

  OpBuilder::InsertionGuard guard(builder);
  builder.setInsertionPointToStart(module.getBody());
  return LLVM::LLVMFuncOp::create(builder, module.getLoc(), fnName, fnTy);
}

static void lowerInvocation(MatmulTileOp op, const FpgaInvocation &inv,
                            LLVM::LLVMFuncOp runtimeFn, IRRewriter &rewriter) {
  Location loc = op.getLoc();
  rewriter.setInsertionPoint(op);

  // Tensor -> memref -> raw pointer. A and B are only read by the runtime;
  // C is read (c_in) and written (the accumulated result) in place.
  Value aMemref = tensorToMemref(rewriter, loc, op.getA(), inv.aTy,
                                 /*readOnly=*/true);
  Value bMemref = tensorToMemref(rewriter, loc, op.getB(), inv.bTy,
                                 /*readOnly=*/true);
  Value cMemref = tensorToMemref(rewriter, loc, op.getCIn(), inv.cTy,
                                 /*readOnly=*/false);

  Value aPtr = memrefToLLVMPtr(rewriter, loc, aMemref);
  Value bPtr = memrefToLLVMPtr(rewriter, loc, bMemref);
  Value cPtr = memrefToLLVMPtr(rewriter, loc, cMemref);

  auto i32 = [&](int64_t v) -> Value {
    return arith::ConstantIntOp::create(rewriter, loc, v, 32);
  };

  LLVM::CallOp::create(rewriter, loc, runtimeFn,
                       ValueRange{i32(inv.m), i32(inv.k), i32(inv.n), aPtr,
                                  bPtr, cPtr, i32(inv.deviceId),
                                  i32(inv.startCycle), i32(inv.estCycles)});

  // The runtime has accumulated the result into cMemref.
  auto toTensor = bufferization::ToTensorOp::create(
      rewriter, loc, inv.cTy, cMemref, /*restrict=*/true, /*writable=*/true);

  rewriter.replaceOp(op, toTensor.getResult());
}

struct LowerSystolicMatmulTileToFpgaPass
    : public PassWrapper<LowerSystolicMatmulTileToFpgaPass,
                         OperationPass<ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(
      LowerSystolicMatmulTileToFpgaPass)

  StringRef getArgument() const final {
    return "lower-systolic-matmul-tile-to-fpga";
  }
  StringRef getDescription() const final {
    return "Lower explicitly device-mapped, scheduled systolic.matmul_tile "
           "operations into calls of the FPGA runtime";
  }

  void getDependentDialects(DialectRegistry &registry) const override {
    registry.insert<linalg::LinalgDialect, bufferization::BufferizationDialect,
                    memref::MemRefDialect, LLVM::LLVMDialect,
                    arith::ArithDialect>();
  }

  void runOnOperation() override {
    ModuleOp module = getOperation();

    SmallVector<MatmulTileOp> tiles;
    module.walk([&](MatmulTileOp op) { tiles.push_back(op); });

    // Check every tile before rewriting any, so a module is either lowered
    // completely or left untouched with one diagnostic per offending tile.
    SmallVector<FpgaInvocation> invocations;
    bool failedAny = false;
    for (MatmulTileOp op : tiles) {
      FailureOr<FpgaInvocation> inv = checkInvocation(op, module);
      if (failed(inv)) {
        failedAny = true;
        continue;
      }
      invocations.push_back(*inv);
    }
    if (failedAny) {
      signalPassFailure();
      return;
    }

    if (tiles.empty())
      return;

    OpBuilder builder(module.getContext());
    LLVM::LLVMFuncOp runtimeFn = getOrDeclareRuntime(module, builder);

    IRRewriter rewriter(module.getContext());
    for (auto [op, inv] : llvm::zip_equal(tiles, invocations))
      lowerInvocation(op, inv, runtimeFn, rewriter);

    // Every matmul_tile is now a runtime call, so systolic.device is
    // compile-time metadata only and nothing references it any more.
    SmallVector<DeviceOp> devices;
    module.walk([&](DeviceOp device) { devices.push_back(device); });
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
