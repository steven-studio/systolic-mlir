// RUN: %report %inputs/loops.mlir --function=nsw | %FileCheck %s
// RUN: %report %inputs/loops.mlir --function=nuw | %FileCheck %s
// RUN: %report %inputs/loops.mlir --function=nneg | %FileCheck %s
// RUN: %report %inputs/loops.mlir --function=mustprogress | %FileCheck %s
// RUN: %report %inputs/loops.mlir --function=willreturn | %FileCheck %s
// RUN: %report %inputs/loops.mlir --function=memory | %FileCheck %s
// RUN: %report %inputs/loops.mlir --function=recursive_call | %FileCheck %s
// CHECK: "certificate":null
// CHECK-SAME: "status":"UNKNOWN"
