#include "Systolic/AffineMatmulAnalysis.h"

#include "mlir/Analysis/Presburger/Matrix.h"
#include "mlir/Dialect/Affine/Analysis/AffineStructures.h"
#include "mlir/Dialect/Arith/IR/Arith.h"

#include "llvm/Support/CheckedArithmetic.h"
#include "llvm/Support/Debug.h"

#include <limits>
#include <numeric>
#include <optional>
#include <utility>

#define DEBUG_TYPE "affine-matmul-analysis"

using namespace mlir;
using namespace mlir::systolic;

/// The recognizer's trace (-debug-only=affine-matmul-analysis). It used to go
/// to llvm::errs() unconditionally, so every pass built on the recognizer
/// printed it.
static llvm::raw_ostream &dbgOut() {
#ifndef NDEBUG
  if (llvm::DebugFlag && llvm::isCurrentDebugType(DEBUG_TYPE))
    return llvm::dbgs();
#endif
  return llvm::nulls();
}

static StringRef classifyLoopIV(Value value,
                                affine::AffineForOp outerLoop,
                                affine::AffineForOp middleLoop,
                                affine::AffineForOp innerLoop) {
  if (value == outerLoop.getInductionVar())
    return "outer";
  if (value == middleLoop.getInductionVar())
    return "middle";
  if (value == innerLoop.getInductionVar())
    return "inner";
  return "other";
}

static SmallVector<Value>
collectAccessIterationIVs(affine::AffineLoadOp lhsLoad,
                          affine::AffineLoadOp rhsLoad,
                          affine::AffineLoadOp accLoad,
                          affine::AffineStoreOp store) {
  SmallVector<Value> usedIVs;

  auto collectUsedIVs = [&](ValueRange indices) {
    for (Value value : indices) {
      if (llvm::find(usedIVs, value) == usedIVs.end())
        usedIVs.push_back(value);
    }
  };

  collectUsedIVs(lhsLoad.getIndices());
  collectUsedIVs(rhsLoad.getIndices());
  collectUsedIVs(accLoad.getIndices());
  collectUsedIVs(store.getIndices());

  // Recover the enclosing affine.for chain around the MAC, initially from
  // inner to outer.
  SmallVector<affine::AffineForOp> enclosingLoops;
  for (Operation *op = store.getOperation()->getParentOp();
       op != nullptr;
       op = op->getParentOp()) {
    if (auto loop = dyn_cast<affine::AffineForOp>(op))
      enclosingLoops.push_back(loop);
  }

  // Canonicalize the iteration domain to lexical outer-to-inner order.
  std::reverse(enclosingLoops.begin(), enclosingLoops.end());

  SmallVector<Value> iterationIVs;
  for (affine::AffineForOp loop : enclosingLoops) {
    Value iv = loop.getInductionVar();
    if (llvm::find(usedIVs, iv) != usedIVs.end())
      iterationIVs.push_back(iv);
  }

  return iterationIVs;
}

static FailureOr<affine::FlatAffineValueConstraints>
recoverSourceDomain(ValueRange iterationIVs) {
  SmallVector<Value> iterationValues(iterationIVs.begin(),
                                     iterationIVs.end());

  affine::FlatAffineValueConstraints domain(
      /*numDims=*/iterationValues.size(),
      /*numSymbols=*/0,
      /*numLocals=*/0,
      iterationValues);

  for (Value iv : iterationValues) {
    affine::AffineForOp loop = affine::getForInductionVarOwner(iv);
    if (!loop)
      return failure();

    if (failed(domain.addAffineForOpDomain(loop)))
      return failure();
  }

  return domain;
}

static FailureOr<AffineMap>
normalizeAccessMap(AffineMap map,
                   ValueRange operands,
                   ValueRange iterationIVs) {
  MLIRContext *context = map.getContext();

  SmallVector<AffineExpr> dimReplacements;
  dimReplacements.reserve(operands.size());

  // Rewrite each map-local dimension into a common iteration domain:
  //
  //   iterationIVs[p] -> common dp
  //
  // The common domain may have any number of access-related loop IVs.
  for (Value operand : operands) {
    auto it = llvm::find(iterationIVs, operand);
    if (it == iterationIVs.end())
      return failure();

    unsigned position =
        static_cast<unsigned>(std::distance(iterationIVs.begin(), it));
    dimReplacements.push_back(getAffineDimExpr(position, context));
  }

  // replaceDimsAndSymbols() requires one replacement for every map dim.
  if (dimReplacements.size() != map.getNumDims())
    return failure();

  // First checkpoint: handle dim-only affine accesses. Symbol support can
  // be added later without complicating the initial normalization logic.
  if (map.getNumSymbols() != 0)
    return failure();

  return map.replaceDimsAndSymbols(
      dimReplacements,
      /*symReplacements=*/{},
      /*numResultDims=*/iterationIVs.size(),
      /*numResultSyms=*/0);
}

struct LinearForm {
  SmallVector<int64_t> coefficients;
  int64_t constant = 0;
};

static FailureOr<LinearForm>
extractLinearForm(AffineExpr expr, unsigned numDims) {
  switch (expr.getKind()) {
  case AffineExprKind::DimId: {
    auto dim = cast<AffineDimExpr>(expr);
    if (dim.getPosition() >= numDims)
      return failure();

    LinearForm result;
    result.coefficients.assign(numDims, 0);
    result.coefficients[dim.getPosition()] = 1;
    return result;
  }

  case AffineExprKind::Constant: {
    LinearForm result;
    result.coefficients.assign(numDims, 0);
    result.constant = cast<AffineConstantExpr>(expr).getValue();
    return result;
  }

  case AffineExprKind::Add: {
    auto binary = cast<AffineBinaryOpExpr>(expr);

    auto lhs = extractLinearForm(binary.getLHS(), numDims);
    auto rhs = extractLinearForm(binary.getRHS(), numDims);
    if (failed(lhs) || failed(rhs))
      return failure();

    // A coefficient or constant that does not fit in 64 bits has no linear
    // form we can compute: refused, never wrapped around.
    LinearForm result;
    result.coefficients.assign(numDims, 0);
    for (unsigned i = 0; i < numDims; ++i) {
      std::optional<int64_t> sum =
          llvm::checkedAdd(lhs->coefficients[i], rhs->coefficients[i]);
      if (!sum)
        return failure();
      result.coefficients[i] = *sum;
    }
    std::optional<int64_t> constant =
        llvm::checkedAdd(lhs->constant, rhs->constant);
    if (!constant)
      return failure();
    result.constant = *constant;
    return result;
  }

  case AffineExprKind::Mul: {
    auto binary = cast<AffineBinaryOpExpr>(expr);

    auto scale =
        dyn_cast<AffineConstantExpr>(binary.getRHS());
    if (!scale)
      return failure();

    auto lhs = extractLinearForm(binary.getLHS(), numDims);
    if (failed(lhs))
      return failure();

    LinearForm result;
    result.coefficients.assign(numDims, 0);
    int64_t factor = scale.getValue();

    for (unsigned i = 0; i < numDims; ++i) {
      std::optional<int64_t> product =
          llvm::checkedMul(lhs->coefficients[i], factor);
      if (!product)
        return failure();
      result.coefficients[i] = *product;
    }
    std::optional<int64_t> constant = llvm::checkedMul(lhs->constant, factor);
    if (!constant)
      return failure();
    result.constant = *constant;
    return result;
  }

  case AffineExprKind::SymbolId:
  case AffineExprKind::Mod:
  case AffineExprKind::FloorDiv:
  case AffineExprKind::CeilDiv:
    return failure();
  }

  return failure();
}

