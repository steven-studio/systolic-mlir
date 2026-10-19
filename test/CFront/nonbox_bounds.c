// REQUIRES: cfront-clang
//
// Loop bounds affine in the enclosing counters with DIFFERENT counter terms
// (docs/AFFINE_GEMM_MAPPING.md section 19.13): the front end translates the
// loop as written -- affine.for with affine_map bounds over the enclosing
// counters, its values at each outer point LB(x), LB(x) + S, ... below UB(x) --
// and computes every subscript range in its scope exactly, by running the
// nest's counters (a hull of the counters' ranges would not be exact: the
// interface's leading_extent sizes the memref descriptors and the copies of
// systolic-cc). Loop conditions that compare an affine expression of the
// counter and the enclosing counters are canonicalized into `counter OP
// bound` (they emit the same affine.for as the canonical spelling, and their
// sides are checked as cheaply: in closed form on a parallelotope). The GEMM
// sites' `executions` count the points of the loops around a site exactly.
//
// RUN: %cfront --interface %t.json --gemm-sites %t.sites %s -o %t.mlir
// RUN: FileCheck %s --input-file=%t.mlir
// RUN: FileCheck %s --check-prefix=IFACE --input-file=%t.json
// RUN: FileCheck %s --check-prefix=SITES --input-file=%t.sites
// RUN: systolic-opt %t.mlir -o /dev/null

// A triangle that degenerates: i = 2, 4, ..., 24, but j's range [i, 4) is
// empty for i >= 4, so the points are i = 2, j in {2, 3}: rows 2 of A and C
// only (leading_extent 3, leading_min 2), all of B's rows 0..3.
void degenerate(float A[restrict 4][4], float B[restrict 4][4], float C[restrict 4][4]) {
  for (int i = 2; i < 25; i += 2)
    for (int j = i; j < 4; ++j)
      for (int k = 0; k < 4; ++k)
        C[i][j] += A[i][k] * B[k][j];
}
// CHECK-LABEL: func.func @degenerate(
// CHECK:       affine.for %i = 2 to 25 step 2 {
// CHECK-NEXT:    affine.for %j = affine_map<(d0) -> (0 + d0 * 1)>(%i) to 4 {
// CHECK-NEXT:      affine.for %k = 0 to 4 {
// IFACE:       {"name": "degenerate",
// IFACE-SAME:  {"name": "A", "ctype": "float (*restrict)[4]", "kind": "memref", "elem": "f32", "leading_extent": 3, "leading_min": 2,
// IFACE-SAME:  {"name": "B", "ctype": "float (*restrict)[4]", "kind": "memref", "elem": "f32", "leading_extent": 4, "leading_min": 0,
// IFACE-SAME:  {"name": "C", "ctype": "float (*restrict)[4]", "kind": "memref", "elem": "f32", "leading_extent": 3, "leading_min": 2,

// A genuine triangle (j in [i, 8)): translated; rows 0..7 of every operand.
void triangle(float A[restrict 8][8], float B[restrict 8][8], float C[restrict 8][8]) {
  for (int i = 0; i < 8; ++i)
    for (int j = i; j < 8; ++j)
      for (int k = 0; k < 8; ++k)
        C[i][j] += A[i][k] * B[k][j];
}
// CHECK-LABEL: func.func @triangle(
// CHECK:       affine.for %i = 0 to 8 {
// CHECK-NEXT:    affine.for %j = affine_map<(d0) -> (0 + d0 * 1)>(%i) to 8 {
// IFACE:       {"name": "triangle",
// IFACE-SAME:  {"name": "A", {{[^}]*}}"leading_extent": 8, "leading_min": 0,
// IFACE-SAME:  {"name": "B", {{[^}]*}}"leading_extent": 8, "leading_min": 0,
// IFACE-SAME:  {"name": "C", {{[^}]*}}"leading_extent": 8, "leading_min": 0,

// Exact, not a hull: on the triangle 0 <= i <= j < 4, j - i runs over 0..3
// (a hull of i in 0..3 and j in 0..3 would give -3..3: leading_min -3) and
// i + 4 - j over 1..4 (the hull: 1..7, leading_extent 8).
void exact_ranges(float A[restrict 8], float C[restrict 8][4]) {
  for (int i = 0; i < 4; ++i)
    for (int j = i; j < 4; ++j)
      C[j - i][i] = A[i + 4 - j];
}
// IFACE:       {"name": "exact_ranges",
// IFACE-SAME:  {"name": "A", {{[^}]*}}"leading_extent": 5, "leading_min": 1,
// IFACE-SAME:  {"name": "C", {{[^}]*}}"leading_extent": 4, "leading_min": 0,

