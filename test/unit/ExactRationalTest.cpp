//===- ExactRationalTest.cpp - Tests of the rational solver ---------------===//
//
// The rational solver of Session.h, against expectations from outside it:
//   - systems worked by hand, and by theorem;
//   - an exact vertex enumeration, in machine integers, for bounded systems;
//   - Fourier-Motzkin on Python integers, through --rational-vectors;
//   - this file's own validators of every witness and every proof;
//   - the tableau invariants and Bland's rule, recomputed from the read-only
//     view before every step and compared with what the step did.
//
// Nothing here links an LLVM solver, so nothing else can be what answers.
//
//===----------------------------------------------------------------------===//

#include "Systolic/Exact/Budget.h"
#include "Systolic/Exact/Checker.h"
#include "Systolic/Exact/Decide.h"
#include "Systolic/Exact/DeltaRational.h"
#include "Systolic/Exact/Provenance.h"
#include "Systolic/Exact/Session.h"
#include "Systolic/Exact/Tableau.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <numeric>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

using namespace systolic::exact;

// A definite answer has no way around these.
static_assert(!std::is_convertible_v<Decision, bool>);
static_assert(!std::is_constructible_v<bool, Decision>);
static_assert(!std::is_copy_constructible_v<Session>);

namespace {

unsigned long passed = 0, failed = 0;

void expect(bool ok, const char *what) {
  if (ok) {
    ++passed;
    return;
  }
  std::cout << "FAILED: " << what << "\n";
  // A run this broken has said what it can, and each more failure may cost a
  // whole step budget.
  if (++failed == 40)
    std::exit(1);
}

Rational q(std::int64_t numerator, std::int64_t denominator = 1) {
  return *Rational::of(Int(numerator), Int(denominator));
}

Int big(const char *text) { return *parseDecimal(text); }

/// Far above what any system here needs. It turns a defect that loops forever
/// into a failed expectation instead of a run that never ends.
WorkLimits bounded() {
  WorkLimits limits;
  limits.maxSteps = std::uint64_t(1) << 24;
  limits.maxCoefficientBits = 1 << 12;
  return limits;
}

Decision solved(const System &system) { return solveRational(system, bounded()); }

struct Raw {
  std::vector<std::int64_t> a;
  std::int64_t c = 0;
};

struct RawSystem {
  unsigned n = 0;
  std::vector<Raw> ge, gt, eq;
};

LinearExpr toExpr(const Raw &row) {
  LinearExpr expr;
  for (std::int64_t c : row.a)
    expr.coefficients.push_back(Int(c));
  expr.constant = Int(row.c);
  return expr;
}

System toSystem(const RawSystem &raw) {
  System system = System::ofSize(raw.n);
  for (const Raw &row : raw.ge)
    expect(system.addInequality(toExpr(row)) == RowError::None, "add ge");
  for (const Raw &row : raw.gt)
    expect(system.addStrictInequality(toExpr(row)) == RowError::None, "add gt");
  for (const Raw &row : raw.eq)
    expect(system.addEquality(toExpr(row)) == RowError::None, "add eq");
  return system;
}

//===----------------------------------------------------------------------===//
// This file's own validators
//===----------------------------------------------------------------------===//

Rational valueAt(const LinearExpr &expr, const std::vector<Rational> &x) {
  Rational total(expr.constant);
  for (std::size_t j = 0; j < x.size(); ++j)
    total = total + Rational(expr.coefficients[j]) * x[j];
  return total;
}

bool pointIsMember(const System &system, const std::vector<Rational> &x) {
  if (x.size() != system.numVariables())
    return false;
  for (const Inequality &row : system.inequalities())
    if (valueAt(row.expr, x).sign() < 0)
      return false;
  for (const StrictInequality &row : system.strictInequalities())
    if (valueAt(row.expr, x).sign() <= 0)
      return false;
  for (const Equality &row : system.equalities())
    if (valueAt(row.expr, x).sign() != 0)
      return false;
  return true;
}

struct Derived {
  std::vector<Rational> a;
  Rational c;
  bool strict = false;
};

/// The row a reference names, or nothing when the reference is not allowed.
bool lookUp(const System &system, const RowRef &ref, Derived &out) {
  const LinearExpr *expr = nullptr;
  switch (ref.kind) {
  case RowRef::Kind::Inequality:
    if (ref.negated || ref.index >= system.inequalities().size())
      return false;
    expr = &system.inequalities()[ref.index].expr;
    break;
  case RowRef::Kind::StrictInequality:
    if (ref.negated || ref.index >= system.strictInequalities().size())
      return false;
    expr = &system.strictInequalities()[ref.index].expr;
    out.strict = true;
    break;
  case RowRef::Kind::Equality:
    if (ref.index >= system.equalities().size())
      return false;
    expr = &system.equalities()[ref.index].expr;
    break;
  case RowRef::Kind::Congruence:
    return false;
  }
  const Rational sign = ref.negated ? q(-1) : q(1);
  for (const Int &c : expr->coefficients)
    out.a.push_back(Rational(c) * sign);
  out.c = Rational(expr->constant) * sign;
  return true;
}

/// Whether a proof is a valid derivation of a false row over the rationals.
/// Written apart from the checker, in rationals, as a reference for it.
bool proofIsValid(const System &system, const InfeasibilityProof &proof) {
  if (proof.steps.empty())
    return false;
  std::vector<Derived> rows;
  for (const ProofStep &step : proof.steps) {
    Derived row;
    if (step.rule == ProofStep::Rule::Tighten)
      return false;
    if (step.rule == ProofStep::Rule::Input) {
      if (!lookUp(system, step.row, row))
        return false;
    } else {
      if (step.terms.empty())
        return false;
      row.a.assign(system.numVariables(), Rational());
      for (const ProofTerm &term : step.terms) {
        if (term.step >= rows.size() || term.multiplier <= Int(0))
          return false;
        const Derived &read = rows[term.step];
        const Rational m(term.multiplier);
        for (std::size_t j = 0; j < row.a.size(); ++j)
          row.a[j] = row.a[j] + m * read.a[j];
        row.c = row.c + m * read.c;
        row.strict = row.strict || read.strict;
      }
    }
    rows.push_back(std::move(row));
  }
  const Derived &last = rows.back();
  for (const Rational &value : last.a)
    if (value.sign() != 0)
      return false;
  return last.c.sign() < 0 || (last.c.sign() == 0 && last.strict);
}

/// Every definite answer, rechecked here. Returns whether it was definite.
bool certified(const System &system, const Decision &decision,
               const char *what) {
  expect(decision.domain() == Domain::Rational, what);
  if (decision.provedFeasible()) {
    expect(decision.rationalWitness() &&
               pointIsMember(system, decision.rationalWitness()->values),
           what);
    expect(!decision.proof(), what);
    return true;
  }
  if (decision.provedInfeasible()) {
    expect(decision.proof() && proofIsValid(system, *decision.proof()), what);
    expect(!decision.rationalWitness(), what);
    return true;
  }
  expect(!decision.proof() && !decision.rationalWitness(), what);
  return false;
}

//===----------------------------------------------------------------------===//
// An exact oracle for bounded systems
//===----------------------------------------------------------------------===//

using Wide = __int128;

/// Solves the square system by fraction-free elimination. Fills `numerators`
/// and a positive `denominator`, or returns false when it is singular.
bool solveSquare(std::vector<std::vector<Wide>> m,
                 std::vector<Wide> &numerators, Wide &denominator) {
  const std::size_t n = m.size();
  Wide previous = 1;
  std::vector<std::size_t> rowOf(n);
  std::iota(rowOf.begin(), rowOf.end(), 0);
  for (std::size_t k = 0; k < n; ++k) {
    std::size_t pivot = k;
    while (pivot < n && m[pivot][k] == 0)
      ++pivot;
    if (pivot == n)
      return false;
    std::swap(m[k], m[pivot]);
    for (std::size_t i = 0; i < n; ++i) {
      if (i == k)
        continue;
      for (std::size_t j = k + 1; j <= n; ++j)
        m[i][j] = (m[i][j] * m[k][k] - m[i][k] * m[k][j]) / previous;
    }
    previous = m[k][k];
  }
  denominator = m[n - 1][n - 1];
  numerators.resize(n);
  for (std::size_t i = 0; i < n; ++i)
    numerators[i] = m[i][n];
  if (denominator < 0) {
    denominator = -denominator;
    for (Wide &value : numerators)
      value = -value;
  }
  return true;
}

/// Whether a system with a bounded closed part has a point, by brute force.
///
/// Let R be the system with every strict row made closed. R is a polytope, so
/// it is nonempty exactly when it has a vertex, and each vertex is the solution
/// of n independent tight rows. Then the strict rows can all hold at once
/// exactly when each holds strictly at some vertex: a strict row f is >= 0 on R,
/// so if f > 0 anywhere on R it is > 0 at a vertex, and the mean of one such
/// vertex per row is a point of R where every f > 0.
bool vertexOracle(const RawSystem &raw) {
  std::vector<const Raw *> rows;
  for (const Raw &row : raw.ge)
    rows.push_back(&row);
  for (const Raw &row : raw.gt)
    rows.push_back(&row);
  for (const Raw &row : raw.eq)
    rows.push_back(&row);
  const unsigned n = raw.n;
  std::vector<bool> strictHolds(raw.gt.size(), false);
  bool found = false;

  auto consider = [&](const std::vector<Wide> &x, Wide den) {
    auto value = [&](const Raw &row) {
      Wide total = row.c * den;
      for (unsigned j = 0; j < n; ++j)
        total += row.a[j] * x[j];
      return total;
    };
    for (const Raw &row : raw.ge)
      if (value(row) < 0)
        return;
    for (const Raw &row : raw.gt)
      if (value(row) < 0)
        return;
    for (const Raw &row : raw.eq)
      if (value(row) != 0)
        return;
    found = true;
    for (std::size_t i = 0; i < raw.gt.size(); ++i)
      if (value(raw.gt[i]) > 0)
        strictHolds[i] = true;
  };

  if (n == 0) {
    consider({}, 1);
  } else if (rows.size() >= n) {
    std::vector<std::size_t> pick(n);
    std::iota(pick.begin(), pick.end(), 0);
    while (true) {
      std::vector<std::vector<Wide>> m(n, std::vector<Wide>(n + 1));
      for (unsigned i = 0; i < n; ++i) {
        for (unsigned j = 0; j < n; ++j)
          m[i][j] = rows[pick[i]]->a[j];
        m[i][n] = -rows[pick[i]]->c;
      }
      std::vector<Wide> x;
      Wide den = 1;
      if (solveSquare(std::move(m), x, den))
        consider(x, den);
      int at = n - 1;
      while (at >= 0 && pick[at] == rows.size() - n + at)
        --at;
      if (at < 0)
        break;
      ++pick[at];
      for (unsigned j = at + 1; j < n; ++j)
        pick[j] = pick[j - 1] + 1;
    }
  }
  if (!found)
    return false;
  return std::all_of(strictHolds.begin(), strictHolds.end(),
                     [](bool held) { return held; });
}

/// Adds -bound <= x_j <= bound for every variable.
RawSystem boxed(RawSystem raw, std::int64_t bound) {
  for (unsigned j = 0; j < raw.n; ++j) {
    Raw low, high;
    low.a.assign(raw.n, 0);
    high.a.assign(raw.n, 0);
    low.a[j] = 1;
    high.a[j] = -1;
    low.c = high.c = bound;
    raw.ge.push_back(low);
    raw.ge.push_back(high);
  }
  return raw;
}

//===----------------------------------------------------------------------===//
// The tableau from outside
//===----------------------------------------------------------------------===//

/// What Bland's rule picks, from the read-only view alone.
struct Choice {
  bool violated = false;
  unsigned leaving = 0;
  bool hasEntering = false;
  unsigned entering = 0;
};

Choice blandChoice(const BoundsTableau &t) {
  Choice choice;
  std::size_t row = 0;
  bool needsIncrease = false;
  const auto &basic = t.basicVariables();
  for (std::size_t r = 0; r < basic.size(); ++r) {
    const unsigned v = basic[r];
    const bool below = t.lower(v) && t.value(v) < t.lower(v)->value;
    const bool above = t.upper(v) && t.value(v) > t.upper(v)->value;
    if ((below || above) && (!choice.violated || v < choice.leaving)) {
      choice.violated = true;
      choice.leaving = v;
      row = r;
      needsIncrease = below;
    }
  }
  if (!choice.violated)
    return choice;
  const auto &nonbasic = t.nonbasicVariables();
  for (std::size_t c = 0; c < nonbasic.size(); ++c) {
    const int sign = t.rows()[row][c].sign();
    if (sign == 0)
      continue;
    const unsigned u = nonbasic[c];
    const bool upward = needsIncrease ? sign > 0 : sign < 0;
    const bool able = upward
                          ? (!t.upper(u) || t.value(u) < t.upper(u)->value)
                          : (!t.lower(u) || t.value(u) > t.lower(u)->value);
    if (able && (!choice.hasEntering || u < choice.entering)) {
      choice.hasEntering = true;
      choice.entering = u;
    }
  }
  return choice;
}

/// I1 to I4 of Tableau.h, recomputed from the definitions.
void expectInvariants(const BoundsTableau &t) {
  const unsigned n = t.numStructural();
  const auto &basic = t.basicVariables();
  const auto &nonbasic = t.nonbasicVariables();
  expect(basic.size() + nonbasic.size() == t.numVariables(),
         "every variable has one place");
  std::vector<unsigned> everyone(basic);
  everyone.insert(everyone.end(), nonbasic.begin(), nonbasic.end());
  std::sort(everyone.begin(), everyone.end());
  for (unsigned v = 0; v < everyone.size(); ++v)
    expect(everyone[v] == v, "each variable appears once");

  // I1: a row, with the form of each variable put in, is an identity.
  for (std::size_t r = 0; r < basic.size(); ++r) {
    std::vector<Rational> total(n);
    for (std::size_t c = 0; c < nonbasic.size(); ++c) {
      const Rational &a = t.rows()[r][c];
      if (a.sign() == 0)
        continue;
      const std::vector<Int> &form = t.form(nonbasic[c]);
      for (unsigned j = 0; j < n; ++j)
        total[j] = total[j] + a * Rational(form[j]);
    }
    const std::vector<Int> &form = t.form(basic[r]);
    bool same = true;
    for (unsigned j = 0; j < n; ++j)
      same = same && total[j] == Rational(form[j]);
    expect(same, "I1: a row is an identity over the structural variables");
  }

  // I2: every value is its form at the structural values.
  for (unsigned v = 0; v < t.numVariables(); ++v) {
    DeltaRational total;
    const std::vector<Int> &form = t.form(v);
    for (unsigned j = 0; j < n; ++j)
      total = total + t.value(j) * Rational(form[j]);
    expect(total == t.value(v), "I2: the assignment satisfies the definitions");
  }

  // I3: a nonbasic variable is within its bounds.
  for (unsigned v : nonbasic) {
    expect(!t.lower(v) || t.lower(v)->value <= t.value(v),
           "I3: a nonbasic variable is above its lower bound");
    expect(!t.upper(v) || t.value(v) <= t.upper(v)->value,
           "I3: a nonbasic variable is below its upper bound");
  }

  // I4: the two sides never cross.
  for (unsigned v = 0; v < t.numVariables(); ++v)
    if (t.lower(v) && t.upper(v))
      expect(t.lower(v)->value <= t.upper(v)->value, "I4: bounds do not cross");
}

/// Whether a combination of input rows is a false row, by summing the rows.
bool conflictIsFalse(const System &system, const std::vector<FarkasTerm> &terms) {
  if (terms.empty())
    return false;
  std::vector<Rational> a(system.numVariables());
  Rational c;
  bool strict = false;
  for (const FarkasTerm &term : terms) {
    if (term.multiplier.sign() <= 0)
      return false;
    Derived row;
    if (!lookUp(system, term.row, row))
      return false;
    for (std::size_t j = 0; j < a.size(); ++j)
      a[j] = a[j] + term.multiplier * row.a[j];
    c = c + term.multiplier * row.c;
    strict = strict || row.strict;
  }
  for (const Rational &value : a)
    if (value.sign() != 0)
      return false;
  return c.sign() < 0 || (c.sign() == 0 && strict);
}

/// One row as bounds, the way Session.h documents it, written again here.
BoundsTableau::AssertOutcome loadRow(BoundsTableau &t, const LinearExpr &expr,
                                     RowRef ref, WorkMeter &meter,
                                     std::vector<FarkasTerm> &terms) {
  using Outcome = BoundsTableau::AssertOutcome;
  const Int g = contentOf(expr.coefficients);
  if (g == Int(0)) {
    const bool holds = ref.kind == RowRef::Kind::Inequality
                           ? expr.constant >= Int(0)
                       : ref.kind == RowRef::Kind::StrictInequality
                           ? expr.constant > Int(0)
                           : expr.constant == Int(0);
    if (holds)
      return Outcome::Applied;
    ref.negated = ref.kind == RowRef::Kind::Equality && expr.constant > Int(0);
    terms.assign(1, FarkasTerm{ref, q(1)});
    return Outcome::Conflict;
  }
  bool negative = false;
  for (const Int &c : expr.coefficients) {
    if (c != Int(0)) {
      negative = c < Int(0);
      break;
    }
  }
  const Int d = negative ? -g : g;
  std::vector<Int> form;
  for (const Int &c : expr.coefficients)
    form.push_back(c / d);
  const unsigned v = *t.variableForForm(form, meter);
  const Rational at = *Rational::of(-expr.constant, d);
  const Rational scale = *Rational::of(Int(1), g);
  auto done = [&](Outcome outcome) {
    if (outcome == Outcome::Conflict)
      terms = t.conflict();
    return outcome;
  };
  if (ref.kind == RowRef::Kind::Equality) {
    RowRef low = ref, high = ref;
    low.negated = negative;
    high.negated = !negative;
    const Outcome first =
        done(t.assertLower(v, Bound{DeltaRational(at), low, scale}, meter));
    if (first != Outcome::Applied)
      return first;
    return done(t.assertUpper(v, Bound{DeltaRational(at), high, scale}, meter));
  }
  const bool strict = ref.kind == RowRef::Kind::StrictInequality;
  if (negative)
    return done(t.assertUpper(
        v, Bound{DeltaRational(at, strict ? q(-1) : q(0)), ref, scale}, meter));
  return done(t.assertLower(
      v, Bound{DeltaRational(at, strict ? q(1) : q(0)), ref, scale}, meter));
}

std::string stateWithoutCounter(const BoundsTableau &t) {
  const std::string text = t.stateText();
  return text.substr(text.find('\n'));
}

unsigned long pivotsDriven = 0;

/// Loads the rows and runs step() by hand, checking each invariant and each
/// choice of Bland's rule, and that no state ever comes back. Returns whether
/// the system is feasible.
bool drive(const System &system) {
  BoundsTableau t(system.numVariables());
  WorkMeter meter{bounded()};
  std::vector<FarkasTerm> terms;
  auto load = [&](const LinearExpr &expr, RowRef ref) {
    const auto outcome = loadRow(t, expr, ref, meter, terms);
    expectInvariants(t);
    return outcome == BoundsTableau::AssertOutcome::Conflict;
  };
  bool conflict = false;
  for (std::size_t i = 0; !conflict && i < system.inequalities().size(); ++i)
    conflict = load(system.inequalities()[i].expr,
                    RowRef{RowRef::Kind::Inequality, i, false});
  for (std::size_t i = 0;
       !conflict && i < system.strictInequalities().size(); ++i)
    conflict = load(system.strictInequalities()[i].expr,
                    RowRef{RowRef::Kind::StrictInequality, i, false});
  for (std::size_t i = 0; !conflict && i < system.equalities().size(); ++i)
    conflict = load(system.equalities()[i].expr,
                    RowRef{RowRef::Kind::Equality, i, false});
  if (conflict) {
    expect(conflictIsFalse(system, terms), "a conflict on assert is false");
    return false;
  }

  std::set<std::string> seen;
  while (true) {
    const bool fresh = seen.insert(stateWithoutCounter(t)).second;
    expect(fresh, "no state of the search ever repeats");
    if (!fresh)
      return false;
    const Choice choice = blandChoice(t);
    const auto outcome = t.step(meter);
    expectInvariants(t);
    if (outcome == BoundsTableau::StepOutcome::Pivoted) {
      ++pivotsDriven;
      expect(choice.violated && choice.hasEntering,
             "a pivot needs a repair to make");
      expect(t.lastPivot() &&
                 *t.lastPivot() ==
                     std::make_pair(choice.leaving, choice.entering),
             "the pivot is the one Bland's rule picks");
      continue;
    }
    if (outcome == BoundsTableau::StepOutcome::Feasible) {
      expect(!choice.violated, "feasible means nothing is out of bounds");
      const auto values = t.concreteValues(meter);
      expect(values && pointIsMember(system, *values),
             "the concrete point satisfies every original row");
      return true;
    }
    expect(outcome == BoundsTableau::StepOutcome::Conflict, "no limit is hit");
    if (outcome != BoundsTableau::StepOutcome::Conflict)
      return false;
    expect(choice.violated && !choice.hasEntering,
           "a conflict has no repair left");
    expect(conflictIsFalse(system, t.conflict()),
           "the combination of a conflict is false");
    return false;
  }
}

//===----------------------------------------------------------------------===//
// Delta values, strict rows and the checker
//===----------------------------------------------------------------------===//

void testDelta() {
  using D = DeltaRational;
  expect(D(q(1), q(0)) < D(q(1), q(1)), "the delta part breaks a tie");
  expect(D(q(1), q(-1)) < D(q(1), q(0)), "a negative delta is below");
  expect(D(q(1), q(1000)) < D(q(2), q(-1000)), "the real part comes first");
  expect(!(D(q(1), q(1)) < D(q(1), q(1))), "the order is strict");
  expect(D(q(1), q(2)) <= D(q(1), q(2)), "and reflexive when weak");
  expect(D(q(1), q(2)) >= D(q(1), q(2)), "both ways");
  expect(D(q(2), q(0)) > D(q(1), q(9)), "greater");
  expect(D(q(1, 2), q(1, 3)) == D(q(2, 4), q(2, 6)), "canonical");
  expect(D(q(1, 2), q(1, 3)) != D(q(1, 2), q(1, 4)), "different");
  expect(D(q(1, 2), q(1)) + D(q(1, 3), q(2)) == D(q(5, 6), q(3)), "addition");
  expect(D(q(1, 2), q(1)) - D(q(1, 3), q(2)) == D(q(1, 6), q(-1)), "subtraction");
  expect(D(q(1, 2), q(3)) * q(-2, 3) == D(q(-1, 3), q(-2)), "scaling");
  expect(D(q(1), q(2)).at(q(1, 4)) == q(3, 2), "a concrete value");
  expect(D(q(1, 2)).toString() == "1/2", "text without a delta");
  expect(D(q(1, 2), q(-3)).toString() == "1/2+-3*d", "text with a delta");
  expect(D().real().sign() == 0 && D().delta().sign() == 0, "zero");
}

void testStrictRows() {
  System system = System::ofSize(1);
  expect(system.addStrictInequality(linearExpr({1, 2}, 0)) ==
             RowError::WrongCoefficientCount,
         "a strict row of the wrong size is refused");
  expect(system.numRows() == 0, "and leaves nothing behind");
  expect(system.addStrictInequality(linearExpr({1}, -2)) == RowError::None,
         "a strict row");
  expect(system.numRows() == 1 && system.strictInequalities().size() == 1,
         "counted");
  expect(canonicalText(system) == "exact-system 1\nvariables 1 p\ngt 1 ; -2\n",
         "and written");

  // A system with no strict row reads as it did before they existed.
  System plain = System::ofSize(1);
  expect(plain.addInequality(linearExpr({1}, -2)) == RowError::None, "ge");
  expect(canonicalText(plain) == "exact-system 1\nvariables 1 p\nge 1 ; -2\n",
         "no strict row, no change in the text");

  // decide() does not read strict rows, and must not act as if they held.
  for (Domain domain : {Domain::Integer, Domain::Rational}) {
    const Decision answer = decide(system, domain);
    expect(answer.isUnknown() &&
               answer.why() == UnknownReason::UnsupportedConstruct,
           "decide refuses a strict row in either domain");
  }
  System impossible = System::ofSize(1);
  expect(impossible.addStrictInequality(linearExpr({0}, -1)) == RowError::None,
         "a false strict row");
  expect(decide(impossible, Domain::Integer).isUnknown(),
         "decide does not call it feasible by skipping it");

  // Witnesses, in both domains.
  const System above = toSystem({1, {}, {{{1}, 0}}, {}});
  expect(checkWitness(above, IntegerWitness{{Int(0)}}).why ==
             "the witness violates a strict inequality",
         "the boundary of a strict row is outside it");
  expect(checkWitness(above, IntegerWitness{{Int(1)}}).accepted, "1 > 0");
  expect(checkWitness(above, RationalWitness{{q(0)}}).why ==
             "the witness violates a strict inequality",
         "the boundary, rationally");
  expect(checkWitness(above, RationalWitness{{q(-1, 3)}}).why ==
             "the witness violates a strict inequality",
         "below it");
  expect(checkWitness(above, RationalWitness{{q(1, 1000)}}).accepted,
         "a small positive value");

  // Proofs. x > 0 and -x >= 0 sum to 0 > 0.
  auto sumOf = [](RowRef a, RowRef b) {
    InfeasibilityProof proof;
    proof.steps.push_back(ProofStep::input(a));
    proof.steps.push_back(ProofStep::input(b));
    proof.steps.push_back(ProofStep::combine({{0, Int(1)}, {1, Int(1)}}));
    return proof;
  };
  const RowRef ge0{RowRef::Kind::Inequality, 0, false};
  const RowRef gt0{RowRef::Kind::StrictInequality, 0, false};
  const System touching = toSystem({1, {{{-1}, 0}}, {{{1}, 0}}, {}});
  for (Domain domain : {Domain::Rational, Domain::Integer})
    expect(checkInfeasibility(touching, domain, sumOf(ge0, gt0)).accepted,
           "0 > 0 is false, in either domain");
  const System closed = toSystem({1, {{{-1}, 0}, {{1}, 0}}, {}, {}});
  expect(checkInfeasibility(closed, Domain::Rational,
                            sumOf(ge0, {RowRef::Kind::Inequality, 1, false}))
                 .why == "the last step is not a false row",
         "0 >= 0 is not false");
  // A strict row with a positive sum is not false: x > 0 and -x + 1 >= 0.
  const System open = toSystem({1, {{{-1}, 1}}, {{{1}, 0}}, {}});
  expect(checkInfeasibility(open, Domain::Rational, sumOf(ge0, gt0)).why ==
             "the last step is not a false row",
         "1 > 0 is not false");
  expect(checkInfeasibility(touching, Domain::Rational,
                            sumOf(ge0, {RowRef::Kind::StrictInequality, 0, true}))
                 .why == "an input step names no usable row",
         "a strict row cannot be negated");
  expect(checkInfeasibility(touching, Domain::Rational,
                            sumOf(ge0, {RowRef::Kind::StrictInequality, 7, false}))
                 .why == "an input step names no usable row",
         "nor named out of range");

  // Strictness survives a chain of sums.
  InfeasibilityProof chain;
  chain.steps.push_back(ProofStep::input(gt0));
  chain.steps.push_back(ProofStep::combine({{0, Int(2)}}));
  chain.steps.push_back(ProofStep::input(ge0));
  chain.steps.push_back(ProofStep::combine({{1, Int(1)}, {2, Int(2)}}));
  expect(checkInfeasibility(touching, Domain::Rational, chain).accepted,
         "strict through a sum of a sum");
  expect(proofIsValid(touching, chain), "and by this file's reference");

  // Tightening is for closed rows.
  InfeasibilityProof tightened;
  tightened.steps.push_back(ProofStep::input(gt0));
  tightened.steps.push_back(ProofStep::tighten(0));
  expect(checkInfeasibility(touching, Domain::Integer, tightened).why ==
             "a tighten step reads a strict row",
         "a strict row is not tightened");

  // The text of a proof names the kind.
  expect(canonicalText(sumOf(ge0, gt0)).find("input gt 0 pos") !=
             std::string::npos,
         "the proof text says gt");
}

//===----------------------------------------------------------------------===//
// Systems worked by hand
//===----------------------------------------------------------------------===//

enum class Want { Feasible, Infeasible };

void expectAnswer(const RawSystem &raw, Want want, const char *what) {
  const System system = toSystem(raw);
  const Decision answer = solved(system);
  expect(want == Want::Feasible ? answer.provedFeasible()
                                : answer.provedInfeasible(),
         what);
  certified(system, answer, what);
  expect(drive(system) == (want == Want::Feasible), what);
}

void testHandSystems() {
  const Want F = Want::Feasible, I = Want::Infeasible;
  // No rows, and no variables.
  expectAnswer({0, {}, {}, {}}, F, "nothing at all");
  expectAnswer({3, {}, {}, {}}, F, "three free variables");

  // Rows with zero coefficients, which are true or false by their constants.
  expectAnswer({0, {{{}, 0}}, {}, {}}, F, "0 >= 0 with no variables");
  expectAnswer({0, {{{}, -1}}, {}, {}}, I, "-1 >= 0 with no variables");
  expectAnswer({1, {{{0}, 1}}, {}, {}}, F, "1 >= 0");
  expectAnswer({1, {{{0}, 0}}, {}, {}}, F, "0 >= 0");
  expectAnswer({1, {{{0}, -1}}, {}, {}}, I, "-1 >= 0");
  expectAnswer({1, {}, {{{0}, 1}}, {}}, F, "1 > 0");
  expectAnswer({1, {}, {{{0}, 0}}, {}}, I, "0 > 0");
  expectAnswer({1, {}, {{{0}, -1}}, {}}, I, "-1 > 0");
  expectAnswer({1, {}, {}, {{{0}, 0}}}, F, "0 == 0");
  expectAnswer({1, {}, {}, {{{0}, 1}}}, I, "1 == 0");
  expectAnswer({1, {}, {}, {{{0}, -1}}}, I, "-1 == 0");

  // One variable, closed and open.
  expectAnswer({1, {{{1}, -1}, {{-1}, 0}}, {}, {}}, I, "x >= 1 and x <= 0");
  expectAnswer({1, {{{1}, 0}, {{-1}, 0}}, {}, {}}, F, "x >= 0 and x <= 0");
  expectAnswer({1, {{{-1}, 0}}, {{{1}, 0}}, {}}, I, "x > 0 and x <= 0");
  expectAnswer({1, {{{1}, 0}}, {{{-1}, 0}}, {}}, I, "x >= 0 and x < 0");
  expectAnswer({1, {}, {{{1}, 0}, {{-1}, 0}}, {}}, I, "x > 0 and x < 0");
  expectAnswer({1, {}, {{{1}, 0}, {{-1}, 1}}, {}}, F, "x > 0 and x < 1");
  expectAnswer({1, {}, {{{2}, -1}, {{-3}, 2}}, {}}, F, "1/2 < x < 2/3");
  expectAnswer({1, {}, {{{3}, -2}, {{-2}, 1}}, {}}, I, "2/3 < x < 1/2");
  expectAnswer({1, {{{2}, -1}, {{-2}, 1}}, {}, {}}, F, "x = 1/2 from both sides");

  // Equalities. Rational, so 2x = 1 is fine, where the integers say no.
  expectAnswer({1, {}, {}, {{{2}, -1}}}, F, "2x = 1");
  expectAnswer({2, {}, {}, {{{2, 4}, -3}}}, F, "2x + 4y = 3");
  expectAnswer({2, {}, {}, {{{1, 1}, -1}, {{1, 1}, -2}}}, I,
               "x + y = 1 and x + y = 2");
  expectAnswer({2, {}, {}, {{{1, 1}, -1}, {{2, 2}, -2}}}, F,
               "the same equality, doubled");
  expectAnswer({2, {}, {}, {{{1, 1}, -1}, {{-1, -1}, 1}}}, F,
               "the same equality, negated");
  expectAnswer({2, {}, {}, {{{1, 1}, -1}, {{-2, -2}, 3}}}, I,
               "an inconsistent multiple, negated");
  expectAnswer({2, {}, {}, {{{1, 1}, -1}, {{1, -1}, 0}, {{1, 0}, -1}}}, I,
               "three equalities with no common point");
  expectAnswer({2, {}, {}, {{{1, 1}, -1}, {{1, -1}, 0}, {{2, 0}, -1}}}, F,
               "three equalities through one point");
  expectAnswer({3, {}, {}, {{{1, 1, 0}, -1}, {{0, 1, 1}, -1}, {{1, 0, -1}, -1}}},
               I, "dependent left sides with an inconsistent constant");
  expectAnswer({3, {}, {}, {{{1, 1, 0}, -1}, {{0, 1, 1}, -1}, {{1, 0, -1}, 0}}},
               F, "dependent left sides, consistent");

  // Redundant rows change nothing.
  expectAnswer({1, {{{1}, 0}, {{1}, 1}, {{2}, 4}, {{3}, 0}}, {}, {}}, F,
               "x >= 0 stated four ways");
  expectAnswer({2, {{{1, 1}, -1}, {{2, 2}, -1}, {{3, 3}, -3}}, {}, {}}, F,
               "parallel rows, keep the tightest");
  expectAnswer({2, {{{1, 1}, -3}, {{2, 2}, -1}, {{-1, -1}, 2}}, {}, {}}, I,
               "the tightest of parallel rows decides");

  // Negative coefficients and coupled rows.
  expectAnswer({2, {{{-2, 3}, -1}, {{5, -7}, 2}}, {}, {}}, F, "two coupled rows");
  expectAnswer({3, {{{1, -1, 0}, 0}, {{0, 1, -1}, 0}, {{-1, 0, 1}, -1}}, {}, {}},
               I, "x >= y >= z >= x + 1");
  expectAnswer({3, {{{1, -1, 0}, 0}, {{0, 1, -1}, 0}, {{-1, 0, 1}, 0}}, {}, {}},
               F, "x >= y >= z >= x, so all equal");
  expectAnswer({3, {{{1, -1, 0}, 0}, {{0, 1, -1}, 0}}, {{{-1, 0, 1}, 0}}, {}}, I,
               "x >= y >= z > x");
  expectAnswer({2, {{{-1, 1}, 0}}, {{{1, -1}, 0}}, {}}, I, "x > y and y >= x");
  expectAnswer({2, {}, {{{1, -1}, 0}, {{-1, 1}, 0}}, {}}, I, "x > y and y > x");
  expectAnswer({2, {{{1, -1}, 0}, {{-1, 1}, 0}}, {}, {}}, F, "x >= y and y >= x");
  expectAnswer({2, {{{-1, -1}, 1}}, {{{1, 0}, 0}, {{0, 1}, 0}}, {}}, F,
               "the open triangle x > 0, y > 0, x + y <= 1");
  expectAnswer({2, {{{-1, -1}, 0}}, {{{1, 0}, 0}, {{0, 1}, 0}}, {}}, I,
               "x > 0, y > 0, x + y <= 0, which only the strict rows exclude");
  expectAnswer({2, {{{1, 0}, 0}, {{0, 1}, 0}, {{-1, -1}, 0}}, {}, {}}, F,
               "the same closed, which leaves the origin");

  // An equality against a strict row.
  expectAnswer({1, {}, {{{1}, 0}}, {{{1}, 0}}}, I, "x > 0 and x == 0");
  expectAnswer({2, {}, {{{1, 0}, 0}, {{0, 1}, 0}}, {{{1, 1}, 0}}}, I,
               "x > 0, y > 0, x + y == 0");
  expectAnswer({2, {}, {{{1, 0}, 0}, {{0, 1}, 0}}, {{{1, 1}, -1}}}, F,
               "x > 0, y > 0, x + y == 1");
}

/// A strict row can demand a very small delta.
void testConcreteDelta() {
  // 0 < x_1 < x_2 < ... < x_k < 10^-18.
  const unsigned k = 12;
  RawSystem chain;
  chain.n = k;
  Raw first;
  first.a.assign(k, 0);
  first.a[0] = 1;
  chain.gt.push_back(first);
  for (unsigned i = 0; i + 1 < k; ++i) {
    Raw step;
    step.a.assign(k, 0);
    step.a[i + 1] = 1;
    step.a[i] = -1;
    chain.gt.push_back(step);
  }
  Raw cap;
  cap.a.assign(k, 0);
  cap.a[k - 1] = -1000000000000000000;
  cap.c = 1;
  chain.gt.push_back(cap);
  const System system = toSystem(chain);
  const Decision answer = solved(system);
  expect(answer.provedFeasible(), "a narrow open chain is feasible");
  certified(system, answer, "and its concrete point is inside");
  expect(drive(system), "by hand too");

  // Delta on both sides of one variable, and on a slack.
  const RawSystem both = {2,
                          {},
                          {{{1, 0}, 0},
                           {{-1, 0}, 1},
                           {{1, -1}, 0},
                           {{-1, 1}, 1},
                           {{1, 1}, -1},
                           {{-1, -1}, 2}},
                          {}};
  expectAnswer(both, Want::Feasible, "open rows on variables and slacks");

  // The witness comes from a concrete delta even on a line.
  const RawSystem line = {3, {}, {{{1, 0, 0}, 0}}, {{{1, -1, 0}, 0}, {{0, 1, -1}, 0}}};
  expectAnswer(line, Want::Feasible, "strict on a line of equalities");
}

void testHugeValues() {
  const Int two200 = big("1606938044258990275541962092341162602522202993782792"
                         "835301376");
  // x >= 2^200, x <= 2^200 - 1.
  System gap = System::ofSize(1);
  expect(gap.addInequality(LinearExpr{{Int(1)}, -two200}) == RowError::None, "a");
  expect(gap.addInequality(LinearExpr{{Int(-1)}, two200 - Int(1)}) ==
             RowError::None,
         "b");
  const Decision gapAnswer = solved(gap);
  expect(gapAnswer.provedInfeasible(), "a gap of 1 at 2^200");
  certified(gap, gapAnswer, "with a proof");
  expect(!drive(gap), "by hand");

  // 2^200 x == 2^200 + 1, and the point must be exact.
  System exact = System::ofSize(1);
  expect(exact.addEquality(LinearExpr{{two200}, -(two200 + Int(1))}) ==
             RowError::None,
         "c");
  const Decision exactAnswer = solved(exact);
  expect(exactAnswer.provedFeasible(), "a huge equality");
  certified(exact, exactAnswer, "with a witness");
  expect(exactAnswer.rationalWitness() &&
             exactAnswer.rationalWitness()->values[0] ==
                 *Rational::of(two200 + Int(1), two200),
         "exactly 1 + 2^-200");

  // Huge coefficients of both signs on coupled rows, feasible by construction:
  // the rows hold at (1, -1), with a margin of 2^200 in the last row.
  System coupled = System::ofSize(2);
  expect(coupled.addInequality(LinearExpr{{two200, two200 + Int(1)}, Int(1)}) ==
             RowError::None,
         "d");
  expect(coupled.addInequality(LinearExpr{{-two200, two200}, two200 * Int(2)}) ==
             RowError::None,
         "e");
  expect(coupled.addStrictInequality(LinearExpr{{Int(-1), Int(0)}, two200}) ==
             RowError::None,
         "f");
  const Decision coupledAnswer = solved(coupled);
  expect(coupledAnswer.provedFeasible(), "huge coupled rows");
  certified(coupled, coupledAnswer, "with a witness");
  expect(drive(coupled), "by hand");

  // The same rows with the tightest one one unit past possible.
  System tooTight = System::ofSize(2);
  expect(tooTight.addEquality(LinearExpr{{two200, Int(0)}, -two200}) ==
             RowError::None,
         "g");
  expect(tooTight.addEquality(LinearExpr{{Int(0), two200}, two200}) ==
             RowError::None,
         "h");
  expect(tooTight.addStrictInequality(
             LinearExpr{{-two200, -two200}, Int(0)}) == RowError::None,
         "i");
  const Decision tightAnswer = solved(tooTight);
  expect(tightAnswer.provedInfeasible(), "x = 1, y = -1, x + y < 0");
  certified(tooTight, tightAnswer, "with a proof");

  // Scaling a row, or a variable, by a huge factor changes nothing.
  std::mt19937_64 rng(5);
  auto pick = [&](int lo, int hi) {
    return lo + int(rng() % std::uint64_t(hi - lo + 1));
  };
  const Int factors[] = {two200, big("-9223372036854775808"),
                         big("170141183460469231731687303715884105727")};
  unsigned same = 0;
  for (int trial = 0; trial < 1500; ++trial) {
    RawSystem raw;
    raw.n = pick(1, 3);
    const int rows = pick(1, 5);
    for (int r = 0; r < rows; ++r) {
      Raw row;
      for (unsigned j = 0; j < raw.n; ++j)
        row.a.push_back(pick(-4, 4));
      row.c = pick(-5, 5);
      const int kind = pick(0, 9);
      (kind < 6 ? raw.ge : kind < 8 ? raw.gt : raw.eq).push_back(row);
    }
    const System plain = toSystem(raw);
    const Decision base = solved(plain);
    certified(plain, base, "before scaling");

    System rowsScaled = System::ofSize(raw.n);
    const Int positive = magnitude(factors[trial % 3]);
    auto scale = [&](LinearExpr expr) {
      for (Int &c : expr.coefficients)
        c *= positive;
      expr.constant *= positive;
      return expr;
    };
    for (const Raw &row : raw.ge)
      expect(rowsScaled.addInequality(scale(toExpr(row))) == RowError::None, "j");
    for (const Raw &row : raw.gt)
      expect(rowsScaled.addStrictInequality(scale(toExpr(row))) ==
                 RowError::None,
             "k");
    for (const Raw &row : raw.eq)
      expect(rowsScaled.addEquality(scale(toExpr(row))) == RowError::None, "l");
    const Decision scaled = solved(rowsScaled);
    certified(rowsScaled, scaled, "after scaling the rows");
    expect(scaled.verdict() == base.verdict(), "row scale keeps the verdict");

    // x_0 = y_0 * factor is onto the rationals, for any nonzero factor.
    System variableScaled = System::ofSize(raw.n);
    const Int factor = factors[(trial + 1) % 3];
    auto stretch = [&](LinearExpr expr) {
      expr.coefficients[0] *= factor;
      return expr;
    };
    for (const Raw &row : raw.ge)
      expect(variableScaled.addInequality(stretch(toExpr(row))) ==
                 RowError::None,
             "m");
    for (const Raw &row : raw.gt)
      expect(variableScaled.addStrictInequality(stretch(toExpr(row))) ==
                 RowError::None,
             "n");
    for (const Raw &row : raw.eq)
      expect(variableScaled.addEquality(stretch(toExpr(row))) == RowError::None,
             "o");
    const Decision stretched = solved(variableScaled);
    certified(variableScaled, stretched, "after scaling a variable");
    expect(stretched.verdict() == base.verdict(),
           "variable scale keeps the verdict");
    same += stretched.verdict() == scaled.verdict();
  }
  expect(same == 1500, "all three agree");
}

//===----------------------------------------------------------------------===//
// Brute force
//===----------------------------------------------------------------------===//

/// Every system of up to three rows over a small alphabet.
void testExhaustive() {
  struct Row {
    Raw raw;
    int kind;
  };
  auto alphabet = [](unsigned n, std::vector<std::int64_t> values,
                     std::vector<std::int64_t> constants) {
    std::vector<Row> rows;
    std::vector<std::int64_t> a(n, values[0]);
    while (true) {
      for (std::int64_t c : constants)
        for (int kind = 0; kind < 3; ++kind)
          rows.push_back(Row{Raw{a, c}, kind});
      unsigned j = 0;
      for (; j < n; ++j) {
        auto at = std::find(values.begin(), values.end(), a[j]);
        if (at + 1 != values.end()) {
          a[j] = *(at + 1);
          break;
        }
        a[j] = values[0];
      }
      if (j == n)
        break;
    }
    return rows;
  };

  unsigned long systems = 0, feasible = 0, infeasible = 0;
  auto run = [&](unsigned n, const std::vector<Row> &chosen, std::int64_t box) {
    RawSystem raw;
    raw.n = n;
    for (const Row &row : chosen)
      (row.kind == 0 ? raw.ge : row.kind == 1 ? raw.gt : raw.eq)
          .push_back(row.raw);
    raw = boxed(std::move(raw), box);
    const bool truth = vertexOracle(raw);
    const System system = toSystem(raw);
    const Decision answer = solved(system);
    expect(truth ? answer.provedFeasible() : answer.provedInfeasible(),
           "the solver agrees with the vertex enumeration");
    certified(system, answer, "exhaustive");
    ++systems;
    (truth ? feasible : infeasible) += 1;
  };

  // One variable: every coefficient and constant in -2..2, up to 3 rows.
  const std::vector<Row> one = alphabet(1, {-2, -1, 0, 1, 2}, {-2, -1, 0, 1, 2});
  for (std::size_t a = 0; a < one.size(); ++a) {
    run(1, {one[a]}, 3);
    for (std::size_t b = a; b < one.size(); ++b) {
      run(1, {one[a], one[b]}, 3);
      for (std::size_t c = b; c < one.size(); ++c)
        run(1, {one[a], one[b], one[c]}, 3);
    }
  }
  // Two variables: every coefficient and constant in -1..1, up to 3 rows.
  const std::vector<Row> two = alphabet(2, {-1, 0, 1}, {-1, 0, 1});
  for (std::size_t a = 0; a < two.size(); ++a) {
    run(2, {two[a]}, 3);
    for (std::size_t b = a; b < two.size(); ++b) {
      run(2, {two[a], two[b]}, 3);
      for (std::size_t c = b; c < two.size(); ++c)
        run(2, {two[a], two[b], two[c]}, 3);
    }
  }
  // Three variables: every pair of rows.
  const std::vector<Row> three = alphabet(3, {-1, 0, 1}, {-1, 0, 1});
  for (std::size_t a = 0; a < three.size(); a += 2)
    for (std::size_t b = a; b < three.size(); b += 3)
      run(3, {three[a], three[b]}, 2);
  std::cout << "exhaustive: " << systems << " systems, " << feasible
            << " feasible, " << infeasible << " infeasible\n";
  expect(feasible > 1000 && infeasible > 1000, "both outcomes occurred");
}

/// Random bounded systems, with the whole search checked step by step.
void testRandomBounded() {
  std::mt19937_64 rng(11);
  auto pick = [&](int lo, int hi) {
    return lo + int(rng() % std::uint64_t(hi - lo + 1));
  };
  unsigned feasible = 0, infeasible = 0;
  for (int trial = 0; trial < 3500; ++trial) {
    RawSystem raw;
    raw.n = pick(2, 4);
    const int rows = pick(1, 6);
    const bool through = trial % 3 == 0;
    std::vector<int> point;
    for (unsigned j = 0; j < raw.n; ++j)
      point.push_back(pick(-2, 2));
    for (int r = 0; r < rows; ++r) {
      Raw row;
      std::int64_t atPoint = 0;
      for (unsigned j = 0; j < raw.n; ++j) {
        row.a.push_back(pick(-4, 4));
        atPoint += row.a[j] * point[j];
      }
      row.c = through ? -atPoint : pick(-6, 6);
      const int kind = pick(0, 9);
      (kind < 6 ? raw.ge : kind < 8 ? raw.gt : raw.eq).push_back(row);
    }
    raw = boxed(std::move(raw), 4);
    const bool truth = vertexOracle(raw);
    const System system = toSystem(raw);
    const Decision answer = solved(system);
    expect(truth ? answer.provedFeasible() : answer.provedInfeasible(),
           "the solver agrees with the vertex enumeration");
    certified(system, answer, "random bounded");
    expect(drive(system) == truth, "and step by step");
    (truth ? feasible : infeasible) += 1;
  }
  expect(feasible > 300 && infeasible > 300, "both outcomes occurred");
}

/// Answers that a transformation of the system must not change.
void testTransformations() {
  std::mt19937_64 rng(13);
  auto pick = [&](int lo, int hi) {
    return lo + int(rng() % std::uint64_t(hi - lo + 1));
  };
  for (int trial = 0; trial < 3000; ++trial) {
    RawSystem raw;
    raw.n = pick(2, 5);
    const int rows = pick(2, 9);
    for (int r = 0; r < rows; ++r) {
      Raw row;
      for (unsigned j = 0; j < raw.n; ++j)
        row.a.push_back(pick(-3, 3));
      row.c = pick(-4, 4);
      const int kind = pick(0, 9);
      (kind < 6 ? raw.ge : kind < 8 ? raw.gt : raw.eq).push_back(row);
    }
    const System base = toSystem(raw);
    const Decision answer = solved(base);
    expect(certified(base, answer, "base"), "a small system is decided");

    // Rows in another order.
    RawSystem shuffled = raw;
    std::shuffle(shuffled.ge.begin(), shuffled.ge.end(), rng);
    std::shuffle(shuffled.gt.begin(), shuffled.gt.end(), rng);
    std::shuffle(shuffled.eq.begin(), shuffled.eq.end(), rng);
    const System other = toSystem(shuffled);
    const Decision reordered = solved(other);
    certified(other, reordered, "reordered");
    expect(reordered.verdict() == answer.verdict(), "row order");

    // Variables in another order.
    std::vector<unsigned> order(raw.n);
    std::iota(order.begin(), order.end(), 0);
    std::shuffle(order.begin(), order.end(), rng);
    auto renamed = [&](Raw row) {
      Raw out = row;
      for (unsigned j = 0; j < raw.n; ++j)
        out.a[order[j]] = row.a[j];
      return out;
    };
    RawSystem swapped;
    swapped.n = raw.n;
    for (const Raw &row : raw.ge)
      swapped.ge.push_back(renamed(row));
    for (const Raw &row : raw.gt)
      swapped.gt.push_back(renamed(row));
    for (const Raw &row : raw.eq)
      swapped.eq.push_back(renamed(row));
    const System permuted = toSystem(swapped);
    const Decision permutedAnswer = solved(permuted);
    certified(permuted, permutedAnswer, "permuted");
    expect(permutedAnswer.verdict() == answer.verdict(), "variable order");

    // x -> -x on one variable.
    RawSystem flipped = raw;
    const unsigned at = pick(0, raw.n - 1);
    for (auto *list : {&flipped.ge, &flipped.gt, &flipped.eq})
      for (Raw &row : *list)
        row.a[at] = -row.a[at];
    const System mirror = toSystem(flipped);
    const Decision mirrored = solved(mirror);
    certified(mirror, mirrored, "mirrored");
    expect(mirrored.verdict() == answer.verdict(), "variable sign");

    // A positive sum of rows adds nothing.
    if (raw.ge.size() >= 2) {
      RawSystem more = raw;
      Raw sum;
      for (unsigned j = 0; j < raw.n; ++j)
        sum.a.push_back(2 * raw.ge[0].a[j] + 3 * raw.ge[1].a[j]);
      sum.c = 2 * raw.ge[0].c + 3 * raw.ge[1].c;
      more.ge.push_back(sum);
      const System extended = toSystem(more);
      const Decision implied = solved(extended);
      certified(extended, implied, "implied row");
      expect(implied.verdict() == answer.verdict(), "an implied row");
    }

    // x = u + v, with a new variable, is onto the rationals.
    RawSystem split;
    split.n = raw.n + 1;
    auto widen = [&](const Raw &row) {
      Raw out = row;
      out.a.push_back(row.a[0]);
      return out;
    };
    for (const Raw &row : raw.ge)
      split.ge.push_back(widen(row));
    for (const Raw &row : raw.gt)
      split.gt.push_back(widen(row));
    for (const Raw &row : raw.eq)
      split.eq.push_back(widen(row));
    const System wider = toSystem(split);
    const Decision widened = solved(wider);
    certified(wider, widened, "widened");
    expect(widened.verdict() == answer.verdict(), "a split variable");
  }
}

//===----------------------------------------------------------------------===//
// Degenerate vertices and cycling
//===----------------------------------------------------------------------===//

void testCyclingExamples() {
  // Chvatal's example, which cycles under the largest coefficient rule:
  // maximize 10a - 57b - 9c - 24d subject to
  // a/2 - 11b/2 - 5c/2 + 9d <= 0, a/2 - 3b/2 - c/2 + d <= 0, a <= 1, all >= 0.
  // Its optimum is 1, at (1, 0, 1, 0).
  auto chvatal = [](std::vector<Raw> extraGe, std::vector<Raw> extraGt) {
    RawSystem raw;
    raw.n = 4;
    raw.ge = {{{-1, 11, 5, -18}, 0}, {{-1, 3, 1, -2}, 0},  {{-1, 0, 0, 0}, 1},
              {{1, 0, 0, 0}, 0},     {{0, 1, 0, 0}, 0},    {{0, 0, 1, 0}, 0},
              {{0, 0, 0, 1}, 0}};
    raw.ge.insert(raw.ge.end(), extraGe.begin(), extraGe.end());
    raw.gt = std::move(extraGt);
    return raw;
  };
  const Raw objective{{10, -57, -9, -24}, -1};
  expectAnswer(chvatal({objective}, {}), Want::Feasible,
               "Chvatal: the optimum 1 is reached");
  expectAnswer(chvatal({}, {objective}), Want::Infeasible,
               "Chvatal: nothing is strictly better");
  expectAnswer(chvatal({Raw{{10, -57, -9, -24}, -2}}, {}), Want::Infeasible,
               "Chvatal: 2 is out of reach");

  // Beale's example, scaled to integers: minimize -3a/4 + 150b - c/50 + 6d
  // subject to a/4 - 60b - c/25 + 9d <= 0, a/2 - 90b - c/50 + 3d <= 0, c <= 1,
  // all >= 0. Its optimum is -1/20, at (1/25, 0, 1, 0).
  auto beale = [](std::vector<Raw> extraGe, std::vector<Raw> extraGt) {
    RawSystem raw;
    raw.n = 4;
    raw.ge = {{{-25, 6000, 4, -900}, 0}, {{-25, 4500, 1, -150}, 0},
              {{0, 0, -1, 0}, 1},        {{1, 0, 0, 0}, 0},
              {{0, 1, 0, 0}, 0},         {{0, 0, 1, 0}, 0},
              {{0, 0, 0, 1}, 0}};
    raw.ge.insert(raw.ge.end(), extraGe.begin(), extraGe.end());
    raw.gt = std::move(extraGt);
    return raw;
  };
  // z <= -1/20 is -75a + 15000b - 2c + 600d + 5 <= 0 after multiplying by 100.
  const Raw reach{{75, -15000, 2, -600}, -5};
  expectAnswer(beale({reach}, {}), Want::Feasible,
               "Beale: the optimum -1/20 is reached");
  expectAnswer(beale({}, {reach}), Want::Infeasible,
               "Beale: nothing is strictly better");

  // Kuhn's example, in equality form: the equalities are degenerate at 0.
  RawSystem kuhn;
  kuhn.n = 7;
  kuhn.eq = {{{1, 0, 0, -2, -9, 1, 9}, 0},
             {{0, 3, 0, 1, 3, -1, -6}, 0},
             {{0, 0, 1, 2, 3, -1, -12}, -2}};
  for (unsigned j = 0; j < 7; ++j) {
    Raw low;
    low.a.assign(7, 0);
    low.a[j] = 1;
    kuhn.ge.push_back(low);
  }
  expectAnswer(kuhn, Want::Feasible, "Kuhn: the feasible region");

  // Every row through one point, in several dimensions: only ties.
  std::mt19937_64 rng(17);
  auto pick = [&](int lo, int hi) {
    return lo + int(rng() % std::uint64_t(hi - lo + 1));
  };
  unsigned feasible = 0, infeasible = 0;
  for (int trial = 0; trial < 1200; ++trial) {
    RawSystem raw;
    raw.n = pick(3, 6);
    const int rows = pick(raw.n, 2 * raw.n + 4);
    for (int r = 0; r < rows; ++r) {
      Raw row;
      for (unsigned j = 0; j < raw.n; ++j)
        row.a.push_back(pick(-2, 2));
      raw.ge.push_back(row);
    }
    // Cones of this kind hold the origin, so ask for a point off it.
    if (trial % 2 == 0) {
      Raw off;
      for (unsigned j = 0; j < raw.n; ++j)
        off.a.push_back(pick(-2, 2));
      raw.gt.push_back(off);
    }
    const System system = toSystem(raw);
    const Decision answer = solved(system);
    expect(certified(system, answer, "degenerate cone"), "a cone is decided");
    expect(drive(system) == answer.provedFeasible(), "and step by step");
    (answer.provedFeasible() ? feasible : infeasible) += 1;
  }
  expect(feasible > 100 && infeasible > 100, "both outcomes occurred");

  // The assignment polytope is as degenerate as polytopes get.
  for (unsigned size = 2; size <= 5; ++size) {
    RawSystem assignment;
    assignment.n = size * size;
    for (unsigned i = 0; i < size; ++i) {
      Raw across, down;
      across.a.assign(assignment.n, 0);
      down.a.assign(assignment.n, 0);
      across.c = down.c = -1;
      for (unsigned j = 0; j < size; ++j) {
        across.a[i * size + j] = 1;
        down.a[j * size + i] = 1;
      }
      assignment.eq.push_back(across);
      assignment.eq.push_back(down);
    }
    for (unsigned k = 0; k < assignment.n; ++k) {
      Raw low;
      low.a.assign(assignment.n, 0);
      low.a[k] = 1;
      assignment.ge.push_back(low);
    }
    expectAnswer(assignment, Want::Feasible, "an assignment polytope");
    // All cells of the first row but one are zero, and that one must exceed 1.
    RawSystem overfull = assignment;
    Raw over;
    over.a.assign(assignment.n, 0);
    over.a[0] = 1;
    over.c = -1;
    overfull.gt.push_back(over);
    expectAnswer(overfull, Want::Infeasible, "a cell above 1");
  }

  // The Klee-Minty cube: its optimum is 5^n, at (0, ..., 0, 5^n). Two certified
  // answers, either side of it, prove that value without any oracle.
  for (unsigned n = 2; n <= 9; ++n) {
    RawSystem cube;
    cube.n = n;
    std::int64_t five = 1;
    for (unsigned i = 0; i < n; ++i) {
      five *= 5;
      Raw row;
      row.a.assign(n, 0);
      for (unsigned j = 0; j < i; ++j)
        row.a[j] = -(std::int64_t(2) << (i - j));
      row.a[i] = -1;
      row.c = five;
      cube.ge.push_back(row);
      Raw low;
      low.a.assign(n, 0);
      low.a[i] = 1;
      cube.ge.push_back(low);
    }
    Raw goal;
    for (unsigned j = 0; j < n; ++j)
      goal.a.push_back(std::int64_t(1) << (n - 1 - j));
    goal.c = -five;
    RawSystem reachable = cube, beyond = cube;
    reachable.ge.push_back(goal);
    beyond.gt.push_back(goal);
    const System yes = toSystem(reachable), no = toSystem(beyond);
    const Decision atOptimum = solved(yes), pastOptimum = solved(no);
    expect(atOptimum.provedFeasible(), "the Klee-Minty optimum is reached");
    expect(pastOptimum.provedInfeasible(), "and cannot be passed");
    certified(yes, atOptimum, "Klee-Minty");
    certified(no, pastOptimum, "Klee-Minty");
  }
}

//===----------------------------------------------------------------------===//
// Systems on which LLVM's Simplex hangs or answers wrongly
//===----------------------------------------------------------------------===//

using Row = std::vector<std::int64_t>;

Raw fromRow(const Row &row) {
  Raw raw;
  raw.a.assign(row.begin(), row.end() - 1);
  raw.c = row.back();
  return raw;
}

RawSystem inequalities(unsigned n, const std::vector<Row> &rows) {
  RawSystem raw;
  raw.n = n;
  for (const Row &row : rows)
    raw.ge.push_back(fromRow(row));
  return raw;
}

/// Hangs in Simplex::addInequality on this order, from the Stage 2 report.
const std::vector<Row> hang7 = {{-2, 0, 2, 0}, {0, 2, 1, 0},   {1, -2, -1, 0},
                                {3, -1, 0, 2}, {0, 1, 0, -1},  {-1, 0, 0, -1},
                                {1, 1, 1, 0}};

/// The nine systems that stalled the compiler, as inequalities and equalities.
#include "exact_stall_systems.inc"

void testLlvmReproducers() {
  // The smallest hang, and every order of its rows: order decides whether LLVM
  // hangs, so every one of the 5040 is asked for.
  const RawSystem hang = inequalities(3, hang7);
  expectAnswer(hang, Want::Infeasible, "the 7 row hang terminates, infeasible");
  std::vector<unsigned> order(7);
  std::iota(order.begin(), order.end(), 0);
  unsigned orders = 0;
  do {
    std::vector<Row> rows;
    for (unsigned i : order)
      rows.push_back(hang7[i]);
    const System system = toSystem(inequalities(3, rows));
    const Decision answer = solved(system);
    expect(answer.provedInfeasible(), "every order is infeasible");
    certified(system, answer, "every order");
    ++orders;
  } while (std::next_permutation(order.begin(), order.end()));
  expect(orders == 5040, "all of them");

  // Fed one row at a time, with a query after each, as LLVM's did.
  Session stepwise(3);
  bool reachedEmpty = false;
  for (const Row &row : hang7) {
    expect(stepwise.addInequality(toExpr(fromRow(row))) == RowError::None, "row");
    const Decision answer = stepwise.check(bounded());
    certified(stepwise.system(), answer, "row by row");
    expect(answer.provedFeasible() || answer.provedInfeasible(),
           "a query after each row");
    // Emptiness, once reached, stays.
    if (reachedEmpty)
      expect(answer.provedInfeasible(), "more rows cannot empty it twice");
    reachedEmpty = reachedEmpty || answer.provedInfeasible();
  }
  expect(reachedEmpty, "and it ends empty");

  // gate_admits_hang.cpp: 7 rows that passed the committed work gate.
  expectAnswer(inequalities(3, {{2, -2, 0, 0},
                                {-2, -3, -3, 0},
                                {1, 3, 2, 0},
                                {-2, 1, -2, 2},
                                {-1, -1, -1, -1},
                                {1, 0, 1, -1},
                                {-2, -2, -3, 0}}),
               Want::Infeasible, "the gated hang");

  // guard_defeated_by_tightening.cpp: scaled copies with a rounding margin.
  for (std::int64_t k : {2, 3, 7, 1000}) {
    std::vector<Row> rows;
    for (const Row &r : hang7)
      rows.push_back({k * r[0], k * r[1], k * r[2], k * r[3] + (k - 1)});
    const System system = toSystem(inequalities(3, rows));
    const Decision answer = solved(system);
    expect(certified(system, answer, "scaled hang"), "the scaled rows finish");
  }

  // hang_subtract.cpp: A minus S, a disjunct at a time. Over the rationals the
  // complement of s >= 0 is s < 0.
  const std::vector<Row> partA = {{0, -1, 0, 0, 0, 0},
                                  {1, 0, 0, 0, 1, 0},
                                  {1, 0, -1, 0, -1, 0},
                                  {-1, 2, 2, -1, 2, 0},
                                  {-1, 0, -1, 1, 1, 0}};
  const std::vector<Row> partS = {{1, 0, 0, 1, 1, -1},
                                  {0, 0, 1, 0, -1, -1},
                                  {1, -1, -1, -2, 0, 0}};
  Session subtract(5);
  for (const Row &row : partA)
    expect(subtract.addInequality(toExpr(fromRow(row))) == RowError::None, "A");
  expect(subtract.check(bounded()).provedFeasible(), "A is not empty");
  for (const Row &row : partS) {
    subtract.push();
    Raw complement = fromRow(row);
    for (std::int64_t &c : complement.a)
      c = -c;
    complement.c = -complement.c;
    expect(subtract.addStrictInequality(toExpr(complement)) == RowError::None,
           "the complement of a row of S");
    const Decision piece = subtract.check(bounded());
    expect(certified(subtract.system(), piece, "a piece of A - S"),
           "the subtraction finishes");
    expect(subtract.pop(), "and rolls back");
  }

  // rollback_false_nonempty.cpp: LLVM's isEmpty() says 0 here.
  Session falseNonEmpty(2);
  for (const Row &row : {Row{-1, 0, 0}, Row{0, -1, 0}, Row{-1, -1, 1}})
    expect(falseNonEmpty.addInequality(toExpr(fromRow(row))) == RowError::None,
           "base");
  expect(falseNonEmpty.check(bounded()).provedFeasible(), "the base holds the origin");
  falseNonEmpty.push();
  for (const Row &row : {Row{1, 0, -1}, Row{-1, 1, -1}})
    expect(falseNonEmpty.addInequality(toExpr(fromRow(row))) == RowError::None,
           "two rows that empty it");
  expect(falseNonEmpty.check(bounded()).provedInfeasible(), "they empty it");
  expect(falseNonEmpty.pop(), "a rollback past empty");
  expect(falseNonEmpty.addInequality(toExpr(fromRow({0, 1, -1}))) ==
             RowError::None,
         "y >= 1");
  const Decision stillEmpty = falseNonEmpty.check(bounded());
  expect(stillEmpty.provedInfeasible(),
         "y <= 0 and y >= 1 is empty, after the rollback too");
  certified(falseNonEmpty.system(), stillEmpty, "after the rollback");

  // rollback_bad_sample.cpp: LLVM's sample (0, 1) violates y <= 0 here.
  Session badSample(2);
  for (const Row &row : {Row{-1, 0, 0}, Row{0, -1, 0}, Row{-1, -1, 1}})
    expect(badSample.addInequality(toExpr(fromRow(row))) == RowError::None,
           "base");
  badSample.push();
  for (const Row &row : {Row{1, 0, -1}, Row{-1, 1, -1}})
    expect(badSample.addInequality(toExpr(fromRow(row))) == RowError::None,
           "two rows");
  expect(badSample.check(bounded()).provedInfeasible(), "empty");
  expect(badSample.pop(), "rolled back");
  const Decision sample = badSample.check(bounded());
  expect(sample.provedFeasible(), "the base is feasible again");
  certified(badSample.system(), sample, "a sample inside the base");

  // The nine stalls of the compiler, which Z3 says are all empty.
  unsigned stalled = 0;
  for (const auto &stall : stallSystems) {
    RawSystem raw;
    raw.n = stall.ge[0].size() - 1;
    for (const Row &row : stall.ge)
      raw.ge.push_back(fromRow(row));
    for (const Row &row : stall.eq)
      raw.eq.push_back(fromRow(row));
    expectAnswer(raw, Want::Infeasible, stall.name);
    ++stalled;
  }
  expect(stalled == 9, "all nine");
}

//===----------------------------------------------------------------------===//
// The shape of the tableau
//===----------------------------------------------------------------------===//

void testTableauShape() {
  // A row on one variable is a bound on it, and takes no tableau row.
  Session single(2);
  expect(single.addInequality(linearExpr({1, 0}, -1)) == RowError::None, "x >= 1");
  expect(single.addInequality(linearExpr({0, -2}, 8)) == RowError::None, "y <= 4");
  expect(single.addEquality(linearExpr({3, 0}, -6)) == RowError::None, "x == 2");
  expect(single.check(bounded()).provedFeasible(), "feasible");
  expect(single.tableau().numVariables() == 2, "no slack was made");
  expect(single.tableau().basicVariables().empty(), "and no row");
  const std::optional<Bound> &lower = single.tableau().lower(0);
  expect(lower && lower->value == DeltaRational(q(2)),
         "the tightest lower bound on x");
  expect(lower && lower->reason.kind == RowRef::Kind::Equality &&
             lower->scale == q(1, 3),
         "from the equality, with its scale");
  const std::optional<Bound> &upper = single.tableau().upper(1);
  expect(upper && upper->value == DeltaRational(q(4)) &&
             upper->scale == q(1, 2),
         "the upper bound on y");

  // Parallel rows share one slack, and a bound keeps the tightest of them.
  Session parallel(2);
  expect(parallel.addInequality(linearExpr({1, 1}, -1)) == RowError::None, "a");
  expect(parallel.addInequality(linearExpr({2, 2}, -3)) == RowError::None, "b");
  expect(parallel.addInequality(linearExpr({-3, -3}, 9)) == RowError::None, "c");
  expect(parallel.addStrictInequality(linearExpr({-1, -1}, 3)) == RowError::None,
         "d");
  expect(parallel.check(bounded()).provedFeasible(), "feasible");
  const BoundsTableau &shared = parallel.tableau();
  expect(shared.numVariables() == 3, "one slack for x + y");
  expect(shared.lower(2) && shared.lower(2)->value == DeltaRational(q(3, 2)),
         "lower bound 3/2");
  expect(shared.upper(2) && shared.upper(2)->value == DeltaRational(q(3), q(-1)),
         "upper bound 3, open");
  expect(shared.upper(2) &&
             shared.upper(2)->reason.kind == RowRef::Kind::StrictInequality,
         "and the open one is the tighter");
  expect(shared.form(2) == std::vector<Int>({Int(1), Int(1)}), "its form");
  // A slack is defined by its form, whatever the session asks of it later.
  expect(parallel.addInequality(linearExpr({1, -1}, 0)) == RowError::None, "e");
  expect(parallel.check(bounded()).provedFeasible(), "still feasible");
  expect(parallel.tableau().numVariables() == 4, "another form, another slack");

  // A form is fixed by the sign of its first entry, whichever row comes first.
  Session signs(2);
  expect(signs.addInequality(linearExpr({-1, 1}, 0)) == RowError::None, "y >= x");
  expect(signs.addInequality(linearExpr({2, -2}, 1)) == RowError::None, "other");
  expect(signs.check(bounded()).provedFeasible(), "feasible");
  expect(signs.tableau().numVariables() == 3, "both rows read one slack");
  expect(signs.tableau().form(2) == std::vector<Int>({Int(1), Int(-1)}),
         "whose first entry is positive");

  // On a tie the bound keeps the row that came first.
  Session ties(1);
  expect(ties.addInequality(linearExpr({1}, -1)) == RowError::None, "x >= 1");
  expect(ties.addInequality(linearExpr({2}, -2)) == RowError::None, "2x >= 2");
  expect(ties.addInequality(linearExpr({-1}, 3)) == RowError::None, "x <= 3");
  expect(ties.addInequality(linearExpr({-2}, 6)) == RowError::None, "2x <= 6");
  expect(ties.check(bounded()).provedFeasible(), "feasible");
  expect(ties.tableau().lower(0) && ties.tableau().lower(0)->reason.index == 0,
         "the first lower bound stays");
  expect(ties.tableau().upper(0) && ties.tableau().upper(0)->reason.index == 2,
         "the first upper bound stays");

  // Asking again after a conflict gives the same explanation, not two.
  const System crossed = toSystem({2, {{{-1, 0}, 0}, {{0, -1}, 0}, {{1, 1}, -2}}, {}, {}});
  BoundsTableau t(2);
  WorkMeter meter{WorkLimits()};
  std::vector<FarkasTerm> terms;
  for (std::size_t i = 0; i < crossed.inequalities().size(); ++i)
    expect(loadRow(t, crossed.inequalities()[i].expr,
                   RowRef{RowRef::Kind::Inequality, i, false}, meter, terms) ==
               BoundsTableau::AssertOutcome::Applied,
           "the rows go in");
  expect(t.step(meter) == BoundsTableau::StepOutcome::Conflict, "a conflict");
  const std::size_t size = t.conflict().size();
  expect(size == 3 && conflictIsFalse(crossed, t.conflict()), "of three rows");
  expect(t.step(meter) == BoundsTableau::StepOutcome::Conflict, "the same one");
  expect(t.conflict().size() == size, "not accumulated");
  expect(conflictIsFalse(crossed, t.conflict()), "and still false");

  const System opposite = toSystem({1, {{{1}, -1}, {{-1}, 0}}, {}, {}});
  BoundsTableau u(1);
  expect(loadRow(u, opposite.inequalities()[0].expr,
                 RowRef{RowRef::Kind::Inequality, 0, false}, meter, terms) ==
             BoundsTableau::AssertOutcome::Applied,
         "x >= 1");
  for (int i = 0; i < 2; ++i) {
    expect(loadRow(u, opposite.inequalities()[1].expr,
                   RowRef{RowRef::Kind::Inequality, 1, false}, meter, terms) ==
               BoundsTableau::AssertOutcome::Conflict,
           "x <= 0 crosses it");
    expect(u.conflict().size() == 2, "two rows, each time");
    expect(conflictIsFalse(opposite, u.conflict()), "false each time");
  }
}

//===----------------------------------------------------------------------===//
// Sessions
//===----------------------------------------------------------------------===//

void testSessionBasics() {
  Session session(2);
  expect(session.addInequality(linearExpr({1}, 0)) ==
             RowError::WrongCoefficientCount,
         "a row of the wrong size is refused");
  expect(session.addStrictInequality(linearExpr({1, 2, 3}, 0)) ==
             RowError::WrongCoefficientCount,
         "a strict one too");
  expect(session.addEquality(linearExpr({}, 0)) ==
             RowError::WrongCoefficientCount,
         "and an equality");
  expect(session.system().numRows() == 0, "leaving no row");
  expect(!session.pop(), "there is no scope to pop");
  expect(session.depth() == 0, "depth stays 0");

  // Repeated queries.
  expect(session.addInequality(linearExpr({1, 1}, -1)) == RowError::None, "row");
  const Decision first = session.check(bounded());
  expect(first.provedFeasible(), "feasible");
  const std::string state = session.stateText();
  const std::string text = canonicalText(first);
  for (int i = 0; i < 5; ++i) {
    const Decision again = session.check(bounded());
    expect(canonicalText(again) == text, "a repeated query gives the same answer");
    expect(!again.receipt().ranTableau && again.receipt().ranChecker,
           "from the cache, through the checker");
    expect(session.stateText() == state, "and changes nothing");
  }

  expect(session.stateText().find("pending\n") != std::string::npos,
         "nothing is left pending after a query");

  // An answer does not survive a row that may break it.
  expect(session.addInequality(linearExpr({-1, 0}, -5)) == RowError::None, "more");
  expect(session.stateText().find("status unchecked") != std::string::npos,
         "a witness is dropped when a row arrives");
  expect(session.stateText().find("witness") == std::string::npos,
         "all of it");
  expect(session.stateText().find("pending 0:1\n") != std::string::npos,
         "and the row waits for the next query");
  const Decision second = session.check(bounded());
  expect(second.provedFeasible() && second.receipt().ranTableau, "asked again");
  certified(session.system(), second, "with the new row");

  // A proof does survive one, since rows only add constraints.
  expect(session.addEquality(linearExpr({0, 0}, 1)) == RowError::None, "1 == 0");
  const Decision third = session.check(bounded());
  expect(third.provedInfeasible(), "1 == 0 is empty");
  expect(session.addInequality(linearExpr({1, 0}, 0)) == RowError::None, "again");
  const Decision fourth = session.check(bounded());
  expect(fourth.provedInfeasible() && !fourth.receipt().ranTableau,
         "still empty, with the same proof");
  certified(session.system(), fourth, "and that proof still checks");

  // A strict row alone needs no small delta, so it gets the first positive
  // value, which keeps the point simple.
  const Decision open = solved(toSystem({1, {}, {{{1}, 0}}, {}}));
  expect(open.rationalWitness() &&
             open.rationalWitness()->values == std::vector<Rational>({q(1)}),
         "x > 0 gives 1");
  const Decision narrow = solved(toSystem({1, {}, {{{2}, -1}, {{-1}, 1}}, {}}));
  expect(narrow.rationalWitness() &&
             narrow.rationalWitness()->values == std::vector<Rational>({q(3, 4)}),
         "1/2 < x < 1 gives the delta that its bounds allow");

  // Scopes restore everything.
  Session scoped(2);
  expect(scoped.addInequality(linearExpr({1, 1}, -1)) == RowError::None, "base");
  expect(scoped.check(bounded()).provedFeasible(), "base");
  const std::string base = scoped.stateText();
  scoped.push();
  expect(scoped.depth() == 1, "one scope");
  expect(scoped.stateText() == base, "a push changes nothing");
  expect(scoped.addInequality(linearExpr({-1, -1}, 0)) == RowError::None, "clash");
  expect(scoped.check(bounded()).provedInfeasible(), "empty in the scope");
  scoped.push();
  const std::string inner = scoped.stateText();
  expect(scoped.addEquality(linearExpr({1, 0}, 0)) == RowError::None, "x == 0");
  expect(scoped.check(bounded()).provedInfeasible(), "empty deeper too");
  expect(scoped.pop(), "out of the inner scope");
  expect(scoped.stateText() == inner, "the inner state is back");
  expect(scoped.pop(), "out of the outer scope");
  expect(scoped.stateText() == base, "the base state is back, byte for byte");
  expect(scoped.depth() == 0, "no scope left");
  expect(scoped.check(bounded()).provedFeasible(), "feasible again");
  expect(!scoped.pop(), "nothing left to pop");

  // A session from a system, and congruences.
  const System rows = toSystem({2, {{{1, 0}, 0}}, {{{0, 1}, -1}}, {{{1, 1}, -3}}});
  Session fromRows{System(rows)};
  const Decision direct = solved(rows);
  expect(canonicalText(fromRows.check(bounded())) == canonicalText(direct),
         "the constructor and solveRational agree");
  System congruent = System::ofSize(1);
  expect(congruent.addCongruence(linearExpr({1}, 0), Int(2)) == RowError::None,
         "a congruence");
  const Decision unsupported = solved(congruent);
  expect(unsupported.isUnknown() &&
             unsupported.why() == UnknownReason::UnsupportedConstruct,
         "a congruence has no rational meaning");
  expect(unsupported.domain() == Domain::Rational, "the domain is recorded");
  expect(!unsupported.provedFeasible() && !unsupported.provedInfeasible() &&
             !unsupported.integerWitness() && !unsupported.rationalWitness() &&
             !unsupported.proof(),
         "and Unknown carries nothing");

  // Rational feasibility is no claim about integers.
  const System half = toSystem({1, {}, {}, {{{2}, -1}}});
  const Decision rational = solved(half);
  expect(rational.provedFeasible() && !rational.integerWitness() &&
             rational.domain() == Domain::Rational,
         "a rational answer carries a rational point only");
  expect(decide(half, Domain::Integer).provedInfeasible(),
         "the same system has no integer point, and decide() says so");
}

/// Random operations, with every query compared against a fresh solve and the
/// oracle, and every pop against the state saved at its push.
void testSessionScripts() {
  std::mt19937_64 rng(19);
  auto pick = [&](int lo, int hi) {
    return lo + int(rng() % std::uint64_t(hi - lo + 1));
  };
  unsigned long checks = 0, pops = 0, feasible = 0, infeasible = 0;
  unsigned maxDepth = 0;
  for (int trial = 0; trial < 2500; ++trial) {
    const unsigned n = pick(1, 3);
    Session session(n);
    RawSystem model = boxed(RawSystem{n, {}, {}, {}}, 4);
    for (const Raw &row : model.ge)
      expect(session.addInequality(toExpr(row)) == RowError::None, "box");

    struct Saved {
      RawSystem model;
      std::string state;
    };
    std::vector<Saved> stack;
    const int steps = pick(5, 30);
    for (int s = 0; s < steps; ++s) {
      const int op = pick(0, 9);
      if (op <= 3) {
        Raw row;
        for (unsigned j = 0; j < n; ++j)
          row.a.push_back(pick(-3, 3));
        row.c = pick(-5, 5);
        if (op <= 1) {
          model.ge.push_back(row);
          expect(session.addInequality(toExpr(row)) == RowError::None, "ge");
        } else if (op == 2) {
          model.gt.push_back(row);
          expect(session.addStrictInequality(toExpr(row)) == RowError::None,
                 "gt");
        } else {
          model.eq.push_back(row);
          expect(session.addEquality(toExpr(row)) == RowError::None, "eq");
        }
      } else if (op <= 5) {
        stack.push_back(Saved{model, session.stateText()});
        session.push();
        maxDepth = std::max<unsigned>(maxDepth, session.depth());
      } else if (op == 6) {
        const bool popped = session.pop();
        expect(popped == !stack.empty(), "pop matches the stack");
        if (!stack.empty()) {
          expect(session.stateText() == stack.back().state,
                 "a pop restores the exact state");
          model = stack.back().model;
          stack.pop_back();
          ++pops;
        }
      } else {
        const Decision answer = session.check(bounded());
        const bool truth = vertexOracle(model);
        expect(truth ? answer.provedFeasible() : answer.provedInfeasible(),
               "the session agrees with the oracle");
        certified(session.system(), answer, "script");
        const System rows = toSystem(model);
        expect(canonicalText(rows) == canonicalText(session.system()) ||
                   rows.numRows() == session.system().numRows(),
               "the session holds the model's rows");
        expect(solved(rows).verdict() == answer.verdict(),
               "and a fresh solve agrees");
        (truth ? feasible : infeasible) += 1;
        ++checks;
      }
    }
    // Unwind everything, checking each level.
    while (!stack.empty()) {
      expect(session.pop(), "unwind");
      expect(session.stateText() == stack.back().state, "unwind state");
      model = stack.back().model;
      stack.pop_back();
      const Decision answer = session.check(bounded());
      expect((vertexOracle(model) ? answer.provedFeasible()
                                  : answer.provedInfeasible()),
             "each level still answers correctly");
    }
  }
  std::cout << "session scripts: " << checks << " queries, " << pops
            << " pops, depth to " << maxDepth << "\n";
  expect(checks > 2000 && pops > 500 && maxDepth >= 4, "the scripts did work");
  expect(feasible > 200 && infeasible > 200, "both outcomes occurred");
}

/// Long incremental runs, on one session, in the order rows arrive.
void testIncremental() {
  std::mt19937_64 rng(23);
  auto pick = [&](int lo, int hi) {
    return lo + int(rng() % std::uint64_t(hi - lo + 1));
  };
  for (int trial = 0; trial < 300; ++trial) {
    const unsigned n = pick(2, 6);
    Session session(n);
    RawSystem model;
    model.n = n;
    bool empty = false;
    for (int r = 0; r < 40; ++r) {
      Raw row;
      for (unsigned j = 0; j < n; ++j)
        row.a.push_back(pick(-2, 2));
      row.c = pick(0, 8); // holds at the origin, so emptiness comes late
      if (r % 7 == 6) {
        // Force a tight row through the current witness now and then.
        row.c = 0;
        model.gt.push_back(row);
        expect(session.addStrictInequality(toExpr(row)) == RowError::None, "gt");
      } else {
        model.ge.push_back(row);
        expect(session.addInequality(toExpr(row)) == RowError::None, "ge");
      }
      const Decision answer = session.check(bounded());
      certified(session.system(), answer, "incremental");
      expect(answer.provedFeasible() || answer.provedInfeasible(),
             "every query is decided");
      if (empty)
        expect(answer.provedInfeasible(), "once empty, always empty");
      empty = empty || answer.provedInfeasible();
      const System rows = toSystem(model);
      expect(solved(rows).verdict() == answer.verdict(),
             "incremental agrees with one shot");
    }
  }
}

//===----------------------------------------------------------------------===//
// Limits and cancellation
//===----------------------------------------------------------------------===//

std::vector<System> budgetSystems() {
  std::vector<System> systems;
  systems.push_back(toSystem(inequalities(3, hang7)));
  systems.push_back(toSystem({2, {{{1, 1}, -1}}, {}, {}}));
  systems.push_back(toSystem(
      {3, {{{1, -1, 0}, 0}, {{0, 1, -1}, 0}}, {{{-1, 0, 1}, 0}}, {}}));
  systems.push_back(toSystem({2, {{{-2, 3}, -1}, {{5, -7}, 2}}, {{{1, 1}, 0}}, {}}));
  for (std::size_t i = 0; i < 3; ++i) {
    RawSystem raw;
    raw.n = stallSystems[i].ge[0].size() - 1;
    for (const Row &row : stallSystems[i].ge)
      raw.ge.push_back(fromRow(row));
    for (const Row &row : stallSystems[i].eq)
      raw.eq.push_back(fromRow(row));
    systems.push_back(toSystem(raw));
  }
  return systems;
}

void testBudgets() {
  unsigned stoppedStates = 0, stoppedByBits = 0;
  for (const System &system : budgetSystems()) {
    const Decision open = solved(system);
    expect(certified(system, open, "budget"), "with enough steps it is decided");
    const std::uint64_t steps = open.receipt().steps;
    expect(steps > 0, "it did some work");
    const std::string answer = canonicalText(open);

    WorkLimits exact;
    exact.maxSteps = steps;
    const Decision same = solveRational(system, exact);
    expect(canonicalText(same) == answer, "exactly enough steps");
    expect(same.receipt().steps == steps, "the same work");

    for (std::uint64_t limit = 0; limit < steps;
         limit += 1 + steps / 300) {
      WorkLimits few;
      few.maxSteps = limit;
      Session session{System(system)};
      const std::string before = session.stateText();
      const Decision stopped = session.check(few);
      expect(stopped.isUnknown() && stopped.why() == UnknownReason::StepLimit,
             "too few steps is Unknown, whatever the answer would be");
      expect(!stopped.rationalWitness() && !stopped.proof(), "and carries nothing");
      expect(stopped.receipt().ranTableau && !stopped.receipt().ranChecker,
             "the receipt shows a run that never reached the checker");
      expect(stopped.receipt().steps <= limit, "and the work it did");
      expect(limit < 3 || stopped.receipt().steps > 0, "which is not nothing");
      expect(session.stateText() == before,
             "a stopped query leaves the session as it was");
      const Decision retried = session.check(bounded());
      expect(canonicalText(retried) == answer, "and a retry gets the answer");
      ++stoppedStates;
    }

    // Every cancellation point.
    CancellationToken counter;
    const Decision counted = solveRational(system, bounded(), &counter);
    expect(canonicalText(counted) == answer, "a token that never fires");
    const std::uint64_t polls = counter.pollCount();
    expect(polls > 0, "the search polls");
    for (std::uint64_t k = 0; k < polls; k += 1 + polls / 100) {
      CancellationToken token;
      token.cancelAfterPolls(k);
      Session session{System(system)};
      const std::string before = session.stateText();
      const Decision cancelled = session.check(bounded(), &token);
      expect(cancelled.isUnknown() && cancelled.why() == UnknownReason::Cancelled,
             "cancelled is Unknown");
      expect(session.stateText() == before, "and leaves the state");
      expect(canonicalText(session.check(bounded())) == answer,
             "and can be retried");
    }
    CancellationToken already;
    already.requestCancellation();
    expect(solveRational(system, WorkLimits(), &already).why() ==
               UnknownReason::Cancelled,
           "cancelled before it starts");

    // Bits: Unknown until the limit is enough, and then the same answer.
    bool wasDecided = false;
    for (unsigned bits = 1; bits <= 80; ++bits) {
      WorkLimits limited = bounded();
      limited.maxCoefficientBits = bits;
      const Decision tried = solveRational(system, limited);
      if (tried.isUnknown()) {
        expect(tried.why() == UnknownReason::BitLimit, "too few bits");
        expect(!wasDecided, "more bits never lose an answer");
        ++stoppedByBits;
      } else {
        expect(canonicalText(tried) == answer, "the same answer");
        wasDecided = true;
      }
    }
    expect(wasDecided, "enough bits give the answer");
  }
  expect(stoppedStates > 100, "the sweeps did work");
  expect(stoppedByBits > 5, "the bit limit stopped some of them");

  // Input values are admitted even where the tableau would not keep them.
  WorkLimits threeBits = bounded();
  threeBits.maxCoefficientBits = 3;
  expect(solveRational(toSystem({1, {{{1024}, 2048}}, {}, {}}), threeBits).why() ==
             UnknownReason::BitLimit,
         "a coefficient over the limit, though x >= -2 would fit");
  expect(solveRational(toSystem({1, {{{1024}, 1}}, {}, {}}), threeBits).why() ==
             UnknownReason::BitLimit,
         "a coefficient over the limit, with a small constant");
  expect(solveRational(toSystem({1, {{{0}, 1024}}, {}, {}}), threeBits).why() ==
             UnknownReason::BitLimit,
         "a constant over the limit, though the row is simply true");

  // Whatever a query keeps fits the limit it ran under.
  auto fits = [](const Rational &value, unsigned bits) {
    return bitLength(value.numerator()) <= bits &&
           bitLength(value.denominator()) <= bits;
  };
  std::mt19937_64 rng(37);
  auto pick = [&](int lo, int hi) {
    return lo + int(rng() % std::uint64_t(hi - lo + 1));
  };
  unsigned measured = 0;
  auto expectFits = [&](const BoundsTableau &t, unsigned bits) {
    ++measured;
    for (const std::vector<Rational> &row : t.rows())
      for (const Rational &cell : row)
        expect(fits(cell, bits), "a cell fits the limit it ran under");
    for (unsigned v = 0; v < t.numVariables(); ++v) {
      expect(fits(t.value(v).real(), bits) && fits(t.value(v).delta(), bits),
             "a value fits the limit it ran under");
      if (t.lower(v))
        expect(fits(t.lower(v)->value.real(), bits), "a lower bound fits");
      if (t.upper(v))
        expect(fits(t.upper(v)->value.real(), bits), "an upper bound fits");
    }
  };
  for (int trial = 0; trial < 1800; ++trial) {
    // Half dense. Half sparse, where a pivot row can be alone in its column.
    RawSystem raw;
    raw.n = pick(3, 8);
    const bool sparse = trial % 2 == 1;
    const int rows = pick(3, sparse ? 14 : 10);
    for (int r = 0; r < rows; ++r) {
      Raw row;
      for (unsigned j = 0; j < raw.n; ++j)
        row.a.push_back(sparse && pick(0, 2) != 0 ? 0 : pick(-9, 9));
      row.c = pick(-20, 20);
      const int kind = pick(0, 9);
      (kind < 6 ? raw.ge : kind < 8 ? raw.gt : raw.eq).push_back(row);
    }
    for (unsigned bits = 3; bits <= 14; ++bits) {
      WorkLimits limited = bounded();
      limited.maxCoefficientBits = bits;
      // All at once, and then one row at a time, which starts each new slack
      // from a point the earlier rows have moved.
      Session whole{toSystem(raw)};
      const Decision answer = whole.check(limited);
      if (answer.isUnknown())
        expect(answer.why() == UnknownReason::BitLimit,
               "only the bits can stop it");
      else
        expectFits(whole.tableau(), bits);

      Session stepwise(raw.n);
      const auto addAll = [&](const std::vector<Raw> &list, int kind) {
        for (const Raw &row : list) {
          const RowError error =
              kind == 0   ? stepwise.addInequality(toExpr(row))
              : kind == 1 ? stepwise.addStrictInequality(toExpr(row))
                          : stepwise.addEquality(toExpr(row));
          expect(error == RowError::None, "a row");
          const Decision step = stepwise.check(limited);
          if (step.isUnknown())
            expect(step.why() == UnknownReason::BitLimit,
                   "only the bits can stop a step");
          else
            expectFits(stepwise.tableau(), bits);
        }
      };
      addAll(raw.ge, 0);
      addAll(raw.gt, 1);
      addAll(raw.eq, 2);
    }
  }
  expect(measured > 1000, "the limit sweep decided enough to measure");

  // Limits on size, at the exact boundary.
  const System two = toSystem({2, {{{1, 1}, -1}, {{1, -1}, 0}}, {}, {}});
  WorkLimits rows;
  rows.maxRows = 2;
  expect(solveRational(two, rows).provedFeasible(), "exactly at the row limit");
  rows.maxRows = 1;
  expect(solveRational(two, rows).why() == UnknownReason::RowLimit, "over it");
  WorkLimits vars;
  vars.maxVariables = 2;
  expect(solveRational(two, vars).provedFeasible(), "at the variable limit");
  vars.maxVariables = 1;
  expect(solveRational(two, vars).why() == UnknownReason::VariableLimit, "over");
  WorkLimits cells;
  cells.maxTableauCells = 4; // two slack rows times two columns
  expect(solveRational(two, cells).provedFeasible(), "at the tableau limit");
  cells.maxTableauCells = 3;
  const Decision over = solveRational(two, cells);
  expect(over.why() == UnknownReason::TableauLimit, "over the tableau limit");
  expect(std::string(unknownReasonName(over.why())) == "tableau-limit", "named");

  // What a session holds, by hand: no tableau rows yet, 2 variables and one row
  // of the system, at 10 units each. A query needs that again for its copy.
  Session held{toSystem({2, {{{1, 1}, -1}}, {}, {}})};
  expect(held.heldCells() == 30, "the size of a state, by hand");
  held.push();
  expect(held.heldCells() == 30, "a scope that shares it adds nothing");
  WorkLimits room = bounded();
  room.maxSessionCells = 59;
  const std::string heldBefore = held.stateText();
  const Decision tooBig = held.check(room);
  expect(tooBig.why() == UnknownReason::TableauLimit,
         "a query that would hold too much is refused");
  expect(!tooBig.receipt().ranTableau, "before it starts");
  expect(held.stateText() == heldBefore, "and changes nothing");
  room.maxSessionCells = 60;
  expect(held.check(room).provedFeasible(), "exactly enough room");
  // It now has a slack: one tableau row of 2 cells, and 3 variables and a row
  // at 10 units each, on top of the 30 that the open scope still holds.
  expect(held.heldCells() == 72, "the state after a query, by hand");
  room.maxSessionCells = 0;
  expect(held.check(room).provedFeasible(), "an answer already known needs no room");
  expect(held.pop(), "closing the scope");
  expect(held.heldCells() == 30, "frees what it held");

  // Two scopes on one state count it once, also behind a newer state: 3 and 1
  // units of 10 for the base, and one more row of the system on top of it.
  Session twice{toSystem({2, {{{1, 1}, -1}}, {}, {}})};
  twice.push();
  twice.push();
  expect(twice.addInequality(linearExpr({1, 0}, 0)) == RowError::None, "a row");
  expect(twice.heldCells() == 70, "two scopes on one state, counted once");

  // Nested scopes run into the limit, and a pop makes room again.
  Session nested(4);
  WorkLimits tight = bounded();
  tight.maxSessionCells = 20000;
  std::mt19937_64 deepRng(41);
  auto small = [&](int lo, int hi) {
    return lo + int(deepRng() % std::uint64_t(hi - lo + 1));
  };
  unsigned reached = 0;
  std::vector<std::string> states;
  while (reached < 500) {
    states.push_back(nested.stateText());
    nested.push();
    LinearExpr row;
    for (unsigned j = 0; j < 4; ++j)
      row.coefficients.push_back(Int(small(-3, 3)));
    row.constant = Int(small(5, 40));
    expect(nested.addInequality(std::move(row)) == RowError::None, "a row");
    const Decision answer = nested.check(tight);
    if (answer.isUnknown()) {
      expect(answer.why() == UnknownReason::TableauLimit, "the session limit");
      break;
    }
    expect(nested.heldCells() <= tight.maxSessionCells, "held stays in the limit");
    ++reached;
  }
  expect(reached > 10 && reached < 500, "the limit is reached at some depth");
  for (std::size_t i = 0; i < 5; ++i) {
    expect(nested.pop(), "closing scopes");
    expect(nested.stateText() == states[states.size() - 1 - i],
           "each one exact, also after a refusal");
  }
  expect(nested.check(tight).provedFeasible(), "and there is room again");

  // A limit near 2^64 does not wrap.
  WorkLimits huge;
  huge.maxSteps = UINT64_MAX;
  huge.maxTableauCells = UINT64_MAX;
  huge.maxSessionCells = UINT64_MAX;
  expect(solveRational(two, huge).provedFeasible(), "the largest limits");

  // A cached answer needs no budget.
  Session session{System(two)};
  expect(session.check().provedFeasible(), "cached");
  WorkLimits none;
  none.maxSteps = 0;
  none.maxRows = 0;
  none.maxVariables = 0;
  expect(session.check(none).provedFeasible(), "the cache takes no work");
}

//===----------------------------------------------------------------------===//
// The certificates themselves
//===----------------------------------------------------------------------===//

/// Damages valid certificates in every way this can think of. The checker must
/// accept a damaged one exactly when this file's own validator does.
void testCertificateMutation() {
  std::mt19937_64 rng(29);
  auto pick = [&](int lo, int hi) {
    return lo + int(rng() % std::uint64_t(hi - lo + 1));
  };
  unsigned long refused = 0, stillValid = 0;
  auto compare = [&](const System &system, const InfeasibilityProof &proof) {
    const bool valid = proofIsValid(system, proof);
    const bool accepted =
        checkInfeasibility(system, Domain::Rational, proof).accepted;
    expect(accepted == valid, "the checker agrees about a damaged proof");
    (valid ? stillValid : refused) += 1;
  };
  auto compareWitness = [&](const System &system,
                            const std::vector<Rational> &point) {
    const bool valid = pointIsMember(system, point);
    const bool accepted = checkWitness(system, RationalWitness{point}).accepted;
    expect(accepted == valid, "the checker agrees about a damaged witness");
    expect(certifyFeasible(system, RationalWitness{point}).provedFeasible() ==
               valid,
           "and so does the gate");
    (valid ? stillValid : refused) += 1;
  };

  for (int trial = 0; trial < 2500; ++trial) {
    RawSystem raw;
    raw.n = pick(1, 4);
    const int rows = pick(2, 8);
    for (int r = 0; r < rows; ++r) {
      Raw row;
      for (unsigned j = 0; j < raw.n; ++j)
        row.a.push_back(pick(-4, 4));
      row.c = pick(-6, 6);
      const int kind = pick(0, 9);
      (kind < 5 ? raw.ge : kind < 8 ? raw.gt : raw.eq).push_back(row);
    }
    const System system = toSystem(raw);
    const Decision answer = solveRational(system);

    if (const RationalWitness *witness = answer.rationalWitness()) {
      compareWitness(system, witness->values);
      for (unsigned j = 0; j < raw.n; ++j) {
        std::vector<Rational> point = witness->values;
        for (const Rational &change :
             {q(1), q(-1), q(1, 7), q(-1, 1000), q(1000000)}) {
          point = witness->values;
          point[j] = point[j] + change;
          compareWitness(system, point);
        }
        point = witness->values;
        point[j] = Rational();
        compareWitness(system, point);
      }
      std::vector<Rational> shortPoint = witness->values;
      shortPoint.pop_back();
      compareWitness(system, shortPoint);
      std::vector<Rational> longPoint = witness->values;
      longPoint.push_back(q(1));
      compareWitness(system, longPoint);
      continue;
    }

    const InfeasibilityProof *proofPointer = answer.proof();
    expect(proofPointer != nullptr, "the other answer has a proof");
    if (!proofPointer)
      continue;
    const InfeasibilityProof &proof = *proofPointer;
    compare(system, proof);
    const std::size_t last = proof.steps.size() - 1;
    expect(proof.steps[last].rule == ProofStep::Rule::Combine,
           "a proof ends in a sum");

    for (std::size_t t = 0; t < proof.steps[last].terms.size(); ++t) {
      for (const Int &value : {Int(0), Int(-1), Int(1), Int(2), Int(1000)}) {
        InfeasibilityProof changed = proof;
        changed.steps[last].terms[t].multiplier = value;
        compare(system, changed);
      }
      InfeasibilityProof bumped = proof;
      bumped.steps[last].terms[t].multiplier += Int(1);
      compare(system, bumped);
      InfeasibilityProof dropped = proof;
      dropped.steps[last].terms.erase(dropped.steps[last].terms.begin() + t);
      compare(system, dropped);
      InfeasibilityProof forward = proof;
      forward.steps[last].terms[t].step = last;
      compare(system, forward);
    }
    for (std::size_t s = 0; s < last; ++s) {
      InfeasibilityProof flipped = proof;
      flipped.steps[s].row.negated = !flipped.steps[s].row.negated;
      compare(system, flipped);
      InfeasibilityProof moved = proof;
      moved.steps[s].row.index += 1;
      compare(system, moved);
      InfeasibilityProof far = proof;
      far.steps[s].row.index = 1000;
      compare(system, far);
      for (RowRef::Kind kind :
           {RowRef::Kind::Inequality, RowRef::Kind::StrictInequality,
            RowRef::Kind::Equality, RowRef::Kind::Congruence}) {
        InfeasibilityProof retyped = proof;
        retyped.steps[s].row.kind = kind;
        compare(system, retyped);
      }
    }
    InfeasibilityProof cut = proof;
    cut.steps.pop_back();
    compare(system, cut);
    InfeasibilityProof empty;
    compare(system, empty);
    InfeasibilityProof doubled = proof;
    doubled.steps.push_back(ProofStep::combine({{last, Int(2)}}));
    compare(system, doubled);

    // Every whole multiple of a proof is a proof, and a gate refuses the rest.
    expect(certifyInfeasible(system, Domain::Rational, doubled).provedInfeasible(),
           "a doubled proof is a proof");
    if (!cut.steps.empty() && !proofIsValid(system, cut)) {
      const Decision gated = certifyInfeasible(system, Domain::Rational, cut);
      expect(gated.isUnknown() &&
                 gated.why() == UnknownReason::CertificateRejected,
             "the gate turns a bad proof into Unknown");
    }

    // A proof of this system says nothing about one with fewer rows.
    if (raw.ge.size() >= 1) {
      RawSystem fewer = raw;
      fewer.ge.pop_back();
      const System smaller = toSystem(fewer);
      compare(smaller, proof);
    }
  }
  std::cout << "certificate mutation: " << refused << " refused, " << stillValid
            << " still valid\n";
  expect(refused > 10000 && stillValid > 1000, "both kinds occurred");
}

//===----------------------------------------------------------------------===//
// Provenance and determinism
//===----------------------------------------------------------------------===//

/// FNV-1a 64, written again here.
std::uint64_t ownFnv1a(const std::string &text) {
  std::uint64_t hash = 14695981039346656037ULL;
  for (unsigned char c : text) {
    hash ^= c;
    hash *= 1099511628211ULL;
  }
  return hash;
}

void testProvenance() {
  const BuildDeclaration &declared = buildDeclaration();
  expect(declared.implementation == SolverImplementation::ProjectOwned,
         "project-owned");
  expect(declared.productionSolver == SolverImplementation::LlvmUpstream,
         "production has not moved");
  expect(declared.solverRevision == "S2", "this is the S2 solver");
  expect(declared.llvmDependency == "ARITHMETIC_ONLY", "LLVM arithmetic only");

  // Reading the declaration is not execution.
  const RuntimeEvidence quiet = runtimeEvidence();
  const std::string report = provenanceJson();
  const RuntimeEvidence same = runtimeEvidence();
  expect(same.rationalChecks == quiet.rationalChecks &&
             same.tableauPivots == quiet.tableauPivots,
         "printing moves no counter");
  expect(report.find("\"rational_checks\"") != std::string::npos &&
             report.find("\"tableau_pivots\"") != std::string::npos,
         "the report has the S2 counters");

  // A worked query. x + y >= 1 is one slack s with s >= 1. The steps are: row
  // 3, form 3, slack row 3 + 3; step 4, move 2, pivot 5; step 4; witness 7.
  // That is one pivot, x_0 for s, and the witness (1, 0).
  const System system = toSystem({2, {{{1, 1}, -1}}, {}, {}});
  const RuntimeEvidence before = runtimeEvidence();
  const Decision answer = solveRational(system);
  const RuntimeEvidence after = runtimeEvidence();
  expect(answer.provedFeasible(), "feasible");
  expect(after.rationalChecks == before.rationalChecks + 1, "one rational check");
  expect(after.tableauPivots == before.tableauPivots + 1, "one pivot");
  expect(after.checkerRuns == before.checkerRuns + 1, "one checker run");
  expect(after.decideCalls == before.decideCalls, "decide() did not run");
  expect(after.latticeRuns == before.latticeRuns, "nor the lattice");
  expect(after.normalizeRuns == before.normalizeRuns, "nor normalization");
  expect(answer.receipt().ranTableau && answer.receipt().ranChecker,
         "the receipt says tableau and checker");
  expect(!answer.receipt().ranLattice && !answer.receipt().ranNormalize,
         "and no other stage");
  expect(answer.receipt().pivots == 1, "one pivot on the receipt");
  expect(answer.receipt().steps == 34, "the work worked out by hand");
  expect(answer.rationalWitness()->values ==
             std::vector<Rational>({q(1), q(0)}),
         "the witness worked out by hand");

  const std::string expected = "exact-system 1\nvariables 2 p p\nge 1 1 ; -1\n"
                               "domain=rational verdict=FEASIBLE reason=none "
                               "steps=34\n"
                               "stages=tableau,checker\npivots=1\n";
  expect(receiptText(system, answer) == expected, "the receipt text");
  expect(answer.receipt().digest == ownFnv1a(expected),
         "the digest, recomputed here");

  // The exact state of that query, worked out by hand.
  Session worked{System(system)};
  expect(worked.check(bounded()).provedFeasible(), "feasible");
  expect(worked.stateText() == "exact-system 1\nvariables 2 p p\nge 1 1 ; -1\n"
                               "pending\nstatus feasible\nwitness 1 0\n"
                               "tableau 2 3 pivots 1\n"
                               "b 0 at 0 value=1 form 1 0\n"
                               "n 1 at 1 value=0 form 0 1\n"
                               "n 2 at 0 value=1 lower=1@ge0+*1 form 1 1\n"
                               "row 0 = 1*x2 -1*x1\n",
         "the whole state, by hand");

  // Two rows that hold at the origin make two slacks, and no pivot.
  Session resting{toSystem({2, {{{1, 1}, 1}, {{1, -1}, 1}}, {}, {}})};
  expect(resting.check(bounded()).provedFeasible(), "feasible");
  expect(resting.stateText() ==
             "exact-system 1\nvariables 2 p p\nge 1 1 ; 1\nge 1 -1 ; 1\n"
             "pending\nstatus feasible\nwitness 0 0\n"
             "tableau 2 4 pivots 0\n"
             "n 0 at 0 value=0 form 1 0\n"
             "n 1 at 1 value=0 form 0 1\n"
             "b 2 at 0 value=0 lower=-1@ge0+*1 form 1 1\n"
             "b 3 at 1 value=0 lower=-1@ge1+*1 form 1 -1\n"
             "row 2 = 1*x0 1*x1\nrow 3 = 1*x0 -1*x1\n",
         "two slacks in their rows, by hand");

  // x >= 1 is a bound on a nonbasic variable, which moves. The steps are: row
  // 2, form 2, move 1, step 2, witness 3.
  const Decision moved = solveRational(toSystem({1, {{{1}, -1}}, {}, {}}));
  expect(moved.provedFeasible() && moved.receipt().steps == 10 &&
             moved.receipt().pivots == 0,
         "the work of a moved variable, by hand");

  // x >= 1 and x <= 0 cross on the second row. The steps are: row 2, form 2,
  // move 1, row 2, form 2, proof 3.
  const Decision crossed = solveRational(toSystem({1, {{{1}, -1}, {{-1}, 0}}, {}, {}}));
  expect(crossed.provedInfeasible() && crossed.receipt().steps == 12 &&
             crossed.receipt().pivots == 0,
         "the work of crossed bounds, by hand");

  // x + y >= 2, x >= y, x <= 0. y enters for s = x + y, and then t = x - y has
  // no repair. The steps are: two slack rows at 12 each, a bound row 6, step 5,
  // update 3, pivot 8, step 5, proof 4.
  const Decision pivoted = solveRational(
      toSystem({2, {{{1, 1}, -2}, {{1, -1}, 0}, {{-1, 0}, 0}}, {}, {}}));
  expect(pivoted.provedInfeasible() && pivoted.receipt().pivots == 1,
         "one pivot before the conflict, by hand");
  expect(pivoted.receipt().steps == 55, "and its work, by hand");

  // decide() has no such receipt, so the two cannot be mixed up.
  const Decision old = decide(toSystem({1, {{{1}, 0}}, {}, {}}), Domain::Integer);
  expect(!old.receipt().ranTableau && old.receipt().pivots == 0,
         "a decide() receipt shows no tableau");
  expect(receiptText(toSystem({1, {{{1}, 0}}, {}, {}}), old).find("pivots") ==
             std::string::npos,
         "and no pivot line");

  // A stage that did not run does not count.
  const RuntimeEvidence s0 = runtimeEvidence();
  WorkLimits none;
  none.maxRows = 0;
  const Decision refused = solveRational(system, none);
  const RuntimeEvidence s1 = runtimeEvidence();
  expect(refused.isUnknown() && s1.rationalChecks == s0.rationalChecks + 1,
         "a refused query is a query");
  expect(s1.tableauPivots == s0.tableauPivots && s1.checkerRuns == s0.checkerRuns,
         "that ran no stage");
  expect(!refused.receipt().ranTableau && !refused.receipt().ranChecker,
         "and its receipt says so");
  expect(refused.receipt().digest == ownFnv1a(receiptText(system, refused)),
         "an Unknown has a receipt too");

  // A refused certificate shows up as evidence.
  const RuntimeEvidence c0 = runtimeEvidence();
  const Decision forged = certifyFeasible(system, RationalWitness{{q(0), q(0)}});
  const RuntimeEvidence c1 = runtimeEvidence();
  expect(forged.why() == UnknownReason::CertificateRejected, "a forged witness");
  expect(c1.checkerRefusals == c0.checkerRefusals + 1, "counted");

  // The receipt follows the input.
  const System other = toSystem({2, {{{1, 1}, -2}}, {}, {}});
  expect(solveRational(other).receipt().digest != answer.receipt().digest,
         "another system, another receipt");
}

void testDeterminism() {
  std::mt19937_64 rng(31);
  auto pick = [&](int lo, int hi) {
    return lo + int(rng() % std::uint64_t(hi - lo + 1));
  };
  std::uint64_t digest = 14695981039346656037ULL;
  for (int trial = 0; trial < 1500; ++trial) {
    RawSystem raw;
    raw.n = pick(1, 5);
    const int rows = pick(1, 9);
    for (int r = 0; r < rows; ++r) {
      Raw row;
      for (unsigned j = 0; j < raw.n; ++j)
        row.a.push_back(pick(-5, 5));
      row.c = pick(-7, 7);
      const int kind = pick(0, 9);
      (kind < 6 ? raw.ge : kind < 8 ? raw.gt : raw.eq).push_back(row);
    }
    const System system = toSystem(raw);
    Session one{System(system)}, two{System(system)};
    const Decision a = one.check(bounded()), b = two.check(bounded());
    expect(canonicalText(a) == canonicalText(b), "the same answer and certificate");
    expect(a.receipt().digest == b.receipt().digest, "the same receipt");
    expect(a.receipt().steps == b.receipt().steps, "the same work");
    expect(one.stateText() == two.stateText(), "the same final state");
    digest = (digest ^ a.receipt().digest) * 1099511628211ULL;
  }
  // A heap with other contents gives the same bits.
  std::vector<std::string> noise;
  for (int i = 0; i < 1000; ++i)
    noise.push_back(std::string(1 + i % 97, 'x'));
  const System system = toSystem(inequalities(3, hang7));
  expect(canonicalText(solveRational(system)) ==
             canonicalText(solveRational(system)),
         "address order does not matter");
  std::cout << "determinism digest: " << digest << "\n";
}

//===----------------------------------------------------------------------===//
// The Python oracle
//===----------------------------------------------------------------------===//

unsigned testRationalVectors(const char *path) {
  std::ifstream in(path);
  std::string line;
  unsigned cases = 0, feasible = 0, infeasible = 0;
  while (std::getline(in, line)) {
    std::istringstream words(line);
    std::string truth;
    unsigned n, nge, ngt, neq;
    words >> truth >> n >> nge >> ngt >> neq;
    System system = System::ofSize(n);
    for (unsigned r = 0; r < nge + ngt + neq; ++r) {
      LinearExpr expr;
      std::string word;
      for (unsigned j = 0; j < n; ++j) {
        words >> word;
        expr.coefficients.push_back(*parseDecimal(word));
      }
      words >> word;
      expr.constant = *parseDecimal(word);
      const RowError error = r < nge         ? system.addInequality(expr)
                             : r < nge + ngt ? system.addStrictInequality(expr)
                                             : system.addEquality(expr);
      expect(error == RowError::None, "a vector row");
    }
    const Decision answer = solveRational(system);
    expect(truth == "F" ? answer.provedFeasible() : answer.provedInfeasible(),
           "the solver agrees with Fourier-Motzkin");
    certified(system, answer, "vector");
    (truth == "F" ? feasible : infeasible) += 1;
    ++cases;
  }
  std::cout << "rational vectors: " << cases << " cases, " << feasible
            << " feasible, " << infeasible << " infeasible\n";
  return cases;
}

} // namespace