/// The two operands' expressions for one shared logical coordinate, matched:
/// the coordinate's own expression and each operand's view of it.
struct SharedCoordinate {
  AffineExpr expr;
  OperandCoordinateView first;
  OperandCoordinateView second;
};

/// Whether the expressions `a` and `b` (over the same iteration dims) address
/// the same logical coordinate, and how.
///
/// They do when they are identical -- then the coordinate is that expression
/// and both views are the identity, as before -- or when both are affine
/// images s * g + o (integer s > 0) of one linear form g over the loop IVs:
/// a constant offset or a stride on one operand only, as in C[i + 8][j] with
/// A[i][k] or C[2i][j] with A[i][k], which are dense GEMMs of the operands'
/// own views.  Then the coordinate is the primitive form g (the coefficients
/// divided by their gcd, no constant) and each operand's view is its (s, o).
/// Expressions over different combinations of the loops (4 i0 + i1 against
/// 8 i0 + i1), of opposite direction (i against 7 - i), or without any loop
/// are not one coordinate.
static std::optional<SharedCoordinate>
matchSharedCoordinate(AffineExpr a, AffineExpr b, unsigned numDims) {
  if (a == b)
    return SharedCoordinate{a, {}, {}};

  auto formA = extractLinearForm(a, numDims);
  auto formB = extractLinearForm(b, numDims);
  if (failed(formA) || failed(formB))
    return std::nullopt;

  auto content = [&](const LinearForm &form) -> int64_t {
    int64_t g = 0;
    for (int64_t coefficient : form.coefficients) {
      // INT64_MIN has no positive absolute value; such a subscript cannot
      // be a coordinate anyone enumerates.
      if (coefficient == std::numeric_limits<int64_t>::min())
        return 0;
      g = std::gcd(g, coefficient < 0 ? -coefficient : coefficient);
    }
    return g;
  };
  const int64_t scaleA = content(*formA);
  const int64_t scaleB = content(*formB);
  // A subscript without a loop (a constant) is not a coordinate.
  if (scaleA == 0 || scaleB == 0)
    return std::nullopt;

  MLIRContext *context = a.getContext();
  AffineExpr shared;
  for (unsigned d = 0; d < numDims; ++d) {
    const int64_t primitive = formA->coefficients[d] / scaleA;
    if (primitive != formB->coefficients[d] / scaleB)
      return std::nullopt;
    if (primitive == 0)
      continue;
    AffineExpr term = getAffineDimExpr(d, context);
    if (primitive != 1)
      term = term * primitive;
    shared = shared ? shared + term : term;
  }

  SharedCoordinate result;
  result.expr = shared;
  result.first = {scaleA, formA->constant};
  result.second = {scaleB, formB->constant};
  return result;
}

/// The number of iterations of `for (x = lower; x < upper; x += step)`, step
/// > 0 and upper > lower, or nullopt when upper - lower does not fit in 64
/// bits. ceil(d / s) is computed as (d - 1) / s + 1, which cannot overflow
/// for d >= 1, unlike (d + s - 1) / s.
static std::optional<int64_t> checkedTripCount(int64_t lower, int64_t upper,
                                               int64_t step) {
  std::optional<int64_t> distance = llvm::checkedSub(upper, lower);
  if (!distance || *distance <= 0 || step <= 0)
    return std::nullopt;
  return (*distance - 1) / step + 1;
}

static FailureOr<int64_t>
recoverStaticIterationOrigin(Value iv) {
  auto blockArg = dyn_cast<BlockArgument>(iv);
  if (!blockArg)
    return failure();

  auto loop =
      dyn_cast<affine::AffineForOp>(blockArg.getOwner()->getParentOp());
  if (!loop || loop.getInductionVar() != iv)
    return failure();

  if (!loop.hasConstantLowerBound())
    return failure();

  return loop.getConstantLowerBound();
}

/// An exact rational number in 64-bit arithmetic. A result that does not fit
/// (an intermediate product or sum included) is not wrapped around: it is
/// marked invalid, every operation on an invalid value is invalid, and the
/// computations below fail when one appears.
struct Rational {
  int64_t numerator = 0;
  int64_t denominator = 1;
  bool valid = true;

  Rational() = default;

  Rational(int64_t numerator, int64_t denominator = 1)
      : numerator(numerator), denominator(denominator) {
    normalize();
  }

  static Rational invalid() {
    Rational result;
    result.valid = false;
    return result;
  }

  bool isZero() const {
    return valid && numerator == 0;
  }

  Rational operator+(const Rational &other) const {
    return combine(other, /*subtract=*/false);
  }

  Rational operator-(const Rational &other) const {
    return combine(other, /*subtract=*/true);
  }

  Rational operator*(const Rational &other) const {
    if (!valid || !other.valid)
      return invalid();
    return make(llvm::checkedMul(numerator, other.numerator),
                llvm::checkedMul(denominator, other.denominator));
  }

  Rational operator/(const Rational &other) const {
    if (!valid || !other.valid)
      return invalid();
    assert(!other.isZero() && "division by zero rational");
    return make(llvm::checkedMul(numerator, other.denominator),
                llvm::checkedMul(denominator, other.numerator));
  }

private:
  static Rational make(std::optional<int64_t> numerator,
                       std::optional<int64_t> denominator) {
    if (!numerator || !denominator)
      return invalid();
    return Rational(*numerator, *denominator);
  }

  Rational combine(const Rational &other, bool subtract) const {
    if (!valid || !other.valid)
      return invalid();
    std::optional<int64_t> left = llvm::checkedMul(numerator, other.denominator);
    std::optional<int64_t> right =
        llvm::checkedMul(other.numerator, denominator);
    if (!left || !right)
      return invalid();
    return make(subtract ? llvm::checkedSub(*left, *right)
                         : llvm::checkedAdd(*left, *right),
                llvm::checkedMul(denominator, other.denominator));
  }

  void normalize() {
    assert(denominator != 0 && "rational denominator must be nonzero");

    // INT64_MIN has no negation (and no absolute value for the gcd).
    if (numerator == std::numeric_limits<int64_t>::min() ||
        denominator == std::numeric_limits<int64_t>::min()) {
      *this = invalid();
      return;
    }

    if (denominator < 0) {
      numerator = -numerator;
      denominator = -denominator;
    }

    int64_t divisor = std::gcd(numerator, denominator);
    numerator /= divisor;
    denominator /= divisor;
  }
};

