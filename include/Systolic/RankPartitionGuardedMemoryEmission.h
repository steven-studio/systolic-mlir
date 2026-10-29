#pragma once

#include "Systolic/RankPartitionDependenceLegality.h"
#include "Systolic/RankPartitionExactInclusion.h"
#include "Systolic/RankPartitionLogicalDomain.h"
#include "Systolic/RankPartitionScheduleLegality.h"
#include "Systolic/RankPartitionSourceAccessEmission.h"
#include "Systolic/RankPartitionSymbolicDomain.h"

#include "mlir/Analysis/Presburger/IntegerRelation.h"
#include "mlir/Analysis/Presburger/PresburgerSpace.h"
#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/Dominance.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/Interfaces/SideEffectInterfaces.h"
#include "mlir/Interfaces/ViewLikeInterface.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DynamicAPInt.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallVector.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <vector>

namespace systolic::logical {

// Phase 5.4.3d-3b-2:
//
// Guarded memory operations.
//
// Inside the then-region of the exact Presburger scf.if,
// emit the memory operations of one source MAC statement
//
//   C[f_C(x)] <- g(A[f_A(x)], B[f_B(x)], C[f_C(x)])
//
// addressed by the already emitted Source IV Recovery
// SSA (x = M*z/d) and Source Affine Access SSA
// (f(x) = rows*x + offsets):
//
//   memref.load  A[f_A(x)]
//   memref.load  B[f_B(x)]
//   memref.load  C[f_C(x)]
//   ... caller-supplied memory-effect-free value g ...
//   memref.store C[f_C(x)]
//
// memref.load / memref.store are used because affine
// memory operations require valid affine dimensions or
// symbols as indices, which the reused i64 access SSA
// (arith operations inside scf.if) is not. The Read /
// Write effects on the addressed element are the same.
//
// Every check runs before any operation is created. A
// rejection returns std::nullopt and leaves the IR
// unchanged, so callers keep their existing fallback.
// The dependence-order check runs in BOTH calls, so a
// statement that cannot be emitted is refused before the
// first load exists.
//
// Certified by a successful emission:
//
//   C1 exactly once: the candidate box covers the
//      logical domain, the logical domain equals the
//      exact image T(D) of the source domain D, and the
//      recovered x is exactly T^{-1} z. Each x in D is
//      executed once, at z = T*x.
//   C2 same addresses: every memory operation addresses
//      f(x) = rows*x + offsets with the source x, the
//      index arithmetic cannot overflow i64 on D, and
//      the address lies inside the static memref shape.
//   C3 same effects: per executed iteration the nest
//      performs exactly Read A, Read B, Read C, Write C
//      and nothing else.
//   C4 dependence preservation: every pair of distinct
//      source iterations that touch the same C element
//      keeps its source execution order under the
//      lexicographic z schedule (Phase 5.4.1/5.4.2
//      analyses). No reduction is reassociated; an order
//      change is refused for every update operator.
//
// Caller obligations (not provable from the inputs):
//
//   - the guard predicate of `guarded` was emitted from
//     `plan` (the same trust as Phase 3a-2);
//   - dimension i of `sourceDomain` is the i-th source
//     loop from the outside, iterated in ascending order
//     with the statement executed once per point; C4 is
//     meaningless otherwise;
//   - memref values whose view roots differ do not
//     alias (the existing alias checks); identical SSA
//     values or identical view roots for A/C or B/C are
//     refused here, everything else is the caller's
//     certificate;
//   - the index type is 64 bits wide;
//   - the MemRef dialect is loaded in the context;
//   - when emitGuardedMemoryStore refuses after a
//     successful emitGuardedMemoryLoads (only a
//     caller-introduced condition does that: an
//     effecting, mistyped or non-dominating value, or
//     altered inputs), the three loads and their index
//     casts remain; the caller erases them, or the whole
//     nest, to restore its fallback.
//
// This phase does NOT:
//   - emit any arithmetic on the loaded values
//     (the MAC belongs to Phase 3b-3);
//   - reassociate or reorder a reduction;
//   - certify multi-disjunct logical domains, dynamic
//     shapes or non-identity memref layouts (refused);
//   - accept operands of different element types (A, B
//     and C share one signless-integer or float element
//     type) or coefficients and offsets outside
//     [-INT64_MAX, INT64_MAX] (refused);
//   - decide the domain equality when the plan's
//     disjunct or the source domain has a local that is
//     neither pinned by an equality nor (for d = 1) a
//     floor division of the variables, including the
//     two-sided existential form the guard emitter
//     accepts and a local with no nonzero coefficient
//     (refused).
//
// Presburger note (docs section 19.14): the library set
// difference is unsound in this LLVM build when the
// subtrahend has locals; every decision here uses only
// conjunctive integer emptiness (exactIntegerSetInclusion,
// the collision/schedule analyses, the cut domains below)
// or the local-free box subtraction of the plan.

struct GuardedMemoryOperand {
  // memref-typed SSA value with a static shape and the
  // identity layout.
  mlir::Value memref;