int main(int argc, char **argv) {
  if (argc == 3 && std::string(argv[1]) == "--rational-vectors") {
    const unsigned cases = testRationalVectors(argv[2]);
    std::cout << "rational vectors checks: " << passed << " passed, " << failed
              << " failed\n";
    return failed != 0 || cases == 0;
  }
  if (argc == 2 && std::string(argv[1]) == "--print-provenance") {
    std::cout << provenanceJson();
    return 0;
  }
  if (argc == 2 &&
      std::string(argv[1]) == "--print-provenance-after-rational-query") {
    System system = System::ofSize(2);
    if (system.addInequality(linearExpr({1, 1}, -1)) != RowError::None)
      return 1;
    const Decision answer = solved(system);
    std::cout << provenanceJson();
    return answer.provedFeasible() ? 0 : 1;
  }

  testDelta();
  testStrictRows();
  testHandSystems();
  testConcreteDelta();
  testHugeValues();
  testExhaustive();
  testRandomBounded();
  testTransformations();
  testCyclingExamples();
  testLlvmReproducers();
  testTableauShape();
  testSessionBasics();
  testSessionScripts();
  testIncremental();
  testBudgets();
  testCertificateMutation();
  testProvenance();
  testDeterminism();

  std::cout << "pivots driven by hand: " << pivotsDriven << "\n";
  std::cout << "exact rational solver: " << passed << " passed, " << failed
            << " failed\n";
  return failed != 0;
}
