// REQUIRES: cfront-clang
//
// S5.5 (docs/AFFINE_GEMM_MAPPING.md section 26.5): a loop upper bound that is
// the minimum of two bounds affine in the enclosing counters becomes the
// affine.for upper bound min (P, Q), exactly. Every spelling of
// min(ii + 4, 10) below runs i over ii, ii + 1, ... while i < ii + 4 and
// i < 10 -- for ii = 0, 4, 8: i = 0..3, 4..7, 8..9 -- and is translated to
// the same exclusive bound: the MIN macro's conditional, the conditional with
// <= and inclusive operands, with > and its branches exchanged, and the &&
// of two comparisons (< or <=). Operands with the same counter terms fold to
// one bound (min(L + a, L + b) = L + min(a, b)), constants to a constant. The
// subscript ranges of the interface and the executions of the GEMM sites are
// computed by running the nest, the min evaluated at every outer point.
//
// RUN: %cfront --interface %t.json --gemm-sites %t.sites %s -o %t.mlir
// RUN: FileCheck %s --input-file=%t.mlir
// RUN: FileCheck %s --check-prefix=IFACE --input-file=%t.json
// RUN: FileCheck %s --check-prefix=SITES --input-file=%t.sites
// RUN: systolic-opt %t.mlir -o /dev/null

#define MIN(a, b) ((a) < (b) ? (a) : (b))

void macro_min(float A[restrict 10][6], float B[restrict 6][5], float C[restrict 10][5]) {
  for (int ii = 0; ii < 10; ii += 4)
    for (int i = ii; i < MIN(ii + 4, 10); ++i)
      for (int j = 0; j < 5; ++j)
        for (int k = 0; k < 6; ++k)
          C[i][j] += A[i][k] * B[k][j];
}
// CHECK-LABEL: func.func @macro_min(
// CHECK:       affine.for %ii = 0 to 10 step 4 {
// CHECK-NEXT:    affine.for %i = affine_map<(d0) -> (0 + d0 * 1)>(%ii) to min affine_map<(d0) -> (4 + d0 * 1, 10)>(%ii) {
// CHECK-NEXT:      affine.for %j = 0 to 5 {
// IFACE:       {"name": "macro_min",
// IFACE-SAME:  {"name": "A", "ctype": "float (*restrict)[6]", "kind": "memref", "elem": "f32", "leading_extent": 10, "leading_min": 0,
// IFACE-SAME:  {"name": "C", "ctype": "float (*restrict)[5]", "kind": "memref", "elem": "f32", "leading_extent": 10, "leading_min": 0,
// SITES:       {"function": "macro_min", {{.*}}"executions": 3}

void ternary_le(float A[restrict 10][6], float B[restrict 6][5], float C[restrict 10][5]) {
  for (int ii = 0; ii < 10; ii += 4)
    for (int i = ii; i <= ((ii + 3) <= 9 ? (ii + 3) : 9); ++i)
      for (int j = 0; j < 5; ++j)
        for (int k = 0; k < 6; ++k)
          C[i][j] += A[i][k] * B[k][j];
}
// CHECK-LABEL: func.func @ternary_le(
// CHECK:       affine.for %ii = 0 to 10 step 4 {
// CHECK-NEXT:    affine.for %i = affine_map<(d0) -> (0 + d0 * 1)>(%ii) to min affine_map<(d0) -> (4 + d0 * 1, 10)>(%ii) {

void ternary_gt(float A[restrict 10][6], float B[restrict 6][5], float C[restrict 10][5]) {
  for (int ii = 0; ii < 10; ii += 4)
    for (int i = ii; i < (ii + 4 > 10 ? 10 : ii + 4); ++i)
      for (int j = 0; j < 5; ++j)
        for (int k = 0; k < 6; ++k)
          C[i][j] += A[i][k] * B[k][j];
}
// CHECK-LABEL: func.func @ternary_gt(
// CHECK:       affine.for %ii = 0 to 10 step 4 {
// CHECK-NEXT:    affine.for %i = affine_map<(d0) -> (0 + d0 * 1)>(%ii) to min affine_map<(d0) -> (4 + d0 * 1, 10)>(%ii) {

void and_and(float A[restrict 10][6], float B[restrict 6][5], float C[restrict 10][5]) {
  for (int ii = 0; ii < 10; ii += 4)
    for (int i = ii; i <= ii + 3 && i < 10; ++i)
      for (int j = 0; j < 5; ++j)
        for (int k = 0; k < 6; ++k)
          C[i][j] += A[i][k] * B[k][j];
}
// CHECK-LABEL: func.func @and_and(
// CHECK:       affine.for %ii = 0 to 10 step 4 {
// CHECK-NEXT:    affine.for %i = affine_map<(d0) -> (0 + d0 * 1)>(%ii) to min affine_map<(d0) -> (4 + d0 * 1, 10)>(%ii) {
// SITES:       {"function": "and_and", {{.*}}"executions": 3}

