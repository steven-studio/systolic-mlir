#include "Systolic/Passes.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Bufferization/IR/Bufferization.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/Dialect/Linalg/IR/Linalg.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Transforms/GreedyPatternRewriteDriver.h"

using namespace mlir;
using namespace mlir::systolic;

namespace {

// -----------------------------------------------------------------------
// This pass lowers linalg.matmul into explicit MLIR tile loops that
// dispatch each tile through a transport-neutral runtime call. The tiling,
// boundary predicates, zero-padding and inter-tile accumulation are all
// generated as inspectable IR rather than hidden inside a C library:
//
//   scf.for %mi = 0 to ceil(M/8)
//     scf.for %ni = 0 to ceil(N/8)
//       <seed 8x8 accumulator from this tile of %C>
//       scf.for %ki = 0 to ceil(K/64)
//         %Kc = min(64, K - %ki*64)
//         <fill 8 x %Kc A-tile from %A, boundary-checked, zero-padded>
//         <fill %Kc x 8 B-tile from %B, boundary-checked, zero-padded>
//         llvm.call @systolic_dispatch_matmul(%h, %Kc, %a, %b, %acc, %acc)
//       <writeback accumulator into %C, boundary-checked>
//
// The pass only ever declares and calls systolic_dispatch_open and
// systolic_dispatch_matmul -- never a UART-specific symbol -- so linking
// the same compiled object against a different implementation of those two
// swaps the backend with no change to the generated IR.
//
// REVISION: was 4x4x4 tiles against systolic_dispatch_matmul4x4.
//
// The array is 8x8 with a reduction depth supplied at run time (up to 64).
// Emitting 4x4 tiles against it left three quarters of the PEs idle and
// pinned the reduction at 4, which is precisely the capability the
// runtime-K hardware rewrite added. Measured over the wire, a 4x4 tile
// padded up to 8x8 sustains ~25 useful MAC/ms; a full 8x8x64 tile sustains
// ~188.
//
// Three consequences worth keeping in view when editing:
//
//   * K is no longer padded. M and N still round up to 8 and their boundary
//     tiles are zero-filled, because the array geometry is fixed; the K tail
//     chunk instead sends exactly the depth that remains. Padding K would
//     reduce this back to a fixed-depth accelerator.
//
//   * Scratch buffers are flat 1-D with an explicit row stride. The wire
//     format wants A packed tightly as A[i*Kc + k]; a memref<8x64> would put
//     row i at offset i*64, which coincides only when Kc == 64 -- correct for
//     K a multiple of 64 and silently wrong otherwise.
//
//   * Tile fills are scf.for loops, not compile-time unrolled. Kc is a Value,
//     so the trip count is not a constant and cannot be unrolled; and at 8x64
//     an unrolled fill would emit 512 scf.if ops per operand. The loop
//     overhead is irrelevant against ~23 ms of link time per tile.
//
// The accumulation order below is not a guess. The backend seeds its
// accumulator with C_init and sums k ascending; that was measured against
// the board after three other orderings were ruled out. It is what makes
// the K-chunk loop exact -- each call consumes the previous call's output as
// its C_init. Had the array folded C_init in at the end instead, this loop
// would add C once per chunk, silently, and only for K > 64.
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

static int64_t ceilDivConst(int64_t a, int64_t b) {
  return (a + b - 1) / b;
}

struct TileMatmulForFpgaPattern : public OpRewritePattern<linalg::MatmulOp> {
  using OpRewritePattern::OpRewritePattern;

  // Array geometry. Must track hls_rk.cfg and systolic_dispatch_new.h; a
  // disagreement here surfaces as wrong numbers, not as a build failure.
  static constexpr int64_t kTileR = 8;
  static constexpr int64_t kTileC = 8;
  static constexpr int64_t kTileKMax = 64;

