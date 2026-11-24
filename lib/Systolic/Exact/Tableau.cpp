//===- Tableau.cpp - A bounds-based tableau for rational feasibility ------===//

#include "Systolic/Exact/Tableau.h"
#include "ExecutionCounters.h"

namespace systolic::exact {

/// Whether a computed value may stay, under the coefficient limit.
static bool admitRational(WorkMeter &meter, const Rational &value) {
  return meter.admit(value.numerator()) && meter.admit(value.denominator());
}

static bool admitDelta(WorkMeter &meter, const DeltaRational &value) {
  return admitRational(meter, value.real()) &&
         admitRational(meter, value.delta());
}

BoundsTableau::BoundsTableau(unsigned numStructural)
    : structural(numStructural) {
  for (unsigned j = 0; j < numStructural; ++j) {
    nonbasic.push_back(j);
    places.push_back(Place{false, j});
  }
  values.resize(numStructural);
  lowers.resize(numStructural);
  uppers.resize(numStructural);
}

std::vector<Int> BoundsTableau::form(unsigned variable) const {
  if (variable >= structural)
    return slackForms[variable - structural];
  std::vector<Int> unit(structural, Int(0));
  unit[variable] = Int(1);
  return unit;
}

std::optional<unsigned>
BoundsTableau::variableForForm(const std::vector<Int> &form, WorkMeter &meter) {
  if (!meter.charge(form.size() + 1))
    return std::nullopt;
  unsigned nonzero = 0, last = 0;
  for (unsigned j = 0; j < form.size(); ++j) {
    if (form[j] != Int(0)) {
      ++nonzero;
      last = j;
    }
  }
  if (nonzero == 1)
    return last; // primitive and positive, so this is the unit form of x_last
  const auto found = known.find(form);
  if (found != known.end())
    return found->second;

  const std::uint64_t numRows = cells.size() + 1;
  if (numRows * nonbasic.size() > meter.workLimits().maxTableauCells) {
    meter.stop(UnknownReason::TableauLimit);
    return std::nullopt;
  }

  std::vector<Rational> row(nonbasic.size());
  DeltaRational value;
  for (unsigned j = 0; j < form.size(); ++j) {
    if (form[j] == Int(0))
      continue;
    const Rational factor(form[j]);
    if (!meter.charge(nonbasic.size() + 1))
      return std::nullopt;
    value = value + values[j] * factor;
    if (!places[j].isBasic) {
      Rational &cell = row[places[j].position];
      cell = cell + factor;
      if (!admitRational(meter, cell))
        return std::nullopt;
      continue;
    }
    const std::vector<Rational> &source = cells[places[j].position];
    for (std::size_t c = 0; c < source.size(); ++c) {
      if (source[c].sign() == 0)
        continue;
      row[c] = row[c] + factor * source[c];
      if (!admitRational(meter, row[c]))
        return std::nullopt;
    }
  }
  if (!admitDelta(meter, value))
    return std::nullopt;

  const unsigned variable = numVariables();
  slackForms.push_back(form);
  known.emplace(form, variable);
  values.push_back(std::move(value));
  lowers.emplace_back();
  uppers.emplace_back();
  places.push_back(Place{true, basic.size()});
  basic.push_back(variable);
  cells.push_back(std::move(row));
  return variable;
}

bool BoundsTableau::isBelowLower(unsigned variable) const {
  return lowers[variable] && values[variable] < lowers[variable]->value;
}

bool BoundsTableau::isAboveUpper(unsigned variable) const {
  return uppers[variable] && values[variable] > uppers[variable]->value;
}

bool BoundsTableau::canIncrease(unsigned variable) const {
  return !uppers[variable] || values[variable] < uppers[variable]->value;
}

bool BoundsTableau::canDecrease(unsigned variable) const {
  return !lowers[variable] || values[variable] > lowers[variable]->value;
}

bool BoundsTableau::moveNonbasic(unsigned variable, const DeltaRational &target,
                                 WorkMeter &meter) {
  if (!meter.charge(cells.size() + 1))
    return false;
  const std::size_t column = places[variable].position;
  const DeltaRational change = target - values[variable];
  for (std::size_t r = 0; r < cells.size(); ++r) {
    const Rational &a = cells[r][column];
    if (a.sign() == 0)
      continue;
    DeltaRational &moved = values[basic[r]];
    moved = moved + change * a;
    if (!admitDelta(meter, moved))
      return false;
  }
  values[variable] = target;
  return true;
}

void BoundsTableau::addTerm(const Bound &bound, const Rational &multiplier) {
  conflictTerms.push_back(FarkasTerm{bound.reason, multiplier * bound.scale});
}

void BoundsTableau::explainCrossing(const Bound &low, const Bound &high) {
  conflictTerms.clear();
  addTerm(low, Rational(Int(1)));
  addTerm(high, Rational(Int(1)));
}

BoundsTableau::AssertOutcome
BoundsTableau::assertLower(unsigned variable, Bound bound, WorkMeter &meter) {
  if (lowers[variable] && bound.value <= lowers[variable]->value)
    return AssertOutcome::Applied;
  if (uppers[variable] && bound.value > uppers[variable]->value) {
    explainCrossing(bound, *uppers[variable]);
    return AssertOutcome::Conflict;
  }
  const DeltaRational target = bound.value;
  lowers[variable] = std::move(bound);
  if (!places[variable].isBasic && values[variable] < target &&
      !moveNonbasic(variable, target, meter))
    return AssertOutcome::Stopped;
  return AssertOutcome::Applied;
}

BoundsTableau::AssertOutcome
BoundsTableau::assertUpper(unsigned variable, Bound bound, WorkMeter &meter) {
  if (uppers[variable] && bound.value >= uppers[variable]->value)
    return AssertOutcome::Applied;
  if (lowers[variable] && bound.value < lowers[variable]->value) {
    explainCrossing(*lowers[variable], bound);
    return AssertOutcome::Conflict;
  }
  const DeltaRational target = bound.value;
  uppers[variable] = std::move(bound);
  if (!places[variable].isBasic && values[variable] > target &&
      !moveNonbasic(variable, target, meter))
    return AssertOutcome::Stopped;
  return AssertOutcome::Applied;
}

/// With x_b = sum_j a_j x_j and no way to repair x_b, every x_j sits at the
/// bound that pushes x_b the wrong way. So x_b minus its bound and each a_j
/// times the distance of x_j to that bound sum to the violation, and the
/// variables cancel.
void BoundsTableau::explainRow(std::size_t row, bool needsIncrease) {
  conflictTerms.clear();
  const unsigned leaving = basic[row];
  addTerm(needsIncrease ? *lowers[leaving] : *uppers[leaving],
          Rational(Int(1)));
  const std::vector<Rational> &coefficients = cells[row];
  for (std::size_t c = 0; c < coefficients.size(); ++c) {
    const int sign = coefficients[c].sign();
    if (sign == 0)
      continue;
    const unsigned variable = nonbasic[c];
    const bool atUpper = (sign > 0) == needsIncrease;
    addTerm(atUpper ? *uppers[variable] : *lowers[variable],
            sign > 0 ? coefficients[c] : -coefficients[c]);
  }
}

bool BoundsTableau::pivot(std::size_t row, std::size_t column,
                          WorkMeter &meter) {
  std::vector<Rational> &source = cells[row];
  std::uint64_t filled = 0;
  for (const Rational &cell : source)
    filled += cell.sign() != 0;
  std::uint64_t touched = 0;
  for (std::size_t r = 0; r < cells.size(); ++r)
    touched += r != row && cells[r][column].sign() != 0;
  if (!meter.charge(cells.size() + source.size() + (touched + 1) * filled))
    return false;

  // Solve x_b = sum_j a_j x_j for the entering x_e, whose coefficient is a_e.
  const Rational inverse =
      *Rational(Int(1)).dividedBy(source[column]);
  for (std::size_t c = 0; c < source.size(); ++c) {
    if (c == column || source[c].sign() == 0)
      continue;
    source[c] = -(source[c] * inverse);
    if (!admitRational(meter, source[c]))
      return false;
  }
  // The same numbers as the cell it replaces, which is already admitted.
  source[column] = inverse;

  // Put that into every other row that reads x_e. x_b takes x_e's column.
  for (std::size_t r = 0; r < cells.size(); ++r) {
    if (r == row)
      continue;
    std::vector<Rational> &target = cells[r];
    const Rational factor = target[column];
    if (factor.sign() == 0)
      continue;
    target[column] = Rational();
    for (std::size_t c = 0; c < source.size(); ++c) {
      if (source[c].sign() == 0)
        continue;
      target[c] = target[c] + factor * source[c];
      if (!admitRational(meter, target[c]))
        return false;
    }
  }

  const unsigned leaving = basic[row], entering = nonbasic[column];
  basic[row] = entering;
  nonbasic[column] = leaving;
  places[entering] = Place{true, row};
  places[leaving] = Place{false, column};
  latestPivot = std::make_pair(leaving, entering);
  ++pivots;
  detail::noteExecution(detail::Stage::Pivot);
  return true;
}

bool BoundsTableau::pivotAndUpdate(std::size_t row, std::size_t column,
                                   const DeltaRational &target,
                                   WorkMeter &meter) {
  if (!meter.charge(cells.size() + 1))
    return false;
  const unsigned leaving = basic[row], entering = nonbasic[column];
  const Rational inverse =
      *Rational(Int(1)).dividedBy(cells[row][column]);
  const DeltaRational change = (target - values[leaving]) * inverse;
  values[leaving] = target;
  values[entering] = values[entering] + change;
  if (!admitDelta(meter, values[entering]))
    return false;
  for (std::size_t r = 0; r < cells.size(); ++r) {
    if (r == row || cells[r][column].sign() == 0)
      continue;
    DeltaRational &moved = values[basic[r]];
    moved = moved + change * cells[r][column];
    if (!admitDelta(meter, moved))
      return false;
  }
  return pivot(row, column, meter);
}

// Why repeating step() ends, by Bland's argument in the bounds form of Dutertre
// and de Moura. Suppose it pivots forever. There are finitely many variables, so
// from some step on, every variable that changes status does so infinitely
// often. Call those F, and let x_t be the largest of F.
//
// Take a step S1 where x_t leaves, say from below its lower bound l. It is the
// smallest basic variable out of bounds, and nonbasic ones are in bounds, so
// every variable smaller than x_t is in bounds at S1. x_t is set to l, and stays
// there until it enters again, at some later step S2, to repair a leaving x_s.
// x_s is in F, so s < t. Say x_s is below its lower bound at S2. The other three
// cases, in which x_t or x_s starts on the upper side, are mirror images.
//
// Let d be the values at S2 minus the values at S1. Both satisfy the same
// definitions, so d does too: d_s = sum_j b_j d_j, over the variables nonbasic
// at S2, where b is the row of x_s at S2. Look at the terms:
//
//   j > t  x_j is not in F, and is nonbasic at S2, so it never moved: d_j = 0.
//   j < t  With b_j nonzero, x_j could not repair x_s, since x_t was the smallest
//          that could. So it sits at its upper bound if b_j > 0 and at its lower
//          bound if b_j < 0. It was in bounds at S1, so b_j d_j >= 0.
//   j = t  x_t is at l, below which it was at S1, so d_t > 0. It could repair
//          x_s from l, where it can only go up, so b_t > 0, and b_t d_t > 0.
//
// So d_s > 0. But x_s was in bounds at S1 and is below its lower bound at S2, so
// d_s < 0. That is a contradiction. Nothing here uses more than an ordered
// vector space, so it holds for values with a delta part too.
BoundsTableau::StepOutcome BoundsTableau::step(WorkMeter &meter) {
  if (!meter.charge(cells.size() + nonbasic.size() + 1))
    return StepOutcome::Stopped;

  // Bland: the smallest basic variable out of bounds.
  std::optional<std::size_t> row;
  for (std::size_t r = 0; r < basic.size(); ++r) {
    const bool smaller = !row || basic[r] < basic[*row];
    if (smaller && (isBelowLower(basic[r]) || isAboveUpper(basic[r])))
      row = r;
  }
  if (!row)
    return StepOutcome::Feasible;

  const unsigned leaving = basic[*row];
  const bool needsIncrease = isBelowLower(leaving);

  // Bland: the smallest nonbasic variable that can move it the right way.
  std::optional<std::size_t> column;
  const std::vector<Rational> &coefficients = cells[*row];
  for (std::size_t c = 0; c < coefficients.size(); ++c) {
    const int sign = coefficients[c].sign();
    if (sign == 0)
      continue;
    const unsigned candidate = nonbasic[c];
    const bool increases = (sign > 0) == needsIncrease;
    const bool able =
        increases ? canIncrease(candidate) : canDecrease(candidate);
    if (able && (!column || candidate < nonbasic[*column]))
      column = c;
  }
  if (!column) {
    explainRow(*row, needsIncrease);
    return StepOutcome::Conflict;
  }

  const DeltaRational target =
      needsIncrease ? lowers[leaving]->value : uppers[leaving]->value;
  if (!pivotAndUpdate(*row, *column, target, meter))
    return StepOutcome::Stopped;
  return StepOutcome::Pivoted;
}

std::optional<std::vector<Rational>>
BoundsTableau::concreteValues(WorkMeter &meter) const {
  if (!meter.charge(2 * std::uint64_t(numVariables()) + 1))
    return std::nullopt;
  // low <= high holds in the delta order. It holds for a concrete epsilon when
  // epsilon * (low.delta - high.delta) <= high.real - low.real. That limits
  // epsilon only when low.delta > high.delta, and then the order says that
  // low.real < high.real, so the room is positive.
  Rational epsilon(Int(1));
  auto keepOrdered = [&epsilon](const DeltaRational &low,
                                const DeltaRational &high) {
    if (low.delta() <= high.delta())
      return;
    const Rational room = *(high.real() - low.real())
                               .dividedBy(low.delta() - high.delta());
    if (room < epsilon)
      epsilon = room;
  };
  for (unsigned v = 0; v < numVariables(); ++v) {
    if (lowers[v])
      keepOrdered(lowers[v]->value, values[v]);
    if (uppers[v])
      keepOrdered(values[v], uppers[v]->value);
  }
  std::vector<Rational> result;
  result.reserve(structural);
  for (unsigned j = 0; j < structural; ++j)
    result.push_back(values[j].at(epsilon));
  return result;
}

static void appendRef(std::string &text, const RowRef &ref) {
  switch (ref.kind) {
  case RowRef::Kind::Inequality:
    text += "ge";
    break;
  case RowRef::Kind::StrictInequality:
    text += "gt";
    break;
  case RowRef::Kind::Equality:
    text += "eq";
    break;
  case RowRef::Kind::Congruence:
    text += "cong";
    break;
  case RowRef::Kind::Assumption:
    text += "case";
    break;
  }
  text += std::to_string(ref.index);
  text += ref.negated ? "-" : "+";
}

static void appendBound(std::string &text, const char *name,
                        const std::optional<Bound> &bound) {
  if (!bound)
    return;
  text += ' ';
  text += name;
  text += '=';
  text += bound->value.toString();
  text += '@';
  appendRef(text, bound->reason);
  text += '*';
  text += bound->scale.toString();
}

std::string BoundsTableau::stateText() const {
  std::string text = "tableau ";
  text += std::to_string(structural);
  text += ' ';
  text += std::to_string(numVariables());
  text += " pivots ";
  text += std::to_string(pivots);
  text += '\n';
  for (unsigned v = 0; v < numVariables(); ++v) {
    text += places[v].isBasic ? "b " : "n ";
    text += std::to_string(v);
    text += " at ";
    text += std::to_string(places[v].position);
    text += " value=";
    text += values[v].toString();
    appendBound(text, "lower", lowers[v]);
    appendBound(text, "upper", uppers[v]);
    text += " form";
    for (const Int &c : form(v)) {
      text += ' ';
      text += toDecimal(c);
    }
    text += '\n';
  }
  for (std::size_t r = 0; r < cells.size(); ++r) {
    text += "row ";
    text += std::to_string(basic[r]);
    text += " =";
    for (std::size_t c = 0; c < cells[r].size(); ++c) {
      if (cells[r][c].sign() == 0)
        continue;
      text += ' ';
      text += cells[r][c].toString();
      text += "*x";
      text += std::to_string(nonbasic[c]);
    }
    text += '\n';
  }
  return text;
}

} // namespace systolic::exact
