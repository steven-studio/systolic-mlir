// RUN: %mlir-opt %inputs/extui-nneg.mlir --verify-each -o /dev/null
// RUN: %cfg-report %inputs/extui-nneg.mlir --function=plain_negative --evaluate=0 | %FileCheck %s --check-prefix=PLAIN
// RUN: %cfg-report %inputs/extui-nneg.mlir --function=nneg_signed_max --evaluate=0 | %FileCheck %s --check-prefix=POSITIVE
// RUN: %cfg-report %inputs/extui-nneg.mlir --function=nneg_minus_one | %FileCheck %s --check-prefix=UNKNOWN
// RUN: %cfg-report %inputs/extui-nneg.mlir --function=nneg_minus_one --mode=signed --horizon=10 | %FileCheck %s --check-prefix=UNKNOWN
// RUN: %cfg-report %inputs/extui-nneg.mlir --function=nneg_minus_one --mode=mathematical --horizon=10 | %FileCheck %s --check-prefix=UNKNOWN
// RUN: %cfg-report %inputs/extui-nneg.mlir --function=nneg_backedge_negative | %FileCheck %s --check-prefix=UNKNOWN
// RUN: %cfg-report %inputs/extui-nneg.mlir --function=nneg_truncated_sign | %FileCheck %s --check-prefix=UNKNOWN
// RUN: %cfg-report %inputs/extui-nneg.mlir --function=nneg_dead | %FileCheck %s --check-prefix=UNKNOWN
// PLAIN: "status":"PROVEN"
// PLAIN-SAME: "values":["0","255"]
// POSITIVE: "status":"PROVEN"
// POSITIVE-SAME: "values":["0","127"]
// UNKNOWN: "reason":"arith.extui nneg precondition violated at source width"
// UNKNOWN-SAME: "states":[],"status":"UNKNOWN"
