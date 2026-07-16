add_mlir_dialect_library(MLIRSystolicTransforms
  ConvertMatmulToSystolic.cpp
  ExpandPEArrayToMac.cpp
  TileMatmulForFpga.cpp
  Conv2DToFpga.cpp
  Conv2DNchwToFpga.cpp

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
  MLIRMemRefDialect
  MLIRBufferizationDialect
  MLIRLLVMDialect
  MLIRPass
  MLIRTransformUtils
)
