#ifndef SYSTOLIC_COSTMODEL_H
#define SYSTOLIC_COSTMODEL_H

#include <cstdint>

namespace systolic {

struct ArrayConfig {
  int64_t rows = 0;
  int64_t cols = 0;
  int64_t kMax = 1;               // reduction capacity per invocation
  int64_t l1Bytes = 0;            // local scratchpad capacity, bytes
  // Calibration constants. These describe the datapath, not the array
  // shape, and must be measured per microarchitecture -- two arrays of
  // identical geometry can differ here by more than an order of magnitude.
  //
  // tileOverhead has no default: a device that does not declare
  // `tile_overhead` is costed with H = 0, which is exactly the geometric
  // model. The HLS pipeline the model was first fitted to measures 6
  // (C/RTL cosim, 14 configurations, residual zero); the hand-written fold
  // RTL on xc7a200t measures 104 with the same II = 1, see
  // test/Systolic/cost_model_fold_rtl.mlir. Both are stated on the device
  // that was measured, not defaulted here.
  int64_t initiationInterval = 1;
  int64_t tileOverhead = 0;
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
