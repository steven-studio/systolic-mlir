#ifndef SYSTOLIC_SYSTOLICCOSTANALYSISPASS_H
#define SYSTOLIC_SYSTOLICCOSTANALYSISPASS_H

#include <memory>

namespace mlir {
class Pass;

namespace systolic {

// Walks the module, resolves each systolic.matmul_tile / systolic.dma op's
// `device` symbol reference against the module's systolic.device
// declarations, and annotates the op with an `est_cycles` attribute using
// the closed-form cost model in lib/Systolic/CostModel.h.
//
// Ops with no `device` attribute yet (i.e. not yet assigned by a
// selection/scheduling pass) are left untouched -- this pass only costs
// what has already been decided, it does not itself choose a device.
std::unique_ptr<Pass> createSystolicCostAnalysisPass();

} // namespace systolic
} // namespace mlir

#endif // SYSTOLIC_SYSTOLICCOSTANALYSISPASS_H
