#include "Systolic/Passes.h"
#include "Systolic/SystolicDialect.h"

#include "mlir/Conversion/Passes.h"

#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Bufferization/IR/Bufferization.h"
#include "mlir/Dialect/Bufferization/Transforms/BufferizableOpInterfaceImpl.h"
#include "mlir/Dialect/Bufferization/Transforms/FuncBufferizableOpInterfaceImpl.h"
#include "mlir/Dialect/Bufferization/Transforms/Passes.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/Linalg/IR/Linalg.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/MemRef/Transforms/Passes.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/Dialect/Tensor/IR/Tensor.h"
#include "mlir/Dialect/Tensor/Transforms/BufferizableOpInterfaceImpl.h"
#include "mlir/IR/Dialect.h"
#include "mlir/Tools/mlir-opt/MlirOptMain.h"

int main(int argc, char **argv) {
  mlir::DialectRegistry registry;
  registry.insert<mlir::systolic::SystolicDialect, mlir::func::FuncDialect,
                   mlir::linalg::LinalgDialect, mlir::arith::ArithDialect,
                   mlir::tensor::TensorDialect, mlir::scf::SCFDialect,
                   mlir::affine::AffineDialect, mlir::memref::MemRefDialect,
                   mlir::bufferization::BufferizationDialect,
                   mlir::LLVM::LLVMDialect>();

  mlir::tensor::registerBufferizableOpInterfaceExternalModels(registry);
  mlir::bufferization::registerBufferizableOpInterfaceExternalModels(registry);
  mlir::bufferization::func_ext::registerBufferizableOpInterfaceExternalModels(
      registry);

  mlir::systolic::registerSystolicPasses();
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
