// REQUIRES: cfront-clang
//
// Dimension-intersection partitioning (docs/AFFINE_GEMM_MAPPING.md section 21,
// 2026-10-19): when C, A and B have more loop dimensions than one GEMM's
// M, N, K, the loops each operand reads classify them. A loop read by all
// three is a batch loop (unchanged); the extra loops of C and A (B shared),
// of C and B (A shared), and the loops read by C only (both shared) partition
// the GEMM into independent slabs of C, each one GEMM; a loop read by both
// factors and not by C is a second reduction coordinate (a partial-C
// reduction: refused), one read by one factor only a broadcast (refused), and
// a partition whose slabs would overlap is refused.
//
// RE-DECIDED 2026-10-21 (docs section 23, reduction-batch partitioning on by
// default): the two refusals above for C += A * B were policies of section 21.
// A loop C does not read (a second contraction loop, a one-factor loop, or a
// loop of a coupled reduction subscript) is now a reduction batch loop (RB):
// for each of its values one GEMM with offset factor views, accumulated into
// the same slab of C in order. ab_extra, partial_with_triple, coupled_k and
// a_only are offloaded for that reason (each case says which loop is RB and
// which is K). Assignments never get reduction batches (assign_overwrite still
// stays on the host). Each offloaded kernel is
// compared bitwise with a CPU reference that runs the loops as written on
// integer-valued data (the factors without zeros) with a NONZERO initial C.
//
// NO REAL HARDWARE HERE: the software runtime model computes every tile;
// SOFT_RUNTIME_MOCK_HARDWARE=1 makes it claim to be the FPGA.
//
// RUN: %systolic-cc --fp-reassociation --explain %s -o %t 2>&1 | FileCheck %s --check-prefix=PLAN
// PLAN-DAG: note: 'triple' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 4 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'ca_extra' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 4 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'cb_extra' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 4 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'triple_two' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 12 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'ca_extra_two' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 12 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'cb_extra_two' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 12 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'mixed_triple_ca' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 8 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'ca_plus_cb' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 8 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'permuted' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 4 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'offset_batch' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 4 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'strided_batch' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 4 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'k1_outer_batch' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 4 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'k1_triple_outer' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 4 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'rank1_mixed' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 4 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'rectangular' is offloaded: 3 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 6 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'skewed_a' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 4 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'merged_a' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 4 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'c_only' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 4 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'ca_assign' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 4 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'inner_partition' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 4 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'ab_extra' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 4 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'assign_overwrite' stays on the host: no GEMM the systolic arrays can run was recognized in it (line {{[0-9]+}}: two iterations of the assignment C = A * B write the same element of C
// PLAN-DAG: note: 'cross_alias' stays on the host: no GEMM the systolic arrays can run was recognized in it (line {{[0-9]+}}: A may overlap C
// PLAN-DAG: note: 'overlap_c' stays on the host: no GEMM the systolic arrays can run was recognized in it{{$}}
// PLAN-DAG: note: 'coupled_k' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 4 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'c_as_factor' stays on the host: no GEMM the systolic arrays can run was recognized in it (line {{[0-9]+}}: B may overlap C
// PLAN-DAG: note: 'flat_collision' stays on the host: no GEMM the systolic arrays can run was recognized in it{{$}}
// PLAN-DAG: note: 'a_only' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 4 invocation(s) per call (the loops around them repeat them)
// PLAN-DAG: note: 'oob_c' stays on the host: no GEMM the systolic arrays can run was recognized in it (line {{[0-9]+}}: C is accessed out of bounds along dimension 2)
// PLAN-DAG: note: 'merged_c' stays on the host: no GEMM the systolic arrays can run was recognized in it{{$}}
// PLAN-DAG: note: 'partial_with_triple' is offloaded: 2 systolic tile invocation(s) on the 1x8x8 + 3x4x4 fleet; 8 invocation(s) per call (the loops around them repeat them)
// PLAN: wrote {{.*}} (24 function(s) offloaded
//
// RUN: %t > %t.soft
// RUN: FileCheck %s --check-prefix=ALL --input-file=%t.soft
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t > %t.mock
// RUN: FileCheck %s --check-prefix=ALL --input-file=%t.mock
// ALL: [case] triple: equal, offloaded (4)
// ALL: [case] ca_extra: equal, offloaded (4)
// ALL: [case] cb_extra: equal, offloaded (4)
// ALL: [case] triple_two: equal, offloaded (12)
// ALL: [case] ca_extra_two: equal, offloaded (12)
// ALL: [case] cb_extra_two: equal, offloaded (12)
// ALL: [case] mixed_triple_ca: equal, offloaded (8)
// ALL: [case] ca_plus_cb: equal, offloaded (8)
// ALL: [case] permuted: equal, offloaded (4)
// ALL: [case] offset_batch: equal, offloaded (4)
// ALL: [case] strided_batch: equal, offloaded (4)
// ALL: [case] k1_outer_batch: equal, offloaded (4)
// ALL: [case] k1_triple_outer: equal, offloaded (4)
// ALL: [case] rank1_mixed: equal, offloaded (4)
// ALL: [case] rectangular: equal, offloaded (6)
// ALL: [case] skewed_a: equal, offloaded (4)
// ALL: [case] merged_a: equal, offloaded (4)
// ALL: [case] c_only: equal, offloaded (4)
// ALL: [case] ca_assign: equal, offloaded (4)
// ALL: [case] inner_partition: equal, offloaded (4)
// ALL: [case] ab_extra: equal, offloaded (4)
// ALL: [case] assign_overwrite: equal, NOT OFFLOADED (0)
// ALL: [case] cross_alias: equal, NOT OFFLOADED (0)
// ALL: [case] overlap_c: equal, NOT OFFLOADED (0)
// ALL: [case] coupled_k: equal, offloaded (4)
// ALL: [case] c_as_factor: equal, NOT OFFLOADED (0)
// ALL: [case] flat_collision: equal, NOT OFFLOADED (0)
// ALL: [case] a_only: equal, offloaded (4)
// ALL: [case] merged_c: equal, NOT OFFLOADED (0)
// ALL: [case] partial_with_triple: equal, offloaded (8)
// ALL: PASS: 0 differ
//
// Every offloaded call also against the function as written, on the CPU:
// RUN: %systolic-cc --fp-reassociation --verify %s -o %t.v > /dev/null 2>&1
// RUN: env SOFT_RUNTIME_MOCK_HARDWARE=1 %t.v 2>&1 >/dev/null | FileCheck %s --check-prefix=VERIFY
// VERIFY-COUNT-24: verified: {{[0-9]+}} elements equal the CPU reference (bitwise)
// VERIFY-NOT: MISMATCH

