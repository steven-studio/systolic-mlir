// REQUIRES: cfront-clang
//
// Pre-reduction of the factors (docs/AFFINE_GEMM_MAPPING.md section 24,
// Lemma 7.6; systolic-cc --fp-distribute, off by default): in an accumulation
// C += A * B on a box whose factors share no loop C does not read, the loops
// read by one factor only are summed into that factor first (A', B' computed
// on the host), and the GEMM is C += A' * B' with K = 1. --fp-distribute is
// the explicit permission to take floating point as commutative, associative
// and distributive; --fp-reassociation does not imply it. The results are NOT
// bitwise those of the loops as written, so this is a numerical differential
// test: each kernel runs against a CPU reference that runs the loops as
// written (one rounded product and one rounded addition per iteration, in
// order), and every case prints its maximum absolute error, its maximum
// relative error, and whether every element is within the tolerance
//   |result - reference| <= 1e-5 * M + 1e-30,
// M the element's magnitude sum |C before| + sum of |product| over its terms
// (derivation at TOL_REL below). The checks pin within_tol=yes and the
// invocation counts, never the error values.
//
// Data: non-integer values of both signs by default; with -DINTEGER
// integer-valued data, on which every partial sum is exact, so pre-reduction
// must be bitwise equal to the loops as written. The case t2 (a loop read by
// both factors, L_AB nonempty) is not pre-reduced and keeps the reduction-batch
// lowering of section 23; the assignment assign_t5 is never pre-reduced (last
// write wins, section 20.5, with --fp-reassociation).
//
// NO REAL HARDWARE HERE: the software runtime model computes every tile;
// SOFT_RUNTIME_MOCK_HARDWARE=1 makes it claim to be the FPGA.
//
// RUN: %systolic-cc --fp-distribute --fp-reassociation --explain %s -o %t 2>&1 | FileCheck %s --check-prefix=PLAN
// PLAN-DAG: note: 't5' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'b_only' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'b_only_ik' is offloaded: 1 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 4 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'a_only' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'ob' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 4 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'neg_const' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 'stepped_sum' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN-DAG: note: 't2' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 8 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'assign_t5' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// PLAN: wrote {{.*}} (9 function(s) offloaded
//
// The pass's remarks (kept in PROGRAM.systolic/): seven nests pre-reduced, t2
// and the assignment not.
// RUN: FileCheck %s --check-prefix=PRE --input-file=%t.systolic/20-remarks.txt
// RUN: grep -c "nest pre-reduced" %t.systolic/20-remarks.txt | FileCheck %s --check-prefix=NPRE
// NPRE: {{^}}7{{$}}
//
// Non-integer data: within the tolerance, the counts of the pre-reduced plan.
// RUN: %t > %t.frac
// RUN: FileCheck %s --check-prefix=FRAC --input-file=%t.frac
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t > %t.frac.mock
// RUN: FileCheck %s --check-prefix=FRAC --input-file=%t.frac.mock
// The mock claims the FPGA ran every call: no shim fell back to the CPU copy,
// the results above are the pre-reduced tiles'.
// RUN: FileCheck %s --check-prefix=MOCK --input-file=%t.frac.mock
// MOCK-COUNT-9: [systolic-cc] FPGA Executed: GEMM #
// MOCK-NOT: CPU Executed
// FRAC: [case] t5: max_abs_err={{[0-9.e+-]+}} max_rel_err={{[0-9.e+-]+}} within_tol=yes, offloaded (2)
// FRAC: [case] b_only: max_abs_err={{[0-9.e+-]+}} max_rel_err={{[0-9.e+-]+}} within_tol=yes, offloaded (2)
// FRAC: [case] b_only_ik: max_abs_err={{[0-9.e+-]+}} max_rel_err={{[0-9.e+-]+}} within_tol=yes, offloaded (4)
// FRAC: [case] a_only: max_abs_err={{[0-9.e+-]+}} max_rel_err={{[0-9.e+-]+}} within_tol=yes, offloaded (2)
// FRAC: [case] ob: max_abs_err={{[0-9.e+-]+}} max_rel_err={{[0-9.e+-]+}} within_tol=yes, offloaded (4)
// FRAC: [case] neg_const: max_abs_err={{[0-9.e+-]+}} max_rel_err={{[0-9.e+-]+}} within_tol=yes, offloaded (2)
// FRAC: [case] stepped_sum: max_abs_err={{[0-9.e+-]+}} max_rel_err={{[0-9.e+-]+}} within_tol=yes, offloaded (2)
// FRAC: [case] t2: max_abs_err={{[0-9.e+-]+}} max_rel_err={{[0-9.e+-]+}} within_tol=yes, offloaded (8)
// FRAC: [case] assign_t5: max_abs_err={{[0-9.e+-]+}} max_rel_err={{[0-9.e+-]+}} within_tol=yes, offloaded (2)
// FRAC: PASS: 0 case(s) out of tolerance
//
// Integer-valued data: pre-reduction is exact, bitwise equal.
// RUN: %systolic-cc --fp-distribute --fp-reassociation -DINTEGER %s -o %t.int > /dev/null 2>&1
// RUN: %t.int | FileCheck %s --check-prefix=INT
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t.int | FileCheck %s --check-prefix=INT
// INT: [case] t5: max_abs_err=0 max_rel_err=0 within_tol=yes, offloaded (2)
// INT-NEXT: [bitwise] t5: equal
// INT: [case] b_only: max_abs_err=0 max_rel_err=0 within_tol=yes, offloaded (2)
// INT-NEXT: [bitwise] b_only: equal
// INT: [case] b_only_ik: max_abs_err=0 max_rel_err=0 within_tol=yes, offloaded (4)
// INT-NEXT: [bitwise] b_only_ik: equal
// INT: [case] a_only: max_abs_err=0 max_rel_err=0 within_tol=yes, offloaded (2)
// INT-NEXT: [bitwise] a_only: equal
// INT: [case] ob: max_abs_err=0 max_rel_err=0 within_tol=yes, offloaded (4)
// INT-NEXT: [bitwise] ob: equal
// INT: [case] neg_const: max_abs_err=0 max_rel_err=0 within_tol=yes, offloaded (2)
// INT-NEXT: [bitwise] neg_const: equal
// INT: [case] stepped_sum: max_abs_err=0 max_rel_err=0 within_tol=yes, offloaded (2)
// INT-NEXT: [bitwise] stepped_sum: equal
// INT: [case] t2: max_abs_err=0 max_rel_err=0 within_tol=yes, offloaded (8)
// INT-NEXT: [bitwise] t2: equal
// INT: [case] assign_t5: max_abs_err=0 max_rel_err=0 within_tol=yes, offloaded (2)
// INT-NEXT: [bitwise] assign_t5: equal
// INT: PASS: 0 case(s) out of tolerance or not bitwise equal
//
// The --verify shim (each offloaded call against the function as written, on
// the CPU): bitwise on integer-valued data, within a relative tolerance per
// element on the non-integer data.
// RUN: %systolic-cc --fp-distribute --fp-reassociation -DINTEGER --verify %s -o %t.v > /dev/null 2>&1
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t.v 2>&1 >/dev/null | FileCheck %s --check-prefix=VERIFY
// VERIFY-COUNT-9: verified: {{[0-9]+}} elements equal the CPU reference (bitwise)
// VERIFY-NOT: MISMATCH
// RUN: %systolic-cc --fp-distribute --fp-reassociation --verify --verify-rtol 1e-4 %s -o %t.vf > /dev/null 2>&1
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t.vf 2>&1 >/dev/null | FileCheck %s --check-prefix=RTOL
// RTOL-COUNT-9: verified: {{[0-9]+}} elements equal the CPU reference (relative tolerance 0.0001)
// RTOL-NOT: MISMATCH
// The same with the asynchronous submission (the transfer-thread runtime):
// the program's own differential check on the non-integer data, and the
// --verify shim, bitwise on integer-valued data and within the relative
// tolerance on the non-integer data.
// RUN: %systolic-cc --fp-distribute --fp-reassociation --submission async %s -o %t.async > /dev/null 2>&1
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t.async > %t.async.frac
// RUN: FileCheck %s --check-prefix=FRAC --input-file=%t.async.frac
// RUN: %systolic-cc --fp-distribute --fp-reassociation --submission async -DINTEGER --verify %s -o %t.va > /dev/null 2>&1
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t.va 2>&1 >/dev/null | FileCheck %s --check-prefix=VERIFY
// RUN: %systolic-cc --fp-distribute --fp-reassociation --submission async --verify --verify-rtol 1e-4 %s -o %t.vfa > /dev/null 2>&1
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t.vfa 2>&1 >/dev/null | FileCheck %s --check-prefix=RTOL
//
// Without --fp-distribute: nothing pre-reduced, the reduction-batch lowering
// (more invocations), bitwise equal on integer-valued data.
// RUN: %systolic-cc --fp-reassociation -DINTEGER --explain %s -o %t.off 2>&1 | FileCheck %s --check-prefix=OFFPLAN
// OFFPLAN-DAG: note: 't5' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 32 invocation(s) per call (the loops around them repeat them)
// OFFPLAN-DAG: note: 'ob' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 64 invocation(s) per call (the loops around them repeat them)
// OFFPLAN: wrote {{.*}} (9 function(s) offloaded
// RUN: not grep -q pre-reduced %t.off.systolic/20-remarks.txt
// RUN: %t.off | FileCheck %s --check-prefix=OFF
// OFF: [case] t5: max_abs_err=0 max_rel_err=0 within_tol=yes, offloaded (32)
// OFF-NEXT: [bitwise] t5: equal
// OFF: [case] b_only: max_abs_err=0 max_rel_err=0 within_tol=yes, offloaded (8)
// OFF-NEXT: [bitwise] b_only: equal
// OFF: [case] b_only_ik: max_abs_err=0 max_rel_err=0 within_tol=yes, offloaded (20)
// OFF-NEXT: [bitwise] b_only_ik: equal
// OFF: [case] a_only: max_abs_err=0 max_rel_err=0 within_tol=yes, offloaded (16)
// OFF-NEXT: [bitwise] a_only: equal
// OFF: [case] ob: max_abs_err=0 max_rel_err=0 within_tol=yes, offloaded (64)
// OFF-NEXT: [bitwise] ob: equal
// OFF: [case] neg_const: max_abs_err=0 max_rel_err=0 within_tol=yes, offloaded (32)
// OFF-NEXT: [bitwise] neg_const: equal
// OFF: [case] stepped_sum: max_abs_err=0 max_rel_err=0 within_tol=yes, offloaded (32)
// OFF-NEXT: [bitwise] stepped_sum: equal
// OFF: [case] t2: max_abs_err=0 max_rel_err=0 within_tol=yes, offloaded (8)
// OFF-NEXT: [bitwise] t2: equal
// OFF: [case] assign_t5: max_abs_err=0 max_rel_err=0 within_tol=yes, offloaded (2)
// OFF-NEXT: [bitwise] assign_t5: equal
// OFF: PASS: 0 case(s) out of tolerance or not bitwise equal
//
// Strict floating point (no option): nothing pre-reduced, nothing cut, nothing
// offloaded.
// RUN: %systolic-cc --explain %s -o %t.strict 2>&1 | FileCheck %s --check-prefix=STRICT
// STRICT-DAG: note: 't5' stays on the host: its GEMM is offloaded only with permission to reassociate floating-point sums
// STRICT-DAG: note: 'assign_t5' stays on the host: no GEMM the systolic arrays can run was recognized in it{{$}}
// STRICT: wrote {{.*}} (0 function(s) offloaded
// RUN: not grep -q pre-reduced %t.strict.systolic/20-remarks.txt
// Nor is the assignment cut (last write wins runs only on a nest that may be
// lowered, section 20.5): it is refused at recognition as written.
// RUN: not grep -q "last write wins" %t.strict.systolic/20-remarks.txt
//
// --fp-distribute alone: the pre-reduced nests carry reassoc (the permission
// covers the reordered sums) and are offloaded; t2 and the assignment, not
// pre-reduced, still need --fp-reassociation (the assignment is not even cut
// without it: no GEMM is recognized in it as written).
// RUN: %systolic-cc --fp-distribute --explain %s -o %t.dist 2>&1 | FileCheck %s --check-prefix=DIST
// DIST-DAG: note: 't5' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet{{$}}
// DIST-DAG: note: 't2' stays on the host: its GEMM is offloaded only with permission to reassociate floating-point sums
// DIST-DAG: note: 'assign_t5' stays on the host: no GEMM the systolic arrays can run was recognized in it{{$}}
// DIST: wrote {{.*}} (7 function(s) offloaded
// RUN: not grep -q "last write wins" %t.dist.systolic/20-remarks.txt

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "systolic_offload.h"

