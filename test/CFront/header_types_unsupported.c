// REQUIRES: cfront-clang
//
// C evaluates every integer node of a loop header -- the counter's initial
// value, the bound, both sides of the condition -- and of a subscript in that
// node's own type (an operator's promoted or common type; every implicit
// conversion is a node of its own): a signed result outside the type is
// undefined behaviour, an unsigned one wraps around, and a conversion to a
// type that cannot represent the value changes it. The front end computes
// with exact integer values, so it translates a loop or an access only when
// every node's exact value lies in its type's range at every point where C
// evaluates it. Each function below breaks that at some point -- C runs other
// iterations or addresses other elements than the exact values say, or has
// undefined behaviour -- and is refused: on a loop with constant bounds, with
// bounds that shift together and with bounds that do not, and in subscripts.
//
// RUN: not %cfront --function=counter_to_unsigned %s -o %t.1 2>&1 | FileCheck %s --check-prefix=CONVU
// RUN: not %cfront --function=counter_to_unsigned_long %s -o %t.2 2>&1 | FileCheck %s --check-prefix=CONVUL
// RUN: not %cfront --function=constant_side_overflow %s -o %t.3 2>&1 | FileCheck %s --check-prefix=RHS
// RUN: not %cfront --function=init_to_unsigned %s -o %t.4 2>&1 | FileCheck %s --check-prefix=INITU
// RUN: not %cfront --function=init_to_int %s -o %t.5 2>&1 | FileCheck %s --check-prefix=INITI
// RUN: not %cfront --function=bound_to_unsigned %s -o %t.6 2>&1 | FileCheck %s --check-prefix=BOUNDU
// RUN: not %cfront --function=down_init_to_int %s -o %t.7 2>&1 | FileCheck %s --check-prefix=DOWNI
// RUN: not %cfront --function=constant_bound_overflow %s -o %t.8 2>&1 | FileCheck %s --check-prefix=CONST
// RUN: not %cfront --function=shifted_never_runs %s -o %t.9 2>&1 | FileCheck %s --check-prefix=SHEMPTY
// RUN: not %cfront --function=shifted_wrap %s -o %t.10 2>&1 | FileCheck %s --check-prefix=SHWRAP
// RUN: not %cfront --function=shifted_overflow %s -o %t.11 2>&1 | FileCheck %s --check-prefix=SHOVF
// RUN: not %cfront --function=nonbox_init_wrap %s -o %t.12 2>&1 | FileCheck %s --check-prefix=NBINIT
// RUN: not %cfront --function=nonbox_bound_wrap %s -o %t.13 2>&1 | FileCheck %s --check-prefix=NBBOUND
// RUN: not %cfront --function=nonbox_mixed_wrap %s -o %t.14 2>&1 | FileCheck %s --check-prefix=NBMIXED
// RUN: not %cfront --function=nonbox_init_overflow %s -o %t.15 2>&1 | FileCheck %s --check-prefix=NBIOVF
// RUN: not %cfront --function=nonbox_bound_overflow %s -o %t.16 2>&1 | FileCheck %s --check-prefix=NBBOVF
// RUN: not %cfront --function=repetitions_wrap %s -o %t.17 2>&1 | FileCheck %s --check-prefix=REP
// RUN: not %cfront --function=subscript_wrap %s -o %t.18 2>&1 | FileCheck %s --check-prefix=SUBWRAP
// RUN: not %cfront --function=subscript_overflow %s -o %t.19 2>&1 | FileCheck %s --check-prefix=SUBOVF
// RUN: %cfront --skip-unsupported %s -o %t.all 2>/dev/null
// RUN: FileCheck %s --check-prefix=NONE --input-file=%t.all
// RUN: not test -e %t.1

// The comparison converts the counter to unsigned int: at k = -5 the
// condition is 4294967291 < 8, false -- C runs no iteration, the exact values
// would run 13.
// CONVU: header_types_unsupported.c:[[@LINE+2]]:20: error: the loop condition converts -5 to 'unsigned int', which cannot represent it (it is evaluated for the counter values -5 .. 8)
void counter_to_unsigned(float A[restrict 16]) {
  for (int k = -5; k < 8u; ++k)
    A[k + 5] = 1.0f;
}

