// RUN: systolic-opt --systolic-declare-fpga-fleet --affine-matmul-to-systolic \
// RUN:   --lower-systolic-matmul-tile-to-fpga="async=true" %s \
// RUN:   | FileCheck %s --implicit-check-not=fpga_matmul_tiled_auto_scheduled --implicit-check-not='memref<8x8xf32, strided' --implicit-check-not='memref<4x4xf32, strided'
// RUN: systolic-opt --systolic-declare-fpga-fleet --affine-matmul-to-systolic \
// RUN:   --lower-systolic-matmul-tile-to-fpga="async=true" --systolic-runtime-to-llvm %s \
// RUN:   | FileCheck %s --check-prefix=LLVM

// The affine path through the asynchronous FPGA lowering: one GEMM of ten
// output blocks is one fpga_gemm_begin, ten fpga_tile_submit -- each with a
// pointer straight into C's own buffer (the memref argument the front end
// bracketed with a restrict, writable to_tensor), row stride 16 -- and ONE
// fpga_gemm_wait after the last submit, before the front end's
// materialize_in_destination (the only reader of the result). C is never
// staged (no strided 8x8 / 4x4 view of it is copied; A and B still are), so
// nothing between two submits reads a result: every tile of the GEMM is
// submitted before any is waited for.
//
// (Before the lowering accepted that to_tensor as the root of an output
// block, each block went through the per-tile fallback, whose insert_slice
// of the result into C read the result tile at once -- so the wait landed
// right after the first submit and the GEMM was submitted one tile at a
// time.)

// CHECK-LABEL: func.func @gemm
// CHECK-SAME: (%[[A:.*]]: memref<16x16xf32> {llvm.noalias}, %[[B:.*]]: memref<16x16xf32> {llvm.noalias}, %[[C:.*]]: memref<16x16xf32> {llvm.noalias})
// CHECK:       bufferization.to_tensor %[[C]] restrict writable
// CHECK:       memref.extract_aligned_pointer_as_index %[[C]]
// CHECK:       llvm.call @fpga_gemm_begin
// CHECK-COUNT-10: llvm.call @fpga_tile_submit
// CHECK-NOT:   llvm.call @fpga_gemm_wait
// CHECK:       llvm.call @fpga_gemm_wait
// CHECK-NOT:   llvm.call @fpga_tile_submit
// CHECK:       bufferization.materialize_in_destination
// CHECK:       return

// The C front end's shape of the same GEMM (test/e2e/affine_c, systolic-cc):
// the arguments are memref<?x16xf32> and the front end views the 16x16 the
// loops cover as a strided subview, so C's root is the to_tensor of a
// memref<16x16xf32, strided<[16, 1]>>. Row-major with a static offset, so
// it is an output-block root too: the pointers go straight into the view
// (ldc = 16 from its stride) and the one wait follows the last submit.
// CHECK-LABEL: func.func @gemm_dyn
// CHECK-SAME: (%[[DA:.*]]: memref<?x16xf32> {llvm.noalias}, %[[DB:.*]]: memref<?x16xf32> {llvm.noalias}, %[[DC:.*]]: memref<?x16xf32> {llvm.noalias})
// CHECK:       %[[CV:.*]] = memref.subview %[[DC]][0, 0] [16, 16] [1, 1] : memref<?x16xf32> to memref<16x16xf32, strided<[16, 1]>>
// CHECK:       bufferization.to_tensor %[[CV]] restrict writable
// CHECK:       memref.extract_aligned_pointer_as_index %[[CV]]
// CHECK:       llvm.call @fpga_gemm_begin
// CHECK-COUNT-10: llvm.call @fpga_tile_submit
// CHECK-NOT:   llvm.call @fpga_gemm_wait
// CHECK:       llvm.call @fpga_gemm_wait
// CHECK-NOT:   llvm.call @fpga_tile_submit
// CHECK:       bufferization.materialize_in_destination
// CHECK:       return

// LLVM-LABEL: llvm.func @gemm
// LLVM:       llvm.call @fpga_gemm_begin
// LLVM-COUNT-10: llvm.call @fpga_tile_submit
// LLVM-NOT:   llvm.call @fpga_gemm_wait
// LLVM:       llvm.call @fpga_gemm_wait
// LLVM-NOT:   llvm.call @fpga_tile_submit
// LLVM-LABEL: llvm.func @gemm_dyn
// LLVM:       llvm.call @fpga_gemm_begin
// LLVM-COUNT-10: llvm.call @fpga_tile_submit
// LLVM-NOT:   llvm.call @fpga_gemm_wait
// LLVM:       llvm.call @fpga_gemm_wait
// LLVM-NOT:   llvm.call @fpga_tile_submit
// LLVM-NOT:   systolic.
// LLVM-NOT:   affine.
// LLVM-NOT:   bufferization.

func.func @gemm(%A: memref<16x16xf32> {llvm.noalias},
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

func.func @gemm_dyn(%A: memref<?x16xf32> {llvm.noalias},
                    %B: memref<?x16xf32> {llvm.noalias},
                    %C: memref<?x16xf32> {llvm.noalias}) {
  affine.for %i = 0 to 16 {
    affine.for %j = 0 to 16 {
      affine.for %k = 0 to 16 {
        %a = affine.load %A[%i, %k] : memref<?x16xf32>
        %b = affine.load %B[%k, %j] : memref<?x16xf32>
        %c = affine.load %C[%i, %j] : memref<?x16xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<?x16xf32>
      }
    }
  }
  return
}
