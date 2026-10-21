// gemm_example.c -- the smallest C program that runs a GEMM through
// systolic-cc on the FPGA (or its RTL co-simulation) and checks the result.
//
// Plain C11, standard headers only: systolic-cc recognizes the loop nest in
// gemm() and offloads that function; everything else, including the CPU
// reference below, is compiled by clang as written.
//
//   bash scripts/reproducibility/run_cosim.sh c test/reproducibility/gemm_example.c
//   bash scripts/reproducibility/run_board.sh c test/reproducibility/gemm_example.c
//
// What the offload needs from the source (docs/AFFINE_GEMM_MAPPING.md):
//   - float (fp32) operands -- the FPGA datapath is binary32;
//   - restrict on every array parameter -- operands that may overlap are not
//     lowered;
//   - the plain C[i][j] += A[i][k] * B[k][j] nest over constant bounds;
//   - systolic-cc --fp-reassociation: offloading reassociates the K sum, and
//     the default (strict FP) keeps every GEMM on the host.
//
// M = 24, N = 20, K = 40 do not divide the 8x8 / 4x4 arrays (edge tiles) and
// K exceeds the bitstream's K_MAX of 16 (K is folded into chained
// invocations). Small integer values keep every partial sum exact in fp32,
// so any reassociation gives the same bits; the comparison still allows a
// relative tolerance for programs that change the data.
//
// Output: the driver's "[systolic-cc] FPGA Executed: GEMM #1" for gemm()
// ("CPU Executed" when it ran on the host: the software model, or a CPU
// fallback), "[systolic-cc] CPU Executed: GEMM #2" for the reference, then
// PASS or FAIL. Exit status 0 = PASS, 1 = FAIL.
//
// lit runs it on the software model of the runtime (NOT the FPGA): a check
// that the example compiles, is offloaded and computes the right result.
//
// REQUIRES: cfront-clang
// RUN: %systolic-cc --fp-reassociation --explain %s -o %t 2>&1 | FileCheck %s --check-prefix=PLAN
// PLAN: note: 'gemm' is offloaded: 54 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet
// RUN: %t | FileCheck %s
// CHECK: [systolic-cc] CPU Executed: GEMM #1
// CHECK-NEXT: [systolic-cc] CPU Executed: GEMM #2
// CHECK-NEXT: gemm 24x20x40: 0 of 480 elements wrong, max |error| 0,
// CHECK-NEXT: PASS

#include <math.h>
#include <stdio.h>

#define M 24
#define N 20
#define K 40

// The GEMM systolic-cc offloads: C = A x B, C cleared first.
void gemm(float A[restrict M][K], float B[restrict K][N],
          float C[restrict M][N]) {
  for (int i = 0; i < M; ++i)
    for (int j = 0; j < N; ++j)
      C[i][j] = 0.0f;
  for (int i = 0; i < M; ++i)
    for (int j = 0; j < N; ++j)
      for (int k = 0; k < K; ++k)
        C[i][j] += A[i][k] * B[k][j];
}

// CPU reference: a local double accumulator, outside the subset systolic-cc
// translates, so it always runs on the CPU (the driver still reports it as
// GEMM #2, "CPU Executed").
static void gemm_reference(float A[M][K], float B[K][N], double R[M][N]) {
  for (int i = 0; i < M; ++i)
    for (int j = 0; j < N; ++j) {
      double s = 0.0;
      for (int k = 0; k < K; ++k)
        s += (double)A[i][k] * (double)B[k][j];
      R[i][j] = s;
    }
}

static float A[M][K], B[K][N], C[M][N];
static double R[M][N];

int main(void) {
  for (int i = 0; i < M; ++i)
    for (int k = 0; k < K; ++k)
      A[i][k] = (float)((i * 3 + k * 7) % 11 - 5);
  for (int k = 0; k < K; ++k)
    for (int j = 0; j < N; ++j)
      B[k][j] = (float)((k * 5 + j * 2) % 9 - 4);
  for (int i = 0; i < M; ++i)
    for (int j = 0; j < N; ++j)
      C[i][j] = 1234.0f;   // overwritten by gemm(): stale data must not survive

  gemm(A, B, C);
  gemm_reference(A, B, R);

  int bad = 0;
  double max_err = 0.0;
  for (int i = 0; i < M; ++i)
    for (int j = 0; j < N; ++j) {
      double err = fabs((double)C[i][j] - R[i][j]);
      double tol = 1e-5 * fmax(1.0, fabs(R[i][j]));
      if (err > max_err)
        max_err = err;
      if (err > tol) {
        if (bad < 5)
          printf("  C[%d][%d] = %.6g, reference %.6g\n", i, j, C[i][j],
                 R[i][j]);
        ++bad;
      }
    }
  double checksum = 0.0;
  for (int i = 0; i < M; ++i)
    for (int j = 0; j < N; ++j)
      checksum += R[i][j] * (i + 1) - j;
  printf("gemm %dx%dx%d: %d of %d elements wrong, max |error| %.3g, "
         "reference checksum %.1f\n", M, N, K, bad, M * N, max_err, checksum);
  printf("%s\n", bad ? "FAIL" : "PASS");
  return bad ? 1 : 0;
}
