/*
 * board_stats.c -- systolic_offload.h for the REAL runtime.
 *
 * The compiled kernel's runtime calls resolve to libsystolic_fpga_runtime:
 *
 *   fpga_gemm_begin / fpga_tile_submit / fpga_gemm_wait
 *       (runtime/fpga_async_runtime.cpp, the default of systolic-cc for a
 *       bitstream with the completion record, COMPLETION_HEADER=1): every
 *       tile of a GEMM is submitted without waiting for the one before it;
 *       the runtime keeps PIPELINE_DEPTH descriptors in flight per array,
 *       matches each completion record to its job and accumulates the
 *       results in k_index order; the GEMM's wait collects them all.
 *   fpga_matmul_tiled_auto_scheduled
 *       (runtime/fpga_matmul_tiled.cpp, --submission sync or a bitstream
 *       without the record): one invocation at a time, each waited for.
 *
 * Either way the tiles go as operand slabs over DPTI (FT2232H sync FIFO) to
 * the programmed 1x8x8 + 3x4x4 bitstream and the results come back into C.
 * This file only reports the two runtimes' own statistics, counted at the
 * FTDI layer, and closes their sessions. A program uses one of the two (the
 * compiler chose at build time), so the sums below have one zero side.
 */

#include "systolic_offload.h"

#include "fpga_async_runtime.h"
#include "fpga_matmul_tiled.h"

void systolic_offload_stats(systolic_offload_stats_t *out) {
  fpga_runtime_stats_t sync;
  fpga_async_stats_t async;
  fpga_runtime_get_stats(&sync);
  fpga_async_runtime_get_stats(&async);
  out->backend = async.tiles_submitted
                     ? "FPGA board over DPTI (libsystolic_fpga_runtime, "
                       "batch submission: fpga_tile_submit / fpga_gemm_wait)"
                     : "FPGA board over DPTI (libsystolic_fpga_runtime)";
  out->is_hardware = 1;
  for (int d = 0; d < 4; ++d)
    out->per_device[d] =
        sync.invocations_per_device[d] + async.tiles_per_device[d];
  /* A tile the asynchronous runtime refused at submit time, or whose
     completion record reported a refusal or a link failure, is a failed
     invocation like the synchronous runtime's. A GEMM that could not be
     opened at all (fpga_gemm_begin returned NULL) submitted nothing, so the
     generated shim sees fewer invocations than expected and falls back to
     the CPU the same way. */
  out->failed = sync.failed_calls + async.tiles_failed;
  out->first_error = sync.first_error ? sync.first_error : async.first_error;
  if (out->first_error == 0)
    out->first_error = fpga_async_runtime_last_error();
  out->bytes_to_fpga = sync.bytes_to_fpga + async.bytes_to_fpga;
  out->bytes_from_fpga = sync.bytes_from_fpga + async.bytes_from_fpga;
}

void systolic_offload_close(void) {
  fpga_async_runtime_close();
  fpga_runtime_close();
}
