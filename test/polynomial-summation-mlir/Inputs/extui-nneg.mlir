// Constant-cast semantics at the exact SOURCE width. All functions verify.
// Poison cases must be UNKNOWN even in modular mode or with an assumption.
module {
  func.func @plain_negative(%n: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    %raw = arith.constant -1 : i8
    %cast = arith.extui %raw : i8 to i32
    cf.br ^header(%zero, %cast : i32, i32)
  ^header(%t: i32, %s: i32):
    %more = arith.cmpi ult, %t, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %tn = arith.addi %t, %one : i32
    %sn = arith.addi %s, %one : i32
    cf.br ^header(%tn, %sn : i32, i32)
  ^exit:
    return %s : i32
  }

  func.func @nneg_zero(%n: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    %raw = arith.constant 0 : i8
    %cast = arith.extui %raw nneg : i8 to i32
    cf.br ^header(%zero, %cast : i32, i32)
  ^header(%t: i32, %s: i32):
    %more = arith.cmpi ult, %t, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %tn = arith.addi %t, %one : i32
    %sn = arith.addi %s, %one : i32
    cf.br ^header(%tn, %sn : i32, i32)
  ^exit:
    return %s : i32
  }

  func.func @nneg_positive(%n: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    %raw = arith.constant 7 : i8
    %cast = arith.extui %raw nneg : i8 to i32
    cf.br ^header(%zero, %cast : i32, i32)
  ^header(%t: i32, %s: i32):
    %more = arith.cmpi ult, %t, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %tn = arith.addi %t, %one : i32
    %sn = arith.addi %s, %one : i32
    cf.br ^header(%tn, %sn : i32, i32)
  ^exit:
    return %s : i32
  }

  func.func @nneg_signed_max(%n: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    %raw = arith.constant 127 : i8
    %cast = arith.extui %raw nneg : i8 to i32
    cf.br ^header(%zero, %cast : i32, i32)
  ^header(%t: i32, %s: i32):
    %more = arith.cmpi ult, %t, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %tn = arith.addi %t, %one : i32
    %sn = arith.addi %s, %one : i32
    cf.br ^header(%tn, %sn : i32, i32)
  ^exit:
    return %s : i32
  }

  func.func @nneg_signed_min(%n: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    %raw = arith.constant -128 : i8
    %cast = arith.extui %raw nneg : i8 to i32
    cf.br ^header(%zero, %cast : i32, i32)
  ^header(%t: i32, %s: i32):
    %more = arith.cmpi ult, %t, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %tn = arith.addi %t, %one : i32
    %sn = arith.addi %s, %one : i32
    cf.br ^header(%tn, %sn : i32, i32)
  ^exit:
    return %s : i32
  }

  func.func @nneg_minus_one(%n: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    %raw = arith.constant -1 : i8
    %cast = arith.extui %raw nneg : i8 to i32
    cf.br ^header(%zero, %cast : i32, i32)
  ^header(%t: i32, %s: i32):
    %more = arith.cmpi ult, %t, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %tn = arith.addi %t, %one : i32
    %sn = arith.addi %s, %one : i32
    cf.br ^header(%tn, %sn : i32, i32)
  ^exit:
    return %s : i32
  }

  func.func @nneg_unsigned_max(%n: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    %raw = arith.constant 255 : i8
    %cast = arith.extui %raw nneg : i8 to i32
    cf.br ^header(%zero, %cast : i32, i32)
  ^header(%t: i32, %s: i32):
    %more = arith.cmpi ult, %t, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %tn = arith.addi %t, %one : i32
    %sn = arith.addi %s, %one : i32
    cf.br ^header(%tn, %sn : i32, i32)
  ^exit:
    return %s : i32
  }

  func.func @nneg_i1_zero(%n: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    %raw = arith.constant 0 : i1
    %cast = arith.extui %raw nneg : i1 to i32
    cf.br ^header(%zero, %cast : i32, i32)
  ^header(%t: i32, %s: i32):
    %more = arith.cmpi ult, %t, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %tn = arith.addi %t, %one : i32
    %sn = arith.addi %s, %one : i32
    cf.br ^header(%tn, %sn : i32, i32)
  ^exit:
    return %s : i32
  }

  func.func @nneg_i1_one(%n: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    %raw = arith.constant 1 : i1
    %cast = arith.extui %raw nneg : i1 to i32
    cf.br ^header(%zero, %cast : i32, i32)
  ^header(%t: i32, %s: i32):
    %more = arith.cmpi ult, %t, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %tn = arith.addi %t, %one : i32
    %sn = arith.addi %s, %one : i32
    cf.br ^header(%tn, %sn : i32, i32)
  ^exit:
    return %s : i32
  }

  func.func @nneg_wide_max(%n: i128) -> i128 {
    %zero = arith.constant 0 : i128
    %one = arith.constant 1 : i128
    %raw = arith.constant 18446744073709551615 : i65
    %cast = arith.extui %raw nneg : i65 to i128
    cf.br ^header(%zero, %cast : i128, i128)
  ^header(%t: i128, %s: i128):
    %more = arith.cmpi ult, %t, %n : i128
    cf.cond_br %more, ^body, ^exit
  ^body:
    %tn = arith.addi %t, %one : i128
    %sn = arith.addi %s, %one : i128
    cf.br ^header(%tn, %sn : i128, i128)
  ^exit:
    return %s : i128
  }

  func.func @nneg_wide_min(%n: i128) -> i128 {
    %zero = arith.constant 0 : i128
    %one = arith.constant 1 : i128
    %raw = arith.constant 18446744073709551616 : i65
    %cast = arith.extui %raw nneg : i65 to i128
    cf.br ^header(%zero, %cast : i128, i128)
  ^header(%t: i128, %s: i128):
    %more = arith.cmpi ult, %t, %n : i128
    cf.cond_br %more, ^body, ^exit
  ^body:
    %tn = arith.addi %t, %one : i128
    %sn = arith.addi %s, %one : i128
    cf.br ^header(%tn, %sn : i128, i128)
  ^exit:
    return %s : i128
  }

  func.func @plain_backedge(%n: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    cf.br ^header(%zero, %zero : i32, i32)
  ^header(%t: i32, %s: i32):
    %more = arith.cmpi ult, %t, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %raw = arith.constant -1 : i8
    %cast = arith.extui %raw : i8 to i32
    %tn = arith.addi %t, %one : i32
    %sn = arith.addi %s, %cast : i32
    cf.br ^header(%tn, %sn : i32, i32)
  ^exit:
    return %s : i32
  }

  func.func @nneg_backedge_positive(%n: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    cf.br ^header(%zero, %zero : i32, i32)
  ^header(%t: i32, %s: i32):
    %more = arith.cmpi ult, %t, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %raw = arith.constant 127 : i8
    %cast = arith.extui %raw nneg : i8 to i32
    %tn = arith.addi %t, %one : i32
    %sn = arith.addi %s, %cast : i32
    cf.br ^header(%tn, %sn : i32, i32)
  ^exit:
    return %s : i32
  }

  func.func @nneg_backedge_negative(%n: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    cf.br ^header(%zero, %zero : i32, i32)
  ^header(%t: i32, %s: i32):
    %more = arith.cmpi ult, %t, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %raw = arith.constant -128 : i8
    %cast = arith.extui %raw nneg : i8 to i32
    %tn = arith.addi %t, %one : i32
    %sn = arith.addi %s, %cast : i32
    cf.br ^header(%tn, %sn : i32, i32)
  ^exit:
    return %s : i32
  }

  func.func @nneg_chain_negative(%n: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    %raw = arith.constant -1 : i8
    %wide = arith.extui %raw nneg : i8 to i64
    %cast = arith.trunci %wide : i64 to i32
    cf.br ^header(%zero, %cast : i32, i32)
  ^header(%t: i32, %s: i32):
    %more = arith.cmpi ult, %t, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %tn = arith.addi %t, %one : i32
    %sn = arith.addi %s, %one : i32
    cf.br ^header(%tn, %sn : i32, i32)
  ^exit:
    return %s : i32
  }

  func.func @nneg_truncated_sign(%n: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    %wide = arith.constant 128 : i16
    %raw = arith.trunci %wide : i16 to i8
    %cast = arith.extui %raw nneg : i8 to i32
    cf.br ^header(%zero, %cast : i32, i32)
  ^header(%t: i32, %s: i32):
    %more = arith.cmpi ult, %t, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %tn = arith.addi %t, %one : i32
    %sn = arith.addi %s, %one : i32
    cf.br ^header(%tn, %sn : i32, i32)
  ^exit:
    return %s : i32
  }

  func.func @nneg_extui_chain_nonnegative(%n: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    %raw = arith.constant -1 : i8
    %wide = arith.extui %raw : i8 to i16
    %cast = arith.extui %wide nneg : i16 to i32
    cf.br ^header(%zero, %cast : i32, i32)
  ^header(%t: i32, %s: i32):
    %more = arith.cmpi ult, %t, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %tn = arith.addi %t, %one : i32
    %sn = arith.addi %s, %one : i32
    cf.br ^header(%tn, %sn : i32, i32)
  ^exit:
    return %s : i32
  }

  func.func @nneg_extsi_chain_negative(%n: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    %raw = arith.constant -1 : i8
    %wide = arith.extsi %raw : i8 to i16
    %cast = arith.extui %wide nneg : i16 to i32
    cf.br ^header(%zero, %cast : i32, i32)
  ^header(%t: i32, %s: i32):
    %more = arith.cmpi ult, %t, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %tn = arith.addi %t, %one : i32
    %sn = arith.addi %s, %one : i32
    cf.br ^header(%tn, %sn : i32, i32)
  ^exit:
    return %s : i32
  }

  func.func @nneg_dynamic_initial(%n: i32, %source: i8) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    %cast = arith.extui %source nneg : i8 to i32
    cf.br ^header(%zero, %cast : i32, i32)
  ^header(%t: i32, %s: i32):
    %more = arith.cmpi ult, %t, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %tn = arith.addi %t, %one : i32
    %sn = arith.addi %s, %one : i32
    cf.br ^header(%tn, %sn : i32, i32)
  ^exit:
    return %s : i32
  }

  func.func @nneg_dynamic_backedge(%n: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    cf.br ^header(%zero, %zero : i32, i32)
  ^header(%t: i32, %s: i32):
    %more = arith.cmpi ult, %t, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %raw = arith.trunci %t : i32 to i8
    %cast = arith.extui %raw nneg : i8 to i32
    %tn = arith.addi %t, %one : i32
    %sn = arith.addi %s, %cast : i32
    cf.br ^header(%tn, %sn : i32, i32)
  ^exit:
    return %s : i32
  }

  func.func @nneg_dead(%n: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    cf.br ^header(%zero, %zero : i32, i32)
  ^header(%t: i32, %s: i32):
    %more = arith.cmpi ult, %t, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %raw = arith.constant -1 : i8
    %unused = arith.extui %raw nneg : i8 to i32
    %tn = arith.addi %t, %one : i32
    %sn = arith.addi %s, %one : i32
    cf.br ^header(%tn, %sn : i32, i32)
  ^exit:
    return %s : i32
  }

  func.func @nneg_canceled_backedge(%n: i32) -> i32 {
    %zero = arith.constant 0 : i32
    %one = arith.constant 1 : i32
    cf.br ^header(%zero, %zero : i32, i32)
  ^header(%t: i32, %s: i32):
    %more = arith.cmpi ult, %t, %n : i32
    cf.cond_br %more, ^body, ^exit
  ^body:
    %raw = arith.constant -1 : i8
    %cast = arith.extui %raw nneg : i8 to i32
    %cancel = arith.subi %cast, %cast : i32
    %tn = arith.addi %t, %one : i32
    %sn = arith.addi %s, %cancel : i32
    cf.br ^header(%tn, %sn : i32, i32)
  ^exit:
    return %s : i32
  }
}
