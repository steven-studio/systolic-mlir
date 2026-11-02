//===- Decide.cpp - The feasibility decision ------------------------------===//

#include "Systolic/Exact/Decide.h"
#include "Systolic/Exact/Checker.h"
#include "Systolic/Exact/Lattice.h"
#include "Systolic/Exact/Normalize.h"
#include "ExecutionCounters.h"

namespace systolic::exact {

/// The only code that builds a Decision. Definite ones go through the checker.
struct DecisionMaker {
  static Decision unknown(Domain domain, UnknownReason reason) {
    Decision decision;
    decision.where = domain;
    decision.reason = reason;
    return decision;
  }

  static Decision feasible(const System &system, IntegerWitness witness) {
    if (!checkWitness(system, witness).accepted)
      return rejected(Domain::Integer);
    Decision decision;
    decision.answer = Verdict::Feasible;
    decision.integerPoint = std::move(witness);
    decision.evidence.ranChecker = true;
    return decision;
  }

  static Decision feasible(const System &system, RationalWitness witness) {
    if (!checkWitness(system, witness).accepted)
      return rejected(Domain::Rational);
    Decision decision;
    decision.answer = Verdict::Feasible;
    decision.where = Domain::Rational;
    decision.rationalPoint = std::move(witness);
    decision.evidence.ranChecker = true;
    return decision;
  }

  static Decision infeasible(const System &system, Domain domain,
                             InfeasibilityProof proof) {
    if (!checkInfeasibility(system, domain, proof).accepted)
      return rejected(domain);
    Decision decision;
    decision.answer = Verdict::Infeasible;
    decision.where = domain;
    decision.emptyProof = std::move(proof);
    decision.evidence.ranChecker = true;
    return decision;
  }

  static void seal(Decision &decision, const System &system, std::uint64_t steps,
                   bool ranLattice, bool ranNormalize) {
    decision.evidence.steps = steps;
    decision.evidence.ranLattice = ranLattice;
    decision.evidence.ranNormalize = ranNormalize;
    decision.evidence.digest = fnv1a64(receiptText(system, decision));
  }

private:
  static Decision rejected(Domain domain) {
    Decision decision = unknown(domain, UnknownReason::CertificateRejected);
    decision.evidence.ranChecker = true;
    return decision;
  }
};

Decision certifyFeasible(const System &system, IntegerWitness witness) {
  return DecisionMaker::feasible(system, std::move(witness));
}

Decision certifyFeasible(const System &system, RationalWitness witness) {
  return DecisionMaker::feasible(system, std::move(witness));
}

Decision certifyInfeasible(const System &system, Domain domain,
                           InfeasibilityProof proof) {
  return DecisionMaker::infeasible(system, domain, std::move(proof));
}

std::uint64_t fnv1a64(std::string_view bytes) {
  std::uint64_t hash = 14695981039346656037ULL;
  for (char c : bytes) {
    hash ^= static_cast<unsigned char>(c);
    hash *= 1099511628211ULL;
  }
  return hash;
}

std::string receiptText(const System &system, const Decision &decision) {
  const ExecutionReceipt &receipt = decision.receipt();
  std::string text = canonicalText(system);
  text += "domain=";
  text += domainName(decision.domain());
  text += " verdict=";
  text += verdictName(decision.verdict());
  text += " reason=";
  text += unknownReasonName(decision.why());
  text += " steps=";
  text += std::to_string(receipt.steps);
  text += "\nstages=";
  std::string stages;
  auto note = [&stages](bool ran, const char *name) {
    if (!ran)
      return;
    if (!stages.empty())
      stages += ',';
    stages += name;
  };
  note(receipt.ranLattice, "lattice");
  note(receipt.ranNormalize, "normalize");
  note(receipt.ranChecker, "checker");
  text += stages;
  text += '\n';
  return text;
}

std::string canonicalText(const Decision &decision) {
  std::string text = verdictName(decision.verdict());
  text += ' ';
  text += domainName(decision.domain());
  text += ' ';
  text += unknownReasonName(decision.why());
  text += '\n';
  if (const IntegerWitness *point = decision.integerWitness())
    for (const Int &value : point->values)
      text += toDecimal(value) + "\n";
  if (const RationalWitness *point = decision.rationalWitness())
    for (const Rational &value : point->values)
      text += value.toString() + "\n";
  if (const InfeasibilityProof *proof = decision.proof())
    text += canonicalText(*proof);
  return text;
}

namespace {

/// One run of decide(). The variables of a run are the system's, followed by
/// one quotient per congruence, which turns expr == 0 (mod m) into the equality
/// expr - m * q == 0.
class Query {
public:
  Query(const System &system, Domain domain, WorkMeter &meter)
      : system(system), domain(domain), meter(meter),
        numColumns(system.numVariables() + system.congruences().size()) {}

