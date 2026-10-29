// Real, verified SSA CFGs. Names describe tests; extraction never uses them.
module {
  func.func @sum_integers(%n: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    cf.br ^header(%zero, %zero : i32, i32)
  ^header(%i: i32, %s: i32):
    %more = arith.cmpi slt, %i, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %sn = arith.addi %s, %i : i32
    %in = arith.addi %i, %one : i32
    cf.br ^header(%in, %sn : i32, i32)
  ^exit:
    return %s : i32
  }
  func.func @sum_squares(%n: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    cf.br ^header(%zero, %zero : i32, i32)
  ^header(%i: i32, %s: i32):
    %more = arith.cmpi slt, %i, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %square = arith.muli %i, %i : i32
    %sn = arith.addi %s, %square : i32
    %in = arith.addi %i, %one : i32
    cf.br ^header(%in, %sn : i32, i32)
  ^exit:
    return %s : i32
  }
  func.func @sum_cubes(%n: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    cf.br ^header(%zero, %zero : i32, i32)
  ^header(%i: i32, %s: i32):
    %more = arith.cmpi slt, %i, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %square = arith.muli %i, %i : i32
    %cube = arith.muli %square, %i : i32
    %sn = arith.addi %cube, %s : i32
    %in = arith.addi %i, %one : i32
    cf.br ^header(%in, %sn : i32, i32)
  ^exit:
    return %s : i32
  }
  func.func @sum_fifth(%n: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    cf.br ^header(%zero, %zero : i32, i32)
  ^header(%i: i32, %s: i32):
    %more = arith.cmpi slt, %i, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %square = arith.muli %i, %i : i32
    %fourth = arith.muli %square, %square : i32
    %fifth = arith.muli %fourth, %i : i32
    %sn = arith.addi %s, %fifth : i32
    %in = arith.addi %i, %one : i32
    cf.br ^header(%in, %sn : i32, i32)
  ^exit:
    return %s : i32
  }
  func.func @quadratic_shifted(%n: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    %two = arith.constant 2 : i32
    %three = arith.constant 3 : i32
    %seven = arith.constant 7 : i32
    %eleven = arith.constant 11 : i32
    %start = arith.constant -5 : i32
    %initial = arith.constant 17 : i32
    cf.br ^header(%zero, %start, %initial : i32, i32, i32)
  ^header(%t: i32, %i: i32, %s: i32):
    %more = arith.cmpi slt, %t, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %square = arith.muli %i, %i : i32
    %a = arith.muli %three, %square : i32
    %b = arith.muli %seven, %i : i32
    %c = arith.subi %a, %b : i32
    %d = arith.addi %c, %eleven : i32
    %sn = arith.addi %s, %d : i32
    %in = arith.addi %i, %two : i32
    %tn = arith.addi %t, %one : i32
    cf.br ^header(%tn, %in, %sn : i32, i32, i32)
  ^exit:
    return %s : i32
  }
  func.func @cubic_negative_step(%n: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    %two = arith.constant 2 : i32
    %three = arith.constant 3 : i32
    %five = arith.constant 5 : i32
    %start = arith.constant 19 : i32
    %initial = arith.constant -31 : i32
    cf.br ^header(%zero, %start, %initial : i32, i32, i32)
  ^header(%t: i32, %i: i32, %s: i32):
    %more = arith.cmpi slt, %t, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %square = arith.muli %i, %i : i32
    %cube = arith.muli %square, %i : i32
    %a = arith.muli %two, %cube : i32
    %b = arith.subi %a, %square : i32
    %c = arith.addi %b, %i : i32
    %d = arith.addi %c, %five : i32
    %sn = arith.addi %s, %d : i32
    %in = arith.subi %i, %three : i32
    %tn = arith.addi %t, %one : i32
    cf.br ^header(%tn, %in, %sn : i32, i32, i32)
  ^exit:
    return %s : i32
  }
  func.func @triangular(%n: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    %three = arith.constant 3 : i32
    %seven = arith.constant 7 : i32
    cf.br ^header(%zero, %three, %seven : i32, i32, i32)
  ^header(%x: i32, %y: i32, %z: i32):
    %more = arith.cmpi slt, %x, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %square = arith.muli %x, %x : i32
    %yn = arith.addi %y, %square : i32
    %zn = arith.addi %z, %y : i32
    %xn = arith.addi %x, %one : i32
    cf.br ^header(%xn, %yn, %zn : i32, i32, i32)
  ^exit:
    return %z : i32
  }
  // Mutation: z explicitly uses the SSA definition of NEXT y.
  func.func @triangular_next_y(%n: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    %three = arith.constant 3 : i32
    %seven = arith.constant 7 : i32
    cf.br ^header(%zero, %three, %seven : i32, i32, i32)
  ^header(%x: i32, %y: i32, %z: i32):
    %more = arith.cmpi slt, %x, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %square = arith.muli %x, %x : i32
    %yn = arith.addi %y, %square : i32
    %zn = arith.addi %z, %yn : i32
    %xn = arith.addi %x, %one : i32
    cf.br ^header(%xn, %yn, %zn : i32, i32, i32)
  ^exit:
    return %z : i32
  }
  // Mutation: sum uses NEXT i, giving sum(1^2..n^2).
  func.func @sum_squares_next_i(%n: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    cf.br ^header(%zero, %zero : i32, i32)
  ^header(%i: i32, %s: i32):
    %more = arith.cmpi slt, %i, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %in = arith.addi %i, %one : i32
    %square = arith.muli %in, %in : i32
    %sn = arith.addi %s, %square : i32
    cf.br ^header(%in, %sn : i32, i32)
  ^exit:
    return %s : i32
  }
  func.func @forwarded_noncanonical(%n: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    cf.br ^initialize(%zero : i32)
  ^exit(%answer: i32):
    return %answer : i32
  ^relay(%newSum: i32, %newIndex: i32):
    cf.br ^header(%newIndex, %newSum : i32, i32)
  ^body(%oldSum: i32, %oldIndex: i32):
    %square = arith.muli %oldIndex, %oldIndex : i32
    %sn = arith.addi %oldSum, %square : i32
    %in = arith.addi %oldIndex, %one : i32
    cf.br ^relay(%sn, %in : i32, i32)
  ^header(%i: i32, %s: i32):
    %done = arith.cmpi sge, %i, %n : i32
    cf.cond_br %done, ^exit(%s : i32), ^body(%s, %i : i32, i32)
  ^initialize(%seed: i32):
    cf.br ^header(%seed, %seed : i32, i32)
  }
  func.func @unreachable_predecessor(%n: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    cf.br ^header(%zero, %zero : i32, i32)
  ^dead:
    %alien = arith.constant 91 : i32
    %unsupported = arith.divui %alien, %alien : i32
    cf.br ^header(%unsupported, %alien : i32, i32)
  ^header(%i: i32, %s: i32):
    %more = arith.cmpi slt, %i, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %sn = arith.addi %s, %i : i32
    %in = arith.addi %i, %one : i32
    cf.br ^header(%in, %sn : i32, i32)
  ^exit:
    return %s : i32
  }
  func.func @posttested(%n: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    cf.br ^header(%zero, %zero : i32, i32)
  ^header(%i: i32, %s: i32):
    %sn = arith.addi %s, %i : i32
    %in = arith.addi %i, %one : i32
    %more = arith.cmpi slt, %in, %n : i32
    cf.cond_br %more, ^header(%in, %sn : i32, i32), ^exit(%sn : i32)
  ^exit(%answer: i32):
    return %answer : i32
  }
  func.func @constant_casts(%n: i32) -> i32 {
    %small = arith.constant -3 : i8
    %initial = arith.extsi %small : i8 to i32
    %raw = arith.constant -1 : i8
    %large = arith.extui %raw : i8 to i64
    %oneWide = arith.constant 1 : i64
    %one = arith.trunci %oneWide : i64 to i32
    %seed = arith.trunci %large : i64 to i32
    %zero = arith.constant 0 : i32
    cf.br ^header(%zero, %initial, %seed : i32, i32, i32)
  ^header(%t: i32, %i: i32, %s: i32):
    %more = arith.cmpi slt, %t, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %sn = arith.addi %s, %i : i32
    %in = arith.addi %i, %one : i32
    %tn = arith.addi %t, %one : i32
    cf.br ^header(%tn, %in, %sn : i32, i32, i32)
  ^exit:
    return %s : i32
  }
  func.func @modular_overflow(%n: i8) -> i8 {
    %zero = arith.constant 0 : i8
    %one = arith.constant 1 : i8
    %start = arith.constant 120 : i8
    %step = arith.constant 7 : i8
    %initial = arith.constant -10 : i8
    cf.br ^header(%zero, %start, %initial : i8, i8, i8)
  ^header(%t: i8, %i: i8, %s: i8):
    %more = arith.cmpi ult, %t, %n : i8
    cf.cond_br %more, ^body, ^exit
  ^body:
    %square = arith.muli %i, %i : i8
    %sn = arith.addi %s, %square : i8
    %in = arith.addi %i, %step : i8
    %tn = arith.addi %t, %one : i8
    cf.br ^header(%tn, %in, %sn : i8, i8, i8)
  ^exit:
    return %s : i8
  }
}
