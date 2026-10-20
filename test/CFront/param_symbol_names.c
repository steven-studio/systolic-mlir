// REQUIRES: cfront-clang
//
// The index-typed copy of an integer parameter -- %<param>_idx, the value a
// subscript names as an affine symbol -- is reserved like every other SSA
// name of the function: a loop counter or another parameter spelled
// <param>_idx gets another name, whichever comes first (regression test: the
// copy's name was not reserved, and such a function translated to MLIR that
// systolic-opt cannot parse, whether or not the parameter was used).
//
// RUN: %cfront %s -o %t.mlir
// RUN: systolic-opt %t.mlir -o /dev/null
// RUN: FileCheck %s --input-file=%t.mlir
//
// CHECK-LABEL: func.func @counter_spelled_idx(
// CHECK:         %n_idx = arith.index_cast %n : i32 to index
// CHECK:         affine.for %n_idx_1 = 0 to 16 {
//
// CHECK-LABEL: func.func @param_spelled_idx_after(
// CHECK-DAG:     %r0_idx = arith.index_cast %r0 : i32 to index
// CHECK-DAG:     %r0_idx_1_idx = arith.index_cast %r0_idx_1 : i32 to index
// CHECK:         affine.load %C[((symbol(%r0_idx) + symbol(%r0_idx_1_idx)) + %i), %j]
//
// CHECK-LABEL: func.func @param_spelled_idx_before(
// CHECK-DAG:     %n_idx_idx = arith.index_cast %n_idx : i32 to index
// CHECK-DAG:     %n_idx_1 = arith.index_cast %n : i32 to index
// CHECK:         affine.load %C[((symbol(%n_idx_1) + symbol(%n_idx_idx)) + %i), %j]

void counter_spelled_idx(float A[restrict 16][16], float B[restrict 16][16],
                         float C[restrict 16][16], int n) {
  for (int n_idx = 0; n_idx < 16; ++n_idx)
    for (int j = 0; j < 16; ++j)
      for (int k = 0; k < 16; ++k)
        C[n_idx][j] += A[n_idx][k] * B[k][j];
}

void param_spelled_idx_after(float A[restrict 8][16], float B[restrict 16][16],
                             float C[restrict 64][16], int r0, int r0_idx) {
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 16; ++j)
      for (int k = 0; k < 16; ++k)
        C[r0 + r0_idx + i][j] += A[i][k] * B[k][j];
}

void param_spelled_idx_before(float A[restrict 8][16], float B[restrict 16][16],
                              float C[restrict 64][16], int n_idx, int n) {
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 16; ++j)
      for (int k = 0; k < 16; ++k)
        C[n + n_idx + i][j] += A[i][k] * B[k][j];
}
