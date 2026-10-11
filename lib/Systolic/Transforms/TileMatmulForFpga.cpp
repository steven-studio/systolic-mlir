#include "Systolic/Passes.h"
#include "Systolic/SystolicOps.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Bufferization/IR/Bufferization.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/Dialect/Linalg/IR/Linalg.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/Tensor/IR/Tensor.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/Interfaces/FunctionInterfaces.h"
#include "mlir/Pass/Pass.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"

#include <algorithm>
#include <limits>
#include <optional>

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
//   k <= 16                 bitstream 一次 invocation 的 slab 深度(K_MAX),
//                           不论 IR 宣告的 k_max 是多少
//
// Runtime 的语意是 C_out = C_in + A x B(linalg.matmul / matmul_tile 的
// 语意):一次 invocation 的乘积由 FPGA 算,runtime 把它加回 C 所在的
// buffer,所以依 c_in 串起来的 K invocation 会正确累加。
// -----------------------------------------------------------------------

// The arrays of the 1x8x8 + 3x4x4 bitstream (tag
// tile-pipelined-1x8x8-3x4x4-20261009): systolic_dma_top's device table,
// and K_MAX = 16 from dma_top_build.tcl -- the reduction depth one
// invocation's operand slab holds, whatever k_max the IR declares. The
// compiler-visible systolic.device symbol is mapped one-to-one onto the
// RTL's physical instance.
struct PhysicalDevice {
  StringLiteral symbol;
  int64_t id;
  int64_t rows, cols;
  int64_t kDepth;
};

static constexpr PhysicalDevice kBitstreamDevices[] = {
    {"acc_8x8_0", 0, 8, 8, 16},
    {"acc_4x4_0", 1, 4, 4, 16},
    {"acc_4x4_1", 2, 4, 4, 16},
    {"acc_4x4_2", 3, 4, 4, 16},
};

// The most elements the m x k, k x n and m x n tiles of one invocation can
// have on the bitstream's arrays.
struct TileCapacity {
  int64_t a = 0, b = 0, c = 0;
};

static constexpr TileCapacity maxTileElements() {
  TileCapacity cap;
  for (const PhysicalDevice &dev : kBitstreamDevices) {
    cap.a = std::max(cap.a, dev.rows * dev.kDepth);
    cap.b = std::max(cap.b, dev.kDepth * dev.cols);
    cap.c = std::max(cap.c, dev.rows * dev.cols);
  }
  return cap;
}
static constexpr TileCapacity kMaxTile = maxTileElements();

