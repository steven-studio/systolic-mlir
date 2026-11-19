// RUN: %domreport %s --function=set_a --lower-structured --index-bit-width=16 --scope=0:1:2 --segments --groups="all" --rlimit=2000000 --timeout-ms=3000 --authorize="roots 0 and 1 are separate,roots 0 and 2 are separate,roots 0 and 3 are separate,roots 1 and 2 are separate,roots 1 and 3 are separate,roots 2 and 3 are separate" | FileCheck %s --check-prefix=A
// RUN: %domreport %s --function=set_b --lower-structured --index-bit-width=16 --scope=0:1:2 --segments --groups="all" --rlimit=2000000 --timeout-ms=3000 --authorize="roots 2 and 3 are separate" | FileCheck %s --check-prefix=APART
// RUN: %domreport %s --function=set_b --lower-structured --index-bit-width=16 --scope=0:1:2 --segments --groups="all" --rlimit=2000000 --timeout-ms=3000 | FileCheck %s --check-prefix=ALIAS
// G5b-M4a. The first case: two groups write the same cells of C. One claim over their
// union, on both routes: the events are disjoint, the cells are not, and a run shows it.
// A: "group_sets": [
// A: "finite": {
// A: "authorizes_lowering": false,
// A: "exact_order": true,
// A: "root_pairs": [],
// A-NEXT: "segment_groups": [
// A-NEXT: 0,
// A-NEXT: 1
// A: "shared_cells": {
// A-NEXT: "detail": "two segments write one cell",
// A: "status": "REFUTED",
// A: "status": "VERIFIED",
// A-NEXT: "tier": "SCOPE_ENUMERATED"
// A: "symbolic": {
// A: "exact_order": true,
// A: "reason": "D1 to D5 discharged over the scope",
// A: "shared_cells": {
// A-NEXT: "detail": "not discharged; a point may show it",
// A-NEXT: "status": "UNKNOWN",
// A: "status": "VERIFIED",
// A-NEXT: "tier": "SYMBOLIC"
// The second case: the second group writes D. Authorized apart, no cell is common.
// APART: "finite": {
// APART: "root_pairs": [
// APART: "assumption": true,
// APART-NEXT: "basis": "authorized",
// APART-NEXT: "first": 2,
// APART-NEXT: "relation": "DISTINCT",
// APART-NEXT: "second": 3
// APART: "shared_cells": {
// APART-NEXT: "detail": "at the listed points",
// APART-NEXT: "status": "PROVEN",
// APART: "status": "VERIFIED",
// APART-NEXT: "tier": "SCOPE_ENUMERATED"
// APART: "symbolic": {
// APART: "shared_cells": {
// APART-NEXT: "detail": "no cell of two segments, over the scope",
// APART-NEXT: "status": "PROVEN",
// APART: "status": "VERIFIED",
// APART-NEXT: "tier": "SYMBOLIC"
// With no name, C and D may alias: the facts stay open and the claim about events stands.
// ALIAS: "finite": {
// ALIAS: "relation": "MAY_ALIAS",
// ALIAS: "shared_cells": {
// ALIAS-NEXT: "detail": "roots 2 and 3 may alias: neither is a fresh allocation, and `roots 2 and 3 are separate` is not authorized",
// ALIAS-NEXT: "status": "UNKNOWN",
// ALIAS: "status": "VERIFIED",
// ALIAS-NEXT: "tier": "SCOPE_ENUMERATED"
// ALIAS: "symbolic": {
// ALIAS: "shared_cells": {
// ALIAS-NEXT: "detail": "roots 2 and 3 may alias: neither is a fresh allocation, and `roots 2 and 3 are separate` is not authorized",
// ALIAS-NEXT: "status": "UNKNOWN",
// ALIAS: "status": "VERIFIED",
// ALIAS-NEXT: "tier": "SYMBOLIC"
func.func @set_a(%a: memref<8xi32>, %b: memref<8xi32>, %c: memref<8xi32>, %d: memref<8xi32>, %n: index) {
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
    %y2 = memref.load %d[%j] : memref<8xi32>
    %z2 = memref.load %c[%j] : memref<8xi32>
    %m2 = arith.muli %x2, %y2 : i32
    %s2 = arith.addi %z2, %m2 : i32
    memref.store %s2, %c[%j] : memref<8xi32>
  }
  return
}
func.func @set_b(%a: memref<8xi32>, %b: memref<8xi32>, %c: memref<8xi32>, %d: memref<8xi32>, %n: index) {
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
    %z2 = memref.load %d[%j] : memref<8xi32>
    %m2 = arith.muli %x2, %y2 : i32
    %s2 = arith.addi %z2, %m2 : i32
    memref.store %s2, %d[%j] : memref<8xi32>
  }
  return
}
