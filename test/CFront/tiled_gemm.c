// REQUIRES: cfront-clang
// Six loops: i,j,k are tile coordinates; ii,jj,kk are point coordinates.
// Spellings and sizes are compile-time inputs, not frontend special cases.
// RUN: %cfront %s -o %t.mlir
// RUN: FileCheck %s --check-prefix=FRONT --input-file=%t.mlir
// RUN: systolic-opt %t.mlir -o /dev/null
// RUN: %cfront -DSPELL=1 %s -o %t.and.mlir
// RUN: FileCheck %s --check-prefix=FRONT --input-file=%t.and.mlir
// RUN: systolic-opt %t.and.mlir -o /dev/null
// RUN: %cfront -DSPELL=2 %s -o %t.ternary.mlir
// RUN: FileCheck %s --check-prefix=FRONT --input-file=%t.ternary.mlir
// RUN: systolic-opt %t.ternary.mlir -o /dev/null
// RUN: %cfront -DSPELL=1 -DM=12 -DN=8 -DK=6 %s -o %t.div.mlir
// RUN: systolic-opt %t.div.mlir -o /dev/null
// RUN: %cfront -DSPELL=2 -DM=2 -DN=1 -DK=2 %s -o %t.small.mlir
// RUN: systolic-opt %t.small.mlir -o /dev/null
// RUN: systolic-opt --systolic-declare-fpga-fleet --test-affine-matmul-discovery %t.ternary.mlir -o /dev/null 2>&1 | FileCheck %s --check-prefix=DISC
// RUN: systolic-opt --systolic-declare-fpga-fleet "--affine-matmul-to-systolic=report=true allow-fp-reassociation=true" %t.ternary.mlir -o %t.sys.mlir 2>%t.legal
// RUN: FileCheck %s --check-prefix=LEGAL --input-file=%t.legal
// RUN: FileCheck %s --check-prefix=SYS --input-file=%t.sys.mlir
// RUN: systolic-opt %t.sys.mlir -o /dev/null
// RUN: systolic-opt --lower-systolic-matmul-tile-to-fpga %t.sys.mlir -o %t.fpga.mlir
// RUN: FileCheck %s --check-prefix=FPGA --input-file=%t.fpga.mlir
// RUN: systolic-opt --systolic-runtime-to-llvm %t.fpga.mlir -o %t.llvm.mlir
// RUN: FileCheck %s --check-prefix=LLVM --input-file=%t.llvm.mlir
// RUN: systolic-opt --systolic-declare-fpga-fleet --affine-matmul-to-systolic %t.ternary.mlir -o %t.strict.mlir
// RUN: FileCheck %s --check-prefix=STRICT --input-file=%t.strict.mlir

#ifndef M
#define M 10
#endif
#ifndef N
#define N 9
#endif
#ifndef K
#define K 7
#endif
#define TM 4
#define TN 4
#define TK 3
#ifndef SPELL
#define SPELL 0
#endif
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#if SPELL == 0
#define CLIP(p,t,s,e) p < MIN(t + s, e)
#elif SPELL == 1
#define CLIP(p,t,s,e) ((t + s > p) && (p < e))
#else
// Inclusive bounds and commuting + in the selected branch, counter right.
#define CLIP(p,t,s,e) ((t + (s - 1) >= (e - 1) ? (e - 1) : (s - 1) + t) >= p)
#endif

void tiled(float A[restrict M][K], float B[restrict K][N], float C[restrict M][N]) {
  for (int i = 0; i < M; i += TM)
    for (int j = 0; j < N; j = j + TN)
      for (int k = 0; k < K; k += TK)
        for (int ii = i; CLIP(ii, i, TM, M); ++ii)
          for (int jj = j; CLIP(jj, j, TN, N); jj++)
            for (int kk = k; CLIP(kk, k, TK, K); kk = kk + 1)
              C[ii][jj] += A[ii][kk] * B[kk][jj];
}
// FRONT-LABEL: func.func @tiled(
// FRONT: affine.for %i = 0 to 10 step 4
// FRONT-NEXT: affine.for %j = 0 to 9 step 4
// FRONT-NEXT: affine.for %k = 0 to 7 step 3
// FRONT-NEXT: affine.for %ii = affine_map<(d0) -> (0 + d0 * 1)>(%i) to min affine_map<(d0) -> (4 + d0 * 1, 10)>(%i)
// FRONT-NEXT: affine.for %jj = {{.*}} to min affine_map<(d0) -> (4 + d0 * 1, 9)>(%j)
// FRONT-NEXT: affine.for %kk = {{.*}} to min affine_map<(d0) -> (3 + d0 * 1, 7)>(%k)
// FRONT: arith.mulf
// FRONT-NEXT: {{.*}}arith.addf
// FRONT-NEXT: affine.store
// DISC: recognition result: success
// DISC-NEXT: {{.*}}scope: {{.*}}the point band of a tiling
// DISC: coverage: PROVEN_VALID
// DISC-SAME: dependence: PROVEN_VALID
// DISC-SAME: execution order: PROVEN_VALID
// LEGAL: coverage: PROVEN_VALID
// LEGAL-SAME: dependence: PROVEN_VALID
// LEGAL-SAME: execution order: PROVEN_VALID
// LEGAL-SAME: untiling: PROVEN_VALID (performed)
// SYS-LABEL: func.func @tiled(
// SYS-COUNT-6: systolic.matmul_tile
// SYS-NOT: systolic.matmul_tile
// FPGA-LABEL: func.func @tiled(
// FPGA-COUNT-6: llvm.call @fpga_matmul_tiled_auto_scheduled
// FPGA-NOT: llvm.call @fpga_matmul_tiled_auto_scheduled
// LLVM-LABEL: llvm.func @tiled(
// LLVM-COUNT-6: llvm.call @fpga_matmul_tiled_auto_scheduled
// LLVM-NOT: llvm.call @fpga_matmul_tiled_auto_scheduled
// STRICT-LABEL: func.func @tiled(
// STRICT-NOT: systolic.matmul_tile
// STRICT: arith.mulf
// STRICT-NOT: fastmath
// STRICT: arith.addf
// STRICT-NOT: fastmath
// STRICT-NOT: systolic.matmul_tile
