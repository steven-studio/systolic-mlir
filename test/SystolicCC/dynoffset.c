// REQUIRES: cfront-clang
//
// GEMMs whose output (and inputs) are views placed by the function's
// integer parameters -- dynamic offsets: C[r0 + i][c0 + j] += A[r0 + i][k]
// * B[k][c0 + j]. The front end takes the parameters as affine symbols,
// the recognizer separates them from the loop part of each access, the
// views are offset at run time, and the lowering passes each tile a
// pointer computed from the view's run-time offset. Checked on the
// software runtime model (NOT the FPGA) against the CPU: the same program
// compiled by clang alone, and --verify's comparison of every offloaded
// call with the function as written. Both lowerings: the synchronous one
// and (--submission async) the asynchronous one, where the pointer comes
// from the view's descriptor (memref.extract_strided_metadata).
//
// RUN: %systolic-cc --verify --explain %s -o %t 2>&1 | FileCheck %s --check-prefix=PLAN
// RUN: %t > %t.out 2> %t.err
// RUN: FileCheck %s --check-prefix=VERIFY --input-file=%t.err
// RUN: %host-cc -std=c11 %s -o %t.cpu
// RUN: %t.cpu > %t.cpu.out
// RUN: grep -v '^\[systolic-cc\]' %t.out | diff %t.cpu.out -
//
// RUN: %systolic-cc --submission async --verify --explain %s -o %t.a 2>&1 | FileCheck %s --check-prefix=PLAN
// RUN: grep -q 'memref.extract_strided_metadata' %t.a.systolic/50-fpga.mlir
// RUN: %t.a > %t.a.out 2> %t.a.err
// RUN: FileCheck %s --check-prefix=VERIFY --input-file=%t.a.err
// RUN: grep -v '^\[systolic-cc\]' %t.a.out | diff %t.cpu.out -
//
// The views the front end made: a subview whose offset is the parameter.
// RUN: FileCheck %s --check-prefix=VIEW --input-file=%t.systolic/40-systolic.mlir

#include <stdio.h>

// A row offset on C: 8 rows at r0 of a 32-row array; A whole, B whole.
void gemm_rowoff(float A[restrict 8][16], float B[restrict 16][16],
                 float C[restrict 32][16], int r0) {
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 16; ++j)
      for (int k = 0; k < 16; ++k)
        C[r0 + i][j] += A[i][k] * B[k][j];
}

// Row and column offsets on C, a row offset on A, a column offset on B, a
// constant offset on top of the rows, K = 40 folded (16 + 16 + 8), edge
// tiles (12x12): twelve rows at r0 + 1 and twelve columns at c0 of a 32x32
// output. (The row coordinate -- the loop part -- is i + 1 for A and C
// alike: the recognizer's logical coordinates are shared by the operands;
// only the symbol terms are per operand.)
void gemm_block(float A[restrict 32][40], float B[restrict 40][32],
                float C[restrict 32][32], int r0, int c0) {
  for (int i = 0; i < 12; ++i)
    for (int j = 0; j < 12; ++j)
      for (int k = 0; k < 40; ++k)
        C[r0 + 1 + i][c0 + j] += A[r0 + 1 + i][k] * B[k][c0 + j];
}

// An access stride with a run-time offset: every other row of C (and of
// A, the shared row coordinate).
void gemm_stride(float A[restrict 16][8], float B[restrict 8][8],
                 float C[restrict 32][8], int r0) {
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 8; ++k)
        C[2 * i + r0][j] += A[2 * i][k] * B[k][j];
}

// Two GEMMs in one function on two offset views of C. (The front end
// writes each nest's result back right after it, so the second is opened
// after the first is waited for; views at run-time offsets of one buffer
// are ordered by the lowering as well.)
void gemm_two(float A[restrict 8][16], float B[restrict 16][16],
              float C[restrict 32][16], int r0, int r1) {
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 16; ++j)
      for (int k = 0; k < 16; ++k)
        C[r0 + i][j] += A[i][k] * B[k][j];
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 16; ++j)
      for (int k = 0; k < 16; ++k)
        C[r1 + i][j] += A[i][k] * B[k][j];
}

// The input and the output are views of one array: not provably disjoint,
// whatever the offsets -- stays on the host.
void gemm_alias(float X[restrict 32][16], float B[restrict 16][16], int r0) {
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 16; ++j)
      for (int k = 0; k < 16; ++k)
        X[r0 + i][j] += X[i][k] * B[k][j];
}

// An unsigned offset would wrap: not an affine symbol -- stays on the host.
void gemm_unsigned(float A[restrict 8][16], float B[restrict 16][16],
                   float C[restrict 32][16], unsigned r0) {
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 16; ++j)
      for (int k = 0; k < 16; ++k)
        C[r0 + i][j] += A[i][k] * B[k][j];
}

static float A[32][40], B[40][32], C[32][32];
static float A8[8][16], B16[16][16], C32[32][16];
static float A88[16][8], B88[8][8], C328[32][8];

