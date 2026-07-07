set(LLVM_LINK_COMPONENTS Support)

add_llvm_executable(systolic-opt systolic-opt.cpp)

llvm_update_compile_flags(systolic-opt)
target_link_libraries(systolic-opt PRIVATE
  MLIRSystolic
  MLIRSystolicTransforms

  # 标准 mlir-opt 需要的东西:让 systolic-opt 也能吃/印一般的 MLIR
  # (func, linalg, arith, tensor ...)
  MLIROptLib
  MLIRIR
  MLIRParser
  MLIRPass
  MLIRSupport
  MLIRTransforms
)

mlir_check_all_link_libraries(systolic-opt)
