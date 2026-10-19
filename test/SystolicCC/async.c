// REQUIRES: cfront-clang
//
// The same program as program.c compiled with --submission async: every
// offloaded function calls the asynchronous runtime (fpga_gemm_begin, one
// fpga_tile_submit per tile, one fpga_gemm_wait per GEMM) instead of one
// synchronous call per tile. On the software runtime model (NOT the FPGA)
// the model's asynchronous API queues the tiles and runs them at the wait,
// so this checks the asynchronous lowering and the shim around it: the
// program's output equals clang's, --verify sees every offloaded result
// equal the CPU reference, the labels say where each GEMM ran, and a
// transport fault behaves as on the real asynchronous runtime -- the link
// stays failed until the session is closed, so every later GEMM of the
// program falls back to the CPU as well.
//
// RUN: %systolic-cc --submission async --verify --explain %s -o %t 2>&1 | FileCheck %s --check-prefix=PLAN
// RUN: grep -c 'llvm.call @fpga_tile_submit' %t.systolic/50-fpga.mlir | FileCheck %s --check-prefix=SUBMITS
// RUN: grep -c 'llvm.call @fpga_gemm_wait' %t.systolic/50-fpga.mlir | FileCheck %s --check-prefix=WAITS
// RUN: not grep -q 'fpga_matmul_tiled_auto_scheduled' %t.systolic/50-fpga.mlir
// RUN: grep '"submission"' %t.systolic/25-plan.json | FileCheck %s --check-prefix=PLANJSON
// RUN: %t > %t.out 2> %t.err
// RUN: FileCheck %s --check-prefix=VERIFY --input-file=%t.err
// RUN: FileCheck %s --check-prefix=LABELS --input-file=%t.out
// RUN: %host-cc -std=c11 %s -o %t.cpu
// RUN: %t.cpu > %t.cpu.out
// RUN: grep -v '^\[systolic-cc\]' %t.out | diff %t.cpu.out -
//
// With the model claiming to be the FPGA, every offloaded GEMM reports FPGA.
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t > %t.mock
// RUN: FileCheck %s --check-prefix=MOCK --input-file=%t.mock --match-full-lines
// RUN: grep -v '^\[systolic-cc\]' %t.mock | diff %t.cpu.out -
//
// A transport fault at the 4th tile of the first GEMM: the GEMM's wait
// reports it, the call is redone on the CPU, and -- the asynchronous
// runtime's link having failed for the session -- the later offloaded
// GEMMs are refused and run on the CPU too. The output is still right.
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 SOFT_RUNTIME_FAULT=fail SOFT_RUNTIME_FAULT_AT=3 %t > %t.f3
// RUN: FileCheck %s --check-prefix=FAULT --input-file=%t.f3 --match-full-lines
// RUN: grep -v '^\[systolic-cc\]' %t.f3 | diff %t.cpu.out -
//
// SUBMITS: 37
// WAITS: 2
// PLANJSON: "submission": "asynchronous"

#include <stdio.h>

void matmul(float A[restrict 16][16], float B[restrict 16][16],
            float C[restrict 16][16]) {
  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 16; ++j)
      for (int k = 0; k < 16; ++k)
        C[i][j] += A[i][k] * B[k][j];
}

void product(float A[restrict 20][40], float B[restrict 40][12],
             float C[restrict 20][12]) {
  for (int i = 0; i < 20; ++i)
    for (int j = 0; j < 12; ++j)
      C[i][j] = 0.0f;
  for (int k = 0; k < 40; ++k)
    for (int i = 0; i < 20; ++i)
      for (int j = 0; j < 12; ++j)
        C[i][j] += A[i][k] * B[k][j];
}

void scale(float x[restrict 16][16], float alpha) {
  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 16; ++j)
      x[i][j] = alpha * x[i][j];
}

static float A[20][40], B[40][16], C16[16][16], C20[20][12];
static float A16[16][16], B12[40][12];

int main(void) {
  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 16; ++j) {
      A16[i][j] = (float)((i * 7 + j * 3) % 9 - 4);
      B[i][j] = (float)((i * 5 + j * 11) % 7 - 3);
      C16[i][j] = (float)((i + 2 * j) % 5 - 2);
    }
  for (int i = 0; i < 20; ++i)
    for (int k = 0; k < 40; ++k)
      A[i][k] = (float)((i * 3 + k) % 7 - 3);
  for (int k = 0; k < 40; ++k)
    for (int j = 0; j < 12; ++j)
      B12[k][j] = (float)((k * 2 + j * 5) % 5 - 2);

  matmul((float (*)[16])B, A16, C16);
  matmul(A16, (float (*)[16])B, C16);
  product(A, B12, C20);
  scale(C16, 0.5f);

  double s16 = 0, s20 = 0;
  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 16; ++j)
      s16 += C16[i][j] * (i + 1) - j;
  for (int i = 0; i < 20; ++i)
    for (int j = 0; j < 12; ++j)
      s20 += C20[i][j] * (j + 1) - i;
  printf("%.1f %.1f %g %g\n", s16, s20, C16[3][5], C20[19][11]);
  return 0;
}

// PLAN-DAG: async.c:{{[0-9]+}}:1: note: 'matmul' is offloaded: 10 systolic tile invocation(s)
// PLAN-DAG: async.c:{{[0-9]+}}:1: note: 'product' is offloaded: 27 systolic tile invocation(s)
// PLAN-DAG: async.c:{{[0-9]+}}:1: note: 'scale' stays on the host: no GEMM the systolic arrays can run was recognized in it{{$}}
// PLAN:     wrote {{.*}} (2 function(s) offloaded, runtime: software model, NOT the FPGA, asynchronous submission (fpga_tile_submit per tile, one fpga_gemm_wait per GEMM)
//
// VERIFY:      matmul (async.c:{{[0-9]+}}): verified: 256 elements equal the CPU reference (bitwise)
// VERIFY-NEXT: matmul (async.c:{{[0-9]+}}): verified: 256 elements equal the CPU reference (bitwise)
// VERIFY-NEXT: product (async.c:{{[0-9]+}}): verified: 240 elements equal the CPU reference (bitwise)
//
// The software model is not hardware: every GEMM reports CPU.
// LABELS: [systolic-cc] CPU Executed: GEMM #1
// LABELS: [systolic-cc] CPU Executed: GEMM #2
// LABELS: [systolic-cc] CPU Executed: GEMM #3
//
// MOCK: [systolic-cc] FPGA Executed: GEMM #1
// MOCK-NEXT: [systolic-cc] FPGA Executed: GEMM #2
// MOCK-NEXT: [systolic-cc] FPGA Executed: GEMM #3
//
// FAULT: [systolic-cc] CPU Executed: GEMM #1
// FAULT-NEXT: [systolic-cc] CPU Executed: GEMM #2
// FAULT-NEXT: [systolic-cc] CPU Executed: GEMM #3
