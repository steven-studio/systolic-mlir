// REQUIRES: cfront-clang
//
// Rank-1 affine accesses and a C that is not injective on the (I, J) box
// (S4; the recognizer's section 25; test/Systolic/affine_gemm_rank1.mlir pins
// the recognition and the IR). An accumulation whose C writes one element
// from two outputs of the (I, J) box -- C[10i + j] with j < 11, C[i + j],
// C[4i + j] with j < 5, T4 with j < 11 -- has one output role demoted to
// batch loops (I first, else J) when those loops and the other batch loops
// are an outermost prefix of the nest, with constant trip counts, on a box
// domain whose counters are as written. Each batch reads C, adds its tile and
// writes C back before the next batch reads it, so every element of C gets
// the contributions of the batches in the order written.
// RE-DECIDED 2026-10-21 (S4): c10_j11, cipj, c4_j5, t4_j11 and j_outer_j11
// stayed on the host before (refused at recognition, G2: "a decomposition
// with the J (or I) loops as batch loops would be valid, and is not
// constructed"); they are now offloaded as that decomposition. The evidence
// is below: each offloaded kernel is compared BITWISE with a CPU reference
// that runs the loops as written (mul() keeps the reference from being
// contracted), on integer-valued nonzero factors with an initial C that is
// not all zero, by the program itself and by --verify, with the synchronous
// and the asynchronous submission. The rank-1 factor views (A[5i + k][5i + k],
// A[i][2k][k], Hankel, the merged dense K, T4 with j < 10) are regressions,
// lowered as before S4. The assignments whose C collides, the nest whose
// reduction loop k is outermost, the sheared nest (its counters rewritten,
// U != I) and the triangular nest stay on the host, computed as written.
//
// NO REAL HARDWARE HERE: the software runtime model computes every tile;
// SOFT_RUNTIME_MOCK_HARDWARE=1 makes it claim to be the FPGA.
//
// RUN: %systolic-cc --fp-reassociation --explain %s -o %t 2>&1 | FileCheck %s --check-prefix=PLAN
// PLAN-DAG: note: 'c10_j10' is offloaded: 3 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'c10_j11' is offloaded: 3 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 6 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'cipj' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 8 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'c4_j4' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'c4_j5' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 6 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'a5ik_k5' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'a5ik_k7' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'a2kk' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'hankel_34' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'merged_k' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 't4_j10' is offloaded: 3 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 't4_j11' is offloaded: 3 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 6 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'j_outer_j11' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 11 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'assign_c10_j11' stays on the host: no GEMM the systolic arrays can run was recognized in it{{$}}
// PLAN-DAG: note: 'assign_cipj' stays on the host: no GEMM the systolic arrays can run was recognized in it{{$}}
// PLAN-DAG: note: 'k_outer_j11' stays on the host: no GEMM the systolic arrays can run was recognized in it{{$}}
// PLAN-DAG: note: 'sheared_j11' stays on the host: no GEMM the systolic arrays can run was recognized in it{{$}}
// PLAN-DAG: note: 'triangle_j' stays on the host: no GEMM the systolic arrays can run was recognized in it{{$}}
// PLAN: wrote {{.*}} (13 function(s) offloaded
//
// RUN: %t > %t.soft
// RUN: FileCheck %s --check-prefix=ALL --input-file=%t.soft
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t > %t.mock
// RUN: FileCheck %s --check-prefix=ALL --input-file=%t.mock
// ALL: [case] c10_j10: equal, offloaded (3)
// ALL: [case] c10_j11: equal, offloaded (6)
// ALL: [case] cipj: equal, offloaded (8)
// ALL: [case] c4_j4: equal, offloaded (1)
// ALL: [case] c4_j5: equal, offloaded (6)
// ALL: [case] a5ik_k5: equal, offloaded (1)
// ALL: [case] a5ik_k7: equal, offloaded (1)
// ALL: [case] a2kk: equal, offloaded (2)
// ALL: [case] hankel_34: equal, offloaded (2)
// ALL: [case] merged_k: equal, offloaded (2)
// ALL: [case] t4_j10: equal, offloaded (3)
// ALL: [case] t4_j11: equal, offloaded (6)
// ALL: [case] j_outer_j11: equal, offloaded (11)
// ALL: [case] assign_c10_j11: equal, NOT OFFLOADED (0)
// ALL: [case] assign_cipj: equal, NOT OFFLOADED (0)
// ALL: [case] k_outer_j11: equal, NOT OFFLOADED (0)
// ALL: [case] sheared_j11: equal, NOT OFFLOADED (0)
// ALL: [case] triangle_j: equal, NOT OFFLOADED (0)
// ALL: PASS: 0 differ
//
// Every offloaded call also against the function as written, on the CPU, with
// the synchronous and the asynchronous submission:
// RUN: %systolic-cc --fp-reassociation --verify %s -o %t.v > /dev/null 2>&1
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t.v 2>&1 >/dev/null | FileCheck %s --check-prefix=VERIFY
// RUN: %systolic-cc --fp-reassociation --submission async --verify %s -o %t.va > /dev/null 2>&1
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t.va 2>&1 >/dev/null | FileCheck %s --check-prefix=VERIFY
// VERIFY: c10_j10 ({{.*}}): verified: {{[0-9]+}} elements equal the CPU reference (bitwise)
// VERIFY: c10_j11 ({{.*}}): verified: {{[0-9]+}} elements equal the CPU reference (bitwise)
// VERIFY: cipj ({{.*}}): verified: {{[0-9]+}} elements equal the CPU reference (bitwise)
// VERIFY: c4_j4 ({{.*}}): verified: {{[0-9]+}} elements equal the CPU reference (bitwise)
// VERIFY: c4_j5 ({{.*}}): verified: {{[0-9]+}} elements equal the CPU reference (bitwise)
// VERIFY: a5ik_k5 ({{.*}}): verified: {{[0-9]+}} elements equal the CPU reference (bitwise)
// VERIFY: a5ik_k7 ({{.*}}): verified: {{[0-9]+}} elements equal the CPU reference (bitwise)
// VERIFY: a2kk ({{.*}}): verified: {{[0-9]+}} elements equal the CPU reference (bitwise)
// VERIFY: hankel_34 ({{.*}}): verified: {{[0-9]+}} elements equal the CPU reference (bitwise)
// VERIFY: merged_k ({{.*}}): verified: {{[0-9]+}} elements equal the CPU reference (bitwise)
// VERIFY: t4_j10 ({{.*}}): verified: {{[0-9]+}} elements equal the CPU reference (bitwise)
// VERIFY: t4_j11 ({{.*}}): verified: {{[0-9]+}} elements equal the CPU reference (bitwise)
// VERIFY: j_outer_j11 ({{.*}}): verified: {{[0-9]+}} elements equal the CPU reference (bitwise)
// VERIFY-NOT: MISMATCH
//
// The asynchronous submission keeps the read-modify-write of C serial across
// batches: in each batch loop body the GEMM is begun, its tiles submitted, and
// waited for (the results accumulated into C) before the loop's next
// iteration begins the next batch (the kernels of the --verify build above):
// RUN: FileCheck %s --check-prefix=ASYNC --input-file=%t.va.systolic/50-fpga.mlir
// ASYNC-LABEL: func.func @c10_j11__systolic_kernel(
// ASYNC:       affine.for %{{.*}} = 0 to 2 {
// ASYNC-NOT:   llvm.call @fpga_
// ASYNC:       %[[G:[0-9]+]] = llvm.call @fpga_gemm_begin(
// ASYNC-COUNT-3: llvm.call @fpga_tile_submit(%[[G]],
// ASYNC-NOT:   llvm.call @fpga_
// ASYNC:       llvm.call @fpga_gemm_wait(%[[G]])
// ASYNC-NOT:   llvm.call @fpga_
// ASYNC:       bufferization.materialize_in_destination
// ASYNC-NEXT:  {{^    }$}}
// ASYNC-NEXT:  return
// ASYNC-LABEL: func.func @cipj__systolic_kernel(
// ASYNC:       affine.for %{{.*}} = 0 to 4 {
// ASYNC-NOT:   llvm.call @fpga_
// ASYNC:       %[[G:[0-9]+]] = llvm.call @fpga_gemm_begin(
// ASYNC-COUNT-2: llvm.call @fpga_tile_submit(%[[G]],
// ASYNC-NOT:   llvm.call @fpga_
// ASYNC:       llvm.call @fpga_gemm_wait(%[[G]])
// ASYNC-NOT:   llvm.call @fpga_
// ASYNC:       bufferization.materialize_in_destination
// ASYNC-NEXT:  {{^    }$}}
// ASYNC-NEXT:  return
// ASYNC-LABEL: func.func @c4_j5__systolic_kernel(
// ASYNC:       affine.for %{{.*}} = 0 to 3 {
// ASYNC-NOT:   llvm.call @fpga_
// ASYNC:       %[[G:[0-9]+]] = llvm.call @fpga_gemm_begin(
// ASYNC-COUNT-2: llvm.call @fpga_tile_submit(%[[G]],
// ASYNC-NOT:   llvm.call @fpga_
// ASYNC:       llvm.call @fpga_gemm_wait(%[[G]])
// ASYNC-NOT:   llvm.call @fpga_
// ASYNC:       bufferization.materialize_in_destination
// ASYNC-NEXT:  {{^    }$}}
// ASYNC-NEXT:  return
// ASYNC-LABEL: func.func @t4_j11__systolic_kernel(
// ASYNC:       affine.for %{{.*}} = 0 to 2 {
// ASYNC-NOT:   llvm.call @fpga_
// ASYNC:       %[[G:[0-9]+]] = llvm.call @fpga_gemm_begin(
// ASYNC-COUNT-3: llvm.call @fpga_tile_submit(%[[G]],
// ASYNC-NOT:   llvm.call @fpga_
// ASYNC:       llvm.call @fpga_gemm_wait(%[[G]])
// ASYNC-NOT:   llvm.call @fpga_
// ASYNC:       bufferization.materialize_in_destination
// ASYNC-NEXT:  {{^    }$}}
// ASYNC-NEXT:  return
// ASYNC-LABEL: func.func @j_outer_j11__systolic_kernel(
// ASYNC:       affine.for %{{.*}} = 0 to 11 {
// ASYNC-NOT:   llvm.call @fpga_
// ASYNC:       %[[G:[0-9]+]] = llvm.call @fpga_gemm_begin(
// ASYNC-COUNT-1: llvm.call @fpga_tile_submit(%[[G]],
// ASYNC-NOT:   llvm.call @fpga_
// ASYNC:       llvm.call @fpga_gemm_wait(%[[G]])
// ASYNC-NOT:   llvm.call @fpga_
// ASYNC:       bufferization.materialize_in_destination
// ASYNC-NEXT:  {{^    }$}}
// ASYNC-NEXT:  return
//
// Strict floating point (no --fp-reassociation): nothing is offloaded.
// RUN: %systolic-cc --explain %s -o %t.strict 2>&1 | FileCheck %s --check-prefix=STRICT
// STRICT-DAG: note: 'c10_j10' stays on the host: its GEMM is offloaded only with permission to reassociate floating-point sums
// STRICT-DAG: note: 'c10_j11' stays on the host: its GEMM is offloaded only with permission to reassociate floating-point sums
// STRICT-DAG: note: 'cipj' stays on the host: its GEMM is offloaded only with permission to reassociate floating-point sums
// STRICT-DAG: note: 'c4_j4' stays on the host: its GEMM is offloaded only with permission to reassociate floating-point sums
// STRICT-DAG: note: 'c4_j5' stays on the host: its GEMM is offloaded only with permission to reassociate floating-point sums
// STRICT-DAG: note: 'a5ik_k5' stays on the host: its GEMM is offloaded only with permission to reassociate floating-point sums
// STRICT-DAG: note: 'a5ik_k7' stays on the host: its GEMM is offloaded only with permission to reassociate floating-point sums
// STRICT-DAG: note: 'a2kk' stays on the host: its GEMM is offloaded only with permission to reassociate floating-point sums
// STRICT-DAG: note: 'hankel_34' stays on the host: its GEMM is offloaded only with permission to reassociate floating-point sums
// STRICT-DAG: note: 'merged_k' stays on the host: its GEMM is offloaded only with permission to reassociate floating-point sums
// STRICT-DAG: note: 't4_j10' stays on the host: its GEMM is offloaded only with permission to reassociate floating-point sums
// STRICT-DAG: note: 't4_j11' stays on the host: its GEMM is offloaded only with permission to reassociate floating-point sums
// STRICT-DAG: note: 'j_outer_j11' stays on the host: its GEMM is offloaded only with permission to reassociate floating-point sums
// STRICT-DAG: note: 'assign_c10_j11' stays on the host: no GEMM the systolic arrays can run was recognized in it{{$}}
// STRICT-DAG: note: 'assign_cipj' stays on the host: no GEMM the systolic arrays can run was recognized in it{{$}}
// STRICT-DAG: note: 'k_outer_j11' stays on the host: no GEMM the systolic arrays can run was recognized in it{{$}}
// STRICT-DAG: note: 'sheared_j11' stays on the host: no GEMM the systolic arrays can run was recognized in it{{$}}
// STRICT-DAG: note: 'triangle_j' stays on the host: no GEMM the systolic arrays can run was recognized in it{{$}}
// STRICT: wrote {{.*}} (0 function(s) offloaded