#include <stdio.h>
#include <string.h>

#include "systolic_offload.h"

/* 1. triple intersection: b read by A, B and C */
void triple(float A[restrict 2][3][4], float B[restrict 2][4][5], float C[restrict 2][3][5]) {
  for (int b = 0; b < 2; ++b) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[b][i][j] += A[b][i][k] * B[b][k][j];
}
/* 2. C and A share p: a partition on p, B shared */
void ca_extra(float A[restrict 2][3][4], float B[restrict 4][5], float C[restrict 2][3][5]) {
  for (int p = 0; p < 2; ++p) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[p][i][j] += A[p][i][k] * B[k][j];
}
/* 3. C and B share q: a partition on q, A shared */
void cb_extra(float A[restrict 3][4], float B[restrict 2][4][5], float C[restrict 2][3][5]) {
  for (int q = 0; q < 2; ++q) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[q][i][j] += A[i][k] * B[q][k][j];
}
/* 4. two triple-intersection dimensions */
void triple_two(float A[restrict 2][3][3][4], float B[restrict 2][3][4][5], float C[restrict 2][3][3][5]) {
  for (int b0 = 0; b0 < 2; ++b0) for (int b1 = 0; b1 < 3; ++b1) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[b0][b1][i][j] += A[b0][b1][i][k] * B[b0][b1][k][j];
}
/* 5. two extra C-and-A dimensions (the kept coordinate: i, read in C's innermost dimension) */
void ca_extra_two(float A[restrict 2][3][3][4], float B[restrict 4][5], float C[restrict 2][3][3][5]) {
  for (int p0 = 0; p0 < 2; ++p0) for (int p1 = 0; p1 < 3; ++p1) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[p0][p1][i][j] += A[p0][p1][i][k] * B[k][j];
}
/* 6. two extra C-and-B dimensions */
void cb_extra_two(float A[restrict 3][4], float B[restrict 2][3][4][5], float C[restrict 2][3][3][5]) {
  for (int q0 = 0; q0 < 2; ++q0) for (int q1 = 0; q1 < 3; ++q1) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[q0][q1][i][j] += A[i][k] * B[q0][q1][k][j];
}
/* 7. mixed: a triple-intersection batch and a C-and-A partition; a C-and-A and a C-and-B partition */
void mixed_triple_ca(float A[restrict 2][2][3][4], float B[restrict 2][4][5], float C[restrict 2][2][3][5]) {
  for (int b = 0; b < 2; ++b) for (int p = 0; p < 2; ++p) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[b][p][i][j] += A[b][p][i][k] * B[b][k][j];
}
void ca_plus_cb(float A[restrict 2][3][4], float B[restrict 2][4][5], float C[restrict 2][2][3][5]) {
  for (int p = 0; p < 2; ++p) for (int q = 0; q < 2; ++q) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[p][q][i][j] += A[p][i][k] * B[q][k][j];
}
/* 8. the partition dimension in different positions of each operand */
void permuted(float A[restrict 3][2][4], float B[restrict 4][5], float C[restrict 3][5][2]) {
  for (int p = 0; p < 2; ++p) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[i][j][p] += A[i][p][k] * B[k][j];
}
/* 9. offset batch indices (C at p + 1, A at 2p) */
void offset_batch(float A[restrict 4][3][4], float B[restrict 4][5], float C[restrict 3][3][5]) {
  for (int p = 0; p < 2; ++p) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[p + 1][i][j] += A[2 * p][i][k] * B[k][j];
}
/* 10. strided batch indices */
void strided_batch(float A[restrict 3][4], float B[restrict 4][4][5], float C[restrict 4][3][5]) {
  for (int q = 0; q < 2; ++q) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[2 * q][i][j] += A[i][k] * B[2 * q + 1][k][j];
}
/* 12. K = 1 batched outer products: a partition with B shared, and a triple batch */
void k1_outer_batch(float A[restrict 2][3], float B[restrict 5], float C[restrict 2][3][5]) {
  for (int p = 0; p < 2; ++p) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) C[p][i][j] += A[p][i] * B[j];
}
void k1_triple_outer(float A[restrict 2][3], float B[restrict 2][5], float C[restrict 2][3][5]) {
  for (int b = 0; b < 2; ++b) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) C[b][i][j] += A[b][i] * B[b][j];
}
/* 13. rank-1 and mixed-rank inputs */
void rank1_mixed(float A[restrict 3], float B[restrict 2][5], float C[restrict 2][3][5]) {
  for (int q = 0; q < 2; ++q) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) C[q][i][j] += A[i] * B[q][j];
}
/* 14. rectangular, several tiles per partition */
void rectangular(float A[restrict 2][7][4], float B[restrict 4][9], float C[restrict 2][7][9]) {
  for (int p = 0; p < 2; ++p) for (int i = 0; i < 7; ++i) for (int j = 0; j < 9; ++j) for (int k = 0; k < 4; ++k) C[p][i][j] += A[p][i][k] * B[k][j];
}
/* 15. non-identity views: A skewed by the partition, A merged over (p, i), C only, an assignment, the partition loop inside */
void skewed_a(float A[restrict 5][4], float B[restrict 4][5], float C[restrict 2][3][5]) {
  for (int p = 0; p < 2; ++p) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[p][i][j] += A[p + i][k] * B[k][j];
}
void merged_a(float A[restrict 6][4], float B[restrict 4][5], float C[restrict 2][3][5]) {
  for (int p = 0; p < 2; ++p) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[p][i][j] += A[i + 3 * p][k] * B[k][j];
}
void c_only(float A[restrict 3][4], float B[restrict 4][5], float C[restrict 2][3][5]) {
  for (int b = 0; b < 2; ++b) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[b][i][j] += A[i][k] * B[k][j];
}
void ca_assign(float A[restrict 2][3][4], float B[restrict 4][5], float C[restrict 2][3][5]) {
  for (int p = 0; p < 2; ++p) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 1; ++k) C[p][i][j] = A[p][i][k] * B[k][j];
}
void inner_partition(float A[restrict 3][2][4], float B[restrict 4][5], float C[restrict 3][2][5]) {
  for (int i = 0; i < 3; ++i) for (int p = 0; p < 2; ++p) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[i][p][j] += A[i][p][k] * B[k][j];
}
/* must stay on the host (except ab_extra, partial_with_triple, coupled_k and
   a_only: RE-DECIDED 2026-10-21, see each) */