// CONVUL: header_types_unsupported.c:[[@LINE+2]]:20: error: the loop condition converts -5 to 'unsigned long', which cannot represent it (it is evaluated for the counter values -5 .. 8)
void counter_to_unsigned_long(float A[restrict 16]) {
  for (int k = -5; k < 8ul; ++k)
    A[k + 5] = 1.0f;
}

// The constant side of the condition, -(-2147483647 - 1), overflows int (a
// node that reads no counter: reported at the node, with its value).
// RHS: header_types_unsupported.c:[[@LINE+2]]:34: error: the loop condition overflows 'int' (an operation's exact value is 2147483648)
void constant_side_overflow(float A[restrict 4]) {
  for (int c = -2147483644; -c > -(-2147483647 - 1); c += 2)
    A[0] = 1.0f;
}

// unsigned r = -1 starts at 4294967295: no iteration (the exact values: 11).
// INITU: header_types_unsupported.c:[[@LINE+2]]:21: error: the loop counter's initial value converts -1 to 'unsigned int', which cannot represent it
void init_to_unsigned(float A[restrict 16]) {
  for (unsigned r = -1; r < 10; ++r)
    A[r + 1] = 1.0f;
}

// int r = 3000000000 starts at -1294967296 (clang wraps the conversion): about
// 1.3e9 iterations (the exact values: none).
// INITI: header_types_unsupported.c:[[@LINE+2]]:16: error: the loop counter's initial value converts 3000000000 to 'int', which cannot represent it
void init_to_int(float A[restrict 16]) {
  for (int r = 3000000000; r < 10; ++r)
    A[0] = 1.0f;
}

// k < -1 compares with 4294967295: 4294967295 iterations (the exact values:
// none).
// BOUNDU: header_types_unsupported.c:[[@LINE+2]]:28: error: the loop bound converts -1 to 'unsigned int', which cannot represent it
void bound_to_unsigned(float A[restrict 16]) {
  for (unsigned k = 0; k < -1; ++k)
    A[0] = 1.0f;
}

// int c = 2147483649L starts at -2147483647: no iteration (the exact values:
// 11).
// DOWNI: header_types_unsupported.c:[[@LINE+2]]:16: error: the loop counter's initial value converts 2147483649 to 'int', which cannot represent it
void down_init_to_int(float A[restrict 16]) {
  for (int c = 2147483649L; c >= 2147483639; --c)
    A[0] = 1.0f;
}

// An overflow inside a constant bound: (2147483647 + 1) - 2147483645 is
// undefined behaviour although the exact value, 3, fits.
// CONST: header_types_unsupported.c:[[@LINE+2]]:24: error: the loop bound overflows 'int' (an operation's exact value is 2147483648)
void constant_bound_overflow(float A[restrict 16]) {
  for (int i = 0; i < (2147483647 + 1) - 2147483645; ++i)
    A[i] = 1.0f;
}

// Bounds that shift together, the trip count 0 at every i -- but the bound i
// is converted to unsigned int: at i = -3, k = 1 runs up to 4294967293.
// SHEMPTY: header_types_unsupported.c:[[@LINE+3]]:34: error: the loop bound converts -3 to 'unsigned int', which cannot represent it
void shifted_never_runs(float A[restrict 4]) {
  for (int i = -3; i < 2; ++i)
    for (unsigned k = i + 4; k < i; ++k)
      A[0] = 1.0f;
}

// u - 2 is unsigned arithmetic: at u = 0 and 1 it wraps around to 4294967294
// and 4294967295 (converted to long, no iteration), the exact values -2, -1.
// SHWRAP: header_types_unsupported.c:[[@LINE+3]]:19: error: the loop counter's initial value wraps around in 'unsigned int' (an operation's exact value is -2)
void shifted_wrap(float A[restrict 4][6]) {
  for (unsigned u = 0; u < 4; ++u)
    for (long k = u - 2; k < u + 2; ++k)
      A[u][k + 2] = 1.0f;
}

