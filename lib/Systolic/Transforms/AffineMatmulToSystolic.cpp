//===- AffineMatmulToSystolic.cpp - Affine GEMM -> systolic.matmul_tile ---===//
//
// --affine-matmul-to-systolic: replace every recognized affine GEMM loop nest
// (AffineMatmulAnalysis.h) whose lowering can be shown correct by
// systolic.matmul_tile ops on the module's systolic.device fleet.
//
//   affine.for (i, j, k in any order) { C[..] = C[..] + A[..] * B[..] }
//     |  recognizeLogicalMatmul         logical I, J, K, access maps, domain
//     |  getDenseLogicalGemm            I, J, K = permutation of the loop
//     |                                 ordinals (offsets, strides, steps)
//     |  planSystolicMatmul             binary-search decomposition over the
//     |                                 fleet, K-folded, scheduled -- the
//     |                                 same plan as the linalg path
//     |  partitionLogicalMatmul         the plan's tiles as exact affine
//     |                                 regions of the iteration domain,
//     |                                 proven to cover it exactly once
//     v  emitSystolicTilePlan           one matmul_tile per region
//   memref.subview + bufferization.to_tensor  (A, B, C as logical matrices)
//   systolic.matmul_tile ... on @acc_* est_cycles start_cycle  (per region)
//   bufferization.materialize_in_destination  (C written back)
//
// The output feeds --lower-systolic-matmul-tile-to-fpga and
// --systolic-runtime-to-llvm unchanged. Without a systolic.device the whole
// domain is one region, lowered to a single unassigned matmul_tile.
//
// A nest is left untouched, with a remark saying why, when the lowering
// cannot be shown correct: a batched or non-permutation mapping, operands
// that may alias C, provably out-of-bounds or non-identity-layout memrefs, or a
// partition that is not exact. Semantics: the lowering keeps every product
// A(i,k) * B(k,j) and accumulates each C(i,j) over K in increasing order of
// the K-folded invocations, but the array's own summation order inside an
// invocation is not the loop's, so floating-point results equal the loop's
// only when the arithmetic is exact (reassociation, as for linalg.matmul).
//
//===----------------------------------------------------------------------===//

#include "Systolic/AffineMatmulAnalysis.h"
#include "Systolic/AffineMatmulPartitioning.h"
#include "Systolic/Passes.h"
#include "Systolic/SystolicDialect.h"
#include "Systolic/SystolicOps.h"
#include "Systolic/SystolicTilePlan.h"

#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Bufferization/IR/Bufferization.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/Tensor/IR/Tensor.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Interfaces/FunctionInterfaces.h"
#include "mlir/Interfaces/ViewLikeInterface.h"
#include "mlir/Pass/Pass.h"

#include <string>

using namespace mlir;
using namespace mlir::systolic;

namespace {

/// A function argument the function declares restrict (C `restrict`,
/// llvm.noalias): nothing else the function sees overlaps the memory it
/// writes through it.
static bool isNoAliasArgument(Value memref) {
  auto arg = dyn_cast<BlockArgument>(memref);
  if (!arg || !arg.getOwner()->isEntryBlock())
    return false;
  auto fn = dyn_cast<FunctionOpInterface>(arg.getOwner()->getParentOp());
  return fn && fn.getArgAttr(arg.getArgNumber(),
                             LLVM::LLVMDialect::getNoAliasAttrName());
}

static bool isFreshAllocation(Value memref) {
  return memref.getDefiningOp<memref::AllocOp>() ||
         memref.getDefiningOp<memref::AllocaOp>();
}

/// The buffer a memref is a view of (through subview, cast, reshapes, ...).
static Value getViewRoot(Value memref) {
  while (auto view = memref.getDefiningOp<ViewLikeOpInterface>())
    memref = view.getViewSource();
  return memref;
}

/// Whether `input` provably does not overlap `output`, the memref the GEMM
/// writes. The loop nest reads A and B while it writes C; the tiles read all
/// of A and B before writing back, so overlapping memory would change the
/// result. Views are traced to the buffer they view: two views of one buffer
/// may overlap; different buffers are disjoint when one of them is a fresh
/// allocation or a restrict argument.
static bool provablyDisjointFromOutput(Value input, Value output,
                                       bool assumeNoAlias) {
  if (input == output)
    return false;
  if (assumeNoAlias)
    return true;
  Value inputRoot = getViewRoot(input);
  Value outputRoot = getViewRoot(output);
  if (inputRoot == outputRoot)
    return false;
  if (isFreshAllocation(inputRoot) || isFreshAllocation(outputRoot))
    return true;
  return isNoAliasArgument(outputRoot) || isNoAliasArgument(inputRoot);
}

/// The logical matrix a memref holds: rows are logical coordinate `rowDim`,
/// columns `colDim`, element (r, c) at memref[offset_r + scale_r * r,
/// offset_c + scale_c * c] -- which is what the recognized access function
/// computes, since its results are the logical coordinates themselves.
struct LogicalOperand {
  Value memref;
  std::array<int64_t, 2> offsets, sizes, strides;