static FailureOr<SmallVector<SmallVector<Rational>>>
invertSquareMapping(
    const SmallVector<SmallVector<int64_t>> &transformation) {
  const unsigned size = transformation.size();
  if (size == 0)
    return failure();

  for (const auto &row : transformation) {
    if (row.size() != size)
      return failure();
  }

  // Gauss-Jordan elimination on [T | I], performed exactly over Q.
  SmallVector<SmallVector<Rational>> augmented(
      size, SmallVector<Rational>(2 * size));

  for (unsigned row = 0; row < size; ++row) {
    for (unsigned column = 0; column < size; ++column) {
      augmented[row][column] =
          Rational(transformation[row][column]);
      if (!augmented[row][column].valid)
        return failure();
      augmented[row][size + column] =
          Rational(row == column ? 1 : 0);
    }
  }

  for (unsigned column = 0; column < size; ++column) {
    unsigned pivotRow = column;
    while (pivotRow < size &&
           augmented[pivotRow][column].isZero())
      ++pivotRow;

    if (pivotRow == size)
      return failure();

    if (pivotRow != column)
      std::swap(augmented[pivotRow], augmented[column]);

    Rational pivot = augmented[column][column];

    for (unsigned j = 0; j < 2 * size; ++j) {
      augmented[column][j] =
          augmented[column][j] / pivot;
      if (!augmented[column][j].valid)
        return failure();
    }

    for (unsigned row = 0; row < size; ++row) {
      if (row == column)
        continue;

      Rational factor = augmented[row][column];
      if (factor.isZero())
        continue;

      for (unsigned j = 0; j < 2 * size; ++j) {
        augmented[row][j] =
            augmented[row][j] -
            factor * augmented[column][j];
        if (!augmented[row][j].valid)
          return failure();
      }
    }
  }

  SmallVector<SmallVector<Rational>> inverse(
      size, SmallVector<Rational>(size));

  for (unsigned row = 0; row < size; ++row) {
    for (unsigned column = 0; column < size; ++column)
      inverse[row][column] =
          augmented[row][size + column];
  }

  return inverse;
}

static FailureOr<SmallVector<SmallVector<Rational>>>
transformDomainToLogical(
    const affine::FlatAffineValueConstraints &sourceDomain,
    const SmallVector<SmallVector<Rational>> &inverse,
    ArrayRef<int64_t> offset) {
  const unsigned numSourceDims = inverse.size();
  if (numSourceDims == 0)
    return failure();

  const unsigned numLogicalDims = inverse.front().size();

  if (sourceDomain.getNumDimVars() != numSourceDims)
    return failure();

  if (offset.size() != numLogicalDims)
    return failure();

  for (const auto &row : inverse) {
    if (row.size() != numLogicalDims)
      return failure();
  }

  SmallVector<SmallVector<Rational>> logicalInequalities;

  for (unsigned i = 0; i < sourceDomain.getNumInequalities(); ++i) {
    auto source = sourceDomain.getInequality64(i);

    // One coefficient per source dimension, followed by the constant.
    if (source.size() != numSourceDims + 1)
      return failure();

    SmallVector<Rational> logical(numLogicalDims + 1);

    // a^T T^-1
    for (unsigned logicalDim = 0;
         logicalDim < numLogicalDims;
         ++logicalDim) {
      Rational coefficient;

      for (unsigned sourceDim = 0;
           sourceDim < numSourceDims;
           ++sourceDim) {
        coefficient =
            coefficient +
            Rational(source[sourceDim]) *
                inverse[sourceDim][logicalDim];
      }

      if (!coefficient.valid)
        return failure();
      logical[logicalDim] = coefficient;
    }

    // b - (a^T T^-1) c
    Rational constant(source.back());
    for (unsigned logicalDim = 0;
         logicalDim < numLogicalDims;
         ++logicalDim) {
      constant =
          constant -
          logical[logicalDim] * Rational(offset[logicalDim]);
    }

    if (!constant.valid)
      return failure();
    logical[numLogicalDims] = constant;
    logicalInequalities.push_back(std::move(logical));
  }

  return logicalInequalities;
}

static FailureOr<presburger::IntegerPolyhedron>
buildExactLogicalDomain(
    const affine::FlatAffineValueConstraints &sourceDomain,
    const SmallVector<SmallVector<int64_t>> &transformation,
    ArrayRef<int64_t> offset) {
  const unsigned numSourceDims = sourceDomain.getNumDimVars();
  const unsigned numSourceSymbols = sourceDomain.getNumSymbolVars();
  const unsigned numSourceLocals = sourceDomain.getNumLocalVars();
  const unsigned numLogicalDims = transformation.size();

  if (numSourceDims == 0 || numLogicalDims == 0)
    return failure();

  // Symbolic source bounds require carrying source symbols into the logical
  // Presburger space.  Keep that case explicit until its representation is
  // implemented.
  if (numSourceSymbols != 0)
    return failure();

  if (offset.size() != numLogicalDims)
    return failure();

  for (const auto &row : transformation) {
    if (row.size() != numSourceDims)
      return failure();
  }

  // Logical coordinates are set dimensions.  Source iteration dimensions and
  // any source-domain locals are existential integer locals.
  //
  // Source layout:
  //
  //   [source dims x | source locals q | constant]
  //
  // Logical-domain layout:
  //
  //   [logical dims l | source dims x | source locals q | constant]
  //
  // This represents:
  //
  //   { l | exists x, q : (x, q) in D_S and l = T x + c }.
  const unsigned numLogicalLocals =
      numSourceDims + numSourceLocals;

  auto space = presburger::PresburgerSpace::getSetSpace(
      /*numDims=*/numLogicalDims,
      /*numSymbols=*/0,
      /*numLocals=*/numLogicalLocals);

  presburger::IntegerPolyhedron logicalDomain(space);

  const unsigned expectedSourceCols =
      numSourceDims + numSourceLocals + 1;

  auto copySourceConstraint =
      [&](ArrayRef<int64_t> source, bool equality) -> LogicalResult {
    if (source.size() != expectedSourceCols)
      return failure();

    SmallVector<int64_t> constraint(
        logicalDomain.getNumCols(), 0);

    // Copy source dimensions into the first existential-local block.
    for (unsigned sourceDim = 0;
         sourceDim < numSourceDims;
         ++sourceDim) {
      constraint[numLogicalDims + sourceDim] =
          source[sourceDim];
    }

    // Preserve source-domain locals, including divisibility/stride witnesses.
    for (unsigned sourceLocal = 0;
         sourceLocal < numSourceLocals;
         ++sourceLocal) {
      constraint[numLogicalDims + numSourceDims + sourceLocal] =
          source[numSourceDims + sourceLocal];
    }

    constraint.back() = source.back();

    if (equality)
      logicalDomain.addEquality(constraint);
    else
      logicalDomain.addInequality(constraint);

    return success();
  };

  for (unsigned i = 0;
       i < sourceDomain.getNumInequalities();
       ++i) {
    if (failed(copySourceConstraint(
            sourceDomain.getInequality64(i),
            /*equality=*/false)))
      return failure();
  }

  for (unsigned i = 0;
       i < sourceDomain.getNumEqualities();
       ++i) {
    if (failed(copySourceConstraint(
            sourceDomain.getEquality64(i),
            /*equality=*/true)))
      return failure();
  }

  // Add l = T x + c as one equality per logical dimension:
  //
  //   l_r - sum_j T[r][j] x_j - c_r = 0.
  //
  // Existing source-domain locals q do not participate in this mapping.
  for (unsigned logicalDim = 0;
       logicalDim < numLogicalDims;
       ++logicalDim) {
    SmallVector<int64_t> equality(
        logicalDomain.getNumCols(), 0);

    equality[logicalDim] = 1;

    // -INT64_MIN does not fit: no exact domain then (it is optional; the
    // nest is not described with a wrapped-around coefficient).
    for (unsigned sourceDim = 0;
         sourceDim < numSourceDims;
         ++sourceDim) {
      std::optional<int64_t> negated =
          llvm::checkedSub(int64_t(0), transformation[logicalDim][sourceDim]);
      if (!negated)
        return failure();
      equality[numLogicalDims + sourceDim] = *negated;
    }

    std::optional<int64_t> negatedOffset =
        llvm::checkedSub(int64_t(0), offset[logicalDim]);
    if (!negatedOffset)
      return failure();
    equality.back() = *negatedOffset;
    logicalDomain.addEquality(equality);
  }

  return logicalDomain;
}

