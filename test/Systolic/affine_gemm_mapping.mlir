// Affine GEMM mapping (docs/AFFINE_GEMM_MAPPING.md): positive and negative
// nests with the reason for each. Every expectation below -- accept or
// refuse, M x N x K, each operand's view (offset, size, stride) -- was
// derived by enumerating the iteration points (the brute-force decision of
// Inputs/affine_gemm_mapping_oracle.py, which does not use the structure
// theorem the compiler relies on), not read from the compiler's output.
// A reason is the compiler's diagnostic for the condition the
// specification says is violated.
//
// RUN: systolic-opt --test-affine-matmul-discovery -split-input-file %s \
// RUN:   -o /dev/null 2>&1 | FileCheck %s --check-prefix=DISC
// RUN: systolic-opt --affine-matmul-to-systolic=allow-fp-reassociation=true -split-input-file %s \
// RUN:   | FileCheck %s
// RUN: systolic-opt "--affine-matmul-to-systolic=report=true allow-fp-reassociation=true" \
// RUN:   -split-input-file %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=REMARK

// DISC:       unique logical matmul count: 1
// DISC:       dense logical GEMM: yes
// DISC:       unique logical matmul count: 1
// DISC:       dense logical GEMM: yes
// DISC:       unique logical matmul count: 1
// DISC:       dense logical GEMM: yes
// DISC:       unique logical matmul count: 1
// DISC:       dense logical GEMM: yes
// DISC:       unique logical matmul count: 1
// DISC:       dense logical GEMM: yes
// DISC:       unique logical matmul count: 1
// DISC:       dense logical GEMM: yes
// DISC:       unique logical matmul count: 1
// DISC:       dense logical GEMM: yes
// DISC:       unique logical matmul count: 1
// DISC:       dense logical GEMM: yes
// DISC:       unique logical matmul count: 1
// DISC:       dense logical GEMM: yes
// DISC:       unique logical matmul count: 1
// DISC:       dense logical GEMM: yes
// DISC:       unique logical matmul count: 1
// DISC:       dense logical GEMM: yes
// DISC:       unique logical matmul count: 1
// DISC:       dense logical GEMM: yes
// DISC:       unique logical matmul count: 1
// DISC:       dense logical GEMM: yes
// DISC:       unique logical matmul count: 1
// DISC:       dense logical GEMM: yes
// DISC:       unique logical matmul count: 1
// DISC:       dense logical GEMM: yes
// DISC:       unique logical matmul count: 1
// DISC:       dense logical GEMM: yes
// DISC:       unique logical matmul count: 1
// DISC:       layout A: roles in dimensions (1, 0) transposed
// DISC:       dense logical GEMM: yes
// DISC:       reason: logical coordinate I (the first factor's dimension 0 d0 * 4 + d1, C's dimension 0 d0 * 8 + d1): the two subscripts are not multiples
// DISC-NEXT:  class: UNKNOWN
// DISC:       unique logical matmul count: 0
// DISC:       reason: a coordinate is read by the second factor only: the other operands would be broadcast along it
// DISC-NEXT:  class: VALID_BUT_LOWERING_UNSUPPORTED
// DISC:       unique logical matmul count: 0
// DISC:       unique logical matmul count: 1
// DISC:       partition: loop(s) 1 demoted to batch loops
// DISC:       dense logical GEMM: yes, I/J/K on loops //2, batched over loop(s) 0+1 (64 batch iteration(s))
// DISC:       unique logical matmul count: 1
// DISC:       dense logical GEMM: no (logical coordinate K is driven by 2 loops that do not enumerate it densely
// DISC:       unique logical matmul count: 1
// DISC:       dense logical GEMM: yes, I/J/K on loops 0//2, batched over loop(s) 1 (8 batch iteration(s))
// DISC:       reason: {{.*}}the accumulator is loaded and stored at different elements of C
// DISC:       unique logical matmul count: 0
// DISC:       unique logical matmul count: 1
// DISC:       partition: loop(s) 0 demoted to batch loops
// DISC:       dense logical GEMM: yes, I/J/K on loops {{.*}}, batched over loop(s) 0 (8 batch iteration(s))
// DISC:       unique logical matmul count: 1
// DISC:       dense logical GEMM: yes
// DISC:       unique logical matmul count: 1
// DISC:       dense logical GEMM: yes
// DISC:       unique logical matmul count: 1
// DISC:       dense logical GEMM: no (a loop of the nest has no iteration (an empty GEMM: M, N or K is 0))
// DISC:       unique logical matmul count: 1
// DISC:       dense logical GEMM: no (the iteration domain is not centrally symmetric -- the point ({{[-0-9, ]+}}) is in it, its mirror ({{[-0-9, ]+}}) about the midpoint of its lexicographic extremes is not --, so it is no affine image of a box; the accesses tell the iterations apart (their stacked map is injective), so the nest is not a GEMM of affine views in any decomposition
// DISC-NEXT:  bounds: loop 1's upper bound depends on another loop's counter: the iteration domain is not a box
// DISC:       unique logical matmul count: 1
// DISC:       dense logical GEMM: yes
// DISC:       unique logical matmul count: 1
// DISC:       dense logical GEMM: yes, I/J/K on loops /1/0+2
//
// CHECK-LABEL: func.func @canonical
// CHECK-NOT:   affine.for
// CHECK:       systolic.matmul_tile {{.*}} m = 8 n = 8 k = 8
// CHECK-LABEL: func.func @different_expressions
// CHECK-NOT:   affine.for
// CHECK:       memref.subview %arg0[1, 0] [4, 4] [2, 3]
// CHECK:       memref.subview %arg1[2, 0] [4, 4] [6, 1]
// CHECK:       memref.subview %arg2[0, 1] [4, 4] [4, 2]
// CHECK:       systolic.matmul_tile {{.*}} m = 4 n = 4 k = 4
// CHECK-LABEL: func.func @operand_offsets
// CHECK-NOT:   affine.for
// CHECK:       memref.subview %arg2[8, 16] [8, 8] [1, 1]
// CHECK:       systolic.matmul_tile {{.*}} m = 8 n = 8 k = 8
// CHECK-LABEL: func.func @strided_views
// CHECK-NOT:   affine.for
// CHECK:       memref.subview %arg0[0, 0] [8, 8] [1, 2]
// CHECK:       memref.subview %arg1[1, 0] [8, 8] [3, 1]
// CHECK:       systolic.matmul_tile {{.*}} m = 8 n = 8 k = 8
// CHECK-LABEL: func.func @step3_origin2
// CHECK-NOT:   affine.for
// CHECK:       memref.subview %arg0[2, 0] [4, 8] [3, 1]
// CHECK:       memref.subview %arg2[2, 0] [4, 8] [3, 1]
// CHECK:       systolic.matmul_tile {{.*}} m = 4 n = 8 k = 8
// CHECK-LABEL: func.func @down_reduction_step2
// CHECK-NOT:   affine.for
// CHECK:       memref.subview %arg0[0, 0] [4, 8] [1, 2]
// CHECK:       memref.subview %arg1[0, 0] [8, 8] [2, 1]
// CHECK:       systolic.matmul_tile {{.*}} m = 4 n = 8 k = 8
// CHECK-LABEL: func.func @affine_condition_down
// CHECK-NOT:   affine.for
// CHECK:       memref.subview %arg0[2, 0] [99, 8] [1, 1]
// CHECK:       memref.subview %arg2[2, 0] [99, 8] [1, 1]
// CHECK:       systolic.matmul_tile {{.*}} m = 99 n = 8 k = 8
// CHECK-LABEL: func.func @split_steps_reversed
// CHECK-NOT:   affine.for
// CHECK:       memref.subview %arg0[1, 2] [8, 3] [2, 1]
// CHECK:       memref.subview %arg1[0, 3] [3, 4] [2, -1]
// CHECK:       memref.subview %arg2[5, 6] [8, 4] [4, -2]
// CHECK:       systolic.matmul_tile {{.*}} m = 8 n = 4 k = 3
// CHECK-LABEL: func.func @once_offset
// CHECK-NOT:   affine.for
// CHECK:       memref.subview %arg2[3, 0] [8, 8] [1, 1]
// CHECK:       systolic.matmul_tile {{.*}} m = 8 n = 8 k = 8
// CHECK-LABEL: func.func @once_in_two_coordinates
// CHECK-NOT:   affine.for
// CHECK:       memref.subview %arg0[6, 0] [8, 8] [1, 1]
// CHECK:       memref.subview %arg1[0, 2] [8, 8] [1, 1]
// CHECK:       memref.subview %arg2[3, 3] [8, 8] [1, 1]
// CHECK:       systolic.matmul_tile {{.*}} m = 8 n = 8 k = 8
// CHECK-LABEL: func.func @once_breaks_radix_syntactically
// CHECK-NOT:   affine.for
// CHECK:       memref.subview %arg0[10, 0] [8, 8] [1, 1]
// CHECK:       memref.subview %arg2[10, 0] [8, 8] [1, 1]
// CHECK:       systolic.matmul_tile {{.*}} m = 8 n = 8 k = 8
// CHECK-LABEL: func.func @acc_equal_on_domain
// CHECK-NOT:   affine.for
// CHECK:       memref.subview %arg2[3, 0] [8, 8] [1, 1]
// CHECK:       systolic.matmul_tile {{.*}} m = 8 n = 8 k = 8
// CHECK-LABEL: func.func @near_int64_max
// CHECK-NOT:   affine.for
// CHECK:       systolic.matmul_tile {{.*}} m = 7 n = 8 k = 8
// CHECK-LABEL: func.func @near_int64_min
// CHECK-NOT:   affine.for
// CHECK:       systolic.matmul_tile {{.*}} m = 7 n = 8 k = 8
// CHECK-LABEL: func.func @large_stride
// CHECK-NOT:   affine.for
// CHECK:       memref.subview %arg0[0, 0] [8, 8] [1099511627776, 1]
// CHECK:       systolic.matmul_tile {{.*}} m = 8 n = 8 k = 8
// CHECK-LABEL: func.func @opposite_directions
// CHECK-NOT:   affine.for
// CHECK:       memref.subview %arg0[7, 0] [8, 8] [-1, 1]
// CHECK-NOT:   memref.subview %arg2
// CHECK:       systolic.matmul_tile {{.*}} m = 8 n = 8 k = 8
// CHECK-LABEL: func.func @transposed_a
// CHECK-NOT:   affine.for
// CHECK:       %[[SV:.*]] = memref.subview %arg0[0, 0] [8, 8] [1, 1]
// CHECK:       memref.transpose %[[SV]] (d0, d1) -> (d1, d0)
// CHECK:       systolic.matmul_tile {{.*}} m = 8 n = 8 k = 8
// CHECK-LABEL: func.func @different_radix_sq1
// CHECK:       affine.for
// CHECK-NOT:   systolic.matmul_tile
// CHECK-LABEL: func.func @reduction_in_output
// CHECK:       affine.for
// CHECK-NOT:   systolic.matmul_tile
// CHECK-LABEL: func.func @wrong_coordinate
// CHECK:       affine.for
// CHECK:       systolic.matmul_tile {{.*}} m = 1 n = 1 k = 8
// CHECK-LABEL: func.func @reduction_multiplicity
// CHECK:       affine.for
// CHECK-NOT:   systolic.matmul_tile
// CHECK-LABEL: func.func @coupled
// CHECK:       affine.for
// CHECK:         systolic.matmul_tile {{.*}} m = 8 n = 1 k = 8
// CHECK-LABEL: func.func @acc_different_element
// CHECK:       affine.for
// CHECK-NOT:   systolic.matmul_tile
// CHECK-LABEL: func.func @broadcast_once
// CHECK:       affine.for
// CHECK:       systolic.matmul_tile {{.*}} m = 1 n = 8 k = 8
// CHECK-LABEL: func.func @oob_static
// CHECK:       affine.for
// CHECK-NOT:   systolic.matmul_tile
// CHECK-LABEL: func.func @oob_negative
// CHECK:       affine.for
// CHECK-NOT:   systolic.matmul_tile
// CHECK-LABEL: func.func @empty_loop
// CHECK:       affine.for
// CHECK-NOT:   systolic.matmul_tile
// CHECK-LABEL: func.func @triangular
// CHECK:       affine.for
// CHECK-NOT:   systolic.matmul_tile
// CHECK-LABEL: func.func @overflow_view
// CHECK:       affine.for
// CHECK-NOT:   systolic.matmul_tile
// CHECK-LABEL: func.func @constant_coordinate
// CHECK-NOT:   affine.for
// CHECK:       memref.subview %arg0[0, 0] [1, 8] [1, 1]
// CHECK:       memref.subview %arg2[0, 0] [1, 8] [1, 1]
// CHECK:       systolic.matmul_tile {{.*}} m = 1 n = 8 k = 8
//
// REMARK:      remark: affine GEMM 8x8x8 lowered as 1 exact region(s)
// REMARK:      remark: affine GEMM 4x4x4 lowered as 1 exact region(s)
// REMARK:      remark: affine GEMM 8x8x8 lowered as 1 exact region(s)
// REMARK:      remark: affine GEMM 8x8x8 lowered as 1 exact region(s)
// REMARK:      remark: affine GEMM 4x8x8 lowered as 1 exact region(s)
// REMARK:      remark: affine GEMM 4x8x8 lowered as 1 exact region(s)
// REMARK:      remark: affine GEMM 99x8x8 lowered as 1 exact region(s)
// REMARK:      remark: affine GEMM 8x4x3 lowered as 1 exact region(s)
// REMARK:      remark: affine GEMM 8x8x8 lowered as 1 exact region(s)
// REMARK:      remark: affine GEMM 8x8x8 lowered as 1 exact region(s)
// REMARK:      remark: affine GEMM 8x8x8 lowered as 1 exact region(s)
// REMARK:      remark: affine GEMM 8x8x8 lowered as 1 exact region(s)
// REMARK:      remark: affine GEMM 7x8x8 lowered as 1 exact region(s)
// REMARK:      remark: affine GEMM 7x8x8 lowered as 1 exact region(s)
// REMARK:      remark: affine GEMM 8x8x8 lowered as 1 exact region(s)
// REMARK:      remark: affine GEMM 8x8x8 lowered as 1 exact region(s)
// REMARK:      remark: affine GEMM 8x8x8 lowered as 1 exact region(s)
// REMARK:      remark: affine GEMM not lowered to systolic.matmul_tile: logical coordinate K is driven by 2 loops that do not enumerate it densely
// REMARK:      remark: affine GEMM 8x1x8 lowered as 1 exact region(s), batched: 8 batch iteration(s) of 1 batch loop(s)
// REMARK:      remark: affine GEMM not lowered to systolic.matmul_tile: C is accessed out of bounds along dimension 0
// REMARK:      remark: affine GEMM not lowered to systolic.matmul_tile: A is accessed out of bounds along dimension 0
// REMARK:      remark: affine GEMM not lowered to systolic.matmul_tile: a loop of the nest has no iteration (an empty GEMM: M, N or K is 0)
// REMARK:      remark: affine GEMM not lowered to systolic.matmul_tile: the iteration domain is not centrally symmetric -- the point ({{[-0-9, ]+}}) is in it, its mirror ({{[-0-9, ]+}}) about the midpoint of its lexicographic extremes is not --, so it is no affine image of a box; the accesses tell the iterations apart (their stacked map is injective), so the nest is not a GEMM of affine views in any decomposition (docs/AFFINE_GEMM_MAPPING.md sections 19.1, 19.7, 19.13) (loop 1's upper bound depends on another loop's counter: the iteration domain is not a box)
// REMARK:      remark: affine GEMM not lowered to systolic.matmul_tile: A's subscripts along dimension 0 overflow 64-bit arithmetic
// REMARK:      remark: affine GEMM 1x8x8 lowered as 1 exact region(s)
// REMARK-NOT:  remark:
//