/* T5: both factors pre-reduced (A' over k, B' over l), then a K = 1 GEMM */
// PRE-DAG: :[[@LINE+2]]:3: remark: affine multiply-accumulate nest pre-reduced (--fp-distribute, docs/AFFINE_GEMM_MAPPING.md section 24): the first factor summed over loop(s) 2, the second factor summed over loop(s) 3 before
void t5(float A[restrict 3][4], float B[restrict 4][5], float C[restrict 3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) for (int l = 0; l < 4; ++l) C[i][j] += A[i][k] * B[l][j];
}

/* only B pre-reduced: C[i][j] += A[i] * B[l][j] */
// PRE-DAG: :[[@LINE+2]]:3: remark: affine multiply-accumulate nest pre-reduced (--fp-distribute, docs/AFFINE_GEMM_MAPPING.md section 24): the first factor summed over loop(s) none, the second factor summed over loop(s) 2 before
void b_only(float A[restrict 3], float B[restrict 4][5], float C[restrict 3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int l = 0; l < 4; ++l) C[i][j] += A[i] * B[l][j];
}

/* only B pre-reduced, B'[k] = sum_j B[k][j]: C[i][k] += A[i][k] * B[k][j] (k an output batch loop) */
// PRE-DAG: :[[@LINE+2]]:3: remark: affine multiply-accumulate nest pre-reduced (--fp-distribute, docs/AFFINE_GEMM_MAPPING.md section 24): the first factor summed over loop(s) none, the second factor summed over loop(s) 2 before
void b_only_ik(float A[restrict 3][4], float B[restrict 4][5], float C[restrict 3][4]) {
  for (int i = 0; i < 3; ++i) for (int k = 0; k < 4; ++k) for (int j = 0; j < 5; ++j) C[i][k] += A[i][k] * B[k][j];
}