/// Whether T is square and of full rank. The determinant is computed in
/// arbitrary precision (presburger::IntMatrix, DynamicAPInt), so no
/// intermediate product can overflow and the answer is exact for any 64-bit
/// coefficients.
static bool
isInvertibleSquareMapping(
    const SmallVector<SmallVector<int64_t>> &transformation) {
  const unsigned size = transformation.size();
  if (size == 0)
    return false;

  for (const auto &row : transformation) {
    if (row.size() != size)
      return false;
  }

  presburger::IntMatrix matrix(size, size);
  for (unsigned row = 0; row < size; ++row)
    for (unsigned column = 0; column < size; ++column)
      matrix(row, column) = DynamicAPInt(transformation[row][column]);
  return matrix.determinant() != 0;
}

static LogicalExtent
recoverLogicalExtent(AffineExpr expr,
                     ValueRange iterationIVs) {
  LogicalExtent result;

  // First checkpoint: recover an extent when the logical coordinate depends
  // on exactly one source iteration dimension.  The affine expression may be
  // composed, e.g. d0 * 2 + 7, because the extent is the number of source
  // iterations rather than the numerical span of the logical coordinate.
  //
  // Multi-IV expressions such as d0 + d1 remain unsupported for now because
  // their extent cannot be recovered from a single source loop directly.

  SmallVector<unsigned> usedDims;
  expr.walk([&](AffineExpr subExpr) {
    if (auto dimExpr = dyn_cast<AffineDimExpr>(subExpr))
      usedDims.push_back(dimExpr.getPosition());
  });

  llvm::sort(usedDims);
  usedDims.erase(std::unique(usedDims.begin(), usedDims.end()),
                 usedDims.end());

  if (usedDims.size() != 1)
    return result;

  unsigned position = usedDims.front();
  if (position >= iterationIVs.size())
    return result;

  Value iv = iterationIVs[position];

  auto blockArg = dyn_cast<BlockArgument>(iv);
  if (!blockArg)
    return result;

  auto loop =
      dyn_cast<affine::AffineForOp>(blockArg.getOwner()->getParentOp());
  if (!loop || loop.getInductionVar() != iv)
    return result;

  // Static affine.for trip count.
  if (loop.hasConstantLowerBound() &&
      loop.hasConstantUpperBound()) {
    int64_t lower = loop.getConstantLowerBound();
    int64_t upper = loop.getConstantUpperBound();
    int64_t step = loop.getStep().getSExtValue();

    if (step <= 0 || upper <= lower)
      return result;

    std::optional<int64_t> trips = checkedTripCount(lower, upper, step);
    if (!trips)
      return result;

    result.kind = LogicalExtent::Kind::Static;
    result.staticValue = *trips;
    return result;
  }

  // First symbolic checkpoint:
  //
  //   affine.for %iv = 0 to %n
  //
  // is represented as:
  //
  //   LB: () -> (0)
  //   UB: ()[s0] -> (s0), operand = %n
  //
  // Keep this deliberately narrow.  More general affine bounds can be
  // supported later without weakening GEMM recognition.
  if (loop.getStep() != 1)
    return result;

  if (!loop.hasConstantLowerBound() ||
      loop.getConstantLowerBound() != 0)
    return result;

  AffineMap upperMap = loop.getUpperBoundMap();
  auto upperOperands = loop.getUpperBoundOperands();

  if (upperMap.getNumDims() != 0 ||
      upperMap.getNumSymbols() != 1 ||
      upperMap.getNumResults() != 1 ||
      upperOperands.size() != 1)
    return result;

  auto symbolExpr =
      dyn_cast<AffineSymbolExpr>(upperMap.getResult(0));
  if (!symbolExpr || symbolExpr.getPosition() != 0)
    return result;

  result.kind = LogicalExtent::Kind::Symbolic;
  result.symbolicValue = *upperOperands.begin();
  return result;
}

void mlir::systolic::collectAffineMatmulCandidates(
    Operation *root,
    SmallVectorImpl<AffineMatmulCandidate> &candidates) {

  if (!root)
    return;

  root->walk([&](affine::AffineForOp outerLoop) {
    // Expect:
    //
    //   affine.for %i
    //     affine.for %j
    //       affine.for %k
    //
    // We deliberately keep discovery conservative here.

    affine::AffineForOp middleLoop;
    affine::AffineForOp innerLoop;

    // Find the unique affine.for directly nested in outerLoop.
    for (Operation &op : *outerLoop.getBody()) {
      if (auto loop = dyn_cast<affine::AffineForOp>(&op)) {
        if (middleLoop)
          return; // More than one candidate child loop.
        middleLoop = loop;
      }
    }

    if (!middleLoop)
      return;

    // Find the unique affine.for directly nested in middleLoop.
    for (Operation &op : *middleLoop.getBody()) {
      if (auto loop = dyn_cast<affine::AffineForOp>(&op)) {
        if (innerLoop)
          return;
        innerLoop = loop;
      }
    }

    if (!innerLoop)
      return;

    AffineMatmulCandidate candidate;
    candidate.anchor = outerLoop.getOperation();

    dbgOut() << "Found affine matmul candidate:\n";
    dbgOut() << "  outer IV: " << outerLoop.getInductionVar() << "\n";
    dbgOut() << "  middle IV: " << middleLoop.getInductionVar() << "\n";
    dbgOut() << "  inner IV: " << innerLoop.getInductionVar() << "\n";

    candidates.push_back(candidate);
  });
}


