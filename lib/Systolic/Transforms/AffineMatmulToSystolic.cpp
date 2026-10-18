//===- AffineMatmulToSystolic.cpp - Affine GEMM -> systolic.matmul_tile ---===//
//
// --affine-matmul-to-systolic: replace every recognized affine GEMM loop nest
// (AffineMatmulAnalysis.h) whose lowering can be shown correct by
// systolic.matmul_tile ops on the module's systolic.device fleet.
//
//   affine.for (any depth, i, j, k in any order) { C[..] += A[..] * B[..] }
//     |  hoistRepetitionLoops           loops no access uses repeat the GEMM:
//     |                                 moved outward when the dependences
//     |                                 allow (affine::permuteLoops)
//     |  recognizeLogicalMatmul         logical I, J, K, access maps, domain
//     |  getDenseLogicalGemm            I, J, K each over one or more loops
//     |                                 (offsets, strides, steps, tiled nests)
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
// Each operand's view (docs/AFFINE_GEMM_MAPPING.md section 5) is a
// rank-reducing memref.subview with signed strides -- a dimension the
// operand reads backwards has a negative stride -- then memref.transpose
// when the operand stores its roles in the other order, then
// memref.expand_shape for a role of extent one it has no dimension for.
//
// A nest is left untouched, with a remark saying why, when the lowering
// cannot be shown correct: a batched or non-permutation mapping, operands
// that may alias C, provably out-of-bounds or non-identity-layout memrefs, an
// address that does not fit in 64 bits, or a partition that is not exact.
// Semantics: the lowering keeps every product A(i,k) * B(k,j) and accumulates
// each C(i,j) over K in increasing order of the K-folded invocations, but
// each invocation sums its products from zero on the device and the host adds
// the sum to C -- a different sequence of IEEE additions than the loop's (even
// the sign of a zero may differ). So a nest is lowered only when the program
// permits reassociating its floating-point sum: the `reassoc` flag on the
// accumulation, or allow-fp-reassociation (section 6). Results then equal the
// loop's exactly when the arithmetic is exact.
//
//===----------------------------------------------------------------------===//

#include "Systolic/AffineMatmulAnalysis.h"
#include "Systolic/AffineMatmulPartitioning.h"
#include "Systolic/Passes.h"
#include "Systolic/SystolicDialect.h"
#include "Systolic/SystolicOps.h"
#include "Systolic/SystolicTilePlan.h"

#include "mlir/Dialect/Affine/Analysis/AffineAnalysis.h"
#include "mlir/Dialect/Affine/Analysis/AffineStructures.h"
#include "mlir/Dialect/Affine/Analysis/LoopAnalysis.h"
#include "mlir/Dialect/Affine/Analysis/Utils.h"
#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Affine/LoopUtils.h"
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

#include "llvm/ADT/DenseMap.h"
#include "llvm/Support/CheckedArithmetic.h"

#include <optional>
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

/// The logical matrix an operand holds (docs/AFFINE_GEMM_MAPPING.md §5, §19):
/// its two GEMM roles (rows, columns) in GEMM order. Two forms:
///
///  * A role-pure layout (§5): each role read in one memref dimension at
///    index alpha + rho * ordinal, rho of either sign (a negative rho is a
///    view that runs backwards: its origin is the largest index of that
///    dimension); every other dimension at a fixed index. Built as a
///    rank-reducing memref.subview of the memref (signed strides), then
///    memref.transpose when the operand stores the roles in the other order,
///    then memref.expand_shape for a role of extent one the operand has no
///    dimension for.
///  * An explicit view (§19.11): the element address of the view's element
///    (row r, column c) is elementOffset + elementStrides[0] r +
///    elementStrides[1] c (+ the batch terms per batch iteration), built as
///    one memref.reinterpret_cast of the memref to rows x columns. Used for
///    the general layouts of §19 (a dimension combining both of an operand's
///    roles -- A[i][i + k] --, a role across several dimensions, a flat
///    C[4i + j], a Hankel A[i + k]), whose address IS affine in (row, column)
///    although no dimension of the memref holds one role alone, and for a
///    role-pure view whose rank-reduced subview type would be ambiguous
///    (a kept dimension of size one next to a dropped one: the verifier
///    cannot tell which was dropped).
///
/// Every element of the view is an element the loop nest itself accesses;
/// for C the view is injective (G2), for A and B it may repeat elements.
struct LogicalOperand {
  Value memref;
  // The subview, over every dimension of the memref.
  SmallVector<int64_t> offsets, sizes, strides;
  // The memref dimension of each role, -1 for none (a unit dimension).
  std::array<int, 2> roleDim{-1, -1};
  // The view's shape: the roles' extents.
  std::array<int64_t, 2> shape{1, 1};
  // Per memref dimension, the batch loops' coefficients (the layout's
  // batchCoefficients): the dimension's offset is offsets[d] plus these
  // terms of the enclosing batch loops' counters -- a dynamic offset,
  // computed per batch iteration (ISSUE-006, ISSUE-007).
  SmallVector<SmallVector<int64_t>> batchCoefficients;
  // The explicit view: element offset, element strides of (rows, columns),
  // and per source loop the batch loops' coefficients on the element offset
  // (the dimensions' batch terms times the memref's row-major strides).
  bool explicitView = false;
  int64_t elementOffset = 0;
  std::array<int64_t, 2> elementStrides{1, 1};
  SmallVector<int64_t> elementBatchCoefficients;

  bool hasBatchTerms() const {
    if (explicitView)
      return llvm::any_of(elementBatchCoefficients,
                          [](int64_t v) { return v != 0; });
    return llvm::any_of(batchCoefficients, [](const SmallVector<int64_t> &c) {
      return llvm::any_of(c, [](int64_t v) { return v != 0; });
    });
  }
  bool isWholeMemref() const {
    if (explicitView)
      return false;
    auto ty = cast<MemRefType>(memref.getType());
    return ty.getRank() == 2 && ty.hasStaticShape() && roleDim[0] == 0 &&
           roleDim[1] == 1 && offsets[0] == 0 && offsets[1] == 0 &&
           strides[0] == 1 && strides[1] == 1 &&
           sizes[0] == ty.getDimSize(0) && sizes[1] == ty.getDimSize(1) &&
           !hasBatchTerms();
  }
  bool sameView(const LogicalOperand &other) const {
    if (memref != other.memref || explicitView != other.explicitView ||
        shape != other.shape)
      return false;
    if (explicitView)
      return elementOffset == other.elementOffset &&
             elementStrides == other.elementStrides &&
             elementBatchCoefficients == other.elementBatchCoefficients;
    return offsets == other.offsets && sizes == other.sizes &&
           strides == other.strides && roleDim == other.roleDim &&
           batchCoefficients == other.batchCoefficients;
  }
  bool isTransposed() const {
    return !explicitView && roleDim[0] >= 0 && roleDim[1] >= 0 &&
           roleDim[0] > roleDim[1];
  }
};

