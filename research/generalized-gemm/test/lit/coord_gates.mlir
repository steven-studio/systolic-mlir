// The gates give no table, and the reason names the gate.
// RUN: %coordreport %s --function=gemm --lower-structured --index-bit-width=8 --scope=0:1:1 --no-certificate | FileCheck %s --check-prefix=NOPROOF
// RUN: %coordreport %s --function=gemm --lower-structured --index-bit-width=8 --max-points=4 | FileCheck %s --check-prefix=LIMIT
// RUN: %coordreport %s --function=gemm --index-bit-width=8 --scope=0:1:1 | FileCheck %s --check-prefix=REGION
// RUN: %coordreport %s --function=accumulator --lower-structured --index-bit-width=8 | FileCheck %s --check-prefix=SCALAR
// RUN: %coordreport %s --function=lookup --lower-structured --index-bit-width=8 | FileCheck %s --check-prefix=DATA
// RUN: %coordreport %s --function=bounds --lower-structured --index-bit-width=8 --scope=0:3:3 | FileCheck %s --check-prefix=STOP
// RUN: not %coordreport %s --function=gemm --lower-structured --index-bit-width=8 --scope=0:-999:1 2>&1 | FileCheck %s --check-prefix=BAD
// NOPROOF: "gate": "UNKNOWN"
// NOPROOF: "points": []
// NOPROOF: "reason": "no termination certificate"
// LIMIT: "gate": "UNKNOWN"
// LIMIT: "reason": "the scope has more points than the limit"
// REGION: "gate": "UNSUPPORTED"
// SCALAR: "gate": "UNSUPPORTED"
// SCALAR: "reason": "a scalar accumulator has no exact output view"
// DATA: "gate": "UNSUPPORTED"
// DATA: "reason": "the trace depends on memory contents"
// A run that stops gives no domain, and a point without a domain is unknown.
// STOP: "gate": "OPEN"
// STOP: "status": "UNKNOWN"
// STOP: "run": "UB"
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
func.func @lookup(%p: memref<4xindex>, %a: memref<4xi32>, %c: memref<4xi32>) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c4 = arith.constant 4 : index
  scf.for %i = %c0 to %c4 step %c1 {
    %t = memref.load %p[%i] : memref<4xindex>
    %x = memref.load %a[%t] : memref<4xi32>
    %y = memref.load %a[%i] : memref<4xi32>
    %z = memref.load %c[%i] : memref<4xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<4xi32>
  }
  return
}
func.func @bounds(%a: memref<2xi32>, %b: memref<2xi32>, %c: memref<2xi32>, %n: index) {
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
