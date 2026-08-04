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
// Revision: previously this pass lowered linalg.matmul to a SINGLE call
// to fpga_matmul_tiled_auto(M, K, N, A, B, C), with all 4x4 tiling,
// zero-padding, and reduction-tile accumulation implemented opaquely in
// the C runtime (runtime/fpga_matmul_tiled.c). That left no tile loops,
// boundary predicates, or accumulation structure visible in MLIR IR --
// the compiler-side contribution was, in substance, a library call.
//
// This version instead generates the tiling explicitly as MLIR IR:
//
//   scf.for %mi = 0 to M_tiles           (M_tiles = ceil(M/4), a compile-
//     scf.for %ni = 0 to N_tiles          time constant since the matmul
//       <zero-init 4x4 accumulator>       is static-shape)
//       scf.for %ki = 0 to K_tiles
//         <fill 4x4 A-tile from %A, scf.if boundary-checked, zero-padded>
//         <fill 4x4 B-tile from %B, scf.if boundary-checked, zero-padded>
//         llvm.call @systolic_dispatch_matmul4x4(%handle, %a_tile, %b_tile, %acc, %acc)
//       <writeback 4x4 accumulator into %C, scf.if boundary-checked>
//
// against a much thinner dispatch runtime primitive,
// systolic_dispatch_matmul4x4, that performs exactly one 4x4x4 dispatch and
// does no tiling of its own. Unlike the earlier revision's runtime API,
// this call is named and typed independently of any specific transport:
// the MLIR pass only ever declares and calls
// systolic_dispatch_open/systolic_dispatch_matmul4x4, never
// UART-specific symbols, so linking the same compiled object file against
// a different implementation of those two symbols -- a UART backend, a
// pure-software simulation backend, or (in principle) an AXI/PCIe backend
// -- changes which transport a compiled matmul dispatches to without
// touching a single line of generated IR (Section runtime-abstraction).
// The only remaining runtime-side responsibility is opening/caching
// whatever transport-specific handle systolic_dispatch_open returns and
// implementing the single-tile dispatch itself -- tiling, boundary
// handling, zero-padding, and inter-tile accumulation are now explicit,
// inspectable MLIR IR, entirely independent of that choice.
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

static int64_t ceilDiv4(int64_t x) { return (x + 3) / 4; }

struct TileMatmulForFpgaPattern : public OpRewritePattern<linalg::MatmulOp> {
  using OpRewritePattern::OpRewritePattern;

  // Fills a 4x4 scratch memref (`dest`) from a 4x4 sub-block of `src`
  // (an MxN-shaped memref, M/N given as compile-time constants) whose
  // top-left corner is at dynamic (rowOffset, colOffset) -- typically
  // rowOffset/colOffset = tileIndex * 4 for some scf.for induction
  // variable `tileIndex`. Positions that fall outside [0,srcRows) x
  // [0,srcCols) are zero-filled via an explicit scf.if boundary check,
  // rather than being read out of bounds.
  void emitBoundedTileFill(PatternRewriter &rewriter, Location loc,
                            Value src, int64_t srcRows, int64_t srcCols,
                            Value rowOffset, Value colOffset,
                            Value dest) const {
    auto f32Ty = rewriter.getF32Type();
    Value zero = rewriter.create<arith::ConstantOp>(
        loc, f32Ty, rewriter.getF32FloatAttr(0.0f));
    Value cSrcRows =
        rewriter.create<arith::ConstantIndexOp>(loc, srcRows);
    Value cSrcCols =
        rewriter.create<arith::ConstantIndexOp>(loc, srcCols);

    for (int64_t r = 0; r < 4; ++r) {
      Value cr = rewriter.create<arith::ConstantIndexOp>(loc, r);
      Value ridx = rewriter.create<arith::AddIOp>(loc, rowOffset, cr);
      // in-bounds row predicate: ridx < srcRows
      Value rowOk = rewriter.create<arith::CmpIOp>(
          loc, arith::CmpIPredicate::slt, ridx, cSrcRows);
      for (int64_t c = 0; c < 4; ++c) {
        Value cc = rewriter.create<arith::ConstantIndexOp>(loc, c);
        Value cidx = rewriter.create<arith::AddIOp>(loc, colOffset, cc);
        Value colOk = rewriter.create<arith::CmpIOp>(
            loc, arith::CmpIPredicate::slt, cidx, cSrcCols);
        Value inBounds = rewriter.create<arith::AndIOp>(loc, rowOk, colOk);

        auto ifOp = rewriter.create<scf::IfOp>(loc, inBounds,
                                                /*withElseRegion=*/true);
        {
          OpBuilder::InsertionGuard g(rewriter);
          rewriter.setInsertionPointToStart(&ifOp.getThenRegion().front());
          Value loaded =
              rewriter.create<memref::LoadOp>(loc, src, ValueRange{ridx, cidx});
          rewriter.create<memref::StoreOp>(loc, loaded, dest,
                                            ValueRange{cr, cc});
        }
        {
          OpBuilder::InsertionGuard g(rewriter);
          rewriter.setInsertionPointToStart(&ifOp.getElseRegion().front());
          rewriter.create<memref::StoreOp>(loc, zero, dest, ValueRange{cr, cc});
        }
      }
    }
  }