  bool isWholeMemref() const {
    auto ty = cast<MemRefType>(memref.getType());
    return ty.hasStaticShape() && offsets[0] == 0 && offsets[1] == 0 &&
           strides[0] == 1 &&
           strides[1] == 1 && sizes[0] == ty.getDimSize(0) &&
           sizes[1] == ty.getDimSize(1);
  }
  bool sameView(const LogicalOperand &other) const {
    return memref == other.memref && offsets == other.offsets &&
           sizes == other.sizes && strides == other.strides;
  }
};

static FailureOr<LogicalOperand>
getLogicalOperand(Value memref, const DenseLogicalGemm &dense, unsigned rowDim,
                  unsigned colDim, StringRef name, std::string &why) {
  auto ty = dyn_cast<MemRefType>(memref.getType());
  if (!ty || ty.getRank() != 2 || !ty.getLayout().isIdentity()) {
    why = (name + " is not a rank-2 memref with the identity layout").str();
    return failure();
  }

  LogicalOperand operand;
  operand.memref = memref;
  for (auto [r, d] : llvm::enumerate(std::array<unsigned, 2>{rowDim, colDim})) {
    operand.offsets[r] = dense.offset[d];
    operand.sizes[r] = dense.extent[d];
    operand.strides[r] = dense.scale[d];
    // The view holds exactly the elements the loop nest itself accesses
    // (every row and column of the logical matrix is touched by some
    // iteration), so it is in bounds whenever the nest was defined. A
    // dynamic extent -- the leading dimension of a C array parameter,
    // float A[][K] -- is therefore taken as the nest's own guarantee; a
    // static one that the accesses provably exceed means the nest was
    // undefined to begin with, and there is nothing correct to lower it to.
    const int64_t last =
        dense.offset[d] + dense.scale[d] * (dense.extent[d] - 1);
    if (dense.offset[d] < 0 ||
        (!ty.isDynamicDim(r) && last >= ty.getDimSize(r))) {
      why = (name + " is accessed out of bounds along dimension " +
             llvm::Twine(r))
                .str();
      return failure();
    }
  }
  return operand;
}

struct AffineMatmulToSystolicPass
    : public PassWrapper<AffineMatmulToSystolicPass,
                         OperationPass<ModuleOp>> {
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(
      AffineMatmulToSystolicPass)

  AffineMatmulToSystolicPass() = default;
  AffineMatmulToSystolicPass(const AffineMatmulToSystolicPass &other)
      : PassWrapper(other) {}

  StringRef getArgument() const final {
    return "affine-matmul-to-systolic";
  }

  StringRef getDescription() const final {
    return "Partition recognized affine GEMM loop nests over the "
           "systolic.device fleet and lower each region to a scheduled "
           "systolic.matmul_tile";
  }

  Option<bool> assumeNoAlias{
      *this, "assume-no-alias",
      llvm::cl::desc("Assume that A and B never overlap C. Without it, the "
                     "operands must be provably disjoint: distinct "
                     "allocations, or llvm.noalias (C restrict) arguments"),
      llvm::cl::init(false)};

  Option<double> sharedReadBytesPerCycle{
      *this, "shared-read-bytes-per-cycle",
      llvm::cl::desc("Operand-read bandwidth all accelerators share, bytes "
                     "per cycle, as --systolic-binary-search-tile-matmul "
                     "takes it (0: every accelerator fills independently)"),
      llvm::cl::init(0.0)};

  Option<bool> report{
      *this, "report",
      llvm::cl::desc("Emit a remark per lowered GEMM with its partition and "
                     "predicted makespan"),
      llvm::cl::init(false)};

  void getDependentDialects(DialectRegistry &registry) const override {
    registry.insert<bufferization::BufferizationDialect, tensor::TensorDialect,
                    memref::MemRefDialect, SystolicDialect>();
  }

  /// Lowers one GEMM, or leaves it untouched and returns why not.
  LogicalResult lowerOne(const LogicalMatmul &matmul,
                         const SystolicFleet &fleet, std::string &why,
                         bool &hardError) {
    Operation *anchor = matmul.anchor;

    FailureOr<DenseLogicalGemm> dense = getDenseLogicalGemm(matmul, &why);
    if (failed(dense))
      return failure();

    const int64_t M = dense->extent[kLogicalI];
    const int64_t N = dense->extent[kLogicalJ];
    const int64_t K = dense->extent[kLogicalK];

    auto lhs = getLogicalOperand(matmul.lhs, *dense, kLogicalI, kLogicalK,
                                 "A", why);
    if (failed(lhs))
      return failure();
    auto rhs = getLogicalOperand(matmul.rhs, *dense, kLogicalK, kLogicalJ,
                                 "B", why);
    if (failed(rhs))
      return failure();
    auto out = getLogicalOperand(matmul.output, *dense, kLogicalI, kLogicalJ,
                                 "C", why);
    if (failed(out))
      return failure();

    Type elementType =
        cast<MemRefType>(matmul.output.getType()).getElementType();
    if (cast<MemRefType>(matmul.lhs.getType()).getElementType() !=
            elementType ||
        cast<MemRefType>(matmul.rhs.getType()).getElementType() !=
            elementType) {
      why = "A, B and C have different element types";
      return failure();
    }

    for (auto [operand, name] :
         {std::pair{matmul.lhs, "A"}, std::pair{matmul.rhs, "B"}}) {
      if (!provablyDisjointFromOutput(operand, matmul.output, assumeNoAlias)) {
        why = (llvm::Twine(name) +
               " may overlap C; declare the arguments llvm.noalias (C "
               "restrict) or pass assume-no-alias")
                  .str();
        return failure();
      }
    }

    // A x A: one read-only view serves both operands. Two different views
    // of one memref would be two restrict tensors of the same buffer.
    const bool sharedInput = lhs->sameView(*rhs);
    if (!sharedInput && matmul.lhs == matmul.rhs) {
      why = "A and B are different views of the same memref";
      return failure();
    }

    // ------------------------------------------------------------------
    // Partition: the hardware plan's tiles (or the whole domain without a
    // fleet) as regions of the iteration domain, proven exact.
    // ------------------------------------------------------------------
    std::optional<SystolicTilePlan> plan;
    SmallVector<LogicalBox> boxes;
    if (!fleet.resources.empty()) {
      SystolicOptimizerOptions options;
      options.sharedReadBytesPerCycle = sharedReadBytesPerCycle;
      FailureOr<SystolicTilePlan> planned =
          planSystolicMatmul(anchor, M, N, K, fleet, options);
      if (failed(planned)) {
        hardError = true;
        return failure();
      }
      plan = std::move(*planned);
      for (const SystolicTileBlock &block : plan->blocks)
        for (const SystolicTileInvocation &inv : block.invocations) {
          LogicalBox box;
          box.lower = {block.row, block.column, inv.k0};
          box.extent = {block.m, block.n, inv.k};
          boxes.push_back(box);
        }
    } else {
      LogicalBox whole;
      whole.extent = dense->extent;
      boxes.push_back(whole);
    }

    auto partitions = partitionLogicalMatmul(matmul, boxes, &why);
    if (failed(partitions)) {
      why = "the partition is not exact: " + why;
      return failure();
    }

    // ------------------------------------------------------------------
    // Materialize. Every check is done; from here on the nest is replaced.
    // ------------------------------------------------------------------
    OpBuilder builder(anchor);
    Location loc = anchor->getLoc();

    auto view = [&](const LogicalOperand &operand) -> Value {
      if (operand.isWholeMemref())
        return operand.memref;
      return memref::SubViewOp::create(builder, loc, operand.memref,
                                       operand.offsets, operand.sizes,
                                       operand.strides);
    };
    auto asTensor = [&](Value memref, const LogicalOperand &operand,
                        bool writable) -> Value {
      auto ty = RankedTensorType::get(operand.sizes, elementType);
      return bufferization::ToTensorOp::create(builder, loc, ty, memref,
                                               /*restrict=*/true, writable);
    };

    Value aTensor = asTensor(view(*lhs), *lhs, /*writable=*/false);
    Value bTensor =
        sharedInput ? aTensor : asTensor(view(*rhs), *rhs, /*writable=*/false);
    Value cView = view(*out);
    Value cTensor = asTensor(cView, *out, /*writable=*/true);

    Value result;
    if (plan) {
      result = emitSystolicTilePlan(builder, loc, aTensor, bTensor, cTensor,
                                    *plan, fleet);
    } else {
      result = MatmulTileOp::create(
          builder, loc, cTensor.getType(), aTensor, bTensor, cTensor,
          builder.getI64IntegerAttr(M), builder.getI64IntegerAttr(N),
          builder.getI64IntegerAttr(K), /*device=*/FlatSymbolRefAttr(),
          /*est_cycles=*/IntegerAttr(), /*start_cycle=*/IntegerAttr());
    }

    auto writeBack = bufferization::MaterializeInDestinationOp::create(
        builder, loc, result, cView);
    writeBack.setRestrict(true);
    writeBack.setWritable(true);

    if (report) {
      auto remark = anchor->emitRemark()
                    << "affine GEMM " << M << "x" << N << "x" << K
                    << " lowered as " << partitions->size()
                    << " exact region(s)";
      if (plan)
        remark << " on " << fleet.resources.size()
               << " accelerator(s): makespan = "
               << plan->optimization.makespan
               << " cycles (read + compute + write), "
               << plan->blocks.size() << " output block(s)";
    }

    anchor->erase();
    return success();
  }

  void runOnOperation() override {
    ModuleOp module = getOperation();

    FailureOr<SystolicFleet> fleet = collectSystolicFleet(module);
    if (failed(fleet)) {
      signalPassFailure();
      return;
    }

    SmallVector<AffineMatmulCandidate> candidates;
    collectAffineMatmulCandidates(module.getOperation(), candidates);

    SmallVector<LogicalMatmul, 1> matmuls;
    for (AffineMatmulCandidate candidate : candidates) {
      auto result = recognizeLogicalMatmul(candidate);
      if (failed(result))
        continue;

      bool duplicate = llvm::any_of(matmuls, [&](const LogicalMatmul &m) {
        return m.outputStore == result->outputStore;
      });
      if (!duplicate)
        matmuls.push_back(*result);
    }

    for (const LogicalMatmul &matmul : matmuls) {
      std::string why;
      bool hardError = false;
      if (succeeded(lowerOne(matmul, *fleet, why, hardError)))
        continue;
      if (hardError) {
        signalPassFailure();
        return;
      }
      matmul.anchor->emitRemark()
          << "affine GEMM not lowered to systolic.matmul_tile: " << why;
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