  // Copy an (nRows x nCols) window of `src` -- a 2-D memref of static shape
  // (srcRows x srcCols) -- into the flat buffer `dest`, laid out row-major
  // with `rowStride` elements per row. The window's top-left corner is the
  // dynamic (rowOffset, colOffset). Positions outside the source are written
  // as zero rather than read out of bounds.
  //
  // nRows, nCols and rowStride are Values because the K extent is only known
  // at run time. rowStride is passed separately from nCols so a caller can
  // pack a Kc-wide A tile tightly (stride == Kc) while a B tile keeps its
  // full 8-wide rows (stride == 8) -- the two differ, and conflating them is
  // the packing bug this signature exists to prevent.
  void emitTileFillFlat(PatternRewriter &rewriter, Location loc, Value src,
                        int64_t srcRows, int64_t srcCols, Value rowOffset,
                        Value colOffset, Value nRows, Value nCols,
                        Value rowStride, Value dest) const {
    auto f32Ty = rewriter.getF32Type();

    Value zero = rewriter.create<arith::ConstantOp>(
        loc, f32Ty, rewriter.getF32FloatAttr(0.0f));
    Value c0 = rewriter.create<arith::ConstantIndexOp>(loc, 0);
    Value c1 = rewriter.create<arith::ConstantIndexOp>(loc, 1);
    Value cSrcRows = rewriter.create<arith::ConstantIndexOp>(loc, srcRows);
    Value cSrcCols = rewriter.create<arith::ConstantIndexOp>(loc, srcCols);

    auto rowLoop = rewriter.create<scf::ForOp>(loc, c0, nRows, c1);
    {
      OpBuilder::InsertionGuard g(rewriter);
      rewriter.setInsertionPointToStart(rowLoop.getBody());

      Value i = rowLoop.getInductionVar();
      Value ridx = rewriter.create<arith::AddIOp>(loc, rowOffset, i);
      Value rowOk = rewriter.create<arith::CmpIOp>(
          loc, arith::CmpIPredicate::slt, ridx, cSrcRows);
      Value rowBase = rewriter.create<arith::MulIOp>(loc, i, rowStride);

      auto colLoop = rewriter.create<scf::ForOp>(loc, c0, nCols, c1);
      {
        OpBuilder::InsertionGuard g2(rewriter);
        rewriter.setInsertionPointToStart(colLoop.getBody());

        Value k = colLoop.getInductionVar();
        Value cidx = rewriter.create<arith::AddIOp>(loc, colOffset, k);
        Value colOk = rewriter.create<arith::CmpIOp>(
            loc, arith::CmpIPredicate::slt, cidx, cSrcCols);
        Value inBounds = rewriter.create<arith::AndIOp>(loc, rowOk, colOk);

        // scf.if yielding a value, rather than a store in each branch: the
        // load must not be hoisted out of the guard, and expressing it as a
        // value makes that structural instead of a convention.
        auto ifOp = rewriter.create<scf::IfOp>(loc, TypeRange{f32Ty}, inBounds,
                                               /*withElseRegion=*/true);
        {
          OpBuilder::InsertionGuard g3(rewriter);
          rewriter.setInsertionPointToStart(&ifOp.getThenRegion().front());
          Value loaded = rewriter.create<memref::LoadOp>(
              loc, src, ValueRange{ridx, cidx});
          rewriter.create<scf::YieldOp>(loc, ValueRange{loaded});
        }
        {
          OpBuilder::InsertionGuard g3(rewriter);
          rewriter.setInsertionPointToStart(&ifOp.getElseRegion().front());
          rewriter.create<scf::YieldOp>(loc, ValueRange{zero});
        }

        Value flat = rewriter.create<arith::AddIOp>(loc, rowBase, k);
        rewriter.create<memref::StoreOp>(loc, ifOp.getResult(0), dest,
                                         ValueRange{flat});
      }
    }
  }

  // Write the flat 8x8 accumulator back into `dest` at the dynamic
  // (rowOffset, colOffset), skipping positions past the true M x N shape.
  void emitTileWritebackFlat(PatternRewriter &rewriter, Location loc,
                             Value src, Value dest, int64_t dstRows,
                             int64_t dstCols, Value rowOffset,
                             Value colOffset) const {
    Value c0 = rewriter.create<arith::ConstantIndexOp>(loc, 0);
    Value c1 = rewriter.create<arith::ConstantIndexOp>(loc, 1);
    Value cR = rewriter.create<arith::ConstantIndexOp>(loc, kTileR);
    Value cC = rewriter.create<arith::ConstantIndexOp>(loc, kTileC);
    Value cDstRows = rewriter.create<arith::ConstantIndexOp>(loc, dstRows);
    Value cDstCols = rewriter.create<arith::ConstantIndexOp>(loc, dstCols);

    auto rowLoop = rewriter.create<scf::ForOp>(loc, c0, cR, c1);
    {
      OpBuilder::InsertionGuard g(rewriter);
      rewriter.setInsertionPointToStart(rowLoop.getBody());

      Value i = rowLoop.getInductionVar();
      Value ridx = rewriter.create<arith::AddIOp>(loc, rowOffset, i);
      Value rowOk = rewriter.create<arith::CmpIOp>(
          loc, arith::CmpIPredicate::slt, ridx, cDstRows);
      Value rowBase = rewriter.create<arith::MulIOp>(loc, i, cC);

      auto colLoop = rewriter.create<scf::ForOp>(loc, c0, cC, c1);
      {
        OpBuilder::InsertionGuard g2(rewriter);
        rewriter.setInsertionPointToStart(colLoop.getBody());

        Value j = colLoop.getInductionVar();
        Value cidx = rewriter.create<arith::AddIOp>(loc, colOffset, j);
        Value colOk = rewriter.create<arith::CmpIOp>(
            loc, arith::CmpIPredicate::slt, cidx, cDstCols);
        Value inBounds = rewriter.create<arith::AndIOp>(loc, rowOk, colOk);

        auto ifOp = rewriter.create<scf::IfOp>(loc, inBounds,
                                               /*withElseRegion=*/false);
        OpBuilder::InsertionGuard g3(rewriter);
        rewriter.setInsertionPointToStart(&ifOp.getThenRegion().front());

        Value flat = rewriter.create<arith::AddIOp>(loc, rowBase, j);
        Value val =
            rewriter.create<memref::LoadOp>(loc, src, ValueRange{flat});
        rewriter.create<memref::StoreOp>(loc, val, dest,
                                         ValueRange{ridx, cidx});
      }
    }
  }

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

