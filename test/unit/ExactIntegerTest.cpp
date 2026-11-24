//===- ExactIntegerTest.cpp - Tests of the integer solver -----------------===//
//
// The integer search of Session.h, against expectations from outside it:
//   - systems worked by hand, and by number theory;
//   - every integer point of a box, tried here and in Python;
//   - transformations that must not change the answer, which reach unbounded
//     systems whose truth is known;
//   - this file's own validators of every witness and every branch proof.
//
// Nothing here links an LLVM solver, so nothing else can be what answers.
//
//===----------------------------------------------------------------------===//

#include "Systolic/Exact/Budget.h"
#include "Systolic/Exact/Checker.h"
#include "Systolic/Exact/Decide.h"
#include "Systolic/Exact/Provenance.h"
#include "Systolic/Exact/Session.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <numeric>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace systolic::exact;

namespace {

unsigned long passed = 0, failed = 0;

void expect(bool ok, const char *what) {
  if (ok) {
    ++passed;
    return;
  }
  std::cout << "FAILED: " << what << "\n";
  if (++failed == 40)
    std::exit(1);
}

Int big(const char *text) { return *parseDecimal(text); }

/// Far above what any system here needs, and low enough that a defect fails an
/// expectation instead of running for hours.
WorkLimits bounded() {
  WorkLimits limits;
  limits.maxSteps = std::uint64_t(1) << 28;
  limits.maxCoefficientBits = 1 << 13;
  limits.maxBranchNodes = 1 << 15;
  return limits;
}

Decision solved(const System &system) { return solveInteger(system, bounded()); }

struct Raw {
  std::vector<std::int64_t> a;
  std::int64_t c = 0;
};

struct RawCongruence {
  Raw row;
  std::int64_t modulus = 1;
};

struct RawSystem {
  unsigned n = 0;
  std::vector<Raw> ge, gt, eq;
  std::vector<RawCongruence> cong;
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
  for (const RawCongruence &row : raw.cong)
    expect(system.addCongruence(toExpr(row.row), Int(row.modulus)) ==
               RowError::None,
           "add cong");
  return system;
}

/// -bound <= x_j <= bound for every variable.
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
// This file's own validators
//===----------------------------------------------------------------------===//

Int valueAt(const LinearExpr &expr, const std::vector<Int> &x) {
  Int total = expr.constant;
  for (std::size_t j = 0; j < x.size(); ++j)
    total += expr.coefficients[j] * x[j];
  return total;
}

bool pointIsMember(const System &system, const std::vector<Int> &x) {
  if (x.size() != system.numVariables())
    return false;
  for (const Inequality &row : system.inequalities())
    if (valueAt(row.expr, x) < Int(0))
      return false;
  for (const StrictInequality &row : system.strictInequalities())
    if (valueAt(row.expr, x) <= Int(0))
      return false;
  for (const Equality &row : system.equalities())
    if (valueAt(row.expr, x) != Int(0))
      return false;
  for (const Congruence &row : system.congruences())
    if (*modFloor(valueAt(row.expr, x), row.modulus) != Int(0))
      return false;
  return true;
}

struct RefRow {
  std::vector<Int> a;
  Int c{0};
  bool strict = false;
};

/// Whether a leaf derives a false row. Written apart from the checker, as a
/// reference for it.
bool leafIsValid(const System &system, const InfeasibilityProof &leaf,
                 const std::vector<RefRow> &assumed) {
  const std::size_t width = system.numVariables() + system.congruences().size();
  if (leaf.steps.empty())
    return false;
  std::vector<RefRow> rows;
  for (const ProofStep &step : leaf.steps) {
    RefRow row;
    if (step.rule == ProofStep::Rule::Input) {
      const RowRef &ref = step.row;
      const LinearExpr *expr = nullptr;
      if (ref.kind == RowRef::Kind::Assumption) {
        if (ref.negated || ref.index >= assumed.size())
          return false;
        rows.push_back(assumed[ref.index]);
        continue;
      }
      row.a.assign(width, Int(0));
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
        row.strict = true;
        break;
      case RowRef::Kind::Equality:
        if (ref.index >= system.equalities().size())
          return false;
        expr = &system.equalities()[ref.index].expr;
        break;
      case RowRef::Kind::Congruence:
        if (ref.index >= system.congruences().size())
          return false;
        expr = &system.congruences()[ref.index].expr;
        row.a[system.numVariables() + ref.index] =
            -system.congruences()[ref.index].modulus;
        break;
      case RowRef::Kind::Assumption:
        return false;
      }
      for (unsigned j = 0; j < system.numVariables(); ++j)
        row.a[j] = expr->coefficients[j];
      row.c = expr->constant;
      if (ref.negated) {
        for (Int &value : row.a)
          value = -value;
        row.c = -row.c;
      }
    } else if (step.rule == ProofStep::Rule::Combine) {
      if (step.terms.empty())
        return false;
      row.a.assign(width, Int(0));
      for (const ProofTerm &term : step.terms) {
        if (term.step >= rows.size() || term.multiplier <= Int(0))
          return false;
        for (std::size_t j = 0; j < width; ++j)
          row.a[j] += term.multiplier * rows[term.step].a[j];
        row.c += term.multiplier * rows[term.step].c;
        row.strict = row.strict || rows[term.step].strict;
      }
    } else if (step.rule == ProofStep::Rule::Tighten) {
      if (step.source >= rows.size() || rows[step.source].strict)
        return false;
      const Int g = contentOf(rows[step.source].a);
      if (g == Int(0))
        return false;
      for (const Int &value : rows[step.source].a)
        row.a.push_back(value / g);
      row.c = *divFloor(rows[step.source].c, g);
    } else {
      if (step.source >= rows.size() || !rows[step.source].strict)
        return false;
      row = rows[step.source];
      row.c = row.c - Int(1);
      row.strict = false;
    }
    rows.push_back(std::move(row));
  }
  const RefRow &last = rows.back();
  for (const Int &value : last.a)
    if (value != Int(0))
      return false;
  return last.c < Int(0) || (last.c == Int(0) && last.strict);
}

bool nodeIsValid(const System &system, const BranchProof &proof,
                 std::size_t index, std::vector<RefRow> &assumed,
                 std::set<std::size_t> &seen) {
  if (index >= proof.nodes.size() || !seen.insert(index).second)
    return false;
  const BranchNode &node = proof.nodes[index];
  if (node.kind == BranchNode::Kind::Leaf)
    return leafIsValid(system, node.leaf, assumed);
  if (node.form.size() != system.numVariables() + system.congruences().size())
    return false;
  RefRow low, high;
  for (const Int &value : node.form) {
    low.a.push_back(-value);
    high.a.push_back(value);
  }
  low.c = node.bound;
  high.c = -node.bound - Int(1);
  assumed.push_back(low);
  const bool lowHolds = nodeIsValid(system, proof, node.low, assumed, seen);
  assumed.back() = high;
  const bool highHolds =
      lowHolds && nodeIsValid(system, proof, node.high, assumed, seen);
  assumed.pop_back();
  return lowHolds && highHolds;
}

bool branchProofIsValid(const System &system, const BranchProof &proof) {
  std::vector<RefRow> assumed;
  std::set<std::size_t> seen;
  return nodeIsValid(system, proof, 0, assumed, seen) &&
         seen.size() == proof.nodes.size();
}

/// Every definite answer, rechecked here. Returns whether it was definite.
bool certified(const System &system, const Decision &decision,
               const char *what) {
  expect(decision.domain() == Domain::Integer, what);
  expect(!decision.rationalWitness() && !decision.proof(), what);
  if (decision.provedFeasible()) {
    expect(decision.integerWitness() &&
               pointIsMember(system, decision.integerWitness()->values),
           what);
    expect(!decision.branchProof(), what);
    return true;
  }
  if (decision.provedInfeasible()) {
    expect(decision.branchProof() &&
               branchProofIsValid(system, *decision.branchProof()),
           what);
    expect(!decision.integerWitness(), what);
    return true;
  }
  expect(!decision.branchProof() && !decision.integerWitness(), what);
  return false;
}

/// Whether every number that a definite answer holds fits in `bits`.
bool numbersFit(const Decision &answer, unsigned bits) {
  auto fits = [&](const Int &value) { return bitLength(value) <= bits; };
  if (const IntegerWitness *witness = answer.integerWitness())
    for (const Int &value : witness->values)
      if (!fits(value))
        return false;
  if (const BranchProof *proof = answer.branchProof()) {
    for (const BranchNode &node : proof->nodes) {
      if (!fits(node.bound))
        return false;
      for (const Int &value : node.form)
        if (!fits(value))
          return false;
      for (const ProofStep &step : node.leaf.steps)
        for (const ProofTerm &term : step.terms)
          if (!fits(term.multiplier))
            return false;
    }
  }
  return true;
}

enum class Want { Feasible, Infeasible };

void expectAnswer(const RawSystem &raw, Want want, const char *what) {
  const System system = toSystem(raw);
  const Decision answer = solved(system);
  expect(want == Want::Feasible ? answer.provedFeasible()
                                : answer.provedInfeasible(),
         what);
  certified(system, answer, what);
}

//===----------------------------------------------------------------------===//
// Brute force
//===----------------------------------------------------------------------===//

std::int64_t rawValue(const Raw &row, const std::vector<std::int64_t> &x) {
  std::int64_t total = row.c;
  for (std::size_t j = 0; j < x.size(); ++j)
    total += row.a[j] * x[j];
  return total;
}

bool rawHolds(const RawSystem &raw, const std::vector<std::int64_t> &x) {
  for (const Raw &row : raw.ge)
    if (rawValue(row, x) < 0)
      return false;
  for (const Raw &row : raw.gt)
    if (rawValue(row, x) <= 0)
      return false;
  for (const Raw &row : raw.eq)
    if (rawValue(row, x) != 0)
      return false;
  for (const RawCongruence &row : raw.cong) {
    const std::int64_t value = rawValue(row.row, x);
    if (((value % row.modulus) + row.modulus) % row.modulus != 0)
      return false;
  }
  return true;
}

/// Whether any point of the box satisfies the system.
bool bruteForce(const RawSystem &raw, std::int64_t box) {
  std::vector<std::int64_t> x(raw.n, -box);
  while (true) {
    if (rawHolds(raw, x))
      return true;
    unsigned j = 0;
    for (; j < raw.n; ++j) {
      if (++x[j] <= box)
        break;
      x[j] = -box;
    }
    if (j == raw.n)
      return false;
  }
}

//===----------------------------------------------------------------------===//
// The certificate rules
//===----------------------------------------------------------------------===//

/// 2x - 1 >= 0 and -2x + 1 >= 0 hold at 1/2 only. The split x <= 0 or x >= 1
/// closes both cases.
BranchProof halfProof() {
  const RowRef first{RowRef::Kind::Inequality, 0, false};
  const RowRef second{RowRef::Kind::Inequality, 1, false};
  const RowRef assumed{RowRef::Kind::Assumption, 0, false};
  BranchProof proof;
  proof.nodes.resize(3);
  proof.nodes[0].kind = BranchNode::Kind::Split;
  proof.nodes[0].form = {Int(1)};
  proof.nodes[0].bound = Int(0);
  proof.nodes[0].low = 1;
  proof.nodes[0].high = 2;
  // (2x - 1) + 2 * (-x) = -1, and (-2x + 1) + 2 * (x - 1) = -1.
  proof.nodes[1].leaf.steps = {ProofStep::input(first),
                               ProofStep::input(assumed),
                               ProofStep::combine({{0, Int(1)}, {1, Int(2)}})};
  proof.nodes[2].leaf.steps = {ProofStep::input(second),
                               ProofStep::input(assumed),
                               ProofStep::combine({{0, Int(1)}, {1, Int(2)}})};
  return proof;
}