#include <stdio.h>
#include <string.h>

#include "systolic_offload.h"

/* C[10i + j], j < 10: C injective, the plain 2 x 10 x 4 GEMM */
void c10_j10(float A[restrict 2][4], float B[restrict 4][10], float C[restrict 20]) {
  for (int i = 0; i < 2; ++i) for (int j = 0; j < 10; ++j) for (int k = 0; k < 4; ++k) C[10 * i + j] += A[i][k] * B[k][j];
}

/* C[10i + j], j < 11: C[10] from (0, 10) and (1, 0); i demoted, 2 ordered 1 x 11 x 4 batches (RE-DECIDED, S4) */
void c10_j11(float A[restrict 2][4], float B[restrict 4][11], float C[restrict 21]) {
  for (int i = 0; i < 2; ++i) for (int j = 0; j < 11; ++j) for (int k = 0; k < 4; ++k) C[10 * i + j] += A[i][k] * B[k][j];
}

/* C[i + j]: overlapping outputs; i demoted, 4 ordered 1 x 5 x 3 batches (RE-DECIDED, S4) */
void cipj(float A[restrict 4][3], float B[restrict 3][5], float C[restrict 8]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 3; ++k) C[i + j] += A[i][k] * B[k][j];
}

/* C[4i + j], j < 4: injective, the plain 3 x 4 x 3 GEMM */
void c4_j4(float A[restrict 3][3], float B[restrict 3][4], float C[restrict 12]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 3; ++k) C[4 * i + j] += A[i][k] * B[k][j];
}

