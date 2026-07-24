//===- CostModel.h - Closed-form systolic array cost model --------------===//
//
// Pure, hardware-agnostic cost functions for fixed-geometry systolic array
// accelerators. Nothing in this file depends on MLIR or on any specific
// board: every hardware fact (array rows/cols/depth, dataflow, bandwidth)
// is a runtime value passed in through ArrayConfig, never a compiled-in
// constant. That's deliberate -- see the "hardware description as data,
// not code" note in the top-level README -- so this file should not need
// to change when the target board (or its config numbers) changes.
//
// This header has no MLIR dependency and can be unit-tested / wrapped in a
// standalone Python binding independently of the rest of the dialect.
//===----------------------------------------------------------------------===//

#ifndef SYSTOLIC_COSTMODEL_H
#define SYSTOLIC_COSTMODEL_H

#include <cstdint>

namespace systolic {

// Fixed hardware geometry of one systolic array accelerator instance.
// Mirrors the attributes on a `systolic.device` op one-to-one.
struct ArrayConfig {
  int64_t rows = 0;              // spatial PE rows
  int64_t cols = 0;              // spatial PE cols
  int64_t depth = 1;             // local reuse capacity along the temporal (K) dim
  double clockHz = 0.0;          // optional; only needed to convert cycles -> seconds
  double dmaBytesPerCycle = 0.0; // optional; used by estimateDmaCycles
};

enum class Dataflow {
  WeightStationary, // K, N mapped to rows/cols; M streams over time
  OutputStationary,
  RowStationary,
};

// Closed-form cycle estimate for a single (M, K) x (K, N) GEMM tile
// executed on `array`, assuming `dataflow`:
//
//   cycles = ceil(M/rows) * ceil(N/cols) * ceil(K/depth) + (rows + cols - 2)
//
// The trailing (rows + cols - 2) term is a first-cut estimate of one
// pipeline fill/drain per tile invocation. It is intentionally *not*
// calibrated against real silicon yet -- see README.md for the plan to
// calibrate against SCALE-Sim / Gemmini before the board arrives.
//
// `dataflow` is accepted but currently does not change which GEMM
// dimension maps to which array axis -- see the NOTE in CostModel.cpp.
int64_t estimateMatmulCycles(int64_t m, int64_t n, int64_t k,
                              const ArrayConfig &array,
                              Dataflow dataflow = Dataflow::WeightStationary);

// Rough DMA transfer cost in cycles: ceil(bytes / bytesPerCycle).
// Deliberately independent of estimateMatmulCycles -- whether a transfer
// overlaps with some neighboring compute tile is a scheduling decision
// (stage 3), not something this function should know about.
int64_t estimateDmaCycles(int64_t bytes, double bytesPerCycle);

} // namespace systolic

#endif // SYSTOLIC_COSTMODEL_H