/* only A pre-reduced, over two loops: C[i][j] += A[i][s][k] * B[j] */
// PRE-DAG: :[[@LINE+2]]:3: remark: affine multiply-accumulate nest pre-reduced (--fp-distribute, docs/AFFINE_GEMM_MAPPING.md section 24): the first factor summed over loop(s) 2+3, the second factor summed over loop(s) none before
void a_only(float A[restrict 3][2][4], float B[restrict 5], float C[restrict 3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int s = 0; s < 2; ++s) for (int k = 0; k < 4; ++k) C[i][j] += A[i][s][k] * B[j];
}

/* an output batch loop b with pre-reduction: C[b][i][j] += A[b][i][k] * B[b][l][j] */
// PRE-DAG: :[[@LINE+2]]:3: remark: affine multiply-accumulate nest pre-reduced (--fp-distribute, docs/AFFINE_GEMM_MAPPING.md section 24): the first factor summed over loop(s) 3, the second factor summed over loop(s) 4 before
void ob(float A[restrict 2][3][4], float B[restrict 2][4][5], float C[restrict 2][3][5]) {
  for (int b = 0; b < 2; ++b) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) for (int l = 0; l < 4; ++l) C[b][i][j] += A[b][i][k] * B[b][l][j];
}

/* a negative coefficient and constants on the summed loops, a kept loop from 1 */
// PRE-DAG: :[[@LINE+2]]:3: remark: affine multiply-accumulate nest pre-reduced (--fp-distribute, docs/AFFINE_GEMM_MAPPING.md section 24): the first factor summed over loop(s) 2, the second factor summed over loop(s) 3 before
void neg_const(float A[restrict 4][6], float B[restrict 9][6], float C[restrict 4][5]) {
  for (int i = 1; i < 4; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) for (int l = 0; l < 4; ++l) C[i][j] += A[i][5 - k] * B[2 * l + 1][j + 1];
}