/* C[4i + j], j < 5: collision; i demoted, 3 ordered 1 x 5 x 3 batches (RE-DECIDED, S4) */
void c4_j5(float A[restrict 3][3], float B[restrict 3][5], float C[restrict 13]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 3; ++k) C[4 * i + j] += A[i][k] * B[k][j];
}

/* rank-1 A[5i + k][5i + k], k < 5 (regression) */
void a5ik_k5(float A[restrict 20][20], float B[restrict 5][3], float C[restrict 4][3]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 3; ++j) for (int k = 0; k < 5; ++k) C[i][j] += A[5 * i + k][5 * i + k] * B[k][j];
}

/* rank-1 A[5i + k][5i + k], k < 7: an overlapping read view (regression) */
void a5ik_k7(float A[restrict 22][22], float B[restrict 7][3], float C[restrict 4][3]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 3; ++j) for (int k = 0; k < 7; ++k) C[i][j] += A[5 * i + k][5 * i + k] * B[k][j];
}

/* rank-1 A[i][2k][k] (regression) */
void a2kk(float A[restrict 3][8][4], float B[restrict 4][5], float C[restrict 3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[i][j] += A[i][2 * k][k] * B[k][j];
}

/* Hankel A[3i + k] * B[4k + j] (regression) */
void hankel_34(float A[restrict 10], float B[restrict 17], float C[restrict 3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[i][j] += A[3 * i + k] * B[4 * k + j];
}

/* merged dense K A[i][4k0 + k1] * B[4k0 + k1][j] (regression) */
void merged_k(float A[restrict 4][12], float B[restrict 12][5], float C[restrict 4][5]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 5; ++j) for (int k0 = 0; k0 < 3; ++k0) for (int k1 = 0; k1 < 4; ++k1) C[i][j] += A[i][4 * k0 + k1] * B[4 * k0 + k1][j];
}

/* T4 C[10i + j] += A[20i + k][20i + k] * B[50k + j][j], j < 10: C injective (regression) */
void t4_j10(float A[restrict 24][24], float B[restrict 160][10], float C[restrict 20]) {
  for (int i = 0; i < 2; ++i) for (int j = 0; j < 10; ++j) for (int k = 0; k < 4; ++k) C[10 * i + j] += A[20 * i + k][20 * i + k] * B[50 * k + j][j];
}

/* T4 with j < 11 (B rows 161): collision; i demoted, 2 ordered 1 x 11 x 4 batches (RE-DECIDED, S4) */
void t4_j11(float A[restrict 24][24], float B[restrict 161][11], float C[restrict 21]) {
  for (int i = 0; i < 2; ++i) for (int j = 0; j < 11; ++j) for (int k = 0; k < 4; ++k) C[10 * i + j] += A[20 * i + k][20 * i + k] * B[50 * k + j][j];
}

/* j outermost, C[10i + j], j < 11: J demoted, 11 ordered 2 x 1 x 3 batches (RE-DECIDED, S4) */
void j_outer_j11(float A[restrict 2][3], float B[restrict 3][11], float C[restrict 21]) {
  for (int j = 0; j < 11; ++j) for (int i = 0; i < 2; ++i) for (int k = 0; k < 3; ++k) C[10 * i + j] += A[i][k] * B[k][j];
}

/* assignment collision C[10i + j] = A[i] * B[j], j < 11: refused (only the last product survives) */
void assign_c10_j11(float A[restrict 2], float B[restrict 11], float C[restrict 21]) {
  for (int i = 0; i < 2; ++i) for (int j = 0; j < 11; ++j) C[10 * i + j] = A[i] * B[j];
}

/* assignment collision C[i + j] = A[i] * B[j]: refused */
void assign_cipj(float A[restrict 4], float B[restrict 5], float C[restrict 8]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 5; ++j) C[i + j] = A[i] * B[j];
}

