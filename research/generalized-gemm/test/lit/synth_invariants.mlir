// The inductive invariant path (docs/GENERALIZED_GEMM_STATEMENT_BV.md, Section 13):
//   for (int i = 0; i < N; i++) g += 2 * x++ + 1;   g_final = g0 + (x0 + N)^2 - x0^2
// with N = p0 >= 0 (the scope), x0 = p1, g0 = p2.
//
// RUN: %synthreport %s --function=odd_sum --loop-invariants '--loop-function=0:p2 + (p1 + p0) * (p1 + p0) - p1 * p1' --scope=0:0:2147483647 | FileCheck %s --check-prefix=SEARCH
// RUN: %synthreport %s --function=odd_sum --loop-invariants '--loop-function=0:p2 + (p1 + p0) * (p1 + p0) - p1 * p1' '--loop-invariant=(0 <=s s0) && (s0 <=s p0) && (s1 == p1 + s0) && (s2 - p2 == s1 * s1 - p1 * p1)' --scope=0:0:2147483647 --loop-invariant-scripts | FileCheck %s --check-prefix=GIVEN
// RUN: %synthreport %s --function=odd_sum --loop-invariants '--loop-function=0:p2 + (p1 + p0) * (p1 + p0)' --scope=0:0:2147483647 | FileCheck %s --check-prefix=WRONG
// RUN: %synthreport %s --function=odd_sum --loop-invariants '--loop-function=0:p2 + (p1 + p0) * (p1 + p0) - p1 * p1' '--loop-invariant=(0 <=s s0) && (s0 <=s p0) && (s1 == p1 + s0) && (s2 - p2 == s1 * s1)' --scope=0:0:2147483647 | FileCheck %s --check-prefix=BADINV
// RUN: %synthreport %s --function=odd_sum_nsw --loop-invariants '--loop-function=0:p2 + (p1 + p0) * (p1 + p0) - p1 * p1' --scope=0:0:2147483647 | FileCheck %s --check-prefix=NSW
// RUN: %synthreport %s --function=memory --index-bit-width=32 --loop-invariants | FileCheck %s --check-prefix=MEMORY
// RUN: %synthreport %s --function=odd_sum --scope=0:0:2147483647 | FileCheck %s --check-prefix=OFF

// The search's certificate: N symbolic, no iteration run by the proof, the claim joined
// with the G2a termination proof.
// SEARCH: "certificate": {
// SEARCH: "origin": "search: Houdini over the template
// SEARCH: "claim": "total correctness over the scope: every run returns, and returns the candidate's value modulo 2^32
// SEARCH: "counterexample": null
// SEARCH: "poison": "excluded"
// SEARCH: "program_runs": 0,
// SEARCH-NEXT: "program_steps": 0,
// SEARCH: "role": "candidate generation only: no obligation reads a sample"
// SEARCH: "status": "PROVED_BY_INVARIANT"
// SEARCH: "verdict": "PROVEN_TERMINATING"
// SEARCH: "unrolling_omitted": true

// The request's own certificate, checked with no search and no run: Pre, G, T with its
// frame, X and Post as the program says, and every obligation's script.
// GIVEN: "atoms": [
// GIVEN-NEXT: "(0 <=s s0)",
// GIVEN-NEXT: "(s0 <=s p0)",
// GIVEN-NEXT: "(s1 == (p1 + s0))",
// GIVEN-NEXT: "((s2 - p2) == ((s1 * s1) - (p1 * p1)))"
// GIVEN: "origin": "given"
// GIVEN: "lemmas": [
// GIVEN-NEXT: "s0 == p0"
// GIVEN: "id": "INV-init",
// GIVEN: "result": "unsat",
// GIVEN: "script": "; INV-init
// GIVEN: "id": "INV-step",
// GIVEN: "result": "unsat",
// GIVEN: "id": "INV-exit-eq",
// GIVEN: "result": "unsat",
// GIVEN: "id": "INV-exit",
// GIVEN: "result": "unsat",
// GIVEN: "program_runs": 0,
// GIVEN: "runs": 0,
// GIVEN: "exit": [
// GIVEN: "condition": "(!(s0 <s p0) && !sp0)",
// GIVEN: "post": [
// GIVEN-NEXT: "r0 == ((p2 + ((p1 + p0) * (p1 + p0))) - (p1 * p1))"
// GIVEN: "pre": [
// GIVEN: "s1'": "p1",
// GIVEN: "transition": [
// GIVEN: "frame": [],
// GIVEN: "s0'": "(s0 + 1)",
// GIVEN-NEXT: "s1'": "(s1 + 1)",
// GIVEN-NEXT: "s2'": "(s2 + ((2 * s1) + 1))"
// GIVEN: "status": "PROVED_BY_INVARIANT"

