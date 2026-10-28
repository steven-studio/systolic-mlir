//===- RankPartitionProductionPassRegistration.cpp -------------------------===//
//
// Phase 5.4.3d-3b-5: registers the production GEMM pass for the unit test.
// The test translation unit must not see the mlir::systolic namespace (its
// rank-partition code lives in systolic::logical and is used under
// `using namespace mlir`), so the registration is forwarded from here.
//
//===----------------------------------------------------------------------===//

#include "Systolic/Passes.h"
#include "Systolic/SystolicDialect.h"

#include "mlir/Dialect/Bufferization/IR/Bufferization.h"
#include "mlir/Dialect/Tensor/IR/Tensor.h"
#include "mlir/IR/MLIRContext.h"

namespace systolic::logical::testing {

// The dialects the production pass's output uses beyond the test's own
// (Phase 3b-6: systolic.device fleets and the tile plan).
void loadProductionDialects(mlir::MLIRContext &context) {
  context.loadDialect<mlir::systolic::SystolicDialect,
                      mlir::bufferization::BufferizationDialect,
                      mlir::tensor::TensorDialect>();
}

void registerProductionGemmPass() {
  static bool registered = false;
  if (registered)
    return;
  registered = true;
  mlir::systolic::registerAffineMatmulToSystolicPass();
}

} // namespace systolic::logical::testing