/// `overflowed` is set when the normalization fails because a checked
/// product, sum or trip count does not fit in 64 bits -- and only then;
/// `emptyLoop` when it fails because a loop with constant bounds has no
/// iteration. Every other failure (a bound that is not one constant, ...)
/// leaves both false.
static FailureOr<AffineLogicalNormalization>
normalizeLogicalCoordinates(const LogicalMatmul &matmul,
                            ValueRange iterationIVs, bool &overflowed,
                            bool &emptyLoop) {
  overflowed = false;
  emptyLoop = false;
  if (!matmul.sourceDomain)
    return failure();

  const auto &sourceDomain = *matmul.sourceDomain;
  const unsigned numSourceDims = sourceDomain.getNumDimVars();

  if (numSourceDims == 0 ||
      iterationIVs.size() != numSourceDims)
    return failure();

  if (!matmul.logicalMapping.hasStaticSourceOrigin ||
      matmul.logicalMapping.sourceOrigin.size() != numSourceDims)
    return failure();

  const auto &T = matmul.logicalMapping.transformation;
  const auto &c = matmul.logicalMapping.offset;

  const unsigned numLogicalDims = T.size();

  if (numLogicalDims == 0 ||
      c.size() != numLogicalDims)
    return failure();

  for (const auto &row : T) {
    if (row.size() != numSourceDims)
      return failure();
  }

  AffineLogicalNormalization normalization;

  // Normalize source loop coordinates:
  //
  //   x_i = s_i * z_i + origin_i
  //
  // where s_i is the affine.for step.

  normalization.sourceTransformation.assign(
      numSourceDims,
      SmallVector<int64_t>(numSourceDims, 0));

  normalization.sourceOffset =
      matmul.logicalMapping.sourceOrigin;

  for (unsigned i = 0; i < numSourceDims; ++i) {
    auto blockArg =
        dyn_cast<BlockArgument>(iterationIVs[i]);

    if (!blockArg)
      return failure();

    auto loop =
        dyn_cast<affine::AffineForOp>(
            blockArg.getOwner()->getParentOp());

    if (!loop)
      return failure();

    const int64_t step = loop.getStep().getSExtValue();

    if (step <= 0)
      return failure();

    normalization.sourceTransformation[i][i] = step;
  }

  // Compose:
  //
  //   x = S z + origin
  //   y = T x + c
  //
  // therefore:
  //
  //   y = (T S) z + (T origin + c).

  normalization.logicalTransformation.assign(
      numLogicalDims,
      SmallVector<int64_t>(numSourceDims, 0));

  normalization.logicalOffset.assign(
      numLogicalDims, 0);

  // Coefficients, steps and origins are whatever the program says; a product
  // or sum that does not fit in 64 bits means the nest has no normalized
  // form we can compute (it is refused downstream), not a wrapped-around one.
  for (unsigned logicalDim = 0;
       logicalDim < numLogicalDims;
       ++logicalDim) {
    for (unsigned sourceDim = 0;
         sourceDim < numSourceDims;
         ++sourceDim) {
      std::optional<int64_t> scaled = llvm::checkedMul(
          T[logicalDim][sourceDim],
          normalization.sourceTransformation[sourceDim][sourceDim]);
      if (!scaled) {
        overflowed = true;
        return failure();
      }
      normalization.logicalTransformation[logicalDim][sourceDim] = *scaled;
    }

    int64_t composedOffset = c[logicalDim];

    for (unsigned sourceDim = 0;
         sourceDim < numSourceDims;
         ++sourceDim) {
      std::optional<int64_t> term = llvm::checkedMul(
          T[logicalDim][sourceDim], normalization.sourceOffset[sourceDim]);
      std::optional<int64_t> sum =
          term ? llvm::checkedAdd(composedOffset, *term) : std::nullopt;
      if (!sum) {
        overflowed = true;
        return failure();
      }
      composedOffset = *sum;
    }

    normalization.logicalOffset[logicalDim] =
        composedOffset;
  }

  // Keep the exact source domain attached for now.
  // The source-domain constraints are still expressed in x-coordinates;
  // the affine composition above is the canonical representation used
  // by downstream logical-coordinate consumers.

  // Canonical normalized execution domain.
  //
  // z_i is the iteration ordinal of the corresponding affine.for:
  //
  //   x_i = lower_i + step_i * z_i
  //
  // For static bounds:
  //
  //   0 <= z_i < ceil((upper_i - lower_i) / step_i)
  //
  // This intentionally constructs the normalized domain directly rather
  // than carrying the original x-coordinates through Presburger
  // substitution.  The result therefore contains only z dimensions,
  // with no auxiliary local variables.

  affine::FlatAffineValueConstraints normalizedDomain(
      numSourceDims, /*numSymbolVars=*/0, /*numLocalVars=*/0);

  for (unsigned i = 0; i < numSourceDims; ++i) {
    auto blockArg = dyn_cast<BlockArgument>(iterationIVs[i]);
    if (!blockArg)
      return failure();

    auto loop = dyn_cast<affine::AffineForOp>(
        blockArg.getOwner()->getParentOp());
    if (!loop)
      return failure();

    // A symbolic bound has no static normalized extent. (Calling
    // getConstantUpperBound() on one is undefined, and went unnoticed in
    // builds without assertions.)
    if (!loop.hasConstantLowerBound() || !loop.hasConstantUpperBound())
      return failure();

    const int64_t lower =
        loop.getConstantLowerBound();
    const int64_t upper =
        loop.getConstantUpperBound();

    const int64_t step =
        loop.getStep().getSExtValue();

    if (step <= 0)
      return failure();
    if (upper <= lower) {
      emptyLoop = true;
      return failure();
    }

    // The trip count, without forming upper - lower or the rounding sum in
    // wrapped-around arithmetic.
    // (step > 0 and upper > lower here, so a missing trip count means
    // upper - lower does not fit.)
    std::optional<int64_t> trips = checkedTripCount(lower, upper, step);
    if (!trips) {
      overflowed = true;
      return failure();
    }
    const int64_t extent = *trips;

    // z_i >= 0
    SmallVector<int64_t> lowerConstraint(
        numSourceDims + 1, 0);
    lowerConstraint[i] = 1;
    normalizedDomain.addInequality(lowerConstraint);

    // z_i <= extent - 1
    SmallVector<int64_t> upperConstraint(
        numSourceDims + 1, 0);
    upperConstraint[i] = -1;
    upperConstraint.back() = extent - 1;
    normalizedDomain.addInequality(upperConstraint);
  }

  normalization.domain = std::move(normalizedDomain);

  dbgOut() << "  normalized execution domain:\n";
  normalization.domain->print(dbgOut());

  normalization.isIdentity = true;

  for (unsigned row = 0;
       row < numLogicalDims;
       ++row) {
    for (unsigned column = 0;
         column < numSourceDims;
         ++column) {
      const int64_t expected =
          (row == column) ? 1 : 0;

      if (normalization.logicalTransformation[row][column] !=
          expected)
        normalization.isIdentity = false;
    }

    if (normalization.logicalOffset[row] != 0)
      normalization.isIdentity = false;
  }

  dbgOut() << "  normalized logical coordinates:\n";

  for (unsigned logicalDim = 0;
       logicalDim < numLogicalDims;
       ++logicalDim) {
    dbgOut() << "    y" << logicalDim << " = ";

    bool printed = false;

    for (unsigned sourceDim = 0;
         sourceDim < numSourceDims;
         ++sourceDim) {
      const int64_t coefficient =
          normalization.logicalTransformation
              [logicalDim][sourceDim];

      if (coefficient == 0)
        continue;

      if (printed)
        dbgOut() << " + ";

      dbgOut() << coefficient
                   << "*z" << sourceDim;

      printed = true;
    }

    if (normalization.logicalOffset[logicalDim] != 0 ||
        !printed) {
      if (printed)
        dbgOut() << " + ";

      dbgOut()
          << normalization.logicalOffset[logicalDim];
    }

    dbgOut() << "\n";
  }

  return normalization;
}

