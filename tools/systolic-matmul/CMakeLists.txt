# systolic-matmul: C = A x B from stdin, compiled by the systolic pipeline,
# JIT-run, and executed on the FPGA through the runtime in runtime/.
#
# Needs MLIR's ExecutionEngine (built whenever MLIR is built with the host
# target). Without it everything else still builds; only this tool is
# skipped.
if(NOT TARGET MLIRExecutionEngine OR NOT TARGET mlir_c_runner_utils)
  message(WARNING "MLIRExecutionEngine / mlir_c_runner_utils not found in "
                  "this MLIR build: not building systolic-matmul")
  return()
endif()

set(LLVM_LINK_COMPONENTS
  Core
  Support
  OrcJIT
  nativecodegen
  native
)

add_llvm_executable(systolic-matmul systolic-matmul.cpp)
llvm_update_compile_flags(systolic-matmul)

target_link_libraries(systolic-matmul PRIVATE
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

mlir_check_all_link_libraries(systolic-matmul)
