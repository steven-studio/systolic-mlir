func.func @invalid_duplicate() {
  %same = arith.constant 0 : i32
  %same = arith.constant 1 : i32
  return
}