// A counting-down outer loop under a triangle: i = 3, 2, 1, 0 (the ordinal
// loop, the counter 3 - %i), k in [i, 4): the bound reads the counter's value.
void down_outer(float A[restrict 4][4]) {
  for (int i = 3; i >= 0; --i)
    for (int k = i; k < 4; ++k)
      A[i][k] = 1.0f;
}
// CHECK-LABEL: func.func @down_outer(
// CHECK:       affine.for %i = 0 to 4 {
// CHECK-NEXT:    affine.for %k = affine_map<(d0) -> (3 - d0 * 1)>(%i) to 4 {
// IFACE:       {"name": "down_outer",
// IFACE-SAME:  {"name": "A", {{[^}]*}}"leading_extent": 4, "leading_min": 0,

// A triangle of repetition loops around a GEMM: (r1, r0) in {(0, 0), (0, 1),
// (1, 1)} -- the GEMM site runs 3 times per call (its `executions`).
void rep_triangle(float A[restrict 4][4], float B[restrict 4][4], float C[restrict 4][4]) {
  for (int r1 = 0; r1 <= 2; r1++)
    for (int r0 = r1; r0 <= 1; r0++)
      for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
          for (int k = 0; k < 4; ++k)
            C[i][j] += A[i][k] * B[k][j];
}
// CHECK-LABEL: func.func @rep_triangle(
// CHECK:       affine.for %r1 = 0 to 3 {
// CHECK-NEXT:    affine.for %r0 = affine_map<(d0) -> (0 + d0 * 1)>(%r1) to 2 {
// SITES:       {"function": "degenerate", {{.*}}"executions": 1}
// SITES:       {"function": "triangle", {{.*}}"executions": 1}
// SITES:       {"function": "rep_triangle", {{.*}}"executions": 3}

// A constant-extent shifted loop around a GEMM: the site runs once per point
// (s, t), t in [s, s + 2): 2 x 2 = 4 times.
void shifted_around(float A[restrict 4][4], float B[restrict 4][4], float C[restrict 4][4]) {
  for (int s = 0; s < 2; ++s)
    for (int t = s; t < s + 2; ++t)
      for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
          for (int k = 0; k < 4; ++k)
            C[i][j] += A[i][k] * B[k][j];
}
// SITES:       {"function": "shifted_around", {{.*}}"executions": 4}

// A repetition triangle spelled with the enclosing counter in the condition
// (s + r < 3 is s < 3 - r): 3 + 2 + 1 = 6 executions (not 3 x 3: the
// constant on the right is no bound of s by itself).
void cond_rep_triangle(float A[restrict 4][4], float B[restrict 4][4], float C[restrict 4][4]) {
  for (int r = 0; r < 3; ++r)
    for (int s = 0; s + r < 3; ++s)
      for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
          for (int k = 0; k < 4; ++k)
            C[i][j] += A[i][k] * B[k][j];
}
// CHECK-LABEL: func.func @cond_rep_triangle(
// CHECK:       affine.for %s = 0 to affine_map<(d0) -> (3 - d0 * 1)>(%r) {
// SITES:       {"function": "cond_rep_triangle", {{.*}}"executions": 6}