static FailureOr<LogicalOperand>
getLogicalOperand(Value memref, const LogicalMatmul &matmul,
                  const DenseLogicalGemm &dense, unsigned rowDim,
                  unsigned colDim,
                  const std::array<OperandCoordinateView, 2> &views,
                  const OperandLayout &layout, StringRef name,
                  std::string &why, GemmMappingClass &mappingClass) {
  auto ty = dyn_cast<MemRefType>(memref.getType());
  // The subview needs the memref's strides: an identity layout (its leading
  // dimension may be dynamic -- C's float A[][K]).
  if (!ty || ty.getRank() < 1 || !ty.getLayout().isIdentity() ||
      layout.constantIndex.size() != size_t(ty.getRank())) {
    why = (name + " is not a memref with the identity layout").str();
    mappingClass = GemmMappingClass::ValidButLoweringUnsupported;
    return failure();
  }
  for (int64_t d = 1; d < ty.getRank(); ++d)
    if (ty.isDynamicDim(d)) {
      why = (name + " has a dynamic dimension other than the leading one").str();
      mappingClass = GemmMappingClass::ValidButLoweringUnsupported;
      return failure();
    }

  static constexpr const char *kName[] = {"I", "J", "K"};
  const unsigned rank = ty.getRank();
  LogicalOperand operand;
  operand.memref = memref;
  operand.roleDim = layout.roleDim;
  operand.offsets.assign(rank, 0);
  operand.sizes.assign(rank, 1);
  operand.strides.assign(rank, 1);

  // The memref's row-major element strides (its layout is the identity; a
  // dynamic leading dimension has a static stride). They turn an index
  // tuple into an element address, for the explicit views and for the
  // address range check at the end.
  SmallVector<int64_t> rowMajor(rank, 1);
  for (int64_t d = int64_t(rank) - 2; d >= 0; --d) {
    std::optional<int64_t> stride =
        llvm::checkedMul(rowMajor[d + 1], ty.getDimSize(d + 1));
    if (!stride) {
      why = (name + "'s row-major strides overflow 64-bit arithmetic").str();
      mappingClass = GemmMappingClass::Unknown;
      return failure();
    }
    rowMajor[d] = *stride;
  }
  auto overflow = [&](const llvm::Twine &what) {
    why = (name + "'s " + what + " overflow 64-bit arithmetic").str();
    mappingClass = GemmMappingClass::Unknown;
    return failure();
  };

  // A dimension's accessed indices [lo, hi] must lie inside it: lo >= 0
  // always, hi < its size when that is static (a dynamic size is the
  // program's own guarantee). Outside, the nest itself accesses out of
  // bounds -- undefined, nothing correct to lower it to.
  auto inBounds = [&](unsigned dim, int64_t lo, int64_t hi) {
    if (lo >= 0 && (ty.isDynamicDim(dim) || hi < ty.getDimSize(dim)))
      return true;
    why = (name + " is accessed out of bounds along dimension " +
           llvm::Twine(dim))
              .str();
    mappingClass = GemmMappingClass::ProvenInvalid;
    return false;
  };

  // What each dimension's batch terms add per batch iteration, as a range on
  // the domain (zero for a dimension without batch terms). A batch
  // dimension's index is its fixed index plus these terms; a factor's role
  // dimension may carry them too (A[4i - j][5k], ISSUE-007). Each range
  // below is the static one widened by them, in checked arithmetic.
  operand.batchCoefficients = layout.batchCoefficients;
  SmallVector<std::pair<int64_t, int64_t>> batchRange(rank, {0, 0});
  for (unsigned d = 0; d < rank; ++d) {
    if (!layout.hasBatchTerms(d))
      continue;
    // Every batch term must be on a loop the lowering re-emits around the
    // tiles; a term on any other loop would be dropped from the address.
    for (auto [s, coefficient] :
         llvm::enumerate(layout.batchCoefficients[d]))
      if (coefficient != 0 && !llvm::is_contained(dense.batchLoops, s)) {
        why = (name + "'s dimension " + llvm::Twine(d) +
               " has a batch term on loop " + llvm::Twine(s) +
               ", which is not a batch loop")
                  .str();
        mappingClass = GemmMappingClass::Unknown;
        return failure();
      }
    auto range = rangeOnDomain(matmul, layout.batchCoefficients[d]);
    if (failed(range)) {
      why = (name + "'s batch offsets along dimension " + llvm::Twine(d) +
             " overflow 64-bit arithmetic")
                .str();
      mappingClass = GemmMappingClass::Unknown;
      return failure();
    }
    batchRange[d] = *range;
  }
  auto widened = [&](unsigned d, int64_t lo, int64_t hi)
      -> std::optional<std::pair<int64_t, int64_t>> {
    std::optional<int64_t> first = llvm::checkedAdd(lo, batchRange[d].first);
    std::optional<int64_t> last = llvm::checkedAdd(hi, batchRange[d].second);
    if (!first || !last) {
      why = (name + "'s indices along dimension " + llvm::Twine(d) +
             " overflow 64-bit arithmetic")
                .str();
      mappingClass = GemmMappingClass::Unknown;
      return std::nullopt;
    }
    return std::pair<int64_t, int64_t>{*first, *last};
  };

  // The index range of every dimension, for the address check below.
  SmallVector<std::pair<int64_t, int64_t>> indexRange(rank, {0, 0});

  // ------------------------------------------------------------------
  // A general layout (docs/AFFINE_GEMM_MAPPING.md section 19.11): every
  // dimension d reads dimConstants[d] + sum over the operand's two roles of
  // roleCoefficients[d][slot] * y_slot (+ its batch terms), y_slot =
  // dense.offset + dense.scale * ordinal. The view's element (r, c) is then
  // at the fixed address sum_d rowMajor_d * (c_d + lambda_{d,0} s_0 r +
  // lambda_{d,1} s_1 c): an explicit view with
  //   elementOffset      = sum_d rowMajor_d * (dimConstants[d] + sum_slot
  //                        lambda_{d,slot} * dense.offset[slot]),
  //   elementStrides[s]  = sum_d rowMajor_d * lambda_{d,s} * dense.scale[s],
  // the batch terms likewise summed with the row-major strides. G3 (bounds):
  // every dimension's index range -- the constant, each role's extreme
  // contribution, the batch range -- inside the dimension; distinct in-bounds
  // index tuples have distinct addresses, so with G2 (checked by the
  // recognizer) the view of C is injective. Checked arithmetic throughout.
  // ------------------------------------------------------------------
  if (layout.general) {
    if (layout.roleCoefficients.size() != rank ||
        layout.dimConstants.size() != rank) {
      why = (name + "'s general layout does not match its memref").str();
      mappingClass = GemmMappingClass::Unknown;
      return failure();
    }
    const std::array<unsigned, 2> logicalOf{rowDim, colDim};
    std::optional<int64_t> elementOffset = int64_t(0);
    std::array<std::optional<int64_t>, 2> elementStrides{int64_t(0),
                                                         int64_t(0)};
    for (unsigned d = 0; d < rank; ++d) {
      // The static index of the dimension at the roles' first values, and
      // the dimension's range over the roles' ordinals.
      std::optional<int64_t> index = layout.dimConstants[d];
      std::optional<int64_t> lo = index, hi = index;
      for (unsigned slot = 0; slot < 2 && index; ++slot) {
        const int64_t lambda = layout.roleCoefficients[d][slot];
        if (lambda == 0)
          continue;
        const unsigned logical = logicalOf[slot];
        std::optional<int64_t> base =
            llvm::checkedMul(lambda, dense.offset[logical]);
        std::optional<int64_t> rho = llvm::checkedMul(lambda, dense.scale[logical]);
        std::optional<int64_t> span =
            rho ? llvm::checkedMul(*rho, dense.extent[logical] - 1)
                : std::nullopt;
        if (!base || !rho || !span) {
          index = std::nullopt;
          break;
        }
        index = llvm::checkedAdd(*index, *base);
        // The role's extreme contributions, checked too (a wrapped sum here
        // once let an out-of-bounds nest pass the bounds test: review of
        // 2026-10-18).
        std::optional<int64_t> lowTerm =
            llvm::checkedAdd(*base, std::min<int64_t>(*span, 0));
        std::optional<int64_t> highTerm =
            llvm::checkedAdd(*base, std::max<int64_t>(*span, 0));
        lo = lo && index && lowTerm ? llvm::checkedAdd(*lo, *lowTerm)
                                    : std::nullopt;
        hi = hi && index && highTerm ? llvm::checkedAdd(*hi, *highTerm)
                                     : std::nullopt;
        std::optional<int64_t> term = llvm::checkedMul(rowMajor[d], *rho);
        elementStrides[slot] = elementStrides[slot] && term
                                   ? llvm::checkedAdd(*elementStrides[slot], *term)
                                   : std::nullopt;
      }
      if (!index || !lo || !hi)
        return overflow("subscripts along dimension " + llvm::Twine(d));
      std::optional<int64_t> term = llvm::checkedMul(rowMajor[d], *index);
      elementOffset = elementOffset && term
                          ? llvm::checkedAdd(*elementOffset, *term)
                          : std::nullopt;
      auto range = widened(d, *lo, *hi);
      if (!range)
        return failure();
      indexRange[d] = *range;
      if (!inBounds(d, range->first, range->second))
        return failure();
    }
    if (!elementOffset || !elementStrides[0] || !elementStrides[1])
      return overflow("element addresses");
    // (The element offset needs no check of its own: without batch terms it
    // is the address of an in-bounds index tuple, in [0, size); with batch
    // terms it is the static part of the affine.apply below and may be
    // negative -- a batch dimension's constant with the batch loop's first
    // value compensating. Every sum above is checked, so it cannot have
    // wrapped to the dynamic marker.)
    operand.explicitView = true;
    operand.roleDim = {-1, -1};
    operand.elementOffset = *elementOffset;
    for (unsigned slot = 0; slot < 2; ++slot) {
      operand.shape[slot] = dense.extent[logicalOf[slot]];
      if (operand.shape[slot] == 1) {
        operand.elementStrides[slot] = 1; // a unit dimension: no stride
        continue;
      }
      if (*elementStrides[slot] == 0) {
        // The roles' terms cancel in the address: the operand would be
        // constant along a coordinate of two or more values -- a stride-0
        // view (not lowered, section 10); unreachable with in-bounds
        // subscripts and distinct dimensions, refused defensively.
        why = (name + "'s view has stride 0 along logical coordinate " +
               kName[logicalOf[slot]] + " (a broadcast, not lowered)")
                  .str();
        mappingClass = GemmMappingClass::ValidButLoweringUnsupported;
        return failure();
      }
      operand.elementStrides[slot] = *elementStrides[slot];
    }
    // The batch terms on the element offset: per source loop, the sum over
    // the dimensions of the dimension's coefficient times its stride.
    const unsigned numLoops =
        layout.batchCoefficients.empty() ? 0 : layout.batchCoefficients[0].size();
    operand.elementBatchCoefficients.assign(numLoops, 0);
    for (unsigned d = 0; d < rank; ++d)
      for (unsigned s = 0; s < numLoops; ++s) {
        const int64_t coefficient = layout.batchCoefficients[d][s];
        if (coefficient == 0)
          continue;
        std::optional<int64_t> term = llvm::checkedMul(rowMajor[d], coefficient);
        std::optional<int64_t> sum =
            term ? llvm::checkedAdd(operand.elementBatchCoefficients[s], *term)
                 : std::nullopt;
        if (!sum)
          return overflow("batch offsets");
        operand.elementBatchCoefficients[s] = *sum;
      }
  } else {
  for (unsigned d = 0; d < rank; ++d)
    if (layout.constantIndex[d]) {
      // A fixed index (an extent-one dimension), or a batch dimension's
      // static index with its batch terms added per batch iteration.
      operand.offsets[d] = *layout.constantIndex[d];
      auto range =
          widened(d, *layout.constantIndex[d], *layout.constantIndex[d]);
      if (!range)
        return failure();
      indexRange[d] = *range;
      if (!inBounds(d, range->first, range->second))
        return failure();
    }

  for (auto [r, logical] :
       llvm::enumerate(std::array<unsigned, 2>{rowDim, colDim})) {
    operand.shape[r] = dense.extent[logical];
    const int dim = layout.roleDim[r];
    if (dim < 0) {
      // No dimension for this role: it has extent 1 (a unit dimension).
      if (dense.extent[logical] != 1) {
        why = (name + " has no dimension for logical coordinate " +
               kName[logical])
                  .str();
        mappingClass = GemmMappingClass::ProvenInvalid;
        return failure();
      }
      continue;
    }
    // Along this dimension the operand reads view.offset + view.scale * y_d
    // with y_d = dense.offset[d] + dense.scale[d] * ordinal: index alpha +
    // rho * ordinal with alpha = view.offset + view.scale * dense.offset and
    // rho = view.scale * dense.scale -- negative when the operand reads the
    // coordinate backwards. All of it in checked arithmetic: a wrapped-around
    // offset or stride could pass the bounds test and address memory the
    // nest never touched.
    const OperandCoordinateView &view = views[r];
    std::optional<int64_t> start =
        llvm::checkedMul(view.scale, dense.offset[logical]);
    std::optional<int64_t> alpha =
        start ? llvm::checkedAdd(*start, view.offset) : std::nullopt;
    std::optional<int64_t> rho =
        llvm::checkedMul(view.scale, dense.scale[logical]);
    std::optional<int64_t> span =
        rho ? llvm::checkedMul(*rho, dense.extent[logical] - 1) : std::nullopt;
    std::optional<int64_t> last =
        (alpha && span) ? llvm::checkedAdd(*alpha, *span) : std::nullopt;
    if (!alpha || !rho || !last || *rho == 0) {
      why = (name + "'s subscripts along dimension " + llvm::Twine(dim) +
             " overflow 64-bit arithmetic")
                .str();
      mappingClass = GemmMappingClass::Unknown;
      return failure();
    }
    operand.offsets[dim] = *alpha;
    operand.sizes[dim] = dense.extent[logical];
    operand.strides[dim] = *rho;
    auto range = widened(dim, std::min(*alpha, *last), std::max(*alpha, *last));
    if (!range)
      return failure();
    indexRange[dim] = *range;
    if (!inBounds(dim, range->first, range->second))
      return failure();
  }

  // A rank-reducing subview whose kept dimensions include one of size one
  // (a role of extent one, or a batched 1 x K view) with the same element
  // stride as a dropped dimension: memref.subview's rank-reduced result type
  // does not say which size-one dimension was dropped, and the verifier's
  // guess (by stride occurrences, then by position) differs from this one --
  // invalid IR for a valid nest (review finding of 2026-10-18; distinct
  // strides are resolved correctly, so those views keep their subview). The
  // same view as an explicit reinterpret_cast is unambiguous: element offset
  // and strides from the subview's own.
  bool ambiguous = false;
  for (unsigned d = 0; d < rank && !ambiguous; ++d) {
    if (int(d) == operand.roleDim[0] || int(d) == operand.roleDim[1])
      continue;
    for (int kept : operand.roleDim) {
      if (kept < 0 || operand.sizes[kept] != 1)
        continue;
      std::optional<int64_t> keptStride =
          llvm::checkedMul(rowMajor[kept], operand.strides[kept]);
      std::optional<int64_t> droppedStride =
          llvm::checkedMul(rowMajor[d], operand.strides[d]);
      if (!keptStride || !droppedStride || *keptStride == *droppedStride)
        ambiguous = true;
    }
  }
  if (ambiguous && !operand.isWholeMemref()) {
    std::optional<int64_t> elementOffset = int64_t(0);
    for (unsigned d = 0; d < rank && elementOffset; ++d) {
      std::optional<int64_t> term =
          llvm::checkedMul(rowMajor[d], operand.offsets[d]);
      elementOffset =
          term ? llvm::checkedAdd(*elementOffset, *term) : std::nullopt;
    }
    if (!elementOffset)
      return overflow("element addresses");
    std::array<int64_t, 2> elementStrides{1, 1};
    for (unsigned r = 0; r < 2; ++r) {
      const int dim = operand.roleDim[r];
      if (dim < 0 || operand.sizes[dim] == 1)
        continue;
      std::optional<int64_t> stride =
          llvm::checkedMul(rowMajor[dim], operand.strides[dim]);
      if (!stride)
        return overflow("element strides");
      elementStrides[r] = *stride;
    }
    const unsigned numLoops =
        operand.batchCoefficients.empty() ? 0 : operand.batchCoefficients[0].size();
    SmallVector<int64_t> elementBatch(numLoops, 0);
    for (unsigned d = 0; d < rank; ++d)
      for (unsigned s = 0; s < numLoops; ++s) {
        const int64_t coefficient = operand.batchCoefficients[d][s];
        if (coefficient == 0)
          continue;
        std::optional<int64_t> term = llvm::checkedMul(rowMajor[d], coefficient);
        std::optional<int64_t> sum =
            term ? llvm::checkedAdd(elementBatch[s], *term) : std::nullopt;
        if (!sum)
          return overflow("batch offsets");
        elementBatch[s] = *sum;
      }
    operand.explicitView = true;
    operand.roleDim = {-1, -1};
    operand.elementOffset = *elementOffset;
    operand.elementStrides = elementStrides;
    operand.elementBatchCoefficients = std::move(elementBatch);
  }
  } // role-pure layout

  // The view's linear element offsets (what the view's strided layout and
  // the memref descriptor hold): sum over the dimensions of index * stride
  // with the row-major strides of the memref. Every index is in range, but
  // with a dynamic leading dimension the products are not bounded by an
  // allocation size: each must fit in 64 bits. The offset is a sum of
  // independent per-dimension terms, so its smallest and largest values are
  // the sums of the per-dimension smallest and largest terms (the roles'
  // extreme indices, a batch dimension's range, a fixed index).
  for (bool largest : {false, true}) {
    std::optional<int64_t> address = int64_t(0);
    for (unsigned d = 0; d < rank && address; ++d) {
      const int64_t index =
          largest ? indexRange[d].second : indexRange[d].first;
      std::optional<int64_t> term = llvm::checkedMul(index, rowMajor[d]);
      address = term ? llvm::checkedAdd(*address, *term) : std::nullopt;
    }
    if (!address) {
      why = (name + "'s element addresses overflow 64-bit arithmetic").str();
      mappingClass = GemmMappingClass::Unknown;
      return failure();
    }
  }
  return operand;
}

