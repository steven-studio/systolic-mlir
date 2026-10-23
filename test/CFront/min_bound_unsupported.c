// REQUIRES: cfront-clang
//
// S5.5 (docs/AFFINE_GEMM_MAPPING.md section 26.5): what only looks like a min
// loop bound is refused, with the reason -- never translated as one. Standard
// C has no min function: a call is not translated, whatever its name (its
// body, side effects and conversions are not analysed), nor a call inside an
// operand of the MIN macro; a max is not a min; a conditional whose branches
// are not the two sides of its comparison, a min of three operands, a
// conversion between the counter's type and the bound's (signed and unsigned
// are not mixed, nor int and long), an operand that overflows its type at
// some outer point, and an && that does not compare the counter on both
// sides are refused.
//
// RUN: not %cfront --function=call_min %s -o %t.1 2>&1 | FileCheck %s --check-prefix=CALL
// RUN: not %cfront --function=side_effect %s -o %t.2 2>&1 | FileCheck %s --check-prefix=SIDE
// RUN: not %cfront --function=max_bound %s -o %t.3 2>&1 | FileCheck %s --check-prefix=MAX
// RUN: not %cfront --function=not_min %s -o %t.4 2>&1 | FileCheck %s --check-prefix=NOTMIN
// RUN: not %cfront --function=nested %s -o %t.5 2>&1 | FileCheck %s --check-prefix=NESTED
// RUN: not %cfront --function=unsigned_mix %s -o %t.6 2>&1 | FileCheck %s --check-prefix=UNSIGNED
// RUN: not %cfront --function=long_counter %s -o %t.7 2>&1 | FileCheck %s --check-prefix=LONG
// RUN: not %cfront --function=overflow %s -o %t.8 2>&1 | FileCheck %s --check-prefix=OVERFLOW
// RUN: not %cfront --function=and_not_counter %s -o %t.9 2>&1 | FileCheck %s --check-prefix=AND
// RUN: not %cfront --function=or_bound %s -o %t.10 2>&1 | FileCheck %s --check-prefix=OR
// RUN: not test -e %t.1

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

static inline int min(int a, int b) { return a < b ? a : b; }
int next(void);

// CALL: min_bound_unsupported.c:[[@LINE+3]]:{{[0-9]+}}: error: the loop bound calls 'min': a function call is not translated (its body, side effects and conversions are not analysed)
void call_min(float A[restrict 10][4]) {
  for (int ii = 0; ii < 10; ii += 4)
    for (int i = ii; i < min(ii + 4, 10); ++i)
      A[i][0] = 1.0f;
}

// SIDE: min_bound_unsupported.c:[[@LINE+3]]:{{[0-9]+}}: error: the loop bound calls 'next': a function call is not translated
void side_effect(float A[restrict 10][4]) {
  for (int ii = 0; ii < 10; ii += 4)
    for (int i = ii; i < MIN(ii + 4, next()); ++i)
      A[i][0] = 1.0f;
}

// MAX: min_bound_unsupported.c:[[@LINE+3]]:{{[0-9]+}}: error: the loop bound is the maximum of its two operands, not the minimum
void max_bound(float A[restrict 16][4]) {
  for (int ii = 0; ii < 10; ii += 4)
    for (int i = ii; i < MAX(ii + 4, 10); ++i)
      A[i][0] = 1.0f;
}

// NOTMIN: min_bound_unsupported.c:[[@LINE+3]]:{{[0-9]+}}: error: the loop bound is a conditional expression whose branches are not the two sides of its comparison: not a min
void not_min(float A[restrict 10][4]) {
  for (int ii = 0; ii < 10; ii += 4)
    for (int i = ii; i < (ii + 4 < 10 ? ii + 4 : 9); ++i)
      A[i][0] = 1.0f;
}

// NESTED: min_bound_unsupported.c:[[@LINE+3]]:{{[0-9]+}}: error: the loop bound is a min of more than two operands (a nested conditional): not translated
void nested(float A[restrict 12][4]) {
  for (int ii = 0; ii < 10; ii += 4)
    for (int i = ii; i < MIN(MIN(ii + 4, 10), 12); ++i)
      A[i][0] = 1.0f;
}

// UNSIGNED: min_bound_unsupported.c:[[@LINE+3]]:{{[0-9]+}}: error: the loop condition compares the counter with a conditional expression of another type, or not the counter itself: the conversion is not translated
void unsigned_mix(float A[restrict 10][4]) {
  for (int ii = 0; ii < 10; ii += 4)
    for (int i = ii; i < MIN(ii + 4u, 10u); ++i)
      A[i][0] = 1.0f;
}

// LONG: min_bound_unsupported.c:[[@LINE+3]]:{{[0-9]+}}: error: the loop condition compares the counter with a conditional expression of another type, or not the counter itself: the conversion is not translated
void long_counter(float A[restrict 10][4]) {
  for (int ii = 0; ii < 10; ii += 4)
    for (long i = ii; i < MIN(ii + 4, 10); ++i)
      A[i][0] = 1.0f;
}

// At ii = 4, ii + 2147483645 is 2147483649, which does not fit int (C would
// overflow while evaluating the bound; at ii = 0 it fits).
// OVERFLOW: min_bound_unsupported.c:[[@LINE+3]]:{{[0-9]+}}: error: the loop bound overflows 'int' (an operation's exact value is 2147483649)
void overflow(float A[restrict 10][4]) {
  for (int ii = 0; ii < 8; ii += 4)
    for (int i = ii; i < MIN(ii + 2147483645, 10); ++i)
      A[i][0] = 1.0f;
}

// AND: min_bound_unsupported.c:[[@LINE+3]]:{{[0-9]+}}: error: a loop condition joined by && must be 'counter < a && counter < b'
void and_not_counter(float A[restrict 10][4]) {
  for (int ii = 0; ii < 10; ii += 4)
    for (int i = ii; i < ii + 4 && ii < 10; ++i)
      A[i][0] = 1.0f;
}

// OR: min_bound_unsupported.c:[[@LINE+3]]:{{[0-9]+}}: error: the loop condition must be 'counter < bound', 'counter <= bound', 'counter > bound' or 'counter >= bound'
void or_bound(float A[restrict 10][4]) {
  for (int ii = 0; ii < 10; ii += 4)
    for (int i = ii; i < ii + 4 || i < 10; ++i)
      A[i][0] = 1.0f;
}
