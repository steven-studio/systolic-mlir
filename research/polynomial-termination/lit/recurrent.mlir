// RUN: %report %inputs/loops.mlir --function=always_true | %FileCheck %s
// RUN: %report %inputs/loops.mlir --function=inclusive --parameter-range=1:127:127 | %FileCheck %s
// CHECK: "kind":"recurrent-guard"
// CHECK-SAME: "recurrent_set":"I AND G"
// CHECK-SAME: "status":"PROVEN_NONTERMINATING"
