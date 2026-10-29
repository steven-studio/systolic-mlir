// REQUIRES: cfront-clang
//
// Phase 5.4.3d-3b-8 M2b-1 (docs/AFFINE_GEMM_MAPPING.md section 29.8): the
// host remainder of every output block runs in pieces -- below and above
// the block's common reduction fiber, or the whole block without one --
// over the exact bounding box of each piece. The results are the same
// products accumulated exactly once; checked here on the software runtime
// model with integer data (bitwise against the CPU reference, C nonzero
// before every call), on 16 x 16 outputs so the fleet's plan mixes 8 x 8
// and 4 x 4 blocks, with the exact nest counts of the remarks.
//
// RUN: env SYSTOLIC_CC_TEST_COMMON_FIBER_OFFLOAD=1 %systolic-cc --fp-reassociation --verify --explain --keep-dir %t.keep %s -o %t 2>&1 | FileCheck %s --check-prefix=PLAN
// RUN: FileCheck %s --check-prefix=REMARKS --input-file=%t.keep/20-remarks.txt
// RUN: %t > %t.out 2> %t.err
// RUN: FileCheck %s --check-prefix=VERIFY --input-file=%t.err
// RUN: %host-cc -std=c11 -O0 -ffp-contract=off %s -o %t.cpu
// RUN: %t.cpu > %t.cpu.out
// RUN: grep -v '^\[systolic-cc\]' %t.out | diff %t.cpu.out -

#include <stdio.h>

// 0 <= k <= i: rows [r0, r1] share the fiber [0, r0]; one piece above it.
void k_le_i(float A[restrict 16][16], float B[restrict 16][16],
            float C[restrict 16][16]) {
  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 16; ++j)
      for (int k = 0; k <= i; ++k)
        C[i][j] += A[i][k] * B[k][j];
}

// k >= j: columns [c0, c1] share the fiber [c1, 15]; one piece below it.
void k_ge_j(float A[restrict 16][16], float B[restrict 16][16],
            float C[restrict 16][16]) {
  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 16; ++j)
      for (int k = j; k < 16; ++k)
        C[i][j] += A[i][k] * B[k][j];
}

// j <= k <= i: blocks below the diagonal have pieces on both sides, blocks
// on it no common fiber (whole block), blocks above it nothing.
void j_le_k_le_i(float A[restrict 16][16], float B[restrict 16][16],
                 float C[restrict 16][16]) {
  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 16; ++j)
      for (int k = j; k <= i; ++k)
        C[i][j] += A[i][k] * B[k][j];
}

// 0 <= k <= i + j (k up to 30 inside 32): every block one piece above its
// fiber [0, r0 + c0].
void k_le_i_plus_j(float A[restrict 16][32], float B[restrict 32][16],
                   float C[restrict 16][16]) {
  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 16; ++j)
      for (int k = 0; k <= i + j; ++k)
        C[i][j] += A[i][k] * B[k][j];
}

// Nonzero lower bounds and shifted coordinates: C[i+1][j+2] over
// i in [2, 13), j in [1, 12), k in [1, i]; an 11 x 11 output box.
void shifted(float A[restrict 16][16], float B[restrict 16][16],
             float C[restrict 16][16]) {
  for (int i = 2; i < 13; ++i)
    for (int j = 1; j < 12; ++j)
      for (int k = 1; k <= i; ++k)
        C[i + 1][j + 2] += A[i + 1][k] * B[k][j + 2];
}

static float A16[16][16], B16[16][16], A32[16][32], B32[32][16];
static float CA[16][16], CB[16][16], CC[16][16], CD[16][16], CS[16][16];

int main(void) {
  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 16; ++j) {
      A16[i][j] = (float)(((i * 7 + j * 3) % 9 + 1) * ((i + j) % 2 ? -1 : 1));
      B16[i][j] = (float)(((i * 5 + j * 11) % 7 + 1) * ((i + 2 * j) % 3 ? 1 : -1));
      CA[i][j] = (float)((i + 2 * j) % 5 - 2);
      CB[i][j] = (float)(j - i);
      CC[i][j] = (float)(3 - i + j);
      CD[i][j] = (float)(i * 16 + j);
      CS[i][j] = (float)(2 * i - j + 1);
    }
  for (int i = 0; i < 16; ++i)
    for (int k = 0; k < 32; ++k)
      A32[i][k] = (float)(((i * 3 + k) % 7 + 1) * ((i + k) % 2 ? 1 : -1));
  for (int k = 0; k < 32; ++k)
    for (int j = 0; j < 16; ++j)
      B32[k][j] = (float)(((k * 2 + j * 5) % 6 + 1) * ((k + j) % 3 ? -1 : 1));

  k_le_i(A16, B16, CA);
  k_ge_j(A16, B16, CB);
  j_le_k_le_i(A16, B16, CC);
  k_le_i_plus_j(A32, B32, CD);
  shifted(A16, B16, CS);

  double sa = 0, sb = 0, sc = 0, sd = 0, ss = 0;
  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 16; ++j) {
      sa += CA[i][j] * (i + 1);
      sb += CB[i][j] * (j + 1);
      sc += CC[i][j] * (i + j + 1);
      sd += CD[i][j] * (2 * i + 1);
      ss += CS[i][j] * (i - j);
    }
  printf("%.1f %.1f %.1f %.1f %.1f %g %g %g %g %g\n", sa, sb, sc, sd, ss,
         CA[15][0], CB[0][15], CC[15][3], CD[15][15], CS[7][7]);
  return 0;
}

