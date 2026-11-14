// The loop-free statement grammar (docs/GENERALIZED_GEMM_STATEMENT_BV.md): off by
// default and then absent from the output; on, a branch on the program's own guard
// is a coordinate, judged by the same checkers.
//
// two_phase: K = if t < n then t else t - n. The statement grammar has it at level 0,
// and the family goes through every later checker unchanged, to the symbolic tiers.
// RUN: %synthreport %s --function=two_phase --lower-structured --index-bit-width=8 --scope=0:2:3 --max-level=3 | FileCheck %s --check-prefix=PLAIN
// RUN: %synthreport %s --function=two_phase --lower-structured --index-bit-width=8 --scope=0:2:3 --max-level=3 --statement-grammar | FileCheck %s --check-prefix=STMT
// RUN: %domreport %s --function=two_phase --lower-structured --index-bit-width=8 --scope=0:2:3 --max-level=3 --statement-grammar --candidates-per-group=1 --rlimit=2000000 --timeout-ms=3000 --authorize="roots 0 and 1 are separate,roots 0 and 2 are separate,roots 1 and 2 are separate" | FileCheck %s --check-prefix=DOM
// PLAIN: "level": 2
// PLAIN-NOT: index_programs
// PLAIN-NOT: statement programs
// STMT: "candidates": [
// STMT: "finite": {
// STMT-NEXT: "equivalence": "UNKNOWN"
// STMT-NEXT: "reason":
// STMT-NEXT: "status": "VERIFIED"
// STMT-NEXT: "tier": "SCOPE_ENUMERATED"
// STMT: "index_programs": {
// STMT: "k": [
// STMT-NEXT: "if {{.*}} < {{.*}} then (v0 := {{.*}}) else (v0 := (sub:bv8 {{.*}})) ; return v0"
// STMT: "origin": "selector search over statement programs"
// STMT: "symbolic": {
// STMT: "status": "VERIFIED"
// STMT-NEXT: "tier": "SYMBOLIC_EXACT"
// STMT: "level": 0
// DOM: "authorizes_lowering": false
// DOM: "computation_finite": {
// DOM-NEXT: "reason": "Herbrand replay equal at each listed point, for all initial memories"
// DOM-NEXT: "status": "VERIFIED"
// DOM-NEXT: "tier": "SCOPE_ENUMERATED"
// DOM: "computation_symbolic": {
// DOM-NEXT: "reason": "E, OP, R, W and F discharged over the scope"
// DOM-NEXT: "status": "VERIFIED"
// DOM-NEXT: "tier": "SYMBOLIC"
// DOM: "domain_finite": {
// DOM: "exact_order": true
// DOM: "status": "VERIFIED"
// DOM: "domain_symbolic": {
// DOM: "exact_order": true
// DOM: "status": "VERIFIED"
// DOM: "index_programs": {
// DOM: "origin": "selector search over statement programs"
// DOM: "theorem13": {
// DOM: "status": "VERIFIED"
// DOM: "tier": "SYMBOLIC"
func.func @two_phase(%a: memref<9x9xi32>, %b: memref<9x2xi32>, %c: memref<9x2xi32>, %n: index) {
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
        %r = arith.subi %t, %n : index
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
