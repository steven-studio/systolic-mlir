// RUN: %mlir-opt %inputs/rejected.mlir --verify-each -o /dev/null
// RUN: %cfg-report %inputs/rejected.mlir --function=multiple_backedges | %FileCheck %s --check-prefix=BACKEDGE
// RUN: %cfg-report %inputs/rejected.mlir --function=irreducible | %FileCheck %s --check-prefix=IRREDUCIBLE
// RUN: %cfg-report %inputs/rejected.mlir --function=memory_dependency | %FileCheck %s --check-prefix=MEMORY
// RUN: %cfg-report %inputs/rejected.mlir --function=dynamic_cast | %FileCheck %s --check-prefix=CAST
// BACKEDGE: "reason":"multiple backedges are unsupported"
// BACKEDGE-SAME: "states":[],"status":"UNKNOWN"
// IRREDUCIBLE: irreducible/multiple/nested cycles unsupported
// IRREDUCIBLE-SAME: "states":[],"status":"UNKNOWN"
// MEMORY: unknown or nonempty memory effects: memref.load
// MEMORY-SAME: "states":[],"status":"UNKNOWN"
// CAST: initial value or cast depends on loop state
// CAST-SAME: "states":[],"status":"UNKNOWN"
