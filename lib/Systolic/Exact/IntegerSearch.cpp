//===- IntegerSearch.cpp - Exact integer feasibility by cases -------------===//

#include "IntegerSearch.h"
#include "ExecutionCounters.h"
#include "Systolic/Exact/Lattice.h"
#include "Systolic/Exact/Normalize.h"
#include "Systolic/Exact/Session.h"

namespace systolic::exact::detail {

namespace {

/// coefficients . (x, q) + constant, over the variables and the quotients.
struct ColumnRow {
  std::vector<Int> coefficients;
  Int constant{0};
};

/// How a row follows from the input. These form a small graph that is written
/// out as a proof only for the leaf that needs it. The `step` of a term here is
/// the index of another derivation.
struct Derivation {
  ProofStep::Rule rule = ProofStep::Rule::Input;
  RowRef row;                   // Input
  std::vector<ProofTerm> terms; // Combine
  std::size_t source = 0;       // Tighten, Harden
};

struct KnownRow {
  ColumnRow row;
  std::size_t derivation = 0;
};

/// row == 0, with a derivation for each sign of it as a row >= 0.
struct KnownEquality {
  ColumnRow row;
  std::size_t positive = 0;
  std::size_t negative = 0;
};

Int commonDenominator(const std::vector<Rational> &values) {
  Int common(1);
  for (const Rational &value : values)
    common = lcmOf(common, value.denominator());
  return common;
}

/// d . y for an integer d and a rational y.
Rational formAt(const std::vector<Int> &direction,
                const std::vector<Rational> &y) {
  Rational total;
  for (std::size_t j = 0; j < y.size(); ++j)
    if (direction[j] != Int(0))
      total = total + Rational(direction[j]) * y[j];
  return total;
}

/// The nearest integer point, with halves going up.
std::vector<Rational> rounded(const std::vector<Rational> &y) {
  const Rational half = *Rational::of(Int(1), Int(2));
  std::vector<Rational> result;
  result.reserve(y.size());
  for (const Rational &value : y)
    result.push_back(Rational((value + half).floor()));
  return result;
}

/// Whether a point keeps every normalized row.
bool keepsRows(const std::vector<NormalizedRow> &rows,
               const std::vector<Rational> &y) {
  for (const NormalizedRow &row : rows) {
    const Rational value = formAt(row.direction, y);
    if (row.lower && value < row.lower->value)
      return false;
    if (row.upper && value > row.upper->value)
      return false;
  }
  return true;
}

Int oneNorm(const std::vector<Int> &direction) {
  Int total(0);
  for (const Int &c : direction)
    total += magnitude(c);
  return total;
}

/// What a value holds, in the units of WorkLimits::maxSearchCells: one for
/// itself, and one for each 32 bytes of digits.
std::uint64_t cellsOf(const Int &value) { return 1 + bitLength(value) / 256; }

std::uint64_t cellsOf(const std::vector<Int> &values) {
  std::uint64_t total = 0;
  for (const Int &value : values)
    total += cellsOf(value);
  return total;
}

/// A two-sided row with no more than this between its ends is thin enough that
/// splitting on it beats anything else: at worst it is a few pinned cases.
constexpr std::int64_t thinRange = 8;

class Search {
public:
  Search(const System &system, const WorkLimits &limits,
         CancellationToken *cancellation)
      : system(system), limits(limits), cancellation(cancellation),
        meter(limits, cancellation),
        columns(system.numVariables() + system.congruences().size()) {}

  SearchOutcome run();

private:
  enum class NodeResult { Feasible, Leaf, Split, Stopped };

  /// A split with a case still open.
  struct OpenSplit {
    std::size_t node = 0; // in the proof
    bool lowFirst = true;
    bool secondCase = false;
  };

  /// What a node knows once its equalities are solved.
  struct Reduction {
    std::vector<const KnownRow *> rows;
    std::vector<KnownEquality> equalities;
    EqualitySolution solution;
    std::vector<NormalizedRow> normalized;
  };

  /// The row a case assumes. A walk along one form leaves a row per step, and
  /// only the last of them does anything, so a node reads that one alone. The
  /// others stay here because a row's place in the path is its index in a proof.
  struct CaseRow {
    KnownRow known;
    /// A later row of the same form is at least as tight.
    bool superseded = false;
    /// The row this one superseded, to restore when it goes.
    std::size_t shadows = SIZE_MAX;
    /// What it reserved.
    std::uint64_t cells = 0;
  };

  /// Which normalized row, and which side of it, a row of a relaxation is.
  struct RelaxedRow {
    std::size_t normalized = 0;
    bool lower = false;
  };

  bool reserve(std::uint64_t cells);
  void release(std::uint64_t cells) { held -= cells; }

  std::size_t addInput(RowRef row);
  std::size_t addCombine(std::vector<ProofTerm> terms);
  std::size_t addTighten(std::size_t source);
  std::size_t addHarden(std::size_t source);
  ColumnRow inputRow(const RowRef &ref) const;
  bool loadInputs();

