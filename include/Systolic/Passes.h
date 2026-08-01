#ifndef SYSTOLIC_PASSES_H
#define SYSTOLIC_PASSES_H

#include "mlir/Pass/Pass.h"

namespace mlir {
namespace systolic {

//===----------------------------------------------------------------------===//
// Stage 1: Accelerator selection
//   - CostAnalysis:  走过 systolic.device / systolic.matmul_tile /
//                    systolic.dma,用 CostModel.h 的封闭公式标注 est_cycles。
//   - SelectDevice:  贪婪 list scheduling,把还没指定 device 的 matmul_tile
//                    分配到能让整体 makespan 最小的 systolic.device 上。
//===----------------------------------------------------------------------===//

std::unique_ptr<Pass> createSystolicCostAnalysisPass();
void registerSystolicCostAnalysisPass();

std::unique_ptr<Pass> createSystolicSelectDevicePass();
void registerSystolicSelectDevicePass();

//===----------------------------------------------------------------------===//
// Stage 2: Tile partitioning
//   - TileMatmul:              把静态形状的 linalg.matmul 切成一格一格的
//                              systolic.matmul_tile(先切、不指定 device)。
//   - ConvertMatmulToSystolic: 形状刚好等于 (rows x cols) 的特例,直接
//                              lowering 成 systolic.stream/pe_array(阶段 2
//                              MVP,不经过 matmul_tile)。
//===----------------------------------------------------------------------===//

std::unique_ptr<Pass> createSystolicTileMatmulPass();
void registerSystolicTileMatmulPass();

std::unique_ptr<Pass> createConvertMatmulToSystolicPass();

//===----------------------------------------------------------------------===//
// Stage 3: Overlap-aware scheduling
//   - ScheduleOverlap: 假设每颗 device 有各自独立的 compute 引擎跟 DMA
//                      引擎,计算每个已指派 tile 的 start_cycle,让
//                      tile i+1 的预取 DMA 跟 tile i 的运算重叠
//                      (double buffering),而不是全部序列化。
//===----------------------------------------------------------------------===//

std::unique_ptr<Pass> createSystolicScheduleOverlapPass();
void registerSystolicScheduleOverlapPass();

//===----------------------------------------------------------------------===//
// PE array expansion
//   把 systolic.pe_array 展开成 rows x cols x K 的三层 scf.for 迴圈,
//   内层是真正的 systolic.mac 调用——不再是黑盒。
//===----------------------------------------------------------------------===//

std::unique_ptr<Pass> createExpandPEArrayToMacPass();
void registerExpandPEArrayToMacPass();

//===----------------------------------------------------------------------===//
// Stage 4: FPGA tiling (fixed 4x4 runtime, no new hardware)
//   把任意静态形状的 linalg.matmul lowering 成呼叫已烧录好的 4x4 FPGA
//   runtime(fpga_matmul_tiled_auto)。
//===----------------------------------------------------------------------===//

std::unique_ptr<Pass> createTileMatmulForFpgaPass();
void registerTileMatmulForFpgaPass();

//===----------------------------------------------------------------------===//
// Stage 5: Conv2D on the same FPGA runtime
//   把 batch=1、静态形状的 linalg.conv_2d_nhwc_hwcf lowering 成呼叫
//   im2col + 4x4 matmul runtime 的调用(fpga_conv2d_im2col_auto),
//   复用同一颗矩阵乘法加速器,不需要新硬件。
//===----------------------------------------------------------------------===//

std::unique_ptr<Pass> createConv2DToFpgaPass();
void registerConv2DToFpgaPass();

//===----------------------------------------------------------------------===//
// Registration entry point (called once from systolic-opt's main)
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Stage 6: Executable scheduling output
//   - TileToFpga: 把已排程(帶 device 屬性)的 systolic.matmul_tile lowering
//                 成 dispatch runtime 呼叫,device 映射為 device id。
//===----------------------------------------------------------------------===//

std::unique_ptr<Pass> createSystolicTileToFpgaPass();
void registerSystolicTileToFpgaPass();

void registerSystolicPasses();

} // namespace systolic
} // namespace mlir

#endif // SYSTOLIC_PASSES_H