#ifndef SYSTOLIC_PASSES_H
#define SYSTOLIC_PASSES_H

#include "mlir/Pass/Pass.h"

namespace mlir {
class DialectRegistry;
class OpPassManager;

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

std::unique_ptr<Pass> createSystolicBinarySearchTileMatmulPass();
/// The same pass with shared-read-bytes-per-cycle set: the operand bandwidth
/// all accelerators' fills share (0: each fills at its own rate).
std::unique_ptr<Pass>
createSystolicBinarySearchTileMatmulPass(double sharedReadBytesPerCycle);
void registerSystolicBinarySearchTileMatmulPass();

std::unique_ptr<Pass> createSystolicMaterializeSchedulePass();
void registerSystolicMaterializeSchedulePass();

std::unique_ptr<Pass> createAffineMatmulToSystolicPass();
void registerAffineMatmulToSystolicPass();

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
// Stage 4: FPGA lowering (--lower-systolic-matmul-tile-to-fpga)
//   把已指派 device、已排程的 systolic.matmul_tile(一个 tile = 该 device
//   的一次 invocation:m<=rows、n<=cols、k<=k_max)lowering 成呼叫
//   runtime/fpga_matmul_tiled.cpp 的 fpga_matmul_tiled_auto_scheduled,
//   device symbol 对应 1x8x8 + 3x4x4 bitstream 的实体 device id。
//===----------------------------------------------------------------------===//

std::unique_ptr<Pass> createTileMatmulForFpgaPass();
/// The same pass for a bitstream whose 8x8 holds physKMax8x8 and whose 4x4s
/// hold physKMax4x4 reduction steps per invocation (K_MAX_8X8, K_MAX_4X4).
std::unique_ptr<Pass> createTileMatmulForFpgaPass(int64_t physKMax8x8,
                                                  int64_t physKMax4x4);
/// ... and, with async, emitting the asynchronous runtime's calls
/// (fpga_gemm_begin / fpga_tile_submit / fpga_gemm_wait, docs/ASYNC.md)
/// instead of one synchronous call per tile.
std::unique_ptr<Pass> createTileMatmulForFpgaPass(int64_t physKMax8x8,
                                                  int64_t physKMax4x4,
                                                  bool async);
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

void registerTestAffineMatmulDiscoveryPass();

//===----------------------------------------------------------------------===//
// The Linalg -> FPGA pipeline (SystolicPipelines.cpp), shared by systolic-opt
// and systolic-matmul:
//
//   --systolic-fpga-pipeline   = --systolic-binary-search-tile-matmul
//                                --lower-systolic-matmul-tile-to-fpga
//                                --systolic-runtime-to-llvm
//   --systolic-runtime-to-llvm = one-shot bufferization across the function
//                                boundary (identity layouts) + conversion of
//                                memref/arith/index/cf/func to LLVM
//===----------------------------------------------------------------------===//

void buildSystolicRuntimeToLLVMPipeline(OpPassManager &pm);
/// physKMax8x8 / physKMax4x4: the loaded bitstream's K_MAX_8X8 / K_MAX_4X4,
/// handed to the FPGA lowering. sharedReadBytesPerCycle: the operand
/// bandwidth the arrays' fills share, handed to the binary search (0: each
/// array fills independently). --systolic-fpga-pipeline defaults them to
/// 16, 16 (the archived bitstream) and the fleet's shared read reference
/// (Systolic/FpgaFleet.h).
/// async: the FPGA lowering targets the asynchronous runtime
/// (--systolic-fpga-pipeline="async=true", docs/ASYNC.md).
void buildSystolicFpgaPipeline(OpPassManager &pm, int64_t physKMax8x8,
                               int64_t physKMax4x4,
                               double sharedReadBytesPerCycle,
                               bool async = false);
void registerSystolicPipelines();

/// Dialects and bufferization external models the pipeline needs.
void registerSystolicPipelineDialects(DialectRegistry &registry);

void registerSystolicPasses();

} // namespace systolic
} // namespace mlir

#endif // SYSTOLIC_PASSES_H