// Phase 4 of the bit-vector adaptation (docs/GENERALIZED_GEMM_STATEMENT_BV.md,
// Section 11): the closed forms and the iteration counters through the whole
// recognition pipeline, with three questions kept apart in the record: is the
// decomposition verified, are the views proved and affine, is a lowering eligible
// under Definition 4.5 (V). Nothing here authorizes a lowering.
//
// stepped (I1, I3): with --iteration-counters the inner counter takes the place of i
// as K; with --closed-form-views the proved form i = 5m + 2(m mod 2), read over the
// counter, becomes an identity the proposer uses for the guard and the views and a
// certificate invariant the certificate checker accepts. Theorem 13 is verified over
// the scope; the views A and B are proved and not affine, so nothing is eligible.
// Without the views the same family stops at the proposer.
// RUN: %domreport %s --function=stepped --lower-structured --index-bit-width=8 --scope=0:8:40 --max-level=0 --iteration-counters --closed-form-views --candidates-per-group=1 --rlimit=4000000 --timeout-ms=5000 --authorize="roots 0 and 1 are separate,roots 0 and 2 are separate,roots 1 and 2 are separate" | FileCheck %s --check-prefix=STEP
// RUN: %domreport %s --function=stepped --lower-structured --index-bit-width=8 --scope=0:8:40 --max-level=0 --iteration-counters --candidates-per-group=1 --rlimit=4000000 --timeout-ms=5000 --authorize="roots 0 and 1 are separate,roots 0 and 2 are separate,roots 1 and 2 are separate" | FileCheck %s --check-prefix=NOVIEW
// STEP: "certificate": {
// STEP-NEXT: "invariants_added": [
// STEP-NEXT: "s1 = (ite:bv8 (icmp.eq:bv1 (urem:bv8 (state:bv8 3) (const:bv8 0x2)) (const:bv8 0x0)) (mul:bv8 (const:bv8 0x5) (state:bv8 3)) (add:bv8 (const:bv8 0x2) (mul:bv8 (const:bv8 0x5) (state:bv8 3)))) at cut 3",
// STEP-NEXT: "(const:bv8 0x0) <= s3 <= (const:bv8 0x8) at cut 3"
// STEP: "note": "the certificate checker accepts the strengthened invariants
// STEP-NEXT: "strengthened": true
// STEP: "coordinate_finite": "SCOPE_ENUMERATED"
// STEP-NEXT: "coordinate_symbolic": "PROPOSED"
// STEP: "domain_symbolic": {
// STEP: "exact_order": true
// STEP: "reason": "D1 to D3 discharged over the scope"
// STEP: "recognition": {
// STEP-NEXT: "authorizes_lowering": false,
// STEP-NEXT: "decomposition": "VERIFIED at SYMBOLIC",
// STEP-NEXT: "lowering_eligible": false,
// STEP-NEXT: "reason": "a view is not affine, which Definition 4.5 (V) requires",
// STEP-NEXT: "views": "PROVEN over the scope",
// STEP-NEXT: "views_affine": false
// STEP: "theorem13": {
// STEP: "status": "VERIFIED",
// STEP-NEXT: "tier": "SYMBOLIC"
// STEP: "views": {
// STEP: "A": {
// STEP-NEXT: "affine": false,
// STEP: "B": {
// STEP-NEXT: "affine": false,
// STEP: "C": {
// STEP-NEXT: "affine": true,
// STEP: "identities": [
// STEP: "origin": "proved closed form of s1 over loop 3, m read as the counter s3
// STEP: "iteration_counters": {
// STEP: "replay_equal": 33,
// STEP-NEXT: "replay_points": 33,
// NOVIEW-NOT: "identities"
// NOVIEW-NOT: "recognition"
// NOVIEW: "proposal_reason": "no guard from the program's conditions is exact at the samples"
// NOVIEW: "theorem13": {
// NOVIEW: "status": "UNKNOWN"
// NOVIEW: "iteration_counters": {
// NOVIEW: "replay_equal": 33,
// NOVIEW-NOT: "recognition"
//
// rotated (I2): the statement family passes the coordinate tier (phase 1) and used
// to stop at the proposer. The entry definition two_n = 2n is an identity for the
// bound t < 2n, the views are checked against the sampled indices of both sites and
// come out as the level variables (I, K), (K, J), (I, J), all affine, the integer
// reorder fact holds, and Theorem 13 is verified over the scope: eligible under (V).
// RUN: %domreport %s --function=rotated --lower-structured --index-bit-width=8 --scope=0:2:3 --max-level=3 --statement-grammar --closed-form-views --candidates-per-group=1 --rlimit=2000000 --timeout-ms=3000 --authorize="roots 0 and 1 are separate,roots 0 and 2 are separate,roots 1 and 2 are separate" | FileCheck %s --check-prefix=ROT
// ROT: "certificate": {
// ROT: "s3 = (mul:bv8 (param:bv8 0) (const:bv8 0x2)) at every cut"
// ROT: "strengthened": true
// ROT: "coordinate_finite": "SCOPE_ENUMERATED"
// ROT: "recognition": {
// ROT-NEXT: "authorizes_lowering": false,
// ROT-NEXT: "decomposition": "VERIFIED at SYMBOLIC",
// ROT-NEXT: "lowering_eligible": true,
// ROT-NEXT: "reason": "decomposition verified over the scope, every view proved and affine, a reorder fact proved; eligibility under Definition 4.5 (V), not an authorization",
// ROT-NEXT: "views": "PROVEN over the scope",
// ROT-NEXT: "views_affine": true
// ROT: "theorem13": {
// ROT: "status": "VERIFIED",
// ROT-NEXT: "tier": "SYMBOLIC"
// ROT-NOT: "authorizes_lowering": true
func.func @stepped(%a: memref<1x64xi32>, %b: memref<64x2xi32>, %c: memref<1x2xi32>, %n: index) {
  %zero = arith.constant 0 : index
  cf.br ^outer(%zero : index)
^outer(%j: index):
  %two = arith.constant 2 : index
  %g = arith.cmpi ult, %j, %two : index
  cf.cond_br %g, ^start, ^exit
^start:
  %z0 = arith.constant 0 : index
  cf.br ^inner(%z0 : index)
^inner(%i: index):
  %h = arith.cmpi ult, %i, %n : index
  cf.cond_br %h, ^body, ^next
^body:
  %c0 = arith.constant 0 : index
  %x = memref.load %a[%c0, %i] : memref<1x64xi32>
  %y = memref.load %b[%i, %j] : memref<64x2xi32>
  %w = memref.load %c[%c0, %j] : memref<1x2xi32>
  %m = arith.muli %x, %y : i32
  %s = arith.addi %w, %m : i32
  memref.store %s, %c[%c0, %j] : memref<1x2xi32>
  %c2 = arith.constant 2 : index
  %c3 = arith.constant 3 : index
  %c7 = arith.constant 7 : index
  %r = arith.remui %i, %c2 : index
  %even = arith.cmpi eq, %r, %c0 : index
  %step = arith.select %even, %c7, %c3 : index
  %ni = arith.addi %i, %step : index
  cf.br ^inner(%ni : index)
^next:
  %one = arith.constant 1 : index
  %nj = arith.addi %j, %one : index
  cf.br ^outer(%nj : index)
^exit:
  return
}
func.func @rotated(%a: memref<9x9xi32>, %b: memref<9x2xi32>, %c: memref<9x2xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  %two_n = arith.muli %n, %c2 : index
  scf.for %i = %c0 to %c2 step %c1 {
    scf.for %t = %c0 to %two_n step %c1 {
      %p = arith.cmpi ult, %t, %n : index
      scf.if %p {
        %x = memref.load %a[%i, %t] : memref<9x9xi32>
        %y = memref.load %b[%t, %c0] : memref<9x2xi32>
        %z = memref.load %c[%i, %c0] : memref<9x2xi32>
        %m = arith.muli %x, %y : i32
        %s = arith.addi %z, %m : i32
        memref.store %s, %c[%i, %c0] : memref<9x2xi32>
      } else {
        %u = arith.addi %t, %c1 : index
        %v = arith.subi %u, %n : index
        %r = arith.remui %v, %n : index
        %x = memref.load %a[%i, %r] : memref<9x9xi32>
        %y = memref.load %b[%r, %c1] : memref<9x2xi32>
        %z = memref.load %c[%i, %c1] : memref<9x2xi32>
        %m = arith.muli %x, %y : i32
        %s = arith.addi %z, %m : i32
        memref.store %s, %c[%i, %c1] : memref<9x2xi32>
      }
    }
  }
  return
}
