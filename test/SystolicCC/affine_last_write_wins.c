// REQUIRES: cfront-clang
//
// Last write wins (docs/AFFINE_GEMM_MAPPING.md section 20.5, on by default).
// In an assignment C = A * B on a box as written whose factors provably do not
// overlap C (restrict), a loop of two or more iterations that C does not read
// is cut to its last iteration -- the only one whose product survives -- and
// the nest becomes the K = 1 GEMM into a zeroed C when every element of C is
// then written once (section 20). The cut changes no value: each offloaded
// kernel is compared BITWISE with a CPU reference that runs the loops as
// written (the product through mul(), so it is not rewritten), on integer-
// valued factors without zeros and a NONZERO initial C. The assignment is not
// made an accumulation; no floating-point permission beyond the one every
// assignment needs (--fp-reassociation: the tile sums from +0) is involved,
// and without that permission nothing is cut (STRICT).
//
// Not lowered: not_written_once (k is cut in the kernel copy -- see CUTREM --
// but C[i + j] is not written once, so the function stays on the host: the
// host runs the function as written) and aliasing (no restrict: no cut, and
// called once with A inside C, where the overwritten values are read back).
//
// NO REAL HARDWARE HERE: the software runtime model computes every tile;
// SOFT_RUNTIME_MOCK_HARDWARE=1 makes it claim to be the FPGA.
//
// RUN: %systolic-cc --fp-reassociation --explain %s -o %t 2>&1 | FileCheck %s --check-prefix=PLAN
// PLAN-DAG: note: 'k_outer' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'k_middle' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'k_inner' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'two_cut' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'stepped' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'lower_bound' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 't5' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 't2' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'one_and_cut' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'not_written_once' stays on the host: no GEMM the systolic arrays can run was recognized in it{{$}}
// PLAN-DAG: note: 'aliasing' stays on the host: no GEMM the systolic arrays can run was recognized in it{{$}}
// PLAN: wrote {{.*}} (9 function(s) offloaded
//
// The cuts, in the remarks systolic-cc keeps (PROGRAM.systolic): the nine
// lowered nests and not_written_once, not aliasing.
// RUN: FileCheck %s --check-prefix=CUTREM --input-file=%t.systolic/20-remarks.txt
// CUTREM-COUNT-10: remark: affine assignment nest C = A * B: last write wins
// CUTREM-NOT:      last write wins
//
// RUN: %t > %t.soft
// RUN: FileCheck %s --check-prefix=ALL --input-file=%t.soft
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t > %t.mock
// RUN: FileCheck %s --check-prefix=ALL --input-file=%t.mock
// ALL: [case] k_outer: equal, offloaded (2)
// ALL: [case] k_middle: equal, offloaded (2)
// ALL: [case] k_inner: equal, offloaded (2)
// ALL: [case] two_cut: equal, offloaded (2)
// ALL: [case] stepped: equal, offloaded (2)
// ALL: [case] lower_bound: equal, offloaded (2)
// ALL: [case] t5: equal, offloaded (2)
// ALL: [case] t2: equal, offloaded (2)
// ALL: [case] one_and_cut: equal, offloaded (2)
// ALL: [case] not_written_once: equal, NOT OFFLOADED (0)
// ALL: [case] aliasing: equal, NOT OFFLOADED (0)
// ALL: [case] aliasing_overlap: equal, NOT OFFLOADED (0)
// ALL: PASS: 0 differ
//
// Every offloaded call also against the function as written, on the CPU, with
// the synchronous and the asynchronous submission:
// RUN: %systolic-cc --fp-reassociation --verify %s -o %t.v > /dev/null 2>&1
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t.v 2>&1 >/dev/null | FileCheck %s --check-prefix=VERIFY
// RUN: %systolic-cc --fp-reassociation --submission async --verify %s -o %t.va > /dev/null 2>&1
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t.va 2>&1 >/dev/null | FileCheck %s --check-prefix=VERIFY
// VERIFY-COUNT-9: verified: {{[0-9]+}} elements equal the CPU reference (bitwise)
// VERIFY-NOT: verified:
// VERIFY-NOT: MISMATCH
//
// Strict floating point (no --fp-reassociation): nothing is cut (last write
// wins runs only on a nest that may be lowered, docs section 20.5), so every
// nest with a loop C does not read is refused at recognition as written -- no
// GEMM is recognized in any function, no remark is kept -- and the results are
// those of the loops as written.
// RUN: %systolic-cc --explain %s -o %t.strict 2>&1 | FileCheck %s --check-prefix=STRICT
// STRICT-NOT: ' is offloaded:
// STRICT-DAG: note: 'k_outer' stays on the host: no GEMM the systolic arrays can run was recognized in it{{$}}
// STRICT-DAG: note: 'k_middle' stays on the host: no GEMM the systolic arrays can run was recognized in it{{$}}
// STRICT-DAG: note: 'k_inner' stays on the host: no GEMM the systolic arrays can run was recognized in it{{$}}
// STRICT-DAG: note: 'two_cut' stays on the host: no GEMM the systolic arrays can run was recognized in it{{$}}
// STRICT-DAG: note: 'stepped' stays on the host: no GEMM the systolic arrays can run was recognized in it{{$}}
// STRICT-DAG: note: 'lower_bound' stays on the host: no GEMM the systolic arrays can run was recognized in it{{$}}
// STRICT-DAG: note: 't5' stays on the host: no GEMM the systolic arrays can run was recognized in it{{$}}
// STRICT-DAG: note: 't2' stays on the host: no GEMM the systolic arrays can run was recognized in it{{$}}
// STRICT-DAG: note: 'one_and_cut' stays on the host: no GEMM the systolic arrays can run was recognized in it{{$}}
// STRICT-DAG: note: 'not_written_once' stays on the host: no GEMM the systolic arrays can run was recognized in it{{$}}
// STRICT-DAG: note: 'aliasing' stays on the host: no GEMM the systolic arrays can run was recognized in it{{$}}
// STRICT: wrote {{.*}} (0 function(s) offloaded
// RUN: FileCheck %s --allow-empty --check-prefix=STRICTREM --input-file=%t.strict.systolic/20-remarks.txt
// STRICTREM-NOT: last write wins
// RUN: %t.strict | FileCheck %s --check-prefix=STRICTRUN
// STRICTRUN-COUNT-12: equal, NOT OFFLOADED (0)
// STRICTRUN-NEXT: PASS: 0 differ

