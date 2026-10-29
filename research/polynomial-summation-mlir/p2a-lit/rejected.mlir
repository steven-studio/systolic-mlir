// RUN: %mlir-opt %inputs/rejected.mlir --verify-each -o /dev/null
// RUN: %report %inputs/rejected.mlir --symbolic --function=symbolic_nneg | %FileCheck %s
// RUN: %report %inputs/rejected.mlir --symbolic --function=poison_constant --mode=signed --horizon=5 --assume-no-overflow=explicit | %FileCheck %s
// RUN: %report %inputs/rejected.mlir --symbolic --function=parameter_degree | %FileCheck %s
// CHECK: "parameters":[]
// CHECK-SAME: "states":[],"status":"UNKNOWN"
