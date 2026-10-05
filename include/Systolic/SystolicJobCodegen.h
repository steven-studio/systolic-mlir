#ifndef SYSTOLIC_JOB_CODEGEN_H
#define SYSTOLIC_JOB_CODEGEN_H

#include "mlir/Pass/Pass.h"

namespace mlir {
namespace systolic {

std::unique_ptr<Pass> createSystolicJobCodegenPass();

void registerSystolicJobCodegenPass();

} // namespace systolic
} // namespace mlir

#endif // SYSTOLIC_JOB_CODEGEN_H
