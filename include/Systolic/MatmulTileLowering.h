//===- MatmulTileLowering.h - What the GEMM lowerings share ------*- C++ -*-===//
//
// The steps that every lowering of a GEMM C += A * B to systolic.matmul_tile takes, whichever
// analysis found the GEMM: the lowering of a GEMM the affine recognizer finds
// (AffineMatmulToSystolic.cpp) and the lowering of a loop nest from its record
// (GeneralizedMatmulToSystolic.cpp). One implementation of each:
//
//   provablyDisjointFromOutput   the alias gate: A and B apart from C
//   LogicalOperand               an operand as the logical matrix it holds
//   materializeOperandView       that matrix as a rank-2 view of the operand's memref
//   materializeTableOperands     the matrices of operands that are no view of their memref:
//                                A, B and C gathered element by element through constant
//                                tables of offsets
//   scatterTableResult           and C's matrix written back through its table
//   planMatmulTiles              the hardware plan of the M x N x K GEMM on the fleet, and
//                                its tiles: as boxes of the iteration domain, for a caller
//                                that proves on a domain of its own that they partition it;
//                                or proven here an exact partition of the M x N x K box,
//                                for a caller whose domain is that box
//   emitMatmulTiles              the plan's tiles as one systolic GEMM, or without a fleet
//                                one unassigned systolic.matmul_tile
//   writeBackMatmulResult        the tiles' result written into C's buffer
//
// Nothing here decides that a nest is a GEMM or which matrix an operand holds. The callers
// do, each by its own analysis, and each keeps its own checks: the bounds, the
// floating-point policy, and, where the domain is not simply the box, that the tiles are
// an exact partition of it.
//
//===----------------------------------------------------------------------===//

#ifndef SYSTOLIC_MATMULTILELOWERING_H
#define SYSTOLIC_MATMULTILELOWERING_H

#include "Systolic/SystolicTilePlan.h"

#include "mlir/IR/AffineExpr.h"
#include "mlir/IR/AffineMap.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/Value.h"
#include "mlir/Support/LLVM.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"

#include <array>
#include <cstdint>
#include <optional>
#include <string>

