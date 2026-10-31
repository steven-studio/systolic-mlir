// RUN: %report %inputs/loops.mlir --function=step_two --parameter-range=1:127:127 | %FileCheck %s
// RUN: %report %inputs/loops.mlir --function=square_guard | %FileCheck %s
// RUN: %report %inputs/loops.mlir --function=collatz | %FileCheck %s
// RUN: %report %inputs/loops.mlir --function=multiphase | %FileCheck %s
// RUN: %report %inputs/loops.mlir --function=lexicographic | %FileCheck %s
// CHECK: "certificate":null
// CHECK-SAME: "status":"UNKNOWN"
