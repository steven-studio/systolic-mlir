//===- CoverageSimulation.h - Theorem 8 -------------------------*- C++ -*-===//
//
// The formulas of exact coverage. The walk from one event to the next is
// unfolded with no regard for cuts, pruned where the solver shows a prefix
// infeasible. A path that reaches the depth limit must be infeasible too.
//
//   S1  The first event is first(v), or the run has none and first(v) is not in
//       the nested set.
//   S2  From b, every next event is succ(b), and it is in the set.
//   S3  From an element with no successor, every path leaves with no event, and
//       an element with one cannot reach the exit.
//
// The enumerated set is the orbit of first(v) under succ. It is the whole nested
// set when that holds first(v), as S2 demands that every step stays inside.
//
//===----------------------------------------------------------------------===//
#ifndef GENERALIZEDGEMM_COVERAGESIMULATION_H
#define GENERALIZEDGEMM_COVERAGESIMULATION_H

#include "GeneralizedGemm/CoordinateFamily.h"
#include "GeneralizedGemm/SmtBackend.h"
#include "GeneralizedGemm/SymbolicDomain.h"

#include <functional>
#include <string>

namespace mlir::generalized_gemm {

/// Answers a query that must be unsat, with its name. The caller records it.
using AskUnsat = std::function<SmtAnswer(const std::string &id, const std::string &where,
                                         Term formula)>;

/// Term of membership in the nested set, over the given level values.
Term inOrderSet(const SemanticProgram &program, const EnumerationOrder &order,
                llvm::ArrayRef<Term> values);

/// The values of the order's levels, restated over a path's start.
std::vector<Term> orderValues(const SemanticProgram &program, const EnumerationOrder &order,
                              const SymbolicPath &path);

/// One step of every proof by simulation: each feasible way from the end of `path` to
/// the next arrival at one of `locations` or to the exit, calling `leaf` with the path,
/// its premise and whether it exited. Prunes where `ask` shows a prefix infeasible, and
/// fails at `depth` transitions past `path` unless that is infeasible. True only when
/// every leaf returned true. Exposed for G5, with the walk proveCoverage uses.
using WalkLeaf = std::function<bool(const SymbolicPath &, Term premise, bool exit)>;
bool walkToNextEvents(const SemanticProgram &program, llvm::ArrayRef<unsigned> locations,
                      unsigned depth, const AskUnsat &ask, std::string &why,
                      const SymbolicPath &path, Term assumption, const WalkLeaf &leaf);

/// True only when every obligation is unsat. `why` names the first that is not.
bool proveCoverage(const SemanticProgram &program, const EnumerationOrder &order,
                   const SiteArrivals &arrivals, llvm::ArrayRef<unsigned> locations,
                   Term scope, unsigned maxUnfolding, const AskUnsat &ask, std::string &why);

} // namespace mlir::generalized_gemm
#endif
