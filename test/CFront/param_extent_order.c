// REQUIRES: cfront-clang
//
// The run-time extremes the interface gives for an array that a static
// access and a parameter-offset access both reach (leading_min_expr,
// leading_extent_expr: what the caller-side shim sizes its copies of the
// caller's array by) are the extremes over those two accesses, in either
// order (regression test: when the static access came first, the offset
// access's loop part -- without its offset -- was folded into the extreme
// as well, an extent that ran past the caller's array for a negative offset;
// the shim's snapshot of the array then read past it).
//
// RUN: %cfront --interface %t.json %s -o %t.mlir
// RUN: FileCheck %s --input-file=%t.json
//
// CHECK: "name": "static_first"
// CHECK: "name": "C"{{.*}}"leading_extent_expr": "(((7LL) > (27LL + (r0) * 1LL) ? (7LL) : (27LL + (r0) * 1LL)) + 1LL)", "leading_min_expr": "((0LL) < (20LL + (r0) * 1LL) ? (0LL) : (20LL + (r0) * 1LL))"
// CHECK: "name": "symbolic_first"
// CHECK: "name": "C"{{.*}}"leading_extent_expr": "(((27LL + (r0) * 1LL) > (7LL) ? (27LL + (r0) * 1LL) : (7LL)) + 1LL)", "leading_min_expr": "((20LL + (r0) * 1LL) < (0LL) ? (20LL + (r0) * 1LL) : (0LL))"

void static_first(float A[restrict 8][16], float B[restrict 16][16],
                  float C[restrict 32][16], int r0) {
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 16; ++j)
      for (int k = 0; k < 16; ++k)
        C[i][j] += A[i][k] * B[k][j];
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 16; ++j)
      for (int k = 0; k < 16; ++k)
        C[r0 + i + 20][j] += A[i][k] * B[k][j];
}

void symbolic_first(float A[restrict 8][16], float B[restrict 16][16],
                    float C[restrict 32][16], int r0) {
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 16; ++j)
      for (int k = 0; k < 16; ++k)
        C[r0 + i + 20][j] += A[i][k] * B[k][j];
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 16; ++j)
      for (int k = 0; k < 16; ++k)
        C[i][j] += A[i][k] * B[k][j];
}
