/*
 * strided_main.c -- numerical check of strided_permuted.mlir (run_strided.sh).
 *
 * The compiled kernel against the same loop nest written in C. Every element
 * of C is compared, including the rows and columns outside the GEMM's
 * footprint, which must be left unchanged. Small integers: exact in binary32
 * in any summation order. The runtime calls go to runtime/offload/soft_runtime.c (a software
 * model, NOT the FPGA) or, with BACKEND=board, the real runtime and board;
 * the verdict line says which (runtime/offload/systolic_offload.h).
 */

#include "systolic_offload.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define MEMREF2D(p, rows, cols)                                               \
  (p), (p), (int64_t)0, (int64_t)(rows), (int64_t)(cols), (int64_t)(cols),     \
      (int64_t)1

void permuted_strided(float *, float *, int64_t, int64_t, int64_t, int64_t,
                      int64_t, float *, float *, int64_t, int64_t, int64_t,
                      int64_t, int64_t, float *, float *, int64_t, int64_t,
                      int64_t, int64_t, int64_t);


static float A[20][4], B[4][5], C[20][5], C_ref[20][5];

int main(void) {
  for (int r = 0; r < 20; ++r)
    for (int c = 0; c < 4; ++c)
      A[r][c] = (float)((r * 3 + c * 5) % 7 - 3);
  for (int r = 0; r < 4; ++r)
    for (int c = 0; c < 5; ++c)
      B[r][c] = (float)((r * 2 + c * 7) % 5 - 2);
  for (int r = 0; r < 20; ++r)
    for (int c = 0; c < 5; ++c)
      C[r][c] = C_ref[r][c] = (float)((r + c) % 3 - 1);

  /* strided_permuted.mlir's loops, in C. */
  for (int k = 0; k < 4; ++k)
    for (int i = 0; i < 6; ++i)
      for (int j = 0; j < 4; ++j)
        C_ref[3 + 2 * i][j + 1] += A[3 + 2 * i][k] * B[k][j + 1];

  permuted_strided(MEMREF2D(&A[0][0], 20, 4), MEMREF2D(&B[0][0], 4, 5),
                   MEMREF2D(&C[0][0], 20, 5));

  systolic_offload_stats_t rt;
  systolic_offload_stats(&rt);
  systolic_offload_close();

  long mismatches = 0;
  for (int r = 0; r < 20; ++r)
    for (int c = 0; c < 5; ++c)
      if (memcmp(&C[r][c], &C_ref[r][c], sizeof(float)) != 0) {
        if (mismatches++ == 0)
          printf("first mismatch C[%d][%d]: systolic %g, reference %g\n", r,
                 c, C[r][c], C_ref[r][c]);
      }

  printf("invocations per device (8x8, 4x4_0, 4x4_1, 4x4_2) = %llu %llu %llu "
         "%llu\n",
         rt.per_device[0], rt.per_device[1], rt.per_device[2], rt.per_device[3]);
  printf("runtime backend = %s\n", rt.backend);
  printf("bytes to / from the FPGA = %llu / %llu\n", rt.bytes_to_fpga,
         rt.bytes_from_fpga);
  printf("failed invocations = %llu (first rc = %d)\n", rt.failed,
         rt.first_error);
  printf("mismatching elements = %ld of 100\n", mismatches);
  const int pass = mismatches == 0 && rt.failed == 0;
  printf("Verification (%s): %s\n",
         rt.is_hardware ? "FPGA hardware" : "software runtime model, not FPGA",
         pass ? "PASS" : "FAIL");
  return pass ? 0 : 1;
}
