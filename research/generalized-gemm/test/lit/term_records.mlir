// Every obligation records its id, CFG location, formula, answer, evidence and
// premises, and the formula is a complete script.
// RUN: %termreport %s --print-scripts --replay-fuel=1000 | FileCheck %s
// RUN: %termreport %s --print-scripts --replay-fuel=1000 --polynomial-hints=false | FileCheck %s --check-prefix=NOHINT
// RUN: %report %s --print-smt > %t.before
// RUN: %termreport %s > /dev/null
// RUN: %report %s --print-smt | diff - %t.before
// CHECK: "id": "O0"
// CHECK: "expected": "sat",
// CHECK-NEXT: "id": "O1"
// CHECK: "id": "O2"
// CHECK: "id": "O3"
// CHECK: "assumptions": [
// CHECK: "evidence": "rank at L1
// CHECK: "id": "O5"
// CHECK-NEXT: "location": "SCC { L1 } path L1
// CHECK: "result": "unsat"
// CHECK: "script": "; O5
// CHECK-SAME: (check-sat)
// CHECK: "id": "O7"
// NOHINT: "origin": "T1 guard distance"
func.func @f(%n: i8) {
  %z = arith.constant 0 : i8
  %o = arith.constant 1 : i8
  cf.br ^h(%z : i8)
^h(%i: i8):
  %g = arith.cmpi slt, %i, %n : i8
  cf.cond_br %g, ^b, ^x
^b:
  %u = arith.addi %i, %o overflow<nsw> : i8
  cf.br ^h(%u : i8)
^x:
  return
}
