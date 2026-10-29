// RUN: %mlir-opt %inputs/accepted.mlir -o %t.before
// RUN: %cfg-report %inputs/accepted.mlir --print-ir | %mlir-opt -o %t.after
// RUN: diff %t.before %t.after
// RUN: %cfg-report %inputs/accepted.mlir > %t.first
// RUN: %cfg-report %inputs/accepted.mlir > %t.second
// RUN: diff %t.first %t.second
// RUN: %FileCheck %s < %t.first
// CHECK: "deterministic":true
// CHECK-SAME: "read_only":true