#include <stdio.h>
#include <string.h>

#include "systolic_offload.h"

/* K outermost: for k, i, j -- only k = 3 survives */
void k_outer(float A[restrict 3][4], float B[restrict 4][5], float C[restrict 3][5]) {
  for (int k = 0; k < 4; ++k) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) C[i][j] = A[i][k] * B[k][j];
}

/* K in the middle: for i, k, j */
void k_middle(float A[restrict 3][4], float B[restrict 4][5], float C[restrict 3][5]) {
  for (int i = 0; i < 3; ++i) for (int k = 0; k < 4; ++k) for (int j = 0; j < 5; ++j) C[i][j] = A[i][k] * B[k][j];
}

/* K innermost: for i, j, k */
void k_inner(float A[restrict 3][4], float B[restrict 4][5], float C[restrict 3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[i][j] = A[i][k] * B[k][j];
}

/* two loops C does not read (k, l): both cut, to k = 3 and l = 1 */
void two_cut(float A[restrict 3][4][2], float B[restrict 4][2][5], float C[restrict 3][5]) {
  for (int i = 0; i < 3; ++i) for (int k = 0; k < 4; ++k) for (int j = 0; j < 5; ++j) for (int l = 0; l < 2; ++l) C[i][j] = A[i][k][l] * B[k][l][j];
}

/* a stepped loop: k = 1, 3, 5, 7; the last writer is k = 7 (not 8) */
void stepped(float A[restrict 3][9], float B[restrict 9][5], float C[restrict 3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 1; k < 9; k += 2) C[i][j] = A[i][k] * B[k][j];
}

/* a nonzero lower bound: k = 2 .. 5, the last writer is k = 5 */
void lower_bound(float A[restrict 3][6], float B[restrict 6][5], float C[restrict 3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 2; k < 6; ++k) C[i][j] = A[i][k] * B[k][j];
}

/* the T5 shape: k read by A only, l by B only */
void t5(float A[restrict 3][4], float B[restrict 4][5], float C[restrict 3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) for (int l = 0; l < 4; ++l) C[i][j] = A[i][k] * B[l][j];
}

/* the T2 shape: A[i][x0 - x1 + 3] * B[x0][j], the last writer is x0 = x1 = 3 */
void t2(float A[restrict 3][7], float B[restrict 4][5], float C[restrict 3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int x0 = 0; x0 < 4; ++x0) for (int x1 = 0; x1 < 4; ++x1) C[i][j] = A[i][x0 - x1 + 3] * B[x0][j];
}

/* a one-iteration loop (l = 2) next to a cut one (k) */
void one_and_cut(float A[restrict 3][4][4], float B[restrict 4][4][5], float C[restrict 3][5]) {
  for (int i = 0; i < 3; ++i) for (int l = 2; l < 3; ++l) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[i][j] = A[i][l][k] * B[l][k][j];
}

/* C not written once over i, j (C[i + j]): k is cut, the nest is refused by
   the written-once rule and runs on the CPU with the cut loop -- the last
   writer of C[e] is still the lexicographically last (i, j) with i + j = e
   at k = 3 */
void not_written_once(float A[restrict 3][4], float B[restrict 4][5], float C[restrict 7]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[i + j] = A[i][k] * B[k][j];
}

/* no restrict: A may overlap C, no cut (called below with A inside C, where
   the earlier k's writes are read by later products) */
void aliasing(float A[3][4], float B[4][5], float C[3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[i][j] = A[i][k] * B[k][j];
}

static float mul(float a, float b) { return a * b; }
static void k_outer_ref(float A[3][4], float B[4][5], float C[3][5]) {
  for (int k = 0; k < 4; ++k) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) C[i][j] = mul(A[i][k], B[k][j]); }
static void k_middle_ref(float A[3][4], float B[4][5], float C[3][5]) {
  for (int i = 0; i < 3; ++i) for (int k = 0; k < 4; ++k) for (int j = 0; j < 5; ++j) C[i][j] = mul(A[i][k], B[k][j]); }
static void k_inner_ref(float A[3][4], float B[4][5], float C[3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[i][j] = mul(A[i][k], B[k][j]); }
static void two_cut_ref(float A[3][4][2], float B[4][2][5], float C[3][5]) {
  for (int i = 0; i < 3; ++i) for (int k = 0; k < 4; ++k) for (int j = 0; j < 5; ++j) for (int l = 0; l < 2; ++l) C[i][j] = mul(A[i][k][l], B[k][l][j]); }
static void stepped_ref(float A[3][9], float B[9][5], float C[3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 1; k < 9; k += 2) C[i][j] = mul(A[i][k], B[k][j]); }
static void lower_bound_ref(float A[3][6], float B[6][5], float C[3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 2; k < 6; ++k) C[i][j] = mul(A[i][k], B[k][j]); }
static void t5_ref(float A[3][4], float B[4][5], float C[3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) for (int l = 0; l < 4; ++l) C[i][j] = mul(A[i][k], B[l][j]); }
static void t2_ref(float A[3][7], float B[4][5], float C[3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int x0 = 0; x0 < 4; ++x0) for (int x1 = 0; x1 < 4; ++x1) C[i][j] = mul(A[i][x0 - x1 + 3], B[x0][j]); }
static void one_and_cut_ref(float A[3][4][4], float B[4][4][5], float C[3][5]) {
  for (int i = 0; i < 3; ++i) for (int l = 2; l < 3; ++l) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[i][j] = mul(A[i][l][k], B[l][k][j]); }
static void not_written_once_ref(float A[3][4], float B[4][5], float C[7]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[i + j] = mul(A[i][k], B[k][j]); }
static void aliasing_ref(float A[3][4], float B[4][5], float C[3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[i][j] = mul(A[i][k], B[k][j]); }

/* Integer-valued data: every product is exact, so the results are bitwise
   those of the loops as written. The factors have no zeros (a cut that kept
   the wrong iteration could not hide behind a zero product) and C starts
   nonzero (a write that did not happen would show). Every factor element
   distinct within a row/column window, so the surviving iteration is
   identified by its value. */
static void fill(float *p, int n, int s) { for (int e = 0; e < n; ++e) p[e] = (float)((e * 7 + s) % 9 - 4); }
static void fill_nz(float *p, int n, int s) { for (int e = 0; e < n; ++e) p[e] = (float)(1 + (e * 5 + s) % 11) * ((e & 1) ? -1.0f : 1.0f); }
static int differ;
static unsigned long long inv(void) { systolic_offload_stats_t s; systolic_offload_stats(&s); return s.per_device[0]+s.per_device[1]+s.per_device[2]+s.per_device[3]; }
static void report(const char *n, const void *g, const void *w, size_t by, unsigned long long i) {
  int same = memcmp(g, w, by) == 0; differ += !same;
  printf("[case] %s: %s, %s (%llu)\n", n, same ? "equal" : "DIFFERENT", i ? "offloaded" : "NOT OFFLOADED", i);
}
#define RUN(name, NA, NB, NC) do { static float A[NA], B[NB], C[NC], RA[NA], RB[NB], RC[NC]; \
  fill_nz(A, NA, 1); fill_nz(B, NB, 2); fill(C, NC, 3); memcpy(RA, A, sizeof A); memcpy(RB, B, sizeof B); memcpy(RC, C, sizeof C); \
  unsigned long long before = inv(); name((void*)A,(void*)B,(void*)C); unsigned long long n = inv() - before; \
  name##_ref((void*)RA,(void*)RB,(void*)RC); report(#name, C, RC, sizeof C, n); } while (0)
int main(void) {
  RUN(k_outer, 12, 20, 15);
  RUN(k_middle, 12, 20, 15);
  RUN(k_inner, 12, 20, 15);
  RUN(two_cut, 24, 40, 15);
  RUN(stepped, 27, 45, 15);
  RUN(lower_bound, 18, 30, 15);
  RUN(t5, 12, 20, 15);
  RUN(t2, 21, 20, 15);
  RUN(one_and_cut, 48, 80, 15);
  RUN(not_written_once, 12, 20, 7);
  RUN(aliasing, 12, 20, 15);
  { /* A inside C: A[i][k] is C's element 4 i + k + 1, so C[i][j] written at
       an earlier k (or an earlier (i, j)) is read by a later product */
    static float X[16], B[20], RX[16], RB[20];
    fill_nz(X, 16, 4); fill_nz(B, 20, 2); memcpy(RX, X, sizeof X); memcpy(RB, B, sizeof B);
    unsigned long long before = inv(); aliasing((void*)(X + 1), (void*)B, (void*)X); unsigned long long n = inv() - before;
    aliasing_ref((void*)(RX + 1), (void*)RB, (void*)RX); report("aliasing_overlap", X, RX, sizeof X, n);
  }
  printf("%s: %d differ\n", differ ? "FAIL" : "PASS", differ);
  return differ;
}
