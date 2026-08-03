// RUN: systolic-opt --systolic-select-device %s | FileCheck %s

// The previous version of this file used `systolic.matmul_tile(64, 64, 64)`,
// which the op's assemblyFormat never accepted -- it takes three SSA
// operands plus m/n/k attributes. The file could not parse, so the greedy
// device-selection pass had no working test at all.
//
// The pass only touches tiles with no device attribute, so nothing here
// may be written `on @acc_...`.
//
// Costs come from the calibrated model, cycles = II*(depth+rows+cols-2)
// + fixedOverhead per tile, times the tile count:
//
//   acc_8x8 (8,8,8):  per-tile 1*(8+8+8-2)+6 = 28
//   acc_4x4 (4,4,4):  per-tile 1*(4+4+4-2)+6 = 16
//
// Devices are walked in declaration order, and tiles are processed
// largest-volume-first, so the expected assignments below are fully
// determined -- there is no tie to break.

module {
  systolic.device @acc_8x8 rows = 8 cols = 8 depth = 8 dataflow = weight_stationary
  systolic.device @acc_4x4 rows = 4 cols = 4 depth = 4 dataflow = weight_stationary

  // Largest tile, considered first. Both devices are idle, so this is a
  // straight cost comparison:
  //   acc_8x8: 8*8*8 = 512 tiles * 28 = 14336
  //   acc_4x4: 16*16*16 = 4096 tiles * 16 = 65536
  // The bigger array wins by 4.6x. Note the old cost model would have
  // said 512+14 vs 4096+6 -- same winner, but for the wrong reason, and
  // with the fill/drain charged once for the whole GEMM instead of once
  // per tile.
  // CHECK-LABEL: func.func @big
  // CHECK: systolic.matmul_tile
  // CHECK-SAME: on @acc_8x8
  // CHECK-SAME: est_cycles = 14336
  func.func @big(%a: tensor<64x64xf32>, %b: tensor<64x64xf32>,
                 %c: tensor<64x64xf32>) -> tensor<64x64xf32> {
    %0 = systolic.matmul_tile %a, %b, %c
         {m = 64 : i64, n = 64 : i64, k = 64 : i64}
         : (tensor<64x64xf32>, tensor<64x64xf32>, tensor<64x64xf32>)
           -> tensor<64x64xf32>
    return %0 : tensor<64x64xf32>
  }

  // Second largest. acc_8x8 is now loaded with 14336 cycles, so the greedy
  // rule compares completion times rather than raw costs:
  //   acc_8x8: 14336 + 64*28 = 16128
  //   acc_4x4:     0 + 512*16 = 8192
  // The slower device wins because it is free. This is the load-balancing
  // behaviour the pass exists for; a cost-only choice would pile
  // everything onto acc_8x8.
  // CHECK-LABEL: func.func @medium
  // CHECK: systolic.matmul_tile
  // CHECK-SAME: on @acc_4x4
  // CHECK-SAME: est_cycles = 8192
  func.func @medium(%a: tensor<32x32xf32>, %b: tensor<32x32xf32>,
                    %c: tensor<32x32xf32>) -> tensor<32x32xf32> {
    %0 = systolic.matmul_tile %a, %b, %c
         {m = 32 : i64, n = 32 : i64, k = 32 : i64}
         : (tensor<32x32xf32>, tensor<32x32xf32>, tensor<32x32xf32>)
           -> tensor<32x32xf32>
    return %0 : tensor<32x32xf32>
  }

  // Smallest, considered last. Loads are 14336 and 8192:
  //   acc_8x8: 14336 + 8*28  = 14560
  //   acc_4x4:  8192 + 64*16 =  9216
  // CHECK-LABEL: func.func @small
  // CHECK: systolic.matmul_tile
  // CHECK-SAME: on @acc_4x4
  // CHECK-SAME: est_cycles = 1024
  func.func @small(%a: tensor<16x16xf32>, %b: tensor<16x16xf32>,
                   %c: tensor<16x16xf32>) -> tensor<16x16xf32> {
    %0 = systolic.matmul_tile %a, %b, %c
         {m = 16 : i64, n = 16 : i64, k = 16 : i64}
         : (tensor<16x16xf32>, tensor<16x16xf32>, tensor<16x16xf32>)
           -> tensor<16x16xf32>
    return %0 : tensor<16x16xf32>
  }

  // A tile that already names a device must be left exactly as it is --
  // the pass reassigning it would silently override an explicit choice,
  // and would also corrupt the running loads it computed above.
  // CHECK-LABEL: func.func @preassigned
  // CHECK: systolic.matmul_tile
  // CHECK-SAME: on @acc_8x8
  // CHECK-NOT: est_cycles
  func.func @preassigned(%a: tensor<8x8xf32>, %b: tensor<8x8xf32>,
                         %c: tensor<8x8xf32>) -> tensor<8x8xf32> {
    %0 = systolic.matmul_tile %a, %b, %c on @acc_8x8
         {m = 8 : i64, n = 8 : i64, k = 8 : i64}
         : (tensor<8x8xf32>, tensor<8x8xf32>, tensor<8x8xf32>)
           -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }
}
