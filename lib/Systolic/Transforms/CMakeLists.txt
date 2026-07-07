add_mlir_dialect_library(MLIRSystolicTransforms
  ConvertMatmulToSystolic.cpp
  ExpandPEArrayToMac.cpp

  ADDITIONAL_HEADER_DIRS
  ${PROJECT_SOURCE_DIR}/include/Systolic

  DEPENDS
  MLIRSystolicOpsIncGen

  LINK_LIBS PUBLIC
  MLIRSystolic
  MLIRLinalgDialect
  MLIRSCFDialect
  MLIRTensorDialect
  MLIRArithDialect
  MLIRPass
  MLIRTransformUtils
)
