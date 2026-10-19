// RUN: systolic-opt --systolic-declare-fpga-fleet --affine-matmul-to-systolic=allow-fp-reassociation=true --systolic-binary-search-tile-matmul \
// RUN:   --lower-systolic-matmul-tile-to-fpga="async=true" %s | FileCheck %s

// GEMMs in one function on views of one buffer, asynchronously.
//
// @two_nests: as the affine front end emits them. Each nest's result is
// written back right after its tiles (materialize_in_destination), which
// is the first reader of the result, so the first GEMM is waited for
// before the second is opened: one GEMM in flight at a time, each on its
// own run-time view of C.
//
// CHECK-LABEL: func.func @two_nests
// CHECK:       %[[H0:.*]] = llvm.call @fpga_gemm_begin
// CHECK-COUNT-5: llvm.call @fpga_tile_submit(%[[H0]],
// CHECK-NOT:   llvm.call @fpga_gemm_begin
// CHECK:       llvm.call @fpga_gemm_wait(%[[H0]])
// CHECK:       bufferization.materialize_in_destination
// CHECK:       %[[H1:.*]] = llvm.call @fpga_gemm_begin
// CHECK-COUNT-5: llvm.call @fpga_tile_submit(%[[H1]],
// CHECK:       llvm.call @fpga_gemm_wait(%[[H1]])
// CHECK:       bufferization.materialize_in_destination
// CHECK:       return
func.func @two_nests(%A: memref<?x16xf32> {llvm.noalias},
                     %B: memref<?x16xf32> {llvm.noalias},
                     %C: memref<?x16xf32> {llvm.noalias}, %r0: i32, %r1: i32) {
  %s0 = arith.index_cast %r0 : i32 to index
  %s1 = arith.index_cast %r1 : i32 to index
  affine.for %i = 0 to 8 {
    affine.for %j = 0 to 16 {
      affine.for %k = 0 to 16 {
        %a = affine.load %A[%i, %k] : memref<?x16xf32>
        %b = affine.load %B[%k, %j] : memref<?x16xf32>
        %c = affine.load %C[%i + symbol(%s0), %j] : memref<?x16xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i + symbol(%s0), %j] : memref<?x16xf32>
      }
    }
  }
  affine.for %i = 0 to 8 {
    affine.for %j = 0 to 16 {
      affine.for %k = 0 to 16 {
        %a = affine.load %A[%i, %k] : memref<?x16xf32>
        %b = affine.load %B[%k, %j] : memref<?x16xf32>
        %c = affine.load %C[%i + symbol(%s1), %j] : memref<?x16xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i + symbol(%s1), %j] : memref<?x16xf32>
      }
    }
  }
  return
}

