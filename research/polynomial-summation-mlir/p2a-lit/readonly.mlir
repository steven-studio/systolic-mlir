// RUN: %mlir-opt %inputs/accepted.mlir -o %t.before
// RUN: %report %inputs/accepted.mlir --symbolic --print-ir > %t.after
// RUN: diff %t.before %t.after
// RUN: %report %inputs/accepted.mlir --symbolic > %t.first
// RUN: %report %inputs/accepted.mlir --symbolic > %t.second
// RUN: diff %t.first %t.second
