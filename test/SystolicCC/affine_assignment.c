// REQUIRES: cfront-clang
//
// Assignment nests C = A * B (docs/AFFINE_GEMM_MAPPING.md section 20): a nest
// that writes every element of C exactly once from one product is the K = 1
// GEMM into a zeroed C, C = 0 + A B, and is lowered as that -- C's window is
// zeroed before the tiles accumulate into it. The rank-1 outer products the
// task of 2026-10-19 names first, with their accumulation twins, permuted
// outputs, mixed ranks, constant and one-iteration reductions, rectangular
// shapes (1 x 7, 7 x 1, 9 x 13 over six tiles), offsets, strides, a strided
// and a skewed and a flattened C, transposed rank-2 factors, a batched and a
// repeated nest. Each is compared bitwise with a CPU reference that runs the
// loops as written on integer-valued data (the factors without zeros: a -0
// product would come back as +0, section 6) with a NONZERO initial C: an
// assignment must not keep it, an accumulation must; `semantics` checks the
// two forms against each other on the same inputs.
//
// The nests that must stay on the host: an assignment inside a reduction loop
// of four iterations (every product but the last is lost), batches that
// overlap in C, an inner-dimension overrun (compiled, not executed: undefined
// in C), a factor that may alias C, an element-wise product, a product plus a
// constant. (bcast_assign, C[i][j] = A[0] * B[j], is offloaded since
// 2026-10-19: i is read by C only, a partition with both factors shared --
// docs section 21 -- three slabs of a 1 x 5 x 1 assignment.)
//
// NO REAL HARDWARE HERE: the software runtime model computes every tile;
// SOFT_RUNTIME_MOCK_HARDWARE=1 makes it claim to be the FPGA.
//
// RUN: %systolic-cc --fp-reassociation --explain %s -o %t 2>&1 | FileCheck %s --check-prefix=PLAN
// PLAN-DAG: note: 'op_assign' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'op_assign_t' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'op_acc' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'op_acc_t' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'perm_assign' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'perm_assign_swap' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'mixed_assign' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'mixed_acc' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'k1_assign' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'k1_loop_assign' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'm1_assign' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'n1_assign' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'big_assign' is offloaded: 6 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'off_assign' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'stride_assign' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'c_stride_assign' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'transposed_assign' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'flat_assign' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'skew_assign' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'rev_assign' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'batched_assign' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 4 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'rep_assign' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 4 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'overwrite_k' stays on the host: no GEMM the systolic arrays can run was recognized in it{{$}}
// PLAN-DAG: note: 'overlap_assign' stays on the host: no GEMM the systolic arrays can run was recognized in it (line 183: two iterations of the assignment C = A * B write the same element of C (its subscripts are not injective on the iteration domain): the later product overwrites the earlier one -- not the GEMM of A and B (docs/AFFINE_GEMM_MAPPING.md section 20))
// PLAN-DAG: note: 'oob_assign' stays on the host: no GEMM the systolic arrays can run was recognized in it (line 186: A is accessed out of bounds along dimension 1)
// PLAN-DAG: note: 'alias_assign' stays on the host: no GEMM the systolic arrays can run was recognized in it (line 189: A may overlap C; declare the arguments llvm.noalias (C restrict) or pass assume-no-alias)
// PLAN-DAG: note: 'elementwise' stays on the host: no GEMM the systolic arrays can run was recognized in it{{$}}
// PLAN-DAG: note: 'bcast_assign' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 6 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'plus_const' stays on the host: no GEMM the systolic arrays can run was recognized in it{{$}}
// PLAN: wrote {{.*}} (23 function(s) offloaded
//
// RUN: %t > %t.soft
// RUN: FileCheck %s --check-prefix=ALL --input-file=%t.soft
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t > %t.mock
// RUN: FileCheck %s --check-prefix=ALL --input-file=%t.mock
// ALL: [case] op_assign: equal, offloaded (2)
// ALL: [case] op_assign_t: equal, offloaded (2)
// ALL: [case] op_acc: equal, offloaded (2)
// ALL: [case] op_acc_t: equal, offloaded (2)
// ALL: [case] perm_assign: equal, offloaded (2)
// ALL: [case] perm_assign_swap: equal, offloaded (2)
// ALL: [case] mixed_assign: equal, offloaded (2)
// ALL: [case] mixed_acc: equal, offloaded (2)
// ALL: [case] k1_assign: equal, offloaded (2)
// ALL: [case] k1_loop_assign: equal, offloaded (2)
// ALL: [case] m1_assign: equal, offloaded (2)
// ALL: [case] n1_assign: equal, offloaded (2)
// ALL: [case] big_assign: equal, offloaded (6)
// ALL: [case] off_assign: equal, offloaded (2)
// ALL: [case] stride_assign: equal, offloaded (2)
// ALL: [case] c_stride_assign: equal, offloaded (2)
// ALL: [case] transposed_assign: equal, offloaded (2)
// ALL: [case] flat_assign: equal, offloaded (2)
// ALL: [case] skew_assign: equal, offloaded (2)
// ALL: [case] rev_assign: equal, offloaded (2)
// ALL: [case] batched_assign: equal, offloaded (4)
// ALL: [case] rep_assign: equal, offloaded (4)
// ALL: [case] overwrite_k: equal, NOT OFFLOADED (0)
// ALL: [case] overlap_assign: equal, NOT OFFLOADED (0)
// ALL: [case] alias_assign: equal, NOT OFFLOADED (0)
// ALL: [case] elementwise: equal, NOT OFFLOADED (0)
// ALL: [case] bcast_assign: equal, offloaded (6)
// ALL: [case] plus_const: equal, NOT OFFLOADED (0)
// ALL: [semantics] assignment vs accumulation on the same inputs: consistent (C_acc - C_initial == C_assign, 15 elements)
// ALL: PASS: 0 differ
//
// Every offloaded call also against the function as written, on the CPU:
// RUN: %systolic-cc --fp-reassociation --verify %s -o %t.v > /dev/null 2>&1
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t.v 2>&1 >/dev/null | FileCheck %s --check-prefix=VERIFY
// VERIFY-COUNT-23: verified: {{[0-9]+}} elements equal the CPU reference (bitwise)
// VERIFY-NOT: MISMATCH
//
// Without the permission to reassociate (the strict default), every assignment
// stays on the host like every accumulation: the tiles start from +0.
// RUN: %systolic-cc --explain %s -o %t.s 2>&1 | FileCheck %s --check-prefix=STRICT
// STRICT: note: 'op_assign' stays on the host: its GEMM is offloaded only with permission to reassociate floating-point sums (--fp-reassociation)
// STRICT: wrote {{.*}} (0 function(s) offloaded

