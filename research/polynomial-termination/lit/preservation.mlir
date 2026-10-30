// RUN: %mlir-opt %inputs/loops.mlir > %t.before
// RUN: %report %inputs/loops.mlir --function=forwarded --print-ir > %t.after
// RUN: diff %t.before %t.after
// RUN: %report %inputs/loops.mlir --function=forwarded > %t.first
// RUN: %report %inputs/loops.mlir --function=forwarded > %t.second
// RUN: diff %t.first %t.second
