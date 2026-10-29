#pragma once

#include "Systolic/RankPartitionGuardedMacEmission.h"

#include "mlir/Analysis/Presburger/IntegerRelation.h"
#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/Operation.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace systolic::logical {

// Phase 5.4.3d-3b-4 / 3b-5:
//
// Recognized GEMM to guarded MAC.
//
// From a loop nest the symbolic recognizer
// (mlir::systolic::analyzeSymbolicRankPartition) accepts,
// the complete guarded statement of Phases 1 to 3b-3 is
// emitted from the information that recognizer verified:
//
//   affine.for z ... {                  exact Presburger box
//     scf.if <exact guard> {            Phase 2b
//       x = M*z/d                       Phase 3a-2
//       f_A(x), f_B(x), f_C(x)          Phase 3b-1a
//       memref.load A, B, C             Phase 3b-2
//       mul, add                        Phase 3b-3
//       memref.store C                  Phase 3b-2
//     }
//   }
//
// Three entry points share one preflight:
//
//   prepareGuardedMac        every decision, no IR;
//   emitGuardedMacTwin       Phase 3b-4: the statement in a
//                            new private, never-called
//                            function of the same module;
//   replaceGemmWithGuardedMac
//                            Phase 3b-5: the statement in
//                            the source nest's own position,
//                            on the source memrefs, after
//                            which the source nest is erased
//                            (transactional, see below).
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
//     the production lowering (provablyDisjointFromOutput):
//     identical values or identical view roots are INVALID;
//     different roots are proven disjoint when one is a fresh
//     memref.alloc/alloca or a function argument marked
//     llvm.noalias, or when the caller passes the production
//     assume-no-alias option; otherwise UNKNOWN. No aliasing
//     assumption beyond that inherited rule is introduced
//     (the rule reads a fresh allocation as a buffer no other
//     value names, which is what the production lowering
//     relies on; an allocation whose memref value escapes
//     through memory is not distinguished by it);
//   - the exact logical domain (Phase 3a-2a normalization) and
//     a finite candidate box from the constant bounds of the
//     source loops; a domain with existential locals, or whose
//     image needs more than one congruence local, is UNKNOWN;
//   - a 64-bit index type (the emitted addresses are i64 cast
//     to index); otherwise UNKNOWN.
//
// Outcomes: Proven (emitted), Unknown (a required property
// could not be established: the source stays), Invalid (the
// transformation is proven illegal: the source stays),
// Unsupported (an emitter refused: the source stays). No
// UNKNOWN is ever reported as proven.

enum class GuardedMacOutcome { Proven, Unknown, Invalid, Unsupported };

// Everything decided before any operation is created.
struct GuardedMacPreflight {
  mlir::affine::AffineForOp outermost; // the source nest's root
  std::array<mlir::Value, 3> memrefs;  // A, B, C of the source
  std::string diagnostic;              // the recognizer's report

  PartitionCoordinateMap coordinateMap;
  mlir::presburger::IntegerPolyhedron sourceDomain;
  SymbolicIterationPlan plan;

  std::array<std::vector<std::vector<BigInt>>, 3> accessRows;
  std::array<std::vector<BigInt>, 3> accessOffsets;
  GuardedMacSemantics semantics;

  // Phase 3b-6: the attributes of the source statement as written.
  // prepareGuardedMac accepts only the strict defaults (then these are
  // none / false); prepareRecognizedGemm records whatever is written, for
  // a caller with its own arithmetic contract (the device-job lowering,
  // whose permission is the accumulation's reassoc flag). The MAC
  // emitters refuse a preflight whose recorded attributes are not strict.
  mlir::arith::FastMathFlags multiplyFastMath =
      mlir::arith::FastMathFlags::none;
  mlir::arith::FastMathFlags addFastMath = mlir::arith::FastMathFlags::none;
  bool roundingModeAttributes = false;

  // Phase 3b-8: the plan's domain is a certified subset of the source
  // domain's image (the host remainder of a hybrid lowering, section 29);
  // the emitters require inclusion only (V3').
  bool partialDomain = false;

  GuardedMacPreflight(PartitionCoordinateMap map,
                      mlir::presburger::IntegerPolyhedron domain,
                      SymbolicIterationPlan iterationPlan)
      : coordinateMap(std::move(map)), sourceDomain(std::move(domain)),
        plan(std::move(iterationPlan)) {}
};

// `anchor` is any affine.for of the source nest (the
// recognizer walks to the outermost loop). On failure
// `outcome` is Unknown or Invalid and `why` starts with
// "UNKNOWN: " or "REFUSED: ".
std::optional<GuardedMacPreflight>
prepareGuardedMac(mlir::Operation *anchor, bool assumeNoAlias,
                  GuardedMacOutcome &outcome, std::string &why);

// Phase 3b-6: the same preflight without the strict-attribute decision.
// Every other decision is prepareGuardedMac's (recognition, statement
// shape, memory disjointness, exact domain, box, access functions); the
// statement's fast-math flags and rounding-mode attributes are recorded on
// the result instead of refused. Not for the MAC emitters: they mirror
// nothing but the strict defaults and refuse such a preflight.
std::optional<GuardedMacPreflight>
prepareRecognizedGemm(mlir::Operation *anchor, bool assumeNoAlias,
                      GuardedMacOutcome &outcome, std::string &why);

struct EmittedGuardedMacStatement {
  EmittedGuardedAffineLoopNest nest;
  EmittedSourceIVRecovery recovery;
  std::array<GuardedMemoryOperand, 3> operands; // A, B, C
  EmittedGuardedMemoryLoads loads;
  EmittedGuardedMultiplyAccumulate mac;
};

// Emit the statement at the builder's insertion point, on
// `memrefs` (A, B, C), which must dominate that point.
// Everything created is erased on failure (`why` then starts
// with "REFUSED: ").
std::optional<EmittedGuardedMacStatement>
emitGuardedMacStatement(mlir::OpBuilder &builder, mlir::Location loc,
                        const GuardedMacPreflight &preflight,
                        llvm::ArrayRef<mlir::Value> memrefs, std::string &why);

// Phase 3b-4: the twin function. The source nest is never
// modified. `twinName` must not already be a symbol of the
// module. Memory disjointness is decided without the
// assume-no-alias option.
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

std::optional<GuardedMacTwin>
emitGuardedMacTwin(mlir::OpBuilder &builder, mlir::Operation *anchor,
                   llvm::StringRef twinName, std::string &why);

// Phase 3b-5: transactional in-place replacement.
//
//   1. prepareGuardedMac: every legality decision, no IR;
//   2. the statement is emitted immediately BEFORE the source
//      nest, in its block, on the source memrefs (which
//      dominate that position), so the surrounding operations
//      and the original execution position are preserved; the
//      source nest has no results and no external uses (the
//      recognizer requires unit-step loops without iter_args);
//   3. the new nest is verified;
//   4. only then is the source nest erased.
//
// Any failure before step 4 erases what was created and
// leaves the source untouched; the program never contains
// both computations after this call returns.
struct GuardedMacReplacement {
  mlir::affine::AffineForOp outermost; // the new nest, in place
  EmittedGuardedMacStatement statement;
  std::string diagnostic;
};

std::optional<GuardedMacReplacement>
replaceGemmWithGuardedMac(mlir::OpBuilder &builder, mlir::Operation *anchor,
                          bool assumeNoAlias, GuardedMacOutcome &outcome,
                          std::string &why);

} // namespace systolic::logical
