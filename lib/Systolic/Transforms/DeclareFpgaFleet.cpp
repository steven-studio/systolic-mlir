//===- DeclareFpgaFleet.cpp - Declare the 1x8x8 + 3x4x4 FPGA fleet --------===//
//
// --systolic-declare-fpga-fleet="k-max-8x8=N k-max-4x4=M pipeline-depth=D":
// add the systolic.device declarations of the FPGA's bitstream to a module
// that has none -- the four arrays systolic-matmul declares for the linalg
// path (Systolic/FpgaFleet.h), under the symbols --lower-systolic-matmul-
// tile-to-fpga maps onto the RTL's device IDs 0..3, pipelined (D > 1, the
// bitstream's PIPELINE_DEPTH) or serialized (D = 1). A frontend's output
// (C -> MLIR) knows nothing about the target; this is where the target comes
// in, so the same input can be compiled for the board without editing it.
//
//===----------------------------------------------------------------------===//

#include "Systolic/FpgaFleet.h"
#include "Systolic/Passes.h"
#include "Systolic/SystolicDialect.h"
#include "Systolic/SystolicOps.h"

#include "mlir/IR/BuiltinOps.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Pass/Pass.h"

using namespace mlir;
using namespace mlir::systolic;

namespace {

struct DeclareFpgaFleetPass
    : public PassWrapper<DeclareFpgaFleetPass, OperationPass<ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(DeclareFpgaFleetPass)

  DeclareFpgaFleetPass() = default;
  DeclareFpgaFleetPass(const DeclareFpgaFleetPass &other)
      : PassWrapper(other) {}

  StringRef getArgument() const final {
    return "systolic-declare-fpga-fleet";
  }
  StringRef getDescription() const final {
    return "Declare the systolic.device fleet of the 1x8x8 + 3x4x4 FPGA "
           "bitstream (the declarations systolic-matmul uses)";
  }

  // The loaded bitstream's K_MAX_8X8 / K_MAX_4X4, as for
  // --lower-systolic-matmul-tile-to-fpga (phys-k-max-*), which checks that
  // the declared k_max matches.
  Option<int64_t> kMax8x8{*this, "k-max-8x8",
                          llvm::cl::desc("k_max of the 8x8 accelerator"),
                          llvm::cl::init(16)};
  Option<int64_t> kMax4x4{*this, "k-max-4x4",
                          llvm::cl::desc("k_max of each 4x4 accelerator"),
                          llvm::cl::init(16)};
  // PIPELINE_DEPTH of the bitstream's record (the context's job queue,
  // JOB_QUEUE_DEPTH): more than 1 declares the pipelined contexts -- the
  // per-accelerator timeline of CostModel.h with FpgaFleet.h's pipeline
  // depth, reduce_cycles and feed_gap -- exactly as systolic-matmul does
  // from the same record; 1 (the archived bitstream, which records no
  // depth) keeps the serialized contexts.
  Option<int64_t> pipelineDepth{
      *this, "pipeline-depth",
      llvm::cl::desc("PIPELINE_DEPTH of the bitstream; > 1 declares the "
                     "pipelined contexts (FpgaFleet.h), 1 the serialized "
                     "ones"),
      llvm::cl::init(1)};

  void getDependentDialects(DialectRegistry &registry) const override {
    registry.insert<SystolicDialect>();
  }

  void runOnOperation() override {
    ModuleOp module = getOperation();

    bool hasDevice = false;
    module.walk([&](DeviceOp) { hasDevice = true; });
    if (hasDevice) {
      module.emitError("the module already declares systolic.device ops; "
                       "--systolic-declare-fpga-fleet would add a second "
                       "fleet");
      signalPassFailure();
      return;
    }

    if (pipelineDepth < 1) {
      module.emitError("--systolic-declare-fpga-fleet: pipeline-depth must "
                       "be >= 1, got ")
          << pipelineDepth.getValue();
      signalPassFailure();
      return;
    }
    const bool pipelined = pipelineDepth > 1;

    std::string text;
    auto device = [&](const char *symbol, int64_t edge, int64_t kMax) {
      text += ::systolic::fpga::deviceDeclaration(symbol, edge, kMax,
                                                  pipelined);
      text += "\n";
    };
    device("acc_8x8_0", 8, kMax8x8);
    for (int64_t i = 0; i < ::systolic::fpga::kNum4x4; ++i)
      device(("acc_4x4_" + std::to_string(i)).c_str(), 4, kMax4x4);

    Block declarations;
    ParserConfig config(module.getContext());
    if (failed(parseSourceString(text, &declarations, config))) {
      module.emitError("could not build the FPGA fleet declarations");
      signalPassFailure();
      return;
    }

    Block *body = module.getBody();
    body->getOperations().splice(body->begin(),
                                 declarations.getOperations());
  }
};

} // namespace

void mlir::systolic::registerDeclareFpgaFleetPass() {
  PassRegistration<DeclareFpgaFleetPass>();
}
