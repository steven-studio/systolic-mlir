module {
  func.func @canceled_intermediate(%n: i8) -> i8 {
    %zero = arith.constant 0 : i8
    %one = arith.constant 1 : i8
    %sixteen = arith.constant 16 : i8
    cf.br ^header(%zero, %sixteen, %zero : i8, i8, i8)
  ^header(%t: i8, %x: i8, %s: i8):
    %more = arith.cmpi ult, %t, %n : i8
    cf.cond_br %more, ^body, ^exit
  ^body:
    %square = arith.muli %x, %x : i8
    %cancel = arith.subi %square, %square : i8
    %sn = arith.addi %s, %cancel : i8
    %tn = arith.addi %t, %one : i8
    cf.br ^header(%tn, %x, %sn : i8, i8, i8)
  ^exit:
    return %s : i8
  }
  func.func @dead_intermediate(%n: i8) -> i8 {
    %zero = arith.constant 0 : i8
    %one = arith.constant 1 : i8
    %max = arith.constant 127 : i8
    cf.br ^header(%zero, %zero : i8, i8)
  ^header(%i: i8, %s: i8):
    %more = arith.cmpi ult, %i, %n : i8
    cf.cond_br %more, ^body, ^exit
  ^body:
    %unused = arith.addi %max, %one : i8
    %sn = arith.addi %s, %i : i8
    %in = arith.addi %i, %one : i8
    cf.br ^header(%in, %sn : i8, i8)
  ^exit:
    return %s : i8
  }
  func.func @header_intermediate(%n: i8) -> i8 {
    %zero = arith.constant 0 : i8
    %one = arith.constant 1 : i8
    %max = arith.constant 127 : i8
    cf.br ^header(%zero : i8)
  ^header(%i: i8):
    %unused = arith.addi %i, %max : i8
    %more = arith.cmpi ult, %i, %n : i8
    cf.cond_br %more, ^body, ^exit
  ^body:
    %in = arith.addi %i, %one : i8
    cf.br ^header(%in : i8)
  ^exit:
    return %i : i8
  }
  func.func @signed_flag(%n: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    cf.br ^header(%zero, %zero : i32, i32)
  ^header(%i: i32, %s: i32):
    %more = arith.cmpi slt, %i, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %sn = arith.addi %s, %i overflow<nsw> : i32
    %in = arith.addi %i, %one overflow<nsw> : i32
    cf.br ^header(%in, %sn : i32, i32)
  ^exit:
    return %s : i32
  }
  func.func @unsigned_flag(%n: i8) -> i8 {
    %zero = arith.constant 0 : i8
    %one = arith.constant 1 : i8
    cf.br ^header(%zero : i8)
  ^header(%i: i8):
    %more = arith.cmpi ult, %i, %n : i8
    cf.cond_br %more, ^body, ^exit
  ^body:
    %in = arith.addi %i, %one overflow<nuw> : i8
    cf.br ^header(%in : i8)
  ^exit:
    return %i : i8
  }
  func.func @loose_but_safe(%n: i8) -> i8 {
    %zero = arith.constant 0 : i8
    %one = arith.constant 1 : i8
    %two = arith.constant 2 : i8
    %nineteen = arith.constant 19 : i8
    cf.br ^header(%zero, %zero : i8, i8)
  ^header(%i: i8, %s: i8):
    %more = arith.cmpi ult, %i, %n : i8
    cf.cond_br %more, ^body, ^exit
  ^body:
    %twice = arith.muli %two, %i : i8
    %step = arith.subi %nineteen, %twice : i8
    %sn = arith.addi %s, %step : i8
    %in = arith.addi %i, %one : i8
    cf.br ^header(%in, %sn : i8, i8)
  ^exit:
    return %s : i8
  }
}
