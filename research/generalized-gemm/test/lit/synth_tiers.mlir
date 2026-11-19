// Each tier is its own evidence. A scope that runs whole is ScopeEnumerated; a
// larger one is sampled and stays at Point, and the symbolic tier stands apart.
// RUN: %synthreport %s --function=gemm --lower-structured --index-bit-width=8 --max-level=0 --scope=0:1:3 | FileCheck %s --check-prefix=WHOLE
// RUN: %synthreport %s --function=gemm --lower-structured --index-bit-width=8 --max-level=0 --scope=0:1:8 --max-points=2 | FileCheck %s --check-prefix=SAMPLED
// RUN: %synthreport %s --function=triangle --lower-structured --index-bit-width=8 --max-level=0 --scope=0:2:4 | FileCheck %s --check-prefix=TRI
// WHOLE: "authorizes_lowering": false
// WHOLE: "tier": "SCOPE_ENUMERATED"
// WHOLE: "tier": "SYMBOLIC_EXACT"
// WHOLE: "sampled": false
// SAMPLED: "tier": "POINT"
// SAMPLED-NOT: SCOPE_ENUMERATED
// SAMPLED: "sampled": true
// A triangle is exact coverage of a domain that is no box.
// TRI: "box": false
// TRI: "tier": "SYMBOLIC_EXACT"
func.func @gemm(%a: memref<9x2xi32>, %b: memref<2x3xi32>, %c: memref<9x3xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  %c3 = arith.constant 3 : index
  scf.for %i = %c0 to %n step %c1 {
    scf.for %j = %c0 to %c3 step %c1 {
      scf.for %k = %c0 to %c2 step %c1 {
        %x = memref.load %a[%i, %k] : memref<9x2xi32>
        %y = memref.load %b[%k, %j] : memref<2x3xi32>
        %z = memref.load %c[%i, %j] : memref<9x3xi32>
        %m = arith.muli %x, %y : i32
        %s = arith.addi %z, %m : i32
        memref.store %s, %c[%i, %j] : memref<9x3xi32>
      }
    }
  }
  return
}
func.func @triangle(%a: memref<9x2xi32>, %b: memref<2x9xi32>, %c: memref<9x9xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  scf.for %i = %c0 to %n step %c1 {
    %t = arith.addi %i, %c1 : index
    scf.for %j = %c0 to %t step %c1 {
      scf.for %k = %c0 to %c2 step %c1 {
        %x = memref.load %a[%i, %k] : memref<9x2xi32>
        %y = memref.load %b[%k, %j] : memref<2x9xi32>
        %z = memref.load %c[%i, %j] : memref<9x9xi32>
        %m = arith.muli %x, %y : i32
        %s = arith.addi %z, %m : i32
        memref.store %s, %c[%i, %j] : memref<9x9xi32>
      }
    }
  }
  return
}
