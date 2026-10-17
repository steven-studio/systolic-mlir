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

  Option<bool> geometry{
      *this, "geometry",
      llvm::cl::desc("Print the geometric screening of every candidate "
                     "(docs/AFFINE_GEMM_MAPPING.md section 19: necessary "
                     "conditions only, not a decision)"),
      llvm::cl::init(false)};

  static void printVector(const lattice::IntVector &v) {
    llvm::errs() << "[";
    llvm::interleaveComma(v, llvm::errs());
    llvm::errs() << "]";
  }

  static void printMatrix(const lattice::IntMatrix &m) {
    llvm::errs() << "[";
    llvm::interleaveComma(m, llvm::errs(),
                          [](const lattice::IntVector &v) { printVector(v); });
    llvm::errs() << "]";
  }

  static void printGeometry(AffineMatmulCandidate candidate) {
    std::string why;
    FailureOr<AffineGeometryScreening> g =
        screenAffineMatmulGeometry(candidate, &why);
    llvm::errs() << "geometry screening (necessary conditions only, not a "
                    "decision):\n";
    if (failed(g)) {
      llvm::errs() << "  not screened: " << why << "\n";
      return;
    }
    static const char *const kOperand[3] = {"A", "B", "C"};
    for (auto [s, loop] : llvm::enumerate(g->loops)) {
      llvm::errs() << "  loop " << s << ": trip " << loop.trip;
      if (loop.trip < 2) {
        llvm::errs() << ", one iteration (a constant)\n";
        continue;
      }
      llvm::errs() << ", readers";
      for (unsigned x = 0; x < 3; ++x)
        if (loop.readers & (1u << x))
          llvm::errs() << " " << kOperand[x];
      llvm::errs() << " -> ";
      switch (loop.role) {
      case 'I':
        llvm::errs() << "I";
        break;
      case 'J':
        llvm::errs() << "J";
        break;
      case 'K':
        llvm::errs() << "K (reduction)";
        break;
      case 'P':
        llvm::errs() << "batch";
        break;
      case '1':
        llvm::errs() << "one reader (a broadcast)";
        break;
      default:
        llvm::errs() << "no reader";
        break;
      }
      llvm::errs() << "\n";
    }
    for (unsigned x = 0; x < 3; ++x) {
      llvm::errs() << "  " << kOperand[x] << " ordinal rows: ";
      printMatrix(g->ordinal[x]);
      llvm::errs() << "\n";
    }
    llvm::errs() << "  rank of C: " << g->cRankFull << " (role space "
                 << g->cRankRoles << "); rank of all subscripts: "
                 << g->allRankFull << " (role space " << g->allRankRoles
                 << ")\n";
    llvm::errs() << "  C kernel: ";
    printMatrix(g->cKernel);
    llvm::errs() << "\n  reduction loops:";
    if (g->reductionLoops.empty())
      llvm::errs() << " none";
    for (unsigned s : g->reductionLoops)
      llvm::errs() << " " << s;
    llvm::errs() << "\n";
    for (const lattice::IntVector &v : g->cKernelBeyondReduction) {
      llvm::errs() << "  C kernel direction ";
      printVector(v);
      llvm::errs() << " joins two points of the box and is not a reduction: "
                      "two iterations write one C element\n";
    }
    auto printConditions =
        [](StringRef space,
           const std::optional<AffineGeometryScreening::SixVectorConditions>
               &c) {
          if (!c)
            return;
          auto yn = [](bool b) { return b ? "yes" : "no"; };
          llvm::errs() << "  six-vector conditions (" << space
                       << "): A " << yn(c->a) << ", B " << yn(c->b)
                       << ", C " << yn(c->c);
          if (c->reductionDirection) {
            llvm::errs() << " (d = ";
            printVector(*c->reductionDirection);
            llvm::errs() << ")";
          }
          llvm::errs() << ", D " << yn(c->d1) << " " << yn(c->d2) << " "
                       << yn(c->d3) << "\n";
          // The task algorithm of 2026-10-18, line by line (section 19.10).
          llvm::errs() << "  task algorithm (" << space
                       << "): rank(Ic,Jc)=2 " << yn(c->a)
                       << ", rank(Ia,Ka)=2 " << yn(c->d1)
                       << ", rank(Kb,Jb)=2 " << yn(c->d2)
                       << ", rank(six)=3 " << yn(c->b)
                       << ", rank(Ia,Ka,Kb,Jb)=3 " << yn(c->e)
                       << ", A varies along null(Ic,Jc) " << yn(c->aVaries)
                       << ", B varies along null(Ic,Jc) " << yn(c->bVaries)
                       << ", swap(Ia,Ka) " << yn(c->swapA)
                       << ", swap(Kb,Jb) " << yn(c->swapB) << " -> "
                       << (c->passes ? "passes" : "error") << "\n";
        };
    printConditions("full space", g->conditionsFull);
    printConditions("role space", g->conditionsRoles);
  }

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
      if (geometry)
        printGeometry(candidate);
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
        if (layout.general) {
          // A skewed or merged view (docs section 19): every dimension's
          // index as constant + coefficients * the operand's two roles.
          static const char *const kRoles[3][2] = {
              {"I", "K"}, {"K", "J"}, {"I", "J"}};
          const unsigned which = operand == "A" ? 0 : operand == "B" ? 1 : 2;
          llvm::errs() << "  layout " << operand << ": general view";
          for (auto [dim, c] : llvm::enumerate(layout.roleCoefficients)) {
            llvm::errs() << (dim ? "; " : ": ") << "dimension " << dim
                         << " = " << layout.dimConstants[dim];
            for (unsigned r = 0; r < 2; ++r)
              if (c[r] != 0)
                llvm::errs() << " + " << c[r] << "*" << kRoles[which][r];
          }
          llvm::errs() << "\n";
          return;
        }
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
