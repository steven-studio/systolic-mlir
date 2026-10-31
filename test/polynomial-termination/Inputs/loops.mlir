module {
  func.func @inc_s(%start: i8, %bound: i8) -> i8 {
    %step = arith.constant 1 : i8
    cf.br ^header(%start : i8)
  ^header(%i: i8):
    %g = arith.cmpi slt, %i, %bound : i8
    cf.cond_br %g, ^body, ^exit
  ^body:

    %next = arith.addi %i, %step : i8
    cf.br ^header(%next : i8)
  ^exit:
    return %i : i8
  }
  func.func @dec_s(%start: i8, %bound: i8) -> i8 {
    %step = arith.constant -1 : i8
    cf.br ^header(%start : i8)
  ^header(%i: i8):
    %g = arith.cmpi sgt, %i, %bound : i8
    cf.cond_br %g, ^body, ^exit
  ^body:

    %next = arith.addi %i, %step : i8
    cf.br ^header(%next : i8)
  ^exit:
    return %i : i8
  }
  func.func @inc_u(%start: i8, %bound: i8) -> i8 {
    %step = arith.constant 1 : i8
    cf.br ^header(%start : i8)
  ^header(%i: i8):
    %g = arith.cmpi ult, %i, %bound : i8
    cf.cond_br %g, ^body, ^exit
  ^body:

    %next = arith.addi %i, %step : i8
    cf.br ^header(%next : i8)
  ^exit:
    return %i : i8
  }
  func.func @dec_u(%start: i8, %bound: i8) -> i8 {
    %step = arith.constant -1 : i8
    cf.br ^header(%start : i8)
  ^header(%i: i8):
    %g = arith.cmpi ugt, %i, %bound : i8
    cf.cond_br %g, ^body, ^exit
  ^body:

    %next = arith.addi %i, %step : i8
    cf.br ^header(%next : i8)
  ^exit:
    return %i : i8
  }
  func.func @inclusive(%start: i8, %bound: i8) -> i8 {
    %step = arith.constant 1 : i8
    cf.br ^header(%start : i8)
  ^header(%i: i8):
    %g = arith.cmpi sle, %i, %bound : i8
    cf.cond_br %g, ^body, ^exit
  ^body:

    %next = arith.addi %i, %step : i8
    cf.br ^header(%next : i8)
  ^exit:
    return %i : i8
  }
  func.func @step_two(%start: i8, %bound: i8) -> i8 {
    %step = arith.constant 2 : i8
    cf.br ^header(%start : i8)
  ^header(%i: i8):
    %g = arith.cmpi slt, %i, %bound : i8
    cf.cond_br %g, ^body, ^exit
  ^body:

    %next = arith.addi %i, %step : i8
    cf.br ^header(%next : i8)
  ^exit:
    return %i : i8
  }
  func.func @wrong_direction(%start: i8, %bound: i8) -> i8 {
    %step = arith.constant 1 : i8
    cf.br ^header(%start : i8)
  ^header(%i: i8):
    %g = arith.cmpi sgt, %i, %bound : i8
    cf.cond_br %g, ^body, ^exit
  ^body:

    %next = arith.addi %i, %step : i8
    cf.br ^header(%next : i8)
  ^exit:
    return %i : i8
  }
  func.func @stutter(%start: i8, %bound: i8) -> i8 {
    %step = arith.constant 0 : i8
    cf.br ^header(%start : i8)
  ^header(%i: i8):
    %g = arith.cmpi slt, %i, %bound : i8
    cf.cond_br %g, ^body, ^exit
  ^body:

    %next = arith.addi %i, %step : i8
    cf.br ^header(%next : i8)
  ^exit:
    return %i : i8
  }
  func.func @always_true(%start: i8, %bound: i8) -> i8 {
    %step = arith.constant 1 : i8
    cf.br ^header(%start : i8)
  ^header(%i: i8):
    %g = arith.constant true
    cf.cond_br %g, ^body, ^exit
  ^body:

    %next = arith.addi %i, %step : i8
    cf.br ^header(%next : i8)
  ^exit:
    return %i : i8
  }
  func.func @always_false(%start: i8, %bound: i8) -> i8 {
    %step = arith.constant 1 : i8
    cf.br ^header(%start : i8)
  ^header(%i: i8):
    %g = arith.constant false
    cf.cond_br %g, ^body, ^exit
  ^body:

    %next = arith.addi %i, %step : i8
    cf.br ^header(%next : i8)
  ^exit:
    return %i : i8
  }
  func.func @invariant_guard(%start: i8, %bound: i8) -> i8 {
    %step = arith.constant 1 : i8
    cf.br ^header(%start : i8)
  ^header(%i: i8):
    %g = arith.cmpi slt, %start, %bound : i8
    cf.cond_br %g, ^body, ^exit
  ^body:

    %next = arith.addi %i, %step : i8
    cf.br ^header(%next : i8)
  ^exit:
    return %i : i8
  }
  func.func @swapped(%start: i8, %bound: i8) -> i8 {
    %step = arith.constant 1 : i8
    cf.br ^header(%start : i8)
  ^header(%i: i8):
    %g = arith.cmpi sgt, %bound, %i : i8
    cf.cond_br %g, ^body, ^exit
  ^body:

    %next = arith.addi %i, %step : i8
    cf.br ^header(%next : i8)
  ^exit:
    return %i : i8
  }
  func.func @reversed_polarity(%start: i8, %bound: i8) -> i8 {
    %step = arith.constant 1 : i8
    cf.br ^header(%start : i8)
  ^header(%i: i8):
    %g = arith.cmpi sge, %i, %bound : i8
    cf.cond_br %g, ^exit, ^body
  ^body:

    %next = arith.addi %i, %step : i8
    cf.br ^header(%next : i8)
  ^exit:
    return %i : i8
  }
  func.func @square_guard(%start: i8, %bound: i8) -> i8 {
    %step = arith.constant 1 : i8
    cf.br ^header(%start : i8)
  ^header(%i: i8):
    %square = arith.muli %i, %i : i8
    %g = arith.cmpi slt, %square, %bound : i8
    cf.cond_br %g, ^body, ^exit
  ^body:

    %next = arith.addi %i, %step : i8
    cf.br ^header(%next : i8)
  ^exit:
    return %i : i8
  }
  func.func @equality(%start: i8, %bound: i8) -> i8 {
    %step = arith.constant 1 : i8
    cf.br ^header(%start : i8)
  ^header(%i: i8):
    %g = arith.cmpi ne, %i, %bound : i8
    cf.cond_br %g, ^body, ^exit
  ^body:

    %next = arith.addi %i, %step : i8
    cf.br ^header(%next : i8)
  ^exit:
    return %i : i8
  }
  func.func @nsw(%start: i8, %bound: i8) -> i8 {
    %step = arith.constant 1 : i8
    cf.br ^header(%start : i8)
  ^header(%i: i8):
    %g = arith.cmpi slt, %i, %bound : i8
    cf.cond_br %g, ^body, ^exit
  ^body:

    %next = arith.addi %i, %step overflow<nsw> : i8
    cf.br ^header(%next : i8)
  ^exit:
    return %i : i8
  }
  func.func @nuw(%start: i8, %bound: i8) -> i8 {
    %step = arith.constant 1 : i8
    cf.br ^header(%start : i8)
  ^header(%i: i8):
    %g = arith.cmpi slt, %i, %bound : i8
    cf.cond_br %g, ^body, ^exit
  ^body:

    %next = arith.addi %i, %step overflow<nuw> : i8
    cf.br ^header(%next : i8)
  ^exit:
    return %i : i8
  }
  func.func @both_flags(%start: i8, %bound: i8) -> i8 {
    %step = arith.constant 1 : i8
    cf.br ^header(%start : i8)
  ^header(%i: i8):
    %g = arith.cmpi slt, %i, %bound : i8
    cf.cond_br %g, ^body, ^exit
  ^body:

    %next = arith.addi %i, %step overflow<nsw, nuw> : i8
    cf.br ^header(%next : i8)
  ^exit:
    return %i : i8
  }
  func.func @zero_poison(%start: i8, %bound: i8) -> i8 {
    %step = arith.constant 1 : i8
    cf.br ^header(%start : i8)
  ^header(%i: i8):
    %g = arith.constant false
    cf.cond_br %g, ^body, ^exit
  ^body:

    %next = arith.addi %i, %step overflow<nsw> : i8
    cf.br ^header(%next : i8)
  ^exit:
    return %i : i8
  }
  func.func @mustprogress(%start: i8, %bound: i8) -> i8 attributes {passthrough = ["mustprogress"]} {
    %step = arith.constant 1 : i8
    cf.br ^header(%start : i8)
  ^header(%i: i8):
    %g = arith.cmpi slt, %i, %bound : i8
    cf.cond_br %g, ^body, ^exit
  ^body:

    %next = arith.addi %i, %step : i8
    cf.br ^header(%next : i8)
  ^exit:
    return %i : i8
  }
  func.func @willreturn(%start: i8, %bound: i8) -> i8 attributes {llvm.willreturn} {
    %step = arith.constant 1 : i8
    cf.br ^header(%start : i8)
  ^header(%i: i8):
    %g = arith.cmpi slt, %i, %bound : i8
    cf.cond_br %g, ^body, ^exit
  ^body:

    %next = arith.addi %i, %step : i8
    cf.br ^header(%next : i8)
  ^exit:
    return %i : i8
  }
  func.func @unknown_attribute(%start: i8, %bound: i8) -> i8 attributes {research.unhandled} {
    %step = arith.constant 1 : i8
    cf.br ^header(%start : i8)
  ^header(%i: i8):
    %g = arith.cmpi slt, %i, %bound : i8
    cf.cond_br %g, ^body, ^exit
  ^body:

    %next = arith.addi %i, %step : i8
    cf.br ^header(%next : i8)
  ^exit:
    return %i : i8
  }
  func.func @nonlinear(%start: i8, %bound: i8) -> i8 {
    %step = arith.constant 1 : i8
    cf.br ^header(%start : i8)
  ^header(%i: i8):
    %g = arith.cmpi slt, %i, %bound : i8
    cf.cond_br %g, ^body, ^exit
  ^body:

    %next = arith.muli %i, %i : i8
    cf.br ^header(%next : i8)
  ^exit:
    return %i : i8
  }
  func.func @too_wide(%start: i128, %bound: i128) -> i128 {
    %step = arith.constant 1 : i128
    cf.br ^header(%start : i128)
  ^header(%i: i128):
    %g = arith.cmpi slt, %i, %bound : i128
    cf.cond_br %g, ^body, ^exit
  ^body:

    %next = arith.addi %i, %step : i128
    cf.br ^header(%next : i128)
  ^exit:
    return %i : i128
  }
  func.func @cast(%start: i8, %bound: i8) -> i8 {
    %step = arith.constant 1 : i8
    cf.br ^header(%start : i8)
  ^header(%i: i8):
    %g = arith.cmpi slt, %i, %bound : i8
    cf.cond_br %g, ^body, ^exit
  ^body:
    %small = arith.trunci %i : i8 to i4
    %next = arith.addi %i, %step : i8
    cf.br ^header(%next : i8)
  ^exit:
    return %i : i8
  }
  func.func @nneg(%start: i8, %bound: i8) -> i8 {
    %step = arith.constant 1 : i8
    cf.br ^header(%start : i8)
  ^header(%i: i8):
    %g = arith.cmpi slt, %i, %bound : i8
    cf.cond_br %g, ^body, ^exit
  ^body:
    %raw = arith.constant -1 : i4
    %bad = arith.extui %raw nneg : i4 to i8
    %next = arith.addi %i, %step : i8
    cf.br ^header(%next : i8)
  ^exit:
    return %i : i8
  }
  func.func @signed_one_bit(%start: i1, %bound: i1) -> i1 {
    %step = arith.constant 1 : i1
    cf.br ^header(%start : i1)
  ^header(%i: i1):
    %g = arith.cmpi slt, %i, %bound : i1
    cf.cond_br %g, ^body, ^exit
  ^body:

    %next = arith.addi %i, %step : i1
    cf.br ^header(%next : i1)
  ^exit:
    return %i : i1
  }
  func.func @forwarded(%start: i8, %bound: i8, %alpha: i8) -> i8 {
    cf.br ^setup(%alpha, %start : i8, i8)
  ^body:
    %sq = arith.muli %i, %i : i8
    %sum_next = arith.addi %sum, %sq : i8
    %i_next = arith.addi %i, %one : i8
    cf.br ^latch(%i_next, %sum_next : i8, i8)
  ^setup(%a: i8, %b: i8):
    %one = arith.constant 1 : i8
    cf.br ^header(%a, %b : i8, i8)
  ^exit:
    return %sum : i8
  ^latch(%ni: i8, %ns: i8):
    cf.br ^header(%ns, %ni : i8, i8)
  ^header(%sum: i8, %i: i8):
    %g = arith.cmpi slt, %i, %bound : i8
    cf.cond_br %g, ^body, ^exit
  ^unreachable:
    %unused = arith.divsi %start, %bound : i8
    return %unused : i8
  }
  func.func @symbolic_step(%start: i8, %bound: i8, %step: i8) -> i8 {
    cf.br ^header(%start : i8)
  ^header(%i: i8):
    %g = arith.cmpi slt, %i, %bound : i8
    cf.cond_br %g, ^body, ^exit
  ^body:
    %next = arith.addi %i, %step : i8
    cf.br ^header(%next : i8)
  ^exit:
    return %i : i8
  }
  func.func @multiphase(%q0: i8, %y0: i8) -> i8 {
    %zero = arith.constant 0 : i8
    %one = arith.constant 1 : i8
    cf.br ^header(%q0, %y0 : i8, i8)
  ^header(%q: i8, %y: i8):
    %g = arith.cmpi sgt, %q, %zero : i8
    cf.cond_br %g, ^body, ^exit
  ^body:
    %qn = arith.subi %q, %y : i8
    %yn = arith.addi %y, %one : i8
    cf.br ^header(%qn, %yn : i8, i8)
  ^exit:
    return %q : i8
  }
  func.func @variant_bound(%start: i8, %bound: i8) -> i8 {
    %one = arith.constant 1 : i8
    cf.br ^header(%start, %bound : i8, i8)
  ^header(%i: i8, %b: i8):
    %g = arith.cmpi slt, %i, %b : i8
    cf.cond_br %g, ^body, ^exit
  ^body:
    %ni = arith.addi %i, %one : i8
    %nb = arith.addi %b, %one : i8
    cf.br ^header(%ni, %nb : i8, i8)
  ^exit:
    return %i : i8
  }
  func.func @lexicographic(%x0: i8, %y0: i8) -> i8 {
    %zero = arith.constant 0 : i8
    %one = arith.constant 1 : i8
    cf.br ^header(%x0, %y0 : i8, i8)
  ^header(%x: i8, %y: i8):
    %g = arith.cmpi sgt, %x, %zero : i8
    cf.cond_br %g, ^choose, ^exit
  ^choose:
    %inner = arith.cmpi sgt, %y, %zero : i8
    cf.cond_br %inner, ^decy, ^decx
  ^decy:
    %yn = arith.subi %y, %one : i8
    cf.br ^header(%x, %yn : i8, i8)
  ^decx:
    %xn = arith.subi %x, %one : i8
    cf.br ^header(%xn, %y0 : i8, i8)
  ^exit:
    return %x : i8
  }
  func.func @collatz(%initial: i8) -> i8 {
    %one = arith.constant 1 : i8
    %two = arith.constant 2 : i8
    %three = arith.constant 3 : i8
    %zero = arith.constant 0 : i8
    cf.br ^header(%initial : i8)
  ^header(%i: i8):
    %g = arith.cmpi ugt, %i, %one : i8
    cf.cond_br %g, ^body, ^exit
  ^body:
    %parity = arith.remui %i, %two : i8
    %even = arith.cmpi eq, %parity, %zero : i8
    cf.cond_br %even, ^left, ^right
  ^left:
    %half = arith.divui %i, %two : i8
    cf.br ^header(%half : i8)
  ^right:
    %product = arith.muli %i, %three : i8
    %inc = arith.addi %product, %one : i8
    cf.br ^header(%inc : i8)
  ^exit:
    return %i : i8
  }
  func.func @memory(%ptr: memref<i8>, %bound: i8) -> i8 {
    %initial = memref.load %ptr[] : memref<i8>
    %one = arith.constant 1 : i8
    cf.br ^header(%initial : i8)
  ^header(%i: i8):
    %g = arith.cmpi slt, %i, %bound : i8
    cf.cond_br %g, ^body, ^exit
  ^body:
    %ni = arith.addi %i, %one : i8
    cf.br ^header(%ni : i8)
  ^exit:
    return %i : i8
  }
  func.func @recursive_call(%start: i8, %bound: i8) -> i8 {
    %one = arith.constant 1 : i8
    cf.br ^header(%start : i8)
  ^header(%i: i8):
    %g = arith.cmpi slt, %i, %bound : i8
    cf.cond_br %g, ^body, ^exit
  ^body:
    %unused = func.call @recursive_call(%i, %bound) : (i8, i8) -> i8
    %ni = arith.addi %i, %one : i8
    cf.br ^header(%ni : i8)
  ^exit:
    return %i : i8
  }
  func.func @latch_test(%start: i8, %bound: i8) -> i8 {
    %one = arith.constant 1 : i8
    cf.br ^header(%start : i8)
  ^header(%i: i8):
    %ni = arith.addi %i, %one : i8
    cf.br ^latch
  ^latch:
    %g = arith.cmpi slt, %ni, %bound : i8
    cf.cond_br %g, ^header(%ni : i8), ^exit
  ^exit:
    return %ni : i8
  }
}
