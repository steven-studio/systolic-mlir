//===- GeneralizedMatmulToSystolic.cpp - GEMMs lowered with records --------===//
//
// The records mode of --affine-matmul-to-systolic: see Systolic/GeneralizedMatmulToSystolic.h.
// Here are the mode's walk (which nest goes which way, and what is said of a nest that is
// left) and the lowering of a GEMM from its record: its gates, its plan, and what is emitted
// in which order, by the record's formulas or by its tables. How each step is emitted is
// MatmulTileLowering.cpp's: this file makes no operation of the affine, arith or memref
// dialects and looks up no symbol. That boundary is a test:
// test/Systolic/architecture_layering.mlir fails when the header of the memref or arith
// operations, or of the partition into boxes, becomes a dependency of this file, through an
// #include here or in a header included here. The record's own checks are
// NestRecordGate.cpp's, the nest's own checks and the formulas and tables are
// NestRecordLowering.cpp's, and the dense lowering of a GEMM the affine recognizer finds is
// the pass's (AffineMatmulToSystolic.cpp, lowerOne), called through DenseMatmulLowering.
//
//===----------------------------------------------------------------------===//

#include "Systolic/GeneralizedMatmulToSystolic.h"

#include "Systolic/MatmulTileLowering.h"
#include "Systolic/NestRecordGate.h"
#include "Systolic/NestRecordLowering.h"

#include "mlir/Dialect/Bufferization/IR/Bufferization.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/Interfaces/DataLayoutInterfaces.h"

#include "llvm/ADT/DenseMap.h"

#include <array>
#include <optional>
#include <string>

using namespace mlir;
using namespace mlir::systolic;

namespace {

/// One run of the records mode. The options are members under the names the pass gives
/// them, so what was the pass's code reads here as it did there.
struct RecordsMode : GeneralizedMatmulOptions {
  RecordsMode(ModuleOp module, const GeneralizedMatmulOptions &options,
              int64_t &nextGemmIndex, DenseMatmulLowering lowerOne)
      : GeneralizedMatmulOptions(options), theModule(module),
        nextGemmIndex(nextGemmIndex), lowerOne(lowerOne) {}

  ModuleOp theModule;
  /// The systolic.gemm index the next GEMM lowered through a plan gets: the pass's.
  int64_t &nextGemmIndex;
  /// The dense lowering of a GEMM the affine recognizer found: the pass's.
  DenseMatmulLowering lowerOne;

  ModuleOp getOperation() const { return theModule; }