// canonical: the canonical GEMM: identical coordinates, whole memrefs
// expected: ACCEPT 8x8x8
func.func @canonical(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %x0 = 0 to 8 {
    affine.for %x1 = 0 to 8 {
      affine.for %x2 = 0 to 8 {
        %a = affine.load %A[%x0, %x2] : memref<8x8xf32>
        %b = affine.load %B[%x2, %x1] : memref<8x8xf32>
        %c = affine.load %C[%x0, %x1] : memref<8x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%x0, %x1] : memref<8x8xf32>
      }
    }
  }
  return
}

// -----

// different_expressions: six different subscripts, A[2i+1][3k] B[6k+2][j] C[4i][2j+1]: each pair positively proportional (C1), so each operand has its own strided view
// expected: ACCEPT 4x4x4
func.func @different_expressions(%A: memref<64x64xf32> {llvm.noalias}, %B: memref<64x64xf32> {llvm.noalias}, %C: memref<64x64xf32> {llvm.noalias}) {
  affine.for %x0 = 0 to 4 {
    affine.for %x1 = 0 to 4 {
      affine.for %x2 = 0 to 4 {
        %a = affine.load %A[%x0 * 2 + 1, %x2 * 3] : memref<64x64xf32>
        %b = affine.load %B[%x2 * 6 + 2, %x1] : memref<64x64xf32>
        %c = affine.load %C[%x0 * 4, %x1 * 2 + 1] : memref<64x64xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%x0 * 4, %x1 * 2 + 1] : memref<64x64xf32>
      }
    }
  }
  return
}

