// REQUIRES: cfront-clang
//
// C the front end does not translate. Each function is outside the affine
// subset in one way; translating it must fail with a diagnostic at the
// construct, and produce no MLIR (exit 1), rather than approximate it.
//
// RUN: not %cfront --function=symbolic_bound %s -o %t.1 2>&1 | FileCheck %s --check-prefix=SYMBOLIC
// RUN: not %cfront --function=while_loop %s -o %t.2 2>&1 | FileCheck %s --check-prefix=WHILE
// RUN: not %cfront --function=branch %s -o %t.3 2>&1 | FileCheck %s --check-prefix=IF
// RUN: not %cfront --function=call %s -o %t.4 2>&1 | FileCheck %s --check-prefix=CALL
// RUN: not %cfront --function=accumulator %s -o %t.5 2>&1 | FileCheck %s --check-prefix=LOCAL
// RUN: not %cfront --function=pointer_arith %s -o %t.6 2>&1 | FileCheck %s --check-prefix=PTR
// RUN: not %cfront --function=partial %s -o %t.7 2>&1 | FileCheck %s --check-prefix=PARTIAL
// RUN: not %cfront --function=counter_written %s -o %t.8 2>&1 | FileCheck %s --check-prefix=WRITTEN
// RUN: not %cfront --function=nonaffine %s -o %t.9 2>&1 | FileCheck %s --check-prefix=NONAFFINE
// RUN: not %cfront --function=modulo %s -o %t.10 2>&1 | FileCheck %s --check-prefix=MODULO
// RUN: not %cfront --function=downward %s -o %t.11 2>&1 | FileCheck %s --check-prefix=DOWN
// RUN: not %cfront --function=not_equal %s -o %t.12 2>&1 | FileCheck %s --check-prefix=NE
// RUN: not %cfront --function=int_array %s -o %t.13 2>&1 | FileCheck %s --check-prefix=INT
// RUN: not %cfront --function=returns %s -o %t.14 2>&1 | FileCheck %s --check-prefix=RET
// RUN: not %cfront --function=overflow %s -o %t.15 2>&1 | FileCheck %s --check-prefix=OVERFLOW
// RUN: not %cfront --function=global_array %s -o %t.16 2>&1 | FileCheck %s --check-prefix=GLOBAL
// RUN: not %cfront --function=conditional %s -o %t.17 2>&1 | FileCheck %s --check-prefix=TERNARY
// RUN: not %cfront --function=int_value %s -o %t.18 2>&1 | FileCheck %s --check-prefix=INTVAL
// RUN: not test -e %t.1
//
// A clang of another major version is refused, not used (the AST JSON of
// another clang is another format):
// RUN: not %cfront --clang-major=1 --function=symbolic_bound %s 2>&1 | FileCheck %s --check-prefix=VERSION
// VERSION: refusing to use it

void symbolic_bound(float a[restrict 64], int n) {
  for (int i = 0; i < n; ++i)
    a[i] = 0.0f;
}

void while_loop(float a[restrict 64]) {
  int i = 0;
  while (i < 64)
    a[i++] = 0.0f;
}

void branch(float a[restrict 64]) {
  for (int i = 0; i < 64; ++i)
    if (i > 3)
      a[i] = 0.0f;
}

float f(float);
void call(float a[restrict 64]) {
  for (int i = 0; i < 64; ++i)
    a[i] = f(a[i]);
}

// The common hand-written GEMM: a scalar accumulator.
void accumulator(float A[restrict 4][4], float B[restrict 4][4],
                 float C[restrict 4][4]) {
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j) {
      float acc = 0.0f;
      for (int k = 0; k < 4; ++k)
        acc += A[i][k] * B[k][j];
      C[i][j] = acc;
    }
}

void pointer_arith(float *restrict a) {
  for (int i = 0; i < 64; ++i)
    *(a + i) = 0.0f;
}

void partial(float A[restrict 4][4], float b[restrict 4]) {
  for (int i = 0; i < 4; ++i)
    b[i] = A[i][0] + *A[i];
}

void counter_written(float a[restrict 64]) {
  for (int i = 0; i < 64; ++i) {
    a[i] = 0.0f;
    i += 1;
  }
}

void nonaffine(float a[restrict 64][64]) {
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j)
      a[i * j][0] = 0.0f;
}

void modulo(float a[restrict 64]) {
  for (int i = 0; i < 64; ++i)
    a[i % 8] = 0.0f;
}

void downward(float a[restrict 64]) {
  for (int i = 63; i >= 0; i++)
    a[i] = 0.0f;
}

void not_equal(float a[restrict 64]) {
  for (int i = 0; i != 64; ++i)
    a[i] = 0.0f;
}

void int_array(int a[restrict 64]) {
  for (int i = 0; i < 64; ++i)
    a[i] = 0;
}

float returns(float a[restrict 64]) { return a[0]; }

// The counter would pass INT_MAX on its way out of the loop.
void overflow(float a[restrict 64]) {
  for (int i = 2147483600; i < 2147483647; i += 64)
    a[0] = 0.0f;
}

float G[64];
void global_array(float a[restrict 64]) {
  for (int i = 0; i < 64; ++i)
    a[i] = G[i];
}

void conditional(float a[restrict 64]) {
  for (int i = 0; i < 64; ++i)
    a[i < 32 ? i : 0] = 0.0f;
}

void int_value(float a[restrict 64]) {
  for (int i = 0; i < 64; ++i)
    a[i] = i;
}

// Expected diagnostics (absolute positions in this file):
// SYMBOLIC: unsupported.c:33:23: error: 'n' is not an integer constant (loop bounds and steps must be static)
// WHILE: unsupported.c:38:3: error: local variables are not supported (only for-loop counters)
// IF: unsupported.c:45:5: error: statement 'IfStmt' is not supported
// CALL: unsupported.c:52:12: error: expression 'CallExpr' is not supported
// LOCAL: unsupported.c:60:7: error: local variables are not supported (only for-loop counters)
// PTR: unsupported.c:69:5: error: only array elements can be assigned
// PARTIAL: unsupported.c:74:22: error: only array elements and scalar float parameters can be read
// WRITTEN: unsupported.c:78:32: error: the loop counter is modified in the loop body
// NONAFFINE: unsupported.c:87:9: error: subscript multiplies two non-constant terms; not an affine expression
// MODULO: unsupported.c:92:7: error: subscript is not an affine expression of the loop counters
// DOWN: unsupported.c:96:28: error: the loop step must be negative (the condition '>=' counts down)
// NE: unsupported.c:101:19: error: the loop condition must be 'counter < bound', 'counter <= bound', 'counter > bound' or 'counter >= bound'
// INT: unsupported.c:105:16: error: parameter 'a' of type 'int *restrict' is not a float/double array or scalar
// RET: unsupported.c:110:1: error: only functions returning void are supported (got 'float')
// OVERFLOW: unsupported.c:114:3: error: the loop counter could overflow 'int'
// GLOBAL: unsupported.c:121:12: error: 'G' is not an array parameter
// TERNARY: unsupported.c:126:7: error: subscript is not an affine expression of the loop counters
// INTVAL: unsupported.c:131:12: error: converting an integer variable to floating point is not supported (only integer constants)