  /// Stage 3 of docs/GENERALIZED_GEMM_SCOPE.md: the GEMM of a nest the recognizer found no
  /// GEMM in, from its record (Systolic/NestRecordLowering.h). The alias gate, the
  /// floating-point policy and the plan are the dense path's; the operands are views of their
  /// memrefs by the record's formulas, or matrices gathered by the tables and C scattered back.
  LogicalResult lowerRecordDriven(const RecordDrivenGemm &g, const SystolicFleet &fleet,
                                  std::string &why, bool &hardError) {
    Operation *anchor = g.anchor;
    const int64_t M = g.M, N = g.N, K = g.K;
    const std::array<std::array<int64_t, 2>, 3> shapeOf = {{{M, K}, {K, N}, {M, N}}};
    Value A = g.operands[0].memref, B = g.operands[1].memref, C = g.operands[2].memref;
    Type elementType = cast<MemRefType>(C.getType()).getElementType();

    for (auto [operand, name] : {std::pair{A, "A"}, std::pair{B, "B"}}) {
      if (!provablyDisjointFromOutput(operand, C, assumeNoAlias)) {
        why = (llvm::Twine(name) +
               " may overlap C; declare the arguments llvm.noalias (C restrict) or pass "
               "assume-no-alias")
                  .str();
        return failure();
      }
    }
    // Experimental: the tables a gathered operand reads its indices from (dp). The nest
    // reads them while it writes C, the packing before the tiles does not: they must not
    // overlap C, which is also what the record assumed.
    for (Value table : g.gatherTables) {
      if (!provablyDisjointFromOutput(table, C, assumeNoAlias)) {
        why = "a gathered index table may overlap C; declare the arguments llvm.noalias (C "
              "restrict) or pass assume-no-alias";
        return failure();
      }
    }
    if (!g.reassoc && !allowFpReassociation) {
      why = "floating-point reassociation is not permitted: the systolic execution sums each "
            "tile from zero and adds the sum to C, a different order of IEEE additions than "
            "the loop's (mark the accumulation reassoc, or pass allow-fp-reassociation)";
      return failure();
    }

    // The plan, its tiles proven an exact partition of the M x N x K box, and how many they are.
    std::optional<SystolicTilePlan> plan;
    size_t regions = 0;
    if (failed(planMatmulTiles(anchor, M, N, K, fleet, sharedReadBytesPerCycle, costModel,
                               operandPath, plan, regions, why, hardError)))
      return failure();

    // ------------------------------------------------------------------
    // Materialize. Every check is done; from here on the nest is replaced.
    // ------------------------------------------------------------------
    OpBuilder builder(anchor);
    Location loc = anchor->getLoc();
    auto asTensor = [&](Value memref, ArrayRef<int64_t> shape, bool writable) -> Value {
      auto ty = RankedTensorType::get(shape, elementType);
      return bufferization::ToTensorOp::create(builder, loc, ty, memref,
                                               /*restrict=*/true, writable);
    };
    auto runTiles = [&](Value aTensor, Value bTensor, Value cTensor) -> Value {
      return emitMatmulTiles(builder, loc, aTensor, bTensor, cTensor, M, N, K, plan, fleet,
                             nextGemmIndex);
    };

    auto packed = [&](unsigned x) -> Value {
      SmallVector<GatherPackDimension, 2> pack;
      for (const DrivenOperand::DrivenGather &one : g.operands[x].pack)
        pack.push_back({one.gathered, one.table, one.zeroExtend, one.coefficient, one.base,
                        one.modScale, one.modulus, one.modBase, one.modCoefficient});
      return materializeGatherPack(builder, loc, g.operands[x].memref, pack, shapeOf[x], elementType);
    };
    if (g.scatter && g.scatterSource.products) {
      // exp/k-scatter: C's address reads K. Every target of (i, j, k) checked at run time;
      // when all are inside, A and B packed, every product by a K = 1 GEMM per k (the plan of
      // M x N x 1), and the products added into C through the two-layer inverse map, bucketed
      // by cell; otherwise the nest as written.
      const RecordDrivenGemm::ScatterSource &src = g.scatterSource;
      std::optional<SystolicTilePlan> productPlan;
      size_t productRegions = 0;
      if (failed(planMatmulTiles(anchor, M, N, 1, fleet, sharedReadBytesPerCycle, costModel, operandPath,
                                 productPlan, productRegions, why, hardError)))
        return failure();
      ScatterRegions scatterRegions =
          beginScatter(builder, loc, src.slice, src.store, src.counterI, src.lowerI, src.counterJ, src.lowerJ, C,
                       {M, N}, src.counterK, src.lowerK, K);
      Value aTensor = asTensor(packed(0), shapeOf[0], /*writable=*/false);
      Value bTensor = asTensor(packed(1), shapeOf[1], /*writable=*/false);
      Value products = materializeProductMatrices(builder, loc, aTensor, bTensor, M, N, K, productPlan, fleet,
                                                  nextGemmIndex, elementType);
      scatterAddByInverseMap(builder, loc, products, scatterRegions.targets, C);
      anchor->moveBefore(scatterRegions.elseBlock->getTerminator());
      if (report)
        anchor->getParentOp()->emitRemark()
            << "GEMM " << M << "x" << N << "x" << K << " of " << g.where << " lowered from its record as "
            << productRegions * K << " exact region(s) (" << g.summary << ")";
      return success();
    }
    if (g.scatter) {
      // exp/general-scatter, Track B: every target checked at run time; when all are inside,
      // A and B packed, the tiles into a zeroed matrix, and its cells added into C through
      // the two-layer inverse map; otherwise the nest as written.
      const RecordDrivenGemm::ScatterSource &src = g.scatterSource;
      const size_t regionCount = regions;
      ScatterRegions scatterRegions = beginScatter(builder, loc, src.slice, src.store, src.counterI, src.lowerI,
                                            src.counterJ, src.lowerJ, C, {M, N});
      Value aTensor = asTensor(packed(0), shapeOf[0], /*writable=*/false);
      Value bTensor = asTensor(packed(1), shapeOf[1], /*writable=*/false);
      Value product = materializeZeroMatrix(builder, loc, {M, N}, elementType);
      Value result = runTiles(aTensor, bTensor, asTensor(product, {M, N}, /*writable=*/true));
      writeBackMatmulResult(builder, loc, result, product);
      scatterAddByInverseMap(builder, loc, product, scatterRegions.targets, C);
      anchor->moveBefore(scatterRegions.elseBlock->getTerminator());
      if (report)
        anchor->getParentOp()->emitRemark()
            << "GEMM " << M << "x" << N << "x" << K << " of " << g.where << " lowered from its record as "
            << regionCount << " exact region(s) (" << g.summary << ")";
      return success();
    }
    if (!g.tables) {
      // The record's formulas: each operand a view of its memref.
      std::array<LogicalOperand, 3> operand;
      for (unsigned x = 0; x < 3; ++x) {
        const DrivenOperand &d = g.operands[x];
        LogicalOperand &o = operand[x];
        o.memref = d.memref;
        // Experimental: a gathered operand, packed into its rows x columns matrix first.
        if (!d.pack.empty())
          o.memref = packed(x);
        const unsigned rank = cast<MemRefType>(o.memref.getType()).getRank();
        o.offsets.assign(rank, 0);
        o.sizes.assign(rank, 1);
        o.strides.assign(rank, 1);
        if (d.explicitView) {
          o.explicitView = true;
          o.roleDim = {-1, -1};
          o.elementOffset = d.elementOffset;
          o.elementStrides = d.elementStrides;
          o.shape = shapeOf[x];
          continue;
        }
        for (auto [dim, dd] : llvm::enumerate(d.dimensions)) {
          o.roleDim[dd.role] = dim;
          o.offsets[dim] = dd.offset;
          o.sizes[dim] = shapeOf[x][dd.role];
          o.strides[dim] = dd.scale;
        }
        o.shape = shapeOf[x];
      }
      llvm::DenseMap<AffineMap, Value> applies;
      auto view = [&](const LogicalOperand &o) -> Value {
        return materializeOperandView(builder, loc, o, {}, {}, applies);
      };
      const bool sharedInput = operand[0].sameView(operand[1]);
      Value aTensor = asTensor(view(operand[0]), operand[0].shape, /*writable=*/false);
      Value bTensor = sharedInput ? aTensor
                                  : asTensor(view(operand[1]), operand[1].shape, /*writable=*/false);
      Value cView = view(operand[2]);
      Value cTensor = asTensor(cView, operand[2].shape, /*writable=*/true);
      Value result = runTiles(aTensor, bTensor, cTensor);
      writeBackMatmulResult(builder, loc, result, cView);
    } else {
      // The tables: constant globals of element offsets; A, B and C gathered into matrices,
      // the tiles on those, C scattered back.
      std::array<TableOperand, 3> operand;
      for (unsigned x = 0; x < 3; ++x)
        operand[x] = {g.operands[x].memref, g.operands[x].table, shapeOf[x]};
      TableOperands tables =
          materializeTableOperands(builder, loc, getOperation(), g.where, operand, elementType);
      Value aTensor = asTensor(tables.matrix[0], shapeOf[0], /*writable=*/false);
      Value bTensor = asTensor(tables.matrix[1], shapeOf[1], /*writable=*/false);
      Value cTensor = asTensor(tables.matrix[2], shapeOf[2], /*writable=*/true);
      Value result = runTiles(aTensor, bTensor, cTensor);
      writeBackMatmulResult(builder, loc, result, tables.matrix[2]);
      scatterTableResult(builder, loc, tables);
      // No dealloc: --systolic-runtime-to-llvm's buffer deallocation frees the three matrices
      // (an explicit dealloc of a buffer given to to_tensor is refused there).
    }

    if (report) {
      auto remark = anchor->emitRemark()
                    << "GEMM " << M << "x" << N << "x" << K << " of " << g.where
                    << " lowered from its record as " << regions << " exact region(s) ("
                    << g.summary << ")";
      if (plan)
        remark << " on " << fleet.resources.size()
               << " accelerator(s): makespan = " << plan->optimization.makespan
               << " cycles (read + compute + write), " << plan->blocks.size()
               << " output block(s)";
    }
    anchor->erase();
    return success();
  }

