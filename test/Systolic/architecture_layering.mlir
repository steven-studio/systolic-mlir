// The layering of the records mode, kept by a check (Inputs/check_layering.py, which lists
// what is forbidden and why). lib/Systolic/Transforms/GeneralizedMatmulToSystolic.cpp is the
// high-level file: which nest goes which way, and in which order a GEMM's steps are emitted.
// It must not depend on the header of the partition into boxes, nor on the headers of the
// memref and arith operations, whether it includes them itself or a header it includes does:
// how a step is emitted is behind Systolic/MatmulTileLowering.h.
//
// The first command checks the tree and this build. The others check the checker, on small
// trees under Inputs/layering that each break the rule one way, and on dependency lists as
// `ninja -t deps` prints them.

// The tree: the file's #include lines, followed through every header of the project it
// includes, and the dependency list this build recorded for its object. A build whose list
// cannot be read is not passed.
// RUN: %python %S/Inputs/check_layering.py --source-root %S/../.. --find-build-root-from %t --require-deps | FileCheck %s --check-prefix=TREE
// TREE: lib/Systolic/Transforms/GeneralizedMatmulToSystolic.cpp: its includes and the compiler's dependency list hold none of the 3 forbidden headers

// A tree that keeps the rule. The header that is to be forbidden later is said, not refused,
// and the other object of the list may depend on what this one may not.
// RUN: %python %S/Inputs/check_layering.py --source-root %S/Inputs/layering/clean --deps-file %S/Inputs/layering/deps_clean.txt --require-deps | FileCheck %s --check-prefix=CLEAN
// CLEAN-NOT: ARCHITECTURE VIOLATION
// CLEAN:     not forbidden yet: lib/Systolic/Transforms/GeneralizedMatmulToSystolic.cpp still depends on mlir/Dialect/Affine/IR/AffineOps.h
// CLEAN:     its includes and the compiler's dependency list hold none of the 3 forbidden headers

// The file includes a forbidden header itself.
// RUN: not %python %S/Inputs/check_layering.py --source-root %S/Inputs/layering/direct | FileCheck %s --check-prefix=DIRECT
// DIRECT:      ARCHITECTURE VIOLATION: lib/Systolic/Transforms/GeneralizedMatmulToSystolic.cpp (the high-level dispatch file of the records mode) depends on mlir/Dialect/MemRef/IR/MemRef.h
// DIRECT-NEXT: through its own #include: lib/Systolic/Transforms/GeneralizedMatmulToSystolic.cpp:8 includes mlir/Dialect/MemRef/IR/MemRef.h
// DIRECT-NEXT: forbidden because it brings memref operations
// DIRECT-NEXT: The high-level dispatch file must not include low-level IR or partitioning headers, itself
// DIRECT-NEXT: or through a header it includes. Put the detail behind MatmulTileLowering
// DIRECT:      高層調度檔不應直接包含底層 IR/Partitioning 標頭，請將細節封裝進 `MatmulTileLowering`。
// DIRECT-NOT:  hold none of the

// Headers the file includes bring them: one a hop away, one two, each chain said hop by
// hop with its file and line.
// RUN: not %python %S/Inputs/check_layering.py --source-root %S/Inputs/layering/transitive | FileCheck %s --check-prefix=THROUGH
// THROUGH:      ARCHITECTURE VIOLATION: lib/Systolic/Transforms/GeneralizedMatmulToSystolic.cpp (the high-level dispatch file of the records mode) depends on Systolic/AffineMatmulPartitioning.h
// THROUGH-NEXT: through a header it includes: lib/Systolic/Transforms/GeneralizedMatmulToSystolic.cpp:4 includes Systolic/MatmulTileLowering.h; include/Systolic/MatmulTileLowering.h:3 includes Systolic/AffineMatmulPartitioning.h
// THROUGH:      ARCHITECTURE VIOLATION: lib/Systolic/Transforms/GeneralizedMatmulToSystolic.cpp (the high-level dispatch file of the records mode) depends on mlir/Dialect/Arith/IR/Arith.h
// THROUGH-NEXT: through a header it includes: lib/Systolic/Transforms/GeneralizedMatmulToSystolic.cpp:5 includes Systolic/NestRecordLowering.h; include/Systolic/NestRecordLowering.h:3 includes Systolic/NestRecordGate.h; include/Systolic/NestRecordGate.h:4 includes mlir/Dialect/Arith/IR/Arith.h
// THROUGH:      高層調度檔不應直接包含底層 IR/Partitioning 標頭，請將細節封裝進 `MatmulTileLowering`。

// No #include of the project names it and the compiler opened it all the same, through a
// header of MLIR: only the build's list shows that, and it is refused as well.
// RUN: not %python %S/Inputs/check_layering.py --source-root %S/Inputs/layering/clean --deps-file %S/Inputs/layering/deps_memref.txt | FileCheck %s --check-prefix=COMPILER
// COMPILER:      ARCHITECTURE VIOLATION: lib/Systolic/Transforms/GeneralizedMatmulToSystolic.cpp (the high-level dispatch file of the records mode) depends on mlir/Dialect/MemRef/IR/MemRef.h
// COMPILER-NEXT: the compiler opened it for this file's object, though no #include of the file or of this project's headers names it
// COMPILER:      高層調度檔不應直接包含底層 IR/Partitioning 標頭，請將細節封裝進 `MatmulTileLowering`。

// The header that is to be forbidden later no longer reaches the file: the check fails
// once, to have its entry moved, so that what was gained is kept.
// RUN: not %python %S/Inputs/check_layering.py --source-root %S/Inputs/layering/tightened --deps-file %S/Inputs/layering/deps_no_affine.txt | FileCheck %s --check-prefix=TIGHTEN
// TIGHTEN-NOT: ARCHITECTURE VIOLATION
// TIGHTEN:     RULE TO TIGHTEN: lib/Systolic/Transforms/GeneralizedMatmulToSystolic.cpp no longer depends on mlir/Dialect/Affine/IR/AffineOps.h. Keep it so: move the entry from PLANNED to FORBIDDEN in test/Systolic/Inputs/check_layering.py.

// What cannot be checked is not passed: the file is not where the rule says (moved or
// renamed: the rule must follow it), or the build's list is asked for and is stale.
// RUN: not %python %S/Inputs/check_layering.py --source-root %S/Inputs/layering/nowhere | FileCheck %s --check-prefix=MOVED
// MOVED: CANNOT CHECK: lib/Systolic/Transforms/GeneralizedMatmulToSystolic.cpp is not under
// RUN: not %python %S/Inputs/check_layering.py --source-root %S/Inputs/layering/clean --deps-file %S/Inputs/layering/deps_stale.txt --require-deps | FileCheck %s --check-prefix=STALE
// STALE: CANNOT CHECK: the compiler's dependency list of lib/Systolic/Transforms/GeneralizedMatmulToSystolic.cpp cannot be read: the dependency record of {{.*}}GeneralizedMatmulToSystolic.cpp.o is stale
