// RUN: systolic-opt --systolic-declare-fpga-fleet --affine-matmul-to-systolic=allow-fp-reassociation=true \
// RUN:   --lower-systolic-matmul-tile-to-fpga="async=true" --systolic-runtime-to-llvm %s | FileCheck %s

// Compile-time guard for the staging of C views the asynchronous runtime
// cannot address in place (affine_to_fpga_async_views.mlir): a 128x128x16
// GEMM whose rows are written backwards is staged in one buffer per GEMM --
// one begin, one wait, every block submitted into the buffer -- at a cost
// linear in the number of blocks, through bufferization and the LLVM
// lowering. Lowering such a view block by block as tensors instead makes
// one-shot bufferization cubic in the number of blocks (about four minutes
// for this function), which shows here as a very slow test.

// CHECK-LABEL: llvm.func @rev128
// CHECK:       llvm.call @fpga_gemm_begin
// CHECK-NOT:   llvm.call @fpga_gemm_begin
// CHECK:       llvm.call @fpga_gemm_wait
// CHECK-NOT:   llvm.call @fpga_gemm_begin
func.func @rev128(%A: memref<128x16xf32> {llvm.noalias}, %B: memref<16x128xf32> {llvm.noalias},
                  %C: memref<128x128xf32> {llvm.noalias}) {
  affine.for %i = 0 to 128 {
    affine.for %j = 0 to 128 {
      affine.for %k = 0 to 16 {
        %a = affine.load %A[%i, %k] : memref<128x16xf32>
        %b = affine.load %B[%k, %j] : memref<16x128xf32>
        %c = affine.load %C[127 - %i, %j] : memref<128x128xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[127 - %i, %j] : memref<128x128xf32>
      }
    }
  }
  return
}