  /// Stage 2 of docs/GENERALIZED_GEMM_SCOPE.md: the dense path, gated by the records of
  /// nest-records (Systolic/NestRecordGate.h). A record is about a nest as the input has
  /// it, so nothing rewrites a nest before its gate: the steps that do (last-write-wins,
  /// the untiling of written tilings, the normalization of parallelotope domains, the
  /// hoisting of repetition loops) are not run, and neither are the paths that read no
  /// records. Each GEMM that passes its gate is lowered by lowerOne, with every check it
  /// makes without records, where it stands and in program order.
  LogicalResult run() {
    ModuleOp module = getOperation();
    if (generalAffine || kernelCosetOffload || certifiedOffloadFirst || commonFiberOffload || fpDistribute) {
      module.emitError() << "affine-matmul-to-systolic: nest-records is read on the dense path "
                            "only; general-affine, kernel-coset-offload, certified-offload-first, "
                            "common-fiber-offload and fp-distribute take other paths, which read "
                            "no records";
      return failure();
    }
    FailureOr<SystolicFleet> fleet = collectSystolicFleet(module);
    if (failed(fleet)) {
      return failure();
    }
    std::string error;
    FailureOr<mlir::systolic::NestRecordFile> records =
        mlir::systolic::NestRecordFile::load(nestRecords, error);
    if (failed(records)) {
      module.emitError() << "affine-matmul-to-systolic: nest-records: " << error;
      return failure();
    }
    const unsigned indexWidth =
        DataLayout(module).getTypeSizeInBits(IndexType::get(module.getContext())).getFixedValue();
    // Every outermost loop's binding, before anything is lowered.
    const mlir::systolic::BoundNests bound(module);
    nextGemmIndex = nextSystolicGemmIndex(module);

    SmallVector<AffineMatmulCandidate> candidates;
    collectAffineMatmulCandidates(module.getOperation(), candidates);
    SmallVector<LogicalMatmul, 1> matmuls;
    for (AffineMatmulCandidate candidate : candidates) {
      AffineMatmulRecognitionOptions recognition;
      recognition.countersAsWritten = true;
      recognition.reductionBatches = reductionBatches;
      std::string notRecognized;
      GemmMappingClass notRecognizedClass = GemmMappingClass::ProvenInvalid;
      auto result = recognizeLogicalMatmul(candidate, &notRecognized,
                                           &notRecognizedClass, recognition);
      if (failed(result))
        continue;
      if (llvm::any_of(matmuls, [&](const LogicalMatmul &m) {
            return m.outputStore == result->outputStore;
          }))
        continue;
      matmuls.push_back(*result);
    }
    // Stage 3: the outermost loops in which the recognizer found no GEMM, in program order,
    // taken now, while every anchor is in place.
    SmallVector<Operation *> drivenNests;
    if (recordDriven)
      module.walk([&](func::FuncOp function) {
        for (Operation *loop : mlir::systolic::outermostLoops(function))
          if (bound.find(loop) && llvm::none_of(matmuls, [&](const LogicalMatmul &m) {
                return m.anchor == loop || loop->isAncestor(m.anchor);
              }))
            drivenNests.push_back(loop);
      });
    for (const LogicalMatmul &matmul : matmuls) {
      mlir::systolic::NestRecordDecision decision =
          mlir::systolic::checkNestRecord(*records, bound, matmul, indexWidth);
      if (!decision.agrees) {
        auto remark = matmul.anchor->emitRemark()
                      << "affine GEMM not lowered to systolic.matmul_tile: nest record: "
                      << decision.why;
        if (classify)
          remark << " [" << stringifyGemmMappingClass(GemmMappingClass::Unknown) << "]";
        continue;
      }
      if (report)
        matmul.anchor->emitRemark() << "nest record agrees: " << decision.summary;
      std::string why;
      GemmMappingClass mappingClass = GemmMappingClass::ProvenValid;
      bool hardError = false;
      if (succeeded(lowerOne(matmul, *fleet, why, mappingClass, hardError)))
        continue;
      if (hardError) {
        return failure();
      }
      auto remark = matmul.anchor->emitRemark()
                    << "affine GEMM not lowered to systolic.matmul_tile: " << why;
      if (classify)
        remark << " [" << stringifyGemmMappingClass(mappingClass) << "]";
    }
    return lowerDrivenNests(*records, bound, drivenNests, *fleet, indexWidth, /*sayAll=*/report);
  }