// Canonicalized conditions: k - i < 4 and i + 4 > k are k < i + 4; k + i < 8
// is k < 8 - i (a triangle); 8 > k is k < 8. Each emits the loop of the
// canonical spelling.
void cond_canonical(float A[restrict 4][8]) {
  for (int i = 0; i < 4; ++i)
    for (int k = i; k < i + 4; ++k)
      A[i][k] = 1.0f;
}
// CHECK-LABEL: func.func @cond_canonical(
// CHECK:       affine.for %k = affine_map<(d0) -> (0 + d0 * 1)>(%i) to affine_map<(d0) -> (4 + d0 * 1)>(%i) {
void cond_minus(float A[restrict 4][8]) {
  for (int i = 0; i < 4; ++i)
    for (int k = i; k - i < 4; ++k)
      A[i][k] = 1.0f;
}
// CHECK-LABEL: func.func @cond_minus(
// CHECK:       affine.for %k = affine_map<(d0) -> (0 + d0 * 1)>(%i) to affine_map<(d0) -> (4 + d0 * 1)>(%i) {
void cond_reversed(float A[restrict 4][8]) {
  for (int i = 0; i < 4; ++i)
    for (int k = i; i + 4 > k; ++k)
      A[i][k] = 1.0f;
}
// CHECK-LABEL: func.func @cond_reversed(
// CHECK:       affine.for %k = affine_map<(d0) -> (0 + d0 * 1)>(%i) to affine_map<(d0) -> (4 + d0 * 1)>(%i) {
void cond_triangle(float A[restrict 8][8]) {
  for (int i = 0; i < 8; ++i)
    for (int k = 0; k + i < 8; ++k)
      A[i][k] = 1.0f;
}
// CHECK-LABEL: func.func @cond_triangle(
// CHECK:       affine.for %k = 0 to affine_map<(d0) -> (8 - d0 * 1)>(%i) {
void cond_triangle_canonical(float A[restrict 8][8]) {
  for (int i = 0; i < 8; ++i)
    for (int k = 0; k < 8 - i; ++k)
      A[i][k] = 1.0f;
}
// CHECK-LABEL: func.func @cond_triangle_canonical(
// CHECK:       affine.for %k = 0 to affine_map<(d0) -> (8 - d0 * 1)>(%i) {
// The shifted stepped loop k = 2i, 2i + 2 (k <= 2i + 3): the bound 2i + 4.
void cond_step(float A[restrict 4][16]) {
  for (int i = 0; i < 4; ++i)
    for (int k = 2 * i; k <= 2 * i + 3; k += 2)
      A[i][k] = 1.0f;
}
// CHECK-LABEL: func.func @cond_step(
// CHECK:       affine.for %k = affine_map<(d0) -> (0 + d0 * 2)>(%i) to affine_map<(d0) -> (4 + d0 * 2)>(%i) step 2 {
void cond_const_left(float A[restrict 8]) {
  for (int k = 0; 8 > k; ++k)
    A[k] = 1.0f;
}
// CHECK-LABEL: func.func @cond_const_left(
// CHECK:       affine.for %k = 0 to 8 {

// A canonicalized condition on bounds that shift together is checked in
// closed form, as the canonical spelling is: k - j < 4 around a 2048 x 2048
// GEMM is translated -- the loop of k < j + 4 -- without running the
// 4,196,352 values of its outer loops.
void cond_minus_large(float A[restrict 2048][2051], float B[restrict 2051][2048], float C[restrict 2048][2048]) {
  for (int i = 0; i < 2048; ++i)
    for (int j = 0; j < 2048; ++j)
      for (int k = j; k - j < 4; ++k)
        C[i][j] += A[i][k] * B[k][j];
}
// CHECK-LABEL: func.func @cond_minus_large(
// CHECK:       affine.for %k = affine_map<(d0) -> (0 + d0 * 1)>(%j) to affine_map<(d0) -> (4 + d0 * 1)>(%j) {
// IFACE:       {"name": "cond_minus_large",
// IFACE-SAME:  {"name": "A", {{[^}]*}}"leading_extent": 2048, "leading_min": 0,
// IFACE-SAME:  {"name": "B", {{[^}]*}}"leading_extent": 2051, "leading_min": 0,

// Enclosing-counter terms that cancel: k + i < 8 + i is k < 8 (its sides,
// at most 11, checked over the enclosing loop).
void cond_cancel(float A[restrict 4][8]) {
  for (int i = 0; i < 4; ++i)
    for (int k = 0; k + i < 8 + i; ++k)
      A[i][k] = 1.0f;
}
// CHECK-LABEL: func.func @cond_cancel(
// CHECK:       affine.for %k = 0 to 8 {

// Unsigned arithmetic is exact where it does not wrap around: k + u < 4 (u,
// k unsigned; k + u at most 4) is k < 4 - u.
void cond_unsigned(float A[restrict 4][4]) {
  for (unsigned u = 0; u < 4; ++u)
    for (unsigned k = 0; k + u < 4; ++k)
      A[u][k] = 1.0f;
}
// CHECK-LABEL: func.func @cond_unsigned(
// CHECK:       affine.for %k = 0 to affine_map<(d0) -> (4 - d0 * 1)>(%u) {
