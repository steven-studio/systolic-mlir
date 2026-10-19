// REQUIRES: cfront-clang
//
// P0 views end to end (docs/AFFINE_GEMM_MAPPING.md sections 5, 6): kernels
// whose operands run backwards (negative strides), are stored transposed,
// have fixed (extent-one) dimensions, rank 1 or 3, or no dimension for a role
// of extent one, through the whole compiler -- front end, recognition, views,
// the fleet partition, the FPGA lowering's staging copies, the runtime -- and
// each result compared with a reference that runs the same loops on the CPU
// (products through a call, so it is not offloaded itself):
//
//   neg_a        C[i][j] += A[7 - i][k] * B[k][j]              A backwards
//   neg_b_step   B[15 - 2k][j]                                   B backwards, stride -2
//   neg_c_tr     C[7 - j][i] += A[i][k] * B[k][j]              C transposed and backwards
//   down_seq     for (i = 10; i >= 0; i -= 2): A[i][k], C[10 - i][j]  a decreasing sequence
//   neg_offset   A[20 - 2i][k], C[i + 5][j]                    backwards, stride -2, offset
//   neg_tr       A[10 - k][i]                                  transposed and backwards
//   mid_origin   A[12 - i][k] in a 16-row A                    origin inside the allocation
//   tr_a, tr_b   A[k][i], B[j][k]                              transposed
//   const_row    C[0][j] += A[0][k] * B[k][j]                  M = 1 (SQ-4)
//   outer        C[i][j] += A[i][3] * B[3][j]                  K = 1
//   gemv         y[i] += M[i][k] * x[k]                        N = 1, rank-1 operands
//   rank3        C[1][i][j] += A[i][2][k] * B[k][j]            rank-3 operands, fixed dimensions
//
// NO REAL HARDWARE HERE: the software runtime model computes every tile;
// SOFT_RUNTIME_MOCK_HARDWARE=1 makes it claim to be the FPGA.
//
// With the reassociation permission every kernel is offloaded:
// RUN: %systolic-cc --fp-reassociation --explain %s -o %t 2>&1 | FileCheck %s --check-prefix=PLAN
// PLAN-COUNT-13: note: '{{[a-z0-9_]+}}' is offloaded:
// PLAN: wrote {{.*}} (13 function(s) offloaded
// RUN: %t > %t.soft
// RUN: FileCheck %s --check-prefix=ALL --input-file=%t.soft
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t > %t.mock
// RUN: FileCheck %s --check-prefix=ALL --input-file=%t.mock
// ALL-COUNT-13: [case] {{[a-z0-9_]+}}: equal to the reference, offloaded
// ALL: PASS: 0 case(s) differ
//
// Every offloaded call also against the function as written, on the CPU:
// RUN: %systolic-cc --fp-reassociation --verify %s -o %t.v > /dev/null 2>&1
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t.v 2>&1 >/dev/null | FileCheck %s --check-prefix=VERIFY
// VERIFY-COUNT-13: verified: {{[0-9]+}} elements equal the CPU reference (bitwise)
// VERIFY-NOT: MISMATCH
//
// Without it (the default: the C program does not permit reassociating its
// sums, section 6) nothing is offloaded, every result is the loop's own:
// RUN: %systolic-cc --explain %s -o %t.strict 2>&1 | FileCheck %s --check-prefix=STRICT
// STRICT-COUNT-13: note: '{{[a-z0-9_]+}}' stays on the host: its GEMM is offloaded only with permission to reassociate floating-point sums (--fp-reassociation) (line {{[0-9]+}}: floating-point reassociation is not permitted
// STRICT: wrote {{.*}} (0 function(s) offloaded
// RUN: %t.strict | FileCheck %s --check-prefix=HOST
// HOST-COUNT-13: [case] {{[a-z0-9_]+}}: equal to the reference, on the host
// HOST: PASS: 0 case(s) differ

#include <stdio.h>
#include <string.h>

#include "systolic_offload.h"

void neg_a(float A[restrict 8][8], float B[restrict 8][8], float C[restrict 8][8]) {
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 8; ++k)
        C[i][j] += A[7 - i][k] * B[k][j];
}
void neg_b_step(float A[restrict 8][8], float B[restrict 16][8], float C[restrict 8][8]) {
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 8; ++k)
        C[i][j] += A[i][k] * B[15 - 2 * k][j];
}
void neg_c_tr(float A[restrict 8][8], float B[restrict 8][8], float C[restrict 8][8]) {
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 8; ++k)
        C[7 - j][i] += A[i][k] * B[k][j];
}
/* i = 10, 8, .., 0 (a decreasing induction sequence, six values); A row i,
   C row 10 - i (0, 2, .., 10): A backwards from 10, C forwards, stride 2. */