// The same counter terms: min(ii + 4, ii + 6) is ii + 4, one bound (a
// parallelotope, section 19.12); constants fold: min(8, 6) is 6.
void folded(float A[restrict 12][6], float B[restrict 6][5], float C[restrict 12][5]) {
  for (int ii = 0; ii < 12; ii += 4)
    for (int i = ii; i < MIN(ii + 4, ii + 6); ++i)
      for (int j = 0; j < 5; ++j)
        for (int k = 0; k < MIN(8, 6); ++k)
          C[i][j] += A[i][k] * B[k][j];
}
// CHECK-LABEL: func.func @folded(
// CHECK:       affine.for %ii = 0 to 12 step 4 {
// CHECK-NEXT:    affine.for %i = affine_map<(d0) -> (0 + d0 * 1)>(%ii) to affine_map<(d0) -> (4 + d0 * 1)>(%ii) {
// CHECK-NEXT:      affine.for %j = 0 to 5 {
// CHECK-NEXT:        affine.for %k = 0 to 6 {

// M = 0: the tile loop runs no iteration; the min loop is translated, and
// never runs (no subscript range, no execution).
void m_zero(float A[restrict 4][6], float B[restrict 6][5], float C[restrict 4][5]) {
  for (int ii = 0; ii < 0; ii += 4)
    for (int i = ii; i < MIN(ii + 4, 0); ++i)
      for (int j = 0; j < 5; ++j)
        for (int k = 0; k < 6; ++k)
          C[i][j] += A[i][k] * B[k][j];
}
// CHECK-LABEL: func.func @m_zero(
// CHECK:       affine.for %ii = 0 to 0 step 4 {
// CHECK-NEXT:    affine.for %i = affine_map<(d0) -> (0 + d0 * 1)>(%ii) to min affine_map<(d0) -> (4 + d0 * 1, 0)>(%ii) {
// SITES:       {"function": "m_zero", {{.*}}"executions": 0}

// An enclosing loop that counts down: its counter is 8 - 4 t in the emitted
// IR (t the ordinal), substituted in both operands of the min.
void tiles_down(float A[restrict 10][6], float B[restrict 6][5], float C[restrict 10][5]) {
  for (int ii = 8; ii >= 0; ii -= 4)
    for (int i = ii; i < MIN(ii + 4, 10); ++i)
      for (int j = 0; j < 5; ++j)
        for (int k = 0; k < 6; ++k)
          C[i][j] += A[i][k] * B[k][j];
}
// CHECK-LABEL: func.func @tiles_down(
// CHECK:       affine.for %ii = 0 to 3 {
// CHECK-NEXT:    affine.for %i = affine_map<(d0) -> (8 - d0 * 4)>(%ii) to min affine_map<(d0) -> (12 - d0 * 4, 10)>(%ii) {
// IFACE:       {"name": "tiles_down",
// IFACE-SAME:  {"name": "A", "ctype": "float (*restrict)[6]", "kind": "memref", "elem": "f32", "leading_extent": 10, "leading_min": 0,
// SITES:       {"function": "tiles_down", {{.*}}"executions": 3}

// Commuted operands retain their AST types and arithmetic grouping. The
// short outer counter is promoted to int in BOTH comparison and branch.
void promoted_commute(float A[restrict 10][4]) {
  for (short tile = 0; tile < 10; tile += 4)
    for (int point = tile; (tile + 4 < 10 ? 4 + tile : 10) > point; ++point)
      A[point][0] = 1.0f;
}
// CHECK-LABEL: func.func @promoted_commute(
// CHECK: affine.for %tile = 0 to 10 step 4
// CHECK: affine.for %point = {{.*}} to min affine_map<(d0) -> (4 + d0 * 1, 10)>

void commuted_product(float A[restrict 10][4]) {
  for (int tile = 0; tile < 5; tile += 2)
    for (int point = 2 * tile; point < (2 * tile + 4 < 10 ? 4 + tile * 2 : 10); ++point)
      A[point][0] = 1.0f;
}
// CHECK-LABEL: func.func @commuted_product(
// CHECK: affine.for %point = {{.*}} to min affine_map<(d0) -> (4 + d0 * 2, 10)>

void swapped_inclusive(float A[restrict 10][4]) {
  for (int tile = 0; tile < 10; tile = tile + 4)
    for (int point = tile; ((tile + 3 >= point) && (point <= 9)); point++)
      A[point][0] = 1.0f;
}
// CHECK-LABEL: func.func @swapped_inclusive(
// CHECK: affine.for %point = {{.*}} to min affine_map<(d0) -> (4 + d0 * 1, 10)>