/// The operand's rank-2 view (rows x columns of the GEMM), on `builder`;
/// `batchIVs` are the counters of the enclosing batch loops, which re-emit
/// the nest's loops `batchLoops` (same order). A dimension with batch terms
/// is read at its static offset plus those terms of the counters -- one
/// affine.apply per such dimension, a dynamic offset of the subview. An
/// explicit view is one memref.reinterpret_cast: element offset (static, or
/// one affine.apply of the batch counters), sizes, element strides.
static Value materializeOperandView(OpBuilder &builder, Location loc,
                                    const LogicalOperand &operand,
                                    ArrayRef<Value> batchIVs,
                                    ArrayRef<unsigned> batchLoops,
                                    llvm::DenseMap<AffineMap, Value> &applies) {
  if (operand.isWholeMemref())
    return operand.memref;
  auto sourceTy = cast<MemRefType>(operand.memref.getType());
  if (operand.explicitView) {
    MLIRContext *ctx = builder.getContext();
    OpFoldResult offset = builder.getIndexAttr(operand.elementOffset);
    int64_t layoutOffset = operand.elementOffset;
    AffineExpr expr = getAffineConstantExpr(operand.elementOffset, ctx);
    bool batched = false;
    for (auto [j, s] : llvm::enumerate(batchLoops)) {
      const int64_t coefficient =
          s < operand.elementBatchCoefficients.size()
              ? operand.elementBatchCoefficients[s]
              : 0;
      if (coefficient == 0)
        continue;
      expr = expr + getAffineDimExpr(j, ctx) * coefficient;
      batched = true;
    }
    if (batched) {
      AffineMap map = AffineMap::get(batchIVs.size(), /*symbolCount=*/0, expr);
      Value &index = applies[map];
      if (!index)
        index = affine::AffineApplyOp::create(builder, loc, map, batchIVs)
                    .getResult();
      offset = index;
      layoutOffset = ShapedType::kDynamic;
    }
    SmallVector<int64_t> viewShape(operand.shape.begin(), operand.shape.end());
    SmallVector<int64_t> viewStrides(operand.elementStrides.begin(),
                                     operand.elementStrides.end());
    auto viewTy = MemRefType::get(
        viewShape, sourceTy.getElementType(),
        StridedLayoutAttr::get(ctx, layoutOffset, viewStrides),
        sourceTy.getMemorySpace());
    SmallVector<OpFoldResult> sizes, strides;
    for (unsigned r = 0; r < 2; ++r) {
      sizes.push_back(builder.getIndexAttr(viewShape[r]));
      strides.push_back(builder.getIndexAttr(viewStrides[r]));
    }
    return memref::ReinterpretCastOp::create(builder, loc, viewTy,
                                             operand.memref, offset, sizes,
                                             strides);
  }
  // The subview keeps the role dimensions (in memref order), drops the rest.
  SmallVector<int64_t> keptShape;
  SmallVector<int> kept;
  for (int64_t d = 0; d < sourceTy.getRank(); ++d)
    if (d == operand.roleDim[0] || d == operand.roleDim[1]) {
      kept.push_back(d);
      keptShape.push_back(operand.sizes[d]);
    }
  SmallVector<OpFoldResult> offsets, sizes, strides;
  for (int64_t d = 0; d < sourceTy.getRank(); ++d) {
    offsets.push_back(builder.getIndexAttr(operand.offsets[d]));
    sizes.push_back(builder.getIndexAttr(operand.sizes[d]));
    strides.push_back(builder.getIndexAttr(operand.strides[d]));
  }
  for (int64_t d = 0; d < sourceTy.getRank(); ++d) {
    if (size_t(d) >= operand.batchCoefficients.size())
      continue;
    AffineExpr expr =
        getAffineConstantExpr(operand.offsets[d], builder.getContext());
    bool batched = false;
    for (auto [j, s] : llvm::enumerate(batchLoops)) {
      const int64_t coefficient = operand.batchCoefficients[d][s];
      if (coefficient == 0)
        continue;
      expr = expr + getAffineDimExpr(j, builder.getContext()) * coefficient;
      batched = true;
    }
    if (!batched)
      continue;
    // One dimension per batch loop, whether or not this offset uses it (the
    // map's dimension count must match the operands).
    // Operands that read the batch coordinate through the same expression
    // share one index (the batch loops' counters are the same values).
    AffineMap map = AffineMap::get(batchIVs.size(), /*symbolCount=*/0, expr);
    Value &index = applies[map];
    if (!index)
      index =
          affine::AffineApplyOp::create(builder, loc, map, batchIVs).getResult();
    offsets[d] = index;
  }
  MemRefType subviewTy = memref::SubViewOp::inferRankReducedResultType(
      keptShape, sourceTy, offsets, sizes, strides);
  Value view = memref::SubViewOp::create(builder, loc, subviewTy,
                                         operand.memref, offsets, sizes,
                                         strides);
  // The roles in the other order: swap the two dimensions.
  if (operand.isTransposed()) {
    AffineMap swap = AffineMap::getPermutationMap(ArrayRef<unsigned>{1, 0},
                                                  builder.getContext());
    view = memref::TransposeOp::create(builder, loc, view,
                                       AffineMapAttr::get(swap));
  }
  // A role the operand has no dimension for: a unit dimension in its place
  // (both roles: an operand of batch dimensions only, a rank-0 view, becomes
  // the 1 x 1 matrix).
  if (kept.size() < 2) {
    SmallVector<int64_t> shape(operand.shape.begin(), operand.shape.end());
    SmallVector<ReassociationIndices> reassociation;
    if (kept.size() == 1)
      reassociation.push_back(ReassociationIndices{0, 1});
    view = memref::ExpandShapeOp::create(builder, loc, shape, view,
                                         reassociation);
  }
  return view;
}

