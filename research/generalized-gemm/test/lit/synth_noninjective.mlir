// The cases canonical recovery cannot split. A view ignoring an axis is not
// genuine, and the decomposition is full and proven all the same.
// RUN: %synthreport %s --function=no_i --lower-structured --index-bit-width=8 --max-level=0 --scope=0:2:2 | FileCheck %s
// RUN: %synthreport %s --function=hidden --lower-structured --index-bit-width=8 --max-level=0 --scope=0:2:2 | FileCheck %s
// RUN: %coordreport %s --function=no_i --lower-structured --index-bit-width=8 --scope=0:2:2 | FileCheck %s --check-prefix=G3A
// CHECK: "genuine": false
// CHECK: "structurally_full": true
// CHECK: "equivalence": "UNKNOWN"
// CHECK: "tier": "SYMBOLIC_EXACT"
// G3A: "status": "UNKNOWN"
// G3A-NOT: structurally_full
func.func @no_i(%a: memref<2xi32>, %b: memref<2x3xi32>, %c: memref<2x3xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  %c3 = arith.constant 3 : index
  scf.for %i = %c0 to %n step %c1 {
    scf.for %j = %c0 to %c3 step %c1 {
      scf.for %k = %c0 to %c2 step %c1 {
        %x = memref.load %a[%k] : memref<2xi32>
        %y = memref.load %b[%k, %j] : memref<2x3xi32>
        %z = memref.load %c[%i, %j] : memref<2x3xi32>
        %m = arith.muli %x, %y : i32
        %s = arith.addi %z, %m : i32
        memref.store %s, %c[%i, %j] : memref<2x3xi32>
      }
    }
  }
  return
}
func.func @hidden(%a: memref<2xi32>, %b: memref<3xi32>, %c: memref<2x3xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  %c3 = arith.constant 3 : index
  scf.for %i = %c0 to %n step %c1 {
    scf.for %j = %c0 to %c3 step %c1 {
      scf.for %k = %c0 to %c2 step %c1 {
        %x = memref.load %a[%i] : memref<2xi32>
        %y = memref.load %b[%j] : memref<3xi32>
        %z = memref.load %c[%i, %j] : memref<2x3xi32>
        %m = arith.muli %x, %y : i32
        %s = arith.addi %z, %m : i32
        memref.store %s, %c[%i, %j] : memref<2x3xi32>
      }
    }
  }
  return
}
