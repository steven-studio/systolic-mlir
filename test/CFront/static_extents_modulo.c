// REQUIRES: cfront-clang
//
// systolic-cfront --static-extents and --modulo-subscripts, the two front-end
// options systolic-cc gives for its Generalized GEMM fallback
// (docs/GENERALIZED_GEMM_SCOPE.md, "The fallback in systolic-cc").
//
// Without them: every array parameter memref<?x...>, and E % C refused.
// RUN: %cfront %s --skip-unsupported -o %t.off.mlir 2>&1 | FileCheck %s --check-prefix=OFFNOTE
// RUN: FileCheck %s --check-prefix=OFF --input-file=%t.off.mlir
// OFFNOTE: note: 'perm' is outside the supported C subset and is not translated: subscript is not an affine expression of the loop counters
// OFF: func.func @bounds(%A: memref<?x12xf32> {llvm.noalias}
// OFF-SAME: %B: memref<?x8xf32> {llvm.noalias}
// OFF-SAME: %C: memref<?x8xf32> {llvm.noalias}
//
// With them.
// RUN: %cfront %s --static-extents --modulo-subscripts --skip-unsupported \
// RUN:   --interface %t.json -o %t.mlir 2>&1 | FileCheck %s --check-prefix=NOTES
// RUN: FileCheck %s --check-prefix=ON --input-file=%t.mlir
// RUN: FileCheck %s --check-prefix=IFACE --input-file=%t.json
// RUN: systolic-opt %t.mlir -o /dev/null
//
// A leading bound written (a literal, a macro, restrict, static): the extent
// the accesses reach, the interface's leading extent. Written as a pointer
// (float (*)[8], B[][8]): open, as without the option.
// ON-LABEL: func.func @bounds(%A: memref<16x12xf32> {llvm.noalias}
// ON-SAME: %B: memref<?x8xf32> {llvm.noalias}
// ON-SAME: %C: memref<16x8xf32> {llvm.noalias}
// ON: affine.load %A[%i, %k] : memref<16x12xf32>
// ON: affine.load %B[%k, %j] : memref<?x8xf32>
// ON: affine.store {{.*}}, %C[%i, %j] : memref<16x8xf32>
// ON-LABEL: func.func @macro_static(%A: memref<16x12xf32>
// ON-SAME: %B: memref<?x8xf32>
// ON-SAME: %C: memref<16x8xf32>
//
// Rows reached are the extent, not the bound written (C does not enforce it).
// ON-LABEL: func.func @fewer_rows(%A: memref<4x12xf32>
// ON-SAME: %C: memref<4x8xf32>
//
// A subscript offset by a parameter: its extent is known at run time only.
// ON-LABEL: func.func @offset(%A: memref<?x12xf32>
// ON-SAME: %C: memref<16x8xf32>
//
// E % C: the affine expression (E) mod C, and the rows it reaches.
// ON-LABEL: func.func @perm(%A: memref<16x12xf32> {llvm.noalias}
// ON: affine.load %A[(((5 * %i)) mod 16), %k] : memref<16x12xf32>
// ON-LABEL: func.func @half(%A: memref<8x12xf32>
// ON: affine.load %A[((%i) mod 8), %k] : memref<8x12xf32>
// IFACE: "name": "perm"
// IFACE-SAME: "name": "A", {{.*}} "leading_extent": 16, "leading_min": 0
// IFACE: "name": "half"
// IFACE-SAME: "name": "A", {{.*}} "leading_extent": 8, "leading_min": 0
//
// Refused: a left operand that can be negative, a modulus that is no positive
// constant, a parameter in the left operand.
// NOTES-DAG: note: 'negative' is outside the supported C subset and is not translated: the left operand of % in this subscript is -3 at a point of the nest; C's % of a negative value is not the floor modulo of an affine map
// NOTES-DAG: note: 'zero_mod' is outside the supported C subset and is not translated: the right operand of % in a subscript is 0; only a positive constant is supported
// NOTES-DAG: note: 'var_mod' is outside the supported C subset and is not translated: the right operand of % in a subscript is not an integer constant
// NOTES-DAG: note: 'param_mod' is outside the supported C subset and is not translated: the left operand of % in a subscript reads a parameter; only loop counters and constants are supported there

#define ROWS 16

void bounds(float A[restrict 16][12], float (*restrict B)[8],
            float C[restrict 16][8]) {
  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 12; ++k)
        C[i][j] += A[i][k] * B[k][j];
}

void macro_static(float A[static ROWS][12], float B[][8], float C[ROWS][8]) {
  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 12; ++k)
        C[i][j] += A[i][k] * B[k][j];
}

void fewer_rows(float A[16][12], float B[12][8], float C[16][8]) {
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 12; ++k)
        C[i][j] += A[i][k] * B[k][j];
}

void offset(float A[32][12], float B[12][8], float C[16][8], int r0) {
  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 12; ++k)
        C[i][j] += A[i + r0][k] * B[k][j];
}

void perm(float A[restrict 16][12], float B[restrict 12][8],
          float C[restrict 16][8]) {
  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 12; ++k)
        C[i][j] += A[(5 * i) % 16][k] * B[k][j];
}

void half(float A[8][12], float B[12][8], float C[16][8]) {
  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 12; ++k)
        C[i][j] += A[i % 8][k] * B[k][j];
}

void negative(float A[16][12], float C[16][8]) {
  for (int i = 0; i < 16; ++i)
    C[i][0] = A[(i - 3) % 16][0];
}

void zero_mod(float A[16][12], float C[16][8]) {
  for (int i = 0; i < 16; ++i)
    C[i][0] = A[i % 0][0];
}

void var_mod(float A[16][12], float C[16][8], int m) {
  for (int i = 0; i < 16; ++i)
    C[i][0] = A[i % m][0];
}

void param_mod(float A[16][12], float C[16][8], int r) {
  for (int i = 0; i < 16; ++i)
    C[i][0] = A[(i + r) % 16][0];
}
