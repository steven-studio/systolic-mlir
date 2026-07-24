add_mlir_dialect_library(MLIRSystolic
  SystolicDialect.cpp
  SystolicOps.cpp
  CostModel.cpp
  SystolicCostAnalysisPass.cpp

  ADDITIONAL_HEADER_DIRS
  ${PROJECT_SOURCE_DIR}/include/Systolic

  DEPENDS
  MLIRSystolicOpsIncGen
  SystolicOpsEnumsIncGen

  LINK_LIBS PUBLIC
  MLIRIR
  MLIRPass
  MLIRSupport
  MLIRDialect
  MLIRFuncDialect
)
