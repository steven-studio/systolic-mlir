func.func @invalid_dominance(%n: i32) -> i32 {
  %zero = arith.constant 0 : i32
  cf.br ^header(%zero : i32)
^header(%i: i32):
  %test = arith.cmpi slt, %late, %n : i32
  cf.cond_br %test, ^body, ^exit
^body:
  %late = arith.addi %i, %i : i32
  cf.br ^header(%late : i32)
^exit:
  return %i : i32
}
