#include "Systolic/AffineMatmulAnalysis.h"

using namespace mlir;
using namespace mlir::systolic;

void mlir::systolic::collectAffineMatmulCandidates(
    Operation *root,
    SmallVectorImpl<AffineMatmulCandidate> &candidates) {

  if (!root)
    return;

  root->walk([&](affine::AffineForOp outerLoop) {
    // Expect:
    //
    //   affine.for %i
    //     affine.for %j
    //       affine.for %k
    //
    // We deliberately keep discovery conservative here.

    affine::AffineForOp middleLoop;
    affine::AffineForOp innerLoop;

    // Find the unique affine.for directly nested in outerLoop.
    for (Operation &op : *outerLoop.getBody()) {
      if (auto loop = dyn_cast<affine::AffineForOp>(&op)) {
        if (middleLoop)
          return; // More than one candidate child loop.
        middleLoop = loop;
      }
    }

    if (!middleLoop)
      return;

    // Find the unique affine.for directly nested in middleLoop.
    for (Operation &op : *middleLoop.getBody()) {
      if (auto loop = dyn_cast<affine::AffineForOp>(&op)) {
        if (innerLoop)
          return;
        innerLoop = loop;
      }
    }

    if (!innerLoop)
      return;

    // Avoid discovering the same nest again starting from middleLoop/innerLoop.
    if (outerLoop->getParentOfType<affine::AffineForOp>())
      return;

    AffineMatmulCandidate candidate;
    candidate.anchor = outerLoop.getOperation();

    llvm::errs() << "Found affine matmul candidate:\n";
    llvm::errs() << "  outer IV: " << outerLoop.getInductionVar() << "\n";
    llvm::errs() << "  middle IV: " << middleLoop.getInductionVar() << "\n";
    llvm::errs() << "  inner IV: " << innerLoop.getInductionVar() << "\n";

    candidates.push_back(candidate);
  });
}

FailureOr<LogicalMatmul>
mlir::systolic::recognizeLogicalMatmul(AffineMatmulCandidate candidate) {
  // TODO:
  //   1. Recover the iteration space.
  //   2. Recover A/B/C accesses F_A, F_B, F_C.
  //   3. Verify multiply-accumulate reduction semantics.
  //   4. Recover P/I/J/K.
  //   5. Solve the access factorizations for phiA/phiB/phiC.
  (void)candidate;
  return failure();
}
