// RUN: %compreport %s --function=sib_roots --lower-structured --index-bit-width=8 --scope=0:1:2 --no-search --rlimit=400000 --timeout-ms=2000 --authorize="roots 0 and 1 are separate,roots 0 and 2 are separate,roots 0 and 3 are separate,roots 0 and 4 are separate,roots 1 and 2 are separate,roots 1 and 3 are separate,roots 1 and 4 are separate,roots 2 and 3 are separate,roots 2 and 4 are separate,roots 3 and 4 are separate" --groups="all;all-reversed;0,0" | FileCheck %s --check-prefix=SETS
// RUN: %compreport %s --function=sib_roots --lower-structured --index-bit-width=8 --scope=0:1:2 --no-search --rlimit=400000 --timeout-ms=2000 --authorize="roots 0 and 1 are separate,roots 0 and 2 are separate,roots 0 and 3 are separate,roots 0 and 4 are separate,roots 1 and 2 are separate,roots 1 and 3 are separate,roots 1 and 4 are separate,roots 2 and 3 are separate,roots 2 and 4 are separate,roots 3 and 4 are separate" | FileCheck %s --check-prefix=NONE
// RUN: not %compreport %s --function=sib_roots --groups="0,x" 2>&1 | FileCheck %s --check-prefix=BAD
// G5b-M3. Two sibling loops into the same cells, from different operand roots. Each
// group alone is refuted, since the other's stores are foreign to it; the two together
// are verified on both routes. The cells are shared, which is a fact; the listing
// backwards refutes the order fact and changes nothing else.
// SETS: "group_sets": [
// SETS: "authorizes_lowering": false
// SETS: "finite": {
// SETS: "group_order": {
// SETS-NEXT: "detail": "the listed order at each replayed point",
// SETS-NEXT: "status": "PROVEN",
// SETS-NEXT: "tier": "SCOPE_ENUMERATED"
// SETS: "shared_cells": {
// SETS-NEXT: "detail": "two listed groups write one cell",
// SETS-NEXT: "point": [
// SETS: "status": "REFUTED",
// SETS: "status": "VERIFIED",
// SETS-NEXT: "tier": "SCOPE_ENUMERATED",
// SETS: "groups": [
// SETS-NEXT: 0,
// SETS-NEXT: 1
// SETS: "symbolic": {
// SETS: "reason": "E, OP, R, W and F discharged over the scope",
// SETS: "status": "VERIFIED",
// SETS-NEXT: "tier": "SYMBOLIC",
// The listing backwards.
// SETS: "group_order": {
// SETS-NEXT: "detail": "an event of a group follows one of a later-listed group",
// SETS: "status": "REFUTED",
// SETS: "status": "VERIFIED",
// SETS-NEXT: "tier": "SCOPE_ENUMERATED",
// SETS: "groups": [
// SETS-NEXT: 1,
// SETS-NEXT: 0
// SETS: "reason": "E, OP, R, W and F discharged over the scope",
// SETS: "status": "VERIFIED",
// A group twice is no set.
// SETS: "reason": "a group is listed twice",
// SETS-NEXT: "separations": [],
// SETS: "status": "UNKNOWN",
// Then each group alone.
// SETS: "groups": [
// SETS: "status": "REFUTED",
// SETS: "status": "UNKNOWN",
// SETS: "status": "REFUTED",
// SETS: "status": "UNKNOWN",
// Without --groups the output has no sets.
// NONE-NOT: "group_sets"
// NONE: "groups": [
// BAD: malformed --groups
func.func @sib_roots(%a: memref<4xi32>, %b: memref<4xi32>, %c: memref<4xi32>, %d: memref<4xi32>, %e: memref<4xi32>, %n: index) {
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
  scf.for %j = %c0 to %n step %c1 {
    %x2 = memref.load %d[%j] : memref<4xi32>
    %y2 = memref.load %e[%j] : memref<4xi32>
    %z2 = memref.load %c[%j] : memref<4xi32>
    %m2 = arith.muli %x2, %y2 : i32
    %s2 = arith.addi %z2, %m2 : i32
    memref.store %s2, %c[%j] : memref<4xi32>
  }
  return
}
