/*
 * e2e_runtime_report.h -- what the runtime behind the compiled kernel did,
 * for main.c / strided_main.c. One implementation per backend, chosen at
 * link time by run.sh:
 *
 *   soft_runtime.c    software model of fpga_matmul_tiled_auto_scheduled
 *                     (BACKEND=soft, the default): NOT the FPGA
 *   board_runtime.c   the real runtime (libsystolic_fpga_runtime: DPTI over
 *                     the FT2232H to the programmed board), BACKEND=board
 *
 * so a result always says which of the two produced it.
 */

#ifndef E2E_RUNTIME_REPORT_H
#define E2E_RUNTIME_REPORT_H

typedef struct {
  const char *backend;                 /* human-readable, for the verdict */
  int is_hardware;                     /* 1 only for the physical FPGA */
  unsigned long long per_device[4];    /* successful invocations, dev 0..3 */
  unsigned long long failed;           /* invocations that returned rc != 0 */
  int first_error;                     /* first rc != 0, 0 = none */
  unsigned long long bytes_to_fpga;    /* on the link (hardware only) */
  unsigned long long bytes_from_fpga;
} e2e_runtime_report_t;

/* Fills *out; also closes the runtime's session, if it has one. */
void e2e_runtime_report(e2e_runtime_report_t *out);

#endif
