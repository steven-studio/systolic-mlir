#include "Systolic/AffineMatmulAnalysis.h"
#include "Systolic/AffineMatmulPartitioning.h"

#include "mlir/Analysis/Presburger/Matrix.h"
#include "mlir/Analysis/Presburger/PresburgerRelation.h"
#include "mlir/Dialect/Affine/Analysis/AffineStructures.h"
#include "mlir/Dialect/Affine/LoopUtils.h"
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
  if (accLoad) // none for an assignment C = A * B
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

static std::optional<int64_t> constantLoopBound(affine::AffineForOp loop,
                                                bool lower);

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

    // A stepped loop whose lower bound is the max of constants (P0.6):
    // addAffineForOpDomain adds the stride only for a lower bound of one
    // constant, and otherwise approximates the domain by every integer
    // between the bounds -- a superset the regions, which hold the loop's
    // iterations only, need not cover (checkSourceCover then refused the
    // nest, e.g. when such a loop and another drive one coordinate). The
    // folded bound gives the stride, (x - lb) mod step = 0: the domain of
    // docs/AFFINE_GEMM_MAPPING.md section 1, x = lb + step * z.
    const int64_t step = loop.getStepAsInt();
    if (step == 1 || loop.hasConstantLowerBound())
      continue;
    std::optional<int64_t> lower = constantLoopBound(loop, /*lower=*/true);
    if (!lower)
      continue;
    std::optional<int64_t> negatedLower =
        llvm::checkedSub(int64_t(0), *lower);
    unsigned pos;
    if (!negatedLower || !domain.findVar(iv, &pos))
      return failure();
    // q = (x - lb) floordiv step, then x - lb - step * q = 0.
    SmallVector<int64_t, 8> dividend(domain.getNumCols(), 0);
    dividend[pos] = 1;
    dividend.back() = *negatedLower;
    const unsigned qPos = domain.addLocalFloorDiv(dividend, step);
    SmallVector<int64_t, 8> equality(domain.getNumCols(), 0);
    equality[pos] = 1;
    equality.back() = *negatedLower;
    equality[qPos] = -step;
    domain.addEquality(equality);
  }

  return domain;
}

/// The symbol-only terms of an access map, separated from the loop terms:
/// each result e of `map` must be e = d(dims) + s(symbols) + c, and the
/// returned map has results s (over the same symbols, no dims) while
/// `stripped` gets d + c (over the dims, no symbols). Failure when a result
/// is not of that form (a product of a symbol and a loop counter, a floordiv
/// or a mod involving a symbol): there is no view offset that stands for it.
/// The symbol part must be a sum of products: a floordiv, ceildiv or mod of
/// symbols (s0 floordiv 2; simplification also makes one of
/// (4 * d0 + s0) floordiv 4) splits off cleanly but is no run-time offset
/// the lowering builds (materializeOffset in AffineMatmulToSystolic.cpp),
/// so the access stays UNKNOWN, as a mod/div subscript does
/// (docs/AFFINE_GEMM_MAPPING.md section 10).
static FailureOr<AffineMap> separateSymbols(AffineMap map,
                                            AffineMap &stripped) {
  MLIRContext *context = map.getContext();
  const unsigned nd = map.getNumDims(), ns = map.getNumSymbols();
  AffineExpr zero = getAffineConstantExpr(0, context);
  SmallVector<AffineExpr> zeroDims(nd, zero), zeroSyms(ns, zero);
  SmallVector<AffineExpr> dimParts, symParts;
  for (AffineExpr e : map.getResults()) {
    AffineExpr dimPart = simplifyAffineExpr(
        e.replaceDimsAndSymbols(/*dimReplacements=*/{}, zeroSyms), nd, ns);
    AffineExpr symPlusConst = simplifyAffineExpr(
        e.replaceDimsAndSymbols(zeroDims, /*symReplacements=*/{}), nd, ns);
    AffineExpr constPart =
        simplifyAffineExpr(e.replaceDimsAndSymbols(zeroDims, zeroSyms), nd, ns);
    AffineExpr symPart = simplifyAffineExpr(symPlusConst - constPart, nd, ns);
    if (simplifyAffineExpr(dimPart + symPart, nd, ns) !=
        simplifyAffineExpr(e, nd, ns))
      return failure();
    bool divOrMod = false;
    symPart.walk([&](AffineExpr sub) {
      divOrMod |= sub.getKind() == AffineExprKind::FloorDiv ||
                  sub.getKind() == AffineExprKind::CeilDiv ||
                  sub.getKind() == AffineExprKind::Mod;
    });
    if (divOrMod)
      return failure();
    dimParts.push_back(dimPart);
    symParts.push_back(symPart);
  }
  stripped = AffineMap::get(nd, 0, dimParts, context);
  return AffineMap::get(0, ns, symParts, context);
}

static bool hasSymbolTerm(AffineMap symbolic) {
  if (!symbolic)
    return false;
  for (AffineExpr e : symbolic.getResults())
    if (!isa<AffineConstantExpr>(e) || cast<AffineConstantExpr>(e).getValue() != 0)
      return true;
  return false;
}

