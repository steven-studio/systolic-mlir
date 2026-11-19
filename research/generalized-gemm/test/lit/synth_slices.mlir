// Phase 2 of the bit-vector adaptation (docs/GENERALIZED_GEMM_STATEMENT_BV.md): the
// slice of each subscript, the loop specification replayed against the program, and
// closed forms checked by the solver. PROVED is unsat on both obligations at the width
// and scope; anything else is not a proof. parity is Listing E.5 of the thesis: the
// column steps by 7 when even and 3 when odd, and the form is 5m + 2(m mod 2).
// RUN: %synthreport %s --function=parity --lower-structured --index-bit-width=8 --scope=0:2:6 --max-level=0 --slices | FileCheck %s --check-prefix=PARITY
// RUN: %synthreport %s --function=parity --lower-structured --index-bit-width=8 --scope=0:2:6 --max-level=0 | FileCheck %s --check-prefix=PLAIN
// RUN: %synthreport %s --function=factorial --lower-structured --index-bit-width=16 --scope=0:1:5 --max-level=0 --slices | FileCheck %s --check-prefix=FACT
// PLAIN-NOT: closed_forms
// PARITY: "closed_forms": {
// PARITY: "closed_forms": [
// PARITY: "program": "v0 := m ; return v0"
// PARITY-NEXT: "reason": "init and step discharged at width 8 for m < 2^8 - 1 under the scope"
// PARITY-NEXT: "rounds": 1
// PARITY-NEXT: "status": "PROVED"
// PARITY: "program": "if (m mod 2) = 0 then (v0 := (5 * m)) else (v0 := (2 + (5 * m))) ; return v0"
// PARITY-NEXT: "reason": "init and step discharged at width 8 for m < 2^8 - 1 under the scope"
// PARITY-NEXT: "rounds": 1
// PARITY-NEXT: "status": "PROVED"
// PARITY: "replay": {
// PARITY-NEXT: "activations": 5
// PARITY-NEXT: "condition_mismatches": 0
// PARITY-NEXT: "iterations": 20
// PARITY-NEXT: "points": 5
// PARITY-NEXT: "step_mismatches": 0
// PARITY-NEXT: "subscript_matches": 40
// PARITY-NEXT: "subscript_mismatches": 0
// PARITY-NEXT: "undefined": 0
//
// factorial: a = (t + 1)!, Listing G.1 of the thesis. No loop-free form is in the
// grammar; the while program is found from the sampled steps (phase 3) and, in
// phase 4, put to the W obligations of the fixed-counter template: shape, init,
// step and termination all unsat, so PROVED. The counter t's form m is proved with a
// read as a free leaf.
// FACT: "closed_forms": [
// FACT: "kind": "loop-free"
// FACT: "program": "v0 := m ; return v0"
// FACT-NEXT: "reason": "init and step discharged at width 16 for m < 2^16 - 1 under the scope"
// FACT: "status": "PROVED"
// FACT: "kind": "while"
// FACT: "id": "W-shape"
// FACT-NEXT: "result": "matched"
// FACT: "id": "W-init"
// FACT-NEXT: "result": "unsat"
// FACT: "id": "W-step"
// FACT-NEXT: "result": "unsat"
// FACT: "id": "W-term"
// FACT-NEXT: "result": "unsat"
// FACT: "program": "v0 := 0 ; v1 := 1 ; while v0 < m do (v1 := (v1 * (2 + v0)) ; v0 := (v0 + 1)) ; return v1"
// FACT-NEXT: "reason": "shape, init, step and termination discharged at width 16 for m < 2^16 - 1 under the scope; the program is the fixed-counter template{{.*}}equal to all sampled iterations as well"
// FACT: "status": "PROVED"
// FACT-NEXT: "symbolic_rounds": 1
// Listing E.5 of the thesis as a reduction: the column i steps by 7 when even and
// by 3 when odd, from 0, over k < n. Closed form i(m) = 5m + 2(m mod 2) (Prop. 4.3).
func.func @parity(%a: memref<1x64xi32>, %b: memref<64x1xi32>, %c: memref<1x1xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  %c3 = arith.constant 3 : index
  %c7 = arith.constant 7 : index
  %i_final = scf.for %k = %c0 to %n step %c1 iter_args(%i = %c0) -> (index) {
    %x = memref.load %a[%c0, %i] : memref<1x64xi32>
    %y = memref.load %b[%i, %c0] : memref<64x1xi32>
    %z = memref.load %c[%c0, %c0] : memref<1x1xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%c0, %c0] : memref<1x1xi32>
    %r = arith.remui %i, %c2 : index
    %even = arith.cmpi eq, %r, %c0 : index
    %step = arith.select %even, %c7, %c3 : index
    %next = arith.addi %i, %step : index
    scf.yield %next : index
  }
  return
}
// Listing G.1 of the thesis as a reduction: a = (t + 1)! steps the column index
// a mod 8 over t < n. No loop-free form of a is in the grammar; a while program is.
func.func @factorial(%a: memref<1x8xi32>, %b: memref<8x1xi32>, %c: memref<1x1xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  %c8 = arith.constant 8 : index
  %final = scf.for %t = %c0 to %n step %c1 iter_args(%acc = %c1) -> (index) {
    %col = arith.remui %acc, %c8 : index
    %x = memref.load %a[%c0, %col] : memref<1x8xi32>
    %y = memref.load %b[%col, %c0] : memref<8x1xi32>
    %z = memref.load %c[%c0, %c0] : memref<1x1xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%c0, %c0] : memref<1x1xi32>
    %t2 = arith.addi %t, %c2 : index
    %next = arith.muli %acc, %t2 : index
    scf.yield %next : index
  }
  return
}
