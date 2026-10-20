// RUN: systolic-opt -split-input-file --test-affine-matmul-discovery %s -o /dev/null 2>&1 \
// RUN:   | FileCheck %s --check-prefix=DISC
// RUN: systolic-opt -split-input-file --systolic-declare-fpga-fleet \
// RUN:   "--affine-matmul-to-systolic=allow-fp-reassociation=true" %s | FileCheck %s
//
// A symbol term that no run-time view offset stands for -- a floordiv,
// ceildiv or mod of the nest's symbols -- is no dynamic offset: the access
// is UNKNOWN, as a mod/div subscript is (docs/AFFINE_GEMM_MAPPING.md
// section 10), and the nest stays as written. Regression test: such a term
// was once separated off and lowered as a multiplication (A's rows read at
// i + 2 r instead of i + r floordiv 2). A product by a constant is a run-time
// offset (@scaled_symbol, the control).

// -----

// DISC:      recognition result: failure
// DISC-NEXT:   reason: an access index is not a counter of the nest's loops, or an access map has
// DISC-NEXT:   class: UNKNOWN
// CHECK-LABEL: func.func @floordiv_symbol
// CHECK-NOT:   systolic.matmul_tile
// CHECK:       affine.for
func.func @floordiv_symbol(%A: memref<?x16xf32> {llvm.noalias}, %B: memref<16x8xf32> {llvm.noalias},
    %C: memref<8x8xf32> {llvm.noalias}, %r0: i32) {
  %r = arith.index_cast %r0 : i32 to index
  affine.for %i = 0 to 8 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 16 {
        %a = affine.load %A[%i + symbol(%r) floordiv 2, %k] : memref<?x16xf32>
        %b = affine.load %B[%k, %j] : memref<16x8xf32>
        %c = affine.load %C[%i, %j] : memref<8x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<8x8xf32>
      }
    }
  }
  return
}

// -----

// DISC:      recognition result: failure
// DISC-NEXT:   reason: an access index is not a counter of the nest's loops, or an access map has
// DISC-NEXT:   class: UNKNOWN
// CHECK-LABEL: func.func @ceildiv_symbol
// CHECK-NOT:   systolic.matmul_tile
// CHECK:       affine.for
func.func @ceildiv_symbol(%A: memref<?x16xf32> {llvm.noalias}, %B: memref<16x8xf32> {llvm.noalias},
    %C: memref<8x8xf32> {llvm.noalias}, %r0: i32) {
  %r = arith.index_cast %r0 : i32 to index
  affine.for %i = 0 to 8 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 16 {
        %a = affine.load %A[%i + symbol(%r) ceildiv 3, %k] : memref<?x16xf32>
        %b = affine.load %B[%k, %j] : memref<16x8xf32>
        %c = affine.load %C[%i, %j] : memref<8x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<8x8xf32>
      }
    }
  }
  return
}

// -----

// DISC:      recognition result: failure
// DISC-NEXT:   reason: an access index is not a counter of the nest's loops, or an access map has
// DISC-NEXT:   class: UNKNOWN
// CHECK-LABEL: func.func @mod_symbol
// CHECK-NOT:   systolic.matmul_tile
// CHECK:       affine.for
func.func @mod_symbol(%A: memref<?x16xf32> {llvm.noalias}, %B: memref<16x8xf32> {llvm.noalias},
    %C: memref<8x8xf32> {llvm.noalias}, %r0: i32) {
  %r = arith.index_cast %r0 : i32 to index
  affine.for %i = 0 to 8 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 16 {
        %a = affine.load %A[%i + symbol(%r) mod 4, %k] : memref<?x16xf32>
        %b = affine.load %B[%k, %j] : memref<16x8xf32>
        %c = affine.load %C[%i, %j] : memref<8x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<8x8xf32>
      }
    }
  }
  return
}

// -----

// DISC:      recognition result: failure
// DISC-NEXT:   reason: an access index is not a counter of the nest's loops, or an access map has
// DISC-NEXT:   class: UNKNOWN
// CHECK-LABEL: func.func @merged_floordiv
// CHECK-NOT:   systolic.matmul_tile
// CHECK:       affine.for
func.func @merged_floordiv(%A: memref<?x16xf32> {llvm.noalias}, %B: memref<16x8xf32> {llvm.noalias},
    %C: memref<8x8xf32> {llvm.noalias}, %r0: i32) {
  %r = arith.index_cast %r0 : i32 to index
  affine.for %i = 0 to 8 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 16 {
        %a = affine.load %A[(%i * 4 + symbol(%r)) floordiv 4, %k] : memref<?x16xf32>
        %b = affine.load %B[%k, %j] : memref<16x8xf32>
        %c = affine.load %C[%i, %j] : memref<8x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<8x8xf32>
      }
    }
  }
  return
}

// -----

// DISC:      recognition result: success
// DISC:        symbolic offsets A: ()[s0] -> (s0 * 2, 0) over 1 symbol(s)
// CHECK-LABEL: func.func @scaled_symbol
// CHECK:       %[[R:.*]] = arith.index_cast
// CHECK:       %[[TWO:.*]] = arith.constant 2 : index
// CHECK:       %[[OFF:.*]] = arith.muli %[[R]], %[[TWO]] : index
// CHECK:       memref.subview %{{.*}}[%[[OFF]], 0] [8, 16] [1, 1]
// CHECK:       systolic.matmul_tile
func.func @scaled_symbol(%A: memref<?x16xf32> {llvm.noalias}, %B: memref<16x8xf32> {llvm.noalias},
    %C: memref<8x8xf32> {llvm.noalias}, %r0: i32) {
  %r = arith.index_cast %r0 : i32 to index
  affine.for %i = 0 to 8 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 16 {
        %a = affine.load %A[%i + symbol(%r) * 2, %k] : memref<?x16xf32>
        %b = affine.load %B[%k, %j] : memref<16x8xf32>
        %c = affine.load %C[%i, %j] : memref<8x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<8x8xf32>
      }
    }
  }
  return
}
