// REQUIRES: cfront-clang
//
// The asynchronous runtime API on the software runtime model
// (runtime/offload/soft_runtime.c, NOT the FPGA): a GEMM is sealed by its
// first wait, and an output tile not all of whose k_count folds were
// accepted -- a fold refused at submit, here k beyond the device's K_MAX --
// fails with -17 instead of being waited for or reported done over a
// partial K-sum, as in the real runtime (fpga_async_runtime.cpp, checked by
// runtime-async-test mode H). The program calls the API directly.
//
// RUN: %host-cc -std=c11 -I%S/../../runtime %s %S/../../runtime/offload/soft_runtime.c -o %t
// RUN: env SOFT_K_MAX_8X8=16 SOFT_K_MAX_4X4=16 %t | FileCheck %s

#include "fpga_async_runtime.h"

#include <stdio.h>

static float a[4 * 17], b[17 * 4], c[4 * 8];

static void show(fpga_gemm_t *h) {
  fpga_gemm_status_t st;
  fpga_gemm_poll(h, &st);
  printf("done %d completed %u failed %u held %u first_error %d\n", st.done,
         st.completed, st.failed, st.held, st.first_error);
}

int main(void) {
  for (int i = 0; i < 4 * 17; ++i)
    a[i] = b[i] = 1.0f;

  // Output tile 0: fold 0 of 2 refused, fold 1 accepted; output tile 1
  // (columns 4..7) complete.
  fpga_gemm_t *h = fpga_gemm_begin(4, 8, 2, 2);
  printf("tile 0 fold 0, k = 17: %d\n", fpga_tile_submit(h, 0, 0, 2, 4, 17, 4, a, b, c, 8, 1, 0, 101));
  printf("tile 0 fold 1, k = 1: %d\n", fpga_tile_submit(h, 0, 1, 2, 4, 1, 4, a, b, c, 8, 1, 0, 101));
  printf("tile 1, k = 1: %d\n", fpga_tile_submit(h, 1, 0, 1, 4, 1, 4, a, b, c + 4, 8, 1, 0, 101));
  printf("wait: %d\n", fpga_gemm_wait_for(h, 1000));
  show(h);
  int written0 = 0, wrong1 = 0;
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j) {
      written0 += c[i * 8 + j] != 0.0f;
      wrong1 += c[i * 8 + 4 + j] != 1.0f;
    }
  printf("tile 0 elements written %d, tile 1 elements wrong %d\n", written0, wrong1);
  printf("release: %d\n", fpga_gemm_release(h));

  // The trailing fold refused: fold 0 of 2 accepted, fold 1 refused.
  for (int i = 0; i < 4 * 8; ++i)
    c[i] = 0.0f;
  h = fpga_gemm_begin(4, 4, 2, 1);
  printf("fold 0, k = 1: %d\n", fpga_tile_submit(h, 0, 0, 2, 4, 1, 4, a, b, c, 8, 1, 0, 101));
  printf("fold 1, k = 17: %d\n", fpga_tile_submit(h, 0, 1, 2, 4, 17, 4, a, b, c, 8, 1, 0, 101));
  printf("wait: %d\n", fpga_gemm_wait_for(h, 1000));
  show(h);
  printf("release: %d\n", fpga_gemm_release(h));

  // Every fold accepted: the seal changes nothing.
  h = fpga_gemm_begin(4, 4, 2, 1);
  printf("fold 0: %d\n", fpga_tile_submit(h, 0, 0, 2, 4, 1, 4, a, b, c, 8, 1, 0, 101));
  printf("fold 1: %d\n", fpga_tile_submit(h, 0, 1, 2, 4, 1, 4, a, b, c, 8, 1, 0, 101));
  printf("wait: %d\n", fpga_gemm_wait(h));
  return 0;
}

// CHECK:      tile 0 fold 0, k = 17: -10
// CHECK-NEXT: tile 0 fold 1, k = 1: 0
// CHECK-NEXT: tile 1, k = 1: 0
// CHECK-NEXT: wait: -17
// CHECK-NEXT: done 1 completed 1 failed 1 held 0 first_error -17
// CHECK-NEXT: tile 0 elements written 0, tile 1 elements wrong 0
// CHECK-NEXT: release: 0
// CHECK-NEXT: fold 0, k = 1: 0
// CHECK-NEXT: fold 1, k = 17: -10
// CHECK-NEXT: wait: -17
// CHECK-NEXT: done 1 completed 0 failed 1 held 0 first_error -17
// CHECK-NEXT: release: 0
// CHECK-NEXT: fold 0: 0
// CHECK-NEXT: fold 1: 0
// CHECK-NEXT: wait: 0