static void fill(void) {
  for (int i = 0; i < 32; ++i)
    for (int k = 0; k < 40; ++k)
      A[i][k] = (float)((i * 3 + k) % 7 - 3);
  for (int k = 0; k < 40; ++k)
    for (int j = 0; j < 32; ++j)
      B[k][j] = (float)((k * 2 + j * 5) % 5 - 2);
  for (int i = 0; i < 32; ++i)
    for (int j = 0; j < 32; ++j)
      C[i][j] = (float)((i + 2 * j) % 5 - 2);
  for (int i = 0; i < 8; ++i)
    for (int k = 0; k < 16; ++k)
      A8[i][k] = (float)((i * 7 + k * 3) % 9 - 4);
  for (int k = 0; k < 16; ++k)
    for (int j = 0; j < 16; ++j)
      B16[k][j] = (float)((k * 5 + j * 11) % 7 - 3);
  for (int i = 0; i < 32; ++i)
    for (int j = 0; j < 16; ++j)
      C32[i][j] = (float)((i * 3 + j) % 5 - 2);
  for (int i = 0; i < 16; ++i)
    for (int k = 0; k < 8; ++k)
      A88[i][k] = (float)((i + 2 * k) % 5 - 2);
  for (int i = 0; i < 8; ++i)
    for (int k = 0; k < 8; ++k)
      B88[i][k] = (float)((3 * i + k) % 7 - 3);
  for (int i = 0; i < 32; ++i)
    for (int j = 0; j < 8; ++j)
      C328[i][j] = (float)((i * j) % 3 - 1);
}

static double sum(const float *p, int n, int w) {
  double s = 0;
  for (int i = 0; i < n; ++i)
    s += p[i] * (double)((i % w) + 1);
  return s;
}

int main(void) {
  fill();
  // The same compiled function at several offsets: the offset is a run-time
  // value, different at each call.
  for (int r0 = 0; r0 <= 24; r0 += 8)
    gemm_rowoff(A8, B16, C32, r0);
  gemm_rowoff(A8, B16, C32, 5);
  gemm_block(A, B, C, 0, 0);
  gemm_block(A, B, C, 19, 20);
  gemm_block(A, B, C, 7, 3);
  gemm_stride(A88, B88, C328, 1);
  gemm_stride(A88, B88, C328, 16);
  gemm_two(A8, B16, C32, 2, 20);
  gemm_alias(C32, B16, 10);
  gemm_unsigned(A8, B16, C32, 3u);
  printf("%.1f %.1f %.1f %g %g %g\n", sum(&C32[0][0], 32 * 16, 16),
         sum(&C[0][0], 32 * 32, 32), sum(&C328[0][0], 32 * 8, 8), C32[29][7],
         C[31][31], C328[17][3]);
  return 0;
}

// PLAN-DAG: dynoffset.c:{{[0-9]+}}:1: note: 'gemm_rowoff' is offloaded: 5 systolic tile invocation(s)
// PLAN-DAG: dynoffset.c:{{[0-9]+}}:1: note: 'gemm_block' is offloaded: {{[0-9]+}} systolic tile invocation(s)
// PLAN-DAG: dynoffset.c:{{[0-9]+}}:1: note: 'gemm_stride' is offloaded: {{[0-9]+}} systolic tile invocation(s)
// PLAN-DAG: dynoffset.c:{{[0-9]+}}:1: note: 'gemm_two' is offloaded: 10 systolic tile invocation(s)
// PLAN-DAG: dynoffset.c:{{[0-9]+}}:1: note: 'gemm_alias' stays on the host: no GEMM the systolic arrays can run was recognized in it (line {{[0-9]+}}: A may overlap C; declare the arguments llvm.noalias (C restrict) or pass assume-no-alias)
// PLAN-DAG: dynoffset.c:{{[0-9]+}}:{{[0-9]+}}: note: 'gemm_unsigned' is outside the supported C subset and is not translated: subscript uses the unsigned parameter 'r0'
// PLAN:     wrote {{.*}} (4 function(s) offloaded
//
// VERIFY:      gemm_rowoff (dynoffset.c:{{[0-9]+}}): verified: 128 elements equal the CPU reference (bitwise)
// VERIFY-NEXT: gemm_rowoff (dynoffset.c:{{[0-9]+}}): verified: 256 elements equal the CPU reference (bitwise)
// VERIFY-NEXT: gemm_rowoff (dynoffset.c:{{[0-9]+}}): verified: 384 elements equal the CPU reference (bitwise)
// VERIFY-NEXT: gemm_rowoff (dynoffset.c:{{[0-9]+}}): verified: 512 elements equal the CPU reference (bitwise)
// VERIFY-NEXT: gemm_rowoff (dynoffset.c:{{[0-9]+}}): verified: 208 elements equal the CPU reference (bitwise)
// VERIFY-NEXT: gemm_block (dynoffset.c:{{[0-9]+}}): verified: 416 elements equal the CPU reference (bitwise)
// VERIFY-NEXT: gemm_block (dynoffset.c:{{[0-9]+}}): verified: 1024 elements equal the CPU reference (bitwise)
// VERIFY-NEXT: gemm_block (dynoffset.c:{{[0-9]+}}): verified: 640 elements equal the CPU reference (bitwise)
// VERIFY-NEXT: gemm_stride (dynoffset.c:{{[0-9]+}}): verified: 128 elements equal the CPU reference (bitwise)
// VERIFY-NEXT: gemm_stride (dynoffset.c:{{[0-9]+}}): verified: 248 elements equal the CPU reference (bitwise)
// VERIFY-NEXT: gemm_two (dynoffset.c:{{[0-9]+}}): verified: 448 elements equal the CPU reference (bitwise)
//
// VIEW: memref.subview %{{.*}}[%{{.*}}, 0] [8, 16] [1, 1] : memref<?x16xf32> to memref<8x16xf32, strided<[16, 1], offset: ?>>