/* N1, N14: a shared reduction dimension missing in C (a partial-C reduction).
   RE-DECIDED 2026-10-21 (docs section 23, reduction-batch partitioning on by
   default): both are offloaded. Refusing a partial-C reduction was a policy of
   section 21; the 2026-10-21 research policy accepts it (docs section 23.5,
   "two contraction loops, A[r][i][k]*B[r][k][j]"). ab_extra: RB = r (loop 0),
   K = k, M = i, N = j. partial_with_triple: b (loop 0) stays a batch loop of
   all three operands, RB = r (loop 1), K = k. Every product runs once and the
   r iterations accumulate into the same slab of C in order. */
void ab_extra(float A[restrict 2][3][4], float B[restrict 2][4][5], float C[restrict 3][5]) {
  for (int r = 0; r < 2; ++r) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[i][j] += A[r][i][k] * B[r][k][j];
}
void partial_with_triple(float A[restrict 2][2][3][4], float B[restrict 2][2][4][5], float C[restrict 2][3][5]) {
  for (int b = 0; b < 2; ++b) for (int r = 0; r < 2; ++r) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[b][i][j] += A[b][r][i][k] * B[b][r][k][j];
}
/* N2, N12: an assignment whose batches write the same C elements */
void assign_overwrite(float A[restrict 2][3], float B[restrict 2][5], float C[restrict 4][5]) {
  for (int b = 0; b < 2; ++b) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) C[i + b][j] = A[b][i] * B[b][j];
}
/* N3, N13: a cross-partition dependence through the output */
void cross_alias(float A[3][3][5], float B[restrict 5][5], float C[restrict 1]) {
  for (int p = 0; p < 2; ++p) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 5; ++k) A[p + 1][i][j] += A[p][i][k] * B[k][j];
}
/* N4: the partitions' outputs overlap */
void overlap_c(float A[restrict 2][3][4], float B[restrict 4][5], float C[restrict 5][5]) {
  for (int p = 0; p < 2; ++p) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[p + i][j] += A[p][i][k] * B[k][j];
}
/* N5: incompatible access maps: the two factors read the reduction loops through
   different combinations (A: k0 + 2 k1, B: k0 + k1) -- no shared coordinate.
   RE-DECIDED 2026-10-21 (docs section 23, reduction-batch partitioning on by
   default): offloaded. The refusal (SQ-1, UNKNOWN: a reduction role is never
   partitioned) was a policy of section 21; the 2026-10-21 research policy
   accepts it (docs section 23.2's example, Case 2-like). RB = k0 (loop 2),
   K = k1 (loop 3): for each k0 the GEMM reads A[i][k0 + 2K] (stride 2) and
   B[k0 + K][j], accumulated into C in k0 order. */