namespace mlir {
namespace systolic {

/// A tile as a box of the iteration domain (Systolic/AffineMatmulPartitioning.h). Only the
/// affine lowering, which proves the partition on a domain of its own, takes the boxes; a
/// file that does not has no use for that header and does not get it from this one.
struct LogicalBox;

/// Whether `input` provably does not overlap `output`, the memref the GEMM
/// writes. The loop nest reads A and B while it writes C; the tiles read all
/// of A and B before writing back, so overlapping memory would change the
/// result. Views are traced to the buffer they view: two views of one buffer
/// may overlap; different buffers are disjoint when one of them is a fresh
/// allocation or a restrict argument.
bool provablyDisjointFromOutput(Value input, Value output, bool assumeNoAlias);

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

/// The operand's rank-2 view (rows x columns of the GEMM), on `builder`;
/// `batchIVs` are the counters of the enclosing batch loops, which re-emit
/// the nest's loops `batchLoops` (same order). A dimension with batch terms
/// is read at its static offset plus those terms of the counters -- one
/// affine.apply per such dimension, a dynamic offset of the subview. An
/// explicit view is one memref.reinterpret_cast: element offset (static, or
/// one affine.apply of the batch counters), sizes, element strides.
Value materializeOperandView(OpBuilder &builder, Location loc,
                             const LogicalOperand &operand,
                             ArrayRef<Value> batchIVs,
                             ArrayRef<unsigned> batchLoops,
                             llvm::DenseMap<AffineMap, Value> &applies);

/// The partition of the M x N x K GEMM at `anchor` into tiles. With a fleet: the hardware
/// plan (planSystolicMatmul: the decomposition over the fleet, K-folded and scheduled, by
/// the cost model `costModel` and the operand path `operandPath`, each "v1" or "v2", and the
/// shared read bandwidth), in `plan`, and each of its invocations as a box of the iteration
/// domain, in `boxes`. Without one: no plan, and the whole domain as one box. A failure is
/// a hard error, said in `hardError`: an option that is neither v1 nor v2 (`why` says so),
/// or no plan (planSystolicMatmul has reported it at `anchor`). That the boxes are an exact
/// partition is the caller's to prove, on its own domain.
LogicalResult planMatmulTiles(Operation *anchor, int64_t M, int64_t N, int64_t K,
                              const SystolicFleet &fleet,
                              double sharedReadBytesPerCycle, StringRef costModel,
                              StringRef operandPath,
                              std::optional<SystolicTilePlan> &plan,
                              SmallVectorImpl<LogicalBox> &boxes, std::string &why,
                              bool &hardError);

/// The same plan for a GEMM whose iteration domain is the whole box [0, M) x [0, N) x
/// [0, K), with the proof that goes with it: the tiles are an exact partition of the box,
/// each inside it, no two overlapping, and together as many points as it has (64-bit
/// arithmetic, checked). `regions` is how many they are; the caller needs no box. A plan
/// that fails is the hard error of the overload above; a partition that is not exact is a
/// refusal that says which of the three fails in `why`, and no hard error.
LogicalResult planMatmulTiles(Operation *anchor, int64_t M, int64_t N, int64_t K,
                              const SystolicFleet &fleet,
                              double sharedReadBytesPerCycle, StringRef costModel,
                              StringRef operandPath,
                              std::optional<SystolicTilePlan> &plan, size_t &regions,
                              std::string &why, bool &hardError);

/// The tiles of C += A * B on the three tensors, at the builder's point, and the tensor of
/// the result. With a plan: its tiles on the fleet's accelerators (emitSystolicTilePlan), as
/// the systolic GEMM numbered `nextGemmIndex`, which is advanced. Without one: a single
/// systolic.matmul_tile of the whole M x N x K GEMM, on no device.
Value emitMatmulTiles(OpBuilder &builder, Location loc, Value aTensor,
                      Value bTensor, Value cTensor, int64_t M, int64_t N,
                      int64_t K, const std::optional<SystolicTilePlan> &plan,
                      const SystolicFleet &fleet, int64_t &nextGemmIndex);

/// The tiles' result written into `destination`, the buffer the tensor of C was taken from:
/// one bufferization.materialize_in_destination, restrict and writable.
void writeBackMatmulResult(OpBuilder &builder, Location loc, Value result,
                           Value destination);

/// An operand whose rows x columns matrix is no view of its memref: element (r, c) of the
/// matrix is the memref's element at the row-major offset `table[r * columns + c]`, an
/// offset of the memref's static shape. The caller has the table from its own analysis and
/// has checked every offset.
struct TableOperand {
  Value memref;
  ArrayRef<int64_t> table;      // rows * columns offsets, row by row
  std::array<int64_t, 2> shape; // rows, columns
};

/// What materializeTableOperands made of A, B and C (in that order).
struct TableOperands {
  /// The operands gathered: rows x columns buffers, what the tiles run on.
  std::array<Value, 3> matrix;
  /// For scatterTableResult: the tables (memref.get_global), the operands' memrefs as flat
  /// buffers, and the shapes.
  std::array<Value, 3> table, flat;
  std::array<std::array<int64_t, 2>, 3> shape;
};

/// The matrices of A, B and C read through tables, at the builder's point. Per operand, in
/// order: a private constant memref.global of its table at the start of `module`, named
/// `__nest_table_<label>_A` (_B, _C) with every character of `label` that is no letter,
/// digit or underscore made an underscore, and with _1, _2, ... after it while the module
/// has a symbol of that name; memref.get_global of it; the operand's memref as a flat buffer
/// of `elementType` (memref.reinterpret_cast); and a rows x columns memref.alloc of
/// `elementType`. Then, per operand, the gather: matrix[r][c] = flat[table[r][c]], two
/// affine.for. The matrices are not freed here and are not the caller's to free:
/// --systolic-runtime-to-llvm's buffer deallocation frees them (an explicit dealloc of a
/// buffer given to to_tensor is refused there).
TableOperands materializeTableOperands(OpBuilder &builder, Location loc,
                                       ModuleOp module, StringRef label,
                                       const std::array<TableOperand, 3> &operands,
                                       Type elementType);

/// C's matrix written back to C's memref through C's table, at the builder's point:
/// flat[table[r][c]] = matrix[r][c], two affine.for. After the tiles' result is in the
/// matrix (writeBackMatmulResult).
void scatterTableResult(OpBuilder &builder, Location loc, const TableOperands &tables);

/// Experimental (exp/semgus-indirect-dp). One dimension of the memref a packed operand is
/// read from: its index at the matrix's element (row, column) is
/// base + coefficient[0] * row + coefficient[1] * column, or, when `gathered`, the value of
/// the one-dimensional integer memref `table` at that index, extended to index (index_cast,
/// or index_castui with `zeroExtend`): A[dp[i], k].
struct GatherPackDimension {
  bool gathered = false;
  Value table;
  bool zeroExtend = false;
  std::array<int64_t, 2> coefficient{0, 0};
  int64_t base = 0;
  /// Plus modScale * ((modCoefficient . (row, column) + modBase) mod modulus) when
  /// modulus > 0 (a closed-form inverse, exp/general-scatter).
  int64_t modScale = 0, modulus = 0, modBase = 0;
  std::array<int64_t, 2> modCoefficient{0, 0};
};

/// A rows x columns memref.alloc of `elementType`, filled at the builder's point by two
/// affine.for: matrix[r][c] = source[index_0(r, c), ...], one GatherPackDimension per
/// dimension of `source`. The packed matrix is a plain dense operand for the tiles. Like
/// the tables' matrices, it is freed by --systolic-runtime-to-llvm's buffer deallocation.
/// The caller has checked every affine index and every table index against its memref; the
/// gathered indices are the program's own and are read as the nest reads them.
Value materializeGatherPack(OpBuilder &builder, Location loc, Value source,
                            ArrayRef<GatherPackDimension> dimensions,
                            std::array<int64_t, 2> shape, Type elementType);

/// Experimental (exp/general-scatter), Track B: the run-time half of a scatter-add.
///
/// beginScatter, at the builder's point: for every (r, c) of the rows x columns domain, the
/// operations `slice` (the nest's own computation of C's indices, `store`'s operands) are
/// re-emitted with counterI = lowerI + r and counterJ = lowerJ + c; every index of a table
/// they load and every index of C is checked inside its memref (a failing index is replaced
/// by 0, so nothing is read or written outside), and the cell of C, row-major, is kept in a
/// targets buffer. Then an scf.if on "every index was inside": the builder is left at the
/// start of its then-block; `elseBlock` is where the nest, as written, is to be moved.
struct ScatterRegions {
  Value targets;            // memref<(rows * columns) x index>
  Block *elseBlock = nullptr;
  Operation *afterThen = nullptr; // the then-block's terminator
};
ScatterRegions beginScatter(OpBuilder &builder, Location loc, ArrayRef<Operation *> slice,
                            Operation *store, Value counterI, int64_t lowerI, Value counterJ,
                            int64_t lowerJ, Value cMemref, std::array<int64_t, 2> shape);

/// A rows x columns memref.alloc of `elementType` filled with zeros.
Value materializeZeroMatrix(OpBuilder &builder, Location loc, std::array<int64_t, 2> shape,
                            Type elementType);

/// The two-layer inverse map, at the builder's point, and the scatter-add: bucket offsets
/// (one per cell of C, plus one) by counting the targets and a prefix sum; the sources
/// (cells of `matrix`, row-major) sorted by target, stably; then, for each cell of C with a
/// source, its value plus those of its sources in order, written once.
void scatterAddByInverseMap(OpBuilder &builder, Location loc, Value matrix, Value targets,
                            Value cMemref);

} // namespace systolic
} // namespace mlir

#endif // SYSTOLIC_MATMULTILELOWERING_H