// -----

// operand_offsets: C[i+8][j+16]: a constant offset on one operand only
// expected: ACCEPT 8x8x8
func.func @operand_offsets(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<32x32xf32> {llvm.noalias}) {
  affine.for %x0 = 0 to 8 {
    affine.for %x1 = 0 to 8 {
      affine.for %x2 = 0 to 8 {
        %a = affine.load %A[%x0, %x2] : memref<8x8xf32>
        %b = affine.load %B[%x2, %x1] : memref<8x8xf32>
        %c = affine.load %C[%x0 + 8, %x1 + 16] : memref<32x32xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%x0 + 8, %x1 + 16] : memref<32x32xf32>
      }
    }
  }
  return
}

// -----

// strided_views: A[i][2k] with B[3k+1][j]: K strided differently in A and in B
// expected: ACCEPT 8x8x8
func.func @strided_views(%A: memref<8x16xf32> {llvm.noalias}, %B: memref<32x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %x0 = 0 to 8 {
    affine.for %x1 = 0 to 8 {
      affine.for %x2 = 0 to 8 {
        %a = affine.load %A[%x0, %x2 * 2] : memref<8x16xf32>
        %b = affine.load %B[%x2 * 3 + 1, %x1] : memref<32x8xf32>
        %c = affine.load %C[%x0, %x1] : memref<8x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%x0, %x1] : memref<8x8xf32>
      }
    }
  }
  return
}

