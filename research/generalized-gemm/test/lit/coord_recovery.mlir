// Standard GEMM over three axes of different sizes, and a triangular domain.
// RUN: %coordreport %s --function=gemm --lower-structured --index-bit-width=8 --scope=0:2:2 | FileCheck %s --check-prefix=GEMM
// RUN: %coordreport %s --function=gemm --lower-structured --index-bit-width=8 --scope=0:1:1 | FileCheck %s --check-prefix=GEMV
// RUN: %coordreport %s --function=triangle --lower-structured --index-bit-width=8 --scope=0:3:3 | FileCheck %s --check-prefix=TRI
// GEMM: "authorizes_lowering": false
// GEMM: "gate": "OPEN"
// GEMM: "equivalence": "UNKNOWN"
// GEMM: "form": "box"
// GEMM: "form": "for_later_verification"
// GEMM: "genuine": true
// GEMM: "class": "gemm"
// GEMM: "status": "CANDIDATE_FOUND"
// A degenerate shape is valid and carried forward, and is not genuine.
// GEMV: "form": "for_later_verification"
// GEMV: "genuine": false
// GEMV: "class": "gemv"
// GEMV: "decomposition_valid": true
// GEMV: "status": "CANDIDATE_FOUND"
// TRI: "form": "partial"
// TRI: "missing_points": "6"
// TRI: "genuine": false
// TRI: "decomposition_valid": true
func.func @gemm(%a: memref<4x2xi32>, %b: memref<2x3xi32>, %c: memref<4x3xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  %c3 = arith.constant 3 : index
  scf.for %i = %c0 to %n step %c1 {
    scf.for %j = %c0 to %c3 step %c1 {
      scf.for %k = %c0 to %c2 step %c1 {
        %x = memref.load %a[%i, %k] : memref<4x2xi32>
        %y = memref.load %b[%k, %j] : memref<2x3xi32>
        %z = memref.load %c[%i, %j] : memref<4x3xi32>
        %m = arith.muli %x, %y : i32
        %s = arith.addi %z, %m : i32
        memref.store %s, %c[%i, %j] : memref<4x3xi32>
      }
    }
  }
  return
}
func.func @triangle(%a: memref<4x2xi32>, %b: memref<2x3xi32>, %c: memref<4x3xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  scf.for %i = %c0 to %n step %c1 {
    %t = arith.addi %i, %c1 : index
    scf.for %j = %c0 to %t step %c1 {
      scf.for %k = %c0 to %c2 step %c1 {
        %x = memref.load %a[%i, %k] : memref<4x2xi32>
        %y = memref.load %b[%k, %j] : memref<2x3xi32>
        %z = memref.load %c[%i, %j] : memref<4x3xi32>
        %m = arith.muli %x, %y : i32
        %s = arith.addi %z, %m : i32
        memref.store %s, %c[%i, %j] : memref<4x3xi32>
      }
    }
  }
  return
}