void down_seq(float A[restrict 11][8], float B[restrict 8][8], float C[restrict 11][8]) {
  for (int i = 10; i >= 0; i -= 2)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 8; ++k)
        C[10 - i][j] += A[i][k] * B[k][j];
}
void neg_offset(float A[restrict 21][8], float B[restrict 8][8], float C[restrict 16][8]) {
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 8; ++k)
        C[i + 5][j] += A[20 - 2 * i][k] * B[k][j];
}
void neg_tr(float A[restrict 12][8], float B[restrict 8][8], float C[restrict 8][8]) {
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 8; ++k)
        C[i][j] += A[10 - k][i] * B[k][j];
}
void mid_origin(float A[restrict 16][8], float B[restrict 8][8], float C[restrict 8][8]) {
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 8; ++k)
        C[i][j] += A[12 - i][k] * B[k][j];
}
void tr_a(float A[restrict 8][8], float B[restrict 8][8], float C[restrict 8][8]) {
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 8; ++k)
        C[i][j] += A[k][i] * B[k][j];
}
void tr_b(float A[restrict 8][8], float B[restrict 8][8], float C[restrict 8][8]) {
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 8; ++k)
        C[i][j] += A[i][k] * B[j][k];
}
void const_row(float A[restrict 8][8], float B[restrict 8][8], float C[restrict 8][8]) {
  for (int j = 0; j < 8; ++j)
    for (int k0 = 0; k0 < 2; ++k0)
      for (int k1 = 0; k1 < 4; ++k1)
        C[0][j] += A[0][4 * k0 + k1] * B[4 * k0 + k1][j];
}
void outer(float A[restrict 8][8], float B[restrict 8][8], float C[restrict 8][8]) {
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j)
      C[i][j] += A[i][3] * B[3][j];
}
void gemv(float M[restrict 8][8], float x[restrict 8], float y[restrict 8]) {
  for (int i = 0; i < 8; ++i)
    for (int k = 0; k < 8; ++k)
      y[i] += M[i][k] * x[k];
}
void rank3(float A[restrict 8][3][8], float B[restrict 8][8], float C[restrict 2][8][8]) {
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 8; ++k)
        C[1][i][j] += A[i][2][k] * B[k][j];
}

/* The same loops on the CPU, products through a call. */
static float mul(float a, float b) { return a * b; }
static void neg_a_ref(float A[8][8], float B[8][8], float C[8][8]) {
  for (int i = 0; i < 8; ++i) for (int j = 0; j < 8; ++j) for (int k = 0; k < 8; ++k)
    C[i][j] = C[i][j] + mul(A[7 - i][k], B[k][j]);
}
static void neg_b_step_ref(float A[8][8], float B[16][8], float C[8][8]) {
  for (int i = 0; i < 8; ++i) for (int j = 0; j < 8; ++j) for (int k = 0; k < 8; ++k)
    C[i][j] = C[i][j] + mul(A[i][k], B[15 - 2 * k][j]);
}
static void neg_c_tr_ref(float A[8][8], float B[8][8], float C[8][8]) {
  for (int i = 0; i < 8; ++i) for (int j = 0; j < 8; ++j) for (int k = 0; k < 8; ++k)
    C[7 - j][i] = C[7 - j][i] + mul(A[i][k], B[k][j]);
}
static void down_seq_ref(float A[11][8], float B[8][8], float C[11][8]) {
  for (int i = 10; i >= 0; i -= 2) for (int j = 0; j < 8; ++j) for (int k = 0; k < 8; ++k)
    C[10 - i][j] = C[10 - i][j] + mul(A[i][k], B[k][j]);
}
static void neg_offset_ref(float A[21][8], float B[8][8], float C[16][8]) {
  for (int i = 0; i < 8; ++i) for (int j = 0; j < 8; ++j) for (int k = 0; k < 8; ++k)
    C[i + 5][j] = C[i + 5][j] + mul(A[20 - 2 * i][k], B[k][j]);
}
static void neg_tr_ref(float A[12][8], float B[8][8], float C[8][8]) {
  for (int i = 0; i < 8; ++i) for (int j = 0; j < 8; ++j) for (int k = 0; k < 8; ++k)
    C[i][j] = C[i][j] + mul(A[10 - k][i], B[k][j]);
}
static void mid_origin_ref(float A[16][8], float B[8][8], float C[8][8]) {
  for (int i = 0; i < 8; ++i) for (int j = 0; j < 8; ++j) for (int k = 0; k < 8; ++k)
    C[i][j] = C[i][j] + mul(A[12 - i][k], B[k][j]);
}
static void tr_a_ref(float A[8][8], float B[8][8], float C[8][8]) {
  for (int i = 0; i < 8; ++i) for (int j = 0; j < 8; ++j) for (int k = 0; k < 8; ++k)
    C[i][j] = C[i][j] + mul(A[k][i], B[k][j]);
}
static void tr_b_ref(float A[8][8], float B[8][8], float C[8][8]) {
  for (int i = 0; i < 8; ++i) for (int j = 0; j < 8; ++j) for (int k = 0; k < 8; ++k)
    C[i][j] = C[i][j] + mul(A[i][k], B[j][k]);
}
static void const_row_ref(float A[8][8], float B[8][8], float C[8][8]) {
  for (int j = 0; j < 8; ++j) for (int k0 = 0; k0 < 2; ++k0) for (int k1 = 0; k1 < 4; ++k1)
    C[0][j] = C[0][j] + mul(A[0][4 * k0 + k1], B[4 * k0 + k1][j]);
}
static void outer_ref(float A[8][8], float B[8][8], float C[8][8]) {
  for (int i = 0; i < 8; ++i) for (int j = 0; j < 8; ++j)
    C[i][j] = C[i][j] + mul(A[i][3], B[3][j]);
}
static void gemv_ref(float M[8][8], float x[8], float y[8]) {
  for (int i = 0; i < 8; ++i) for (int k = 0; k < 8; ++k)
    y[i] = y[i] + mul(M[i][k], x[k]);
}
static void rank3_ref(float A[8][3][8], float B[8][8], float C[2][8][8]) {
  for (int i = 0; i < 8; ++i) for (int j = 0; j < 8; ++j) for (int k = 0; k < 8; ++k)
    C[1][i][j] = C[1][i][j] + mul(A[i][2][k], B[k][j]);
}

