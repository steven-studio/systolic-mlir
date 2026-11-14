// rotated (docs/GENERALIZED_GEMM_STATEMENT_BV.md): K = if t < n then t else (t + 1 - n) mod n.
// No term of the expression grammar within levels 0 to 3 is a full family (the unit test
// asserts that); the statement grammar's is accepted at the
// finite tier. The later stages say UNKNOWN with their reason: the G5a proposer has no
// guarded lexicographic order for a rotated phase. That is a limit of the later stage,
// and it is reported as it is.
// RUN: %synthreport %s --function=rotated --lower-structured --index-bit-width=8 --scope=0:2:3 --max-level=3 --statement-grammar | FileCheck %s --check-prefix=RSTMT
// RUN: %domreport %s --function=rotated --lower-structured --index-bit-width=8 --scope=0:2:3 --max-level=3 --statement-grammar --candidates-per-group=1 --rlimit=2000000 --timeout-ms=3000 --authorize="roots 0 and 1 are separate,roots 0 and 2 are separate,roots 1 and 2 are separate" | FileCheck %s --check-prefix=RDOM
// RSTMT: "finite": {
// RSTMT-NEXT: "equivalence": "UNKNOWN"
// RSTMT-NEXT: "reason":
// RSTMT-NEXT: "status": "VERIFIED"
// RSTMT-NEXT: "tier": "SCOPE_ENUMERATED"
// RSTMT: "index_programs": {
// RSTMT: "k": [
// RSTMT-NEXT: "if {{.*}} < {{.*}} then (v0 := {{.*}}) else (v0 := (urem:bv8 {{.*}})) ; return v0"
// RSTMT: "origin": "selector search over statement programs"
// RSTMT-NOT: "tier": "SYMBOLIC
// RDOM: "authorizes_lowering": false
// RDOM: "computation_finite": {
// RDOM-NEXT: "reason": "not checked"
// RDOM-NEXT: "status": "UNKNOWN"
// RDOM: "index_programs": {
// RDOM: "proposal_reason": "no guard from the program's conditions is exact at the samples"
// RDOM: "theorem13": {
// RDOM: "status": "UNKNOWN"
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
