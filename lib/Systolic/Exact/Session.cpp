//===- Session.cpp - Incremental rational feasibility ---------------------===//

#include "Systolic/Exact/Session.h"
#include "DecisionGate.h"
#include "ExecutionCounters.h"

namespace systolic::exact {

namespace {
enum class Status { Unchecked, Feasible, Infeasible };
enum class RowOutcome { Applied, Conflict, Stopped };
} // namespace

struct Session::State {
  explicit State(System rows)
      : system(std::move(rows)), tableau(system.numVariables()) {}

  const LinearExpr &exprOf(const RowRef &ref) const;
  void noteRow(const RowRef &ref);
  std::uint64_t size() const;
  RowOutcome apply(const RowRef &ref, WorkMeter &meter,
                   std::vector<FarkasTerm> &terms);

  System system;
  BoundsTableau tableau;
  /// Rows not yet in the tableau, oldest first.
  std::vector<RowRef> pending;
  Status status = Status::Unchecked;
  std::optional<RationalWitness> witness;  // when Feasible
  std::optional<InfeasibilityProof> proof; // when Infeasible
};

const LinearExpr &Session::State::exprOf(const RowRef &ref) const {
  switch (ref.kind) {
  case RowRef::Kind::Inequality:
    return system.inequalities()[ref.index].expr;
  case RowRef::Kind::StrictInequality:
    return system.strictInequalities()[ref.index].expr;
  case RowRef::Kind::Equality:
    return system.equalities()[ref.index].expr;
  case RowRef::Kind::Congruence:
    break;
  }
  return system.congruences()[ref.index].expr;
}

void Session::State::noteRow(const RowRef &ref) {
  pending.push_back(ref);
  // Rows only add constraints, so a proof stays valid. A witness may not.
  if (status == Status::Feasible) {
    status = Status::Unchecked;
    witness.reset();
  }
}

/// Tableau cells, and each variable and each row of the system as a row of
/// about that many cells more, for its form, its bounds and its own overhead.
/// Calibrated by measurement: one unit held about 31 bytes at 1000 and at 4000
/// nested scopes.
std::uint64_t Session::State::size() const {
  const std::uint64_t width = system.numVariables();
  return tableau.rows().size() * width +
         (tableau.numVariables() + system.numRows()) * (width + 8);
}

/// A row with no variables is true or false by its constant.
static RowOutcome applyConstant(RowRef ref, const Int &constant,
                                std::vector<FarkasTerm> &terms) {
  bool holds = false;
  switch (ref.kind) {
  case RowRef::Kind::Inequality:
    holds = constant >= Int(0);
    break;
  case RowRef::Kind::StrictInequality:
    holds = constant > Int(0);
    break;
  case RowRef::Kind::Equality:
    holds = constant == Int(0);
    ref.negated = constant > Int(0);
    break;
  case RowRef::Kind::Congruence:
    break;
  }
  if (holds)
    return RowOutcome::Applied;
  terms.assign(1, FarkasTerm{ref, Rational(Int(1))});
  return RowOutcome::Conflict;
}

/// Writes the row as divisor * (form . x - zero), for a primitive form with a
/// positive first entry, and asserts it as a bound on the variable for form.
RowOutcome Session::State::apply(const RowRef &ref, WorkMeter &meter,
                                 std::vector<FarkasTerm> &terms) {
  const LinearExpr &expr = exprOf(ref);
  if (!meter.charge(expr.coefficients.size() + 1) ||
      !meter.admit(expr.constant))
    return RowOutcome::Stopped;
  for (const Int &c : expr.coefficients)
    if (!meter.admit(c))
      return RowOutcome::Stopped;

  const Int content = contentOf(expr.coefficients);
  if (content == Int(0))
    return applyConstant(ref, expr.constant, terms);

  bool flipped = false;
  for (const Int &c : expr.coefficients) {
    if (c != Int(0)) {
      flipped = c < Int(0);
      break;
    }
  }
  const Int divisor = flipped ? -content : content;
  std::vector<Int> form;
  form.reserve(expr.coefficients.size());
  for (const Int &c : expr.coefficients)
    form.push_back(c / divisor);
  const std::optional<unsigned> variable =
      tableau.variableForForm(form, meter);
  if (!variable)
    return RowOutcome::Stopped;

  const Rational zero = *Rational::of(-expr.constant, divisor);
  const Rational scale = *Rational::of(Int(1), content);

  auto finish = [&](BoundsTableau::AssertOutcome outcome) {
    if (outcome == BoundsTableau::AssertOutcome::Conflict) {
      terms = tableau.conflict();
      return RowOutcome::Conflict;
    }
    return outcome == BoundsTableau::AssertOutcome::Stopped
               ? RowOutcome::Stopped
               : RowOutcome::Applied;
  };

  if (ref.kind == RowRef::Kind::Equality) {
    RowRef forLower = ref, forUpper = ref;
    forLower.negated = flipped;
    forUpper.negated = !flipped;
    const RowOutcome first = finish(tableau.assertLower(
        *variable, Bound{DeltaRational(zero), forLower, scale}, meter));
    if (first != RowOutcome::Applied)
      return first;
    return finish(tableau.assertUpper(
        *variable, Bound{DeltaRational(zero), forUpper, scale}, meter));
  }

  // A strict row is the same bound, moved by delta into the open side.
  const bool strict = ref.kind == RowRef::Kind::StrictInequality;
  if (flipped) {
    const Rational shift = strict ? Rational(Int(-1)) : Rational();
    return finish(tableau.assertUpper(
        *variable, Bound{DeltaRational(zero, shift), ref, scale}, meter));
  }
  const Rational shift = strict ? Rational(Int(1)) : Rational();
  return finish(tableau.assertLower(
      *variable, Bound{DeltaRational(zero, shift), ref, scale}, meter));
}

/// Input steps for the rows, and one sum with integer multipliers. A conflict
/// reads each variable's bound once and a row bounds one variable, so no row
/// repeats, apart from an equality with both signs, which are two rows here.
static InfeasibilityProof proofFrom(const std::vector<FarkasTerm> &terms) {
  Int common(1);
  for (const FarkasTerm &term : terms)
    common = lcmOf(common, term.multiplier.denominator());

  InfeasibilityProof proof;
  std::vector<ProofTerm> sum;
  for (const FarkasTerm &term : terms) {
    sum.push_back(ProofTerm{proof.steps.size(),
                            (term.multiplier * Rational(common)).numerator()});
    proof.steps.push_back(ProofStep::input(term.row));
  }
  proof.steps.push_back(ProofStep::combine(std::move(sum)));
  return proof;
}

Session::Session(unsigned numVariables)
    : Session(System::ofSize(numVariables)) {}

Session::Session(System system)
    : current(std::make_shared<State>(std::move(system))) {
  const System &rows = current->system;
  for (std::size_t i = 0; i < rows.inequalities().size(); ++i)
    current->pending.push_back(RowRef{RowRef::Kind::Inequality, i, false});
  for (std::size_t i = 0; i < rows.strictInequalities().size(); ++i)
    current->pending.push_back(
        RowRef{RowRef::Kind::StrictInequality, i, false});
  for (std::size_t i = 0; i < rows.equalities().size(); ++i)
    current->pending.push_back(RowRef{RowRef::Kind::Equality, i, false});
}

Session::~Session() = default;

Session::State &Session::writable() {
  if (current.use_count() > 1)
    current = std::make_shared<State>(*current);
  return *current;
}

RowError Session::addInequality(LinearExpr expr) {
  if (expr.coefficients.size() != current->system.numVariables())
    return RowError::WrongCoefficientCount;
  State &state = writable();
  const RowError error = state.system.addInequality(std::move(expr));
  state.noteRow(RowRef{RowRef::Kind::Inequality,
                       state.system.inequalities().size() - 1, false});
  return error;
}

RowError Session::addStrictInequality(LinearExpr expr) {
  if (expr.coefficients.size() != current->system.numVariables())
    return RowError::WrongCoefficientCount;
  State &state = writable();
  const RowError error = state.system.addStrictInequality(std::move(expr));
  state.noteRow(RowRef{RowRef::Kind::StrictInequality,
                       state.system.strictInequalities().size() - 1, false});
  return error;
}

RowError Session::addEquality(LinearExpr expr) {
  if (expr.coefficients.size() != current->system.numVariables())
    return RowError::WrongCoefficientCount;
  State &state = writable();
  const RowError error = state.system.addEquality(std::move(expr));
  state.noteRow(RowRef{RowRef::Kind::Equality,
                       state.system.equalities().size() - 1, false});
  return error;
}

void Session::push() { scopes.push_back(current); }

bool Session::pop() {
  if (scopes.empty())
    return false;
  current = std::move(scopes.back());
  scopes.pop_back();
  return true;
}

std::uint64_t Session::heldCells() const {
  std::uint64_t total = current->size();
  const State *previous = current.get();
  for (auto scope = scopes.rbegin(); scope != scopes.rend(); ++scope) {
    if (scope->get() != previous)
      total += (*scope)->size();
    previous = scope->get();
  }
  return total;
}

const System &Session::system() const { return current->system; }

const BoundsTableau &Session::tableau() const { return current->tableau; }

Decision Session::check(const WorkLimits &limits,
                        CancellationToken *cancellation) {
  detail::noteExecution(detail::Stage::RationalCheck);
  detail::TableauFacts facts;

  // An earlier answer still needs the checker, and no work.
  if (current->status == Status::Feasible)
    return detail::gateWitness(current->system, *current->witness, facts);
  if (current->status == Status::Infeasible)
    return detail::gateProof(current->system, *current->proof, facts);

  const System &rows = current->system;
  if (!rows.congruences().empty())
    return detail::gateUnknown(rows, UnknownReason::UnsupportedConstruct, facts);
  if (rows.numVariables() > limits.maxVariables)
    return detail::gateUnknown(rows, UnknownReason::VariableLimit, facts);
  if (rows.numRows() > limits.maxRows)
    return detail::gateUnknown(rows, UnknownReason::RowLimit, facts);
  // What is held now, and the copy that is about to be made.
  if (heldCells() + current->size() > limits.maxSessionCells)
    return detail::gateUnknown(rows, UnknownReason::TableauLimit, facts);

  WorkMeter meter(limits, cancellation);
  facts.ranTableau = true;
  // Everything below happens to a copy, which is kept only at the end.
  auto work = std::make_shared<State>(*current);
  const std::uint64_t pivotsBefore = work->tableau.numPivots();
  auto stopped = [&] {
    facts.steps = meter.steps();
    facts.pivots = work->tableau.numPivots() - pivotsBefore;
    return detail::gateUnknown(rows, meter.reason(), facts);
  };

  std::vector<FarkasTerm> terms;
  bool conflict = false;
  std::size_t applied = 0;
  for (; applied < work->pending.size() && !conflict; ++applied) {
    const RowOutcome outcome = work->apply(work->pending[applied], meter, terms);
    if (outcome == RowOutcome::Stopped)
      return stopped();
    conflict = outcome == RowOutcome::Conflict;
  }
  work->pending.erase(work->pending.begin(), work->pending.begin() + applied);

  bool searching = !conflict;
  while (searching) {
    switch (work->tableau.step(meter)) {
    case BoundsTableau::StepOutcome::Pivoted:
      break;
    case BoundsTableau::StepOutcome::Stopped:
      return stopped();
    case BoundsTableau::StepOutcome::Conflict:
      terms = work->tableau.conflict();
      conflict = true;
      searching = false;
      break;
    case BoundsTableau::StepOutcome::Feasible:
      searching = false;
      break;
    }
  }

  if (conflict) {
    if (!meter.charge(terms.size() + 1))
      return stopped();
    InfeasibilityProof proof = proofFrom(terms);
    facts.steps = meter.steps();
    facts.pivots = work->tableau.numPivots() - pivotsBefore;
    Decision decision = detail::gateProof(work->system, proof, facts);
    if (decision.provedInfeasible()) {
      work->status = Status::Infeasible;
      work->proof = std::move(proof);
      current = std::move(work);
    }
    return decision;
  }

  std::optional<std::vector<Rational>> point =
      work->tableau.concreteValues(meter);
  if (!point)
    return stopped();
  RationalWitness witness{std::move(*point)};
  facts.steps = meter.steps();
  facts.pivots = work->tableau.numPivots() - pivotsBefore;
  Decision decision = detail::gateWitness(work->system, witness, facts);
  if (decision.provedFeasible()) {
    work->status = Status::Feasible;
    work->witness = std::move(witness);
    current = std::move(work);
  }
  return decision;
}

std::string Session::stateText() const {
  const State &state = *current;
  std::string text = canonicalText(state.system);
  text += "pending";
  for (const RowRef &ref : state.pending) {
    text += ' ';
    text += std::to_string(static_cast<int>(ref.kind));
    text += ':';
    text += std::to_string(ref.index);
  }
  text += "\nstatus ";
  text += state.status == Status::Unchecked  ? "unchecked"
          : state.status == Status::Feasible ? "feasible"
                                             : "infeasible";
  text += '\n';
  if (state.witness) {
    text += "witness";
    for (const Rational &value : state.witness->values) {
      text += ' ';
      text += value.toString();
    }
    text += '\n';
  }
  if (state.proof)
    text += canonicalText(*state.proof);
  text += state.tableau.stateText();
  return text;
}

Decision solveRational(const System &system, const WorkLimits &limits,
                       CancellationToken *cancellation) {
  return Session(system).check(limits, cancellation);
}

} // namespace systolic::exact
