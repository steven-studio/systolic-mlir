# -----------------------------------------------------------------------
# 这里的 add_mlir_dialect() 是 MLIR 提供的 CMake 宏,
# 会自动跑 mlir-tblgen 产生:
#   SystolicDialect.h.inc / .cpp.inc  (dialect 本身)
#   SystolicOps.h.inc     / .cpp.inc  (op 的 class 定义)
#   SystolicOpsEnums.h.inc/ .cpp.inc  (StationaryKind / StreamDirection enum)
# -----------------------------------------------------------------------
add_mlir_dialect(SystolicOps systolic)

# 额外产生 enum 的 decls/defs(add_mlir_dialect 预设不会自动产生 enum)
set(LLVM_TARGET_DEFINITIONS SystolicOps.td)
mlir_tablegen(SystolicOpsEnums.h.inc -gen-enum-decls)
mlir_tablegen(SystolicOpsEnums.cpp.inc -gen-enum-defs)
add_public_tablegen_target(SystolicOpsEnumsIncGen)