    const int64_t M = aTy.getShape()[0];
    const int64_t K = aTy.getShape()[1];
    const int64_t N = bTy.getShape()[1];

    const int64_t Mt = ceilDivConst(M, kTileR);
    const int64_t Nt = ceilDivConst(N, kTileC);
    const int64_t Kt = ceilDivConst(K, kTileKMax);

    Location loc = op.getLoc();
    auto module = op->getParentOfType<ModuleOp>();
    auto ptrTy = LLVM::LLVMPointerType::get(rewriter.getContext());
    auto i32Ty = rewriter.getI32Type();
    auto f32Ty = rewriter.getF32Type();

    auto declareFn = [&](StringRef name, LLVM::LLVMFunctionType fnTy) {
      auto fn = module.lookupSymbol<LLVM::LLVMFuncOp>(name);
      if (!fn) {
        OpBuilder::InsertionGuard guard(rewriter);
        rewriter.setInsertionPointToStart(module.getBody());
        fn = rewriter.create<LLVM::LLVMFuncOp>(module.getLoc(), name, fnTy);
      }
      return fn;
    };

    //   int systolic_dispatch_open(void);
    //     Opens and caches a connection to the accelerator, or returns a
    //     placeholder on backends where there is nothing to open.
    //
    //   int systolic_dispatch_matmul(int handle, int K, const float *A,
    //       const float *B, const float *C_init, float *C_out);
    //     One 8x8 output tile with a K-deep reduction, K in [1, 64]:
    //     C_out = A @ B + C_init. A is 8*K row-major, B is K*8 row-major,
    //     both C operands are 64 floats. Performs no tiling or padding.
    //
    // The name no longer carries a shape. systolic_dispatch_matmul4x4
    // promised a geometry the hardware had stopped having, and nothing in
    // the type system caught it -- the mismatch surfaced as wrong numbers.
    auto openFnTy = LLVM::LLVMFunctionType::get(i32Ty, {}, false);
    auto openFn = declareFn("systolic_dispatch_open", openFnTy);

    auto matmulFnTy = LLVM::LLVMFunctionType::get(
        i32Ty, {i32Ty, i32Ty, ptrTy, ptrTy, ptrTy, ptrTy}, false);
    auto matmulFn = declareFn("systolic_dispatch_matmul", matmulFnTy);

    Value aMemref = tensorToMemref(rewriter, loc, a, aTy);
    Value bMemref = tensorToMemref(rewriter, loc, b, bTy);
    Value cMemref = tensorToMemref(rewriter, loc, c, cTy);

    // One handle for the whole matmul, before the tile loops. Reopening a
    // serial port per tile would dominate everything else on the link.
    auto openCall = rewriter.create<LLVM::CallOp>(loc, openFn, ValueRange{});
    Value handle = openCall.getResult();

    // Flat scratch, reused across every tile. Sized for the largest chunk;
    // the tail chunk uses a prefix. acc doubles as C_init and C_out, which
    // is safe because the backend finishes reading its inputs before it
    // writes its result.
    auto aBufTy = MemRefType::get({kTileR * kTileKMax}, f32Ty);
    auto bBufTy = MemRefType::get({kTileKMax * kTileC}, f32Ty);
    auto cBufTy = MemRefType::get({kTileR * kTileC}, f32Ty);

    Value aTile = rewriter.create<memref::AllocaOp>(loc, aBufTy);
    Value bTile = rewriter.create<memref::AllocaOp>(loc, bBufTy);
    Value acc = rewriter.create<memref::AllocaOp>(loc, cBufTy);

    Value aTilePtr = memrefToLLVMPtr(rewriter, loc, aTile);
    Value bTilePtr = memrefToLLVMPtr(rewriter, loc, bTile);
    Value accPtr = memrefToLLVMPtr(rewriter, loc, acc);

