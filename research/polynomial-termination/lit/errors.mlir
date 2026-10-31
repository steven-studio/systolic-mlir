// RUN: %report %inputs/loops.mlir --function=inc_s --solver-resource-limit=1 | %FileCheck %s
// RUN: %report %inputs/loops.mlir --function=inc_s --max-queries=1 | %FileCheck %s
// RUN: %not %report %s | %FileCheck %s
// CHECK: "certificate":null
// CHECK-SAME: "status":"UNKNOWN"
module {
  func.func @invalid() -> i8 {
    return %undefined : i8
  }
}
