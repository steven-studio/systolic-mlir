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
//     --systolic-runtime-to-llvm               bufferize, lower to LLVM
//   LLVM dialect
//
//===----------------------------------------------------------------------===//

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
  // contiguous row-major memrefs, which is what a C caller passes. Tile
  // operands that are strided slices are copied into contiguous buffers
  // here, because the runtime takes raw row-major pointers.
  bufferization::OneShotBufferizePassOptions bufferize;
  bufferize.bufferizeFunctionBoundaries = true;
  bufferize.functionBoundaryTypeConversion =
      bufferization::LayoutMapOption::IdentityLayoutMap;
  pm.addPass(bufferization::createOneShotBufferizePass(bufferize));

  // Free the contiguous staging copies once their tile has been dispatched.
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

void mlir::systolic::buildSystolicFpgaPipeline(OpPassManager &pm) {
  pm.addPass(createSystolicBinarySearchTileMatmulPass());
  pm.addPass(createTileMatmulForFpgaPass());
  buildSystolicRuntimeToLLVMPipeline(pm);
}

void mlir::systolic::registerSystolicPipelines() {
  PassPipelineRegistration<>(
      "systolic-runtime-to-llvm",
      "Bufferize FPGA-runtime-lowered IR and convert it to the LLVM dialect",
      buildSystolicRuntimeToLLVMPipeline);

  PassPipelineRegistration<>(
      "systolic-fpga-pipeline",
      "linalg.matmul -> systolic.matmul_tile -> FPGA runtime calls -> LLVM "
      "dialect (binary-search decomposition, K-folding, FPGA lowering, "
      "bufferization)",
      buildSystolicFpgaPipeline);
}
