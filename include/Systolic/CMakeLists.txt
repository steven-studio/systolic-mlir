# NOTE: `add_mlir_dialect` / `add_mlir_doc` macro names have shifted slightly
# across LLVM releases in the past. This mirrors the layout used by MLIR's
# own out-of-tree "standalone" dialect example. If your LLVM version renamed
# something, mlir-tblgen's error will point directly at the fix.
# SystolicOps.td selects the Symbol trait spelling by this define; see the
# comment on Systolic_DeviceOp. LLVM_VERSION_MAJOR comes from LLVMConfig.cmake.
if(LLVM_VERSION_MAJOR GREATER_EQUAL 24)
  list(APPEND LLVM_TABLEGEN_FLAGS "-DSYSTOLIC_MLIR_SYMBOL_TRAITS")
endif()

add_mlir_dialect(SystolicOps systolic)
add_mlir_doc(SystolicDialect SystolicDialect Systolic/ -gen-dialect-doc)
add_mlir_doc(SystolicOps SystolicOps Systolic/ -gen-op-doc)

set(LLVM_TARGET_DEFINITIONS SystolicOps.td)
mlir_tablegen(SystolicOpsEnums.h.inc -gen-enum-decls)
mlir_tablegen(SystolicOpsEnums.cpp.inc -gen-enum-defs)
add_public_tablegen_target(SystolicOpsEnumsIncGen)