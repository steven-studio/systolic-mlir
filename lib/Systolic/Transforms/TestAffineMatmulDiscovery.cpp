#include "Systolic/AffineMatmulAnalysis.h"
#include "Systolic/Passes.h"

#include "mlir/IR/BuiltinOps.h"
#include "mlir/Pass/Pass.h"

#include "llvm/Support/raw_ostream.h"

using namespace mlir;
using namespace mlir::systolic;

namespace {

struct TestAffineMatmulDiscoveryPass
    : public PassWrapper<TestAffineMatmulDiscoveryPass,
                         OperationPass<ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(
      TestAffineMatmulDiscoveryPass)

  StringRef getArgument() const final {
    return "test-affine-matmul-discovery";
  }

  StringRef getDescription() const final {
    return "Test discovery of candidate affine matmul loop nests";
  }

  void runOnOperation() override {
    SmallVector<AffineMatmulCandidate> candidates;

    collectAffineMatmulCandidates(
        getOperation().getOperation(), candidates);

    llvm::errs() << "candidate count: "
                 << candidates.size()
                 << "\n";
  }
};

} // namespace

void mlir::systolic::registerTestAffineMatmulDiscoveryPass() {
  PassRegistration<TestAffineMatmulDiscoveryPass>();
}