  std::vector<ProofTerm> equalityTerms(const std::vector<KnownEquality> &eqs,
                                       const std::vector<Int> &multipliers);
  std::optional<KnownRow> deriveReduced(const Reduction &c, std::size_t row);
  std::optional<KnownRow> tightened(const Reduction &c, std::size_t row);

  NodeResult closeLeaf(std::size_t last, BranchNode &node);
  NodeResult refuteEqualities(const Reduction &c,
                              const EqualityConflict &conflict,
                              BranchNode &node);
  NodeResult refuteOpposite(const Reduction &c, std::size_t low,
                            std::size_t high, BranchNode &node);
  NodeResult refuteRelaxation(const Reduction &c, const Decision &answer,
                              const std::vector<RelaxedRow> &owners,
                              BranchNode &node);

  std::optional<NodeResult> reduce(Reduction &c, BranchNode &node);
  Decision relax(const System &rows);
  bool boundedRows(const std::vector<NormalizedRow> &rows, unsigned numFree,
                   std::vector<char> &bounded);
  EqualitySolution spanOfBounded(const Reduction &c,
                                 const std::vector<char> &bounded);
  std::vector<Int> coordinateForm(const Reduction &c, unsigned coordinate);
  bool takePoint(const Reduction &c, const std::vector<Rational> &y);
  NodeResult split(const Reduction &c, std::size_t row,
                   const std::vector<Rational> &y, BranchNode &node,
                   bool &lowFirst);
  NodeResult splitCoordinate(const Reduction &c, unsigned coordinate,
                             const Rational &value, BranchNode &node,
                             bool &lowFirst);
  NodeResult evaluateNode(BranchNode &node, bool &lowFirst);

  bool pushCase(const BranchNode &node, bool lowCase);
  void popCase();

  const System &system;
  const WorkLimits &limits;
  CancellationToken *cancellation;
  WorkMeter meter;
  const std::uint64_t columns;

  std::vector<Derivation> arena;
  std::vector<KnownRow> inputs;
  std::vector<KnownEquality> rootEqualities;
  /// The rows the open cases assume, by depth.
  std::vector<CaseRow> path;

