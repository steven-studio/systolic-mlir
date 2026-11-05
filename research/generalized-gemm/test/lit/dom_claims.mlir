// RUN: %domreport %s --function=rect --lower-structured --index-bit-width=16 --scope=0:1:2 --max-level=0 --candidates-per-group=1 --rlimit=2000000 --timeout-ms=3000 --authorize="roots 0 and 1 are separate,roots 0 and 2 are separate,roots 1 and 2 are separate" | FileCheck %s --check-prefix=RECT
// RUN: %domreport %s --function=holed --lower-structured --index-bit-width=16 --scope=0:1:2 --max-level=0 --candidates-per-group=1 --rlimit=2000000 --timeout-ms=3000 --authorize="roots 0 and 1 are separate,roots 0 and 2 are separate,roots 1 and 2 are separate" | FileCheck %s --check-prefix=HOLED
// RUN: %domreport %s --function=two --lower-structured --index-bit-width=16 --scope=0:1:2 --max-level=0 --candidates-per-group=1 --rlimit=2000000 --timeout-ms=3000 --authorize="roots 0 and 1 are separate,roots 0 and 2 are separate,roots 1 and 2 are separate" | FileCheck %s --check-prefix=TWO
// RECT: "authorizes_lowering": false
// RECT: "domain_symbolic": {
// RECT: "exact_order": true
// RECT: "status": "VERIFIED"
// RECT: "tier": "SYMBOLIC"
// RECT: "integer": {
// RECT-NEXT: "detail": "wrapping addition with no flag, unobserved, at the computation's tier"
// RECT-NEXT: "status": "PROVEN"
// RECT: "theorem13": {
// RECT: "status": "VERIFIED"
// RECT-NEXT: "tier": "SYMBOLIC"
// RECT: "transformation": "UNKNOWN"
// A domain with holes is exact under its guard, and no Cartesian product.
// HOLED: "domain_symbolic": {
// HOLED: "exact_order": true
// HOLED: "theorem13": {
// HOLED: "status": "VERIFIED"
// HOLED-NEXT: "tier": "SYMBOLIC"
// Two sites in one block are two events a step: no symbolic claim.
// TWO-NOT: "tier": "SYMBOLIC"
func.func @rect(%a: memref<8x8xi32>, %b: memref<8x8xi32>, %c: memref<8x8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c3 = arith.constant 3 : index
  scf.for %i = %c0 to %n step %c1 {
    scf.for %j = %c0 to %c3 step %c1 {
      scf.for %k = %c0 to %c3 step %c1 {
        %x = memref.load %a[%i, %k] : memref<8x8xi32>
        %y = memref.load %b[%k, %j] : memref<8x8xi32>
        %z = memref.load %c[%i, %j] : memref<8x8xi32>
        %m = arith.muli %x, %y : i32
        %s = arith.addi %z, %m : i32
        memref.store %s, %c[%i, %j] : memref<8x8xi32>
      }
    }
  }
  return
}
func.func @holed(%a: memref<8x8xi32>, %b: memref<8x8xi32>, %c: memref<8x8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c3 = arith.constant 3 : index
  %mask = arith.constant 365 : index
  scf.for %i = %c0 to %n step %c1 {
    scf.for %j = %c0 to %c3 step %c1 {
      scf.for %k = %c0 to %c3 step %c1 {
        %sh = arith.muli %j, %c3 : index
        %pos = arith.addi %sh, %k : index
        %bit = arith.shrui %mask, %pos : index
        %one = arith.andi %bit, %c1 : index
        %on = arith.cmpi eq, %one, %c1 : index
        scf.if %on {
          %x = memref.load %a[%i, %k] : memref<8x8xi32>
          %y = memref.load %b[%k, %j] : memref<8x8xi32>
          %z = memref.load %c[%i, %j] : memref<8x8xi32>
          %m = arith.muli %x, %y : i32
          %s = arith.addi %z, %m : i32
          memref.store %s, %c[%i, %j] : memref<8x8xi32>
        }
      }
    }
  }
  return
}
func.func @two(%a: memref<8x8xi32>, %b: memref<8x8xi32>, %c: memref<8x8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c3 = arith.constant 3 : index
  scf.for %i = %c0 to %n step %c1 {
    scf.for %j = %c0 to %c3 step %c1 {
      scf.for %k = %c0 to %c3 step %c1 {
        %x = memref.load %a[%i, %k] : memref<8x8xi32>
        %y = memref.load %b[%k, %j] : memref<8x8xi32>
        %z = memref.load %c[%i, %j] : memref<8x8xi32>
        %m = arith.muli %x, %y : i32
        %s = arith.addi %z, %m : i32
        memref.store %s, %c[%i, %j] : memref<8x8xi32>
        %x2 = memref.load %a[%i, %k] : memref<8x8xi32>
        %y2 = memref.load %b[%k, %j] : memref<8x8xi32>
        %z2 = memref.load %c[%i, %j] : memref<8x8xi32>
        %m2 = arith.muli %x2, %y2 : i32
        %s2 = arith.addi %z2, %m2 : i32
        memref.store %s2, %c[%i, %j] : memref<8x8xi32>
      }
    }
  }
  return
}