// -----

// step3_origin2: for (i = 2; i < 13; i += 3): IV values 2, 5, 8, 11 are not the ordinals 0..3 (section 1)
// expected: ACCEPT 4x8x8
func.func @step3_origin2(%A: memref<16x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<16x8xf32> {llvm.noalias}) {
  affine.for %x0 = 2 to 13 step 3 {
    affine.for %x1 = 0 to 8 {
      affine.for %x2 = 0 to 8 {
        %a = affine.load %A[%x0, %x2] : memref<16x8xf32>
        %b = affine.load %B[%x2, %x1] : memref<8x8xf32>
        %c = affine.load %C[%x0, %x1] : memref<16x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%x0, %x1] : memref<16x8xf32>
      }
    }
  }
  return
}

// -----

// down_reduction_step2: for (k = 14; k >= 0; k -= 2), as the front end emits it: k = 14 - 2t, t in [0, 8): the reduction runs backwards (section 7: same terms, reassociated)
// expected: ACCEPT 4x8x8
func.func @down_reduction_step2(%A: memref<4x16xf32> {llvm.noalias}, %B: memref<16x8xf32> {llvm.noalias}, %C: memref<4x8xf32> {llvm.noalias}) {
  affine.for %x0 = 0 to 4 {
    affine.for %x1 = 0 to 8 {
      affine.for %x2 = 0 to 8 {
        %a = affine.load %A[%x0, %x2 * -2 + 14] : memref<4x16xf32>
        %b = affine.load %B[%x2 * -2 + 14, %x1] : memref<16x8xf32>
        %c = affine.load %C[%x0, %x1] : memref<4x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%x0, %x1] : memref<4x8xf32>
      }
    }
  }
  return
}

