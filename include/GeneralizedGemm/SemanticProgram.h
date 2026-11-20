//===- SemanticProgram.h - Exact guarded transition system ------*- C++ -*-===//
//
// The G1 semantic model. A program is a finite transition system over
//
//   configuration = (location, state, poison shadow, memory)
//
// One Transition exists per CFG successor slot, so parallel edges stay
// distinct. Every term of a transition is over the configuration at ENTRY to
// the source block: assignments are therefore simultaneous (invariant I5).
// Operations inside a block execute in order; a Store chain in the memory term
// carries that order into later Load terms.
//
// The model owns all of its data and holds no MLIR pointer, so it is valid
// after the input IR is mutated or destroyed. It is a snapshot, not a view:
// matchesSource() tells whether it still describes a function.
//
// A dynamic instance of a MAC site is an event. Events form an ordered
// sequence, which is the semantic object: D is the index set of that sequence,
// so order and multiplicity are preserved.
//
//===----------------------------------------------------------------------===//
#ifndef GENERALIZEDGEMM_SEMANTICPROGRAM_H
#define GENERALIZEDGEMM_SEMANTICPROGRAM_H

#include "GeneralizedGemm/ProofStatus.h"
#include "GeneralizedGemm/SemanticTerm.h"

#include "llvm/ADT/SmallVector.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace mlir::generalized_gemm {

/// Every reason a model can be refused. Invariant I3: when set, the program
/// has no partial content.
enum class UnsupportedKind {
  NotVerified,
  ExternalFunction,
  NestedRegion,
  UnsupportedOperation,
  UnsupportedAttribute,
  UnsupportedType,
  UnsupportedTerminator,
  IndexWidthUnspecified,
  ConstantOutOfRange,
  NonIdentityLayout,
  NonDefaultMemorySpace,
  UnsupportedView,
  MemrefBlockArgument,
  MemrefReturn,
  AllocationInCycle,
  DynamicAllocation,
  DynamicDimensionIndex,
  RecursiveCall,
  ExternalCall,
  IndirectCall,
  CyclicCallee,
  LoweringFailed,
  ResourceLimit,
};
const char *stringifyUnsupportedKind(UnsupportedKind kind);

struct UnsupportedReason {
  UnsupportedKind kind;
  std::string detail;
};

struct Parameter {
  enum Kind { Scalar, Dimension };
  Kind kind = Scalar;
  unsigned id = 0;
  Sort sort;
  // Scalar: the function argument. Dimension: the root's argument and dim.
  unsigned argument = 0, dimension = 0;
};

struct StateVar {
  // IterationCounter: added by LoopCounters.h after import, never by the importer.
  // It counts the arrivals at `location` (a loop header) since the loop was last
  // entered; nothing in the program reads it (invariant of the instrumentation).
  enum Kind { BlockArgument, CrossBlockResult, IterationCounter };
  Kind kind = BlockArgument;
  unsigned id = 0;
  Sort sort;
  unsigned location = 0, position = 0; // Argument number, or op index.
  bool loopCarried = false;            // A block argument inside a CFG cycle.
};

struct MemoryRoot {
  enum Origin { FunctionArgument, FreshAllocation };
  Origin origin = FunctionArgument;
  unsigned id = 0;
  unsigned argument = 0;    // Function argument, for FunctionArgument.
  unsigned definingOp = 0;  // TermOp index, for FreshAllocation.
  bool restrictArgument = false; // llvm.noalias on the argument.
  Sort sort;                // Mem sort.
  // Per dimension: the static extent, or -1 and a Dimension parameter id.
  llvm::SmallVector<int64_t> extents;
  llvm::SmallVector<int> dimensionParameter;
};

struct RootPairStatus {
  unsigned first = 0, second = 0;
  ProofStatus separated;
};

enum class SideKind {
  UndefinedBehavior,  // The source program has no defined meaning here.
  Underspecified,     // More than one behaviour is allowed, or the operation's
                      // specification and lowering disagree.
  ModelLimit,         // Defined, but outside what G1 represents.
};
const char *stringifySideKind(SideKind kind);

/// `violated` true on entry to the block means the block does not execute
/// normally. Conditions are in operation order; the first true one is the
/// reported cause.
struct SideCondition {
  SideKind kind = SideKind::UndefinedBehavior;
  std::string reason;
  unsigned opIndex = 0;
  Term violated = nullptr;
};

struct MemoryEffect {
  enum Kind { Read, Write };
  Kind kind = Read;
  unsigned root = 0, opIndex = 0;
  llvm::SmallVector<Term, 3> index;
  Term value = nullptr; // Value read or written.
};

