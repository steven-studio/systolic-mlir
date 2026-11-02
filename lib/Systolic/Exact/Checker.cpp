//===- Checker.cpp - Independent checks of definite answers ---------------===//

#include "Systolic/Exact/Checker.h"
#include "ExecutionCounters.h"

namespace systolic::exact {

static CheckResult accept() {
  detail::noteExecution(detail::Stage::Check);
  return CheckResult{true, std::string()};
}

static CheckResult refuse(std::string why) {
  detail::noteExecution(detail::Stage::Check);
  detail::noteExecution(detail::Stage::CheckRefused);
  return CheckResult{false, std::move(why)};
}

static Int evaluate(const LinearExpr &expr, const std::vector<Int> &point) {
  Int total = expr.constant;
  for (std::size_t j = 0; j < point.size(); ++j)
    total += expr.coefficients[j] * point[j];
  return total;
}

/// The row at a rational point, times `common`, which every denominator divides.
/// Integers alone: the checker shares no rational arithmetic with the solver.
static Int evaluateScaled(const LinearExpr &expr,
                          const std::vector<Rational> &point,
                          const Int &common) {
  Int total = expr.constant * common;
  for (std::size_t j = 0; j < point.size(); ++j)
    total += expr.coefficients[j] * point[j].numerator() *
             (common / point[j].denominator());
  return total;
}

CheckResult checkWitness(const System &system, const IntegerWitness &witness) {
  if (witness.values.size() != system.numVariables())
    return refuse("the witness does not have one value per variable");
  for (const Inequality &row : system.inequalities())
    if (evaluate(row.expr, witness.values) < Int(0))
      return refuse("the witness violates an inequality");
  for (const Equality &row : system.equalities())
    if (evaluate(row.expr, witness.values) != Int(0))
      return refuse("the witness violates an equality");
  for (const Congruence &row : system.congruences())
    if (*modFloor(evaluate(row.expr, witness.values), row.modulus) != Int(0))
      return refuse("the witness violates a congruence");
  return accept();
}

CheckResult checkWitness(const System &system, const RationalWitness &witness) {
  if (!system.congruences().empty())
    return refuse("a congruence has no rational meaning");
  if (witness.values.size() != system.numVariables())
    return refuse("the witness does not have one value per variable");
  Int common(1);
  for (const Rational &value : witness.values)
    common = lcmOf(common, value.denominator());
  for (const Inequality &row : system.inequalities())
    if (evaluateScaled(row.expr, witness.values, common) < Int(0))
      return refuse("the witness violates an inequality");
  for (const Equality &row : system.equalities())
    if (evaluateScaled(row.expr, witness.values, common) != Int(0))
      return refuse("the witness violates an equality");
  return accept();
}

namespace {
/// a . (x, q) + c, over the system's variables and then the quotients.
struct Row {
  std::vector<Int> coefficients;
  Int constant{0};
};
} // namespace

/// The row a RowRef names, or std::nullopt when it names nothing usable.
static std::optional<Row> inputRow(const System &system, const RowRef &ref,
                                   std::size_t width) {
  const std::size_t quotientBase = system.numVariables();
  const LinearExpr *expr = nullptr;
  Row row;
  row.coefficients.assign(width, Int(0));
  switch (ref.kind) {
  case RowRef::Kind::Inequality:
    if (ref.negated || ref.index >= system.inequalities().size())
      return std::nullopt;
    expr = &system.inequalities()[ref.index].expr;
    break;
  case RowRef::Kind::Equality:
    if (ref.index >= system.equalities().size())
      return std::nullopt;
    expr = &system.equalities()[ref.index].expr;
    break;
  case RowRef::Kind::Congruence:
    if (ref.index >= system.congruences().size())
      return std::nullopt;
    expr = &system.congruences()[ref.index].expr;
    row.coefficients[quotientBase + ref.index] =
        -system.congruences()[ref.index].modulus;
    break;
  }
  for (std::size_t j = 0; j < quotientBase; ++j)
    row.coefficients[j] = expr->coefficients[j];
  row.constant = expr->constant;
  if (ref.negated) {
    for (Int &c : row.coefficients)
      c = -c;
    row.constant = -row.constant;
  }
  return row;
}

CheckResult checkInfeasibility(const System &system, Domain domain,
                               const InfeasibilityProof &proof) {
  if (domain == Domain::Rational && !system.congruences().empty())
    return refuse("a congruence has no rational meaning");
  if (proof.steps.empty())
    return refuse("the proof has no steps");
  const std::size_t width =
      system.numVariables() + system.congruences().size();
  std::vector<Row> rows;
  rows.reserve(proof.steps.size());
  for (const ProofStep &step : proof.steps) {
    Row row;
    switch (step.rule) {
    case ProofStep::Rule::Input: {
      std::optional<Row> named = inputRow(system, step.row, width);
      if (!named)
        return refuse("an input step names no usable row");
      row = std::move(*named);
      break;
    }
    case ProofStep::Rule::Combine:
      if (step.terms.empty())
        return refuse("a combine step has no terms");
      row.coefficients.assign(width, Int(0));
      for (const ProofTerm &term : step.terms) {
        if (term.step >= rows.size())
          return refuse("a combine step reads a later step");
        if (term.multiplier <= Int(0))
          return refuse("a combine step has a multiplier that is not positive");
        const Row &read = rows[term.step];
        for (std::size_t j = 0; j < width; ++j)
          row.coefficients[j] += term.multiplier * read.coefficients[j];
        row.constant += term.multiplier * read.constant;
      }
      break;
    case ProofStep::Rule::Tighten: {
      if (domain != Domain::Integer)
        return refuse("tightening is not valid over the rationals");
      if (step.source >= rows.size())
        return refuse("a tighten step reads a later step");
      const Row &read = rows[step.source];
      const Int g = contentOf(read.coefficients);
      if (g == Int(0))
        return refuse("a tighten step reads a row with no variables");
      row.coefficients.reserve(width);
      for (const Int &c : read.coefficients)
        row.coefficients.push_back(*divFloor(c, g));
      row.constant = *divFloor(read.constant, g);
      break;
    }
    }
    rows.push_back(std::move(row));
  }
  const Row &last = rows.back();
  for (const Int &c : last.coefficients)
    if (c != Int(0))
      return refuse("the last step still has variables");
  if (last.constant >= Int(0))
    return refuse("the last step is not a false row");
  return accept();
}

} // namespace systolic::exact
