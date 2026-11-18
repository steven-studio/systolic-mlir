// Index invariants for a GEMM's subscripts (docs/GENERALIZED_GEMM_STATEMENT_BV.md, Section 14).
// The K subscript of A and of B is x, carried by scf.for iter_args: x starts at x0 = p2 and
// steps by one, so x == x0 + k at the k header (s5 is k, s6 is x, L5 the k header).
//
// RUN: %synthreport %s --function=gemm_x --lower-structured --index-bit-width=8 --scope=0:1:3,1:1:4,2:0:5 --max-level=0 --max-candidates=1 | FileCheck %s --check-prefix=OFF
// RUN: %synthreport %s --function=gemm_x --lower-structured --index-bit-width=8 --scope=0:1:3,1:1:4,2:0:5 --max-level=0 --max-candidates=1 --index-invariants | FileCheck %s --check-prefix=AUTO
// RUN: %synthreport %s --function=gemm_x --lower-structured --index-bit-width=8 --scope=0:1:3,1:1:4,2:0:5 --max-level=0 --max-candidates=1 '--index-invariant=5:s6 == s5 + p2' | FileCheck %s --check-prefix=GIVEN
// RUN: %synthreport %s --function=gemm_x --lower-structured --index-bit-width=8 --scope=0:1:3,1:1:4,2:0:5 --max-level=0 --max-candidates=1 '--index-invariant=5:s6 == s5 + p2 + 1' | FileCheck %s --check-prefix=BAD
// RUN: %synthreport %s --function=gemm_x --lower-structured --index-bit-width=8 --scope=0:1:3,1:1:4,2:0:5 --max-level=0 --max-candidates=1 --index-invariants '--index-relation=0:A:1:s5 + p2 + 1' | FileCheck %s --check-prefix=REL
// RUN: %synthreport %s --function=carried --lower-structured --index-bit-width=8 --scope=0:1:2,1:1:2,2:0:2 --max-level=0 --max-candidates=1 '--index-invariant=5:s7 == s6 + p2' | FileCheck %s --check-prefix=ENTRY
// RUN: %synthreport %s --function=narrow --lower-structured --index-bit-width=16 --scope=0:1:2,1:1:300,2:0:100 --max-level=0 --max-candidates=1 --index-invariants | FileCheck %s --check-prefix=NARROW
// RUN: %synthreport %s --function=narrow --lower-structured --index-bit-width=16 --scope=0:1:2,1:1:300,2:0:100 --max-level=0 --max-candidates=1 '--index-invariant=5:zext16(s7) == s6 + zext16(p2)' | FileCheck %s --check-prefix=INTEGER
// RUN: %synthreport %s --function=bump --lower-structured --index-bit-width=8 --scope=0:1:3,1:1:3,2:1:3 --max-level=0 --max-candidates=1 --index-invariants --index-invariant-degrees=1,2 | FileCheck %s --check-prefix=BUMP
// RUN: %domreport %s --function=gemm_x --lower-structured --index-bit-width=8 --scope=0:1:3,1:1:4,2:0:5 --max-level=0 --candidates-per-group=1 '--authorize=roots 0 and 1 are separate,roots 0 and 2 are separate,roots 1 and 2 are separate' | FileCheck %s --check-prefix=DOMOFF
// RUN: %domreport %s --function=gemm_x --lower-structured --index-bit-width=8 --scope=0:1:3,1:1:4,2:0:5 --max-level=0 --candidates-per-group=1 '--authorize=roots 0 and 1 are separate,roots 0 and 2 are separate,roots 1 and 2 are separate' --index-invariants | FileCheck %s --check-prefix=DOM

// Without the option the output has no such record, and the coordinate checker stops at
// V1f: equal (P, I, K) with another A address has a model, since nothing relates x to k.
// OFF: "reason": "V1f dependence: sat",
// OFF-NEXT: "status": "UNKNOWN",
// OFF-NEXT: "tier": "PROPOSED"
// OFF-NOT: index_invariants

