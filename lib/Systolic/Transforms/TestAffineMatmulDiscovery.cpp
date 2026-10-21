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

  // Deliberately wrong logical mappings for every verified GEMM: the
  // bijection certificate must refuse each (a test that it is not vacuous,
  // docs/AFFINE_GEMM_MAPPING.md section 19.13.4).
  Option<bool> perturbCertificate{
      *this, "perturb-certificate",
      llvm::cl::desc("After a verified logical bijection, check it again on "
                     "wrong mappings (M + 1, M - 1, I's scale doubled, loop "
                     "0's origin + 1, I's row of T zero), each of which the "
                     "certificate must refuse"),
      llvm::cl::init(false)};

  Option<bool> geometry{
      *this, "geometry",
      llvm::cl::desc("Print the geometric screening of every candidate "
                     "(docs/AFFINE_GEMM_MAPPING.md section 19: necessary "
                     "conditions only, not a decision)"),
      llvm::cl::init(false)};

  Option<bool> reductionBatches{
      *this, "reduction-batches",
      llvm::cl::desc("Recognize with reduction-batch partitioning "
                     "(docs/AFFINE_GEMM_MAPPING.md section 23; on by default, "
                     "as in --affine-matmul-to-systolic)"),
      llvm::cl::init(true)};

  Option<bool> r8{
      *this, "r8",
      llvm::cl::desc("Print the R8 structure of every candidate: dot "
                     "products and ranks on the counters as written, the "
                     "loop classes and Rule E (docs/AFFINE_GEMM_MAPPING.md "
                     "section 22: a diagnostic, not a decision)"),
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

  static void printR8(AffineMatmulCandidate candidate,
                      ArrayRef<DomainNormalization> normalized) {
    llvm::errs() << "R8 structure (diagnostic, not a decision):\n";
    // R8 is defined on the counters as written; a normalized nest's
    // counters are x' (x = U x'), another basis.
    if (countersRewritten(normalized, candidate.anchor)) {
      llvm::errs() << "  not analysed: the domain normalization rewrote "
                      "the counters of this nest (x = U x'), they are not "
                      "the counters as written\n";
      return;
    }
    std::string why;
    FailureOr<AffineGeometryScreening> g =
        screenAffineMatmulGeometry(candidate, &why);
    if (failed(g)) {
      llvm::errs() << "  not analysed: " << why << "\n";
      return;
    }
    SmallVector<int64_t> trips;
    for (const AffineGeometryScreening::Loop &loop : g->loops)
      trips.push_back(loop.trip);
    std::optional<AffineR8Structure> r = computeR8Structure(g->written, trips);
    if (!r) {
      llvm::errs() << "  not analysed: overflow\n";
      return;
    }
    // A and B are the product's first and second factor, C the accumulator
    // (not the memrefs' names).
    static const char *const kOperand[3] = {"A", "B", "C"};
    for (unsigned x = 0; x < 3; ++x) {
      llvm::errs() << "  " << kOperand[x] << " rows: ";
      printMatrix(g->written[x]);
      llvm::errs() << "\n";
    }
    for (auto [s, readers] : llvm::enumerate(r->readers)) {
      llvm::errs() << "  loop " << s << ":";
      if (trips[s] < 2) {
        llvm::errs() << " one iteration (a constant)\n";
        continue;
      }
      if (readers == 0) {
        llvm::errs() << " used by no access\n";
        continue;
      }
      llvm::errs() << " readers";
      for (unsigned x = 0; x < 3; ++x)
        if (readers & (1u << x))
          llvm::errs() << " " << kOperand[x];
      llvm::errs() << " -> ";
      switch (readers) {
      case 0b111:
        llvm::errs() << "OB (batch)";
        break;
      case 0b101:
        llvm::errs() << "M or OB";
        break;
      case 0b110:
        llvm::errs() << "N or OB";
        break;
      case 0b100:
        llvm::errs() << "OB (C only)";
        break;
      case 0b011:
        llvm::errs() << "K or RB (contraction block)";
        break;
      case 0b001:
      case 0b010:
        llvm::errs() << "RB (one factor only)";
        break;
      default:
        break;
      }
      llvm::errs() << "\n";
    }
    llvm::errs() << "  reduction loops Z:";
    if (r->reductionLoops.empty())
      llvm::errs() << " none";
    for (unsigned s : r->reductionLoops)
      llvm::errs() << " " << s;
    llvm::errs() << "; shared loops S:";
    if (r->contractionLoops.empty())
      llvm::errs() << " none";
    for (unsigned s : r->contractionLoops)
      llvm::errs() << " " << s;
    llvm::errs() << "; groups:";
    for (const SmallVector<unsigned> &group : r->contractionGroups) {
      llvm::errs() << " {";
      llvm::interleave(group, llvm::errs(), " ");
      llvm::errs() << "}";
    }
    llvm::errs() << "\n";
    llvm::errs() << "  on Z: rank A " << r->rankFirst << ", rank B "
                 << r->rankSecond << ", rank [A; B] " << r->rankStacked
                 << " (row spaces meet in dimension "
                 << r->rankFirst + r->rankSecond - r->rankStacked
                 << "), coupling rank(A B^T) " << r->coupling << "\n";
    llvm::errs() << "  batch block coupling: " << r->batchCoupling << "\n";
    llvm::errs() << "  literal, loops of two or more iterations: rank(A B^T) " << r->literalAB
                 << ", rank(A C^T) " << r->literalAC << ", rank(B C^T) "
                 << r->literalBC << "; nonzero A.B pairs " << r->pairsAB
                 << " (not a dimension)\n";
    if (r->literalAB != r->coupling + r->batchCoupling)
      llvm::errs() << "  literal and restricted readings differ: dot "
                      "products of different loop classes cancel or add "
                      "(section 22.4)\n";
    llvm::errs() << "  operand ranks: A " << r->operandRank[0] << ", B "
                 << r->operandRank[1] << ", C " << r->operandRank[2];
    for (unsigned rank : {1u, 0u}) {
      bool first = true;
      for (unsigned x = 0; x < 3; ++x) {
        if (r->operandRank[x] != rank)
          continue;
        if (first)
          llvm::errs() << (rank == 1 ? "; one-dimensional (Phase 2):"
                                     : "; constant (rank 0):");
        llvm::errs() << " " << kOperand[x];
        first = false;
      }
    }
    llvm::errs() << "\n";
    auto yn = [](bool b) { return b ? "yes" : "no"; };
    llvm::errs() << "  intersect(A,B) (some a.b != 0 on Z): "
                 << yn(r->intersect)
                 << "; shared loop variables (some sum |a_t||b_t| > 0): "
                 << yn(r->sharedLoops) << "\n";
  }

  void runOnOperation() override {
    // The domain normalization the lowering pass applies first (docs
    // sections 19.7, 19.12), reported line by line.
    SmallVector<DomainNormalization, 1> normalized =
        normalizeParallelotopeDomains(getOperation().getOperation());
    if (!normalized.empty()) {
      llvm::errs() << "domain normalization: " << normalized.size()
                   << " nest(s)\n";
      for (const DomainNormalization &dn : normalized)
        describeDomainNormalization(dn, llvm::errs(), "  ");
    }

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
      if (r8)
        printR8(candidate, normalized);
      std::string notRecognized;
      GemmMappingClass notRecognizedClass = GemmMappingClass::ProvenInvalid;
      AffineMatmulRecognitionOptions recognition;
      recognition.countersAsWritten =
          !countersRewritten(normalized, candidate.anchor);
      recognition.reductionBatches =
          reductionBatches && recognition.countersAsWritten;
      auto result = recognizeLogicalMatmul(candidate, &notRecognized,
                                           &notRecognizedClass, recognition);

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

      // The task's exact mapping (section 19.12), reported, not decided.
      {
        const ExactMapping &em = result.exactMapping;
        llvm::errs() << "  exact mapping (task of 2026-10-17, over the counters of the nest as analysed): ";
        if (!em.defined) {
          llvm::errs() << em.note << "\n";
        } else {
          auto printRow = [](llvm::ArrayRef<int64_t> v) {
            llvm::errs() << "[";
            llvm::interleaveComma(v, llvm::errs());
            llvm::errs() << "]";
          };
          llvm::errs() << (em.degenerateK ? "2-D (K = 1: a degenerate reduction, "
                                            "the non-degenerate rule does not apply)"
                                          : "3-D")
                       << ", T = [";
          for (unsigned r = 0; r < em.T.size(); ++r) {
            if (r)
              llvm::errs() << ", ";
            printRow(em.T[r]);
          }
          llvm::errs() << "], d = ";
          printRow(em.d);
          if (em.singular) {
            llvm::errs() << ", det 0 (singular: rank(Ic, Jc, q) < " << em.T.size()
                         << ", the task's mapping has no inverse)\n";
          } else {
            llvm::errs() << ", det " << em.det
                         << (em.unimodular ? ", unimodular" : ", not unimodular")
                         << ", T diag(step) a signed permutation: "
                         << (em.boxImage ? "yes (the image of the box is a complete box)"
                                         : "no (the image is a sublattice, not a complete box in these coordinates)")
                         << "\n";
          }
          if (em.rowsSwapped)
            llvm::errs() << "    factor order: the recognizer takes the second "
                            "factor as the row operand (C = B' A')\n";
          if (!em.degenerateK) {
            llvm::errs() << "    v = ";
            if (em.v.empty()) {
              llvm::errs() << "none (rank(Ic, Jc) < 2 or overflow)\n";
            } else {
              printRow(em.v);
              llvm::errs() << "; Ia.v = " << em.deltas[0] << ", Ka.v = " << em.deltas[1]
                           << ", Kb.v = " << em.deltas[2] << ", Jb.v = " << em.deltas[3]
                           << "; A varies along v: " << (em.aVaries ? "yes" : "no")
                           << ", B varies along v: " << (em.bVaries ? "yes" : "no")
                           << "\n";
            }
          } else {
            llvm::errs() << "    no reduction direction: K has no loop\n";
          }
          llvm::errs() << "    rank(Cmat) " << em.rankC << ", rank(Amat) " << em.rankA
                       << ", rank(Bmat) " << em.rankB << "\n";
          auto printLambda = [&](const char *name,
                                 llvm::ArrayRef<lattice::RationalRow> rows,
                                 const char *restriction, bool holds) {
            llvm::errs() << "    lambda(" << name << ") = ";
            for (unsigned r = 0; r < rows.size(); ++r) {
              if (r)
                llvm::errs() << ", ";
              printRow(rows[r].numerators);
              llvm::errs() << "/" << rows[r].denominator;
            }
            if (rows.empty())
              llvm::errs() << "undefined";
            llvm::errs() << " -- " << restriction << ": " << (holds ? "yes" : "no")
                         << "\n";
          };
          printLambda("A", em.lambdaA, "A free of J", em.aFreeOfJ);
          printLambda("B", em.lambdaB, "B free of I", em.bFreeOfI);
        }
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

      // The run-time (symbolic) offsets of the accesses, if any.
      auto printSymbolic = [](const char *name, AffineMap m, ArrayRef<Value> syms) {
        llvm::errs() << "  symbolic offsets " << name << ": ";
        if (!m) {
          llvm::errs() << "none\n";
          return;
        }
        m.print(llvm::errs());
        llvm::errs() << " over " << syms.size() << " symbol(s)\n";
      };
      printSymbolic("A", result.symbolicA, result.symbolsA);
      printSymbolic("B", result.symbolicB, result.symbolsB);
      printSymbolic("C", result.symbolicC, result.symbolsC);

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
      llvm::errs() << "  statement: "
                   << (result.accumulates
                           ? "C = C + A * B (accumulation)"
                           : "C = A * B (assignment: every element of C "
                             "written once, lowered as the K = 1 GEMM into "
                             "a zeroed C)")
                   << "\n";
      // Dimension-intersection partitioning (docs section 21): the loops
      // made batch loops beyond those read by all three operands.
      SmallVector<unsigned> partitionLoops;
      for (unsigned d : result.demotedLoops)
        if (!llvm::is_contained(result.reductionBatchLoops, d))
          partitionLoops.push_back(d);
      if (!partitionLoops.empty()) {
        llvm::errs() << "  partition: loop(s) ";
        llvm::interleave(partitionLoops, llvm::errs(), "+");
        llvm::errs() << " demoted to batch loops (each value is one GEMM on "
                        "its slab of C; the factor that does not read it is "
                        "shared; slabs of an accumulation may overlap, the "
                        "batches then run in program order, docs section "
                        "25)\n";
      }
      // Reduction-batch partitioning (docs section 23).
      if (!result.reductionBatchLoops.empty()) {
        llvm::errs() << "  reduction batch: loop(s) ";
        llvm::interleave(result.reductionBatchLoops, llvm::errs(), "+");
        llvm::errs() << " (not read by C: for each value a GEMM with offset "
                        "factor views, all accumulated into the same slab of "
                        "C in order)\n";
      }

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
        // The logical extents the partition and the lowering use (a role
        // with no loop has extent 1: K = 1 is the outer product, SQ-4).
        llvm::errs() << "  dense extents: M " << dense->extent[0] << ", N "
                     << dense->extent[1] << ", K " << dense->extent[2] << "\n";
        // The logical mapping as an exact bijection onto the box (docs
        // section 19.13): no logical iteration missing or duplicated.
        std::string notBijective;
        const bool bijective =
            succeeded(verifyLogicalBijection(result, *dense, &notBijective));
        if (bijective)
          llvm::errs() << "  logical bijection: verified (the iteration domain "
                          "maps one to one onto the box of the GEMM)\n";
        else
          llvm::errs() << "  logical bijection: not verified (" << notBijective
                       << ")\n";
        if (bijective && perturbCertificate) {
          auto check = [&](StringRef what, const LogicalMatmul &matmul,
                           const DenseLogicalGemm &gemm) {
            std::string why;
            llvm::errs() << "  perturbed mapping (" << what << "): ";
            if (succeeded(verifyLogicalBijection(matmul, gemm, &why)))
              llvm::errs() << "verified\n";
            else
              llvm::errs() << "not verified (" << why << ")\n";
          };
          DenseLogicalGemm wider = *dense;
          wider.extent[0] += 1;
          check("M + 1", result, wider);
          if (dense->extent[0] >= 2) {
            DenseLogicalGemm narrower = *dense;
            narrower.extent[0] -= 1;
            check("M - 1", result, narrower);
          }
          DenseLogicalGemm coarser = *dense;
          coarser.scale[0] *= 2;
          check("I's scale doubled", result, coarser);
          if (!result.normalization.sourceOffset.empty()) {
            LogicalMatmul shifted = result;
            shifted.normalization.sourceOffset[0] += 1;
            check("loop 0's origin + 1", shifted, *dense);
          }
          const unsigned rowI = dense->numBatchDims;
          if (rowI < result.logicalMapping.transformation.size()) {
            LogicalMatmul collapsed = result;
            for (int64_t &entry : collapsed.logicalMapping.transformation[rowI])
              entry = 0;
            check("I's row of T zero", collapsed, *dense);
          }
        }
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
