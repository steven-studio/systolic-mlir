//===- CanonicalRecovery.h - Partition joins --------------------*- C++ -*-===//
//
// An untrusted proposer. It computes the canonical joins over T and offers them
// as a candidate; only CandidateChecker can say anything about that candidate.
// The joins are exact for injective views on a batched box, and nothing more:
// see docs/GENERALIZED_GEMM_G3A.md for the theorems and their limits.
//
//===----------------------------------------------------------------------===//
#ifndef GENERALIZEDGEMM_CANONICALRECOVERY_H
#define GENERALIZEDGEMM_CANONICALRECOVERY_H

#include "GeneralizedGemm/CoordinateModel.h"

namespace mlir::generalized_gemm {

/// All partitions are over T, indexed like AccessTable::triples().
struct CanonicalPartitions {
  Partition kerH, kerF, kerG;
  Partition rhoI;  // ker h v ker f: the kernel of (P,I) in the class.
  Partition rhoJ;  // ker h v ker g.
  Partition rhoK;  // ker f v ker g.
  Partition rhoP;  // ker h v ker f v ker g.
  // C1 to C3. A hint: the checker decides independently.
  bool meetH = false, meetF = false, meetG = false;
};

/// The kernels of the three views over T.
void viewKernels(const AccessTable &table, Partition &kerH, Partition &kerF,
                 Partition &kerG);

/// Union-find joins of the three kernels, which must be over one set.
CanonicalPartitions joinKernels(Partition kerH, Partition kerF, Partition kerG);

/// Canonical blocks as labels per event, which keeps events apart from T.
CoordinateCandidate canonicalCandidate(const AccessTable &table,
                                       const CanonicalPartitions &partitions);

} // namespace mlir::generalized_gemm
#endif
