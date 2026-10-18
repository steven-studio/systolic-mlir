/*
 * gemm.c -- the C program the affine path compiles (test/e2e/affine_c/run.sh).
 *
 * An ordinary triple loop: C += A x B, row-major fp32, the reduction
 * innermost. restrict states what the compiler has to know to replace the
 * loops by tiles: A and B do not overlap C (--affine-matmul-to-systolic
 * refuses the nest otherwise).
 *
 * run.sh compiles this file twice with clang: to LLVM IR (the frontend's
 * first stage, kept as an artifact) and, renamed gemm_reference, into the
 * test binary as the CPU reference the systolic result is checked against.
 */

#ifndef GEMM_M
#define GEMM_M 16
#endif
#ifndef GEMM_N
#define GEMM_N 16
#endif
#ifndef GEMM_K
#define GEMM_K 16
#endif

void gemm(float A[restrict GEMM_M][GEMM_K], float B[restrict GEMM_K][GEMM_N],
          float C[restrict GEMM_M][GEMM_N]) {
  for (int i = 0; i < GEMM_M; ++i)
    for (int j = 0; j < GEMM_N; ++j)
      for (int k = 0; k < GEMM_K; ++k)
        C[i][j] += A[i][k] * B[k][j];
}
