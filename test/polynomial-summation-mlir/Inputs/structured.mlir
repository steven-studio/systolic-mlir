module {
  func.func @structured(%n: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    %result:2 = scf.while (%i = %zero, %s = %zero) : (i32, i32) -> (i32, i32) {
      %more = arith.cmpi slt, %i, %n : i32
      scf.condition(%more) %i, %s : i32, i32
    } do {
    ^bb0(%i: i32, %s: i32):
      %square = arith.muli %i, %i : i32
      %sn = arith.addi %s, %square : i32
      %in = arith.addi %i, %one : i32
      scf.yield %in, %sn : i32, i32
    }
    return %result#1 : i32
  }
}