// -----

// affine_condition_down: for (int i = 100; 2 * i - 3 > 0; i--): i > 1, 99 iterations, i = 100 - t; rows 2..100
// expected: ACCEPT 99x8x8
func.func @affine_condition_down(%A: memref<101x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<101x8xf32> {llvm.noalias}) {
  affine.for %x0 = 0 to 99 {
    affine.for %x1 = 0 to 8 {
      affine.for %x2 = 0 to 8 {
        %a = affine.load %A[%x0 * -1 + 100, %x2] : memref<101x8xf32>
        %b = affine.load %B[%x2, %x1] : memref<8x8xf32>
        %c = affine.load %C[%x0 * -1 + 100, %x1] : memref<101x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%x0 * -1 + 100, %x1] : memref<101x8xf32>
      }
    }
  }
  return
}

// -----

// split_steps_reversed: I split over i0 and i1 = 1, 3, 5, 7 (step 2) with different strides in A and C; J decreasing in both B and C; K offset in A and strided in B
// expected: ACCEPT 8x4x3
func.func @split_steps_reversed(%A: memref<16x8xf32> {llvm.noalias}, %B: memref<8x4xf32> {llvm.noalias}, %C: memref<40x8xf32> {llvm.noalias}) {
  affine.for %x0 = 0 to 2 {
    affine.for %x1 = 1 to 8 step 2 {
      affine.for %x2 = 0 to 4 {
        affine.for %x3 = 0 to 3 {
          %a = affine.load %A[%x0 * 8 + %x1, %x3 + 2] : memref<16x8xf32>
          %b = affine.load %B[%x3 * 2, %x2 * -1 + 3] : memref<8x4xf32>
          %c = affine.load %C[%x0 * 16 + %x1 * 2 + 3, %x2 * -2 + 6] : memref<40x8xf32>
          %p = arith.mulf %a, %b : f32
          %s = arith.addf %c, %p : f32
          affine.store %s, %C[%x0 * 16 + %x1 * 2 + 3, %x2 * -2 + 6] : memref<40x8xf32>
        }
      }
    }
  }
  return
}

// -----

// once_offset: t runs once (t = 3): C[i + t][j] equals C[i + 3][j] on the domain (Lemma 0); refused at the checkpoint
// expected: ACCEPT 8x8x8
func.func @once_offset(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<16x8xf32> {llvm.noalias}) {
  affine.for %x0 = 0 to 8 {
    affine.for %x1 = 3 to 4 {
      affine.for %x2 = 0 to 8 {
        affine.for %x3 = 0 to 8 {
          %a = affine.load %A[%x0, %x3] : memref<8x8xf32>
          %b = affine.load %B[%x3, %x2] : memref<8x8xf32>
          %c = affine.load %C[%x0 + %x1, %x2] : memref<16x8xf32>
          %p = arith.mulf %a, %b : f32
          %s = arith.addf %c, %p : f32
          affine.store %s, %C[%x0 + %x1, %x2] : memref<16x8xf32>
        }
      }
    }
  }
  return
}

// -----

// once_in_two_coordinates: t runs once and appears in I and in J: its coefficients constrain nothing (Theorem 3, degenerate loops); refused at the checkpoint
// expected: ACCEPT 8x8x8
func.func @once_in_two_coordinates(%A: memref<16x8xf32> {llvm.noalias}, %B: memref<8x16xf32> {llvm.noalias}, %C: memref<16x16xf32> {llvm.noalias}) {
  affine.for %x0 = 0 to 8 {
    affine.for %x1 = 3 to 4 {
      affine.for %x2 = 0 to 8 {
        affine.for %x3 = 0 to 8 {
          %a = affine.load %A[%x0 + %x1 * 2, %x3] : memref<16x8xf32>
          %b = affine.load %B[%x3, %x1 * -1 + %x2 + 5] : memref<8x16xf32>
          %c = affine.load %C[%x0 + %x1, %x1 + %x2] : memref<16x16xf32>
          %p = arith.mulf %a, %b : f32
          %s = arith.addf %c, %p : f32
          affine.store %s, %C[%x0 + %x1, %x1 + %x2] : memref<16x16xf32>
        }
      }
    }
  }
  return
}

// -----

