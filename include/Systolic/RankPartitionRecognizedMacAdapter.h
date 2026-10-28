#pragma once

#include "Systolic/RankPartitionGuardedMacEmission.h"

#include "mlir/Analysis/Presburger/IntegerRelation.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Operation.h"

#include "llvm/ADT/StringRef.h"

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace systolic::logical {

// Phase 5.4.3d-3b-4:
//
// Recognized GEMM to guarded MAC.
//
// Takes a loop nest the symbolic recognizer
// (mlir::systolic::analyzeSymbolicRankPartition) accepts and
// emits, from the information that recognizer verified, the
// complete guarded statement of Phases 1 to 3b-3 into a new,
// private, never-called function of the same module (the
// "twin"):
//
//   func.func private @<twinName>(%A, %B, %C) {
//     affine.for z ... {                  exact Presburger box
//       scf.if <exact guard> {            Phase 2b
//         x = M*z/d                       Phase 3a-2
//         f_A(x), f_B(x), f_C(x)          Phase 3b-1a
//         memref.load A, B, C             Phase 3b-2
//         mul, add                        Phase 3b-3
//         memref.store C                  Phase 3b-2
//       }
//     }
//   }
//
// The source nest is never modified or replaced. The twin is
// private and unreferenced, so it cannot execute as part of
// the program; the unit tests execute it, and the source,
// separately.
//
// Information used, and where it comes from (nothing is
// re-derived from the IR here):
//
//   - the source loops outermost first (the iteration order),
//     the three loads, the store, the multiplication and the
//     addition: the recognized statement;
//   - the A/B/C access rows and offsets over the loop
//     universe: the recognizer's linear forms;
//   - the coordinate map z = T*x and its exact inverse: the
//     rank partition;
//   - the exact source domain over the loops: the recognizer's
//     index set.
//
// Decided here from those objects:
//
//   - operand order of the multiplication and the addition,
//     and their attributes (fast-math, rounding mode): they
//     become GuardedMacSemantics; anything but the strict
//     defaults is UNKNOWN;
//   - memory disjointness of A and B from C, with the rule of
//     the production lowering (provablyDisjointFromOutput
//     without its assume-no-alias option): identical values or
//     identical view roots are refused; different roots are
//     proven disjoint only when one is a fresh
//     memref.alloc/alloca or a function argument marked
//     llvm.noalias; otherwise UNKNOWN. No aliasing assumption
//     beyond that inherited rule is introduced (the rule reads
//     a fresh allocation as a buffer no other value names,
//     which is what the production lowering relies on; an
//     allocation whose memref value escapes through memory is
//     not distinguished by it);
//   - a 64-bit index type (the emitted addresses are i64 cast
//     to index) and a source function that is a direct child
//     of its module; otherwise UNKNOWN;
//   - the exact logical domain (Phase 3a-2a normalization) and
//     a finite candidate box from the constant bounds of the
//     source loops; a domain with existential locals, or whose
//     image needs more than one congruence local, is UNKNOWN.
//
// Every refusal leaves the module exactly as it was: the twin
// function is created last and erased on any failure.
//
// Result status in `why`: "UNKNOWN: ..." when a required
// property could not be established, "REFUSED: ..." when an
// emitter refused. No UNKNOWN is ever reported as proven.

struct GuardedMacTwin {
  mlir::func::FuncOp function; // private, never called

  PartitionCoordinateMap coordinateMap;
  mlir::presburger::IntegerPolyhedron sourceDomain;
  SymbolicIterationPlan plan;

  EmittedGuardedAffineLoopNest nest;
  EmittedSourceIVRecovery recovery;
  std::array<GuardedMemoryOperand, 3> operands; // A, B, C
  EmittedGuardedMemoryLoads loads;
  EmittedGuardedMultiplyAccumulate mac;
  GuardedMacSemantics semantics;

  GuardedMacTwin(PartitionCoordinateMap map,
                 mlir::presburger::IntegerPolyhedron domain,
                 SymbolicIterationPlan iterationPlan)
      : coordinateMap(std::move(map)), sourceDomain(std::move(domain)),
        plan(std::move(iterationPlan)) {}
};

// `anchor` is any affine.for of the source nest (the
// recognizer walks to the outermost loop). `twinName` must
// not already be a symbol of the module.
std::optional<GuardedMacTwin>
emitGuardedMacTwin(mlir::OpBuilder &builder, mlir::Operation *anchor,
                   llvm::StringRef twinName, std::string &why);

} // namespace systolic::logical
