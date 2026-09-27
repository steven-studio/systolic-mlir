#include "Systolic/AffineMatmulAnalysis.h"
#include "Systolic/Passes.h"
#include "Systolic/SystolicOps.h"

#include "mlir/Dialect/Affine/Analysis/Utils.h"
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
        !matmul.normalization.domain) {
      llvm::errs() << "[AffineMatmulToSystolic] LOWER_FAIL #1"
                   << " at lowerOne-relative line 5\n";
      llvm::errs() << "LOWER_FAIL_01\n";
      return failure();
    }

    if (matmul.rowExtent.kind != LogicalExtent::Kind::Static ||
        matmul.columnExtent.kind != LogicalExtent::Kind::Static ||
        matmul.reductionExtent.kind != LogicalExtent::Kind::Static) {
      llvm::errs() << "[AffineMatmulToSystolic] LOWER_FAIL #2"
                   << " at lowerOne-relative line 10\n";
      llvm::errs() << "LOWER_FAIL_02\n";
      return failure();
    }

    const int64_t M = matmul.rowExtent.staticValue;
    const int64_t N = matmul.columnExtent.staticValue;
    const int64_t K = matmul.reductionExtent.staticValue;

    if (M <= 0 || N <= 0 || K <= 0) {
      llvm::errs() << "[AffineMatmulToSystolic] LOWER_FAIL #3"
                   << " at lowerOne-relative line 17\n";
      llvm::errs() << "LOWER_FAIL_03\n";
      return failure();
    }


    auto lhsTy =
        dyn_cast<MemRefType>(matmul.lhs.getType());
    auto rhsTy =
        dyn_cast<MemRefType>(matmul.rhs.getType());

    auto outTy =
        dyn_cast<MemRefType>(matmul.output.getType());

    if (!lhsTy || !rhsTy || !outTy) {
      llvm::errs() << "[AffineMatmulToSystolic] LOWER_FAIL #4\n";
      llvm::errs() << "  lhs type: " << matmul.lhs.getType() << "\n";
      llvm::errs() << "  rhs type: " << matmul.rhs.getType() << "\n";
      llvm::errs() << "  output type: " << matmul.output.getType() << "\n";
      llvm::errs() << "  lhs is MemRefType: " << (lhsTy ? "yes" : "no") << "\n";
      llvm::errs() << "  rhs is MemRefType: " << (rhsTy ? "yes" : "no") << "\n";
      llvm::errs() << "  output is MemRefType: " << (outTy ? "yes" : "no") << "\n";
      llvm::errs() << "LOWER_FAIL_04\n";
      return failure();
    }

    if (!lhsTy.hasStaticShape() ||
        !rhsTy.hasStaticShape() ||
        !outTy.hasStaticShape()) {
      llvm::errs() << "[AffineMatmulToSystolic] LOWER_FAIL #5\n";
      llvm::errs() << "  lhs type: " << matmul.lhs.getType() << "\n";
      llvm::errs() << "  rhs type: " << matmul.rhs.getType() << "\n";
      llvm::errs() << "  output type: " << matmul.output.getType() << "\n";
      llvm::errs() << "  lhs static shape: "
                   << (lhsTy.hasStaticShape() ? "yes" : "no") << "\n";
      llvm::errs() << "  rhs static shape: "
                   << (rhsTy.hasStaticShape() ? "yes" : "no") << "\n";
      llvm::errs() << "  output static shape: "
                   << (outTy.hasStaticShape() ? "yes" : "no") << "\n";
      llvm::errs() << "LOWER_FAIL_05\n";
      return failure();
    }

    if (lhsTy.getElementType() != rhsTy.getElementType() ||
        lhsTy.getElementType() != outTy.getElementType()) {
      llvm::errs() << "[AffineMatmulToSystolic] LOWER_FAIL #6\n";
      llvm::errs() << "  lhs element type: "
                   << lhsTy.getElementType() << "\n";
      llvm::errs() << "  rhs element type: "
                   << rhsTy.getElementType() << "\n";
      llvm::errs() << "  output element type: "
                   << outTy.getElementType() << "\n";
      llvm::errs() << "LOWER_FAIL_06\n";
      return failure();
    }

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
            numPartitionDims + 2) {
      llvm::errs() << "[AffineMatmulToSystolic] LOWER_FAIL #7\n";
      llvm::errs() << "  numPartitionDims: "
                   << numPartitionDims << "\n";
      llvm::errs() << "  accessA results: "
                   << matmul.accessA.getNumResults() << "\n";
      llvm::errs() << "  accessB results: "
                   << matmul.accessB.getNumResults() << "\n";
      llvm::errs() << "  accessC results: "
                   << matmul.accessC.getNumResults() << "\n";
      llvm::errs() << "  accessA map: "
                   << matmul.accessA << "\n";
      llvm::errs() << "  accessB map: "
                   << matmul.accessB << "\n";
      llvm::errs() << "  accessC map: "
                   << matmul.accessC << "\n";
      llvm::errs() << "LOWER_FAIL_07\n";
      return failure();
    }

    if (lhsTy.getRank() != numPartitionDims + 2 ||
        rhsTy.getRank() != numPartitionDims + 2 ||
        outTy.getRank() != numPartitionDims + 2) {
      llvm::errs() << "[AffineMatmulToSystolic] LOWER_FAIL #8\n";
      llvm::errs() << "  expected rank: "
                   << numPartitionDims + 2 << "\n";
      llvm::errs() << "  lhs rank: "
                   << lhsTy.getRank() << "\n";
      llvm::errs() << "  rhs rank: "
                   << rhsTy.getRank() << "\n";
      llvm::errs() << "  output rank: "
                   << outTy.getRank() << "\n";
      llvm::errs() << "LOWER_FAIL_08\n";
      return failure();
    }

    if (!matmul.logicalMapping.hasStaticSourceOrigin ||
        matmul.logicalMapping.sourceOrigin.size() !=
            matmul.normalization.sourceTransformation.size()) {
      llvm::errs() << "[AffineMatmulToSystolic] LOWER_FAIL #9\n";
      llvm::errs() << "  hasStaticSourceOrigin: "
                   << (matmul.logicalMapping.hasStaticSourceOrigin
                           ? "yes"
                           : "no")
                   << "\n";
      llvm::errs() << "  sourceOrigin size: "
                   << matmul.logicalMapping.sourceOrigin.size()
                   << "\n";
      llvm::errs() << "  sourceTransformation size: "
                   << matmul.normalization.sourceTransformation.size()
                   << "\n";
      llvm::errs() << "LOWER_FAIL_09\n";
      return failure();
    }

    const unsigned numSourceDims =
        matmul.normalization.sourceTransformation.size();

    if (numSourceDims != numPartitionDims + 3) {
      llvm::errs() << "[AffineMatmulToSystolic] LOWER_FAIL #10\n";
      llvm::errs() << "  numPartitionDims: "
                   << numPartitionDims << "\n";
      llvm::errs() << "  expected numSourceDims: "
                   << numPartitionDims + 3 << "\n";
      llvm::errs() << "  actual numSourceDims: "
                   << numSourceDims << "\n";
      llvm::errs() << "  sourceTransformation size: "
                   << matmul.normalization.sourceTransformation.size()
                   << "\n";
      llvm::errs() << "  sourceOrigin size: "
                   << matmul.logicalMapping.sourceOrigin.size()
                   << "\n";
      llvm::errs() << "LOWER_FAIL_10\n";
      return failure();
    }

    Operation *anchor = matmul.anchor;
    Location loc = anchor->getLoc();

    OpBuilder builder(anchor);

    // ----------------------------------------------------------
    // Recover the complete original enclosing affine loop chain.
    // ----------------------------------------------------------
    //
    // The analysis intentionally keeps only IVs that participate in
    // memory accesses as logical GEMM coordinates.
    //
    // Therefore an enclosing loop may be absent from sourceDomain
    // even though it still has execution semantics.
    //
    // Example:
    //
    //   affine.for %p2 = 0 to 4 {
    //     affine.for %i = 0 to 100 {
    //       affine.for %p1 = 0 to %p {
    //         affine.for %j = 0 to 100 {
    //           affine.for %k = 0 to 100 {
    //             ...
    //
    // Logical coordinates:
    //
    //   %i, %j, %k
    //
    // Preserved execution loops:
    //
    //   %p2, %p1
    //
    // We must preserve the latter when replacing the original
    // affine nest with systolic.matmul_tile.
    // ----------------------------------------------------------

    SmallVector<affine::AffineForOp> enclosingLoops;
    affine::getAffineForIVs(
        *matmul.outputStore,
        &enclosingLoops);

    if (enclosingLoops.empty()) {
      llvm::errs() << "[AffineMatmulToSystolic] LOWER_FAIL #11\n";
      llvm::errs() << "  enclosing affine loop count: "
                   << enclosingLoops.size() << "\n";
      llvm::errs() << "  anchor: ";
      anchor->print(llvm::errs());
      llvm::errs() << "\n";
      llvm::errs() << "  outputStore: ";
      matmul.outputStore->print(llvm::errs());
      llvm::errs() << "\n";

      llvm::errs() << "  outputStore parent chain:\n";
      Operation *parent = matmul.outputStore->getParentOp();
      unsigned depth = 0;
      while (parent && depth < 16) {
        llvm::errs() << "    [" << depth << "] "
                     << parent->getName() << "\n";
        parent = parent->getParentOp();
        ++depth;
      }

      llvm::errs() << "LOWER_FAIL_11\n";
      return failure();
    }

    // The candidate anchor must be the outermost affine loop
    // surrounding the recognized MAC. Otherwise another enclosing
    // candidate could cause duplicated reconstruction.
    if (enclosingLoops.front().getOperation() != anchor) {
      llvm::errs() << "[AffineMatmulToSystolic] LOWER_FAIL #12\n";

      llvm::errs() << "  anchor: ";
      anchor->print(llvm::errs());
      llvm::errs() << "\n";

      llvm::errs() << "  anchor name: "
                   << anchor->getName() << "\n";

      llvm::errs() << "  outermost enclosing loop: ";
      enclosingLoops.front()->print(llvm::errs());
      llvm::errs() << "\n";

      llvm::errs() << "  outermost loop name: "
                   << enclosingLoops.front()->getName() << "\n";

      llvm::errs() << "  enclosing affine loop count: "
                   << enclosingLoops.size() << "\n";

      llvm::errs() << "  enclosing affine loops:\n";
      for (unsigned i = 0; i < enclosingLoops.size(); ++i) {
        llvm::errs() << "    [" << i << "] ";
        enclosingLoops[i]->print(llvm::errs());
        llvm::errs() << "\n";
      }

      llvm::errs() << "  anchor parent chain:\n";
      Operation *anchorParent = anchor->getParentOp();
      unsigned anchorDepth = 0;
      while (anchorParent && anchorDepth < 16) {
        llvm::errs() << "    [" << anchorDepth << "] "
                     << anchorParent->getName() << "\n";
        anchorParent = anchorParent->getParentOp();
        ++anchorDepth;
      }

      llvm::errs() << "LOWER_FAIL_12\n";
      return failure();
    }


    if (!matmul.sourceDomain ||
        matmul.sourceDomain->getNumDimVars() != numSourceDims) {
      llvm::errs() << "[AffineMatmulToSystolic] LOWER_FAIL #13\n";

      llvm::errs() << "  numSourceDims: "
                   << numSourceDims << "\n";

      llvm::errs() << "  has sourceDomain: "
                   << (matmul.sourceDomain ? "yes" : "no") << "\n";

      if (matmul.sourceDomain) {
        llvm::errs() << "  sourceDomain num dims: "
                     << matmul.sourceDomain->getNumDimVars() << "\n";

        llvm::errs() << "  sourceDomain num symbols: "
                     << matmul.sourceDomain->getNumSymbolVars() << "\n";

        llvm::errs() << "  sourceDomain num locals: "
                     << matmul.sourceDomain->getNumLocalVars() << "\n";

        llvm::errs() << "  sourceDomain values:\n";
        for (unsigned i = 0;
             i < matmul.sourceDomain->getNumDimVars();
             ++i) {
          llvm::errs() << "    [" << i << "] ";
          matmul.sourceDomain->getValue(i).print(llvm::errs());
          llvm::errs() << "\n";
        }
      }

      llvm::errs() << "  sourceTransformation size: "
                   << matmul.normalization.sourceTransformation.size()
                   << "\n";

      llvm::errs() << "  sourceOrigin size: "
                   << matmul.logicalMapping.sourceOrigin.size()
                   << "\n";

      llvm::errs() << "  logicalMapping source dimension count: "
                   << matmul.logicalMapping.sourceOrigin.size()
                   << "\n";

      llvm::errs() << "  enclosing affine loop count: "
                   << enclosingLoops.size() << "\n";

      llvm::errs() << "  enclosing loops:\n";
      for (unsigned i = 0; i < enclosingLoops.size(); ++i) {
        llvm::errs() << "    [" << i << "] ";
        enclosingLoops[i]->print(llvm::errs());
        llvm::errs() << "\n";
      }

      llvm::errs() << "LOWER_FAIL_13\n";
      return failure();
    }

    // sourceDomain dimensions are exactly the logical source IVs
    // retained by AffineMatmulAnalysis.
    SmallVector<Value> logicalIVs;
    matmul.sourceDomain->getValues(
        /*start=*/0,
        /*end=*/numSourceDims,
        &logicalIVs);

    if (logicalIVs.size() != numSourceDims) {
      llvm::errs() << "[AffineMatmulToSystolic] LOWER_FAIL #14\n";
      llvm::errs() << "  logicalIVs size: "
                   << logicalIVs.size() << "\n";
      llvm::errs() << "  expected numSourceDims: "
                   << numSourceDims << "\n";
      llvm::errs() << "LOWER_FAIL_14\n";
      return failure();
    }

    SmallVector<affine::AffineForOp> preservedLoops;

    for (affine::AffineForOp loop : enclosingLoops) {
      Value iv = loop.getInductionVar();

      if (llvm::find(logicalIVs, iv) == logicalIVs.end())
        preservedLoops.push_back(loop);
    }

    llvm::errs() << "  enclosing affine loops: "
                 << enclosingLoops.size() << "\n";
    llvm::errs() << "  logical GEMM loops: "
                 << logicalIVs.size() << "\n";
    llvm::errs() << "  preserved execution loops: "
                 << preservedLoops.size() << "\n";

    for (affine::AffineForOp loop : preservedLoops) {
      llvm::errs() << "    preserved IV: "
                   << loop.getInductionVar()
                   << "\n";
    }

    // A preserved loop cannot have a bound that depends on a logical
    // loop that is going to be erased.
    //
    // Direct IV dependencies are caught explicitly.  We also reject
    // values defined inside a logical loop, which covers affine.apply
    // results computed from removed logical IVs.
    auto dependsOnRemovedLogicalLoop =
        [&](Value value) -> bool {
      if (llvm::find(logicalIVs, value) != logicalIVs.end())
        return true;

      Operation *def = value.getDefiningOp();
      if (!def)
        return false;

      for (Value logicalIV : logicalIVs) {
        auto owner =
            affine::getForInductionVarOwner(logicalIV);

        if (owner && owner->isAncestor(def))
          return true;
      }

      return false;
    };

    for (affine::AffineForOp loop : preservedLoops) {
      for (Value operand : loop.getLowerBoundOperands()) {
        if (dependsOnRemovedLogicalLoop(operand)) {
          llvm::errs()
              << "[AffineMatmulToSystolic] LOWER_FAIL #15\n";
          llvm::errs() << "LOWER_FAIL_15\n";
          return failure();
        }
      }

      for (Value operand : loop.getUpperBoundOperands()) {
        if (dependsOnRemovedLogicalLoop(operand)) {
          llvm::errs()
              << "[AffineMatmulToSystolic] LOWER_FAIL #16\n";
          llvm::errs() << "LOWER_FAIL_16\n";
          return failure();
        }
      }
    }

    // Mapping from an original preserved-loop IV to the newly
    // reconstructed preserved-loop IV.
    IRMapping preservedIVMapping;

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
      if (logicalCoordinates.size() != numSourceDims) {
        llvm::errs()
            << "[AffineMatmulToSystolic] LOWER_FAIL #17\n";
        llvm::errs() << "LOWER_FAIL_17\n";
        return failure();
      }

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

      if (T.size() != numSourceDims) {
        llvm::errs()
            << "[AffineMatmulToSystolic] LOWER_FAIL #18\n";
        llvm::errs() << "LOWER_FAIL_18\n";
        return failure();
      }

      for (unsigned d = 0; d < numPartitionDims; ++d) {
        for (unsigned c = 0; c < numSourceDims; ++c) {
          int64_t expected = (d == c) ? 1 : 0;

          if (T[d][c] != expected) {
            llvm::errs()
                << "[AffineMatmulToSystolic] LOWER_FAIL #19\n";
            llvm::errs() << "LOWER_FAIL_19\n";
            return failure();
          }
        }

        if (matmul.logicalMapping.offset[d] != 0) {
          llvm::errs()
              << "[AffineMatmulToSystolic] LOWER_FAIL #20\n";
          llvm::errs() << "LOWER_FAIL_20\n";
          return failure();
        }
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

      if (!memrefTy) {
        llvm::errs()
            << "[AffineMatmulToSystolic] LOWER_FAIL #21\n";
        llvm::errs() << "LOWER_FAIL_21\n";
        return failure();
      }

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

      if (failed(lhsTensor)) {
        llvm::errs()
            << "[AffineMatmulToSystolic] LOWER_FAIL #22\n";
        llvm::errs() << "LOWER_FAIL_22\n";
        return failure();
      }

      auto rhsTensor =
          buildLogicalTensor(
              b, l,
              matmul.rhs,
              matmul.accessB,
              rhsShape,
              partitionIVs,
              /*logicalDim0=*/2,
              /*logicalDim1=*/1);

      if (failed(rhsTensor)) {
        llvm::errs()
            << "[AffineMatmulToSystolic] LOWER_FAIL #23\n";
        llvm::errs() << "LOWER_FAIL_23\n";
        return failure();
      }

      auto outTensor =
          buildLogicalTensor(
              b, l,
              matmul.output,
              matmul.accessC,
              outShape,
              partitionIVs,
              /*logicalDim0=*/0,
              /*logicalDim1=*/1);

      if (failed(outTensor)) {
        llvm::errs()
            << "[AffineMatmulToSystolic] LOWER_FAIL #24\n";
        llvm::errs() << "LOWER_FAIL_24\n";
        return failure();
      }

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

        if (failed(sourceIVs)) {
          llvm::errs()
              << "[AffineMatmulToSystolic] LOWER_FAIL #25\\n";
          llvm::errs() << "LOWER_FAIL_25\\n";
          return failure();
        }

        auto physicalIndices =
            buildPhysicalIndices(
                b,
                l,
                matmul.output,
                matmul.accessC,
                *sourceIVs);

        if (failed(physicalIndices)) {
          llvm::errs()
              << "[AffineMatmulToSystolic] LOWER_FAIL #26\\n";
          llvm::errs() << "LOWER_FAIL_26\\n";
          return failure();
        }

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

          if (failed(emitScatter(i, j))) {
            llvm::errs()
                << "[AffineMatmulToSystolic] LOWER_FAIL #27\\n";
            llvm::errs() << "LOWER_FAIL_27\\n";
            return failure();
          }
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
                    i))) {
          llvm::errs()
              << "[AffineMatmulToSystolic] LOWER_FAIL #28\\n";
          llvm::errs() << "LOWER_FAIL_28\\n";
          return failure();
        }
      }

      return success();
    };

    // ----------------------------------------------------------
    // Symbolic source bounds that are not represented by logical
    // partition dimensions may change the number of GEMM
    // invocations.  Do not silently erase those loops.
    // ----------------------------------------------------------

    if (matmul.sourceDomain->getNumSymbolVars() > 0 &&
        numPartitionDims == 0) {
      llvm::errs() << "[AffineMatmulToSystolic] LOWER_FAIL #29" << " at lowerOne-relative line 677\\n";
      llvm::errs() << "LOWER_FAIL_29\\n";
      return failure();
    }

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
        llvm::errs() << "[AffineMatmulToSystolic] LOWER_FAIL #30" << " at lowerOne-relative line 718\\n";
        llvm::errs() << "LOWER_FAIL_30\\n";
        return failure();

      if (extent <= 0)
        llvm::errs() << "[AffineMatmulToSystolic] LOWER_FAIL #31" << " at lowerOne-relative line 721\\n";
        llvm::errs() << "LOWER_FAIL_31\\n";
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
          llvm::errs() << "[AffineMatmulToSystolic] LOWER_FAIL #32" << " at lowerOne-relative line 761\\n";
          llvm::errs() << "LOWER_FAIL_32\\n";
          return failure();

        partitionIVs.pop_back();
      }

      return success();
    };

    // ----------------------------------------------------------
    // Reconstruct preserved execution loops.
    // ----------------------------------------------------------
    //
    // The logical GEMM loops (I/J/K) disappear because their work
    // is represented by systolic.matmul_tile.
    //
    // Any enclosing loop that was NOT a logical GEMM coordinate
    // must remain.  Otherwise we silently change the number of
    // times the GEMM executes.
    //
    // Example:
    //
    //   affine.for %p2 = 0 to 4 {
    //     affine.for %i = 0 to 100 {
    //       affine.for %p1 = 0 to %p {
    //         affine.for %j = 0 to 100 {
    //           affine.for %k = 0 to 100 {
    //             MAC
    //
    // becomes:
    //
    //   affine.for %p2 = 0 to 4 {
    //     affine.for %p1 = 0 to %p {
    //       systolic.matmul_tile
    //
    // The original execution multiplicity is therefore preserved.
    // ----------------------------------------------------------

    std::function<LogicalResult(unsigned)> emitPreservedLoopNest;

    emitPreservedLoopNest =
        [&](unsigned depth) -> LogicalResult {
      if (depth == preservedLoops.size())
        return emitPartitionNest(0);

      affine::AffineForOp originalLoop =
          preservedLoops[depth];

      SmallVector<Value> lbOperands;
      SmallVector<Value> ubOperands;

      lbOperands.reserve(
          originalLoop.getLowerBoundOperands().size());
      ubOperands.reserve(
          originalLoop.getUpperBoundOperands().size());

      for (Value operand :
           originalLoop.getLowerBoundOperands()) {
        Value mapped =
            preservedIVMapping.lookupOrDefault(operand);

        // Values belonging to removed logical loops have already
        // been rejected above.  At this point an unmapped value is
        // therefore defined outside the reconstructed nest.
        lbOperands.push_back(mapped);
      }

      for (Value operand :
           originalLoop.getUpperBoundOperands()) {
        Value mapped =
            preservedIVMapping.lookupOrDefault(operand);

        ubOperands.push_back(mapped);
      }

      auto reconstructed =
          affine::AffineForOp::create(
              builder,
              originalLoop.getLoc(),
              lbOperands,
              originalLoop.getLowerBoundMap(),
              ubOperands,
              originalLoop.getUpperBoundMap(),
              originalLoop.getStepAsInt());

      // Map the old loop IV to the reconstructed IV so nested
      // preserved-loop bounds can refer to it.
      preservedIVMapping.map(
          originalLoop.getInductionVar(),
          reconstructed.getInductionVar());

      {
        OpBuilder::InsertionGuard guard(builder);

        builder.setInsertionPointToStart(
            reconstructed.getBody());

        if (failed(
                emitPreservedLoopNest(depth + 1))) {
          llvm::errs()
              << "[AffineMatmulToSystolic] LOWER_FAIL #33\n";
          llvm::errs() << "LOWER_FAIL_33\n";
          return failure();
        }
      }

      preservedIVMapping.erase(
          originalLoop.getInductionVar());

      return success();
    };

    if (failed(emitPreservedLoopNest(0))) {
      llvm::errs()
          << "[AffineMatmulToSystolic] LOWER_FAIL #34\n";
      llvm::errs() << "LOWER_FAIL_34\n";
      return failure();
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
      llvm::errs() << "=== AffineMatmulToSystolic: lowering candidate ===\\n";

      if (failed(lowerOne(matmul))) {
        llvm::errs()
            << "=== AffineMatmulToSystolic: lowerOne FAILED ===\\n";
        continue;
      }

      llvm::errs()
          << "=== AffineMatmulToSystolic: lowerOne SUCCESS ===\\n";
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