/// How many times the loops enclosing `op` (up to its function) run it, or
/// nullopt when one of them has no constant trip count or the product does
/// not fit in 64 bits.
static std::optional<uint64_t> enclosingRepetitions(Operation *op) {
  uint64_t times = 1;
  for (auto loop = op->getParentOfType<affine::AffineForOp>(); loop;
       loop = loop->getParentOfType<affine::AffineForOp>()) {
    std::optional<uint64_t> trips = affine::getConstantTripCount(loop);
    // A product past 64 bits is as unknown as a non-constant trip count.
    std::optional<uint64_t> product =
        trips ? llvm::checkedMulUnsigned(times, *trips) : std::nullopt;
    if (!product)
      return std::nullopt;
    times = *product;
  }
  return times;
}

/// Loops of a perfect nest around a multiply-accumulate whose IV no access
/// uses repeat the accumulation rather than being GEMM dimensions. The
/// recognizer takes a GEMM as a perfect chain of loops that are all its
/// dimensions, with any repeating loops outside it (AffineMatmulAnalysis:
/// recognizeLogicalMatmul); a repeating loop between the dimensions -- at any
/// depth -- is moved outward here, ahead of them, keeping the relative order
/// of both groups, when affine dependence analysis shows the interchange
/// legal (affine::isValidLoopInterchangePermutation). Otherwise the nest is
/// left as it is, with a remark.
///
/// A repeating loop outside the reduction loop moves out without changing
/// the order in which any C element accumulates; one inside it would
/// interleave the repetitions with the reduction, which the dependences on C
/// forbid -- such a nest stays on the CPU.
///
/// The legality test is exact over the integers (since 2026-10-18). Moving the
/// repeating loops outward turns a dependence vector lexicographically
/// negative iff two iterations that access one element of a memref (one of
/// them a store) differ in a dimension loop that encloses a repeating loop
/// of two or more iterations: with the repeating loops first, their
/// components -- free, since no access reads them -- may be made negative
/// while the original order was positive. So, for every such repeating loop
/// and every dimension loop before it, the dependence polyhedron of MLIR's
/// analysis at that loop's depth (equal on the outer loops, strictly ordered
/// at this one) is built and tested for an integer point.
/// affine::isValidLoopInterchangePermutation tests the same polyhedra for
/// rational feasibility only, and refused legal hoists (a loop of step 2
/// against a stride-3 placement: i - i' = 3 (l' - l) has rational but no
/// integer solutions in the box).
static bool hoistReordersDependentIterations(ArrayRef<affine::AffineForOp> band,
                                             ArrayRef<unsigned> repeating,
                                             ArrayRef<unsigned> dimensions) {
  SmallVector<Operation *> memOps;
  affine::AffineForOp innermost = band.back(), outermost = band.front();
  for (Operation &op : *innermost.getBody())
    if (isa<affine::AffineLoadOp, affine::AffineStoreOp>(op))
      memOps.push_back(&op);
  auto trip = [](affine::AffineForOp loop) -> int64_t {
    const int64_t lb = loop.getConstantLowerBound();
    const int64_t ub = loop.getConstantUpperBound();
    const int64_t step = loop.getStepAsInt();
    return ub <= lb ? 0 : (ub - lb + step - 1) / step;
  };
  // MLIR's loop depth counts every affine.for enclosing the accesses, those
  // outside the band included.
  const unsigned outer = affine::getNestingDepth(outermost.getOperation());
  for (unsigned rho : repeating) {
    if (trip(band[rho]) < 2)
      continue;
    for (unsigned s : dimensions) {
      if (s >= rho)
        continue;
      for (Operation *src : memOps)
        for (Operation *dst : memOps) {
          if (!isa<affine::AffineStoreOp>(src) &&
              !isa<affine::AffineStoreOp>(dst))
            continue;
          affine::MemRefAccess srcAccess(src), dstAccess(dst);
          affine::FlatAffineValueConstraints dependence;
          affine::DependenceResult result = affine::checkMemrefAccessDependence(
              srcAccess, dstAccess, /*loopDepth=*/outer + s + 1, &dependence,
              /*dependenceComponents=*/nullptr, /*allowRAR=*/false);
          if (!affine::hasDependence(result))
            continue;
          // Rationally feasible: the exact question is whether an integer
          // iteration pair exists.
          if (dependence.isIntegerEmpty())
            continue;
          return true;
        }
    }
  }
  return false;
}