  // Writes the 4x4 scratch memref `src` back into an MxN-shaped `dest`
  // memref at dynamic (rowOffset, colOffset), skipping positions that
  // fall outside [0,dstRows) x [0,dstCols) via an scf.if boundary check
  // (rather than writing past the true output shape).
  void emitBoundedTileWriteback(PatternRewriter &rewriter, Location loc,
                                 Value src, Value dest, int64_t dstRows,
                                 int64_t dstCols, Value rowOffset,
                                 Value colOffset) const {
    Value cDstRows =
        rewriter.create<arith::ConstantIndexOp>(loc, dstRows);
    Value cDstCols =
        rewriter.create<arith::ConstantIndexOp>(loc, dstCols);

    for (int64_t r = 0; r < 4; ++r) {
      Value cr = rewriter.create<arith::ConstantIndexOp>(loc, r);
      Value ridx = rewriter.create<arith::AddIOp>(loc, rowOffset, cr);
      Value rowOk = rewriter.create<arith::CmpIOp>(
          loc, arith::CmpIPredicate::slt, ridx, cDstRows);
      for (int64_t c = 0; c < 4; ++c) {
        Value cc = rewriter.create<arith::ConstantIndexOp>(loc, c);
        Value cidx = rewriter.create<arith::AddIOp>(loc, colOffset, cc);
        Value colOk = rewriter.create<arith::CmpIOp>(
            loc, arith::CmpIPredicate::slt, cidx, cDstCols);
        Value inBounds = rewriter.create<arith::AndIOp>(loc, rowOk, colOk);

        auto ifOp = rewriter.create<scf::IfOp>(loc, inBounds,
                                                /*withElseRegion=*/false);
        OpBuilder::InsertionGuard g(rewriter);
        rewriter.setInsertionPointToStart(&ifOp.getThenRegion().front());
        Value val =
            rewriter.create<memref::LoadOp>(loc, src, ValueRange{cr, cc});
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

    int64_t M = aTy.getShape()[0];
    int64_t K = aTy.getShape()[1];
    int64_t N = bTy.getShape()[1];

    int64_t Mt = ceilDiv4(M), Kt = ceilDiv4(K), Nt = ceilDiv4(N);

    Location loc = op.getLoc();
    auto module = op->getParentOfType<ModuleOp>();
    auto ptrTy = LLVM::LLVMPointerType::get(rewriter.getContext());
    auto i32Ty = rewriter.getI32Type();

    // Declare (once per module) the two external dispatch-runtime
    // functions this pattern depends on. Both names are deliberately
    // transport-neutral -- neither mentions UART -- so that linking the
    // same compiled object file against a different implementation of
    // these two symbols swaps the backend without touching this pass or
    // any generated IR (Section runtime-abstraction):
    //
    //   int systolic_dispatch_open(void);
    //     Opens (and, on backends where that is meaningful, caches) a
    //     connection to the target 4x4 accelerator, returning an
    //     opaque handle (or a negative value on failure). What "opening
    //     a connection" means is entirely up to the backend: a UART
    //     backend opens a serial port; a simulation backend can return
    //     an arbitrary placeholder value, since there is no real
    //     transport to open.
    //
    //   int systolic_dispatch_matmul4x4(int handle, const float A[16],
    //       const float B[16], const float C_init[16], float C_out[16]);
    //     Exactly one 4x4x4 dispatch: C_out = A @ B + C_init. Performs
    //     no tiling, padding, or accumulation of its own; how the
    //     dispatch is actually carried out (a UART round trip, a
    //     direct in-process computation, or anything else) is entirely
    //     the backend's concern.
    auto declareFn = [&](StringRef name, LLVM::LLVMFunctionType fnTy) {
      auto fn = module.lookupSymbol<LLVM::LLVMFuncOp>(name);
      if (!fn) {
        OpBuilder::InsertionGuard guard(rewriter);
        rewriter.setInsertionPointToStart(module.getBody());
        fn = rewriter.create<LLVM::LLVMFuncOp>(module.getLoc(), name, fnTy);
      }
      return fn;
    };

    auto openFnTy = LLVM::LLVMFunctionType::get(i32Ty, {}, false);
    auto openFn = declareFn("systolic_dispatch_open", openFnTy);

    auto matmul4x4FnTy = LLVM::LLVMFunctionType::get(
        i32Ty, {i32Ty, ptrTy, ptrTy, ptrTy, ptrTy}, false);
    auto matmul4x4Fn = declareFn("systolic_dispatch_matmul4x4", matmul4x4FnTy);

    Value aMemref = tensorToMemref(rewriter, loc, a, aTy);
    Value bMemref = tensorToMemref(rewriter, loc, b, bTy);
    Value cMemref = tensorToMemref(rewriter, loc, c, cTy);

    // One handle for the whole matmul, fetched before the tile loops
    // begin -- matches the previous runtime's behavior of caching a
    // single connection across every tile of one matmul invocation (and
    // across separate invocations), rather than reopening per tile.
    auto openCall = rewriter.create<LLVM::CallOp>(loc, openFn, ValueRange{});
    Value handle = openCall.getResult();

    // Scratch 4x4 buffers, reused across every tile: A-tile, B-tile, and
    // the reduction accumulator (which doubles as both C_init and C_out
    // for systolic_dispatch_matmul4x4, since a well-behaved backend
    // finishes reading its inputs before writing its result -- see
    // Section runtime-abstraction).
    auto tile4x4Ty = MemRefType::get({4, 4}, rewriter.getF32Type());
    Value aTile = rewriter.create<memref::AllocaOp>(loc, tile4x4Ty);
    Value bTile = rewriter.create<memref::AllocaOp>(loc, tile4x4Ty);
    Value acc = rewriter.create<memref::AllocaOp>(loc, tile4x4Ty);

    Value aTilePtr = memrefToLLVMPtr(rewriter, loc, aTile);
    Value bTilePtr = memrefToLLVMPtr(rewriter, loc, bTile);
    Value accPtr = memrefToLLVMPtr(rewriter, loc, acc);

    Value c0 = rewriter.create<arith::ConstantIndexOp>(loc, 0);
    Value c1 = rewriter.create<arith::ConstantIndexOp>(loc, 1);
    Value c4 = rewriter.create<arith::ConstantIndexOp>(loc, 4);
    Value cMt = rewriter.create<arith::ConstantIndexOp>(loc, Mt);
    Value cNt = rewriter.create<arith::ConstantIndexOp>(loc, Nt);
    Value cKt = rewriter.create<arith::ConstantIndexOp>(loc, Kt);
    Value zeroF32 = rewriter.create<arith::ConstantOp>(
        loc, rewriter.getF32Type(), rewriter.getF32FloatAttr(0.0f));

    auto miLoop = rewriter.create<scf::ForOp>(loc, c0, cMt, c1);
    {
      OpBuilder::InsertionGuard g(rewriter);
      rewriter.setInsertionPointToStart(miLoop.getBody());
      Value mi = miLoop.getInductionVar();
      Value rowOffsetA = rewriter.create<arith::MulIOp>(loc, mi, c4);

      auto niLoop = rewriter.create<scf::ForOp>(loc, c0, cNt, c1);
      {
        OpBuilder::InsertionGuard g2(rewriter);
        rewriter.setInsertionPointToStart(niLoop.getBody());
        Value ni = niLoop.getInductionVar();
        Value colOffsetB = rewriter.create<arith::MulIOp>(loc, ni, c4);

        // Zero-init this output tile's accumulator before the K-tile
        // reduction loop -- matches the previous runtime's
        // memcpy(acc, zero16, ...) before its kt loop.
        for (int64_t r = 0; r < 4; ++r) {
          Value cr = rewriter.create<arith::ConstantIndexOp>(loc, r);
          for (int64_t cc = 0; cc < 4; ++cc) {
            Value ccIdx = rewriter.create<arith::ConstantIndexOp>(loc, cc);
            rewriter.create<memref::StoreOp>(loc, zeroF32, acc,
                                              ValueRange{cr, ccIdx});
          }
        }

        auto kiLoop = rewriter.create<scf::ForOp>(loc, c0, cKt, c1);
        {
          OpBuilder::InsertionGuard g3(rewriter);
          rewriter.setInsertionPointToStart(kiLoop.getBody());
          Value ki = kiLoop.getInductionVar();
          Value colOffsetA = rewriter.create<arith::MulIOp>(loc, ki, c4);
          Value rowOffsetB = rewriter.create<arith::MulIOp>(loc, ki, c4);

          // Fill this iteration's A-tile and B-tile, zero-padded at the
          // boundary -- these are the explicit tile/boundary/padding
          // semantics that were previously opaque inside the C runtime.
          emitBoundedTileFill(rewriter, loc, aMemref, M, K, rowOffsetA,
                               colOffsetA, aTile);
          emitBoundedTileFill(rewriter, loc, bMemref, K, N, rowOffsetB,
                               colOffsetB, bTile);

          // acc = a_tile @ b_tile + acc, computed by one 4x4x4 hardware
          // round trip; acc is reused as both C_init and C_out.
          rewriter.create<LLVM::CallOp>(
              loc, matmul4x4Fn,
              ValueRange{handle, aTilePtr, bTilePtr, accPtr, accPtr});
        }

        // Reduction over K-tiles for this output tile is done; write the
        // accumulator back into C, skipping positions past the true
        // M x N output shape.
        emitBoundedTileWriteback(rewriter, loc, acc, cMemref, M, N,
                                  rowOffsetA, colOffsetB);
      }
    }

    // c_mem has now been written in place by the tile loops above;
    // convert back to a tensor to replace the original matmul result.
    // restrict=true: this memref was only just materialized from a
    // distinct tensor and has no other aliases, which One-Shot
    // Bufferize's alias analysis requires to be told explicitly.
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
           "loops (scf.for) with boundary-checked (scf.if) 4x4 tiling, "
           "zero-padding, and reduction accumulation, dispatching each "
           "tile through a transport-neutral dispatch-runtime call "
           "(systolic_dispatch_matmul4x4), independent of the backend "
           "linked to provide it";
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
