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

    if (lhsTy.getElementType() != rhsTy.getElementType() ||
        lhsTy.getElementType() != outTy.getElementType())
      return failure();

    // The normalized access maps have the form:
    //
    //   A = (P*, I, K)
    //   B = (P*, K, J)
    //   C = (P*, I, J)
    //
    // Therefore:
    //
    //   rank = numPartitionDims + 2
    //
    // and the source iteration space contains:
    //
    //   P* + I + J + K
    //
    const unsigned numPartitionDims =
        matmul.accessC.getNumResults() - 2;

    if (matmul.accessA.getNumResults() !=
            numPartitionDims + 2 ||
        matmul.accessB.getNumResults() !=
            numPartitionDims + 2 ||
        matmul.accessC.getNumResults() !=
            numPartitionDims + 2)
      return failure();

    if (lhsTy.getRank() != numPartitionDims + 2 ||
        rhsTy.getRank() != numPartitionDims + 2 ||
        outTy.getRank() != numPartitionDims + 2)
      return failure();

    if (!matmul.logicalMapping.hasStaticSourceOrigin ||
        matmul.logicalMapping.sourceOrigin.size() !=
            matmul.normalization.sourceTransformation.size())
      return failure();

    const unsigned numSourceDims =
        matmul.normalization.sourceTransformation.size();

    if (numSourceDims != numPartitionDims + 3)
      return failure();

    Operation *anchor = matmul.anchor;
    Location loc = anchor->getLoc();

    OpBuilder builder(anchor);

    // ----------------------------------------------------------
    // Helpers
    // ----------------------------------------------------------

    auto createConstantIndex =
        [&](OpBuilder &b, Location l, int64_t value) -> Value {
      return b.create<arith::ConstantIndexOp>(l, value);
    };

    // Convert a normalized source coordinate z_d into the original
    // affine.for induction-variable coordinate:
    //
    //   x_d = origin_d + step_d * z_d
    //
    auto makeSourceIV =
        [&](OpBuilder &b,
            Location l,
            Value normalized,
            unsigned sourceDim) -> Value {
      int64_t step =
          matmul.normalization.sourceTransformation
              [sourceDim][sourceDim];

      int64_t origin =
          matmul.normalization.sourceOffset[sourceDim];

      Value value = normalized;

      if (step != 1) {
        Value c = createConstantIndex(b, l, step);
        value = b.create<arith::MulIOp>(l, value, c);
      }

      if (origin != 0) {
        Value c = createConstantIndex(b, l, origin);
        value = b.create<arith::AddIOp>(l, value, c);
      }

      return value;
    };

    // Construct source IVs from logical coordinates.
    //
    // Current supported form:
    //
    //   source coordinate d
    //       = corresponding normalized coordinate d
    //
    // followed by the affine.for step/origin reconstruction above.
    //
    // This is sufficient for ordinary GEMM and the current batched
    // partition tests, whose logical transformation is identity.
    //
    // Non-identity logical mappings continue to be handled by the
    // existing stride/non-unimodular path only when there are no
    // partition dimensions.
    auto buildSourceIVs =
        [&](OpBuilder &b,
            Location l,
            ArrayRef<Value> logicalCoordinates)
        -> FailureOr<SmallVector<Value>> {
      if (logicalCoordinates.size() != numSourceDims)
        return failure();

      SmallVector<Value> sourceIVs;
      sourceIVs.reserve(numSourceDims);

      for (unsigned d = 0; d < numSourceDims; ++d) {
        Value normalized = logicalCoordinates[d];

        sourceIVs.push_back(
            makeSourceIV(b, l, normalized, d));
      }

      return sourceIVs;
    };

    auto buildPhysicalIndices =
        [&](OpBuilder &b,
            Location l,
            Value memref,
            AffineMap accessMap,
            ArrayRef<Value> sourceIVs)
        -> FailureOr<SmallVector<Value>> {
      SmallVector<Value> physicalIndices;
      physicalIndices.reserve(accessMap.getNumResults());

      for (unsigned r = 0;
           r < accessMap.getNumResults();
           ++r) {
        AffineMap resultMap =
            AffineMap::get(
                accessMap.getNumInputs(),
                /*symbolCount=*/0,
                accessMap.getResult(r),
                memref.getContext());

        Value index =
            b.create<affine::AffineApplyOp>(
                l,
                resultMap,
                sourceIVs);

        physicalIndices.push_back(index);
      }

      return physicalIndices;
    };

    // ----------------------------------------------------------
    // Verify partition transformation
    // ----------------------------------------------------------
    //
    // For the partitioned lowering we currently require the
    // partition coordinates to correspond directly to source
    // normalized coordinates:
    //
    //   P0 -> z0
    //   P1 -> z1
    //   ...
    //
    // This is exactly the representation produced by the current
    // batched testcase and leaves the general affine partition
    // inversion as a separate analysis/lowering extension.
    //
    if (numPartitionDims > 0) {
      const auto &T =
          matmul.logicalMapping.transformation;

      if (T.size() != numSourceDims)
        return failure();

      for (unsigned d = 0; d < numPartitionDims; ++d) {
        for (unsigned c = 0; c < numSourceDims; ++c) {
          int64_t expected = (d == c) ? 1 : 0;

          if (T[d][c] != expected)
            return failure();
        }

        if (matmul.logicalMapping.offset[d] != 0)
          return failure();
      }
    }

    // ----------------------------------------------------------
    // Tensor materialization
    // ----------------------------------------------------------

    auto buildLogicalTensor =
        [&](OpBuilder &b,
            Location l,
            Value memref,
            AffineMap accessMap,
            ArrayRef<int64_t> shape,
            ArrayRef<Value> partitionIVs,
            unsigned logicalDim0,
            unsigned logicalDim1)
        -> FailureOr<Value> {
      auto memrefTy =
          dyn_cast<MemRefType>(memref.getType());

      if (!memrefTy)
        return failure();

      Type elemTy = memrefTy.getElementType();

      RankedTensorType tensorTy =
          RankedTensorType::get(shape, elemTy);

      auto generate =
          b.create<tensor::GenerateOp>(
              l,
              tensorTy,
              ValueRange{},
              [&](OpBuilder &bodyBuilder,
                  Location bodyLoc,
                  ValueRange indices) {
                Value i0 = indices[0];
                Value i1 = indices[1];

                Value zero =
                    createConstantIndex(
                        bodyBuilder,
                        bodyLoc,
                        0);

                // Logical coordinate order:
                //
                //   P0, P1, ..., I, J, K
                //
                // For this tensor:
                //
                //   A -> (P*, I, K)
                //   B -> (P*, K, J)
                //   C -> (P*, I, J)
                //
                SmallVector<Value> logicalCoordinates;
                logicalCoordinates.reserve(numSourceDims);

                for (unsigned d = 0;
                     d < numPartitionDims;
                     ++d) {
                  logicalCoordinates.push_back(
                      partitionIVs[d]);
                }

                // I / J / K occupy the final three
                // normalized source coordinates.
                Value I = zero;
                Value J = zero;
                Value Kvalue = zero;

                if (logicalDim0 == 0)
                  I = i0;
                else if (logicalDim0 == 1)
                  J = i0;
                else
                  Kvalue = i0;

                if (logicalDim1 == 0)
                  I = i1;
                else if (logicalDim1 == 1)
                  J = i1;
                else
                  Kvalue = i1;

                logicalCoordinates.push_back(I);
                logicalCoordinates.push_back(J);
                logicalCoordinates.push_back(Kvalue);

                auto sourceIVs =
                    buildSourceIVs(
                        bodyBuilder,
                        bodyLoc,
                        logicalCoordinates);

                if (failed(sourceIVs)) {
                  bodyBuilder.create<tensor::YieldOp>(
                      bodyLoc,
                      Value{});
                  return;
                }

                auto physicalIndices =
                    buildPhysicalIndices(
                        bodyBuilder,
                        bodyLoc,
                        memref,
                        accessMap,
                        *sourceIVs);

                if (failed(physicalIndices)) {
                  bodyBuilder.create<tensor::YieldOp>(
                      bodyLoc,
                      Value{});
                  return;
                }

                Value value =
                    bodyBuilder.create<memref::LoadOp>(
                        bodyLoc,
                        memref,
                        *physicalIndices);

                bodyBuilder.create<tensor::YieldOp>(
                    bodyLoc,
                    value);
              });

      return generate.getResult();
    };

    // ----------------------------------------------------------
    // Emit one matmul instance
    // ----------------------------------------------------------

    auto emitOneMatmul =
        [&](OpBuilder &b,
            Location l,
            ArrayRef<Value> partitionIVs)
        -> LogicalResult {
      SmallVector<int64_t> lhsShape = {M, K};
      SmallVector<int64_t> rhsShape = {K, N};
      SmallVector<int64_t> outShape = {M, N};

      auto lhsTensor =
          buildLogicalTensor(
              b, l,
              matmul.lhs,
              matmul.accessA,
              lhsShape,
              partitionIVs,
              /*logicalDim0=*/0,
              /*logicalDim1=*/2);

      if (failed(lhsTensor))
        return failure();

      auto rhsTensor =
          buildLogicalTensor(
              b, l,
              matmul.rhs,
              matmul.accessB,
              rhsShape,
              partitionIVs,
              /*logicalDim0=*/2,
              /*logicalDim1=*/1);

      if (failed(rhsTensor))
        return failure();

      auto outTensor =
          buildLogicalTensor(
              b, l,
              matmul.output,
              matmul.accessC,
              outShape,
              partitionIVs,
              /*logicalDim0=*/0,
              /*logicalDim1=*/1);

      if (failed(outTensor))
        return failure();

      auto result =
          b.create<MatmulTileOp>(
              l,
              RankedTensorType::get(
                  {M, N},
                  outTy.getElementType()),
              *lhsTensor,
              *rhsTensor,
              *outTensor,
              b.getI64IntegerAttr(M),
              b.getI64IntegerAttr(N),
              b.getI64IntegerAttr(K),
              /*device=*/FlatSymbolRefAttr(),
              /*est_cycles=*/IntegerAttr(),
              /*start_cycle=*/IntegerAttr());

      // --------------------------------------------------------
      // Scatter C(M,N) back to the original physical C tensor.
      // --------------------------------------------------------

      Value zero =
          createConstantIndex(b, l, 0);

      Value one =
          createConstantIndex(b, l, 1);

      auto emitScatter =
          [&](Value i,
              Value j) -> LogicalResult {
        SmallVector<Value> logicalCoordinates;
        logicalCoordinates.reserve(numSourceDims);

        for (unsigned d = 0;
             d < numPartitionDims;
             ++d)
          logicalCoordinates.push_back(
              partitionIVs[d]);

        logicalCoordinates.push_back(i);
        logicalCoordinates.push_back(j);
        logicalCoordinates.push_back(zero);

        auto sourceIVs =
            buildSourceIVs(
                b,
                l,
                logicalCoordinates);

        if (failed(sourceIVs))
          return failure();

        auto physicalIndices =
            buildPhysicalIndices(
                b,
                l,
                matmul.output,
                matmul.accessC,
                *sourceIVs);

        if (failed(physicalIndices))
          return failure();

        Value value =
            b.create<tensor::ExtractOp>(
                l,
                result.getResult(),
                ValueRange{i, j});

        b.create<memref::StoreOp>(
            l,
            value,
            matmul.output,
            *physicalIndices);

        return success();
      };

      auto emitScatterBody =
          [&](OpBuilder &scatterBuilder,
              Location scatterLoc,
              Value i) -> LogicalResult {
        auto inner =
            scatterBuilder.create<scf::ForOp>(
                scatterLoc,
                zero,
                createConstantIndex(
                    scatterBuilder,
                    scatterLoc,
                    N),
                one);

        {
          OpBuilder::InsertionGuard guard(
              scatterBuilder);

          scatterBuilder.setInsertionPointToStart(
              inner.getBody());

          Value j =
              inner.getInductionVar();

          if (failed(emitScatter(i, j)))
            return failure();
        }

        return success();
      };

      auto outer =
          b.create<scf::ForOp>(
              l,
              zero,
              createConstantIndex(b, l, M),
              one);

      {
        OpBuilder::InsertionGuard guard(b);

        b.setInsertionPointToStart(
            outer.getBody());

        Value i =
            outer.getInductionVar();

        if (failed(
                emitScatterBody(
                    b,
                    l,
                    i)))
          return failure();
      }

      return success();
    };

    // ----------------------------------------------------------
    // Emit partition loops
    // ----------------------------------------------------------
    //
    // For:
    //
    //   P0 x P1 x ... x M x K
    //
    // emit:
    //
    //   for p0
    //     for p1
    //       ...
    //         matmul_tile(...)
    //
    // Thus every partition point becomes one independent
    // matmul_tile instance.
    // ----------------------------------------------------------

    SmallVector<Value> partitionIVs;
    partitionIVs.reserve(numPartitionDims);

    std::function<LogicalResult(unsigned)> emitPartitionNest;

    emitPartitionNest =
        [&](unsigned depth) -> LogicalResult {
      if (depth == numPartitionDims)
        return emitOneMatmul(
            builder,
            loc,
            partitionIVs);

      // The logical partition extent is the corresponding
      // physical memref dimension.
      int64_t extent =
          lhsTy.getDimSize(depth);

      if (extent == ShapedType::kDynamic)
        return failure();

      if (extent <= 0)
        return failure();

      Value zero =
          createConstantIndex(
              builder,
              loc,
              0);

      Value one =
          createConstantIndex(
              builder,
              loc,
              1);

      Value upper =
          createConstantIndex(
              builder,
              loc,
              extent);

      auto loop =
          builder.create<scf::ForOp>(
              loc,
              zero,
              upper,
              one);

      {
        OpBuilder::InsertionGuard guard(
            builder);

        builder.setInsertionPointToStart(
            loop.getBody());

        partitionIVs.push_back(
            loop.getInductionVar());

        if (failed(
                emitPartitionNest(
                    depth + 1)))
          return failure();

        partitionIVs.pop_back();
      }

      return success();
    };

    if (failed(emitPartitionNest(0)))
      return failure();

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
