// The project-owned integer solver. This is a .mlir file only because lit
// collects those; there is no IR in it.
//
// RUN: exact-integer-test | FileCheck %s --check-prefix=UNIT
// UNIT: exact integer solver: {{[0-9]+}} passed, 0 failed
//
// Every point of a box, tried in Python, generated here so that it cannot drift.
// RUN: %python %S/Inputs/integer_vectors.py > %t.vectors
// RUN: exact-integer-test --integer-vectors %t.vectors \
// RUN:   | FileCheck %s --check-prefix=VECTORS
// VECTORS: integer vectors checks: {{[0-9]+}} passed, 0 failed
//
// RUN: %python %S/Inputs/check_integer_solver.py --source-root %S/../.. \
// RUN:   --test-binary exact-integer-test --production-binary systolic-opt \
// RUN:   | FileCheck %s --check-prefix=SOLVER
// SOLVER: sources: the checker reads no search code, and no floating point
// SOLVER: test binary: the integer search is linked, and no LLVM solver code
// SOLVER: production: integer search not linked, LLVM Simplex still in use
// SOLVER: provenance: an integer query ran the search, the tableau and the checker
