// RUN: systolic-opt --systolic-declare-fpga-fleet --affine-matmul-to-systolic=allow-fp-reassociation=true \
// RUN:   -split-input-file -verify-diagnostics %s | FileCheck %s

// GEMMs --affine-matmul-to-systolic recognizes but cannot show the lowering
// correct for. Each is left exactly as it was, with a remark saying why.

// Plain memref arguments may overlap: C written while A is read.
// CHECK-LABEL: func.func @may_alias
// CHECK:       affine.for
// CHECK-NOT:   systolic.matmul_tile
func.func @may_alias(%A: memref<4x4xf32>, %B: memref<4x4xf32>,
                     %C: memref<4x4xf32>) {
  // expected-remark @+1 {{affine GEMM not lowered to systolic.matmul_tile: A may overlap C; declare the arguments llvm.noalias (C restrict) or pass assume-no-alias}}
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i, %k] : memref<4x4xf32>
        %b = affine.load %B[%k, %j] : memref<4x4xf32>
        %c = affine.load %C[%i, %j] : memref<4x4xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<4x4xf32>
      }
    }
  }
  return
}

// -----

// A batched GEMM is lowered since 2026-10-15 (ISSUE-006: the batch loop is
// re-emitted around the tiles); what stays refused is a batch coordinate the
// operands read through different expressions (A[b + 1] with C[b]): one view
// per operand of a batch coordinate is not lowered.
// CHECK-LABEL: func.func @batched
// CHECK-NOT:   systolic.matmul_tile
func.func @batched(%A: memref<3x4x4xf32> {llvm.noalias},
                   %B: memref<2x4x4xf32> {llvm.noalias},
                   %C: memref<2x4x4xf32> {llvm.noalias}) {
  affine.for %b = 0 to 2 {
    affine.for %i = 0 to 4 {
      affine.for %j = 0 to 4 {
        affine.for %k = 0 to 4 {
          %a = affine.load %A[%b + 1, %i, %k] : memref<3x4x4xf32>
          %bb = affine.load %B[%b, %k, %j] : memref<2x4x4xf32>
          %c = affine.load %C[%b, %i, %j] : memref<2x4x4xf32>
          %p = arith.mulf %a, %bb : f32
          %s = arith.addf %c, %p : f32
          affine.store %s, %C[%b, %i, %j] : memref<2x4x4xf32>
        }
      }
    }
  }
  return
}

// -----

// I = i + j: loop j drives both I and J, so no box of (I, J, K) is a set
// of whole iterations.
// CHECK-LABEL: func.func @coupled
// CHECK-NOT:   systolic.matmul_tile
func.func @coupled(%A: memref<8x4xf32> {llvm.noalias},
                   %B: memref<4x4xf32> {llvm.noalias},
                   %C: memref<8x4xf32> {llvm.noalias}) {
  // expected-remark @+1 {{two logical coordinates are driven by the same loop (loop 1 drives I and J)}}
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i + %j, %k] : memref<8x4xf32>
        %b = affine.load %B[%k, %j] : memref<4x4xf32>
        %c = affine.load %C[%i + %j, %j] : memref<8x4xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i + %j, %j] : memref<8x4xf32>
      }
    }
  }
  return
}

// -----

// Rows 5 + 8 * q reach 157 of a 104-row memref.
// CHECK-LABEL: func.func @out_of_bounds
// CHECK-NOT:   systolic.matmul_tile
func.func @out_of_bounds(%A: memref<104x4xf32> {llvm.noalias},
                         %B: memref<4x4xf32> {llvm.noalias},
                         %C: memref<104x4xf32> {llvm.noalias}) {
  // expected-remark @+1 {{A is accessed out of bounds along dimension 0}}
  affine.for %q = 0 to 20 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[5 + 8 * %q, %k] : memref<104x4xf32>
        %b = affine.load %B[%k, %j] : memref<4x4xf32>
        %c = affine.load %C[5 + 8 * %q, %j] : memref<104x4xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[5 + 8 * %q, %j] : memref<104x4xf32>
      }
    }
  }
  return
}