static void hoistRepetitionLoops(ModuleOp module) {
  SmallVector<affine::AffineForOp> roots;
  module.walk([&](affine::AffineForOp loop) {
    auto parent = dyn_cast<affine::AffineForOp>(loop->getParentOp());
    // The top of a perfect band: not the only operation of a parent loop.
    if (parent && &parent.getBody()->front() == loop.getOperation() &&
        std::next(parent.getBody()->begin(), 2) == parent.getBody()->end())
      return;
    roots.push_back(loop);
  });

  for (affine::AffineForOp root : roots) {
    SmallVector<affine::AffineForOp> band;
    affine::getPerfectlyNestedLoops(band, root);
    // At least one loop to move and one GEMM dimension (a GEMM may have one
    // or two loops since roles of extent one are lowered, SQ-4 / SQ-5; a
    // band of four with three dimensions was required before 2026-10-18).
    if (band.size() < 2)
      continue;
    // A multiply-accumulate body, rectangular constant bounds (no IV in
    // another loop's bounds).
    bool macBody = true;
    SmallVector<Value> used;
    for (Operation &op : *band.back().getBody()) {
      if (auto load = dyn_cast<affine::AffineLoadOp>(op))
        llvm::append_range(used, load.getIndices());
      else if (auto store = dyn_cast<affine::AffineStoreOp>(op))
        llvm::append_range(used, store.getIndices());
      else if (!isa<arith::MulFOp, arith::AddFOp, affine::AffineYieldOp>(op))
        macBody = false;
    }
    if (!macBody || !llvm::all_of(band, [](affine::AffineForOp loop) {
          return loop.hasConstantLowerBound() && loop.hasConstantUpperBound() &&
                 loop.getNumResults() == 0;
        }))
      continue;

    // A loop whose counter an access uses is a GEMM dimension; so is a loop
    // that runs exactly once whatever its uses (a constant on the domain,
    // docs/AFFINE_GEMM_MAPPING.md Lemma 0: the recognizer reads it as an
    // iteration dimension of one value, review of 2026-10-18); every other
    // loop repeats the accumulation.
    SmallVector<unsigned> repeating, dimensions;
    for (auto [index, loop] : llvm::enumerate(band)) {
      std::optional<uint64_t> trip = affine::getConstantTripCount(loop);
      const bool dimension =
          llvm::is_contained(used, loop.getInductionVar()) ||
          (trip && *trip == 1);
      (dimension ? dimensions : repeating).push_back(index);
    }
    if (repeating.empty() || dimensions.empty())
      continue;
    // Already outside every dimension: nothing to move.
    if (repeating.back() + 1 == repeating.size())
      continue;

    SmallVector<unsigned> permMap(band.size());
    unsigned position = 0;
    for (unsigned index : repeating)
      permMap[index] = position++;
    for (unsigned index : dimensions)
      permMap[index] = position++;
    if (hoistReordersDependentIterations(band, repeating, dimensions)) {
      root->emitRemark()
          << "affine GEMM not lowered to systolic.matmul_tile: loop "
          << repeating.back() << " of the nest repeats the accumulation "
          << "inside it, and moving it outward is not a legal loop "
          << "interchange (it would reorder dependent iterations)";
      continue;
    }
    affine::permuteLoops(band, permMap);
  }
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

  Option<bool> allowFpReassociation{
      *this, "allow-fp-reassociation",
      llvm::cl::desc("Permit reassociating the floating-point accumulation: "
                     "the systolic execution sums each tile from zero and adds "
                     "the sum to C, a different order of IEEE additions than "
                     "the loop's, so results may differ in rounding and in the "
                     "sign of a zero (docs/AFFINE_GEMM_MAPPING.md section 6). "
                     "Without it only nests whose accumulation carries the "
                     "`reassoc` fast-math flag are lowered"),
      llvm::cl::init(false)};

  Option<bool> classify{
      *this, "classify",
      llvm::cl::desc("Append each refused nest's classification "
                     "(docs/AFFINE_GEMM_MAPPING.md section 9: PROVEN_VALID, "
                     "VALID_BUT_LOWERING_UNSUPPORTED, PROVEN_INVALID, UNKNOWN) "
                     "to its remark, as [CLASS]"),
      llvm::cl::init(false)};

  Option<bool> report{
      *this, "report",
      llvm::cl::desc("Emit a remark per lowered GEMM with its partition and "
                     "predicted makespan"),
      llvm::cl::init(false)};

  void getDependentDialects(DialectRegistry &registry) const override {
    registry.insert<affine::AffineDialect, bufferization::BufferizationDialect,
                    tensor::TensorDialect, memref::MemRefDialect,
                    SystolicDialect>();
  }

  /// Lowers one GEMM, or leaves it untouched and returns why not and how the
  /// nest is classified (docs/AFFINE_GEMM_MAPPING.md section 9).
  LogicalResult lowerOne(const LogicalMatmul &matmul,
                         const SystolicFleet &fleet, std::string &why,
                         GemmMappingClass &mappingClass, bool &hardError) {
    Operation *anchor = matmul.anchor;
    mappingClass = GemmMappingClass::ProvenValid;

    FailureOr<DenseLogicalGemm> dense =
        getDenseLogicalGemm(matmul, &why, &mappingClass);
    if (failed(dense)) {
      // Which bound is not a constant, and why (a symbol, or a bound on
      // another loop's counter: not a box).
      if (!matmul.normalization.domain && !matmul.normalizationDetail.empty())
        why += " (" + matmul.normalizationDetail + ")";
      return failure();
    }

    const int64_t M = dense->extent[kLogicalI];
    const int64_t N = dense->extent[kLogicalJ];
    const int64_t K = dense->extent[kLogicalK];

    auto lhs = getLogicalOperand(matmul.lhs, matmul, *dense, kLogicalI, kLogicalK,
                                 matmul.viewA, matmul.layoutA, "A", why,
                                 mappingClass);
    if (failed(lhs))
      return failure();
    auto rhs = getLogicalOperand(matmul.rhs, matmul, *dense, kLogicalK, kLogicalJ,
                                 matmul.viewB, matmul.layoutB, "B", why,
                                 mappingClass);
    if (failed(rhs))
      return failure();
    auto out = getLogicalOperand(matmul.output, matmul, *dense, kLogicalI, kLogicalJ,
                                 matmul.viewC, matmul.layoutC, "C", why,
                                 mappingClass);
    if (failed(out))
      return failure();

    Type elementType =
        cast<MemRefType>(matmul.output.getType()).getElementType();
    if (cast<MemRefType>(matmul.lhs.getType()).getElementType() !=
            elementType ||
        cast<MemRefType>(matmul.rhs.getType()).getElementType() !=
            elementType) {
      why = "A, B and C have different element types";
      mappingClass = GemmMappingClass::ValidButLoweringUnsupported;
      return failure();
    }

    for (auto [operand, name] :
         {std::pair{matmul.lhs, "A"}, std::pair{matmul.rhs, "B"}}) {
      if (!provablyDisjointFromOutput(operand, matmul.output, assumeNoAlias)) {
        why = (llvm::Twine(name) +
               " may overlap C; declare the arguments llvm.noalias (C "
               "restrict) or pass assume-no-alias")
                  .str();
        mappingClass = GemmMappingClass::Unknown;
        return failure();
      }
    }

    // A x A: when the two factors are one read-only view -- the same memref,
    // the same element map -- one tensor serves both operands (sharedInput).
    const bool sharedInput = lhs->sameView(*rhs);
    // Two different views of one memref (A . A^T, A[i][i + k] * A[k][j],
    // A[i][2k] * A[2k][j]) are two read-only operand views sharing a base
    // (docs section 7, 19.12, 2026-10-18): each gets its own view and its own
    // to_tensor below. Both tensors carry `restrict` although their memrefs
    // alias -- the invariant that keeps this sound: neither is writable,
    // every use of them is a tensor.extract_slice staged through
    // bufferization.to_buffer read_only (TileMatmulForFpga) or a read-only
    // tile operand, and the output is provably disjoint from the shared base
    // (provablyDisjointFromOutput refuses one root for input and output), so
    // no write can reach either view through any alias. Until 2026-10-18
    // (morning) this was refused as VALID_BUT_LOWERING_UNSUPPORTED; an
    // orientation search that made some pairs one tensor was superseded by
    // the two views the same day.

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
      mappingClass = GemmMappingClass::Unknown;
      return failure();
    }

    // ------------------------------------------------------------------
    // Floating point (docs/AFFINE_GEMM_MAPPING.md section 6). The mapping is
    // valid, but the systolic execution is not the loop's sequence of IEEE
    // operations: each tile's product is summed from zero (+0) on the device
    // and the sum added to C -- C + (a1 b1 + a2 b2 + ...) against the loop's
    // ((C + a1 b1) + a2 b2) + ...; even a one-term tile turns a -0 product
    // into +0. The program must permit reassociation.
    // ------------------------------------------------------------------
    if (!matmul.reductionMayReassociate && !allowFpReassociation) {
      why = "floating-point reassociation is not permitted: the systolic "
            "execution sums each tile from zero and adds the sum to C, a "
            "different order of IEEE additions than the loop's (mark the "
            "accumulation reassoc -- systolic-cc --fp-reassociation -- or "
            "pass allow-fp-reassociation)";
      mappingClass = GemmMappingClass::ProvenValid;
      return failure();
    }

    // The batch loops' iterations, from the analysis: the origin, step and
    // trip count of the normalized domain -- the folded bounds (a bound may
    // be a max or min of constants, P0.6), the ones every check above used;
    // a bound map's first result would run other iterations. An upper bound
    // of one constant is re-emitted as written (the same iterations), any
    // other as the last iteration plus one. Checked before any IR is built.
    struct BatchLoopBounds {
      int64_t lower, upper, step;
    };
    SmallVector<BatchLoopBounds> batchBounds;
    for (unsigned s : dense->batchLoops) {
      affine::AffineForOp loop =
          affine::getForInductionVarOwner(matmul.sourceDomain->getValue(s));
      const AffineLogicalNormalization &norm = matmul.normalization;
      std::optional<int64_t> lastOrdinal =
          norm.domain ? norm.domain->getConstantBound64(
                            presburger::BoundType::UB, s)
                      : std::nullopt;
      const int64_t step = norm.sourceTransformation[s][s];
      const int64_t lower = norm.sourceOffset[s];
      std::optional<int64_t> span =
          lastOrdinal ? llvm::checkedMul(step, *lastOrdinal) : std::nullopt;
      std::optional<int64_t> last =
          span ? llvm::checkedAdd(lower, *span) : std::nullopt;
      std::optional<int64_t> upper =
          last ? llvm::checkedAdd(*last, int64_t(1)) : std::nullopt;
      if (!upper || step <= 0) {
        why = "a batch loop's bounds overflow 64-bit arithmetic";
        mappingClass = GemmMappingClass::Unknown;
        return failure();
      }
      batchBounds.push_back(
          {lower,
           loop.hasConstantUpperBound() ? loop.getConstantUpperBound() : *upper,
           step});
    }

    // ------------------------------------------------------------------
    // Materialize. Every check is done; from here on the nest is replaced.
    // ------------------------------------------------------------------
    OpBuilder builder(anchor);
    Location loc = anchor->getLoc();

    // A batched GEMM: the batch loops -- the nest's loops that drive a batch
    // coordinate, with their own iterations (the bounds above), in their
    // original order -- are re-emitted around the tiles, so that every batch
    // iteration runs the tiles once on its own slice of each operand: exactly
    // the original iteration domain, whatever the batch coordinate's values
    // (ISSUE-006, docs/AFFINE_GEMM_MAPPING.md section 2). Each operand
    // dimension with batch terms is read at its static offset plus those
    // terms of the new loops' counters (materializeOperandView): a batch
    // dimension's index, or a factor's role view moving with the batch
    // (ISSUE-007). The terms of loops that run once are constants on the
    // domain, already in the static offsets (domain forms); such loops are not
    // re-emitted.
    SmallVector<Value> newIVs;
    for (const BatchLoopBounds &bounds : batchBounds) {
      auto batchLoop = affine::AffineForOp::create(
          builder, loc, bounds.lower, bounds.upper, bounds.step);
      newIVs.push_back(batchLoop.getInductionVar());
      builder.setInsertionPointToStart(batchLoop.getBody());
    }

    llvm::DenseMap<AffineMap, Value> applies;
    auto view = [&](const LogicalOperand &operand) -> Value {
      return materializeOperandView(builder, loc, operand, newIVs,
                                    dense->batchLoops, applies);
    };
    auto asTensor = [&](Value memref, const LogicalOperand &operand,
                        bool writable) -> Value {
      auto ty = RankedTensorType::get(operand.shape, elementType);
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
      if (dense->numBatchDims)
        remark << ", batched: " << dense->batchTrips
               << " batch iteration(s) of " << dense->batchLoops.size()
               << " batch loop(s)";
      // Enclosing loops (repeating ones the nest kept or that were moved
      // out of it, and the batch loops) run the tiles more than once per
      // call of the function.
      std::optional<uint64_t> times = enclosingRepetitions(anchor);
      if (times)
        times = llvm::checkedMulUnsigned(*times, uint64_t(dense->batchTrips));
      if (!times)
        remark << "; the enclosing loops run it an unknown number of times";
      else if (*times != 1)
        remark << "; the enclosing loops run it " << *times << " time(s)";
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

    // A constant-extent parallelotope domain (docs/AFFINE_GEMM_MAPPING.md
    // sections 19.7, 19.12) becomes the equivalent box nest first, x = U x';
    // then the repetition loops move outward, then the box pipeline runs.
    SmallVector<DomainNormalization, 1> normalized =
        normalizeParallelotopeDomains(module.getOperation());
    if (report)
      for (const DomainNormalization &dn : normalized) {
        std::string lines;
        llvm::raw_string_ostream os(lines);
        describeDomainNormalization(dn, os, "");
        dn.outermost->emitRemark()
            << "affine multiply-accumulate nest: iteration domain normalized "
               "to a box (x = U x', docs/AFFINE_GEMM_MAPPING.md section "
               "19.12): "
            << llvm::StringRef(lines).rtrim();
      }
    hoistRepetitionLoops(module);

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
      GemmMappingClass mappingClass = GemmMappingClass::ProvenValid;
      bool hardError = false;
      if (succeeded(lowerOne(matmul, *fleet, why, mappingClass, hardError)))
        continue;
      if (hardError) {
        signalPassFailure();
        return;
      }
      auto remark = matmul.anchor->emitRemark()
                    << "affine GEMM not lowered to systolic.matmul_tile: "
                    << why;
      if (classify)
        remark << " [" << stringifyGemmMappingClass(mappingClass) << "]";
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
