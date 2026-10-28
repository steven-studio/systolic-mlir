#pragma once

#include "Systolic/RankPartitionIterationPlan.h"

#include "llvm/ADT/SmallVector.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <utility>
#include <vector>

namespace systolic::logical {

// Phase 5.4.3d-1a
//
// Build an exact, machine-arithmetic-safe representation
// of a Presburger membership predicate.
//
// A PresburgerSet is a disjunction of conjunctions:
//
//   G(z) = OR_t (AND_r constraint[t][r](z))
//
// Every row is:
//
//   c + sum_i a[i]*z[i] >= 0
//
// or:
//
//   c + sum_i a[i]*z[i] == 0
//
// The symbolic plan has already proved that its logical
// domain fits inside the supplied finite bounding box.
//
// This module proves that evaluating each constraint
// with signed i64 arithmetic is safe over that box.
//
// Unsupported existential locals are rejected.
//
// No source-point enumeration.
// No arbitrary-precision arithmetic at runtime.
// No IR modification.

enum class GuardConstraintKind {
  Equality,
  Inequality,
  Congruence, // linear expression is divisible by modulus
  ExistentialBounds // sum of two floor quotients >= 0
};

struct GuardConstraint {
  GuardConstraintKind kind;
  std::vector<std::int64_t> coefficients;
  std::int64_t constant = 0;
  std::int64_t modulus = 0; // positive for Congruence only

  // A recognized Presburger floor local: q = floor((c + a*z)/d), d>0.
  // floorCoefficient*q is added AFTER the ordinary affine terms.
  std::vector<std::int64_t> floorCoefficients;
  std::int64_t floorConstant = 0;
  std::int64_t floorDivisor = 0;
  std::int64_t floorCoefficient = 0;

  // 1d-c: second floor term for existential interval feasibility.
  std::vector<std::int64_t> secondFloorCoefficients;
  std::int64_t secondFloorConstant = 0;
  std::int64_t secondFloorDivisor = 0;
};

struct GuardConjunction {
  std::vector<GuardConstraint> constraints;
};

struct ExactGuardPlan {
  std::size_t dimension = 0;

