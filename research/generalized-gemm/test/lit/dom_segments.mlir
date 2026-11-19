// RUN: %domreport %s --function=sib_same --lower-structured --index-bit-width=16 --scope=0:1:2 --segments --rlimit=2000000 --timeout-ms=3000 --authorize="roots 0 and 1 are separate,roots 0 and 2 are separate,roots 1 and 2 are separate" | FileCheck %s --check-prefix=SAME
// RUN: %domreport %s --function=sib_same --lower-structured --index-bit-width=16 --scope=0:1:2 --max-level=0 --candidates-per-group=1 --rlimit=2000000 --timeout-ms=3000 --authorize="roots 0 and 1 are separate,roots 0 and 2 are separate,roots 1 and 2 are separate" | FileCheck %s --check-prefix=FLAT
// Two sibling loops into the same cells: two segments, Theorem 15 over the scope, the
// cells shared, Theorem 13 over the segmented key, and nothing authorized.
// SAME: "authorizes_lowering": false
// SAME: "domain_symbolic": {
// SAME: "exact_order": true
// SAME: "reason": "D1 to D5 discharged over the scope"
// SAME: "segments": [
// SAME: "status": "VERIFIED"
// SAME: "tier": "SYMBOLIC"
// SAME: "theorem13": {
// SAME: "logical_tier": "PROPOSED"
// SAME: "status": "VERIFIED"
// SAME-NEXT: "tier": "SYMBOLIC"
// Without segments the search finds no family: Unknown, never a false claim.
// FLAT: "search": "UNKNOWN"
// FLAT-NOT: "tier": "SYMBOLIC"
func.func @sib_same(%a: memref<8xi32>, %b: memref<8xi32>, %c: memref<8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  scf.for %i = %c0 to %n step %c1 {
    %x = memref.load %a[%i] : memref<8xi32>
    %y = memref.load %b[%i] : memref<8xi32>
    %z = memref.load %c[%i] : memref<8xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<8xi32>
  }
  scf.for %j = %c0 to %n step %c1 {
    %x2 = memref.load %a[%j] : memref<8xi32>
    %y2 = memref.load %b[%j] : memref<8xi32>
    %z2 = memref.load %c[%j] : memref<8xi32>
    %m2 = arith.muli %x2, %y2 : i32
    %s2 = arith.addi %z2, %m2 : i32
    memref.store %s2, %c[%j] : memref<8xi32>
  }
  return
}
