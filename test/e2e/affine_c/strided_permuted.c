/*
 * strided_permuted.c -- a GEMM that is not a plain row-major triple loop, in
 * C, for run_strided.sh: the loops in k, i, j order, and the logical
 * matrices embedded in larger arrays -- A and C rows at 3 + 2 * i, B and C
 * columns at 1 + j. strided_permuted.mlir is the same nest written by hand
 * (FRONTEND=fixture); strided_main.c checks every element of C, the rows and
 * columns this function does not touch included, against the same loops
 * compiled by the host C compiler.
 */

void permuted_strided(float A[restrict 20][4], float B[restrict 4][5],
                      float C[restrict 20][5]) {
  for (int k = 0; k < 4; ++k)
    for (int i = 0; i < 6; ++i)
      for (int j = 0; j < 4; ++j)
        C[3 + 2 * i][j + 1] += A[3 + 2 * i][k] * B[k][j + 1];
}
