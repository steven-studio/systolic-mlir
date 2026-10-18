/*
 * systolic_cc_report.c -- the execution report of a program compiled by
 * systolic-cc: one line per GEMM execution, on stdout (so that it stays in
 * order with the program's own output),
 *
 *   [systolic-cc] FPGA Executed: GEMM #N     the FPGA computed it
 *   [systolic-cc] CPU Executed: GEMM #N      the CPU computed it
 *
 * N counts the program's GEMM executions from 1. Called by the generated
 * shims (an offloaded GEMM, after the runtime confirmed the FPGA ran it) and
 * by the CPU code of every GEMM loop nest (after the nest has run); nothing
 * else in the program reports.
 */

#include <stdio.h>

/* Set by a shim while it runs a reference copy for --verify: that run is a
   check, not an execution of the program's GEMM. */
int __systolic_cc_quiet;

static unsigned long long executed;

void __systolic_cc_gemm_executed(int on_fpga) {
  if (__systolic_cc_quiet)
    return;
  printf("[systolic-cc] %s Executed: GEMM #%llu\n", on_fpga ? "FPGA" : "CPU",
         ++executed);
  fflush(stdout);
}
