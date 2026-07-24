#ifndef SYSTOLIC_PASSES_H
#define SYSTOLIC_PASSES_H

#include "mlir/Pass/Pass.h"

namespace mlir {
namespace systolic {

// 把 linalg.matmul lowering 成 systolic dialect。
// 目前只处理形状固定、且刚好等于 (rows x cols) 的 matmul(阶段 2 MVP)。
std::unique_ptr<Pass> createConvertMatmulToSystolicPass();

// 阶段 1:走过 systolic.device / systolic.matmul_tile / systolic.dma,
// 用 CostModel.h 的封闭公式标注 est_cycles。
std::unique_ptr<Pass> createSystolicCostAnalysisPass();
void registerSystolicCostAnalysisPass();

// 把 systolic.pe_array 展开成 rows x cols x K 的三层 scf.for 迴圈,
// 内层是真正的 systolic.mac 调用——不再是黑盒。
std::unique_ptr<Pass> createExpandPEArrayToMacPass();

// 各自 pass 的 registration function,由 registerSystolicPasses() 统一呼叫
void registerExpandPEArrayToMacPass();

// 阶段 4:把任意静态形状的 linalg.matmul lowering 成呼叫已烧录好的
// 4x4 FPGA runtime(fpga_matmul_tiled_auto),不产生新硬件。
std::unique_ptr<Pass> createTileMatmulForFpgaPass();
void registerTileMatmulForFpgaPass();

// 阶段 5:把 batch=1、静态形状的 linalg.conv_2d_nhwc_hwcf lowering
// 成呼叫 im2col + 4x4 matmul runtime 的调用(fpga_conv2d_im2col_auto),
// 复用同一颗矩阵乘法加速器,不需要新硬件。
std::unique_ptr<Pass> createConv2DToFpgaPass();
void registerConv2DToFpgaPass();

// 在 systolic-opt 工具里注册这个 pass
void registerSystolicPasses();

} // namespace systolic
} // namespace mlir

#endif // SYSTOLIC_PASSES_H
