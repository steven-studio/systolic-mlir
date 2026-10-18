#include "Systolic/DeviceConfig.h"

using namespace mlir;
using namespace mlir::systolic;

::systolic::ArrayConfig mlir::systolic::getArrayConfig(DeviceOp device) {
  ::systolic::ArrayConfig cfg;
  cfg.rows = device.getRows();
  cfg.cols = device.getCols();
  if (IntegerAttr km = device.getKMaxAttr())
    cfg.kMax = km.getInt();
  if (IntegerAttr l1 = device.getL1BytesAttr())
    cfg.l1Bytes = l1.getInt();
  if (FloatAttr clk = device.getClockHzAttr())
    cfg.clockHz = clk.getValueAsDouble();
  if (FloatAttr bw = device.getDmaBytesPerCycleAttr())
    cfg.dmaBytesPerCycle = bw.getValueAsDouble();
  // Calibration is a property of the microarchitecture, not of the array
  // shape, so it travels on the device op. Leaving the attribute off keeps
  // the ArrayConfig default (H = 0, the geometric model).
  if (IntegerAttr to = device.getTileOverheadAttr())
    cfg.tileOverhead = to.getInt();

  if (FloatAttr bw = device.getReadBytesPerCycleAttr())
    cfg.readBytesPerCycle = bw.getValueAsDouble();
  if (FloatAttr bw = device.getWriteBytesPerCycleAttr())
    cfg.writeBytesPerCycle = bw.getValueAsDouble();
  if (IntegerAttr c = device.getReadOverheadAttr())
    cfg.readOverhead = c.getInt();
  if (IntegerAttr c = device.getWriteOverheadAttr())
    cfg.writeOverhead = c.getInt();
  if (IntegerAttr lanes = device.getOperandLanesAttr())
    cfg.operandLanes = lanes.getInt();
  if (IntegerAttr window = device.getKWindowAttr())
    cfg.kWindow = window.getInt();

  // The pipelined per-accelerator timeline; absent (the verifier requires
  // all three or none) keeps the serialized model: pipelineDepth = 1.
  if (IntegerAttr depth = device.getPipelineDepthAttr())
    cfg.pipelineDepth = depth.getInt();
  if (IntegerAttr reduce = device.getReduceCyclesAttr())
    cfg.reduceCycles = reduce.getInt();
  if (IntegerAttr gap = device.getFeedGapAttr())
    cfg.feedGap = gap.getInt();
  return cfg;
}