#include <stdio.h>
#include <string.h>

#include "systolic_offload.h"

void op_assign(float A[restrict 3], float B[restrict 5], float C[restrict 3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) C[i][j] = A[i] * B[j];
}
void op_assign_t(float A[restrict 5], float B[restrict 3], float C[restrict 3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) C[i][j] = A[j] * B[i];
}
void op_acc(float A[restrict 3], float B[restrict 5], float C[restrict 3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) C[i][j] += A[i] * B[j];
}
void op_acc_t(float A[restrict 5], float B[restrict 3], float C[restrict 3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) C[i][j] += A[j] * B[i];
}
void perm_assign(float A[restrict 3], float B[restrict 5], float C[restrict 5][3]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) C[j][i] = A[i] * B[j];
}
void perm_assign_swap(float A[restrict 5], float B[restrict 3], float C[restrict 5][3]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) C[j][i] = A[j] * B[i];
}
void mixed_assign(float A[restrict 3], float B[restrict 1][5], float C[restrict 3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) C[i][j] = A[i] * B[0][j];
}
void mixed_acc(float A[restrict 3][1], float B[restrict 5], float C[restrict 3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) C[i][j] += A[i][0] * B[j];
}
void k1_assign(float A[restrict 3][2], float B[restrict 2][5], float C[restrict 3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) C[i][j] = A[i][1] * B[0][j];
}
void k1_loop_assign(float A[restrict 3][2], float B[restrict 2][5], float C[restrict 3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 1; k < 2; ++k) C[i][j] = A[i][k] * B[k][j];
}
void m1_assign(float A[restrict 1], float B[restrict 7], float C[restrict 1][7]) {
  for (int i = 0; i < 1; ++i) for (int j = 0; j < 7; ++j) C[i][j] = A[i] * B[j];
}
void n1_assign(float A[restrict 7], float B[restrict 1], float C[restrict 7][1]) {
  for (int i = 0; i < 7; ++i) for (int j = 0; j < 1; ++j) C[i][j] = A[i] * B[j];
}
void big_assign(float A[restrict 9], float B[restrict 13], float C[restrict 9][13]) {
  for (int i = 0; i < 9; ++i) for (int j = 0; j < 13; ++j) C[i][j] = A[i] * B[j];
}
void off_assign(float A[restrict 5], float B[restrict 8], float C[restrict 3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) C[i][j] = A[i + 1] * B[j + 2];
}
void stride_assign(float A[restrict 6], float B[restrict 15], float C[restrict 3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) C[i][j] = A[2 * i] * B[3 * j];
}
void c_stride_assign(float A[restrict 3], float B[restrict 5], float C[restrict 6][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) C[2 * i][j] = A[i] * B[j];
}
void transposed_assign(float A[restrict 2][3], float B[restrict 5][2], float C[restrict 3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) C[i][j] = A[0][i] * B[j][1];
}
void flat_assign(float A[restrict 3], float B[restrict 5], float C[restrict 15]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) C[5 * i + j] = A[i] * B[j];
}
void skew_assign(float A[restrict 3], float B[restrict 5], float C[restrict 3][8]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) C[i][i + j] = A[i] * B[j];
}
void rev_assign(float A[restrict 3], float B[restrict 5], float C[restrict 3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) C[2 - i][4 - j] = A[i] * B[j];
}
void batched_assign(float A[restrict 2][3], float B[restrict 2][5], float C[restrict 2][3][5]) {
  for (int b = 0; b < 2; ++b) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) C[b][i][j] = A[b][i] * B[b][j];
}
void rep_assign(float A[restrict 3], float B[restrict 5], float C[restrict 3][5]) {
  for (int r = 0; r < 2; ++r) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) C[i][j] = A[i] * B[j];
}
/* must stay on the host */
void overwrite_k(float A[restrict 3][4], float B[restrict 4][5], float C[restrict 3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[i][j] = A[i][k] * B[k][j];
}
void overlap_assign(float A[restrict 2][3], float B[restrict 2][5], float C[restrict 4][5]) {
  for (int b = 0; b < 2; ++b) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) C[i + b][j] = A[b][i] * B[b][j];
}
void oob_assign(float A[restrict 3][4], float B[restrict 5], float C[restrict 3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) C[i][j] = A[i][4] * B[j];
}
void alias_assign(float A[3][5], float B[restrict 5], float C[restrict 3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) A[i][j] = A[i][0] * B[j];
}
void elementwise(float A[restrict 5], float B[restrict 5], float C[restrict 5]) {
  for (int i = 0; i < 5; ++i) C[i] = A[i] * B[i];
}
/* a C-only loop: a partition (section 21), both factors shared */
void bcast_assign(float A[restrict 3], float B[restrict 5], float C[restrict 3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) C[i][j] = A[0] * B[j];
}
void plus_const(float A[restrict 3], float B[restrict 5], float C[restrict 3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) C[i][j] = A[i] * B[j] + 1.0f;
}

