//===- CutPaths.h - Every path between cut points ---------------*- C++ -*-===//
//
// With a cut set that breaks all cycles, every run decomposes into paths from
// the entry or a cut to the next cut or to the exit. There are finitely many,
// and enumerating them is what makes each feasible branch an obligation of its
// own. A path is one sequence of G1 transitions, so parallel edges stay apart.
//
// condition covers the guards and the no-stop conditions of the SOURCE blocks
// of a path, not of the block where it ends. That block's conditions belong to
// the path that leaves it. This matters: an invariant must hold at a state that
// is stuck on arrival, or it could not show that state unreachable.
//
// condition omits the side conditions that contain a float term. A premise only
// gets weaker, so each proof about these paths covers a superset of the runs.
// Every omitted condition still appears as a StopSite.
//
// Paths from the entry start with every shadow false, as initialConfiguration
// does. State is free there: SSA dominance means nothing reads it before it is
// assigned.
//
//===----------------------------------------------------------------------===//
#ifndef GENERALIZEDGEMM_CUTPATHS_H
#define GENERALIZEDGEMM_CUTPATHS_H

#include "GeneralizedGemm/SemanticProgram.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"

#include <string>
#include <vector>

namespace mlir::generalized_gemm {

struct PathLimits {
  unsigned maxPaths = 512, maxPathLength = 256, maxStopSites = 2048;
  unsigned maxNewTerms = 1u << 18;
};

struct CutPath {
  unsigned id = 0;
  unsigned from = 0, to = 0; // `to` may be the exit location.
  bool fromEntry = false;
  llvm::SmallVector<unsigned> transitions;
  Term condition = nullptr;
  // Parallel to program.state and program.roots. nullptr means unchanged.
  std::vector<Term> state, shadow, memory;
};

/// One side condition at one block reached from a source. `reached` says the
/// walk got there; `violated` is over the configuration at the source.
struct StopSite {
  unsigned from = 0;
  bool fromEntry = false;
  llvm::SmallVector<unsigned> prefix; // Transitions before the block.
  unsigned location = 0;
  SideKind kind = SideKind::UndefinedBehavior;
  std::string reason;
  Term reached = nullptr, violated = nullptr;
};

struct PathSet {
  bool complete = false;
  std::string reason;
  std::vector<CutPath> paths;
  std::vector<StopSite> sites;
};

/// Exhausting a limit returns complete = false and nothing else. `cuts` must
/// have passed cutSetBreaksCycles.
PathSet enumerateCutPaths(const SemanticProgram &program,
                          llvm::ArrayRef<unsigned> cuts,
                          const PathLimits &limits = {});

/// `term`, a term over the configuration at the end of `path`, restated over
/// the configuration at its start. All replacements are simultaneous.
Term postTerm(const SemanticProgram &program, const CutPath &path, Term term);

/// Whether any node below `term` has float sort.
bool containsFloat(Term term);

/// "L1 -0-> L2 -1-> L1": locations and successor slots.
std::string describePath(const SemanticProgram &program, const CutPath &path);

} // namespace mlir::generalized_gemm
#endif