// The search finds x == x0 + k and proves it by one query per path between cuts. The
// coordinate checker, handed the certificate with that invariant, discharges V1f and
// V1g and reaches the symbolic tier; the same family with the certificate as it was
// stays where it stood. Each subscript is proved equal to a term over the coordinates.
// AUTO: "index_invariants": {
// AUTO-NEXT: "atoms_beyond_the_termination_certificate": 1,
// AUTO-NEXT: "automatic": true,
// AUTO: "confirmation_runs": 0,
// AUTO: "index_checker": {
// AUTO-NEXT: "V1f": "unsat",
// AUTO-NEXT: "V1g": "unsat",
// AUTO-NEXT: "V1h": "unsat",
// AUTO-NEXT: "reason": "V0 to V4 and S1 to S3 discharged",
// AUTO-NEXT: "status": "VERIFIED",
// AUTO-NEXT: "tier": "SYMBOLIC_EXACT"
// AUTO: "index_checker_without_the_invariants": {
// AUTO-NEXT: "V1f": "sat",
// AUTO-NEXT: "V1g": "not asked",
// AUTO-NEXT: "V1h": "unsat",
// AUTO-NEXT: "reason": "V1f dependence: sat",
// AUTO-NEXT: "status": "UNKNOWN",
// AUTO-NEXT: "tier": "PROPOSED"
// AUTO: "operand": "A",
// AUTO: "operand": "A",
// AUTO-NEXT: "origin": "the subscript rewritten by the equalities proved at L5",
// AUTO-NEXT: "over_coordinates": true,
// AUTO-NEXT: "reason": "IDX-use unsat at 1 arrival(s) at the site, under the invariants",
// AUTO-NEXT: "relation": "(p2 + s5)",
// AUTO-NEXT: "site": 0,
// AUTO-NEXT: "status": "PROVED",
// AUTO-NEXT: "subscript": "s6"
// AUTO: "operand": "B",
// AUTO: "relation": "(p2 + s5)",
// AUTO: "IDX-use": {
// AUTO-NEXT: "unsat": 6
// AUTO: "checker_verdict": "PROVEN_TERMINATING",
// AUTO: "strengthened": true
// AUTO: "(s6 == (p2 + s5))"
// AUTO-NEXT: ],
// AUTO-NEXT: "cut": 5,
// AUTO: "origin": "search: Houdini over the null space of the sampled states at each cut, degree 1",
// AUTO: "invariant": "INDUCTIVE",
// AUTO-NEXT: "loop_unrolling": "none: IDX-init and IDX-step are one query per path between cuts, whatever the trip counts",
// AUTO: "IDX-init": {
// AUTO-NEXT: "unsat": 1
// AUTO: "IDX-step": {
// AUTO-NEXT: "unsat": 5
// AUTO: "program_runs": 0,
// AUTO-NEXT: "queries": 6,
// AUTO: "location": "L3 -0-> L4 -0-> L5",
// AUTO-NEXT: "result": "unsat"
// AUTO: "location": "L5 -0-> L6 -0-> L5",
// AUTO-NEXT: "result": "unsat"
// AUTO: "sample_points": 24,
// AUTO-NEXT: "sample_runs": 24,
// AUTO: "status": "INDUCTIVE"

// The same invariant given by hand: checked with no search and no sample, and reported
// as given, never as found.
// GIVEN: "index_invariants": {
// GIVEN-NEXT: "atoms_beyond_the_termination_certificate": 1,
// GIVEN-NEXT: "automatic": false,
// GIVEN: "tier": "SYMBOLIC_EXACT"
// GIVEN: "strengthened": true
// GIVEN: "origin": "given",
// GIVEN: "invariant": "INDUCTIVE",
// GIVEN: "sample_runs": 0,
// GIVEN: "template": "none: the certificate was given"
// GIVEN: "status": "INDUCTIVE"

// A wrong invariant is refused where the k loop is entered. Nothing is handed on: the
// coordinate checker stays at V1f, and no relation is judged under it.
// BAD: "reason": "V1f dependence: sat",
// BAD: "index_invariants": {
// BAD: "automatic": false,
// BAD-NEXT: "candidates": [],
// BAD: "strengthened": false
// BAD: "falsified": {
// BAD-NEXT: "atoms": [
// BAD-NEXT: "(s6 == ((s5 + p2) + 1))"
// BAD: "obligation": "IDX-step",
// BAD-NEXT: "refutes": "the invariant candidate, not the program"
// BAD: "invariant": "NOT_INDUCTIVE",
// BAD: "reason": "IDX-step has a model on L3 -0-> L4 -0-> L5: the conjunction is not inductive (the model's state need not be reachable), at L5"
// BAD: "status": "NOT_INDUCTIVE"