// once_breaks_radix_syntactically: A[i + 5t]: as an expression not a mixed radix of i and t, on the domain it is i + 10; refused at the checkpoint
// expected: ACCEPT 8x8x8
func.func @once_breaks_radix_syntactically(%A: memref<24x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<24x8xf32> {llvm.noalias}) {
  affine.for %x0 = 0 to 8 {
    affine.for %x1 = 2 to 3 {
      affine.for %x2 = 0 to 8 {
        affine.for %x3 = 0 to 8 {
          %a = affine.load %A[%x0 + %x1 * 5, %x3] : memref<24x8xf32>
          %b = affine.load %B[%x3, %x2] : memref<8x8xf32>
          %c = affine.load %C[%x0 + %x1 * 5, %x2] : memref<24x8xf32>
          %p = arith.mulf %a, %b : f32
          %s = arith.addf %c, %p : f32
          affine.store %s, %C[%x0 + %x1 * 5, %x2] : memref<24x8xf32>
        }
      }
    }
  }
  return
}

// -----

// acc_equal_on_domain: C loaded at [i + t][j] and stored at [i + 3][j] with t = 3: the same element at every iteration (C0); refused at the checkpoint
// expected: ACCEPT 8x8x8
func.func @acc_equal_on_domain(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<16x8xf32> {llvm.noalias}) {
  affine.for %x0 = 0 to 8 {
    affine.for %x1 = 3 to 4 {
      affine.for %x2 = 0 to 8 {
        affine.for %x3 = 0 to 8 {
          %a = affine.load %A[%x0, %x3] : memref<8x8xf32>
          %b = affine.load %B[%x3, %x2] : memref<8x8xf32>
          %c = affine.load %C[%x0 + %x1, %x2] : memref<16x8xf32>
          %p = arith.mulf %a, %b : f32
          %s = arith.addf %c, %p : f32
          affine.store %s, %C[%x0 + 3, %x2] : memref<16x8xf32>
        }
      }
    }
  }
  return
}

// -----

// near_int64_max: IV values 9223372036854775800 .. 9223372036854775806: origins near INT64_MAX, every intermediate representable
// expected: ACCEPT 7x8x8
func.func @near_int64_max(%A: memref<7x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<7x8xf32> {llvm.noalias}) {
  affine.for %x0 = 9223372036854775800 to 9223372036854775807 {
    affine.for %x1 = 0 to 8 {
      affine.for %x2 = 0 to 8 {
        %a = affine.load %A[%x0 - 9223372036854775800, %x2] : memref<7x8xf32>
        %b = affine.load %B[%x2, %x1] : memref<8x8xf32>
        %c = affine.load %C[%x0 - 9223372036854775800, %x1] : memref<7x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%x0 - 9223372036854775800, %x1] : memref<7x8xf32>
      }
    }
  }
  return
}

// -----

// near_int64_min: IV values -9223372036854775807 .. -9223372036854775801
// expected: ACCEPT 7x8x8
func.func @near_int64_min(%A: memref<7x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<7x8xf32> {llvm.noalias}) {
  affine.for %x0 = -9223372036854775807 to -9223372036854775800 {
    affine.for %x1 = 0 to 8 {
      affine.for %x2 = 0 to 8 {
        %a = affine.load %A[%x0 + 9223372036854775807, %x2] : memref<7x8xf32>
        %b = affine.load %B[%x2, %x1] : memref<8x8xf32>
        %c = affine.load %C[%x0 + 9223372036854775807, %x1] : memref<7x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%x0 + 9223372036854775807, %x1] : memref<7x8xf32>
      }
    }
  }
  return
}

// -----

// large_stride: A[2^40 * i][k]: a view stride of 2^40, last element 7 * 2^40, in bounds
// expected: ACCEPT 8x8x8
func.func @large_stride(%A: memref<8796093022209x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %x0 = 0 to 8 {
    affine.for %x1 = 0 to 8 {
      affine.for %x2 = 0 to 8 {
        %a = affine.load %A[%x0 * 1099511627776, %x2] : memref<8796093022209x8xf32>
        %b = affine.load %B[%x2, %x1] : memref<8x8xf32>
        %c = affine.load %C[%x0, %x1] : memref<8x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%x0, %x1] : memref<8x8xf32>
      }
    }
  }
  return
}

// -----

// opposite_directions: A[7 - i] with C[i]: C row i reads A row 7 - i, a negative-stride view (P0.3; refused until 2026-10-15): A row 7 - w (offset 7, stride -1), C row w (whole C)
// expected: ACCEPT 8x8x8
func.func @opposite_directions(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %x0 = 0 to 8 {
    affine.for %x1 = 0 to 8 {
      affine.for %x2 = 0 to 8 {
        %a = affine.load %A[%x0 * -1 + 7, %x2] : memref<8x8xf32>
        %b = affine.load %B[%x2, %x1] : memref<8x8xf32>
        %c = affine.load %C[%x0, %x1] : memref<8x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%x0, %x1] : memref<8x8xf32>
      }
    }
  }
  return
}

// -----

// transposed_a: A[k][i]: a transposed operand, lowered through memref.transpose (P0.2; refused until 2026-10-15)
// expected: ACCEPT 8x8x8
func.func @transposed_a(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %x0 = 0 to 8 {
    affine.for %x1 = 0 to 8 {
      affine.for %x2 = 0 to 8 {
        %a = affine.load %A[%x2, %x0] : memref<8x8xf32>
        %b = affine.load %B[%x2, %x1] : memref<8x8xf32>
        %c = affine.load %C[%x0, %x1] : memref<8x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%x0, %x1] : memref<8x8xf32>
      }
    }
  }
  return
}

