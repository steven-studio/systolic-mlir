// RUN: systolic-opt --systolic-declare-fpga-fleet \
// RUN:   --affine-matmul-to-systolic=general-affine=true -split-input-file %s \
// RUN:   | FileCheck %s
// RUN: systolic-opt --systolic-declare-fpga-fleet \
// RUN:   "--affine-matmul-to-systolic=general-affine=true report=true" \
// RUN:   -split-input-file %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=REMARK
//
// Phase C4a: a single-reduction general-affine GEMM whose logical image needs
// more than one congruence local (CASE 1: det(T) = -9, stride-3 reduction
// fibers) is refused by the guarded one-local path, then lowered to real
// systolic.matmul_tile device ops through the C3 symbolic tile partition. The
// fibers have constant lengths 1..4, each a compile-time-constant device K.
// A non-unit reduction step is refused safely, leaving the source unchanged.

// CHECK-LABEL: func.func @general
// CHECK: systolic.matmul_tile
// CHECK-DAG: k = 4
// CHECK-DAG: k = 3
// CHECK-DAG: k = 2
// CHECK-DAG: k = 1
// CHECK-NOT: affine.load

// REMARK: guarded MAC replacement not applied: UNKNOWN: the exact logical image needs more than one congruence local
// REMARK: affine GEMM finite-domain lowered as 37 exact region(s); source MACs=64; output fibers=37; exact Presburger cover and source-order certificate verified
module {
  func.func @general(%C: memref<16x16xf32> {llvm.noalias}, %A: memref<16x16xf32> {llvm.noalias}, %B: memref<16x16xf32> {llvm.noalias}) {
    affine.for %i = 0 to 4 {
      affine.for %j = 0 to 4 {
        affine.for %k = 0 to 4 {
          %t0 = affine.load %C[(((%i + %j) - (2 * %k)) + 6), (((%i - (2 * %j)) + %k) + 6)] : memref<16x16xf32>
          %t1 = affine.load %A[(((%i + %j) - (2 * %k)) + 6), ((%i + %j) + %k)] : memref<16x16xf32>
          %t2 = affine.load %B[((%i + %j) + %k), (((%i - (2 * %j)) + %k) + 6)] : memref<16x16xf32>
          %t3 = arith.mulf %t1, %t2 fastmath<reassoc> : f32
          %t4 = arith.addf %t0, %t3 fastmath<reassoc> : f32
          affine.store %t4, %C[(((%i + %j) - (2 * %k)) + 6), (((%i - (2 * %j)) + %k) + 6)] : memref<16x16xf32>
        }
      }
    }
    func.return
  }
}

// -----

// A non-unit reduction step is not a finite unit-step affine MAC: it must be
// refused, and the original affine nest must survive untouched (no device op).

// CHECK-LABEL: func.func @nonunit
// CHECK-NOT: systolic.matmul_tile
// CHECK: affine.load
// REMARK: general affine
module {
  func.func @nonunit(%C: memref<16x16xf32> {llvm.noalias}, %A: memref<16x16xf32> {llvm.noalias}, %B: memref<16x16xf32> {llvm.noalias}) {
    affine.for %i = 0 to 4 {
      affine.for %j = 0 to 4 {
        affine.for %k = 0 to 4 step 2 {
          %t0 = affine.load %C[(((%i + %j) - (2 * %k)) + 6), (((%i - (2 * %j)) + %k) + 6)] : memref<16x16xf32>
          %t1 = affine.load %A[(((%i + %j) - (2 * %k)) + 6), ((%i + %j) + %k)] : memref<16x16xf32>
          %t2 = affine.load %B[((%i + %j) + %k), (((%i - (2 * %j)) + %k) + 6)] : memref<16x16xf32>
          %t3 = arith.mulf %t1, %t2 fastmath<reassoc> : f32
          %t4 = arith.addf %t0, %t3 fastmath<reassoc> : f32
          affine.store %t4, %C[(((%i + %j) - (2 * %k)) + 6), (((%i - (2 * %j)) + %k) + 6)] : memref<16x16xf32>
        }
      }
    }
    func.return
  }
}