/* a stepped summed loop (l = 0, 2, 4, 6) */
// PRE-DAG: :[[@LINE+2]]:3: remark: affine multiply-accumulate nest pre-reduced (--fp-distribute, docs/AFFINE_GEMM_MAPPING.md section 24): the first factor summed over loop(s) 2, the second factor summed over loop(s) 3 before
void stepped_sum(float A[restrict 3][4], float B[restrict 8][5], float C[restrict 3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) for (int l = 0; l < 8; l += 2) C[i][j] += A[i][k] * B[l][j];
}

/* NOT pre-reduced -- L_AB nonempty (T2: x0 read by both factors, not by C): the reduction-batch lowering */
void t2(float A[restrict 3][7], float B[restrict 4][5], float C[restrict 3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int x0 = 0; x0 < 4; ++x0) for (int x1 = 0; x1 < 4; ++x1) C[i][j] += A[i][x0 - x1 + 3] * B[x0][j];
}

/* NOT pre-reduced -- an assignment (the twin of T5): last write wins, A[i][3] * B[3][j] into a zeroed C */
// PRE-DAG: :[[@LINE+2]]:3: remark: affine assignment nest C = A * B: last write wins (docs/AFFINE_GEMM_MAPPING.md section 20.5): loop(s) 2+3
void assign_t5(float A[restrict 3][4], float B[restrict 4][5], float C[restrict 3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) for (int l = 0; l < 4; ++l) C[i][j] = A[i][k] * B[l][j];
}