// -----

// different_radix_sq1: A[4 i0 + i1] with C[8 i0 + i1]: C's rows are not an arithmetic progression, no strided view (spec question SQ-1)
// expected: REJECT
func.func @different_radix_sq1(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<16x8xf32> {llvm.noalias}) {
  affine.for %x0 = 0 to 2 {
    affine.for %x1 = 0 to 4 {
      affine.for %x2 = 0 to 8 {
        affine.for %x3 = 0 to 8 {
          %a = affine.load %A[%x0 * 4 + %x1, %x3] : memref<8x8xf32>
          %b = affine.load %B[%x3, %x2] : memref<8x8xf32>
          %c = affine.load %C[%x0 * 8 + %x1, %x2] : memref<16x8xf32>
          %p = arith.mulf %a, %b : f32
          %s = arith.addf %c, %p : f32
          affine.store %s, %C[%x0 * 8 + %x1, %x2] : memref<16x8xf32>
        }
      }
    }
  }
  return
}

// -----

// reduction_in_output: C[i][k]: k is read by A, B and C (a batch coordinate) and j by B only: C[i][k] += A[i][k] * (sum over j of B[k][j]), a valid contraction that needs B broadcast (stride 0) -- not a GEMM of views; VALID_BUT_LOWERING_UNSUPPORTED
// expected: REJECT
func.func @reduction_in_output(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %x0 = 0 to 8 {
    affine.for %x1 = 0 to 8 {
      affine.for %x2 = 0 to 8 {
        %a = affine.load %A[%x0, %x2] : memref<8x8xf32>
        %b = affine.load %B[%x2, %x1] : memref<8x8xf32>
        %c = affine.load %C[%x0, %x2] : memref<8x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%x0, %x2] : memref<8x8xf32>
      }
    }
  }
  return
}

// -----

// wrong_coordinate: B[k][i]: B reads I where J belongs -- i is read by A, B and C and j by C only (every j receives the same sum).
// RE-DECIDED 2026-10-19 (docs section 21, the dimension-intersection partitioning): j, read by C only, partitions the
// GEMM with both factors shared -- per (i, j) the 1 x 1 x 8 dot product C[i][j] += sum_k A[i][k] B[k][i]; lowered.
// expected: ACCEPT
func.func @wrong_coordinate(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %x0 = 0 to 8 {
    affine.for %x1 = 0 to 8 {
      affine.for %x2 = 0 to 8 {
        %a = affine.load %A[%x0, %x2] : memref<8x8xf32>
        %b = affine.load %B[%x2, %x0] : memref<8x8xf32>
        %c = affine.load %C[%x0, %x1] : memref<8x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%x0, %x1] : memref<8x8xf32>
      }
    }
  }
  return
}

// -----

// reduction_multiplicity: K = k + u, k in [0, 4), u in [0, 2): K = 1, 2, 3 are each visited twice (a weighted sum, Theorem 3 S2)
// expected: REJECT
func.func @reduction_multiplicity(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %x0 = 0 to 8 {
    affine.for %x1 = 0 to 8 {
      affine.for %x2 = 0 to 4 {
        affine.for %x3 = 0 to 2 {
          %a = affine.load %A[%x0, %x2 + %x3] : memref<8x8xf32>
          %b = affine.load %B[%x2 + %x3, %x1] : memref<8x8xf32>
          %c = affine.load %C[%x0, %x1] : memref<8x8xf32>
          %p = arith.mulf %a, %b : f32
          %s = arith.addf %c, %p : f32
          affine.store %s, %C[%x0, %x1] : memref<8x8xf32>
        }
      }
    }
  }
  return
}

// -----

// coupled: C[i + j][j] with A[i + j][k]. RE-DECIDED 2026-10-16 (docs/AFFINE_GEMM_MAPPING.md
// section 19.3): j is read by all three operands -- a batch loop -- so the positional
// reading I = i + j (whose (I, J) points form a parallelogram, Theorem 3 S1) is not the
// decomposition; per j it is the GEMV C'(I) = C[I + j][j] += A[I + j][K] * B[K][j].
// expected: ACCEPT (batched, 8 x 1 x 8 per j)
func.func @coupled(%A: memref<16x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<16x8xf32> {llvm.noalias}) {
  affine.for %x0 = 0 to 8 {
    affine.for %x1 = 0 to 8 {
      affine.for %x2 = 0 to 8 {
        %a = affine.load %A[%x0 + %x1, %x2] : memref<16x8xf32>
        %b = affine.load %B[%x2, %x1] : memref<8x8xf32>
        %c = affine.load %C[%x0 + %x1, %x1] : memref<16x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%x0 + %x1, %x1] : memref<16x8xf32>
      }
    }
  }
  return
}

// -----