void coupled_k(float A[restrict 3][4], float B[restrict 3][5], float C[restrict 3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k0 = 0; k0 < 2; ++k0) for (int k1 = 0; k1 < 2; ++k1) C[i][j] += A[i][k0 + 2 * k1] * B[k0 + k1][j];
}
/* N6: an invalid reduction dependence (C read as a factor across the batches) */
void c_as_factor(float A[restrict 2][3][4], float B[restrict 1], float C[3][4][5]) {
  for (int b = 0; b < 2; ++b) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[b + 1][i][j] += A[b][i][k] * C[b][k][j];
}
/* N7: an unsafe flattened output */
void flat_collision(float A[restrict 2][3][4], float B[restrict 4][5], float C[restrict 20]) {
  for (int p = 0; p < 2; ++p) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[4 * p + 5 * i + j] += A[p][i][k] * B[k][j];
}
/* N9: a broadcast (a loop read by one factor only).
   RE-DECIDED 2026-10-21 (docs section 23, reduction-batch partitioning on by
   default): offloaded. Refusing the stride-0 broadcast was a policy of
   section 21 (section 10); the 2026-10-21 research policy accepts it (docs
   section 23.5, "A-only loops", the T5 kind of one-factor loop). RB = s
   (loop 2), K = k (loop 3): for each s a GEMM with A offset by s and B shared,
   accumulated into C in s order (B's elements are reused, every product runs
   once). */