  // Source-space access f(x) = accessRows*x + accessOffsets.
  std::vector<std::vector<BigInt>> accessRows;
  std::vector<BigInt> accessOffsets;

  // The i64 index SSA emitted by emitSourceAffineAccess
  // for exactly these rows and offsets.
  EmittedSourceAccess access;
};

struct EmittedGuardedMemoryLoads {
  // index-typed casts of the i64 access SSA.
  llvm::SmallVector<mlir::Value> indicesA;
  llvm::SmallVector<mlir::Value> indicesB;
  llvm::SmallVector<mlir::Value> indicesC;

  mlir::memref::LoadOp loadA;
  mlir::memref::LoadOp loadB;
  mlir::memref::LoadOp loadC;
};

struct EmittedGuardedMemoryStore {
  mlir::memref::StoreOp store;
};

// Exact image T(D) of a source polyhedron, expressed over
// the logical coordinates z only.
//
// With M = d*T^{-1} (exact rational inverse, d > 0):
//
//   z in T(D)  <=>  x := M*z/d is integral and x in D
//
// Every row of D is scaled by d with x replaced by M*z;
// the locals of D keep their meaning (coefficients scaled
// by d). Integrality adds one local u_i per row i of M
// that is not identically 0 modulo d:
//
//   (M*z)_i - d*u_i == 0
//
// Each added local is pinned by an equality in which it is
// the only local, so exactIntegerSetInclusion can
// eliminate it. A floor-division local of D is no longer
// recognized as such after scaling when d > 1; such a
// domain leaves the inclusion undecided and is refused.
//
// Coefficients must fit i64; otherwise nullopt.
//
// No point enumeration. No approximation.
inline std::optional<mlir::presburger::IntegerPolyhedron>
buildExactLogicalImage(
    const PartitionCoordinateMap &map,
    const mlir::presburger::IntegerPolyhedron &sourceDomain) {

  using namespace mlir::presburger;

  const unsigned n = sourceDomain.getNumDimVars();

  if (n == 0 ||
      sourceDomain.getNumSymbolVars() != 0 ||
      map.sourceToLogical.size() != n ||
      map.logicalToSource.size() != n)
    return std::nullopt;

  AffineIntegerDomain unconstrained;
  unconstrained.sourceDimension = n;

  auto symbolic = transformToSymbolicDomain(map, unconstrained);

  if (!symbolic || symbolic->dimension != n ||
      symbolic->denominator <= 0 ||
      symbolic->inverseNumerator.size() != n)
    return std::nullopt;

  const BigInt &d = symbolic->denominator;
  const IntMatrix &inverse = symbolic->inverseNumerator;

  std::vector<unsigned> congruenceRows;

  for (unsigned i = 0; i < n; ++i) {
    if (inverse[i].size() != n)
      return std::nullopt;

    for (const BigInt &coefficient : inverse[i]) {
      if (coefficient % d != 0) {
        congruenceRows.push_back(i);
        break;
      }
    }
  }

  const unsigned locals = sourceDomain.getNumLocalVars();
  const unsigned added = static_cast<unsigned>(congruenceRows.size());
  const unsigned columns = n + locals + added + 1;

  if (sourceDomain.getNumCols() != n + locals + 1)
    return std::nullopt;

  auto toI64 = [](const BigInt &value)
      -> std::optional<std::int64_t> {
    if (value < BigInt(std::numeric_limits<std::int64_t>::min()) ||
        value > BigInt(std::numeric_limits<std::int64_t>::max()))
      return std::nullopt;

    return value.convert_to<std::int64_t>();
  };

  // Rows of D over [x(n), q(locals), 1] become rows over
  // [z(n), q(locals), u(added), 1].

  auto convertRow = [&](llvm::ArrayRef<llvm::DynamicAPInt> row)
      -> std::optional<llvm::SmallVector<llvm::DynamicAPInt>> {

    if (row.size() != n + locals + 1)
      return std::nullopt;

    std::vector<BigInt> sourceCoefficients(n);

    for (unsigned i = 0; i < n; ++i) {
      auto value = guard_detail::toI64(row[i]);

      if (!value)
        return std::nullopt;

      sourceCoefficients[i] = BigInt(*value);
    }

    llvm::SmallVector<llvm::DynamicAPInt> out(
        columns, llvm::DynamicAPInt(0));

    for (unsigned j = 0; j < n; ++j) {
      BigInt sum = 0;

      for (unsigned i = 0; i < n; ++i)
        sum += sourceCoefficients[i] * inverse[i][j];

      auto value = toI64(sum);

      if (!value)
        return std::nullopt;

      out[j] = llvm::DynamicAPInt(*value);
    }

    for (unsigned t = 0; t < locals; ++t) {
      auto value = guard_detail::toI64(row[n + t]);

      if (!value)
        return std::nullopt;

      auto scaled = toI64(BigInt(*value) * d);

      if (!scaled)
        return std::nullopt;

      out[n + t] = llvm::DynamicAPInt(*scaled);
    }

    auto constant = guard_detail::toI64(row.back());

    if (!constant)
      return std::nullopt;

    auto scaledConstant = toI64(BigInt(*constant) * d);

    if (!scaledConstant)
      return std::nullopt;

    out.back() = llvm::DynamicAPInt(*scaledConstant);

    return out;
  };

  IntegerPolyhedron image(
      PresburgerSpace::getSetSpace(n, 0, locals + added));

  for (unsigned r = 0; r < sourceDomain.getNumInequalities(); ++r) {
    auto row = convertRow(sourceDomain.getInequality(r));

    if (!row)
      return std::nullopt;

    image.addInequality(*row);
  }

  for (unsigned r = 0; r < sourceDomain.getNumEqualities(); ++r) {
    auto row = convertRow(sourceDomain.getEquality(r));

    if (!row)
      return std::nullopt;

    image.addEquality(*row);
  }

  auto denominator = toI64(d);
  auto negatedDenominator = toI64(-d);

  if (!denominator || !negatedDenominator)
    return std::nullopt;

  for (unsigned index = 0; index < added; ++index) {
    const unsigned i = congruenceRows[index];

    llvm::SmallVector<llvm::DynamicAPInt> equality(
        columns, llvm::DynamicAPInt(0));

    for (unsigned j = 0; j < n; ++j) {
      auto value = toI64(inverse[i][j]);

      if (!value)
        return std::nullopt;

      equality[j] = llvm::DynamicAPInt(*value);
    }

    equality[n + locals + index] =
        llvm::DynamicAPInt(*negatedDenominator);

    image.addEquality(equality);
  }

  return image;
}

namespace guarded_memory_detail {

inline bool fitsI64(const BigInt &value) {
  return value >= BigInt(
             std::numeric_limits<std::int64_t>::min()) &&
         value <= BigInt(
             std::numeric_limits<std::int64_t>::max());
}

// Coefficients and offsets are restricted to
// [-INT64_MAX, INT64_MAX] so that every downstream
// analysis (collision relation, schedule check) can negate
// them.
inline std::optional<std::int64_t>
toSymmetricI64(const BigInt &value) {
  if (!fitsI64(value) ||
      value == BigInt(std::numeric_limits<std::int64_t>::min()))
    return std::nullopt;

  return value.convert_to<std::int64_t>();
}

// An i64 arith.constant with the expected value, defined
// in `block`.
inline bool isI64Constant(mlir::Value value,
                          std::int64_t expected,
                          mlir::Block *block) {
  auto constant =
      value.getDefiningOp<mlir::arith::ConstantIntOp>();

  if (!constant || constant->getBlock() != block)
    return false;

  if (!constant.getResult().getType().isInteger(64))
    return false;

  return constant.value() == expected;
}

// Match the left-folded i64 chain emitted by the
// recovery and access emitters:
//
//   acc_0     = constant start
//   p_j       = muli(constant coefficients[j], variables[j])
//   acc_(j+1) = addi(acc_j, p_j)
//   value     = acc_n
//
// Every operation must be defined in `block`.
inline bool matchesLinearChain(
    mlir::Value value,
    std::int64_t start,
    llvm::ArrayRef<std::int64_t> coefficients,
    llvm::ArrayRef<mlir::Value> variables,
    mlir::Block *block) {

  using namespace mlir;

  const std::size_t n = coefficients.size();

  if (variables.size() != n || !value ||
      !value.getType().isInteger(64))
    return false;

  Value current = value;

  for (std::size_t step = n; step > 0; --step) {
    const std::size_t j = step - 1;

    auto add = current.getDefiningOp<arith::AddIOp>();

    if (!add || add->getBlock() != block)
      return false;

    auto product =
        add.getRhs().getDefiningOp<arith::MulIOp>();

    if (!product || product->getBlock() != block)
      return false;

    if (!isI64Constant(product.getLhs(),
                       coefficients[j], block) ||
        product.getRhs() != variables[j])
      return false;

    current = add.getLhs();
  }

  return isI64Constant(current, start, block);
}

// Integer emptiness of
//
//   domain  and  constant + coefficients . x >= 0
//
// over [x(n), locals, constant]. Conjunctive emptiness
// only (sound in this LLVM build). The local columns of
// the cut row are zero.
inline std::optional<bool> cutIsEmpty(
    const mlir::presburger::IntegerPolyhedron &domain,
    llvm::ArrayRef<llvm::DynamicAPInt> coefficients,
    const llvm::DynamicAPInt &constant) {

  const unsigned n = domain.getNumDimVars();

  if (coefficients.size() != n ||
      domain.getNumSymbolVars() != 0 ||
      domain.getNumCols() != n + domain.getNumLocalVars() + 1)
    return std::nullopt;

  llvm::SmallVector<llvm::DynamicAPInt> row(
      domain.getNumCols(), llvm::DynamicAPInt(0));

  for (unsigned j = 0; j < n; ++j)
    row[j] = coefficients[j];

  row.back() = constant;

  mlir::presburger::IntegerPolyhedron cut = domain;

  cut.addInequality(row);

  return cut.isIntegerEmpty();
}

// Prove, on the exact source domain, that the affine
// form g(x) = constant + coefficients . x satisfies
//
//   lower <= g(x) <= upper
//
// at every integer point of the domain. All constants are
// formed in arbitrary precision.
inline bool affineFormWithin(
    const mlir::presburger::IntegerPolyhedron &domain,
    llvm::ArrayRef<llvm::DynamicAPInt> coefficients,
    const llvm::DynamicAPInt &constant,
    const llvm::DynamicAPInt &lower,
    const llvm::DynamicAPInt &upper) {

  const unsigned n = domain.getNumDimVars();

  if (coefficients.size() != n)
    return false;

  llvm::SmallVector<llvm::DynamicAPInt> negated(
      n, llvm::DynamicAPInt(0));

  for (unsigned j = 0; j < n; ++j)
    negated[j] = -coefficients[j];

  // g(x) <= lower - 1  <=>  -g(x) + lower - 1 >= 0
  auto below = cutIsEmpty(
      domain, negated, -constant + lower - llvm::DynamicAPInt(1));

  // g(x) >= upper + 1  <=>  g(x) - upper - 1 >= 0
  auto above = cutIsEmpty(
      domain, coefficients, constant - upper - llvm::DynamicAPInt(1));

  return below && above && *below && *above;
}

// The root buffer of a memref value through view-like
// operations (memref.cast, subview, reinterpret_cast ...).
inline mlir::Value viewRoot(mlir::Value memref) {
  while (memref) {
    auto view = memref.getDefiningOp<mlir::ViewLikeOpInterface>();

    if (!view)
      break;

    memref = view.getViewSource();
  }

  return memref;
}

// Operand validation shared by both emitters:
//
//   - memref type, ranked, static shape, identity layout,
//     rank >= 1;
//   - rank == rows == offsets == emitted indices;
//   - the index SSA is exactly the access chain for
//     these rows over the recovered source IVs;
//   - the chain cannot overflow i64 on the exact domain;
//   - every address lies inside the shape.
// Phase 3b-8: `dynamicLeadingDimension` (the host remainder of a hybrid
// lowering, section 29.2) admits a memref whose leading dimension is
// dynamic: the remainder reads and writes only addresses the source nest
// itself accesses (its plan is a subset of the source image, V3'), so the
// upper bound of that dimension is the program's own guarantee, as for the
// device views (section 27, J4); the lower bound and every static dimension
// are still proven. Default false: the Phase 3b-2 rule, static shapes only.
inline bool validateOperand(
    const GuardedMemoryOperand &operand,
    const mlir::presburger::IntegerPolyhedron &sourceDomain,
    llvm::ArrayRef<mlir::Value> sourceIVs,
    mlir::Block *thenBlock,
    bool dynamicLeadingDimension = false) {

  using namespace mlir;

  const std::size_t n = sourceIVs.size();

  if (!operand.memref)
    return false;

  auto type = dyn_cast<MemRefType>(operand.memref.getType());

  if (!type || !type.hasRank() ||
      !type.getLayout().isIdentity() || type.getRank() < 1)
    return false;

  for (std::int64_t d = 0; d < type.getRank(); ++d)
    if (type.isDynamicDim(d) && !(dynamicLeadingDimension && d == 0))
      return false;

  const std::size_t rank =
      static_cast<std::size_t>(type.getRank());

  if (operand.accessRows.size() != rank ||
      operand.accessOffsets.size() != rank ||
      operand.access.indices.size() != rank)
    return false;

  const llvm::DynamicAPInt minI64(
      std::numeric_limits<std::int64_t>::min());

  const llvm::DynamicAPInt maxI64(
      std::numeric_limits<std::int64_t>::max());

  for (std::size_t r = 0; r < rank; ++r) {

    const auto &row = operand.accessRows[r];

    if (row.size() != n)
      return false;

    auto offset = toSymmetricI64(operand.accessOffsets[r]);

    if (!offset)
      return false;

    std::vector<std::int64_t> coefficients;
    coefficients.reserve(n);

    for (const BigInt &coefficient : row) {
      auto converted = toSymmetricI64(coefficient);

      if (!converted)
        return false;

      coefficients.push_back(*converted);
    }

    // The emitted SSA must be this access function.

    if (!matchesLinearChain(
            operand.access.indices[r], *offset,
            coefficients, sourceIVs, thenBlock))
      return false;

    // i64 safety of the emitted evaluation order on the
    // exact source domain, independent of the `bounds`
    // argument the access emitter was called with: every
    // product and every partial sum is an affine form
    // of x.

    for (std::size_t j = 0; j < n; ++j) {
      llvm::SmallVector<llvm::DynamicAPInt> product(
          n, llvm::DynamicAPInt(0));

      product[j] = llvm::DynamicAPInt(coefficients[j]);

      if (!affineFormWithin(sourceDomain, product,
                            llvm::DynamicAPInt(0),
                            minI64, maxI64))
        return false;
    }

    llvm::SmallVector<llvm::DynamicAPInt> partial(
        n, llvm::DynamicAPInt(0));

    for (std::size_t k = 0; k < n; ++k) {
      partial[k] = llvm::DynamicAPInt(coefficients[k]);

      if (!affineFormWithin(sourceDomain, partial,
                            llvm::DynamicAPInt(*offset),
                            minI64, maxI64))
        return false;
    }

    // Exact tensor bounds: 0 <= f_r(x) <= shape_r - 1 (a dynamic leading
    // dimension: 0 <= f_r(x), the upper bound the program's own).

    if (type.isDynamicDim(static_cast<std::int64_t>(r))) {
      if (!affineFormWithin(sourceDomain, partial,
                            llvm::DynamicAPInt(*offset),
                            llvm::DynamicAPInt(0), maxI64))
        return false;
      continue;
    }

    const std::int64_t extent = type.getShape()[r];

    if (extent < 1)
      return false;

    if (!affineFormWithin(sourceDomain, partial,
                          llvm::DynamicAPInt(*offset),
                          llvm::DynamicAPInt(0),
                          llvm::DynamicAPInt(extent - 1)))
      return false;
  }

  return true;
}

// Structure shared by both emitters. Returns the guarded
// then-block on success.
// Phase 3b-8: `partialDomain` -- the plan's logical domain is a certified
// SUBSET of the image T(D) (the remainder of a hybrid lowering, section
// 29), possibly several disjuncts: V3 then requires every disjunct to lie
// inside T(D) and nothing more; that every source point is executed exactly
// once is the caller's partition proof, not this emitter's. Default false:
// the Phase 3b-2 equality.
inline mlir::Block *validateGuardedStatementContext(
    const PartitionCoordinateMap &map,
    const SymbolicIterationPlan &plan,
    const mlir::presburger::IntegerPolyhedron &sourceDomain,
    const EmittedGuardedAffineLoopNest &guarded,
    const EmittedSourceIVRecovery &recovery,
    bool partialDomain = false) {

  using namespace mlir;

  const std::size_t n = plan.bounds.size();

  // V1: dimensions and representable bounds.

  if (n == 0 ||
      map.sourceToLogical.size() != n ||
      map.logicalToSource.size() != n ||
      guarded.logicalIVs.size() != n ||
      guarded.logicalI64IVs.size() != n ||
      recovery.sourceIVs.size() != n ||
      sourceDomain.getNumDimVars() != n ||
      sourceDomain.getNumSymbolVars() != 0 ||
      !guarded.outermost || !guarded.guardIf ||
      !guarded.predicate)
    return nullptr;

  for (const auto &bound : plan.bounds) {
    if (bound.lower > bound.upper ||
        bound.lower ==
            std::numeric_limits<std::int64_t>::min() ||
        bound.upper ==
            std::numeric_limits<std::int64_t>::max())
      return nullptr;
  }

  affine::AffineForOp outermostLoop = guarded.outermost;

  if (!outermostLoop->getBlock())
    return nullptr;

  for (std::size_t j = 0; j < n; ++j) {
    if (!guarded.logicalIVs[j] ||
        !guarded.logicalIVs[j].getType().isIndex() ||
        !guarded.logicalI64IVs[j] ||
        !guarded.logicalI64IVs[j].getType().isInteger(64))
      return nullptr;
  }

  // V2: the plan still certifies box coverage and an
  // exact, arithmetic-safe guard.

  if (!buildSymbolicIterationPlan(
          plan.logicalDomain, plan.bounds) ||
      !buildExactGuardPlan(plan))
    return nullptr;

  // V3: the logical domain of the plan is exactly the
  // image T(D) of the source domain.
  //
  //   T(D) subset of P: the existential projection of
  //   Phase 5.4.3a on the left (its locals stay
  //   existential);
  //
  //   P subset of T(D): the z-only image, whose added
  //   locals are pinned alone and therefore eliminable.
  //
  // Both inclusions are decided with conjunctive
  // emptiness only.

  auto projected = buildLogicalDomain(map, sourceDomain);
  auto image = buildExactLogicalImage(map, sourceDomain);

  if (!projected || !image || plan.logicalDomain.getNumDisjuncts() == 0)
    return nullptr;

  if (partialDomain) {
    // V3' (Phase 3b-8): every disjunct inside T(D).
    for (unsigned d = 0; d < plan.logicalDomain.getNumDisjuncts(); ++d) {
      auto inside = exactIntegerSetInclusion(
          plan.logicalDomain.getDisjunct(d), *image);
      if (!inside || !*inside)
        return nullptr;
    }
  } else {
    if (plan.logicalDomain.getNumDisjuncts() != 1)
      return nullptr;

    const presburger::IntegerRelation &certified =
        plan.logicalDomain.getDisjunct(0);

    auto imageInPlan =
        exactIntegerSetInclusion(*projected, certified);

    auto planInImage =
        exactIntegerSetInclusion(certified, *image);

    if (!imageInPlan || !planInImage ||
        !*imageInPlan || !*planInImage)
      return nullptr;
  }

  // V4: loop nest structure.

  affine::AffineForOp loop = guarded.outermost;

  for (std::size_t i = 0; i < n; ++i) {

    if (!loop ||
        loop.getInductionVar() != guarded.logicalIVs[i] ||
        !loop.hasConstantBounds() ||
        loop.getConstantLowerBound() !=
            plan.bounds[i].lower ||
        loop.getConstantUpperBound() !=
            plan.bounds[i].upper + 1 ||
        loop.getStepAsInt() != 1 ||
        loop.getNumResults() != 0)
      return nullptr;

    if (i + 1 == n)
      break;

    Value nextIV = guarded.logicalIVs[i + 1];

    Block *nextBody = nextIV.getParentBlock();

    auto next = dyn_cast_or_null<affine::AffineForOp>(
        nextBody ? nextBody->getParentOp() : nullptr);

    if (!next || next->getParentOp() != loop.getOperation())
      return nullptr;

    loop = next;
  }

  scf::IfOp guardIf = guarded.guardIf;

  if (guardIf->getParentOp() != loop.getOperation() ||
      guardIf.getCondition() != guarded.predicate ||
      !guarded.predicate.getType().isInteger(1) ||
      guardIf.getNumResults() != 0 ||
      !guardIf.getElseRegion().empty() ||
      !guardIf.getThenRegion().hasOneBlock())
    return nullptr;

  Block *innerBlock = loop.getBody();

  for (std::size_t j = 0; j < n; ++j) {
    auto cast = guarded.logicalI64IVs[j]
                    .getDefiningOp<arith::IndexCastOp>();

    if (!cast ||
        cast.getIn() != guarded.logicalIVs[j] ||
        !cast.getResult().getType().isInteger(64) ||
        cast->getBlock() != innerBlock ||
        !cast->isBeforeInBlock(guardIf.getOperation()))
      return nullptr;
  }

  Block &thenBlock = guardIf.getThenRegion().front();

  if (thenBlock.empty() || !thenBlock.mightHaveTerminator() ||
      !isa<scf::YieldOp>(thenBlock.back()))
    return nullptr;

  // V5: the recovery SSA is exactly x = M*z/d for the
  // re-certified recovery plan.

  auto recoveryPlan =
      buildSourceIVRecoveryPlan(map, plan.bounds);

  if (!recoveryPlan || recoveryPlan->dimension != n ||
      recoveryPlan->denominator <= 0 ||
      recoveryPlan->inverseNumerator.size() != n)
    return nullptr;

  for (std::size_t i = 0; i < n; ++i) {

    Value recovered = recovery.sourceIVs[i];

    if (!recovered || !recovered.getType().isInteger(64))
      return nullptr;

    Value numerator = recovered;

    if (recoveryPlan->denominator != 1) {
      auto division =
          recovered.getDefiningOp<arith::DivSIOp>();

      if (!division || division->getBlock() != &thenBlock ||
          !isI64Constant(division.getRhs(),
                         recoveryPlan->denominator,
                         &thenBlock))
        return nullptr;

      numerator = division.getLhs();
    }

    if (!matchesLinearChain(
            numerator, 0,
            recoveryPlan->inverseNumerator[i],
            guarded.logicalI64IVs, &thenBlock))
      return nullptr;
  }

  return &thenBlock;
}

// S3: every pair of distinct source iterations that
// touch the same C element must keep its source order
// under the lexicographic z schedule, which is the
// coordinate map itself. Reads and writes of C share one
// access function, so the write-collision relation covers
// every C-carried dependence.
//
// No reduction operator is classified: the order is
// preserved, never reassociated.
inline bool orderPreservedUnderLogicalSchedule(
    const PartitionCoordinateMap &map,
    const mlir::presburger::IntegerPolyhedron &sourceDomain,
    const GuardedMemoryOperand &c,
    std::size_t n) {

  auto conflicts = analyzeOrderedWriteConflicts(
      map, sourceDomain, c.accessRows, c.accessOffsets);

  if (!conflicts)
    return false;

  if (!conflicts->hasOrderedWriteConflict())
    return true;

  auto legality = checkOrderedWriteSchedule(
      *conflicts, map.sourceToLogical, n);

  return legality && legality->preservesOrderedWrites;
}

// V11: every operation of the nest other than the chain
// loops, the guard and `allowed` must be memory-effect
// free.
inline bool nestHasOnlyAllowedEffects(
    const EmittedGuardedAffineLoopNest &guarded,
    const llvm::SmallPtrSetImpl<mlir::Operation *> &allowed) {

  using namespace mlir;

  affine::AffineForOp outermost = guarded.outermost;
  scf::IfOp guardIf = guarded.guardIf;

  bool pure = true;

  outermost->walk([&](Operation *op) {
    if (!pure)
      return;

    if (isa<affine::AffineForOp>(op) ||
        op == guardIf.getOperation() ||
        allowed.count(op))
      return;

    if (!isMemoryEffectFree(op))
      pure = false;
  });

  return pure;
}

inline bool memrefDominatesNest(
    mlir::Value memref,
    const EmittedGuardedAffineLoopNest &guarded) {

  mlir::affine::AffineForOp outermost = guarded.outermost;

  mlir::DominanceInfo dominance;

  return dominance.properlyDominates(
      memref, outermost.getOperation());
}

// V7 and V8, shared by both emitters: one element type
// for A, B and C (signless integer or float), every
// memref dominating the nest, and no provable buffer
// sharing between A/C or B/C (identical values or
// identical view roots). A and B may share a buffer.
inline std::optional<mlir::Type> validateOperandTypes(
    const GuardedMemoryOperand &a,
    const GuardedMemoryOperand &b,
    const GuardedMemoryOperand &c,
    const EmittedGuardedAffineLoopNest &guarded) {

  using namespace mlir;

  Type elementType;

  for (const GuardedMemoryOperand *operand : {&a, &b, &c}) {
    if (!operand->memref)
      return std::nullopt;

    auto type = dyn_cast<MemRefType>(operand->memref.getType());

    if (!type)
      return std::nullopt;

    if (!elementType)
      elementType = type.getElementType();

    if (type.getElementType() != elementType)
      return std::nullopt;

    if (!memrefDominatesNest(operand->memref, guarded))
      return std::nullopt;
  }

  if (!elementType.isSignlessInteger() &&
      !isa<FloatType>(elementType))
    return std::nullopt;

  const Value rootC = viewRoot(c.memref);

  if (viewRoot(a.memref) == rootC ||
      viewRoot(b.memref) == rootC)
    return std::nullopt;

  return elementType;
}

// S1: `load` is the load emitGuardedMemoryLoads emits for
// `operand`: in the then block, reading operand.memref
// with the element type through index casts of exactly
// the operand's i64 access SSA.
inline bool loadMatchesOperand(
    mlir::memref::LoadOp load,
    llvm::ArrayRef<mlir::Value> indices,
    const GuardedMemoryOperand &operand,
    mlir::Type elementType,
    mlir::Block *thenBlock) {

  using namespace mlir;

  if (!load || load->getBlock() != thenBlock ||
      load.getResult().getType() != elementType ||
      load.getMemRef() != operand.memref)
    return false;

  const std::size_t rank = operand.access.indices.size();

  if (indices.size() != rank ||
      load.getIndices().size() != rank)
    return false;

  for (std::size_t r = 0; r < rank; ++r) {
    Value index = indices[r];

    if (!index)
      return false;

    auto cast = index.getDefiningOp<arith::IndexCastOp>();

    if (!cast || cast->getBlock() != thenBlock ||
        !cast.getResult().getType().isIndex() ||
        cast.getIn() != operand.access.indices[r] ||
        load.getIndices()[r] != index)
      return false;
  }

  return true;
}

} // namespace guarded_memory_detail

inline std::optional<EmittedGuardedMemoryLoads>
emitGuardedMemoryLoads(
    mlir::OpBuilder &builder,
    mlir::Location loc,
    const PartitionCoordinateMap &map,
    const SymbolicIterationPlan &plan,
    const mlir::presburger::IntegerPolyhedron &sourceDomain,
    const EmittedGuardedAffineLoopNest &guarded,
    const EmittedSourceIVRecovery &recovery,
    const GuardedMemoryOperand &a,
    const GuardedMemoryOperand &b,
    const GuardedMemoryOperand &c,
    bool partialDomain = false) {

  using namespace mlir;
  namespace detail = guarded_memory_detail;

  // --------------------------------------------------
  // Preflight: no MLIR operation created yet.
  // --------------------------------------------------

  if (!builder.getContext()
           ->getLoadedDialect<memref::MemRefDialect>())
    return std::nullopt;

  Block *thenBlock = detail::validateGuardedStatementContext(
      map, plan, sourceDomain, guarded, recovery, partialDomain);

  if (!thenBlock)
    return std::nullopt;

  // V7 and V8: operand types and provable aliasing.

  if (!detail::validateOperandTypes(a, b, c, guarded))
    return std::nullopt;

  // V6, V9, V10 per operand.

  for (const GuardedMemoryOperand *operand : {&a, &b, &c}) {
    if (!detail::validateOperand(
            *operand, sourceDomain,
            recovery.sourceIVs, thenBlock, partialDomain))
      return std::nullopt;
  }

  // S3: refuse the whole statement before the first load
  // when the store could not follow.

  if (!detail::orderPreservedUnderLogicalSchedule(
          map, sourceDomain, c, plan.bounds.size()))
    return std::nullopt;

  // V11: no memory effect exists in the nest yet.

  llvm::SmallPtrSet<Operation *, 4> allowed;

  if (!detail::nestHasOnlyAllowedEffects(guarded, allowed))
    return std::nullopt;

  // --------------------------------------------------
  // All checks have passed. Emission below has no
  // expected recoverable failure path.
  // --------------------------------------------------

  OpBuilder::InsertionGuard insertionGuard(builder);

  builder.setInsertionPoint(thenBlock->getTerminator());

  EmittedGuardedMemoryLoads result;

  auto emitLoad = [&](const GuardedMemoryOperand &operand,
                      llvm::SmallVector<Value> &indices)
      -> memref::LoadOp {

    for (Value index : operand.access.indices) {
      indices.push_back(arith::IndexCastOp::create(
          builder, loc, builder.getIndexType(), index));
    }

    return memref::LoadOp::create(
        builder, loc, operand.memref, indices);
  };

  result.loadA = emitLoad(a, result.indicesA);
  result.loadB = emitLoad(b, result.indicesB);
  result.loadC = emitLoad(c, result.indicesC);

  return result;
}

inline std::optional<EmittedGuardedMemoryStore>
emitGuardedMemoryStore(
    mlir::OpBuilder &builder,
    mlir::Location loc,
    const PartitionCoordinateMap &map,
    const SymbolicIterationPlan &plan,
    const mlir::presburger::IntegerPolyhedron &sourceDomain,
    const EmittedGuardedAffineLoopNest &guarded,
    const EmittedSourceIVRecovery &recovery,
    const GuardedMemoryOperand &a,
    const GuardedMemoryOperand &b,
    const GuardedMemoryOperand &c,
    const EmittedGuardedMemoryLoads &loads,
    mlir::Value valueToStore,
    bool partialDomain = false) {

  using namespace mlir;
  namespace detail = guarded_memory_detail;

  // --------------------------------------------------
  // Preflight: no MLIR operation created yet. Nothing
  // from the loads call is trusted: every operand is
  // re-validated and bound to its load.
  // --------------------------------------------------

  if (!builder.getContext()
           ->getLoadedDialect<memref::MemRefDialect>())
    return std::nullopt;

  Block *thenBlock = detail::validateGuardedStatementContext(
      map, plan, sourceDomain, guarded, recovery, partialDomain);

  if (!thenBlock)
    return std::nullopt;

  const std::size_t n = plan.bounds.size();

  // V7 and V8.

  auto elementType = detail::validateOperandTypes(a, b, c, guarded);

  if (!elementType)
    return std::nullopt;

  // V6, V9, V10 per operand.

  for (const GuardedMemoryOperand *operand : {&a, &b, &c}) {
    if (!detail::validateOperand(
            *operand, sourceDomain,
            recovery.sourceIVs, thenBlock, partialDomain))
      return std::nullopt;
  }

  // S1: the loads are the ones of this statement.

  memref::LoadOp loadA = loads.loadA;
  memref::LoadOp loadB = loads.loadB;
  memref::LoadOp loadC = loads.loadC;

  if (!detail::loadMatchesOperand(
          loadA, loads.indicesA, a, *elementType, thenBlock) ||
      !detail::loadMatchesOperand(
          loadB, loads.indicesB, b, *elementType, thenBlock) ||
      !detail::loadMatchesOperand(
          loadC, loads.indicesC, c, *elementType, thenBlock))
    return std::nullopt;

  // S2: the stored value.

  if (!valueToStore || valueToStore.getType() != *elementType)
    return std::nullopt;

  {
    DominanceInfo dominance;

    if (!dominance.properlyDominates(
            valueToStore, thenBlock->getTerminator()))
      return std::nullopt;
  }

  // S3: dependence-order preservation (re-checked).

  if (!detail::orderPreservedUnderLogicalSchedule(
          map, sourceDomain, c, n))
    return std::nullopt;

  // V11 / S4: only the three loads may have effects.

  llvm::SmallPtrSet<Operation *, 4> allowed;
  allowed.insert(loadA.getOperation());
  allowed.insert(loadB.getOperation());
  allowed.insert(loadC.getOperation());

  if (!detail::nestHasOnlyAllowedEffects(guarded, allowed))
    return std::nullopt;

  // --------------------------------------------------
  // All checks have passed.
  // --------------------------------------------------

  OpBuilder::InsertionGuard insertionGuard(builder);

  builder.setInsertionPoint(thenBlock->getTerminator());

  EmittedGuardedMemoryStore result;

  result.store = memref::StoreOp::create(
      builder, loc, valueToStore, c.memref, loads.indicesC);

  return result;
}

} // namespace systolic::logical
