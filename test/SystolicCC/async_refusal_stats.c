// REQUIRES: cfront-clang
//
// A tile fpga_tile_submit refuses (here ldc < n) is counted in the session
// statistics (systolic_offload_stats: failed, first_error) as the
// synchronous entry point counts a refused call -- the compiled kernels
// ignore the return code, and the caller-side shim and board_stats.c read
// the statistics -- without changing the GEMM's own result: its wait still
// reports only its accepted tiles (0 here). A synchronous call refused the
// same way is counted once. On the software runtime model (NOT the FPGA;
// runtime-async-test mode H checks the real runtime's counts).
//
// RUN: %host-cc -std=c11 -I%S/../../runtime -I%S/../../runtime/offload %s %S/../../runtime/offload/soft_runtime.c -o %t
// RUN: env SOFT_K_MAX_8X8=16 SOFT_K_MAX_4X4=16 %t | FileCheck %s

#include "fpga_async_runtime.h"
#include "systolic_offload.h"

#include <stdio.h>

int fpga_matmul_tiled_auto_scheduled(int M, int K, int N, const float *A,
                                     const float *B, float *C,
                                     unsigned device_id, unsigned start_cycle,
                                     unsigned est_cycles);

static float a[64], b[64], c[64];

static unsigned long long invocations(const systolic_offload_stats_t *s) {
  return s->per_device[0] + s->per_device[1] + s->per_device[2] + s->per_device[3];
}

int main(void) {
  for (int i = 0; i < 64; ++i)
    a[i] = b[i] = 1.0f;
  systolic_offload_stats_t s0, s1, s2;
  systolic_offload_stats(&s0);

  fpga_gemm_t *h = fpga_gemm_begin(4, 8, 1, 2);
  printf("ldc 2 < n 4: %d\n", fpga_tile_submit(h, 0, 0, 1, 4, 1, 4, a, b, c, 2, 1, 0, 101));
  printf("accepted: %d\n", fpga_tile_submit(h, 1, 0, 1, 4, 1, 4, a, b, c + 4, 8, 1, 0, 101));
  printf("wait: %d\n", fpga_gemm_wait(h));
  systolic_offload_stats(&s1);
  printf("failed %llu first_error %d invocations %llu\n", s1.failed - s0.failed,
         s1.first_error, invocations(&s1) - invocations(&s0));
  int written0 = 0, wrong1 = 0;
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j) {
      written0 += c[i * 8 + j] != 0.0f;
      wrong1 += c[i * 8 + 4 + j] != 1.0f;
    }
  printf("refused tile elements written %d, accepted tile elements wrong %d\n", written0, wrong1);

  // The synchronous entry point, k beyond K_MAX: one failed call, not two.
  printf("sync k = 17: %d\n", fpga_matmul_tiled_auto_scheduled(4, 17, 4, a, b, c, 1, 0, 101));
  systolic_offload_stats(&s2);
  printf("failed %llu\n", s2.failed - s1.failed);
  return 0;
}

// CHECK:      ldc 2 < n 4: -11
// CHECK-NEXT: accepted: 0
// CHECK-NEXT: wait: 0
// CHECK-NEXT: failed 1 first_error -11 invocations 1
// CHECK-NEXT: refused tile elements written 0, accepted tile elements wrong 0
// CHECK-NEXT: sync k = 17: -10
// CHECK-NEXT: failed 1
