// RUN: systolic-opt --test-affine-matmul-discovery %s -o /dev/null 2>&1 \
// RUN:   | FileCheck %s
//
// CHECK: recognition result: failure
// CHECK: unique logical matmul count: 0

// affine_matmul_k.mlir

func.func @matmul_k() {
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 4 {
      affine.for %k = 0 to 4 {
      }
    }
  }
  return
}
