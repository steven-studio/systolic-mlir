// RUN: systolic-opt %s --lower-systolic-matmul-tile-to-fpga | FileCheck %s

// -----------------------------------------------------------------------------
// Stage 4 FPGA dispatch contract
//
// Only an explicitly physical-mapped 4x4 systolic.matmul_tile may reach
// fpga_matmul_tiled_auto.
//
//   missing device   -> must remain systolic.matmul_tile
//   8x8 device       -> must remain systolic.matmul_tile
//   4x4 device       -> must lower to llvm.call
// -----------------------------------------------------------------------------

systolic.device @acc_8x8
    rows = 8 cols = 8 depth = 1
    dataflow = weight_stationary

systolic.device @acc_4x4_1
    rows = 4 cols = 4 depth = 1
    dataflow = weight_stationary


// ============================================================================
// CASE 1: No physical accelerator assignment.
//
// This is still intermediate compiler IR.
// Stage 4 must NOT dispatch it to FPGA.
//
// CHECK-LABEL: func.func @unassigned
// CHECK: systolic.matmul_tile
// CHECK-NOT: llvm.call
// ============================================================================

func.func @unassigned(
    %a: tensor<4x4xf32>,
    %b: tensor<4x4xf32>,
    %c: tensor<4x4xf32>) -> tensor<4x4xf32> {

  %r = systolic.matmul_tile
      %a, %b, %c
      m = 4 n = 4 k = 4
      : (tensor<4x4xf32>, tensor<4x4xf32>, tensor<4x4xf32>)
        -> tensor<4x4xf32>

  return %r : tensor<4x4xf32>
}


// ============================================================================
// CASE 2: Explicit 8x8 physical accelerator.
//
// The current FPGA runtime is ONLY a 4x4 backend.
// An 8x8 tile must NOT be silently routed through the 4x4 runtime.
//
// CHECK-LABEL: func.func @mapped_8x8
// CHECK: systolic.matmul_tile
// CHECK-NOT: llvm.call
// ============================================================================

func.func @mapped_8x8(
    %a: tensor<8x8xf32>,
    %b: tensor<8x8xf32>,
    %c: tensor<8x8xf32>) -> tensor<8x8xf32> {

  %r = systolic.matmul_tile
      %a, %b, %c
      m = 8 n = 8 k = 8
      on @acc_8x8
      : (tensor<8x8xf32>, tensor<8x8xf32>, tensor<8x8xf32>)
        -> tensor<8x8xf32>

  return %r : tensor<8x8xf32>
}


// ============================================================================
// CASE 3: Explicit 4x4 physical accelerator.
//
// This is the only case that may cross the Stage-4 FPGA boundary.
//
// CHECK-LABEL: func.func @mapped_4x4
// CHECK: bufferization.to_memref
// CHECK: bufferization.to_memref
// CHECK: bufferization.to_memref
// CHECK: llvm.call @fpga_matmul_tiled_auto
// CHECK-NOT: systolic.matmul_tile
// ============================================================================

func.func @mapped_4x4(
    %a: tensor<4x4xf32>,
    %b: tensor<4x4xf32>,
    %c: tensor<4x4xf32>) -> tensor<4x4xf32> {

  %r = systolic.matmul_tile
      %a, %b, %c
      m = 4 n = 4 k = 4
      on @acc_4x4_1
      : (tensor<4x4xf32>, tensor<4x4xf32>, tensor<4x4xf32>)
        -> tensor<4x4xf32>

  return %r : tensor<4x4xf32>
}
