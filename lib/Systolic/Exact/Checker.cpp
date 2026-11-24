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
  for (const StrictInequality &row : system.strictInequalities())
    if (evaluate(row.expr, witness.values) <= Int(0))
      return refuse("the witness violates a strict inequality");
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
  for (const StrictInequality &row : system.strictInequalities())
    if (evaluateScaled(row.expr, witness.values, common) <= Int(0))
      return refuse("the witness violates a strict inequality");
  for (const Equality &row : system.equalities())
    if (evaluateScaled(row.expr, witness.values, common) != Int(0))
      return refuse("the witness violates an equality");
  return accept();
}

namespace {
/// a . (x, q) + c, over the system's variables and then the quotients. It is
/// known to be >= 0, or > 0 when `strict` is set.
struct Row {
  std::vector<Int> coefficients;
  Int constant{0};
  bool strict = false;
};
} // namespace

/// The row a RowRef names, or std::nullopt when it names nothing usable.
static std::optional<Row> inputRow(const System &system, const RowRef &ref,
                                   std::size_t width,
                                   const std::vector<Row> *assumptions) {
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
  case RowRef::Kind::StrictInequality:
    if (ref.negated || ref.index >= system.strictInequalities().size())
      return std::nullopt;
    expr = &system.strictInequalities()[ref.index].expr;
    row.strict = true;
    break;
  case RowRef::Kind::Assumption:
    if (ref.negated || !assumptions || ref.index >= assumptions->size())
      return std::nullopt;
    return (*assumptions)[ref.index];
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

/// Why the steps do not derive a false row, or null when they do. `assumptions`
/// are the rows assumed by the splits above this leaf, and may be null.
static const char *derivationError(const System &system, Domain domain,
                                   const InfeasibilityProof &proof,
                                   const std::vector<Row> *assumptions) {
  if (domain == Domain::Rational && !system.congruences().empty())
    return "a congruence has no rational meaning";
  if (proof.steps.empty())
    return "the proof has no steps";
  const std::size_t width =
      system.numVariables() + system.congruences().size();
  std::vector<Row> rows;
  rows.reserve(proof.steps.size());
  for (const ProofStep &step : proof.steps) {
    Row row;
    switch (step.rule) {
    case ProofStep::Rule::Input: {
      std::optional<Row> named = inputRow(system, step.row, width, assumptions);
      if (!named)
        return "an input step names no usable row";
      row = std::move(*named);
      break;
    }
    case ProofStep::Rule::Combine:
      if (step.terms.empty())
        return "a combine step has no terms";
      row.coefficients.assign(width, Int(0));
      for (const ProofTerm &term : step.terms) {
        if (term.step >= rows.size())
          return "a combine step reads a later step";
        if (term.multiplier <= Int(0))
          return "a combine step has a multiplier that is not positive";
        const Row &read = rows[term.step];
        for (std::size_t j = 0; j < width; ++j)
          row.coefficients[j] += term.multiplier * read.coefficients[j];
        row.constant += term.multiplier * read.constant;
        row.strict = row.strict || read.strict;
      }
      break;
    case ProofStep::Rule::Tighten: {
      if (domain != Domain::Integer)
        return "tightening is not valid over the rationals";
      if (step.source >= rows.size())
        return "a tighten step reads a later step";
      const Row &read = rows[step.source];
      if (read.strict)
        return "a tighten step reads a strict row";
      const Int g = contentOf(read.coefficients);
      if (g == Int(0))
        return "a tighten step reads a row with no variables";
      row.coefficients.reserve(width);
      for (const Int &c : read.coefficients)
        row.coefficients.push_back(*divFloor(c, g));
      row.constant = *divFloor(read.constant, g);
      break;
    }
    case ProofStep::Rule::Harden: {
      if (domain != Domain::Integer)
        return "hardening is not valid over the rationals";
      if (step.source >= rows.size())
        return "a harden step reads a later step";
      const Row &read = rows[step.source];
      if (!read.strict)
        return "a harden step reads a closed row";
      row = read;
      row.constant -= Int(1);
      row.strict = false;
      break;
    }
    }
    rows.push_back(std::move(row));
  }
  const Row &last = rows.back();
  for (const Int &c : last.coefficients)
    if (c != Int(0))
      return "the last step still has variables";
  if (last.constant > Int(0) || (last.constant == Int(0) && !last.strict))
    return "the last step is not a false row";
  return nullptr;
}

CheckResult checkInfeasibility(const System &system, Domain domain,
                               const InfeasibilityProof &proof) {
  if (const char *why = derivationError(system, domain, proof, nullptr))
    return refuse(why);
  return accept();
}

/// The row that one case of a split assumes.
static Row assumedRow(const BranchNode &node, bool lowCase) {
  Row row;
  row.coefficients.reserve(node.form.size());
  for (const Int &c : node.form)
    row.coefficients.push_back(lowCase ? -c : c);
  row.constant = lowCase ? node.bound : -node.bound - Int(1);
  return row;
}

CheckResult checkBranchProof(const System &system, const BranchProof &proof) {
  const std::size_t count = proof.nodes.size();
  if (count == 0)
    return refuse("the branch proof has no nodes");
  const std::size_t width =
      system.numVariables() + system.congruences().size();

  struct Visit {
    std::size_t node;
    unsigned casesStarted;
  };
  std::vector<Visit> stack;
  stack.push_back(Visit{0, 0});
  std::vector<bool> reached(count, false);
  reached[0] = true;
  std::size_t numReached = 1;
  std::vector<Row> assumptions;

  while (!stack.empty()) {
    const std::size_t index = stack.back().node;
    const BranchNode &node = proof.nodes[index];
    if (node.kind == BranchNode::Kind::Leaf) {
      if (const char *why = derivationError(system, Domain::Integer, node.leaf,
                                            &assumptions))
        return refuse(why);
      stack.pop_back();
      continue;
    }
    if (node.form.size() != width)
      return refuse("a split does not have one coefficient per column");
    const unsigned started = stack.back().casesStarted;
    if (started == 2) {
      assumptions.pop_back();
      stack.pop_back();
      continue;
    }
    const std::size_t child = started == 0 ? node.low : node.high;
    if (child >= count || reached[child])
      return refuse("a split reads a node that is missing or already reached");
    reached[child] = true;
    ++numReached;
    stack.back().casesStarted = started + 1;
    if (started == 0)
      assumptions.push_back(assumedRow(node, true));
    else
      assumptions.back() = assumedRow(node, false);
    stack.push_back(Visit{child, 0});
  }
  if (numReached != count)
    return refuse("a node of the proof is never reached");
  return accept();
}


} // namespace systolic::exact