static const PhysicalDevice *lookupPhysicalDevice(StringRef name,
                                                  int64_t rows,
                                                  int64_t cols) {
  for (const PhysicalDevice &dev : kBitstreamDevices)
    if (name == dev.symbol && rows == dev.rows && cols == dev.cols)
      return &dev;
  return nullptr;
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

  const PhysicalDevice *phys =
      lookupPhysicalDevice(device.getSymName(), rows, cols);
  if (!phys)
    return op.emitError("systolic.device @")
           << device.getSymName() << " (" << rows << "x" << cols
           << ") has no physical FPGA device ID; the bitstream provides "
              "@acc_8x8_0 (8x8) and @acc_4x4_0..2 (4x4)";
  inv.deviceId = phys->id;

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

  // k_max is what the IR declares; the slab depth is what the bitstream
  // has. A fleet declaring more than the hardware holds would fold K too
  // coarsely and every such invocation would fail in the runtime.
  if (inv.k > phys->kDepth)
    return op.emitError("tile reduction depth k = ")
           << inv.k << " exceeds the " << phys->kDepth
           << " reduction steps one invocation of @" << device.getSymName()
           << " holds in the bitstream; declare k_max <= " << phys->kDepth
           << " on the systolic.device";

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

static bool isStaticUnitStrideSlice(OffsetSizeAndStrideOpInterface op) {
  return op.getOffsets().empty() && op.getSizes().empty() &&
         op.getStrides().empty() &&
         llvm::all_of(op.getStaticStrides(),
                      [](int64_t s) { return s == 1; });
}

/// A static, unit-stride, non-rank-reducing slice of a static f32 tensor
/// whose only use is the operand being lowered.
static tensor::ExtractSliceOp stageableSlice(Value operand) {
  auto slice = operand.getDefiningOp<tensor::ExtractSliceOp>();
  if (!slice || !slice->hasOneUse() || !isStaticUnitStrideSlice(slice) ||
      slice.getResultType().getShape() != slice.getStaticSizes())
    return nullptr;
  auto sourceTy = dyn_cast<RankedTensorType>(slice.getSource().getType());
  if (!sourceTy || !sourceTy.hasStaticShape() ||
      !sourceTy.getElementType().isF32())
    return nullptr;
  return slice;
}

// -----------------------------------------------------------------------
// Operand staging.
//
// The runtime takes contiguous row-major m x k, k x n and m x n tiles. The
// operands --systolic-binary-search-tile-matmul produces are blocks of the
// whole A, B and C, and every output block of C is
//
//   %c  = tensor.extract_slice %R[o0, o1] [m, n] [1, 1]
//   %t1 = systolic.matmul_tile %a1, %b1, %c ...
//   ...                                          (one per K fold)
//   %tI = systolic.matmul_tile %aI, %bI, %t(I-1) ...
//   %R2 = tensor.insert_slice %tI into %R[o0, o1] [m, n] [1, 1]
//
// with %R threaded from block to block. Lowered as tensors (one buffer per
// slice), that is one ever longer chain of aliasing tensors and thousands
// of staging buffers: one-shot bufferization's read-after-write analysis
// over the chain grew with the cube of the number of blocks (96x96x16,
// 336 blocks: 34 s; 128x128x16, 597 blocks: 208 s), and buffer-deallocation
// simplification with the square of the staging buffers.
//
// So the tiles are staged at the buffer level instead: a block of A or B is
// copied straight out of its source's buffer into a contiguous scratch
// tile; an output block of C is copied into a scratch tile once,
// accumulated there by all its invocations, and copied back, and %R2
// becomes a to_tensor of %R's buffer. That last step asserts that nothing
// else can reach %R's buffer, so a block is staged this way only when
// nothing but its chain uses %R, %c or any %t_j, %R is a function argument
// or the previous staged block's result, and the block does not interleave
// with another (see runOnOperation). The scratch tiles are three stack
// buffers per function sized for the largest array; the runtime is
// synchronous, so every invocation reuses them. Anything else goes through
// bufferization as a plain tensor.
// -----------------------------------------------------------------------

struct OutputBlock {
  tensor::ExtractSliceOp extract;
  SmallVector<unsigned> tiles; // indices into the pass's tile list, in order
  tensor::InsertSliceOp insert;

  // Where %R comes from: an argument of the function, or the insert_slice
  // of an earlier block of the same chain.
  bool rootIsArgument = false;
  Operation *previousInsert = nullptr;
};

static std::optional<OutputBlock>
matchOutputBlock(unsigned head, ArrayRef<MatmulTileOp> tiles,
                 const DenseMap<Operation *, unsigned> &tileIndex) {
  MatmulTileOp first = tiles[head];
  tensor::ExtractSliceOp extract = stageableSlice(first.getCIn());
  if (!extract)
    return std::nullopt;

  OutputBlock block;
  block.extract = extract;
  block.tiles.push_back(head);

  Block *parent = extract->getBlock();
  MatmulTileOp cur = first;
  for (;;) {
    if (cur->getBlock() != parent || !cur->hasOneUse())
      return std::nullopt;
    Operation *user = *cur->getUsers().begin();
    if (auto next = dyn_cast<MatmulTileOp>(user)) {
      if (next.getCIn() != cur.getResult())
        return std::nullopt;
      block.tiles.push_back(tileIndex.lookup(next));
      cur = next;
      continue;
    }
    auto insert = dyn_cast<tensor::InsertSliceOp>(user);
    if (!insert || insert.getSource() != cur.getResult() ||
        insert->getBlock() != parent)
      return std::nullopt;
    block.insert = insert;
    break;
  }

  Value r = extract.getSource();
  if (block.insert.getDest() != r || !llvm::hasNItems(r.getUses(), 2) ||
      !isStaticUnitStrideSlice(block.insert) ||
      block.insert.getStaticOffsets() != extract.getStaticOffsets() ||
      block.insert.getStaticSizes() != extract.getStaticSizes())
    return std::nullopt;

  if (auto arg = dyn_cast<BlockArgument>(r))
    block.rootIsArgument =
        arg.getOwner()->isEntryBlock() &&
        isa<FunctionOpInterface>(arg.getOwner()->getParentOp());
  else
    block.previousInsert = r.getDefiningOp<tensor::InsertSliceOp>();
  if (!block.rootIsArgument && !block.previousInsert)
    return std::nullopt;
  return block;
}

/// Blocks whose extract .. insert ranges interleave with another block's
/// ops (nested regions included). They would share the one C scratch tile.
static SmallVector<bool> findInterleavedBlocks(ArrayRef<OutputBlock> blocks,
                                               ArrayRef<MatmulTileOp> tiles) {
  DenseMap<Operation *, unsigned> owner;
  for (auto [i, block] : llvm::enumerate(blocks)) {
    owner[block.extract] = i;
    owner[block.insert] = i;
    for (unsigned idx : block.tiles)
      owner[tiles[idx]] = i;
  }

  SmallVector<bool> interleaved(blocks.size(), false);
  for (auto [i, block] : llvm::enumerate(blocks)) {
    Operation *last = block.insert;
    for (Operation *op = block.extract;; op = op->getNextNode()) {
      op->walk([&](Operation *nested) {
        auto it = owner.find(nested);
        if (it != owner.end() && it->second != i) {
          interleaved[i] = true;
          interleaved[it->second] = true;
        }
      });
      if (op == last)
        break;
    }
  }
  return interleaved;
}

class RuntimeCallLowering {
public:
  RuntimeCallLowering(IRRewriter &rewriter, LLVM::LLVMFuncOp runtimeFn,
                      ArrayRef<MatmulTileOp> tiles)
      : rewriter(rewriter), runtimeFn(runtimeFn) {
    // A function argument that is only ever sliced into A and B tiles is
    // read through one buffer for the whole function: every one of those
    // slices is staged from it, and nothing writes it. (One to_buffer per
    // tile would give the argument a use per invocation, and one-shot
    // bufferization's analysis visits every use for every use.)
    DenseMap<Value, unsigned> stagedUses;
    for (MatmulTileOp op : tiles)
      for (Value operand : {op.getA(), op.getB()})
        if (tensor::ExtractSliceOp slice = stageableSlice(operand))
          ++stagedUses[slice.getSource()];
    for (auto [source, uses] : stagedUses) {
      auto arg = dyn_cast<BlockArgument>(source);
      if (arg && arg.getOwner()->isEntryBlock() &&
          isa<FunctionOpInterface>(arg.getOwner()->getParentOp()) &&
          llvm::hasNItems(source.getUses(), uses))
        sharedSources.insert(source);
    }
  }

  /// One output block: C staged once, accumulated by every invocation.
  void lowerOutputBlock(const OutputBlock &block,
                        ArrayRef<MatmulTileOp> tiles,
                        ArrayRef<FpgaInvocation> invocations) {
    tensor::ExtractSliceOp extract = block.extract;
    Location loc = extract.getLoc();
    const FpgaInvocation &firstInv = invocations[block.tiles.front()];

    // %R's buffer, the m x n block of it, and the tile it is staged in.
    rewriter.setInsertionPoint(extract);
    Value r = extract.getSource();
    auto rTy = cast<RankedTensorType>(r.getType());
    Value rMemref = tensorToMemref(rewriter, loc, r, rTy, /*readOnly=*/false);
    Value cBlock = memref::SubViewOp::create(
        rewriter, loc, rMemref, extract.getStaticOffsets(),
        extract.getStaticSizes(), extract.getStaticStrides());
    Value cTile = scratchTile(loc, scratchFor(extract).c,
                              {firstInv.m, firstInv.n});
    memref::CopyOp::create(rewriter, loc, cBlock, cTile);
    Value cPtr = memrefToLLVMPtr(rewriter, loc, cTile);

    for (unsigned idx : block.tiles) {
      MatmulTileOp op = tiles[idx];
      rewriter.setInsertionPoint(op);
      emitCall(op, invocations[idx], cPtr);
    }

    tensor::InsertSliceOp insert = block.insert;
    rewriter.setInsertionPoint(insert);
    memref::CopyOp::create(rewriter, loc, cTile, cBlock);
    Value result = bufferization::ToTensorOp::create(
        rewriter, loc, rTy, rMemref, /*restrict=*/true, /*writable=*/true);
    rewriter.replaceOp(insert, result);

    for (unsigned idx : llvm::reverse(block.tiles)) {
      MatmulTileOp op = tiles[idx];
      rewriter.eraseOp(op);
    }
    rewriter.eraseOp(extract);
  }

  /// A matmul_tile outside an output block: C is read (c_in) and written
  /// (the accumulated result) in place in a buffer of the c_in tensor.
  void lowerInvocation(MatmulTileOp op, const FpgaInvocation &inv) {
    Location loc = op.getLoc();
    rewriter.setInsertionPoint(op);

    Value cMemref = tensorToMemref(rewriter, loc, op.getCIn(), inv.cTy,
                                   /*readOnly=*/false);
    emitCall(op, inv, memrefToLLVMPtr(rewriter, loc, cMemref));

    // The runtime has accumulated the result into cMemref.
    auto toTensor = bufferization::ToTensorOp::create(
        rewriter, loc, inv.cTy, cMemref, /*restrict=*/true, /*writable=*/true);
    rewriter.replaceOp(op, toTensor.getResult());
  }

  /// The A and B slices staged from their sources' buffers have no user
  /// left once every tile is lowered.
  void eraseStagedSlices() {
    for (tensor::ExtractSliceOp slice : stagedSlices)
      if (slice->use_empty())
        rewriter.eraseOp(slice);
  }

private:
  struct Scratch {
    Value a, b, c;
  };

  Scratch &scratchFor(Operation *op) {
    auto fn = op->getParentOfType<FunctionOpInterface>();
    Scratch &scratch = scratchOf[fn];
    if (!scratch.c) {
      OpBuilder::InsertionGuard guard(rewriter);
      rewriter.setInsertionPointToStart(&fn.getFunctionBody().front());
      auto stackBuffer = [&](int64_t elements) -> Value {
        return memref::AllocaOp::create(
            rewriter, fn.getLoc(),
            MemRefType::get({elements}, rewriter.getF32Type()));
      };
      scratch.a = stackBuffer(kMaxTile.a);
      scratch.b = stackBuffer(kMaxTile.b);
      scratch.c = stackBuffer(kMaxTile.c);
    }
    return scratch;
  }

  /// A contiguous row-major rows x cols view of a scratch buffer.
  Value scratchTile(Location loc, Value scratch, ArrayRef<int64_t> shape) {
    return memref::ReinterpretCastOp::create(
        rewriter, loc, MemRefType::get(shape, rewriter.getF32Type()), scratch,
        /*offset=*/int64_t{0}, /*sizes=*/shape,
        /*strides=*/ArrayRef<int64_t>{shape[1], 1});
  }

  /// The buffer a staged slice is read from: the function-wide one of a
  /// shared source, else a to_buffer right here.
  Value sourceBuffer(Location loc, Value source) {
    auto sourceTy = cast<RankedTensorType>(source.getType());
    if (!sharedSources.contains(source))
      return tensorToMemref(rewriter, loc, source, sourceTy,
                            /*readOnly=*/true);
    Value &buffer = sharedBuffers[source];
    if (!buffer) {
      OpBuilder::InsertionGuard guard(rewriter);
      rewriter.setInsertionPointToStart(
          cast<BlockArgument>(source).getOwner());
      buffer = tensorToMemref(rewriter, source.getLoc(), source, sourceTy,
                              /*readOnly=*/true);
    }
    return buffer;
  }

  /// A raw pointer to a contiguous row-major copy of the A or B operand.
  Value stageOperand(Location loc, Value operand, RankedTensorType ty,
                     Value scratch) {
    if (tensor::ExtractSliceOp slice = stageableSlice(operand)) {
      Value source = sourceBuffer(loc, slice.getSource());
      Value view = memref::SubViewOp::create(
          rewriter, loc, source, slice.getStaticOffsets(),
          slice.getStaticSizes(), slice.getStaticStrides());
      Value tile = scratchTile(loc, scratch, ty.getShape());
      memref::CopyOp::create(rewriter, loc, view, tile);
      stagedSlices.push_back(slice);
      return memrefToLLVMPtr(rewriter, loc, tile);
    }
    Value buffer = tensorToMemref(rewriter, loc, operand, ty,
                                  /*readOnly=*/true);
    return memrefToLLVMPtr(rewriter, loc, buffer);
  }

  /// The runtime call of one invocation, its C tile at cPtr.
  void emitCall(MatmulTileOp op, const FpgaInvocation &inv, Value cPtr) {
    Location loc = op.getLoc();
    Scratch &scratch = scratchFor(op);
    Value aPtr = stageOperand(loc, op.getA(), inv.aTy, scratch.a);
    Value bPtr = stageOperand(loc, op.getB(), inv.bTy, scratch.b);

    auto i32 = [&](int64_t v) -> Value {
      return arith::ConstantIntOp::create(rewriter, loc, v, 32);
    };

    LLVM::CallOp::create(rewriter, loc, runtimeFn,
                         ValueRange{i32(inv.m), i32(inv.k), i32(inv.n), aPtr,
                                    bPtr, cPtr, i32(inv.deviceId),
                                    i32(inv.startCycle), i32(inv.estCycles)});
  }

  IRRewriter &rewriter;
  LLVM::LLVMFuncOp runtimeFn;
  DenseMap<Operation *, Scratch> scratchOf;
  SmallVector<tensor::ExtractSliceOp> stagedSlices;
  llvm::DenseSet<Value> sharedSources;
  DenseMap<Value, Value> sharedBuffers;
};

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

    // Output blocks first (see "Operand staging"), then any tile outside
    // one.
    DenseMap<Operation *, unsigned> tileIndex;
    for (auto [i, op] : llvm::enumerate(tiles))
      tileIndex[op] = i;

    SmallVector<OutputBlock> candidates;
    SmallVector<bool> claimed(tiles.size(), false);
    for (unsigned i = 0; i < tiles.size(); ++i) {
      if (claimed[i])
        continue;
      std::optional<OutputBlock> block =
          matchOutputBlock(i, tiles, tileIndex);
      if (!block)
        continue;
      for (unsigned idx : block->tiles)
        claimed[idx] = true;
      candidates.push_back(std::move(*block));
    }

    // A block is staged only if
    //  - %R is a buffer nothing else reaches: a function argument that only
    //    this chain uses, or the result of the previous *staged* block of
    //    the chain. Any other tensor (tensor.empty, linalg.fill, a loop-
    //    carried value, ...) can share its buffer in place with tensors
    //    that the restrict to_tensor would hide from bufferization;
    //  - it does not interleave with another block, since all blocks of a
    //    function accumulate in the same C scratch tile.
    // Everything else goes through bufferization as tensors.
    SmallVector<bool> interleaved = findInterleavedBlocks(candidates, tiles);
    SmallVector<OutputBlock> blocks;
    SmallVector<bool> inBlock(tiles.size(), false);
    llvm::DenseSet<Operation *> stagedInserts;
    for (auto [block, overlaps] : llvm::zip_equal(candidates, interleaved)) {
      const bool safeRoot =
          block.rootIsArgument ||
          (block.previousInsert && stagedInserts.contains(block.previousInsert));
      if (overlaps || !safeRoot)
        continue;
      stagedInserts.insert(block.insert);
      for (unsigned idx : block.tiles)
        inBlock[idx] = true;
      blocks.push_back(block);
    }

    IRRewriter rewriter(module.getContext());
    RuntimeCallLowering lowering(rewriter, runtimeFn, tiles);

    for (const OutputBlock &block : blocks)
      lowering.lowerOutputBlock(block, tiles, invocations);

    for (unsigned i = 0; i < tiles.size(); ++i)
      if (!inBlock[i])
        lowering.lowerInvocation(tiles[i], invocations[i]);

    lowering.eraseStagedSlices();

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
