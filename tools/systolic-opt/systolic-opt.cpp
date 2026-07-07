#include "Systolic/Passes.h"
#include "Systolic/SystolicDialect.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/Linalg/IR/Linalg.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/Dialect/Tensor/IR/Tensor.h"
#include "mlir/IR/Dialect.h"
#include "mlir/Tools/mlir-opt/MlirOptMain.h"

int main(int argc, char **argv) {
  mlir::DialectRegistry registry;
  registry.insert<mlir::systolic::SystolicDialect, mlir::func::FuncDialect,
                   mlir::linalg::LinalgDialect, mlir::arith::ArithDialect,
                   mlir::tensor::TensorDialect, mlir::scf::SCFDialect>();

  mlir::systolic::registerSystolicPasses();

  return mlir::asMainReturnCode(mlir::MlirOptMain(
      argc, argv, "Systolic dialect optimizer driver\n", registry));
}
