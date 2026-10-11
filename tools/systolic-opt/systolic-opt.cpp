#include "Systolic/Passes.h"
#include "Systolic/SystolicDialect.h"

#include "mlir/Conversion/Passes.h"

#include "mlir/Dialect/Bufferization/Transforms/Passes.h"
#include "mlir/Dialect/MemRef/Transforms/Passes.h"
#include "mlir/IR/Dialect.h"
#include "mlir/Tools/mlir-opt/MlirOptMain.h"

int main(int argc, char **argv) {
  mlir::DialectRegistry registry;
  // The same dialects and bufferization models systolic-matmul uses, so a
  // pipeline that runs here runs there.
  mlir::systolic::registerSystolicPipelineDialects(registry);

  mlir::systolic::registerSystolicPasses();

  // Upstream passes, so each stage of --systolic-runtime-to-llvm can also be
  // run on its own from the command line.
  mlir::bufferization::registerBufferizationPasses();
  mlir::memref::registerExpandOpsPass();
  mlir::memref::registerExpandStridedMetadataPass();
  mlir::registerFinalizeMemRefToLLVMConversionPass();
  mlir::registerArithToLLVMConversionPass();
  mlir::registerConvertIndexToLLVMPass();
  mlir::registerConvertFuncToLLVMPass();
  mlir::registerLowerAffinePass();
  mlir::registerSCFToControlFlowPass();
  mlir::registerConvertControlFlowToLLVMPass();
  mlir::registerReconcileUnrealizedCastsPass();

  return mlir::asMainReturnCode(mlir::MlirOptMain(
      argc, argv, "Systolic dialect optimizer driver\n", registry));
}
