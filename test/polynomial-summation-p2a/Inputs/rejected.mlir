module {
  func.func @nonlinear(%n: i32, %alpha: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    cf.br ^header(%zero, %alpha : i32, i32)
  ^header(%t: i32, %s: i32):
    %more = arith.cmpi slt, %t, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %sn = arith.muli %s, %s : i32
    %tn = arith.addi %t, %one : i32
    cf.br ^header(%tn, %sn : i32, i32)
  ^exit:
    return %s : i32
  }
  func.func @cyclic(%n: i32, %alpha: i32, %beta: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    cf.br ^header(%zero, %alpha, %beta : i32, i32, i32)
  ^header(%t: i32, %s: i32, %x: i32):
    %more = arith.cmpi slt, %t, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %sn = arith.addi %s, %x : i32
    %xn = arith.addi %x, %s : i32
    %tn = arith.addi %t, %one : i32
    cf.br ^header(%tn, %sn, %xn : i32, i32, i32)
  ^exit:
    return %s : i32
  }
  func.func @symbolic_extui(%n: i32, %alpha: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    %small = arith.trunci %alpha : i32 to i8
    %cast = arith.extui %small : i8 to i32
    cf.br ^header(%zero, %cast : i32, i32)
  ^header(%t: i32, %s: i32):
    %more = arith.cmpi slt, %t, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %sn = arith.addi %s, %one : i32
    %tn = arith.addi %t, %one : i32
    cf.br ^header(%tn, %sn : i32, i32)
  ^exit:
    return %s : i32
  }
  func.func @symbolic_nneg(%n: i32, %alpha: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    %small = arith.trunci %alpha : i32 to i8
    %cast = arith.extui %small nneg : i8 to i32
    cf.br ^header(%zero, %cast : i32, i32)
  ^header(%t: i32, %s: i32):
    %more = arith.cmpi slt, %t, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %sn = arith.addi %s, %one : i32
    %tn = arith.addi %t, %one : i32
    cf.br ^header(%tn, %sn : i32, i32)
  ^exit:
    return %s : i32
  }
  func.func @variant_cast(%n: i32, %alpha: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    cf.br ^header(%zero, %alpha : i32, i32)
  ^header(%t: i32, %s: i32):
    %more = arith.cmpi slt, %t, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %small = arith.trunci %s : i32 to i8
    %cast = arith.extui %small nneg : i8 to i32
    %sn = arith.addi %s, %cast : i32
    %tn = arith.addi %t, %one : i32
    cf.br ^header(%tn, %sn : i32, i32)
  ^exit:
    return %s : i32
  }
  func.func @poison_constant(%n: i32, %alpha: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    cf.br ^header(%zero, %alpha : i32, i32)
  ^header(%t: i32, %s: i32):
    %more = arith.cmpi slt, %t, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %small = arith.constant -1 : i8
    %cast = arith.extui %small nneg : i8 to i32
    %sn = arith.addi %s, %cast : i32
    %tn = arith.addi %t, %one : i32
    cf.br ^header(%tn, %sn : i32, i32)
  ^exit:
    return %s : i32
  }
  func.func @parameter_degree(%n: i32, %alpha: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    %power0 = arith.muli %alpha, %alpha : i32
    %power1 = arith.muli %power0, %power0 : i32
    %power2 = arith.muli %power1, %power1 : i32
    %power3 = arith.muli %power2, %power2 : i32
    %power4 = arith.muli %power3, %power3 : i32
    cf.br ^header(%zero, %power4 : i32, i32)
  ^header(%t: i32, %s: i32):
    %more = arith.cmpi slt, %t, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %sn = arith.addi %s, %one : i32
    %tn = arith.addi %t, %one : i32
    cf.br ^header(%tn, %sn : i32, i32)
  ^exit:
    return %s : i32
  }
  func.func @memory(%n: i32, %ptr: memref<i32>) -> i32 {
    %alpha = memref.load %ptr[] : memref<i32>
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    cf.br ^header(%zero, %alpha : i32, i32)
  ^header(%t: i32, %s: i32):
    %more = arith.cmpi slt, %t, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %sn = arith.addi %s, %one : i32
    %tn = arith.addi %t, %one : i32
    cf.br ^header(%tn, %sn : i32, i32)
  ^exit:
    return %s : i32
  }
  func.func @conditional(%n: i32, %alpha: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    cf.br ^header(%zero, %alpha : i32, i32)
  ^header(%t: i32, %s: i32):
    %more = arith.cmpi slt, %t, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %choose = arith.cmpi slt, %s, %zero : i32
    cf.cond_br %choose, ^left, ^right
  ^left:
    %a = arith.addi %s, %one : i32
    cf.br ^join(%a : i32)
  ^right:
    cf.br ^join(%s : i32)
  ^join(%sn: i32):
    %tn = arith.addi %t, %one : i32
    cf.br ^header(%tn, %sn : i32, i32)
  ^exit:
    return %s : i32
  }
}

