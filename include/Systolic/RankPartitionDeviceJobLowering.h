//===- RankPartitionDeviceJobLowering.h - Certified GEMM to device jobs ---===//
//
// Phase 5.4.3d-3b-6: from a recognized GEMM (the symbolic rank partition
// analysis, through the shared preflight of RankPartitionRecognizedMacAdapter.h)
// to the repository's existing device-job representation: the
// systolic.matmul_tile plan of SystolicTilePlan.h on rank-2 views of the
// source memrefs, which the established pipeline (TileMatmulForFpga,
// the runtime ABI of runtime/fpga_matmul_tiled.h / fpga_async_runtime.h)
// turns into device jobs. Specification: docs/AFFINE_GEMM_MAPPING.md
// section 27.
//
// What a device job is (nothing here invents a field the ABI lacks): one
// invocation C[m x n] += A[m x k] x B[k x n] of one accelerator, A and B
// staged contiguously, C addressed with a row stride, m, n at most the
// array's size, 1 <= k <= K_MAX; the device sums each invocation from +0.0
// in its own order (sixteen banks and a reduction tree, separate binary32
// multiply and add cores, flush-to-zero asserted) and the host adds the sum
// into C; K beyond K_MAX is folded by the compiler into invocations added
// into C in ascending order (SystolicTilePlan.h). Only binary32.
//
// The job-lowering legality contract (every item decided before any
// operation is created; a failed item is UNKNOWN unless stated):
//
//   J1 exact iteration-domain coverage: three logical coordinates, no
//      batch coordinate; a unimodular coordinate map (integral inverse);
//      the exact logical domain equals its candidate box, both inclusions
//      decided with conjunctive integer emptiness only
//      (RankPartitionExactInclusion.h);
//   J2 exact address mapping: in logical coordinates each memref dimension
//      of A, B and C reads exactly one coordinate with coefficient one plus
//      a constant: A as (I, K), B as (K, J), C as (I, J) -- the standard
//      contiguous layout. Strided, transposed, skewed, flattened and
//      broadcast views are not lowered in this phase;
//   J3 dimensions and tile boundaries: M, N, K are the box extents; the
//      planner's blocks and K folds are re-verified to partition
//      [0, M) x [0, N) x [0, K) exactly (planPartitionsBox);
//   J4 layout: rank-2 identity-layout memrefs, static shape except a
//      dynamic leading dimension (the program's own bound, as the fast
//      path assumes); each view is the memref or a static memref.subview
//      with unit strides, in bounds in checked arithmetic;
//   J5 no skipped or duplicated contribution: J1 + the recognizer's
//      multiplicity proof + J3;
//   J6 accumulator initialization: the source accumulates (C += A B), so
//      do the jobs; nothing is zeroed;
//   J7 writeback: C's view is written back with
//      bufferization.materialize_in_destination, as the fast path does;
//   J8 memory: A and B provably disjoint from C by the production rule
//      (the shared preflight; REFUSED when they share a buffer);
//   J9 precision: f32 operands only;
//   J10 arithmetic contract: under strict IEEE semantics no offload is
//      provable (the device's order of additions, its +0 start and its
//      flush-to-zero differ from the loop's: section 6); the jobs are
//      emitted only with the program's permission to reassociate -- the
//      reassoc fast-math flag on the accumulation or the pass option
//      allow-fp-reassociation (section 6's relaxed contract: results equal
//      the loop's exactly when every partial sum is exact, else they may
//      differ in rounding, in the sign of a zero and in NaN payload; a
//      rounding-mode attribute is UNKNOWN).
//
// Phase 3b-7 (section 28): a non-box domain D is lowered when it factors
// as D = O x BK -- every output coordinate's reduction fiber is empty or the
// whole K range of the box (the full-reduction-fiber property, decided
// facet by facet with conjunctive integer emptiness) -- as the dense GEMM
// of the box into a fresh zero temporary T, then C[i][j] += T[i][j] for
// (i, j) in O under the exact Presburger guard of Phase 2b (O is D at the
// lowest K value: a conjunctive polyhedron, no projection). Unselected
// elements of C are never read or written. The box is tightened to the
// domain's constant bounds, and with a dynamic leading dimension the box's
// last row must be an iteration of the nest (the dense GEMM reads every row
// of the box). Arithmetic: the folds' sums are added into T from +0 in
// order and T is added into C once -- with one fold bitwise section 27's
// result (a sum from +0 is never -0), with several folds a reassociation
// of the fold sums, permitted by the relaxed contract. A domain with a
// partial fiber (a K bound depending on i or j) is UNKNOWN.
//
// Transactional: the jobs are emitted immediately before the source nest
// on the source memrefs, the function is verified, and only then is the
// source nest erased; any failure erases what was created and leaves the
// source untouched. The program never holds both computations.
//
//===----------------------------------------------------------------------===//

