#ifndef SYSTOLIC_PASSES_H
#define SYSTOLIC_PASSES_H

#include "mlir/Pass/Pass.h"

namespace mlir {
namespace systolic {

// 把 linalg.matmul lowering 成 systolic dialect。
// 目前只处理形状固定、且刚好等于 (rows x cols) 的 matmul(阶段 2 MVP)。
std::unique_ptr<Pass> createConvertMatmulToSystolicPass();

// 把 systolic.pe_array 展开成 rows x cols x K 的三层 scf.for 迴圈,
// 内层是真正的 systolic.mac 调用——不再是黑盒。
std::unique_ptr<Pass> createExpandPEArrayToMacPass();

// 各自 pass 的 registration function,由 registerSystolicPasses() 统一呼叫
void registerExpandPEArrayToMacPass();

// 在 systolic-opt 工具里注册这个 pass
void registerSystolicPasses();

} // namespace systolic
} // namespace mlir

#endif // SYSTOLIC_PASSES_H
