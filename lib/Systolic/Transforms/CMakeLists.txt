add_mlir_dialect_library(MLIRSystolicTransforms
  AffineMatmulAnalysis.cpp
  TestAffineMatmulDiscovery.cpp
  ConvertMatmulToSystolic.cpp
  ExpandPEArrayToMac.cpp
  TileMatmulForFpga.cpp
  Conv2DToFpga.cpp
  Conv2DNchwToFpga.cpp
  BatchMatmulToFpga.cpp
  VecmatToFpga.cpp
  MatvecToFpga.cpp
  DotGenericToFpga.cpp

  ADDITIONAL_HEADER_DIRS
  ${PROJECT_SOURCE_DIR}/include/Systolic

  DEPENDS
  MLIRSystolicOpsIncGen

  LINK_LIBS PUBLIC
  MLIRSystolic
  MLIRAffineDialect
  MLIRAffineAnalysis
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