// A wrong index relation is refuted by a run of the program, and only then.
// REL: "relation": "((s5 + p2) + 1)",
// REL-NEXT: "site": 0,
// REL-NEXT: "status": "REFUTED",
// REL-NEXT: "subscript": "s6",
// REL-NEXT: "witness": {
// REL: "refutes": "the relation, by a run of the program; not the program",
// REL: "IDX-use": {
// REL-NEXT: "sat": 1,
// REL-NEXT: "unsat": 6

// The entry condition is missing: x is carried through the j loop, so x == x0 + k fails
// on the path that enters the k loop from the j header.
// ENTRY: "automatic": false,
// ENTRY: "strengthened": false
// ENTRY: "obligation": "IDX-step",
// ENTRY: "reason": "IDX-step has a model on L3 -0-> L4 -0-> L5: the conjunction is not inductive
// ENTRY: "status": "NOT_INDUCTIVE"

// x carried at 8 bits beside a 16-bit counter, with K up to 300: the relation that holds
// through the wrap is found and proved, with as many queries as for any K.
// NARROW: "automatic": true,
// NARROW: "tier": "SYMBOLIC_EXACT"
// NARROW: "relation": "zext16((p2 + trunc8(s6)))",
// NARROW: "(s7 == (p2 + trunc8(s6)))"
// NARROW: "invariant": "INDUCTIVE",
// NARROW: "program_runs": 0,
// NARROW-NEXT: "queries": 6,
// The relation that reads x as an integer is not inductive once x can wrap.
// INTEGER: "strengthened": false
// INTEGER: "(zext16(s7) == (s6 + zext16(p2)))"
// INTEGER: "obligation": "IDX-step",
// INTEGER: "reason": "IDX-step has a model on L5 -0-> L6 -0-> L5
// INTEGER: "status": "NOT_INDUCTIVE"

// A strength-reduced GEMM over linearized arrays, every subscript a recurrence: the row
// starts need degree 2 (ra == i*kk, rc == i*m, at the outer headers), and each subscript
// is then proved to be a term over the coordinates.
// BUMP: "tier": "SYMBOLIC_EXACT"
// BUMP: "index_checker_without_the_invariants": {
// BUMP: "reason": "V1h dependence: sat",
// BUMP: "relation": "((s2 * p2) + s7)",
// BUMP-NEXT: "site": 0,
// BUMP-NEXT: "status": "PROVED",
// BUMP: "relation": "(s5 + (p1 * s7))",
// BUMP: "relation": "((s2 * p1) + s5)",
// BUMP: "(s3 == (p2 * s2))",
// BUMP-NEXT: "(s4 == (p1 * s2))"
// BUMP: "invariant": "INDUCTIVE",
// BUMP: "degree": 2,

// The domain report. Without the option no view of A or B is proposed. With it, the
// proposer is handed x = p2 + k and the domain checker proves the views over the levels.
// DOMOFF: "detail": "nothing proposed",
// DOMOFF-NOT: index_invariants
// DOM: "coordinate_symbolic": "SYMBOLIC_EXACT",
// DOM: "recognition": {
// DOM-NEXT: "authorizes_lowering": false,
// DOM-NEXT: "decomposition": "VERIFIED at SYMBOLIC",
// DOM: "views": "PROVEN over the scope",
// DOM-NEXT: "views_affine": true
// DOM: "views": {
// DOM-NEXT: "A": {
// DOM-NEXT: "affine": true,
// DOM: "status": "PROVEN",
// DOM: "status": "PROVEN",
// DOM: "index_identities": [
// DOM: "origin": "index invariant (s6 == (p2 + s5)) at L5, solved for s6",
// DOM: "index_invariants": {
// DOM: "identities_for_the_proposer": [
// DOM: "status": "INDUCTIVE"

