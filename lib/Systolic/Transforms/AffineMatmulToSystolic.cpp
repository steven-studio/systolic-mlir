#include "Systolic/AffineMatmulAnalysis.h"
#include "Systolic/Passes.h"
#include "Systolic/SystolicOps.h"

#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Bufferization/IR/Bufferization.h"
#include "mlir/Dialect/Linalg/IR/Linalg.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/Dialect/Tensor/IR/Tensor.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/Pass/Pass.h"

using namespace mlir;
using namespace mlir::systolic;

namespace {

struct AffineMatmulToSystolicPass
    : public PassWrapper<AffineMatmulToSystolicPass,
                         OperationPass<ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(
      AffineMatmulToSystolicPass)

  StringRef getArgument() const final {
    return "affine-matmul-to-systolic";
  }

  StringRef getDescription() const final {
    return "Lower a recognized identity affine GEMM into one "
           "systolic.matmul_tile.";
  }

  void getDependentDialects(DialectRegistry &registry) const override {
    registry.insert<
        bufferization::BufferizationDialect,
        tensor::TensorDialect,
        memref::MemRefDialect,
        scf::SCFDialect,
        systolic::SystolicDialect>();
  }

  LogicalResult lowerOne(LogicalMatmul &matmul) {
    if (!matmul.anchor ||
        !matmul.sourceDomain ||
        !matmul.normalization.domain)
      return failure();

    // The recognized logical GEMM may use a non-identity affine
    // mapping in the original loop nest.  We materialize the logical
    // operands explicitly below instead of requiring physical memref
    // shapes to equal logical GEMM shapes.
    if (matmul.rowExtent.kind != LogicalExtent::Kind::Static ||
        matmul.columnExtent.kind != LogicalExtent::Kind::Static ||
        matmul.reductionExtent.kind != LogicalExtent::Kind::Static)
      return failure();

    const int64_t M = matmul.rowExtent.staticValue;
    const int64_t N = matmul.columnExtent.staticValue;
    const int64_t K = matmul.reductionExtent.staticValue;

    if (M <= 0 || N <= 0 || K <= 0)
      return failure();

    auto lhsTy =
        dyn_cast<MemRefType>(matmul.lhs.getType());
    auto rhsTy =
        dyn_cast<MemRefType>(matmul.rhs.getType());
    auto outTy =
        dyn_cast<MemRefType>(matmul.output.getType());

    if (!lhsTy || !rhsTy || !outTy)
      return failure();

    if (!lhsTy.hasStaticShape() ||
        !rhsTy.hasStaticShape() ||
        !outTy.hasStaticShape())
      return failure();

    if (lhsTy.getRank() != 2 ||
        rhsTy.getRank() != 2 ||
        outTy.getRank() != 2)
      return failure();

    if (lhsTy.getElementType() != rhsTy.getElementType() ||
        lhsTy.getElementType() != outTy.getElementType())
      return failure();

    if (lhsTy.getRank() != 2 ||
        rhsTy.getRank() != 2 ||
        outTy.getRank() != 2)
      return failure();

    if (matmul.accessA.getNumResults() != 2 ||
        matmul.accessB.getNumResults() != 2 ||
        matmul.accessC.getNumResults() != 2)
      return failure();

    if (!matmul.logicalMapping.hasStaticSourceOrigin ||
        matmul.logicalMapping.sourceOrigin.size() != 3)
      return failure();

    Operation *anchor = matmul.anchor;
    Location loc = anchor->getLoc();

    OpBuilder builder(anchor);

    auto lhsTensorTy =
        RankedTensorType::get({M, K}, lhsTy.getElementType());
    auto rhsTensorTy =
        RankedTensorType::get({K, N}, rhsTy.getElementType());
    auto outTensorTy =
        RankedTensorType::get({M, N}, outTy.getElementType());

    auto materialize = [&](Value memref, AffineMap accessMap,
                           ArrayRef<int64_t> logicalShape) -> Value {
      auto tensorTy =
          RankedTensorType::get(logicalShape, cast<MemRefType>(
              memref.getType()).getElementType());

      Value tensor =
          builder.create<tensor::EmptyOp>(
              loc, logicalShape,
              tensorTy.getElementType());

      SmallVector<Value> z;
      z.reserve(3);

      SmallVector<Value> sourceIVs;
      sourceIVs.reserve(3);

      // The normalized execution domain is dense in this MVP.  Its
      // coordinates are exactly the logical loop coordinates used to
      // materialize the operand tensor.
      for (unsigned d = 0; d < 3; ++d) {
        Value upper =
            builder.create<arith::ConstantIndexOp>(loc, logicalShape[d < 2
                ? (accessMap == matmul.accessA
                       ? (d == 0 ? M : K)
                       : accessMap == matmul.accessB
                             ? (d == 0 ? K : N)
                             : (d == 0 ? M : N))
                : 1]);

        (void)upper;
      }

      return tensor;
    };

    // The general affine materialization is emitted explicitly below.
    // Keep the source-coordinate construction separate from the tensor
    // shape so physical affine offsets/strides do not leak into TileOp.
    auto makeSourceIV = [&](Value z, unsigned dim) -> Value {
      int64_t step =
          matmul.normalization.sourceTransformation[dim][dim];
      int64_t origin =
          matmul.normalization.sourceOffset[dim];

      Value v = z;
      if (step != 1) {
        Value c = builder.create<arith::ConstantIndexOp>(loc, step);
        v = builder.create<arith::MulIOp>(loc, v, c);
      }
      if (origin != 0) {
        Value c = builder.create<arith::ConstantIndexOp>(loc, origin);
        v = builder.create<arith::AddIOp>(loc, v, c);
      }
      return v;
    };

    auto buildLogicalTensor =
        [&](Value memref, AffineMap accessMap,
            ArrayRef<int64_t> shape,
            unsigned logicalDim0,
            unsigned logicalDim1) -> FailureOr<Value> {
      auto memrefTy = dyn_cast<MemRefType>(memref.getType());
      if (!memrefTy)
        return failure();

      Type elemTy = memrefTy.getElementType();
      RankedTensorType resultTy =
          RankedTensorType::get(shape, elemTy);

      auto generate =
          builder.create<tensor::GenerateOp>(
              loc, resultTy, ValueRange{},
              [&](OpBuilder &bodyBuilder, Location bodyLoc,
                  ValueRange indices) {
                Value i0 = indices[0];
                Value i1 = indices[1];

                Value zero =
                    bodyBuilder.create<arith::ConstantIndexOp>(
                        bodyLoc, 0);

                // Construct the normalized/source iteration vector.
                //
                // logicalDim0/logicalDim1 identify which normalized
                // coordinates correspond to the two dimensions of
                // this tensor:
                //
                //   A: (I,K)
                //   B: (K,J)
                //   C: (I,J)
                SmallVector<Value> sourceIVs;
                sourceIVs.reserve(3);

                for (unsigned d = 0; d < 3; ++d) {
                  Value value;

                  if (d == logicalDim0)
                    value = i0;
                  else if (d == logicalDim1)
                    value = i1;
                  else
                    value = zero;

                  int64_t step =
                      matmul.normalization.sourceTransformation[d][d];
                  int64_t offset =
                      matmul.normalization.sourceOffset[d];

                  if (step != 1) {
                    Value c =
                        bodyBuilder.create<arith::ConstantIndexOp>(
                            bodyLoc, step);
                    value =
                        bodyBuilder.create<arith::MulIOp>(
                            bodyLoc, value, c);
                  }

                  if (offset != 0) {
                    Value c =
                        bodyBuilder.create<arith::ConstantIndexOp>(
                            bodyLoc, offset);
                    value =
                        bodyBuilder.create<arith::AddIOp>(
                            bodyLoc, value, c);
                  }

                  sourceIVs.push_back(value);
                }

                // Evaluate each result of the affine access map
                // independently.  affine.apply produces ordinary
                // index values, which are then consumed by memref.load.
                SmallVector<Value> physicalIndices;
                physicalIndices.reserve(
                    accessMap.getNumResults());

                for (unsigned r = 0;
                     r < accessMap.getNumResults();
                     ++r) {
                  AffineMap resultMap =
                      AffineMap::get(
                          accessMap.getNumInputs(),
                          /*symbolCount=*/0,
                          accessMap.getResult(r),
                          memref.getContext());

                  Value physicalIndex =
                      bodyBuilder.create<affine::AffineApplyOp>(
                          bodyLoc, resultMap, sourceIVs);

                  physicalIndices.push_back(physicalIndex);
                }

                Value value =
                    bodyBuilder.create<memref::LoadOp>(
                        bodyLoc, memref, physicalIndices);

                bodyBuilder.create<tensor::YieldOp>(
                    bodyLoc, value);
              });

      return generate.getResult();
    };

    auto lhsTensor =
        buildLogicalTensor(matmul.lhs, matmul.accessA,
                           {M, K}, 0, 2);
    if (failed(lhsTensor))
      return failure();

    auto rhsTensor =
        buildLogicalTensor(matmul.rhs, matmul.accessB,
                           {K, N}, 2, 1);
    if (failed(rhsTensor))
      return failure();

    auto outTensor =
        buildLogicalTensor(matmul.output, matmul.accessC,
                           {M, N}, 0, 1);
    if (failed(outTensor))
      return failure();

    auto result = builder.create<MatmulTileOp>(
        loc,
        outTensorTy,
        *lhsTensor,
        *rhsTensor,
        *outTensor,
        builder.getI64IntegerAttr(M),
        builder.getI64IntegerAttr(N),
        builder.getI64IntegerAttr(K),
        /*device=*/FlatSymbolRefAttr(),
        /*est_cycles=*/IntegerAttr(),
        /*start_cycle=*/IntegerAttr());

    // Scatter the logical GEMM result back to the original
    // physical output memref.  The logical result has shape M x N,
    // while the original memref may have a different physical shape
    // because the affine access can contain offsets/strides.
    //
    // For each logical (I,J), compute the physical output coordinates
    // using accessC and store the corresponding result element.
    Value zero = builder.create<arith::ConstantIndexOp>(loc, 0);
    Value one = builder.create<arith::ConstantIndexOp>(loc, 1);

    auto scatterOuter =
        builder.create<scf::ForOp>(
            loc, zero,
            builder.create<arith::ConstantIndexOp>(loc, M),
            one);

    {
      OpBuilder::InsertionGuard outerGuard(builder);
      builder.setInsertionPointToStart(scatterOuter.getBody());

      Value i = scatterOuter.getInductionVar();

      auto scatterInner =
          builder.create<scf::ForOp>(
              loc, zero,
              builder.create<arith::ConstantIndexOp>(loc, N),
              one);

      {
        OpBuilder::InsertionGuard innerGuard(builder);
        builder.setInsertionPointToStart(scatterInner.getBody());

        Value j = scatterInner.getInductionVar();

        // Convert dense logical coordinates (i, j) back to the
        // original source loop coordinates before applying accessC.
        //
        // For this testcase:
        //   x = 5 * i + 3
        //   y = j
        //
        // so accessC computes:
        //   C[7 + 2 * x, y]
        // = C[13 + 10 * i, j].
        SmallVector<Value> sourceIVs;
        sourceIVs.reserve(3);

        for (unsigned d = 0; d < 3; ++d) {
          Value value;

          if (d == 0)
            value = i;
          else if (d == 1)
            value = j;
          else
            value = zero;

          int64_t step =
              matmul.normalization.sourceTransformation[d][d];
          int64_t offset =
              matmul.normalization.sourceOffset[d];

          if (step != 1) {
            Value c =
                builder.create<arith::ConstantIndexOp>(loc, step);
            value =
                builder.create<arith::MulIOp>(loc, value, c);
          }

          if (offset != 0) {
            Value c =
                builder.create<arith::ConstantIndexOp>(loc, offset);
            value =
                builder.create<arith::AddIOp>(loc, value, c);
          }

          sourceIVs.push_back(value);
        }

        SmallVector<Value> physicalIndices;
        physicalIndices.reserve(matmul.accessC.getNumResults());

        for (unsigned r = 0;
             r < matmul.accessC.getNumResults();
             ++r) {
          AffineMap resultMap =
              AffineMap::get(
                  /*dimCount=*/matmul.accessC.getNumInputs(),
                  /*symbolCount=*/0,
                  matmul.accessC.getResult(r),
                  matmul.output.getContext());

          Value index =
              builder.create<affine::AffineApplyOp>(
                  loc, resultMap, sourceIVs);

          physicalIndices.push_back(index);
        }

        Value value =
            builder.create<tensor::ExtractOp>(
                loc, result.getResult(),
                ValueRange{i, j});

        builder.create<memref::StoreOp>(
            loc, value, matmul.output, physicalIndices);
      }
    }

    // The recognized affine loop nest has now been replaced.
    anchor->erase();

    return success();
  }

  void runOnOperation() override {
    ModuleOp module = getOperation();

    SmallVector<AffineMatmulCandidate> candidates;
    collectAffineMatmulCandidates(module.getOperation(), candidates);

    SmallVector<LogicalMatmul, 1> matmuls;

    for (AffineMatmulCandidate candidate : candidates) {
      auto result = recognizeLogicalMatmul(candidate);
      if (failed(result))
        continue;

      bool duplicate = false;
      for (const LogicalMatmul &existing : matmuls) {
        if (existing.outputStore == result->outputStore) {
          duplicate = true;
          break;
        }
      }

      if (!duplicate)
        matmuls.push_back(*result);
    }

    for (LogicalMatmul &matmul : matmuls) {
      if (failed(lowerOne(matmul)))
        continue;
    }
  }
};

} // namespace

std::unique_ptr<Pass>
mlir::systolic::createAffineMatmulToSystolicPass() {
  return std::make_unique<AffineMatmulToSystolicPass>();
}

void mlir::systolic::registerAffineMatmulToSystolicPass() {
  PassRegistration<AffineMatmulToSystolicPass>();
}