static float mul(float a, float b) { return a * b; }
static void op_assign_ref(float A[3], float B[5], float C[3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) C[i][j] = mul(A[i], B[j]); }
static void op_assign_t_ref(float A[5], float B[3], float C[3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) C[i][j] = mul(A[j], B[i]); }
static void op_acc_ref(float A[3], float B[5], float C[3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) C[i][j] = C[i][j] + mul(A[i], B[j]); }
static void op_acc_t_ref(float A[5], float B[3], float C[3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) C[i][j] = C[i][j] + mul(A[j], B[i]); }
static void perm_assign_ref(float A[3], float B[5], float C[5][3]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) C[j][i] = mul(A[i], B[j]); }
static void perm_assign_swap_ref(float A[5], float B[3], float C[5][3]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) C[j][i] = mul(A[j], B[i]); }
static void mixed_assign_ref(float A[3], float B[1][5], float C[3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) C[i][j] = mul(A[i], B[0][j]); }
static void mixed_acc_ref(float A[3][1], float B[5], float C[3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) C[i][j] = C[i][j] + mul(A[i][0], B[j]); }
static void k1_assign_ref(float A[3][2], float B[2][5], float C[3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) C[i][j] = mul(A[i][1], B[0][j]); }
static void k1_loop_assign_ref(float A[3][2], float B[2][5], float C[3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 1; k < 2; ++k) C[i][j] = mul(A[i][k], B[k][j]); }
static void m1_assign_ref(float A[1], float B[7], float C[1][7]) {
  for (int i = 0; i < 1; ++i) for (int j = 0; j < 7; ++j) C[i][j] = mul(A[i], B[j]); }
