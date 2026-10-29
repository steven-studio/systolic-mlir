// RUN: %mlir-opt %inputs/overflow.mlir --verify-each -o /dev/null
// RUN: %cfg-report %inputs/overflow.mlir --function=canceled_intermediate | %FileCheck %s --check-prefix=MOD
// RUN: %cfg-report %inputs/overflow.mlir --function=canceled_intermediate --mode=signed --horizon=10 | %FileCheck %s --check-prefix=SIGNED
// RUN: %cfg-report %inputs/overflow.mlir --function=loose_but_safe --mode=signed --horizon=20 --assume-no-overflow='every original intermediate fits signed i8' | %FileCheck %s --check-prefix=ASSUMED
// RUN: %cfg-report %inputs/overflow.mlir --function=signed_flag --mode=mathematical --horizon=20 | %FileCheck %s --check-prefix=PROVED
// MOD: "mode":"modular"
// MOD-SAME: "status":"PROVEN"
// SIGNED: no-overflow proof inconclusive
// SIGNED-SAME: "states":[],"status":"UNKNOWN"
// ASSUMED: "assumption":"every original intermediate fits signed i8"
// ASSUMED-SAME: "status":"CONDITIONAL"
// PROVED: "mode":"mathematical"
// PROVED-SAME: "status":"PROVEN"
