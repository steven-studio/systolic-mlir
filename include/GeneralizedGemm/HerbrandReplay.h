//===- HerbrandReplay.h - Values as uninterpreted terms ---------*- C++ -*-===//
//
// Runs a program at one parameter point with every initial cell an unknown. Control
// and addresses are concrete, since the program is oblivious. A value that depends
// on memory is a hash-consed node keyed by its operation, flags, widths and operand
// order, and no node is ever simplified.
//
// Equal nodes have equal values under every initial memory, whatever the arithmetic
// means, so equality is a proof for all of them (Theorem 11). It is sufficient and
// not complete: x + y and y + x are different nodes. So a mismatch means only that
// this argument failed, and the checker must find a concrete run to say more.
//
// A side condition whose value depends on memory is taken as not violated: a run
// that violates it stops, and the claim is about runs that complete. Each such
// condition is reported.
//
// The program side here uses the transitions alone. Only the replay side reads the
// site records, so the two are compared rather than one following from the other.
//
//===----------------------------------------------------------------------===//
#ifndef GENERALIZEDGEMM_HERBRANDREPLAY_H
#define GENERALIZEDGEMM_HERBRANDREPLAY_H

#include "GeneralizedGemm/ComputationModel.h"
#include "GeneralizedGemm/FiniteDomain.h"

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace mlir::generalized_gemm {

struct HerbrandLimits {
  uint64_t maxSteps = 1u << 22, maxNodes = 1u << 23, maxCells = 1u << 20;
};

enum class ReplayOutcome {
  Equal,           // Every event, operand and final cell agrees.
  NoCompletingRun, // Every initial memory stops, with no dependence on values.
  Mismatch,        // Nodes differ. Not a refutation by itself.
  Unobservable,    // A return value depends on memory.
  Limit,           // A resource limit. Says nothing.
  Refused,         // Outside what this replay handles. Says nothing.
};
const char *stringifyReplayOutcome(ReplayOutcome outcome);

using CellKey = std::pair<unsigned, uint64_t>; // Root, row-major offset.

struct ReplayResult {
  ReplayOutcome outcome = ReplayOutcome::Refused;
  std::string reason;
  ReplayFacts facts;
  // Constants a cell holds before its first event, which start its fold.
  std::map<CellKey, llvm::APInt> prefix;
  std::set<MemoryDependentStop> conditionalStops;
  // G5b-M3, from replayGroupSet only: the events of each listed group, the cells two
  // listed groups write, and whether no event of a group followed one of a later-listed
  // group. Read only when the run reached the exit: Equal or Mismatch.
  std::vector<uint64_t> groupEvents;
  uint64_t sharedCells = 0;
  bool listedOrder = true;
};

/// The relation for one group at one point, over all initial memories. The machine
/// reads only `group.sites`, and each event's roots from its own site.
ReplayResult replayGroup(const SemanticProgram &program, const SiteGroup &group,
                         const std::vector<llvm::APInt> &parameters,
                         const HerbrandLimits &limits);

/// G5b-M3: the same relation over `united`, the sites of several groups, with every
/// event counted for its group. `groupOfSite[s]` is the listed index of site s's group,
/// below `groupCount`, for every site of `united`.
ReplayResult replayGroupSet(const SemanticProgram &program, const SiteGroup &united,
                            const std::vector<int> &groupOfSite, unsigned groupCount,
                            const std::vector<llvm::APInt> &parameters,
                            const HerbrandLimits &limits);

} // namespace mlir::generalized_gemm
#endif
