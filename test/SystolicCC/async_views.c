// REQUIRES: cfront-clang
//
// GEMMs whose C the asynchronous runtime cannot address in place, compiled
// with --submission async: rows written backwards (C[11 - i][j], a view with
// row stride -10), the same per batch (a run-time offset per batch, row
// stride -8), one output row stored down a column (the 1 x 3 view of a 3 x 1
// array, row stride 1 < n = 3), and a reversed GEMM after a forward one in
// the same function. fpga_tile_submit refuses ldc < n, so such a view is
// staged in one contiguous buffer per GEMM: every submit carries a positive
// ldc constant, every GEMM runs on the (mocked) FPGA -- none falls back to
// the CPU, which would also discard the forward GEMM of the same function --
// and the results equal clang's bit for bit. (A forward view at an offset,
// @offset_c, is the direct-path control.) On the software runtime model
// (NOT the FPGA).
//
// RUN: %systolic-cc --submission async --fp-reassociation --verify %s -o %t > /dev/null 2>&1
// RUN: not grep -q 'arith.constant -[0-9]* : i32' %t.systolic/50-fpga.mlir
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t > %t.out 2> %t.err
// RUN: FileCheck %s --check-prefix=MOCK --input-file=%t.out --match-full-lines
// RUN: FileCheck %s --check-prefix=VERIFY --input-file=%t.err
// RUN: %host-cc -std=c11 %s -o %t.cpu
// RUN: %t.cpu > %t.cpu.out
// RUN: grep -v '^\[systolic-cc\]' %t.out | diff %t.cpu.out -
//
// MOCK:      [systolic-cc] FPGA Executed: GEMM #1
// MOCK-NEXT: [systolic-cc] FPGA Executed: GEMM #2
// MOCK-NEXT: [systolic-cc] FPGA Executed: GEMM #3
// MOCK-NEXT: [systolic-cc] FPGA Executed: GEMM #4
// MOCK-NEXT: [systolic-cc] FPGA Executed: GEMM #5
// MOCK-NEXT: [systolic-cc] FPGA Executed: GEMM #6
// VERIFY-COUNT-5: verified: {{[0-9]+}} elements equal the CPU reference (bitwise)
// VERIFY-NOT: MISMATCH

#include <stdio.h>

void rev_rows(float A[restrict 12][20], float B[restrict 20][10], float C[restrict 12][10]) {
  for (int i = 0; i < 12; ++i) for (int j = 0; j < 10; ++j) for (int k = 0; k < 20; ++k)
    C[11 - i][j] += A[i][k] * B[k][j];
}

void batch_rev(float A[restrict 2][8][16], float B[restrict 16][8], float C[restrict 2][8][8]) {
  for (int b = 0; b < 2; ++b) for (int i = 0; i < 8; ++i) for (int j = 0; j < 8; ++j) for (int k = 0; k < 16; ++k)
    C[b][7 - i][j] += A[b][i][k] * B[k][j];
}

void one_row(float A[restrict 1][20], float B[restrict 20][3], float C[restrict 3][1]) {
  for (int i = 0; i < 1; ++i) for (int j = 0; j < 3; ++j) for (int k = 0; k < 20; ++k)
    C[j][i] += A[i][k] * B[k][j];
}

void offset_c(float A[restrict 12][20], float B[restrict 20][10], float C[restrict 15][14]) {
  for (int i = 0; i < 12; ++i) for (int j = 0; j < 10; ++j) for (int k = 0; k < 20; ++k)
    C[i + 2][j + 3] += A[i][k] * B[k][j];
}

// A forward GEMM into C, then a reversed one into D, in one function.
void rr_dep(float A[restrict 8][16], float B[restrict 16][8], float C[restrict 8][8],
            float D[restrict 8][8]) {
  for (int i = 0; i < 8; ++i) for (int j = 0; j < 8; ++j) for (int k = 0; k < 16; ++k)
    C[i][j] += A[i][k] * B[k][j];
  for (int i = 0; i < 8; ++i) for (int j = 0; j < 8; ++j) for (int k = 0; k < 16; ++k)
    D[7 - i][j] += A[i][k] * B[k][j];
}

static float A1[12][20], B1[20][10], C1[12][10], C4[15][14];
static float A5[8][16], B5[16][8], C5[8][8], D5[8][8];
static float A2[2][8][16], B2[16][8], C2[2][8][8];
static float A3[1][20], B3[20][3], C3[3][1];

int main(void) {
  for (int i = 0; i < 12; ++i) for (int k = 0; k < 20; ++k) A1[i][k] = (float)((i * 7 + k * 3) % 9 - 4);
  for (int k = 0; k < 20; ++k) for (int j = 0; j < 10; ++j) B1[k][j] = (float)((k * 5 + j * 11) % 7 - 3);
  for (int i = 0; i < 12; ++i) for (int j = 0; j < 10; ++j) C1[i][j] = (float)((i + 2 * j) % 5 - 2);
  for (int i = 0; i < 15; ++i) for (int j = 0; j < 14; ++j) C4[i][j] = (float)((i * 3 + j) % 4 - 2);
  for (int b = 0; b < 2; ++b) for (int i = 0; i < 8; ++i) for (int k = 0; k < 16; ++k) A2[b][i][k] = (float)((b + i * 3 + k) % 6 - 2);
  for (int k = 0; k < 16; ++k) for (int j = 0; j < 8; ++j) B2[k][j] = (float)((k * 3 + j) % 5 - 2);
  for (int b = 0; b < 2; ++b) for (int i = 0; i < 8; ++i) for (int j = 0; j < 8; ++j) C2[b][i][j] = (float)((b + i + j) % 3 - 1);
  for (int k = 0; k < 20; ++k) A3[0][k] = (float)(k % 5 - 2);
  for (int k = 0; k < 20; ++k) for (int j = 0; j < 3; ++j) B3[k][j] = (float)((k * 2 + j) % 7 - 3);
  for (int j = 0; j < 3; ++j) C3[j][0] = (float)(j - 1);
  for (int i = 0; i < 8; ++i) for (int k = 0; k < 16; ++k) A5[i][k] = (float)((i * 5 + k) % 7 - 3);
  for (int k = 0; k < 16; ++k) for (int j = 0; j < 8; ++j) B5[k][j] = (float)((k + j * 3) % 5 - 2);
  for (int i = 0; i < 8; ++i) for (int j = 0; j < 8; ++j) { C5[i][j] = (float)((i + j) % 4 - 2); D5[i][j] = (float)((i * j) % 3 - 1); }
  rev_rows(A1, B1, C1);
  batch_rev(A2, B2, C2);
  one_row(A3, B3, C3);
  offset_c(A1, B1, C4);
  rr_dep(A5, B5, C5, D5);
  double s = 0;
  for (int i = 0; i < 8; ++i) for (int j = 0; j < 8; ++j) s += C5[i][j] * (i + 3) + D5[i][j] * (j + 7);
  for (int i = 0; i < 12; ++i) for (int j = 0; j < 10; ++j) s += C1[i][j] * (i + 1) + j;
  for (int b = 0; b < 2; ++b) for (int i = 0; i < 8; ++i) for (int j = 0; j < 8; ++j) s += C2[b][i][j] * (b + 1) * (i + 2) + j;
  for (int j = 0; j < 3; ++j) s += 7 * C3[j][0] * (j + 2);
  for (int i = 0; i < 15; ++i) for (int j = 0; j < 14; ++j) s += C4[i][j] * (j + 5);
  printf("checksum %.1f\n", s);
  return 0;
}
