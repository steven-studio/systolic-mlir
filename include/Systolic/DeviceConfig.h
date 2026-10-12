#ifndef SYSTOLIC_DEVICECONFIG_H
#define SYSTOLIC_DEVICECONFIG_H

// systolic.device -> the cost model's ArrayConfig, in one place. Every pass
// that costs work on a device reads its attributes through this function, so
// an attribute the cost model learns is seen by all of them at once.

#include "Systolic/CostModel.h"
#include "Systolic/SystolicOps.h"

namespace mlir {
namespace systolic {

/// Every cost-relevant attribute of `device`. An attribute the device does
/// not declare keeps the ArrayConfig default: no k_max (one invocation per
/// reduction), H = 0, no read or write term.
::systolic::ArrayConfig getArrayConfig(DeviceOp device);

} // namespace systolic
} // namespace mlir

#endif // SYSTOLIC_DEVICECONFIG_H
