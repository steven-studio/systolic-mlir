//===- ReductionSemantics.h - Reorder facts ---------------------*- C++ -*-===//
//
// Facts about reordering the contributions of a cell. They are facts, never a licence:
// G6 decides how to use one, and G7 whether a lowering may.
//
//   trivial   Every cell has at most one contribution, so no order exists to change.
//   integer   Wrapping integer folds whose reduction carries no nsw or nuw, whose output
//             no other effect observes, and whose stops read no output cell: every
//             permutation within each cell gives the same final memory, in every run
//             that completes. Equal values alone are never taken as enough.
//   float     Proven only by `trivial`. Refuted by a replayed memory at which reversing
//             the contributions of a cell changes a bit of its final value.
//
// A refutation carries the memory and the permutation. The test judges it again.
//
// G5b-M4b-2a: the integer fact and its witness over the union of several groups. The
// conditions are the same, read over every listed output root; the witness replays the
// set (replayGroupSet) and judges the reversal of every cell the union's events write.
// The one-group functions are the union forms over one group, byte for byte.
//
//===----------------------------------------------------------------------===//
#ifndef GENERALIZEDGEMM_REDUCTIONSEMANTICS_H
#define GENERALIZEDGEMM_REDUCTIONSEMANTICS_H

#include "GeneralizedGemm/ComputationChecker.h"
#include "GeneralizedGemm/DomainModel.h"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace mlir::generalized_gemm {

/// The structural conditions of the integer fact, or the first that fails.
bool integerReorderConditions(const SemanticProgram &program, const SiteGroup &group,
                              const CheckedComputation &computation, std::string &why);

/// G5b-M4b-2a: the same conditions over the union of several groups. `outputs` are the
/// output roots of every listed group: a stop or a guard that loads from any of them, or
/// from state that holds a value from memory, refuses the fact. The frame and the flags
/// come from `computation`, the set claim, which already covers every site of the union.
/// The one-group function is this one over the group's root.
bool integerReorderConditionsOver(const SemanticProgram &program, llvm::ArrayRef<unsigned> outputs,
                                  const CheckedComputation &computation, std::string &why);

/// G5b-M4b-2a: the listed groups as one site group for the witness judge, through the gate
/// every claim over a set shares (SiteSets): each group whole, no group and no site twice.
/// `groupOfSite` is the listed index of each program site, as replayGroupSet takes it.
/// Nullopt, with `why`, when the gate refuses. It claims nothing.
std::optional<SiteGroup> unitedSiteGroup(const SemanticProgram &program,
                                         llvm::ArrayRef<const SiteGroup *> listed,
                                         std::vector<int> &groupOfSite, std::string &why);

/// Looks for a memory at `parameters` where the run completes and reversing the
/// contributions of one cell overflows (integers with nsw or nuw) or changes a bit
/// (floats). The fill sequence is fixed, so a result is reproducible.
Fact reorderWitness(const SemanticProgram &program, const SiteGroup &group,
                    const std::vector<llvm::APInt> &parameters, unsigned fills,
                    uint64_t maxCells);

/// G5b-M4b-2a: reorderWitness over the union of the listed groups. The set's replay gives
/// the constants a cell holds before its first event; the same fill sequence is searched;
/// reversalDiffers judges the union's events on every listed root. Unknown, with the
/// reason, when the gate refuses the listing or the replay is not equal.
Fact reorderWitnessOver(const SemanticProgram &program, llvm::ArrayRef<const SiteGroup *> listed,
                        const std::vector<llvm::APInt> &parameters, unsigned fills, uint64_t maxCells);

/// Judges a witness again: true when the run of `entry` completes and the reversed fold
/// of some cell overflows or differs. Shared by the checker and the tests.
bool reversalDiffers(const SemanticProgram &program, const SiteGroup &group,
                     const EntryState &entry, const std::map<CellKey, llvm::APInt> &prefix,
                     std::string &detail);

} // namespace mlir::generalized_gemm
#endif