#ifndef SYSTOLIC_RANKPARTITIONDEVICEJOBLOWERING_H
#define SYSTOLIC_RANKPARTITIONDEVICEJOBLOWERING_H

#include "Systolic/RankPartitionRecognizedMacAdapter.h"
#include "Systolic/SystolicOptimizer.h"
#include "Systolic/SystolicTilePlan.h"

#include "mlir/IR/Builders.h"
#include "mlir/IR/Operation.h"
#include "mlir/IR/Value.h"

#include <array>
#include <cstdint>
#include <optional>
#include <string>

namespace mlir::systolic {

/// One operand's rank-2 view: the memref itself, or the static subview
/// [origin] [size] [1, 1] of it.
struct DeviceJobView {
  Value memref;
  std::array<int64_t, 2> origin{};
  std::array<int64_t, 2> size{};
  bool wholeMemref = false;
};

/// Everything decided before any operation is created.
struct DeviceJobPreflight {
  ::systolic::logical::GuardedMacPreflight common;
  int64_t M = 0, N = 0, K = 0;
  std::array<DeviceJobView, 3> views; // A (M x K), B (K x N), C (M x N)
  bool sharedFactorView = false;      // A and B: one memref, one view
  bool permittedByFlag = false;       // the accumulation carries reassoc
  bool permittedByOption = false;     // allow-fp-reassociation

  // Phase 3b-7: the domain is not a box. The dense GEMM of the box runs
  // into a zero temporary and the exact output mask (`maskPlan`, over the
  // I and J box coordinates in that order) selects the elements of C that
  // receive it. `maskLower` are the box's lower bounds of I and J: the
  // temporary's element of (i, j) is T[i - maskLower[0]][j - maskLower[1]].
  bool maskedOutput = false;
  std::optional<::systolic::logical::SymbolicIterationPlan> maskPlan;
  std::array<int64_t, 2> maskLower{};

  explicit DeviceJobPreflight(::systolic::logical::GuardedMacPreflight pre)
      : common(std::move(pre)) {}
};

/// The contract J1-J10 on the nest `anchor` belongs to. On failure
/// `outcome` is Unknown or Invalid and `why` starts with "UNKNOWN: " or
/// "REFUSED: ".
std::optional<DeviceJobPreflight>
prepareDeviceJobs(Operation *anchor, bool assumeNoAlias,
                  bool allowFpReassociation,
                  ::systolic::logical::GuardedMacOutcome &outcome,
                  std::string &why);

struct DeviceJobReplacement {
  int64_t M = 0, N = 0, K = 0;
  bool maskedOutput = false; // Phase 3b-7: dense GEMM + exact output mask
  int64_t blocks = 0;      // output blocks (1 without a fleet)
  int64_t invocations = 0; // systolic.matmul_tile operations
  std::optional<int64_t> makespan; // the plan's, with a fleet
  Operation *firstEmitted = nullptr; // the first operation of the replacement
  std::string diagnostic;            // the recognizer's report
};

/// The transactional replacement. With an empty fleet one unassigned
/// systolic.matmul_tile covers the GEMM (as the fast path emits it);
/// otherwise the plan of planSystolicMatmul, emitted by
/// emitSystolicTilePlan with the GEMM index `nextGemmIndex` (advanced on
/// success only). `hardError`: the planner refused with its own diagnostic
/// (the caller fails the pass, as for the fast path); the source is
/// untouched then too.
std::optional<DeviceJobReplacement>
replaceGemmWithDeviceJobs(OpBuilder &builder, Operation *anchor,
                          const SystolicFleet &fleet,
                          const SystolicOptimizerOptions &options,
                          bool assumeNoAlias, bool allowFpReassociation,
                          int64_t &nextGemmIndex,
                          ::systolic::logical::GuardedMacOutcome &outcome,
                          std::string &why, bool &hardError);

} // namespace mlir::systolic

#endif // SYSTOLIC_RANKPARTITIONDEVICEJOBLOWERING_H
