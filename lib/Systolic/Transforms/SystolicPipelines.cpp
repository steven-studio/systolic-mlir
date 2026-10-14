//===- SystolicPipelines.cpp - The Linalg -> FPGA pipeline ----------------===//
//
// One definition of the pass sequence that takes a linalg.matmul to LLVM
// dialect code calling the FPGA runtime, shared by systolic-opt
// (--systolic-fpga-pipeline) and systolic-matmul, so the command-line
// checkpoints and the executable cannot drift apart.
//
//   linalg.matmul + systolic.device fleet
//     --systolic-binary-search-tile-matmul     decomposition, device
//                                              assignment, schedule, K-fold
//   systolic.matmul_tile ... on @acc_* est_cycles start_cycle
//     --lower-systolic-matmul-tile-to-fpga     one runtime call per tile
//   llvm.call @fpga_matmul_tiled_auto_scheduled(m, k, n, A, B, C, dev, ...)
//     (async=true: @fpga_gemm_begin / @fpga_tile_submit / @fpga_gemm_wait)
//     --systolic-runtime-to-llvm               bufferize, lower to LLVM
//   LLVM dialect
//
//===----------------------------------------------------------------------===//

#include "Systolic/FpgaFleet.h"
#include "Systolic/Passes.h"
#include "Systolic/SystolicDialect.h"

#include "llvm/Config/llvm-config.h"

#include "mlir/Conversion/Passes.h"
#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Bufferization/IR/Bufferization.h"
#include "mlir/Dialect/Bufferization/Pipelines/Passes.h"
#if LLVM_VERSION_MAJOR >= 24
#include "mlir/Dialect/Bufferization/Transforms/BufferizableOpInterfaceImpl.h"
#endif
#include "mlir/Dialect/Bufferization/Transforms/FuncBufferizableOpInterfaceImpl.h"
#include "mlir/Dialect/Bufferization/Transforms/Passes.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlow.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/Dialect/Linalg/IR/Linalg.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/MemRef/Transforms/Passes.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/Dialect/Tensor/IR/Tensor.h"
#include "mlir/Dialect/Tensor/Transforms/BufferizableOpInterfaceImpl.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Pass/PassRegistry.h"

using namespace mlir;

void mlir::systolic::registerSystolicPipelineDialects(
    DialectRegistry &registry) {
  registry.insert<SystolicDialect, func::FuncDialect, linalg::LinalgDialect,
                  arith::ArithDialect, tensor::TensorDialect, scf::SCFDialect,
                  cf::ControlFlowDialect, affine::AffineDialect,
                  memref::MemRefDialect,
                  bufferization::BufferizationDialect, LLVM::LLVMDialect>();

  tensor::registerBufferizableOpInterfaceExternalModels(registry);
#if LLVM_VERSION_MAJOR >= 24
  // LLVM 24 moved the bufferization dialect's own models out of the dialect.
  bufferization::registerBufferizableOpInterfaceExternalModels(registry);
#endif
  bufferization::func_ext::registerBufferizableOpInterfaceExternalModels(
      registry);
}

void mlir::systolic::buildSystolicRuntimeToLLVMPipeline(OpPassManager &pm) {
  // The entry point takes and returns tensors, so bufferization has to
  // cross the function boundary. Identity layouts keep the arguments plain
  // contiguous row-major memrefs, which is what a C caller passes. The FPGA
  // lowering has already staged the tiles of A, B and C through scratch
  // tiles (TileMatmulForFpga.cpp); what is left to bufferize is the function
  // boundary and any operand that lowering could not stage.
  bufferization::OneShotBufferizePassOptions bufferize;
  bufferize.bufferizeFunctionBoundaries = true;
  bufferize.functionBoundaryTypeConversion =
      bufferization::LayoutMapOption::IdentityLayoutMap;
  pm.addPass(bufferization::createOneShotBufferizePass(bufferize));

  // Free what bufferization allocated -- copies of operands that were not
  // staged -- at the end of the function; the result handed back to the
  // caller is the caller's to free.
  bufferization::buildBufferDeallocationPipeline(pm);
  pm.addPass(createConvertBufferizationToMemRefPass());

  pm.addPass(memref::createExpandStridedMetadataPass());
  pm.addPass(createLowerAffinePass());
  pm.addPass(createFinalizeMemRefToLLVMConversionPass());
  pm.addPass(createSCFToControlFlowPass());
  pm.addPass(createArithToLLVMConversionPass());
  pm.addPass(createConvertIndexToLLVMPass());
  pm.addPass(createConvertControlFlowToLLVMPass());
  pm.addPass(createConvertFuncToLLVMPass());
  pm.addPass(createReconcileUnrealizedCastsPass());
}

