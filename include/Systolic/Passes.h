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

// 阶段 4:把任意静态形状的 linalg.matmul lowering 成呼叫已烧录好的
// 4x4 FPGA runtime(fpga_matmul_tiled_auto),不产生新硬件。
std::unique_ptr<Pass> createTileMatmulForFpgaPass();
void registerTileMatmulForFpgaPass();

// 阶段 5:把 batch=1、静态形状的 linalg.conv_2d_nhwc_hwcf lowering
// 成呼叫 im2col + 4x4 matmul runtime 的调用(fpga_conv2d_im2col_auto),
// 复用同一颗矩阵乘法加速器,不需要新硬件。
std::unique_ptr<Pass> createConv2DToFpgaPass();
void registerConv2DToFpgaPass();

// NCHW/FCHW 布局版本的 conv2d runtime offload(对应 PyTorch/torch-mlir
// 默认 lowering 出来的布局),复用同一颗 4x4 matmul 加速器。
std::unique_ptr<Pass> createConv2DNchwToFpgaPass();
void registerConv2DNchwToFpgaPass();

// 把 linalg.batch_matmul lowering 成呼叫 fpga_batch_matmul_tiled_auto,
// 依序把每個 batch 派送到既有 4x4 加速器,不需要新硬體。
std::unique_ptr<Pass> createBatchMatmulToFpgaPass();
void registerBatchMatmulToFpgaPass();

// 把 linalg.vecmat lowering 成呼叫 fpga_vecmat_tiled_auto,
// 把輸入向量當成 M=1 的 matmul 派送到既有 4x4 加速器。
std::unique_ptr<Pass> createVecmatToFpgaPass();
void registerVecmatToFpgaPass();

// 把 linalg.matvec lowering 成呼叫 fpga_matvec_tiled_auto,
// 把輸入向量當成 N=1 的 matmul 派送到既有 4x4 加速器。
std::unique_ptr<Pass> createMatvecToFpgaPass();
void registerMatvecToFpgaPass();

// 在 systolic-opt 工具里注册这个 pass
void registerSystolicPasses();

} // namespace systolic
} // namespace mlir

#endif // SYSTOLIC_PASSES_H