static unsigned long long invocations(void) {
  systolic_offload_stats_t s;
  systolic_offload_stats(&s);
  return s.per_device[0] + s.per_device[1] + s.per_device[2] + s.per_device[3];
}

/* Small integers: every partial sum is exact in float, so the reassociated
   systolic sum equals the loop's bit for bit. Elements no kernel touches keep
   their values -- compared too (memcmp over the whole array). */
static void fill(float *p, int n, int seed) {
  for (int e = 0; e < n; ++e)
    p[e] = (float)((e * 7 + seed) % 9 - 4);
}

static int differ;

static void report(const char *name, const void *got, const void *want,
                   size_t bytes, unsigned long long inv) {
  int same = memcmp(got, want, bytes) == 0;
  differ += !same;
  printf("[case] %s: %s, %s\n", name,
         same ? "equal to the reference" : "DIFFERENT FROM THE REFERENCE",
         inv ? "offloaded" : "on the host");
}

#define RUN_CASE(name, a, b, c)                                                \
  do {                                                                         \
    static float A[sizeof(a) / sizeof(float)], B[sizeof(b) / sizeof(float)],   \
        C[sizeof(c) / sizeof(float)], R[sizeof(c) / sizeof(float)];            \
    fill(A, sizeof A / sizeof(float), 1);                                      \
    fill(B, sizeof B / sizeof(float), 2);                                      \
    fill(C, sizeof C / sizeof(float), 3);                                      \
    memcpy(R, C, sizeof C);                                                    \
    unsigned long long before = invocations();                                 \
    name((void *)A, (void *)B, (void *)C);                                     \
    unsigned long long inv = invocations() - before;                           \
    name##_ref((void *)A, (void *)B, (void *)R);                               \
    report(#name, C, R, sizeof C, inv);                                        \
  } while (0)

int main(void) {
  float m88[8][8], m168[16][8], m118[11][8], m218[21][8], m128[12][8],
      m838[8][3][8], m288[2][8][8], v8[8];
  printf("affine P0 test\n");
  RUN_CASE(neg_a, m88, m88, m88);
  RUN_CASE(neg_b_step, m88, m168, m88);
  RUN_CASE(neg_c_tr, m88, m88, m88);
  RUN_CASE(down_seq, m118, m88, m118);
  RUN_CASE(neg_offset, m218, m88, m168);
  RUN_CASE(neg_tr, m128, m88, m88);
  RUN_CASE(mid_origin, m168, m88, m88);
  RUN_CASE(tr_a, m88, m88, m88);
  RUN_CASE(tr_b, m88, m88, m88);
  RUN_CASE(const_row, m88, m88, m88);
  RUN_CASE(outer, m88, m88, m88);
  RUN_CASE(gemv, m88, v8, v8);
  RUN_CASE(rank3, m838, m88, m288);
  printf("%s: %d case(s) differ\n", differ ? "FAIL" : "PASS", differ);
  return differ != 0;
}
