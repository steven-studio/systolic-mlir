/*
 * main.c -- numerical check of the compiled affine GEMM (test/e2e/affine_c).
 *
 *   GEMM_KERNEL     the kernel the compiler produced from the affine loops
 *                   (gemm, from gemm.c through the C front end; gemm_systolic,
 *                   from the hand-written fixture):
 *                   systolic.matmul_tile regions -> runtime calls -> LLVM
 *   gemm_reference  gemm.c compiled by clang, run on the CPU
 *
 * Both start from the same A, B and a nonzero C (so the accumulation into C,
 * C += A x B, is checked too). The values are small integers: every product
 * and partial sum is exact in binary32, so the results must be equal element
 * for element whatever the summation order.
 *
 * The runtime calls go to whatever fpga_matmul_tiled_auto_scheduled the
 * binary is linked with: run.sh links soft_runtime.c (BACKEND=soft, a
 * software model, NOT the FPGA) or the real runtime (BACKEND=board,
 * board_runtime.c + libsystolic_fpga_runtime: DPTI to the programmed board).
 * The verdict line names which one ran (e2e_runtime_report.h).
 *
 * Exit: 0 PASS, 1 FAIL (mismatch, or an invocation that failed: refused,
 * or a transport failure on the board).
 */

#include "e2e_runtime_report.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifndef GEMM_M
#define GEMM_M 16
#endif
#ifndef GEMM_N
#define GEMM_N 16
#endif
#ifndef GEMM_K
#define GEMM_K 16
#endif

/* A memref<RxCxf32> argument of the lowered func.func: allocated and aligned
 * pointer, offset, two sizes, two strides. */
#define MEMREF2D(p, rows, cols)                                               \
  (p), (p), (int64_t)0, (int64_t)(rows), (int64_t)(cols), (int64_t)(cols),     \
      (int64_t)1

#ifndef GEMM_KERNEL
#define GEMM_KERNEL gemm_systolic
#endif

void GEMM_KERNEL(float *, float *, int64_t, int64_t, int64_t, int64_t,
                   int64_t, float *, float *, int64_t, int64_t, int64_t,
                   int64_t, int64_t, float *, float *, int64_t, int64_t,
                   int64_t, int64_t, int64_t);

void gemm_reference(float A[GEMM_M][GEMM_K], float B[GEMM_K][GEMM_N],
                    float C[GEMM_M][GEMM_N]);


static float A[GEMM_M][GEMM_K], B[GEMM_K][GEMM_N];
static float C[GEMM_M][GEMM_N], C_ref[GEMM_M][GEMM_N];

int main(void) {
  for (int i = 0; i < GEMM_M; ++i)
    for (int k = 0; k < GEMM_K; ++k)
      A[i][k] = (float)((i * 7 + k * 3) % 9 - 4);
  for (int k = 0; k < GEMM_K; ++k)
    for (int j = 0; j < GEMM_N; ++j)
      B[k][j] = (float)((k * 5 + j * 11) % 7 - 3);
  for (int i = 0; i < GEMM_M; ++i)
    for (int j = 0; j < GEMM_N; ++j)
      C[i][j] = C_ref[i][j] = (float)((i + 2 * j) % 5 - 2);

  GEMM_KERNEL(MEMREF2D(&A[0][0], GEMM_M, GEMM_K),
              MEMREF2D(&B[0][0], GEMM_K, GEMM_N),
              MEMREF2D(&C[0][0], GEMM_M, GEMM_N));
  gemm_reference(A, B, C_ref);

  e2e_runtime_report_t rt;
  e2e_runtime_report(&rt);

  long mismatches = 0;
  int first_i = -1, first_j = -1;
  for (int i = 0; i < GEMM_M; ++i)
    for (int j = 0; j < GEMM_N; ++j)
      if (memcmp(&C[i][j], &C_ref[i][j], sizeof(float)) != 0) {
        if (mismatches++ == 0) {
          first_i = i;
          first_j = j;
        }
      }

  printf("M N K = %d %d %d\n", GEMM_M, GEMM_N, GEMM_K);
  printf("invocations per device (8x8, 4x4_0, 4x4_1, 4x4_2) = %llu %llu %llu "
         "%llu\n",
         rt.per_device[0], rt.per_device[1], rt.per_device[2], rt.per_device[3]);
  printf("runtime backend = %s\n", rt.backend);
  printf("bytes to / from the FPGA = %llu / %llu\n", rt.bytes_to_fpga,
         rt.bytes_from_fpga);
  printf("failed invocations = %llu (first rc = %d)\n", rt.failed,
         rt.first_error);
  printf("mismatching elements = %ld\n", mismatches);
  if (mismatches)
    printf("first mismatch C[%d][%d]: systolic %g, reference %g\n", first_i,
           first_j, C[first_i][first_j], C_ref[first_i][first_j]);

  const int pass = mismatches == 0 && rt.failed == 0;
  printf("Verification (%s): %s\n",
         rt.is_hardware ? "FPGA hardware" : "software runtime model, not FPGA",
         pass ? "PASS" : "FAIL");
  return pass ? 0 : 1;
}