void testCertificateRules() {
  const System half = toSystem({1, {{{2}, -1}, {{-2}, 1}}, {}, {}, {}});
  const BranchProof good = halfProof();
  expect(checkBranchProof(half, good).accepted, "a proof by cases");
  expect(branchProofIsValid(half, good), "and by this file's reference");
  expect(certifyInfeasible(half, good).provedInfeasible(), "through the gate");
  expect(certifyInfeasible(half, good).branchProof() != nullptr, "held there");

  auto why = [&](const BranchProof &proof) {
    return checkBranchProof(half, proof).why;
  };
  expect(why(BranchProof()) == "the branch proof has no nodes", "no nodes");

  BranchProof wide = good;
  wide.nodes[0].form = {Int(1), Int(0)};
  expect(why(wide) == "a split does not have one coefficient per column",
         "a form of the wrong size");

  BranchProof missing = good;
  missing.nodes[0].high = 9;
  expect(why(missing) ==
             "a split reads a node that is missing or already reached",
         "a child that does not exist");
  BranchProof twice = good;
  twice.nodes[0].high = 1;
  expect(why(twice) == "a split reads a node that is missing or already reached",
         "one leaf used for both cases");
  BranchProof loop = good;
  loop.nodes[0].low = 0;
  expect(why(loop) == "a split reads a node that is missing or already reached",
         "a split that reads itself");

  BranchProof extra = good;
  extra.nodes.push_back(good.nodes[1]);
  expect(why(extra) == "a node of the proof is never reached", "an extra node");

  // The two cases swapped: each leaf then reads the other's row.
  BranchProof swapped = good;
  std::swap(swapped.nodes[0].low, swapped.nodes[0].high);
  expect(why(swapped) == "the last step still has variables", "swapped cases");

  // A bound one off leaves a value uncovered by neither leaf's arithmetic.
  BranchProof shifted = good;
  shifted.nodes[0].bound = Int(1);
  expect(why(shifted) == "the last step is not a false row", "a shifted bound");

  // The assumption is only there inside its own case.
  BranchProof deep = good;
  deep.nodes[1].leaf.steps[1] =
      ProofStep::input({RowRef::Kind::Assumption, 1, false});
  expect(why(deep) == "an input step names no usable row", "a depth out of range");
  BranchProof negated = good;
  negated.nodes[1].leaf.steps[1] =
      ProofStep::input({RowRef::Kind::Assumption, 0, true});
  expect(why(negated) == "an input step names no usable row",
         "an assumption cannot be negated");

  // A plain derivation has no assumptions at all.
  expect(checkInfeasibility(half, Domain::Integer, good.nodes[1].leaf).why ==
             "an input step names no usable row",
         "assumptions are for branch proofs");

  // A root leaf is a branch proof with no split.
  const System never = toSystem({1, {{{0}, -1}}, {}, {}, {}});
  BranchProof leafOnly;
  leafOnly.nodes.resize(1);
  leafOnly.nodes[0].leaf.steps = {
      ProofStep::input({RowRef::Kind::Inequality, 0, false})};
  expect(checkBranchProof(never, leafOnly).accepted, "a single leaf");

  // Hardening: x > 0 and -x + 1 > 0 leave no integer between 0 and 1.
  const System gap = toSystem({1, {}, {{{1}, 0}, {{-1}, 1}}, {}, {}});
  InfeasibilityProof hard;
  hard.steps = {ProofStep::input({RowRef::Kind::StrictInequality, 0, false}),
                ProofStep::input({RowRef::Kind::StrictInequality, 1, false}),
                ProofStep::harden(0), ProofStep::harden(1),
                ProofStep::combine({{2, Int(1)}, {3, Int(1)}})};
  expect(checkInfeasibility(gap, Domain::Integer, hard).accepted,
         "hardening proves an empty gap");
  expect(checkInfeasibility(gap, Domain::Rational, hard).why ==
             "hardening is not valid over the rationals",
         "and only over the integers");
  InfeasibilityProof late = hard;
  late.steps[2] = ProofStep::harden(7);
  expect(checkInfeasibility(gap, Domain::Integer, late).why ==
             "a harden step reads a later step",
         "a harden step reads earlier steps");
  const System closed = toSystem({1, {{{1}, 0}}, {}, {}, {}});
  InfeasibilityProof soft;
  soft.steps = {ProofStep::input({RowRef::Kind::Inequality, 0, false}),
                ProofStep::harden(0)};
  expect(checkInfeasibility(closed, Domain::Integer, soft).why ==
             "a harden step reads a closed row",
         "a closed row is not hardened");

  // The text of a proof shows its structure.
  const std::string text = canonicalText(good);
  expect(text.find("node 0 split 1 ; 0 low 1 high 2\n") != std::string::npos,
         "the text of a split");
  expect(text.find("input case 0 pos") != std::string::npos,
         "the text of an assumption");
  expect(canonicalText(hard).find("harden 0") != std::string::npos,
         "the text of a harden step");
}

//===----------------------------------------------------------------------===//
// Rounds of pinning
//===----------------------------------------------------------------------===//

/// Each pair of rows pins one variable, but only once the variables before it
/// are known, so a single node pins them one round at a time and every round
/// reads all the earlier ones. Reading a row back through its derivations would
/// double with each round, and no budget would see it: this file would take
/// hours. The answer is x = 0.
RawSystem pinChain(unsigned n) {
  RawSystem raw;
  raw.n = n;
  for (unsigned k = 0; k < n; ++k) {
    Raw up, down;
    up.a.assign(n, 0);
    down.a.assign(n, 0);
    for (unsigned j = 0; j <= k; ++j) {
      up.a[j] = 1;
      down.a[j] = j < k ? -2 : -1;
    }
    raw.ge.push_back(up);
    raw.ge.push_back(down);
  }
  return raw;
}

void testPinChains() {
  for (unsigned n : {1u, 2u, 3u, 8u, 16u, 32u}) {
    const RawSystem chain = pinChain(n);
    expectAnswer(chain, Want::Feasible, "a chain has its one point");
    const Decision found = solved(toSystem(chain));
    expect(found.receipt().nodes == 1, "a chain needs no split");
    if (found.integerWitness())
      for (const Int &value : found.integerWitness()->values)
        expect(value == Int(0), "the one point of a chain is zero");

    // Only the last round sees the row that fails.
    RawSystem closed = chain;
    Raw last;
    last.a.assign(n, 0);
    last.a[n - 1] = 1;
    last.c = -1;
    closed.ge.push_back(last);
    expectAnswer(closed, Want::Infeasible, "a chain closes at its last round");

    // The sum of every variable is even, and one more than that must be too.
    // Only with all of them pinned does the lattice see it, so the proof reads
    // every round.
    RawSystem parity = chain;
    RawCongruence odd;
    odd.row.a.assign(n, 1);
    odd.row.c = 1;
    odd.modulus = 2;
    parity.cong.push_back(odd);
    expectAnswer(parity, Want::Infeasible, "a chain closes on a congruence");
  }
}

//===----------------------------------------------------------------------===//
// Systems worked by hand and by number theory
//===----------------------------------------------------------------------===//

/// a_1 x_1 + ... + a_n x_n == target, for x >= 0.
RawSystem coinProblem(const std::vector<std::int64_t> &coins,
                      std::int64_t target) {
  RawSystem raw;
  raw.n = coins.size();
  raw.eq.push_back(Raw{coins, -target});
  for (unsigned j = 0; j < raw.n; ++j) {
    Raw low;
    low.a.assign(raw.n, 0);
    low.a[j] = 1;
    raw.ge.push_back(low);
  }
  return raw;
}

/// n + 1 pigeons in n holes, one each, with each pair of pigeons kept apart by a
/// row of its own. All 1/2 is a rational point, for two holes or more.
RawSystem pigeonhole(unsigned holes) {
  const unsigned pigeons = holes + 1;
  RawSystem raw;
  raw.n = pigeons * holes;
  auto at = [&](unsigned p, unsigned h) { return p * holes + h; };
  for (unsigned k = 0; k < raw.n; ++k) {
    Raw low, high;
    low.a.assign(raw.n, 0);
    high.a.assign(raw.n, 0);
    low.a[k] = 1;
    high.a[k] = -1;
    high.c = 1;
    raw.ge.push_back(low);
    raw.ge.push_back(high);
  }
  for (unsigned p = 0; p < pigeons; ++p) {
    Raw each;
    each.a.assign(raw.n, 0);
    each.c = -1;
    for (unsigned h = 0; h < holes; ++h)
      each.a[at(p, h)] = 1;
    raw.ge.push_back(each);
  }
  for (unsigned h = 0; h < holes; ++h) {
    for (unsigned p = 0; p < pigeons; ++p) {
      for (unsigned q = p + 1; q < pigeons; ++q) {
        Raw apart;
        apart.a.assign(raw.n, 0);
        apart.c = 1;
        apart.a[at(p, h)] = -1;
        apart.a[at(q, h)] = -1;
        raw.ge.push_back(apart);
      }
    }
  }
  return raw;
}

