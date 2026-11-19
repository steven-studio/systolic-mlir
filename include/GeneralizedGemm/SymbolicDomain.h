//===- SymbolicDomain.h - An over-approximation of D ------------*- C++ -*-===//
//
// Every state at which a site can fire, as paths from the entry or a certified
// cut. Each arrival's premise covers the scope, the invariant at its start, and
// every guard and no-stop condition up to and including the site's block, since
// a block that stops contributes no event. Conditions that read memory or floats
// are left out, which only weakens the premise.
//
// The result contains the states of every completing run, and may contain more.
// So it supports universal claims and no others (invariant I8): a failure found
// here is a counterexample only once it is replayed.
//
// controlLiveState is the state that decides the future control of a location.
// Theorem 7: two events of one site in a completing run of an oblivious program
// differ there, or the run would repeat forever.
//
//===----------------------------------------------------------------------===//
#ifndef GENERALIZEDGEMM_SYMBOLICDOMAIN_H
#define GENERALIZEDGEMM_SYMBOLICDOMAIN_H

#include "GeneralizedGemm/CutPaths.h"
#include "GeneralizedGemm/SemanticProgram.h"
#include "GeneralizedGemm/SymbolicEvaluator.h"

#include <map>
#include <set>
#include <string>
#include <vector>

namespace mlir::generalized_gemm {

struct SiteArrival {
  unsigned site = 0, location = 0;
  SymbolicPath path;
  Term premise = nullptr;
};

struct SiteArrivals {
  bool complete = false;
  std::string reason;
  std::vector<SiteArrival> arrivals;
};

/// `starts`: the certified cuts, each with its invariant. The entry is implied.
SiteArrivals reachSites(const SemanticProgram &program,
                        const std::map<unsigned, Term> &invariants,
                        llvm::ArrayRef<unsigned> cuts,
                        llvm::ArrayRef<unsigned> sites, Term scope,
                        const PathLimits &limits);

/// The same for blocks: every arrival at each location, whether or not a site is
/// there, with `site` holding the location. For effects that belong to no site.
SiteArrivals reachLocations(const SemanticProgram &program,
                            const std::map<unsigned, Term> &invariants,
                            llvm::ArrayRef<unsigned> cuts,
                            llvm::ArrayRef<unsigned> locations, Term scope,
                            const PathLimits &limits);

/// The guards so far and the no-stop conditions of the blocks left, with memory
/// and float conditions dropped. Null when a guard itself reads memory.
Term pathPremise(const SemanticProgram &program, const SymbolicPath &path);

/// `term` at the end of `path`, restated over the path's start.
Term atPathEnd(const SemanticProgram &program, const SymbolicPath &path, Term term);

/// The same term over a second, disjoint copy of the state. Parameters are shared.
Term secondCopy(const SemanticProgram &program, Term term);

std::map<unsigned, std::set<unsigned>> controlLiveState(const SemanticProgram &program);

} // namespace mlir::generalized_gemm
#endif