/* reduction loop outermost (k, i, j), C[10i + j], j < 11: refused (no outermost prefix of batches) */
void k_outer_j11(float A[restrict 2][3], float B[restrict 3][11], float C[restrict 21]) {
  for (int k = 0; k < 3; ++k) for (int i = 0; i < 2; ++i) for (int j = 0; j < 11; ++j) C[10 * i + j] += A[i][k] * B[k][j];
}

/* sheared collision t in [i, i + 11), C[10i + t - i]: counters rewritten (U != I), refused */
void sheared_j11(float A[restrict 2][4], float B[restrict 4][11], float C[restrict 21]) {
  for (int i = 0; i < 2; ++i) for (int t = i; t < i + 11; ++t) for (int k = 0; k < 4; ++k) C[10 * i + t - i] += A[i][k] * B[k][t - i];
}

/* non-box collision j < i + 10, C[10i + j]: refused */
void triangle_j(float A[restrict 3][4], float B[restrict 4][12], float C[restrict 32]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < i + 10; ++j) for (int k = 0; k < 4; ++k) C[10 * i + j] += A[i][k] * B[k][j];
}

static float mul(float a, float b) { return a * b; }
static void c10_j10_ref(float A[2][4], float B[4][10], float C[20]) {
  for (int i = 0; i < 2; ++i) for (int j = 0; j < 10; ++j) for (int k = 0; k < 4; ++k) C[10 * i + j] = C[10 * i + j] + mul(A[i][k], B[k][j]); }
