// ISSUE-001: showcase case_065 as the C front end translates it (clang 24,
// `systolic-cfront --function=case_065_kernel showcase/positive/case_065.c`).
//
//   for i, j, k in [0, 8):  C[i + 8][j + 16] += A[i][k] * B[k][j]
//
// A valid GEMM: C[8:16, 16:24] += A x B.  The specification requires it to be
// recognized, partitioned into one exact region and lowered with C's view at
// offset [8, 16].  Commit 6e25528 refused it in recognizeLogicalMatmul
// (matchesGemmAccesses required the same affine expression for a shared
// coordinate in both operands: (d0 + 8) != d0); fixed by matchSharedCoordinate
// (each operand's own view of the shared coordinate), see ISSUE-001.md.  This
// test was the XFAIL reproducer of the issue and is now its permanent
// regression test.
//
// RUN: systolic-opt --systolic-declare-fpga-fleet --test-affine-matmul-discovery %s -o /dev/null 2>&1 \
// RUN:   | FileCheck %s --check-prefix=DISC
// RUN: systolic-opt --systolic-declare-fpga-fleet --affine-matmul-to-systolic=allow-fp-reassociation=true %s \
// RUN:   | FileCheck %s --check-prefix=SYS
//
// DISC: recognition result: success
// DISC: view A: [1*I + 0, 1*K + 0]
// DISC: view B: [1*K + 0, 1*J + 0]
// DISC: view C: [1*I + 8, 1*J + 16]
// DISC: row extent: static 8
// DISC: column extent: static 8
// DISC: reduction extent: static 8
// DISC: dense logical GEMM: yes, I/J/K on loops 0/1/2
// DISC: fleet partition: 1 regions, exact
//
// SYS-LABEL: func.func @case_065_kernel
// SYS-NOT:   affine.for
// SYS:       memref.subview %{{.*}}[8, 16] [8, 8] [1, 1] : memref<?x32xf32>
// SYS:       systolic.matmul_tile {{.*}} m = 8 n = 8 k = 8 on @acc_8x8_0
// SYS:       bufferization.materialize_in_destination

module {
  func.func @case_065_kernel(%A: memref<?x8xf32> {llvm.noalias}, %B: memref<?x8xf32> {llvm.noalias}, %C: memref<?x32xf32> {llvm.noalias}) {
    affine.for %i = 0 to 8 {
      affine.for %j = 0 to 8 {
        affine.for %k = 0 to 8 {
          %t0 = affine.load %C[(%i + 8), (%j + 16)] : memref<?x32xf32>
          %t1 = affine.load %A[%i, %k] : memref<?x8xf32>
          %t2 = affine.load %B[%k, %j] : memref<?x8xf32>
          %t3 = arith.mulf %t1, %t2 : f32
          %t4 = arith.addf %t0, %t3 : f32
          affine.store %t4, %C[(%i + 8), (%j + 16)] : memref<?x32xf32>
        }
      }
    }
    func.return
  }
}
