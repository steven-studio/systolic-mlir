add_mlir_dialect_library(MLIRSystolicTransforms
  AffineMatmulToSystolic.cpp
  AffineMatmulAnalysis.cpp
  AffineMatmulPartitioning.cpp
  SystolicTiling.cpp
  SystolicComputeCost.cpp
  TestAffineMatmulDiscovery.cpp
  ConvertMatmulToSystolic.cpp
  CostModel.cpp
  SystolicCostAnalysisPass.cpp
  SystolicSelectDevicePass.cpp
  SystolicTileMatmulPass.cpp
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
