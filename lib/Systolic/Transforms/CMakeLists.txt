add_mlir_dialect_library(MLIRSystolicTransforms
  AffineMatmulAnalysis.cpp
  TestAffineMatmulDiscovery.cpp
  ConvertMatmulToSystolic.cpp
  ExpandPEArrayToMac.cpp
  TileMatmulForFpga.cpp
  Conv2DToFpga.cpp
  CostModel.cpp
  SystolicCostAnalysisPass.cpp
  SystolicSelectDevicePass.cpp
  SystolicTileMatmulPass.cpp
  SystolicTileToFpga.cpp
  SystolicScheduleOverlapPass.cpp

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
