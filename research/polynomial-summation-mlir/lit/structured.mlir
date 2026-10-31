// The conversion is an external MLIR pass; P1 itself never rewrites this IR.
// RUN: %mlir-opt %inputs/structured.mlir --verify-each -o /dev/null
// RUN: %cfg-report %inputs/structured.mlir | %FileCheck %s --check-prefix=RAW
// RUN: %mlir-opt %inputs/structured.mlir --convert-scf-to-cf --verify-each -o %t.mlir
// RUN: %cfg-report %t.mlir --evaluate=10 | %FileCheck %s --check-prefix=CFG
// RAW: "states":[],"status":"UNKNOWN"
// CFG: "status":"PROVEN"
// CFG-SAME: "values":["10","285"]
