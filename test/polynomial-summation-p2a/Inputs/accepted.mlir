module {
  func.func @accumulator(%n: i32, %alpha: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    cf.br ^header(%zero, %alpha : i32, i32)
  ^header(%t: i32, %s: i32):
    %more = arith.cmpi slt, %t, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %sn = arith.addi %s, %t : i32
    %tn = arith.addi %t, %one : i32
    cf.br ^header(%tn, %sn : i32, i32)
  ^exit:
    return %s : i32
  }
  func.func @shifted_square(%n: i32, %alpha: i32, %beta: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    cf.br ^header(%zero, %beta, %alpha : i32, i32, i32)
  ^header(%t: i32, %i: i32, %s: i32):
    %more = arith.cmpi slt, %t, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %square = arith.muli %i, %i : i32
    %sn = arith.addi %s, %square : i32
    %in = arith.addi %i, %one : i32
    %tn = arith.addi %t, %one : i32
    cf.br ^header(%tn, %in, %sn : i32, i32, i32)
  ^exit:
    return %s : i32
  }
  func.func @negative_step(%n: i32, %alpha: i32, %beta: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    %two = arith.constant 2 : i32
    cf.br ^header(%zero, %beta, %alpha : i32, i32, i32)
  ^header(%t: i32, %i: i32, %s: i32):
    %more = arith.cmpi slt, %t, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %square = arith.muli %i, %i : i32
    %sn = arith.addi %s, %square : i32
    %in = arith.subi %i, %two : i32
    %tn = arith.addi %t, %one : i32
    cf.br ^header(%tn, %in, %sn : i32, i32, i32)
  ^exit:
    return %s : i32
  }
  func.func @symbolic_step(%n: i32, %alpha: i32, %beta: i32, %step: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    cf.br ^header(%zero, %beta, %alpha : i32, i32, i32)
  ^header(%t: i32, %i: i32, %s: i32):
    %more = arith.cmpi slt, %t, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %square = arith.muli %i, %i : i32
    %sn = arith.addi %s, %square : i32
    %in = arith.addi %i, %step : i32
    %tn = arith.addi %t, %one : i32
    cf.br ^header(%tn, %in, %sn : i32, i32, i32)
  ^exit:
    return %s : i32
  }
  func.func @polynomial_offset(%n: i32, %alpha: i32, %beta: i32, %offset: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    %offset2 = arith.muli %beta, %beta : i32
    %start = arith.subi %beta, %offset : i32
    %initial = arith.addi %alpha, %offset2 : i32
    cf.br ^header(%zero, %start, %initial : i32, i32, i32)
  ^header(%t: i32, %i: i32, %s: i32):
    %more = arith.cmpi slt, %t, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %shift = arith.addi %i, %offset : i32
    %square = arith.muli %shift, %shift : i32
    %inc = arith.addi %square, %offset2 : i32
    %sn = arith.addi %s, %inc : i32
    %in = arith.subi %i, %one : i32
    %tn = arith.addi %t, %one : i32
    cf.br ^header(%tn, %in, %sn : i32, i32, i32)
  ^exit:
    return %s : i32
  }
  func.func @wrap8(%n: i8, %alpha: i8, %beta: i8, %offset: i8) -> i8 {
    %zero = arith.constant 0 : i8
    %one = arith.constant 1 : i8
    %offset2 = arith.muli %beta, %beta : i8
    %start = arith.subi %beta, %offset : i8
    %initial = arith.addi %alpha, %offset2 : i8
    cf.br ^header(%zero, %start, %initial : i8, i8, i8)
  ^header(%t: i8, %i: i8, %s: i8):
    %more = arith.cmpi slt, %t, %n : i8
    cf.cond_br %more, ^body, ^exit
  ^body:
    %shift = arith.addi %i, %offset : i8
    %square = arith.muli %shift, %shift : i8
    %inc = arith.addi %square, %offset2 : i8
    %sn = arith.addi %s, %inc : i8
    %in = arith.subi %i, %one : i8
    %tn = arith.addi %t, %one : i8
    cf.br ^header(%tn, %in, %sn : i8, i8, i8)
  ^exit:
    return %s : i8
  }
  func.func @triangular_old(%n: i32, %alpha: i32, %beta: i32, %gamma: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    cf.br ^header(%zero, %beta, %alpha, %gamma : i32, i32, i32, i32)
  ^header(%t: i32, %x: i32, %y: i32, %s: i32):
    %more = arith.cmpi slt, %t, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %square = arith.muli %x, %x : i32
    %yn = arith.addi %y, %square : i32
    %sn = arith.addi %s, %y : i32
    %xn = arith.addi %x, %one : i32
    %tn = arith.addi %t, %one : i32
    cf.br ^header(%tn, %xn, %yn, %sn : i32, i32, i32, i32)
  ^exit:
    return %s : i32
  }
  func.func @triangular_next(%n: i32, %alpha: i32, %beta: i32, %gamma: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    cf.br ^header(%zero, %beta, %alpha, %gamma : i32, i32, i32, i32)
  ^header(%t: i32, %x: i32, %y: i32, %s: i32):
    %more = arith.cmpi slt, %t, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %square = arith.muli %x, %x : i32
    %yn = arith.addi %y, %square : i32
    %sn = arith.addi %s, %yn : i32
    %xn = arith.addi %x, %one : i32
    %tn = arith.addi %t, %one : i32
    cf.br ^header(%tn, %xn, %yn, %sn : i32, i32, i32, i32)
  ^exit:
    return %s : i32
  }
  func.func @invariant_carried(%n: i32, %alpha: i32, %beta: i32, %factor: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    cf.br ^header(%zero, %beta, %alpha, %factor : i32, i32, i32, i32)
  ^header(%t: i32, %i: i32, %s: i32, %c: i32):
    %more = arith.cmpi slt, %t, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %inc = arith.muli %c, %i : i32
    %sn = arith.addi %s, %inc : i32
    %in = arith.addi %i, %one : i32
    %tn = arith.addi %t, %one : i32
    cf.br ^header(%tn, %in, %sn, %c : i32, i32, i32, i32)
  ^exit:
    return %s : i32
  }
  func.func @forwarded(%n: i32, %alpha: i32, %beta: i32) -> i32 {
    cf.br ^setup(%beta, %alpha : i32, i32)
  ^body:
    %square = arith.muli %i, %i : i32
    %inc = arith.addi %square, %a : i32
    %sn = arith.addi %s, %inc : i32
    %in = arith.addi %i, %one : i32
    %tn = arith.addi %t, %one : i32
    cf.br ^forward(%sn, %tn, %in : i32, i32, i32)
  ^setup(%b: i32, %a: i32):
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    %initial = arith.subi %a, %b : i32
    cf.br ^header(%zero, %b, %initial : i32, i32, i32)
  ^exit:
    return %s : i32
  ^forward(%news: i32, %newt: i32, %newi: i32):
    cf.br ^header(%newt, %newi, %news : i32, i32, i32)
  ^header(%t: i32, %i: i32, %s: i32):
    %more = arith.cmpi slt, %t, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^unreachable:
    %dead = arith.divsi %alpha, %beta : i32
    return %dead : i32
  }
}

