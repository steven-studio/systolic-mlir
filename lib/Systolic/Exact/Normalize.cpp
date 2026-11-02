//===- Normalize.cpp - Reduction of inequality rows -----------------------===//

#include "Systolic/Exact/Normalize.h"
#include "ExecutionCounters.h"

#include <map>

namespace systolic::exact {

bool NormalizedRow::isSingleVariable() const {
  unsigned nonzero = 0;
  for (const Int &c : direction)
    nonzero += c != Int(0);
  return nonzero == 1;
}

NormalizeResult normalizeRows(const std::vector<AffineRow> &rows, Domain domain,
                              WorkMeter &meter) {
  detail::noteExecution(detail::Stage::Normalize);
  NormalizeResult result;
  // Only looks entries up, so its order cannot reach the output.
  std::map<std::vector<Int>, std::size_t> positions;
  for (const AffineRow &row : rows) {
    if (!meter.charge(row.coefficients.size() + 1))
      return result;
    const Int g = contentOf(row.coefficients);
    if (g == Int(0)) {
      if (row.constant.sign() < 0) {
        result.status = NormalizeResult::Status::ConstantConflict;
        result.lowerOrigin = row.origin;
        return result;
      }
      continue;
    }
    Rational constant = *row.constant.dividedBy(Rational(g));
    if (domain == Domain::Integer)
      constant = Rational(constant.floor());
    std::vector<Int> direction;
    direction.reserve(row.coefficients.size());
    for (const Int &c : row.coefficients)
      direction.push_back(c / g);
    bool flipped = false;
    for (const Int &c : direction)
      if (c != Int(0)) {
        flipped = c < Int(0);
        break;
      }
    if (flipped)
      for (Int &c : direction)
        c = -c;

    auto [position, isNew] = positions.emplace(direction, result.rows.size());
    if (isNew) {
      result.rows.emplace_back();
      result.rows.back().direction = std::move(direction);
    }
    NormalizedRow &entry = result.rows[position->second];
    // d . y + c >= 0 is a lower bound of -c; -d . y + c >= 0 an upper bound of c.
    if (flipped) {
      if (!entry.upper || constant < entry.upper->value)
        entry.upper = RowBound{constant, row.origin};
    } else {
      const Rational bound = -constant;
      if (!entry.lower || bound > entry.lower->value)
        entry.lower = RowBound{bound, row.origin};
    }
    if (entry.lower && entry.upper && entry.lower->value > entry.upper->value) {
      result.status = NormalizeResult::Status::OppositeConflict;
      result.lowerOrigin = entry.lower->origin;
      result.upperOrigin = entry.upper->origin;
      return result;
    }
  }
  result.status = NormalizeResult::Status::Normalized;
  return result;
}

} // namespace systolic::exact
