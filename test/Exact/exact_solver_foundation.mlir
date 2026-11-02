// The foundation of the project-owned exact solver. This is a .mlir file only
// because lit collects those; there is no IR in it.
//
// RUN: exact-solver-test | FileCheck %s --check-prefix=UNIT
// UNIT: exact solver foundation: {{[0-9]+}} passed, 0 failed
//
// Arithmetic against Python's integers, generated here so that it cannot drift.
// RUN: %python %S/Inputs/arithmetic_vectors.py > %t.vectors
// RUN: exact-solver-test --arithmetic-vectors %t.vectors \
// RUN:   | FileCheck %s --check-prefix=VECTORS
// VECTORS: arithmetic vectors: {{[0-9]+}} cases, {{[0-9]+}} passed, 0 failed
//
// RUN: %python %S/Inputs/check_exact_independence.py --source-root %S/../.. \
// RUN:   --test-binary exact-solver-test --production-binary systolic-opt \
// RUN:   | FileCheck %s --check-prefix=INDEPENDENT
// INDEPENDENT: sources: {{[0-9]+}} files, no forbidden dependency
// INDEPENDENT: archive: {{[0-9]+}} undefined symbols, none forbidden
// INDEPENDENT: checker: {{[0-9]+}} calls into the solver, all Int helpers or its counter
// INDEPENDENT: test binary: no LLVM solver code linked
// INDEPENDENT: production: solver not linked, LLVM Simplex still in use
// INDEPENDENT: provenance: declaration and runtime evidence verified
//
// Provenance from repositories built in a known state, including every case
// that must report UNKNOWN.
// RUN: rm -rf %t.identity
// RUN: %python %S/Inputs/check_identity_generator.py \
// RUN:   --generator %S/../../lib/Systolic/Exact/GenerateIdentity.cmake \
// RUN:   --work %t.identity | FileCheck %s --check-prefix=IDENTITY
// IDENTITY: identity generator: {{[0-9]+}} cases verified