static void c10_j11_ref(float A[2][4], float B[4][11], float C[21]) {
  for (int i = 0; i < 2; ++i) for (int j = 0; j < 11; ++j) for (int k = 0; k < 4; ++k) C[10 * i + j] = C[10 * i + j] + mul(A[i][k], B[k][j]); }
static void cipj_ref(float A[4][3], float B[3][5], float C[8]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 3; ++k) C[i + j] = C[i + j] + mul(A[i][k], B[k][j]); }
static void c4_j4_ref(float A[3][3], float B[3][4], float C[12]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 4; ++j) for (int k = 0; k < 3; ++k) C[4 * i + j] = C[4 * i + j] + mul(A[i][k], B[k][j]); }
static void c4_j5_ref(float A[3][3], float B[3][5], float C[13]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 3; ++k) C[4 * i + j] = C[4 * i + j] + mul(A[i][k], B[k][j]); }
static void a5ik_k5_ref(float A[20][20], float B[5][3], float C[4][3]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 3; ++j) for (int k = 0; k < 5; ++k) C[i][j] = C[i][j] + mul(A[5 * i + k][5 * i + k], B[k][j]); }
static void a5ik_k7_ref(float A[22][22], float B[7][3], float C[4][3]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 3; ++j) for (int k = 0; k < 7; ++k) C[i][j] = C[i][j] + mul(A[5 * i + k][5 * i + k], B[k][j]); }
static void a2kk_ref(float A[3][8][4], float B[4][5], float C[3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[i][j] = C[i][j] + mul(A[i][2 * k][k], B[k][j]); }
static void hankel_34_ref(float A[10], float B[17], float C[3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[i][j] = C[i][j] + mul(A[3 * i + k], B[4 * k + j]); }
static void merged_k_ref(float A[4][12], float B[12][5], float C[4][5]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 5; ++j) for (int k0 = 0; k0 < 3; ++k0) for (int k1 = 0; k1 < 4; ++k1) C[i][j] = C[i][j] + mul(A[i][4 * k0 + k1], B[4 * k0 + k1][j]); }
static void t4_j10_ref(float A[24][24], float B[160][10], float C[20]) {
  for (int i = 0; i < 2; ++i) for (int j = 0; j < 10; ++j) for (int k = 0; k < 4; ++k) C[10 * i + j] = C[10 * i + j] + mul(A[20 * i + k][20 * i + k], B[50 * k + j][j]); }
static void t4_j11_ref(float A[24][24], float B[161][11], float C[21]) {
  for (int i = 0; i < 2; ++i) for (int j = 0; j < 11; ++j) for (int k = 0; k < 4; ++k) C[10 * i + j] = C[10 * i + j] + mul(A[20 * i + k][20 * i + k], B[50 * k + j][j]); }
static void j_outer_j11_ref(float A[2][3], float B[3][11], float C[21]) {
  for (int j = 0; j < 11; ++j) for (int i = 0; i < 2; ++i) for (int k = 0; k < 3; ++k) C[10 * i + j] = C[10 * i + j] + mul(A[i][k], B[k][j]); }
static void assign_c10_j11_ref(float A[2], float B[11], float C[21]) {
  for (int i = 0; i < 2; ++i) for (int j = 0; j < 11; ++j) C[10 * i + j] = mul(A[i], B[j]); }
static void assign_cipj_ref(float A[4], float B[5], float C[8]) {
  for (int i = 0; i < 4; ++i) for (int j = 0; j < 5; ++j) C[i + j] = mul(A[i], B[j]); }
static void k_outer_j11_ref(float A[2][3], float B[3][11], float C[21]) {
  for (int k = 0; k < 3; ++k) for (int i = 0; i < 2; ++i) for (int j = 0; j < 11; ++j) C[10 * i + j] = C[10 * i + j] + mul(A[i][k], B[k][j]); }
static void sheared_j11_ref(float A[2][4], float B[4][11], float C[21]) {
  for (int i = 0; i < 2; ++i) for (int t = i; t < i + 11; ++t) for (int k = 0; k < 4; ++k) C[10 * i + t - i] = C[10 * i + t - i] + mul(A[i][k], B[k][t - i]); }
static void triangle_j_ref(float A[3][4], float B[4][12], float C[32]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < i + 10; ++j) for (int k = 0; k < 4; ++k) C[10 * i + j] = C[10 * i + j] + mul(A[i][k], B[k][j]); }

