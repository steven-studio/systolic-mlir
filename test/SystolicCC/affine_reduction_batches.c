// REQUIRES: cfront-clang
//
// Reduction-batch partitioning (docs/AFFINE_GEMM_MAPPING.md section 23, on by
// default; systolic-cc --no-reduction-batches turns it off). The loops of a multiply-accumulate nest
// that C does not read become reduction batch loops when that makes the nest a
// GEMM: a loop read by one factor only, and -- when the loops both factors read
// are not one GEMM coordinate -- all of those but the one that stays K. Each
// reduction batch loop runs around the tiles with the factors' views offset by
// it, and its values accumulate into the same slab of C, in order. Every
// iteration of the nest still runs exactly once, so repeated products
// (case1) keep their multiplicity. Each offloaded kernel is compared bitwise
// with a CPU reference that runs the loops as written on integer-valued data
// (the factors without zeros) with a NONZERO initial C; the permission to
// reassociate (--fp-reassociation) is required, as for every accumulation.
// Assignments (C = A * B) are considered only with an actual K = 1 or with no
// reduction loop (virtual K = 1, index association dimension 0), and never get
// reduction batch loops (section 20.4).
//
// NO REAL HARDWARE HERE: the software runtime model computes every tile;
// SOFT_RUNTIME_MOCK_HARDWARE=1 makes it claim to be the FPGA.
//
// RUN: %systolic-cc --fp-reassociation --explain %s -o %t 2>&1 | FileCheck %s --check-prefix=PLAN
// PLAN-DAG: note: 't1' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 8 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 't2' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 8 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 't5' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 32 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'case1' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 32 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'case2' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 8 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'coupled' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 8 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'noninj' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 8 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'two_k' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 4 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'partial' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 4 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'a_only' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 4 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'b_only' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 6 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'ob_rb' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 64 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'rb_negative' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 8 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'rb_constants' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 32 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 't2_assign' stays on the host: no GEMM the systolic arrays can run was recognized in it{{$}}
// PLAN-DAG: note: 'assign_k1' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet
// PLAN-DAG: note: 'assign_outer_batched' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 4 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'assign_t5' stays on the host: no GEMM the systolic arrays can run was recognized in it{{$}}
// PLAN: wrote {{.*}} (16 function(s) offloaded
//
// RUN: %t > %t.soft
// RUN: FileCheck %s --check-prefix=ALL --input-file=%t.soft
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t > %t.mock
// RUN: FileCheck %s --check-prefix=ALL --input-file=%t.mock
// ALL: [case] t1: equal, offloaded (8)
// ALL: [case] t2: equal, offloaded (8)
// ALL: [case] t5: equal, offloaded (32)
// ALL: [case] case1: equal, offloaded (32)
// ALL: [case] case2: equal, offloaded (8)
// ALL: [case] coupled: equal, offloaded (8)
// ALL: [case] noninj: equal, offloaded (8)
// ALL: [case] two_k: equal, offloaded (4)
// ALL: [case] partial: equal, offloaded (4)
// ALL: [case] a_only: equal, offloaded (4)
// ALL: [case] b_only: equal, offloaded (6)
// ALL: [case] ob_rb: equal, offloaded (64)
// ALL: [case] rb_negative: equal, offloaded (8)
// ALL: [case] rb_constants: equal, offloaded (32)
// ALL: [case] t2_assign: equal, NOT OFFLOADED (0)
// ALL: [case] assign_k1: equal, offloaded (2)
// ALL: [case] assign_outer_batched: equal, offloaded (4)
// ALL: [case] assign_t5: equal, NOT OFFLOADED (0)
// ALL: PASS: 0 differ
//
// Every offloaded call also against the function as written, on the CPU, with
// the synchronous and the asynchronous submission (the asynchronous one waits
// for each reduction batch iteration before the next reads C):
// RUN: %systolic-cc --fp-reassociation --verify %s -o %t.v > /dev/null 2>&1
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t.v 2>&1 >/dev/null | FileCheck %s --check-prefix=VERIFY
// RUN: %systolic-cc --fp-reassociation --submission async --verify %s -o %t.va > /dev/null 2>&1
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t.va 2>&1 >/dev/null | FileCheck %s --check-prefix=VERIFY
// VERIFY-COUNT-16: verified: {{[0-9]+}} elements equal the CPU reference (bitwise)
// VERIFY-NOT: MISMATCH
//
// Non-integer data (-DFRACTIONAL): the reduction batches reorder each sum, so
// these results are checked within a relative tolerance -- not bitwise:
// RUN: %systolic-cc --fp-reassociation -DFRACTIONAL --verify --verify-rtol 1e-4 %s -o %t.f > /dev/null 2>&1
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t.f 2>&1 >/dev/null | FileCheck %s --check-prefix=RTOL
// RTOL-COUNT-16: verified: {{[0-9]+}} elements equal the CPU reference (relative tolerance 0.0001)
// RTOL-NOT: MISMATCH
//
// Strict floating point (no --fp-reassociation): nothing is offloaded.
// RUN: %systolic-cc --explain %s -o %t.strict 2>&1 | FileCheck %s --check-prefix=STRICT
// STRICT-DAG: note: 't1' stays on the host: its GEMM is offloaded only with permission to reassociate floating-point sums
// STRICT-DAG: note: 't2' stays on the host: its GEMM is offloaded only with permission to reassociate floating-point sums
// STRICT-DAG: note: 't5' stays on the host: its GEMM is offloaded only with permission to reassociate floating-point sums
// STRICT: wrote {{.*}} (0 function(s) offloaded
//
// With --no-reduction-batches the recognition is that of sections 19-21: every
// kernel here except the assignments with K = 1 stays on the host.
// RUN: %systolic-cc --no-reduction-batches --fp-reassociation --explain %s -o %t.off 2>&1 | FileCheck %s --check-prefix=OFF
// OFF-DAG: note: 't1' stays on the host: no GEMM the systolic arrays can run was recognized in it{{$}}
// OFF-DAG: note: 't2' stays on the host: no GEMM the systolic arrays can run was recognized in it{{$}}
// OFF-DAG: note: 't5' stays on the host: no GEMM the systolic arrays can run was recognized in it{{$}}
// OFF: wrote {{.*}} (2 function(s) offloaded

