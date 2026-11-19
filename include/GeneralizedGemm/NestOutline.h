//===- NestOutline.h - One analysis function per loop nest ------*- C++ -*-===//
//
// Stage 1 of docs/GENERALIZED_GEMM_SCOPE.md. Every claim of the library is about a whole
// function, from its entry to its exit. To ask about one loop nest, the nest is copied
// into a function of its own, in a module of its own, and the importer and the checkers
// run on that copy unchanged. The function given is read and never written.
//
// What the copy keeps, and what it says of the nest:
//
//   inputs          Every value the nest reads and does not define. A memory root becomes
//                   an argument: any memory at the nest's entry. A scalar becomes an
//                   argument: any value. Only an `arith.constant` is copied in, because
//                   only it has the same value at the nest's entry whatever ran before.
//                   Nothing else is copied: a value computed from memory before the nest
//                   would be read again at another time.
//   entry state     Nothing is fixed but the constants. What earlier statements did,
//                   other nests included, is part of the memory and the scalars the claim
//                   ranges over. So a claim about the copy is a claim about the nest
//                   wherever it runs in the function, and no nest is taken to be
//                   independent of another.
//   memory effects  The nest's operations are cloned as they are, in their order.
//   preconditions   How two roots stand is decided in the function, not in the copy: a
//                   root allocated in the function is apart from every other (a proof),
//                   two arguments are apart only by a name somebody authorized (an
//                   assumption). In the copy both are arguments, so each fact is handed
//                   over by name, with the basis it had in the function.
//
// What cannot be kept is refused in words, and nothing is outlined: a loop whose value
// is used after it, a parallel loop (its iterations have no order, and every claim is
// about the nest's own), a call, a memref that is no root of the function (one made before
// the nest by anything but an allocation, or made inside it: a view, a global, a memref
// carried round the loop), a value of a type that is no scalar, a function whose control
// flow is by branches.
//
// The order of the function's statements and how the nests meet on roots are recorded
// beside the nests. They are a description, by root, of what may depend on what: an
// over-approximation, and never a reason to call two nests independent.
//
//===----------------------------------------------------------------------===//
#ifndef GENERALIZEDGEMM_NESTOUTLINE_H
#define GENERALIZEDGEMM_NESTOUTLINE_H

#include "GeneralizedGemm/ProofStatus.h"

#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/OwningOpRef.h"

#include <optional>
#include <string>
#include <vector>

namespace mlir::generalized_gemm {

/// A memory root of the function, numbered as the importer numbers them: the memref
/// arguments in order, then the allocations in program order.
struct FunctionRoot {
  enum Origin { FunctionArgument, FreshAllocation };
  Origin origin = FunctionArgument;
  unsigned id = 0;
  unsigned argument = 0;         // For FunctionArgument.
  bool restrictArgument = false; // llvm.noalias on the argument.
  bool insideLoop = false;       // For FreshAllocation: allocated inside a loop.
  std::string type, location;
};

enum NestAccess : unsigned { NestReads = 1, NestWrites = 2 };

/// A root as one nest uses it.
struct NestRoot {
  unsigned root = 0;     // The function's root id.
  unsigned outlined = 0; // The root's id in the copy, which is its argument number.
  unsigned access = 0;   // NestAccess bits, by the nest's own operations.
};

/// A scalar the nest reads and does not define.
struct NestScalar {
  enum Source { FunctionArgument, ComputedValue };
  Source source = FunctionArgument;
  unsigned argument = 0;     // For FunctionArgument.
  unsigned outlined = 0;     // The argument number in the copy.
  std::string type, definition, location; // The defining operation, for ComputedValue.
};

/// How two roots of one nest stand in the function.
struct NestSeparation {
  unsigned first = 0, second = 0; // Function root ids, first < second.
  bool apart = false;
  bool proof = false;       // True only for a fresh allocation.
  std::string basis;        // "fresh allocation", "authorized", or empty when not apart.
  std::string name;         // The name over the function's roots.
  std::string outlinedName; // The same pair over the copy's roots: what is handed over.
};

struct OutlinedNest {
  unsigned ordinal = 0;    // Among the function's outermost loops, in program order.
  unsigned statement = 0;  // The statement of the function's body that is or holds it.
  unsigned conditions = 0; // Operations with regions around it. None of them is a loop.
  std::string function, loop, location;
  unsigned line = 0, column = 0; // Of the loop, when its location is a file position.

  /// Nonempty: the nest has no faithful copy, and `module` is null.
  std::string unsupported;

  /// The private copy: one function named `outlinedName`, and nothing else.
  OwningOpRef<ModuleOp> module;
  std::string outlinedName;
  std::vector<NestRoot> roots;     // In the copy's argument order.
  std::vector<NestScalar> scalars; // After the roots, in the copy's argument order.
  unsigned constantsCopied = 0;
  /// Every pair of the nest's roots, in order.
  std::vector<NestSeparation> separations;

  /// Stage 2: which loop of which input the nest is (Systolic/NestBinding.h): the SHA-256 of
  /// the binding the production pass computes with the same code. Empty, with the reason in
  /// `bindingRefusal`, when no binding can be computed. Set whether or not the nest has a copy.
  std::string bindingSha, bindingRefusal;

  func::FuncOp outlined() const;
};

/// One statement of the function's body that touches memory, or a nest, in program order.
struct OrderEntry {
  enum Kind { Nest, Access, Allocation, Opaque };
  Kind kind = Opaque;
  unsigned nest = 0;       // For Nest.
  unsigned root = 0;       // For Access and Allocation.
  unsigned access = 0;     // NestAccess bits, for Access.
  unsigned statement = 0, conditions = 0;
  std::string operation, location;
};

/// Two nests that meet on memory, the earlier first. By root: what may be, not what is.
struct NestDependence {
  /// SameRoot: both use one root, and one of them writes it. MayAlias: two roots not
  /// known apart. UnknownEffects: one of the nests has effects that are no access to a
  /// root (a call, a memref that is no root, an operation with effects of its own), so
  /// it may meet the other anywhere.
  enum Relation { SameRoot, MayAlias, UnknownEffects };
  Relation relation = UnknownEffects;
  unsigned earlier = 0, later = 0;         // Nest ordinals.
  unsigned earlierRoot = 0, laterRoot = 0; // Not for UnknownEffects.
  unsigned earlierAccess = 0, laterAccess = 0;
};

struct FunctionOutline {
  std::string function;
  /// Nonempty: the function's loops cannot be told apart here, and there are no nests.
  std::string unsupported;
  std::vector<FunctionRoot> roots;
  std::vector<OutlinedNest> nests;
  std::vector<OrderEntry> order;
  std::vector<NestDependence> dependences;
};

/// The name of the separation of two roots, as every checker spells it.
std::string separationName(unsigned first, unsigned second);

/// Outlines every outermost loop of `function`. `authorized` names separations over the
/// function's roots. `function` is not changed.
FunctionOutline outlineNests(func::FuncOp function, const Authorizations &authorized);

} // namespace mlir::generalized_gemm
#endif