/* The references: the loops as written, one rounded product and one rounded
   addition per iteration in the order written (mul() keeps clang from fusing
   them into an FMA), and, per element of C, M = |C before| + sum |product|
   over the element's terms, in double: the magnitude sum the tolerance is
   relative to. */
static float mul(float a, float b) { return a * b; }
#define ACC(c, m, a, b) do { float p_ = mul((a), (b)); (c) = (c) + p_; (m) += fabs((double)p_); } while (0)
static void t5_ref(float A[3][4], float B[4][5], float C[3][5], double M[3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) for (int l = 0; l < 4; ++l) ACC(C[i][j], M[i][j], A[i][k], B[l][j]); }
static void b_only_ref(float A[3], float B[4][5], float C[3][5], double M[3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int l = 0; l < 4; ++l) ACC(C[i][j], M[i][j], A[i], B[l][j]); }
static void b_only_ik_ref(float A[3][4], float B[4][5], float C[3][4], double M[3][4]) {
  for (int i = 0; i < 3; ++i) for (int k = 0; k < 4; ++k) for (int j = 0; j < 5; ++j) ACC(C[i][k], M[i][k], A[i][k], B[k][j]); }
static void a_only_ref(float A[3][2][4], float B[5], float C[3][5], double M[3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int s = 0; s < 2; ++s) for (int k = 0; k < 4; ++k) ACC(C[i][j], M[i][j], A[i][s][k], B[j]); }
static void ob_ref(float A[2][3][4], float B[2][4][5], float C[2][3][5], double M[2][3][5]) {
  for (int b = 0; b < 2; ++b) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) for (int l = 0; l < 4; ++l) ACC(C[b][i][j], M[b][i][j], A[b][i][k], B[b][l][j]); }
static void neg_const_ref(float A[4][6], float B[9][6], float C[4][5], double M[4][5]) {
  for (int i = 1; i < 4; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) for (int l = 0; l < 4; ++l) ACC(C[i][j], M[i][j], A[i][5 - k], B[2 * l + 1][j + 1]); }
static void stepped_sum_ref(float A[3][4], float B[8][5], float C[3][5], double M[3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) for (int l = 0; l < 8; l += 2) ACC(C[i][j], M[i][j], A[i][k], B[l][j]); }
static void t2_ref(float A[3][7], float B[4][5], float C[3][5], double M[3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int x0 = 0; x0 < 4; ++x0) for (int x1 = 0; x1 < 4; ++x1) ACC(C[i][j], M[i][j], A[i][x0 - x1 + 3], B[x0][j]); }
static void assign_t5_ref(float A[3][4], float B[4][5], float C[3][5], double M[3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) for (int l = 0; l < 4; ++l) { C[i][j] = mul(A[i][k], B[l][j]); M[i][j] = fabs((double)C[i][j]); } }