#include <stdio.h>
#include <string.h>

#include "systolic_offload.h"

/* T1: K = x1 (equal trips: the innermost), x0 a reduction batch loop offsetting both factors */
void t1(float A[restrict 3][7], float B[restrict 7][5], float C[restrict 3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int x0 = 0; x0 < 4; ++x0) for (int x1 = 0; x1 < 4; ++x1) C[i][j] += A[i][x0 - x1 + 3] * B[x0 + x1][j];
}

/* T2: K = x0, x1 (read by A only) a reduction batch loop */
void t2(float A[restrict 3][7], float B[restrict 4][5], float C[restrict 3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int x0 = 0; x0 < 4; ++x0) for (int x1 = 0; x1 < 4; ++x1) C[i][j] += A[i][x0 - x1 + 3] * B[x0][j];
}

/* T5: no loop read by both factors: K = 1, k and l reduction batch loops */
void t5(float A[restrict 3][4], float B[restrict 4][5], float C[restrict 3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) for (int l = 0; l < 4; ++l) C[i][j] += A[i][k] * B[l][j];
}

/* A[i][k0 - k2] * B[k0 + k1][j]: products repeat (k0+1, k1-1, k2+1 reads the same pair); every iteration still runs once */
void case1(float A[restrict 3][7], float B[restrict 7][5], float C[restrict 3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k0 = 0; k0 < 4; ++k0) for (int k1 = 0; k1 < 4; ++k1) for (int k2 = 0; k2 < 4; ++k2) C[i][j] += A[i][k0 - k2 + 3] * B[k0 + k1][j];
}

/* A[i][k0][k1] * B[k0 + k1][j]: K = k1, k0 a reduction batch loop */
void case2(float A[restrict 3][4][4], float B[restrict 7][5], float C[restrict 3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k0 = 0; k0 < 4; ++k0) for (int k1 = 0; k1 < 4; ++k1) C[i][j] += A[i][k0][k1] * B[k0 + k1][j];
}

/* A[i][k0 + 2k1] * B[k0 + k1][j]: K = k1 (A stride 2, B stride 1), k0 a reduction batch loop */
void coupled(float A[restrict 3][10], float B[restrict 7][5], float C[restrict 3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k0 = 0; k0 < 4; ++k0) for (int k1 = 0; k1 < 4; ++k1) C[i][j] += A[i][k0 + 2 * k1] * B[k0 + k1][j];
}

/* A[i][k0 + k1] * B[k0 + k1][j]: k0 + k1 is no dense K; K = k1, k0 a reduction batch loop */
void noninj(float A[restrict 3][7], float B[restrict 7][5], float C[restrict 3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k0 = 0; k0 < 4; ++k0) for (int k1 = 0; k1 < 4; ++k1) C[i][j] += A[i][k0 + k1] * B[k0 + k1][j];
}

/* two loops read by both factors: K = k (the longer), l a reduction batch loop */
void two_k(float A[restrict 3][4][2], float B[restrict 4][2][5], float C[restrict 3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) for (int l = 0; l < 2; ++l) C[i][j] += A[i][k][l] * B[k][l][j];
}