// Hand-written IR with both write-backs at the end (the ones the
// affine-to-systolic pass makes are moved there by %delay below being the
// output of the first nest fed through memory, not SSA): the lowering
// orders the GEMMs itself. @overlapping: views at run-time offsets of one
// buffer are not provably disjoint, so the second GEMM's begin waits for
// the first. @disjoint: static views of different rows are independent, and
// both GEMMs are in flight before either wait.
//
// CHECK-LABEL: func.func @disjoint
// CHECK:       %[[D0:.*]] = llvm.call @fpga_gemm_begin
// CHECK-COUNT-5: llvm.call @fpga_tile_submit(%[[D0]],
// CHECK-NOT:   llvm.call @fpga_gemm_wait
// CHECK:       %[[D1:.*]] = llvm.call @fpga_gemm_begin
// CHECK-COUNT-5: llvm.call @fpga_tile_submit(%[[D1]],
// CHECK:       llvm.call @fpga_gemm_wait(%[[D0]])
// CHECK:       llvm.call @fpga_gemm_wait(%[[D1]])
// CHECK:       return
func.func @disjoint(%A: tensor<8x16xf32>, %B: tensor<16x16xf32>,
                    %C: memref<16x16xf32> {llvm.noalias}) {
  %v0 = memref.subview %C[0, 0] [8, 16] [1, 1] : memref<16x16xf32> to memref<8x16xf32, strided<[16, 1]>>
  %v1 = memref.subview %C[8, 0] [8, 16] [1, 1] : memref<16x16xf32> to memref<8x16xf32, strided<[16, 1], offset: 128>>
  %t0 = bufferization.to_tensor %v0 restrict writable : memref<8x16xf32, strided<[16, 1]>> to tensor<8x16xf32>
  %t1 = bufferization.to_tensor %v1 restrict writable : memref<8x16xf32, strided<[16, 1], offset: 128>> to tensor<8x16xf32>
  %r0 = linalg.matmul ins(%A, %B : tensor<8x16xf32>, tensor<16x16xf32>) outs(%t0 : tensor<8x16xf32>) -> tensor<8x16xf32>
  %r1 = linalg.matmul ins(%A, %B : tensor<8x16xf32>, tensor<16x16xf32>) outs(%t1 : tensor<8x16xf32>) -> tensor<8x16xf32>
  bufferization.materialize_in_destination %r0 in restrict writable %v0 : (tensor<8x16xf32>, memref<8x16xf32, strided<[16, 1]>>) -> ()
  bufferization.materialize_in_destination %r1 in restrict writable %v1 : (tensor<8x16xf32>, memref<8x16xf32, strided<[16, 1], offset: 128>>) -> ()
  return
}

// CHECK-LABEL: func.func @overlapping
// CHECK:       %[[O0:.*]] = llvm.call @fpga_gemm_begin
// CHECK-COUNT-5: llvm.call @fpga_tile_submit(%[[O0]],
// CHECK-NOT:   llvm.call @fpga_gemm_begin
// CHECK:       llvm.call @fpga_gemm_wait(%[[O0]])
// CHECK:       %[[O1:.*]] = llvm.call @fpga_gemm_begin
// CHECK-COUNT-5: llvm.call @fpga_tile_submit(%[[O1]],
// CHECK:       llvm.call @fpga_gemm_wait(%[[O1]])
// CHECK:       return
func.func @overlapping(%A: tensor<8x16xf32>, %B: tensor<16x16xf32>,
                       %C: memref<?x16xf32> {llvm.noalias}, %o0: i32, %o1: i32) {
  %s0 = arith.index_cast %o0 : i32 to index
  %s1 = arith.index_cast %o1 : i32 to index
  %v0 = memref.subview %C[%s0, 0] [8, 16] [1, 1] : memref<?x16xf32> to memref<8x16xf32, strided<[16, 1], offset: ?>>
  %v1 = memref.subview %C[%s1, 0] [8, 16] [1, 1] : memref<?x16xf32> to memref<8x16xf32, strided<[16, 1], offset: ?>>
  %t0 = bufferization.to_tensor %v0 restrict writable : memref<8x16xf32, strided<[16, 1], offset: ?>> to tensor<8x16xf32>
  %t1 = bufferization.to_tensor %v1 restrict writable : memref<8x16xf32, strided<[16, 1], offset: ?>> to tensor<8x16xf32>
  %r0 = linalg.matmul ins(%A, %B : tensor<8x16xf32>, tensor<16x16xf32>) outs(%t0 : tensor<8x16xf32>) -> tensor<8x16xf32>
  %r1 = linalg.matmul ins(%A, %B : tensor<8x16xf32>, tensor<16x16xf32>) outs(%t1 : tensor<8x16xf32>) -> tensor<8x16xf32>
  bufferization.materialize_in_destination %r0 in restrict writable %v0 : (tensor<8x16xf32>, memref<8x16xf32, strided<[16, 1], offset: ?>>) -> ()
  bufferization.materialize_in_destination %r1 in restrict writable %v1 : (tensor<8x16xf32>, memref<8x16xf32, strided<[16, 1], offset: ?>>) -> ()
  return
}
