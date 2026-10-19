// REQUIRES: cfront-clang
//
// Affine loop conditions the front end must refuse. C evaluates the condition
// before every iteration and once more when the loop ends; a signed overflow
// in it -- on an iteration or only at that last evaluation -- is undefined
// behaviour, and an unsigned one wraps around (2 * i - 3 > 0 never becomes
// false for an unsigned i). Conditions that are not E OP C with E affine in
// the counter keep the diagnostic they had.
//
// RUN: not %cfront --function=overflow_first %s -o %t.1 2>&1 | FileCheck %s --check-prefix=FIRST
// RUN: not %cfront --function=overflow_at_exit %s -o %t.2 2>&1 | FileCheck %s --check-prefix=EXIT
// RUN: not %cfront --function=unsigned_condition %s -o %t.3 2>&1 | FileCheck %s --check-prefix=UNSIGNED
// RUN: not %cfront --function=no_counter %s -o %t.4 2>&1 | FileCheck %s --check-prefix=CONST
// RUN: not %cfront --function=not_equal %s -o %t.5 2>&1 | FileCheck %s --check-prefix=NE
// RUN: not %cfront --function=quadratic %s -o %t.6 2>&1 | FileCheck %s --check-prefix=QUAD
// RUN: not test -e %t.1

// 2 * 2000000000 does not fit in int at the first evaluation.
void overflow_first(float a[restrict 8]) {
  for (int i = 2000000000; 2 * i - 3 > 0; i--)
    a[0] = 1.0f;
}

// Every iteration's 2 * i fits; the final evaluation at i = 1073741824 gives
// 2^31, which does not.
void overflow_at_exit(float a[restrict 8]) {
  for (int i = 1073741800; 2 * i < 2147483647; i++)
    a[0] = 1.0f;
}

void unsigned_condition(float a[restrict 8]) {
  for (unsigned i = 10; 2 * i - 3 > 0; i--)
    a[0] = 1.0f;
}

void no_counter(float a[restrict 8]) {
  for (int i = 0; 3 > 0; i++)
    a[0] = 1.0f;
}

void not_equal(float a[restrict 8]) {
  for (int i = 100; 2 * i - 3 != 0; i--)
    a[0] = 1.0f;
}

void quadratic(float a[restrict 8]) {
  for (int i = 0; i * i < 50; i++)
    a[0] = 1.0f;
}

// FIRST: affine_condition_unsupported.c:20:28: error: the loop condition could overflow (it is evaluated for the counter values 2000000000 .. 1)
// EXIT: affine_condition_unsupported.c:27:28: error: the loop condition could overflow (it is evaluated for the counter values 1073741800 .. 1073741824)
// UNSIGNED: affine_condition_unsupported.c:32:25: error: the loop condition could overflow (it is evaluated for the counter values 10 .. 1)
// CONST: affine_condition_unsupported.c:37:19: error: the loop condition must be 'counter < bound', 'counter <= bound', 'counter > bound' or 'counter >= bound'
// NE: affine_condition_unsupported.c:42:21: error: the loop condition must be 'counter < bound', 'counter <= bound', 'counter > bound' or 'counter >= bound'
// QUAD: affine_condition_unsupported.c:47:19: error: the loop condition must be 'counter < bound', 'counter <= bound', 'counter > bound' or 'counter >= bound'
