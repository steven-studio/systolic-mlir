// RUN: systolic-opt --systolic-declare-fpga-fleet \
// RUN:   "--affine-matmul-to-systolic=report=true allow-fp-reassociation=true" \
// RUN:   -split-input-file -verify-diagnostics %s | FileCheck %s

// GEMMs at any depth of an affine loop nest. The logical I, J, K come from
// the accesses (AffineMatmulAnalysis), each over one or more loops at any
// level; loops no access uses repeat the GEMM and are moved outward when
// that is a legal interchange; the tiles replace the GEMM's own loops inside
// whatever repeats it.

// A repeating loop between J and K (I, J on loops 1 and 2, K on loop 4):
// moved outward -- outside the reduction it reorders no C element's
// accumulation -- and the GEMM inside it runs 3 times.
// CHECK-LABEL: func.func @repeat_between
// CHECK:       affine.for %{{.*}} = 0 to 3 {
// CHECK-NOT:     affine.for
// CHECK:         systolic.matmul_tile {{.*}} on @acc_8x8_0
// CHECK:       {{^ *}$}}
// CHECK-NEXT:  return
func.func @repeat_between(%A: memref<8x8xf32> {llvm.noalias},
                          %B: memref<8x8xf32> {llvm.noalias},
                          %C: memref<8x8xf32> {llvm.noalias}) {
  // expected-remark @+1 {{affine GEMM 8x8x8 lowered as 1 exact region(s)}}
  affine.for %i = 0 to 8 {
    affine.for %j = 0 to 8 {
      affine.for %r = 0 to 3 {
        affine.for %k = 0 to 8 {
          %a = affine.load %A[%i, %k] : memref<8x8xf32>
          %b = affine.load %B[%k, %j] : memref<8x8xf32>
          %c = affine.load %C[%i, %j] : memref<8x8xf32>
          %p = arith.mulf %a, %b : f32
          %s = arith.addf %c, %p : f32
          affine.store %s, %C[%i, %j] : memref<8x8xf32>
        }
      }
    }
  }
  return
}

// -----

// The remark also says how often the loops around the tiles run them.
// CHECK-LABEL: func.func @repeat_count
func.func @repeat_count(%A: memref<8x8xf32> {llvm.noalias},
                        %B: memref<8x8xf32> {llvm.noalias},
                        %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %r = 0 to 2 {
    // expected-remark @+1 {{; the enclosing loops run it 2 time(s)}}
    affine.for %i = 0 to 8 {
      affine.for %j = 0 to 8 {
        affine.for %k = 0 to 8 {
          %a = affine.load %A[%i, %k] : memref<8x8xf32>
          %b = affine.load %B[%k, %j] : memref<8x8xf32>
          %c = affine.load %C[%i, %j] : memref<8x8xf32>
          %p = arith.mulf %a, %b : f32
          %s = arith.addf %c, %p : f32
          affine.store %s, %C[%i, %j] : memref<8x8xf32>
        }
      }
    }
  }
  return
}

// -----

// A repeating loop inside the reduction: moving it outward would reorder
// dependent iterations (C[i][j] accumulates over k and r interleaved), so
// the interchange is not legal and the nest is left as it is.
// CHECK-LABEL: func.func @repeat_inside_reduction
// CHECK:       affine.for %{{.*}} = 0 to 8 {
// CHECK-NEXT:    affine.for %{{.*}} = 0 to 8 {
// CHECK-NEXT:      affine.for %{{.*}} = 0 to 8 {
// CHECK-NEXT:        affine.for %{{.*}} = 0 to 2 {
// CHECK-NOT:   systolic.matmul_tile
func.func @repeat_inside_reduction(%A: memref<8x8xf32> {llvm.noalias},
                                   %B: memref<8x8xf32> {llvm.noalias},
                                   %C: memref<8x8xf32> {llvm.noalias}) {
  // expected-remark @+1 {{loop 3 of the nest repeats the accumulation inside it, and moving it outward is not a legal loop interchange}}
  affine.for %i = 0 to 8 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        affine.for %r = 0 to 2 {
          %a = affine.load %A[%i, %k] : memref<8x8xf32>
          %b = affine.load %B[%k, %j] : memref<8x8xf32>
          %c = affine.load %C[%i, %j] : memref<8x8xf32>
          %p = arith.mulf %a, %b : f32
          %s = arith.addf %c, %p : f32
          affine.store %s, %C[%i, %j] : memref<8x8xf32>
        }
      }
    }
  }
  return
}

// -----

