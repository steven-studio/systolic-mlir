// RUN: %mlir-opt %inputs/accepted.mlir --verify-each -o /dev/null
// RUN: %report %inputs/accepted.mlir --symbolic --function=shifted_square --evaluate=3 --parameter-value=p1:7 --parameter-value=p2:-2 | %FileCheck %s
// CHECK: "initial":"p2"
// CHECK-SAME: "initial":"p1"
// CHECK-SAME: "powers":{"p2":2}
// CHECK-SAME: "status":"PROVEN"
// CHECK-SAME: "values":["3","1","12"]
