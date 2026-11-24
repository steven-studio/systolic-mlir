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
  PARTIAL_SOURCES_INTENDED
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

# Phase 5.2.1: symbolic source-to-logical coordinate relation.
add_llvm_executable(
  rank-partition-presburger-test
  PARTIAL_SOURCES_INTENDED
  RankPartitionPresburgerTest.cpp
  RankPartitionProductionPassRegistration.cpp
)
llvm_update_compile_flags(rank-partition-presburger-test)
target_link_libraries(rank-partition-presburger-test PRIVATE
  MLIRAffineDialect
  MLIRArithDialect
  MLIRMemRefDialect
  MLIRSCFDialect
  MLIRFuncDialect
  MLIRTensorDialect
  MLIRBufferizationDialect
  MLIRPresburger
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
mlir_check_all_link_libraries(rank-partition-presburger-test)

# The project-owned exact solver. It links the solver alone, with no MLIR
# library, so that nothing else can be what answers in this test.
add_executable(exact-solver-test ExactSolverTest.cpp)
llvm_update_compile_flags(exact-solver-test)
target_link_libraries(exact-solver-test PRIVATE SystolicExact)

# The rational solver, on the same terms.
add_executable(exact-rational-test ExactRationalTest.cpp)
llvm_update_compile_flags(exact-rational-test)
target_link_libraries(exact-rational-test PRIVATE SystolicExact)

# The integer solver, on the same terms.
add_executable(exact-integer-test ExactIntegerTest.cpp)
llvm_update_compile_flags(exact-integer-test)
target_link_libraries(exact-integer-test PRIVATE SystolicExact)
