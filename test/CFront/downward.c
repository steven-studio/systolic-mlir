// REQUIRES: cfront-clang
//
// Loops that count down (>, >=; --i, i--, i -= S, i = i - S). affine.for
// only counts up, so the front end emits the ordinal loop
// affine.for %i = 0 to N (N = the trip count) and uses the counter as the
// affine expression LB - S * %i: the same values in the same order.
//
// RUN: %cfront %s -o %t.mlir
// RUN: FileCheck %s --input-file=%t.mlir
// RUN: systolic-opt %t.mlir -o /dev/null

// i = 5, 3, 1: the step does not divide the distance to the bound.
void ge_odd(float a[restrict 8]) {
  for (int i = 5; i >= 0; i -= 2)
    a[i] = 1.0f;
}
// CHECK-LABEL: func.func @ge_odd(
// CHECK:       affine.for %i = 0 to 3 {
// CHECK:         affine.store %{{.*}}, %a[(5 - %i * 2)]

// i = 5, 3, 1 with '>' (0 is excluded, and not reached anyway).
void gt_odd(float a[restrict 8]) {
  for (int i = 5; i > 0; i -= 2)
    a[i] = 1.0f;
}
// CHECK-LABEL: func.func @gt_odd(
// CHECK:       affine.for %i = 0 to 3 {
// CHECK:         affine.store %{{.*}}, %a[(5 - %i * 2)]

// i = 6, 4, 2, 0: the step divides the distance; '>=' includes the bound.
void ge_even(float a[restrict 8]) {
  for (int i = 6; i >= 0; i -= 2)
    a[i] = 1.0f;
}
// CHECK-LABEL: func.func @ge_even(
// CHECK:       affine.for %i = 0 to 4 {
// CHECK:         affine.store %{{.*}}, %a[(6 - %i * 2)]

// i = 6, 4, 2: '>' excludes the bound.
void gt_even(float a[restrict 8]) {
  for (int i = 6; i > 0; i = i - 2)
    a[i] = 1.0f;
}
// CHECK-LABEL: func.func @gt_even(
// CHECK:       affine.for %i = 0 to 3 {
// CHECK:         affine.store %{{.*}}, %a[(6 - %i * 2)]

// --i and i--: step 1.
void unit_steps(float a[restrict 8]) {
  for (int i = 7; i >= 0; --i)
    a[i] = 1.0f;
  for (long i = 7; i > 3; i--)
    a[i] = 2.0f;
}
// CHECK-LABEL: func.func @unit_steps(
// CHECK:       affine.for %i = 0 to 8 {
// CHECK:         affine.store %{{.*}}, %a[(7 - %i)]
// CHECK:       affine.for %i_1 = 0 to 4 {
// CHECK:         affine.store %{{.*}}, %a[(7 - %i_1)]

// A downward counter inside an affine subscript, next to an upward one.
void mixed(float a[restrict 8][8]) {
  for (int i = 0; i < 8; i += 2)
    for (int j = 7; j > 1; j -= 3)
      a[i][2 * j - 7] = 1.0f;
}
// CHECK-LABEL: func.func @mixed(
// CHECK:       affine.for %i = 0 to 8 step 2 {
// CHECK:         affine.for %j = 0 to 2 {
// CHECK:           affine.store %{{.*}}, %a[%i, ((2 * (7 - %j * 3)) - 7)]

// Loops that never run: a downward loop whose start is below its bound, and
// an upward one for comparison.
void empty(float a[restrict 8]) {
  for (int i = 0; i > 0; --i)
    a[i] = 1.0f;
  for (int i = -1; i >= 0; i -= 2)
    a[0] = 2.0f;
  for (int i = 8; i < 8; ++i)
    a[0] = 3.0f;
}
// CHECK-LABEL: func.func @empty(
// CHECK:       affine.for %i = 0 to 0 {
// CHECK:       affine.for %i_1 = 0 to 0 {
// CHECK:       affine.for %i_2 = 8 to 8 {

// An unsigned counter counting down to a bound it reaches exactly: 6, 4, 2.
void unsigned_exact(float a[restrict 8]) {
  for (unsigned i = 6; i > 0; i -= 2)
    a[i] = 1.0f;
}
// CHECK-LABEL: func.func @unsigned_exact(
// CHECK:       affine.for %i = 0 to 3 {
// CHECK:         affine.store %{{.*}}, %a[(6 - %i * 2)]

// Counting up is unchanged.
void upward(float a[restrict 8]) {
  for (int i = 1; i < 6; i += 2)
    a[i] = 1.0f;
}
// CHECK-LABEL: func.func @upward(
// CHECK:       affine.for %i = 1 to 6 step 2 {
// CHECK:         affine.store %{{.*}}, %a[%i]