void a_only(float A[restrict 3][2][4], float B[restrict 4][5], float C[restrict 3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int s = 0; s < 2; ++s) for (int k = 0; k < 4; ++k) C[i][j] += A[i][s][k] * B[k][j];
}
/* N10: out of bounds (compiled, not executed: undefined in C) */
void oob_c(float A[restrict 2][3][4], float B[restrict 4][5], float C[restrict 2][3][4]) {
  for (int p = 0; p < 2; ++p) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[p][i][j] += A[p][i][k] * B[k][j];
}
/* N11: an ambiguous output mapping (C merges the partition and the row coordinate in one dimension) */
void merged_c(float A[restrict 2][3][4], float B[restrict 4][5], float C[restrict 6][5]) {
  for (int p = 0; p < 2; ++p) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[3 * p + i][j] += A[p][i][k] * B[k][j];
}

static float mul(float a, float b) { return a * b; }
static void triple_ref(float A[2][3][4], float B[2][4][5], float C[2][3][5]) {
  for (int b = 0; b < 2; ++b) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[b][i][j] = C[b][i][j] + mul(A[b][i][k], B[b][k][j]); }
static void ca_extra_ref(float A[2][3][4], float B[4][5], float C[2][3][5]) {
  for (int p = 0; p < 2; ++p) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[p][i][j] = C[p][i][j] + mul(A[p][i][k], B[k][j]); }
static void cb_extra_ref(float A[3][4], float B[2][4][5], float C[2][3][5]) {
  for (int q = 0; q < 2; ++q) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[q][i][j] = C[q][i][j] + mul(A[i][k], B[q][k][j]); }
static void triple_two_ref(float A[2][3][3][4], float B[2][3][4][5], float C[2][3][3][5]) {
  for (int b0 = 0; b0 < 2; ++b0) for (int b1 = 0; b1 < 3; ++b1) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[b0][b1][i][j] = C[b0][b1][i][j] + mul(A[b0][b1][i][k], B[b0][b1][k][j]); }
static void ca_extra_two_ref(float A[2][3][3][4], float B[4][5], float C[2][3][3][5]) {
  for (int p0 = 0; p0 < 2; ++p0) for (int p1 = 0; p1 < 3; ++p1) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[p0][p1][i][j] = C[p0][p1][i][j] + mul(A[p0][p1][i][k], B[k][j]); }
static void cb_extra_two_ref(float A[3][4], float B[2][3][4][5], float C[2][3][3][5]) {
  for (int q0 = 0; q0 < 2; ++q0) for (int q1 = 0; q1 < 3; ++q1) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[q0][q1][i][j] = C[q0][q1][i][j] + mul(A[i][k], B[q0][q1][k][j]); }
static void mixed_triple_ca_ref(float A[2][2][3][4], float B[2][4][5], float C[2][2][3][5]) {
  for (int b = 0; b < 2; ++b) for (int p = 0; p < 2; ++p) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[b][p][i][j] = C[b][p][i][j] + mul(A[b][p][i][k], B[b][k][j]); }
static void ca_plus_cb_ref(float A[2][3][4], float B[2][4][5], float C[2][2][3][5]) {
  for (int p = 0; p < 2; ++p) for (int q = 0; q < 2; ++q) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[p][q][i][j] = C[p][q][i][j] + mul(A[p][i][k], B[q][k][j]); }
static void permuted_ref(float A[3][2][4], float B[4][5], float C[3][5][2]) {
  for (int p = 0; p < 2; ++p) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[i][j][p] = C[i][j][p] + mul(A[i][p][k], B[k][j]); }
static void offset_batch_ref(float A[4][3][4], float B[4][5], float C[3][3][5]) {
  for (int p = 0; p < 2; ++p) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[p + 1][i][j] = C[p + 1][i][j] + mul(A[2 * p][i][k], B[k][j]); }
static void strided_batch_ref(float A[3][4], float B[4][4][5], float C[4][3][5]) {
  for (int q = 0; q < 2; ++q) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[2 * q][i][j] = C[2 * q][i][j] + mul(A[i][k], B[2 * q + 1][k][j]); }
static void k1_outer_batch_ref(float A[2][3], float B[5], float C[2][3][5]) {
  for (int p = 0; p < 2; ++p) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) C[p][i][j] = C[p][i][j] + mul(A[p][i], B[j]); }
static void k1_triple_outer_ref(float A[2][3], float B[2][5], float C[2][3][5]) {
  for (int b = 0; b < 2; ++b) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) C[b][i][j] = C[b][i][j] + mul(A[b][i], B[b][j]); }
static void rank1_mixed_ref(float A[3], float B[2][5], float C[2][3][5]) {
  for (int q = 0; q < 2; ++q) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) C[q][i][j] = C[q][i][j] + mul(A[i], B[q][j]); }