/* Non-integer data by default (values (v * 0.3 + 0.011), v in -4..4, of both
   signs: the pre-reduced sums round differently from the loops as written);
   with -DINTEGER integer-valued data, on which every partial sum here is an
   exact float, so pre-reduction is exact and must be bitwise equal. The
   factors are never zero (no -0 product in the assignment). */
#ifdef INTEGER
#define SCALE(x) (x)
#else
#define SCALE(x) ((x) * 0.3f + 0.011f)
#endif
static void fill(float *p, int n, int s) { for (int e = 0; e < n; ++e) p[e] = SCALE((float)((e * 7 + s) % 9 - 4)); }
static void fill_nz(float *p, int n, int s) { for (int e = 0; e < n; ++e) { int v = (e * 7 + s) % 9 - 4; p[e] = SCALE((float)(v ? v : 5 - 2 * (e & 1))); } }
static unsigned long long inv(void) { systolic_offload_stats_t s; systolic_offload_stats(&s); return s.per_device[0]+s.per_device[1]+s.per_device[2]+s.per_device[3]; }

/* The tolerance: |result - reference| <= TOL_REL * M + TOL_ABS for every
   element, M the element's magnitude sum. Both the loops as written and the
   pre-reduced (A' = sum_{z_A} A, B' = sum_{z_B} B, C + A' B') evaluation are
   within gamma_n * M of the exact value (gamma_n = n u / (1 - n u), u = 2^-24,
   n the longest chain of roundings, here at most 33; and sum|A'| |B'| <= sum
   |A B|), so they differ by at most 2 gamma_33 M ~ 3.9e-6 M: 1e-5 leaves
   margin. max_rel_err is |result - reference| / |reference| over the elements
   whose reference is nonzero. */
#define TOL_REL 1e-5
#define TOL_ABS 1e-30
static int bad;
static void report(const char *name, const float *got, const float *want, const double *mag, int n, unsigned long long count) {
  double max_abs = 0, max_rel = 0;
  int within = 1;
  for (int e = 0; e < n; ++e) {
    double d = fabs((double)got[e] - (double)want[e]);
    if (d > max_abs) max_abs = d;
    if (want[e] != 0 && d / fabs((double)want[e]) > max_rel) max_rel = d / fabs((double)want[e]);
    if (!(d <= TOL_REL * mag[e] + TOL_ABS)) within = 0;
  }
  int bitwise = memcmp(got, want, n * sizeof *got) == 0;
  bad += !within;
#ifdef INTEGER
  bad += !bitwise;
#endif
  printf("[case] %s: max_abs_err=%g max_rel_err=%g within_tol=%s, %s (%llu)\n", name, max_abs, max_rel, within ? "yes" : "no", count ? "offloaded" : "NOT OFFLOADED", count);
  printf("[bitwise] %s: %s\n", name, bitwise ? "equal" : "different");
}
#define RUN(name, NA, NB, NC) do { static float A[NA], B[NB], C[NC], RA[NA], RB[NB], RC[NC]; static double M[NC]; \
  fill_nz(A, NA, 1); fill_nz(B, NB, 2); fill(C, NC, 3); memcpy(RA, A, sizeof A); memcpy(RB, B, sizeof B); memcpy(RC, C, sizeof C); \
  for (int e = 0; e < NC; ++e) M[e] = fabs((double)C[e]); \
  unsigned long long before = inv(); name((void*)A,(void*)B,(void*)C); unsigned long long n = inv() - before; \
  name##_ref((void*)RA,(void*)RB,(void*)RC,(void*)M); report(#name, C, RC, M, NC, n); } while (0)
int main(void) {
  RUN(t5, 12, 20, 15);
  RUN(b_only, 3, 20, 15);
  RUN(b_only_ik, 12, 20, 12);
  RUN(a_only, 24, 5, 15);
  RUN(ob, 24, 40, 30);
  RUN(neg_const, 24, 54, 20);
  RUN(stepped_sum, 12, 40, 15);
  RUN(t2, 21, 20, 15);
  RUN(assign_t5, 12, 20, 15);
  printf("%s: %d case(s) out of tolerance%s\n", bad ? "FAIL" : "PASS", bad,
#ifdef INTEGER
         " or not bitwise equal"
#else
         ""
#endif
  );
  return bad != 0;
}
