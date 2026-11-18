//===- NestRecordLowering.h - A GEMM lowered from its record ------*- C++ -*-===//
//
// Stage 3 of docs/GENERALIZED_GEMM_SCOPE.md. With
// --affine-matmul-to-systolic="nest-records=FILE record-driven=true", a loop nest in which the
// production recognizer finds no GEMM is lowered from its record: the research recognizer's
// verified coordinates I, J, K and its operand mappings drive the lowering, and the
// production recognizer's success is not asked for. Everything stage 2 checks of the record
// by itself still holds (NestRecordGate.h, checkRecordAtNest): the binding, the roots, the
// index width, the verified claims. What stage 2 then compared with the production recognizer
// is compared here with the nest itself, read from its IR:
//
//   - the nest is a perfect band of loops with constant bounds and step 1, one per level of
//     the record, in its order, with the record's bounds, or with bounds that are the
//     record's less one constant (the record's level is then the counter plus that constant,
//     taken only with closed forms, which are rewritten to the counter before they are
//     compared with the nest's); the record's axes put I, J and K on
//     three of them, and its extents are their trip counts. A loop of the band is an
//     affine.for, an scf.for (bounds and step arith.constant values, an unsigned comparison
//     only of bounds that are not negative), or a counting scf.while: its one carried value is
//     its counter, an index started at a constant; its before region holds one comparison,
//     counter < constant, and hands the counter on as it is; its after region yields
//     counter + 1; its result has no use. An scf.for or scf.while may besides carry index
//     recurrences: a value of index type started at an arith.constant, handed on as itself
//     plus or minus a constant, and not used after the loop. Such a value is read, from the IR
//     alone, as start + step * (counter - lower) of its loop; the record's view must then be
//     that form, as for any index, and a record that says otherwise is refused. A loop that
//     carries anything else (a reduction held in a value, an index started or stepped by a
//     value known at run time, one read after the loop), every carried value of an affine.for,
//     and any other loop are refused;
//   - the innermost body is one multiply-accumulate and pure index arithmetic: three loads,
//     one arith.mulf of two of them, one arith.addf of the product and the third, one store of
//     the sum to the cell the third load read; the record's element type, operations, flags
//     and accumulator side are the nest's; f32 only (the device is binary32);
//   - the operands' memrefs are roots of the function, the ones the record names, with static
//     shapes and the identity layout; the separations the record rests on are proofs by an
//     allocation or the pairs (A, C) and (B, C), which the lowering's alias gate decides.
//
// Then one of two ways to read the operands:
//
//   formulas  The record has closed forms of the views (F(I, K), G(K, J), H(I, J)), proved
//             and affine. Each index the nest computes is read as an affine form of its loop
//             counters (constants, the counters, the index recurrences, addi, subi, muli by a
//             constant, affine.apply of such) and must be the record's form, dimension by dimension, reading only the
//             operand's two roles. The operands are then views of their memrefs: a subview
//             (transposed when the roles are stored the other way) when each of two dimensions
//             reads one role, an explicit view (one reinterpret_cast, positive element strides)
//             otherwise, as for a flat array. Every index is inside the memref, and C's view is
//             one to one (checked exactly over the box).
//   tables    The record is VERIFIED but for the closed forms of some views (the research
//             proposed none, or one that is not affine, as (5 i) mod 16), and the domain is
//             at most `tableBudget` iterations. Every
//             iteration of the nest's static domain is visited at compile time and the
//             addresses of its four accesses evaluated by the nest's own index arithmetic, with
//             no load, no store and no multiply-accumulate (an address that depends on memory,
//             on an argument, or on an operation the evaluator does not know is refused). The
//             tables keep each operand's element offset per (row, column) of its matrix, and
//             hold only if A's address is one per (I, K) whatever J, B's one per (K, J), C's
//             one per (I, J) and the same for the load and the store of the update; every
//             address is inside its memref, and no two cells (I, J) share an address of C.
//             The forward map takes an iteration to (I, J, K) (the counters, from their lower
//             bounds); its reverse holds one iteration per point, so it is a bijection, and
//             each contribution is counted once. A table is built at compile time for the one
//             static domain of the nest, which is every run's: nothing is sampled, and nothing
//             is said of another domain. Tables built at run time are not made.
//
// Nothing here changes the IR; AffineMatmulToSystolic.cpp emits the GEMM from the result,
// after its own alias gate and floating-point policy.
//
//===----------------------------------------------------------------------===//
#ifndef SYSTOLIC_NESTRECORDLOWERING_H
#define SYSTOLIC_NESTRECORDLOWERING_H

#include "Systolic/NestRecordGate.h"

#include "mlir/IR/Operation.h"
#include "mlir/IR/Value.h"
#include "llvm/ADT/SmallVector.h"

#include <array>
#include <cstdint>
#include <string>

namespace mlir::systolic {

/// How the GEMM reads one memref dimension of an operand, by formulas: the index is
/// scale * y + offset, y the coordinate of the operand's role `role` (0 or 1 in the operand's
/// GEMM order: A (I, K), B (K, J), C (I, J)), counted from 0.
struct DrivenDimension {
  int role = -1;
  int64_t scale = 0;
  int64_t offset = 0;
};

/// One operand of a GEMM lowered from its record.
struct DrivenOperand {
  Value memref;
  /// Formulas, a view of one role per dimension of a rank-2 memref: one entry per dimension.
  llvm::SmallVector<DrivenDimension, 2> dimensions;
  /// Formulas, any other affine view (a flat array, a dimension combining both roles): the
  /// element address of the matrix's (row, column) is elementOffset + elementStrides[0] * row +
  /// elementStrides[1] * column, over the memref's row-major strides; both strides positive.
  bool explicitView = false;
  int64_t elementOffset = 0;
  std::array<int64_t, 2> elementStrides{1, 1};
  /// Tables: the element offset (row-major over the memref's static shape) of each element of
  /// the operand's rows x columns matrix, row by row.
  llvm::SmallVector<int64_t> table;
};

struct RecordDrivenGemm {
  Operation *anchor = nullptr; // the nest's outermost loop
  int64_t M = 0, N = 0, K = 0;
  bool tables = false;
  std::array<DrivenOperand, 3> operands; // A, B, C
  bool reassoc = false;                  // the accumulation's arith.addf carries reassoc
  int64_t iterations = 0;                // tables: the iterations evaluated
  std::string where;                     // "nest N of F"
  std::string summary;                   // what was read and checked, for the remark
};

/// The default largest domain, in iterations, that the tables are built for.
constexpr int64_t kDefaultRecordTableBudget = int64_t(1) << 20;

/// The GEMM of the nest whose outermost loop is `anchor`, from its record in `file`, or a
/// refusal in `why` that names the nest once it is found. Nothing is changed.
FailureOr<RecordDrivenGemm> analyzeRecordDrivenNest(const NestRecordFile &file, const BoundNests &bound,
                                                    Operation *anchor, unsigned indexWidth, int64_t tableBudget,
                                                    std::string &why);

} // namespace mlir::systolic

#endif // SYSTOLIC_NESTRECORDLOWERING_H
