// RUN: systolic-opt --systolic-declare-fpga-fleet="pipeline-depth=4" %s \
// RUN:   | FileCheck %s --check-prefix=PIPE
// RUN: systolic-opt --systolic-declare-fpga-fleet %s \
// RUN:   | FileCheck %s --check-prefix=SER
// RUN: systolic-opt --systolic-declare-fpga-fleet="pipeline-depth=1" %s \
// RUN:   | FileCheck %s --check-prefix=SER
// RUN: not systolic-opt --systolic-declare-fpga-fleet="pipeline-depth=0" %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=BAD
// RUN: systolic-opt --systolic-declare-fpga-fleet="pipeline-depth=4" \
// RUN:   --affine-matmul-to-systolic=report=true %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=SCHED4
// RUN: systolic-opt --systolic-declare-fpga-fleet \
// RUN:   --affine-matmul-to-systolic=report=true %s 2>&1 \
// RUN:   | FileCheck %s --check-prefix=SCHED1

// The C / affine path declares the fleet from the bitstream's record, so
// PIPELINE_DEPTH must reach the device declarations: more than 1 declares
// the pipelined contexts (the per-accelerator timeline of CostModel.h with
// Systolic/FpgaFleet.h's pipeline depth, reduce cycles and feed gap), 1 or
// no option the serialized ones, exactly as systolic-matmul declares them
// for the linalg path. systolic-cc passes the depth it reads from
// runtime-dispatch-test --print-config.

// PIPE: systolic.device @acc_8x8_0 rows = 8 cols = 8 {{.*}} k_max = 16 {{.*}} pipeline_depth = 4 reduce_cycles = 69 feed_gap = 26
// PIPE: systolic.device @acc_4x4_0 {{.*}} pipeline_depth = 4 reduce_cycles = 69 feed_gap = 26
// PIPE: systolic.device @acc_4x4_1 {{.*}} pipeline_depth = 4 reduce_cycles = 69 feed_gap = 26
// PIPE: systolic.device @acc_4x4_2 {{.*}} pipeline_depth = 4 reduce_cycles = 69 feed_gap = 26

// SER:     systolic.device @acc_8x8_0 rows = 8 cols = 8 {{.*}} k_max = 16
// SER-NOT: pipeline_depth
// SER:     systolic.device @acc_4x4_2
// SER-NOT: pipeline_depth
// SER:     func.func @mm_16x16x16

// BAD: pipeline-depth must be >= 1, got 0

// The same 16x16x16 GEMM the linalg path schedules in
// binary_search_pipelined.mlir (pipelined: 949 cycles, the second 8x8 tile's
// fold at 42 while the first is still being reduced) and
// binary_search_rcw.mlir (serialized: 1245 cycles, the second tile at 125).
// SCHED4: makespan = 949 cycles (read + compute + write), 10 output block(s)
// SCHED4: systolic.matmul_tile {{.*}} m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 0
// SCHED4: systolic.matmul_tile {{.*}} m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 42
// SCHED4: systolic.matmul_tile {{.*}} m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 0
// SCHED4: systolic.matmul_tile {{.*}} m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 42
// SCHED4: systolic.matmul_tile {{.*}} m = 4 n = 4 k = 16 on @acc_4x4_0 est_cycles = 117 start_cycle = 120

// SCHED1: makespan = 1245 cycles (read + compute + write), 10 output block(s)
// SCHED1: systolic.matmul_tile {{.*}} m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 0
// SCHED1: systolic.matmul_tile {{.*}} m = 8 n = 8 k = 16 on @acc_8x8_0 est_cycles = 125 start_cycle = 125

func.func @mm_16x16x16(%A: memref<16x16xf32> {llvm.noalias},
                       %B: memref<16x16xf32> {llvm.noalias},
                       %C: memref<16x16xf32> {llvm.noalias}) {
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 16 {
      affine.for %k = 0 to 16 {
        %a = affine.load %A[%i, %k] : memref<16x16xf32>
        %b = affine.load %B[%k, %j] : memref<16x16xf32>
        %c = affine.load %C[%i, %j] : memref<16x16xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<16x16xf32>
      }
    }
  }
  return
}
