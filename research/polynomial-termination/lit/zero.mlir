// RUN: %report %inputs/loops.mlir --function=always_false | %FileCheck %s
// RUN: %report %inputs/loops.mlir --function=stutter --parameter-range=0:1:1 --parameter-range=1:1:1 | %FileCheck %s
// CHECK: "kind":"zero-iterations"
// CHECK-SAME: "status":"PROVEN_TERMINATING"
