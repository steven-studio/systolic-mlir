// RUN: %report %s --index-bit-width=64 --lower-structured | FileCheck %s
// An affine GEMM reaches the model through the upstream lowering of a clone.
// CHECK: "function": "gemm"
// CHECK: "status": "MODELLED"
// CHECK: "structured_lowering": true
// CHECK: "assumptions": [
// CHECK-NEXT: "roots 0 and 1 are separate"
// CHECK: "domain": "UNKNOWN"
// CHECK: "control_oblivious": true
// CHECK: "address_oblivious": true
// CHECK: "form": "direct"
// CHECK-NEXT: "reduction": "fadd"
func.func @gemm(%A: memref<?x?xf32>, %B: memref<?x?xf32>, %C: memref<?x?xf32>,
                %m: index, %n: index, %p: index) {
  affine.for %i = 0 to %m {
    affine.for %j = 0 to %n {
      affine.for %k = 0 to %p {
        %a = affine.load %A[%i, %k] : memref<?x?xf32>
        %b = affine.load %B[%k, %j] : memref<?x?xf32>
        %c = affine.load %C[%i, %j] : memref<?x?xf32>
        %d = arith.mulf %a, %b : f32
        %e = arith.addf %c, %d : f32
        affine.store %e, %C[%i, %j] : memref<?x?xf32>
      }
    }
  }
  return
}
