// Reduction-batch partitioning (docs/AFFINE_GEMM_MAPPING.md section 23, opt-in:
// reduction-batches=true). The loops C does not read become reduction batch
// loops when that makes the nest a GEMM: a loop read by one factor only, and --
// when the loops both factors read are not one dense GEMM coordinate -- all of
// those but the one that stays K (the longest first, ties the innermost). A
// reduction batch
// loop runs around the tiles; for each of its values the factor views are
// offset by it and the tile accumulates into the whole C (to_tensor of C,
// matmul_tile, materialize_in_destination of C inside the loop), so its values
// accumulate in order. The checks pin, in order: the classification (prefix
// DISC), the lowering with the permission to reassociate (REMARK and the IR),
// that nothing is lowered without that permission (STRICT), and that without
// the option the recognition is unchanged (OFF).
//
// RUN: systolic-opt "--test-affine-matmul-discovery=reduction-batches=true" -split-input-file %s \
// RUN:   -o /dev/null 2>&1 | FileCheck %s --check-prefix=DISC
// RUN: systolic-opt "--affine-matmul-to-systolic=report=true allow-fp-reassociation=true classify=true reduction-batches=true" \
// RUN:   -split-input-file %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=REMARK
// RUN: systolic-opt "--affine-matmul-to-systolic=allow-fp-reassociation=true reduction-batches=true" \
// RUN:   -split-input-file %s | FileCheck %s
// RUN: systolic-opt "--affine-matmul-to-systolic=report=true classify=true reduction-batches=true" \
// RUN:   -split-input-file %s -o /dev/null 2>&1 | FileCheck %s --check-prefix=STRICT
// RUN: systolic-opt --test-affine-matmul-discovery -split-input-file %s -o /dev/null 2>&1 \
// RUN:   | FileCheck %s --check-prefix=OFF
// OFF-NOT: reduction batch:
// STRICT-NOT: lowered as

