// RUN: %domreport %s --function=set_a --lower-structured --index-bit-width=16 --scope=0:1:2 --segments --max-level=1 --candidates-per-group=1 --rlimit=2000000 --timeout-ms=3000 --authorize="roots 0 and 1 are separate,roots 0 and 2 are separate,roots 0 and 3 are separate,roots 1 and 2 are separate,roots 1 and 3 are separate,roots 2 and 3 are separate" --groups="all" | FileCheck %s --check-prefix=A
// RUN: %domreport %s --function=set_reads --lower-structured --index-bit-width=16 --scope=0:1:2 --segments --max-level=1 --candidates-per-group=1 --rlimit=2000000 --timeout-ms=3000 --authorize="roots 0 and 1 are separate,roots 0 and 2 are separate,roots 0 and 3 are separate,roots 1 and 2 are separate,roots 1 and 3 are separate,roots 2 and 3 are separate" --groups="all" | FileCheck %s --check-prefix=READS
// RUN: %domreport %s --function=set_float --lower-structured --index-bit-width=16 --scope=0:1:2 --segments --max-level=1 --candidates-per-group=1 --rlimit=2000000 --timeout-ms=3000 --authorize="roots 0 and 1 are separate,roots 0 and 2 are separate,roots 0 and 3 are separate,roots 1 and 2 are separate,roots 1 and 3 are separate,roots 2 and 3 are separate" --groups="all" | FileCheck %s --check-prefix=FLOAT
// RUN: %domreport %s --function=set_nsw --lower-structured --index-bit-width=16 --scope=0:1:2 --segments --max-level=1 --candidates-per-group=1 --rlimit=2000000 --timeout-ms=3000 --authorize="roots 0 and 1 are separate,roots 0 and 2 are separate,roots 0 and 3 are separate,roots 1 and 2 are separate,roots 1 and 3 are separate,roots 2 and 3 are separate" --groups="all" | FileCheck %s --check-prefix=NSW
// G5b-M4b-1. Theorem 17 over the union of two groups writing the same cells of C: the
// computation over the set (M3) and the domain over the set (M4a) both symbolic, so the
// verdict is symbolic, a modular fold; the trivial fact is refuted at a point (two
// contributions per cell). G5b-M4b-2a: the integer fact is Proven at the verdict's tier
// by Theorem 13's rule over the union (wrapping addition, no flag, no observer, no stop
// on an output root); the float fact stays Unknown.
// A: "reduction": {
// A: "fold": "MODULAR_INTEGER"
// A: "logical_tier": "PROPOSED"
// A: "refuted_by": "NOTHING"
// A: "reorder": {
// A: "integer": {
// A-NEXT: "detail": "wrapping addition with no flag, unobserved, at the computation's tier"
// A-NEXT: "status": "PROVEN"
// A-NEXT: "tier": "SYMBOLIC"
// A: "trivial": {
// A: "status": "REFUTED"
// A: "status": "VERIFIED"
// A-NEXT: "tier": "SYMBOLIC"
// A-NEXT: "transformation": "UNKNOWN"
// The second group reads what the first wrote: M3 refutes the computation over the set
// with a witness, and the verdict says so, whatever the domain proved.
// READS: "reduction": {
// READS: "computation_finite": {
// READS-NEXT: "reason": "the program and the replay differ
// READS-NEXT: "status": "REFUTED"
// READS: "refuted_by": "COMPUTATION"
// READS: "status": "REFUTED"
// READS-NEXT: "tier": "PROPOSED"
// READS: "witness": {
// Doubles: the fold's order is the claim. G5b-M4b-2b-1: the float reorder fact is refuted
// by the fixed fill sequence with a replayed witness (at n = 1: C[0] = -1, contributions
// -0.001 then +0.001: the program gives -0.9999999999999999, the other order -1), printed
// with the fact; the verdict is untouched. Nothing proves a float fold reorderable but trivial.
// FLOAT: "reduction": {
// FLOAT: "fold": "ORDERED_FLOAT"
// FLOAT: "refuted_by": "NOTHING"
// FLOAT: "reorder": {
// FLOAT: "float": {
// FLOAT-NEXT: "detail": "reversing cell {{[0-9]+}} of root 2 changes its final bits"
// FLOAT-NEXT: "memory": [
// FLOAT: "status": "REFUTED"
// FLOAT: "integer": {
// FLOAT-NEXT: "detail": "not an integer fold, or no claim"
// FLOAT: "status": "VERIFIED"
// FLOAT-NEXT: "tier": "SYMBOLIC"
// G5b-M4b-2a. The same two groups with nsw on both adds: the verdict stands (the claim
// is about the runs that complete), the integer fact is never proven, and the fixed fill
// sequence finds a memory at which the program completes and the reversal of one cell
// overflows a flagged addition. The witness is printed with the fact.
// NSW: "reduction": {
// NSW: "fold": "MODULAR_INTEGER"
// NSW: "reorder": {
// NSW: "integer": {
// NSW-NEXT: "detail": "reversing cell {{[0-9]+}} of root 2 overflows a flagged addition"
// NSW-NEXT: "memory": [
// NSW: "status": "REFUTED"
// NSW: "trivial": {
// NSW: "status": "VERIFIED"
// NSW-NEXT: "tier": "SYMBOLIC"
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
func.func @set_reads(%a: memref<8xi32>, %b: memref<8xi32>, %c: memref<8xi32>, %d: memref<8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  scf.for %i = %c0 to %n step %c1 {
    %xi = memref.load %a[%i] : memref<8xi32>
    %yi = memref.load %b[%i] : memref<8xi32>
    %zi = memref.load %c[%i] : memref<8xi32>
    %mi = arith.muli %xi, %yi : i32
    %si = arith.addi %zi, %mi : i32
    memref.store %si, %c[%i] : memref<8xi32>
  }
  scf.for %j = %c0 to %n step %c1 {
    %xj = memref.load %c[%j] : memref<8xi32>
    %yj = memref.load %b[%j] : memref<8xi32>
    %zj = memref.load %d[%j] : memref<8xi32>
    %mj = arith.muli %xj, %yj : i32
    %sj = arith.addi %zj, %mj : i32
    memref.store %sj, %d[%j] : memref<8xi32>
  }
  return
}
func.func @set_float(%a: memref<8xf64>, %b: memref<8xf64>, %c: memref<8xf64>, %d: memref<8xf64>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  scf.for %i = %c0 to %n step %c1 {
    %xi = memref.load %a[%i] : memref<8xf64>
    %yi = memref.load %b[%i] : memref<8xf64>
    %zi = memref.load %c[%i] : memref<8xf64>
    %mi = arith.mulf %xi, %yi : f64
    %si = arith.addf %zi, %mi : f64
    memref.store %si, %c[%i] : memref<8xf64>
  }
  scf.for %j = %c0 to %n step %c1 {
    %xj = memref.load %a[%j] : memref<8xf64>
    %yj = memref.load %d[%j] : memref<8xf64>
    %zj = memref.load %c[%j] : memref<8xf64>
    %mj = arith.mulf %xj, %yj : f64
    %sj = arith.addf %zj, %mj : f64
    memref.store %sj, %c[%j] : memref<8xf64>
  }
  return
}
func.func @set_nsw(%a: memref<8xi32>, %b: memref<8xi32>, %c: memref<8xi32>, %d: memref<8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  scf.for %i = %c0 to %n step %c1 {
    %xi = memref.load %a[%i] : memref<8xi32>
    %yi = memref.load %b[%i] : memref<8xi32>
    %zi = memref.load %c[%i] : memref<8xi32>
    %mi = arith.muli %xi, %yi : i32
    %si = arith.addi %zi, %mi overflow<nsw> : i32
    memref.store %si, %c[%i] : memref<8xi32>
  }
  scf.for %j = %c0 to %n step %c1 {
    %xj = memref.load %a[%j] : memref<8xi32>
    %yj = memref.load %d[%j] : memref<8xi32>
    %zj = memref.load %c[%j] : memref<8xi32>
    %mj = arith.muli %xj, %yj : i32
    %sj = arith.addi %zj, %mj overflow<nsw> : i32
    memref.store %sj, %c[%j] : memref<8xi32>
  }
  return
}
