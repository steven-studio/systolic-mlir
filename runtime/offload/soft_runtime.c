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
 * K_MAX_8X8 / K_MAX_4X4 come from SOFT_K_MAX_8X8 / SOFT_K_MAX_4X4 in the
 * environment, else from the macros SOFT_DEFAULT_K_MAX_8X8 /
 * SOFT_DEFAULT_K_MAX_4X4 it was compiled with (systolic-cc passes the
 * fleet's), else 16 / 16 (the archived bitstream's).
 *
 * Fault injection, for testing the callers' classification of failures:
 * SOFT_RUNTIME_FAULT=fail makes invocation SOFT_RUNTIME_FAULT_AT (default 0)
 * return -8 (a transport timeout) without computing; =corrupt makes it return
 * 0 with one element of its result tile off by one -- a wrong result the
 * runtime does not report.
 */

#include "systolic_offload.h"

#include <stdio.h>
#include <stdlib.h>

static const int kRows[4] = {8, 4, 4, 4};

static unsigned long long invocations[4];
static unsigned long long failed_calls;
static int first_error;
static int trace = -1;

#ifndef SOFT_DEFAULT_K_MAX_8X8
#define SOFT_DEFAULT_K_MAX_8X8 16
#endif
#ifndef SOFT_DEFAULT_K_MAX_4X4
#define SOFT_DEFAULT_K_MAX_4X4 16
#endif

static unsigned long long calls;

static int k_max_of(unsigned device_id) {
  const char *name = device_id == 0 ? "SOFT_K_MAX_8X8" : "SOFT_K_MAX_4X4";
  const char *value = getenv(name);
  if (value)
    return atoi(value);
  return device_id == 0 ? SOFT_DEFAULT_K_MAX_8X8 : SOFT_DEFAULT_K_MAX_4X4;
}

/* "fail", "corrupt" or NULL for this call. */
static const char *fault_now(void) {
  const char *fault = getenv("SOFT_RUNTIME_FAULT");
  const char *at = getenv("SOFT_RUNTIME_FAULT_AT");
  if (!fault || calls - 1 != (unsigned long long)(at ? atoll(at) : 0))
    return NULL;
  return fault;
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
  ++calls;
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
  const char *fault = fault_now();
  if (fault && fault[0] == 'f')
    return refuse(-8);

  for (int i = 0; i < M; ++i)
    for (int j = 0; j < N; ++j) {
      float acc = 0.0f;
      for (int k = 0; k < K; ++k)
        acc += A[i * K + k] * B[k * N + j];
      C[i * N + j] += acc;
    }
  if (fault && fault[0] == 'c')
    C[0] += 1.0f;
  ++invocations[device_id];
  return 0;
}

void systolic_offload_stats(systolic_offload_stats_t *out) {
  out->backend = "software runtime model (soft_runtime.c), not the FPGA";
  out->is_hardware = 0;
  for (int d = 0; d < 4; ++d)
    out->per_device[d] = invocations[d];
  out->failed = failed_calls;
  out->first_error = first_error;
  out->bytes_to_fpga = 0;
  out->bytes_from_fpga = 0;
}

void systolic_offload_close(void) {}
