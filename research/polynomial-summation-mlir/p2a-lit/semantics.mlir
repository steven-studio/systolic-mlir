// RUN: %mlir-opt %inputs/semantics.mlir --verify-each -o /dev/null
// RUN: %report %inputs/accepted.mlir --symbolic --function=shifted_square --mode=signed --horizon=5 --parameter-range=1:-2:2 --parameter-range=2:-2:2 | %FileCheck %s --check-prefix=PROVED
// RUN: %report %inputs/accepted.mlir --symbolic --function=shifted_square --mode=mathematical --horizon=5 | %FileCheck %s --check-prefix=UNKNOWN
// RUN: %report %inputs/accepted.mlir --symbolic --function=shifted_square --mode=signed --horizon=5 --assume-no-overflow=explicit | %FileCheck %s --check-prefix=ASSUMED
// RUN: %report %inputs/semantics.mlir --symbolic --function=flagged --mode=signed --horizon=5 --assume-no-overflow=explicit | %FileCheck %s --check-prefix=UNKNOWN
// RUN: %report %inputs/semantics.mlir --symbolic --function=flagged --mode=signed --horizon=5 --parameter-range=1:-2:2 --parameter-range=2:-2:2 | %FileCheck %s --check-prefix=PROVED
// PROVED: "status":"PROVEN"
// UNKNOWN: "states":[],"status":"UNKNOWN"
// ASSUMED: "assumption":"explicit"
// ASSUMED-SAME: "status":"CONDITIONAL"
