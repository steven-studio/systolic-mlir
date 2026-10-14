// REQUIRES: cfront-clang
//
// A C program compiled by systolic-cc: the compiler finds the matrix
// multiplications it can run on the 1x8x8 + 3x4x4 fleet, offloads those
// functions, keeps everything else on the host, and the program behaves
// exactly as the same file compiled by clang alone. Software runtime model
// (NOT the FPGA): this is a compiler-correctness test.
//
// RUN: %systolic-cc --verify %s -o %t 2>&1 | FileCheck %s --check-prefix=PLAN
// RUN: %t > %t.out 2> %t.err
// RUN: FileCheck %s --check-prefix=VERIFY --input-file=%t.err
// RUN: %host-cc -std=c11 %s -o %t.cpu
// RUN: %t.cpu > %t.cpu.out
// RUN: diff %t.cpu.out %t.out

#include <stdio.h>

// An ordinary GEMM.
void matmul(float A[restrict 16][16], float B[restrict 16][16],
            float C[restrict 16][16]) {
  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 16; ++j)
      for (int k = 0; k < 16; ++k)
        C[i][j] += A[i][k] * B[k][j];
}

// C = A x B written as one would: C cleared first, loops k, i, j, sizes
// that do not divide the arrays (edge tiles) and K = 40 > k_max (K folds).
// The clearing loop stays a loop; the GEMM nest becomes tiles.
void product(float A[restrict 20][40], float B[restrict 40][12],
             float C[restrict 20][12]) {
  for (int i = 0; i < 20; ++i)
    for (int j = 0; j < 12; ++j)
      C[i][j] = 0.0f;
  for (int k = 0; k < 40; ++k)
    for (int i = 0; i < 20; ++i)
      for (int j = 0; j < 12; ++j)
        C[i][j] += A[i][k] * B[k][j];
}

// In the subset, but no GEMM: stays on the host.
void scale(float x[restrict 16][16], float alpha) {
  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 16; ++j)
      x[i][j] = alpha * x[i][j];
}

// A GEMM in double precision: the FPGA datapath is fp32.
void matmul_f64(double A[restrict 4][4], double B[restrict 4][4],
                double C[restrict 4][4]) {
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j)
      for (int k = 0; k < 4; ++k)
        C[i][j] += A[i][k] * B[k][j];
}

// A GEMM whose operands may overlap (no restrict): not lowered.
void matmul_alias(float A[8][8], float B[8][8], float C[8][8]) {
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 8; ++k)
        C[i][j] += A[i][k] * B[k][j];
}

static float A[20][40], B[40][16], C16[16][16], C20[20][12];
static float A16[16][16], B12[40][12];
static double D[4][4], E[4][4], F[4][4];
static float P[8][8], Q[8][8], R[8][8];

int main(void) {
  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 16; ++j) {
      A16[i][j] = (float)((i * 7 + j * 3) % 9 - 4);
      B[i][j] = (float)((i * 5 + j * 11) % 7 - 3);
      C16[i][j] = (float)((i + 2 * j) % 5 - 2);
    }
  for (int i = 0; i < 20; ++i)
    for (int k = 0; k < 40; ++k)
      A[i][k] = (float)((i * 3 + k) % 7 - 3);
  for (int k = 0; k < 40; ++k)
    for (int j = 0; j < 12; ++j)
      B12[k][j] = (float)((k * 2 + j * 5) % 5 - 2);
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j) {
      D[i][j] = i + j;
      E[i][j] = i - j;
      F[i][j] = 1;
    }
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j) {
      P[i][j] = i;
      Q[i][j] = j;
      R[i][j] = 0;
    }

  matmul((float (*)[16])B, A16, C16);   // A = rows of B: any 16x16 data
  matmul(A16, (float (*)[16])B, C16);   // again, accumulating into C16
  product(A, B12, C20);
  scale(C16, 0.5f);
  matmul_f64(D, E, F);
  matmul_alias(P, Q, R);

  double s16 = 0, s20 = 0, sd = 0, sr = 0;
  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 16; ++j)
      s16 += C16[i][j] * (i + 1) - j;
  for (int i = 0; i < 20; ++i)
    for (int j = 0; j < 12; ++j)
      s20 += C20[i][j] * (j + 1) - i;
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j)
      sd += F[i][j] * (i + 2 * j + 1);
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j)
      sr += R[i][j];
  printf("%.1f %.1f %.1f %.1f %g %g\n", s16, s20, sd, sr, C16[3][5],
         C20[19][11]);
  return 0;
}

// The plan the compiler made, in source order:
//
// PLAN-DAG: program.c:70:1: note: 'main' is outside the supported C subset and is not translated: only functions returning void are supported (got 'int')
// PLAN-DAG: program.c:19:1: note: 'matmul' is offloaded: 10 systolic tile invocation(s)
// PLAN-DAG: program.c:30:1: note: 'product' is offloaded: 27 systolic tile invocation(s)
// PLAN-DAG: program.c:42:1: note: 'scale' stays on the host: no GEMM the systolic arrays can run was recognized in it{{$}}
// PLAN-DAG: program.c:49:1: note: 'matmul_f64' stays on the host: the FPGA datapath is fp32 and it uses double arrays
// PLAN-DAG: program.c:58:1: note: 'matmul_alias' stays on the host: no GEMM the systolic arrays can run was recognized in it (line 59: A may overlap C; declare the arguments llvm.noalias (C restrict) or pass assume-no-alias)
// PLAN:     wrote {{.*}} (2 function(s) offloaded, runtime: software model, NOT the FPGA
//
// Every offloaded call checked against the same function compiled for the
// CPU: two calls of matmul, one of product.
//
// VERIFY:      matmul (program.c:19): verified: 256 elements equal the CPU reference (bitwise); 10 invocations (8x8 2, 4x4 3/3/2) on software runtime model
// VERIFY-NEXT: matmul (program.c:19): verified: 256 elements equal the CPU reference (bitwise); 10 invocations
// VERIFY-NEXT: product (program.c:30): verified: 240 elements equal the CPU reference (bitwise); 27 invocations
// VERIFY-NOT:  MISMATCH
// VERIFY-NOT:  FAILURE
