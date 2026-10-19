// REQUIRES: cfront-clang
//
// The node-by-node checks of header_types_unsupported.c where C evaluates
// nothing, in bit-precise integer types, and on very deep expressions
// (docs/AFFINE_GEMM_MAPPING.md section 19.13.1, review of 2026-10-19):
//
// - code that never runs: a node that reads a counter -- the loop's own one
//   included, and also when its counter terms cancel -- is not checked there,
//   since C evaluates none; a node that reads none is checked as written,
//   wherever it is (in a nest with a loop whose bounds do not shift together
//   too), as the other checks of a constant are;
// - a `wb` / `uwb` literal has the type _BitInt(N): it fits it, and arithmetic
//   in such a type is checked in its range like any other;
// - the interface states an array's extent as the last row reached plus one,
//   which must fit in 64 bits; an access that never runs reaches no row;
// - an expression of thousands of terms (generated), a side of a loop
//   condition too, is translated, not a stack overflow.
//
// RUN: %cfront --function=dead_reads_counter %s -o - 2>/dev/null | FileCheck %s --check-prefix=DREAD
// RUN: %cfront --function=dead_own_counter %s -o - 2>/dev/null | FileCheck %s --check-prefix=DOWN
// RUN: %cfront --function=dead_subscript_cancels %s -o - 2>/dev/null | FileCheck %s --check-prefix=DSUB
// RUN: %cfront --function=gemm_and_dead_tail %s -o - 2>/dev/null | FileCheck %s --check-prefix=TAIL
// RUN: not %cfront --function=dead_constant_bound %s -o %t.1 2>&1 | FileCheck %s --check-prefix=DCONST
// RUN: not %cfront --function=dead_constant_nonbox %s -o %t.2 2>&1 | FileCheck %s --check-prefix=DNONBOX
// RUN: not %cfront --function=live_reads_counter %s -o %t.3 2>&1 | FileCheck %s --check-prefix=LREAD
// RUN: not %cfront --function=condition_constant %s -o %t.4 2>&1 | FileCheck %s --check-prefix=CCONST
// RUN: %cfront --function=bitint_literals %s -o - 2>/dev/null | FileCheck %s --check-prefix=BITINT
// RUN: not %cfront --function=bitint_wraps %s -o %t.5 2>&1 | FileCheck %s --check-prefix=BITWRAP
// RUN: not %cfront --function=extent_past_64_bits %s -o %t.6 2>&1 | FileCheck %s --check-prefix=EXTENT
// RUN: %cfront --function=gemm_and_unreached_tail --interface %t.json %s -o %t.7
// RUN: FileCheck %s --check-prefix=IFACE --input-file=%t.json
// RUN: %python -c "print('void deep(float A[restrict 8][8]) {\n  for (int i = 0' + ' + 0' * 4000 + '; i < 4' + ' + 0' * 4000 + '; ++i)\n    for (int k = i' + ' + 0' * 4000 + '; k < i + 4; ++k)\n      A[i][k - i' + ' + 0' * 4000 + '] = 1.0f;\n}\nvoid deep_cond(float A[restrict 8][8]) {\n  for (int i = 0; 4 > i' + ' + 0' * 4000 + '; ++i)\n    for (int k = i; k - i' + ' + 0' * 4000 + ' < 4; ++k)\n      A[i][k - i] = 1.0f;\n}')" > %t.deep.c
// RUN: %cfront %t.deep.c -o - 2>/dev/null | FileCheck %s --check-prefix=DEEP

// An outer loop that never runs: C evaluates neither the inner condition
// (k - i converted to unsigned int would be -1 at k = i - 1) nor the access.
// DREAD-LABEL: func.func @dead_reads_counter
// DREAD:         affine.for %[[I:.*]] = 0 to 0 {
// DREAD-NEXT:      affine.for %{{.*}} = affine_map<(d0) -> (-1 + d0 * 1)>(%[[I]]) to affine_map<(d0) -> (4 + d0 * 1)>(%[[I]]) {
void dead_reads_counter(float A[restrict 8][8]) {
  for (int i = 0; i < 0; ++i)
    for (int k = i - 1; k - i < 4u; ++k)
      A[k + 1][0] = 1.0f;
}

// The condition converts the inner loop's own counter, -5 at first; the loop
// is never reached.
// DOWN-LABEL: func.func @dead_own_counter
// DOWN:         affine.for %{{.*}} = 0 to 0 {
// DOWN-NEXT:      affine.for %{{.*}} = -5 to 8 {
void dead_own_counter(float A[restrict 4]) {
  for (int i = 0; i < 0; ++i)
    for (int k = -5; k < 8u; ++k)
      A[0] += 1.0f;
}

// i - i - 1u reads i (its terms cancel): -1 in unsigned int where C would
// evaluate it, which it never does.
// DSUB-LABEL: func.func @dead_subscript_cancels
// DSUB:         affine.for %[[I:.*]] = 0 to 0 {
// DSUB:           affine.store %{{.*}}, %{{.*}}[%[[I]], ((%[[I]] - %[[I]]) - 1)]
void dead_subscript_cancels(float A[restrict 8][8]) {
  for (int i = 0; i < 0; ++i)
    A[i][i - i - 1u] = 1.0f;
}

// A GEMM and a tail loop that never runs: the function is translated.
// TAIL-LABEL: func.func @gemm_and_dead_tail
// TAIL-COUNT-3: affine.for %{{.*}} = 0 to 16 {
// TAIL:         affine.for %{{.*}} = 0 to 0 {
void gemm_and_dead_tail(float A[restrict 16][16], float B[restrict 16][16],
                        float C[restrict 16][16]) {
  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 16; ++j)
      for (int k = 0; k < 16; ++k)
        C[i][j] += A[i][k] * B[k][j];
  for (int t = 0; t < 0; ++t)
    for (int k = t - 1; k - t < 4u; ++k)
      C[k + 1][0] = 0.0f;
}