static void rectangular_ref(float A[2][7][4], float B[4][9], float C[2][7][9]) {
  for (int p = 0; p < 2; ++p) for (int i = 0; i < 7; ++i) for (int j = 0; j < 9; ++j) for (int k = 0; k < 4; ++k) C[p][i][j] = C[p][i][j] + mul(A[p][i][k], B[k][j]); }
static void skewed_a_ref(float A[5][4], float B[4][5], float C[2][3][5]) {
  for (int p = 0; p < 2; ++p) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[p][i][j] = C[p][i][j] + mul(A[p + i][k], B[k][j]); }
static void merged_a_ref(float A[6][4], float B[4][5], float C[2][3][5]) {
  for (int p = 0; p < 2; ++p) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[p][i][j] = C[p][i][j] + mul(A[i + 3 * p][k], B[k][j]); }
static void c_only_ref(float A[3][4], float B[4][5], float C[2][3][5]) {
  for (int b = 0; b < 2; ++b) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[b][i][j] = C[b][i][j] + mul(A[i][k], B[k][j]); }
static void ca_assign_ref(float A[2][3][4], float B[4][5], float C[2][3][5]) {
  for (int p = 0; p < 2; ++p) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 1; ++k) C[p][i][j] = mul(A[p][i][k], B[k][j]); }
static void inner_partition_ref(float A[3][2][4], float B[4][5], float C[3][2][5]) {
  for (int i = 0; i < 3; ++i) for (int p = 0; p < 2; ++p) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[i][p][j] = C[i][p][j] + mul(A[i][p][k], B[k][j]); }
static void ab_extra_ref(float A[2][3][4], float B[2][4][5], float C[3][5]) {
  for (int r = 0; r < 2; ++r) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[i][j] = C[i][j] + mul(A[r][i][k], B[r][k][j]); }
static void partial_with_triple_ref(float A[2][2][3][4], float B[2][2][4][5], float C[2][3][5]) {
  for (int b = 0; b < 2; ++b) for (int r = 0; r < 2; ++r) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[b][i][j] = C[b][i][j] + mul(A[b][r][i][k], B[b][r][k][j]); }
static void assign_overwrite_ref(float A[2][3], float B[2][5], float C[4][5]) {
  for (int b = 0; b < 2; ++b) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) C[i + b][j] = mul(A[b][i], B[b][j]); }
static void cross_alias_ref(float A[3][3][5], float B[5][5], float C[1]) {
  (void)C; for (int p = 0; p < 2; ++p) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 5; ++k) A[p + 1][i][j] = A[p + 1][i][j] + mul(A[p][i][k], B[k][j]); }
static void overlap_c_ref(float A[2][3][4], float B[4][5], float C[5][5]) {
  for (int p = 0; p < 2; ++p) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[p + i][j] = C[p + i][j] + mul(A[p][i][k], B[k][j]); }
static void coupled_k_ref(float A[3][4], float B[3][5], float C[3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k0 = 0; k0 < 2; ++k0) for (int k1 = 0; k1 < 2; ++k1) C[i][j] = C[i][j] + mul(A[i][k0 + 2 * k1], B[k0 + k1][j]); }
static void c_as_factor_ref(float A[2][3][4], float B[1], float C[3][4][5]) {
  (void)B; for (int b = 0; b < 2; ++b) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[b + 1][i][j] = C[b + 1][i][j] + mul(A[b][i][k], C[b][k][j]); }
static void flat_collision_ref(float A[2][3][4], float B[4][5], float C[20]) {
  for (int p = 0; p < 2; ++p) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[4 * p + 5 * i + j] = C[4 * p + 5 * i + j] + mul(A[p][i][k], B[k][j]); }
static void a_only_ref(float A[3][2][4], float B[4][5], float C[3][5]) {
  for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int s = 0; s < 2; ++s) for (int k = 0; k < 4; ++k) C[i][j] = C[i][j] + mul(A[i][s][k], B[k][j]); }
static void merged_c_ref(float A[2][3][4], float B[4][5], float C[6][5]) {
  for (int p = 0; p < 2; ++p) for (int i = 0; i < 3; ++i) for (int j = 0; j < 5; ++j) for (int k = 0; k < 4; ++k) C[3 * p + i][j] = C[3 * p + i][j] + mul(A[p][i][k], B[k][j]); }

