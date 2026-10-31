// RUN: %report %s --index-bit-width=64 | FileCheck %s
// A refusal prints its reason and no partial model.
// CHECK: "function": "structured"
// CHECK-NEXT: "fingerprint"
// CHECK-NEXT: "index_bit_width": 64
// CHECK-NEXT: "status": "UNSUPPORTED"
// CHECK-NEXT: "reason": "NestedRegion"
// CHECK-NOT: "transitions"
func.func @structured(%c: i1) {
  scf.if %c {
  }
  return
}
