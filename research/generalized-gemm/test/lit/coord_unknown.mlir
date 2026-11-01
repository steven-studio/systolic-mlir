// A canonical failure is Unknown with hints: a true GEMM is here, with A ignoring
// I. A read of the wrong cell refutes the form on exact events.
// RUN: %coordreport %s --function=no_i --lower-structured --index-bit-width=8 | FileCheck %s --check-prefix=HINT
// RUN: %coordreport %s --function=wrong_read --lower-structured --index-bit-width=8 | FileCheck %s --check-prefix=C0
// HINT: "attempted"
// HINT: "kind": "dependence_h"
// HINT: "hints"
// HINT: "meet_h": false
// HINT-NOT: CANDIDATE_REFUTED
// HINT: "status": "UNKNOWN"
// C0: "kind": "output_read_differs"
// C0: "status": "CANDIDATE_REFUTED"
func.func @no_i(%a: memref<2xi32>, %b: memref<2x2xi32>, %c: memref<2x2xi32>) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  scf.for %i = %c0 to %c2 step %c1 {
    scf.for %j = %c0 to %c2 step %c1 {
      scf.for %k = %c0 to %c2 step %c1 {
        %x = memref.load %a[%k] : memref<2xi32>
        %y = memref.load %b[%k, %j] : memref<2x2xi32>
        %z = memref.load %c[%i, %j] : memref<2x2xi32>
        %m = arith.muli %x, %y : i32
        %s = arith.addi %z, %m : i32
        memref.store %s, %c[%i, %j] : memref<2x2xi32>
      }
    }
  }
  return
}
func.func @wrong_read(%a: memref<2xi32>, %b: memref<2xi32>, %c: memref<2x2xi32>) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  scf.for %i = %c0 to %c2 step %c1 {
    scf.for %j = %c0 to %c2 step %c1 {
      %x = memref.load %a[%i] : memref<2xi32>
      %y = memref.load %b[%j] : memref<2xi32>
      %z = memref.load %c[%j, %i] : memref<2x2xi32>
      %m = arith.muli %x, %y : i32
      %s = arith.addi %z, %m : i32
      memref.store %s, %c[%i, %j] : memref<2x2xi32>
    }
  }
  return
}
