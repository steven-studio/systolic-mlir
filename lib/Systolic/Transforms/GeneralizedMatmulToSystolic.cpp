//===- GeneralizedMatmulToSystolic.cpp - GEMMs lowered with records --------===//
//
// The records mode of --affine-matmul-to-systolic: see Systolic/GeneralizedMatmulToSystolic.h.
// Here are the mode's walk (which nest goes which way, and what is said of a nest that is
// left) and the emission of a GEMM from its record, by formulas or by tables. The record's
// own checks are NestRecordGate.cpp's, the nest's own checks and the formulas and tables are
// NestRecordLowering.cpp's, and the dense lowering of a GEMM the affine recognizer finds is
// the pass's (AffineMatmulToSystolic.cpp, lowerOne), called through
// DenseMatmulLowering.
//
//===----------------------------------------------------------------------===//

#include "Systolic/GeneralizedMatmulToSystolic.h"

#include "Systolic/MatmulTileLowering.h"
#include "Systolic/NestRecordGate.h"
#include "Systolic/NestRecordLowering.h"

#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Bufferization/IR/Bufferization.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/Interfaces/DataLayoutInterfaces.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/StringExtras.h"

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

    if (!g.tables) {
      // The record's formulas: each operand a view of its memref.
      std::array<LogicalOperand, 3> operand;
      for (unsigned x = 0; x < 3; ++x) {
        const DrivenOperand &d = g.operands[x];
        const unsigned rank = cast<MemRefType>(d.memref.getType()).getRank();
        LogicalOperand &o = operand[x];
        o.memref = d.memref;
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
      ModuleOp module = getOperation();
      auto symbolName = [&](StringRef base) {
        std::string name = base.str();
        for (unsigned n = 1; SymbolTable::lookupSymbolIn(module, name); ++n)
          name = base.str() + "_" + std::to_string(n);
        return name;
      };
      std::string stem = "__nest_table_" + g.where;
      for (char &ch : stem)
        if (!llvm::isAlnum(ch) && ch != '_')
          ch = '_';
      std::array<Value, 3> table, flat, matrix;
      static const char *const kName[] = {"A", "B", "C"};
      for (unsigned x = 0; x < 3; ++x) {
        auto tableTy = MemRefType::get(shapeOf[x], builder.getI64Type());
        auto values = DenseElementsAttr::get(RankedTensorType::get(shapeOf[x], builder.getI64Type()),
                                             ArrayRef<int64_t>(g.operands[x].table));
        std::string name = symbolName(stem + "_" + kName[x]);
        {
          OpBuilder::InsertionGuard guard(builder);
          builder.setInsertionPointToStart(module.getBody());
          memref::GlobalOp::create(builder, loc, name, builder.getStringAttr("private"), tableTy,
                                   values, /*constant=*/true, /*alignment=*/IntegerAttr());
        }
        table[x] = memref::GetGlobalOp::create(builder, loc, tableTy, name);
        auto memTy = cast<MemRefType>(g.operands[x].memref.getType());
        const int64_t elements = memTy.getNumElements();
        auto flatTy = MemRefType::get({elements}, elementType);
        flat[x] = memref::ReinterpretCastOp::create(
            builder, loc, flatTy, g.operands[x].memref, /*offset=*/builder.getIndexAttr(0),
            ArrayRef<OpFoldResult>{builder.getIndexAttr(elements)},
            ArrayRef<OpFoldResult>{builder.getIndexAttr(1)});
        matrix[x] = memref::AllocOp::create(builder, loc, MemRefType::get(shapeOf[x], elementType));
      }
      // matrix[x][r][c] = operand x at table[x][r][c], or the reverse for the scatter.
      auto copy = [&](unsigned x, bool scatter) {
        auto rows = affine::AffineForOp::create(builder, loc, 0, shapeOf[x][0], 1);
        OpBuilder::InsertionGuard guard(builder);
        builder.setInsertionPointToStart(rows.getBody());
        auto columns = affine::AffineForOp::create(builder, loc, 0, shapeOf[x][1], 1);
        builder.setInsertionPointToStart(columns.getBody());
        SmallVector<Value, 2> rc{rows.getInductionVar(), columns.getInductionVar()};
        Value offset64 = affine::AffineLoadOp::create(builder, loc, table[x], rc);
        Value offset = arith::IndexCastOp::create(builder, loc, builder.getIndexType(), offset64);
        if (!scatter) {
          Value v = memref::LoadOp::create(builder, loc, flat[x], ValueRange{offset});
          affine::AffineStoreOp::create(builder, loc, v, matrix[x], rc);
        } else {
          Value v = affine::AffineLoadOp::create(builder, loc, matrix[x], rc);
          memref::StoreOp::create(builder, loc, v, flat[x], ValueRange{offset});
        }
      };
      for (unsigned x = 0; x < 3; ++x)
        copy(x, /*scatter=*/false);
      Value aTensor = asTensor(matrix[0], shapeOf[0], /*writable=*/false);
      Value bTensor = asTensor(matrix[1], shapeOf[1], /*writable=*/false);
      Value cTensor = asTensor(matrix[2], shapeOf[2], /*writable=*/true);
      Value result = runTiles(aTensor, bTensor, cTensor);
      writeBackMatmulResult(builder, loc, result, matrix[2]);
      copy(2, /*scatter=*/true);
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
    for (Operation *loop : drivenNests) {
      std::string why;
      FailureOr<mlir::systolic::RecordDrivenGemm> driven = mlir::systolic::analyzeRecordDrivenNest(
          *records, bound, loop, indexWidth, recordTableBudget, why);
      // A refusal is said where the research found a multiply-accumulate in the nest (its
      // record has a GEMM), or always with report=true; other loops stay as they are, silently.
      auto worthSaying = [&]() {
        if (report)
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
      if (succeeded(lowerRecordDriven(*driven, *fleet, why, hardError)))
        continue;
      if (hardError) {
        return failure();
      }
      loop->emitRemark() << "GEMM not lowered from its record: " << driven->where << ": " << why;
    }
    return success();
  }
};

} // namespace

LogicalResult mlir::systolic::runGeneralizedMatmulToSystolic(
    ModuleOp module, const GeneralizedMatmulOptions &options,
    int64_t &nextGemmIndex, DenseMatmulLowering lowerOne) {
  return RecordsMode(module, options, nextGemmIndex, lowerOne).run();
}