/* Integer-valued data: every sum is exact, so the results are bitwise equal
   whatever the order inside a tile; the initial C is nonzero. */
static void fill(float *p, int n, int s) { for (int e = 0; e < n; ++e) p[e] = (float)((e * 7 + s) % 9 - 4); }
static void fill_nz(float *p, int n, int s) { for (int e = 0; e < n; ++e) { int v = (e * 7 + s) % 9 - 4; p[e] = (float)(v ? v : 5 - 2 * (e & 1)); } }
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
  RUN(c10_j10, 8, 40, 20);
  RUN(c10_j11, 8, 44, 21);
  RUN(cipj, 12, 15, 8);
  RUN(c4_j4, 9, 12, 12);
  RUN(c4_j5, 9, 15, 13);
  RUN(a5ik_k5, 400, 15, 12);
  RUN(a5ik_k7, 484, 21, 12);
  RUN(a2kk, 96, 20, 15);
  RUN(hankel_34, 10, 17, 15);
  RUN(merged_k, 48, 60, 20);
  RUN(t4_j10, 576, 1600, 20);
  RUN(t4_j11, 576, 1771, 21);
  RUN(j_outer_j11, 6, 33, 21);
  RUN(assign_c10_j11, 2, 11, 21);
  RUN(assign_cipj, 4, 5, 8);
  RUN(k_outer_j11, 6, 33, 21);
  RUN(sheared_j11, 8, 44, 21);
  RUN(triangle_j, 12, 48, 32);
  printf("%s: %d differ\n", differ ? "FAIL" : "PASS", differ);
  return differ;
}