// A constant node is checked as written: 0 - 1 converted to unsigned int
// (a macro N - 1 with N = 0, say), although the loop is never reached.
// DCONST: header_reach_bitint_depth.c:[[@LINE+3]]:30: error: the loop bound converts -1 to 'unsigned int', which cannot represent it
void dead_constant_bound(float A[restrict 8]) {
  for (int i = 0; i < 0; ++i)
    for (unsigned k = 0; k < 0 - 1; ++k)
      A[i] = 1.0f;
}

// The same in a nest whose j loop runs at no point (j from 2i + 8 below 4).
// DNONBOX: header_reach_bitint_depth.c:[[@LINE+4]]:10: error: the subscript overflows 'int' (an operation's exact value is 2147483648)
void dead_constant_nonbox(float A[restrict 8]) {
  for (int i = 0; i < 4; ++i)
    for (int j = 2 * i + 8; j < 4; ++j)
      A[(2147483647 + 1) - 2147483647] = 1.0f;
}

// dead_reads_counter with an outer loop that runs: refused.
// LREAD: header_reach_bitint_depth.c:[[@LINE+3]]:25: error: the loop condition converts -1 to 'unsigned int', which cannot represent it (it is evaluated for the counter values -1 .. 5)
void live_reads_counter(float A[restrict 8][8]) {
  for (int i = 0; i < 2; ++i)
    for (int k = i - 1; k - i < 4u; ++k)
      A[k + 1][0] = 1.0f;
}

// A constant node of a condition is reported at the node, with its value.
// CCONST: header_reach_bitint_depth.c:[[@LINE+2]]:24: error: the loop condition wraps around in 'unsigned int' (an operation's exact value is -1)
void condition_constant(float A[restrict 8]) {
  for (int k = 0; k + (0u - 1u) < 5u; ++k)
    A[k] = 1.0f;
}

// 1uwb is an unsigned _BitInt(1), 4wb a _BitInt(4), 1wb a _BitInt(2):
// converted to int they keep their values.
// BITINT-LABEL: func.func @bitint_literals
// BITINT:         affine.for %[[I:.*]] = 1 to 4 {
// BITINT:           affine.store %{{.*}}, %{{.*}}[%[[I]], 1]
void bitint_literals(float A[restrict 8][8]) {
  for (int i = 1uwb; i < 4wb; ++i)
    A[i][1wb] = 1.0f;
}

// 7uwb + 2uwb is unsigned _BitInt(3) arithmetic: 9 wraps around to 1.
// BITWRAP: header_reach_bitint_depth.c:[[@LINE+2]]:23: error: the loop bound wraps around in 'unsigned _BitInt(3)' (an operation's exact value is 9)
void bitint_wraps(float A[restrict 16]) {
  for (int i = 0; i < 7uwb + 2uwb; ++i)
    A[i] = 1.0f;
}

// A tail that never runs touches no element, so it adds no row to the
// interface's extents (systolic-cc copies that many rows of the caller's
// array): C's is the GEMM's 16 rows, not 23, and D's is 0.
// IFACE:      {"name": "gemm_and_unreached_tail",
// IFACE-SAME: {"name": "C", {{[^}]*}}"leading_extent": 16, "leading_min": 0,
// IFACE-SAME: {"name": "D", {{[^}]*}}"leading_extent": 0, "leading_min": 0,
void gemm_and_unreached_tail(float A[restrict 16][16], float B[restrict 16][16],
                             float C[restrict 16][16], float D[restrict 4][4]) {
  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 16; ++j)
      for (int k = 0; k < 16; ++k)
        C[i][j] += A[i][k] * B[k][j];
  for (int t = 0; t < 0; ++t)
    for (int k = 0; k < 8; ++k)
      C[k + 15][0] = 0.0f;
  for (int t = 0; t < 0; ++t)
    for (int k = 0; k < 8; ++k)
      D[k + 20][0] = 0.0f;
}

// The leading extent would be 2^63, which no 64-bit integer holds.
// EXTENT: header_reach_bitint_depth.c:[[@LINE+3]]:5: error: the access reaches row 9223372036854775807 of 'A', so the array's extent does not fit in 64 bits
void extent_past_64_bits(float A[restrict]) {
  for (int i = 0; i < 1; ++i)
    A[9223372036854775807LL] = 1.0f;
}

// An initial value, a bound, a shifted initial value and a subscript of 4,001
// terms each.
// DEEP-LABEL: func.func @deep
// DEEP:         affine.for %[[I:.*]] = 0 to 4 {
// DEEP-NEXT:      affine.for %{{.*}} = affine_map<(d0) -> (0 + d0 * 1)>(%[[I]]) to affine_map<(d0) -> (4 + d0 * 1)>(%[[I]]) {

// The two sides of loop conditions of 4,001 terms, canonicalized: 4 > i + 0
// + ... is i < 4, and k - i + 0 + ... < 4 is k < i + 4.
// DEEP-LABEL: func.func @deep_cond
// DEEP:         affine.for %[[I2:.*]] = 0 to 4 {
// DEEP-NEXT:      affine.for %{{.*}} = affine_map<(d0) -> (0 + d0 * 1)>(%[[I2]]) to affine_map<(d0) -> (4 + d0 * 1)>(%[[I2]]) {
