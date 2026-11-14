// Phase 5 of the bit-vector adaptation (docs/GENERALIZED_GEMM_STATEMENT_BV.md,
// Section 12): a statement program with an array, synthesized (not written by hand)
// for a subscript held as a two-term recurrence, and proved by the TT obligations of
// the history table template at width 8 for m < 2^8 - 2. The trailing variable g has
// a table that fits every sample but no carried variable trails it, so it stays
// SAMPLE_CONSISTENT with that reason. The counter k is the loop-free m.
// RUN: %synthreport %s --function=history --lower-structured --index-bit-width=8 --scope=0:2:9 --max-level=0 --slices | FileCheck %s --check-prefix=TABLE
// TABLE: "closed_forms": [
// TABLE: "kind": "loop-free"
// TABLE: "program": "v0 := m ; return v0"
// TABLE: "status": "PROVED"
// TABLE: "history": [
// TABLE-NEXT: 6
// TABLE-NEXT: ],
// TABLE: "kind": "table"
// TABLE: "id": "TT-shape"
// TABLE-NEXT: "result": "matched; history chain s5 s6"
// TABLE: "id": "TT-init0"
// TABLE-NEXT: "result": "unsat"
// TABLE: "id": "TT-init1"
// TABLE-NEXT: "result": "unsat"
// TABLE: "id": "TT-step"
// TABLE-NEXT: "result": "unsat"
// TABLE: "id": "TT-shift1"
// TABLE-NEXT: "result": "unsat"
// TABLE: "id": "TT-bounds"
// TABLE-NEXT: "result": "unsat"
// TABLE: "id": "TT-term"
// TABLE-NEXT: "result": "unsat"
// TABLE: "program": "array T0[(m + 2)] ; T0[0] := 0 ; T0[1] := 1 ; v0 := 2 ; while v0 < (m + 1) do (T0[v0] := (1 + (T0[(v0 - 1)] + T0[(v0 - 2)])) ; v0 := (v0 + 1)) ; return T0[m]"
// TABLE-NEXT: "reason": "shape, init, step, shift, bounds and termination discharged at width 8 for m < 2^8 - 1 under the scope; the program is the history table template of G.5{{.*}}for m < 2^8 - 2, where the extent m + d does not wrap; equal to all sampled iterations as well"
// TABLE: "status": "PROVED"
// TABLE: "kind": "table"
// TABLE: "program": "array T0[(m + 2)] ; T0[0] := 0 ; T0[1] := 0 ; v0 := 2 ; while v0 < (m + 1) do (T0[v0] := (1 + (T0[(v0 - 1)] + T0[(v0 - 2)])) ; v0 := (v0 + 1)) ; return T0[m]"
// TABLE-NEXT: "reason": "equal to all 104 sampled iterations of 16 activations at the finite tier; not a proof; symbolic: not of a supported template: not matched: no carried variable trails the chain at depth 1 (the history is not in the loop's state)"
// TABLE: "status": "SAMPLE_CONSISTENT"
func.func @history(%a: memref<1x64xi32>, %b: memref<64x2xi32>, %c: memref<1x2xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  scf.for %j = %c0 to %c2 step %c1 {
    %r:2 = scf.for %k = %c0 to %n step %c1 iter_args(%f = %c0, %g = %c0) -> (index, index) {
      %x = memref.load %a[%c0, %f] : memref<1x64xi32>
      %y = memref.load %b[%f, %j] : memref<64x2xi32>
      %z = memref.load %c[%c0, %j] : memref<1x2xi32>
      %m = arith.muli %x, %y : i32
      %s = arith.addi %z, %m : i32
      memref.store %s, %c[%c0, %j] : memref<1x2xi32>
      %fg = arith.addi %f, %g : index
      %nf = arith.addi %fg, %c1 : index
      scf.yield %nf, %f : index, index
    }
  }
  return
}