// i + 2147483647 is int arithmetic: at i = 1 it overflows, although k is long.
// SHOVF: header_types_unsupported.c:[[@LINE+3]]:19: error: the loop counter's initial value overflows 'int' (an operation's exact value is 2147483648)
void shifted_overflow(float A[restrict 4][4]) {
  for (int i = 0; i < 2; ++i)
    for (long k = i + 2147483647; k < i + 2147483649L; ++k)
      A[i][k - 2147483647] = 1.0f;
}

// The same, on bounds that do not shift together (checked at every point of
// the enclosing loops).
// NBINIT: header_types_unsupported.c:[[@LINE+3]]:19: error: the loop counter's initial value wraps around in 'unsigned int' (an operation's exact value is -2)
void nonbox_init_wrap(float A[restrict 4][6]) {
  for (unsigned u = 0; u < 4; ++u)
    for (long k = u - 2; k < 4; ++k)
      A[u][k + 2] = 1.0f;
}

// NBBOUND: header_types_unsupported.c:[[@LINE+3]]:26: error: the loop bound wraps around in 'unsigned int' (an operation's exact value is -2)
void nonbox_bound_wrap(float A[restrict 4][4]) {
  for (unsigned u = 0; u < 4; ++u)
    for (long k = 0; k < u - 2; ++k)
      A[u][0] = 1.0f;
}

// i - 2u converts i to unsigned int (fine) and subtracts in unsigned int.
// NBMIXED: header_types_unsupported.c:[[@LINE+3]]:26: error: the loop bound wraps around in 'unsigned int' (an operation's exact value is -2)
void nonbox_mixed_wrap(float A[restrict 4][4]) {
  for (int i = 0; i < 4; ++i)
    for (long k = 0; k < i - 2u; ++k)
      A[i][0] = 1.0f;
}

// NBIOVF: header_types_unsupported.c:[[@LINE+3]]:19: error: the loop counter's initial value overflows 'int' (an operation's exact value is 2147483648)
void nonbox_init_overflow(float A[restrict 4][4]) {
  for (int i = 0; i < 2; ++i)
    for (long k = i + 2147483647; k < 2147483649L; ++k)
      A[i][k - 2147483647] = 1.0f;
}

// NBBOVF: header_types_unsupported.c:[[@LINE+3]]:36: error: the loop bound overflows 'int' (an operation's exact value is 2147483648)
void nonbox_bound_overflow(float A[restrict 4][4]) {
  for (int i = 0; i < 2; ++i)
    for (long k = 2147483645L; k < i + 2147483647; ++k)
      A[i][k - 2147483645L] = 1.0f;
}

// Repetition loops around a GEMM: s from r - 1 (unsigned) to 1 runs once in
// C (r = 1, s = 0; at r = 0 it starts at 4294967295), not 3 times.
// REP: header_types_unsupported.c:[[@LINE+3]]:19: error: the loop counter's initial value wraps around in 'unsigned int' (an operation's exact value is -1)
void repetitions_wrap(float A[restrict 4][4], float B[restrict 4][4], float C[restrict 4][4]) {
  for (unsigned r = 0; r < 2; ++r)
    for (long s = r - 1; s < 1; ++s)
      for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
          for (int k = 0; k < 4; ++k)
            C[i][j] += A[i][k] * B[k][j];
}

// A subscript computed in unsigned int: at u = 0, u - 1 is 4294967295 -- C
// addresses A[4294967295], not A[-1].
// SUBWRAP: header_types_unsupported.c:[[@LINE+3]]:7: error: the subscript wraps around in 'unsigned int' (an operation's exact value is -1)
void subscript_wrap(float A[restrict 4]) {
  for (unsigned u = 0; u < 4; ++u)
    A[u - 1] = 1.0f;
}

// i + 2147483647 overflows int at i = 1, although the subscript's exact value
// i is in range.
// SUBOVF: header_types_unsupported.c:[[@LINE+3]]:10: error: the subscript overflows 'int' (an operation's exact value is 2147483648)
void subscript_overflow(float A[restrict 4][4]) {
  for (int i = 0; i < 2; ++i)
    A[0][i + 2147483647 - 2147483647] = 1.0f;
}

// None of the functions is translated.
// NONE-NOT: func.func