// -----

// A symbolic trip count: no static M for the plan.
// CHECK-LABEL: func.func @symbolic
// CHECK-NOT:   systolic.matmul_tile
func.func @symbolic(%A: memref<?x4xf32> {llvm.noalias},
                    %B: memref<4x4xf32> {llvm.noalias},
                    %C: memref<?x4xf32> {llvm.noalias}, %n: index) {
  // expected-remark @+1 {{the loop bounds are not static, so the iteration domain has no static normalized extent}}
  affine.for %i = 0 to %n {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i, %k] : memref<?x4xf32>
        %b = affine.load %B[%k, %j] : memref<4x4xf32>
        %c = affine.load %C[%i, %j] : memref<?x4xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<?x4xf32>
      }
    }
  }
  return
}

// -----

// Not recognized at all (no remark): C is initialized between the j and k
// loops, which a GEMM replacing the nest would drop.
// CHECK-LABEL: func.func @initialized
// CHECK:       affine.store %{{.*}}, %arg2
// CHECK-NOT:   systolic.matmul_tile
func.func @initialized(%A: memref<4x4xf32> {llvm.noalias},
                       %B: memref<4x4xf32> {llvm.noalias},
                       %C: memref<4x4xf32> {llvm.noalias}) {
  %zero = arith.constant 0.0 : f32
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.store %zero, %C[%i, %j] : memref<4x4xf32>
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i, %k] : memref<4x4xf32>
        %b = affine.load %B[%k, %j] : memref<4x4xf32>
        %c = affine.load %C[%i, %j] : memref<4x4xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<4x4xf32>
      }
    }
  }
  return
}

// -----

// Not recognized: another side effect in the MAC loop.
// CHECK-LABEL: func.func @side_effect
// CHECK:       func.call @observe
// CHECK-NOT:   systolic.matmul_tile
func.func private @observe(f32)
func.func @side_effect(%A: memref<4x4xf32> {llvm.noalias},
                       %B: memref<4x4xf32> {llvm.noalias},
                       %C: memref<4x4xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i, %k] : memref<4x4xf32>
        %b = affine.load %B[%k, %j] : memref<4x4xf32>
        %c = affine.load %C[%i, %j] : memref<4x4xf32>
        %p = arith.mulf %a, %b : f32
        func.call @observe(%p) : (f32) -> ()
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<4x4xf32>
      }
    }
  }
  return
}

// -----

// C is a view (here a cast) of A's allocation: a fresh allocation, but the
// same buffer, so it may overlap.
// CHECK-LABEL: func.func @same_buffer
// CHECK-NOT:   systolic.matmul_tile
func.func @same_buffer(%B: memref<4x4xf32>) {
  %A = memref.alloc() : memref<4x4xf32>
  %C = memref.cast %A : memref<4x4xf32> to memref<4x4xf32>
  // expected-remark @+1 {{A may overlap C}}
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i, %k] : memref<4x4xf32>
        %b = affine.load %B[%k, %j] : memref<4x4xf32>
        %c = affine.load %C[%i, %j] : memref<4x4xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<4x4xf32>
      }
    }
  }
  return
}

// -----

// C += C x B, even with C restrict: A is C.
// CHECK-LABEL: func.func @self
// CHECK-NOT:   systolic.matmul_tile
func.func @self(%B: memref<4x4xf32> {llvm.noalias},
                %C: memref<4x4xf32> {llvm.noalias}) {
  // expected-remark @+1 {{A may overlap C}}
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        %a = affine.load %C[%i, %k] : memref<4x4xf32>
        %b = affine.load %B[%k, %j] : memref<4x4xf32>
        %c = affine.load %C[%i, %j] : memref<4x4xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<4x4xf32>
      }
    }
  }
  return
}