void testHandSystems() {
  const Want F = Want::Feasible, I = Want::Infeasible;
  expectAnswer({0, {}, {}, {}, {}}, F, "nothing at all");
  expectAnswer({3, {}, {}, {}, {}}, F, "three free variables");
  expectAnswer({1, {{{0}, -1}}, {}, {}, {}}, I, "-1 >= 0");
  expectAnswer({1, {{{0}, 0}}, {}, {}, {}}, F, "0 >= 0");
  expectAnswer({1, {}, {{{0}, 0}}, {}, {}}, I, "0 > 0");
  expectAnswer({1, {}, {{{0}, 1}}, {}, {}}, F, "1 > 0");
  expectAnswer({1, {}, {}, {{{0}, 1}}, {}}, I, "1 == 0");
  expectAnswer({1, {}, {}, {}, {{{{0}, 1}, 2}}}, I, "1 == 0 (mod 2)");
  expectAnswer({1, {}, {}, {}, {{{{0}, 4}, 2}}}, F, "4 == 0 (mod 2)");

  // A gcd that the constant misses, with and without variables to spare.
  expectAnswer({2, {}, {}, {{{2, 4}, -3}}, {}}, I, "2x + 4y = 3");
  expectAnswer({2, {}, {}, {{{2, 4}, -6}}, {}}, F, "2x + 4y = 6");
  expectAnswer({3, {}, {}, {{{6, 10, 15}, -1}}, {}}, F, "gcd 1 from three coins");
  expectAnswer({2, {}, {}, {{{3, -3}, -1}}, {}}, I, "3x - 3y = 1");
  expectAnswer({2, {{{3, -3}, -1}, {{-3, 3}, 2}}, {}, {}, {}}, I,
               "1 <= 3x - 3y <= 2, an unbounded strip with no integer point");
  expectAnswer({2, {{{3, -2}, -1}, {{-3, 2}, 1}}, {}, {}, {}}, F,
               "3x - 2y = 1 written as two rows");

  // Strict rows, which harden over the integers.
  expectAnswer({1, {}, {{{1}, 0}, {{-1}, 1}}, {}, {}}, I, "0 < x < 1");
  expectAnswer({1, {}, {{{2}, 0}, {{-2}, 2}}, {}, {}}, I, "0 < 2x < 2");
  expectAnswer({1, {}, {{{2}, 0}, {{-2}, 3}}, {}, {}}, F, "0 < 2x < 3, x = 1");
  expectAnswer({1, {}, {{{3}, -1}, {{-3}, 2}}, {}, {}}, I, "1 < 3x < 2");
  expectAnswer({2, {}, {{{1, -1}, 0}, {{-1, 1}, 1}}, {}, {}}, I,
               "x - y strictly between 0 and 1");

  // Congruences, by the Chinese remainder theorem.
  expectAnswer({1, {}, {}, {}, {{{{1}, 0}, 2}, {{{1}, -1}, 2}}}, I,
               "x even and x odd");
  expectAnswer({1, {}, {}, {}, {{{{1}, -1}, 2}, {{{1}, -2}, 3}}}, F,
               "x = 1 mod 2 and x = 2 mod 3, x = 5");
  expectAnswer({1, {}, {}, {}, {{{{1}, 0}, 4}, {{{1}, -2}, 6}}}, F,
               "x = 0 mod 4 and x = 2 mod 6, x = 8");
  expectAnswer({1, {}, {}, {}, {{{{1}, -1}, 4}, {{{1}, -2}, 6}}}, I,
               "x = 1 mod 4 and x = 2 mod 6 differ mod 2");
  expectAnswer({1, {}, {}, {}, {{{{2}, -1}, 4}}}, I, "2x = 1 mod 4");
  expectAnswer({1, {}, {}, {}, {{{{3}, -1}, 7}}}, F, "3x = 1 mod 7, x = 5");
  expectAnswer({1, {}, {}, {}, {{{{5}, -3}, 1}}}, F, "modulus 1");
  expectAnswer({1, {{{1}, 0}, {{-1}, 2}}, {}, {}, {{{{1}, -3}, 7}}}, I,
               "x = 3 mod 7 with 0 <= x <= 2");
  expectAnswer({1, {{{1}, 0}, {{-1}, 3}}, {}, {}, {{{{1}, -3}, 7}}}, F,
               "x = 3 mod 7 with 0 <= x <= 3");
  expectAnswer({2, {}, {}, {}, {{{{1, 1}, -1}, 2}, {{{1, -1}, 0}, 2}}}, I,
               "x + y odd and x - y even");
  expectAnswer({3, {{{1, 0, 0}, 0}, {{-1, 0, 0}, 1}, {{0, 1, 0}, 0},
                    {{0, -1, 0}, 1}, {{0, 0, 1}, 0}, {{0, 0, -1}, 1}},
                {},
                {},
                {{{{1, 1, 1}, -1}, 2}}},
               F, "odd parity of three bits");

  // Coin problems. The largest value that coins cannot make is known.
  expectAnswer(coinProblem({5, 7}, 23), I, "23 = 5a + 7b is the Frobenius number");
  expectAnswer(coinProblem({5, 7}, 24), F, "24 = 5a + 7b");
  expectAnswer(coinProblem({6, 10, 15}, 29), I, "29 from 6, 10 and 15");
  expectAnswer(coinProblem({6, 10, 15}, 30), F, "30 from 6, 10 and 15");
  expectAnswer(coinProblem({6, 9, 20}, 43), I, "43 chicken nuggets");
  expectAnswer(coinProblem({6, 9, 20}, 44), F, "44 chicken nuggets");
  expectAnswer(coinProblem({12345, 54321}, 12345 * 54321 + 1), I,
               "coins that share 3 miss a value not divisible by 3");
  expectAnswer(coinProblem({1234, 56789}, 1234 * 56789 - 1234 - 56789), I,
               "the Frobenius number of two large coprime coins");
  expectAnswer(coinProblem({1234, 56789}, 1234 * 56789 - 1234 - 56789 + 1), F,
               "and one more");

  // Rationally feasible, with no integer point.
  for (unsigned n : {3u, 5u, 7u, 9u, 21u}) {
    // Jeroslow: 2 x_1 + ... + 2 x_n = n for bits, n odd.
    RawSystem jeroslow;
    jeroslow.n = n;
    jeroslow.eq.push_back(Raw{std::vector<std::int64_t>(n, 2), -std::int64_t(n)});
    expectAnswer(boxed(jeroslow, 1), I, "Jeroslow's parity row");
  }
  // The odd cycle: pairs at most 1, and 2(x + y + z) >= 3. All 1/2 satisfies it,
  // and the gcd of the last row closes it at the root.
  const RawSystem oddCycle = {3,
                              {{{-1, -1, 0}, 1},
                               {{0, -1, -1}, 1},
                               {{-1, 0, -1}, 1},
                               {{2, 2, 2}, -3},
                               {{1, 0, 0}, 0},
                               {{0, 1, 0}, 0},
                               {{0, 0, 1}, 0}},
                              {},
                              {},
                              {}};
  expect(solveRational(toSystem(oddCycle)).provedFeasible(),
         "the odd cycle has a rational point");
  expectAnswer(oddCycle, I, "and no integer one");
  expectAnswer(pigeonhole(1), I, "two pigeons in one hole");
  for (unsigned holes : {2u, 3u, 4u}) {
    expect(solveRational(toSystem(pigeonhole(holes))).provedFeasible(),
           "pigeons fit in holes by fractions");
    expectAnswer(pigeonhole(holes), I, "and not whole");
  }

  // Existential variables are coordinates of the witness.
  std::vector<Variable> kinds(3);
  kinds[1].kind = VarKind::Existential;
  kinds[2].kind = VarKind::Existential;
  System evenAndOdd(kinds);
  expect(evenAndOdd.addEquality(linearExpr({1, -2, 0}, 0)) == RowError::None,
         "x = 2e");
  expect(evenAndOdd.addEquality(linearExpr({1, 0, -2}, -1)) == RowError::None,
         "x = 2f + 1");
  const Decision noneEven = solved(evenAndOdd);
  expect(noneEven.provedInfeasible(), "no x is even and odd");
  certified(evenAndOdd, noneEven, "existentials");

  // y = floor(x / 3), through a local: 3y <= x <= 3y + 2.
  std::vector<Variable> floorKinds(2);
  floorKinds[1].kind = VarKind::Existential;
  auto floorOf = [&](std::int64_t x, std::int64_t y) {
    System system(floorKinds);
    expect(system.addInequality(linearExpr({1, -3}, 0)) == RowError::None, "low");
    expect(system.addInequality(linearExpr({-1, 3}, 2)) == RowError::None, "high");
    expect(system.addEquality(linearExpr({1, 0}, -x)) == RowError::None, "x");
    expect(system.addEquality(linearExpr({0, 1}, -y)) == RowError::None, "y");
    return system;
  };
  const System right = floorOf(7, 2), wrong = floorOf(7, 3), minus = floorOf(-7, -3);
  expect(solved(right).provedFeasible(), "floor(7 / 3) = 2");
  expect(solved(wrong).provedInfeasible(), "floor(7 / 3) is not 3");
  expect(solved(minus).provedFeasible(), "floor(-7 / 3) = -3");

  // Unbounded and full dimensional, where the point comes from moving inward.
  expectAnswer({2, {{{3, -2}, -1}, {{-2, 3}, -2}, {{1, 1}, -7}}, {}, {}, {}}, F,
               "a cone whose vertex is a fraction");
  expectAnswer({3, {{{5, -3, 1}, -1}, {{-3, 5, 2}, -1}, {{1, 1, -4}, -1}}, {}, {}, {}},
               F, "a cone in three dimensions");
  // Unbounded and thin: x + y is pinned between rows, with a free direction.
  expectAnswer({3, {{{2, 2, 1}, -1}, {{-2, -2, -1}, 1}, {{0, 0, 1}, 0}}, {}, {}, {}},
               F, "a slab with a free direction");
  expectAnswer({3, {{{3, 3, 3}, -1}, {{-3, -3, -3}, 2}}, {}, {}, {}}, I,
               "1 <= 3(x + y + z) <= 2 with two free directions");
  expectAnswer({3, {{{2, 3, 5}, -1}, {{-2, -3, -5}, 1}}, {}, {{{4, 6, 10}, -2}}, {}},
               F, "a row and its multiple as an equality");
}

/// Answers that a transformation must not change, and that build the unbounded
/// systems whose truth is known.
void testUnbounded() {
  std::mt19937_64 rng(101);
  auto pick = [&](int lo, int hi) {
    return lo + int(rng() % std::uint64_t(hi - lo + 1));
  };
  unsigned feasible = 0, infeasible = 0;
  unsigned long nodes = 0;
  for (int trial = 0; trial < 2500; ++trial) {
    // A small system whose truth is found by brute force.
    RawSystem base;
    base.n = pick(1, 3);
    const int rows = pick(1, 4);
    for (int r = 0; r < rows; ++r) {
      Raw row;
      for (unsigned j = 0; j < base.n; ++j)
        row.a.push_back(pick(-3, 3) * (trial % 3 == 0 ? 2 : 1));
      row.c = pick(-5, 5);
      const int kind = pick(0, 9);
      if (kind < 6)
        base.ge.push_back(row);
      else if (kind < 8)
        base.gt.push_back(row);
      else
        base.eq.push_back(row);
    }
    base = boxed(base, 3);
    const bool truth = bruteForce(base, 3);

    // Free directions, and then a unimodular change of coordinates. Both are
    // bijections on the integer points of the system, so the truth stays, while
    // the free directions now run through every variable.
    const unsigned extra = pick(1, 3);
    const unsigned n = base.n + extra;
    std::vector<std::vector<std::int64_t>> shear(
        n, std::vector<std::int64_t>(n, 0));
    for (unsigned j = 0; j < n; ++j)
      shear[j][j] = 1;
    for (int step = 0; step < 6; ++step) {
      const unsigned a = pick(0, n - 1), b = pick(0, n - 1);
      if (a == b)
        continue;
      const int k = pick(-2, 2);
      for (unsigned r = 0; r < n; ++r)
        shear[r][b] += k * shear[r][a];
    }
    auto moved = [&](const Raw &row) {
      Raw out;
      out.a.assign(n, 0);
      out.c = row.c;
      for (unsigned r = 0; r < base.n; ++r)
        for (unsigned j = 0; j < n; ++j)
          out.a[j] += row.a[r] * shear[r][j];
      return out;
    };
    RawSystem image;
    image.n = n;
    for (const Raw &row : base.ge)
      image.ge.push_back(moved(row));
    for (const Raw &row : base.gt)
      image.gt.push_back(moved(row));
    for (const Raw &row : base.eq)
      image.eq.push_back(moved(row));

    const System system = toSystem(image);
    const Decision answer = solved(system);
    expect(truth ? answer.provedFeasible() : answer.provedInfeasible(),
           "an unbounded image has the truth of its base");
    certified(system, answer, "unbounded");
    nodes += answer.receipt().nodes;
    (truth ? feasible : infeasible) += 1;
  }
  std::cout << "unbounded images: " << feasible << " feasible, " << infeasible
            << " infeasible, " << nodes << " nodes\n";
  expect(feasible > 300 && infeasible > 300, "both outcomes occurred");
}