// acc_different_element: C loaded at [i][j], stored at [i][j + 1] (C0)
// expected: REJECT
func.func @acc_different_element(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x9xf32> {llvm.noalias}) {
  affine.for %x0 = 0 to 8 {
    affine.for %x1 = 0 to 8 {
      affine.for %x2 = 0 to 8 {
        %a = affine.load %A[%x0, %x2] : memref<8x8xf32>
        %b = affine.load %B[%x2, %x1] : memref<8x8xf32>
        %c = affine.load %C[%x0, %x1] : memref<8x9xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%x0, %x1 + 1] : memref<8x9xf32>
      }
    }
  }
  return
}

// -----

// broadcast_once: A[t][k], t runs once, with C[i][j]: A reads one row for every I (a broadcast, rho = 0).
// RE-DECIDED 2026-10-19 (docs section 21): i is read by C only -- a partition with both factors shared: per i
// the 1 x 8 x 8 GEMM C[i][:] += A[2][:] B; lowered as 8 partitions.
// expected: ACCEPT
func.func @broadcast_once(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %x0 = 0 to 8 {
    affine.for %x1 = 2 to 3 {
      affine.for %x2 = 0 to 8 {
        affine.for %x3 = 0 to 8 {
          %a = affine.load %A[%x1, %x3] : memref<8x8xf32>
          %b = affine.load %B[%x3, %x2] : memref<8x8xf32>
          %c = affine.load %C[%x0, %x2] : memref<8x8xf32>
          %p = arith.mulf %a, %b : f32
          %s = arith.addf %c, %p : f32
          affine.store %s, %C[%x0, %x2] : memref<8x8xf32>
        }
      }
    }
  }
  return
}

// -----

// oob_static: C[i + 1][j] in an 8x8 C: the last row 8 is out of bounds (undefined program, section 6)
// expected: REJECT
func.func @oob_static(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %x0 = 0 to 8 {
    affine.for %x1 = 0 to 8 {
      affine.for %x2 = 0 to 8 {
        %a = affine.load %A[%x0, %x2] : memref<8x8xf32>
        %b = affine.load %B[%x2, %x1] : memref<8x8xf32>
        %c = affine.load %C[%x0 + 1, %x1] : memref<8x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%x0 + 1, %x1] : memref<8x8xf32>
      }
    }
  }
  return
}

// -----

// oob_negative: A[i - 1][k]: row -1 (undefined program, section 6)
// expected: REJECT
func.func @oob_negative(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %x0 = 0 to 8 {
    affine.for %x1 = 0 to 8 {
      affine.for %x2 = 0 to 8 {
        %a = affine.load %A[%x0 - 1, %x2] : memref<8x8xf32>
        %b = affine.load %B[%x2, %x1] : memref<8x8xf32>
        %c = affine.load %C[%x0, %x1] : memref<8x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%x0, %x1] : memref<8x8xf32>
      }
    }
  }
  return
}

// -----

// empty_loop: a loop of no iteration: an empty GEMM, spec question SQ-2 (refused, its own reason)
// expected: REJECT
func.func @empty_loop(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %i = 5 to 5 {
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
  return
}

// -----

// triangular: j < i: the iteration domain is not a box (Theorem 3 needs one); since 2026-10-18
// (docs section 19.13) the compiler proves it no GEMM: the accesses tell the iterations apart
// (their stacked map has rank 3) and the domain is not centrally symmetric, so it is no affine
// image of a box (PROVEN_INVALID; it was "the loop bounds are not static", UNKNOWN, before)
// expected: REJECT
func.func @triangular(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 8 {
    affine.for %j = 0 to affine_map<(d0) -> (d0)>(%i) {
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
  return
}

// -----

// overflow_view: A[2^62 * i][k], i in [0, 4): the view's last row 3 * 2^62 does not fit in 64 bits (refused, never wrapped)
// expected: REJECT
func.func @overflow_view(%A: memref<?x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<4x8xf32> {llvm.noalias}) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 8 {
      affine.for %k = 0 to 8 {
        %a = affine.load %A[%i * 4611686018427387904, %k] : memref<?x8xf32>
        %b = affine.load %B[%k, %j] : memref<8x8xf32>
        %c = affine.load %C[%i, %j] : memref<4x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[%i, %j] : memref<4x8xf32>
      }
    }
  }
  return
}

// -----

// constant_coordinate: C[0][j] += A[0][k] * B[k][j], K split over two loops: crashed before ISSUE-003; I is driven by no loop: M = 1 (SQ-4, decided 2026-10-15: offloaded)
// expected: ACCEPT 1x8x8
func.func @constant_coordinate(%A: memref<8x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, %C: memref<8x8xf32> {llvm.noalias}) {
  affine.for %k0 = 0 to 2 {
    affine.for %j = 0 to 8 {
      affine.for %k1 = 0 to 4 {
        %a = affine.load %A[0, %k0 * 4 + %k1] : memref<8x8xf32>
        %b = affine.load %B[%k0 * 4 + %k1, %j] : memref<8x8xf32>
        %c = affine.load %C[0, %j] : memref<8x8xf32>
        %p = arith.mulf %a, %b : f32
        %s = arith.addf %c, %p : f32
        affine.store %s, %C[0, %j] : memref<8x8xf32>
      }
    }
  }
  return
}
