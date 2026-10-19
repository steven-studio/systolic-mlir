// REQUIRES: cfront-clang
//
// Loop conditions that compare an affine expression of the counter with a
// constant, E OP C with E = a * counter + b (a != 0): the front end turns them
// into the equivalent comparison of the counter itself -- exact over the
// integers (2 * i - 3 > 0 is i > 1.5, i.e. i > 1) -- and then translates the
// loop as before (counting down: the ordinal loop 0 to N and LB - S * t).
// The expected trip counts are those of the C loops as written (measured by
// running them).
//
// RUN: %cfront %s -o %t.mlir
// RUN: FileCheck %s --input-file=%t.mlir
// RUN: systolic-opt %t.mlir -o /dev/null

// The original test loop: i = 100, 99, ..., 2 (99 iterations); the condition
// is false for the first time at i = 1 (2 * 1 - 3 = -1).
void original(float a[restrict 128]) {
  for (int i = 100; 2 * i - 3 > 0; i--)
    a[i] = 1.0f;
}
// CHECK-LABEL: func.func @original(
// CHECK:       affine.for %i = 0 to 99 {
// CHECK:         affine.store %{{.*}}, %a[(100 - %i)]

// '>=' (2 * i - 3 >= 0 is i >= 2): the same 99 values.
void original_ge(float a[restrict 128]) {
  for (int i = 100; 2 * i - 3 >= 0; --i)
    a[i] = 1.0f;
}
// CHECK-LABEL: func.func @original_ge(
// CHECK:       affine.for %i = 0 to 99 {

// A negative coefficient turns the comparison around: 100 - 3 * i > 0 is
// i < 33.3, i.e. i <= 33 -- counting up, i = 0 .. 33 (34 iterations).
void negative_coefficient(float a[restrict 128]) {
  for (int i = 0; 100 - 3 * i > 0; i++)
    a[i] = 1.0f;
}
// CHECK-LABEL: func.func @negative_coefficient(
// CHECK:       affine.for %i = 0 to 34 {
// CHECK:         affine.store %{{.*}}, %a[%i]

// 7 - 2 * i < 0 is i > 3.5: i = 40, 37, ..., 4 (13 iterations).
void negative_coefficient_down(float a[restrict 128]) {
  for (int i = 40; 7 - 2 * i < 0; i -= 3)
    a[i] = 1.0f;
}
// CHECK-LABEL: func.func @negative_coefficient_down(
// CHECK:       affine.for %i = 0 to 13 {
// CHECK:         affine.store %{{.*}}, %a[(40 - %i * 3)]

// Counting up with a step: 3 * i + 1 < 20 is i <= 6; i = 0, 2, 4, 6.
void up_step(float a[restrict 128]) {
  for (int i = 0; 3 * i + 1 < 20; i += 2)
    a[i] = 1.0f;
}
// CHECK-LABEL: func.func @up_step(
// CHECK:       affine.for %i = 0 to 7 step 2 {

// -(i - 4) < 0 is i > 4: i = 10 .. 5 (6 iterations).
void parenthesized(float a[restrict 128]) {
  for (int i = 10; -(i - 4) < 0; i = i - 1)
    a[i] = 1.0f;
}
// CHECK-LABEL: func.func @parenthesized(
// CHECK:       affine.for %i = 0 to 6 {

// False from the start (2 * 1 - 3 = -1): no iteration.
void never_runs(float a[restrict 128]) {
  for (int i = 1; 2 * i - 3 > 0; i--)
    a[0] = 1.0f;
}
// CHECK-LABEL: func.func @never_runs(
// CHECK:       affine.for %i = 0 to 0 {

// A long counter: 2 * i is computed in long and fits; i = 3e9, 2e9, 1e9.
void long_counter(float a[restrict 128]) {
  for (long i = 3000000000; 2 * i - 3 > 0; i -= 1000000000)
    a[0] = 1.0f;
}
// CHECK-LABEL: func.func @long_counter(
// CHECK:       affine.for %i = 0 to 3 {

// Unsigned arithmetic is exact where it does not wrap around: 2 * i < 10 is
// i < 5 for an unsigned i (2 * i at most 10).
void unsigned_affine(float a[restrict 128]) {
  for (unsigned i = 0; 2 * i < 10; ++i)
    a[i] = 1.0f;
}
// CHECK-LABEL: func.func @unsigned_affine(
// CHECK:       affine.for %i = 0 to 5 {

// An unsigned char counter (the comparison promotes it to int): its values
// 0 .. 50 fit unsigned char, so c < 50 is exact.
void unsigned_char_counter(float a[restrict 128]) {
  for (unsigned char c = 0; c < 50; ++c)
    a[c] = 1.0f;
}
// CHECK-LABEL: func.func @unsigned_char_counter(
// CHECK:       affine.for %c = 0 to 50 {