  std::uint64_t held = 0;
  std::uint64_t inputCells = 0;
  std::uint64_t pivots = 0;
  std::uint64_t nodes = 0;
  bool ranTableau = false;
  IntegerWitness witness;
  BranchProof proof;
};

/// Applied before the thing it counts is allocated.
bool Search::reserve(std::uint64_t cells) {
  if (cells > limits.maxSearchCells - held) {
    meter.stop(UnknownReason::MemoryLimit);
    return false;
  }
  held += cells;
  return true;
}

std::size_t Search::addInput(RowRef row) {
  Derivation derivation;
  derivation.row = row;
  arena.push_back(std::move(derivation));
  return arena.size() - 1;
}

std::size_t Search::addCombine(std::vector<ProofTerm> terms) {
  Derivation derivation;
  derivation.rule = ProofStep::Rule::Combine;
  derivation.terms = std::move(terms);
  arena.push_back(std::move(derivation));
  return arena.size() - 1;
}

std::size_t Search::addTighten(std::size_t source) {
  Derivation derivation;
  derivation.rule = ProofStep::Rule::Tighten;
  derivation.source = source;
  arena.push_back(std::move(derivation));
  return arena.size() - 1;
}

std::size_t Search::addHarden(std::size_t source) {
  Derivation derivation;
  derivation.rule = ProofStep::Rule::Harden;
  derivation.source = source;
  arena.push_back(std::move(derivation));
  return arena.size() - 1;
}

ColumnRow Search::inputRow(const RowRef &ref) const {
  const LinearExpr *expr = nullptr;
  ColumnRow row;
  row.coefficients.assign(columns, Int(0));
  switch (ref.kind) {
  case RowRef::Kind::Inequality:
    expr = &system.inequalities()[ref.index].expr;
    break;
  case RowRef::Kind::StrictInequality:
    expr = &system.strictInequalities()[ref.index].expr;
    break;
  case RowRef::Kind::Equality:
    expr = &system.equalities()[ref.index].expr;
    break;
  case RowRef::Kind::Congruence:
    expr = &system.congruences()[ref.index].expr;
    row.coefficients[system.numVariables() + ref.index] =
        -system.congruences()[ref.index].modulus;
    break;
  case RowRef::Kind::Assumption:
    // Never asked for: the rows of a path are built by pushCase().
    return row;
  }
  for (unsigned j = 0; j < system.numVariables(); ++j)
    row.coefficients[j] = expr->coefficients[j];
  row.constant = expr->constant;
  return row;
}

/// The input as rows over the columns. A strict row is hardened here, once.
bool Search::loadInputs() {
  const std::uint64_t perRow = columns + 1;
  if (!meter.charge(perRow * system.numRows() + 1))
    return false;

  auto admitted = [&](const ColumnRow &row) {
    if (!reserve(cellsOf(row.coefficients) + cellsOf(row.constant)) ||
        !meter.admit(row.constant))
      return false;
    for (const Int &c : row.coefficients)
      if (!meter.admit(c))
        return false;
    return true;
  };

  for (std::size_t i = 0; i < system.inequalities().size(); ++i) {
    const RowRef ref{RowRef::Kind::Inequality, i, false};
    ColumnRow row = inputRow(ref);
    if (!admitted(row))
      return false;
    inputs.push_back(KnownRow{std::move(row), addInput(ref)});
  }
  for (std::size_t i = 0; i < system.strictInequalities().size(); ++i) {
    const RowRef ref{RowRef::Kind::StrictInequality, i, false};
    ColumnRow row = inputRow(ref);
    if (!admitted(row))
      return false;
    row.constant -= Int(1);
    inputs.push_back(KnownRow{std::move(row), addHarden(addInput(ref))});
  }
  for (std::size_t i = 0; i < system.equalities().size(); ++i) {
    const RowRef ref{RowRef::Kind::Equality, i, false};
    ColumnRow row = inputRow(ref);
    if (!admitted(row))
      return false;
    const std::size_t positive = addInput(ref);
    const std::size_t negative =
        addInput(RowRef{RowRef::Kind::Equality, i, true});
    rootEqualities.push_back(KnownEquality{std::move(row), positive, negative});
  }
  for (std::size_t i = 0; i < system.congruences().size(); ++i) {
    const RowRef ref{RowRef::Kind::Congruence, i, false};
    ColumnRow row = inputRow(ref);
    if (!admitted(row))
      return false;
    const std::size_t positive = addInput(ref);
    const std::size_t negative =
        addInput(RowRef{RowRef::Kind::Congruence, i, true});
    rootEqualities.push_back(KnownEquality{std::move(row), positive, negative});
  }
  inputCells = held;
  return true;
}

/// Terms for sum m_i (equality i), with the sign of each taken by choice.
std::vector<ProofTerm>
Search::equalityTerms(const std::vector<KnownEquality> &eqs,
                      const std::vector<Int> &multipliers) {
  std::vector<ProofTerm> terms;
  for (std::size_t i = 0; i < multipliers.size(); ++i) {
    if (multipliers[i] == Int(0))
      continue;
    const bool negated = multipliers[i] < Int(0);
    terms.push_back(ProofTerm{negated ? eqs[i].negative : eqs[i].positive,
                              magnitude(multipliers[i])});
  }
  return terms;
}

/// A multiple of a row minus equalities, so that it reads the free coordinates
/// alone. std::nullopt when the meter stopped it.
///
/// The row is summed here from rows already known. Reading it back through the
/// derivations would be a walk of a graph as a tree: a pinned equality reads the
/// ones before it, so that walk doubles with each round of pinning.
std::optional<KnownRow> Search::deriveReduced(const Reduction &c,
                                              std::size_t index) {
  const KnownRow &row = *c.rows[index];
  const std::vector<Rational> weights =
      c.solution.multipliers(row.row.coefficients);
  const Int scale = commonDenominator(weights);
  if (!meter.admit(scale) ||
      !meter.charge((weights.size() + columns + 1) * columns))
    return std::nullopt;
  std::vector<Int> multipliers;
  multipliers.reserve(weights.size());
  for (const Rational &weight : weights) {
    multipliers.push_back(-(weight * Rational(scale)).numerator());
    if (!meter.admit(multipliers.back()))
      return std::nullopt;
  }
  std::vector<ProofTerm> terms = equalityTerms(c.equalities, multipliers);
  if (terms.empty() && scale == Int(1))
    return row;

  ColumnRow total;
  total.coefficients.reserve(columns);
  for (const Int &value : row.row.coefficients)
    total.coefficients.push_back(scale * value);
  total.constant = scale * row.row.constant;
  for (std::size_t i = 0; i < multipliers.size(); ++i) {
    const ColumnRow &equality = c.equalities[i].row;
    for (std::uint64_t j = 0; j < columns; ++j)
      total.coefficients[j] += multipliers[i] * equality.coefficients[j];
    total.constant += multipliers[i] * equality.constant;
  }
  terms.insert(terms.begin(), ProofTerm{row.derivation, scale});
  return KnownRow{std::move(total), addCombine(std::move(terms))};
}

std::optional<KnownRow> Search::tightened(const Reduction &c,
                                          std::size_t index) {
  const std::optional<KnownRow> reduced = deriveReduced(c, index);
  if (!reduced)
    return std::nullopt;
  const Int g = contentOf(reduced->row.coefficients);
  if (g == Int(0)) {
    // Only a row with a direction over y gets here, and that has a variable.
    meter.stop(UnknownReason::RequiresSearch);
    return std::nullopt;
  }
  ColumnRow row;
  row.coefficients.reserve(columns);
  for (const Int &value : reduced->row.coefficients) {
    row.coefficients.push_back(*divFloor(value, g));
    if (!meter.admit(row.coefficients.back()))
      return std::nullopt;
  }
  row.constant = *divFloor(reduced->row.constant, g);
  return KnownRow{std::move(row), addTighten(reduced->derivation)};
}

/// Writes out the derivations that `last` needs, sources first.
Search::NodeResult Search::closeLeaf(std::size_t last, BranchNode &node) {
  if (!meter.charge(arena.size() + 1))
    return NodeResult::Stopped;
  std::vector<std::size_t> stepOf(arena.size(), SIZE_MAX);
  std::vector<std::size_t> stack{last};
  InfeasibilityProof leaf;
  std::uint64_t cells = 0;
  while (!stack.empty()) {
    const std::size_t index = stack.back();
    if (stepOf[index] != SIZE_MAX) {
      stack.pop_back();
      continue;
    }
    const Derivation &derivation = arena[index];
    bool ready = true;
    auto need = [&](std::size_t source) {
      if (stepOf[source] == SIZE_MAX) {
        stack.push_back(source);
        ready = false;
      }
    };
    if (derivation.rule == ProofStep::Rule::Combine)
      for (const ProofTerm &term : derivation.terms)
        need(term.step);
    else if (derivation.rule != ProofStep::Rule::Input)
      need(derivation.source);
    if (!ready)
      continue;

    switch (derivation.rule) {
    case ProofStep::Rule::Input:
      leaf.steps.push_back(ProofStep::input(derivation.row));
      break;
    case ProofStep::Rule::Combine: {
      std::vector<ProofTerm> terms = derivation.terms;
      for (ProofTerm &term : terms) {
        term.step = stepOf[term.step];
        cells += cellsOf(term.multiplier);
      }
      leaf.steps.push_back(ProofStep::combine(std::move(terms)));
      break;
    }
    case ProofStep::Rule::Tighten:
      leaf.steps.push_back(ProofStep::tighten(stepOf[derivation.source]));
      break;
    case ProofStep::Rule::Harden:
      leaf.steps.push_back(ProofStep::harden(stepOf[derivation.source]));
      break;
    }
    stepOf[index] = leaf.steps.size() - 1;
    stack.pop_back();
  }
  if (!reserve(cells + 2 * leaf.steps.size() + 4))
    return NodeResult::Stopped;
  node.kind = BranchNode::Kind::Leaf;
  node.leaf = std::move(leaf);
  return NodeResult::Leaf;
}

/// The two cases of the integer Farkas lemma, as in S1's decide().
Search::NodeResult Search::refuteEqualities(const Reduction &c,
                                            const EqualityConflict &conflict,
                                            BranchNode &node) {
  const Int scale = commonDenominator(conflict.multipliers);
  if (!meter.admit(scale))
    return NodeResult::Stopped;
  std::vector<Int> up, down;
  up.reserve(conflict.multipliers.size());
  down.reserve(conflict.multipliers.size());
  for (const Rational &value : conflict.multipliers) {
    up.push_back((value * Rational(scale)).numerator());
    down.push_back(-up.back());
    if (!meter.admit(up.back()))
      return NodeResult::Stopped;
  }
  if (conflict.kind == EqualityConflict::Kind::Inconsistent) {
    // The sum has no variables, so one of its two signs is a false row.
    Int constant(0);
    for (std::size_t i = 0; i < up.size(); ++i)
      constant += up[i] * c.equalities[i].row.constant;
    return closeLeaf(addCombine(equalityTerms(
                         c.equalities, constant < Int(0) ? up : down)),
                     node);
  }
  // scale * (v . x + q) and its negative, with v integral and q not. Each
  // tightens to floor(q) and floor(-q), which sum to -1.
  const std::size_t plus = addCombine(equalityTerms(c.equalities, up));
  const std::size_t minus = addCombine(equalityTerms(c.equalities, down));
  const std::size_t tightPlus = addTighten(plus);
  const std::size_t tightMinus = addTighten(minus);
  return closeLeaf(addCombine({ProofTerm{tightPlus, Int(1)},
                               ProofTerm{tightMinus, Int(1)}}),
                   node);
}

Search::NodeResult Search::refuteOpposite(const Reduction &c, std::size_t low,
                                          std::size_t high, BranchNode &node) {
  const std::optional<KnownRow> tightLow = tightened(c, low);
  const std::optional<KnownRow> tightHigh = tightened(c, high);
  if (!tightLow || !tightHigh)
    return NodeResult::Stopped;
  return closeLeaf(addCombine({ProofTerm{tightLow->derivation, Int(1)},
                               ProofTerm{tightHigh->derivation, Int(1)}}),
                   node);
}

/// The Farkas sum of the rational solver, over the tightened rows it read.
Search::NodeResult Search::refuteRelaxation(
    const Reduction &c, const Decision &answer,
    const std::vector<RelaxedRow> &owners, BranchNode &node) {
  const InfeasibilityProof &farkas = *answer.proof();
  std::vector<ProofTerm> terms;
  for (const ProofTerm &term : farkas.steps.back().terms) {
    const RelaxedRow &owner = owners[farkas.steps[term.step].row.index];
    const NormalizedRow &row = c.normalized[owner.normalized];
    const std::size_t origin =
        owner.lower ? row.lower->origin : row.upper->origin;
    const std::optional<KnownRow> derived = tightened(c, origin);
    if (!derived || !meter.admit(term.multiplier))
      return NodeResult::Stopped;
    terms.push_back(ProofTerm{derived->derivation, term.multiplier});
  }
  return closeLeaf(addCombine(std::move(terms)), node);
}

/// Steps 1 and 2 of the search. Returns a result when the node ended here.
std::optional<Search::NodeResult> Search::reduce(Reduction &c,
                                                 BranchNode &node) {
  while (true) {
    // What the steps below allocate, read before they allocate it.
    const std::uint64_t cells =
        (c.rows.size() + c.equalities.size()) * columns + columns * columns;
    if (cells > limits.maxTableauCells) {
      meter.stop(UnknownReason::TableauLimit);
      return NodeResult::Stopped;
    }
    if (!meter.charge((c.equalities.size() + 1) * columns + 1))
      return NodeResult::Stopped;

    std::vector<EqualityRow> lines;
    lines.reserve(c.equalities.size());
    for (const KnownEquality &equality : c.equalities)
      lines.push_back(
          EqualityRow{equality.row.coefficients, equality.row.constant});
    LatticeResult lattice =
        solveEqualities(columns, lines, Domain::Integer, meter);
    if (lattice.status == LatticeResult::Status::Stopped)
      return NodeResult::Stopped;
    if (lattice.status == LatticeResult::Status::Conflict)
      return refuteEqualities(c, lattice.conflict, node);
    c.solution = std::move(lattice.solution);

    std::vector<AffineRow> reduced;
    reduced.reserve(c.rows.size());
    for (std::size_t i = 0; i < c.rows.size(); ++i) {
      if (!meter.charge(columns * columns + 1))
        return NodeResult::Stopped;
      const ColumnRow &row = c.rows[i]->row;
      AffineRow affine{c.solution.freeCoefficients(row.coefficients),
                       c.solution.constantAtOrigin(row.coefficients,
                                                   row.constant),
                       i};
      reduced.push_back(std::move(affine));
    }

    NormalizeResult normalized =
        normalizeRows(reduced, Domain::Integer, meter);
    switch (normalized.status) {
    case NormalizeResult::Status::Stopped:
      return NodeResult::Stopped;
    case NormalizeResult::Status::ConstantConflict: {
      const std::optional<KnownRow> derived =
          deriveReduced(c, normalized.lowerOrigin);
      if (!derived)
        return NodeResult::Stopped;
      return closeLeaf(derived->derivation, node);
    }
    case NormalizeResult::Status::OppositeConflict:
      return refuteOpposite(c, normalized.lowerOrigin, normalized.upperOrigin,
                            node);
    case NormalizeResult::Status::Normalized:
      break;
    }
    c.normalized = std::move(normalized.rows);

    // A row that meets itself at one value is an equality in disguise.
    bool pinned = false;
    for (const NormalizedRow &row : c.normalized) {
      if (!row.lower || !row.upper || row.lower->value != row.upper->value)
        continue;
      std::optional<KnownRow> low = tightened(c, row.lower->origin);
      const std::optional<KnownRow> high = tightened(c, row.upper->origin);
      if (!low || !high)
        return NodeResult::Stopped;
      // Both read the free coordinates alone, so they are negatives of each
      // other over the columns too.
      c.equalities.push_back(KnownEquality{std::move(low->row), low->derivation,
                                           high->derivation});
      pinned = true;
    }
    if (!pinned)
      return std::nullopt;
  }
}

/// Runs the rational solver under what is left of the budget.
Decision Search::relax(const System &rows) {
  WorkLimits inner = limits;
  inner.maxSteps = limits.maxSteps - meter.steps();
  // maxRows is a limit on the caller's system. A cone query has more rows than
  // that by design, and maxTableauCells still bounds what it holds.
  inner.maxRows = UINT64_MAX;
  Decision answer = solveRational(rows, inner, cancellation);
  pivots += answer.receipt().pivots;
  ranTableau = ranTableau || answer.receipt().ranTableau;
  if (meter.charge(answer.receipt().steps) && answer.isUnknown())
    meter.stop(answer.why());
  return answer;
}

/// Sets bounded[i] for rows known to have a finite range on the polyhedron: the
/// two-sided ones, and those that vanish on the recession cone. One query asks
/// for a point of the cone strictly inside every one-sided row. If there is one,
/// none of those rows is bounded. If not, a Farkas proof names rows whose positive
/// sum is zero on the cone, so all of them vanish there, and at least one of them
/// is one-sided, because the origin keeps the rows that are not strict. It need
/// not name them all, and the search only needs one.
bool Search::boundedRows(const std::vector<NormalizedRow> &rows,
                         unsigned numFree, std::vector<char> &bounded) {
  bounded.assign(rows.size(), 0);
  std::vector<std::size_t> open;
  for (std::size_t i = 0; i < rows.size(); ++i) {
    if (rows[i].lower && rows[i].upper)
      bounded[i] = 1;
    else
      open.push_back(i);
  }
  if (open.empty())
    return true;

  auto signedForm = [](const std::vector<Int> &direction, bool lower) {
    LinearExpr expr;
    expr.coefficients.reserve(direction.size());
    for (const Int &c : direction)
      expr.coefficients.push_back(lower ? c : -c);
    return expr;
  };

  System cone = System::ofSize(numFree);
  std::vector<std::size_t> owners;
  for (std::size_t i = 0; i < rows.size(); ++i) {
    if (rows[i].lower) {
      (void)cone.addInequality(signedForm(rows[i].direction, true));
      owners.push_back(i);
    }
    if (rows[i].upper) {
      (void)cone.addInequality(signedForm(rows[i].direction, false));
      owners.push_back(i);
    }
  }
  for (std::size_t i : open)
    (void)cone.addStrictInequality(
        signedForm(rows[i].direction, rows[i].lower.has_value()));

  const Decision answer = relax(cone);
  if (meter.stopped())
    return false;
  if (answer.provedFeasible())
    return true;
  const InfeasibilityProof &farkas = *answer.proof();
  for (const ProofTerm &term : farkas.steps.back().terms) {
    const RowRef &row = farkas.steps[term.step].row;
    bounded[row.kind == RowRef::Kind::Inequality ? owners[row.index]
                                                 : open[row.index]] = 1;
  }
  return true;
}

/// The directions on which every bounded row is zero, as the free columns of
/// that solution. A form is in the span of the bounded rows exactly when it is
/// zero on all of them. Read it only if the meter has not stopped.
EqualitySolution Search::spanOfBounded(const Reduction &c,
                                       const std::vector<char> &bounded) {
  std::vector<EqualityRow> rows;
  for (std::size_t i = 0; i < c.normalized.size(); ++i)
    if (bounded[i])
      rows.push_back(EqualityRow{c.normalized[i].direction, Int(0)});
  LatticeResult lattice =
      solveEqualities(c.solution.numFree(), rows, Domain::Rational, meter);
  // Rows with no constants have the origin as a solution.
  if (lattice.status == LatticeResult::Status::Conflict)
    meter.stop(UnknownReason::RequiresSearch);
  return std::move(lattice.solution);
}

/// The point for an integral y.
bool Search::takePoint(const Reduction &c, const std::vector<Rational> &y) {
  const std::vector<Rational> x = c.solution.point(y);
  witness.values.clear();
  witness.values.reserve(system.numVariables());
  for (unsigned j = 0; j < system.numVariables(); ++j) {
    if (!x[j].isInteger()) {
      // An integral y gives an integral x. If that failed, say nothing.
      meter.stop(UnknownReason::RequiresSearch);
      return false;
    }
    witness.values.push_back(x[j].numerator());
    if (!meter.admit(witness.values.back()))
      return false;
  }
  return true;
}

/// Splits on a normalized row, at a value chosen so that both cases are smaller.
Search::NodeResult Search::split(const Reduction &c, std::size_t index,
                                 const std::vector<Rational> &y,
                                 BranchNode &node, bool &lowFirst) {
  const NormalizedRow &row = c.normalized[index];
  const Rational value = formAt(row.direction, y);
  Int at; // the cases are d . y <= at and d . y >= at + 1
  if (!value.isInteger()) {
    at = value.floor();
    lowFirst = value - Rational(at) < *Rational::of(Int(1), Int(2));
  } else if (row.lower && row.lower->value == value) {
    // At the end of the range, leave that value alone in the low case.
    at = value.numerator();
    lowFirst = true;
  } else {
    // Otherwise the high case holds it, and is at the end of its own range.
    at = value.numerator() - Int(1);
    lowFirst = false;
  }

  // The form over the columns comes from the tightened row itself: it reads
  // d . y - lower, or upper - d . y, with those exact coefficients.
  const std::size_t origin =
      row.lower ? row.lower->origin : row.upper->origin;
  std::optional<KnownRow> derived = tightened(c, origin);
  if (!derived)
    return NodeResult::Stopped;
  ColumnRow &known = derived->row;
  node.kind = BranchNode::Kind::Split;
  if (row.lower) {
    node.form = std::move(known.coefficients);
    node.bound = at - row.lower->value.numerator() - known.constant;
  } else {
    node.form.reserve(known.coefficients.size());
    for (const Int &value : known.coefficients)
      node.form.push_back(-value);
    node.bound = at - row.upper->value.numerator() + known.constant;
  }
  if (!meter.admit(node.bound))
    return NodeResult::Stopped;
  if (!reserve(cellsOf(node.form) + cellsOf(node.bound) + 3))
    return NodeResult::Stopped;
  return NodeResult::Split;
}

/// The integer form over the columns that equals free coordinate j at every
/// point. x = U z, so it is the row of the inverse of U for that coordinate, and
/// U is unimodular, so that row is integral. Solved here over the rationals. The
/// answer does not have to be trusted: a wrong form is a wrong split, which a
/// leaf below it would fail to close, and the checker reads the form as given.
std::vector<Int> Search::coordinateForm(const Reduction &c, unsigned coordinate) {
  const std::vector<std::vector<Int>> &u = c.solution.transform();
  const std::size_t size = u.size();
  // The transpose of U, with the unit vector of the coordinate on the right.
  std::vector<std::vector<Rational>> m(size,
                                       std::vector<Rational>(size + 1));
  for (std::size_t r = 0; r < size; ++r)
    for (std::size_t j = 0; j < size; ++j)
      m[r][j] = Rational(u[j][r]);
  m[size - c.solution.numFree() + coordinate][size] = Rational(Int(1));
  for (std::size_t k = 0; k < size; ++k) {
    std::size_t pivot = k;
    while (m[pivot][k].sign() == 0)
      ++pivot;
    std::swap(m[k], m[pivot]);
    for (std::size_t r = 0; r < size; ++r) {
      if (r == k || m[r][k].sign() == 0)
        continue;
      const Rational factor = *m[r][k].dividedBy(m[k][k]);
      for (std::size_t j = k; j <= size; ++j)
        m[r][j] = m[r][j] - factor * m[k][j];
    }
  }
  std::vector<Int> form;
  form.reserve(size);
  for (std::size_t r = 0; r < size; ++r)
    form.push_back(m[r][size].dividedBy(m[r][r])->numerator());
  return form;
}

/// Splits on a free coordinate at a fraction.
Search::NodeResult Search::splitCoordinate(const Reduction &c, unsigned coordinate,
                                           const Rational &value,
                                           BranchNode &node, bool &lowFirst) {
  if (!meter.charge(columns * columns * columns + 1))
    return NodeResult::Stopped;
  node.kind = BranchNode::Kind::Split;
  node.form = coordinateForm(c, coordinate);
  node.bound = value.floor();
  lowFirst = value - Rational(node.bound) < *Rational::of(Int(1), Int(2));
  for (const Int &entry : node.form)
    if (!meter.admit(entry))
      return NodeResult::Stopped;
  if (!reserve(cellsOf(node.form) + cellsOf(node.bound) + 3))
    return NodeResult::Stopped;
  return NodeResult::Split;
}

Search::NodeResult Search::evaluateNode(BranchNode &node, bool &lowFirst) {
  Reduction c;
  c.rows.reserve(inputs.size() + path.size());
  for (const KnownRow &row : inputs)
    c.rows.push_back(&row);
  for (const CaseRow &row : path)
    if (!row.superseded)
      c.rows.push_back(&row.known);
  c.equalities = rootEqualities;

  if (const std::optional<NodeResult> ended = reduce(c, node))
    return *ended;

  // The rational relaxation, over the free coordinates.
  const unsigned numFree = c.solution.numFree();
  auto sideOf = [](const std::vector<Int> &direction, bool lower,
                   const Int &constant) {
    LinearExpr expr;
    expr.coefficients.reserve(direction.size());
    for (const Int &value : direction)
      expr.coefficients.push_back(lower ? value : -value);
    expr.constant = constant;
    return expr;
  };
  System relaxation = System::ofSize(numFree);
  std::vector<RelaxedRow> owners;
  for (std::size_t i = 0; i < c.normalized.size(); ++i) {
    const NormalizedRow &row = c.normalized[i];
    if (row.lower) {
      (void)relaxation.addInequality(
          sideOf(row.direction, true, -row.lower->value.numerator()));
      owners.push_back(RelaxedRow{i, true});
    }
    if (row.upper) {
      (void)relaxation.addInequality(
          sideOf(row.direction, false, row.upper->value.numerator()));
      owners.push_back(RelaxedRow{i, false});
    }
  }
  const Decision answer = relax(relaxation);
  if (meter.stopped())
    return NodeResult::Stopped;
  if (answer.provedInfeasible())
    return refuteRelaxation(c, answer, owners, node);
  const std::vector<Rational> &y = answer.rationalWitness()->values;

  // The reads of every row below, which hold no budget of their own.
  if (!meter.charge(c.normalized.size() * numFree + 1))
    return NodeResult::Stopped;
  // An integral point rounds to itself.
  const std::vector<Rational> near = rounded(y);
  if (keepsRows(c.normalized, near))
    return takePoint(c, near) ? NodeResult::Feasible : NodeResult::Stopped;

  // A row with little room is a flat direction, whatever its coefficients. In a
  // skewed basis a coordinate can have a huge range across the same set.
  for (std::size_t i = 0; i < c.normalized.size(); ++i) {
    const NormalizedRow &row = c.normalized[i];
    if (row.lower && row.upper &&
        row.upper->value.numerator() - row.lower->value.numerator() <=
            Int(thinRange) &&
        !formAt(row.direction, y).isInteger())
      return split(c, i, y, node, lowFirst);
  }

  std::vector<char> bounded;
  if (!boundedRows(c.normalized, numFree, bounded))
    return NodeResult::Stopped;

  // A coordinate of the lattice, which is a form of norm 1 there, so a step in
  // it is a real step.
  const EqualitySolution span = spanOfBounded(c, bounded);
  if (meter.stopped())
    return NodeResult::Stopped;
  const std::vector<std::vector<Int>> &directions = span.transform();
  for (unsigned j = 0; j < numFree; ++j) {
    if (y[j].isInteger())
      continue;
    bool inSpan = true;
    for (unsigned k = numFree - span.numFree(); k < numFree; ++k)
      inSpan = inSpan && directions[j][k] == Int(0);
    if (inSpan)
      return splitCoordinate(c, j, y[j], node, lowFirst);
  }

  for (std::size_t i = 0; i < c.normalized.size(); ++i)
    if (bounded[i])
      return split(c, i, y, node, lowFirst);

  // No row is bounded, so every row can move in by half its norm.
  // Each row here has one side, and is doubled to keep the half an integer.
  System deflated = System::ofSize(numFree);
  for (const NormalizedRow &row : c.normalized) {
    const bool lower = row.lower.has_value();
    const Int bound =
        lower ? -row.lower->value.numerator() : row.upper->value.numerator();
    std::vector<Int> doubled;
    doubled.reserve(row.direction.size());
    for (const Int &value : row.direction)
      doubled.push_back(Int(2) * value);
    (void)deflated.addInequality(
        sideOf(doubled, lower, Int(2) * bound - oneNorm(row.direction)));
  }
  const Decision deep = relax(deflated);
  if (meter.stopped())
    return NodeResult::Stopped;
  if (deep.provedFeasible()) {
    const std::vector<Rational> point = rounded(deep.rationalWitness()->values);
    if (keepsRows(c.normalized, point))
      return takePoint(c, point) ? NodeResult::Feasible : NodeResult::Stopped;
  }
  // The argument in IntegerSearch.h says this cannot happen. If it ever did, it
  // must not be read as an answer.
  meter.stop(UnknownReason::RequiresSearch);
  return NodeResult::Stopped;
}

/// The row a case of a split assumes, as the checker reads it.
bool Search::pushCase(const BranchNode &node, bool lowCase) {
  ColumnRow row;
  row.coefficients.reserve(columns);
  for (const Int &c : node.form)
    row.coefficients.push_back(lowCase ? -c : c);
  row.constant = lowCase ? node.bound : -node.bound - Int(1);
  const std::uint64_t cells =
      cellsOf(row.coefficients) + cellsOf(row.constant) + 3;
  if (!reserve(cells))
    return false;
  std::size_t shadows = SIZE_MAX;
  for (std::size_t i = path.size(); i-- > 0;) {
    CaseRow &older = path[i];
    if (!older.superseded && older.known.row.constant >= row.constant &&
        older.known.row.coefficients == row.coefficients) {
      older.superseded = true;
      shadows = i;
      break;
    }
  }
  const std::size_t derivation =
      addInput(RowRef{RowRef::Kind::Assumption, path.size(), false});
  path.push_back(
      CaseRow{KnownRow{std::move(row), derivation}, false, shadows, cells});
  return true;
}

void Search::popCase() {
  if (path.back().shadows != SIZE_MAX)
    path[path.back().shadows].superseded = false;
  release(path.back().cells);
  path.pop_back();
  arena.pop_back();
}

SearchOutcome Search::run() {
  SearchOutcome outcome;
  std::vector<OpenSplit> open;
  bool finished = !loadInputs();
  std::size_t current = 0;
  if (!finished)
    proof.nodes.emplace_back();

  while (!finished) {
    if (nodes >= limits.maxBranchNodes) {
      meter.stop(UnknownReason::NodeLimit);
      break;
    }
    if (!meter.charge(path.size() + 1))
      break;
    ++nodes;
    noteExecution(Stage::BranchNode);

    BranchNode node;
    bool lowFirst = true;
    const std::size_t mark = arena.size();
    const NodeResult result = evaluateNode(node, lowFirst);
    arena.resize(mark);

    if (result == NodeResult::Stopped)
      break;
    if (result == NodeResult::Feasible) {
      outcome.kind = SearchOutcome::Kind::Feasible;
      outcome.witness = std::move(witness);
      finished = true;
      continue;
    }

    bool lowCase = lowFirst;
    if (result == NodeResult::Split) {
      proof.nodes[current] = std::move(node);
      open.push_back(OpenSplit{current, lowFirst, false});
    } else {
      proof.nodes[current] = std::move(node);
      // Back up to the nearest split with a case left.
      while (!open.empty() && open.back().secondCase) {
        popCase();
        open.pop_back();
      }
      if (open.empty()) {
        outcome.kind = SearchOutcome::Kind::Infeasible;
        outcome.proof = std::move(proof);
        outcome.proofCells = held - inputCells;
        finished = true;
        continue;
      }
      popCase();
      open.back().secondCase = true;
      lowCase = !open.back().lowFirst;
    }

    const std::size_t child = proof.nodes.size();
    proof.nodes.emplace_back();
    BranchNode &parent = proof.nodes[open.back().node];
    (lowCase ? parent.low : parent.high) = child;
    if (!pushCase(parent, lowCase))
      break;
    current = child;
  }

  if (meter.stopped())
    outcome.kind = SearchOutcome::Kind::Stopped;
  outcome.reason = meter.reason();
  outcome.steps = meter.steps();
  outcome.pivots = pivots;
  outcome.nodes = nodes;
  outcome.ranTableau = ranTableau;
  return outcome;
}

} // namespace

SearchOutcome searchIntegerPoint(const System &system, const WorkLimits &limits,
                                 CancellationToken *cancellation) {
  return Search(system, limits, cancellation).run();
}

} // namespace systolic::exact::detail
