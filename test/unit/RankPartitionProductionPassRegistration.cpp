//===- RankPartitionProductionPassRegistration.cpp -------------------------===//
//
// Phase 5.4.3d-3b-5: registers the production GEMM pass for the unit test.
// The test translation unit must not see the mlir::systolic namespace (its
// rank-partition code lives in systolic::logical and is used under
// `using namespace mlir`), so the registration is forwarded from here.
//
//===----------------------------------------------------------------------===//

#include "Systolic/Passes.h"

namespace systolic::logical::testing {

void registerProductionGemmPass() {
  static bool registered = false;
  if (registered)
    return;
  registered = true;
  mlir::systolic::registerAffineMatmulToSystolicPass();
}

} // namespace systolic::logical::testing