  // Empty means the constant-false predicate.
  std::vector<GuardConjunction> disjuncts;
};

namespace guard_detail {

inline bool fitsI64(const BigInt &value) {
  static const BigInt minValue(
      std::numeric_limits<std::int64_t>::min());

  static const BigInt maxValue(
      std::numeric_limits<std::int64_t>::max());

  return value >= minValue && value <= maxValue;
}

inline std::optional<std::int64_t>
toI64(const llvm::DynamicAPInt &value) {

  const llvm::DynamicAPInt minimum(
      std::numeric_limits<std::int64_t>::min());

  const llvm::DynamicAPInt maximum(
      std::numeric_limits<std::int64_t>::max());

  if (value < minimum || value > maximum)
    return std::nullopt;

  return static_cast<std::int64_t>(value);
}

// Prove safe arithmetic for this precise evaluation order:
//
//   accumulator = constant
//
//   for i = 0..n-1:
//      term = coefficient[i] * z[i]
//      accumulator += term
//
// Every product and every partial sum must fit i64.
//
// All proofs are performed with arbitrary precision.

inline bool safeLinearEvaluation(
    const GuardConstraint &constraint,
    const std::vector<ClosedIntegerInterval> &bounds) {

  if (constraint.coefficients.size() != bounds.size())
    return false;

  BigInt lower = constraint.constant;
  BigInt upper = constraint.constant;

  for (std::size_t i = 0; i < bounds.size(); ++i) {

    const BigInt coefficient(
        constraint.coefficients[i]);

    const BigInt a = coefficient * bounds[i].lower;
    const BigInt b = coefficient * bounds[i].upper;

    const BigInt termLower = a < b ? a : b;
    const BigInt termUpper = a > b ? a : b;

    // The runtime multiplication must not overflow.
    if (!fitsI64(termLower) || !fitsI64(termUpper))
      return false;

    lower += termLower;
    upper += termUpper;

    // Every runtime partial sum must fit i64.
    if (!fitsI64(lower) || !fitsI64(upper))
      return false;
  }

  return true;
}

inline std::optional<GuardConstraint>
convertConstraint(
    llvm::ArrayRef<llvm::DynamicAPInt> row,
    GuardConstraintKind kind,
    const std::vector<ClosedIntegerInterval> &bounds) {

  const std::size_t n = bounds.size();

  if (row.size() != n + 1)
    return std::nullopt;

  GuardConstraint result;
  result.kind = kind;

  for (std::size_t i = 0; i < n; ++i) {
    auto coefficient = toI64(row[i]);

    if (!coefficient)
      return std::nullopt;

    result.coefficients.push_back(*coefficient);
  }

  auto constant = toI64(row.back());

  if (!constant)
    return std::nullopt;

  result.constant = *constant;

  if (!safeLinearEvaluation(result, bounds))
    return std::nullopt;

  return result;
}


// 1d-b: only a single, explicitly defined floor local is supported.
// The defining inequalities must be an exact opposite pair:
//    N(z) - d*q >= 0
//   -N(z) + d*q + (d-1) >= 0
// where d>0. This uniquely fixes q = floor(N/d).
// All other local-bearing rows are substituted with this q.
// Unknown or ambiguous forms fail closed; no iteration enumeration.
inline std::optional<GuardConjunction> convertFloorLocal(
    const mlir::presburger::IntegerRelation &poly,
    const std::vector<ClosedIntegerInterval> &bounds) {
  const std::size_t n = bounds.size();
  if (poly.getNumLocalVars() != 1 ||
      poly.getNumEqualities() != 0)
    return std::nullopt;

  struct Row {
    std::vector<std::int64_t> entries;
  };
  std::vector<Row> rows;
  for (unsigned i = 0; i < poly.getNumInequalities(); ++i) {
    auto original = poly.getInequality(i);
    if (original.size() != n + 2)
      return std::nullopt;
    Row row;
    for (const auto &item : original) {
      auto converted = toI64(item);
      if (!converted)
        return std::nullopt;
      row.entries.push_back(*converted);
    }
    rows.push_back(std::move(row));
  }

  // Intentionally require one unambiguous defining pair.
  std::optional<std::pair<std::size_t, std::size_t>> definition;
  for (std::size_t a = 0; a < rows.size(); ++a) {
    const auto &left = rows[a].entries;
    if (left[n] >= 0 ||
        left[n] == std::numeric_limits<std::int64_t>::min())
      continue;
    const std::int64_t divisor = -left[n];
    for (std::size_t b = 0; b < rows.size(); ++b) {
      if (a == b)
        continue;
      const auto &right = rows[b].entries;
      if (right[n] != divisor)
        continue;
      bool opposite = true;
      for (std::size_t k = 0; k < n; ++k) {
        if (static_cast<__int128>(left[k]) + right[k] != 0) {
          opposite = false;
          break;
        }
      }
      if (!opposite ||
          static_cast<__int128>(left.back()) +
                  right.back() != divisor - 1)
        continue;
      if (definition)
        return std::nullopt;
      definition = std::make_pair(a, b);
    }
  }
  if (!definition)
    return std::nullopt;
  const auto &numerator = rows[definition->first].entries;
  const std::int64_t divisor = -numerator[n];

  llvm::SmallVector<llvm::DynamicAPInt> numeratorRow;
  for (std::size_t k = 0; k < n; ++k)
    numeratorRow.push_back(llvm::DynamicAPInt(numerator[k]));
  numeratorRow.push_back(llvm::DynamicAPInt(numerator.back()));
  auto checkedNumerator = convertConstraint(
      numeratorRow, GuardConstraintKind::Inequality, bounds);
  if (!checkedNumerator)
    return std::nullopt;

  // Bounds on N and floor(N/d), computed in widened arithmetic.
  __int128 low = numerator.back();
  __int128 high = numerator.back();
  for (std::size_t k = 0; k < n; ++k) {
    const __int128 x = static_cast<__int128>(numerator[k]) *
                       bounds[k].lower;
    const __int128 y = static_cast<__int128>(numerator[k]) *
                       bounds[k].upper;
    low += x < y ? x : y;
    high += x > y ? x : y;
  }
  // convertConstraint already proved each intermediate affine
  // operation and hence these extrema fit in i64.
  const auto floorDivide = [divisor](__int128 value) -> __int128 {
    __int128 quotient = value / divisor;
    if (value < 0 && value % divisor != 0)
      --quotient;
    return quotient;
  };
  const __int128 qLow = floorDivide(low);
  const __int128 qHigh = floorDivide(high);
  const __int128 minI64 = std::numeric_limits<std::int64_t>::min();
  const __int128 maxI64 = std::numeric_limits<std::int64_t>::max();
  if (qLow < minI64 || qHigh > maxI64)
    return std::nullopt;

  GuardConjunction result;
  for (std::size_t r = 0; r < rows.size(); ++r) {
    if (r == definition->first || r == definition->second)
      continue;
    const auto &row = rows[r].entries;
    llvm::SmallVector<llvm::DynamicAPInt> projected;
    for (std::size_t k = 0; k < n; ++k)
      projected.push_back(llvm::DynamicAPInt(row[k]));
    projected.push_back(llvm::DynamicAPInt(row.back()));
    auto constraint = convertConstraint(
        projected, GuardConstraintKind::Inequality, bounds);
    if (!constraint)
      return std::nullopt;

    if (row[n] != 0) {
      // Check floorCoefficient * q and the final addition.
      const __int128 first = static_cast<__int128>(row[n]) * qLow;
      const __int128 second = static_cast<__int128>(row[n]) * qHigh;
      const __int128 termLow = first < second ? first : second;
      const __int128 termHigh = first > second ? first : second;
      if (termLow < minI64 || termHigh > maxI64)
        return std::nullopt;
      __int128 affineLow = row.back();
      __int128 affineHigh = row.back();
      for (std::size_t k = 0; k < n; ++k) {
        const __int128 x = static_cast<__int128>(row[k]) *
                           bounds[k].lower;
        const __int128 y = static_cast<__int128>(row[k]) *
                           bounds[k].upper;
        affineLow += x < y ? x : y;
        affineHigh += x > y ? x : y;
      }
      if (affineLow + termLow < minI64 ||
          affineHigh + termHigh > maxI64)
        return std::nullopt;
      constraint->floorCoefficients.assign(numerator.begin(),
                                           numerator.begin() + n);
      constraint->floorConstant = numerator.back();
      constraint->floorDivisor = divisor;
      constraint->floorCoefficient = row[n];
    }
    result.constraints.push_back(std::move(*constraint));
  }
  return result;
}


// 1d-c bounded subphase: ONE existential local, inequality-only.
// A(z)+b*q >= 0 (b>0) means q >= -floor(A(z)/b).
// B(z)-a*q >= 0 (a>0) means q <= floor(B(z)/a).
// Existence is equivalent to EVERY lower/upper pair satisfying
// floor(A(z)/b) + floor(B(z)/a) >= 0.
// Reject one-sided/ambiguous cases here rather than broadening old tests.
inline std::optional<GuardConjunction> convertTwoSidedLocal(
    const mlir::presburger::IntegerRelation &poly,
    const std::vector<ClosedIntegerInterval> &bounds) {
  const std::size_t n = bounds.size();
  if (poly.getNumLocalVars() != 1 || poly.getNumEqualities() != 0)
    return std::nullopt;

  struct Bound {
    GuardConstraint expression;
    std::int64_t divisor;
    BigInt low;
    BigInt high;
  };
  std::vector<Bound> lowers, uppers;
  GuardConjunction result;

  auto signedFloor = [](const BigInt &numerator,
                        const BigInt &denominator) -> BigInt {
    BigInt q = numerator / denominator;
    if (numerator < 0 && numerator % denominator != 0)
      --q;
    return q;
  };

  for (unsigned r = 0; r < poly.getNumInequalities(); ++r) {
    const auto row = poly.getInequality(r);
    if (row.size() != n + 2)
      return std::nullopt;
    auto local = toI64(row[n]);
    if (!local || *local == std::numeric_limits<std::int64_t>::min())
      return std::nullopt;
    llvm::SmallVector<llvm::DynamicAPInt> projected;
    for (std::size_t k = 0; k < n; ++k)
      projected.push_back(row[k]);
    projected.push_back(row.back());
    auto expression = convertConstraint(
        projected, GuardConstraintKind::Inequality, bounds);
    if (!expression)
      return std::nullopt;
    if (*local == 0) {
      result.constraints.push_back(std::move(*expression));
      continue;
    }
    const std::int64_t divisor = *local > 0 ? *local : -*local;
    BigInt lo = expression->constant;
    BigInt hi = expression->constant;
    for (std::size_t k = 0; k < n; ++k) {
      BigInt x = BigInt(expression->coefficients[k]) * bounds[k].lower;
      BigInt y = BigInt(expression->coefficients[k]) * bounds[k].upper;
      lo += x < y ? x : y;
      hi += x > y ? x : y;
    }
    Bound bound{std::move(*expression), divisor,
                signedFloor(lo, BigInt(divisor)),
                signedFloor(hi, BigInt(divisor))};
    if (!fitsI64(bound.low) || !fitsI64(bound.high))
      return std::nullopt;
    if (*local > 0)
      lowers.push_back(std::move(bound));
    else
      uppers.push_back(std::move(bound));
  }
  if (lowers.empty() || uppers.empty())
    return std::nullopt;
  if (lowers.size() > 64 || uppers.size() > 64 ||
      lowers.size() * uppers.size() > 256)
    return std::nullopt;

  for (const Bound &lower : lowers) {
    for (const Bound &upper : uppers) {
      if (!fitsI64(lower.low + upper.low) ||
          !fitsI64(lower.high + upper.high))
        return std::nullopt;
      GuardConstraint test;
      test.kind = GuardConstraintKind::ExistentialBounds;
      test.coefficients.assign(n, 0);
      test.floorCoefficients = lower.expression.coefficients;
      test.floorConstant = lower.expression.constant;
      test.floorDivisor = lower.divisor;
      test.floorCoefficient = 1;
      test.secondFloorCoefficients = upper.expression.coefficients;
      test.secondFloorConstant = upper.expression.constant;
      test.secondFloorDivisor = upper.divisor;
      result.constraints.push_back(std::move(test));
    }
  }
  return result;
}

} // namespace guard_detail

inline std::optional<ExactGuardPlan>
buildExactGuardPlan(
    const SymbolicIterationPlan &plan) {

  const std::size_t n = plan.bounds.size();

  if (n == 0 ||
      n > std::numeric_limits<unsigned>::max() - 1)
    return std::nullopt;

  if (plan.logicalDomain.getNumDomainVars() != 0 ||
      plan.logicalDomain.getNumRangeVars() != n ||
      plan.logicalDomain.getNumSymbolVars() != 0)
    return std::nullopt;

  for (const auto &bound : plan.bounds) {
    if (bound.lower > bound.upper)
      return std::nullopt;
  }

  // Revalidate the coverage invariant: callers may
  // construct SymbolicIterationPlan directly.
  //
  // Do not trust an unchecked plan object.

  auto verified = buildSymbolicIterationPlan(
      plan.logicalDomain, plan.bounds);

  if (!verified)
    return std::nullopt;

  ExactGuardPlan result;
  result.dimension = n;

  for (const auto &disjunct :
       plan.logicalDomain.getAllDisjuncts()) {

    if (disjunct.getNumLocalVars() > 1 ||
        disjunct.getNumDimVars() != n ||
        disjunct.getNumSymbolVars() != 0)
      return std::nullopt;

    // Prefer exact floor-local recognition if a local occurs in
    // any inequality; the congruence-only path is unchanged.
    if (disjunct.getNumLocalVars() == 1) {
      bool localInInequality = false;
      for (unsigned r = 0; r < disjunct.getNumInequalities(); ++r)
        localInInequality |= disjunct.getInequality(r)[n] != 0;
      if (localInInequality) {
        auto floor = guard_detail::convertFloorLocal(disjunct, plan.bounds);
        if (!floor)
          floor = guard_detail::convertTwoSidedLocal(disjunct, plan.bounds);
        if (!floor)
          return std::nullopt;
        result.disjuncts.push_back(std::move(*floor));
        continue;
      }
    }

    GuardConjunction conjunction;

    // Phase 5.4.3d-1d-a: The only supported existential form is
    //   exists q: E(z) + b*q == 0
    // with exactly one equality mentioning q, and no other row using q.
    // This is equivalent to E(z) % abs(b) == 0, for nonzero b.
    // An unconstrained local is harmless. Other local patterns fail closed.
    const bool hasLocal = disjunct.getNumLocalVars() == 1;
    bool consumedLocal = false;

    for (unsigned r = 0;
         r < disjunct.getNumEqualities(); ++r) {
      auto row = disjunct.getEquality(r);
      GuardConstraintKind kind = GuardConstraintKind::Equality;
      std::int64_t modulus = 0;
      if (hasLocal && row[n] != 0) {
        if (consumedLocal)
          return std::nullopt;
        auto coefficient = guard_detail::toI64(row[n]);
        if (!coefficient ||
            *coefficient == std::numeric_limits<std::int64_t>::min() ||
            *coefficient == 0)
          return std::nullopt;
        modulus = *coefficient < 0 ? -*coefficient : *coefficient;
        kind = GuardConstraintKind::Congruence;
        consumedLocal = true;
      }
      // Convert only the logical dimension coefficients and constant.
      llvm::SmallVector<llvm::DynamicAPInt> projected;
      projected.reserve(n + 1);
      for (std::size_t i = 0; i < n; ++i)
        projected.push_back(row[i]);
      projected.push_back(row.back());
      auto constraint = guard_detail::convertConstraint(
          projected, kind, plan.bounds);
      if (!constraint)
        return std::nullopt;
      constraint->modulus = modulus;
      conjunction.constraints.push_back(std::move(*constraint));
    }

    for (unsigned r = 0;
         r < disjunct.getNumInequalities(); ++r) {
      auto row = disjunct.getInequality(r);
      if (hasLocal && row[n] != 0)
        return std::nullopt;
      llvm::SmallVector<llvm::DynamicAPInt> projected;
      projected.reserve(n + 1);
      for (std::size_t i = 0; i < n; ++i)
        projected.push_back(row[i]);
      projected.push_back(row.back());
      auto constraint = guard_detail::convertConstraint(
          projected, GuardConstraintKind::Inequality, plan.bounds);
      if (!constraint)
        return std::nullopt;
      conjunction.constraints.push_back(std::move(*constraint));
    }

    result.disjuncts.push_back(
        std::move(conjunction));
  }

  return result;
}

} // namespace systolic::logical