void testExhaustive() {
  struct Row {
    Raw raw;
    int kind; // 0 ge, 1 gt, 2 eq, 3 cong 2, 4 cong 3
  };
  auto alphabet = [](unsigned n, std::vector<std::int64_t> values,
                     std::vector<std::int64_t> constants) {
    std::vector<Row> rows;
    std::vector<std::int64_t> a(n, values[0]);
    while (true) {
      for (std::int64_t c : constants)
        for (int kind = 0; kind < 5; ++kind)
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
  unsigned long systems = 0, feasible = 0, infeasible = 0, splits = 0;
  auto run = [&](unsigned n, const std::vector<const Row *> &chosen,
                 std::int64_t box) {
    RawSystem raw;
    raw.n = n;
    for (const Row *row : chosen) {
      if (row->kind == 0)
        raw.ge.push_back(row->raw);
      else if (row->kind == 1)
        raw.gt.push_back(row->raw);
      else if (row->kind == 2)
        raw.eq.push_back(row->raw);
      else
        raw.cong.push_back(RawCongruence{row->raw, row->kind == 3 ? 2 : 3});
    }
    raw = boxed(std::move(raw), box);
    const bool truth = bruteForce(raw, box);
    const System system = toSystem(raw);
    const Decision answer = solved(system);
    expect(truth ? answer.provedFeasible() : answer.provedInfeasible(),
           "the solver agrees with every point of the box");
    certified(system, answer, "exhaustive");
    ++systems;
    splits += answer.receipt().nodes > 1;
    (truth ? feasible : infeasible) += 1;
  };

  // One variable: every coefficient in -3..3 and constant in -2..2.
  const std::vector<Row> one = alphabet(1, {-3, -2, -1, 0, 1, 2, 3}, {-2, -1, 0, 1, 2});
  for (std::size_t a = 0; a < one.size(); ++a) {
    run(1, {&one[a]}, 3);
    for (std::size_t b = a; b < one.size(); b += 2) {
      run(1, {&one[a], &one[b]}, 3);
      for (std::size_t c = b; c < one.size(); c += 7)
        run(1, {&one[a], &one[b], &one[c]}, 3);
    }
  }
  // Two variables: every coefficient and constant in -2..2 by steps, in pairs.
  const std::vector<Row> two = alphabet(2, {-2, -1, 0, 1, 2}, {-1, 0, 1});
  for (std::size_t a = 0; a < two.size(); a += 3) {
    run(2, {&two[a]}, 2);
    for (std::size_t b = a; b < two.size(); b += 5)
      run(2, {&two[a], &two[b]}, 2);
  }
  std::cout << "exhaustive: " << systems << " systems, " << feasible
            << " feasible, " << infeasible << " infeasible, " << splits
            << " needed a split\n";
  expect(feasible > 1000 && infeasible > 1000, "both outcomes occurred");
  expect(splits > 50, "and some needed a split");
}

void testRandomBounded() {
  std::mt19937_64 rng(103);
  auto pick = [&](int lo, int hi) {
    return lo + int(rng() % std::uint64_t(hi - lo + 1));
  };
  unsigned feasible = 0, infeasible = 0, rationalOnly = 0;
  for (int trial = 0; trial < 5000; ++trial) {
    RawSystem raw;
    raw.n = pick(2, 4);
    const int rows = pick(1, 7);
    for (int r = 0; r < rows; ++r) {
      Raw row;
      for (unsigned j = 0; j < raw.n; ++j)
        row.a.push_back(pick(-5, 5));
      row.c = pick(-9, 9);
      const int kind = pick(0, 11);
      if (kind < 6)
        raw.ge.push_back(row);
      else if (kind < 8)
        raw.gt.push_back(row);
      else if (kind < 10)
        raw.eq.push_back(row);
      else
        raw.cong.push_back(RawCongruence{row, pick(1, 7)});
    }
    raw = boxed(raw, 3);
    const bool truth = bruteForce(raw, 3);
    const System system = toSystem(raw);
    const Decision answer = solved(system);
    expect(truth ? answer.provedFeasible() : answer.provedInfeasible(),
           "the solver agrees with every point of the box");
    certified(system, answer, "random");
    (truth ? feasible : infeasible) += 1;

    // Whether the rationals agree has no bearing on the integer answer.
    if (raw.cong.empty() && !truth && solveRational(system).provedFeasible())
      ++rationalOnly;
  }
  std::cout << "random: " << feasible << " feasible, " << infeasible
            << " infeasible, " << rationalOnly << " of them rationally feasible\n";
  expect(feasible > 300 && infeasible > 300 && rationalOnly > 50,
         "all three kinds occurred");
}

void testHugeValues() {
  const Int two100 = big("1267650600228229401496703205376");
  const Int two70 = big("1180591620717411303424");
  // x = 2^100 is a point past every machine word.
  System far = System::ofSize(1);
  expect(far.addEquality(LinearExpr{{Int(1)}, -two100}) == RowError::None, "a");
  const Decision farAnswer = solved(far);
  expect(farAnswer.provedFeasible() && farAnswer.integerWitness()->values[0] == two100,
         "the witness is exactly 2^100");

  // 2^100 x + 1 == 0 has no integer point, by a gcd the size of 2^100.
  System odd = System::ofSize(1);
  expect(odd.addEquality(LinearExpr{{two100}, Int(1)}) == RowError::None, "b");
  const Decision oddAnswer = solved(odd);
  expect(oddAnswer.provedInfeasible(), "a gcd of 2^100");
  certified(odd, oddAnswer, "huge gcd");

  // A congruence with a modulus past 64 bits, and the witness it forces.
  System wide = System::ofSize(1);
  expect(wide.addCongruence(LinearExpr{{Int(1)}, Int(-7)}, two70) == RowError::None,
         "c");
  expect(wide.addInequality(LinearExpr{{Int(1)}, -two70}) == RowError::None, "d");
  const Decision wideAnswer = solved(wide);
  expect(wideAnswer.provedFeasible(), "x = 7 mod 2^70, x >= 2^70");
  certified(wide, wideAnswer, "huge modulus");

  // Rows around the signed 64 bit limits, either side of a gap of one.
  const Int top = big("9223372036854775807");
  System edge = System::ofSize(1);
  expect(edge.addInequality(LinearExpr{{Int(1)}, -(top + Int(1))}) == RowError::None,
         "e");
  expect(edge.addInequality(LinearExpr{{Int(-1)}, top}) == RowError::None, "f");
  const Decision edgeAnswer = solved(edge);
  expect(edgeAnswer.provedInfeasible(), "x >= 2^63 and x <= 2^63 - 1");
  certified(edge, edgeAnswer, "the edge of 64 bits");

  // Shifting, stretching a row, and a huge shear keep the integer points in
  // bijection, so they keep the truth of a small system.
  std::mt19937_64 rng(107);
  auto pick = [&](int lo, int hi) {
    return lo + int(rng() % std::uint64_t(hi - lo + 1));
  };
  unsigned same = 0, feasible = 0, infeasible = 0;
  for (int trial = 0; trial < 1200; ++trial) {
    RawSystem raw;
    raw.n = pick(2, 3);
    const int rows = pick(1, 5);
    for (int r = 0; r < rows; ++r) {
      Raw row;
      for (unsigned j = 0; j < raw.n; ++j)
        row.a.push_back(pick(-4, 4));
      row.c = pick(-6, 6);
      (pick(0, 3) == 0 ? raw.eq : raw.ge).push_back(row);
    }
    raw = boxed(raw, 3);
    const bool truth = bruteForce(raw, 3);
    (truth ? feasible : infeasible) += 1;

    auto rebuilt = [&](auto transform) {
      System system = System::ofSize(raw.n);
      for (const Raw &row : raw.ge)
        expect(system.addInequality(transform(toExpr(row))) == RowError::None, "g");
      for (const Raw &row : raw.eq)
        expect(system.addEquality(transform(toExpr(row))) == RowError::None, "h");
      return system;
    };
    // x_0 -> x_0 + 2^70.
    const System shifted = rebuilt([&](LinearExpr e) {
      e.constant -= e.coefficients[0] * two70;
      return e;
    });
    // x_1 -> x_1 + 2^100 x_0, a shear past 64 bits.
    const System sheared = rebuilt([&](LinearExpr e) {
      e.coefficients[0] -= e.coefficients[1] * two100;
      return e;
    });
    // A positive multiple of each row.
    const System scaled = rebuilt([&](LinearExpr e) {
      for (Int &c : e.coefficients)
        c *= two70;
      e.constant *= two70;
      return e;
    });
    for (const System *system : {&shifted, &sheared, &scaled}) {
      const Decision answer = solved(*system);
      expect(truth ? answer.provedFeasible() : answer.provedInfeasible(),
             "a bijection on the integers keeps the truth");
      certified(*system, answer, "huge");
      ++same;
    }
  }
  expect(same == 3600, "all of them ran");
  expect(feasible > 100 && infeasible > 100, "both outcomes occurred");
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

const std::vector<Row> hang7 = {{-2, 0, 2, 0}, {0, 2, 1, 0},   {1, -2, -1, 0},
                                {3, -1, 0, 2}, {0, 1, 0, -1},  {-1, 0, 0, -1},
                                {1, 1, 1, 0}};

#include "exact_stall_systems.inc"

void testLlvmReproducers() {
  // isIntegerEmpty and findIntegerSample hang on this, and row order decides it.
  expectAnswer(inequalities(3, hang7), Want::Infeasible, "the 7 row hang");
  std::vector<unsigned> order(7);
  std::iota(order.begin(), order.end(), 0);
  unsigned orders = 0;
  do {
    std::vector<Row> rows;
    for (unsigned i : order)
      rows.push_back(hang7[i]);
    const System system = toSystem(inequalities(3, rows));
    const Decision answer = solved(system);
    expect(answer.provedInfeasible(), "every order is empty");
    certified(system, answer, "every order");
    ++orders;
  } while (std::next_permutation(order.begin(), order.end()));
  expect(orders == 5040, "all of them");

  expectAnswer(inequalities(3, {{2, -2, 0, 0},
                                {-2, -3, -3, 0},
                                {1, 3, 2, 0},
                                {-2, 1, -2, 2},
                                {-1, -1, -1, -1},
                                {1, 0, 1, -1},
                                {-2, -2, -3, 0}}),
               Want::Infeasible, "the gated hang");

  // guard_defeated_by_tightening.cpp, where tightening recreates the hang.
  for (std::int64_t k : {2, 3, 7, 1000}) {
    std::vector<Row> rows;
    for (const Row &r : hang7)
      rows.push_back({k * r[0], k * r[1], k * r[2], k * r[3] + (k - 1)});
    expectAnswer(inequalities(3, rows), Want::Infeasible, "tightened to the hang");
  }

  // hang_subtract.cpp: A minus S. Over the integers the complement of s >= 0 is
  // -s - 1 >= 0, and each piece must be decided.
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
  expect(subtract.checkInteger(bounded()).provedFeasible(), "A has a point");
  for (const Row &row : partS) {
    subtract.push();
    Raw complement = fromRow(row);
    for (std::int64_t &c : complement.a)
      c = -c;
    complement.c = -complement.c - 1;
    expect(subtract.addInequality(toExpr(complement)) == RowError::None, "not s");
    const Decision piece = subtract.checkInteger(bounded());
    expect(certified(subtract.system(), piece, "a piece of A - S"),
           "the subtraction finishes");
    expect(subtract.pop(), "and rolls back");
  }

  // rollback_false_nonempty.cpp and rollback_bad_sample.cpp, over the integers.
  Session rolled(2);
  for (const Row &row : {Row{-1, 0, 0}, Row{0, -1, 0}, Row{-1, -1, 1}})
    expect(rolled.addInequality(toExpr(fromRow(row))) == RowError::None, "base");
  const Decision before = rolled.checkInteger(bounded());
  expect(before.provedFeasible(), "the base holds the origin");
  rolled.push();
  for (const Row &row : {Row{1, 0, -1}, Row{-1, 1, -1}})
    expect(rolled.addInequality(toExpr(fromRow(row))) == RowError::None, "rows");
  expect(rolled.checkInteger(bounded()).provedInfeasible(), "they empty it");
  expect(rolled.pop(), "a rollback past empty");
  const Decision after = rolled.checkInteger(bounded());
  expect(after.provedFeasible(), "the base is feasible again");
  certified(rolled.system(), after, "a point inside the base");
  expect(rolled.addInequality(toExpr(fromRow({0, 1, -1}))) == RowError::None, "y");
  const Decision empty = rolled.checkInteger(bounded());
  expect(empty.provedInfeasible(), "y <= 0 and y >= 1, after the rollback too");
  certified(rolled.system(), empty, "after the rollback");

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
// Sessions
//===----------------------------------------------------------------------===//

void testSessionBasics() {
  Session session(2);
  expect(session.addCongruence(linearExpr({1}, 0), Int(2)) ==
             RowError::WrongCoefficientCount,
         "a congruence of the wrong size");
  expect(session.addCongruence(linearExpr({1, 0}, 0), Int(0)) ==
             RowError::NonPositiveModulus,
         "a modulus of 0");
  expect(session.addCongruence(linearExpr({1, 0}, 0), Int(-2)) ==
             RowError::NonPositiveModulus,
         "a negative modulus");
  expect(session.system().numRows() == 0, "leaving no row");

  // Repeated queries come from the cache, and through the checker.
  expect(session.addInequality(linearExpr({1, 1}, -1)) == RowError::None, "row");
  const Decision first = session.checkInteger(bounded());
  expect(first.provedFeasible() && first.receipt().ranSearch, "searched once");
  const std::string state = session.stateText();
  const std::string text = canonicalText(first);
  for (int i = 0; i < 5; ++i) {
    const Decision again = session.checkInteger(bounded());
    expect(canonicalText(again) == text, "a repeated query gives the same answer");
    expect(!again.receipt().ranSearch && again.receipt().ranChecker,
           "from the cache, through the checker");
    expect(session.stateText() == state, "and changes nothing");
  }

  // The two questions are apart, and each keeps its own answer.
  const Decision rational = session.check(bounded());
  expect(rational.provedFeasible() && rational.domain() == Domain::Rational,
         "the rational question");
  expect(session.checkInteger(bounded()).domain() == Domain::Integer,
         "the integer question");

  // A row drops a witness, and leaves a proof.
  expect(session.addInequality(linearExpr({-2, -2}, 1)) == RowError::None, "2x + 2y <= 1");
  expect(session.stateText().find("integer feasible") == std::string::npos,
         "the witness is gone");
  const Decision second = session.checkInteger(bounded());
  expect(second.provedInfeasible(), "1 <= x + y and 2x + 2y <= 1");
  certified(session.system(), second, "with the new row");
  expect(session.addInequality(linearExpr({1, 0}, 0)) == RowError::None, "x >= 0");
  const Decision third = session.checkInteger(bounded());
  expect(third.provedInfeasible() && !third.receipt().ranSearch,
         "the proof stays through another row");
  certified(session.system(), third, "and still checks");

  // A congruence adds a column, so a proof does not survive one.
  Session columns(1);
  expect(columns.addEquality(linearExpr({2}, -1)) == RowError::None, "2x = 1");
  expect(columns.checkInteger(bounded()).provedInfeasible(), "no integer");
  expect(columns.addCongruence(linearExpr({1}, 0), Int(3)) == RowError::None, "cong");
  const Decision wider = columns.checkInteger(bounded());
  expect(wider.provedInfeasible() && wider.receipt().ranSearch,
         "searched again, over one more column");
  certified(columns.system(), wider, "with the wider proof");

  // A congruence stops the rational question, and a pop brings it back.
  Session mixed(1);
  expect(mixed.addInequality(linearExpr({1}, -1)) == RowError::None, "x >= 1");
  expect(mixed.check(bounded()).provedFeasible(), "rationally feasible");
  mixed.push();
  expect(mixed.addCongruence(linearExpr({1}, 0), Int(2)) == RowError::None, "even");
  expect(mixed.check(bounded()).why() == UnknownReason::UnsupportedConstruct,
         "a congruence has no rational meaning, also over a cached answer");
  expect(mixed.checkInteger(bounded()).provedFeasible(), "x = 2");
  expect(mixed.pop(), "out of the scope");
  expect(mixed.check(bounded()).provedFeasible(), "the rational answer is back");

  // An empty rational proof with a congruence on top is not an answer either.
  Session emptied(1);
  expect(emptied.addInequality(linearExpr({0}, -1)) == RowError::None, "false");
  expect(emptied.check(bounded()).provedInfeasible(), "empty");
  expect(emptied.addCongruence(linearExpr({1}, 0), Int(2)) == RowError::None, "cong");
  expect(emptied.check(bounded()).why() == UnknownReason::UnsupportedConstruct,
         "not over the rationals once a congruence is in");
  expect(emptied.checkInteger(bounded()).provedInfeasible(), "and empty over Z");

  // Scopes restore the integer answer exactly.
  Session scoped(2);
  expect(scoped.addInequality(linearExpr({2, 3}, -1)) == RowError::None, "base");
  expect(scoped.checkInteger(bounded()).provedFeasible(), "base");
  const std::string base = scoped.stateText();
  scoped.push();
  expect(scoped.stateText() == base, "a push changes nothing");
  expect(scoped.addEquality(linearExpr({2, 4}, -3)) == RowError::None, "2x + 4y = 3");
  expect(scoped.checkInteger(bounded()).provedInfeasible(), "empty in the scope");
  scoped.push();
  const std::string inner = scoped.stateText();
  expect(scoped.addCongruence(linearExpr({1, 0}, 0), Int(5)) == RowError::None, "c");
  expect(scoped.checkInteger(bounded()).provedInfeasible(), "empty deeper");
  expect(scoped.pop(), "out of the inner scope");
  expect(scoped.stateText() == inner, "the proof of that scope is back");
  expect(scoped.pop(), "out of the outer scope");
  expect(scoped.stateText() == base, "the witness of the base is back");
  expect(scoped.checkInteger(bounded()).provedFeasible(), "feasible again");
}

void testSessionScripts() {
  std::mt19937_64 rng(109);
  auto pick = [&](int lo, int hi) {
    return lo + int(rng() % std::uint64_t(hi - lo + 1));
  };
  unsigned long checks = 0, pops = 0, feasible = 0, infeasible = 0;
  unsigned maxDepth = 0;
  for (int trial = 0; trial < 1800; ++trial) {
    const unsigned n = pick(1, 3);
    Session session(n);
    RawSystem model = boxed(RawSystem{n, {}, {}, {}, {}}, 3);
    for (const Raw &row : model.ge)
      expect(session.addInequality(toExpr(row)) == RowError::None, "box");
    struct Saved {
      RawSystem model;
      std::string state;
    };
    std::vector<Saved> stack;
    const int steps = pick(5, 25);
    for (int s = 0; s < steps; ++s) {
      const int op = pick(0, 10);
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
          expect(session.addStrictInequality(toExpr(row)) == RowError::None, "gt");
        } else if (pick(0, 1) == 0) {
          model.eq.push_back(row);
          expect(session.addEquality(toExpr(row)) == RowError::None, "eq");
        } else {
          const int modulus = pick(1, 5);
          model.cong.push_back(RawCongruence{row, modulus});
          expect(session.addCongruence(toExpr(row), Int(modulus)) == RowError::None,
                 "cong");
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
        const Decision answer = session.checkInteger(bounded());
        const bool truth = bruteForce(model, 3);
        expect(truth ? answer.provedFeasible() : answer.provedInfeasible(),
               "the session agrees with every point of the box");
        certified(session.system(), answer, "script");
        expect(solveInteger(session.system(), bounded()).verdict() ==
                   answer.verdict(),
               "and a fresh solve agrees");
        (truth ? feasible : infeasible) += 1;
        ++checks;
      }
    }
  }
  std::cout << "session scripts: " << checks << " queries, " << pops
            << " pops, depth to " << maxDepth << "\n";
  expect(checks > 1000 && pops > 300 && maxDepth >= 4, "the scripts did work");
  expect(feasible > 100 && infeasible > 100, "both outcomes occurred");
}

//===----------------------------------------------------------------------===//
// Limits
//===----------------------------------------------------------------------===//

std::vector<System> budgetSystems() {
  std::vector<System> systems;
  systems.push_back(toSystem(pigeonhole(2)));
  systems.push_back(toSystem(pigeonhole(3)));
  systems.push_back(toSystem(coinProblem({6, 9, 20}, 43)));
  systems.push_back(toSystem(coinProblem({6, 9, 20}, 44)));
  systems.push_back(toSystem(inequalities(3, hang7)));
  return systems;
}

void testBudgets() {
  unsigned stoppedByNodes = 0, stoppedByBits = 0, stoppedByMemory = 0;
  for (const System &system : budgetSystems()) {
    const Decision open = solved(system);
    expect(certified(system, open, "budget"), "with enough budget it is decided");
    const std::string answer = canonicalText(open);
    const std::uint64_t steps = open.receipt().steps;
    const std::uint64_t nodes = open.receipt().nodes;

    WorkLimits exactNodes = bounded();
    exactNodes.maxBranchNodes = nodes;
    expect(canonicalText(solveInteger(system, exactNodes)) == answer,
           "exactly enough nodes");
    for (std::uint64_t limit = 0; limit < nodes; limit += 1 + nodes / 50) {
      WorkLimits few = bounded();
      few.maxBranchNodes = limit;
      Session session{System(system)};
      const std::string before = session.stateText();
      const Decision stopped = session.checkInteger(few);
      expect(stopped.isUnknown() && stopped.why() == UnknownReason::NodeLimit,
             "too few nodes is Unknown");
      expect(!stopped.integerWitness() && !stopped.branchProof(),
             "and carries nothing");
      expect(stopped.receipt().nodes == limit, "the receipt counts them");
      expect(session.stateText() == before, "a stopped query changes nothing");
      expect(canonicalText(session.checkInteger(bounded())) == answer,
             "and a retry gets the answer");
      ++stoppedByNodes;
    }

    WorkLimits exactSteps = bounded();
    exactSteps.maxSteps = steps;
    expect(canonicalText(solveInteger(system, exactSteps)) == answer,
           "exactly enough steps");
    for (std::uint64_t limit = 0; limit < steps; limit += 1 + steps / 100) {
      WorkLimits few = bounded();
      few.maxSteps = limit;
      CancellationToken watch;
      const Decision stopped = solveInteger(system, few, &watch);
      expect(stopped.isUnknown() && stopped.why() == UnknownReason::StepLimit,
             "too few steps is Unknown");
      expect(stopped.receipt().steps <= limit, "and stays within them");
      // A relaxation adds one charge of its own to the steps it spent, and every
      // other charge is at least a step. Its budget is what is left of the query.
      expect(watch.pollCount() <= limit + 4 * stopped.receipt().nodes + 8,
             "a relaxation gets only what is left");
    }

    // Every cancellation point, sampled.
    CancellationToken counter;
    expect(canonicalText(solveInteger(system, bounded(), &counter)) == answer,
           "a token that never fires");
    const std::uint64_t polls = counter.pollCount();
    expect(polls > 0, "the search polls");
    for (std::uint64_t k = 0; k < polls; k += 1 + polls / 100) {
      CancellationToken token;
      token.cancelAfterPolls(k);
      Session session{System(system)};
      const std::string before = session.stateText();
      const Decision cancelled = session.checkInteger(bounded(), &token);
      expect(cancelled.isUnknown() && cancelled.why() == UnknownReason::Cancelled,
             "cancelled is Unknown");
      expect(session.stateText() == before, "and changes nothing");
    }

    // Bits: Unknown until the limit is enough, and then the same answer.
    bool decided = false;
    for (unsigned bits = 1; bits <= 64; ++bits) {
      WorkLimits limited = bounded();
      limited.maxCoefficientBits = bits;
      const Decision tried = solveInteger(system, limited);
      if (tried.isUnknown()) {
        expect(tried.why() == UnknownReason::BitLimit, "too few bits");
        expect(!decided, "more bits never lose an answer");
        ++stoppedByBits;
      } else {
        expect(canonicalText(tried) == answer, "the same answer");
        expect(numbersFit(tried, bits), "and every number of it is in the limit");
        decided = true;
      }
    }
    expect(decided, "enough bits give the answer");

    // Memory: Unknown until the limit is enough.
    bool fit = false;
    for (std::uint64_t cells = 1; cells <= (1 << 22); cells *= 2) {
      WorkLimits limited = bounded();
      limited.maxSearchCells = cells;
      const Decision tried = solveInteger(system, limited);
      if (tried.isUnknown()) {
        expect(tried.why() == UnknownReason::MemoryLimit, "too little memory");
        expect(!fit, "more memory never loses an answer");
        ++stoppedByMemory;
      } else {
        expect(canonicalText(tried) == answer, "the same answer");
        fit = true;
      }
    }
    expect(fit, "enough memory gives the answer");
  }
  expect(stoppedByNodes > 10 && stoppedByBits > 5 && stoppedByMemory > 5,
         "the sweeps did work");

  // Size limits, at the exact boundary. Two rows and one congruence: three rows
  // of the system, over two variables and one quotient.
  const RawSystem raw = {2, {{{1, 1}, -1}, {{1, -1}, 0}}, {}, {}, {{{{1, 0}, 0}, 2}}};
  const System three = toSystem(raw);
  WorkLimits rows = bounded();
  rows.maxRows = 3;
  expect(solveInteger(three, rows).provedFeasible(), "exactly at the row limit");
  rows.maxRows = 2;
  expect(solveInteger(three, rows).why() == UnknownReason::RowLimit, "over it");
  WorkLimits vars = bounded();
  vars.maxVariables = 3;
  expect(solveInteger(three, vars).provedFeasible(), "columns at the limit");
  vars.maxVariables = 2;
  expect(solveInteger(three, vars).why() == UnknownReason::VariableLimit,
         "a quotient counts as a column");
  WorkLimits cells = bounded();
  cells.maxTableauCells = 18; // (2 rows + 1 equality) * 3 + 3 * 3
  expect(solveInteger(three, cells).provedFeasible(), "at the working limit");
  cells.maxTableauCells = 17;
  expect(solveInteger(three, cells).why() == UnknownReason::TableauLimit,
         "the limit is read before the lattice allocates");
  WorkLimits huge;
  huge.maxSteps = UINT64_MAX;
  huge.maxSearchCells = UINT64_MAX;
  huge.maxBranchNodes = UINT64_MAX;
  huge.maxTableauCells = UINT64_MAX;
  huge.maxSessionCells = UINT64_MAX;
  expect(solveInteger(three, huge).provedFeasible(), "the largest limits");

  // A long number counts by its digits, so that a cell means the same in bytes.
  auto twoRows = [](const Int &value) {
    LinearExpr first, second;
    first.coefficients = {Int(1), value};
    second.coefficients = {Int(1), Int(1)};
    System system = System::ofSize(2);
    expect(system.addInequality(first) == RowError::None, "add");
    expect(system.addInequality(second) == RowError::None, "add");
    return system;
  };
  Int longValue(1);
  for (int bit = 0; bit < 4096; ++bit)
    longValue *= Int(2);
  longValue += Int(1); // 4097 bits, which is 17 cells, and 22 for the two rows
  WorkLimits digits = bounded();
  digits.maxSearchCells = 6;
  expect(solveInteger(twoRows(Int(3)), digits).provedFeasible(),
         "short numbers take a cell each");
  digits.maxSearchCells = 21;
  expect(solveInteger(twoRows(longValue), digits).why() ==
             UnknownReason::MemoryLimit,
         "a long number takes more");
  digits.maxSearchCells = 22;
  expect(solveInteger(twoRows(longValue), digits).provedFeasible(),
         "exactly what it holds");

  expect(std::string(unknownReasonName(UnknownReason::NodeLimit)) == "node-limit",
         "named");
  expect(std::string(unknownReasonName(UnknownReason::MemoryLimit)) ==
             "memory-limit",
         "named");

  // A cached answer needs no budget.
  Session session{System(three)};
  expect(session.checkInteger(bounded()).provedFeasible(), "cached");
  WorkLimits none;
  none.maxSteps = 0;
  none.maxRows = 0;
  none.maxVariables = 0;
  none.maxBranchNodes = 0;
  none.maxSearchCells = 0;
  expect(session.checkInteger(none).provedFeasible(), "the cache takes no work");
}

/// The limits on a session, which are read before anything is copied.
void testSessionMemory() {
  // A row is 10 units here: 2 variables and 8. A state of no rows is 20.
  Session session(2);
  expect(session.heldCells() == 20, "the size of an empty state, by hand");
  session.setMemoryLimit(40);
  expect(session.addInequality(linearExpr({1, 0}, 0)) == RowError::None, "30");
  expect(session.addInequality(linearExpr({0, 1}, 0)) == RowError::None, "40");
  const std::string full = session.stateText();
  expect(session.addInequality(linearExpr({1, 1}, 0)) == RowError::SessionLimit,
         "one more row would pass the limit");
  expect(session.addStrictInequality(linearExpr({1, 1}, 0)) ==
             RowError::SessionLimit,
         "the same for a strict row");
  expect(session.addEquality(linearExpr({1, 1}, 0)) == RowError::SessionLimit,
         "and an equality");
  expect(session.addCongruence(linearExpr({1, 1}, 0), Int(2)) ==
             RowError::SessionLimit,
         "and a congruence");
  expect(session.stateText() == full && session.heldCells() == 40,
         "a refused row changes nothing");

  // In a scope the row also costs the copy of the state it would cause.
  session.setMemoryLimit(89);
  session.push();
  expect(session.addInequality(linearExpr({1, 1}, 0)) == RowError::SessionLimit,
         "40 held, 40 for the copy, and 10 for the row");
  expect(session.stateText() == full, "and nothing was copied");
  session.setMemoryLimit(90);
  expect(session.addInequality(linearExpr({1, 1}, 0)) == RowError::None, "exactly");
  expect(session.heldCells() == 90, "the scope and the copy");
  expect(session.pop(), "closing the scope");
  expect(session.heldCells() == 40, "frees the copy");

  // A session of many variables costs no square of them up front.
  Session wide(4096);
  expect(wide.heldCells() == 4096 * 4104, "a state holds what it needs");
  const BoundsTableau &tableau = wide.tableau();
  expect(tableau.numVariables() == 4096, "every variable");
  const std::vector<Int> unit = tableau.form(4095);
  expect(unit.size() == 4096 && unit[4095] == Int(1) && unit[0] == Int(0),
         "and a variable's form is still there to read");
}

/// Two variables, whose cases walk along one form for over a thousand steps. Z3
/// says unsat too. A node that read every row of the path would need 17 times
/// these steps, since each step leaves one more.
void testLongWalk() {
  const RawSystem raw = {2,
                         {{{-649, -794}, 1178}, {{-72, 588}, 608}},
                         {{{668, 357}, -668}},
                         {},
                         {{{{-772, -1}, -1752}, 1},
                          {{{532, 43}, -2907}, 19},
                          {{{881, -72}, -2941}, 10}}};
  const System system = toSystem(raw);
  WorkLimits limits = bounded();
  limits.maxSteps = std::uint64_t(1) << 23;
  const Decision answer = solveInteger(system, limits);
  expect(answer.provedInfeasible(), "a long walk fits in a small budget");
  certified(system, answer, "a long walk");
  expect(answer.receipt().nodes > 2000, "and it is one");
}

/// What a kept proof holds, and that it goes when the proof does.
void testKeptProofMemory() {
  // x >= 1 and x <= 0 meet at no integer. Five steps: two inputs, each tightened,
  // and their sum, with two multipliers. That is 2 * 5 + 4 + 2 cells.
  Session session(1);
  expect(session.addInequality(linearExpr({1}, -1)) == RowError::None, "x >= 1");
  expect(session.addInequality(linearExpr({-1}, 0)) == RowError::None, "x <= 0");
  const std::uint64_t empty = session.heldCells();
  const Decision first = session.checkInteger(bounded());
  expect(first.provedInfeasible(), "the two rows");
  expect(first.branchProof()->nodes.size() == 1 &&
             first.branchProof()->nodes[0].leaf.steps.size() == 5,
         "the proof counted here");
  expect(session.heldCells() == empty + 16, "a kept proof counts");

  session.push();
  expect(session.addCongruence(linearExpr({1}, 0), Int(2)) == RowError::None,
         "a new quotient");
  expect(session.heldCells() == empty + 16 + empty + 1 + 8,
         "the scope keeps the proof, and the new state has none");
  expect(session.pop(), "back");
  expect(session.heldCells() == empty + 16, "and the proof is back");
}

Int powerOfTwo(unsigned bits) {
  Int value(1);
  for (unsigned bit = 0; bit < bits; ++bit)
    value *= Int(2);
  return value;
}

LinearExpr wideExpr(std::vector<Int> coefficients, Int constant) {
  LinearExpr expr;
  expr.coefficients = std::move(coefficients);
  expr.constant = std::move(constant);
  return expr;
}

/// The bit limit, on numbers that the input does not hold.
void testBitLimits() {
  auto limited = [](unsigned bits) {
    WorkLimits limits = bounded();
    limits.maxCoefficientBits = bits;
    return limits;
  };

  // x = 1 and y = 1, and x + y >= 3. Refuting that takes the two pivots at once,
  // and their product has 81 bits, though every input fits in 41.
  const Int p = powerOfTwo(40) + Int(1), q = powerOfTwo(40) + Int(3);
  System product = System::ofSize(2);
  expect(product.addEquality(wideExpr({p, Int(0)}, -p)) == RowError::None, "x");
  expect(product.addEquality(wideExpr({Int(0), q}, -q)) == RowError::None, "y");
  expect(product.addInequality(linearExpr({1, 1}, -3)) == RowError::None, "sum");
  expect(solveInteger(product, limited(64)).why() == UnknownReason::BitLimit,
         "a scale past the limit stops the proof");
  const Decision scaled = solveInteger(product, limited(128));
  expect(scaled.provedInfeasible() && numbersFit(scaled, 128),
         "with room it is proved");
  certified(product, scaled, "the scale");

  // x = 1 and y = 1 again, and a row that weighs x by 2^50. The scale is now 42
  // bits, and the multiplier of the second equality takes 91.
  const Int weight = powerOfTwo(50) + Int(1);
  System heavy = System::ofSize(2);
  expect(heavy.addEquality(linearExpr({3, 0}, -3)) == RowError::None, "x");
  expect(heavy.addEquality(wideExpr({Int(0), q}, -q)) == RowError::None, "y");
  expect(heavy.addInequality(wideExpr({weight, Int(1)}, -(weight + Int(2)))) ==
             RowError::None,
         "the row");
  expect(solveInteger(heavy, limited(64)).why() == UnknownReason::BitLimit,
         "a multiplier past the limit stops the proof");
  expect(solveInteger(heavy, limited(128)).provedInfeasible(), "with room");

  // x1 = 2^40 x2 and x2 >= 2^40: the point has 81 bits, and the search never
  // holds that value in any row.
  const Int k = powerOfTwo(40);
  System far = System::ofSize(2);
  expect(far.addEquality(wideExpr({Int(1), -k}, Int(0))) == RowError::None, "x1");
  expect(far.addInequality(wideExpr({Int(0), Int(1)}, -k)) == RowError::None,
         "x2");
  expect(solveInteger(far, limited(64)).why() == UnknownReason::BitLimit,
         "a witness past the limit is not given");
  const Decision reached = solveInteger(far, limited(128));
  expect(reached.provedFeasible() && numbersFit(reached, 128), "with room");

  // Every input fits in 7 bits, and so does the answer, but a row on the way to
  // it does not fit in 8.
  const RawSystem line = {
      3, {{{-89, -14, 32}, -2}, {{5, -2, 0}, -2}, {{63, 12, -24}, 5}}, {}, {}, {}};
  expect(solveInteger(toSystem(line), limited(8)).why() == UnknownReason::BitLimit,
         "a row on the way to an answer has a limit too");
  expect(!solveInteger(toSystem(line), limited(16)).isUnknown(), "with room");

  // Input past the limit stops before the search has a node.
  const Int wide = powerOfTwo(70);
  for (const System &system :
       {toSystem({1, {}, {}, {}, {}}), toSystem({1, {{{1}, 0}}, {}, {}, {}})}) {
    System coefficient = system, constant = system, modulus = system;
    expect(coefficient.addInequality(wideExpr({wide}, Int(0))) == RowError::None,
           "a long coefficient");
    expect(constant.addInequality(wideExpr({Int(1)}, wide)) == RowError::None,
           "a long constant");
    expect(modulus.addCongruence(wideExpr({Int(1)}, Int(0)), wide) ==
               RowError::None,
           "a long modulus");
    for (const System &input : {coefficient, constant, modulus}) {
      const Decision stopped = solveInteger(input, limited(64));
      expect(stopped.why() == UnknownReason::BitLimit, "over the limit");
      expect(stopped.receipt().nodes == 0 && !stopped.receipt().ranSearch,
             "and no node was reached");
    }
  }
}

/// The least memory a system can be decided in, found by trying.
std::uint64_t leastCells(const System &system) {
  for (std::uint64_t cells = 0;; ++cells) {
    WorkLimits limits = bounded();
    limits.maxSearchCells = cells;
    const Decision answer = solveInteger(system, limits);
    if (!answer.isUnknown())
      return cells;
    expect(answer.why() == UnknownReason::MemoryLimit, "memory is what stopped");
  }
}

/// What a search keeps, to the cell: the input, every node of the proof, and the
/// rows of the open cases, which go when their cases close.
void testExactMemory() {
  const RawSystem oneSplit = {
      2, {{{3, -7}, 2}, {{-3, 7}, 4}, {{-1, -4}, -2}, {{3, 2}, 1}}, {}, {}, {}};
  expect(solved(toSystem(oneSplit)).receipt().nodes == 3, "one split");
  // Four rows of 3 cells is 12. The split holds its 2 coefficients, its bound
  // and 3 more, and the row of the open case the same, so 6 and 6. The first
  // leaf has 11 steps and 6 multipliers, so 2 * 11 + 6 + 4 is 32, and the second
  // has 7 steps and 3 multipliers, so 21. That is 77, counted from its proof.
  expect(leastCells(toSystem(oneSplit)) == 77, "one split, by hand");
  const RawSystem coordinate = {
      3, {{{1, 0, 0}, 0}, {{0, 1, 0}, 0}, {{0, 0, 1}, 0}}, {}, {{{7, 3, 2}, -1}}, {}};
  // 16 for the rows, 7 and 7 for the split and its case, and 38 and 33 for the
  // leaves, which have 13 steps and 8 multipliers, and 11 and 7.
  expect(leastCells(toSystem(coordinate)) == 101, "another, by hand");
  expect(leastCells(toSystem(pigeonhole(2))) == 269, "and a larger one, measured");
}

/// Budget.h promises that a system under given limits stops at the same point on
/// every machine, so the steps of a query are part of the contract. These were
/// measured, not derived. A change here changes what every limit means, so it
/// should be a deliberate one, and it is not by itself a defect.
void testStepCounts() {
  const std::vector<std::pair<RawSystem, std::uint64_t>> table = {
      {{1, {{{1}, 0}}, {}, {}, {}}, 21},
      {{1, {{{1}, 1}, {{-1}, 0}}, {}, {}, {}}, 31},
      {{1, {{{2}, -1}, {{-2}, 3}}, {}, {}, {}}, 34},
      {{2, {{{3, -7}, 2}, {{-3, 7}, 4}, {{-1, -4}, -2}, {{3, 2}, 1}}, {}, {}, {}},
       511},
      {{3,
        {{{1, 0, 0}, 0}, {{0, 1, 0}, 0}, {{0, 0, 1}, 0}},
        {},
        {{{7, 3, 2}, -1}},
        {}},
       786},
      {{2, {{{1, 0}, 0}, {{-1, 0}, 5}}, {}, {}, {{{{1, 1}, 1}, 3}}}, 113},
      {pigeonhole(2), 12540},
      {pinChain(8), 13950},
      // Two wide two-sided rows, so there is nothing for a cone query to ask.
      {{2, {{{5, 7}, -1}, {{-5, -7}, 10}, {{7, 9}, -2}, {{-7, -9}, 11}}, {}, {}, {}},
       1453},
      // The same with a row of one side, which is what a cone query asks about.
      {{2,
        {{{5, 7}, -1}, {{-5, -7}, 10}, {{7, 9}, -2}, {{-7, -9}, 11}, {{1, 3}, 100}},
        {},
        {},
        {}},
       1860},
  };
  for (const auto &entry : table)
    expect(solved(toSystem(entry.first)).receipt().steps == entry.second,
           "the steps of a query, as measured");
}

/// The limit of a session reaches an integer query too.
void testIntegerSessionLimit() {
  Session session(2);
  expect(session.addInequality(linearExpr({1, 0}, 0)) == RowError::None, "x");
  const std::string before = session.stateText();
  const std::uint64_t held = session.heldCells();
  WorkLimits limits = bounded();
  limits.maxSessionCells = 2 * held - 1;
  const Decision refused = session.checkInteger(limits);
  expect(refused.why() == UnknownReason::MemoryLimit,
         "the state and the copy that keeping an answer may make");
  expect(refused.receipt().nodes == 0, "and no search ran");
  expect(session.stateText() == before, "and nothing changed");
  limits.maxSessionCells = 2 * held;
  expect(session.checkInteger(limits).provedFeasible(), "exactly enough");

  // A modulus that cannot be is refused before it can touch a kept answer.
  const std::string kept = session.stateText();
  expect(session.addCongruence(linearExpr({1, 0}, 0), Int(0)) ==
             RowError::NonPositiveModulus,
         "modulus zero");
  expect(session.stateText() == kept, "and the answer stays");
}

/// What the receipt of a query says.
void testReceipts() {
  const System cycle = toSystem(pigeonhole(2));
  WorkLimits none = bounded();
  none.maxBranchNodes = 0;
  const Decision stopped = solveInteger(cycle, none);
  expect(stopped.why() == UnknownReason::NodeLimit, "no node allowed");
  expect(stopped.domain() == Domain::Integer, "an integer answer of Unknown");
  expect(!stopped.receipt().ranSearch && !stopped.receipt().ranTableau,
         "and no stage is claimed");
  expect(canonicalText(stopped).find("search") == std::string::npos,
         "in the text either");

  const RuntimeEvidence before = runtimeEvidence();
  const Decision answer = solved(cycle);
  const RuntimeEvidence after = runtimeEvidence();
  expect(answer.receipt().pivots > 0 &&
             answer.receipt().pivots ==
                 after.tableauPivots - before.tableauPivots,
         "the pivots of every relaxation are on the receipt");
  expect(canonicalText(answer).find("exact-branch-proof 1") != std::string::npos,
         "the text of an answer holds its proof");

  // A relaxation has only what is left of the query. The first of this system
  // starts about 90 steps in and needs about 70, so with 104 it stops before its
  // first pivot, though 104 would be plenty on its own.
  const System oneSplit = toSystem(
      {2, {{{3, -7}, 2}, {{-3, 7}, 4}, {{-1, -4}, -2}, {{3, 2}, 1}}, {}, {}, {}});
  WorkLimits few = bounded();
  few.maxSteps = 104;
  const Decision cut = solveInteger(oneSplit, few);
  expect(cut.why() == UnknownReason::StepLimit && cut.receipt().pivots == 0,
         "a relaxation gets only what is left");
}

/// Rules of the search that no answer depends on, and that each pay for
/// themselves on some system.
void testStrategy() {
  // x in [-1, 0]. Its rational point need not be integral, and rounding it
  // answers with no split.
  const RawSystem rounding = {1, {{{1}, 1}, {{-1}, 0}}, {}, {}, {}};
  expect(solved(toSystem(rounding)).receipt().nodes == 1, "rounding needs no split");

  // No row is bounded here, so the point comes from moving every row in by half
  // its norm. That only keeps the rows if rounding is to the nearest integer.
  const RawSystem deflated = {
      2, {{{0, 3}, -1}, {{2147483647, -1}, -4294967296}}, {}, {}, {}};
  expectAnswer(deflated, Want::Feasible, "nearest rounding keeps a deflated point");

  // A triangle with no integer point, under a row that is not bounded. Walking
  // along that row would never end, so it must not be taken for a bounded one.
  const RawSystem triangle = {3,
                              {{{2, 3, 0}, -2},
                               {{-3, 1, 0}, 2},
                               {{1, -4, 0}, 2},
                               {{1, 0, 3}, -1}},
                              {},
                              {},
                              {}};
  expectAnswer(triangle, Want::Infeasible, "the row above the triangle is free");

  // A thin row that is already at an integer has nothing to give. Splitting on it
  // here takes 15 nodes instead of 3.
  const RawSystem settled = {3,
                             {{{1, -8, 0}, 5},
                              {{-1, 8, 0}, 2},
                              {{-2, 1, -9}, 4},
                              {{2, -1, 9}, 4}},
                             {},
                             {{{-3, 0, -1}, 7}},
                             {{{{-4, -4, 0}, -4}, 5}}};
  expect(solved(toSystem(settled)).receipt().nodes == 3,
         "a thin row at an integer is left alone");
}

/// A valid proof 100,000 splits deep. The checker has to walk it with a stack of
/// its own: a call per level would not fit in the default stack. Every leaf
/// refutes the system outright, so the cases are not needed, and the proof is
/// still valid, since the checker only asks that every case closes.
void testDeepProof() {
  const System system = toSystem({1, {}, {}, {{{2}, -1}}, {}});
  InfeasibilityProof leaf;
  leaf.steps.push_back(ProofStep::input({RowRef::Kind::Equality, 0, false}));
  leaf.steps.push_back(ProofStep::input({RowRef::Kind::Equality, 0, true}));
  leaf.steps.push_back(ProofStep::tighten(0));
  leaf.steps.push_back(ProofStep::tighten(1));
  leaf.steps.push_back(ProofStep::combine({{2, Int(1)}, {3, Int(1)}}));

  const std::size_t depth = 100000;
  BranchProof proof;
  proof.nodes.resize(2 * depth + 1);
  for (std::size_t k = 0; k < depth; ++k) {
    BranchNode &split = proof.nodes[2 * k];
    split.kind = BranchNode::Kind::Split;
    split.form = {Int(1)};
    split.bound = Int(static_cast<std::int64_t>(k));
    split.low = 2 * k + 1;
    split.high = 2 * k + 2;
    proof.nodes[2 * k + 1].leaf = leaf;
  }
  proof.nodes[2 * depth].leaf = leaf;
  // This file's own validator recurses, so it reads the same proof cut short.
  expect(checkBranchProof(system, proof).accepted, "a deep proof is walked");

  proof.nodes[2 * depth].leaf.steps.pop_back();
  expect(!checkBranchProof(system, proof).accepted, "its last leaf is still read");
}

//===----------------------------------------------------------------------===//
// The certificates themselves
//===----------------------------------------------------------------------===//

/// Damages valid proofs and witnesses in every way this can think of. The
/// checker must accept a damaged one exactly when this file's reference does.
void testCertificateMutation() {
  unsigned long refused = 0, stillValid = 0;
  auto compare = [&](const System &system, const BranchProof &proof) {
    const bool valid = !proof.nodes.empty() && branchProofIsValid(system, proof);
    const bool accepted = checkBranchProof(system, proof).accepted;
    expect(accepted == valid, "the checker agrees about a damaged proof");
    (valid ? stillValid : refused) += 1;
  };
  auto compareWitness = [&](const System &system, const std::vector<Int> &x) {
    const bool valid = pointIsMember(system, x);
    expect(checkWitness(system, IntegerWitness{x}).accepted == valid,
           "the checker agrees about a damaged witness");
    expect(certifyFeasible(system, IntegerWitness{x}).provedFeasible() == valid,
           "and so does the gate");
    (valid ? stillValid : refused) += 1;
  };

  auto damage = [&](const System &system, const BranchProof &proof) {
    compare(system, proof);
    for (std::size_t i = 0; i < proof.nodes.size(); ++i) {
      const BranchNode &node = proof.nodes[i];
      if (node.kind == BranchNode::Kind::Split) {
        for (const Int &shift : {Int(1), Int(-1), Int(2)}) {
          BranchProof moved = proof;
          moved.nodes[i].bound += shift;
          compare(system, moved);
        }
        for (std::size_t j = 0; j < node.form.size(); ++j) {
          BranchProof bent = proof;
          bent.nodes[i].form[j] += Int(1);
          compare(system, bent);
        }
        BranchProof swapped = proof;
        std::swap(swapped.nodes[i].low, swapped.nodes[i].high);
        compare(system, swapped);
        BranchProof shared = proof;
        shared.nodes[i].high = shared.nodes[i].low;
        compare(system, shared);
        BranchProof collapsed = proof;
        collapsed.nodes[i].kind = BranchNode::Kind::Leaf;
        compare(system, collapsed);
        continue;
      }
      const std::size_t last = node.leaf.steps.size() - 1;
      for (std::size_t s = 0; s <= last; ++s) {
        const ProofStep &step = node.leaf.steps[s];
        if (step.rule == ProofStep::Rule::Input) {
          BranchProof flipped = proof;
          flipped.nodes[i].leaf.steps[s].row.negated = !step.row.negated;
          compare(system, flipped);
          BranchProof moved = proof;
          moved.nodes[i].leaf.steps[s].row.index += 1;
          compare(system, moved);
          BranchProof retyped = proof;
          retyped.nodes[i].leaf.steps[s].row.kind = RowRef::Kind::Assumption;
          compare(system, retyped);
        } else if (step.rule == ProofStep::Rule::Combine) {
          for (std::size_t t = 0; t < step.terms.size(); ++t) {
            BranchProof bumped = proof;
            bumped.nodes[i].leaf.steps[s].terms[t].multiplier += Int(1);
            compare(system, bumped);
            BranchProof zeroed = proof;
            zeroed.nodes[i].leaf.steps[s].terms[t].multiplier = Int(0);
            compare(system, zeroed);
            BranchProof dropped = proof;
            auto &terms = dropped.nodes[i].leaf.steps[s].terms;
            terms.erase(terms.begin() + t);
            compare(system, dropped);
          }
        } else {
          BranchProof turned = proof;
          turned.nodes[i].leaf.steps[s].rule =
              step.rule == ProofStep::Rule::Tighten ? ProofStep::Rule::Harden
                                                    : ProofStep::Rule::Tighten;
          compare(system, turned);
        }
      }
      BranchProof cut = proof;
      cut.nodes[i].leaf.steps.pop_back();
      compare(system, cut);
    }
    BranchProof shortened = proof;
    shortened.nodes.pop_back();
    compare(system, shortened);
    BranchProof longer = proof;
    longer.nodes.push_back(proof.nodes.back());
    compare(system, longer);
  };

  std::mt19937_64 rng(113);
  auto pick = [&](int lo, int hi) {
    return lo + int(rng() % std::uint64_t(hi - lo + 1));
  };
  unsigned withSplits = 0;
  for (int trial = 0; trial < 700; ++trial) {
    RawSystem raw;
    raw.n = pick(2, 4);
    const int rows = pick(2, 6);
    for (int r = 0; r < rows; ++r) {
      Raw row;
      for (unsigned j = 0; j < raw.n; ++j)
        row.a.push_back(pick(-5, 5));
      row.c = pick(-8, 8);
      const int kind = pick(0, 9);
      if (kind < 5)
        raw.ge.push_back(row);
      else if (kind < 7)
        raw.gt.push_back(row);
      else if (kind < 9)
        raw.eq.push_back(row);
      else
        raw.cong.push_back(RawCongruence{row, pick(2, 5)});
    }
    if (trial % 2 == 0)
      raw = boxed(raw, 2);
    const System system = toSystem(raw);
    const Decision answer = solved(system);
    if (const IntegerWitness *witness = answer.integerWitness()) {
      compareWitness(system, witness->values);
      for (unsigned j = 0; j < raw.n; ++j) {
        std::vector<Int> point = witness->values;
        point[j] += Int(1);
        compareWitness(system, point);
        point = witness->values;
        point[j] -= Int(3);
        compareWitness(system, point);
      }
      std::vector<Int> shortPoint = witness->values;
      shortPoint.pop_back();
      compareWitness(system, shortPoint);
      continue;
    }
    if (const BranchProof *proof = answer.branchProof()) {
      withSplits += proof->nodes.size() > 1;
      if (proof->nodes.size() <= 24)
        damage(system, *proof);
    }
  }
  // Proofs with real splits, by construction.
  for (const System &system : budgetSystems()) {
    const Decision answer = solved(system);
    if (const BranchProof *proof = answer.branchProof()) {
      withSplits += proof->nodes.size() > 1;
      if (proof->nodes.size() <= 40)
        damage(system, *proof);
    }
  }
  std::cout << "certificate mutation: " << refused << " refused, " << stillValid
            << " still valid, " << withSplits << " proofs with splits\n";
  expect(refused > 5000 && stillValid > 100 && withSplits > 3,
         "both kinds occurred");

  // A bad proof reaches no one through the gate.
  const System half = toSystem({1, {{{2}, -1}, {{-2}, 1}}, {}, {}, {}});
  BranchProof bad = halfProof();
  bad.nodes[0].bound = Int(7);
  const Decision refusedProof = certifyInfeasible(half, bad);
  expect(refusedProof.isUnknown() &&
             refusedProof.why() == UnknownReason::CertificateRejected,
         "the gate turns a bad proof into Unknown");
}

//===----------------------------------------------------------------------===//
// Provenance and determinism
//===----------------------------------------------------------------------===//

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
  expect(declared.integerRevision == "S3", "this is the S3 search");
  expect(declared.llvmDependency == "ARITHMETIC_ONLY", "LLVM arithmetic only");

  const RuntimeEvidence quiet = runtimeEvidence();
  const std::string report = provenanceJson();
  const RuntimeEvidence same = runtimeEvidence();
  expect(same.integerChecks == quiet.integerChecks &&
             same.branchNodes == quiet.branchNodes,
         "printing moves no counter");
  expect(report.find("\"integer_checks\"") != std::string::npos &&
             report.find("\"branch_nodes\"") != std::string::npos,
         "the report has the S3 counters");
  expect(report.find("\"integer_revision\": \"S3\"") != std::string::npos,
         "and the revision");

  // 2x - 1 >= 0 and -2x + 3 >= 0, so x is 1, after tightening: both rows meet,
  // which is a pinned equality, and no rational solve or split is left to do.
  const System one = toSystem({1, {{{2}, -1}, {{-2}, 3}}, {}, {}, {}});
  const Decision pinned = solved(one);
  expect(pinned.provedFeasible() && pinned.receipt().nodes == 1,
         "one node, by hand");
  expect(pinned.integerWitness()->values == std::vector<Int>({Int(1)}),
         "x = 1, by hand");
  expect(pinned.receipt().ranSearch && pinned.receipt().ranChecker,
         "the receipt says search and checker");

  // Three pigeons in two holes: the rational point is all fractions, rounding
  // it breaks a row, so the root splits and each case closes.
  const System cycle = toSystem(pigeonhole(2));
  const RuntimeEvidence before = runtimeEvidence();
  const Decision answer = solved(cycle);
  const RuntimeEvidence after = runtimeEvidence();
  expect(answer.provedInfeasible(), "empty");
  expect(after.integerChecks == before.integerChecks + 1, "one integer check");
  expect(after.branchNodes == before.branchNodes + answer.receipt().nodes,
         "the nodes on the receipt are the nodes counted");
  expect(answer.receipt().nodes >= 3, "the root split");
  expect(after.rationalChecks > before.rationalChecks, "the rational solver ran");
  expect(after.checkerRuns > before.checkerRuns, "the checker ran");
  expect(after.decideCalls == before.decideCalls, "decide() did not");
  expect(answer.receipt().ranSearch && answer.receipt().ranTableau &&
             answer.receipt().ranLattice && answer.receipt().ranNormalize &&
             answer.receipt().ranChecker,
         "the receipt says every stage");

  const std::string text = receiptText(cycle, answer);
  expect(text.find("stages=lattice,normalize,tableau,search,checker\n") !=
             std::string::npos,
         "the receipt text names them");
  expect(text.find("nodes=" + std::to_string(answer.receipt().nodes) + "\n") !=
             std::string::npos,
         "and the nodes");
  expect(answer.receipt().digest == ownFnv1a(text), "the digest, recomputed here");

  // A Decision of the other solvers says so.
  const Decision rational = solveRational(cycle);
  expect(rational.provedFeasible() && !rational.receipt().ranSearch,
         "the rational answer ran no search, and differs");
  const Decision plain = decide(one, Domain::Integer);
  expect(!plain.receipt().ranSearch && plain.receipt().nodes == 0,
         "decide() ran no search");
  expect(receiptText(one, plain).find("nodes=") == std::string::npos,
         "and its receipt has no line for it");

  // A refused query ran nothing.
  WorkLimits none = bounded();
  none.maxRows = 0;
  const RuntimeEvidence s0 = runtimeEvidence();
  const Decision refused = solveInteger(cycle, none);
  const RuntimeEvidence s1 = runtimeEvidence();
  expect(refused.isUnknown() && s1.integerChecks == s0.integerChecks + 1,
         "a refused query is a query");
  expect(s1.branchNodes == s0.branchNodes && !refused.receipt().ranSearch,
         "that ran no node");
  expect(refused.receipt().digest == ownFnv1a(receiptText(cycle, refused)),
         "an Unknown has a receipt too");
}

void testDeterminism() {
  std::mt19937_64 rng(127);
  auto pick = [&](int lo, int hi) {
    return lo + int(rng() % std::uint64_t(hi - lo + 1));
  };
  for (int trial = 0; trial < 800; ++trial) {
    RawSystem raw;
    raw.n = pick(1, 5);
    const int rows = pick(1, 8);
    for (int r = 0; r < rows; ++r) {
      Raw row;
      for (unsigned j = 0; j < raw.n; ++j)
        row.a.push_back(pick(-5, 5));
      row.c = pick(-7, 7);
      const int kind = pick(0, 9);
      if (kind < 6)
        raw.ge.push_back(row);
      else if (kind < 8)
        raw.gt.push_back(row);
      else if (kind < 9)
        raw.eq.push_back(row);
      else
        raw.cong.push_back(RawCongruence{row, pick(1, 6)});
    }
    const System system = toSystem(raw);
    const Decision a = solved(system), b = solved(system);
    expect(canonicalText(a) == canonicalText(b), "the same answer and certificate");
    expect(a.receipt().digest == b.receipt().digest, "the same receipt");
    expect(a.receipt().steps == b.receipt().steps, "the same work");
  }
}

//===----------------------------------------------------------------------===//
// The Python oracle
//===----------------------------------------------------------------------===//

unsigned testIntegerVectors(const char *path) {
  std::ifstream in(path);
  std::string line;
  unsigned cases = 0, feasible = 0, infeasible = 0;
  while (std::getline(in, line)) {
    std::istringstream words(line);
    std::string truth;
    unsigned n, nge, ngt, neq, ncong;
    words >> truth >> n >> nge >> ngt >> neq >> ncong;
    System system = System::ofSize(n);
    auto next = [&] {
      LinearExpr expr;
      std::string word;
      for (unsigned j = 0; j < n; ++j) {
        words >> word;
        expr.coefficients.push_back(*parseDecimal(word));
      }
      words >> word;
      expr.constant = *parseDecimal(word);
      return expr;
    };
    for (unsigned r = 0; r < nge; ++r)
      expect(system.addInequality(next()) == RowError::None, "a vector row");
    for (unsigned r = 0; r < ngt; ++r)
      expect(system.addStrictInequality(next()) == RowError::None, "a vector row");
    for (unsigned r = 0; r < neq; ++r)
      expect(system.addEquality(next()) == RowError::None, "a vector row");
    for (unsigned r = 0; r < ncong; ++r) {
      LinearExpr expr = next();
      std::string word;
      words >> word;
      expect(system.addCongruence(std::move(expr), *parseDecimal(word)) ==
                 RowError::None,
             "a vector row");
    }
    const Decision answer = solved(system);
    expect(truth == "F" ? answer.provedFeasible() : answer.provedInfeasible(),
           "the solver agrees with every point Python tried");
    certified(system, answer, "vector");
    (truth == "F" ? feasible : infeasible) += 1;
    ++cases;
  }
  std::cout << "integer vectors: " << cases << " cases, " << feasible
            << " feasible, " << infeasible << " infeasible\n";
  return cases;
}

} // namespace

int main(int argc, char **argv) {
  if (argc == 3 && std::string(argv[1]) == "--integer-vectors") {
    const unsigned cases = testIntegerVectors(argv[2]);
    std::cout << "integer vectors checks: " << passed << " passed, " << failed
              << " failed\n";
    return failed != 0 || cases == 0;
  }
  if (argc == 2 && std::string(argv[1]) == "--print-provenance") {
    std::cout << provenanceJson();
    return 0;
  }
  if (argc == 2 &&
      std::string(argv[1]) == "--print-provenance-after-integer-query") {
    // This needs a split, so the search has to run.
    const Decision answer = solveInteger(toSystem(pigeonhole(2)));
    std::cout << provenanceJson();
    return answer.provedInfeasible() ? 0 : 1;
  }

  testCertificateRules();
  testHandSystems();
  testExhaustive();
  testRandomBounded();
  testPinChains();
  testUnbounded();
  testHugeValues();
  testLlvmReproducers();
  testSessionBasics();
  testSessionScripts();
  testBudgets();
  testSessionMemory();
  testLongWalk();
  testKeptProofMemory();
  testBitLimits();
  testExactMemory();
  testStepCounts();
  testDeepProof();
  testIntegerSessionLimit();
  testReceipts();
  testStrategy();
  testCertificateMutation();
  testProvenance();
  testDeterminism();

  std::cout << "exact integer solver: " << passed << " passed, " << failed
            << " failed\n";
  return failed != 0;
}
