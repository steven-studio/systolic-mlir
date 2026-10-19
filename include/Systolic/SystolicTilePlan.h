//===- SystolicTilePlan.h - The binary-search tile plan of one GEMM -------===//
//
// What --systolic-binary-search-tile-matmul decides for one M x N x K GEMM,
// as data rather than IR: the spatial decomposition the binary-search
// optimizer picks over the systolic.device fleet, every block's reduction
// folded by its device's k_max, and the compute-clock schedule of every
// invocation. Both IR producers of systolic.matmul_tile on a fleet use it:
//
//   linalg.matmul          --systolic-binary-search-tile-matmul
//   recognized affine GEMM --affine-matmul-to-systolic (through the exact
//                          affine partition of its iteration domain,
//                          AffineMatmulPartitioning.h)
//
// so the two cannot decompose, assign or schedule the same GEMM differently.
//
//===----------------------------------------------------------------------===//

#ifndef SYSTOLIC_SYSTOLICTILEPLAN_H
#define SYSTOLIC_SYSTOLICTILEPLAN_H

#include "Systolic/SystolicOptimizer.h"
#include "Systolic/SystolicTiling.h"

#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Support/LLVM.h"

#include <cstdint>

namespace mlir {
namespace systolic {

/// The systolic.device ops of a module, in walk order: fleet[i] is the
/// accelerator symbols[i] names, and acceleratorId == i.
struct SystolicFleet {
  SmallVector<SystolicArrayResource> resources;
  SmallVector<FlatSymbolRefAttr> symbols;
};

/// Collects and validates the module's systolic.device fleet: positive
/// square geometries, cost parameters the cost model can use, and one set of
/// cost parameters per geometry (the optimizer bills one cost per geometry).
/// Diagnostics go to the offending device or the module. An empty fleet is
/// not an error here.
FailureOr<SystolicFleet> collectSystolicFleet(ModuleOp module);

/// One invocation of one device: C[row .. row+m, column .. column+n] +=
/// A[row .., k0 .. k0+k] x B[k0 .., column ..].
struct SystolicTileInvocation {
  int64_t k0 = 0;
  int64_t k = 0;
  int64_t estCycles = 0;
  int64_t startCycle = 0;
};

/// One m x n block of C on one accelerator, its reduction folded into
/// invocations covering [0, K) in order, chained through c_in.
struct SystolicTileBlock {
  int64_t row = 0;
  int64_t column = 0;
  int64_t m = 0;
  int64_t n = 0;
  int64_t acceleratorId = 0;
  /// The optimizer's task index of this block (its position in the
  /// decomposition): what systolic.tile names, see emitSystolicTilePlan.
  int64_t taskIndex = 0;
  SmallVector<SystolicTileInvocation> invocations;
};

struct SystolicTilePlan {
  int64_t M = 0, N = 0, K = 0;
  SmallVector<SystolicTileBlock> blocks;
  ExactSystolicOptimizationResult optimization;
};

/// The binary-search decomposition of an M x N x K GEMM over `fleet`, K-folded
/// and scheduled. Diagnostics go to `diagOp`.
FailureOr<SystolicTilePlan>
planSystolicMatmul(Operation *diagOp, int64_t M, int64_t N, int64_t K,
                   const SystolicFleet &fleet,
                   const SystolicOptimizerOptions &options);

/// Emits the plan on tensors A (M x K), B (K x N), C (M x N): per block an
/// extract_slice of C, its invocations' systolic.matmul_tile chain on slices
/// of A and B, and an insert_slice back, threaded block to block. Returns
/// the final C.
///
/// gemmIndex >= 0: every matmul_tile also carries the discardable attributes
/// the asynchronous FPGA lowering needs (TileMatmulForFpga.cpp async=true,
/// docs/ASYNC.md) to hand a GEMM's tiles to the runtime without waiting for
/// each one: systolic.gemm = gemmIndex (which GEMM of the module),
/// systolic.tile = the block's task index (which output block of it),
/// systolic.k_index / systolic.k_count (which K fold of that block, of how
/// many). The synchronous lowering ignores them; -1 emits none.
Value emitSystolicTilePlan(OpBuilder &builder, Location loc, Value A, Value B,
                           Value C, const SystolicTilePlan &plan,
                           const SystolicFleet &fleet, int64_t gemmIndex = -1);

/// The first systolic.gemm index no matmul_tile of `module` carries yet: 0
/// in a fresh module, one past the largest index present otherwise. Both IR
/// producers number the GEMMs they emit from it, so a module through which
/// the linalg and the affine path both run keeps every GEMM's index unique
/// (the asynchronous lowering groups a GEMM's tiles by it).
int64_t nextSystolicGemmIndex(ModuleOp module);

} // namespace systolic
} // namespace mlir

#endif // SYSTOLIC_SYSTOLICTILEPLAN_H
