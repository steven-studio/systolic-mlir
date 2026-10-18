/*
 * systolic_offload.h -- what the runtime behind a compiled kernel has done,
 * for the code that calls the kernel: the call shims systolic-cc generates
 * (tools/systolic-cc) and the test harnesses (test/e2e/affine_c).
 *
 * The compiled kernels call fpga_matmul_tiled_auto_scheduled (the runtime
 * ABI, runtime/fpga_matmul_tiled.h) and do not look at its return code. The
 * caller asks here, after the kernel returns, whether any invocation failed,
 * so that a failed transfer is reported as a runtime failure and never
 * mistaken for a result. One implementation per backend, chosen at link
 * time:
 *
 *   soft_runtime.c   a software model of fpga_matmul_tiled_auto_scheduled:
 *                    NOT the FPGA
 *   board_stats.c    the real runtime (libsystolic_fpga_runtime: DPTI over
 *                    the FT2232H to the programmed board)
 */

#ifndef SYSTOLIC_OFFLOAD_H
#define SYSTOLIC_OFFLOAD_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  const char *backend;                 /* human-readable, for reports */
  int is_hardware;                     /* 1 only for the physical FPGA */
  unsigned long long per_device[4];    /* completed invocations, dev 0..3 */
  unsigned long long failed;           /* invocations that returned rc != 0 */
  int first_error;                     /* first rc != 0 since start, 0 = none */
  unsigned long long bytes_to_fpga;    /* on the link (hardware only) */
  unsigned long long bytes_from_fpga;
} systolic_offload_stats_t;

/* Totals since the program started. */
void systolic_offload_stats(systolic_offload_stats_t *out);

/* Release the runtime's link, if it has one (call once, at the end). */
void systolic_offload_close(void);

#ifdef __cplusplus
}
#endif

#endif
