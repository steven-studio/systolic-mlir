//===- SymbolicEvaluator.h - Path summaries of acyclic regions --*- C++ -*-===//
//
// Enumerates the control paths leaving one location, composing transitions by
// substitution. A path stops at the exit, or on reaching a location it already
// visited: a loop is an explicit stop, never a guess, and no loop is
// summarized. Every path term is over the configuration at `from`.
//
// The path conditions partition the configurations at `from`: exactly one is
// true. Exhausting a limit returns complete = false with no paths.
//
//===----------------------------------------------------------------------===//
#ifndef GENERALIZEDGEMM_SYMBOLICEVALUATOR_H
#define GENERALIZEDGEMM_SYMBOLICEVALUATOR_H

#include "GeneralizedGemm/SemanticProgram.h"

namespace mlir::generalized_gemm {

struct SymbolicLimits {
  unsigned maxPaths = 256, maxPathLength = 256;
  unsigned maxNewTerms = 1u << 18;
};

struct SymbolicPath {
  enum Stop { ReachedExit, ReachedRevisit };
  Stop stop = ReachedExit;
  unsigned end = 0;
  llvm::SmallVector<unsigned> transitions;
  Term condition = nullptr;
  // Parallel to program.state and program.roots. nullptr means unchanged.
  std::vector<Term> state, shadow, memory;
  llvm::SmallVector<SideCondition, 0> sideConditions;
  llvm::SmallVector<MemoryEffect, 0> effects;
  llvm::SmallVector<Term> returns, returnPoison;
};

struct SymbolicRegion {
  bool complete = false;
  std::string reason;
  std::vector<SymbolicPath> paths;
};

SymbolicRegion summarizeAcyclic(const SemanticProgram &program, unsigned from,
                                const SymbolicLimits &limits = {});

} // namespace mlir::generalized_gemm
#endif