FailureOr<LogicalMatmul>
mlir::systolic::recognizeLogicalMatmul(
    AffineMatmulCandidate candidate) {
  if (!candidate.anchor)
    return failure();

  auto outerLoop = dyn_cast<affine::AffineForOp>(candidate.anchor);
  if (!outerLoop)
    return failure();

  affine::AffineForOp middleLoop;
  affine::AffineForOp innerLoop;

  for (Operation &op : *outerLoop.getBody()) {
    if (auto loop = dyn_cast<affine::AffineForOp>(&op)) {
      if (middleLoop)
        return failure();
      middleLoop = loop;
    }
  }

  if (!middleLoop)
    return failure();

  for (Operation &op : *middleLoop.getBody()) {
    if (auto loop = dyn_cast<affine::AffineForOp>(&op)) {
      if (innerLoop)
        return failure();
      innerLoop = loop;
    }
  }

  if (!innerLoop)
    return failure();

  SmallVector<affine::AffineLoadOp> loads;
  SmallVector<affine::AffineStoreOp> stores;

  // The anchored nest must be a perfect chain of affine.for ops around the
  // MAC and nothing else: a pass that replaces the nest with a GEMM drops
  // whatever else is in it -- an initialization of C between the j and k
  // loops, a call, a store through another dialect -- or reorders it. The
  // chain runs from the anchor down to the innermost loop; every loop in it
  // has the next one as its only operation, and the innermost holds only the
  // MAC. Whether every loop's IV is used is checked once the accesses are
  // known (below).
  SmallVector<affine::AffineForOp> chain{outerLoop};
  for (;;) {
    affine::AffineForOp child;
    for (Operation &op : *chain.back().getBody()) {
      if (isa<affine::AffineYieldOp>(op))
        continue;
      auto loop = dyn_cast<affine::AffineForOp>(op);
      if (!loop || child) {
        child = nullptr;
        break;
      }
      child = loop;
    }
    if (!child)
      break;
    chain.push_back(child);
  }
  if (chain.size() < 3)
    return failure();
  for (affine::AffineForOp loop : chain)
    if (loop.getNumResults() != 0)
      return failure();

  for (Operation &op : *chain.back().getBody()) {
    if (auto load = dyn_cast<affine::AffineLoadOp>(op))
      loads.push_back(load);
    else if (auto store = dyn_cast<affine::AffineStoreOp>(op))
      stores.push_back(store);
    else if (!isa<arith::MulFOp, arith::AddFOp, affine::AffineYieldOp>(op))
      return failure();
  }

  dbgOut() << "Affine matmul recognition:\n";
  dbgOut() << "  loads: " << loads.size() << "\n";
  dbgOut() << "  stores: " << stores.size() << "\n";

  for (auto [index, load] : llvm::enumerate(loads)) {
    dbgOut() << "  load[" << index << "] map: ";
    load.getMap().print(dbgOut());
    dbgOut() << "\n";

    dbgOut() << "    operands:";
    for (Value operand : load.getIndices())
      dbgOut() << " "
                   << classifyLoopIV(operand, outerLoop, middleLoop, innerLoop);
    dbgOut() << "\n";
  }

  for (auto [index, store] : llvm::enumerate(stores)) {
    dbgOut() << "  store[" << index << "] map: ";
    store.getMap().print(dbgOut());
    dbgOut() << "\n";

    dbgOut() << "    operands:";
    for (Value operand : store.getIndices())
      dbgOut() << " "
                   << classifyLoopIV(operand, outerLoop, middleLoop, innerLoop);
    dbgOut() << "\n";
  }

  // Canonical GEMM currently expected to contain:
  //
  //   A load
  //   B load
  //   C load
  //   C store
  //
  // This is only the first recognition checkpoint.  We have not yet
  // established multiply-accumulate semantics or recovered logical I/J/K.
  if (loads.size() != 3 || stores.size() != 1)
    return failure();

  affine::AffineStoreOp store = stores.front();

  // Follow the SSA def-use chain backward from the stored value.
  auto add = store.getValue().getDefiningOp<arith::AddFOp>();
  if (!add)
    return failure();

  affine::AffineLoadOp accLoad;
  arith::MulFOp mul;

  // Accept either:
  //
  //   acc + lhs * rhs
  //
  // or, because addf is commutative:
  //
  //   lhs * rhs + acc
  if (auto load = add.getLhs().getDefiningOp<affine::AffineLoadOp>()) {
    if (auto candidateMul =
            add.getRhs().getDefiningOp<arith::MulFOp>()) {
      accLoad = load;
      mul = candidateMul;
    }
  }

  if (!accLoad || !mul) {
    if (auto load =
            add.getRhs().getDefiningOp<affine::AffineLoadOp>()) {
      if (auto candidateMul =
              add.getLhs().getDefiningOp<arith::MulFOp>()) {
        accLoad = load;
        mul = candidateMul;
      }
    }
  }

  if (!accLoad || !mul)
    return failure();

  auto lhsLoad =
      mul.getLhs().getDefiningOp<affine::AffineLoadOp>();
  auto rhsLoad =
      mul.getRhs().getDefiningOp<affine::AffineLoadOp>();

  if (!lhsLoad || !rhsLoad)
    return failure();

  auto iterationIVs =
      collectAccessIterationIVs(lhsLoad, rhsLoad, accLoad, store);

  dbgOut() << "  access iteration IV count: "
               << iterationIVs.size() << "\n";

  // The iteration space is exactly the anchored chain. A chain loop whose IV
  // no access uses repeats the accumulation (C += A*B once per iteration),
  // which a single GEMM over the used IVs would not; an IV from outside the
  // chain is a parameter of the nest, not one of its iteration dimensions.
  if (iterationIVs.size() != chain.size())
    return failure();
  for (affine::AffineForOp loop : chain)
    if (!llvm::is_contained(iterationIVs, loop.getInductionVar()))
      return failure();

  auto sourceDomain = recoverSourceDomain(iterationIVs);
  if (failed(sourceDomain))
    return failure();

  dbgOut() << "  source iteration domain:\n";
  sourceDomain->print(dbgOut());

  dbgOut() << "  source inequalities:\n";
  for (unsigned i = 0; i < sourceDomain->getNumInequalities(); ++i) {
    auto inequality = sourceDomain->getInequality64(i);

    dbgOut() << "    [";
    for (unsigned j = 0; j < inequality.size(); ++j) {
      if (j != 0)
        dbgOut() << " ";
      dbgOut() << inequality[j];
    }
    dbgOut() << "] >= 0\n";
  }

  // The accumulator must be read from and written back to the same
  // memref location.
  if (accLoad.getMemref() != store.getMemref())
    return failure();

  if (accLoad.getMap() != store.getMap())
    return failure();

  auto loadIndices = accLoad.getIndices();
  auto storeIndices = store.getIndices();

  if (loadIndices.size() != storeIndices.size())
    return failure();

  for (unsigned i = 0; i < loadIndices.size(); ++i) {
    if (loadIndices[i] != storeIndices[i])
      return failure();
  }

  // Recover canonical logical GEMM coordinates from access operands:
  //
  //   A = (I, K)
  //   B = (K, J)
  //   C = (I, J)
  //
  // This first checkpoint handles direct loop-IV operands only.
  auto normalizedLhs =
      normalizeAccessMap(lhsLoad.getMap(), lhsLoad.getIndices(),
                         iterationIVs);
  auto normalizedRhs =
      normalizeAccessMap(rhsLoad.getMap(), rhsLoad.getIndices(),
                         iterationIVs);
  auto normalizedAcc =
      normalizeAccessMap(accLoad.getMap(), accLoad.getIndices(),
                         iterationIVs);

  if (failed(normalizedLhs) ||
      failed(normalizedRhs) ||
      failed(normalizedAcc))
    return failure();

  dbgOut() << "  normalized A: ";
  normalizedLhs->print(dbgOut());
  dbgOut() << "\n";

  dbgOut() << "  normalized B: ";
  normalizedRhs->print(dbgOut());
  dbgOut() << "\n";

  dbgOut() << "  normalized C: ";
  normalizedAcc->print(dbgOut());
  dbgOut() << "\n";

  // A logical GEMM may have a shared partition prefix P*:
  //
  //   A = (P*, I, K)
  //   B = (P*, K, J)
  //   C = (P*, I, J)
  //
  // Ordinary GEMM is the special case where P* is empty.
  unsigned numAccessResults = normalizedAcc->getNumResults();
  if (numAccessResults < 2 ||
      normalizedLhs->getNumResults() != numAccessResults ||
      normalizedRhs->getNumResults() != numAccessResults)
    return failure();

  unsigned numPartitionDims = numAccessResults - 2;

  // Multiplication is commutative, so the source operand order does not
  // determine which load is logical A=(P*,I,K) and which is B=(P*,K,J).
  affine::AffineLoadOp aLoad = lhsLoad;
  affine::AffineLoadOp bLoad = rhsLoad;
  AffineMap accessA = *normalizedLhs;
  AffineMap accessB = *normalizedRhs;

  unsigned numIterationDims = accessA.getNumDims();

  // The coordinates shared by two operands -- I by A and C, J by B and C, K
  // by A and B -- must be the same logical coordinate in both, each operand
  // through its own view (matchSharedCoordinate); the partition prefix must
  // be spelled identically; and I, J, K must be three different coordinates.
  struct GemmCoordinates {
    SharedCoordinate i, j, k;
  };
  auto matchesGemmAccesses =
      [&](AffineMap a, AffineMap b) -> std::optional<GemmCoordinates> {
    for (unsigned p = 0; p < numPartitionDims; ++p) {
      if (a.getResult(p) != normalizedAcc->getResult(p) ||
          b.getResult(p) != normalizedAcc->getResult(p))
        return std::nullopt;
    }

    auto i = matchSharedCoordinate(a.getResult(numPartitionDims),
                                   normalizedAcc->getResult(numPartitionDims),
                                   numIterationDims);
    auto j = matchSharedCoordinate(
        b.getResult(numPartitionDims + 1),
        normalizedAcc->getResult(numPartitionDims + 1), numIterationDims);
    auto k = matchSharedCoordinate(a.getResult(numPartitionDims + 1),
                                   b.getResult(numPartitionDims),
                                   numIterationDims);
    if (!i || !j || !k)
      return std::nullopt;
    if (i->expr == j->expr || i->expr == k->expr || j->expr == k->expr)
      return std::nullopt;
    return GemmCoordinates{*i, *j, *k};
  };

  std::optional<GemmCoordinates> coordinates =
      matchesGemmAccesses(accessA, accessB);
  if (!coordinates) {
    coordinates = matchesGemmAccesses(accessB, accessA);
    if (!coordinates)
      return failure();

    std::swap(aLoad, bLoad);
    std::swap(accessA, accessB);
  }

  AffineExpr logicalI = coordinates->i.expr;
  AffineExpr logicalJ = coordinates->j.expr;
  AffineExpr logicalK = coordinates->k.expr;

  // Each operand's view of its coordinates: A = (I, K), B = (K, J), C = (I, J).
  // matchSharedCoordinate's `first` is the first expression it was given.
  const std::array<OperandCoordinateView, 2> viewA{coordinates->i.first,
                                                   coordinates->k.first};
  const std::array<OperandCoordinateView, 2> viewB{coordinates->k.second,
                                                   coordinates->j.first};
  const std::array<OperandCoordinateView, 2> viewC{coordinates->i.second,
                                                   coordinates->j.second};

  auto linearI = extractLinearForm(logicalI, numIterationDims);
  auto linearJ = extractLinearForm(logicalJ, numIterationDims);
  auto linearK = extractLinearForm(logicalK, numIterationDims);

  if (failed(linearI) ||
      failed(linearJ) ||
      failed(linearK))
    return failure();

  // Every operand subscript of a coordinate must be exactly its view of the
  // coordinate, scale * y + offset with scale > 0, as linear forms over the
  // loop IVs -- the property the lowering builds each operand's strided view
  // from. matchSharedCoordinate constructs the views so that this holds; it
  // is checked here, in 64-bit-checked arithmetic, so that a nest for which it
  // would not hold (or would hold only after wrapping around) is refused
  // instead of lowered through a wrong view.
  auto subscriptIsView = [&](AffineExpr subscript, const LinearForm &coordinate,
                             const OperandCoordinateView &view) {
    if (view.scale <= 0)
      return false;
    auto form = extractLinearForm(subscript, numIterationDims);
    if (failed(form))
      return false;
    for (unsigned d = 0; d < numIterationDims; ++d) {
      std::optional<int64_t> expected =
          llvm::checkedMul(view.scale, coordinate.coefficients[d]);
      if (!expected || *expected != form->coefficients[d])
        return false;
    }
    std::optional<int64_t> scaled =
        llvm::checkedMul(view.scale, coordinate.constant);
    std::optional<int64_t> expected =
        scaled ? llvm::checkedAdd(*scaled, view.offset) : std::nullopt;
    return expected && *expected == form->constant;
  };
  if (!subscriptIsView(accessA.getResult(numPartitionDims), *linearI,
                       viewA[0]) ||
      !subscriptIsView(accessA.getResult(numPartitionDims + 1), *linearK,
                       viewA[1]) ||
      !subscriptIsView(accessB.getResult(numPartitionDims), *linearK,
                       viewB[0]) ||
      !subscriptIsView(accessB.getResult(numPartitionDims + 1), *linearJ,
                       viewB[1]) ||
      !subscriptIsView(normalizedAcc->getResult(numPartitionDims), *linearI,
                       viewC[0]) ||
      !subscriptIsView(normalizedAcc->getResult(numPartitionDims + 1),
                       *linearJ, viewC[1]))
    return failure();

  dbgOut() << "  affine logical mapping\n";
  if (succeeded(linearI) &&
      succeeded(linearJ) &&
      succeeded(linearK)) {
    auto printLinearForm = [](StringRef name,
                              const LinearForm &form) {
      dbgOut() << "    " << name << " = [";
      for (unsigned i = 0; i < form.coefficients.size(); ++i) {
        if (i != 0)
          dbgOut() << " ";
        dbgOut() << form.coefficients[i];
      }
      dbgOut() << " | " << form.constant << "]\n";
    };

    printLinearForm("I", *linearI);
    printLinearForm("J", *linearJ);
    printLinearForm("K", *linearK);
  } else {
    dbgOut() << "    unsupported non-linear affine form\n";
  }

  dbgOut() << "  logical GEMM coordinates recovered\n";
  dbgOut() << "    I: ";
  logicalI.print(dbgOut());
  dbgOut() << "\n";

  dbgOut() << "    J: ";
  logicalJ.print(dbgOut());
  dbgOut() << "\n";

  dbgOut() << "    K: ";
  logicalK.print(dbgOut());
  dbgOut() << "\n";

  dbgOut() << "  accumulator read-modify-write verified\n";
  dbgOut() << "  MAC dataflow recognized\n";
  dbgOut() << "    accumulator load: ";
  accLoad.getOperation()->print(dbgOut());
  dbgOut() << "\n";

  dbgOut() << "    lhs load: ";
  lhsLoad.getOperation()->print(dbgOut());
  dbgOut() << "\n";

  dbgOut() << "    rhs load: ";
  rhsLoad.getOperation()->print(dbgOut());
  dbgOut() << "\n";

  LogicalMatmul result;
  result.anchor = candidate.anchor;
  result.outputStore = store.getOperation();

  // Preserve the exact source/execution iteration domain D_S.  Its dimension
  // order matches iterationIVs and therefore the columns of logicalMapping.
  result.sourceDomain = *sourceDomain;

  SmallVector<int64_t> sourceOrigin;
  sourceOrigin.reserve(iterationIVs.size());

  bool hasStaticSourceOrigin = true;
  for (Value iv : iterationIVs) {
    auto origin = recoverStaticIterationOrigin(iv);
    if (failed(origin)) {
      hasStaticSourceOrigin = false;
      break;
    }

    sourceOrigin.push_back(*origin);
  }

  if (hasStaticSourceOrigin) {
    result.logicalMapping.sourceOrigin = std::move(sourceOrigin);
    result.logicalMapping.hasStaticSourceOrigin = true;
  }

  result.lhs = aLoad.getMemRef();
  result.rhs = bLoad.getMemRef();
  result.output = store.getMemRef();

  result.accessA = accessA;
  result.accessB = accessB;
  result.accessC = *normalizedAcc;

  // Materialize operand factor maps over the logical coordinates
  // y = (P*, I, J, K):
  //
  //   F_A = phiA(P*, I, K)
  //   F_B = phiB(P*, K, J)
  //   F_C = phiC(P*, I, J)
  //
  // Each result is the operand's own view of the coordinate (scale * y +
  // offset): the identity when the operand's subscript is the coordinate's
  // expression itself.
  result.viewA = viewA;
  result.viewB = viewB;
  result.viewC = viewC;

  MLIRContext *context = candidate.anchor->getContext();
  const unsigned numLogicalDims = numPartitionDims + 3;
  auto viewExpr = [&](unsigned logicalDim, const OperandCoordinateView &view) {
    AffineExpr y = getAffineDimExpr(numPartitionDims + logicalDim, context);
    return y * view.scale + view.offset;
  };
  SmallVector<AffineExpr> phiAResults;
  SmallVector<AffineExpr> phiBResults;
  SmallVector<AffineExpr> phiCResults;

  for (unsigned p = 0; p < numPartitionDims; ++p) {
    phiAResults.push_back(getAffineDimExpr(p, context));
    phiBResults.push_back(getAffineDimExpr(p, context));
    phiCResults.push_back(getAffineDimExpr(p, context));
  }

  // y = (P*, I, J, K): I is logical dim 0, J dim 1, K dim 2 after P*.
  phiAResults.push_back(viewExpr(0, viewA[0]));  // I
  phiAResults.push_back(viewExpr(2, viewA[1]));  // K

  phiBResults.push_back(viewExpr(2, viewB[0]));  // K
  phiBResults.push_back(viewExpr(1, viewB[1]));  // J

  phiCResults.push_back(viewExpr(0, viewC[0]));  // I
  phiCResults.push_back(viewExpr(1, viewC[1]));  // J

  result.phiA =
      AffineMap::get(
          /*dimCount=*/numLogicalDims,
          /*symbolCount=*/0,
          phiAResults,
          context);

  result.phiB =
      AffineMap::get(
          /*dimCount=*/numLogicalDims,
          /*symbolCount=*/0,
          phiBResults,
          context);

  result.phiC =
      AffineMap::get(
          /*dimCount=*/numLogicalDims,
          /*symbolCount=*/0,
          phiCResults,
          context);

  SmallVector<AffineExpr> logicalPartitions;
  logicalPartitions.reserve(numPartitionDims);
  for (unsigned p = 0; p < numPartitionDims; ++p)
    logicalPartitions.push_back(accessA.getResult(p));

  // Materialize the complete affine coordinate transformation
  //
  //   y = T x + c
  //
  // where y = (P*, I, J, K).
  for (AffineExpr logicalPartition : logicalPartitions) {
    auto linearPartition =
        extractLinearForm(logicalPartition, numIterationDims);
    if (failed(linearPartition))
      return failure();

    result.logicalMapping.transformation.push_back(
        linearPartition->coefficients);
    result.logicalMapping.offset.push_back(
        linearPartition->constant);
  }

  result.logicalMapping.transformation.push_back(linearI->coefficients);
  result.logicalMapping.transformation.push_back(linearJ->coefficients);
  result.logicalMapping.transformation.push_back(linearK->coefficients);

  result.logicalMapping.offset.push_back(linearI->constant);
  result.logicalMapping.offset.push_back(linearJ->constant);
  result.logicalMapping.offset.push_back(linearK->constant);

  result.logicalMapping.isInvertible =
      isInvertibleSquareMapping(result.logicalMapping.transformation);

  if (result.logicalMapping.isInvertible) {
    auto inverse =
        invertSquareMapping(result.logicalMapping.transformation);
    // The inverse feeds only the debug output below: one whose entries (or
    // intermediate values) do not fit in 64 bits is not printed. Nothing is
    // decided from it, so this is no reason to refuse the nest.
    if (failed(inverse)) {
      dbgOut() << "  inverse logical mapping: not representable in 64 bits\n";
    } else {

      dbgOut() << "  inverse logical mapping\n";
      for (unsigned row = 0; row < inverse->size(); ++row) {
        dbgOut() << "    T^-1[" << row << "] = [";

        for (unsigned column = 0;
             column < (*inverse)[row].size();
             ++column) {
          if (column != 0)
            dbgOut() << " ";

          const Rational &value = (*inverse)[row][column];
          dbgOut() << value.numerator;
          if (value.denominator != 1)
            dbgOut() << "/" << value.denominator;
        }

        dbgOut() << "]\n";
      }

      auto logicalInequalities =
          transformDomainToLogical(
              *sourceDomain,
              *inverse,
              result.logicalMapping.offset);

      if (succeeded(logicalInequalities)) {
        dbgOut() << "  logical iteration domain:\n";
        for (const auto &inequality : *logicalInequalities) {
          dbgOut() << "    [";

          for (unsigned column = 0;
               column < inequality.size();
               ++column) {
            if (column != 0)
              dbgOut() << " ";

            const Rational &value = inequality[column];
            dbgOut() << value.numerator;
            if (value.denominator != 1)
              dbgOut() << "/" << value.denominator;
          }

          dbgOut() << "] >= 0\n";
        }
      }
    }
  }

  auto exactLogicalDomain =
      buildExactLogicalDomain(
          *sourceDomain,
          result.logicalMapping.transformation,
          result.logicalMapping.offset);

  if (succeeded(exactLogicalDomain)) {
    result.logicalDomain = std::move(*exactLogicalDomain);

    dbgOut() << "  exact logical iteration domain:\n";
    result.logicalDomain->print(dbgOut());
  }

  result.partitionMap =
      AffineMap::get(/*dimCount=*/numIterationDims,
                     /*symbolCount=*/0,
                     logicalPartitions,
                     candidate.anchor->getContext());
  result.rowMap =
      AffineMap::get(/*dimCount=*/numIterationDims,
                     /*symbolCount=*/0,
                     logicalI,
                     candidate.anchor->getContext());
  result.columnMap =
      AffineMap::get(/*dimCount=*/numIterationDims,
                     /*symbolCount=*/0,
                     logicalJ,
                     candidate.anchor->getContext());
  result.reductionMap =
      AffineMap::get(/*dimCount=*/numIterationDims,
                     /*symbolCount=*/0,
                     logicalK,
                     candidate.anchor->getContext());

  // Normalization needs static loop bounds. A nest with a symbolic bound is
  // still a recognized GEMM, with no normalized domain (consumers that need
  // one check normalization.domain).
  auto normalization =
      normalizeLogicalCoordinates(result, iterationIVs,
                                  result.normalizationOverflow,
                                  result.normalizationEmptyLoop);
  if (succeeded(normalization))
    result.normalization = std::move(*normalization);

  result.rowExtent =
      recoverLogicalExtent(logicalI, iterationIVs);
  result.columnExtent =
      recoverLogicalExtent(logicalJ, iterationIVs);
  result.reductionExtent =
      recoverLogicalExtent(logicalK, iterationIVs);

  return result;
}