  /// Stage 3 for each of `drivenNests`, in order: each lowered from its record, or a remark.
  LogicalResult lowerDrivenNests(const mlir::systolic::NestRecordFile &file,
                                 const mlir::systolic::BoundNests &bound,
                                 ArrayRef<Operation *> drivenNests, const SystolicFleet &fleet,
                                 unsigned indexWidth, bool sayAll) {
    const mlir::systolic::NestRecordFile *records = &file;
    for (Operation *loop : drivenNests) {
      std::string why;
      FailureOr<mlir::systolic::RecordDrivenGemm> driven = mlir::systolic::analyzeRecordDrivenNest(
          *records, bound, loop, indexWidth, recordTableBudget, why);
      // A refusal is said where the research found a multiply-accumulate in the nest (its
      // record has a GEMM), or always with sayAll (report=true in the records mode); other
      // loops stay as they are, silently.
      auto worthSaying = [&]() {
        if (sayAll)
          return true;
        const mlir::systolic::BoundNests::Nest *nest = bound.find(loop);
        const llvm::json::Object *record =
            nest ? records->nest(func::FuncOp(nest->function).getSymName(), nest->ordinal) : nullptr;
        return record && record->getObject("gemm");
      };
      if (failed(driven)) {
        if (worthSaying())
          loop->emitRemark() << "GEMM not lowered from its record: " << why;
        continue;
      }
      bool hardError = false;
      if (succeeded(lowerRecordDriven(*driven, fleet, why, hardError)))
        continue;
      if (hardError) {
        return failure();
      }
      loop->emitRemark() << "GEMM not lowered from its record: " << driven->where << ": " << why;
    }
    return success();
  }