// A wrong candidate (the -x0^2 is missing): refuted by a run of the program.
// WRONG: "claim": "the candidate is wrong: a run of the program returns another value"
// WRONG: "counterexample": {
// WRONG: "returned_by_the_run": [
// WRONG: "status": "REFUTED"
// WRONG: "unrolling_omitted": false

// A wrong invariant: an initial state violates its last atom. It proves nothing, and the
// fallback says only what the runs say.
// BADINV: "claim": "sample tests only: equal on the runs made; not a proof"
// BADINV: "falsified_atoms": [
// BADINV-NEXT: 3
// BADINV: "falsified_by": "INV-init"
// BADINV: "verdict": "NOT_INDUCTIVE"
// BADINV: "status": "SAMPLE_CONSISTENT"
// BADINV: "unrolling_omitted": false

// C's int: the bits are proved, and a poison return (an overflow) is not excluded.
// NSW: "claim": "total correctness {{.*}} A returned value is not shown free of poison
// NSW: "poison": "not excluded"
// NSW: "status": "PROVED_BY_INVARIANT"

// MEMORY: "reason": "the function has 1 memory root(s): arrays and memory are outside stage 1"
// MEMORY: "status": "UNSUPPORTED"

// Without the option nothing changes.
// OFF-NOT: loop_invariant

func.func @odd_sum(%n: i32, %x0: i32, %g0: i32) -> i32 {
  %c0 = arith.constant 0 : i32
  cf.br ^header(%c0, %x0, %g0 : i32, i32, i32)
^header(%i: i32, %x: i32, %g: i32):
  %more = arith.cmpi slt, %i, %n : i32
  cf.cond_br %more, ^body, ^done
^body:
  %c1 = arith.constant 1 : i32
  %c2 = arith.constant 2 : i32
  %twice = arith.muli %c2, %x : i32
  %odd = arith.addi %twice, %c1 : i32
  %g1 = arith.addi %g, %odd : i32
  %x1 = arith.addi %x, %c1 : i32
  %i1 = arith.addi %i, %c1 : i32
  cf.br ^header(%i1, %x1, %g1 : i32, i32, i32)
^done:
  return %g : i32
}

func.func @odd_sum_nsw(%n: i32, %x0: i32, %g0: i32) -> i32 {
  %c0 = arith.constant 0 : i32
  cf.br ^header(%c0, %x0, %g0 : i32, i32, i32)
^header(%i: i32, %x: i32, %g: i32):
  %more = arith.cmpi slt, %i, %n : i32
  cf.cond_br %more, ^body, ^done
^body:
  %c1 = arith.constant 1 : i32
  %c2 = arith.constant 2 : i32
  %twice = arith.muli %c2, %x overflow<nsw> : i32
  %odd = arith.addi %twice, %c1 overflow<nsw> : i32
  %g1 = arith.addi %g, %odd overflow<nsw> : i32
  %x1 = arith.addi %x, %c1 overflow<nsw> : i32
  %i1 = arith.addi %i, %c1 overflow<nsw> : i32
  cf.br ^header(%i1, %x1, %g1 : i32, i32, i32)
^done:
  return %g : i32
}

func.func @memory(%m: memref<4xi32>, %n: i32) -> i32 {
  %c0 = arith.constant 0 : i32
  %z = arith.constant 0 : index
  cf.br ^header(%c0, %c0 : i32, i32)
^header(%i: i32, %s: i32):
  %more = arith.cmpi slt, %i, %n : i32
  cf.cond_br %more, ^body, ^done
^body:
  %v = memref.load %m[%z] : memref<4xi32>
  %s1 = arith.addi %s, %v : i32
  %c1 = arith.constant 1 : i32
  %i1 = arith.addi %i, %c1 : i32
  cf.br ^header(%i1, %s1 : i32, i32)
^done:
  return %s : i32
}
