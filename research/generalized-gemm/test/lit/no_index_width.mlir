// RUN: %report %s | FileCheck %s
// CHECK: "reason": "IndexWidthUnspecified"
func.func @f(%a: index) -> index {
  return %a : index
}
