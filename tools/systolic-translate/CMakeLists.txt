set(LLVM_LINK_COMPONENTS Support)

add_llvm_executable(systolic-translate systolic-translate.cpp)

llvm_update_compile_flags(systolic-translate)
target_link_libraries(systolic-translate PRIVATE
  MLIRSystolic
  MLIRIR
  MLIRParser
  MLIRSupport
  MLIRFuncDialect
  MLIRSCFDialect
  MLIRTensorDialect
  MLIRArithDialect
  MLIRLinalgDialect
)

mlir_check_all_link_libraries(systolic-translate)
