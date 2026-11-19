// RUN: %report %s --print-smt | FileCheck %s
// CHECK: (declare-const loc (_ BitVec 16))
// CHECK: (declare-const s0 (_ BitVec 8))
// CHECK: (declare-const s0_n (_ BitVec 8))
// CHECK: bvadd
// CHECK: (define-fun Stuck_UB () Bool
// CHECK: (define-fun T () Bool
func.func @f(%n: i8) -> i8 {
  %z = arith.constant 0 : i8
  %o = arith.constant 1 : i8
  cf.br ^h(%z : i8)
^h(%i: i8):
  %t = arith.cmpi ult, %i, %n : i8
  cf.cond_br %t, ^b, ^e
^b:
  %x = arith.addi %i, %o overflow<nuw> : i8
  cf.br ^h(%x : i8)
^e:
  return %i : i8
}
