//===- Normalize.h - Reduction of inequality rows ---------------*- C++ -*-===//
//
// Rewrites rows a . y + c >= 0 into a small canonical form with one entry per
// direction: lower <= d . y <= upper, where d is primitive and its first
// nonzero entry is positive. Rows with the same direction merge, so a pair of
// opposite rows either meets in an interval or is a conflict.
//
// What is preserved depends on the domain, and this is the one place the two
// differ. Over the rationals a row is only scaled, which keeps every point.
// Over the integers it is also tightened: with g = gcd(a), the row becomes
// (a / g) . y + floor(c / g) >= 0, as in Pugh's Omega test (1991). That keeps
// every integer point and removes rational ones, which is why a rational query
// must never reach it.
//
//===----------------------------------------------------------------------===//

#ifndef SYSTOLIC_EXACT_NORMALIZE_H
#define SYSTOLIC_EXACT_NORMALIZE_H

#include "Systolic/Exact/Budget.h"
#include "Systolic/Exact/Rational.h"

namespace systolic::exact {

/// coefficients . y + constant >= 0. `origin` names the row it came from.
struct AffineRow {
  std::vector<Int> coefficients;
  Rational constant;
  std::size_t origin = 0;
};

/// A bound of a NormalizedRow, with the row that gave it.
struct RowBound {
  Rational value;
  std::size_t origin = 0;
};

/// lower <= direction . y <= upper. At least one bound is present.
struct NormalizedRow {
  std::vector<Int> direction;
  std::optional<RowBound> lower;
  std::optional<RowBound> upper;

  /// True when the direction has a single nonzero entry, which is then 1.
  bool isSingleVariable() const;
};

struct NormalizeResult {
  enum class Status { Normalized, ConstantConflict, OppositeConflict, Stopped };
  Status status = Status::Stopped;
  /// In order of first appearance, so the order depends only on the input.
  std::vector<NormalizedRow> rows;
  /// ConstantConflict: the false row. OppositeConflict: the row of the bound
  /// that is too high.
  std::size_t lowerOrigin = 0;
  /// OppositeConflict: the row of the bound that is too low.
  std::size_t upperOrigin = 0;
};

/// `meter` is borrowed for the call. Stopped leaves the reason in the meter.
NormalizeResult normalizeRows(const std::vector<AffineRow> &rows,
                              Domain domain, WorkMeter &meter);

} // namespace systolic::exact

#endif // SYSTOLIC_EXACT_NORMALIZE_H
