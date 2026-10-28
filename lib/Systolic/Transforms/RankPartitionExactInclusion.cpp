//===- RankPartitionExactInclusion.cpp ------------------------------------===//
//
// Phase 5.4.3d-3b-2: sound conjunctive-set inclusion for the rank-partition
// emitters, forwarded to the exact decision procedure of section 19.14.
//
//===----------------------------------------------------------------------===//

#include "Systolic/RankPartitionExactInclusion.h"

#include "Systolic/AffineMatmulAnalysis.h"

std::optional<bool> systolic::logical::exactIntegerSetInclusion(
    const mlir::presburger::IntegerRelation &a,
    const mlir::presburger::IntegerRelation &b) {
  return mlir::systolic::exactIsSubset(a, b);
}