static void n1_assign_ref(float A[7], float B[1], float C[7][1]) {
  for (int i = 0; i < 7; ++i) for (int j = 0; j < 1; ++j) C[i][j] = mul(A[i], B[j]); }
static void big_assign_ref(float A[9], float B[13], float C[9][13]) {
  for (int i = 0; i < 9; ++i) for (int j = 0; j < 13; ++j) C[i][j] = mul(A[i], B[j]); }
static void off_assign_ref(float A[5], float B[8], float C[3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) C[i][j] = mul(A[i + 1], B[j + 2]); }
static void stride_assign_ref(float A[6], float B[15], float C[3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) C[i][j] = mul(A[2 * i], B[3 * j]); }
static void c_stride_assign_ref(float A[3], float B[5], float C[6][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) C[2 * i][j] = mul(A[i], B[j]); }
static void transposed_assign_ref(float A[2][3], float B[5][2], float C[3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) C[i][j] = mul(A[0][i], B[j][1]); }
static void flat_assign_ref(float A[3], float B[5], float C[15]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) C[5 * i + j] = mul(A[i], B[j]); }
static void skew_assign_ref(float A[3], float B[5], float C[3][8]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) C[i][i + j] = mul(A[i], B[j]); }
static void rev_assign_ref(float A[3], float B[5], float C[3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) C[2 - i][4 - j] = mul(A[i], B[j]); }
static void batched_assign_ref(float A[2][3], float B[2][5], float C[2][3][5]) {
  for (int b = 0; b < 2; ++b) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) C[b][i][j] = mul(A[b][i], B[b][j]); }
static void rep_assign_ref(float A[3], float B[5], float C[3][5]) {
  for (int r = 0; r < 2; ++r) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) C[i][j] = mul(A[i], B[j]); }
static void overwrite_k_ref(float A[3][4], float B[4][5], float C[3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[i][j] = mul(A[i][k], B[k][j]); }
static void overlap_assign_ref(float A[2][3], float B[2][5], float C[4][5]) {
  for (int b = 0; b < 2; ++b) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) C[i + b][j] = mul(A[b][i], B[b][j]); }
static void alias_assign_ref(float A[3][5], float B[5], float C[3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) A[i][j] = mul(A[i][0], B[j]); }
static void elementwise_ref(float A[5], float B[5], float C[5]) {
  for (int i = 0; i < 5; ++i) C[i] = mul(A[i], B[i]); }
static void bcast_assign_ref(float A[3], float B[5], float C[3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) C[i][j] = mul(A[0], B[j]); }
static void plus_const_ref(float A[3], float B[5], float C[3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) C[i][j] = mul(A[i], B[j]) + 1.0f; }

static void fill(float *p, int n, int s) { for (int e = 0; e < n; ++e) p[e] = (float)((e * 7 + s) % 9 - 4); }
/* The factors: integer values in [-4, 4] without 0. A zero factor with a
   negative one gives the product -0, which the tiles (summed from +0) return
   as +0 -- the documented deviation of docs/AFFINE_GEMM_MAPPING.md section 6,
   shown in affine_assignment_signzero.c; here every product is nonzero. */
