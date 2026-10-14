/*
 * board_runtime.c -- e2e_runtime_report for the REAL runtime (BACKEND=board).
 *
 * The compiled kernel's fpga_matmul_tiled_auto_scheduled calls resolve to
 * libsystolic_fpga_runtime (runtime/fpga_matmul_tiled.cpp): every invocation
 * is staged as an operand slab, sent over DPTI (FT2232H sync FIFO) to the
 * programmed 1x8x8 + 3x4x4 bitstream, and its result tile read back and added
 * into C. This file only reports the runtime's own statistics, counted at
 * the FTDI layer, and closes the session.
 */

#include "e2e_runtime_report.h"

#include "fpga_matmul_tiled.h"

void e2e_runtime_report(e2e_runtime_report_t *out) {
  fpga_runtime_stats_t stats;
  fpga_runtime_get_stats(&stats);
  out->backend = "FPGA board over DPTI (libsystolic_fpga_runtime)";
  out->is_hardware = 1;
  for (int d = 0; d < 4; ++d)
    out->per_device[d] = stats.invocations_per_device[d];
  out->failed = stats.failed_calls;
  out->first_error = stats.first_error;
  out->bytes_to_fpga = stats.bytes_to_fpga;
  out->bytes_from_fpga = stats.bytes_from_fpga;
  fpga_runtime_close();
}