// Six loops, I, J and K each over two (a tiled nest): I = 8 i0 + i1,
// J = 4 j0 + j1, K = 8 k0 + k1 number the 16x16x24 GEMM densely; all six
// loops are replaced.
// CHECK-LABEL: func.func @tiled6
// CHECK-NOT:   affine.for
// CHECK:       systolic.matmul_tile
// CHECK-NOT:   affine.for
// CHECK:       return
func.func @tiled6(%A: memref<16x24xf32> {llvm.noalias},
                  %B: memref<24x16xf32> {llvm.noalias},
                  %C: memref<16x16xf32> {llvm.noalias}) {
  // expected-remark @+1 {{affine GEMM 16x16x24 lowered as 20 exact region(s)}}
  affine.for %i0 = 0 to 2 {
    affine.for %j0 = 0 to 4 {
      affine.for %k0 = 0 to 3 {
        affine.for %i1 = 0 to 8 {
          affine.for %j1 = 0 to 4 {
            affine.for %k1 = 0 to 8 {
              %a = affine.load %A[8 * %i0 + %i1, 8 * %k0 + %k1] : memref<16x24xf32>
              %b = affine.load %B[8 * %k0 + %k1, 4 * %j0 + %j1] : memref<24x16xf32>
              %c = affine.load %C[8 * %i0 + %i1, 4 * %j0 + %j1] : memref<16x16xf32>
              %p = arith.mulf %a, %b : f32
              %s = arith.addf %c, %p : f32
              affine.store %s, %C[8 * %i0 + %i1, 4 * %j0 + %j1] : memref<16x16xf32>
            }
          }
        }
      }
    }
  }
  return
}

// -----

// Seven loops: r0 outside the GEMM, r1 inside it between J and the second
// I loop (moved outward), I = 4 i0 + i1, K = 8 k0 + k1. The GEMM's five
// loops become the tiles, inside r0 and r1: 2 x 3 = 6 runs.
// CHECK-LABEL: func.func @seven
// CHECK:       affine.for %{{.*}} = 0 to 2 {
// CHECK-NEXT:    affine.for %{{.*}} = 0 to 3 {
// CHECK-NOT:       affine.for
// CHECK:           systolic.matmul_tile
// CHECK:         {{^ *}$}}
// CHECK-NEXT:  }
// CHECK-NEXT:  return
func.func @seven(%A: memref<8x16xf32> {llvm.noalias},
                 %B: memref<16x8xf32> {llvm.noalias},
                 %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %r0 = 0 to 2 {
    // expected-remark @+1 {{affine GEMM 8x8x16 lowered as 1 exact region(s) on 4 accelerator(s): makespan = 500 cycles (read + compute + write), 1 output block(s); the enclosing loops run it 6 time(s)}}
    affine.for %i0 = 0 to 2 {
      affine.for %j = 0 to 8 {
        affine.for %r1 = 0 to 3 {
          affine.for %i1 = 0 to 4 {
            affine.for %k0 = 0 to 2 {
              affine.for %k1 = 0 to 8 {
                %a = affine.load %A[4 * %i0 + %i1, 8 * %k0 + %k1] : memref<8x16xf32>
                %b = affine.load %B[8 * %k0 + %k1, %j] : memref<16x8xf32>
                %c = affine.load %C[4 * %i0 + %i1, %j] : memref<8x8xf32>
                %p = arith.mulf %a, %b : f32
                %s = arith.addf %c, %p : f32
                affine.store %s, %C[4 * %i0 + %i1, %j] : memref<8x8xf32>
              }
            }
          }
        }
      }
    }
  }
  return
}

// -----

// I = 4 i0 + i1 with i1 over 8 values: rows 4..7 are visited twice, so the
// two loops do not number I densely, and no box of (I, J, K) is a set of
// whole iterations.
// CHECK-LABEL: func.func @not_dense
// CHECK-NOT:   systolic.matmul_tile
func.func @not_dense(%A: memref<12x4xf32> {llvm.noalias},
                     %B: memref<4x4xf32> {llvm.noalias},
                     %C: memref<12x4xf32> {llvm.noalias}) {
  // expected-remark @+1 {{logical coordinate I is driven by 2 loops that do not enumerate it densely}}
  affine.for %i0 = 0 to 2 {
    affine.for %i1 = 0 to 8 {
      affine.for %j = 0 to 4 {
        affine.for %k = 0 to 4 {
          %a = affine.load %A[4 * %i0 + %i1, %k] : memref<12x4xf32>
          %b = affine.load %B[%k, %j] : memref<4x4xf32>
          %c = affine.load %C[4 * %i0 + %i1, %j] : memref<12x4xf32>
          %p = arith.mulf %a, %b : f32
          %s = arith.addf %c, %p : f32
          affine.store %s, %C[4 * %i0 + %i1, %j] : memref<12x4xf32>
        }
      }
    }
  }
  return
}
