// Phase 5 (docs/GENERALIZED_GEMM_STATEMENT_BV.md, Section 12): the synthesized table
// program of synth_tables.mlir inside the whole recognition record. The coordinates
// are the loop variables (J = j, K = k); the domain and Theorem 13 are verified over
// the scope; the column of A (and the row of B) is the slice variable f, which no term
// describes: the record carries the PROVED table program on that dimension, the view
// has no affine form, and lowering is not eligible. The coordinate tier stays at the
// enumerated points (V1f: the symbolic dependence check does not go through).
// RUN: %domreport %s --function=history --lower-structured --index-bit-width=8 --scope=0:2:9 --max-level=0 --closed-form-views --candidates-per-group=1 --rlimit=4000000 --timeout-ms=5000 --authorize="roots 0 and 1 are separate,roots 0 and 2 are separate,roots 1 and 2 are separate" | FileCheck %s --check-prefix=HIST
// HIST: "coordinate_finite": "SCOPE_ENUMERATED"
// HIST: "domain_symbolic": {
// HIST: "reason": "D1 to D3 discharged over the scope"
// HIST: "recognition": {
// HIST-NEXT: "authorizes_lowering": false,
// HIST-NEXT: "decomposition": "VERIFIED at SYMBOLIC",
// HIST-NEXT: "lowering_eligible": false,
// HIST-NEXT: "reason": "a view's dimension is a slice variable described by a statement program with a loop or a table (see views.*.index_programs), which has no affine form; Definition 4.5 (V) is not met",
// HIST-NEXT: "views": "NOT_PROVEN",
// HIST-NEXT: "views_affine": null
// HIST: "theorem13": {
// HIST: "status": "VERIFIED",
// HIST-NEXT: "tier": "SYMBOLIC"
// HIST: "views": {
// HIST: "A": {
// HIST: "index_programs": [
// HIST-NEXT: null,
// HIST-NEXT: {
// HIST-NEXT: "kind": "table",
// HIST-NEXT: "loop": 3,
// HIST-NEXT: "program": "array T0[(m + 2)] ; T0[0] := 0 ; T0[1] := 1 ; v0 := 2 ; while v0 < (m + 1) do (T0[v0] := (1 + (T0[(v0 - 1)] + T0[(v0 - 2)])) ; v0 := (v0 + 1)) ; return T0[m]",
// HIST-NEXT: "reason": "shape, init, step, shift, bounds and termination discharged
// HIST-NEXT: "status": "PROVED"
// HIST: "proposed": false
// HIST: "C": {
// HIST: "affine": true,
// HIST: "identity_notes": "{{.*}}a proved table program of s5 gives no term, so no identity"
// HIST-NOT: "authorizes_lowering": true
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