func.func @gemm_x(%a: memref<8x16xi32>, %b: memref<16x3xi32>, %c: memref<8x3xi32>,
                  %n: index, %kk: index, %x0: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c3 = arith.constant 3 : index
  scf.for %i = %c0 to %n step %c1 {
    scf.for %j = %c0 to %c3 step %c1 {
      %xf = scf.for %k = %c0 to %kk step %c1 iter_args(%x = %x0) -> (index) {
        %p = memref.load %a[%i, %x] : memref<8x16xi32>
        %q = memref.load %b[%x, %j] : memref<16x3xi32>
        %z = memref.load %c[%i, %j] : memref<8x3xi32>
        %m = arith.muli %p, %q : i32
        %s = arith.addi %z, %m : i32
        memref.store %s, %c[%i, %j] : memref<8x3xi32>
        %x1 = arith.addi %x, %c1 : index
        scf.yield %x1 : index
      }
    }
  }
  return
}
func.func @carried(%a: memref<8x16xi32>, %b: memref<16x3xi32>, %c: memref<8x3xi32>,
                   %n: index, %kk: index, %x0: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c3 = arith.constant 3 : index
  scf.for %i = %c0 to %n step %c1 {
    %xo = scf.for %j = %c0 to %c3 step %c1 iter_args(%xj = %x0) -> (index) {
      %xf = scf.for %k = %c0 to %kk step %c1 iter_args(%x = %xj) -> (index) {
        %p = memref.load %a[%i, %x] : memref<8x16xi32>
        %q = memref.load %b[%x, %j] : memref<16x3xi32>
        %z = memref.load %c[%i, %j] : memref<8x3xi32>
        %m = arith.muli %p, %q : i32
        %s = arith.addi %z, %m : i32
        memref.store %s, %c[%i, %j] : memref<8x3xi32>
        %x1 = arith.addi %x, %c1 : index
        scf.yield %x1 : index
      }
      scf.yield %xf : index
    }
  }
  return
}
func.func @bump(%a: memref<64xi32>, %b: memref<64xi32>, %c: memref<64xi32>,
                %n: index, %m: index, %kk: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %raf, %rcf = scf.for %i = %c0 to %n step %c1 iter_args(%ra = %c0, %rc = %c0) -> (index, index) {
    scf.for %j = %c0 to %m step %c1 {
      %cj = arith.addi %rc, %j : index
      %xf, %yf = scf.for %k = %c0 to %kk step %c1 iter_args(%x = %ra, %y = %j) -> (index, index) {
        %p = memref.load %a[%x] : memref<64xi32>
        %q = memref.load %b[%y] : memref<64xi32>
        %z = memref.load %c[%cj] : memref<64xi32>
        %mm = arith.muli %p, %q : i32
        %s = arith.addi %z, %mm : i32
        memref.store %s, %c[%cj] : memref<64xi32>
        %x1 = arith.addi %x, %c1 : index
        %y1 = arith.addi %y, %m : index
        scf.yield %x1, %y1 : index, index
      }
    }
    %ra1 = arith.addi %ra, %kk : index
    %rc1 = arith.addi %rc, %m : index
    scf.yield %ra1, %rc1 : index, index
  }
  return
}
func.func @narrow(%a: memref<4x256xi32>, %b: memref<256x3xi32>, %c: memref<4x3xi32>,
                  %n: index, %kk: index, %x0: i8) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c3 = arith.constant 3 : index
  %one = arith.constant 1 : i8
  scf.for %i = %c0 to %n step %c1 {
    scf.for %j = %c0 to %c3 step %c1 {
      %xf = scf.for %k = %c0 to %kk step %c1 iter_args(%x = %x0) -> (i8) {
        %xi = arith.index_castui %x : i8 to index
        %p = memref.load %a[%i, %xi] : memref<4x256xi32>
        %q = memref.load %b[%xi, %j] : memref<256x3xi32>
        %z = memref.load %c[%i, %j] : memref<4x3xi32>
        %m = arith.muli %p, %q : i32
        %s = arith.addi %z, %m : i32
        memref.store %s, %c[%i, %j] : memref<4x3xi32>
        %x1 = arith.addi %x, %one : i8
        scf.yield %x1 : i8
      }
    }
  }
  return
}
