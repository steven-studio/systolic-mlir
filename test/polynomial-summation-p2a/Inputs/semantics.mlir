module {
  // A bounded flagged RESULT is insufficient when an operand wrapped first.
  func.func @flag_operand_wrap(%n: i32, %alpha: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    %max = arith.constant 2147483647 : i32
    cf.br ^header(%zero, %alpha : i32, i32)
  ^header(%t: i32, %s: i32):
    %more = arith.cmpi slt, %t, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %wrapped = arith.addi %alpha, %max : i32
    %flagged = arith.subi %wrapped, %alpha overflow<nsw> : i32
    %sn = arith.addi %s, %one : i32
    %tn = arith.addi %t, %one : i32
    cf.br ^header(%tn, %sn : i32, i32)
  ^exit:
    return %s : i32
  }
  func.func @flagged(%n: i32, %alpha: i32, %beta: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    cf.br ^header(%zero, %beta, %alpha : i32, i32, i32)
  ^header(%t: i32, %i: i32, %s: i32):
    %more = arith.cmpi slt, %t, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %square = arith.muli %i, %i overflow<nsw> : i32
    %sn = arith.addi %s, %square overflow<nsw> : i32
    %in = arith.addi %i, %one overflow<nsw> : i32
    %tn = arith.addi %t, %one : i32
    cf.br ^header(%tn, %in, %sn : i32, i32, i32)
  ^exit:
    return %s : i32
  }
  func.func @dead_flag(%n: i32, %alpha: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    cf.br ^header(%zero, %alpha : i32, i32)
  ^header(%t: i32, %s: i32):
    %more = arith.cmpi slt, %t, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %dead = arith.muli %alpha, %alpha overflow<nsw> : i32
    %sn = arith.addi %s, %one : i32
    %tn = arith.addi %t, %one : i32
    cf.br ^header(%tn, %sn : i32, i32)
  ^exit:
    return %s : i32
  }
  func.func @canceled_overflow(%n: i32, %alpha: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    cf.br ^header(%zero, %alpha : i32, i32)
  ^header(%t: i32, %s: i32):
    %more = arith.cmpi slt, %t, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %square = arith.muli %alpha, %alpha : i32
    %cancel = arith.subi %square, %square : i32
    %sn = arith.addi %s, %cancel : i32
    %tn = arith.addi %t, %one : i32
    cf.br ^header(%tn, %sn : i32, i32)
  ^exit:
    return %s : i32
  }
}