    Value c0 = rewriter.create<arith::ConstantIndexOp>(loc, 0);
    Value c1 = rewriter.create<arith::ConstantIndexOp>(loc, 1);
    Value cR = rewriter.create<arith::ConstantIndexOp>(loc, kTileR);
    Value cC = rewriter.create<arith::ConstantIndexOp>(loc, kTileC);
    Value cKMax = rewriter.create<arith::ConstantIndexOp>(loc, kTileKMax);
    Value cK = rewriter.create<arith::ConstantIndexOp>(loc, K);
    Value cMt = rewriter.create<arith::ConstantIndexOp>(loc, Mt);
    Value cNt = rewriter.create<arith::ConstantIndexOp>(loc, Nt);
    Value cKt = rewriter.create<arith::ConstantIndexOp>(loc, Kt);

    auto miLoop = rewriter.create<scf::ForOp>(loc, c0, cMt, c1);
    {
      OpBuilder::InsertionGuard g(rewriter);
      rewriter.setInsertionPointToStart(miLoop.getBody());

      Value mi = miLoop.getInductionVar();
      Value rowOffset = rewriter.create<arith::MulIOp>(loc, mi, cR);

      auto niLoop = rewriter.create<scf::ForOp>(loc, c0, cNt, c1);
      {
        OpBuilder::InsertionGuard g2(rewriter);
        rewriter.setInsertionPointToStart(niLoop.getBody());

        Value ni = niLoop.getInductionVar();
        Value colOffset = rewriter.create<arith::MulIOp>(loc, ni, cC);

        // Seed the accumulator with this output tile of C, so the chain
        // below computes C += A @ B as linalg.matmul specifies. The
        // previous revision zeroed it and let the writeback overwrite C,
        // computing C = A @ B and discarding whatever C held -- equivalent
        // only when C is known zero. Boundary positions read as zero here,
        // and the writeback skips them, so padding never reaches C.
        emitTileFillFlat(rewriter, loc, cMemref, M, N, rowOffset, colOffset,
                         cR, cC, cC, acc);

        auto kiLoop = rewriter.create<scf::ForOp>(loc, c0, cKt, c1);
        {
          OpBuilder::InsertionGuard g3(rewriter);
          rewriter.setInsertionPointToStart(kiLoop.getBody());

          Value ki = kiLoop.getInductionVar();
          Value kOffset = rewriter.create<arith::MulIOp>(loc, ki, cKMax);

          // Kc = min(64, K - kOffset). Every chunk but the last is 64; the
          // tail is whatever remains.
          Value remaining = rewriter.create<arith::SubIOp>(loc, cK, kOffset);
          Value Kc = rewriter.create<arith::MinSIOp>(loc, cKMax, remaining);

          // A tile: 8 rows x Kc cols, packed tight -- row stride is Kc.
          emitTileFillFlat(rewriter, loc, aMemref, M, K, rowOffset, kOffset,
                           cR, Kc, Kc, aTile);

          // B tile: Kc rows x 8 cols, row stride 8. The host-side packer
          // transposes this onto the wire; the pass emits the natural
          // row-major layout and never sees that convention.
          emitTileFillFlat(rewriter, loc, bMemref, K, N, kOffset, colOffset,
                           Kc, cC, cC, bTile);

          Value KcI32 = rewriter.create<arith::IndexCastOp>(loc, i32Ty, Kc);

          rewriter.create<LLVM::CallOp>(
              loc, matmulFn,
              ValueRange{handle, KcI32, aTilePtr, bTilePtr, accPtr, accPtr});
        }

        emitTileWritebackFlat(rewriter, loc, acc, cMemref, M, N, rowOffset,
                              colOffset);
      }
    }

    // c_mem has now been written in place by the tile loops above; convert
    // back to a tensor to replace the original matmul result.
    // restrict=true: this memref was only just materialized from a distinct
    // tensor and has no other aliases, which One-Shot Bufferize's alias
    // analysis requires to be told explicitly.
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
    return "Lower arbitrary-shape linalg.matmul into explicit MLIR tile "
           "loops (scf.for) with boundary-checked (scf.if) 8x8 tiling and "
           "a runtime reduction depth of up to 64, zero-padding M and N "
           "but not K, dispatching each tile through a transport-neutral "
           "dispatch-runtime call (systolic_dispatch_matmul), independent "
           "of the backend linked to provide it";
  }

  void getDependentDialects(DialectRegistry &registry) const override {
    registry.insert<linalg::LinalgDialect, bufferization::BufferizationDialect,
                     memref::MemRefDialect, LLVM::LLVMDialect,
                     arith::ArithDialect, scf::SCFDialect>();
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