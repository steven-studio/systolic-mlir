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
//===----------------------------------------------------------------------===//
#ifndef GENERALIZEDGEMM_REDUCTIONSEMANTICS_H
#define GENERALIZEDGEMM_REDUCTIONSEMANTICS_H

#include "GeneralizedGemm/ComputationChecker.h"
#include "GeneralizedGemm/DomainModel.h"

#include <map>
#include <string>
#include <vector>

namespace mlir::generalized_gemm {

/// The structural conditions of the integer fact, or the first that fails.
bool integerReorderConditions(const SemanticProgram &program, const SiteGroup &group,
                              const CheckedComputation &computation, std::string &why);

/// Looks for a memory at `parameters` where the run completes and reversing the
/// contributions of one cell overflows (integers with nsw or nuw) or changes a bit
/// (floats). The fill sequence is fixed, so a result is reproducible.
Fact reorderWitness(const SemanticProgram &program, const SiteGroup &group,
                    const std::vector<llvm::APInt> &parameters, unsigned fills,
                    uint64_t maxCells);

/// Judges a witness again: true when the run of `entry` completes and the reversed fold
/// of some cell overflows or differs. Shared by the checker and the tests.
bool reversalDiffers(const SemanticProgram &program, const SiteGroup &group,
                     const EntryState &entry, const std::map<CellKey, llvm::APInt> &prefix,
                     std::string &detail);

} // namespace mlir::generalized_gemm
#endif