struct Transition {
  unsigned from = 0, slot = 0, to = 0;
  Term guard = nullptr;
  // Sparse simultaneous updates. Variables not listed keep their value.
  llvm::SmallVector<std::pair<unsigned, Term>> assignments;
  llvm::SmallVector<std::pair<unsigned, Term>> shadowAssignments;
  // Per root, sparse: the memory after the block.
  llvm::SmallVector<std::pair<unsigned, Term>> memoryUpdates;
  llvm::SmallVector<SideCondition, 0> sideConditions;
  llvm::SmallVector<MemoryEffect, 0> effects;
  // Only on a transition to the exit location.
  llvm::SmallVector<Term> returns, returnPoison;
};

enum class ReductionForm { Direct, ScalarAccumulator };
enum class ReductionOp { FAdd, Add };

struct MacAccess {
  unsigned root = 0;
  llvm::SmallVector<Term, 3> index;
};

/// A recorded occurrence of C[h] := C[h'] + A[f] * B[g], or of the scalar form
/// s := s + A[f] * B[g]. Recording is structural. It claims nothing about h
/// and h' being equal, about the operands being different, or about any GEMM.
struct MacSite {
  unsigned id = 0, location = 0, opIndex = 0;
  unsigned effectIndex = 0; // The event happens after this effect.
  ReductionForm form = ReductionForm::Direct;
  ReductionOp reduction = ReductionOp::FAdd;
  unsigned reductionFlags = 0, multiplyFlags = 0;
  MacAccess a, b;
  std::optional<MacAccess> output, outputRead;
  std::optional<unsigned> accumulator;
  Term aValue = nullptr, bValue = nullptr, accumulatedValue = nullptr;
  Term updateValue = nullptr;
  bool oblivious = false;
  llvm::SmallVector<unsigned> coordinates; // Default projection x of the state.
};

struct SemanticProgram {
  std::shared_ptr<TermContext> terms;
  std::string functionName, fingerprint;
  unsigned indexBitWidth = 0;
  bool structuredLoweringApplied = false;

  unsigned locationCount = 0;
  unsigned entryLocation = 0;
  unsigned exitLocation() const { return locationCount; }
  llvm::SmallVector<Parameter, 0> parameters;
  llvm::SmallVector<StateVar, 0> state;
  llvm::SmallVector<MemoryRoot, 0> roots;
  llvm::SmallVector<Transition, 0> transitions;
  llvm::SmallVector<Sort> returnSorts;
  llvm::SmallVector<RootPairStatus, 0> rootPairs;
  llvm::SmallVector<MacSite, 0> sites;

  // Slice results (invariant I4), computed, never assumed.
  bool controlOblivious = false;
  bool addressOblivious = false;
  // Experimental (exp/semgus-indirect-dp). Computed: every address that depends on memory
  // does so only through one level of gather, a Load from a root the program never writes,
  // at an index that reads no memory and no state that does: A[dp[i], k]. Such a load is a
  // function of its index for the whole run, as long as that root is separate from every
  // root written (a separation the claims then name). `gatherRoots` lists those roots.
  bool addressGather = false;
  llvm::SmallVector<unsigned, 2> gatherRoots;
  // Copied from ImportOptions::admitGather: the checkers take an addressGather program as
  // oblivious. Off by default, and then nothing changes.
  bool admitGather = false;

  // Named premises of this model. Never Proven (invariant I9).
  std::vector<ProofStatus> assumptions;

  // State ids of IterationCounter kind (LoopCounters.h), in header order; empty
  // for a freshly imported model.
  std::vector<unsigned> iterationCounters;

  std::optional<UnsupportedReason> unsupported;

  bool isSupported() const { return !unsupported.has_value(); }
};

/// Whether the trace is a function of the parameters alone: control oblivious, and every
/// address either oblivious or, when admitted, a one-level gather from read-only roots.
inline bool traceOblivious(const SemanticProgram &program) {
  return program.controlOblivious &&
         (program.addressOblivious || (program.admitGather && program.addressGather));
}

/// Whether `root` is a gather root of an admitted gather program.
inline bool isGatherRoot(const SemanticProgram &program, unsigned root) {
  return program.admitGather && program.addressGather &&
         llvm::is_contained(program.gatherRoots, root);
}

/// The contents a replay gives a gather root's cell c when it fixes them: c mod `gatherFillModulus`,
/// the smallest static extent of a dimension a gathered index addresses (1 when none is
/// static), so every gathered address is in bounds. A sample of the gather's contents,
/// never a claim about them.
uint64_t gatherFillModulus(const SemanticProgram &program);

/// Whether every Load in `term` is a gather of an admitted gather program: from a gather
/// root's entry memory, at an index with no Load. True of a term with no Load.
bool gatherLoadsOnly(const SemanticProgram &program, Term term);

/// The domain evidence a freshly imported model supports: none.
DomainEvidence importedDomainEvidence();

} // namespace mlir::generalized_gemm
#endif
