#include "Systolic/Passes.h"
#include "Systolic/SystolicJobCodegen.h"

#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Operation.h"
#include "mlir/Pass/Pass.h"

#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/raw_ostream.h"

using namespace mlir;

namespace {

static IntegerAttr getI64Attr(Operation *op, StringRef name, int64_t fallback) {
  if (auto attr = op->getAttrOfType<IntegerAttr>(name))
    return attr;
  return IntegerAttr::get(IntegerType::get(op->getContext(), 64), fallback);
}

struct SystolicJobCodegenPass
    : public PassWrapper<SystolicJobCodegenPass,
                        OperationPass<ModuleOp>> {

  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(SystolicJobCodegenPass)

  StringRef getArgument() const final {
    return "systolic-job-codegen";
  }

  StringRef getDescription() const final {
    return "Emit compiler-side runtime descriptors for systolic jobs";
  }

  void runOnOperation() override {
    ModuleOp module = getOperation();
    MLIRContext *ctx = module.getContext();

    SmallVector<Attribute, 16> jobs;

    module.walk([&](Operation *op) {
      if (op->getName().getStringRef() != "systolic.matmul_tile")
        return;

      // ---------------------------------------------------------------
      // Runtime identity / scheduling metadata.
      // ---------------------------------------------------------------

      auto jobID =
          getI64Attr(op, "job_id", static_cast<int64_t>(jobs.size()));

      auto deviceID =
          getI64Attr(op, "device_id", 0);

      auto startCycle =
          getI64Attr(op, "start_cycle", 0);

      auto estCycles =
          getI64Attr(op, "est_cycles", 0);

      // ---------------------------------------------------------------
      // Tile shape.
      //
      // Prefer explicit m/n/k attributes.  These are the fields that
      // eventually become job_m/job_n/job_k in systolic_dma_top.
      // ---------------------------------------------------------------

      auto m = getI64Attr(op, "m", 0);
      auto n = getI64Attr(op, "n", 0);
      auto k = getI64Attr(op, "k", 0);

      // ---------------------------------------------------------------
      // Physical memory ABI.
      //
      // These are intentionally explicit attributes.  Later the host
      // runtime will replace them with actual allocated device buffers.
      // ---------------------------------------------------------------

      auto aBase =
          getI64Attr(op, "a_base", 0);

      auto bBase =
          getI64Attr(op, "b_base", 0);

      auto cBase =
          getI64Attr(op, "c_base", 0);

      NamedAttrList job;

      job.set("job_id", jobID);
      job.set("device_id", deviceID);

      job.set("m", m);
      job.set("n", n);
      job.set("k", k);

      job.set("start_cycle", startCycle);
      job.set("est_cycles", estCycles);

      job.set("a_base", aBase);
      job.set("b_base", bBase);
      job.set("c_base", cBase);

      jobs.push_back(DictionaryAttr::get(ctx, job));

      // Mark the operation so later lowering can distinguish it from
      // an ordinary systolic.matmul_tile.
      op->setAttr(
          "systolic.job_codegen",
          UnitAttr::get(ctx));
    });

    // The compiler's runtime ABI is now represented explicitly at the
    // module boundary.
    module->setAttr(
        "systolic.runtime.jobs",
        ArrayAttr::get(ctx, jobs));

    llvm::errs()
        << "[systolic-job-codegen] emitted "
        << jobs.size()
        << " runtime job descriptor(s)\n";
  }
};

} // namespace

std::unique_ptr<Pass> mlir::systolic::createSystolicJobCodegenPass() {
  return std::make_unique<SystolicJobCodegenPass>();
}

void mlir::systolic::registerSystolicJobCodegenPass() {
  PassRegistration<SystolicJobCodegenPass>();
}
