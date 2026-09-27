#ifndef SYSTOLIC_COSTMODEL_H
#define SYSTOLIC_COSTMODEL_H

#include <cstdint>

namespace systolic {

struct ArrayConfig {
  int64_t rows = 0;
  int64_t cols = 0;
  int64_t depth = 1;              // K-tile depth (HLS K_DIM)
  int64_t kMax = 0;               // Maximum K reduction depth
  int64_t l1Bytes = 0;            // Local scratchpad capacity
  int64_t initiationInterval = 1; // 校正值：cosim 實測 II
  int64_t fixedOverhead = 95;     // 校正值：init/drain 迴圈 + 介面握手
  int64_t tileOverhead = 95;      // Per-tile calibrated overhead
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