  /// generalized-fallback: after the pass's own paths have run on the module. The nests left
  /// that are still as they were at the pass's input (`atInput`) and in which the dense
  /// recognizer finds no GEMM are lowered from their records (stage 3), each where it stands.
  /// A nest the recognizer finds a GEMM in stays as the pass's paths left it: they refused it
  /// for a reason of their own, and a record never overrules a refusal.
  LogicalResult runFallback(const mlir::systolic::BoundNests &atInput) {
    ModuleOp module = getOperation();
    FailureOr<SystolicFleet> fleet = collectSystolicFleet(module);
    if (failed(fleet))
      return failure();
    std::string error;
    FailureOr<mlir::systolic::NestRecordFile> records =
        mlir::systolic::NestRecordFile::load(nestRecords, error);
    if (failed(records)) {
      module.emitError() << "affine-matmul-to-systolic: nest-records: " << error;
      return failure();
    }
    const unsigned indexWidth =
        DataLayout(module).getTypeSizeInBits(IndexType::get(module.getContext())).getFixedValue();
    const mlir::systolic::BoundNests bound(module, atInput);
    nextGemmIndex = nextSystolicGemmIndex(module);

    SmallVector<AffineMatmulCandidate> candidates;
    collectAffineMatmulCandidates(module.getOperation(), candidates);
    SmallVector<Operation *> recognized;
    for (AffineMatmulCandidate candidate : candidates) {
      AffineMatmulRecognitionOptions recognition;
      recognition.countersAsWritten = true;
      recognition.reductionBatches = reductionBatches;
      std::string notRecognized;
      GemmMappingClass notRecognizedClass = GemmMappingClass::ProvenInvalid;
      auto result = recognizeLogicalMatmul(candidate, &notRecognized, &notRecognizedClass, recognition);
      if (succeeded(result))
        recognized.push_back(result->anchor);
    }
    SmallVector<Operation *> drivenNests;
    module.walk([&](func::FuncOp function) {
      for (Operation *loop : mlir::systolic::outermostLoops(function))
        if (bound.find(loop) && llvm::none_of(recognized, [&](Operation *anchor) {
              return anchor == loop || loop->isAncestor(anchor);
            }))
          drivenNests.push_back(loop);
    });
    return lowerDrivenNests(*records, bound, drivenNests, *fleet, indexWidth, /*sayAll=*/false);
  }
};

} // namespace

LogicalResult mlir::systolic::runGeneralizedMatmulToSystolic(
    ModuleOp module, const GeneralizedMatmulOptions &options,
    int64_t &nextGemmIndex, DenseMatmulLowering lowerOne) {
  return RecordsMode(module, options, nextGemmIndex, lowerOne).run();
}

LogicalResult mlir::systolic::runGeneralizedMatmulFallback(ModuleOp module,
                                                           const GeneralizedMatmulOptions &options,
                                                           int64_t &nextGemmIndex,
                                                           const BoundNests &atInput) {
  auto noDenseLowering = [](const LogicalMatmul &, const SystolicFleet &, std::string &why,
                            GemmMappingClass &, bool &) -> LogicalResult {
    why = "the fallback lowers no GEMM of the dense recognizer";
    return failure();
  };
  return RecordsMode(module, options, nextGemmIndex, noDenseLowering).runFallback(atInput);
}
