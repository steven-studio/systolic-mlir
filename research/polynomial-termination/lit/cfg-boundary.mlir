// RUN: %mlir-opt %s --verify-each -o /dev/null
// RUN: %report %s --function=multiple | %FileCheck %s
// RUN: %report %s --function=nested | %FileCheck %s
// RUN: %report %s --function=irreducible | %FileCheck %s
// CHECK: "certificate":null
// CHECK-SAME: "status":"UNKNOWN"
module {
  func.func @multiple(%n: i8) -> i8 {
    %zero = arith.constant 0 : i8
    %one = arith.constant 1 : i8
    cf.br ^first(%zero : i8)
  ^first(%i: i8):
    %c = arith.cmpi slt, %i, %n : i8
    cf.cond_br %c, ^firstbody, ^second(%zero : i8)
  ^firstbody:
    %ni = arith.addi %i, %one : i8
    cf.br ^first(%ni : i8)
  ^second(%j: i8):
    %d = arith.cmpi slt, %j, %n : i8
    cf.cond_br %d, ^secondbody, ^exit
  ^secondbody:
    %nj = arith.addi %j, %one : i8
    cf.br ^second(%nj : i8)
  ^exit:
    return %j : i8
  }
  func.func @nested(%n: i8) -> i8 {
    %zero = arith.constant 0 : i8
    %one = arith.constant 1 : i8
    cf.br ^outer(%zero : i8)
  ^outer(%i: i8):
    %c = arith.cmpi slt, %i, %n : i8
    cf.cond_br %c, ^inner(%zero : i8), ^exit
  ^inner(%j: i8):
    %d = arith.cmpi slt, %j, %n : i8
    cf.cond_br %d, ^innerbody, ^outerbody
  ^innerbody:
    %nj = arith.addi %j, %one : i8
    cf.br ^inner(%nj : i8)
  ^outerbody:
    %ni = arith.addi %i, %one : i8
    cf.br ^outer(%ni : i8)
  ^exit:
    return %i : i8
  }
  func.func @irreducible(%start: i8, %bound: i8) -> i8 {
    %one = arith.constant 1 : i8
    %first = arith.cmpi slt, %start, %bound : i8
    cf.cond_br %first, ^left(%start : i8), ^right(%start : i8)
  ^left(%i: i8):
    %ni = arith.addi %i, %one : i8
    %c = arith.cmpi slt, %ni, %bound : i8
    cf.cond_br %c, ^right(%ni : i8), ^exit(%ni : i8)
  ^right(%j: i8):
    %nj = arith.addi %j, %one : i8
    %d = arith.cmpi slt, %nj, %bound : i8
    cf.cond_br %d, ^left(%nj : i8), ^exit(%nj : i8)
  ^exit(%answer: i8):
    return %answer : i8
  }
}
