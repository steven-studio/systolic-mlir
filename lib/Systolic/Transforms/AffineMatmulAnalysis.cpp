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
  if (!candidate.anchor)
    return failure();

  SmallVector<affine::AffineLoadOp> loads;
  SmallVector<affine::AffineStoreOp> stores;

  candidate.anchor->walk([&](Operation *op) {
    if (auto load = dyn_cast<affine::AffineLoadOp>(op))
      loads.push_back(load);
    else if (auto store = dyn_cast<affine::AffineStoreOp>(op))
      stores.push_back(store);
  });

  llvm::errs() << "Affine matmul recognition:\n";
  llvm::errs() << "  loads: " << loads.size() << "\n";
  llvm::errs() << "  stores: " << stores.size() << "\n";

  for (auto [index, load] : llvm::enumerate(loads)) {
    llvm::errs() << "  load[" << index << "] map: ";
    load.getMap().print(llvm::errs());
    llvm::errs() << "\n";
  }

  for (auto [index, store] : llvm::enumerate(stores)) {
    llvm::errs() << "  store[" << index << "] map: ";
    store.getMap().print(llvm::errs());
    llvm::errs() << "\n";
  }

  // Canonical GEMM currently expected to contain:
  //
  //   A load
  //   B load
  //   C load
  //   C store
  //
  // This is only the first recognition checkpoint.  We have not yet
  // established multiply-accumulate semantics or recovered logical I/J/K.
  if (loads.size() != 3 || stores.size() != 1)
    return failure();

  return failure();
}
