func.func @invalid_branch() {
  %c = arith.constant 0 : i32
  cf.br ^bb(%c : i32)
^bb(%x: i64):
  return
}
