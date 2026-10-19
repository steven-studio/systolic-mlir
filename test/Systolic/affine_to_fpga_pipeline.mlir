// RUN: systolic-opt --systolic-declare-fpga-fleet --affine-matmul-to-systolic=allow-fp-reassociation=true \
// RUN:   --lower-systolic-matmul-tile-to-fpga --systolic-runtime-to-llvm %s \
// RUN:   | FileCheck %s

// The affine path down to the LLVM dialect, through the FPGA lowering the
// linalg path uses: one runtime call per scheduled region, on the physical
// device the region was assigned (0 = the 8x8, 1..3 = the 4x4s), with its
// m, k, n, start_cycle and est_cycles. The calls follow the IR order of the
// regions; each output block's K invocations are consecutive.

// CHECK:     llvm.func @fpga_matmul_tiled_auto_scheduled(i32, i32, i32, !llvm.ptr, !llvm.ptr, !llvm.ptr, i32, i32, i32) -> i32
// CHECK:     llvm.func @gemm(
// CHECK-COUNT-10: llvm.call @fpga_matmul_tiled_auto_scheduled
// CHECK-NOT: llvm.call @fpga_matmul_tiled_auto_scheduled
// CHECK-NOT: systolic.
// CHECK-NOT: affine.

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
