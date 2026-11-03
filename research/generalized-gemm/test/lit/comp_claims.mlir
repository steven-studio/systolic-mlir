// RUN: %compreport %s --function=clean --lower-structured --index-bit-width=8 --scope=0:1:2 --no-search --rlimit=400000 --timeout-ms=2000 --authorize="roots 0 and 2 are separate,roots 1 and 2 are separate" | FileCheck %s --check-prefix=CLEAN
// RUN: %compreport %s --function=lost --lower-structured --index-bit-width=8 --scope=0:1:2 --no-search --rlimit=400000 --timeout-ms=2000 --authorize="roots 0 and 2 are separate,roots 1 and 2 are separate" | FileCheck %s --check-prefix=LOST
// RUN: %compreport %s --function=overwrite --lower-structured --index-bit-width=8 --scope=0:1:2 --no-search --rlimit=400000 --timeout-ms=2000 --authorize="roots 0 and 2 are separate,roots 1 and 2 are separate" | FileCheck %s --check-prefix=OVER
// RUN: %compreport %s --function=identity --lower-structured --index-bit-width=8 --scope=0:1:2 --no-search --rlimit=400000 --timeout-ms=2000 --authorize="roots 0 and 2 are separate,roots 1 and 2 are separate" | FileCheck %s --check-prefix=SAME
// CLEAN: "authorizes_lowering": false
// CLEAN: "domain_and_order": "UNKNOWN"
// CLEAN: "fold": "MODULAR_INTEGER"
// CLEAN: "name": "roots 0 and 2 are separate"
// CLEAN: "status": "VERIFIED"
// CLEAN: "tier": "SCOPE_ENUMERATED"
// CLEAN: "transformation": "UNKNOWN"
// CLEAN: "status": "VERIFIED"
// CLEAN: "tier": "SYMBOLIC"
// LOST: "reason": "the program and the replay differ at a concrete run
// LOST: "status": "REFUTED"
// LOST: "tier": "PROPOSED"
// LOST: "witness"
// LOST: "status": "UNKNOWN"
// OVER: "reason": "the program and the replay differ at a concrete run: a final cell differs
// OVER: "status": "REFUTED"
// A value that is equal and looks different is no refutation.
// SAME: "reason": "MISMATCH: a cell's final value is not the replay's"
// SAME: "status": "UNKNOWN"
// SAME-NOT: "status": "REFUTED"
func.func @clean(%a: memref<4xi32>, %b: memref<4xi32>, %c: memref<4xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  scf.for %i = %c0 to %n step %c1 {
    %x = memref.load %a[%i] : memref<4xi32>
    %y = memref.load %b[%i] : memref<4xi32>
    %z = memref.load %c[%i] : memref<4xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<4xi32>
  }
  return
}
func.func @lost(%a: memref<4xi32>, %b: memref<4xi32>, %c: memref<4xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  scf.for %i = %c0 to %n step %c1 {
    %x = memref.load %a[%i] : memref<4xi32>
    %y = memref.load %b[%i] : memref<4xi32>
    %z = memref.load %c[%i] : memref<4xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<4xi32>
    %t = arith.addi %z, %m : i32
    memref.store %t, %c[%i] : memref<4xi32>
  }
  return
}
func.func @overwrite(%a: memref<4xi32>, %b: memref<4xi32>, %c: memref<4xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %v = arith.constant 0 : i32
  scf.for %i = %c0 to %n step %c1 {
    %x = memref.load %a[%i] : memref<4xi32>
    %y = memref.load %b[%i] : memref<4xi32>
    %z = memref.load %c[%i] : memref<4xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<4xi32>
  }
  memref.store %v, %c[%c0] : memref<4xi32>
  return
}
func.func @identity(%a: memref<4xi32>, %b: memref<4xi32>, %c: memref<4xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %one = arith.constant 1 : i32
  scf.for %i = %c0 to %n step %c1 {
    %x = memref.load %a[%i] : memref<4xi32>
    %y = memref.load %b[%i] : memref<4xi32>
    %z = memref.load %c[%i] : memref<4xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<4xi32>
  }
  %q = memref.load %c[%c0] : memref<4xi32>
  %r = arith.muli %q, %one : i32
  memref.store %r, %c[%c0] : memref<4xi32>
  return
}
