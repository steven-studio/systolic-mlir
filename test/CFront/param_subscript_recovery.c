// REQUIRES: cfront-clang
//
// A function refused while one of its subscripts is being parsed (here: a
// subscript that uses an unsigned parameter) must not change how the
// functions after it are translated. A signed int parameter is an affine
// symbol only inside a subscript; in a loop bound it stays outside the
// subset, whatever came before (regression test: the subscript state once
// survived the abandoned function, and the next function's loop bound `n`
// was accepted as an affine map on the i32 parameter -- invalid IR).
//
// RUN: %cfront --skip-unsupported %s -o %t.mlir 2>&1 | FileCheck %s
// RUN: FileCheck %s --check-prefix=IR --input-file=%t.mlir --implicit-check-not='func.func @unsigned_offset' --implicit-check-not='func.func @bound_after_refusal'
// RUN: %cfront --skip-unsupported --function=bound_after_refusal %s -o %t.alone.mlir 2>&1 | FileCheck %s --check-prefix=ALONE
//
// CHECK: note: 'unsigned_offset' is outside the supported C subset and is not translated: subscript uses the unsigned parameter 'r0'
// CHECK: note: 'bound_after_refusal' is outside the supported C subset and is not translated: the loop bound must be an integer constant or an affine expression of the enclosing loop counters
// CHECK-NOT: note:
//
// ALONE: note: 'bound_after_refusal' is outside the supported C subset and is not translated: the loop bound must be an integer constant or an affine expression of the enclosing loop counters
//
// IR-LABEL: func.func @offset_after_refusal
// IR:       arith.index_cast %r0 : i32 to index
// IR:       affine.load %C[(symbol(%r0_idx) + %i), %j]

void unsigned_offset(float A[restrict 8][16], float B[restrict 16][16],
                     float C[restrict 32][16], unsigned r0) {
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 16; ++j)
      for (int k = 0; k < 16; ++k)
        C[r0 + i][j] += A[i][k] * B[k][j];
}

void bound_after_refusal(float A[restrict 8][16], float B[restrict 16][16],
                         float C[restrict 8][16], int n) {
  for (int i = 0; i < n; ++i)
    for (int j = 0; j < 16; ++j)
      for (int k = 0; k < 16; ++k)
        C[i][j] += A[i][k] * B[k][j];
}

// Still a symbol inside a subscript after the refusals above.
void offset_after_refusal(float A[restrict 8][16], float B[restrict 16][16],
                          float C[restrict 32][16], int r0) {
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 16; ++j)
      for (int k = 0; k < 16; ++k)
        C[r0 + i][j] += A[i][k] * B[k][j];
}
