// RUN: systolic-opt --systolic-select-device %s | FileCheck %s

// The previous version of this file used `systolic.matmul_tile(64, 64, 64)`,
// which the op's assemblyFormat never accepted -- it takes three SSA
// operands plus m/n/k attributes. The file could not parse, so the greedy
// device-selection pass had no working test at all.
//
// The pass only touches tiles with no device attribute, so nothing here
// may be written `on @acc_...`.
//
// Costs come from the calibrated model, cycles = II*(k_max+rows+cols-2)
// + tile_overhead per tile, times the tile count:
//
//   acc_8x8 (8,8,8):  per-tile 1*(8+8+8-2)+6 = 28
//   acc_4x4 (4,4,4):  per-tile 1*(4+4+4-2)+6 = 16
//
// Devices are walked in declaration order, and tiles are processed
// largest-volume-first, so the expected assignments below are fully
// determined -- there is no tie to break.

module {
  systolic.device @acc_8x8 rows = 8 cols = 8 dataflow = weight_stationary
      {k_max = 8 : i64, tile_overhead = 6 : i64}
  systolic.device @acc_4x4 rows = 4 cols = 4 dataflow = weight_stationary
      {k_max = 4 : i64, tile_overhead = 6 : i64}

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

  // A second tile of the same size. acc_8x8 is now carrying 14336 cycles and
  // acc_4x4 is completely idle, and the fast device still wins:
  //   acc_8x8: 14336 + 14336 = 28672
  //   acc_4x4:     0 + 65536 = 65536
  // Finishing second in line on the fast array beats starting first on the
  // slow one. A load balancer that routed by idleness would take the 4x4
  // here and finish 2.3x later.
  // CHECK-LABEL: func.func @big2
  // CHECK: systolic.matmul_tile
  // CHECK-SAME: on @acc_8x8
  // CHECK-SAME: est_cycles = 14336
  func.func @big2(%a: tensor<64x64xf32>, %b: tensor<64x64xf32>,
                  %c: tensor<64x64xf32>) -> tensor<64x64xf32> {
    %0 = systolic.matmul_tile %a, %b, %c
         {m = 64 : i64, n = 64 : i64, k = 64 : i64}
         : (tensor<64x64xf32>, tensor<64x64xf32>, tensor<64x64xf32>)
           -> tensor<64x64xf32>
    return %0 : tensor<64x64xf32>
  }

  // Third. acc_8x8 now carries 28672, and the balance tips the other way:
  //   acc_8x8: 28672 + 16*112 = 30464
  //   acc_4x4:     0 + 64*128  =  8192
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
  //   acc_8x8: 28672 + 16*64 = 29696
  //   acc_4x4:  8192 + 16*64 =  9216
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
