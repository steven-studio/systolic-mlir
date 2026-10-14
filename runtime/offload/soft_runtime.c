/*
 * soft_runtime.c -- a SOFTWARE MODEL of the FPGA runtime's scheduled entry
 * point, for checking what the compiler emits without the board.
 *
 * NOT the FPGA. It implements the contract of
 * fpga_matmul_tiled_auto_scheduled (runtime/fpga_matmul_tiled.h) on the CPU:
 *
 *   C[M][N] += A[M][K] x B[K][N]       row-major fp32, contiguous
 *
 * on the 1x8x8 + 3x4x4 device table (device 0: 8x8, devices 1..3: 4x4) with
 * the same refusals as the runtime -- -10 for K outside 1..K_MAX of the
 * device, -11 for an unknown device or a tile larger than the array, -12 for
 * a null pointer, -15 for est_cycles 0 -- so a compiled kernel that passes
 * here has issued only invocations the real runtime would accept, and its
 * tiles, offsets, K-folding and accumulation through C compute the GEMM.
 * It says nothing about the bitstream, the DPTI link or the array's own
 * floating-point summation order.
 *
 * K_MAX_8X8 / K_MAX_4X4 come from SOFT_K_MAX_8X8 / SOFT_K_MAX_4X4 (default
 * 16 / 16, the archived bitstream's).
 */

#include "e2e_runtime_report.h"

#include <stdio.h>
#include <stdlib.h>

static const int kRows[4] = {8, 4, 4, 4};

static unsigned long long invocations[4];
static unsigned long long failed_calls;
static int first_error;
static int trace = -1;

static int k_max_of(unsigned device_id) {
  const char *name = device_id == 0 ? "SOFT_K_MAX_8X8" : "SOFT_K_MAX_4X4";
  const char *value = getenv(name);
  return value ? atoi(value) : 16;
}

static int refuse(int rc) {
  ++failed_calls;
  if (first_error == 0)
    first_error = rc;
  return rc;
}

int fpga_matmul_tiled_auto_scheduled(int M, int K, int N, const float *A,
                                     const float *B, float *C,
                                     unsigned device_id, unsigned start_cycle,
                                     unsigned est_cycles) {
  if (trace < 0)
    trace = getenv("SOFT_RUNTIME_TRACE") != NULL;
  if (trace)
    fprintf(stderr,
            "[soft-runtime] device %u tile %dx%d k=%d start_cycle=%u "
            "est_cycles=%u\n",
            device_id, M, N, K, start_cycle, est_cycles);

  if (!A || !B || !C)
    return refuse(-12);
  if (device_id > 3 || M < 1 || N < 1 || M > kRows[device_id] ||
      N > kRows[device_id])
    return refuse(-11);
  if (K < 1 || K > k_max_of(device_id))
    return refuse(-10);
  if (est_cycles == 0)
    return refuse(-15);

  for (int i = 0; i < M; ++i)
    for (int j = 0; j < N; ++j) {
      float acc = 0.0f;
      for (int k = 0; k < K; ++k)
        acc += A[i * K + k] * B[k * N + j];
      C[i * N + j] += acc;
    }
  ++invocations[device_id];
  return 0;
}

void e2e_runtime_report(e2e_runtime_report_t *out) {
  out->backend = "software runtime model (soft_runtime.c), not the FPGA";
  out->is_hardware = 0;
  for (int d = 0; d < 4; ++d)
    out->per_device[d] = invocations[d];
  out->failed = failed_calls;
  out->first_error = first_error;
  out->bytes_to_fpga = 0;
  out->bytes_from_fpga = 0;
}
