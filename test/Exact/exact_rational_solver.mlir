// The project-owned rational solver. This is a .mlir file only because lit
// collects those; there is no IR in it.
//
// RUN: exact-rational-test | FileCheck %s --check-prefix=UNIT
// UNIT: exact rational solver: {{[0-9]+}} passed, 0 failed
//
// Fourier-Motzkin on Python integers, generated here so that it cannot drift.
// RUN: %python %S/Inputs/rational_vectors.py > %t.vectors
// RUN: exact-rational-test --rational-vectors %t.vectors \
// RUN:   | FileCheck %s --check-prefix=VECTORS
// VECTORS: rational vectors checks: {{[0-9]+}} passed, 0 failed
//
// RUN: %python %S/Inputs/check_rational_solver.py --source-root %S/../.. \
// RUN:   --test-binary exact-rational-test --production-binary systolic-opt \
// RUN:   | FileCheck %s --check-prefix=SOLVER
// SOLVER: sources: the checker reads no search code, and one file sets verdicts
// SOLVER: test binary: the rational solver is linked, and no LLVM solver code
// SOLVER: production: rational solver not linked, LLVM Simplex still in use
// SOLVER: provenance: a rational query ran the tableau and the checker