void mlir::systolic::buildSystolicFpgaPipeline(OpPassManager &pm,
                                               int64_t physKMax8x8,
                                               int64_t physKMax4x4,
                                               double sharedReadBytesPerCycle,
                                               bool async) {
  pm.addPass(createSystolicBinarySearchTileMatmulPass(sharedReadBytesPerCycle));
  pm.addPass(createTileMatmulForFpgaPass(physKMax8x8, physKMax4x4, async));
  buildSystolicRuntimeToLLVMPipeline(pm);
}

namespace {
// --systolic-fpga-pipeline="phys-k-max-8x8=N phys-k-max-4x4=M": the loaded
// bitstream's K_MAX_8X8 and K_MAX_4X4, forwarded to the FPGA lowering.
// shared-read-words-per-cycle: the operand words per cycle the four arrays'
// fills share through the one AXI read port, forwarded to the binary search
// in bytes; by default the fleet's reference (Systolic/FpgaFleet.h), 0 for
// independent fills.
struct SystolicFpgaPipelineOptions
    : public PassPipelineOptions<SystolicFpgaPipelineOptions> {
  Option<int64_t> physKMax8x8{
      *this, "phys-k-max-8x8",
      llvm::cl::desc("K_MAX_8X8 of the loaded bitstream"), llvm::cl::init(16)};
  Option<int64_t> physKMax4x4{
      *this, "phys-k-max-4x4",
      llvm::cl::desc("K_MAX_4X4 of the loaded bitstream"), llvm::cl::init(16)};
  Option<double> sharedReadWordsPerCycle{
      *this, "shared-read-words-per-cycle",
      llvm::cl::desc("Operand words (4 B) per cycle all arrays' fills share "
                     "(default: the fleet's reference, 3.51; 0: each array "
                     "fills independently)"),
      llvm::cl::init(::systolic::fpga::kSharedReadWordsPerCycle)};
  Option<bool> async{
      *this, "async",
      llvm::cl::desc("Lower to the asynchronous runtime (fpga_gemm_begin / "
                     "fpga_tile_submit / fpga_gemm_wait) instead of one "
                     "synchronous call per tile"),
      llvm::cl::init(false)};
};
} // namespace

void mlir::systolic::registerSystolicPipelines() {
  PassPipelineRegistration<>(
      "systolic-runtime-to-llvm",
      "Bufferize FPGA-runtime-lowered IR and convert it to the LLVM dialect",
      buildSystolicRuntimeToLLVMPipeline);

  PassPipelineRegistration<SystolicFpgaPipelineOptions>(
      "systolic-fpga-pipeline",
      "linalg.matmul -> systolic.matmul_tile -> FPGA runtime calls -> LLVM "
      "dialect (binary-search decomposition, K-folding, FPGA lowering, "
      "bufferization)",
      [](OpPassManager &pm, const SystolicFpgaPipelineOptions &options) {
        buildSystolicFpgaPipeline(
            pm, options.physKMax8x8, options.physKMax4x4,
            ::systolic::fpga::sharedReadBytesPerCycle(
                options.sharedReadWordsPerCycle),
            options.async);
      });
}
