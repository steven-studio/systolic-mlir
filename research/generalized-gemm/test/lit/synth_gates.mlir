// RUN: %synthreport %s --function=gemm --lower-structured --index-bit-width=8 --no-certificate --scope=0:1:1 | FileCheck %s --check-prefix=NOPROOF
// RUN: %synthreport %s --function=accumulator --lower-structured --index-bit-width=8 | FileCheck %s --check-prefix=SCALAR
// RUN: %synthreport %s --function=wrong_read --lower-structured --index-bit-width=8 | FileCheck %s --check-prefix=C0
// RUN: not %synthreport %s --function=gemm --lower-structured --index-bit-width=8 --scope=0:-999:1 2>&1 | FileCheck %s --check-prefix=BAD
// NOPROOF: "gate": "UNKNOWN"
// NOPROOF: "groups": []
// NOPROOF: "reason": "no termination certificate"
// SCALAR: "gate": "UNSUPPORTED"
// SCALAR: "reason": "a scalar accumulator has no exact output view"
// C0: "reason": "output_read_differs: no += decomposition exists"
// C0: "status": "CANDIDATE_REFUTED"
// BAD: malformed --scope
func.func @gemm(%a: memref<2xi32>, %b: memref<2xi32>, %c: memref<2xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  scf.for %i = %c0 to %n step %c1 {
    %x = memref.load %a[%i] : memref<2xi32>
    %y = memref.load %b[%i] : memref<2xi32>
    %z = memref.load %c[%i] : memref<2xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<2xi32>
  }
  return
}
func.func @accumulator(%a: memref<4xi32>, %b: memref<4xi32>, %c: memref<1xi32>) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c4 = arith.constant 4 : index
  %zero = arith.constant 0 : i32
  %r = scf.for %k = %c0 to %c4 step %c1 iter_args(%s = %zero) -> (i32) {
    %x = memref.load %a[%k] : memref<4xi32>
    %y = memref.load %b[%k] : memref<4xi32>
    %m = arith.muli %x, %y : i32
    %t = arith.addi %s, %m : i32
    scf.yield %t : i32
  }
  memref.store %r, %c[%c0] : memref<1xi32>
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
