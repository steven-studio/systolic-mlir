module {
  func.func @nonlinear(%n: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    %two = arith.constant 2 : i32
    cf.br ^header(%zero, %two : i32, i32)
  ^header(%t: i32, %x: i32):
    %more = arith.cmpi slt, %t, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %xn = arith.muli %x, %x : i32
    %tn = arith.addi %t, %one : i32
    cf.br ^header(%tn, %xn : i32, i32)
  ^exit:
    return %x : i32
  }
  func.func @cyclic_dependencies(%n: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    cf.br ^header(%zero, %one, %one : i32, i32, i32)
  ^header(%t: i32, %x: i32, %y: i32):
    %more = arith.cmpi slt, %t, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %xn = arith.addi %x, %y : i32
    %yn = arith.addi %y, %x : i32
    %tn = arith.addi %t, %one : i32
    cf.br ^header(%tn, %xn, %yn : i32, i32, i32)
  ^exit:
    return %x : i32
  }
  func.func @conditional_update(%n: i32, %flag: i1) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    cf.br ^header(%zero, %zero : i32, i32)
  ^header(%i: i32, %s: i32):
    %more = arith.cmpi slt, %i, %n : i32
    cf.cond_br %more, ^choose, ^exit
  ^choose:
    cf.cond_br %flag, ^yes, ^no
  ^yes:
    %sn = arith.addi %s, %i : i32
    cf.br ^latch(%sn : i32)
  ^no:
    cf.br ^latch(%s : i32)
  ^latch(%chosen: i32):
    %in = arith.addi %i, %one : i32
    cf.br ^header(%in, %chosen : i32, i32)
  ^exit:
    return %s : i32
  }
  func.func @multiple_backedges(%n: i32, %flag: i1) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    cf.br ^header(%zero, %zero : i32, i32)
  ^header(%i: i32, %s: i32):
    %more = arith.cmpi slt, %i, %n : i32
    cf.cond_br %more, ^choose, ^exit
  ^choose:
    %in = arith.addi %i, %one : i32
    %sn = arith.addi %s, %i : i32
    cf.cond_br %flag, ^latch1, ^latch2
  ^latch1:
    cf.br ^header(%in, %sn : i32, i32)
  ^latch2:
    cf.br ^header(%in, %sn : i32, i32)
  ^exit:
    return %s : i32
  }
  func.func @irreducible(%choose: i1, %again: i1) {
    cf.cond_br %choose, ^a, ^b
  ^a:
    cf.br ^b
  ^b:
    cf.cond_br %again, ^a, ^exit
  ^exit:
    return
  }
  func.func @multiple_entries(%n: i32, %choose: i1) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    cf.cond_br %choose, ^init1, ^init2
  ^init1:
    cf.br ^header(%zero : i32)
  ^init2:
    cf.br ^header(%one : i32)
  ^header(%i: i32):
    %more = arith.cmpi slt, %i, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %in = arith.addi %i, %one : i32
    cf.br ^header(%in : i32)
  ^exit:
    return %i : i32
  }
  func.func @memory_dependency(%n: i32, %memory: memref<1xi32>) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    %idx = arith.constant 0 : index
    cf.br ^header(%zero, %zero : i32, i32)
  ^header(%i: i32, %s: i32):
    %more = arith.cmpi slt, %i, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %v = memref.load %memory[%idx] : memref<1xi32>
    %sn = arith.addi %s, %v : i32
    %in = arith.addi %i, %one : i32
    cf.br ^header(%in, %sn : i32, i32)
  ^exit:
    return %s : i32
  }
  func.func @unused_memory_effect(%n: i32, %memory: memref<1xi32>) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    %idx = arith.constant 0 : index
    cf.br ^header(%zero, %zero : i32, i32)
  ^header(%i: i32, %s: i32):
    %more = arith.cmpi slt, %i, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    memref.store %s, %memory[%idx] : memref<1xi32>
    %sn = arith.addi %s, %i : i32
    %in = arith.addi %i, %one : i32
    cf.br ^header(%in, %sn : i32, i32)
  ^exit:
    return %s : i32
  }
  func.func @dynamic_cast(%n: i64) -> i64 {
    %zero = arith.constant 0 : i64
    %one = arith.constant 1 : i64
    cf.br ^header(%zero, %zero : i64, i64)
  ^header(%i: i64, %s: i64):
    %more = arith.cmpi slt, %i, %n : i64
    cf.cond_br %more, ^body, ^exit
  ^body:
    %narrow = arith.trunci %i : i64 to i8
    %back = arith.extsi %narrow : i8 to i64
    %sn = arith.addi %s, %back : i64
    %in = arith.addi %i, %one : i64
    cf.br ^header(%in, %sn : i64, i64)
  ^exit:
    return %s : i64
  }
  func.func @division(%n: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    %two = arith.constant 2 : i32
    cf.br ^header(%zero, %zero : i32, i32)
  ^header(%i: i32, %s: i32):
    %more = arith.cmpi slt, %i, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %half = arith.divsi %i, %two : i32
    %sn = arith.addi %s, %half : i32
    %in = arith.addi %i, %one : i32
    cf.br ^header(%in, %sn : i32, i32)
  ^exit:
    return %s : i32
  }
  func.func @mixed_width(%n: i32) -> i64 {
    %zero32 = arith.constant 0 : i32
    %zero64 = arith.constant 0 : i64
    %one = arith.constant 1 : i32
    cf.br ^header(%zero32, %zero64 : i32, i64)
  ^header(%i: i32, %s: i64):
    %more = arith.cmpi slt, %i, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %wide = arith.extsi %i : i32 to i64
    %sn = arith.addi %s, %wide : i64
    %in = arith.addi %i, %one : i32
    cf.br ^header(%in, %sn : i32, i64)
  ^exit:
    return %s : i64
  }
  func.func @unknown_initial(%n: i32, %seed: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    cf.br ^header(%zero, %seed : i32, i32)
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
  func.func @index_state(%n: index) -> index {
    %zero = arith.constant 0 : index
    %one = arith.constant 1 : index
    cf.br ^header(%zero : index)
  ^header(%i: index):
    %more = arith.cmpi slt, %i, %n : index
    cf.cond_br %more, ^body, ^exit
  ^body:
    %in = arith.addi %i, %one : index
    cf.br ^header(%in : index)
  ^exit:
    return %i : index
  }
  func.func @select_update(%n: i32, %choose: i1) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    cf.br ^header(%zero, %zero : i32, i32)
  ^header(%i: i32, %s: i32):
    %more = arith.cmpi slt, %i, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %value = arith.select %choose, %i, %zero : i32
    %sn = arith.addi %s, %value : i32
    %in = arith.addi %i, %one : i32
    cf.br ^header(%in, %sn : i32, i32)
  ^exit:
    return %s : i32
  }
}
