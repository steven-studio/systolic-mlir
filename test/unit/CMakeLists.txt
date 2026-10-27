# Mathematical unit tests for General Affine Tensor Contraction.
#
# Reuse the project's existing LLVM/MLIR linking configuration.

set(LLVM_LINK_COMPONENTS
  Core
  Support
  OrcJIT
  nativecodegen
  native
)

add_llvm_executable(
  tensor-space-dependencies-test
  TensorSpaceDependenciesTest.cpp
)

llvm_update_compile_flags(tensor-space-dependencies-test)

target_link_libraries(tensor-space-dependencies-test PRIVATE
  MLIRSystolic
  MLIRSystolicTransforms
  MLIRExecutionEngine
  MLIRExecutionEngineUtils
  MLIRBuiltinToLLVMIRTranslation
  MLIRLLVMToLLVMIRTranslation
  MLIRTargetLLVMIRExport
  MLIRParser
  MLIRPass
  MLIRIR
  MLIRSupport
  mlir_c_runner_utils
  systolic_fpga_runtime
)

mlir_check_all_link_libraries(tensor-space-dependencies-test)
