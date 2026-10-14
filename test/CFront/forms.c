// REQUIRES: cfront-clang
//
// The C forms the front end translates, other than the GEMM in gemm.c, and
// that every one of them gives MLIR the LLVM/MLIR 24 tools verify.
//
// RUN: %cfront %s -o %t.mlir
// RUN: FileCheck %s --input-file=%t.mlir
// RUN: systolic-opt %t.mlir -o /dev/null
// RUN: systolic-opt --systolic-declare-fpga-fleet --affine-matmul-to-systolic %t.mlir 2>&1 \
// RUN:   | FileCheck %s --check-prefix=SYS

// Loop forms: <=, i++, i += S, i = i + S, nonzero lower bounds; integer
// constant expressions as bounds.
void loops(float a[restrict 64]) {
  for (int i = 2; i <= 9; i++)
    a[i] = 1.0f;
  for (int i = 1; i < 2 * 8 + 1; i += 3)
    a[i] = 2.0f;
  for (long i = 0; i < 30; i = i + 5)
    a[i] = 3.0f;
}
// CHECK-LABEL: func.func @loops(%a: memref<?xf32> {llvm.noalias}
// CHECK:       affine.for %i = 2 to 10 {
// CHECK-NEXT:    %[[ONE:.*]] = arith.constant 0x3F800000 : f32
// CHECK-NEXT:    affine.store %[[ONE]], %a[%i]
// CHECK:       affine.for %i_1 = 1 to 17 step 3 {
// CHECK:         affine.store %{{.*}}, %a[%i_1]
// CHECK:       affine.for %i_2 = 0 to 30 step 5 {

// Affine subscripts: offsets, constant strides, negation, 3-D arrays; and
// statements in order.
void affine_subscripts(float x[restrict 32][8][4], float y[16][8]) {
  for (int i = 0; i < 4; ++i)
    for (int j = 1; j < 8; ++j) {
      y[2 * i + 3][j - 1] = x[i][7 - j][(i + 1) * 3 - i * 3];
      y[0][j] = -y[0][j];
    }
}
// y is not restrict: no llvm.noalias.
// CHECK-LABEL: func.func @affine_subscripts(%x: memref<?x8x4xf32> {llvm.noalias} {{.*}}, %y: memref<?x8xf32> loc
// CHECK:         %[[X:.*]] = affine.load %x[%i, (7 - %j), (((%i + 1) * 3) - (%i * 3))]
// CHECK-NEXT:    affine.store %[[X]], %y[((2 * %i) + 3), (%j - 1)]
// CHECK-NEXT:    %[[Y:.*]] = affine.load %y[0, %j]
// CHECK-NEXT:    %[[N:.*]] = arith.negf %[[Y]] : f32
// CHECK-NEXT:    affine.store %[[N]], %y[0, %j]

// C's conversions, kept: float += double is computed in double and
// truncated back; a float scalar parameter; float and double literals,
// bit-exact (the hex patterns are binary32 / binary64); integer constants
// converted to floating point; every operator.
void types(float f[restrict 8], double d[restrict 8], float alpha) {
  for (int i = 0; i < 8; ++i) {
    f[i] += d[i];
    f[i] = alpha * f[i] / 3 - 0.1f;
    d[i] -= 0x1.0000000000001p0 * 1e-45f;
    f[i] *= 3.4028235e38f;
  }
}
// CHECK-LABEL: func.func @types(%f: memref<?xf32> {llvm.noalias} {{.*}}, %d: memref<?xf64> {llvm.noalias} {{.*}}, %alpha: f32
// f[i] += d[i]
// CHECK:      %[[F0:.*]] = affine.load %f[%i] : memref<?xf32>
// CHECK-NEXT: %[[F0D:.*]] = arith.extf %[[F0]] : f32 to f64
// CHECK-NEXT: %[[D0:.*]] = affine.load %d[%i] : memref<?xf64>
// CHECK-NEXT: %[[S0:.*]] = arith.addf %[[F0D]], %[[D0]] : f64
// CHECK-NEXT: %[[S0F:.*]] = arith.truncf %[[S0]] : f64 to f32
// CHECK-NEXT: affine.store %[[S0F]], %f[%i]
// f[i] = alpha * f[i] / 3 - 0.1f
// CHECK-NEXT: %[[F1:.*]] = affine.load %f[%i]
// CHECK-NEXT: %[[M:.*]] = arith.mulf %alpha, %[[F1]] : f32
// CHECK-NEXT: %[[THREE:.*]] = arith.constant 0x40400000 : f32
// CHECK-NEXT: %[[Q:.*]] = arith.divf %[[M]], %[[THREE]] : f32
// CHECK-NEXT: %[[TENTH:.*]] = arith.constant 0x3DCCCCCD : f32
// CHECK-NEXT: %[[R:.*]] = arith.subf %[[Q]], %[[TENTH]] : f32
// CHECK-NEXT: affine.store %[[R]], %f[%i]
// d[i] -= 0x1.0000000000001p0 * 1e-45f   (the float literal promoted)
// CHECK-NEXT: %[[D1:.*]] = affine.load %d[%i]
// CHECK-NEXT: %[[BIG:.*]] = arith.constant 0x3FF0000000000001 : f64
// CHECK-NEXT: %[[TINY:.*]] = arith.constant 0x00000001 : f32
// CHECK-NEXT: %[[TINYD:.*]] = arith.extf %[[TINY]] : f32 to f64
// CHECK-NEXT: %[[P:.*]] = arith.mulf %[[BIG]], %[[TINYD]] : f64
// CHECK-NEXT: %[[D2:.*]] = arith.subf %[[D1]], %[[P]] : f64
// CHECK-NEXT: affine.store %[[D2]], %d[%i]
// f[i] *= 3.4028235e38f
// CHECK:      arith.constant 0x7F7FFFFF : f32

// The loops of test/e2e/affine_c/strided_main.c, in C: k, i, j order, the
// logical matrices at offsets and strides inside larger arrays. The
// recognizer maps the loop order back to I, J, K; two tiles on the fleet.
void permuted_strided(float A[restrict 20][4], float B[restrict 4][5],
                      float C[restrict 20][5]) {
  for (int k = 0; k < 4; ++k)
    for (int i = 0; i < 6; ++i)
      for (int j = 0; j < 4; ++j)
        C[3 + 2 * i][j + 1] += A[3 + 2 * i][k] * B[k][j + 1];
}
// SYS-LABEL: func.func @permuted_strided
// SYS:       memref.subview %arg0[3, 0] [6, 4] [2, 1] : memref<?x4xf32>
// SYS:       memref.subview %arg1[0, 1] [4, 4] [1, 1] : memref<?x5xf32>
// SYS:       memref.subview %arg2[3, 1] [6, 4] [2, 1] : memref<?x5xf32>
// SYS:       systolic.matmul_tile {{.*}} m = 4 n = 4 k = 4 on @acc_4x4_0
// SYS:       systolic.matmul_tile {{.*}} m = 2 n = 4 k = 4 on @acc_4x4_1
