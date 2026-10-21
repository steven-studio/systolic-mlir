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
#include "mlir/IR/IRMapping.h"
#include "mlir/Interfaces/FunctionInterfaces.h"
#include "mlir/Interfaces/ViewLikeInterface.h"
#include "mlir/Pass/Pass.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/MapVector.h"
#include "llvm/Support/CheckedArithmetic.h"

#include <functional>
#include <limits>
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
  // Per memref dimension, the run-time offset the access adds through the
  // nest's symbols (LogicalMatmul::symbolicA/B/C: C[r0 + i][j] offsets the
  // rows by the parameter r0), as a map of no dims over `symbols` -- a
  // "dynamic offset" of another kind than the batch terms: the view is
  // taken at run time, offset by the symbols' values. Null when the access
  // has none.
  AffineMap symbolic;
  SmallVector<Value> symbols;

  bool hasSymbolicOffset(unsigned d) const {
    if (!symbolic || d >= symbolic.getNumResults())
      return false;
    AffineExpr e = symbolic.getResult(d);
    return !isa<AffineConstantExpr>(e) ||
           cast<AffineConstantExpr>(e).getValue() != 0;
  }
  bool hasSymbolicOffsets() const {
    for (unsigned d = 0, n = symbolic ? symbolic.getNumResults() : 0; d < n; ++d)
      if (hasSymbolicOffset(d))
        return true;
    return false;
  }
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
           !hasBatchTerms() && !hasSymbolicOffsets();
  }
  bool sameView(const LogicalOperand &other) const {
    if (memref != other.memref || explicitView != other.explicitView ||
        shape != other.shape || symbolic != other.symbolic ||
        symbols != other.symbols)
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

/// `expr` (over `symbols`, no dims) plus `constant`, as index arithmetic at
/// the builder's point: the run-time offset of a view dimension. A bare
/// symbol with no constant is the symbol's value itself.
static Value materializeOffset(OpBuilder &builder, Location loc,
                               AffineExpr expr, ArrayRef<Value> symbols,
                               int64_t constant) {
  std::function<Value(AffineExpr)> build = [&](AffineExpr e) -> Value {
    if (auto c = dyn_cast<AffineConstantExpr>(e))
      return arith::ConstantIndexOp::create(builder, loc, c.getValue());
    if (auto sym = dyn_cast<AffineSymbolExpr>(e))
      return symbols[sym.getPosition()];
    auto bin = cast<AffineBinaryOpExpr>(e);
    Value lhs = build(bin.getLHS()), rhs = build(bin.getRHS());
    switch (e.getKind()) {
    case AffineExprKind::Add:
      return arith::AddIOp::create(builder, loc, lhs, rhs);
    case AffineExprKind::Mul:
      return arith::MulIOp::create(builder, loc, lhs, rhs);
    default:
      // separateSymbols refuses floordiv, ceildiv and mod. Not
      // llvm_unreachable: in a build without assertions that is undefined
      // behaviour, which once lowered s0 floordiv 2 as s0 * 2.
      llvm::report_fatal_error(
          "materializeOffset: a symbolic offset that is not a sum of products");
    }
  };
  Value v = build(expr);
  if (constant != 0)
    v = arith::AddIOp::create(
        builder, loc, v, arith::ConstantIndexOp::create(builder, loc, constant));
  return v;
}

static FailureOr<LogicalOperand>
getLogicalOperand(Value memref, const LogicalMatmul &matmul,
                  const DenseLogicalGemm &dense, unsigned rowDim,
                  unsigned colDim,
                  const std::array<OperandCoordinateView, 2> &views,
                  const OperandLayout &layout, AffineMap symbolic,
                  ArrayRef<Value> symbols, StringRef name,
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
  // The run-time offsets the nest's symbols add, per memref dimension (the
  // access map's results are the memref's dimensions).
  if (symbolic && symbolic.getNumResults() != rank) {
    why = (name + "'s symbolic offsets do not match its memref").str();
    mappingClass = GemmMappingClass::Unknown;
    return failure();
  }
  operand.symbolic = symbolic;
  operand.symbols.assign(symbols.begin(), symbols.end());

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
    // A dimension with a run-time offset is the nest's own guarantee, as a
    // dynamic extent is: where its symbols put the view is not known here,
    // and the tiles touch no element the nest did not.
    if (operand.hasSymbolicOffset(dim))
      return true;
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
    // The symbols' terms on the element offset: per dimension with a
    // run-time offset, its term times the memref's row-major stride.
    if (operand.hasSymbolicOffsets()) {
      SmallVector<int64_t> rowMajor(sourceTy.getRank(), 1);
      for (int64_t d = sourceTy.getRank() - 2; d >= 0; --d)
        rowMajor[d] = rowMajor[d + 1] * sourceTy.getDimSize(d + 1);
      Value sum = batched ? cast<Value>(offset)
                          : arith::ConstantIndexOp::create(
                                builder, loc, operand.elementOffset);
      for (int64_t d = 0; d < sourceTy.getRank(); ++d) {
        if (!operand.hasSymbolicOffset(d))
          continue;
        Value term = materializeOffset(builder, loc,
                                       operand.symbolic.getResult(d),
                                       operand.symbols, /*constant=*/0);
        if (rowMajor[d] != 1)
          term = arith::MulIOp::create(
              builder, loc, term,
              arith::ConstantIndexOp::create(builder, loc, rowMajor[d]));
        sum = arith::AddIOp::create(builder, loc, sum, term);
      }
      offset = sum;
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
  // A dimension the nest's symbols offset: the static offset (or the batch
  // terms' affine.apply, which already holds it) plus the symbols' term,
  // as index arithmetic -- a dynamic offset of the subview (a bare symbol
  // with no static part is the symbol's value itself).
  for (int64_t d = 0; d < sourceTy.getRank(); ++d) {
    if (!operand.hasSymbolicOffset(d))
      continue;
    AffineExpr expr = operand.symbolic.getResult(d);
    if (auto base = dyn_cast<Value>(offsets[d])) {
      Value term = materializeOffset(builder, loc, expr, operand.symbols,
                                     /*constant=*/0);
      offsets[d] = arith::AddIOp::create(builder, loc, base, term).getResult();
    } else {
      offsets[d] = materializeOffset(builder, loc, expr, operand.symbols,
                                     operand.offsets[d]);
    }
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

/// The value of an affine expression at constant dims (no symbols), in
/// checked 64-bit arithmetic; nullopt otherwise.
static std::optional<int64_t> evaluateAffine(AffineExpr expr,
                                             ArrayRef<int64_t> dims) {
  if (auto dim = dyn_cast<AffineDimExpr>(expr))
    return dim.getPosition() < dims.size()
               ? std::optional<int64_t>(dims[dim.getPosition()])
               : std::nullopt;
  if (auto constant = dyn_cast<AffineConstantExpr>(expr))
    return constant.getValue();
  auto binary = dyn_cast<AffineBinaryOpExpr>(expr);
  if (!binary)
    return std::nullopt;
  std::optional<int64_t> lhs = evaluateAffine(binary.getLHS(), dims);
  std::optional<int64_t> rhs = evaluateAffine(binary.getRHS(), dims);
  if (!lhs || !rhs)
    return std::nullopt;
  switch (expr.getKind()) {
  case AffineExprKind::Add:
    return llvm::checkedAdd(*lhs, *rhs);
  case AffineExprKind::Mul:
    return llvm::checkedMul(*lhs, *rhs);
  case AffineExprKind::FloorDiv:
  case AffineExprKind::CeilDiv:
  case AffineExprKind::Mod: {
    if (*rhs <= 0)
      return std::nullopt;
    int64_t q = *lhs / *rhs, r = *lhs % *rhs;
    if (r < 0) {
      q -= 1;
      r += *rhs;
    }
    if (expr.getKind() == AffineExprKind::Mod)
      return r;
    if (expr.getKind() == AffineExprKind::CeilDiv && r != 0)
      return llvm::checkedAdd(q, int64_t(1));
    return q;
  }
  default:
    return std::nullopt;
  }
}

/// An affine expression of a map over `operands` as a linear form over those
/// values: coefficient per value, constant; checked 64-bit arithmetic.
/// nullopt for floordiv / ceildiv / mod or on overflow.
struct ValueForm {
  llvm::SmallMapVector<Value, int64_t, 4> coefficients;
  int64_t constant = 0;
};
static std::optional<ValueForm> valueForm(AffineExpr expr, AffineMap map,
                                          ValueRange operands) {
  ValueForm out;
  if (auto dim = dyn_cast<AffineDimExpr>(expr)) {
    out.coefficients[operands[dim.getPosition()]] = 1;
    return out;
  }
  if (auto symbol = dyn_cast<AffineSymbolExpr>(expr)) {
    out.coefficients[operands[map.getNumDims() + symbol.getPosition()]] = 1;
    return out;
  }
  if (auto constant = dyn_cast<AffineConstantExpr>(expr)) {
    out.constant = constant.getValue();
    return out;
  }
  auto binary = dyn_cast<AffineBinaryOpExpr>(expr);
  if (!binary)
    return std::nullopt;
  std::optional<ValueForm> lhs = valueForm(binary.getLHS(), map, operands);
  if (!lhs)
    return std::nullopt;
  if (expr.getKind() == AffineExprKind::Mul) {
    auto scale = dyn_cast<AffineConstantExpr>(binary.getRHS());
    if (!scale)
      return std::nullopt;
    for (auto &[value, c] : lhs->coefficients) {
      std::optional<int64_t> product = llvm::checkedMul(c, scale.getValue());
      if (!product)
        return std::nullopt;
      c = *product;
    }
    std::optional<int64_t> constant =
        llvm::checkedMul(lhs->constant, scale.getValue());
    if (!constant)
      return std::nullopt;
    lhs->constant = *constant;
    return lhs;
  }
  if (expr.getKind() != AffineExprKind::Add)
    return std::nullopt;
  std::optional<ValueForm> rhs = valueForm(binary.getRHS(), map, operands);
  if (!rhs)
    return std::nullopt;
  for (auto &[value, c] : rhs->coefficients) {
    std::optional<int64_t> sum = llvm::checkedAdd(lhs->coefficients[value], c);
    if (!sum)
      return std::nullopt;
    lhs->coefficients[value] = *sum;
  }
  std::optional<int64_t> constant = llvm::checkedAdd(lhs->constant, rhs->constant);
  if (!constant)
    return std::nullopt;
  lhs->constant = *constant;
  return lhs;
}

/// The trip count of `loop` when its bounds fix it whatever the values they
/// read: one lower result -- or several constants, whose max is the bound as
/// the recognizer folds it (constantAffineForBound, review of 2026-10-19) --
/// and upper results that each differ from it by a constant (the min of
/// those, clamped at 0, over the step; a difference beyond 64 bits is never
/// the min above the lower bound and means no iteration below it, review of
/// 2026-10-19, round 4). nullopt otherwise.
/// affine::getConstantTripCount casts a negative difference to uint64 and
/// takes an unsigned min over the upper results, so a loop that never runs
/// got a count near 2^64 (review of 2026-10-18); this is signed and checked.
static std::optional<uint64_t> fixedTripCount(affine::AffineForOp loop) {
  const int64_t step = loop.getStepAsInt();
  AffineMap lbMap = loop.getLowerBoundMap(), ubMap = loop.getUpperBoundMap();
  if (step < 1 || lbMap.getNumResults() == 0 || ubMap.getNumResults() == 0)
    return std::nullopt;
  std::optional<ValueForm> lb;
  if (lbMap.getNumResults() == 1) {
    lb = valueForm(lbMap.getResult(0), lbMap, loop.getLowerBoundOperands());
  } else if (std::optional<int64_t> folded =
                 constantAffineForBound(loop, /*lower=*/true)) {
    lb = ValueForm();
    lb->constant = *folded;
  }
  if (!lb)
    return std::nullopt;
  std::optional<int64_t> least;
  for (AffineExpr result : ubMap.getResults()) {
    std::optional<ValueForm> ub =
        valueForm(result, ubMap, loop.getUpperBoundOperands());
    if (!ub)
      return std::nullopt;
    for (auto &[value, c] : lb->coefficients)
      if (ub->coefficients.lookup(value) != c)
        return std::nullopt;
    for (auto &[value, c] : ub->coefficients)
      if (lb->coefficients.lookup(value) != c)
        return std::nullopt;
    std::optional<int64_t> span = llvm::checkedSub(ub->constant, lb->constant);
    if (!span) {
      if (ub->constant < lb->constant)
        return 0;
      continue;
    }
    least = least ? std::min(*least, *span) : *span;
  }
  if (!least)
    return std::nullopt;
  if (*least <= 0)
    return 0;
  return uint64_t(*least / step + (*least % step != 0));
}

/// How many times the loops enclosing `op` (up to its function) run it, or
/// nullopt when that is not known exactly: the product of their fixed trip
/// counts (fixedTripCount); otherwise -- loops whose bounds read outer counters, a
/// triangle of repetition loops around the tiles (docs section 19.13) --
/// the number of points of their nest, counted by running it (each bound
/// evaluated at the outer counters' values) up to a limit. nullopt for a
/// bound over anything but the enclosing counters, a count past the limit or
/// past 64 bits.
static std::optional<uint64_t> enclosingRepetitions(Operation *op) {
  SmallVector<affine::AffineForOp> chain; // outermost first
  for (auto loop = op->getParentOfType<affine::AffineForOp>(); loop;
       loop = loop->getParentOfType<affine::AffineForOp>())
    chain.insert(chain.begin(), loop);
  uint64_t times = 1;
  bool constant = true;
  for (affine::AffineForOp loop : chain) {
    std::optional<uint64_t> trips = fixedTripCount(loop);
    // A product past 64 bits is as unknown as a non-constant trip count.
    std::optional<uint64_t> product =
        trips ? llvm::checkedMulUnsigned(times, *trips) : std::nullopt;
    if (!trips) {
      constant = false;
      break;
    }
    if (!product)
      return std::nullopt;
    times = *product;
  }
  if (constant)
    return times;

  constexpr uint64_t kLimit = uint64_t(1) << 24;
  uint64_t visited = 0, count = 0;
  SmallVector<int64_t> values;
  bool known = true;
  std::function<void(unsigned)> run = [&](unsigned depth) {
    if (!known)
      return;
    if (depth == chain.size()) {
      ++count;
      return;
    }
    affine::AffineForOp loop = chain[depth];
    auto bound = [&](AffineMap map, ValueRange operands,
                     bool lower) -> std::optional<int64_t> {
      if (map.getNumSymbols() != 0)
        return std::nullopt;
      SmallVector<int64_t> dims;
      for (Value operand : operands) {
        auto at = llvm::find_if(chain, [&](affine::AffineForOp outer) {
          return outer.getInductionVar() == operand;
        });
        const unsigned position = at - chain.begin();
        if (at == chain.end() || position >= depth)
          return std::nullopt;
        dims.push_back(values[position]);
      }
      std::optional<int64_t> result;
      for (AffineExpr expr : map.getResults()) {
        std::optional<int64_t> value = evaluateAffine(expr, dims);
        if (!value)
          return std::nullopt;
        result = !result ? *value
                         : (lower ? std::max(*result, *value)
                                  : std::min(*result, *value));
      }
      return result;
    };
    std::optional<int64_t> lb = bound(loop.getLowerBoundMap(),
                                      loop.getLowerBoundOperands(), true);
    std::optional<int64_t> ub = bound(loop.getUpperBoundMap(),
                                      loop.getUpperBoundOperands(), false);
    const int64_t step = loop.getStepAsInt();
    if (!lb || !ub || step < 1) {
      known = false;
      return;
    }
    for (int64_t v = *lb; v < *ub && known; ) {
      if (++visited > kLimit) {
        known = false;
        return;
      }
      values.push_back(v);
      run(depth + 1);
      values.pop_back();
      std::optional<int64_t> next = llvm::checkedAdd(v, step);
      if (!next)
        break;
      v = *next;
    }
  };
  run(0);
  if (!known)
    return std::nullopt;
  return count;
}

/// Whether `loop` may run two or more times at some point of the loops around
/// it: not when its bounds fix its trip count at 0 or 1 (fixedTripCount); a
/// loop whose bounds read outer counters with different coefficients (a
/// triangle of repetitions) may.
static bool mayRunTwice(affine::AffineForOp loop) {
  std::optional<uint64_t> trips = fixedTripCount(loop);
  return !trips || *trips >= 2;
}

/// An upper bound on the magnitude of every coefficient and constant that
/// flattening `expr` gives (sums add, a constant factor multiplies, a
/// floordiv / ceildiv / mod adds a local with its divisor); nullopt past 64
/// bits, and for INT64_MIN, whose negation does not fit.
static std::optional<uint64_t> flattenedMagnitude(AffineExpr expr) {
  if (isa<AffineDimExpr, AffineSymbolExpr>(expr))
    return 1;
  if (auto constant = dyn_cast<AffineConstantExpr>(expr)) {
    const int64_t value = constant.getValue();
    if (value == std::numeric_limits<int64_t>::min())
      return std::nullopt;
    return uint64_t(value < 0 ? -value : value);
  }
  auto binary = cast<AffineBinaryOpExpr>(expr);
  std::optional<uint64_t> lhs = flattenedMagnitude(binary.getLHS());
  std::optional<uint64_t> rhs = flattenedMagnitude(binary.getRHS());
  if (!lhs || !rhs)
    return std::nullopt;
  if (expr.getKind() == AffineExprKind::Add)
    return llvm::checkedAddUnsigned(*lhs, *rhs);
  if (expr.getKind() == AffineExprKind::Mul)
    return llvm::checkedMulUnsigned(*lhs, *rhs);
  return std::max(*lhs, *rhs);
}

/// Whether MLIR's dependence analysis can decide the hoist of `band`: every
/// bound map of the loops around the accesses (the band's and the enclosing
/// ones') and every access map of the body flattens to coefficients and
/// constants below 2^48 in magnitude. The analysis computes them, their
/// negations and their sums in 64-bit integers, which wrap beyond that
/// (review of 2026-10-19, round 4: with the lower bound max(-2^63, 0) it
/// found no dependence between the iterations of i, j, k, t, so the hoist
/// moved t out of the reduction loop; under strict floating point the nest
/// was then refused, and its CPU code ran the accumulation permuted).
static bool dependenceAnalysisFits(ArrayRef<affine::AffineForOp> band) {
  constexpr uint64_t kLimit = uint64_t(1) << 48;
  auto fits = [&](AffineMap map) {
    return llvm::all_of(map.getResults(), [&](AffineExpr result) {
      std::optional<uint64_t> magnitude = flattenedMagnitude(result);
      return magnitude && *magnitude < kLimit;
    });
  };
  affine::AffineForOp innermost = band.back();
  for (Operation *op = innermost; op; op = op->getParentOp())
    if (auto loop = dyn_cast<affine::AffineForOp>(op))
      if (!fits(loop.getLowerBoundMap()) || !fits(loop.getUpperBoundMap()))
        return false;
  for (Operation &op : *innermost.getBody()) {
    if (auto load = dyn_cast<affine::AffineLoadOp>(op))
      if (!fits(load.getAffineMap()))
        return false;
    if (auto store = dyn_cast<affine::AffineStoreOp>(op))
      if (!fits(store.getAffineMap()))
        return false;
  }
  return true;
}

/// Loops of a perfect nest around a multiply-accumulate whose IV no access
/// uses repeat the accumulation rather than being GEMM dimensions (a loop of
/// one iteration with constant bounds is a dimension of one value, unless a
/// loop's bounds read its counter). The
/// recognizer takes a GEMM as a perfect chain of loops that are all its
/// dimensions, with any repeating loops outside it (AffineMatmulAnalysis:
/// recognizeLogicalMatmul); a repeating loop between the dimensions -- at any
/// depth -- is moved outward here, ahead of them, keeping the relative order
/// of both groups. When no repeating loop that may run twice lies inside a
/// dimension that may run twice, the move keeps the order of every two
/// iterations and is made without a test (review of 2026-10-19). Otherwise
/// it is made only when every other memref of the body is provably disjoint
/// from each one it stores to (the alias rule) and the dependence test below
/// finds no dependent iterations it would reorder (it is not trusted with
/// constants of 2^48 or more: dependenceAnalysisFits); else the nest is left
/// as it is, with a remark -- silently when the bounds tie a repetition loop
/// to a dimension (boundsOk): the recognizer then refuses it, UNKNOWN.
///
/// A repeating loop that may run twice inside a reduction loop of two or
/// more iterations would interleave the repetitions with the reduction,
/// which the dependences on C forbid -- such a nest stays on the CPU; one
/// outside the reduction loop moves out without changing the order in which
/// any C element accumulates.
///
/// The legality test is exact over the integers (since 2026-10-18). Moving
/// the repeating loops outward turns a dependence vector lexicographically
/// negative iff two iterations that access one element of a memref (one of
/// them a store) differ in a dimension loop that encloses a repeating loop
/// that may run twice: with the repeating loops first, their components --
/// free, since no access reads them -- may be made negative while the
/// original order was positive. So, for every such repeating loop
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
  // MLIR's loop depth counts every affine.for enclosing the accesses, those
  // outside the band included.
  const unsigned outer = affine::getNestingDepth(outermost.getOperation());
  for (unsigned rho : repeating) {
    // A repeating loop of one iteration (or none) reorders nothing.
    if (!mayRunTwice(band[rho]))
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

//===----------------------------------------------------------------------===//
// Rewrites of the nests as written, before the box pipeline: the last write
// of an assignment (docs/AFFINE_GEMM_MAPPING.md section 20.5) and the
// pre-reduction of the factors of an accumulation (section 24).
//===----------------------------------------------------------------------===//

/// The perfect bands of `module`, outermost loop first: one for each loop
/// that is not the only operation of its parent loop.
static SmallVector<SmallVector<affine::AffineForOp>>
perfectBandsOf(ModuleOp module) {
  SmallVector<SmallVector<affine::AffineForOp>> bands;
  module.walk([&](affine::AffineForOp loop) {
    auto parent = dyn_cast<affine::AffineForOp>(loop->getParentOp());
    if (parent && &parent.getBody()->front() == loop.getOperation() &&
        std::next(parent.getBody()->begin(), 2) == parent.getBody()->end())
      return;
    SmallVector<affine::AffineForOp> band;
    affine::getPerfectlyNestedLoops(band, loop);
    bands.push_back(std::move(band));
  });
  return bands;
}

/// A loop of a band with constant bounds: x = lower + step * z, z in
/// [0, trips).
struct ConstantLoop {
  int64_t lower = 0, trips = 0, step = 1;
};

/// The band's loops when every bound is a constant (a box as written) and
/// no loop has results; nullopt otherwise.
static std::optional<SmallVector<ConstantLoop>>
constantLoopsOf(ArrayRef<affine::AffineForOp> band) {
  SmallVector<ConstantLoop> loops;
  for (affine::AffineForOp loop : band) {
    std::optional<int64_t> lower = constantAffineForBound(loop, /*lower=*/true);
    std::optional<int64_t> upper =
        constantAffineForBound(loop, /*lower=*/false);
    if (loop.getNumResults() != 0 || !lower || !upper)
      return std::nullopt;
    ConstantLoop c{*lower, 0, loop.getStepAsInt()};
    if (*upper > *lower) {
      std::optional<int64_t> span = llvm::checkedSub(*upper, *lower);
      if (!span)
        return std::nullopt;
      c.trips = (*span - 1) / c.step + 1;
    }
    loops.push_back(c);
  }
  return loops;
}

/// Whether an access with `map` over `operands` reads the counter `iv`.
static bool readsCounter(AffineMap map, ValueRange operands, Value iv) {
  for (auto [position, operand] : llvm::enumerate(operands))
    if (operand == iv &&
        (position < map.getNumDims()
             ? map.isFunctionOfDim(position)
             : map.isFunctionOfSymbol(position - map.getNumDims())))
      return true;
  return false;
}

/// Last-write-wins (section 20.5): in an assignment C = A * B on a box as
/// written, a loop of two or more iterations that C does not read writes the
/// same elements of C at every iteration, and in the order written only its
/// last iteration's product survives: for one element of C, the iterations
/// that write it form a product with that loop's whole range (the box), so
/// the last of them in the lexicographic order of execution has that loop at
/// its last value -- whatever the other loops, whether or not C is injective
/// on them. Such loops are cut to that last iteration, [last, last + 1): the
/// nest then writes every element it wrote, in the same order, with the same
/// final value. Only when nothing else can see the removed writes: A and B
/// provably do not overlap C, the body is the one product stored, the domain
/// is not empty; and only where the nest may be lowered (the permission to
/// reassociate), so that strict floating point keeps the nest as written.
/// The assignment is not made an accumulation; whether the nest is then a
/// GEMM is decided as for any assignment (section 20).
static void applyLastWriteWins(ModuleOp module, bool assumeNoAlias,
                               bool allowFpReassociation, bool report) {
  for (SmallVector<affine::AffineForOp> &band : perfectBandsOf(module)) {
    SmallVector<affine::AffineLoadOp> loads;
    affine::AffineStoreOp store;
    arith::MulFOp mul;
    bool other = false;
    for (Operation &op : *band.back().getBody()) {
      if (auto load = dyn_cast<affine::AffineLoadOp>(op))
        loads.push_back(load);
      else if (auto s = dyn_cast<affine::AffineStoreOp>(op))
        other |= static_cast<bool>(std::exchange(store, s));
      else if (auto m = dyn_cast<arith::MulFOp>(op))
        other |= static_cast<bool>(std::exchange(mul, m));
      else if (!isa<affine::AffineYieldOp>(op))
        other = true;
    }
    if (other || loads.size() != 2 || !store || !mul ||
        store.getValueToStore() != mul.getResult())
      continue;
    // Only where the nest may be lowered: an assignment is lowered only with
    // the permission to reassociate (section 20); under strict floating
    // point the nest stays exactly as written.
    if (!allowFpReassociation &&
        !arith::bitEnumContainsAll(mul.getFastmath(),
                                   arith::FastMathFlags::reassoc))
      continue;
    // The product of the two loads, each read once.
    if (!((mul.getLhs() == loads[0].getResult() &&
           mul.getRhs() == loads[1].getResult()) ||
          (mul.getLhs() == loads[1].getResult() &&
           mul.getRhs() == loads[0].getResult())))
      continue;
    if (!llvm::all_of(loads, [&](affine::AffineLoadOp load) {
          return provablyDisjointFromOutput(load.getMemRef(),
                                            store.getMemRef(), assumeNoAlias);
        }))
      continue;
    std::optional<SmallVector<ConstantLoop>> loops = constantLoopsOf(band);
    if (!loops || llvm::any_of(*loops, [](const ConstantLoop &l) {
          return l.trips == 0;
        }))
      continue;
    SmallVector<unsigned> cut;
    for (auto [d, loop] : llvm::enumerate(band))
      if ((*loops)[d].trips >= 2 &&
          !readsCounter(store.getMap(), store.getMapOperands(),
                        loop.getInductionVar()))
        cut.push_back(d);
    if (cut.empty())
      continue;
    for (unsigned d : cut) {
      const ConstantLoop &l = (*loops)[d];
      const int64_t last = l.lower + (l.trips - 1) * l.step;
      band[d].setConstantLowerBound(last);
      band[d].setConstantUpperBound(last + 1);
    }
    if (report) {
      auto remark = band.front()->emitRemark()
                    << "affine assignment nest C = A * B: last write wins "
                       "(docs/AFFINE_GEMM_MAPPING.md section 20.5): loop(s) ";
      llvm::interleave(cut, remark, "+");
      remark << " of the nest, which C does not read, run their last "
                "iteration only (the earlier ones write values overwritten "
                "later)";
    }
  }
}

/// Pre-reduction of the factors (section 24, with --fp-distribute only): in
/// an accumulation C += A * B on a box as written whose factors share no loop
/// that C does not read (L_AB empty), the loops read by one factor only are
/// summed into that factor first --
///   sum_{z_A, z_B} A(o, z_A) B(o, z_B) = (sum_{z_A} A(o, z_A)) (sum_{z_B} B(o, z_B))
/// (Lemma 7.6 as formalized in section 24: the box makes the reduction domain
/// of every output point the product of the A-only and the B-only ranges,
/// independent of it, and distributivity factors the sum). The derived
/// operands A' (indexed by the loops A and C read) and B' are fresh buffers,
/// computed on the host; the nest becomes C += A' * B' over the loops C reads.
/// A and B are the product's first and second factor.
/// The premises: the multiply-accumulate body; constant bounds, a nonempty
/// domain; every loop of two or more iterations read by some operand; no
/// loop read by both factors and not by C; at least one loop read by one
/// factor only; A and B provably disjoint from C; unit steps on the loops
/// that index A' and B'. Floating point is then taken as commutative,
/// associative and distributive (the permission --fp-distribute): the
/// result differs in rounding from the loops as written.
static void preReduceFactors(ModuleOp module, bool assumeNoAlias,
                             bool report) {
  for (SmallVector<affine::AffineForOp> &band : perfectBandsOf(module)) {
    SmallVector<affine::AffineLoadOp> loads;
    affine::AffineStoreOp store;
    arith::MulFOp mul;
    arith::AddFOp add;
    bool other = false;
    for (Operation &op : *band.back().getBody()) {
      if (auto load = dyn_cast<affine::AffineLoadOp>(op))
        loads.push_back(load);
      else if (auto s = dyn_cast<affine::AffineStoreOp>(op))
        other |= static_cast<bool>(std::exchange(store, s));
      else if (auto m = dyn_cast<arith::MulFOp>(op))
        other |= static_cast<bool>(std::exchange(mul, m));
      else if (auto a = dyn_cast<arith::AddFOp>(op))
        other |= static_cast<bool>(std::exchange(add, a));
      else if (!isa<affine::AffineYieldOp>(op))
        other = true;
    }
    if (other || loads.size() != 3 || !store || !mul || !add ||
        store.getValueToStore() != add.getResult())
      continue;
    const bool productFirst = add.getLhs() == mul.getResult();
    Value accValue = productFirst                         ? add.getRhs()
                     : add.getRhs() == mul.getResult() ? add.getLhs()
                                                          : Value();
    auto acc = accValue ? accValue.getDefiningOp<affine::AffineLoadOp>()
                        : affine::AffineLoadOp();
    auto aLoad = mul.getLhs().getDefiningOp<affine::AffineLoadOp>();
    auto bLoad = mul.getRhs().getDefiningOp<affine::AffineLoadOp>();
    if (!acc || !aLoad || !bLoad || acc == aLoad || acc == bLoad ||
        aLoad == bLoad || acc.getMemRef() != store.getMemRef() ||
        acc.getMap() != store.getMap() ||
        !llvm::equal(acc.getMapOperands(), store.getMapOperands()))
      continue;
    Value c = store.getMemRef();
    if (!provablyDisjointFromOutput(aLoad.getMemRef(), c, assumeNoAlias) ||
        !provablyDisjointFromOutput(bLoad.getMemRef(), c, assumeNoAlias))
      continue;
    std::optional<SmallVector<ConstantLoop>> loops = constantLoopsOf(band);
    if (!loops || llvm::any_of(*loops, [](const ConstantLoop &l) {
          return l.trips == 0;
        }))
      continue;

    // The loops by readers: A-only, B-only (summed), A and C, B and C (the
    // derived operands' indices); none may be read by both factors without C.
    const unsigned n = band.size();
    SmallVector<unsigned> onlyA, onlyB, keepA, keepB;
    bool shared = false, unread = false, stepped = false;
    for (unsigned d = 0; d < n; ++d) {
      if ((*loops)[d].trips < 2)
        continue;
      Value iv = band[d].getInductionVar();
      const bool ra = readsCounter(aLoad.getMap(), aLoad.getMapOperands(), iv);
      const bool rb = readsCounter(bLoad.getMap(), bLoad.getMapOperands(), iv);
      const bool rc = readsCounter(store.getMap(), store.getMapOperands(), iv);
      unread |= !ra && !rb && !rc;
      shared |= ra && rb && !rc;
      if (ra && !rb && !rc)
        onlyA.push_back(d);
      if (rb && !ra && !rc)
        onlyB.push_back(d);
      if (ra && rc)
        keepA.push_back(d);
      if (rb && rc)
        keepB.push_back(d);
      stepped |= (rc && (ra || rb)) && (*loops)[d].step != 1;
    }
    if (shared || unread || stepped || (onlyA.empty() && onlyB.empty()))
      continue;

    affine::AffineForOp root = band.front();
    Location loc = root.getLoc();
    MLIRContext *context = root.getContext();
    // A nest of the band's loops `included` (and every loop of one
    // iteration), the builder left in its innermost body.
    auto emitNest = [&](OpBuilder &builder, ArrayRef<bool> included,
                        IRMapping &mapping) {
      for (unsigned d = 0; d < n; ++d) {
        const ConstantLoop &l = (*loops)[d];
        if (!included[d] && l.trips >= 2)
          continue;
        auto loop = affine::AffineForOp::create(
            builder, loc, l.lower, l.lower + (l.trips - 1) * l.step + 1,
            l.step);
        mapping.map(band[d].getInductionVar(), loop.getInductionVar());
        builder.setInsertionPointToStart(loop.getBody());
      }
    };
    // The index of a derived operand: its loops' ordinals (unit steps).
    auto indexMap = [&](ArrayRef<unsigned> keep) {
      SmallVector<AffineExpr> results;
      for (auto [t, d] : llvm::enumerate(keep))
        results.push_back(getAffineDimExpr(t, context) - (*loops)[d].lower);
      if (keep.empty())
        results.push_back(getAffineConstantExpr(0, context));
      return AffineMap::get(keep.size(), 0, results, context);
    };
    auto indexOperands = [&](ArrayRef<unsigned> keep, IRMapping &mapping) {
      SmallVector<Value> operands;
      for (unsigned d : keep)
        operands.push_back(mapping.lookup(band[d].getInductionVar()));
      return operands;
    };

    OpBuilder builder(root);
    // A' = sum over the loops only this factor reads, zeroed first.
    auto derive = [&](affine::AffineLoadOp load, ArrayRef<unsigned> keep,
                      ArrayRef<unsigned> summed) -> Value {
      if (summed.empty())
        return Value();
      Type element = cast<MemRefType>(load.getMemRef().getType())
                         .getElementType();
      SmallVector<int64_t> shape;
      for (unsigned d : keep)
        shape.push_back((*loops)[d].trips);
      if (shape.empty())
        shape.push_back(1);
      Value buffer = memref::AllocOp::create(
          builder, loc, MemRefType::get(shape, element));
      Value zero = arith::ConstantOp::create(builder, loc,
                                             builder.getZeroAttr(element));
      {
        OpBuilder zb(builder.getInsertionBlock(), builder.getInsertionPoint());
        SmallVector<Value> ivs;
        for (int64_t extent : shape) {
          auto loop = affine::AffineForOp::create(zb, loc, 0, extent, 1);
          ivs.push_back(loop.getInductionVar());
          zb.setInsertionPointToStart(loop.getBody());
        }
        affine::AffineStoreOp::create(zb, loc, zero, buffer, ivs);
      }
      {
        OpBuilder sb(builder.getInsertionBlock(), builder.getInsertionPoint());
        IRMapping mapping;
        SmallVector<bool> included(n, false);
        for (unsigned d : keep)
          included[d] = true;
        for (unsigned d : summed)
          included[d] = true;
        emitNest(sb, included, mapping);
        Value term = sb.clone(*load, mapping)->getResult(0);
        AffineMap map = indexMap(keep);
        SmallVector<Value> operands = indexOperands(keep, mapping);
        Value partial =
            affine::AffineLoadOp::create(sb, loc, buffer, map, operands);
        Value sum = arith::AddFOp::create(sb, loc, partial, term);
        affine::AffineStoreOp::create(sb, loc, sum, buffer, map, operands);
      }
      return buffer;
    };
    Value aBuffer = derive(aLoad, keepA, onlyA);
    Value bBuffer = derive(bLoad, keepB, onlyB);

    // C += A' * B' over the loops C reads, the accumulation's flags plus
    // reassoc: the permission covers the reordered sums.
    {
      OpBuilder mb(builder.getInsertionBlock(), builder.getInsertionPoint());
      IRMapping mapping;
      SmallVector<bool> included(n, true);
      for (unsigned d : onlyA)
        included[d] = false;
      for (unsigned d : onlyB)
        included[d] = false;
      emitNest(mb, included, mapping);
      Value cValue = mb.clone(*acc, mapping)->getResult(0);
      auto factor = [&](affine::AffineLoadOp load, Value buffer,
                        ArrayRef<unsigned> keep) -> Value {
        if (!buffer)
          return mb.clone(*load, mapping)->getResult(0);
        return affine::AffineLoadOp::create(mb, loc, buffer, indexMap(keep),
                                            indexOperands(keep, mapping));
      };
      Value aValue = factor(aLoad, aBuffer, keepA);
      Value bValue = factor(bLoad, bBuffer, keepB);
      Value product =
          arith::MulFOp::create(mb, loc, aValue, bValue, mul.getFastmathAttr());
      auto flags = arith::FastMathFlagsAttr::get(
          context, add.getFastmath() | arith::FastMathFlags::reassoc);
      Value sum = productFirst
                      ? arith::AddFOp::create(mb, loc, product, cValue, flags)
                      : arith::AddFOp::create(mb, loc, cValue, product, flags);
      mapping.map(add.getResult(), sum);
      mb.clone(*store, mapping);
    }
    // The buffers are freed by the pipeline's buffer deallocation (an explicit
    // memref.dealloc is refused by the lowering to the LLVM dialect).
    if (report) {
      auto remark = root->emitRemark()
                    << "affine multiply-accumulate nest pre-reduced "
                       "(--fp-distribute, docs/AFFINE_GEMM_MAPPING.md section "
                       "24): ";
      auto list = [&](StringRef name, ArrayRef<unsigned> summed) {
        remark << name << " summed over loop(s) ";
        if (summed.empty())
          remark << "none";
        llvm::interleave(summed, remark, "+");
      };
      list("the first factor", onlyA);
      remark << ", ";
      list("the second factor", onlyB);
      remark << " before the GEMM (no loop is read by both factors and not "
                "by C)";
    }
    root->erase();
  }
}

static void hoistRepetitionLoops(ModuleOp module, bool assumeNoAlias,
                                 bool classify) {
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
    // A multiply-accumulate body and loops without results; the bounds are
    // checked below (constants for the GEMM loops, a repeating loop's may
    // read outer repeating counters).
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
          return loop.getNumResults() == 0;
        }))
      continue;

    // A loop whose counter an access uses is a GEMM dimension; so is a loop
    // with constant bounds that runs exactly once whatever its uses (a
    // constant on the domain, docs/AFFINE_GEMM_MAPPING.md Lemma 0: the
    // recognizer reads it as an iteration dimension of one value, review of
    // 2026-10-18) -- constant as the recognizer folds them, a max / min of
    // constants included (constantAffineForBound, review of 2026-10-19).
    // Every other loop repeats the accumulation -- one that runs once with
    // bounds that read outer counters too (k in [r, r + 1)): no constant, the
    // recognizer does not take it for a dimension (singleIterationValue), and
    // it repeats nothing (review of 2026-10-19) -- and so does a loop of one
    // iteration whose counter a loop's bounds read (r1 in [t, 3)): it cannot
    // run twice, so it moves outward with its readers keeping the program's
    // order, while as a dimension it made its readers stop the hoist
    // (boundsOk; review of 2026-10-19, round 4: such nests stayed on the
    // host, and a max / min spelling of t lost lowerings the previous build
    // made).
    auto constantBounds = [](affine::AffineForOp loop) {
      return constantAffineForBound(loop, /*lower=*/true) &&
             constantAffineForBound(loop, /*lower=*/false);
    };
    auto readByBounds = [&](Value iv) {
      return llvm::any_of(band, [&](affine::AffineForOp other) {
        return llvm::is_contained(other.getLowerBoundOperands(), iv) ||
               llvm::is_contained(other.getUpperBoundOperands(), iv);
      });
    };
    SmallVector<unsigned> repeating, dimensions;
    for (auto [index, loop] : llvm::enumerate(band)) {
      std::optional<uint64_t> trip = fixedTripCount(loop);
      const bool dimension =
          llvm::is_contained(used, loop.getInductionVar()) ||
          (trip && *trip == 1 && constantBounds(loop) &&
           !readByBounds(loop.getInductionVar()));
      (dimension ? dimensions : repeating).push_back(index);
    }
    if (repeating.empty() || dimensions.empty())
      continue;
    // Bounds: a dimension's are constants (the box the recognizer reads; a
    // constant map's operands can only be values outside it, which stay
    // outside it). A repeating loop's may read the counters of repeating
    // loops outside it and values defined outside the band -- a triangle of
    // repetitions (docs section 19.13) -- but never a dimension's counter:
    // the repetitions and the dimensions are then independent, the iteration
    // set is their product (every point of the GEMM runs as often as the
    // repetition loops have points), and the hoist moves no bound operand
    // inward past its definition. Its operands are checked when its maps are
    // constant too: a constant map may list operands it does not read, and
    // the permutation would have moved the loop past their definitions
    // (review of 2026-10-19: invalid IR).
    bool boundsOk = true;
    for (auto [index, loop] : llvm::enumerate(band)) {
      if (llvm::is_contained(dimensions, unsigned(index))) {
        if (!constantBounds(loop)) {
          boundsOk = false;
          break;
        }
        continue;
      }
      for (ValueRange operands :
           {ValueRange(loop.getLowerBoundOperands()),
            ValueRange(loop.getUpperBoundOperands())})
        for (Value operand : operands)
          for (auto [t, outer] : llvm::enumerate(band))
            if (outer.getInductionVar() == operand &&
                (t >= index || !llvm::is_contained(repeating, unsigned(t))))
              boundsOk = false;
    }
    if (!boundsOk)
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
    // The interchange changes the order of two iterations only if they differ
    // in a dimension that runs at least twice and in a repeating loop inside
    // it that may run twice (the first repeating loop inside the dimension in
    // which they differ reads only counters on which they agree, so a
    // one-iteration one takes one value for both). Moving repeating loops
    // outward past loops that run once -- a leading one-iteration loop --
    // keeps the program's order, so it needs neither the alias rule nor the
    // dependence test (review of 2026-10-19: the alias rule refused such a
    // nest, which the inner GEMM candidate then recognized).
    bool reorders = false;
    for (unsigned rho : repeating)
      for (unsigned s : dimensions)
        if (s < rho && mayRunTwice(band[rho]) && mayRunTwice(band[s]))
          reorders = true;
    if (!reorders) {
      affine::permuteLoops(band, permMap);
      continue;
    }
    // The dependence test below sees one memref at a time (accesses through
    // different memref values never depend for checkMemrefAccessDependence),
    // so it decides the interchange only when every other memref of the body
    // is provably disjoint from each one the body stores to -- the alias rule
    // of the lowering (provablyDisjointFromOutput). Otherwise the nest stays
    // as written: it is not lowered then either, and its CPU code must keep
    // the program's order (review of 2026-10-18: a hoisted nest refused for
    // 'A may overlap C' ran its updates permuted).
    bool disjoint = true;
    for (Operation &op : *band.back().getBody()) {
      auto store = dyn_cast<affine::AffineStoreOp>(op);
      if (!store)
        continue;
      for (Operation &other : *band.back().getBody()) {
        Value memref;
        if (auto load = dyn_cast<affine::AffineLoadOp>(other))
          memref = load.getMemRef();
        else if (auto st = dyn_cast<affine::AffineStoreOp>(other))
          memref = st.getMemRef();
        if (memref && memref != store.getMemRef() &&
            !provablyDisjointFromOutput(memref, store.getMemRef(),
                                        assumeNoAlias))
          disjoint = false;
      }
    }
    // Each refusal leaves the nest as written: a repeating loop that may run
    // twice stays inside a dimension that may run twice, so every GEMM
    // candidate that covers the accesses' counters holds it, and the
    // recognizer refuses the nest. Whether it computes a GEMM is not decided
    // (UNKNOWN, docs section 19.9).
    if (!disjoint) {
      auto remark =
          root->emitRemark()
          << "affine GEMM not lowered to systolic.matmul_tile: loop "
          << repeating.back() << " of the nest repeats the accumulation "
          << "inside it, and moving it outward is a legal loop interchange "
          << "only if the memrefs the nest reads do not overlap the one it "
          << "writes; declare the arguments llvm.noalias (C restrict)";
      if (classify)
        remark << " ["
               << stringifyGemmMappingClass(GemmMappingClass::Unknown) << "]";
      continue;
    }
    if (!dependenceAnalysisFits(band)) {
      auto remark =
          root->emitRemark()
          << "affine GEMM not lowered to systolic.matmul_tile: loop "
          << repeating.back() << " of the nest repeats the accumulation "
          << "inside it, and whether moving it outward reorders dependent "
          << "iterations is not decided (a bound or subscript has a constant "
          << "of 2^48 or more, beyond the dependence analysis' 64-bit "
          << "arithmetic)";
      if (classify)
        remark << " ["
               << stringifyGemmMappingClass(GemmMappingClass::Unknown) << "]";
      continue;
    }
    if (hoistReordersDependentIterations(band, repeating, dimensions)) {
      auto remark =
          root->emitRemark()
          << "affine GEMM not lowered to systolic.matmul_tile: loop "
          << repeating.back() << " of the nest repeats the accumulation "
          << "inside it, and moving it outward is not a legal loop "
          << "interchange (it would reorder dependent iterations)";
      if (classify)
        remark << " ["
               << stringifyGemmMappingClass(GemmMappingClass::Unknown) << "]";
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

  Option<bool> reductionBatches{
      *this, "reduction-batches",
      llvm::cl::desc("Reduction-batch partitioning "
                     "(docs/AFFINE_GEMM_MAPPING.md section 23): loops C does "
                     "not read may become reduction batch loops, run around "
                     "the tiles and accumulated into the same slab of C in "
                     "order (on by default; false: the recognition of "
                     "sections 19-21 alone)"),
      llvm::cl::init(true)};

  Option<bool> lastWriteWins{
      *this, "last-write-wins",
      llvm::cl::desc("In an assignment C = A * B on a box, cut the loops C "
                     "does not read to their last iteration, whose product "
                     "is the one that survives (docs/AFFINE_GEMM_MAPPING.md "
                     "section 20.5; on by default)"),
      llvm::cl::init(true)};

  Option<bool> fpDistribute{
      *this, "fp-distribute",
      llvm::cl::desc("Permit taking floating point as commutative, "
                     "associative and distributive to pre-reduce the "
                     "factors of an accumulation whose factors share no "
                     "loop C does not read (docs/AFFINE_GEMM_MAPPING.md "
                     "section 24): the result differs in rounding from the "
                     "loops as written. Not implied by "
                     "allow-fp-reassociation"),
      llvm::cl::init(false)};

  void getDependentDialects(DialectRegistry &registry) const override {
    registry.insert<affine::AffineDialect, bufferization::BufferizationDialect,
                    tensor::TensorDialect, memref::MemRefDialect,
                    SystolicDialect>();
  }

  /// The systolic.gemm index the next GEMM lowered through a plan gets
  /// (SystolicTilePlan.h nextSystolicGemmIndex): numbered like the linalg
  /// path's GEMMs, so the asynchronous FPGA lowering can hand this GEMM's
  /// tiles to the runtime as one GEMM.
  int64_t nextGemmIndex = 0;

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
                                 matmul.viewA, matmul.layoutA, matmul.symbolicA,
                                 matmul.symbolsA, "A", why, mappingClass);
    if (failed(lhs))
      return failure();
    auto rhs = getLogicalOperand(matmul.rhs, matmul, *dense, kLogicalK, kLogicalJ,
                                 matmul.viewB, matmul.layoutB, matmul.symbolicB,
                                 matmul.symbolsB, "B", why, mappingClass);
    if (failed(rhs))
      return failure();
    auto out = getLogicalOperand(matmul.output, matmul, *dense, kLogicalI, kLogicalJ,
                                 matmul.viewC, matmul.layoutC, matmul.symbolicC,
                                 matmul.symbolsC, "C", why, mappingClass);
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
    // ------------------------------------------------------------------
    // An assignment C = A * B (docs section 20): every element of C is
    // written exactly once -- C's subscripts injective on the whole iteration
    // domain, batches included (the recognizer refused the loops C does not
    // read; a batch coordinate may still shift C's window onto an earlier
    // batch's) -- and the reduction has extent one (each element gets one
    // product; follows from the injectivity, checked here for the
    // zero-then-accumulate lowering below). Decided before the floating-point
    // permission: a nest that is no GEMM is PROVEN_INVALID whatever the
    // permission.
    // ------------------------------------------------------------------
    if (!matmul.accumulates) {
      if (failed(verifyOutputWrittenOnce(matmul, &why))) {
        mappingClass = GemmMappingClass::ProvenInvalid;
        return failure();
      }
      if (K != 1) {
        why = "the assignment C = A * B has a reduction of extent " +
              std::to_string(K) + ": only its last product would survive";
        mappingClass = GemmMappingClass::ProvenInvalid;
        return failure();
      }
    }

    if (!matmul.reductionMayReassociate && !allowFpReassociation) {
      why = matmul.accumulates
                ? "floating-point reassociation is not permitted: the "
                  "systolic execution sums each tile from zero and adds the "
                  "sum to C, a different order of IEEE additions than the "
                  "loop's (mark the accumulation reassoc -- systolic-cc "
                  "--fp-reassociation -- or pass allow-fp-reassociation)"
                : "floating-point reassociation is not permitted: the "
                  "systolic execution sums each tile from zero (+0) and "
                  "writes the sum to C, so a -0 product of the assignment "
                  "C = A * B becomes +0 (mark the product reassoc -- "
                  "systolic-cc --fp-reassociation -- or pass "
                  "allow-fp-reassociation)";
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
    if (!matmul.accumulates) {
      // C = A * B: the tiles accumulate into C's prior value (C + sum from
      // zero), so the window the nest writes -- exactly the logical M x N view
      // (verifyLogicalBijection, verifyOutputWrittenOnce) -- is zeroed first,
      // per batch iteration, before the tensor of C is taken. The result is
      // 0 + (0 + a b): the product, except that a -0 product becomes +0
      // (section 6, the permission checked above).
      Value zero = arith::ConstantOp::create(builder, loc,
                                             builder.getZeroAttr(elementType));
      auto rows = affine::AffineForOp::create(builder, loc, 0, out->shape[0],
                                              /*step=*/1);
      builder.setInsertionPointToStart(rows.getBody());
      auto columns = affine::AffineForOp::create(builder, loc, 0,
                                                 out->shape[1], /*step=*/1);
      builder.setInsertionPointToStart(columns.getBody());
      affine::AffineStoreOp::create(
          builder, loc, zero, cView,
          ValueRange{rows.getInductionVar(), columns.getInductionVar()});
      builder.setInsertionPointAfter(rows);
    }
    Value cTensor = asTensor(cView, *out, /*writable=*/true);

    Value result;
    if (plan) {
      result = emitSystolicTilePlan(builder, loc, aTensor, bTensor, cTensor,
                                    *plan, fleet, nextGemmIndex++);
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
      if (!matmul.accumulates)
        remark << " (an assignment C = A B: C zeroed before the tiles)";
      // Reduction batch loops (section 23) are demoted loops too, but they
      // accumulate into one slab of C instead of partitioning it.
      const size_t partitionLoops =
          matmul.demotedLoops.size() - matmul.reductionBatchLoops.size();
      if (partitionLoops)
        remark << ", partitioned on " << partitionLoops
               << " demoted loop(s)";
      if (!matmul.reductionBatchLoops.empty())
        remark << ", " << matmul.reductionBatchLoops.size()
               << " reduction batch loop(s) (not read by C, accumulated "
                  "into C in order)";
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
    auto reportNormalizations =
        [&](ArrayRef<DomainNormalization> normalized) {
          if (!report)
            return;
          for (const DomainNormalization &dn : normalized) {
            std::string lines;
            llvm::raw_string_ostream os(lines);
            describeDomainNormalization(dn, os, "");
            dn.outermost->emitRemark()
                << "affine multiply-accumulate nest: iteration domain "
                   "normalized to a box (x = U x', "
                   "docs/AFFINE_GEMM_MAPPING.md section 19.12): "
                << llvm::StringRef(lines).rtrim();
          }
        };
    // On the nests as written: the last write of an assignment (section
    // 20.5), the pre-reduction of the factors with --fp-distribute (24).
    if (lastWriteWins)
      applyLastWriteWins(module, assumeNoAlias, allowFpReassociation, report);
    if (fpDistribute)
      preReduceFactors(module, assumeNoAlias, report);
    // Reduction batches (section 23) are made only on nests whose counters
    // are the ones as written: not after a normalization with U != I.
    const SmallVector<DomainNormalization, 1> normalizations =
        normalizeParallelotopeDomains(module.getOperation());
    reportNormalizations(normalizations);
    hoistRepetitionLoops(module, assumeNoAlias, classify);
    nextGemmIndex = nextSystolicGemmIndex(module);

    SmallVector<AffineMatmulCandidate> candidates;
    collectAffineMatmulCandidates(module.getOperation(), candidates);

    SmallVector<LogicalMatmul, 1> matmuls;
    for (AffineMatmulCandidate candidate : candidates) {
      AffineMatmulRecognitionOptions recognition;
      recognition.reductionBatches =
          reductionBatches &&
          !countersRewritten(normalizations, candidate.anchor);
      auto result =
          recognizeLogicalMatmul(candidate, nullptr, nullptr, recognition);
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
