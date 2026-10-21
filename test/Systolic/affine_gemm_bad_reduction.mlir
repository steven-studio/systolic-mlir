// RUN: systolic-opt --test-affine-matmul-discovery %s -o /dev/null 2>&1 \
// RUN:   | FileCheck %s
// RUN: systolic-opt "--test-affine-matmul-discovery=reduction-batches=false" %s -o /dev/null 2>&1 \
// RUN:   | FileCheck %s --check-prefix=OFF
//
// C[i][j] += A[i][k] * B[i][j]. RE-DECIDED 2026-10-21 (docs/AFFINE_GEMM_MAPPING.md section 23,
// reduction-batch partitioning on by default): k is read by the first factor only (not by B,
// not by C), so it becomes a reduction batch loop (RB = {k}); i, read by all three operands, is
// an output batch loop; J = j; there is no I and no contraction loop (M = 1, virtual K = 1). For
// each (i, k), in order, C[i][J] += A[i][k] * B[i][J]: every iteration runs once, every access
// reads the element the program reads, and the k terms accumulate serially into C[i][j]
// starting from its initial value. Verified bitwise against the loop as written (integer-valued
// data, nonzero initial C, sync and async soft runtime, --fp-reassociation); under strict
// floating point the lowering still refuses it. Before: refused, "a coordinate is read by the
// first factor only" (a stride-0 broadcast, VALID_BUT_LOWERING_UNSUPPORTED); without the option
// that is still the result (OFF).
//
// CHECK: unique logical matmul count: 1
// CHECK: recognition result: success
// CHECK: statement: C = C + A * B (accumulation)
// CHECK: reduction batch: loop(s) 2 (not read by C: for each value a GEMM with offset factor views, all accumulated into the same slab of C in order)
// CHECK: dense logical GEMM: yes, I/J/K on loops /1/, batched over loop(s) 0+2 (16 batch iteration(s))
// CHECK: dense extents: M 1, N 4, K 1
// CHECK: logical bijection: verified
//
// OFF-NOT: reduction batch:
// OFF: recognition result: failure
// OFF-NEXT: reason: a coordinate is read by the first factor only
// OFF-NEXT: class: VALID_BUT_LOWERING_UNSUPPORTED
// OFF: unique logical matmul count: 0

func.func @affine_gemm(
    %A: memref<4x4xf32>,
    %B: memref<4x4xf32>,
    %C: memref<4x4xf32>) {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
        %a = affine.load %A[%i, %k] : memref<4x4xf32>
        %b = affine.load %B[%i, %j] : memref<4x4xf32>
        %c = affine.load %C[%i, %j] : memref<4x4xf32>

        %product = arith.mulf %a, %b : f32
        %sum = arith.addf %c, %product : f32

        affine.store %sum, %C[%i, %j] : memref<4x4xf32>
      }
    }
  }

  return
}