static void fill_nz(float *p, int n, int s) { for (int e = 0; e < n; ++e) { int v = (e * 7 + s) % 9 - 4; p[e] = (float)(v ? v : 5 - 2 * (e & 1)); } }
static int differ;
static unsigned long long inv(void) { systolic_offload_stats_t s; systolic_offload_stats(&s); return s.per_device[0]+s.per_device[1]+s.per_device[2]+s.per_device[3]; }
static void report(const char *n, const void *g, const void *w, size_t by, unsigned long long i) {
  int same = memcmp(g, w, by) == 0; differ += !same;
  printf("[case] %s: %s, %s (%llu)\n", n, same ? "equal" : "DIFFERENT", i ? "offloaded" : "NOT OFFLOADED", i);
}
#define RUN(name, NA, NB, NC, OUT) do { static float A[NA], B[NB], C[NC], RA[NA], RB[NB], RC[NC]; \
  fill_nz(A, NA, 1); fill_nz(B, NB, 2); fill(C, NC, 3); memcpy(RA, A, sizeof A); memcpy(RB, B, sizeof B); memcpy(RC, C, sizeof C); \
  unsigned long long before = inv(); name((void*)A,(void*)B,(void*)C); unsigned long long n = inv() - before; \
  name##_ref((void*)RA,(void*)RB,(void*)RC); \
  if (OUT == 0) report(#name, A, RA, sizeof A, n); else report(#name, C, RC, sizeof C, n); } while (0)
/* The two forms on the same inputs and the same nonzero initial C: the
   accumulation's result minus the initial C is the assignment's result
   (exact: integer-valued data). */
static void semantics(void) {
  static float A[3], B[5], C0[3][5], Ca[3][5], Cs[3][5];
  fill_nz(A, 3, 1); fill_nz(B, 5, 2); fill(&C0[0][0], 15, 3);
  memcpy(Ca, C0, sizeof Ca); memcpy(Cs, C0, sizeof Cs);
  op_acc(A, B, Ca); op_assign(A, B, Cs);
  int bad = 0;
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) bad += (Ca[i][j] - C0[i][j]) != Cs[i][j];
  differ += bad != 0;
  printf("[semantics] assignment vs accumulation on the same inputs: %s (C_acc - C_initial == C_assign, 15 elements)\n",
         bad ? "INCONSISTENT" : "consistent");
}
int main(void) {
  RUN(op_assign, 3, 5, 3*5, 1);
  RUN(op_assign_t, 5, 3, 3*5, 1);
  RUN(op_acc, 3, 5, 3*5, 1);
  RUN(op_acc_t, 5, 3, 3*5, 1);
  RUN(perm_assign, 3, 5, 5*3, 1);
  RUN(perm_assign_swap, 5, 3, 5*3, 1);
  RUN(mixed_assign, 3, 1*5, 3*5, 1);
  RUN(mixed_acc, 3*1, 5, 3*5, 1);
  RUN(k1_assign, 3*2, 2*5, 3*5, 1);
  RUN(k1_loop_assign, 3*2, 2*5, 3*5, 1);
  RUN(m1_assign, 1, 7, 1*7, 1);
  RUN(n1_assign, 7, 1, 7*1, 1);
  RUN(big_assign, 9, 13, 9*13, 1);
  RUN(off_assign, 5, 8, 3*5, 1);
  RUN(stride_assign, 6, 15, 3*5, 1);
  RUN(c_stride_assign, 3, 5, 6*5, 1);
  RUN(transposed_assign, 2*3, 5*2, 3*5, 1);
  RUN(flat_assign, 3, 5, 15, 1);
  RUN(skew_assign, 3, 5, 3*8, 1);
  RUN(rev_assign, 3, 5, 3*5, 1);
  RUN(batched_assign, 2*3, 2*5, 2*3*5, 1);
  RUN(rep_assign, 3, 5, 3*5, 1);
  RUN(overwrite_k, 3*4, 4*5, 3*5, 1);
  RUN(overlap_assign, 2*3, 2*5, 4*5, 1);
  RUN(alias_assign, 3*5, 5, 3*5, 0);
  RUN(elementwise, 5, 5, 5, 1);
  RUN(bcast_assign, 3, 5, 3*5, 1);
  RUN(plus_const, 3, 5, 3*5, 1);
  semantics();
  printf("%s: %d differ\n", differ ? "FAIL" : "PASS", differ); return differ;
}
