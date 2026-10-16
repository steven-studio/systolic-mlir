#include "Systolic/AffineMatmulAnalysis.h"
#include "Systolic/AffineMatmulPartitioning.h"
#include "Systolic/Passes.h"
#include "Systolic/SystolicTilePlan.h"

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

  TestAffineMatmulDiscoveryPass() = default;
  TestAffineMatmulDiscoveryPass(const TestAffineMatmulDiscoveryPass &other)
      : PassWrapper(other) {}

  // Regions to hand partitionLogicalMatmul directly, six integers each:
  // lower I, J, K, then extent I, J, K. Exercises the exactness proof on
  // partitions no plan would produce (overlaps, gaps, out of the domain).
  ListOption<int64_t> regions{
      *this, "regions",
      llvm::cl::desc("Regions to verify as a partition: lower I,J,K, "
                     "extent I,J,K per region")};

  void runOnOperation() override {
    SmallVector<AffineMatmulCandidate> candidates;

    collectAffineMatmulCandidates(
        getOperation().getOperation(), candidates);

    FailureOr<SystolicFleet> fleetOrFailure =
        collectSystolicFleet(getOperation());
    if (failed(fleetOrFailure)) {
      signalPassFailure();
      return;
    }
    const SystolicFleet &fleet = *fleetOrFailure;

    llvm::errs() << "candidate count: "
                 << candidates.size()
                 << "\n";

    SmallVector<LogicalMatmul, 1> logicalMatmuls;

    for (AffineMatmulCandidate candidate : candidates) {
      std::string notRecognized;
      GemmMappingClass notRecognizedClass = GemmMappingClass::ProvenInvalid;
      auto result =
          recognizeLogicalMatmul(candidate, &notRecognized, &notRecognizedClass);

      if (failed(result)) {
        llvm::errs() << "recognition result: failure\n";
        llvm::errs() << "  reason: " << notRecognized << "\n";
        llvm::errs() << "  class: "
                     << stringifyGemmMappingClass(notRecognizedClass) << "\n";
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

      llvm::errs() << "  phiA: ";
      result.phiA.print(llvm::errs());
      llvm::errs() << "\n";

      llvm::errs() << "  phiB: ";
      result.phiB.print(llvm::errs());
      llvm::errs() << "\n";

      llvm::errs() << "  phiC: ";
      result.phiC.print(llvm::errs());
      llvm::errs() << "\n";

      // Each operand's own view of its coordinates: subscript = scale *
      // coordinate + offset (1 * I + 0 when the subscript is the coordinate).
      auto printView = [](StringRef operand, StringRef c0, StringRef c1,
                          const std::array<OperandCoordinateView, 2> &view) {
        llvm::errs() << "  view " << operand << ": [" << view[0].scale << "*"
                     << c0 << " + " << view[0].offset << ", " << view[1].scale
                     << "*" << c1 << " + " << view[1].offset << "]\n";
      };
      printView("A", "I", "K", result.viewA);
      printView("B", "K", "J", result.viewB);
      printView("C", "I", "J", result.viewC);
      // Where the roles sit in each memref, when not the canonical rank-2
      // (rows, columns): a transposed operand, a constant (extent-one)
      // dimension, or no dimension for a role of extent one.
      auto printLayout = [](StringRef operand, const OperandLayout &layout) {
        bool canonical = layout.roleDim[0] == 0 && layout.roleDim[1] == 1 &&
                         layout.constantIndex.size() == 2;
        if (canonical)
          return;
        llvm::errs() << "  layout " << operand << ": roles in dimensions ("
                     << layout.roleDim[0] << ", " << layout.roleDim[1] << ")"
                     << (layout.isTransposed() ? " transposed" : "");
        for (auto [dim, index] : llvm::enumerate(layout.constantIndex))
          if (index && !layout.hasBatchTerms(dim))
            llvm::errs() << ", dimension " << dim << " fixed at " << *index;
        llvm::errs() << "\n";
      };
      printLayout("A", result.layoutA);
      printLayout("B", result.layoutB);
      printLayout("C", result.layoutC);
      // What each dimension adds per batch value (ISSUE-007): a batch
      // dimension's index "= static + terms", a role dimension's view
      // "+= terms"; x_s is the counter of loop s.
      auto printBatchTerms = [](StringRef operand,
                                const OperandLayout &layout) {
        SmallVector<std::string> parts;
        for (unsigned dim = 0; dim < layout.batchCoefficients.size(); ++dim) {
          if (!layout.hasBatchTerms(dim))
            continue;
          std::string text;
          llvm::raw_string_ostream os(text);
          os << "dimension " << dim;
          const bool role =
              int(dim) == layout.roleDim[0] || int(dim) == layout.roleDim[1];
          if (role)
            os << " +=";
          else
            os << " = " << layout.constantIndex[dim].value_or(0) << " +";
          bool first = true;
          for (auto [s, coefficient] :
               llvm::enumerate(layout.batchCoefficients[dim])) {
            if (coefficient == 0)
              continue;
            os << (first ? " " : " + ") << coefficient << "*x" << s;
            first = false;
          }
          parts.push_back(os.str());
        }
        if (parts.empty())
          return;
        llvm::errs() << "  batch terms " << operand << ": ";
        llvm::interleave(parts, llvm::errs(), ", ");
        llvm::errs() << "\n";
      };
      printBatchTerms("A", result.layoutA);
      printBatchTerms("B", result.layoutB);
      printBatchTerms("C", result.layoutC);
      if (result.reductionMayReassociate)
        llvm::errs() << "  reduction: reassoc permitted\n";

      printLogicalExtent("row", result.rowExtent);
      printLogicalExtent("column", result.columnExtent);
      printLogicalExtent("reduction", result.reductionExtent);

      std::string notDense;
      GemmMappingClass notDenseClass = GemmMappingClass::ProvenValid;
      auto dense = getDenseLogicalGemm(result, &notDense, &notDenseClass);
      if (succeeded(dense))
      {
        // Each coordinate's loops, outermost first ("0+3" for a split one).
        llvm::errs() << "  dense logical GEMM: yes, I/J/K on loops ";
        for (unsigned d = 0; d < 3; ++d) {
          SmallVector<unsigned, 2> loops(dense->sourceDims[d]);
          llvm::sort(loops);
          llvm::errs() << (d ? "/" : "");
          llvm::interleave(loops, llvm::errs(), "+");
        }
        // A batched GEMM: the batch loops, re-emitted around the tiles.
        if (dense->numBatchDims) {
          llvm::errs() << ", batched over loop(s) ";
          llvm::interleave(dense->batchLoops, llvm::errs(), "+");
          llvm::errs() << " (" << dense->batchTrips << " batch iteration(s))";
        }
        llvm::errs() << "\n";
      }
      else {
        llvm::errs() << "  dense logical GEMM: no (" << notDense << ")\n";
        // Which loop bound is not a constant, and why.
        if (!result.normalization.domain &&
            !result.normalizationDetail.empty())
          llvm::errs() << "  bounds: " << result.normalizationDetail << "\n";
        llvm::errs() << "  class: " << stringifyGemmMappingClass(notDenseClass)
                     << "\n";
      }

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

      if (!regions.empty() && succeeded(dense)) {
        if (regions.size() % 6 != 0) {
          getOperation().emitError("regions takes six integers per region");
          signalPassFailure();
          return;
        }
        SmallVector<LogicalBox> given;
        for (size_t r = 0; r < regions.size(); r += 6) {
          LogicalBox box;
          for (unsigned d = 0; d < 3; ++d) {
            box.lower[d] = regions[r + d];
            box.extent[d] = regions[r + 3 + d];
          }
          given.push_back(box);
        }
        std::string why;
        auto givenPartitions = partitionLogicalMatmul(result, given, &why);
        if (failed(givenPartitions))
          llvm::errs() << "  given partition: not exact (" << why << ")\n";
        else
          llvm::errs() << "  given partition: " << givenPartitions->size()
                       << " regions, exact\n";
      }

      // The partition the systolic fleet induces, when the module declares
      // one: the binary-search plan's tiles as regions, and whether they
      // were proven an exact partition of the iteration domain.
      if (fleet.resources.empty() || failed(dense))
        continue;
      auto plan = planSystolicMatmul(result.anchor, dense->extent[0],
                                     dense->extent[1], dense->extent[2],
                                     fleet, {});
      if (failed(plan))
        continue;
      SmallVector<LogicalBox> boxes;
      SmallVector<int64_t> boxDevice;
      for (const SystolicTileBlock &block : plan->blocks)
        for (const SystolicTileInvocation &inv : block.invocations) {
          LogicalBox box;
          box.lower = {block.row, block.column, inv.k0};
          box.extent = {block.m, block.n, inv.k};
          boxes.push_back(box);
          boxDevice.push_back(block.acceleratorId);
        }
      std::string why;
      auto fleetPartitions = partitionLogicalMatmul(result, boxes, &why);
      if (failed(fleetPartitions)) {
        llvm::errs() << "  fleet partition: not exact (" << why << ")\n";
        continue;
      }
      llvm::errs() << "  fleet partition: " << fleetPartitions->size()
                   << " regions, exact\n";
      for (auto [i, box] : llvm::enumerate(boxes))
        llvm::errs() << "    region[" << i << "] I[" << box.lower[0] << ", "
                     << box.lower[0] + box.extent[0] << ") J["
                     << box.lower[1] << ", " << box.lower[1] + box.extent[1]
                     << ") K[" << box.lower[2] << ", "
                     << box.lower[2] + box.extent[2] << ") on "
                     << fleet.symbols[boxDevice[i]] << "\n";
    }
  }
};

} // namespace

void mlir::systolic::registerTestAffineMatmulDiscoveryPass() {
  PassRegistration<TestAffineMatmulDiscoveryPass>();
}
