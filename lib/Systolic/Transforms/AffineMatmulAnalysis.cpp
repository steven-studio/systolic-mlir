#include "Systolic/AffineMatmulAnalysis.h"

#include "mlir/Analysis/Presburger/Matrix.h"
#include "mlir/Dialect/Affine/Analysis/AffineStructures.h"
#include "mlir/Dialect/Arith/IR/Arith.h"

#include "llvm/Support/CheckedArithmetic.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"

#include <limits>
#include <numeric>
#include <optional>
#include <string>
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

/// The position of `value` in the loop chain, for the debug trace.
static std::string classifyLoopIV(Value value,
                                  ArrayRef<affine::AffineForOp> chain) {
  for (auto [position, loop] : llvm::enumerate(chain))
    if (value == affine::AffineForOp(loop).getInductionVar())
      return "loop" + std::to_string(position);
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

/// A bound of `loop` that is a constant: one constant result, or the max (a
/// lower bound) / min (an upper bound) of results that are all constants --
/// `min(8, 16)` is 8 (docs/AFFINE_GEMM_MAPPING.md section 1, P0.6). nullopt
/// when a result is not a constant (a symbol, another loop's IV). A
/// comparison of 64-bit constants cannot overflow.
static std::optional<int64_t> constantLoopBound(affine::AffineForOp loop,
                                                bool lower) {
  AffineMap map = lower ? loop.getLowerBoundMap() : loop.getUpperBoundMap();
  if (map.getNumResults() == 0)
    return std::nullopt;
  std::optional<int64_t> bound;
  for (AffineExpr result : map.getResults()) {
    auto constant = dyn_cast<AffineConstantExpr>(result);
    if (!constant)
      return std::nullopt;
    const int64_t value = constant.getValue();
    if (!bound || (lower ? value > *bound : value < *bound))
      bound = value;
  }
  return bound;
}

/// Why a bound of `loop` is not a constant, for the diagnostics: the domain
/// is not a box when it depends on another loop's IV.
static std::string describeNonConstantBound(affine::AffineForOp loop,
                                            unsigned position, bool lower) {
  AffineMap map = lower ? loop.getLowerBoundMap() : loop.getUpperBoundMap();
  ValueRange operands =
      lower ? loop.getLowerBoundOperands() : loop.getUpperBoundOperands();
  std::string which = "loop " + std::to_string(position) + "'s " +
                      (lower ? "lower" : "upper") + " bound";
  if (llvm::any_of(operands, [](Value operand) {
        return affine::isAffineForInductionVar(operand);
      }))
    return which + " depends on another loop's counter: the iteration "
                   "domain is not a box";
  if (map.getNumResults() > 1)
    return which + " is the " + (lower ? "max" : "min") +
           " of several values, not one constant";
  return which + " is a symbol, a value known only at run time";
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

  std::optional<int64_t> lower = constantLoopBound(loop, /*lower=*/true);
  if (!lower)
    return failure();

  return *lower;
}

/// The value the IV `iv` has on the whole iteration domain when its loop runs
/// exactly once (constant bounds, one iteration), nullopt otherwise -- also
/// for a loop that runs zero times: the domain is then empty, refused by the
/// dense partition (SQ-2).
static std::optional<int64_t> singleIterationValue(Value iv) {
  affine::AffineForOp loop = affine::getForInductionVarOwner(iv);
  if (!loop)
    return std::nullopt;
  std::optional<int64_t> lower = constantLoopBound(loop, /*lower=*/true);
  std::optional<int64_t> upper = constantLoopBound(loop, /*lower=*/false);
  if (!lower || !upper)
    return std::nullopt;
  std::optional<int64_t> trips =
      checkedTripCount(*lower, *upper, loop.getStep().getSExtValue());
  if (!trips || *trips != 1)
    return std::nullopt;
  return *lower;
}

/// For every iteration dimension, the single value of a loop that runs once
/// (singleIterationValue), nullopt for every other loop.
using SingleValues = SmallVector<std::optional<int64_t>>;

/// `form` as a function on the iteration domain D: the term of a loop that
/// runs exactly once is a constant there, so it is folded into the constant
/// (nullopt when the folded constant does not fit in 64 bits). Two affine
/// functions of the IVs are equal on D iff their domain forms are identical:
/// a loop that runs at least twice exposes its coefficient
/// (docs/AFFINE_GEMM_MAPPING.md, Lemma 0).
static std::optional<LinearForm> domainForm(const LinearForm &form,
                                            ArrayRef<std::optional<int64_t>>
                                                single) {
  LinearForm result = form;
  for (unsigned d = 0; d < result.coefficients.size(); ++d) {
    if (!single[d] || result.coefficients[d] == 0)
      continue;
    std::optional<int64_t> term =
        llvm::checkedMul(result.coefficients[d], *single[d]);
    std::optional<int64_t> constant =
        term ? llvm::checkedAdd(result.constant, *term) : std::nullopt;
    if (!constant)
      return std::nullopt;
    result.constant = *constant;
    result.coefficients[d] = 0;
  }
  return result;
}

static bool usesSingleIterationLoop(const LinearForm &form,
                                    ArrayRef<std::optional<int64_t>> single) {
  for (unsigned d = 0; d < form.coefficients.size(); ++d)
    if (single[d] && form.coefficients[d] != 0)
      return true;
  return false;
}

static bool isConstantForm(const LinearForm &form) {
  return llvm::all_of(form.coefficients,
                      [](int64_t coefficient) { return coefficient == 0; });
}

/// The two operands' expressions for one shared logical coordinate, matched:
/// the coordinate's own expression and each operand's view of it.
struct SharedCoordinate {
  AffineExpr expr;
  OperandCoordinateView first;
  OperandCoordinateView second;
};

/// Whether the linear forms `formA` and `formB` are affine images s * g + o
/// (integer s != 0) of one linear form g of the loop IVs: then the coordinate
/// is the primitive form g in formA's direction (the coefficients divided by
/// their gcd, no constant) and each operand's view is its (s, o) -- s < 0 for
/// formB when it runs the other way (a negative-stride view). The linear
/// parts must be multiples of each other (docs/AFFINE_GEMM_MAPPING.md,
/// Lemma 1: necessary for any strided view of a coordinate that takes two
/// values or more). On failure `reason` says why.
static std::optional<SharedCoordinate>
matchProportionalForms(const LinearForm &formA, const LinearForm &formB,
                       unsigned numDims, MLIRContext *context,
                       std::string &reason) {
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
  const int64_t scaleA = content(formA);
  const int64_t scaleB = content(formB);
  // A subscript without a loop (a constant) is not a coordinate.
  if (scaleA == 0 || scaleB == 0) {
    reason = "a subscript varies with no loop (a constant subscript)";
    return std::nullopt;
  }

  AffineExpr shared;
  bool opposite = true;
  bool same = true;
  for (unsigned d = 0; d < numDims; ++d) {
    const int64_t primitiveA = formA.coefficients[d] / scaleA;
    const int64_t primitiveB = formB.coefficients[d] / scaleB;
    same = same && primitiveA == primitiveB;
    opposite = opposite && primitiveA == -primitiveB;
    if (primitiveA == 0)
      continue;
    AffineExpr term = getAffineDimExpr(d, context);
    if (primitiveA != 1)
      term = term * primitiveA;
    shared = shared ? shared + term : term;
  }
  if (!same && !opposite) {
    reason = "the two subscripts are not multiples of one linear form of the "
             "loop counters (no strided view of one matches the other)";
    return std::nullopt;
  }

  // The coordinate runs along the iteration order of its outermost loop: its
  // first nonzero coefficient is positive. An operand whose subscript
  // decreases in that order (A[7 - i], or a C loop that counts down read
  // through a different expression than the other operand's) reads the
  // coordinate backwards: a negative scale, a negative-stride view of that
  // operand itself (docs/AFFINE_GEMM_MAPPING.md section 5). In opposite
  // directions (A[7 - i] with C[i]) exactly one of the two views is negative.
  int64_t direction = 1;
  for (unsigned d = 0; d < numDims; ++d)
    if (formA.coefficients[d] != 0) {
      direction = formA.coefficients[d] > 0 ? 1 : -1;
      break;
    }
  if (direction < 0)
    shared = shared * -1;
  SharedCoordinate result;
  result.expr = shared;
  result.first = {direction * scaleA, formA.constant};
  result.second = {direction * (same ? scaleB : -scaleB), formB.constant};
  return result;
}

/// Whether the expressions `a` and `b` (over the same iteration dims) address
/// the same logical coordinate, and how.
///
/// They do when they are identical -- then the coordinate is that expression
/// and both views are the identity -- or when both are affine images s * g + o
/// (integer s != 0) of one linear form g over the loop IVs: a constant offset,
/// a stride or a reversal on one operand only, as in C[i + 8][j], C[2i][j] or
/// C[7 - i][j] with A[i][k], which are dense GEMMs of the operands' own views
/// (matchProportionalForms). Expressions over different combinations of the
/// loops (4 i0 + i1 against 8 i0 + i1) or without any loop are not one
/// coordinate.
///
/// The test is on the iteration domain D, not on the expressions: when the
/// expressions do not match but involve a loop that runs exactly once, they
/// are compared as their domain forms, in which that loop's term is a
/// constant (C[i + t][j] with A[i][k], t in [0, 1)). When both are constant
/// on D the coordinate has one value (an extent-1 coordinate, spec question
/// SQ-4): only the expression rule above applies, as before. When exactly one
/// is constant on D, that operand reads one index while the other runs
/// through several: a broadcast, not a view.
static std::optional<SharedCoordinate>
matchSharedCoordinate(AffineExpr a, AffineExpr b, unsigned numDims,
                      ArrayRef<std::optional<int64_t>> single,
                      std::string &reason) {
  if (a == b)
    return SharedCoordinate{a, {}, {}};

  auto formA = extractLinearForm(a, numDims);
  auto formB = extractLinearForm(b, numDims);
  if (failed(formA) || failed(formB)) {
    reason = "a subscript is not a linear form of the loop counters (mod, "
             "floordiv, ceildiv, a symbol, or a coefficient beyond 64 bits)";
    return std::nullopt;
  }

  MLIRContext *context = a.getContext();
  std::optional<SharedCoordinate> matched =
      matchProportionalForms(*formA, *formB, numDims, context, reason);
  if (matched || (!usesSingleIterationLoop(*formA, single) &&
                  !usesSingleIterationLoop(*formB, single)))
    return matched;

  std::optional<LinearForm> onDomainA = domainForm(*formA, single);
  std::optional<LinearForm> onDomainB = domainForm(*formB, single);
  if (!onDomainA || !onDomainB) {
    reason = "a subscript's value on the iteration domain overflows 64-bit "
             "arithmetic";
    return std::nullopt;
  }
  const bool constantA = isConstantForm(*onDomainA);
  const bool constantB = isConstantForm(*onDomainB);
  if (constantA && constantB) {
    reason += " (both subscripts are constant on the iteration domain: a "
              "coordinate with one value, spec question SQ-4)";
    return std::nullopt;
  }
  if (constantA != constantB) {
    reason = "one subscript is constant on the iteration domain while the "
             "other varies (a broadcast operand, not a strided view)";
    return std::nullopt;
  }
  return matchProportionalForms(*onDomainA, *onDomainB, numDims, context,
                                reason);
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

    // A shallower nest (one or two loops: a matrix-vector or dot product,
    // a GEMM with a role of extent one, docs/AFFINE_GEMM_MAPPING.md section 2)
    // is a candidate only when its innermost body stores an arith.addf -- the
    // three-deep rule below is unchanged.
    auto storesSum = [](affine::AffineForOp loop) {
      unsigned storesOfSum = 0;
      for (Operation &op : *loop.getBody()) {
        if (isa<affine::AffineForOp>(op))
          return false;
        if (auto store = dyn_cast<affine::AffineStoreOp>(op))
          storesOfSum += store.getValue().getDefiningOp<arith::AddFOp>() != nullptr;
      }
      return storesOfSum == 1;
    };
    // The anchor is the outermost loop whose IV the statement uses: a parent
    // loop that no access uses only repeats the nest (y += 4 M x is four
    // matrix-vector products in a row; the tiles run inside it), one whose
    // IV it uses makes this loop an inner part of a deeper nest.
    auto usedByAccesses = [&](affine::AffineForOp loop,
                              affine::AffineForOp innermost) {
      bool used = false;
      innermost.getBody()->walk([&](Operation *op) {
        ValueRange indices;
        if (auto load = dyn_cast<affine::AffineLoadOp>(op))
          indices = load.getIndices();
        else if (auto store = dyn_cast<affine::AffineStoreOp>(op))
          indices = store.getIndices();
        used |= llvm::is_contained(indices, loop.getInductionVar());
      });
      return used;
    };
    auto addShallow = [&](affine::AffineForOp innermost) {
      if (!storesSum(innermost))
        return;
      for (auto parent = dyn_cast<affine::AffineForOp>(outerLoop->getParentOp());
           parent;
           parent = dyn_cast<affine::AffineForOp>(parent->getParentOp()))
        if (usedByAccesses(parent, innermost))
          return;
      dbgOut() << "Found affine matmul candidate (shallow nest)\n";
      candidates.push_back(AffineMatmulCandidate{outerLoop.getOperation()});
    };
    if (!middleLoop) {
      addShallow(outerLoop);
      return;
    }

    // Find the unique affine.for directly nested in middleLoop.
    for (Operation &op : *middleLoop.getBody()) {
      if (auto loop = dyn_cast<affine::AffineForOp>(&op)) {
        if (innerLoop)
          return;
        innerLoop = loop;
      }
    }

    if (!innerLoop) {
      addShallow(middleLoop);
      return;
    }

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
/// leaves both false; for a bound that is not a constant, `detail` says which
/// and why (describeNonConstantBound).
static FailureOr<AffineLogicalNormalization>
normalizeLogicalCoordinates(const LogicalMatmul &matmul,
                            ValueRange iterationIVs, bool &overflowed,
                            bool &emptyLoop, std::string &detail) {
  overflowed = false;
  emptyLoop = false;
  detail.clear();
  if (!matmul.sourceDomain)
    return failure();

  const auto &sourceDomain = *matmul.sourceDomain;
  const unsigned numSourceDims = sourceDomain.getNumDimVars();

  if (numSourceDims == 0 ||
      iterationIVs.size() != numSourceDims)
    return failure();

  // Static bounds first: a lower bound that is not a constant also leaves the
  // source origin unknown (checked next), and the reason is the bound.
  for (auto [position, iv] : llvm::enumerate(iterationIVs)) {
    affine::AffineForOp loop = affine::getForInductionVarOwner(iv);
    if (!loop)
      return failure();
    for (bool lower : {true, false})
      if (!constantLoopBound(loop, lower)) {
        detail = describeNonConstantBound(loop, position, lower);
        return failure();
      }
  }

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
    // builds without assertions.) Every bound is a constant here (checked
    // above).
    std::optional<int64_t> constantLower =
        constantLoopBound(loop, /*lower=*/true);
    std::optional<int64_t> constantUpper =
        constantLoopBound(loop, /*lower=*/false);
    if (!constantLower || !constantUpper)
      return failure();

    const int64_t lower = *constantLower;
    const int64_t upper = *constantUpper;

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
    AffineMatmulCandidate candidate, std::string *why,
    GemmMappingClass *mappingClass) {
  // Every refusal says why, and classifies the nest
  // (docs/AFFINE_GEMM_MAPPING.md, sections 5 and 9).
  auto refuseAs = [&](GemmMappingClass kind,
                      const llvm::Twine &reason) -> FailureOr<LogicalMatmul> {
    if (why)
      *why = reason.str();
    if (mappingClass)
      *mappingClass = kind;
    return failure();
  };
  auto refuse = [&](const llvm::Twine &reason) {
    return refuseAs(GemmMappingClass::ProvenInvalid, reason);
  };
  auto unknown = [&](const llvm::Twine &reason) {
    return refuseAs(GemmMappingClass::Unknown, reason);
  };
  auto unsupported = [&](const llvm::Twine &reason) {
    return refuseAs(GemmMappingClass::ValidButLoweringUnsupported, reason);
  };

  if (!candidate.anchor)
    return refuse("no anchor operation");

  auto outerLoop = dyn_cast<affine::AffineForOp>(candidate.anchor);
  if (!outerLoop)
    return refuse("the anchor is not an affine.for");

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
  // Any number of loops: a role of extent one needs no loop (a matrix-vector
  // product is a GEMM with N = 1, docs/AFFINE_GEMM_MAPPING.md section 2).
  for (affine::AffineForOp loop : chain)
    if (loop.getNumResults() != 0)
      return refuse("a loop of the nest carries a value (iter_args)");

  const char *const notOneMac =
      "the innermost body is not exactly one multiply-accumulate "
      "C = C + A * B (three affine.load, one arith.mulf, one arith.addf, one "
      "affine.store)";
  for (Operation &op : *chain.back().getBody()) {
    if (auto load = dyn_cast<affine::AffineLoadOp>(op))
      loads.push_back(load);
    else if (auto store = dyn_cast<affine::AffineStoreOp>(op))
      stores.push_back(store);
    else if (!isa<arith::MulFOp, arith::AddFOp, affine::AffineYieldOp>(op))
      return refuse(notOneMac);
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
                   << classifyLoopIV(operand, chain);
    dbgOut() << "\n";
  }

  for (auto [index, store] : llvm::enumerate(stores)) {
    dbgOut() << "  store[" << index << "] map: ";
    store.getMap().print(dbgOut());
    dbgOut() << "\n";

    dbgOut() << "    operands:";
    for (Value operand : store.getIndices())
      dbgOut() << " "
                   << classifyLoopIV(operand, chain);
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
    return refuse(notOneMac);

  affine::AffineStoreOp store = stores.front();

  // Follow the SSA def-use chain backward from the stored value.
  auto add = store.getValue().getDefiningOp<arith::AddFOp>();
  if (!add)
    return refuse("the stored value is not an arith.addf");

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
    return refuse("the stored value is not a load plus a product");

  auto lhsLoad =
      mul.getLhs().getDefiningOp<affine::AffineLoadOp>();
  auto rhsLoad =
      mul.getRhs().getDefiningOp<affine::AffineLoadOp>();

  if (!lhsLoad || !rhsLoad)
    return refuse("a factor of the product is not an affine.load");
  // Three different loads: the accumulator's and one per factor. (A factor
  // that is the accumulator's own load, or one load used as both factors,
  // is not C += A * B.)
  if (lhsLoad == rhsLoad || lhsLoad == accLoad || rhsLoad == accLoad)
    return refuse("the two factors and the accumulator are not three "
                  "different loads");

  auto iterationIVs =
      collectAccessIterationIVs(lhsLoad, rhsLoad, accLoad, store);

  dbgOut() << "  access iteration IV count: "
               << iterationIVs.size() << "\n";

  // The iteration space is exactly the anchored chain. A chain loop whose IV
  // no access uses repeats the accumulation (C += A*B once per iteration),
  // which a single GEMM over the used IVs would not; an IV from outside the
  // chain is a parameter of the nest, not one of its iteration dimensions.
  for (auto [position, loop] : llvm::enumerate(chain))
    if (!llvm::is_contained(iterationIVs, loop.getInductionVar()))
      return refuse("loop " + llvm::Twine(position) +
                    " of the nest is used by no access (it repeats the "
                    "accumulation)");
  if (iterationIVs.size() != chain.size())
    return refuse("an access uses the counter of a loop outside the nest");

  auto sourceDomain = recoverSourceDomain(iterationIVs);
  if (failed(sourceDomain))
    return unknown("the iteration domain of the loops cannot be represented "
                  "(a bound outside the affine constraint system)");

  // The loops that run exactly once, by iteration dimension: their IV is a
  // constant on the iteration domain D.
  SingleValues single;
  for (Value iv : iterationIVs)
    single.push_back(singleIterationValue(iv));

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
  // memref location -- at every iteration, so as functions on the iteration
  // domain: the same map and operands, or (checked below, once the access
  // maps are normalized) subscripts with identical domain forms.
  if (accLoad.getMemref() != store.getMemref())
    return refuse("the accumulator is loaded from one memref and stored to "
                  "another");
  const bool sameAccumulatorAccess =
      accLoad.getMap() == store.getMap() &&
      llvm::equal(accLoad.getIndices(), store.getIndices());

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
    return unknown("an access index is not a counter of the nest's loops, or "
                  "an access map has symbols");

  if (!sameAccumulatorAccess) {
    auto normalizedStore =
        normalizeAccessMap(store.getMap(), store.getIndices(), iterationIVs);
    const char *const differentElements =
        "the accumulator is loaded and stored at different elements of C";
    if (failed(normalizedStore) ||
        normalizedStore->getNumResults() != normalizedAcc->getNumResults())
      return refuse(differentElements);
    const unsigned numDims = normalizedAcc->getNumDims();
    for (unsigned r = 0; r < normalizedAcc->getNumResults(); ++r) {
      auto loaded = extractLinearForm(normalizedAcc->getResult(r), numDims);
      auto stored = extractLinearForm(normalizedStore->getResult(r), numDims);
      std::optional<LinearForm> loadedOnDomain =
          succeeded(loaded) ? domainForm(*loaded, single) : std::nullopt;
      std::optional<LinearForm> storedOnDomain =
          succeeded(stored) ? domainForm(*stored, single) : std::nullopt;
      if (!loadedOnDomain || !storedOnDomain ||
          loadedOnDomain->coefficients != storedOnDomain->coefficients ||
          loadedOnDomain->constant != storedOnDomain->constant)
        return refuse(differentElements);
    }
  }

  dbgOut() << "  normalized A: ";
  normalizedLhs->print(dbgOut());
  dbgOut() << "\n";

  dbgOut() << "  normalized B: ";
  normalizedRhs->print(dbgOut());
  dbgOut() << "\n";

  dbgOut() << "  normalized C: ";
  normalizedAcc->print(dbgOut());
  dbgOut() << "\n";

  // ---------------------------------------------------------------------
  // Logical coordinates from the subscripts (docs/AFFINE_GEMM_MAPPING.md,
  // section 2). No position is assumed: the operands may have any rank, a
  // role may sit in any dimension (a transposed operand), a dimension whose
  // subscript is constant on the iteration domain is an extent-one dimension,
  // and a role no loop drives has extent one.
  //
  //  1. Every subscript's domain form (a loop that runs once is a constant).
  //  2. Loops that occur together in one subscript belong to one coordinate:
  //     the connected components ("groups") of that relation. In a valid
  //     mapping each group is exactly one logical coordinate (Lemma 1).
  //  3. A group's role follows from the operands that read it: both factors
  //     and not C -> K; one factor and C -> I or J; all three -> a batch
  //     coordinate P; one operand only -> a broadcast (not lowered).
  //  4. Each role's two subscripts must address one coordinate, each operand
  //     through its own view s * g + o, s of either sign
  //     (matchSharedCoordinate).
  //
  // The checkpoint's positional form is tried first -- A = (P*, I, K),
  // B = (P*, K, J), C = (P*, I, J) in that order, the factors either way --
  // so every nest it recognized keeps its logical mapping, views and
  // diagnostics (a coupled or singular mapping stays recognized, and the dense
  // partition refuses it). The analysis by groups covers what that form
  // cannot express: transposed operands, extent-one roles, other ranks.
  // ---------------------------------------------------------------------
  const unsigned numIterationDims = normalizedAcc->getNumDims();
  MLIRContext *mapContext = candidate.anchor->getContext();
  // Operand 0 and 1 are the two factors in the order of the product, 2 is C.
  const std::array<AffineMap, 3> operandMap{*normalizedLhs, *normalizedRhs,
                                            *normalizedAcc};
  static constexpr const char *kOperandName[] = {"the first factor",
                                                 "the second factor", "C"};
  struct Subscript {
    unsigned operand, dim;
    AffineExpr expr;
    LinearForm onDomain;
  };
  SmallVector<Subscript> subscripts;
  for (unsigned operand = 0; operand < 3; ++operand)
    for (unsigned dim = 0; dim < operandMap[operand].getNumResults(); ++dim) {
      AffineExpr expr = operandMap[operand].getResult(dim);
      auto form = extractLinearForm(expr, numIterationDims);
      if (failed(form))
        return unknown(llvm::Twine("subscript ") + llvm::Twine(dim) + " of " +
                       kOperandName[operand] +
                       " is not a linear form of the loop counters (mod, "
                       "floordiv, ceildiv, or a coefficient beyond 64 bits)");
      std::optional<LinearForm> onDomain = domainForm(*form, single);
      if (!onDomain)
        return unknown(llvm::Twine("subscript ") + llvm::Twine(dim) + " of " +
                       kOperandName[operand] +
                       "'s value on the iteration domain overflows 64-bit "
                       "arithmetic");
      subscripts.push_back({operand, dim, expr, *onDomain});
    }

  auto exprText = [](AffineExpr expr) {
    std::string text;
    llvm::raw_string_ostream os(text);
    expr.print(os);
    return text;
  };

  // The result, from either analysis.
  unsigned numPartitionDims = 0;
  affine::AffineLoadOp aLoad = lhsLoad;
  affine::AffineLoadOp bLoad = rhsLoad;
  AffineMap accessA = operandMap[0];
  AffineMap accessB = operandMap[1];
  std::array<SharedCoordinate, 3> coordinate;
  OperandLayout layoutA, layoutB, layoutC;
  SmallVector<AffineExpr> logicalPartitions;

  // 1. The positional form (the checkpoint's rule, with views of either sign).
  auto positional = [&]() -> bool {
    const unsigned rank = operandMap[2].getNumResults();
    if (rank < 2 || operandMap[0].getNumResults() != rank ||
        operandMap[1].getNumResults() != rank)
      return false;
    const unsigned prefix = rank - 2;
    for (auto [a, b] : {std::pair{0u, 1u}, std::pair{1u, 0u}}) {
      AffineMap ma = operandMap[a], mb = operandMap[b], mc = operandMap[2];
      bool samePrefix = true;
      for (unsigned p = 0; p < prefix; ++p)
        samePrefix &= ma.getResult(p) == mc.getResult(p) &&
                      mb.getResult(p) == mc.getResult(p);
      if (!samePrefix)
        continue;
      std::string ignored;
      auto i = matchSharedCoordinate(ma.getResult(prefix), mc.getResult(prefix),
                                     numIterationDims, single, ignored);
      auto j = matchSharedCoordinate(mb.getResult(prefix + 1),
                                     mc.getResult(prefix + 1), numIterationDims,
                                     single, ignored);
      auto k = matchSharedCoordinate(ma.getResult(prefix + 1),
                                     mb.getResult(prefix), numIterationDims,
                                     single, ignored);
      if (!i || !j || !k || i->expr == j->expr || i->expr == k->expr ||
          j->expr == k->expr)
        continue;
      numPartitionDims = prefix;
      aLoad = a == 0 ? lhsLoad : rhsLoad;
      bLoad = a == 0 ? rhsLoad : lhsLoad;
      accessA = ma;
      accessB = mb;
      coordinate = {*i, *j, *k};
      OperandLayout canonical;
      canonical.roleDim = {int(prefix), int(prefix + 1)};
      canonical.constantIndex.assign(rank, std::nullopt);
      for (unsigned p = 0; p < prefix; ++p)
        canonical.batchDim.push_back(p);
      layoutA = layoutB = layoutC = canonical;
      for (unsigned p = 0; p < prefix; ++p)
        logicalPartitions.push_back(ma.getResult(p));
      return true;
    }
    return false;
  };

  // 2. Otherwise the coordinates by groups and roles.
  if (!positional()) {
  // Groups: union-find over the loops, joined by every subscript's support.
  SmallVector<unsigned> parentLoop(numIterationDims);
  std::iota(parentLoop.begin(), parentLoop.end(), 0u);
  auto findLoop = [&](unsigned loop) {
    while (parentLoop[loop] != loop)
      loop = parentLoop[loop] = parentLoop[parentLoop[loop]];
    return loop;
  };
  for (const Subscript &sub : subscripts) {
    std::optional<unsigned> first;
    for (unsigned d = 0; d < numIterationDims; ++d) {
      if (sub.onDomain.coefficients[d] == 0)
        continue;
      if (!first)
        first = findLoop(d);
      else
        parentLoop[findLoop(d)] = *first;
    }
  }
  // The group of each subscript (-1: constant on the domain), groups in the
  // order their first subscript appears.
  SmallVector<unsigned> groupRoot;
  SmallVector<int> groupOf(subscripts.size(), -1);
  for (auto [index, sub] : llvm::enumerate(subscripts)) {
    for (unsigned d = 0; d < numIterationDims; ++d) {
      if (sub.onDomain.coefficients[d] == 0)
        continue;
      unsigned root = findLoop(d);
      auto it = llvm::find(groupRoot, root);
      groupOf[index] = it - groupRoot.begin();
      if (it == groupRoot.end())
        groupRoot.push_back(root);
      break;
    }
  }
  const unsigned numGroups = groupRoot.size();

  // Who reads each group, and in which dimension.
  SmallVector<std::array<int, 3>> readerDim(numGroups, {-1, -1, -1});
  for (auto [index, sub] : llvm::enumerate(subscripts)) {
    if (groupOf[index] < 0)
      continue;
    int &dim = readerDim[groupOf[index]][sub.operand];
    if (dim >= 0)
      return refuse(llvm::Twine(kOperandName[sub.operand]) +
                    " reads one logical coordinate in two of its dimensions "
                    "(" + llvm::Twine(dim) + " and " + llvm::Twine(sub.dim) +
                    "): two coordinates would share a loop, so the points "
                    "are not a box (Theorem 3, S1)");
    dim = sub.dim;
  }

  // Roles. With the factors as written: a group read by factor f and C is
  // "R_f"; by both factors, K; by all three, P (batch).
  SmallVector<unsigned> groupsWithFirst, groupsWithSecond, reductionGroups,
      batchGroups;
  for (unsigned g = 0; g < numGroups; ++g) {
    const bool f0 = readerDim[g][0] >= 0, f1 = readerDim[g][1] >= 0,
               c = readerDim[g][2] >= 0;
    if (f0 && f1 && c)
      batchGroups.push_back(g);
    else if (f0 && f1)
      reductionGroups.push_back(g);
    else if (f0 && c)
      groupsWithFirst.push_back(g);
    else if (f1 && c)
      groupsWithSecond.push_back(g);
    else
      return unsupported(
          llvm::Twine("a coordinate is read by ") +
          kOperandName[f0 ? 0 : f1 ? 1 : 2] +
          " only: the other operands would be broadcast along it (a "
          "stride-0 view, not lowered: docs/AFFINE_GEMM_MAPPING.md section 10)");
  }
  // A batch coordinate the positional form did not recognize (one not in
  // the leading dimensions of all three operands): a batched GEMM, lowered
  // with the batch loops around the tiles (ISSUE-006) when the three
  // operands read the coordinate through one and the same expression on the
  // domain -- the batch index is computed once per batch iteration and
  // addresses the same slice of each operand. An element-wise product (every
  // coordinate a batch one, M = N = K = 1) is a GEMM of 1 x 1 x 1 tiles in
  // name only: refused.
  if (!batchGroups.empty() && groupsWithFirst.empty() &&
      groupsWithSecond.empty() && reductionGroups.empty())
    return refuse(llvm::Twine(batchGroups.size()) +
                  " coordinate(s) are read by all three operands and none by "
                  "two: an element-wise product (a batched GEMM with "
                  "M = N = K = 1), not a matrix multiplication");
  auto domainFormOf = [&](unsigned operand, int dim) -> const LinearForm & {
    for (const Subscript &sub : subscripts)
      if (sub.operand == operand && int(sub.dim) == dim)
        return sub.onDomain;
    llvm_unreachable("every (operand, dimension) has a subscript");
  };
  for (unsigned g : batchGroups) {
    const LinearForm &reference = domainFormOf(2, readerDim[g][2]);
    for (unsigned operand = 0; operand < 2; ++operand) {
      const LinearForm &other = domainFormOf(operand, readerDim[g][operand]);
      if (other.coefficients != reference.coefficients ||
          other.constant != reference.constant)
        return unsupported(
            "a batch coordinate is read through different expressions by C "
            "and " + llvm::Twine(kOperandName[operand]) +
            " (one per-operand view of a batch coordinate is not lowered)");
    }
  }
  for (auto [groups, role] :
       {std::pair{&groupsWithFirst, "the first factor and C"},
        std::pair{&groupsWithSecond, "the second factor and C"},
        std::pair{&reductionGroups, "both factors (the reduction)"}})
    if (groups->size() > 1)
      return unsupported(llvm::Twine(groups->size()) +
                         " separate coordinates are read by " + role +
                         ": one GEMM role over several coordinates needs a "
                         "merged (reshaped) view, not lowered");

  // Which factor is A. A = (I, K), B = (K, J), C = (I, J): the factor that
  // shares a coordinate with C... both do; prefer the assignment that
  // transposes nothing (C first), then the factors as written.
  auto dimOf = [&](ArrayRef<unsigned> groups, unsigned operand) -> int {
    return groups.empty() ? -1 : readerDim[groups.front()][operand];
  };
  struct Assignment {
    unsigned a, b;                 // operand index of A and B
    SmallVector<unsigned> iGroups, jGroups;
    int cost = 0;
  };
  auto assignment = [&](unsigned a) {
    Assignment as;
    as.a = a;
    as.b = 1 - a;
    as.iGroups = a == 0 ? groupsWithFirst : groupsWithSecond;
    as.jGroups = a == 0 ? groupsWithSecond : groupsWithFirst;
    auto transposed = [](int first, int second) {
      return first >= 0 && second >= 0 && first > second;
    };
    as.cost = 2 * transposed(dimOf(as.iGroups, 2), dimOf(as.jGroups, 2)) +
              transposed(dimOf(as.iGroups, as.a), dimOf(reductionGroups, as.a)) +
              transposed(dimOf(reductionGroups, as.b), dimOf(as.jGroups, as.b));
    return as;
  };
  Assignment asWritten = assignment(0), swapped = assignment(1);
  const Assignment &chosen = swapped.cost < asWritten.cost ? swapped : asWritten;

  aLoad = chosen.a == 0 ? lhsLoad : rhsLoad;
  bLoad = chosen.a == 0 ? rhsLoad : lhsLoad;
  accessA = operandMap[chosen.a];
  accessB = operandMap[chosen.b];

  // Each role: its group (if any), and the dimension each of its two
  // operands reads it in. The roles in the order of the views: I by (A, C),
  // J by (B, C), K by (A, B).
  struct Role {
    const char *name;
    std::optional<unsigned> group;
    unsigned first, second;        // the two operands that read it
  };
  std::array<Role, 3> roles{
      Role{"I", chosen.iGroups.empty() ? std::nullopt
                                       : std::optional(chosen.iGroups.front()),
           chosen.a, 2},
      Role{"J", chosen.jGroups.empty() ? std::nullopt
                                       : std::optional(chosen.jGroups.front()),
           chosen.b, 2},
      Role{"K", reductionGroups.empty()
                    ? std::nullopt
                    : std::optional(reductionGroups.front()),
           chosen.a, chosen.b}};

  // Every operand's layout: the dimension of each of its two GEMM roles, and
  // the constant index of each other dimension. A role of extent one (no
  // group) takes, when the operand has one, a constant dimension in the
  // position that keeps the operand untransposed; otherwise the operand has
  // no dimension for it (its view gets a unit dimension).
  auto constantIndexOf = [&](unsigned operand, unsigned dim) -> int64_t {
    for (const Subscript &sub : subscripts)
      if (sub.operand == operand && sub.dim == dim)
        return sub.onDomain.constant;
    return 0;
  };
  auto layoutOf = [&](unsigned operand, std::optional<unsigned> firstGroup,
                      std::optional<unsigned> secondGroup) {
    OperandLayout layout;
    const unsigned rank = operandMap[operand].getNumResults();
    layout.roleDim = {firstGroup ? readerDim[*firstGroup][operand] : -1,
                      secondGroup ? readerDim[*secondGroup][operand] : -1};
    SmallVector<bool> isConstant(rank, true);
    for (int dim : layout.roleDim)
      if (dim >= 0)
        isConstant[dim] = false;
    for (unsigned p : batchGroups) {
      isConstant[readerDim[p][operand]] = false;
      layout.batchDim.push_back(readerDim[p][operand]);
    }
    // A role of extent one takes a constant dimension: the last one before
    // the other role's dimension (role 0), the first one after it (role 1).
    for (unsigned r = 0; r < 2; ++r) {
      if (layout.roleDim[r] >= 0)
        continue;
      const int other = layout.roleDim[1 - r];
      int pick = -1;
      for (unsigned dim = 0; dim < rank; ++dim) {
        if (!isConstant[dim])
          continue;
        const bool inOrder =
            other < 0 || (r == 0 ? int(dim) < other : int(dim) > other);
        if (inOrder && (pick < 0 || r == 0))
          pick = dim;          // role 0: the last such, role 1: the first
        if (inOrder && r == 1)
          break;
      }
      if (pick < 0)
        for (unsigned dim = 0; dim < rank && pick < 0; ++dim)
          if (isConstant[dim])
            pick = dim;
      if (pick >= 0) {
        layout.roleDim[r] = pick;
        isConstant[pick] = false;
      }
    }
    layout.constantIndex.assign(rank, std::nullopt);
    for (unsigned dim = 0; dim < rank; ++dim)
      if (isConstant[dim])
        layout.constantIndex[dim] = constantIndexOf(operand, dim);
    return layout;
  };
  layoutA = layoutOf(chosen.a, roles[0].group, roles[2].group);
  layoutB = layoutOf(chosen.b, roles[2].group, roles[1].group);
  layoutC = layoutOf(2, roles[0].group, roles[1].group);

  // The coordinates. A role with a group: matchSharedCoordinate of its two
  // subscripts. A role of extent one: the constant 0; each operand's view of
  // it is its constant index (or nothing).
  for (auto [r, role] : llvm::enumerate(roles)) {
    const OperandLayout &firstLayout =
        r == 0 ? layoutA : r == 1 ? layoutB : layoutA;
    const OperandLayout &secondLayout =
        r == 0 ? layoutC : r == 1 ? layoutC : layoutB;
    const int firstDim = firstLayout.roleDim[r == 1 ? 1 : r == 0 ? 0 : 1];
    const int secondDim = secondLayout.roleDim[r == 2 ? 0 : r == 0 ? 0 : 1];
    if (!role.group) {
      auto constantView = [&](unsigned operand, int dim) {
        return OperandCoordinateView{
            1, dim >= 0 ? constantIndexOf(operand, dim) : 0};
      };
      coordinate[r] = SharedCoordinate{
          getAffineConstantExpr(0, mapContext),
          constantView(role.first, firstDim),
          constantView(role.second, secondDim)};
      continue;
    }
    AffineExpr x = operandMap[role.first].getResult(firstDim);
    AffineExpr y = operandMap[role.second].getResult(secondDim);
    std::string reason;
    auto shared = matchSharedCoordinate(x, y, numIterationDims, single, reason);
    if (!shared)
      return unknown(llvm::Twine("logical coordinate ") + role.name + " (" +
                     kOperandName[role.first] + "'s dimension " +
                     llvm::Twine(firstDim) + " " + exprText(x) + ", " +
                     kOperandName[role.second] + "'s dimension " +
                     llvm::Twine(secondDim) + " " + exprText(y) + "): " +
                     reason + " -- operand-specific decompositions of one "
                     "coordinate are not lowered (spec question SQ-1)");
    coordinate[r] = *shared;
  }

  // Batch coordinates: read by all three operands; the same coordinate in
  // each (any views). They keep the partition prefix of the logical mapping
  // y = (P*, I, J, K); the dense partition refuses a batched GEMM.
  for (unsigned p : batchGroups) {
    AffineExpr reference = operandMap[2].getResult(readerDim[p][2]);
    for (unsigned operand = 0; operand < 2; ++operand) {
      AffineExpr other = operandMap[operand].getResult(readerDim[p][operand]);
      std::string reason;
      if (!matchSharedCoordinate(reference, other, numIterationDims, single,
                                 reason))
        return unknown("a batch coordinate is read differently by C and " +
                       llvm::Twine(kOperandName[operand]) + ": " + reason);
    }
    logicalPartitions.push_back(reference);
  }
  numPartitionDims = logicalPartitions.size();
  } // the analysis by groups

  AffineExpr logicalI = coordinate[0].expr;
  AffineExpr logicalJ = coordinate[1].expr;
  AffineExpr logicalK = coordinate[2].expr;

  // Each operand's view of its coordinates: A = (I, K), B = (K, J), C = (I, J).
  // matchSharedCoordinate's `first` is the first expression it was given.
  const std::array<OperandCoordinateView, 2> viewA{coordinate[0].first,
                                                   coordinate[2].first};
  const std::array<OperandCoordinateView, 2> viewB{coordinate[2].second,
                                                   coordinate[1].first};
  const std::array<OperandCoordinateView, 2> viewC{coordinate[0].second,
                                                   coordinate[1].second};

  auto linearI = extractLinearForm(logicalI, numIterationDims);
  auto linearJ = extractLinearForm(logicalJ, numIterationDims);
  auto linearK = extractLinearForm(logicalK, numIterationDims);

  if (failed(linearI) ||
      failed(linearJ) ||
      failed(linearK))
    return unknown("a logical coordinate is not a linear form of the loop "
                   "counters");

  // Every operand subscript of a role must be exactly its view of the
  // coordinate, scale * y + offset with scale != 0, on the iteration domain
  // (as domain forms: a loop that runs once is a constant) -- the property
  // the lowering builds each operand's strided view from. matchSharedCoordinate
  // constructs the views so that this holds; it is checked here, in
  // 64-bit-checked arithmetic, so that a nest for which it would not hold (or
  // would hold only after wrapping around) is refused instead of lowered
  // through a wrong view. A dimension that a role of extent one took holds
  // its constant (scale * 0 + offset).
  auto subscriptIsView = [&](AffineExpr subscript, const LinearForm &coordinate,
                             const OperandCoordinateView &view) {
    if (view.scale == 0)
      return false;
    auto expression = extractLinearForm(subscript, numIterationDims);
    if (failed(expression))
      return false;
    std::optional<LinearForm> form = domainForm(*expression, single);
    std::optional<LinearForm> onDomain = domainForm(coordinate, single);
    if (!form || !onDomain)
      return false;
    for (unsigned d = 0; d < numIterationDims; ++d) {
      std::optional<int64_t> expected =
          llvm::checkedMul(view.scale, onDomain->coefficients[d]);
      if (!expected || *expected != form->coefficients[d])
        return false;
    }
    std::optional<int64_t> scaled =
        llvm::checkedMul(view.scale, onDomain->constant);
    std::optional<int64_t> expected =
        scaled ? llvm::checkedAdd(*scaled, view.offset) : std::nullopt;
    return expected && *expected == form->constant;
  };
  auto operandIsView = [&](AffineMap access, const OperandLayout &layout,
                           const LinearForm &first, const LinearForm &second,
                           const std::array<OperandCoordinateView, 2> &view) {
    for (unsigned r = 0; r < 2; ++r)
      if (layout.roleDim[r] >= 0 &&
          !subscriptIsView(access.getResult(layout.roleDim[r]),
                           r == 0 ? first : second, view[r]))
        return false;
    return true;
  };
  if (!operandIsView(accessA, layoutA, *linearI, *linearK, viewA) ||
      !operandIsView(accessB, layoutB, *linearK, *linearJ, viewB) ||
      !operandIsView(*normalizedAcc, layoutC, *linearI, *linearJ, viewC))
    return unknown("an operand's subscript is not its view of the logical "
                   "coordinate on the iteration domain (or the check "
                   "overflows 64-bit arithmetic)");


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
  result.layoutA = layoutA;
  result.layoutB = layoutB;
  result.layoutC = layoutC;
  result.reductionMayReassociate = arith::bitEnumContainsAll(
      add.getFastmath(), arith::FastMathFlags::reassoc);

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


  // Materialize the complete affine coordinate transformation
  //
  //   y = T x + c
  //
  // where y = (P*, I, J, K).
  for (AffineExpr logicalPartition : logicalPartitions) {
    auto linearPartition =
        extractLinearForm(logicalPartition, numIterationDims);
    if (failed(linearPartition))
      return refuse("a partition (batch) subscript is not a linear form of "
                    "the loop counters");

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
                                  result.normalizationEmptyLoop,
                                  result.normalizationDetail);
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
