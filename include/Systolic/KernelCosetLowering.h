//===- KernelCosetLowering.h - Experimental symbolic kernel-coset emission ===//
//
// Phase 5.4.3d M4b.2a (EXPERIMENTAL, off by default).
//
// Emits the symbolic kernel-coset offload of a recognized affine MAC statement
// WITHOUT enumerating source iterations, fibers or tiles. The physical-C output
// fibers are the cosets of the integer kernel of the C access map; this lowering
// walks the output coordinates, peels the outer reduction axes and tiles the
// innermost one with the exact parametric M4b.1 affine bound faces
// (RankPartitionKernelCoset.h, consumed here for the first time in production),
// reconstructing every source coordinate through x = U*y and every physical A/B/C
// address through the original access maps. Each reduction point is a static
// K = 1 device tile (planSystolicMatmul(1,1,1) / emitSystolicTilePlan), so the
// runtime-dependent ragged fiber length is carried entirely by the affine tile
// loop's trip count -- there is no per-fiber static K and no remainder to
// approximate.
//
// It is reached only when the experimental `kernel-coset-offload` pass option is
// set AND every legality check below holds; otherwise it fails closed, leaving
// the source nest exactly as written:
//   - a declared fleet, f32 operands, and A/B provably disjoint from C;
//   - floating-point reductions require reassociation permission (the existing
//     contract): device reduction order is NOT inferred from the M3
//     source-order certificate, so strict FP DECLINES;
//   - the kernel-coset scan plan and the exact M4b.1 tile bounds must both be
//     available (<= 2 reduction axes, unit tile stride, no congruence locals);
//   - the device must admit the 1x1x1 reduction tile.
// No convex-hull/bounding-box approximation, no cardinality budget, no source
// enumeration (including in diagnostics).
//
//===----------------------------------------------------------------------===//

#ifndef SYSTOLIC_KERNELCOSETLOWERING_H
#define SYSTOLIC_KERNELCOSETLOWERING_H

#include "Systolic/GeneralAffineMatmul.h"
#include "Systolic/SystolicTilePlan.h"

#include "mlir/Support/LLVM.h"

#include <cstdint>
#include <string>

namespace mlir {
namespace systolic {

/// Emit the experimental symbolic kernel-coset offload of `stmt`. On success the
/// source nest (stmt.loops.front()) is erased and the new nest is left in its
/// place; `success()` is returned. On any legality failure nothing is mutated,
/// `why` is set to a reason that never enumerates source points, and `failure()`
/// is returned (the caller keeps the source as written).
///
/// `operandsDisjointFromOutput` is the caller's A/B-vs-C aliasing verdict (the
/// aliasing helper is pass-local). `allowFpReassociation` is the pass option of
/// the same name. `options` are the fleet cost-model options for the device
/// planner. `nextGemmIndex` is accepted for signature parity with the fast path
/// but the emission is synchronous (no async FPGA attributes on a looped tile),
/// so it is left unchanged.
///
/// Phase M4b.2b-1: `kdev` is the static device reduction-tile width. `kdev == 1`
/// is the original M4b.2a emission (one 1x1x1 tile per reduction point).
/// `kdev > 1` blocks the contiguous innermost tile axis into full `kdev`-wide
/// `matmul_tile(1,1,kdev)` tiles plus a `k == 1` remainder loop of exactly
/// `L mod kdev` points, where `L` is the (runtime) tile-interval length; the
/// device must admit the 1x1xkdev tile or the offload declines. The peel axes,
/// physical addresses, coverage and C-accumulation are unchanged.
LogicalResult lowerKernelCosetOffload(const RecognizedMacStatement &stmt,
                                      const SystolicFleet &fleet,
                                      const SystolicOptimizerOptions &options,
                                      bool allowFpReassociation,
                                      bool operandsDisjointFromOutput,
                                      unsigned kdev, int64_t &nextGemmIndex,
                                      bool report, std::string &why);

} // namespace systolic
} // namespace mlir

#endif // SYSTOLIC_KERNELCOSETLOWERING_H
