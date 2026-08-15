#ifndef SYSTOLIC_COSTMODEL_H
#define SYSTOLIC_COSTMODEL_H

#include <cstdint>

namespace systolic {

struct ArrayConfig {
  int64_t rows = 0;
  int64_t cols = 0;
  int64_t depth = 1;              // K-tile depth (HLS K_DIM)
  // Calibration constants. These describe the datapath, not the array
  // shape, and must be measured per microarchitecture -- two arrays of
  // identical geometry can differ here by more than an order of magnitude.
  //
  // The defaults below are the HLS pipeline the model was first fitted to
  // (C/RTL cosim, 14 configurations, residual zero). The hand-written fold
  // RTL on xc7a200t measures fixedOverhead = 104 with the same II = 1;
  // see test/Systolic/cost_model_fold_rtl.mlir.
  //
  // A device that carries `initiation_interval` / `fixed_overhead`
  // attributes overrides these.
  int64_t initiationInterval = 1;
  int64_t fixedOverhead = 6;
  double clockHz = 0.0;
  double dmaBytesPerCycle = 0.0;
};

enum class Dataflow { WeightStationary, OutputStationary, RowStationary };

int64_t estimateMatmulCycles(int64_t m, int64_t n, int64_t k,
                              const ArrayConfig &array,
                              Dataflow dataflow = Dataflow::WeightStationary);

int64_t estimateDmaCycles(int64_t bytes, double bytesPerCycle);

} // namespace systolic

#endif // SYSTOLIC_COSTMODEL_H
