// RUN: %domreport %s --function=rect --lower-structured --index-bit-width=16 --scope=0:1:2 --max-level=0 --candidates-per-group=1 --rlimit=2000000 --timeout-ms=3000 | FileCheck %s --check-prefix=NAMES
// RUN: %domreport %s --function=rect --lower-structured --index-bit-width=16 --scope=0:1:2 --max-level=0 --candidates-per-group=1 --rlimit=2000000 --timeout-ms=0 --authorize="roots 0 and 1 are separate,roots 0 and 2 are separate,roots 1 and 2 are separate" | FileCheck %s --check-prefix=ZERO
// Without the separation named, G4a gives nothing, and so no Theorem 13.
// NAMES: "authorizes_lowering": false
// NAMES: "theorem13": {
// NAMES-NOT: "status": "VERIFIED"
// ZERO: "reason": "a solver limit of zero is no limit"
// ZERO-NOT: "tier": "SYMBOLIC"
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