/* C[i][j] += A[r][i][k] * B[r][k][j]: K = k, r a reduction batch loop */
void partial(float A[restrict 2][3][4], float B[restrict 2][4][5], float C[restrict 3][5]) {
  for (int r = 0; r < 2; ++r) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[i][j] += A[r][i][k] * B[r][k][j];
}

/* a loop read by A only (s): a reduction batch loop */
void a_only(float A[restrict 3][2][4], float B[restrict 4][5], float C[restrict 3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int s = 0; s < 2; ++s) for (int k = 0; k < 4; ++k) C[i][j] += A[i][s][k] * B[k][j];
}

/* a loop read by B only (l): a reduction batch loop */
void b_only(float A[restrict 3][4], float B[restrict 4][3][5], float C[restrict 3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) for (int l = 0; l < 3; ++l) C[i][j] += A[i][k] * B[k][l][j];
}

/* an output batch loop (b, read by all three) and reduction batch loops (k, l) */
void ob_rb(float A[restrict 2][3][4], float B[restrict 2][4][5], float C[restrict 2][3][5]) {
  for (int b = 0; b < 2; ++b) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) for (int l = 0; l < 4; ++l) C[b][i][j] += A[b][i][k] * B[b][l][j];
}

/* a negative coefficient on a reduction batch loop: A[i][3 - k][l] * B[l][j] */
void rb_negative(float A[restrict 3][4][2], float B[restrict 2][5], float C[restrict 3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) for (int l = 0; l < 2; ++l) C[i][j] += A[i][3 - k][l] * B[l][j];
}

/* affine constants in every subscript: C[i + 1][j] += A[i + 1][k + 2] * B[l + 1][j + 1] */
void rb_constants(float A[restrict 4][6], float B[restrict 5][6], float C[restrict 4][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) for (int l = 0; l < 4; ++l) C[i + 1][j] += A[i + 1][k + 2] * B[l + 1][j + 1];
}

/* the assignment twin of T2 (C = A * B): x1 not read by C, only the last product survives -- stays on the host */
void t2_assign(float A[restrict 3][7], float B[restrict 4][5], float C[restrict 3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int x0 = 0; x0 < 4; ++x0) for (int x1 = 0; x1 < 4; ++x1) C[i][j] = A[i][x0 - x1 + 3] * B[x0][j];
}

/* assignment with an actual K = 1 (k runs once): considered, every element of C written once -- lowered into a zeroed C */
void assign_k1(float A[restrict 3][1], float B[restrict 1][5], float C[restrict 3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 1; ++k) C[i][j] = A[i][k] * B[k][j];
}

/* assignment C[b][i][j] = A[b][i] * B[b][j]: no reduction loop, index association dimension 0, virtual K = 1 -- lowered */
void assign_outer_batched(float A[restrict 2][3], float B[restrict 2][5], float C[restrict 2][3][5]) {
  for (int b = 0; b < 2; ++b) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) C[b][i][j] = A[b][i] * B[b][j];
}

/* the assignment twin of T5 (C = A * B with k, l not read by C): overwrites, refused */
void assign_t5(float A[restrict 3][4], float B[restrict 4][5], float C[restrict 3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) for (int l = 0; l < 4; ++l) C[i][j] = A[i][k] * B[l][j];
}

static float mul(float a, float b) { return a * b; }
static void t1_ref(float A[3][7], float B[7][5], float C[3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int x0 = 0; x0 < 4; ++x0) for (int x1 = 0; x1 < 4; ++x1) C[i][j] = C[i][j] + mul(A[i][x0 - x1 + 3], B[x0 + x1][j]); }
static void t2_ref(float A[3][7], float B[4][5], float C[3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int x0 = 0; x0 < 4; ++x0) for (int x1 = 0; x1 < 4; ++x1) C[i][j] = C[i][j] + mul(A[i][x0 - x1 + 3], B[x0][j]); }
static void t5_ref(float A[3][4], float B[4][5], float C[3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) for (int l = 0; l < 4; ++l) C[i][j] = C[i][j] + mul(A[i][k], B[l][j]); }
static void case1_ref(float A[3][7], float B[7][5], float C[3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k0 = 0; k0 < 4; ++k0) for (int k1 = 0; k1 < 4; ++k1) for (int k2 = 0; k2 < 4; ++k2) C[i][j] = C[i][j] + mul(A[i][k0 - k2 + 3], B[k0 + k1][j]); }
static void case2_ref(float A[3][4][4], float B[7][5], float C[3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k0 = 0; k0 < 4; ++k0) for (int k1 = 0; k1 < 4; ++k1) C[i][j] = C[i][j] + mul(A[i][k0][k1], B[k0 + k1][j]); }
static void coupled_ref(float A[3][10], float B[7][5], float C[3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k0 = 0; k0 < 4; ++k0) for (int k1 = 0; k1 < 4; ++k1) C[i][j] = C[i][j] + mul(A[i][k0 + 2 * k1], B[k0 + k1][j]); }
static void noninj_ref(float A[3][7], float B[7][5], float C[3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k0 = 0; k0 < 4; ++k0) for (int k1 = 0; k1 < 4; ++k1) C[i][j] = C[i][j] + mul(A[i][k0 + k1], B[k0 + k1][j]); }
static void two_k_ref(float A[3][4][2], float B[4][2][5], float C[3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) for (int l = 0; l < 2; ++l) C[i][j] = C[i][j] + mul(A[i][k][l], B[k][l][j]); }
static void partial_ref(float A[2][3][4], float B[2][4][5], float C[3][5]) {
  for (int r = 0; r < 2; ++r) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[i][j] = C[i][j] + mul(A[r][i][k], B[r][k][j]); }
