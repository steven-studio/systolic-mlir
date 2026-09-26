#include "Systolic/AffineMatmulAnalysis.h"
#include "Systolic/AffineMatmulPartitioning.h"
#include "Systolic/Passes.h"

#include "mlir/IR/BuiltinOps.h"
#include "mlir/Pass/Pass.h"

#include "llvm/Support/raw_ostream.h"

using namespace mlir;
using namespace mlir::systolic;

namespace {

static void printLogicalExtent(StringRef name,
                               const LogicalExtent &extent) {
  llvm::errs() << "  " << name << " extent: ";

  switch (extent.kind) {
  case LogicalExtent::Kind::Unknown:
    llvm::errs() << "unknown";
    break;

  case LogicalExtent::Kind::Static:
    llvm::errs() << "static " << extent.staticValue;
    break;

  case LogicalExtent::Kind::Symbolic:
    llvm::errs() << "symbolic " << extent.symbolicValue;
    break;
  }

  llvm::errs() << "\n";
}

struct TestAffineMatmulDiscoveryPass
    : public PassWrapper<TestAffineMatmulDiscoveryPass,
                         OperationPass<ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(
      TestAffineMatmulDiscoveryPass)

  StringRef getArgument() const final {
    return "test-affine-matmul-discovery";
  }

  StringRef getDescription() const final {
    return "Test discovery of candidate affine matmul loop nests";
  }

  void runOnOperation() override {
    SmallVector<AffineMatmulCandidate> candidates;

    collectAffineMatmulCandidates(
        getOperation().getOperation(), candidates);

    llvm::errs() << "candidate count: "
                 << candidates.size()
                 << "\n";

    SmallVector<LogicalMatmul, 1> logicalMatmuls;

    for (AffineMatmulCandidate candidate : candidates) {
      auto result = recognizeLogicalMatmul(candidate);

      if (failed(result)) {
        llvm::errs() << "recognition result: failure\n";
        continue;
      }

      bool duplicate = false;
      for (const LogicalMatmul &existing : logicalMatmuls) {
        if (existing.outputStore == result->outputStore) {
          duplicate = true;
          break;
        }
      }

      if (!duplicate)
        logicalMatmuls.push_back(*result);
    }

    llvm::errs() << "unique logical matmul count: "
                 << logicalMatmuls.size()
                 << "\n";

    for (const LogicalMatmul &result : logicalMatmuls) {
      llvm::errs() << "recognition result: success\n";

      llvm::errs() << "  row map: ";
      result.rowMap.print(llvm::errs());
      llvm::errs() << "\n";

      llvm::errs() << "  column map: ";
      result.columnMap.print(llvm::errs());
      llvm::errs() << "\n";

      llvm::errs() << "  reduction map: ";
      result.reductionMap.print(llvm::errs());
      llvm::errs() << "\n";

      llvm::errs() << "  logical mapping:\n";
      for (unsigned row = 0;
           row < result.logicalMapping.transformation.size();
           ++row) {
        llvm::errs() << "    T[" << row << "] = [";
        const auto &coefficients =
            result.logicalMapping.transformation[row];

        for (unsigned column = 0;
             column < coefficients.size();
             ++column) {
          if (column != 0)
            llvm::errs() << " ";
          llvm::errs() << coefficients[column];
        }

        llvm::errs() << "]\n";
      }

      llvm::errs() << "    c = [";
      for (unsigned i = 0;
           i < result.logicalMapping.offset.size();
           ++i) {
        if (i != 0)
          llvm::errs() << " ";
        llvm::errs() << result.logicalMapping.offset[i];
      }
      llvm::errs() << "]\n";

      llvm::errs() << "    source origin: [";
      for (unsigned i = 0;
           i < result.logicalMapping.sourceOrigin.size();
           ++i) {
        if (i != 0)
          llvm::errs() << " ";
        llvm::errs() << result.logicalMapping.sourceOrigin[i];
      }
      llvm::errs() << "]\n";

      llvm::errs() << "    static source origin: "
                   << (result.logicalMapping.hasStaticSourceOrigin
                           ? "yes"
                           : "no")
                   << "\n";

      llvm::errs() << "    invertible: "
                   << (result.logicalMapping.isInvertible ? "yes" : "no")
                   << "\n";

      if (result.sourceDomain) {
        llvm::errs() << "  source iteration domain:\n";
        result.sourceDomain->dump();
      }

      printLogicalExtent("row", result.rowExtent);
      printLogicalExtent("column", result.columnExtent);
      printLogicalExtent("reduction", result.reductionExtent);

      auto partitions = partitionLogicalMatmul(result);
      if (failed(partitions)) {
        llvm::errs() << "  partitioning result: failure\n";
      } else {
        llvm::errs() << "  partition count: "
                     << partitions->size()
                     << "\n";

        for (unsigned i = 0; i < partitions->size(); ++i) {
          llvm::errs() << "  partition[" << i
                       << "] execution domain:\n";
          (*partitions)[i].iterationDomain.dump();
        }
      }
    }
  }
};

} // namespace

void mlir::systolic::registerTestAffineMatmulDiscoveryPass() {
  PassRegistration<TestAffineMatmulDiscoveryPass>();
}
