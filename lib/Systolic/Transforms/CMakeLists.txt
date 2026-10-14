add_mlir_dialect_library(MLIRSystolicTransforms
  AffineMatmulToSystolic.cpp
  AffineMatmulAnalysis.cpp
  AffineMatmulPartitioning.cpp
  SystolicScheduling.cpp
  SystolicTilePlan.cpp
  SystolicOptimizer.cpp
  SystolicMaterializeSchedulePass.cpp
  SystolicBinarySearchTileMatmulPass.cpp
  TestAffineMatmulDiscovery.cpp
  ConvertMatmulToSystolic.cpp
  ExpandPEArrayToMac.cpp
  TileMatmulForFpga.cpp
  Conv2DToFpga.cpp
  CostModel.cpp
  DeclareFpgaFleet.cpp
  DeviceConfig.cpp
  SystolicCostAnalysisPass.cpp
  SystolicSelectDevicePass.cpp
  SystolicTileMatmulPass.cpp
  SystolicScheduleOverlapPass.cpp
  SystolicPipelines.cpp

  ADDITIONAL_HEADER_DIRS
  ${PROJECT_SOURCE_DIR}/include/Systolic

  DEPENDS
  MLIRSystolicOpsIncGen

  LINK_LIBS PUBLIC
  MLIRSystolic
  MLIRAffineDialect
  MLIRAffineAnalysis
  MLIRAffineUtils
  MLIRLinalgDialect
  MLIRSCFDialect
  MLIRTensorDialect
  MLIRArithDialect
  MLIRMemRefDialect
  MLIRBufferizationDialect
  MLIRLLVMDialect
  MLIRPass
  MLIRParser
  MLIRTransformUtils

  # SystolicPipelines.cpp: bufferization + conversion to the LLVM dialect.
  MLIRBufferizationTransforms
  MLIRBufferizationPipelines
  MLIRBufferizationToMemRef
  MLIRFuncTransforms
  MLIRTensorTransforms
  MLIRMemRefTransforms
  MLIRControlFlowDialect
  MLIRMemRefToLLVM
  MLIRArithToLLVM
  MLIRIndexToLLVM
  MLIRFuncToLLVM
  MLIRAffineToStandard
  MLIRSCFToControlFlow
  MLIRControlFlowToLLVM
  MLIRReconcileUnrealizedCasts
)