static void a_only_ref(float A[3][2][4], float B[4][5], float C[3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int s = 0; s < 2; ++s) for (int k = 0; k < 4; ++k) C[i][j] = C[i][j] + mul(A[i][s][k], B[k][j]); }
static void b_only_ref(float A[3][4], float B[4][3][5], float C[3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) for (int l = 0; l < 3; ++l) C[i][j] = C[i][j] + mul(A[i][k], B[k][l][j]); }
static void ob_rb_ref(float A[2][3][4], float B[2][4][5], float C[2][3][5]) {
  for (int b = 0; b < 2; ++b) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) for (int l = 0; l < 4; ++l) C[b][i][j] = C[b][i][j] + mul(A[b][i][k], B[b][l][j]); }
static void rb_negative_ref(float A[3][4][2], float B[2][5], float C[3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) for (int l = 0; l < 2; ++l) C[i][j] = C[i][j] + mul(A[i][3 - k][l], B[l][j]); }
static void rb_constants_ref(float A[4][6], float B[5][6], float C[4][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) for (int l = 0; l < 4; ++l) C[i + 1][j] = C[i + 1][j] + mul(A[i + 1][k + 2], B[l + 1][j + 1]); }
static void t2_assign_ref(float A[3][7], float B[4][5], float C[3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int x0 = 0; x0 < 4; ++x0) for (int x1 = 0; x1 < 4; ++x1) C[i][j] = mul(A[i][x0 - x1 + 3], B[x0][j]); }
static void assign_k1_ref(float A[3][1], float B[1][5], float C[3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 1; ++k) C[i][j] = mul(A[i][k], B[k][j]); }
static void assign_outer_batched_ref(float A[2][3], float B[2][5], float C[2][3][5]) {
  for (int b = 0; b < 2; ++b) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) C[b][i][j] = mul(A[b][i], B[b][j]); }
static void assign_t5_ref(float A[3][4], float B[4][5], float C[3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) for (int l = 0; l < 4; ++l) C[i][j] = mul(A[i][k], B[l][j]); }

/* Integer-valued data (results bitwise equal); with -DFRACTIONAL non-integer
   data, whose sums depend on the order of the additions. */
#ifdef FRACTIONAL
#define SCALE(x) ((x) * 0.3f + 0.011f)
#else
#define SCALE(x) (x)
#endif
static void fill(float *p, int n, int s) { for (int e = 0; e < n; ++e) p[e] = SCALE((float)((e * 7 + s) % 9 - 4)); }
static void fill_nz(float *p, int n, int s) { for (int e = 0; e < n; ++e) { int v = (e * 7 + s) % 9 - 4; p[e] = SCALE((float)(v ? v : 5 - 2 * (e & 1))); } }
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
  RUN(t1, 21, 35, 15);
  RUN(t2, 21, 20, 15);
  RUN(t5, 12, 20, 15);
  RUN(case1, 21, 35, 15);
  RUN(case2, 48, 35, 15);
  RUN(coupled, 30, 35, 15);
  RUN(noninj, 21, 35, 15);
  RUN(two_k, 24, 40, 15);
  RUN(partial, 24, 40, 15);
  RUN(a_only, 24, 20, 15);
  RUN(b_only, 12, 60, 15);
  RUN(ob_rb, 24, 40, 30);
  RUN(rb_negative, 24, 10, 15);
  RUN(rb_constants, 24, 30, 20);
  RUN(t2_assign, 21, 20, 15);
  RUN(assign_k1, 3, 5, 15);
  RUN(assign_outer_batched, 6, 10, 30);
  RUN(assign_t5, 12, 20, 15);
  printf("%s: %d differ\n", differ ? "FAIL" : "PASS", differ);
#ifdef FRACTIONAL
  return 0; /* the --verify shim checks the tolerance */
#else
  return differ;
#endif
}
