// REQUIRES: cfront-clang
//
// What stays outside the subset for loops whose bounds read the enclosing
// counters (docs/AFFINE_GEMM_MAPPING.md section 19.13): a counter coefficient
// other than 1 or -1 in a condition that also reads enclosing counters (the
// equivalent bound would need a floor division), a loop with such bounds that
// counts down, a counter that could overflow at some outer point, a
// condition that overflows where C evaluates it, and a non-parallelotope
// nest too large to run for its exact subscript ranges. The limit counts the
// values given to the counters in one run of the loops in scope (every level
// of the nest), not the innermost points: it bounds the work.
//
// RUN: not %cfront --function=coefficient %s -o %t.1 2>&1 | FileCheck %s --check-prefix=COEF
// RUN: not %cfront --function=counting_down %s -o %t.2 2>&1 | FileCheck %s --check-prefix=DOWN
// RUN: not %cfront --function=overflow_somewhere %s -o %t.3 2>&1 | FileCheck %s --check-prefix=OVERFLOW
// RUN: not %cfront --function=condition_overflow %s -o %t.4 2>&1 | FileCheck %s --check-prefix=COND
// RUN: not %cfront --function=too_large %s -o %t.5 2>&1 | FileCheck %s --check-prefix=LARGE
// RUN: not %cfront --function=few_points %s -o %t.7 2>&1 | FileCheck %s --check-prefix=VALUES
// RUN: %cfront --skip-unsupported --function=too_large --function=fine %s -o %t.6 2>&1 | FileCheck %s --check-prefix=SKIP
// RUN: FileCheck %s --check-prefix=FINE --input-file=%t.6
// RUN: not test -e %t.1

// COEF: nonbox_bounds_unsupported.c:[[@LINE+3]]:21: error: the loop condition must compare the counter, with coefficient 1 or -1, with an affine expression of the enclosing loop counters
void coefficient(float A[restrict 4][8]) {
  for (int i = 0; i < 4; ++i)
    for (int k = 0; 2 * k < i + 8; ++k)
      A[i][k] = 1.0f;
}

// DOWN: nonbox_bounds_unsupported.c:[[@LINE+3]]:5: error: a loop whose bounds depend on an enclosing loop counter must count up
void counting_down(float A[restrict 4][8]) {
  for (int i = 0; i < 4; ++i)
    for (int k = i + 3; k >= i; --k)
      A[i][k] = 1.0f;
}

// At i = 0..3 the loop runs k from 2147483600 + i to 2147483647, whose
// increment leaves int.
// OVERFLOW: nonbox_bounds_unsupported.c:[[@LINE+3]]:5: error: the loop counter could overflow 'int'
void overflow_somewhere(float A[restrict 4][8]) {
  for (int i = 0; i < 4; ++i)
    for (int k = 2147483600 + i; k <= 2147483647; ++k)
      A[i][0] = 1.0f;
}

// k + i * 3 is evaluated at k = 2147483640, i = 3: 2147483649 does not fit int
// (the canonical bound k < 2147483647 - 3 i would not overflow, C's sides do).
// COND: nonbox_bounds_unsupported.c:[[@LINE+3]]:30: error: the loop condition could overflow (it is evaluated for the counter values 2147483640 .. 2147483640)
void condition_overflow(float A[restrict 4][8]) {
  for (int i = 0; i < 4; ++i)
    for (int k = 2147483640; k + i * 3 < 2147483647; ++k)
      A[i][0] = 1.0f;
}

// 256 x 256 x (256 - i): about 8.4 million points, so more than 4194304
// counter values in the run for the access.
// LARGE: nonbox_bounds_unsupported.c:[[@LINE+6]]:9: error: the iteration domain is not a parallelotope and running its loops takes more than 4194304 counter values: its exact subscript ranges are not computed
// SKIP: note: 'too_large' is outside the supported C subset and is not translated: the iteration domain is not a parallelotope and running its loops takes more than 4194304 counter values
void too_large(float A[restrict 256][256]) {
  for (int i = 0; i < 256; ++i)
    for (int j = 0; j < 256; ++j)
      for (int k = i; k < 256; ++k)
        A[k][j] = 1.0f;
}

// Only 2048 points (k runs at i = 0 alone), but checking k's bounds at every
// outer point runs i and j: 2048 + 2048 * 2048 values, more than 4194304.
// VALUES: nonbox_bounds_unsupported.c:[[@LINE+4]]:7: error: the iteration domain is not a parallelotope and running its loops takes more than 4194304 counter values
void few_points(float A[restrict 2048][2048]) {
  for (int i = 0; i < 2048; ++i)
    for (int j = 0; j < 2048; ++j)
      for (int k = i; k < 1; ++k)
        A[j][k] = 1.0f;
}

void fine(float A[restrict 4][4]) {
  for (int i = 0; i < 4; ++i)
    for (int k = i; k < 4; ++k)
      A[i][k] = 1.0f;
}
// FINE-NOT:    func.func @too_large
// FINE:        func.func @fine(
// FINE:        affine.for %k = affine_map<(d0) -> (0 + d0 * 1)>(%i) to 4 {