  Decision run();

  bool ranLattice = false;
  bool ranNormalize = false;

private:
  Decision stopped() const {
    return DecisionMaker::unknown(domain, meter.reason());
  }
  Decision unknown(UnknownReason reason) const {
    return DecisionMaker::unknown(domain, reason);
  }

  bool admitInput();
  void buildEqualities();
  std::vector<Int> padded(const LinearExpr &expr) const;

  Decision refuteEqualities(const EqualityConflict &conflict);
  Decision refuteConstantRow(std::size_t inequality);
  Decision refuteOppositeRows(std::size_t lower, std::size_t upper);
  Decision witnessFrom(const std::vector<NormalizedRow> &rows);

  /// Appends a step and returns its index.
  std::size_t add(ProofStep step) {
    proof.steps.push_back(std::move(step));
    return proof.steps.size() - 1;
  }
  /// A row that is `scale` times the reduced form of an inequality.
  struct Derived {
    std::size_t step;
    Int scale;
  };
  Derived deriveReduced(std::size_t inequality);
  /// Adds the input steps for sum m_i (row i), for integer multipliers m_i.
  std::vector<ProofTerm> equalityTerms(const std::vector<Int> &multipliers);

  const System &system;
  Domain domain;
  WorkMeter &meter;
  unsigned numColumns;
  std::vector<EqualityRow> equalities;
  std::vector<RowRef> equalityRefs;
  EqualitySolution solution;
  InfeasibilityProof proof;
};

std::vector<Int> Query::padded(const LinearExpr &expr) const {
  std::vector<Int> coefficients = expr.coefficients;
  coefficients.resize(numColumns, Int(0));
  return coefficients;
}

bool Query::admitInput() {
  auto admitExpr = [&](const LinearExpr &expr) {
    if (!meter.charge(expr.coefficients.size() + 1) ||
        !meter.admit(expr.constant))
      return false;
    for (const Int &c : expr.coefficients)
      if (!meter.admit(c))
        return false;
    return true;
  };
  for (const Inequality &row : system.inequalities())
    if (!admitExpr(row.expr))
      return false;
  for (const Equality &row : system.equalities())
    if (!admitExpr(row.expr))
      return false;
  for (const Congruence &row : system.congruences())
    if (!admitExpr(row.expr) || !meter.admit(row.modulus))
      return false;
  return true;
}

void Query::buildEqualities() {
  const auto &eqs = system.equalities();
  for (std::size_t i = 0; i < eqs.size(); ++i) {
    equalities.push_back(EqualityRow{padded(eqs[i].expr), eqs[i].expr.constant});
    equalityRefs.push_back(RowRef{RowRef::Kind::Equality, i, false});
  }
  const auto &congs = system.congruences();
  for (std::size_t i = 0; i < congs.size(); ++i) {
    EqualityRow row{padded(congs[i].expr), congs[i].expr.constant};
    row.coefficients[system.numVariables() + i] = -congs[i].modulus;
    equalities.push_back(std::move(row));
    equalityRefs.push_back(RowRef{RowRef::Kind::Congruence, i, false});
  }
}

/// The least common multiple of the denominators.
static Int commonDenominator(const std::vector<Rational> &values) {
  Int common(1);
  for (const Rational &value : values)
    common = lcmOf(common, value.denominator());
  return common;
}

static std::vector<Int> scaledToIntegers(const std::vector<Rational> &values,
                                         const Int &scale) {
  std::vector<Int> scaled;
  scaled.reserve(values.size());
  for (const Rational &value : values)
    scaled.push_back((value * Rational(scale)).numerator());
  return scaled;
}

std::vector<ProofTerm>
Query::equalityTerms(const std::vector<Int> &multipliers) {
  std::vector<ProofTerm> terms;
  for (std::size_t i = 0; i < multipliers.size(); ++i) {
    if (multipliers[i] == Int(0))
      continue;
    RowRef row = equalityRefs[i];
    row.negated = multipliers[i] < Int(0);
    terms.push_back(ProofTerm{add(ProofStep::input(row)), magnitude(multipliers[i])});
  }
  return terms;
}

Decision Query::refuteEqualities(const EqualityConflict &conflict) {
  const Int scale = commonDenominator(conflict.multipliers);
  const std::vector<Int> up = scaledToIntegers(conflict.multipliers, scale);
  std::vector<Int> down;
  down.reserve(up.size());
  for (const Int &value : up)
    down.push_back(-value);

  if (conflict.kind == EqualityConflict::Kind::Inconsistent) {
    // The sum has no variables, so one of its two signs is a false row.
    Int constant(0);
    for (std::size_t i = 0; i < up.size(); ++i)
      constant += up[i] * equalities[i].constant;
    add(ProofStep::combine(equalityTerms(constant < Int(0) ? up : down)));
  } else {
    // scale * (v . x + q) and its negative, with v integral and q not. Each
    // tightens by `scale` to floor(q) and floor(-q), which sum to -1.
    const std::size_t plus = add(ProofStep::combine(equalityTerms(up)));
    const std::size_t minus = add(ProofStep::combine(equalityTerms(down)));
    const std::size_t tightPlus = add(ProofStep::tighten(plus));
    const std::size_t tightMinus = add(ProofStep::tighten(minus));
    add(ProofStep::combine(
        {ProofTerm{tightPlus, Int(1)}, ProofTerm{tightMinus, Int(1)}}));
  }
  return DecisionMaker::infeasible(system, domain, std::move(proof));
}

Query::Derived Query::deriveReduced(std::size_t inequality) {
  const LinearExpr &expr = system.inequalities()[inequality].expr;
  const std::vector<Rational> weights = solution.multipliers(padded(expr));
  const Int scale = commonDenominator(weights);
  const std::size_t base = add(ProofStep::input(
      RowRef{RowRef::Kind::Inequality, inequality, false}));
  std::vector<Int> multipliers = scaledToIntegers(weights, scale);
  // The row minus sum w_i (equality i) depends on the free coordinates alone.
  for (Int &value : multipliers)
    value = -value;
  std::vector<ProofTerm> terms = equalityTerms(multipliers);
  if (terms.empty() && scale == Int(1))
    return Derived{base, scale};
  terms.insert(terms.begin(), ProofTerm{base, scale});
  return Derived{add(ProofStep::combine(std::move(terms))), scale};
}

Decision Query::refuteConstantRow(std::size_t inequality) {
  // The reduced row has no variables, and its constant is negative.
  deriveReduced(inequality);
  return DecisionMaker::infeasible(system, domain, std::move(proof));
}

Decision Query::refuteOppositeRows(std::size_t lower, std::size_t upper) {
  const Derived low = deriveReduced(lower);
  const Derived high = deriveReduced(upper);
  if (domain == Domain::Integer) {
    const std::size_t tightLow = add(ProofStep::tighten(low.step));
    const std::size_t tightHigh = add(ProofStep::tighten(high.step));
    add(ProofStep::combine(
        {ProofTerm{tightLow, Int(1)}, ProofTerm{tightHigh, Int(1)}}));
  } else {
    // The rows are g_l * s_l * d and -g_h * s_h * d in the free coordinates.
    const Int lowSize =
        low.scale * contentOf(solution.freeCoefficients(
                        padded(system.inequalities()[lower].expr)));
    const Int highSize =
        high.scale * contentOf(solution.freeCoefficients(
                         padded(system.inequalities()[upper].expr)));
    add(ProofStep::combine(
        {ProofTerm{low.step, highSize}, ProofTerm{high.step, lowSize}}));
  }
  return DecisionMaker::infeasible(system, domain, std::move(proof));
}

Decision Query::witnessFrom(const std::vector<NormalizedRow> &rows) {
  // Every row is an interval on one free coordinate, so the coordinates are
  // independent. Take the value nearest zero in each.
  std::vector<Rational> free(solution.numFree());
  for (const NormalizedRow &row : rows) {
    std::size_t j = 0;
    while (row.direction[j] == Int(0))
      ++j;
    if (row.lower && free[j] < row.lower->value)
      free[j] = row.lower->value;
    if (row.upper && free[j] > row.upper->value)
      free[j] = row.upper->value;
  }
  std::vector<Rational> point = solution.point(free);
  point.resize(system.numVariables());
  if (domain == Domain::Rational)
    return DecisionMaker::feasible(system, RationalWitness{std::move(point)});
  IntegerWitness witness;
  witness.values.reserve(point.size());
  for (const Rational &value : point) {
    if (!value.isInteger())
      return unknown(UnknownReason::CertificateRejected);
    witness.values.push_back(value.numerator());
  }
  return DecisionMaker::feasible(system, std::move(witness));
}

Decision Query::run() {
  if (domain == Domain::Rational && !system.congruences().empty())
    return unknown(UnknownReason::UnsupportedConstruct);
  const WorkLimits &limits = meter.workLimits();
  if (numColumns > limits.maxVariables)
    return unknown(UnknownReason::VariableLimit);
  if (system.numRows() > limits.maxRows)
    return unknown(UnknownReason::RowLimit);
  if (!admitInput())
    return stopped();

  buildEqualities();
  ranLattice = true;
  LatticeResult lattice =
      solveEqualities(numColumns, equalities, domain, meter);
  if (lattice.status == LatticeResult::Status::Stopped)
    return stopped();
  if (lattice.status == LatticeResult::Status::Conflict)
    return refuteEqualities(lattice.conflict);
  solution = std::move(lattice.solution);

  std::vector<AffineRow> reduced;
  reduced.reserve(system.inequalities().size());
  for (std::size_t i = 0; i < system.inequalities().size(); ++i) {
    if (!meter.charge(std::uint64_t(numColumns) * numColumns + 1))
      return stopped();
    const LinearExpr &expr = system.inequalities()[i].expr;
    const std::vector<Int> coefficients = padded(expr);
    reduced.push_back(AffineRow{solution.freeCoefficients(coefficients),
                                solution.constantAtOrigin(coefficients,
                                                          expr.constant),
                                i});
  }

  ranNormalize = true;
  NormalizeResult normalized = normalizeRows(reduced, domain, meter);
  switch (normalized.status) {
  case NormalizeResult::Status::Stopped:
    return stopped();
  case NormalizeResult::Status::ConstantConflict:
    return refuteConstantRow(normalized.lowerOrigin);
  case NormalizeResult::Status::OppositeConflict:
    return refuteOppositeRows(normalized.lowerOrigin, normalized.upperOrigin);
  case NormalizeResult::Status::Normalized:
    break;
  }
  for (const NormalizedRow &row : normalized.rows)
    if (!row.isSingleVariable())
      return unknown(UnknownReason::RequiresSearch);
  return witnessFrom(normalized.rows);
}

} // namespace

Decision decide(const System &system, Domain domain, const WorkLimits &limits,
                CancellationToken *cancellation) {
  detail::noteExecution(detail::Stage::Decide);
  WorkMeter meter(limits, cancellation);
  Query query(system, domain, meter);
  Decision decision = query.run();
  DecisionMaker::seal(decision, system, meter.steps(), query.ranLattice,
                      query.ranNormalize);
  return decision;
}

} // namespace systolic::exact