// PLAN-DAG: note: 'k_le_i' is offloaded: {{[0-9]+}} systolic tile invocation(s)
// PLAN-DAG: note: 'k_ge_j' is offloaded: {{[0-9]+}} systolic tile invocation(s)
// PLAN-DAG: note: 'j_le_k_le_i' is offloaded: {{[0-9]+}} systolic tile invocation(s)
// PLAN-DAG: note: 'k_le_i_plus_j' is offloaded: {{[0-9]+}} systolic tile invocation(s)
// PLAN-DAG: note: 'shifted' is offloaded: {{[0-9]+}} systolic tile invocation(s)
// PLAN:     wrote {{.*}} (5 function(s) offloaded, runtime: software model, NOT the FPGA
//
// The fleet's plan for a 16 x 16 output: two 8 x 8 blocks and eight 4 x 4
// blocks. k <= i: 2 x 224 + 8 x 24 = 640 host MACs in one piece per block;
// k >= j: the same by symmetry; j <= k <= i: 400 host MACs in 13 nests (two
// per block below the diagonal, one per block on it); k <= i + j: per block
// n m(m-1)/2 + m n(n-1)/2 host MACs, 2 x 448 + 8 x 48 = 1280 of 4096; shifted:
// an 11 x 11 output in 6 blocks, 847 MACs, 293 on the host.
// REMARKS-DAG: remark: affine GEMM 16x16x16 lowered as {{[0-9]+}} exact region(s) by the certified device-job lowering (docs/AFFINE_GEMM_MAPPING.md section 27) with the common reduction fibers of 10 output block(s) on the device (1536 of 2176 MACs) and the remainder on the host (640 MACs in 10 guarded nest(s)) (section 29)
// REMARKS-DAG: remark: affine GEMM 16x16x16 lowered as {{[0-9]+}} exact region(s) by the certified device-job lowering (docs/AFFINE_GEMM_MAPPING.md section 27) with the common reduction fibers of 10 output block(s) on the device (1536 of 2176 MACs) and the remainder on the host (640 MACs in 10 guarded nest(s)) (section 29)
// REMARKS-DAG: remark: affine GEMM 16x16x16 lowered as {{[0-9]+}} exact region(s) by the certified device-job lowering (docs/AFFINE_GEMM_MAPPING.md section 27) with the common reduction fibers of 5 output block(s) on the device (416 of 816 MACs) and the remainder on the host (400 MACs in 13 guarded nest(s)) (section 29)
// REMARKS-DAG: remark: affine GEMM 16x16x31 lowered as {{[0-9]+}} exact region(s) by the certified device-job lowering (docs/AFFINE_GEMM_MAPPING.md section 27) with the common reduction fibers of 10 output block(s) on the device (2816 of 4096 MACs) and the remainder on the host (1280 MACs in 10 guarded nest(s)) (section 29)
// REMARKS-DAG: remark: affine GEMM 11x11x12 lowered as 6 exact region(s) by the certified device-job lowering (docs/AFFINE_GEMM_MAPPING.md section 27) with the common reduction fibers of 6 output block(s) on the device (554 of 847 MACs) and the remainder on the host (293 MACs in 6 guarded nest(s)) (section 29)
//
// VERIFY-DAG: k_le_i ({{.*}}): verified: 256 elements equal the CPU reference (bitwise)
// VERIFY-DAG: k_ge_j ({{.*}}): verified: 256 elements equal the CPU reference (bitwise)
// VERIFY-DAG: j_le_k_le_i ({{.*}}): verified: 256 elements equal the CPU reference (bitwise)
// VERIFY-DAG: k_le_i_plus_j ({{.*}}): verified: 256 elements equal the CPU reference (bitwise)
// VERIFY-DAG: shifted ({{.*}}): verified: {{[0-9]+}} elements equal the CPU reference (bitwise)
// VERIFY-NOT: MISMATCH