static void fill(float *p, int n, int s) { for (int e = 0; e < n; ++e) p[e] = (float)((e * 7 + s) % 9 - 4); }
static void fill_nz(float *p, int n, int s) { for (int e = 0; e < n; ++e) { int v = (e * 7 + s) % 9 - 4; p[e] = (float)(v ? v : 5 - 2 * (e & 1)); } }
static int differ;
static unsigned long long inv(void) { systolic_offload_stats_t s; systolic_offload_stats(&s); return s.per_device[0]+s.per_device[1]+s.per_device[2]+s.per_device[3]; }
static void report(const char *n, const void *g, const void *w, size_t by, unsigned long long i) {
  int same = memcmp(g, w, by) == 0; differ += !same;
  printf("[case] %s: %s, %s (%llu)\n", n, same ? "equal" : "DIFFERENT", i ? "offloaded" : "NOT OFFLOADED", i);
}
/* OUT: 1 -- C is the output, 0 -- A is (the aliasing kernels) */
#define RUN(name, NA, NB, NC, OUT) do { static float A[NA], B[NB], C[NC], RA[NA], RB[NB], RC[NC]; \
  fill_nz(A, NA, 1); fill_nz(B, NB, 2); fill(C, NC, 3); memcpy(RA, A, sizeof A); memcpy(RB, B, sizeof B); memcpy(RC, C, sizeof C); \
  unsigned long long before = inv(); name((void*)A,(void*)B,(void*)C); unsigned long long n = inv() - before; \
  name##_ref((void*)RA,(void*)RB,(void*)RC); \
  if (OUT == 0) report(#name, A, RA, sizeof A, n); else report(#name, C, RC, sizeof C, n); } while (0)
int main(void) {
  RUN(triple, 2*3*4, 2*4*5, 2*3*5, 1);
  RUN(ca_extra, 2*3*4, 4*5, 2*3*5, 1);
  RUN(cb_extra, 3*4, 2*4*5, 2*3*5, 1);
  RUN(triple_two, 2*3*3*4, 2*3*4*5, 2*3*3*5, 1);
  RUN(ca_extra_two, 2*3*3*4, 4*5, 2*3*3*5, 1);
  RUN(cb_extra_two, 3*4, 2*3*4*5, 2*3*3*5, 1);
  RUN(mixed_triple_ca, 2*2*3*4, 2*4*5, 2*2*3*5, 1);
  RUN(ca_plus_cb, 2*3*4, 2*4*5, 2*2*3*5, 1);
  RUN(permuted, 3*2*4, 4*5, 3*5*2, 1);
  RUN(offset_batch, 4*3*4, 4*5, 3*3*5, 1);
  RUN(strided_batch, 3*4, 4*4*5, 4*3*5, 1);
  RUN(k1_outer_batch, 2*3, 5, 2*3*5, 1);
  RUN(k1_triple_outer, 2*3, 2*5, 2*3*5, 1);
  RUN(rank1_mixed, 3, 2*5, 2*3*5, 1);
  RUN(rectangular, 2*7*4, 4*9, 2*7*9, 1);
  RUN(skewed_a, 5*4, 4*5, 2*3*5, 1);
  RUN(merged_a, 6*4, 4*5, 2*3*5, 1);
  RUN(c_only, 3*4, 4*5, 2*3*5, 1);
  RUN(ca_assign, 2*3*4, 4*5, 2*3*5, 1);
  RUN(inner_partition, 3*2*4, 4*5, 3*2*5, 1);
  RUN(ab_extra, 2*3*4, 2*4*5, 3*5, 1);
  RUN(assign_overwrite, 2*3, 2*5, 4*5, 1);
  RUN(cross_alias, 3*3*5, 5*5, 1, 0);
  RUN(overlap_c, 2*3*4, 4*5, 5*5, 1);
  RUN(coupled_k, 3*4, 3*5, 3*5, 1);
  RUN(c_as_factor, 2*3*4, 1, 3*4*5, 1);
  RUN(flat_collision, 2*3*4, 4*5, 20, 1);
  RUN(a_only, 3*2*4, 4*5, 3*5, 1);
  RUN(merged_c, 2*3*4, 4*5, 6*5, 1);
  RUN(partial_with_triple, 2*2*3*4, 2*2*4*5, 2*3*5, 1);
  printf("%s: %d differ\n", differ ? "FAIL" : "PASS", differ); return differ;
}