// T1: K = x1 (equal trips: the innermost), x0 a reduction batch loop offsetting both factors
// DISC:          reduction batch: loop(s) 2 (not read by C: for each value a GEMM with offset factor views, all accumulated into the same slab of C in order)
// DISC:          dense logical GEMM: yes, I/J/K on loops 0/1/3, batched over loop(s) 2 (4 batch iteration(s))
// REMARK:      remark: affine GEMM 3x5x4 lowered as 1 exact region(s), 1 reduction batch loop(s) (not read by C, accumulated into C in order), batched: 4 batch iteration(s) of 1 batch loop(s); the enclosing loops run it 4 time(s)
// STRICT:      remark: affine GEMM not lowered to systolic.matmul_tile: floating-po{{.*}}[PROVEN_VALID]
// CHECK-LABEL: func.func @t1(
// CHECK:       affine.for
// CHECK:       bufferization.to_tensor {{.*}} restrict writable
// CHECK-NEXT:  systolic.matmul_tile {{.*}} m = 3 n = 5 k = 4
// CHECK-NEXT:  bufferization.materialize_in_destination {{.*}} in restrict writable
// OFF:           reason: logical coordinate K (the first factor's dimension 1 d2 - d3 + 3, the second factor's dime
// OFF-NEXT:      class: UNKNOWN
func.func @t1(%A: memref<3x7xf32> {llvm.noalias}, %B: memref<7x5xf32> {llvm.noalias}, %C: memref<3x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 3 {
    affine.for %j = 0 to 5 {
      affine.for %x0 = 0 to 4 {
        affine.for %x1 = 0 to 4 {
          %va = affine.load %A[%i, %x0 - %x1 + 3] : memref<3x7xf32>
          %vb = affine.load %B[%x0 + %x1, %j] : memref<7x5xf32>
          %vc = affine.load %C[%i, %j] : memref<3x5xf32>
          %prod = arith.mulf %va, %vb : f32
          %sum = arith.addf %vc, %prod : f32
          affine.store %sum, %C[%i, %j] : memref<3x5xf32>
        }
      }
    }
  }
  return
}
// -----
// T2: K = x0, x1 (read by A only) a reduction batch loop
// DISC:          reduction batch: loop(s) 3 (not read by C: for each value a GEMM with offset factor views, all accumulated into the same slab of C in order)
// DISC:          dense logical GEMM: yes, I/J/K on loops 0/1/2, batched over loop(s) 3 (4 batch iteration(s))
// REMARK:      remark: affine GEMM 3x5x4 lowered as 1 exact region(s), 1 reduction batch loop(s) (not read by C, accumulated into C in order), batched: 4 batch iteration(s) of 1 batch loop(s); the enclosing loops run it 4 time(s)
// STRICT:      remark: affine GEMM not lowered to systolic.matmul_tile: floating-po{{.*}}[PROVEN_VALID]
// CHECK-LABEL: func.func @t2(
// CHECK:       affine.for
// CHECK:       bufferization.to_tensor {{.*}} restrict writable
// CHECK-NEXT:  systolic.matmul_tile {{.*}} m = 3 n = 5 k = 4
// CHECK-NEXT:  bufferization.materialize_in_destination {{.*}} in restrict writable
// OFF:           reason: logical coordinate K (the first factor's dimension 1 d2 - d3 + 3, the second factor's dime
// OFF-NEXT:      class: UNKNOWN
func.func @t2(%A: memref<3x7xf32> {llvm.noalias}, %B: memref<4x5xf32> {llvm.noalias}, %C: memref<3x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 3 {
    affine.for %j = 0 to 5 {
      affine.for %x0 = 0 to 4 {
        affine.for %x1 = 0 to 4 {
          %va = affine.load %A[%i, %x0 - %x1 + 3] : memref<3x7xf32>
          %vb = affine.load %B[%x0, %j] : memref<4x5xf32>
          %vc = affine.load %C[%i, %j] : memref<3x5xf32>
          %prod = arith.mulf %va, %vb : f32
          %sum = arith.addf %vc, %prod : f32
          affine.store %sum, %C[%i, %j] : memref<3x5xf32>
        }
      }
    }
  }
  return
}
// -----
// T5: no loop read by both factors: K = 1, k and l reduction batch loops
// DISC:          reduction batch: loop(s) 2+3 (not read by C: for each value a GEMM with offset factor views, all accumulated into the same slab of C in order)
// DISC:          dense logical GEMM: yes, I/J/K on loops 0/1/, batched over loop(s) 2+3 (16 batch iteration(s))
// REMARK:      remark: affine GEMM 3x5x1 lowered as 1 exact region(s), 2 reduction batch loop(s) (not read by C, accumulated into C in order), batched: 16 batch iteration(s) of 2 batch loop(s); the enclosing loops run it 16 time(s)
// STRICT:      remark: affine GEMM not lowered to systolic.matmul_tile: floating-po{{.*}}[PROVEN_VALID]
// CHECK-LABEL: func.func @t5(
// CHECK:       affine.for
// CHECK:       affine.for
// CHECK:       bufferization.to_tensor {{.*}} restrict writable
// CHECK-NEXT:  systolic.matmul_tile {{.*}} m = 3 n = 5 k = 1
// CHECK-NEXT:  bufferization.materialize_in_destination {{.*}} in restrict writable
// OFF:           reason: a coordinate is read by the first factor only: the other operands would be broadcast along
// OFF-NEXT:      class: VALID_BUT_LOWERING_UNSUPPORTED
func.func @t5(%A: memref<3x4xf32> {llvm.noalias}, %B: memref<4x5xf32> {llvm.noalias}, %C: memref<3x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 3 {
    affine.for %j = 0 to 5 {
      affine.for %k = 0 to 4 {
        affine.for %l = 0 to 4 {
          %va = affine.load %A[%i, %k] : memref<3x4xf32>
          %vb = affine.load %B[%l, %j] : memref<4x5xf32>
          %vc = affine.load %C[%i, %j] : memref<3x5xf32>
          %prod = arith.mulf %va, %vb : f32
          %sum = arith.addf %vc, %prod : f32
          affine.store %sum, %C[%i, %j] : memref<3x5xf32>
        }
      }
    }
  }
  return
}
// -----
// A[i][k0 - k2] * B[k0 + k1][j]: products repeat (k0+1, k1-1, k2+1 reads the same pair); every iteration still runs once
// DISC:          reduction batch: loop(s) 3+4 (not read by C: for each value a GEMM with offset factor views, all accumulated into the same slab of C in order)
// DISC:          dense logical GEMM: yes, I/J/K on loops 0/1/2, batched over loop(s) 3+4 (16 batch iteration(s))
// REMARK:      remark: affine GEMM 3x5x4 lowered as 1 exact region(s), 2 reduction batch loop(s) (not read by C, accumulated into C in order), batched: 16 batch iteration(s) of 2 batch loop(s); the enclosing loops run it 16 time(s)
// STRICT:      remark: affine GEMM not lowered to systolic.matmul_tile: floating-po{{.*}}[PROVEN_VALID]
// CHECK-LABEL: func.func @case1(
// CHECK:       affine.for
// CHECK:       affine.for
// CHECK:       bufferization.to_tensor {{.*}} restrict writable
// CHECK-NEXT:  systolic.matmul_tile {{.*}} m = 3 n = 5 k = 4
// CHECK-NEXT:  bufferization.materialize_in_destination {{.*}} in restrict writable
// OFF:           reason: logical coordinate K (the first factor's dimension 1 d2 - d4 + 3, the second factor's dime
// OFF-NEXT:      class: UNKNOWN
func.func @case1(%A: memref<3x7xf32> {llvm.noalias}, %B: memref<7x5xf32> {llvm.noalias}, %C: memref<3x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 3 {
    affine.for %j = 0 to 5 {
      affine.for %k0 = 0 to 4 {
        affine.for %k1 = 0 to 4 {
          affine.for %k2 = 0 to 4 {
            %va = affine.load %A[%i, %k0 - %k2 + 3] : memref<3x7xf32>
            %vb = affine.load %B[%k0 + %k1, %j] : memref<7x5xf32>
            %vc = affine.load %C[%i, %j] : memref<3x5xf32>
            %prod = arith.mulf %va, %vb : f32
            %sum = arith.addf %vc, %prod : f32
            affine.store %sum, %C[%i, %j] : memref<3x5xf32>
          }
        }
      }
    }
  }
  return
}
// -----
// A[i][k0][k1] * B[k0 + k1][j]: K = k1, k0 a reduction batch loop
// DISC:          reduction batch: loop(s) 2 (not read by C: for each value a GEMM with offset factor views, all accumulated into the same slab of C in order)
// DISC:          dense logical GEMM: yes, I/J/K on loops 0/1/3, batched over loop(s) 2 (4 batch iteration(s))
// REMARK:      remark: affine GEMM 3x5x4 lowered as 1 exact region(s), 1 reduction batch loop(s) (not read by C, accumulated into C in order), batched: 4 batch iteration(s) of 1 batch loop(s); the enclosing loops run it 4 time(s)
// STRICT:      remark: affine GEMM not lowered to systolic.matmul_tile: floating-po{{.*}}[PROVEN_VALID]
// CHECK-LABEL: func.func @case2(
// CHECK:       affine.for
// CHECK:       bufferization.to_tensor {{.*}} restrict writable
// CHECK-NEXT:  systolic.matmul_tile {{.*}} m = 3 n = 5 k = 4
// CHECK-NEXT:  bufferization.materialize_in_destination {{.*}} in restrict writable
// OFF:           reason: the first factor reads logical coordinate K through two different combinations of its loop
// OFF-NEXT:      class: VALID_BUT_LOWERING_UNSUPPORTED
func.func @case2(%A: memref<3x4x4xf32> {llvm.noalias}, %B: memref<7x5xf32> {llvm.noalias}, %C: memref<3x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 3 {
    affine.for %j = 0 to 5 {
      affine.for %k0 = 0 to 4 {
        affine.for %k1 = 0 to 4 {
          %va = affine.load %A[%i, %k0, %k1] : memref<3x4x4xf32>
          %vb = affine.load %B[%k0 + %k1, %j] : memref<7x5xf32>
          %vc = affine.load %C[%i, %j] : memref<3x5xf32>
          %prod = arith.mulf %va, %vb : f32
          %sum = arith.addf %vc, %prod : f32
          affine.store %sum, %C[%i, %j] : memref<3x5xf32>
        }
      }
    }
  }
  return
}
// -----
// A[i][k0 + 2k1] * B[k0 + k1][j]: K = k1 (A stride 2, B stride 1), k0 a reduction batch loop
// DISC:          reduction batch: loop(s) 2 (not read by C: for each value a GEMM with offset factor views, all accumulated into the same slab of C in order)
// DISC:          dense logical GEMM: yes, I/J/K on loops 0/1/3, batched over loop(s) 2 (4 batch iteration(s))
// REMARK:      remark: affine GEMM 3x5x4 lowered as 1 exact region(s), 1 reduction batch loop(s) (not read by C, accumulated into C in order), batched: 4 batch iteration(s) of 1 batch loop(s); the enclosing loops run it 4 time(s)
// STRICT:      remark: affine GEMM not lowered to systolic.matmul_tile: floating-po{{.*}}[PROVEN_VALID]
// CHECK-LABEL: func.func @coupled(
// CHECK:       affine.for
// CHECK:       bufferization.to_tensor {{.*}} restrict writable
// CHECK-NEXT:  systolic.matmul_tile {{.*}} m = 3 n = 5 k = 4
// CHECK-NEXT:  bufferization.materialize_in_destination {{.*}} in restrict writable
// OFF:           reason: logical coordinate K (the first factor's dimension 1 d2 + d3 * 2, the second factor's dime
// OFF-NEXT:      class: UNKNOWN
func.func @coupled(%A: memref<3x10xf32> {llvm.noalias}, %B: memref<7x5xf32> {llvm.noalias}, %C: memref<3x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 3 {
    affine.for %j = 0 to 5 {
      affine.for %k0 = 0 to 4 {
        affine.for %k1 = 0 to 4 {
          %va = affine.load %A[%i, %k0 + 2 * %k1] : memref<3x10xf32>
          %vb = affine.load %B[%k0 + %k1, %j] : memref<7x5xf32>
          %vc = affine.load %C[%i, %j] : memref<3x5xf32>
          %prod = arith.mulf %va, %vb : f32
          %sum = arith.addf %vc, %prod : f32
          affine.store %sum, %C[%i, %j] : memref<3x5xf32>
        }
      }
    }
  }
  return
}
// -----
// A[i][k0 + k1] * B[k0 + k1][j]: k0 + k1 is no dense K; K = k1, k0 a reduction batch loop
// DISC:          reduction batch: loop(s) 2 (not read by C: for each value a GEMM with offset factor views, all accumulated into the same slab of C in order)
// DISC:          dense logical GEMM: yes, I/J/K on loops 0/1/3, batched over loop(s) 2 (4 batch iteration(s))
// REMARK:      remark: affine GEMM 3x5x4 lowered as 1 exact region(s), 1 reduction batch loop(s) (not read by C, accumulated into C in order), batched: 4 batch iteration(s) of 1 batch loop(s); the enclosing loops run it 4 time(s)
// STRICT:      remark: affine GEMM not lowered to systolic.matmul_tile: floating-po{{.*}}[PROVEN_VALID]
// CHECK-LABEL: func.func @noninj(
// CHECK:       affine.for
// CHECK:       bufferization.to_tensor {{.*}} restrict writable
// CHECK-NEXT:  systolic.matmul_tile {{.*}} m = 3 n = 5 k = 4
// CHECK-NEXT:  bufferization.materialize_in_destination {{.*}} in restrict writable
func.func @noninj(%A: memref<3x7xf32> {llvm.noalias}, %B: memref<7x5xf32> {llvm.noalias}, %C: memref<3x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 3 {
    affine.for %j = 0 to 5 {
      affine.for %k0 = 0 to 4 {
        affine.for %k1 = 0 to 4 {
          %va = affine.load %A[%i, %k0 + %k1] : memref<3x7xf32>
          %vb = affine.load %B[%k0 + %k1, %j] : memref<7x5xf32>
          %vc = affine.load %C[%i, %j] : memref<3x5xf32>
          %prod = arith.mulf %va, %vb : f32
          %sum = arith.addf %vc, %prod : f32
          affine.store %sum, %C[%i, %j] : memref<3x5xf32>
        }
      }
    }
  }
  return
}
// -----
// two loops read by both factors: K = k (the longer), l a reduction batch loop
// DISC:          reduction batch: loop(s) 3 (not read by C: for each value a GEMM with offset factor views, all accumulated into the same slab of C in order)
// DISC:          dense logical GEMM: yes, I/J/K on loops 0/1/2, batched over loop(s) 3 (2 batch iteration(s))
// REMARK:      remark: affine GEMM 3x5x4 lowered as 1 exact region(s), 1 reduction batch loop(s) (not read by C, accumulated into C in order), batched: 2 batch iteration(s) of 1 batch loop(s); the enclosing loops run it 2 time(s)
// STRICT:      remark: affine GEMM not lowered to systolic.matmul_tile: floating-po{{.*}}[PROVEN_VALID]
// CHECK-LABEL: func.func @two_k(
// CHECK:       affine.for
// CHECK:       bufferization.to_tensor {{.*}} restrict writable
// CHECK-NEXT:  systolic.matmul_tile {{.*}} m = 3 n = 5 k = 4
// CHECK-NEXT:  bufferization.materialize_in_destination {{.*}} in restrict writable
func.func @two_k(%A: memref<3x4x2xf32> {llvm.noalias}, %B: memref<4x2x5xf32> {llvm.noalias}, %C: memref<3x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 3 {
    affine.for %j = 0 to 5 {
      affine.for %k = 0 to 4 {
        affine.for %l = 0 to 2 {
          %va = affine.load %A[%i, %k, %l] : memref<3x4x2xf32>
          %vb = affine.load %B[%k, %l, %j] : memref<4x2x5xf32>
          %vc = affine.load %C[%i, %j] : memref<3x5xf32>
          %prod = arith.mulf %va, %vb : f32
          %sum = arith.addf %vc, %prod : f32
          affine.store %sum, %C[%i, %j] : memref<3x5xf32>
        }
      }
    }
  }
  return
}
// -----
// C[i][j] += A[r][i][k] * B[r][k][j]: K = k, r a reduction batch loop
// DISC:          reduction batch: loop(s) 0 (not read by C: for each value a GEMM with offset factor views, all accumulated into the same slab of C in order)
// DISC:          dense logical GEMM: yes, I/J/K on loops 1/2/3, batched over loop(s) 0 (2 batch iteration(s))
// REMARK:      remark: affine GEMM 3x5x4 lowered as 1 exact region(s), 1 reduction batch loop(s) (not read by C, accumulated into C in order), batched: 2 batch iteration(s) of 1 batch loop(s); the enclosing loops run it 2 time(s)
// STRICT:      remark: affine GEMM not lowered to systolic.matmul_tile: floating-po{{.*}}[PROVEN_VALID]
// CHECK-LABEL: func.func @partial(
// CHECK:       affine.for
// CHECK:       bufferization.to_tensor {{.*}} restrict writable
// CHECK-NEXT:  systolic.matmul_tile {{.*}} m = 3 n = 5 k = 4
// CHECK-NEXT:  bufferization.materialize_in_destination {{.*}} in restrict writable
func.func @partial(%A: memref<2x3x4xf32> {llvm.noalias}, %B: memref<2x4x5xf32> {llvm.noalias}, %C: memref<3x5xf32> {llvm.noalias}) {
  affine.for %r = 0 to 2 {
    affine.for %i = 0 to 3 {
      affine.for %j = 0 to 5 {
        affine.for %k = 0 to 4 {
          %va = affine.load %A[%r, %i, %k] : memref<2x3x4xf32>
          %vb = affine.load %B[%r, %k, %j] : memref<2x4x5xf32>
          %vc = affine.load %C[%i, %j] : memref<3x5xf32>
          %prod = arith.mulf %va, %vb : f32
          %sum = arith.addf %vc, %prod : f32
          affine.store %sum, %C[%i, %j] : memref<3x5xf32>
        }
      }
    }
  }
  return
}
// -----
// a loop read by A only (s): a reduction batch loop
// DISC:          reduction batch: loop(s) 2 (not read by C: for each value a GEMM with offset factor views, all accumulated into the same slab of C in order)
// DISC:          dense logical GEMM: yes, I/J/K on loops 0/1/3, batched over loop(s) 2 (2 batch iteration(s))
// REMARK:      remark: affine GEMM 3x5x4 lowered as 1 exact region(s), 1 reduction batch loop(s) (not read by C, accumulated into C in order), batched: 2 batch iteration(s) of 1 batch loop(s); the enclosing loops run it 2 time(s)
// STRICT:      remark: affine GEMM not lowered to systolic.matmul_tile: floating-po{{.*}}[PROVEN_VALID]
// CHECK-LABEL: func.func @a_only(
// CHECK:       affine.for
// CHECK:       bufferization.to_tensor {{.*}} restrict writable
// CHECK-NEXT:  systolic.matmul_tile {{.*}} m = 3 n = 5 k = 4
// CHECK-NEXT:  bufferization.materialize_in_destination {{.*}} in restrict writable
func.func @a_only(%A: memref<3x2x4xf32> {llvm.noalias}, %B: memref<4x5xf32> {llvm.noalias}, %C: memref<3x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 3 {
    affine.for %j = 0 to 5 {
      affine.for %s = 0 to 2 {
        affine.for %k = 0 to 4 {
          %va = affine.load %A[%i, %s, %k] : memref<3x2x4xf32>
          %vb = affine.load %B[%k, %j] : memref<4x5xf32>
          %vc = affine.load %C[%i, %j] : memref<3x5xf32>
          %prod = arith.mulf %va, %vb : f32
          %sum = arith.addf %vc, %prod : f32
          affine.store %sum, %C[%i, %j] : memref<3x5xf32>
        }
      }
    }
  }
  return
}
// -----
// a loop read by B only (l): a reduction batch loop
// DISC:          reduction batch: loop(s) 3 (not read by C: for each value a GEMM with offset factor views, all accumulated into the same slab of C in order)
// DISC:          dense logical GEMM: yes, I/J/K on loops 0/1/2, batched over loop(s) 3 (3 batch iteration(s))
// REMARK:      remark: affine GEMM 3x5x4 lowered as 1 exact region(s), 1 reduction batch loop(s) (not read by C, accumulated into C in order), batched: 3 batch iteration(s) of 1 batch loop(s); the enclosing loops run it 3 time(s)
// STRICT:      remark: affine GEMM not lowered to systolic.matmul_tile: floating-po{{.*}}[PROVEN_VALID]
// CHECK-LABEL: func.func @b_only(
// CHECK:       affine.for
// CHECK:       bufferization.to_tensor {{.*}} restrict writable
// CHECK-NEXT:  systolic.matmul_tile {{.*}} m = 3 n = 5 k = 4
// CHECK-NEXT:  bufferization.materialize_in_destination {{.*}} in restrict writable
func.func @b_only(%A: memref<3x4xf32> {llvm.noalias}, %B: memref<4x3x5xf32> {llvm.noalias}, %C: memref<3x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 3 {
    affine.for %j = 0 to 5 {
      affine.for %k = 0 to 4 {
        affine.for %l = 0 to 3 {
          %va = affine.load %A[%i, %k] : memref<3x4xf32>
          %vb = affine.load %B[%k, %l, %j] : memref<4x3x5xf32>
          %vc = affine.load %C[%i, %j] : memref<3x5xf32>
          %prod = arith.mulf %va, %vb : f32
          %sum = arith.addf %vc, %prod : f32
          affine.store %sum, %C[%i, %j] : memref<3x5xf32>
        }
      }
    }
  }
  return
}
// -----
// an output batch loop (b, read by all three) and reduction batch loops (k, l)
// DISC:          reduction batch: loop(s) 3+4 (not read by C: for each value a GEMM with offset factor views, all accumulated into the same slab of C in order)
// DISC:          dense logical GEMM: yes, I/J/K on loops 1/2/, batched over loop(s) 0+3+4 (32 batch iteration(s))
// REMARK:      remark: affine GEMM 3x5x1 lowered as 1 exact region(s), 2 reduction batch loop(s) (not read by C, accumulated into C in order), batched: 32 batch iteration(s) of 3 batch loop(s); the enclosing loops run it 32 time(s)
// STRICT:      remark: affine GEMM not lowered to systolic.matmul_tile: floating-po{{.*}}[PROVEN_VALID]
// CHECK-LABEL: func.func @ob_rb(
// CHECK:       affine.for
// CHECK:       affine.for
// CHECK:       affine.for
// CHECK:       bufferization.to_tensor {{.*}} restrict writable
// CHECK-NEXT:  systolic.matmul_tile {{.*}} m = 3 n = 5 k = 1
// CHECK-NEXT:  bufferization.materialize_in_destination {{.*}} in restrict writable
func.func @ob_rb(%A: memref<2x3x4xf32> {llvm.noalias}, %B: memref<2x4x5xf32> {llvm.noalias}, %C: memref<2x3x5xf32> {llvm.noalias}) {
  affine.for %b = 0 to 2 {
    affine.for %i = 0 to 3 {
      affine.for %j = 0 to 5 {
        affine.for %k = 0 to 4 {
          affine.for %l = 0 to 4 {
            %va = affine.load %A[%b, %i, %k] : memref<2x3x4xf32>
            %vb = affine.load %B[%b, %l, %j] : memref<2x4x5xf32>
            %vc = affine.load %C[%b, %i, %j] : memref<2x3x5xf32>
            %prod = arith.mulf %va, %vb : f32
            %sum = arith.addf %vc, %prod : f32
            affine.store %sum, %C[%b, %i, %j] : memref<2x3x5xf32>
          }
        }
      }
    }
  }
  return
}
// -----
// a negative coefficient on a reduction batch loop: A[i][3 - k][l] * B[l][j]
// DISC:          reduction batch: loop(s) 2 (not read by C: for each value a GEMM with offset factor views, all accumulated into the same slab of C in order)
// DISC:          dense logical GEMM: yes, I/J/K on loops 0/1/3, batched over loop(s) 2 (4 batch iteration(s))
// REMARK:      remark: affine GEMM 3x5x2 lowered as 1 exact region(s), 1 reduction batch loop(s) (not read by C, accumulated into C in order), batched: 4 batch iteration(s) of 1 batch loop(s); the enclosing loops run it 4 time(s)
// STRICT:      remark: affine GEMM not lowered to systolic.matmul_tile: floating-po{{.*}}[PROVEN_VALID]
// CHECK-LABEL: func.func @rb_negative(
// CHECK:       affine.for
// CHECK:       bufferization.to_tensor {{.*}} restrict writable
// CHECK-NEXT:  systolic.matmul_tile {{.*}} m = 3 n = 5 k = 2
// CHECK-NEXT:  bufferization.materialize_in_destination {{.*}} in restrict writable
func.func @rb_negative(%A: memref<3x4x2xf32> {llvm.noalias}, %B: memref<2x5xf32> {llvm.noalias}, %C: memref<3x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 3 {
    affine.for %j = 0 to 5 {
      affine.for %k = 0 to 4 {
        affine.for %l = 0 to 2 {
          %va = affine.load %A[%i, 3 - %k, %l] : memref<3x4x2xf32>
          %vb = affine.load %B[%l, %j] : memref<2x5xf32>
          %vc = affine.load %C[%i, %j] : memref<3x5xf32>
          %prod = arith.mulf %va, %vb : f32
          %sum = arith.addf %vc, %prod : f32
          affine.store %sum, %C[%i, %j] : memref<3x5xf32>
        }
      }
    }
  }
  return
}
// -----
// affine constants in every subscript: C[i + 1][j] += A[i + 1][k + 2] * B[l + 1][j + 1]
// DISC:          reduction batch: loop(s) 2+3 (not read by C: for each value a GEMM with offset factor views, all accumulated into the same slab of C in order)
// DISC:          dense logical GEMM: yes, I/J/K on loops 0/1/, batched over loop(s) 2+3 (16 batch iteration(s))
// REMARK:      remark: affine GEMM 3x5x1 lowered as 1 exact region(s), 2 reduction batch loop(s) (not read by C, accumulated into C in order), batched: 16 batch iteration(s) of 2 batch loop(s); the enclosing loops run it 16 time(s)
// STRICT:      remark: affine GEMM not lowered to systolic.matmul_tile: floating-po{{.*}}[PROVEN_VALID]
// CHECK-LABEL: func.func @rb_constants(
// CHECK:       affine.for
// CHECK:       affine.for
// CHECK:       bufferization.to_tensor {{.*}} restrict writable
// CHECK-NEXT:  systolic.matmul_tile {{.*}} m = 3 n = 5 k = 1
// CHECK-NEXT:  bufferization.materialize_in_destination {{.*}} in restrict writable
func.func @rb_constants(%A: memref<4x6xf32> {llvm.noalias}, %B: memref<5x6xf32> {llvm.noalias}, %C: memref<4x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 3 {
    affine.for %j = 0 to 5 {
      affine.for %k = 0 to 4 {
        affine.for %l = 0 to 4 {
          %va = affine.load %A[%i + 1, %k + 2] : memref<4x6xf32>
          %vb = affine.load %B[%l + 1, %j + 1] : memref<5x6xf32>
          %vc = affine.load %C[%i + 1, %j] : memref<4x5xf32>
          %prod = arith.mulf %va, %vb : f32
          %sum = arith.addf %vc, %prod : f32
          affine.store %sum, %C[%i + 1, %j] : memref<4x5xf32>
        }
      }
    }
  }
  return
}
// -----
// the assignment twin of T2 (C = A * B): x1 not read by C, only the last product survives -- stays on the host
// DISC:          reason: the statement overwrites C (C = A * B, no accumulation) and loop 2 of the nest, which runs
// DISC-NEXT:     class: PROVEN_INVALID
// CHECK-LABEL: func.func @t2_assign(
// CHECK-NOT:   systolic.matmul_tile
func.func @t2_assign(%A: memref<3x7xf32> {llvm.noalias}, %B: memref<4x5xf32> {llvm.noalias}, %C: memref<3x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 3 {
    affine.for %j = 0 to 5 {
      affine.for %x0 = 0 to 4 {
        affine.for %x1 = 0 to 4 {
          %va = affine.load %A[%i, %x0 - %x1 + 3] : memref<3x7xf32>
          %vb = affine.load %B[%x0, %j] : memref<4x5xf32>
          %prod = arith.mulf %va, %vb : f32
          affine.store %prod, %C[%i, %j] : memref<3x5xf32>
        }
      }
    }
  }
  return
}
// -----
// a plain GEMM: unchanged with the option (no reduction batch loop)
// DISC:          dense logical GEMM: yes, I/J/K on loops 0/1/2
// REMARK:      remark: affine GEMM 3x5x4 lowered as 1 exact region(s)
// STRICT:      remark: affine GEMM not lowered to systolic.matmul_tile: floating-po{{.*}}[PROVEN_VALID]
// CHECK-LABEL: func.func @gemm_unchanged(
// CHECK:       bufferization.to_tensor {{.*}} restrict writable
// CHECK-NEXT:  systolic.matmul_tile {{.*}} m = 3 n = 5 k = 4
// CHECK-NEXT:  bufferization.materialize_in_destination {{.*}} in restrict writable
func.func @gemm_unchanged(%A: memref<3x4xf32> {llvm.noalias}, %B: memref<4x5xf32> {llvm.noalias}, %C: memref<3x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 3 {
    affine.for %j = 0 to 5 {
      affine.for %k = 0 to 4 {
        %va = affine.load %A[%i, %k] : memref<3x4xf32>
        %vb = affine.load %B[%k, %j] : memref<4x5xf32>
        %vc = affine.load %C[%i, %j] : memref<3x5xf32>
        %prod = arith.mulf %va, %vb : f32
        %sum = arith.addf %vc, %prod : f32
        affine.store %sum, %C[%i, %j] : memref<3x5xf32>
      }
    }
  }
  return
}
// -----
// T3: unchanged with the option (k and 2k are one K direction)
// DISC:          dense logical GEMM: yes, I/J/K on loops 0/1/2
// REMARK:      remark: affine GEMM 3x5x4 lowered as 1 exact region(s)
// STRICT:      remark: affine GEMM not lowered to systolic.matmul_tile: floating-po{{.*}}[PROVEN_VALID]
// CHECK-LABEL: func.func @t3_unchanged(
// CHECK:       bufferization.to_tensor {{.*}} restrict writable
// CHECK-NEXT:  systolic.matmul_tile {{.*}} m = 3 n = 5 k = 4
// CHECK-NEXT:  bufferization.materialize_in_destination {{.*}} in restrict writable
func.func @t3_unchanged(%A: memref<3x3x4x8xf32> {llvm.noalias}, %B: memref<4x5x10xf32> {llvm.noalias}, %C: memref<3x5xf32> {llvm.noalias}) {
  affine.for %i = 0 to 3 {
    affine.for %j = 0 to 5 {
      affine.for %k = 0 to 4 {
        %va = affine.load %A[%i, %i, %k, 2 * %k] : memref<3x3x4x8xf32>
        %vb = affine.load %B[%k, %j, 2 * %j] : memref<4x5x10xf32>
        %vc = affine.load %C[%i, %j] : memref<3x5xf32>
        %prod = arith.mulf %va, %vb : f32
        %sum = arith.addf %vc, %prod : f32
        affine.store %sum, %C[%i, %j] : memref<3x5xf32>
      }
    }
  }
  return
}