/// The loop part of an access, over the common iteration domain; the
/// symbol part, if any, in `symbolic` (over `symbols`, the map's symbol
/// operands), else a null map. Symbols -- the nest's parameters -- are
/// additive offsets of the access, separated off here and carried as the
/// operand's run-time offset (LogicalMatmul::symbolicA/B/C); the recognition
/// sees the loop part alone.
static FailureOr<AffineMap>
normalizeAccessMap(AffineMap map,
                   ValueRange operands,
                   ValueRange iterationIVs,
                   AffineMap &symbolic,
                   SmallVector<Value> &symbols) {
  MLIRContext *context = map.getContext();

  SmallVector<AffineExpr> dimReplacements;
  dimReplacements.reserve(map.getNumDims());

  // Rewrite each map-local dimension into a common iteration domain:
  //
  //   iterationIVs[p] -> common dp
  //
  // The common domain may have any number of access-related loop IVs.
  if (operands.size() != map.getNumDims() + map.getNumSymbols())
    return failure();
  for (Value operand : operands.take_front(map.getNumDims())) {
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

  symbolic = AffineMap();
  symbols.clear();
  AffineMap loopPart = map;
  if (map.getNumSymbols() != 0) {
    FailureOr<AffineMap> sym = separateSymbols(map, loopPart);
    if (failed(sym))
      return failure();
    if (hasSymbolTerm(*sym)) {
      symbolic = *sym;
      symbols.assign(operands.begin() + map.getNumDims(), operands.end());
    }
  }

  return loopPart.replaceDimsAndSymbols(
      dimReplacements,
      /*symReplacements=*/{},
      /*numResultDims=*/iterationIVs.size(),
      /*numResultSyms=*/0);
}

/// The loop part alone (the symbol part, if any, discarded): for analyses
/// of the loop geometry.
static FailureOr<AffineMap>
normalizeAccessMap(AffineMap map,
                   ValueRange operands,
                   ValueRange iterationIVs) {
  AffineMap symbolic;
  SmallVector<Value> symbols;
  return normalizeAccessMap(map, operands, iterationIVs, symbolic, symbols);
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
    // is a candidate only when its innermost body stores an arith.addf (an
    // accumulation C = C + A * B) or an arith.mulf (an assignment C = A * B,
    // the K = 1 GEMM into a zeroed C, section 20) -- the three-deep rule below
    // is unchanged.
    auto storesSum = [](affine::AffineForOp loop) {
      unsigned storesOfSum = 0;
      for (Operation &op : *loop.getBody()) {
        if (isa<affine::AffineForOp>(op))
          return false;
        if (auto store = dyn_cast<affine::AffineStoreOp>(op))
          storesOfSum +=
              store.getValue().getDefiningOp<arith::AddFOp>() != nullptr ||
              store.getValue().getDefiningOp<arith::MulFOp>() != nullptr;
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

// One recognition. With reduction-batch partitioning (docs section 23)
// `keepK` names the loop, read by both factors and not by C, that stays K
// while the other such loops become reduction batch loops, and
// `sharedLoops` receives those loops (the candidates for keepK).
static FailureOr<LogicalMatmul>
recognizeOnce(AffineMatmulCandidate candidate, std::string *why,
              GemmMappingClass *mappingClass,
              const AffineMatmulRecognitionOptions &options,
              std::optional<unsigned> keepK,
              SmallVector<unsigned> *sharedLoops) {
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
      "affine.store) nor one assignment C = A * B (two affine.load, one "
      "arith.mulf, one affine.store)";
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

  // The two statement forms (docs/AFFINE_GEMM_MAPPING.md sections 2, 20):
  //
  //   accumulation  C = C + A * B    A load, B load, C load, C store
  //   assignment    C = A * B        A load, B load, C store
  //
  // An assignment has no accumulator: it is the K = 1 GEMM into a zeroed C
  // when every element of C is written exactly once on the iteration domain
  // (checked once the subscripts are known, below, and proven exactly by the
  // lowering); the lowering zeroes C's window before the tiles.
  //
  // This is only the first recognition checkpoint.  We have not yet
  // established multiply-accumulate semantics or recovered logical I/J/K.
  if (stores.size() != 1 || (loads.size() != 3 && loads.size() != 2))
    return refuse(notOneMac);

  affine::AffineStoreOp store = stores.front();
  const bool accumulates = loads.size() == 3;

  // Follow the SSA def-use chain backward from the stored value.
  affine::AffineLoadOp accLoad;
  arith::MulFOp mul;
  arith::AddFOp add;
  if (accumulates) {
    add = store.getValue().getDefiningOp<arith::AddFOp>();
    if (!add)
      return refuse("the stored value is not an arith.addf");

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
  } else {
    // Two loads: the stored value must be their product and nothing else
    // (a sum with a constant, a negation or a second product in the body is
    // not C = A * B).
    mul = store.getValue().getDefiningOp<arith::MulFOp>();
    if (!mul)
      return refuse("the stored value is not an arith.addf of the "
                    "accumulator and a product (C = C + A * B) nor an "
                    "arith.mulf of two loads (C = A * B)");
    for (Operation &op : *chain.back().getBody())
      if (isa<arith::AddFOp>(op) ||
          (isa<arith::MulFOp>(op) && &op != mul.getOperation()))
        return refuse("the body of an assignment C = A * B holds an "
                      "arith.addf or a second arith.mulf besides the product");
  }

  auto lhsLoad =
      mul.getLhs().getDefiningOp<affine::AffineLoadOp>();
  auto rhsLoad =
      mul.getRhs().getDefiningOp<affine::AffineLoadOp>();

  if (!lhsLoad || !rhsLoad)
    return refuse("a factor of the product is not an affine.load");
  // Three different loads: the accumulator's and one per factor. (A factor
  // that is the accumulator's own load, or one load used as both factors,
  // is not C += A * B.) An assignment: one load per factor.
  if (lhsLoad == rhsLoad || lhsLoad == accLoad || rhsLoad == accLoad)
    return refuse(accumulates ? "the two factors and the accumulator are not "
                                "three different loads"
                              : "the two factors are not two different loads");
  // (An assignment that stores to a memref it also reads as a factor is
  // decided by the lowering's aliasing rule, as the accumulation form is:
  // lowered when the factor's window is provably disjoint from C's, UNKNOWN
  // otherwise -- later products may read elements the nest wrote.)

  auto iterationIVs =
      collectAccessIterationIVs(lhsLoad, rhsLoad, accLoad, store);

  // A chain loop that runs exactly once is a constant on the domain (Lemma 0)
  // whether or not an access names its counter: an iteration dimension with
  // one value, not a repetition (an all-constant nest of such loops is the
  // 1 x 1 x 1 GEMM, SQ-4; review of 2026-10-18). Appended after the IVs the
  // accesses name, in chain order.
  for (affine::AffineForOp loop : chain) {
    Value iv = loop.getInductionVar();
    if (!llvm::is_contained(iterationIVs, iv) && singleIterationValue(iv))
      iterationIVs.push_back(iv);
  }

  dbgOut() << "  access iteration IV count: "
               << iterationIVs.size() << "\n";

  // The iteration space is exactly the anchored chain. A chain loop whose IV
  // no access uses repeats the accumulation (C += A*B once per iteration),
  // which a single GEMM over the used IVs would not; an IV from outside the
  // chain is a parameter of the nest, not one of its iteration dimensions.
  // The lowering pass first moves such a loop outward when that is a legal
  // interchange (AffineMatmulToSystolic.cpp, hoistRepetitionLoops); a nest
  // that still has one here is not analysed further, so nothing is decided
  // about its mapping: UNKNOWN (the specification lists a repetition loop
  // inside a reduction loop as a documented limitation, README section 2).
  for (auto [position, loop] : llvm::enumerate(chain))
    if (!llvm::is_contained(iterationIVs, loop.getInductionVar()))
      return unknown("loop " + llvm::Twine(position) +
                     " of the nest is used by no access (it repeats the "
                     "accumulation; moved outward by the lowering pass when "
                     "that is a legal interchange -- here it was not, so the "
                     "mapping is not decided)");
  // A counter from outside the chain is a parameter of this candidate: the
  // band may be a GEMM at each of its values (the inner band of a batched
  // GEMM, whose enclosing candidate holds that loop), which nothing here
  // decides -- UNKNOWN, not PROVEN_INVALID (review of 2026-10-19).
  if (iterationIVs.size() != chain.size())
    return unknown("an access uses the counter of a loop outside the nest");

  auto sourceDomain = recoverSourceDomain(iterationIVs);
  if (failed(sourceDomain))
    return unknown("the iteration domain of the loops cannot be represented "
                  "(a bound outside the affine constraint system)");

  // The loops that run exactly once, by iteration dimension: their IV is a
  // constant on the iteration domain D.
  SingleValues single;
  for (Value iv : iterationIVs)
    single.push_back(singleIterationValue(iv));

  // The shape of the domain. A box: every bound a constant (a symbol or
  // another loop's counter in a bound makes the domain a non-box, docs/
  // AFFINE_GEMM_MAPPING.md section 19.7). Empty: a loop with constant bounds
  // and no iteration (the dense partition reports it, SQ-2).
  bool boxDomain = true, emptyDomain = false;
  for (Value iv : iterationIVs) {
    affine::AffineForOp loop = affine::getForInductionVarOwner(iv);
    std::optional<int64_t> lower =
        loop ? constantLoopBound(loop, /*lower=*/true) : std::nullopt;
    std::optional<int64_t> upper =
        loop ? constantLoopBound(loop, /*lower=*/false) : std::nullopt;
    if (!lower || !upper)
      boxDomain = false;
    else if (*upper <= *lower)
      emptyDomain = true;
  }

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
  // maps are normalized) subscripts with identical domain forms. An
  // assignment has no accumulator load: C's access is the store's.
  if (accumulates && accLoad.getMemref() != store.getMemref())
    return refuse("the accumulator is loaded from one memref and stored to "
                  "another");
  const bool sameAccumulatorAccess =
      !accumulates || (accLoad.getMap() == store.getMap() &&
                       llvm::equal(accLoad.getIndices(), store.getIndices()));
  // C's access map and indices: the accumulator load's (the store's agree on
  // the domain, checked below) or, for an assignment, the store's.
  const AffineMap cAccessMap = accumulates ? accLoad.getMap() : store.getMap();
  const ValueRange cAccessIndices =
      accumulates ? accLoad.getIndices() : store.getIndices();

  // Recover canonical logical GEMM coordinates from access operands:
  //
  //   A = (I, K)
  //   B = (K, J)
  //   C = (I, J)
  //
  // The loop part of each access; its symbol part (the nest's parameters as
  // additive offsets, a dynamic offset of the operand's view) is separated
  // off and carried into the result below.
  AffineMap symbolicLhs, symbolicRhs, symbolicAcc;
  SmallVector<Value> symbolsLhs, symbolsRhs, symbolsAcc;
  auto normalizedLhs =
      normalizeAccessMap(lhsLoad.getMap(), lhsLoad.getIndices(),
                         iterationIVs, symbolicLhs, symbolsLhs);
  auto normalizedRhs =
      normalizeAccessMap(rhsLoad.getMap(), rhsLoad.getIndices(),
                         iterationIVs, symbolicRhs, symbolsRhs);
  auto normalizedAcc =
      normalizeAccessMap(cAccessMap, cAccessIndices, iterationIVs,
                         symbolicAcc, symbolsAcc);

  if (failed(normalizedLhs) ||
      failed(normalizedRhs) ||
      failed(normalizedAcc))
    return unknown("an access index is not a counter of the nest's loops, or "
                  "an access map has a symbol that is not an additive offset");

  if (!sameAccumulatorAccess) {
    AffineMap symbolicStore;
    SmallVector<Value> symbolsStore;
    auto normalizedStore =
        normalizeAccessMap(store.getMap(), store.getIndices(), iterationIVs,
                           symbolicStore, symbolsStore);
    const char *const differentElements =
        "the accumulator is loaded and stored at different elements of C";
    if (failed(normalizedStore) ||
        normalizedStore->getNumResults() != normalizedAcc->getNumResults() ||
        symbolicStore != symbolicAcc || symbolsStore != symbolsAcc)
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

  // An assignment C = A * B writes each element of C exactly once only when
  // every loop of two or more iterations is a subscript of C on the domain
  // (docs/AFFINE_GEMM_MAPPING.md section 20): a loop C does not read -- a
  // reduction loop, or a loop the factors read -- writes the same elements
  // again at every iteration and only its last product survives: the result
  // is not the GEMM of A and B (not a GEMM at all). A loop that runs once is a
  // constant on the domain (Lemma 0) and writes nothing twice. Whether C's
  // subscripts are injective on the whole domain (batches overlapping in C)
  // is proven by the lowering (verifyOutputWrittenOnce).
  if (!accumulates)
    for (unsigned d = 0; d < numIterationDims; ++d) {
      if (single[d])
        continue;
      const bool readByC = llvm::any_of(subscripts, [&](const Subscript &s) {
        return s.operand == 2 && s.onDomain.coefficients[d] != 0;
      });
      if (readByC)
        continue;
      affine::AffineForOp loop =
          affine::getForInductionVarOwner(iterationIVs[d]);
      std::optional<int64_t> lower =
          loop ? constantLoopBound(loop, /*lower=*/true) : std::nullopt;
      std::optional<int64_t> upper =
          loop ? constantLoopBound(loop, /*lower=*/false) : std::nullopt;
      // A loop of no iteration (constant bounds, upper <= lower) writes
      // nothing: an empty GEMM, reported by the dense partition as such
      // (SQ-2), as for an accumulation.
      if (lower && upper && *upper <= *lower)
        continue;
      std::optional<int64_t> trips =
          lower && upper ? checkedTripCount(*lower, *upper,
                                            loop.getStep().getSExtValue())
                         : std::nullopt;
      if (!trips)
        return unknown(llvm::Twine("the assignment C = A * B does not read "
                                   "loop ") +
                       llvm::Twine(d) +
                       " in any subscript of C and the loop's trip count is "
                       "not a constant: whether it writes elements of C more "
                       "than once is not decided");
      return refuse(llvm::Twine("the statement overwrites C (C = A * B, no "
                                "accumulation) and loop ") +
                    llvm::Twine(d) +
                    " of the nest, which runs more than once, is not a "
                    "subscript of C: every iteration of it writes the same "
                    "elements of C again and only the last product survives "
                    "-- not the GEMM of A and B (an assignment is the K = 1 "
                    "GEMM into a zeroed C only when every element of C is "
                    "written once: docs/AFFINE_GEMM_MAPPING.md section 20)");
    }

  // The assignment policy (docs section 20.4, 2026-10-21): an assignment is
  // considered further only when (A) its actual reduction has one iteration
  // (K = 1), or (B) the index association dimension of its factors -- the
  // rank of the dot products P Q^T of their reduction index vectors, the
  // factors' rows on the loops of two or more iterations C does not read
  // (section 22) -- is 0 and it maps to the GEMM with a virtual K = 1. Any
  // other assignment is refused; an overwrite is never turned into an
  // accumulation, and reduction batch loops are never made for it. After the
  // check above every loop of two or more iterations is a subscript of C, so
  // this holds (no reduction loop: K = 1, dimension 0); the write semantics
  // -- every element of C written exactly once -- is proven by
  // verifyOutputWrittenOnce, and the lowering requires K = 1 again.
  if (!accumulates) {
    std::array<lattice::IntMatrix, 3> rows;
    SmallVector<int64_t> trips(numIterationDims, 2);
    for (const Subscript &sub : subscripts)
      rows[sub.operand].push_back(lattice::IntVector(
          sub.onDomain.coefficients.begin(), sub.onDomain.coefficients.end()));
    for (unsigned d = 0; d < numIterationDims; ++d) {
      if (single[d]) {
        trips[d] = 1;
        continue;
      }
      // A loop of no iteration writes nothing (an empty GEMM, as above).
      affine::AffineForOp loop =
          affine::getForInductionVarOwner(iterationIVs[d]);
      std::optional<int64_t> lower =
          loop ? constantLoopBound(loop, /*lower=*/true) : std::nullopt;
      std::optional<int64_t> upper =
          loop ? constantLoopBound(loop, /*lower=*/false) : std::nullopt;
      if (lower && upper && *upper <= *lower)
        trips[d] = 0;
    }
    std::optional<AffineR8Structure> association =
        computeR8Structure(rows, trips);
    if (!association)
      return unknown("the assignment C = A * B: the index association of its "
                     "factors overflows 64-bit arithmetic");
    if (!association->reductionLoops.empty() || association->coupling != 0)
      return refuse(llvm::Twine("the assignment C = A * B reduces over ") +
                    llvm::Twine(association->reductionLoops.size()) +
                    " loop(s) C does not read (index association dimension " +
                    llvm::Twine(association->coupling) +
                    "): an assignment is considered only with K = 1, or with "
                    "index association dimension 0 and a virtual K = 1 "
                    "(docs/AFFINE_GEMM_MAPPING.md section 20.4)");
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
  // The batch loops (ISSUE-007, docs/AFFINE_GEMM_MAPPING.md section 16): the
  // loops whose counter all three operands read on the iteration domain. The
  // positional form takes the loops of its partition prefix; the analysis by
  // groups classifies every loop by the operands that read it. A factor may
  // read a batch loop anywhere -- in a dimension of its own or as an affine
  // offset added to a role's dimension (A[4i - j][5k]) -- each factor through
  // its own expression; C reads every batch loop alone, in a dimension that
  // holds nothing else. For each value of the batch loops the role loops
  // then form an ordinary GEMM of strided views whose base offsets are affine
  // in the batch counters (OperandLayout::batchCoefficients).
  SmallVector<bool> isBatchLoop(numIterationDims, false);

  // A refusal of the analysis by groups that the readers analysis (3.,
  // docs/AFFINE_GEMM_MAPPING.md section 19) lifts: a subscript that is not
  // role-pure -- C mixing a batch loop with a role, a dimension combining two
  // roles, an operand reading one coordinate in two dimensions.
  bool relaxableRefusal = false;

  // ---------------------------------------------------------------------
  // Dimension-intersection partitioning (docs/AFFINE_GEMM_MAPPING.md section
  // 21, 2026-10-19). The loops read by all three operands are batch loops in
  // every decomposition (section 19.2); the analyses below make batch loops
  // of some others too ("demoted" loops), each partitioning the GEMM into
  // independent slabs of C:
  //   - a loop read by C only (C[b][i][j] += A[i][k] * B[k][j]): both factors
  //     are shared by its partitions;
  //   - the loops of a row or column role that are not one dense coordinate
  //     (C[p][i][j] += A[p][i][k] * B[k][j]: p and i are both read by A and
  //     C, in separate dimensions): one dense sub-coordinate stays the role,
  //     the others partition the GEMM, and the factor that does not read them
  //     (B) is shared by the partitions.
  // A demoted loop must be read by C alone in a dimension of its own (the
  // partitions' outputs are then disjoint slabs of C; with the exact
  // bijection of section 19.13 every slab is one GEMM). A loop read by both
  // factors and not by C is never demoted: its batches would all accumulate
  // into one slab of C -- a K-fold, a partial-C reduction, not constructed
  // (C[i][j] += A[r][i][k] * B[r][k][j]); nor is a loop read by one factor
  // only (a broadcast, section 10). The choice among several legal roles:
  // chooseDemotion below. On a domain that is not a box the readers decide
  // nothing (section 19.7): no demotion.
  // ---------------------------------------------------------------------
  SmallVector<bool> demoted(numIterationDims, false);
  SmallVector<unsigned> readersOf(numIterationDims, 0);
  for (const Subscript &sub : subscripts)
    for (unsigned d = 0; d < numIterationDims; ++d)
      if (sub.onDomain.coefficients[d] != 0)
        readersOf[d] |= 1u << sub.operand;
  if (boxDomain)
    for (unsigned d = 0; d < numIterationDims; ++d)
      if (readersOf[d] == 0b100)
        demoted[d] = true;
  // Reduction-batch partitioning (section 23, opt-in): a loop read by one
  // factor only, and not by C, is a reduction batch loop -- for each of its
  // values the nest is a GEMM with that factor's view offset by it, and the
  // values accumulate into the same slab of C, in order
  // (C[i][j] += A[i][k] * B[l][j]: k and l, with K = 1).
  const bool reductionBatches =
      options.reductionBatches && boxDomain && accumulates;
  bool anyReductionBatch = false;
  if (reductionBatches)
    for (unsigned d = 0; d < numIterationDims; ++d) {
      if (readersOf[d] == 0b001 || readersOf[d] == 0b010)
        demoted[d] = anyReductionBatch = true;
      if (readersOf[d] != 0b011)
        continue;
      if (sharedLoops)
        sharedLoops->push_back(d);
      if (keepK && d != *keepK)
        demoted[d] = anyReductionBatch = true;
    }
  // The loops of a row or column role an analysis refused as not one dense
  // coordinate: the retry below demotes some of them (nullopt: no retry).
  std::optional<SmallVector<unsigned>> demotionCandidate;
  // An output role whose loops the retry demotes all together (section 25:
  // C not injective on the (I, J) box of an accumulation), not through
  // chooseDemotion, which keeps one component of a role.
  std::optional<SmallVector<unsigned>> wholeRoleDemotion;
  auto tripCountOf = [&](unsigned d) -> std::optional<int64_t> {
    affine::AffineForOp loop =
        affine::getForInductionVarOwner(iterationIVs[d]);
    std::optional<int64_t> lower =
        loop ? constantLoopBound(loop, /*lower=*/true) : std::nullopt;
    std::optional<int64_t> upper =
        loop ? constantLoopBound(loop, /*lower=*/false) : std::nullopt;
    if (!lower || !upper)
      return std::nullopt;
    return checkedTripCount(*lower, *upper, loop.getStep().getSExtValue());
  };
  if (sharedLoops)
    llvm::stable_sort(*sharedLoops, [&](unsigned x, unsigned y) {
      const int64_t tx = tripCountOf(x).value_or(0),
                    ty = tripCountOf(y).value_or(0);
      return tx != ty ? tx > ty : x > y;
    });
  auto innermostCDim = [&](unsigned d) {
    int dim = -1;
    for (const Subscript &sub : subscripts)
      if (sub.operand == 2 && sub.onDomain.coefficients[d] != 0)
        dim = std::max(dim, int(sub.dim));
    return dim;
  };
  // Which loops of a refused role to demote. The role's loops joined by a
  // subscript form components (A[4p + i] joins p and i); the component of the
  // largest extent (the product of its trip counts: the largest tiles, the
  // fewest partitions; ties: the one read in C's innermost dimension, then
  // the lowest loop) stays the role and the others are demoted; a single
  // component that is not one coordinate keeps its loop of the largest
  // extent (same ties). Empty when a trip count is not a constant.
  auto chooseDemotion =
      [&](ArrayRef<unsigned> loops) -> SmallVector<unsigned> {
    SmallVector<unsigned> parent(numIterationDims);
    std::iota(parent.begin(), parent.end(), 0u);
    auto find = [&](unsigned x) {
      while (parent[x] != x)
        x = parent[x] = parent[parent[x]];
      return x;
    };
    for (const Subscript &sub : subscripts) {
      std::optional<unsigned> first;
      for (unsigned d : loops) {
        if (sub.onDomain.coefficients[d] == 0)
          continue;
        if (!first)
          first = find(d);
        else
          parent[find(d)] = *first;
      }
    }
    struct Component {
      SmallVector<unsigned> loops;
      int64_t extent;
      int cDim;
    };
    SmallVector<Component> components;
    for (unsigned d : loops) {
      const unsigned root = find(d);
      Component *component = nullptr;
      for (Component &c : components)
        if (find(c.loops.front()) == root)
          component = &c;
      if (!component) {
        components.push_back(Component{{}, 1, -1});
        component = &components.back();
      }
      std::optional<int64_t> trips = tripCountOf(d);
      std::optional<int64_t> extent =
          trips ? llvm::checkedMul(component->extent, *trips) : std::nullopt;
      if (!extent)
        return {};
      component->loops.push_back(d);
      component->extent = *extent;
      component->cDim = std::max(component->cDim, innermostCDim(d));
    }
    auto better = [](const Component &x, const Component &y) {
      if (x.extent != y.extent)
        return x.extent > y.extent;
      if (x.cDim != y.cDim)
        return x.cDim > y.cDim;
      return x.loops.front() < y.loops.front();
    };
    const Component *keep = &components.front();
    for (const Component &c : components)
      if (better(c, *keep))
        keep = &c;
    SmallVector<unsigned> result;
    if (components.size() > 1) {
      for (const Component &c : components)
        if (&c != keep)
          llvm::append_range(result, c.loops);
      return result;
    }
    unsigned best = keep->loops.front();
    for (unsigned d : keep->loops) {
      Component x{{d}, *tripCountOf(d), innermostCDim(d)};
      Component y{{best}, *tripCountOf(best), innermostCDim(best)};
      if (better(x, y))
        best = d;
    }
    for (unsigned d : keep->loops)
      if (d != best)
        result.push_back(d);
    return result;
  };
  // (partitionOutputsDisjoint, the check that a demoted loop is read by C
  // alone, follows the refusal helpers below.)

  // The verdicts of the two analyses below -- PROVEN_INVALID (not a GEMM in
  // any decomposition) and VALID_BUT_LOWERING_UNSUPPORTED (a batched GEMM
  // after demoting loops, not constructed) -- rest on Theorem 3 for a box
  // domain (section 19.2). On any other domain the readers taken in the
  // original counters decide nothing (section 19.7: a loop read by all three
  // operands may drive a role after the parametrization), so both become
  // UNKNOWN there, with the same reason.
  auto refuseRoles = [&](const llvm::Twine &reason) {
    if (boxDomain)
      return refuse(reason);
    return unknown(reason + " -- on a domain that is not a box the readers "
                            "decide nothing (docs/AFFINE_GEMM_MAPPING.md "
                            "section 19.7)");
  };
  auto unsupportedRoles = [&](const llvm::Twine &reason) {
    if (boxDomain)
      return unsupported(reason);
    return unknown(reason + " -- on a domain that is not a box the readers "
                            "decide nothing (docs/AFFINE_GEMM_MAPPING.md "
                            "section 19.7)");
  };
  // A demoted loop (section 21) is read by C alone in a dimension of its own
  // (with other batch loops at most): the partitions' outputs are disjoint
  // slabs. C reading it together with a role coordinate -- C[p + i][j], a
  // merged C[3p + i][j] -- would make the partitions overlap (an
  // accumulation in order, valid but not an independent partition) or a
  // merged output dimension: not constructed.
  // Whether the batch loops -- read by all three operands, demoted, or
  // reduction batch loops -- and the loops `extra` are an outermost prefix of
  // the nest's loops of two or more iterations (section 25): the batch loops
  // then run around the tiles in the program's order, and every element of C
  // receives the contributions of different batches in the order written
  // (only the order inside one tile, over K, changes, under the permission
  // to reassociate). On a box domain whose counters are the ones as written
  // only.
  auto batchesOuterPrefix = [&](ArrayRef<unsigned> extra) -> bool {
    if (!boxDomain || !options.countersAsWritten)
      return false;
    bool inner = false;
    for (unsigned d = 0; d < numIterationDims; ++d) {
      if (single[d])
        continue;
      const bool batch =
          isBatchLoop[d] || demoted[d] || llvm::is_contained(extra, d);
      if (!batch)
        inner = true;
      else if (inner)
        return false;
    }
    return true;
  };
  auto partitionOutputsDisjoint = [&]() -> LogicalResult {
    // An accumulation whose batches run in the program's order may have
    // partitions that overlap in C (section 25): each batch reads C, adds its
    // tile and writes C back before the next one reads it, so every
    // contribution is kept in the order written. The tiles themselves are
    // still checked injective (G2) below.
    if (accumulates && batchesOuterPrefix({}))
      return success();
    for (const Subscript &sub : subscripts) {
      if (sub.operand != 2)
        continue;
      int demotedLoop = -1;
      bool other = false;
      for (unsigned d = 0; d < numIterationDims; ++d) {
        if (sub.onDomain.coefficients[d] == 0)
          continue;
        if (demoted[d])
          demotedLoop = demotedLoop < 0 ? int(d) : demotedLoop;
        else if (!isBatchLoop[d])
          other = true;
      }
      if (demotedLoop >= 0 && other)
        return unsupportedRoles(
            llvm::Twine("loop ") + llvm::Twine(demotedLoop) +
            " would partition the GEMM (dimension-intersection "
            "partitioning, docs/AFFINE_GEMM_MAPPING.md section 21), but C "
            "reads it in dimension " + llvm::Twine(sub.dim) +
            " together with a role coordinate: the partitions' outputs would "
            "not be independent slabs of C (overlapping, or one merged "
            "dimension) -- not constructed");
    }
    return success();
  };

  // 2. The analysis by groups and roles (its refusals set why/mappingClass).
  auto byGroups = [&]() -> LogicalResult {
  SmallVector<std::array<bool, 3>> readsLoop(numIterationDims,
                                             {false, false, false});
  for (const Subscript &sub : subscripts)
    for (unsigned d = 0; d < numIterationDims; ++d)
      if (sub.onDomain.coefficients[d] != 0)
        readsLoop[d][sub.operand] = true;
  for (unsigned d = 0; d < numIterationDims; ++d)
    isBatchLoop[d] =
        (readsLoop[d][0] && readsLoop[d][1] && readsLoop[d][2]) || demoted[d];
  if (failed(partitionOutputsDisjoint()))
    return failure();
  // C reads every batch coordinate alone: a subscript of C with a batch loop
  // and any other loop would move the output position with the batch -- the
  // coupled and skewed placements (C[i + j][j] with A[i + j][k],
  // C[i + k][j]) that the specification rejects (not one dense GEMM).
  for (const Subscript &sub : subscripts) {
    if (sub.operand != 2)
      continue;
    std::optional<unsigned> batchLoop;
    bool otherLoop = false;
    for (unsigned d = 0; d < numIterationDims; ++d) {
      if (sub.onDomain.coefficients[d] == 0)
        continue;
      if (isBatchLoop[d])
        batchLoop = batchLoop.value_or(d);
      else
        otherLoop = true;
    }
    if (batchLoop && otherLoop) {
      relaxableRefusal = true;
      return refuseRoles(llvm::Twine("C's dimension ") + llvm::Twine(sub.dim) +
                    " mixes a batch coordinate (loop " +
                    llvm::Twine(*batchLoop) +
                    ", read by all three operands) with another coordinate: "
                    "the output position would move with the batch (a "
                    "coupled or skewed placement, not one dense GEMM)");
    }
  }
  // A loop the union-find below joins: a role loop in any subscript; a batch
  // loop only in C's subscripts (one batch coordinate per dimension of C). A
  // factor's batch offset joins nothing.
  auto grouped = [&](const Subscript &sub, unsigned d) {
    return sub.onDomain.coefficients[d] != 0 &&
           (!isBatchLoop[d] || sub.operand == 2);
  };

  // Groups: union-find over the loops, joined by every subscript's support
  // (without the factors' batch offsets).
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
      if (!grouped(sub, d))
        continue;
      if (!first)
        first = findLoop(d);
      else
        parentLoop[findLoop(d)] = *first;
    }
  }
  // The group of each subscript (-1: constant on the domain, or a factor's
  // dimension that reads batch loops only), groups in the order their first
  // subscript appears.
  SmallVector<unsigned> groupRoot;
  SmallVector<int> groupOf(subscripts.size(), -1);
  for (auto [index, sub] : llvm::enumerate(subscripts)) {
    for (unsigned d = 0; d < numIterationDims; ++d) {
      if (!grouped(sub, d))
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
    // C may read a batch coordinate in several dimensions of its own
    // (C[b][b - 1][i][j]: each one a batch dimension, the output slice a
    // function of the batch loops alone) -- as the positional form allows
    // for a prefix that repeats a batch loop.
    if (dim >= 0 && isBatchLoop[groupRoot[groupOf[index]]])
      continue;
    if (dim >= 0) {
      relaxableRefusal = true;
      return refuseRoles(llvm::Twine(kOperandName[sub.operand]) +
                    " reads one logical coordinate in two of its dimensions "
                    "(" + llvm::Twine(dim) + " and " + llvm::Twine(sub.dim) +
                    "): two coordinates would share a loop, so the points "
                    "are not a box (Theorem 3, S1)");
    }
    dim = sub.dim;
  }

  // Roles. With the factors as written: a group read by factor f and C is
  // "R_f"; by both factors, K; by all three, P (batch).
  SmallVector<unsigned> groupsWithFirst, groupsWithSecond, reductionGroups,
      batchGroups;
  for (unsigned g = 0; g < numGroups; ++g) {
    // A group of batch loops: one batch coordinate, read alone by one
    // dimension of C (the factors read it through their own expressions).
    if (isBatchLoop[groupRoot[g]]) {
      batchGroups.push_back(g);
      continue;
    }
    const bool f0 = readerDim[g][0] >= 0, f1 = readerDim[g][1] >= 0,
               c = readerDim[g][2] >= 0;
    // Every loop of a role group is read by the same two operands; a group
    // that all three read merges loops of different roles through one
    // subscript (A[i + k], I with K): a coupled mapping, no box per role.
    if (f0 && f1 && c) {
      relaxableRefusal = true;
      return refuseRoles("a subscript couples loops of different coordinates "
                    "(their group is read by both factors and C, but none of "
                    "its loops is read by all three): a coupled mapping, not "
                    "one dense GEMM");
    }
    if (f0 && f1)
      reductionGroups.push_back(g);
    else if (f0 && c)
      groupsWithFirst.push_back(g);
    else if (f1 && c)
      groupsWithSecond.push_back(g);
    else
      return unsupportedRoles(
          llvm::Twine("a coordinate is read by ") +
          kOperandName[f0 ? 0 : f1 ? 1 : 2] +
          " only: the other operands would be broadcast along it (a "
          "stride-0 view, not lowered: docs/AFFINE_GEMM_MAPPING.md section 10)");
  }
  // A batch coordinate the positional form did not recognize (one not in
  // the leading dimensions of all three operands): a batched GEMM, lowered
  // with the batch loops around the tiles (ISSUE-006), each operand's view
  // offset by its own batch terms per batch iteration (ISSUE-007). An
  // element-wise product (every coordinate a batch one, M = N = K = 1) is a
  // GEMM of 1 x 1 x 1 tiles in name only: refused.
  if (!batchGroups.empty() && groupsWithFirst.empty() &&
      groupsWithSecond.empty() && reductionGroups.empty())
    return refuseRoles(llvm::Twine(batchGroups.size()) +
                  " coordinate(s) are read by all three operands and none by "
                  "two: an element-wise product (a batched GEMM with "
                  "M = N = K = 1), not a matrix multiplication");
  // The factors read the batch coordinates through their own expressions --
  // in dimensions of their own or as offsets of their role dimensions
  // (ISSUE-007): nothing to match against C here. What each dimension adds
  // per batch value is recorded in the operands' layouts below.
  for (auto [groups, role, r] :
       {std::tuple{&groupsWithFirst, "the first factor and C", 0u},
        std::tuple{&groupsWithSecond, "the second factor and C", 1u},
        std::tuple{&reductionGroups, "both factors (the reduction)", 2u}})
    if (groups->size() > 1) {
      // Two reduction coordinates (C[i][j] += A[r][i][k] * B[r][k][j]): the
      // extra one is not a partition -- its batches would all accumulate into
      // one slab of C -- but a K-fold summed into C, a partial-C reduction
      // (docs section 21): not constructed.
      if (r == 2)
        return unsupportedRoles(
            llvm::Twine(groups->size()) + " separate coordinates are read by " +
            role + ": one GEMM role over several coordinates needs a merged "
            "(reshaped) view, not lowered; as a batch the extra reduction "
            "coordinate would accumulate every batch into one slab of C "
            "(requires a partial-C reduction, docs/AFFINE_GEMM_MAPPING.md "
            "section 21)");
      // Several coordinates in a row or column role: all but one of them
      // partition the GEMM (dimension-intersection partitioning, section
      // 21) -- the retry demotes them to batch loops.
      if (boxDomain) {
        SmallVector<unsigned> loops;
        for (unsigned g : *groups)
          for (unsigned d = 0; d < numIterationDims; ++d)
            if (!isBatchLoop[d] && findLoop(d) == groupRoot[g] &&
                (readsLoop[d][0] || readsLoop[d][1] || readsLoop[d][2]))
              loops.push_back(d);
        llvm::sort(loops);
        demotionCandidate = loops;
      }
      return unsupportedRoles(llvm::Twine(groups->size()) +
                         " separate coordinates are read by " + role +
                         ": one GEMM role over several coordinates needs a "
                         "merged (reshaped) view, not lowered (a partition "
                         "on the extra coordinates needs constant trip "
                         "counts and a box domain, docs/AFFINE_GEMM_MAPPING.md "
                         "section 21)");
    }

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
  auto domainFormOf = [&](unsigned operand, unsigned dim) -> const LinearForm & {
    for (const Subscript &sub : subscripts)
      if (sub.operand == operand && sub.dim == dim)
        return sub.onDomain;
    llvm_unreachable("every (operand, dimension) has a subscript");
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
    // A batch dimension: one whose subscript reads batch loops only (for C,
    // the dimension of a batch coordinate). Never taken by a role of extent
    // one below.
    for (unsigned dim = 0; dim < rank; ++dim) {
      const LinearForm &form = domainFormOf(operand, dim);
      bool batch = false, role = false;
      for (unsigned d = 0; d < numIterationDims; ++d)
        if (form.coefficients[d] != 0)
          (isBatchLoop[d] ? batch : role) = true;
      if (batch && !role) {
        isConstant[dim] = false;
        layout.batchDim.push_back(dim);
      }
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
    // What each dimension adds per batch value: its batch loops' terms. A
    // batch dimension's static index is its subscript's constant part.
    layout.batchCoefficients.assign(rank,
                                    SmallVector<int64_t>(numIterationDims, 0));
    for (unsigned dim = 0; dim < rank; ++dim) {
      const LinearForm &form = domainFormOf(operand, dim);
      for (unsigned d = 0; d < numIterationDims; ++d)
        if (isBatchLoop[d])
          layout.batchCoefficients[dim][d] = form.coefficients[d];
    }
    for (int dim : layout.batchDim)
      layout.constantIndex[dim] = domainFormOf(operand, dim).constant;
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
    // A role's subscripts without their batch offsets (a factor's dimension
    // may add batch terms to the role, A[4i - j]; C's role dimensions have
    // none): the batch loops set to 0, their terms are added per batch value.
    SmallVector<AffineExpr> withoutBatch;
    for (unsigned d = 0; d < numIterationDims; ++d)
      withoutBatch.push_back(isBatchLoop[d]
                                 ? getAffineConstantExpr(0, mapContext)
                                 : getAffineDimExpr(d, mapContext));
    AffineExpr x =
        operandMap[role.first].getResult(firstDim).replaceDims(withoutBatch);
    AffineExpr y =
        operandMap[role.second].getResult(secondDim).replaceDims(withoutBatch);
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
    // A row or column role whose group has several loops must be one dense
    // coordinate of them (Theorem 3, S2: a mixed radix); else all but one of
    // its loops partition the GEMM (section 21) -- the retry demotes them.
    // The dense partition checks the reduction role the same way (and the
    // roles again) once the mapping is built.
    if (r < 2 && boxDomain && !emptyDomain) {
      SmallVector<unsigned> groupLoops;
      for (unsigned d = 0; d < numIterationDims; ++d)
        if (!isBatchLoop[d] && findLoop(d) == groupRoot[*role.group] &&
            (readsLoop[d][0] || readsLoop[d][1] || readsLoop[d][2]))
          groupLoops.push_back(d);
      if (groupLoops.size() > 1) {
        auto form = extractLinearForm(shared->expr, numIterationDims);
        lattice::IntVector magnitudes;
        SmallVector<int64_t> trips;
        int64_t scale = 0;
        bool known = succeeded(form);
        for (unsigned d : groupLoops) {
          affine::AffineForOp loop =
              affine::getForInductionVarOwner(iterationIVs[d]);
          std::optional<int64_t> trip = tripCountOf(d);
          std::optional<int64_t> c =
              known && loop ? llvm::checkedMul(form->coefficients[d],
                                               loop.getStepAsInt())
                            : std::nullopt;
          std::optional<int64_t> g = c ? lattice::gcd(scale, *c) : std::nullopt;
          if (!trip || !g) {
            known = false;
            break;
          }
          magnitudes.push_back(*c);
          trips.push_back(*trip);
          scale = *g;
        }
        if (known && scale != 0)
          for (int64_t &m : magnitudes)
            m /= scale;
        if (!known || scale == 0 ||
            !lattice::mixedRadixExtent(magnitudes, trips)) {
          demotionCandidate = groupLoops;
          return unsupportedRoles(
              llvm::Twine("logical coordinate ") + role.name +
              " is driven by " + llvm::Twine(groupLoops.size()) +
              " loops that do not enumerate it densely (or its loops' "
              "bounds are not constant): no box of " + role.name +
              " values (with all but one of them as batch loops it would be "
              "a batched GEMM, constructed only with constant trip counts, "
              "docs/AFFINE_GEMM_MAPPING.md section 21)");
        }
      }
    }
  }

  // Batch coordinates: every dimension of C that reads batch loops only is a
  // row of the partition prefix P* of the logical mapping y = (P*, I, J, K)
  // -- a batch coordinate filling several dimensions of C gives several rows,
  // so every batch loop is in some row. The factors' own batch expressions
  // and offsets are in their layouts' batchCoefficients.
  SmallVector<bool> covered(numIterationDims, false);
  for (const Subscript &sub : subscripts) {
    if (sub.operand != 2)
      continue;
    bool batch = false, other = false;
    for (unsigned d = 0; d < numIterationDims; ++d)
      if (sub.onDomain.coefficients[d] != 0)
        (isBatchLoop[d] ? batch : other) = true;
    if (batch && !other) {
      logicalPartitions.push_back(operandMap[2].getResult(sub.dim));
      for (unsigned d = 0; d < numIterationDims; ++d)
        if (sub.onDomain.coefficients[d] != 0)
          covered[d] = true;
    }
  }
  // A batch loop C does not read (a reduction batch loop, section 23) is a
  // row of its own: its values run in order around the tiles.
  for (unsigned d = 0; d < numIterationDims; ++d)
    if (isBatchLoop[d] && !covered[d] && !(readersOf[d] & 0b100))
      logicalPartitions.push_back(getAffineDimExpr(d, mapContext));
  numPartitionDims = logicalPartitions.size();
  return success();
  };

  // 3. The readers analysis (docs/AFFINE_GEMM_MAPPING.md section 19): the
  // decomposition for the nests the analysis by groups refuses only because
  // a subscript is not role-pure -- a dimension combining both of an
  // operand's roles (A[i][i + k], C[i + j][j]), a merged dimension
  // (C[4 i + j]), C reading batch loops in its role dimensions (C[i + k][j]).
  // Every loop is classified by its readers alone (AC -> I, BC -> J,
  // AB -> K, ABC -> batch; by Theorem 3 applied to the slot map, the
  // decomposition with the fewest batch coordinates, section 19.2); each
  // role's subscripts, restricted to the role's loops, have one common
  // direction -- the role's coordinate -- or the nest is refused (separate
  // coordinates, or SQ-1); every subscript is then an affine view of its
  // operand's two coordinates plus batch terms (G1, verified below), and C's
  // view must be injective per batch (G2, section 19.3).
  auto byReaders = [&]() -> LogicalResult {
    std::fill(isBatchLoop.begin(), isBatchLoop.end(), false);
    SmallVector<unsigned> readers(numIterationDims, 0);
    for (const Subscript &sub : subscripts)
      for (unsigned d = 0; d < numIterationDims; ++d)
        if (sub.onDomain.coefficients[d] != 0)
          readers[d] |= 1u << sub.operand;
    // The roles with the factors as written (A = the first factor):
    // 0 = I (first factor and C), 1 = J (second factor and C), 2 = K.
    static constexpr const char *kRole[] = {"I", "J", "K"};
    std::array<SmallVector<unsigned>, 3> roleLoops;
    for (unsigned d = 0; d < numIterationDims; ++d) {
      // A demoted loop (section 21) is a batch loop whatever its readers.
      if (demoted[d]) {
        isBatchLoop[d] = true;
        continue;
      }
      switch (readers[d]) {
      case 0b111:
        isBatchLoop[d] = true;
        break;
      case 0b101:
        roleLoops[0].push_back(d);
        break;
      case 0b110:
        roleLoops[1].push_back(d);
        break;
      case 0b011:
        roleLoops[2].push_back(d);
        break;
      case 0:
        break; // a loop that runs once: a constant on the domain
      default:
        return unsupportedRoles(
            llvm::Twine("a coordinate is read by ") +
            kOperandName[readers[d] == 1 ? 0 : readers[d] == 2 ? 1 : 2] +
            " only: the other operands would be broadcast along it (a "
            "stride-0 view, not lowered: docs/AFFINE_GEMM_MAPPING.md "
            "section 10)");
      }
    }
    if (failed(partitionOutputsDisjoint()))
      return failure();
    if (roleLoops[0].empty() && roleLoops[1].empty() && roleLoops[2].empty())
      return refuseRoles("every loop is read by all three operands: no row, "
                    "column or reduction coordinate -- an element-wise "
                    "product, not a GEMM (docs/AFFINE_GEMM_MAPPING.md "
                    "section 19.1)");

    // Each role's coordinate: the common direction of every subscript's
    // restriction to the role's loops (the zero restriction included: an
    // operand that does not read the role).
    std::array<std::optional<lattice::CommonDirection>, 3> direction;
    for (unsigned r = 0; r < 3; ++r) {
      if (roleLoops[r].empty())
        continue;
      SmallVector<lattice::IntVector> forms;
      for (const Subscript &sub : subscripts) {
        lattice::IntVector form(numIterationDims, 0);
        for (unsigned d : roleLoops[r])
          form[d] = sub.onDomain.coefficients[d];
        forms.push_back(std::move(form));
      }
      direction[r] = lattice::commonDirection(forms);
      if (direction[r])
        continue;
      // Not proportional: two dimensions of one operand read the role
      // through different combinations of its loops (separate coordinates in
      // one role), or two operands do (an operand-specific decomposition).
      auto proportional = [](const lattice::IntVector &u,
                             const lattice::IntVector &v) {
        for (unsigned s = 0; s < u.size(); ++s)
          for (unsigned t = 0; t < u.size(); ++t) {
            std::optional<int64_t> x = llvm::checkedMul(u[s], v[t]);
            std::optional<int64_t> y = llvm::checkedMul(u[t], v[s]);
            if (!x || !y || *x != *y)
              return false;
          }
        return true;
      };
      for (unsigned i = 0; i < forms.size(); ++i)
        for (unsigned j = i + 1; j < forms.size(); ++j) {
          if (proportional(forms[i], forms[j]))
            continue;
          // A row or column role over several coordinates: all but one of
          // them partition the GEMM (section 21) -- the retry demotes them.
          if (r < 2 && boxDomain)
            demotionCandidate = roleLoops[r];
          if (subscripts[i].operand == subscripts[j].operand)
            return unsupportedRoles(
                llvm::Twine(kOperandName[subscripts[i].operand]) +
                " reads logical coordinate " + kRole[r] +
                " through two different combinations of its loops "
                "(dimensions " + llvm::Twine(subscripts[i].dim) + " and " +
                llvm::Twine(subscripts[j].dim) + "): separate coordinates "
                "in one GEMM role need a merged (reshaped) view, not "
                "lowered" +
                (r == 2 ? " (as a batch the extra reduction coordinate would "
                          "accumulate every batch into one slab of C: "
                          "requires a partial-C reduction, docs/"
                          "AFFINE_GEMM_MAPPING.md section 21)"
                        : ""));
          return unknown(
              llvm::Twine("logical coordinate ") + kRole[r] + " (" +
              kOperandName[subscripts[i].operand] + "'s dimension " +
              llvm::Twine(subscripts[i].dim) + " " +
              exprText(subscripts[i].expr) + ", " +
              kOperandName[subscripts[j].operand] + "'s dimension " +
              llvm::Twine(subscripts[j].dim) + " " +
              exprText(subscripts[j].expr) + "): the subscripts are not "
              "multiples of one linear form of the role's loops -- "
              "operand-specific decompositions of one coordinate are not "
              "lowered (spec question SQ-1)");
        }
      return unknown(llvm::Twine("logical coordinate ") + kRole[r] +
                     ": the common direction overflows 64-bit arithmetic");
    }

    // The layouts: per memref dimension, the coefficients of the operand's
    // two roles (GEMM order), the constant and the batch terms.
    static constexpr std::array<std::array<unsigned, 2>, 3> kRolesOf{
        {{0, 2}, {2, 1}, {0, 1}}};
    std::array<OperandLayout, 3> layouts;
    for (unsigned x = 0; x < 3; ++x) {
      const unsigned rank = operandMap[x].getNumResults();
      layouts[x].roleCoefficients.assign(rank, {0, 0});
      layouts[x].dimConstants.assign(rank, 0);
      layouts[x].constantIndex.assign(rank, std::nullopt);
      layouts[x].batchCoefficients.assign(
          rank, SmallVector<int64_t>(numIterationDims, 0));
    }
    for (auto [index, sub] : llvm::enumerate(subscripts)) {
      OperandLayout &layout = layouts[sub.operand];
      for (unsigned slot = 0; slot < 2; ++slot) {
        const unsigned r = kRolesOf[sub.operand][slot];
        if (direction[r])
          layout.roleCoefficients[sub.dim][slot] =
              direction[r]->multipliers[index];
      }
      layout.dimConstants[sub.dim] = sub.onDomain.constant;
      for (unsigned d = 0; d < numIterationDims; ++d)
        if (isBatchLoop[d])
          layout.batchCoefficients[sub.dim][d] = sub.onDomain.coefficients[d];
    }
    for (OperandLayout &layout : layouts) {
      const unsigned rank = layout.roleCoefficients.size();
      std::array<int, 2> dimOfRole{-1, -1};
      bool rolePure = true;
      for (unsigned d = 0; d < rank; ++d) {
        if (layout.roleCoefficients[d][0] != 0 &&
            layout.roleCoefficients[d][1] != 0)
          rolePure = false;
        for (unsigned slot = 0; slot < 2; ++slot) {
          if (layout.roleCoefficients[d][slot] == 0)
            continue;
          if (dimOfRole[slot] >= 0)
            rolePure = false;
          dimOfRole[slot] = d;
        }
        if (layout.roleCoefficients[d][0] == 0 &&
            layout.roleCoefficients[d][1] == 0) {
          // No role: a fixed index, plus its batch terms per batch value.
          layout.constantIndex[d] = layout.dimConstants[d];
          if (layout.hasBatchTerms(d))
            layout.batchDim.push_back(d);
        }
      }
      layout.general = !rolePure;
      layout.roleDim = rolePure ? dimOfRole : std::array<int, 2>{-1, -1};
    }

    // G2: C's view injective on the (I, J) box. The roles' ordinals: the
    // coordinate is scale * ordinal + base with the ordinal a mixed radix of
    // the role's loops' ordinals (Theorem 3, S2); a role that is not one is
    // refused here, as the dense partition would.
    auto roleOrdinal = [&](unsigned r)
        -> FailureOr<std::pair<int64_t, int64_t>> {
      if (!direction[r])
        return std::pair<int64_t, int64_t>{1, 1};
      lattice::IntVector magnitudes;
      SmallVector<int64_t> trips;
      int64_t scale = 0;
      for (unsigned d : roleLoops[r]) {
        affine::AffineForOp loop =
            affine::getForInductionVarOwner(iterationIVs[d]);
        std::optional<int64_t> lower = constantLoopBound(loop, true);
        std::optional<int64_t> upper = constantLoopBound(loop, false);
        std::optional<int64_t> trip =
            lower && upper ? checkedTripCount(*lower, *upper,
                                              loop.getStepAsInt())
                           : std::nullopt;
        std::optional<int64_t> c = llvm::checkedMul(
            direction[r]->direction[d], loop.getStepAsInt());
        std::optional<int64_t> g = c ? lattice::gcd(scale, *c) : std::nullopt;
        if (!trip || !g)
          return failure();
        magnitudes.push_back(*c);
        trips.push_back(*trip);
        scale = *g;
      }
      for (int64_t &m : magnitudes)
        m /= scale;
      std::optional<int64_t> extent =
          lattice::mixedRadixExtent(magnitudes, trips);
      if (!extent)
        return failure();
      return std::pair<int64_t, int64_t>{scale, *extent};
    };
    // An empty domain (a loop with no iteration) has no box to check the
    // roles' radixes or C's injectivity on; nothing runs. The nest is
    // recognized with its layouts and the dense partition reports the empty
    // GEMM (normalizationEmptyLoop, SQ-2), as for a role-pure nest.
    if (!emptyDomain) {
    std::array<std::pair<int64_t, int64_t>, 3> ordinal;
    for (unsigned r = 0; r < 3; ++r) {
      FailureOr<std::pair<int64_t, int64_t>> o = roleOrdinal(r);
      if (failed(o)) {
        // A row or column role that is not one dense coordinate: all but
        // one of its loops partition the GEMM (section 21) -- the retry
        // demotes them; a reduction role is not partitioned.
        if (r < 2 && boxDomain && roleLoops[r].size() > 1)
          demotionCandidate = roleLoops[r];
        return unsupportedRoles(
            llvm::Twine("logical coordinate ") + kRole[r] + " is driven by " +
            llvm::Twine(roleLoops[r].size()) +
            " loops that do not enumerate it densely (or its loops' bounds "
            "are not constant): no box of " + kRole[r] +
            " values (with all but one of them as batch loops it would be a "
            "batched GEMM, which is " +
            (r == 2 ? "not constructed for a reduction coordinate: every "
                      "batch would accumulate into one slab of C, a "
                      "partial-C reduction, docs/AFFINE_GEMM_MAPPING.md "
                      "section 21)"
                    : "constructed only with constant trip counts, section "
                      "21)"));
      }
      ordinal[r] = *o;
    }
    lattice::IntVector lamI, lamJ;
    for (const std::array<int64_t, 2> &c : layouts[2].roleCoefficients) {
      std::optional<int64_t> a = llvm::checkedMul(c[0], ordinal[0].first);
      std::optional<int64_t> b = llvm::checkedMul(c[1], ordinal[1].first);
      if (!a || !b)
        return unknown("C's view overflows 64-bit arithmetic");
      lamI.push_back(*a);
      lamJ.push_back(*b);
    }
    std::optional<lattice::BoxInjectivity> injective = lattice::injectiveOnBox(
        lamI, lamJ, ordinal[0].second, ordinal[1].second);
    if (!injective)
      return unknown("C's injectivity check overflows 64-bit arithmetic");
    if (!injective->injective()) {
      // (A std::string, not a Twine: a Twine of temporaries dangles.)
      const std::string collision =
          (llvm::Twine("C's view is not injective on the (I, J) box: the "
                       "outputs with ordinals (I, J) and (I + ") +
           llvm::Twine(injective->collision->first) + ", J + " +
           llvm::Twine(injective->collision->second) +
           ") write the same element of C")
              .str();
      // For an assignment the later product overwrites the earlier one: the
      // result is not the GEMM of A and B in any decomposition (section 20).
      if (!accumulates)
        return refuseRoles(collision +
                           ": the assignment C = A * B overwrites it, only "
                           "the last product survives -- not the GEMM of A "
                           "and B (docs/AFFINE_GEMM_MAPPING.md section 20)");
      // An accumulation (section 25): the colliding outputs are a reduction
      // that must be kept. One output role becomes batch loops -- I first,
      // then J -- when its loops and the other batch loops are an outermost
      // prefix of the nest (the batches then run in the order written, each
      // reading the C the previous one wrote), with constant trip counts;
      // the retry demotes it and checks every tile injective again. Else the
      // refusal stands.
      for (unsigned r : {0u, 1u}) {
        if (roleLoops[r].empty() ||
            llvm::any_of(roleLoops[r],
                         [&](unsigned d) { return !tripCountOf(d); }) ||
            !batchesOuterPrefix(roleLoops[r]))
          continue;
        wholeRoleDemotion = roleLoops[r];
        break;
      }
      return unsupportedRoles(
          collision +
          ", so one tile would lose an update "
          "(G2, docs/AFFINE_GEMM_MAPPING.md section 19.3); a decomposition "
          "with the J (or I) loops as batch loops would be valid, and is not "
          "constructed");
    }
    }

    // The result. The coordinates' expressions; each operand's view of a
    // role, for a role-pure layout, is its role dimension's coefficient and
    // constant (the general layouts keep theirs in roleCoefficients).
    auto exprOf = [&](unsigned r) {
      AffineExpr e = getAffineConstantExpr(0, mapContext);
      if (direction[r])
        for (unsigned d = 0; d < numIterationDims; ++d)
          if (direction[r]->direction[d] != 0)
            e = e + getAffineDimExpr(d, mapContext) *
                        direction[r]->direction[d];
      return e;
    };
    auto viewOf = [&](unsigned x, unsigned slot) {
      const OperandLayout &layout = layouts[x];
      const int dim = layout.roleDim[slot];
      if (layout.general || dim < 0)
        return OperandCoordinateView{1, 0};
      return OperandCoordinateView{layout.roleCoefficients[dim][slot],
                                   layout.dimConstants[dim]};
    };
    aLoad = lhsLoad;
    bLoad = rhsLoad;
    accessA = operandMap[0];
    accessB = operandMap[1];
    coordinate[0] = SharedCoordinate{exprOf(0), viewOf(0, 0), viewOf(2, 0)};
    coordinate[1] = SharedCoordinate{exprOf(1), viewOf(1, 1), viewOf(2, 1)};
    coordinate[2] = SharedCoordinate{exprOf(2), viewOf(0, 1), viewOf(1, 0)};
    layoutA = layouts[0];
    layoutB = layouts[1];
    layoutC = layouts[2];

    // The batch coordinates: every dimension of C that reads batch loops
    // only is a row of P* (as in the analysis by groups); a batch loop that
    // no such dimension covers -- C reads it in a role dimension, or not at
    // all apart from its factors' views -- is a row of its own.
    logicalPartitions.clear();
    SmallVector<bool> covered(numIterationDims, false);
    for (const Subscript &sub : subscripts) {
      if (sub.operand != 2)
        continue;
      bool batch = false, other = false;
      for (unsigned d = 0; d < numIterationDims; ++d)
        if (sub.onDomain.coefficients[d] != 0)
          (isBatchLoop[d] ? batch : other) = true;
      if (!batch || other)
        continue;
      logicalPartitions.push_back(operandMap[2].getResult(sub.dim));
      for (unsigned d = 0; d < numIterationDims; ++d)
        if (sub.onDomain.coefficients[d] != 0)
          covered[d] = true;
    }
    for (unsigned d = 0; d < numIterationDims; ++d)
      if (isBatchLoop[d] && !covered[d])
        logicalPartitions.push_back(getAffineDimExpr(d, mapContext));
    numPartitionDims = logicalPartitions.size();
    return success();
  };

  // The positional form's partition prefix: read identically by the three
  // operands; its loops are the batch loops.
  auto recordPrefix = [&]() -> LogicalResult {
    // The positional form: its partition prefix is read identically by the
    // three operands; its loops are the batch loops. The prefix dimensions
    // are batch dimensions; the role dimensions get no batch terms here (a
    // role coupled to the prefix, C[b][b + i][j], stays in the logical
    // mapping, where the dense partition refuses it).
    // A prefix subscript constant on the domain (a literal, or a loop that
    // runs once) is no batch coordinate: its dimensions are fixed indices
    // (constantIndex below) and it is no row of P* -- else a plain GEMM with
    // such a prefix would count as batched (SQ-5).
    SmallVector<AffineExpr> batchPartitions;
    for (AffineExpr partition : logicalPartitions) {
      auto form = extractLinearForm(partition, numIterationDims);
      std::optional<LinearForm> onDomain =
          succeeded(form) ? domainForm(*form, single) : std::nullopt;
      if (!onDomain)
        return unknown("a partition (batch) subscript's value on the "
                       "iteration domain overflows 64-bit arithmetic");
      bool varies = false;
      for (unsigned d = 0; d < numIterationDims; ++d)
        if (onDomain->coefficients[d] != 0) {
          isBatchLoop[d] = true;
          varies = true;
        }
      if (varies)
        batchPartitions.push_back(partition);
    }
    logicalPartitions = batchPartitions;
    numPartitionDims = logicalPartitions.size();
    for (OperandLayout *layout : {&layoutA, &layoutB, &layoutC}) {
      const AffineMap &access = layout == &layoutA   ? accessA
                                : layout == &layoutB ? accessB
                                                     : *normalizedAcc;
      layout->batchCoefficients.assign(
          access.getNumResults(), SmallVector<int64_t>(numIterationDims, 0));
      for (int dim : layout->batchDim) {
        auto form = extractLinearForm(access.getResult(dim), numIterationDims);
        std::optional<LinearForm> onDomain =
            succeeded(form) ? domainForm(*form, single) : std::nullopt;
        if (!onDomain)
          return unknown("a partition (batch) subscript's value on the "
                         "iteration domain overflows 64-bit arithmetic");
        layout->batchCoefficients[dim] = onDomain->coefficients;
        layout->constantIndex[dim] = onDomain->constant;
      }
    }
    return success();
  };

  // Which analysis decides. The positional form is tried first, so that
  // every nest it recognized keeps its logical mapping, views and
  // diagnostics. A positional reading in which one loop drives two of its
  // coordinates -- the prefix (as one), I, J, K -- is never lowerable (the
  // dense partition refuses it: "two logical coordinates are driven by the
  // same loop"), and such a loop is read by all three operands: A[b][i][b]
  // with B[b][b][j] (K = b, the prefix loop), A[i][k + j] with B[k + j][j]
  // (K and J both driven by j). For such a reading the analysis by groups
  // decides, classifying every loop by its readers; the positional reading
  // is kept only when that analysis refuses the nest too, with its
  // diagnostics unchanged (C[b][b + i][j], C[i + j][j]).
  auto positionalIsCoupled = [&]() {
    auto onDomainOf = [&](AffineExpr expr) -> std::optional<LinearForm> {
      auto form = extractLinearForm(expr, numIterationDims);
      return succeeded(form) ? domainForm(*form, single) : std::nullopt;
    };
    SmallVector<unsigned> drives(numIterationDims, 0);
    SmallVector<bool> prefixLoop(numIterationDims, false);
    for (AffineExpr partition : logicalPartitions) {
      std::optional<LinearForm> form = onDomainOf(partition);
      if (!form)
        return false;
      for (unsigned d = 0; d < numIterationDims; ++d)
        prefixLoop[d] = prefixLoop[d] || form->coefficients[d] != 0;
    }
    for (unsigned d = 0; d < numIterationDims; ++d)
      drives[d] += prefixLoop[d];
    for (const SharedCoordinate &shared : coordinate) {
      std::optional<LinearForm> form = onDomainOf(shared.expr);
      if (!form)
        return false;
      for (unsigned d = 0; d < numIterationDims; ++d)
        drives[d] += form->coefficients[d] != 0;
    }
    return llvm::any_of(drives, [](unsigned n) { return n >= 2; });
  };
  // The analysis by groups, then -- when it refuses only because a
  // subscript is not role-pure -- the readers analysis, from a clean result.
  auto resetResult = [&]() {
    numPartitionDims = 0;
    aLoad = lhsLoad;
    bLoad = rhsLoad;
    accessA = operandMap[0];
    accessB = operandMap[1];
    coordinate = {};
    layoutA = layoutB = layoutC = OperandLayout();
    logicalPartitions.clear();
    std::fill(isBatchLoop.begin(), isBatchLoop.end(), false);
  };
  auto groupsOrReaders = [&]() -> LogicalResult {
    const std::string savedWhy = why ? *why : std::string();
    const GemmMappingClass savedClass =
        mappingClass ? *mappingClass : GemmMappingClass::ProvenValid;
    relaxableRefusal = false;
    if (succeeded(byGroups()))
      return success();
    if (!relaxableRefusal)
      return failure();
    resetResult();
    if (failed(byReaders()))
      return failure();
    if (why)
      *why = savedWhy;
    if (mappingClass)
      *mappingClass = savedClass;
    return success();
  };
  // The analyses, with the dimension-intersection partitioning (section 21):
  // when an analysis refuses a row or column role as not one dense
  // coordinate, the loops chooseDemotion names become batch loops and the
  // analyses run again, until they succeed or nothing is left to demote
  // (every pass demotes at least one loop, so at most numIterationDims
  // passes). The positional form cannot succeed once it failed, so the
  // retries use the analyses by groups and readers only.
  {
    // The positional form reads the operands by position and knows no
    // demoted loop: with reduction batch loops (section 23) the analyses by
    // groups and readers decide.
    bool positionalForm = !anyReductionBatch && positional();
    for (unsigned pass = 0;; ++pass) {
      demotionCandidate.reset();
      wholeRoleDemotion.reset();
      LogicalResult decided = failure();
      if (pass > 0) {
        resetResult();
        decided = groupsOrReaders();
      } else if (!positionalForm) {
        decided = groupsOrReaders();
      } else if (positionalIsCoupled()) {
        // A coupled positional reading is never lowerable (one loop drives
        // two of its coordinates; that loop is read by all three operands):
        // the classification by readers decides, and its refusal is the
        // reason.
        resetResult();
        decided = groupsOrReaders();
      } else {
        decided = recordPrefix();
      }
      if (succeeded(decided))
        break;
      if ((!demotionCandidate && !wholeRoleDemotion) ||
          pass >= numIterationDims)
        return failure();
      SmallVector<unsigned> toDemote = wholeRoleDemotion
                                           ? *wholeRoleDemotion
                                           : chooseDemotion(*demotionCandidate);
      if (toDemote.empty())
        return failure();
      for (unsigned d : toDemote)
        demoted[d] = true;
    }
  }

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
  // A factor's role dimension may add batch terms (ISSUE-007): the subscript
  // minus those terms is the view; the terms are added per batch value.
  auto subscriptIsView = [&](AffineExpr subscript, const LinearForm &coordinate,
                             const OperandCoordinateView &view,
                             ArrayRef<int64_t> batchTerms) {
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
      std::optional<int64_t> withBatch =
          expected && !batchTerms.empty()
              ? llvm::checkedAdd(*expected, batchTerms[d])
              : expected;
      if (!withBatch || *withBatch != form->coefficients[d])
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
    for (unsigned r = 0; r < 2; ++r) {
      if (layout.roleDim[r] < 0)
        continue;
      ArrayRef<int64_t> batchTerms;
      if (size_t(layout.roleDim[r]) < layout.batchCoefficients.size())
        batchTerms = layout.batchCoefficients[layout.roleDim[r]];
      if (!subscriptIsView(access.getResult(layout.roleDim[r]),
                           r == 0 ? first : second, view[r], batchTerms))
        return false;
    }
    return true;
  };
  if (!operandIsView(accessA, layoutA, *linearI, *linearK, viewA) ||
      !operandIsView(accessB, layoutB, *linearK, *linearJ, viewB) ||
      !operandIsView(*normalizedAcc, layoutC, *linearI, *linearJ, viewC))
    return unknown("an operand's subscript is not its view of the logical "
                   "coordinate on the iteration domain (or the check "
                   "overflows 64-bit arithmetic)");
  // A layout of the readers analysis records every dimension as an affine
  // view of the operand's two coordinates (roleCoefficients, dimConstants)
  // plus its batch terms: checked for every dimension on the domain, in
  // checked arithmetic (G1: every iteration's update is its slot's).
  auto dimensionsAreViews = [&](AffineMap access, const OperandLayout &layout,
                                const LinearForm &first,
                                const LinearForm &second) {
    if (layout.roleCoefficients.empty())
      return true;
    std::optional<LinearForm> firstOnDomain = domainForm(first, single);
    std::optional<LinearForm> secondOnDomain = domainForm(second, single);
    if (!firstOnDomain || !secondOnDomain ||
        layout.roleCoefficients.size() != access.getNumResults())
      return false;
    for (unsigned d = 0; d < access.getNumResults(); ++d) {
      auto expression =
          extractLinearForm(access.getResult(d), numIterationDims);
      if (failed(expression))
        return false;
      std::optional<LinearForm> form = domainForm(*expression, single);
      if (!form)
        return false;
      const int64_t l0 = layout.roleCoefficients[d][0];
      const int64_t l1 = layout.roleCoefficients[d][1];
      for (unsigned s = 0; s < numIterationDims; ++s) {
        std::optional<int64_t> t0 =
            llvm::checkedMul(l0, firstOnDomain->coefficients[s]);
        std::optional<int64_t> t1 =
            llvm::checkedMul(l1, secondOnDomain->coefficients[s]);
        std::optional<int64_t> sum =
            t0 && t1 ? llvm::checkedAdd(*t0, *t1) : std::nullopt;
        std::optional<int64_t> withBatch =
            sum ? llvm::checkedAdd(*sum, layout.batchCoefficients[d][s])
                : std::nullopt;
        if (!withBatch || *withBatch != form->coefficients[s])
          return false;
      }
      std::optional<int64_t> c0 =
          llvm::checkedMul(l0, firstOnDomain->constant);
      std::optional<int64_t> c1 =
          llvm::checkedMul(l1, secondOnDomain->constant);
      std::optional<int64_t> c =
          c0 && c1 ? llvm::checkedAdd(*c0, *c1) : std::nullopt;
      std::optional<int64_t> expected =
          c ? llvm::checkedAdd(*c, layout.dimConstants[d]) : std::nullopt;
      if (!expected || *expected != form->constant)
        return false;
    }
    return true;
  };
  if (!dimensionsAreViews(accessA, layoutA, *linearI, *linearK) ||
      !dimensionsAreViews(accessB, layoutB, *linearK, *linearJ) ||
      !dimensionsAreViews(*normalizedAcc, layoutC, *linearI, *linearJ))
    return unknown("an operand's dimension is not its affine view of the "
                   "logical coordinates on the iteration domain (or the "
                   "check overflows 64-bit arithmetic)");


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

  if (accumulates) {
    dbgOut() << "  accumulator read-modify-write verified\n";
    dbgOut() << "  MAC dataflow recognized\n";
    dbgOut() << "    accumulator load: ";
    accLoad.getOperation()->print(dbgOut());
    dbgOut() << "\n";
  } else {
    dbgOut() << "  assignment C = A * B: every element of C written once "
                "(the K = 1 GEMM into a zeroed C)\n";
  }

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
  // The symbol parts follow the loads the roles were assigned to (A may be
  // the nest's second factor).
  const bool aIsLhs = aLoad == lhsLoad;
  result.symbolicA = aIsLhs ? symbolicLhs : symbolicRhs;
  result.symbolsA = aIsLhs ? symbolsLhs : symbolsRhs;
  result.symbolicB = aIsLhs ? symbolicRhs : symbolicLhs;
  result.symbolsB = aIsLhs ? symbolsRhs : symbolsLhs;
  result.symbolicC = symbolicAcc;
  result.symbolsC = symbolsAcc;

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
  // The program's permission to reassociate (docs section 6): the flag of
  // the accumulation's addf -- or, for an assignment (no addf), of the
  // product's mulf, the nest's only floating-point operation (the systolic
  // execution still starts each tile from +0: a -0 product becomes +0).
  result.reductionMayReassociate = arith::bitEnumContainsAll(
      accumulates ? add.getFastmath() : mul.getFastmath(),
      arith::FastMathFlags::reassoc);
  result.accumulates = accumulates;
  for (unsigned d = 0; d < numIterationDims; ++d) {
    if (demoted[d])
      result.demotedLoops.push_back(d);
    if (demoted[d] && !(readersOf[d] & 0b100))
      result.reductionBatchLoops.push_back(d);
  }

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


  // -------------------------------------------------------------------
  // The exact mapping of the task of 2026-10-17 (Parts 2-6), reported, not
  // decided: C's own indices as I and J, the reduction row as K, over the
  // loops of two or more iterations; K = 1 is the degenerate 2-D path.
  // -------------------------------------------------------------------
  {
    ExactMapping &em = result.exactMapping;
    SmallVector<unsigned> effective;
    for (unsigned d = 0; d < numIterationDims; ++d)
      if (!single[d])
        effective.push_back(d);
    auto project = [&](ArrayRef<int64_t> full) {
      lattice::IntVector v;
      for (unsigned d : effective)
        v.push_back(full[d]);
      return v;
    };
    auto hasLoops = [&](const LinearForm &f) {
      return llvm::any_of(effective,
                          [&](unsigned d) { return f.coefficients[d] != 0; });
    };
    const unsigned cRank = operandMap[2].getNumResults();
    const bool iHas = hasLoops(*linearI), jHas = hasLoops(*linearJ),
               kHas = hasLoops(*linearK);
    const unsigned rolesWithLoops = iHas + jHas + kHas;
    if (!logicalPartitions.empty())
      em.note = "batch coordinates: not the task's three-loop case (the role "
                "mapping decides)";
    else if (cRank != 2)
      em.note = "C has " + std::to_string(cRank) +
                " index(es), not two: not the task's case";
    else if (effective.size() != rolesWithLoops)
      em.note = "a role over several loops: not the task's case (the role "
                "mapping decides)";
    else if (!iHas || !jHas)
      em.note = "M = 1 or N = 1: not the task's non-degenerate case (lowered "
                "by the role mapping, SQ-4)";
    else {
      em.defined = true;
      em.degenerateK = !kHas;
      em.dims = effective.size();
      LinearForm cRows[2];
      SmallVector<LinearForm> aRows, bRows;
      for (const Subscript &sub : subscripts) {
        if (sub.operand == 2)
          cRows[sub.dim] = sub.onDomain;
        else
          (sub.operand == 0 ? aRows : bRows).push_back(sub.onDomain);
      }
      em.T.push_back(project(cRows[0].coefficients));
      em.T.push_back(project(cRows[1].coefficients));
      em.d.push_back(cRows[0].constant);
      em.d.push_back(cRows[1].constant);
      if (!em.degenerateK) {
        em.T.push_back(project(linearK->coefficients));
        em.d.push_back(linearK->constant);
      }
      std::optional<int64_t> det = lattice::determinant(em.T);
      if (!det) {
        em.defined = false;
        em.note = "the determinant does not fit in 64 bits";
      } else {
        em.det = *det;
        em.singular = *det == 0;
        em.unimodular = *det == 1 || *det == -1;
        // T diag(step): a signed permutation iff every logical coordinate is
        // one loop's counter up to sign and the step is one -- then T(D) + d
        // is a complete integer box.
        lattice::IntMatrix scaled = em.T;
        bool fits = true;
        for (auto &row : scaled)
          for (unsigned c = 0; c < effective.size() && fits; ++c) {
            affine::AffineForOp loop =
                affine::getForInductionVarOwner(iterationIVs[effective[c]]);
            std::optional<int64_t> product =
                llvm::checkedMul(row[c], loop ? loop.getStepAsInt() : 1);
            if (!product)
              fits = false;
            else
              row[c] = *product;
          }
        em.boxImage = fits && lattice::isSignedPermutation(scaled);
        auto rankOfRows = [&](ArrayRef<LinearForm> rows) -> unsigned {
          lattice::IntMatrix m;
          for (const LinearForm &r : rows)
            m.push_back(project(r.coefficients));
          std::optional<lattice::ColumnEchelon> e =
              m.empty() ? std::nullopt
                        : lattice::columnEchelon(m, effective.size());
          return e ? e->rank : 0;
        };
        em.rankC = rankOfRows({cRows[0], cRows[1]});
        em.rankA = rankOfRows(aRows);
        em.rankB = rankOfRows(bRows);
        if (!em.degenerateK) {
          std::optional<lattice::ColumnEchelon> cEchelon = lattice::columnEchelon(
              {em.T[0], em.T[1]}, effective.size());
          if (cEchelon && cEchelon->kernel.size() == 1) {
            std::optional<lattice::IntVector> v =
                lattice::primitive(cEchelon->kernel[0]);
            if (v) {
              em.v = *v;
              auto dot = [&](const LinearForm &r) -> std::optional<int64_t> {
                lattice::IntVector p = project(r.coefficients);
                int64_t sum = 0;
                for (unsigned c = 0; c < p.size(); ++c) {
                  std::optional<int64_t> term = llvm::checkedMul(p[c], (*v)[c]);
                  std::optional<int64_t> next =
                      term ? llvm::checkedAdd(sum, *term) : std::nullopt;
                  if (!next)
                    return std::nullopt;
                  sum = *next;
                }
                return sum;
              };
              std::optional<int64_t> dIa = aRows.size() > 0 ? dot(aRows[0]) : 0,
                                     dKa = aRows.size() > 1 ? dot(aRows[1]) : 0,
                                     dKb = bRows.size() > 0 ? dot(bRows[0]) : 0,
                                     dJb = bRows.size() > 1 ? dot(bRows[1]) : 0;
              if (dIa && dKa && dKb && dJb) {
                em.deltas = {*dIa, *dKa, *dKb, *dJb};
                em.aVaries = !(*dIa == 0 && *dKa == 0);
                em.bVaries = !(*dKb == 0 && *dJb == 0);
              }
            }
          }
        }
        if (*det != 0) {
          em.aFreeOfJ = true;
          em.bFreeOfI = true;
          for (const LinearForm &r : aRows) {
            std::optional<lattice::RationalRow> lam =
                lattice::rowTimesInverse(project(r.coefficients), em.T);
            if (!lam) {
              em.aFreeOfJ = false;
              break;
            }
            em.aFreeOfJ = em.aFreeOfJ && lam->numerators[1] == 0;
            em.lambdaA.push_back(*lam);
          }
          for (const LinearForm &r : bRows) {
            std::optional<lattice::RationalRow> lam =
                lattice::rowTimesInverse(project(r.coefficients), em.T);
            if (!lam) {
              em.bFreeOfI = false;
              break;
            }
            em.bFreeOfI = em.bFreeOfI && lam->numerators[0] == 0;
            em.lambdaB.push_back(*lam);
          }
        }
        em.rowsSwapped = aLoad != lhsLoad;
      }
    }
  }

  return result;
}

FailureOr<LogicalMatmul>
mlir::systolic::recognizeLogicalMatmul(
    AffineMatmulCandidate candidate, std::string *why,
    GemmMappingClass *mappingClass,
    const AffineMatmulRecognitionOptions &options) {
  if (!options.reductionBatches)
    return recognizeOnce(candidate, why, mappingClass, options, std::nullopt,
                         nullptr);
  // Reduction-batch partitioning (section 23). First with the loops read by
  // one factor only as reduction batch loops; then, when the loops both
  // factors read and C does not are not one dense GEMM coordinate -- the
  // analyses refuse them (A[i][x0 - x1] * B[x0 + x1][j],
  // A[i][k0][k1] * B[k0 + k1][j]) or K is not dense
  // (A[i][k0 + k1] * B[k0 + k1][j]) -- with each of them in turn as K and the
  // others as reduction batch loops, the longest first (ties: the innermost).
  // The first dense GEMM is taken.
  auto dense = [](const FailureOr<LogicalMatmul> &m) {
    return succeeded(m) && succeeded(getDenseLogicalGemm(*m));
  };
  SmallVector<unsigned> shared;
  std::string ignored;
  GemmMappingClass ignoredClass = GemmMappingClass::ProvenValid;
  FailureOr<LogicalMatmul> first = recognizeOnce(
      candidate, &ignored, &ignoredClass, options, std::nullopt, &shared);
  if (dense(first))
    return first;
  if (shared.size() >= 2)
    for (unsigned k : shared) {
      FailureOr<LogicalMatmul> attempt = recognizeOnce(
          candidate, &ignored, &ignoredClass, options, k, nullptr);
      if (dense(attempt))
        return attempt;
    }
  // No reduction-batch decomposition is a dense GEMM: the recognition
  // without the option decides, with its reason and class.
  return recognizeOnce(candidate, why, mappingClass,
                       AffineMatmulRecognitionOptions(), std::nullopt,
                       nullptr);
}

//===----------------------------------------------------------------------===//
// The geometric screening (docs/AFFINE_GEMM_MAPPING.md section 19, layer 1).
//===----------------------------------------------------------------------===//

FailureOr<AffineGeometryScreening>
mlir::systolic::screenAffineMatmulGeometry(AffineMatmulCandidate candidate,
                                           std::string *why) {
  auto fail = [&](const llvm::Twine &reason)
      -> FailureOr<AffineGeometryScreening> {
    if (why)
      *why = reason.str();
    return failure();
  };

  // The nest: the perfect chain from the anchor and the one MAC inside it
  // (the same shape recognizeLogicalMatmul requires).
  auto outer = dyn_cast_or_null<affine::AffineForOp>(candidate.anchor);
  if (!outer)
    return fail("the anchor is not an affine.for");
  SmallVector<affine::AffineForOp> chain{outer};
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
  SmallVector<affine::AffineLoadOp> loads;
  SmallVector<affine::AffineStoreOp> stores;
  for (Operation &op : *chain.back().getBody()) {
    if (auto load = dyn_cast<affine::AffineLoadOp>(op))
      loads.push_back(load);
    else if (auto store = dyn_cast<affine::AffineStoreOp>(op))
      stores.push_back(store);
    else if (!isa<arith::MulFOp, arith::AddFOp, affine::AffineYieldOp>(op))
      return fail("the innermost body is not one multiply-accumulate");
  }
  if (loads.size() != 3 || stores.size() != 1)
    return fail("the innermost body is not one multiply-accumulate");
  auto add = stores.front().getValue().getDefiningOp<arith::AddFOp>();
  if (!add)
    return fail("the stored value is not an arith.addf");
  affine::AffineLoadOp accLoad;
  arith::MulFOp mul;
  for (auto [x, y] : {std::pair{add.getLhs(), add.getRhs()},
                      std::pair{add.getRhs(), add.getLhs()}}) {
    auto load = x.getDefiningOp<affine::AffineLoadOp>();
    auto product = y.getDefiningOp<arith::MulFOp>();
    if (load && product) {
      accLoad = load;
      mul = product;
      break;
    }
  }
  if (!mul)
    return fail("the stored value is not a load plus a product");
  auto firstLoad = mul.getLhs().getDefiningOp<affine::AffineLoadOp>();
  auto secondLoad = mul.getRhs().getDefiningOp<affine::AffineLoadOp>();
  if (!firstLoad || !secondLoad)
    return fail("a factor of the product is not an affine.load");

  AffineGeometryScreening result;
  SmallVector<Value> ivs;
  SmallVector<int64_t> steps;
  for (auto [position, loop] : llvm::enumerate(chain)) {
    ivs.push_back(loop.getInductionVar());
    std::optional<int64_t> lower = constantLoopBound(loop, /*lower=*/true);
    std::optional<int64_t> upper = constantLoopBound(loop, /*lower=*/false);
    if (!lower)
      return fail(describeNonConstantBound(loop, position, /*lower=*/true));
    if (!upper)
      return fail(describeNonConstantBound(loop, position, /*lower=*/false));
    if (*upper <= *lower)
      return fail("loop " + llvm::Twine(position) + " has no iteration");
    const int64_t step = loop.getStepAsInt();
    std::optional<int64_t> trip = checkedTripCount(*lower, *upper, step);
    if (!trip)
      return fail("loop " + llvm::Twine(position) +
                  "'s trip count overflows 64-bit arithmetic");
    AffineGeometryScreening::Loop info;
    info.trip = *trip;
    result.loops.push_back(info);
    steps.push_back(step);
  }
  const unsigned n = chain.size();

  // The accesses on the ordinals: coefficient x step.
  std::array<affine::AffineLoadOp, 3> accesses{firstLoad, secondLoad,
                                                     accLoad};
  for (unsigned x = 0; x < 3; ++x) {
    FailureOr<AffineMap> map = normalizeAccessMap(
        accesses[x].getMap(), accesses[x].getIndices(), ivs);
    if (failed(map))
      return fail("an access index is not a counter of the nest's loops, or "
                  "an access map has symbols");
    for (AffineExpr expr : map->getResults()) {
      FailureOr<LinearForm> form = extractLinearForm(expr, n);
      if (failed(form))
        return fail("a subscript is not a linear form of the loop counters");
      lattice::IntVector row;
      for (unsigned s = 0; s < n; ++s) {
        std::optional<int64_t> c =
            llvm::checkedMul(form->coefficients[s], steps[s]);
        if (!c)
          return fail("an ordinal coefficient overflows 64-bit arithmetic");
        row.push_back(*c);
      }
      result.ordinal[x].push_back(std::move(row));
      result.written[x].push_back(lattice::IntVector(
          form->coefficients.begin(), form->coefficients.begin() + n));
    }
  }

  // Readers, classes, the reduction loops.
  SmallVector<unsigned> fullLoops, roleLoops;
  for (unsigned s = 0; s < n; ++s) {
    AffineGeometryScreening::Loop &loop = result.loops[s];
    if (loop.trip < 2)
      continue;
    fullLoops.push_back(s);
    for (unsigned x = 0; x < 3; ++x)
      for (const lattice::IntVector &row : result.ordinal[x])
        if (row[s] != 0)
          loop.readers |= 1u << x;
    switch (loop.readers) {
    case 0b101:
      loop.role = 'I';
      break;
    case 0b110:
      loop.role = 'J';
      break;
    case 0b011:
      loop.role = 'K';
      break;
    case 0b111:
      loop.role = 'P';
      break;
    case 0:
      loop.role = '-';
      break;
    default:
      loop.role = '1';
      break;
    }
    if (loop.role == 'K')
      result.reductionLoops.push_back(s);
    if (loop.role != 'P')
      roleLoops.push_back(s);
  }

  auto restrictTo = [&](const lattice::IntMatrix &rows,
                        ArrayRef<unsigned> columns) {
    lattice::IntMatrix out;
    for (const lattice::IntVector &row : rows) {
      lattice::IntVector v;
      for (unsigned s : columns)
        v.push_back(row[s]);
      out.push_back(std::move(v));
    }
    return out;
  };
  auto rankOf = [&](const lattice::IntMatrix &m,
                    unsigned columns) -> std::optional<unsigned> {
    if (m.empty() || columns == 0)
      return 0u;
    std::optional<lattice::ColumnEchelon> e = lattice::columnEchelon(m, columns);
    return e ? std::optional<unsigned>(e->rank) : std::nullopt;
  };
  lattice::IntMatrix all;
  for (unsigned x = 0; x < 3; ++x)
    all.append(result.ordinal[x].begin(), result.ordinal[x].end());
  std::optional<unsigned> cFull =
      rankOf(restrictTo(result.ordinal[2], fullLoops), fullLoops.size());
  std::optional<unsigned> cRoles =
      rankOf(restrictTo(result.ordinal[2], roleLoops), roleLoops.size());
  std::optional<unsigned> aFull =
      rankOf(restrictTo(all, fullLoops), fullLoops.size());
  std::optional<unsigned> aRoles =
      rankOf(restrictTo(all, roleLoops), roleLoops.size());
  if (!cFull || !cRoles || !aFull || !aRoles)
    return fail("a rank computation overflows 64-bit arithmetic");
  result.cRankFull = *cFull;
  result.cRankRoles = *cRoles;
  result.allRankFull = *aFull;
  result.allRankRoles = *aRoles;

  // C's integer kernel on the loops of two or more iterations.
  if (!fullLoops.empty()) {
    lattice::IntMatrix cRows = restrictTo(result.ordinal[2], fullLoops);
    lattice::IntMatrix basis;
    if (cRows.empty()) {
      for (unsigned i = 0; i < fullLoops.size(); ++i) {
        lattice::IntVector e(fullLoops.size(), 0);
        e[i] = 1;
        basis.push_back(std::move(e));
      }
    } else {
      std::optional<lattice::ColumnEchelon> e =
          lattice::columnEchelon(cRows, fullLoops.size());
      if (!e)
        return fail("C's kernel overflows 64-bit arithmetic");
      basis = e->kernel;
    }
    for (const lattice::IntVector &v : basis) {
      lattice::IntVector expanded(n, 0);
      bool joins = true, beyondReduction = false;
      for (auto [i, s] : llvm::enumerate(fullLoops)) {
        expanded[s] = v[i];
        std::optional<int64_t> magnitude = lattice::checkedAbs(v[i]);
        if (!magnitude || *magnitude >= result.loops[s].trip)
          joins = false;
        if (v[i] != 0 && result.loops[s].role != 'K')
          beyondReduction = true;
      }
      if (joins && beyondReduction)
        result.cKernelBeyondReduction.push_back(expanded);
      result.cKernel.push_back(std::move(expanded));
    }
  }

  // The six-vector conditions, for operands of two dimensions each.
  auto conditions = [&](ArrayRef<unsigned> columns)
      -> std::optional<AffineGeometryScreening::SixVectorConditions> {
    auto vector = [&](unsigned x, unsigned d) {
      lattice::IntVector v;
      for (unsigned s : columns)
        v.push_back(result.ordinal[x][d][s]);
      return v;
    };
    const lattice::IntVector cI = vector(2, 0), cJ = vector(2, 1);
    const lattice::IntVector aI = vector(0, 0), aK = vector(0, 1);
    const lattice::IntVector bK = vector(1, 0), bJ = vector(1, 1);
    auto rank = [&](lattice::IntMatrix m) {
      return rankOf(m, columns.size()).value_or(~0u);
    };
    AffineGeometryScreening::SixVectorConditions c;
    c.a = rank({cI, cJ}) == 2;
    c.b = rank({cI, cJ, aI, aK, bK, bJ}) == 3;
    c.d1 = rank({aI, aK}) == 2;
    c.d2 = rank({bK, bJ}) == 2;
    c.d3 = rank({aI, bJ}) == 2;
    c.e = rank({aI, aK, bK, bJ}) == 3;
    // Some d in ker [cI; cJ] with aK.d != 0 and bK.d != 0: exists iff
    // neither aK nor bK vanishes on the whole kernel; then u + t w works for
    // all but at most two integers t.
    auto dot = [](const lattice::IntVector &p, const lattice::IntVector &q)
        -> std::optional<int64_t> {
      int64_t sum = 0;
      for (auto [x, y] : llvm::zip(p, q)) {
        std::optional<int64_t> product = llvm::checkedMul(x, y);
        std::optional<int64_t> next =
            product ? llvm::checkedAdd(sum, *product) : std::nullopt;
        if (!next)
          return std::nullopt;
        sum = *next;
      }
      return sum;
    };
    lattice::IntMatrix kernel;
    if (columns.empty())
      return c;
    if (std::optional<lattice::ColumnEchelon> e =
            lattice::columnEchelon({cI, cJ}, columns.size()))
      kernel = e->kernel;
    else
      return std::nullopt;
    // The task algorithm's null-space tests, on the whole kernel basis: a
    // vector is "dot(v, null(Ic, Jc)) != 0" iff some basis vector has a
    // nonzero dot with it (the basis spans the kernel, so this does not
    // depend on the basis chosen). An empty kernel (nullity 0) makes every
    // dot zero: A and B then vary along nothing that keeps C fixed.
    auto variesAlongKernel = [&](const lattice::IntVector &v)
        -> std::optional<bool> {
      for (const lattice::IntVector &k : kernel) {
        std::optional<int64_t> product = dot(v, k);
        if (!product)
          return std::nullopt;
        if (*product != 0)
          return true;
      }
      return false;
    };
    std::optional<bool> aIv = variesAlongKernel(aI),
                        aKv = variesAlongKernel(aK),
                        bKv = variesAlongKernel(bK),
                        bJv = variesAlongKernel(bJ);
    if (!aIv || !aKv || !bKv || !bJv)
      return std::nullopt;
    c.aVaries = *aIv || *aKv;
    c.bVaries = *bKv || *bJv;
    c.swapA = *aIv && !*aKv;
    c.swapB = !*bKv && *bJv;
    c.passes = c.a && c.d1 && c.d2 && c.b && c.e && c.aVaries && c.bVaries;
    const lattice::IntVector *u = nullptr, *w = nullptr;
    for (const lattice::IntVector &v : kernel) {
      std::optional<int64_t> av = dot(aK, v), bv = dot(bK, v);
      if (!av || !bv)
        return std::nullopt;
      if (*av != 0 && !u)
        u = &v;
      if (*bv != 0 && !w)
        w = &v;
    }
    if (u && w) {
      for (int64_t t = 0; t <= 2; ++t) {
        lattice::IntVector d(columns.size());
        bool ok = true;
        for (unsigned i = 0; i < d.size(); ++i) {
          std::optional<int64_t> tw = llvm::checkedMul(t, (*w)[i]);
          std::optional<int64_t> sum =
              tw ? llvm::checkedAdd((*u)[i], *tw) : std::nullopt;
          if (!sum) {
            ok = false;
            break;
          }
          d[i] = *sum;
        }
        std::optional<int64_t> av = ok ? dot(aK, d) : std::nullopt;
        std::optional<int64_t> bv = ok ? dot(bK, d) : std::nullopt;
        if (av && bv && *av != 0 && *bv != 0) {
          lattice::IntVector expanded(n, 0);
          for (auto [i, s] : llvm::enumerate(columns))
            expanded[s] = d[i];
          c.c = true;
          c.reductionDirection = std::move(expanded);
          break;
        }
      }
    }
    return c;
  };
  if (result.ordinal[0].size() == 2 && result.ordinal[1].size() == 2 &&
      result.ordinal[2].size() == 2) {
    result.conditionsFull = conditions(fullLoops);
    result.conditionsRoles = conditions(roleLoops);
  }
  return result;
}


//===----------------------------------------------------------------------===//
// R8, the structural recognition relation (docs/AFFINE_GEMM_MAPPING.md
// section 22): dot products and ranks on the counters as written.
//===----------------------------------------------------------------------===//

std::optional<AffineR8Structure>
mlir::systolic::computeR8Structure(const std::array<lattice::IntMatrix, 3> &rows,
                                   ArrayRef<int64_t> trips) {
  const unsigned n = trips.size();
  for (const lattice::IntMatrix &m : rows)
    for (const lattice::IntVector &row : m)
      if (row.size() != n)
        return std::nullopt;

  AffineR8Structure r;
  r.readers.assign(n, 0);
  SmallVector<unsigned> live, batch;
  for (unsigned s = 0; s < n; ++s) {
    if (trips[s] < 2)
      continue;
    live.push_back(s);
    for (unsigned x = 0; x < 3; ++x)
      for (const lattice::IntVector &row : rows[x])
        if (row[s] != 0)
          r.readers[s] |= 1u << x;
    if (r.readers[s] == 0b011)
      r.contractionLoops.push_back(s);
    else if (r.readers[s] == 0b111)
      batch.push_back(s);
    if (r.readers[s] == 0b001 || r.readers[s] == 0b010 ||
        r.readers[s] == 0b011)
      r.reductionLoops.push_back(s);
  }

  // The rows restricted to `columns`, zero rows dropped (they read none of
  // the columns and add nothing to a rank or a dot product).
  auto restrictTo = [](const lattice::IntMatrix &m,
                       ArrayRef<unsigned> columns) {
    lattice::IntMatrix out;
    for (const lattice::IntVector &row : m) {
      lattice::IntVector v;
      bool nonzero = false;
      for (unsigned s : columns) {
        v.push_back(row[s]);
        nonzero |= row[s] != 0;
      }
      if (!nonzero)
        continue;
      // A row divided by the gcd of its entries: ranks and the ranks of
      // products do not change, and large proportional rows do not overflow.
      std::optional<lattice::IntVector> reduced = lattice::primitive(v);
      out.push_back(reduced ? std::move(*reduced) : std::move(v));
    }
    return out;
  };
  auto rankOf = [](const lattice::IntMatrix &m,
                   unsigned columns) -> std::optional<unsigned> {
    if (m.empty() || columns == 0)
      return 0u;
    std::optional<lattice::ColumnEchelon> e = lattice::columnEchelon(m, columns);
    return e ? std::optional<unsigned>(e->rank) : std::nullopt;
  };
  auto dot = [](ArrayRef<int64_t> u,
                ArrayRef<int64_t> v) -> std::optional<int64_t> {
    int64_t sum = 0;
    for (auto [a, b] : llvm::zip_equal(u, v)) {
      std::optional<int64_t> p = llvm::checkedMul(a, b);
      if (!p)
        return std::nullopt;
      std::optional<int64_t> t = llvm::checkedAdd(sum, *p);
      if (!t)
        return std::nullopt;
      sum = *t;
    }
    return sum;
  };
  // rank(P Q^T) of two restricted row sets: R8's coupling.
  auto couplingOf = [&](const lattice::IntMatrix &p,
                        const lattice::IntMatrix &q) -> std::optional<unsigned> {
    if (p.empty() || q.empty())
      return 0u;
    lattice::IntMatrix gram;
    for (const lattice::IntVector &u : p) {
      lattice::IntVector g;
      for (const lattice::IntVector &v : q) {
        std::optional<int64_t> d = dot(u, v);
        if (!d)
          return std::nullopt;
        g.push_back(*d);
      }
      gram.push_back(std::move(g));
    }
    return rankOf(gram, q.size());
  };

  std::array<lattice::IntMatrix, 3> onLive;
  for (unsigned x = 0; x < 3; ++x) {
    onLive[x] = restrictTo(rows[x], live);
    std::optional<unsigned> rank = rankOf(onLive[x], live.size());
    if (!rank)
      return std::nullopt;
    r.operandRank[x] = *rank;
  }
  std::optional<unsigned> ab = couplingOf(onLive[0], onLive[1]);
  std::optional<unsigned> ac = couplingOf(onLive[0], onLive[2]);
  std::optional<unsigned> bc = couplingOf(onLive[1], onLive[2]);
  if (!ab || !ac || !bc)
    return std::nullopt;
  r.literalAB = *ab;
  r.literalAC = *ac;
  r.literalBC = *bc;
  for (const lattice::IntVector &u : onLive[0])
    for (const lattice::IntVector &v : onLive[1]) {
      std::optional<int64_t> d = dot(u, v);
      if (!d)
        return std::nullopt;
      r.pairsAB += *d != 0;
    }

  const lattice::IntMatrix p = restrictTo(rows[0], r.reductionLoops);
  const lattice::IntMatrix q = restrictTo(rows[1], r.reductionLoops);
  lattice::IntMatrix stacked = p;
  stacked.append(q.begin(), q.end());
  const unsigned width = r.reductionLoops.size();
  std::optional<unsigned> rp = rankOf(p, width), rq = rankOf(q, width),
                          rs = rankOf(stacked, width),
                          kappa = couplingOf(p, q),
                          beta = couplingOf(restrictTo(rows[0], batch),
                                            restrictTo(rows[1], batch));
  if (!rp || !rq || !rs || !kappa || !beta)
    return std::nullopt;
  r.rankFirst = *rp;
  r.rankSecond = *rq;
  r.rankStacked = *rs;
  r.coupling = *kappa;
  r.batchCoupling = *beta;
  r.intersect = r.coupling > 0;
  r.sharedLoops = !r.contractionLoops.empty();

  // Groups of S: loops joined by a subscript of a factor that reads both.
  SmallVector<unsigned> parent(n);
  std::iota(parent.begin(), parent.end(), 0u);
  auto find = [&](unsigned s) {
    while (parent[s] != s)
      s = parent[s] = parent[parent[s]];
    return s;
  };
  for (unsigned x = 0; x < 2; ++x)
    for (const lattice::IntVector &row : rows[x]) {
      std::optional<unsigned> first;
      for (unsigned s : r.contractionLoops) {
        if (row[s] == 0)
          continue;
        if (!first)
          first = s;
        else
          parent[find(s)] = find(*first);
      }
    }
  for (unsigned s : r.contractionLoops) {
    auto it = llvm::find_if(r.contractionGroups,
                            [&](const SmallVector<unsigned> &g) {
                              return find(g.front()) == find(s);
                            });
    if (it == r.contractionGroups.end())
      r.contractionGroups.push_back({s});
    else
      it->push_back(s);
  }
  return r;
}

bool mlir::systolic::countersRewritten(
    ArrayRef<DomainNormalization> normalized, Operation *anchor) {
  for (const DomainNormalization &dn : normalized) {
    bool identity = true;
    for (auto [r, row] : llvm::enumerate(dn.U))
      for (auto [c, v] : llvm::enumerate(row))
        identity &= v == (r == c ? 1 : 0);
    if (!identity && (dn.outermost == anchor ||
                      dn.outermost->isProperAncestor(anchor) ||
                      anchor->isProperAncestor(dn.outermost)))
      return true;
  }
  return false;
}

// ---------------------------------------------------------------------------
// Domain normalization (docs/AFFINE_GEMM_MAPPING.md sections 19.7, 19.12):
// constant-extent parallelotopes rewritten into box nests, x = U x'.
// ---------------------------------------------------------------------------

namespace {

/// A bound of `loop`: its linear form over the band's counters (coefficient
/// per band position) and constant, when the map has one result (or several
/// constant results, folded as max / min), no symbols, and every operand is
/// an outer counter of the band. nullopt otherwise.
struct BandBound {
  SmallVector<int64_t> coefficients; // over the band positions
  int64_t constant = 0;
};

std::optional<BandBound> bandBound(affine::AffineForOp loop, bool lower,
                                   ArrayRef<affine::AffineForOp> band,
                                   unsigned position) {
  AffineMap map = lower ? loop.getLowerBoundMap() : loop.getUpperBoundMap();
  ValueRange operands =
      lower ? loop.getLowerBoundOperands() : loop.getUpperBoundOperands();
  if (map.getNumSymbols() != 0 || map.getNumResults() == 0)
    return std::nullopt;
  // Every result as a linear form over the band's counters; several results
  // (max of lower bounds, min of upper bounds) must share one linear part
  // (docs section 19.7: "max/min of such forms with one linear part") and
  // fold to that part plus the max / min of their constants.
  std::optional<BandBound> bound;
  for (AffineExpr result : map.getResults()) {
    FailureOr<LinearForm> form = extractLinearForm(result, map.getNumDims());
    if (failed(form))
      return std::nullopt;
    BandBound one;
    one.coefficients.assign(band.size(), 0);
    one.constant = form->constant;
    for (auto [j, operand] : llvm::enumerate(operands)) {
      if (form->coefficients[j] == 0)
        continue;
      std::optional<unsigned> at;
      for (unsigned t = 0; t < position; ++t) {
        affine::AffineForOp outer = band[t];
        if (outer.getInductionVar() == operand)
          at = t;
      }
      if (!at)
        return std::nullopt; // not an outer counter of the band
      std::optional<int64_t> sum =
          llvm::checkedAdd(one.coefficients[*at], form->coefficients[j]);
      if (!sum)
        return std::nullopt;
      one.coefficients[*at] = *sum;
    }
    if (!bound) {
      bound = one;
      continue;
    }
    if (bound->coefficients != one.coefficients)
      return std::nullopt; // max / min of different linear parts: not folded
    bound->constant = lower ? std::max(bound->constant, one.constant)
                            : std::min(bound->constant, one.constant);
  }
  return bound;
}

/// The band's position of a counter value, if it is one.
std::optional<unsigned> bandPosition(Value value,
                                     ArrayRef<affine::AffineForOp> band) {
  for (auto [t, loop] : llvm::enumerate(band)) {
    affine::AffineForOp candidate = loop;
    if (candidate.getInductionVar() == value)
      return t;
  }
  return std::nullopt;
}

// ---------------------------------------------------------------------------
// Exact domains (docs/AFFINE_GEMM_MAPPING.md section 19.13): bounds that are
// not one affine form of the outer counters at both ends -- a triangle that
// degenerates, a max / min of forms with different linear parts -- whose
// integer iteration set is nevertheless a constant-extent parallelotope.
// Decided with the Presburger library, which is integer-exact: the set is
// built from the bounds, the parallelotope's shift rows are taken from the
// bounds' own results, and the two sets are compared for equality.
// ---------------------------------------------------------------------------

/// One result of a loop bound as a linear form over the band positions.
struct BandForm {
  SmallVector<int64_t> coefficients; // over the band positions
  int64_t constant = 0;
};

/// An expression of a bound map of the loop at band position `position`, over
/// the map's `operands` (dims only), as a linear form over the band
/// positions: every operand it reads an outer counter of the band, no
/// floordiv / ceildiv / mod, checked 64-bit arithmetic. nullopt otherwise.
std::optional<BandForm> bandFormOf(AffineExpr expr, unsigned numDims,
                                   ValueRange operands,
                                   ArrayRef<affine::AffineForOp> band,
                                   unsigned position) {
  FailureOr<LinearForm> form = extractLinearForm(expr, numDims);
  if (failed(form))
    return std::nullopt;
  BandForm one;
  one.coefficients.assign(band.size(), 0);
  one.constant = form->constant;
  for (auto [j, operand] : llvm::enumerate(operands)) {
    if (form->coefficients[j] == 0)
      continue;
    std::optional<unsigned> at;
    for (unsigned t = 0; t < position; ++t) {
      affine::AffineForOp outer = band[t];
      if (outer.getInductionVar() == operand)
        at = t;
    }
    if (!at)
      return std::nullopt; // not an outer counter of the band
    std::optional<int64_t> sum =
        llvm::checkedAdd(one.coefficients[*at], form->coefficients[j]);
    if (!sum)
      return std::nullopt;
    one.coefficients[*at] = *sum;
  }
  return one;
}

/// Every result of a loop bound (max of the lower, min of the upper ones) as
/// a linear form over the outer counters of the band: no symbols, every
/// operand an outer band counter, no floordiv / ceildiv / mod. nullopt
/// otherwise.
std::optional<SmallVector<BandForm>>
bandBoundResults(affine::AffineForOp loop, bool lower,
                 ArrayRef<affine::AffineForOp> band, unsigned position) {
  AffineMap map = lower ? loop.getLowerBoundMap() : loop.getUpperBoundMap();
  ValueRange operands =
      lower ? loop.getLowerBoundOperands() : loop.getUpperBoundOperands();
  if (map.getNumSymbols() != 0 || map.getNumResults() == 0)
    return std::nullopt;
  SmallVector<BandForm> out;
  for (AffineExpr result : map.getResults()) {
    std::optional<BandForm> one =
        bandFormOf(result, map.getNumDims(), operands, band, position);
    if (!one)
      return std::nullopt;
    out.push_back(std::move(*one));
  }
  return out;
}

using BigInt = llvm::DynamicAPInt;

/// A row of `cols` zeros.
SmallVector<BigInt> zeroRow(unsigned cols) {
  return SmallVector<BigInt>(cols, BigInt(0));
}

/// The band's iteration set, exactly: a set over its n counters with one
/// local quotient per stepped loop. Every lower-bound result is a lower bound
/// (affine.for takes their max), every upper-bound result an exclusive upper
/// bound (their min), a step sigma > 1 the congruence x_s = lb(x) + sigma q
/// relative to the lower-bound result `strideBase[s]` -- the one that is the
/// max at every outer point entering the loop (the caller establishes it). Coefficients are copied exactly; nothing is
/// approximated (unlike FlatAffineValueConstraints::addAffineForOpDomain,
/// which drops the stride of a stepped loop with a non-constant bound).
presburger::IntegerPolyhedron
exactBandDomain(ArrayRef<SmallVector<BandForm>> lowers,
                ArrayRef<SmallVector<BandForm>> uppers,
                ArrayRef<int64_t> steps, ArrayRef<unsigned> strideBase) {
  const unsigned n = steps.size();
  const unsigned numLocals =
      llvm::count_if(steps, [](int64_t step) { return step > 1; });
  presburger::IntegerPolyhedron set(
      presburger::PresburgerSpace::getSetSpace(n, 0, numLocals));
  const unsigned cols = set.getNumCols();
  unsigned local = 0;
  for (unsigned s = 0; s < n; ++s) {
    for (const BandForm &lb : lowers[s]) {
      SmallVector<BigInt> row = zeroRow(cols); // x_s - lb(x) >= 0
      row[s] = BigInt(1);
      for (unsigned t = 0; t < s; ++t)
        row[t] = -BigInt(lb.coefficients[t]);
      row.back() = -BigInt(lb.constant);
      set.addInequality(row);
    }
    for (const BandForm &ub : uppers[s]) {
      SmallVector<BigInt> row = zeroRow(cols); // ub(x) - 1 - x_s >= 0
      row[s] = BigInt(-1);
      for (unsigned t = 0; t < s; ++t)
        row[t] = BigInt(ub.coefficients[t]);
      row.back() = BigInt(ub.constant) - BigInt(1);
      set.addInequality(row);
    }
    if (steps[s] > 1) {
      const BandForm &lb = lowers[s][strideBase[s]];
      SmallVector<BigInt> row = zeroRow(cols); // x_s - lb(x) - sigma q = 0
      row[s] = BigInt(1);
      for (unsigned t = 0; t < s; ++t)
        row[t] = -BigInt(lb.coefficients[t]);
      row[n + local] = -BigInt(steps[s]);
      row.back() = -BigInt(lb.constant);
      set.addEquality(row);
      ++local;
    }
  }
  return set;
}

/// `set` over its first `keep` dims, the others existential (an exact
/// integer projection).
presburger::IntegerPolyhedron prefixOf(const presburger::IntegerPolyhedron &set,
                                       unsigned keep) {
  presburger::IntegerPolyhedron prefix = set;
  const unsigned dims = prefix.getNumDimVars();
  if (keep < dims)
    prefix.convertVarKind(presburger::VarKind::SetDim, keep, dims,
                          presburger::VarKind::Local);
  return prefix;
}

/// The integer minimum of coef . x + constant over `set` (coef over its first
/// coef.size() dims), exactly: the first coordinate of the lexicographic
/// integer minimum of the set with the expression inserted as its first
/// dim. nullopt when the set is empty or the expression unbounded below.
std::optional<BigInt> integerMinimum(const presburger::IntegerPolyhedron &set,
                                     ArrayRef<int64_t> coef, int64_t constant) {
  presburger::IntegerPolyhedron withValue = set;
  withValue.insertVar(presburger::VarKind::SetDim, 0);
  SmallVector<BigInt> row = zeroRow(withValue.getNumCols());
  row[0] = BigInt(1); // t - (coef . x + constant) = 0
  for (unsigned t = 0; t < coef.size(); ++t)
    row[1 + t] = -BigInt(coef[t]);
  row.back() = -BigInt(constant);
  withValue.addEquality(row);
  presburger::MaybeOptimum<SmallVector<BigInt, 8>> lexmin =
      withValue.findIntegerLexMin();
  if (!lexmin.isBounded())
    return std::nullopt;
  return (*lexmin)[0];
}

std::optional<int64_t> toInt64(const BigInt &value) {
  if (value > BigInt(std::numeric_limits<int64_t>::max()) ||
      value < BigInt(std::numeric_limits<int64_t>::min()))
    return std::nullopt;
  return int64_t(value);
}

/// Whether form . x has its exact minimum and maximum over `outer` (a set
/// over the first `s` band counters, not empty) within 64 bits.
bool rangeFitsInt64(const presburger::IntegerPolyhedron &outer,
                    const BandForm &form, unsigned s) {
  if (s == 0)
    return true; // a constant
  SmallVector<int64_t> coef(form.coefficients.begin(),
                            form.coefficients.begin() + s);
  SmallVector<int64_t> negated(s, 0);
  for (unsigned t = 0; t < s; ++t) {
    std::optional<int64_t> minus = llvm::checkedSub(int64_t(0), coef[t]);
    if (!minus)
      return false;
    negated[t] = *minus;
  }
  std::optional<int64_t> negatedConstant =
      llvm::checkedSub(int64_t(0), form.constant);
  if (!negatedConstant)
    return false;
  std::optional<BigInt> lowest = integerMinimum(outer, coef, form.constant);
  std::optional<BigInt> negatedHighest =
      integerMinimum(outer, negated, *negatedConstant);
  return lowest && negatedHighest && toInt64(*lowest) &&
         toInt64(-*negatedHighest);
}

/// The run time evaluates every result of a loop's bounds (affine.for takes
/// their max / min) at every outer point that enters the loop, in 64-bit
/// arithmetic: lower-affine emits a coefficient times a counter as
/// `arith.muli ... overflow<nsw>` -- poison when the product does not fit --
/// and a sum as a plain `arith.addi`, which wraps and so gives the true value
/// whenever that fits. So the mathematical set is the one the program runs
/// when every product and every result of every written bound -- results a
/// fold drops included -- has its exact range over those outer points within
/// 64 bits. `outer` is the outer loops' own iteration set (exactBandDomain of
/// the loops outside `loop`, band position `s`); when it is empty the bounds
/// are never evaluated.
bool writtenBoundsFitInt64(affine::AffineForOp loop,
                           ArrayRef<affine::AffineForOp> band, unsigned s,
                           const presburger::IntegerPolyhedron &outer) {
  if (s > 0 && outer.isIntegerEmpty())
    return true;
  for (bool lower : {true, false}) {
    AffineMap map = lower ? loop.getLowerBoundMap() : loop.getUpperBoundMap();
    ValueRange operands =
        lower ? loop.getLowerBoundOperands() : loop.getUpperBoundOperands();
    if (map.getNumSymbols() != 0)
      return false;
    for (AffineExpr result : map.getResults()) {
      bool fits = true;
      result.walk([&](AffineExpr node) {
        if (!fits || (node != result && node.getKind() != AffineExprKind::Mul))
          return;
        std::optional<BandForm> form =
            bandFormOf(node, map.getNumDims(), operands, band, s);
        fits = form && rangeFitsInt64(outer, *form, s);
      });
      if (!fits)
        return false;
    }
  }
  return true;
}

/// A band's exact iteration set (exactBandDomain) and the bound results it
/// is built from: per loop every written result of both bounds -- a stepped
/// loop's lower results folded to one per linear part, the largest constant
/// -- and the lower result its stride counts from.
struct ExactBandSet {
  SmallVector<SmallVector<BandForm>> lowers, uppers;
  SmallVector<int64_t> steps;
  SmallVector<unsigned> strideBases;
  presburger::IntegerPolyhedron domain{
      presburger::PresburgerSpace::getSetSpace(0)};
};

/// The band's exact integer iteration set, when it is the set the program
/// runs. nullopt when a bound result is not a linear form of the band's
/// outer counters, a step is below 1, a stepped loop has a max of lower
/// bounds of which none is the max at every outer point entering it, or a
/// product or result of a written bound can leave 64 bits at such a point
/// (writtenBoundsFitInt64). An empty set is returned as such.
std::optional<ExactBandSet> exactBandSet(ArrayRef<affine::AffineForOp> band) {
  const unsigned n = band.size();
  ExactBandSet out;
  for (unsigned s = 0; s < n; ++s) {
    affine::AffineForOp loop = band[s];
    std::optional<SmallVector<BandForm>> lo =
        bandBoundResults(loop, /*lower=*/true, band, s);
    std::optional<SmallVector<BandForm>> hi =
        bandBoundResults(loop, /*lower=*/false, band, s);
    if (!lo || !hi)
      return std::nullopt;
    const int64_t step = loop.getStepAsInt();
    if (step < 1)
      return std::nullopt;
    presburger::IntegerPolyhedron outer =
        exactBandDomain(out.lowers, out.uppers, out.steps, out.strideBases);
    if (!writtenBoundsFitInt64(loop, band, s, outer))
      return std::nullopt;
    // A stepped loop starts at the max of its lower bounds and counts from
    // there. Results with one linear part fold to the largest constant; with
    // several linear parts, one of them must be the max at every outer point
    // that enters the loop -- then the congruence is relative to it at every
    // such point. Decided exactly: for every other result j, no point of the
    // outer loops' own iteration set has lb_j(x) >= lb_r(x) + 1.
    unsigned base = 0;
    if (step > 1 && lo->size() > 1) {
      SmallVector<BandForm> folded;
      for (const BandForm &form : *lo) {
        auto same = llvm::find_if(folded, [&](const BandForm &f) {
          return f.coefficients == form.coefficients;
        });
        if (same == folded.end())
          folded.push_back(form);
        else
          same->constant = std::max(same->constant, form.constant);
      }
      lo = std::move(folded);
      if (lo->size() > 1) {
        std::optional<unsigned> dominant;
        for (unsigned r = 0; r < lo->size() && !dominant; ++r) {
          bool always = true;
          for (unsigned j = 0; j < lo->size() && always; ++j) {
            if (j == r)
              continue;
            presburger::IntegerPolyhedron wins = outer;
            SmallVector<BigInt> row = zeroRow(wins.getNumCols());
            for (unsigned t = 0; t < s; ++t) // lb_j(x) - lb_r(x) - 1 >= 0
              row[t] = BigInt((*lo)[j].coefficients[t]) -
                       BigInt((*lo)[r].coefficients[t]);
            row.back() = BigInt((*lo)[j].constant) -
                         BigInt((*lo)[r].constant) - BigInt(1);
            wins.addInequality(row);
            always = wins.isIntegerEmpty();
          }
          if (always)
            dominant = r;
        }
        if (!dominant)
          return std::nullopt;
        base = *dominant;
      }
    }
    out.lowers.push_back(std::move(*lo));
    out.uppers.push_back(std::move(*hi));
    out.steps.push_back(step);
    out.strideBases.push_back(base);
  }
  out.domain =
      exactBandDomain(out.lowers, out.uppers, out.steps, out.strideBases);
  return out;
}

/// The constant-extent parallelotope a band's exact iteration set equals,
/// if any: for every loop s a shift row l_s (zero, or the linear part of one
/// of the loop's written bound results) and constants a_s <= e_s with
///   D = { x : a_s <= x_s - l_s . x_<s <= e_s,
///             x_s - l_s . x_<s - a_s = 0 (mod step_s) }   (exactly).
/// Found loop by loop, outermost first: the projection of D onto the first
/// s + 1 counters must equal the projection of the parallelotope -- a_s and
/// e_s are the exact integer minimum and maximum of x_s - l_s . x_<s on D,
/// and the equality is the Presburger library's integer-exact test. A choice
/// that passes fixes nothing beyond the projection, which is D's own, so the
/// greedy search is complete over these candidates. Returned as the bounds
/// of the family-F form: lower l_s . x + a_s, exclusive upper l_s . x + e_s +
/// 1. nullopt when exactBandSet has no set for the band (a bound that is not
/// a linear form of the band's outer counters, a stepped loop with a max of
/// lower bounds of which none is the max at every outer point entering it, a
/// written bound that can leave 64 bits there), D is empty, or no candidate
/// fits.
struct ExactParallelotope {
  SmallVector<BandBound> lowers, uppers;
};

std::optional<ExactParallelotope>
findExactParallelotope(ArrayRef<affine::AffineForOp> band) {
  const unsigned n = band.size();
  std::optional<ExactBandSet> exactSet = exactBandSet(band);
  if (!exactSet || exactSet->domain.isIntegerEmpty())
    return std::nullopt;
  const presburger::IntegerPolyhedron &domain = exactSet->domain;
  ArrayRef<SmallVector<BandForm>> lowerResults = exactSet->lowers;
  ArrayRef<SmallVector<BandForm>> upperResults = exactSet->uppers;
  ArrayRef<int64_t> steps = exactSet->steps;

  ExactParallelotope found;
  presburger::IntegerPolyhedron prefix(
      presburger::PresburgerSpace::getSetSpace(0));
  for (unsigned s = 0; s < n; ++s) {
    presburger::IntegerPolyhedron target = prefixOf(domain, s + 1);
    SmallVector<SmallVector<int64_t>> candidates;
    auto addCandidate = [&](ArrayRef<int64_t> row) {
      SmallVector<int64_t> l(n, 0);
      for (unsigned t = 0; t < s; ++t)
        l[t] = row[t];
      if (!llvm::is_contained(candidates, l))
        candidates.push_back(std::move(l));
    };
    addCandidate(SmallVector<int64_t>(n, 0));
    for (const BandForm &form : lowerResults[s])
      addCandidate(form.coefficients);
    for (const BandForm &form : upperResults[s])
      addCandidate(form.coefficients);
    bool accepted = false;
    for (const SmallVector<int64_t> &l : candidates) {
      // e(x) = x_s - l . x_<s over the first s + 1 counters, and -e(x).
      SmallVector<int64_t> value(s + 1, 0), negated(s + 1, 0);
      value[s] = 1;
      negated[s] = -1;
      bool fits = true;
      for (unsigned t = 0; t < s && fits; ++t) {
        std::optional<int64_t> minus = llvm::checkedSub(int64_t(0), l[t]);
        if (!minus)
          fits = false;
        else {
          value[t] = *minus;
          negated[t] = l[t];
        }
      }
      if (!fits)
        continue;
      std::optional<BigInt> lowest = integerMinimum(target, value, 0);
      std::optional<BigInt> negatedHighest = integerMinimum(target, negated, 0);
      if (!lowest || !negatedHighest)
        continue;
      std::optional<int64_t> a = toInt64(*lowest);
      std::optional<int64_t> e = toInt64(-*negatedHighest);
      std::optional<int64_t> past = e ? llvm::checkedAdd(*e, int64_t(1)) : std::nullopt;
      if (!a || !e || !past)
        continue;
      // The parallelotope's slice for loop s on the accepted prefix.
      presburger::IntegerPolyhedron candidate = prefix;
      candidate.insertVar(presburger::VarKind::SetDim, s);
      SmallVector<BigInt> lowerRow = zeroRow(candidate.getNumCols());
      SmallVector<BigInt> upperRow = zeroRow(candidate.getNumCols());
      lowerRow[s] = BigInt(1); // x_s - l . x - a >= 0
      upperRow[s] = BigInt(-1); // e - x_s + l . x >= 0
      for (unsigned t = 0; t < s; ++t) {
        lowerRow[t] = -BigInt(l[t]);
        upperRow[t] = BigInt(l[t]);
      }
      lowerRow.back() = -BigInt(*a);
      upperRow.back() = BigInt(*e);
      candidate.addInequality(lowerRow);
      candidate.addInequality(upperRow);
      if (steps[s] > 1) {
        const unsigned q = candidate.appendVar(presburger::VarKind::Local);
        SmallVector<BigInt> congruence = zeroRow(candidate.getNumCols());
        congruence[s] = BigInt(1); // x_s - l . x - a - step q = 0
        for (unsigned t = 0; t < s; ++t)
          congruence[t] = -BigInt(l[t]);
        congruence[q] = -BigInt(steps[s]);
        congruence.back() = -BigInt(*a);
        candidate.addEquality(congruence);
      }
      if (!candidate.isEqual(target))
        continue;
      prefix = std::move(candidate);
      BandBound lb, ub;
      lb.coefficients = l;
      lb.constant = *a;
      ub.coefficients = l;
      ub.constant = *past;
      found.lowers.push_back(std::move(lb));
      found.uppers.push_back(std::move(ub));
      accepted = true;
      break;
    }
    if (!accepted)
      return std::nullopt;
  }
  return found;
}

/// An access rewritten over the band's counters x = U x': the composed map
/// and its operands -- only the counters the composed map reads, so that
/// operand membership keeps meaning "the access depends on this counter" for
/// the hoist of repetition loops, the candidate collection and the
/// recognizer. Every operand of the original access must be a band counter
/// and every result a linear form; the composed coefficients are formed in
/// checked 64-bit arithmetic before any AffineExpr is built (MLIR's own
/// flattener wraps). lower-affine emits each coefficient times a counter as
/// `arith.muli ... overflow<nsw>` (poison when it does not fit) and the sums
/// as plain `arith.addi` (they wrap, and their total is the original
/// subscript's value): so every product must fit over the box `box` (per
/// band position, the normalized counter's [lower, upper)), or the rewrite
/// would add an overflow the written access does not have. nullopt when any
/// of that fails.
struct ShiftedAccess {
  AffineMap map;
  SmallVector<Value> operands;
};

std::optional<ShiftedAccess>
composeWithShift(AffineMap map, ValueRange operands,
                 ArrayRef<affine::AffineForOp> band,
                 const SmallVector<SmallVector<int64_t>> &U,
                 ArrayRef<std::pair<int64_t, int64_t>> box) {
  if (map.getNumSymbols() != 0)
    return std::nullopt;
  const unsigned n = band.size();
  SmallVector<unsigned> position;
  for (Value operand : operands) {
    std::optional<unsigned> at = bandPosition(operand, band);
    if (!at)
      return std::nullopt;
    position.push_back(*at);
  }
  SmallVector<SmallVector<int64_t>> coefficients; // per result, over x'
  SmallVector<int64_t> constants;
  for (AffineExpr expr : map.getResults()) {
    FailureOr<LinearForm> form = extractLinearForm(expr, map.getNumDims());
    if (failed(form))
      return std::nullopt;
    SmallVector<int64_t> row(n, 0);
    for (unsigned j = 0; j < form->coefficients.size(); ++j) {
      const int64_t c = form->coefficients[j];
      if (c == 0)
        continue;
      for (unsigned u = 0; u < n; ++u) {
        std::optional<int64_t> term = llvm::checkedMul(c, U[position[j]][u]);
        std::optional<int64_t> sum =
            term ? llvm::checkedAdd(row[u], *term) : std::nullopt;
        if (!sum)
          return std::nullopt;
        row[u] = *sum;
      }
    }
    // A box with no point runs no access: nothing is evaluated.
    const bool empty = llvm::any_of(box, [](std::pair<int64_t, int64_t> r) {
      return r.second <= r.first;
    });
    for (unsigned u = 0; u < n && !empty; ++u) {
      if (row[u] == 0)
        continue;
      std::optional<int64_t> last = llvm::checkedSub(box[u].second, int64_t(1));
      if (!last || !llvm::checkedMul(row[u], box[u].first) ||
          !llvm::checkedMul(row[u], *last))
        return std::nullopt;
    }
    coefficients.push_back(std::move(row));
    constants.push_back(form->constant);
  }
  // The counters the composed access reads, in band order.
  SmallVector<int> dimOf(n, -1);
  ShiftedAccess out;
  for (unsigned u = 0; u < n; ++u)
    if (llvm::any_of(coefficients, [&](const SmallVector<int64_t> &row) {
          return row[u] != 0;
        })) {
      dimOf[u] = out.operands.size();
      affine::AffineForOp loop = band[u];
      out.operands.push_back(loop.getInductionVar());
    }
  MLIRContext *ctx = map.getContext();
  SmallVector<AffineExpr> results;
  for (auto [row, constant] : llvm::zip_equal(coefficients, constants)) {
    AffineExpr result = getAffineConstantExpr(constant, ctx);
    for (unsigned u = 0; u < n; ++u)
      if (row[u] != 0)
        result = result + getAffineDimExpr(dimOf[u], ctx) * row[u];
    results.push_back(result);
  }
  out.map = AffineMap::get(out.operands.size(), 0, results, ctx);
  return out;
}

} // namespace

std::optional<presburger::IntegerPolyhedron>
mlir::systolic::exactIterationSet(ArrayRef<affine::AffineForOp> loops) {
  std::optional<ExactBandSet> set = exactBandSet(loops);
  if (!set)
    return std::nullopt;
  return std::move(set->domain);
}

std::optional<int64_t>
mlir::systolic::constantAffineForBound(affine::AffineForOp loop, bool lower) {
  return constantLoopBound(loop, lower);
}

std::optional<unsigned>
mlir::systolic::stackedAccessRank(const LogicalMatmul &matmul) {
  const AffineMap maps[] = {matmul.accessA, matmul.accessB, matmul.accessC};
  if (!maps[0] || !maps[1] || !maps[2])
    return std::nullopt;
  const unsigned n = maps[2].getNumDims();
  lattice::IntMatrix rows;
  for (AffineMap map : maps) {
    if (map.getNumDims() != n || map.getNumSymbols() != 0)
      return std::nullopt;
    for (AffineExpr result : map.getResults()) {
      FailureOr<LinearForm> form = extractLinearForm(result, n);
      if (failed(form))
        return std::nullopt;
      rows.emplace_back(form->coefficients.begin(), form->coefficients.end());
    }
  }
  std::optional<lattice::ColumnEchelon> echelon =
      lattice::columnEchelon(rows, n);
  if (!echelon)
    return std::nullopt;
  return echelon->rank;
}

void mlir::systolic::describeDomainNormalization(
    const DomainNormalization &normalization, llvm::raw_ostream &os,
    StringRef indent) {
  const unsigned n = normalization.U.size();
  if (normalization.exact)
    os << indent
       << "exact iteration set: the written bounds do not shift together, "
          "their integer points are the parallelotope below (integer-set "
          "equality, docs section 19.13)\n";
  for (unsigned s = 0; s < n; ++s) {
    if (!normalization.exact && !llvm::is_contained(normalization.shifted, s))
      continue;
    // The shift as the bounds state it: x_s = x_s' + l_s . x_<s (U's rows
    // compose these shifts).
    os << indent << "loop " << s << ": x" << s << " = x" << s << "'";
    for (unsigned u = 0; u < s; ++u)
      if (normalization.shift[s][u] != 0)
        os << (normalization.shift[s][u] < 0 ? " - " : " + ")
           << std::abs(normalization.shift[s][u]) << "*x" << u;
    os << ", box [" << normalization.lower[s] << ", " << normalization.upper[s]
       << ") step " << normalization.step[s] << "\n";
  }
}

SmallVector<DomainNormalization, 1>
mlir::systolic::normalizeParallelotopeDomains(Operation *root) {
  SmallVector<DomainNormalization, 1> applied;
  SmallVector<affine::AffineForOp> roots;
  root->walk([&](affine::AffineForOp loop) {
    auto parent = dyn_cast<affine::AffineForOp>(loop->getParentOp());
    if (parent && &parent.getBody()->front() == loop.getOperation() &&
        std::next(parent.getBody()->begin(), 2) == parent.getBody()->end())
      return;
    roots.push_back(loop);
  });

  // One band: the constant-extent family F (both bounds of every loop one
  // affine form of the outer counters, docs section 19.12) directly, else
  // the exact path (section 19.13): the band's integer iteration set, if it
  // equals a constant-extent parallelotope. True when the band was
  // rewritten.
  auto normalizeBand = [&](ArrayRef<affine::AffineForOp> band) -> bool {
    const unsigned n = band.size();
    if (n < 2)
      return false;
    affine::AffineForOp top = band.front();
    // Exactly the recognizer's multiply-accumulate body: three loads, one
    // product of two of them, one sum of the third with the product, one
    // store of the sum to the third load's element (and the yield). Any
    // other body -- a nest the recognizer would refuse anyway -- is left as
    // written.
    SmallVector<Operation *> accesses;
    unsigned loads = 0, stores = 0, muls = 0, adds = 0, others = 0;
    affine::AffineStoreOp store;
    affine::AffineForOp innermost = band.back();
    for (Operation &op : *innermost.getBody()) {
      if (isa<affine::AffineLoadOp>(op)) {
        ++loads;
        accesses.push_back(&op);
      } else if (auto st = dyn_cast<affine::AffineStoreOp>(op)) {
        ++stores;
        store = st;
        accesses.push_back(&op);
      } else if (isa<arith::MulFOp>(op)) {
        ++muls;
      } else if (isa<arith::AddFOp>(op)) {
        ++adds;
      } else if (!isa<affine::AffineYieldOp>(op)) {
        ++others;
      }
    }
    // C = C + A * B (three loads, an addf) or the assignment C = A * B (two
    // loads, no addf; docs section 20).
    bool macBody = stores == 1 && muls == 1 && others == 0 &&
                   ((loads == 3 && adds == 1) || (loads == 2 && adds == 0));
    if (macBody && loads == 3) {
      auto add = store.getValueToStore().getDefiningOp<arith::AddFOp>();
      affine::AffineLoadOp acc;
      arith::MulFOp mul;
      if (add) {
        for (Value v : {add.getLhs(), add.getRhs()}) {
          if (auto l = v.getDefiningOp<affine::AffineLoadOp>())
            acc = l;
          else if (auto m = v.getDefiningOp<arith::MulFOp>())
            mul = m;
        }
      }
      // The accumulator is loaded from the memref the sum is stored to, with
      // as many indices; whether the two address the same element at every
      // point is the recognizer's question (the maps may differ by terms of
      // loops that run once, constants on the domain -- the oracle of
      // 2026-10-18 found such nests left sheared and refused in silence when
      // this test demanded equal maps).
      macBody = add && acc && mul &&
                mul.getLhs().getDefiningOp<affine::AffineLoadOp>() &&
                mul.getRhs().getDefiningOp<affine::AffineLoadOp>() &&
                acc.getMemRef() == store.getMemRef() &&
                acc.getAffineMap().getNumResults() ==
                    store.getAffineMap().getNumResults();
    } else if (macBody) {
      auto mul = store.getValueToStore().getDefiningOp<arith::MulFOp>();
      macBody = mul && mul.getLhs().getDefiningOp<affine::AffineLoadOp>() &&
                mul.getRhs().getDefiningOp<affine::AffineLoadOp>() &&
                mul.getLhs() != mul.getRhs();
    }
    if (!macBody || !llvm::all_of(band, [](affine::AffineForOp loop) {
          return loop.getNumResults() == 0 && loop.getStepAsInt() >= 1;
        }))
      return false;

    // The bounds: the same linear form of the outer counters at both ends.
    SmallVector<BandBound> lowers, uppers;
    bool inFamily = true, anyShift = false, exact = false;
    for (unsigned s = 0; s < n && inFamily; ++s) {
      std::optional<BandBound> lo = bandBound(band[s], /*lower=*/true, band, s);
      std::optional<BandBound> hi = bandBound(band[s], /*lower=*/false, band, s);
      if (!lo || !hi || lo->coefficients != hi->coefficients) {
        inFamily = false;
        break;
      }
      if (llvm::any_of(lo->coefficients, [](int64_t c) { return c != 0; }))
        anyShift = true;
      lowers.push_back(*lo);
      uppers.push_back(*hi);
    }
    if (inFamily && !anyShift)
      return false; // a box already
    // The shifted set is the one the program runs only when its written
    // bounds fit in 64 bits at every outer point (exactBandSet; the exact
    // path below checks the same inside findExactParallelotope).
    if (inFamily && !exactBandSet(band))
      return false;
    if (!inFamily) {
      // Not in F as written: the exact integer set, if a parallelotope.
      std::optional<ExactParallelotope> parallelotope =
          findExactParallelotope(band);
      if (!parallelotope)
        return false;
      lowers = std::move(parallelotope->lowers);
      uppers = std::move(parallelotope->uppers);
      exact = true;
    }

    // x = U x': U[s][u] = delta + sum_{u <= t < s} l_s[t] U[t][u].
    SmallVector<SmallVector<int64_t>> U(n, SmallVector<int64_t>(n, 0));
    bool fits = true;
    for (unsigned s = 0; s < n && fits; ++s) {
      U[s][s] = 1;
      for (unsigned u = 0; u < s && fits; ++u) {
        int64_t value = 0;
        for (unsigned t = u; t < s && fits; ++t) {
          std::optional<int64_t> term =
              llvm::checkedMul(lowers[s].coefficients[t], U[t][u]);
          std::optional<int64_t> sum =
              term ? llvm::checkedAdd(value, *term) : std::nullopt;
          if (!sum)
            fits = false;
          else
            value = *sum;
        }
        U[s][u] = value;
      }
    }
    if (!fits)
      return false;

    // Every use of a counter is a bound of an inner band loop or an access of
    // the body; every access rewrites (checked) before anything changes.
    bool usesOk = true;
    for (affine::AffineForOp loop : band)
      for (OpOperand &use : loop.getInductionVar().getUses()) {
        Operation *user = use.getOwner();
        if (auto inner = dyn_cast<affine::AffineForOp>(user)) {
          if (!llvm::is_contained(band, inner))
            usesOk = false;
          continue;
        }
        if (!isa<affine::AffineLoadOp, affine::AffineStoreOp>(user) ||
            user->getParentOp() != innermost.getOperation())
          usesOk = false;
      }
    if (!usesOk)
      return false;
    SmallVector<std::pair<int64_t, int64_t>> box; // x'_s in [lower, upper)
    for (unsigned s = 0; s < n; ++s)
      box.push_back({lowers[s].constant, uppers[s].constant});
    SmallVector<ShiftedAccess> rewritten;
    for (Operation *op : accesses) {
      AffineMap map;
      ValueRange operands;
      if (auto load = dyn_cast<affine::AffineLoadOp>(op)) {
        map = load.getAffineMap();
        operands = load.getMapOperands();
      } else {
        auto st = cast<affine::AffineStoreOp>(op);
        map = st.getAffineMap();
        operands = st.getMapOperands();
      }
      std::optional<ShiftedAccess> composed =
          composeWithShift(map, operands, band, U, box);
      if (!composed)
        return false;
      rewritten.push_back(std::move(*composed));
    }

    // Rewrite: constant bounds, composed access maps over the band counters.
    // On the exact path every loop's bounds are replaced -- the written ones
    // describe the same set only all together.
    DomainNormalization record;
    record.outermost = top.getOperation();
    record.U = U;
    record.exact = exact;
    for (unsigned s = 0; s < n; ++s) {
      affine::AffineForOp loop = band[s];
      record.shift.push_back(lowers[s].coefficients);
      record.lower.push_back(lowers[s].constant);
      record.upper.push_back(uppers[s].constant);
      record.step.push_back(loop.getStepAsInt());
      const bool shifted =
          llvm::any_of(lowers[s].coefficients, [](int64_t c) { return c != 0; });
      if (shifted)
        record.shifted.push_back(s);
      if (shifted || exact) {
        loop.setConstantLowerBound(lowers[s].constant);
        loop.setConstantUpperBound(uppers[s].constant);
      }
    }
    for (auto [op, access] : llvm::zip_equal(accesses, rewritten)) {
      OpBuilder builder(op);
      if (auto load = dyn_cast<affine::AffineLoadOp>(op)) {
        auto replacement = affine::AffineLoadOp::create(
            builder, load.getLoc(), load.getMemRef(), access.map,
            access.operands);
        load.getResult().replaceAllUsesWith(replacement.getResult());
        load.erase();
      } else {
        auto st = cast<affine::AffineStoreOp>(op);
        affine::AffineStoreOp::create(builder, st.getLoc(),
                                      st.getValueToStore(), st.getMemRef(),
                                      access.map, access.operands);
        st.erase();
      }
    }
    applied.push_back(std::move(record));
    return true;
  };

  for (affine::AffineForOp root : roots) {
    SmallVector<affine::AffineForOp> band;
    affine::getPerfectlyNestedLoops(band, root);
    // The whole band, or -- when it is not normalizable -- the band below a
    // prefix of loops whose counters nothing below them reads (repetition
    // loops in any shape, a triangle of them included: they only run the
    // inner band again, docs section 19.13).
    for (unsigned start = 0; start + 1 < band.size(); ++start) {
      if (start > 0) {
        affine::AffineForOp below = band[start];
        bool independent = true;
        for (unsigned t = 0; t < start && independent; ++t)
          for (Operation *user : band[t].getInductionVar().getUsers())
            if (below->isAncestor(user))
              independent = false;
        if (!independent)
          continue;
      }
      if (normalizeBand(ArrayRef<affine::AffineForOp>(band).drop_front(start)))
        break;
    }
  }
  return applied;
}
