// RUN: %not %mlir-opt %inputs/invalid_branch.mlir -o /dev/null 2>&1 | %FileCheck %s --check-prefix=BRANCH
// RUN: %not %mlir-opt %inputs/invalid_dominance.mlir -o /dev/null 2>&1 | %FileCheck %s --check-prefix=DOM
// RUN: %not %mlir-opt %inputs/invalid_duplicate.mlir -o /dev/null 2>&1 | %FileCheck %s --check-prefix=SSA
// RUN: %not %cfg-report %inputs/invalid_branch.mlir 2>&1 | %FileCheck %s --check-prefix=UNKNOWN
// RUN: %not %cfg-report %inputs/invalid_dominance.mlir 2>&1 | %FileCheck %s --check-prefix=UNKNOWN
// RUN: %not %cfg-report %inputs/invalid_duplicate.mlir 2>&1 | %FileCheck %s --check-prefix=UNKNOWN
// BRANCH: type mismatch for bb argument
// DOM: does not dominate this use
// SSA: redefinition of SSA value
// UNKNOWN: {"status":"UNKNOWN","reason":"MLIR parsing/verification failed","states":[]}
