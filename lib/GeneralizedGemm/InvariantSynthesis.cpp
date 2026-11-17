#include "GeneralizedGemm/InvariantSynthesis.h"

#include "GeneralizedGemm/TerminationObligations.h"

#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/DynamicAPInt.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/JSON.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <numeric>
#include <functional>
#include <map>
#include <set>

using namespace llvm;

namespace mlir::generalized_gemm {

namespace {

std::string decimal(const APInt &value) {
  SmallString<40> text;
  bool negative = value.getBitWidth() > 1 && value.isNegative();
  value.toString(text, 10, /*Signed=*/negative);
  return std::string(text);
}

/// A sample row as an entry and a configuration, for evaluateTerm. Rows are the
/// header variables, their poison bits, then the parameters (LoopSamples).
struct RowView {
  EntryState entry;
  Configuration config;
};

RowView rowView(const SemanticProgram &program, const InvariantLoop &loop, ArrayRef<APInt> row) {
  RowView view;
  size_t nv = loop.variables.size();
  view.config.state.assign(program.state.size(), std::nullopt);
  view.config.shadow.assign(program.state.size(), false);
  for (size_t i = 0; i < nv; ++i) {
    view.config.state[loop.variables[i]] = row[i];
    view.config.shadow[loop.variables[i]] = row[nv + i].isOne();
  }
  for (const Parameter &p : program.parameters)
    view.entry.parameters.push_back(APInt::getZero(p.sort.width));
  for (size_t j = 0; j < loop.parameters.size(); ++j)
    view.entry.parameters[loop.parameters[j]] = row[2 * nv + j];
  return view;
}

bool holds(const SemanticProgram &program, const RowView &view, Term atom) {
  auto value = evaluateTerm(program, view.entry, view.config, atom);
  return value && value->isOne();
}

/// Random rows at the same columns: header values and parameters drawn uniformly,
/// poison bits clear. They are the witnesses that an equality is not an identity.
std::vector<std::vector<APInt>> witnessRows(const SemanticProgram &program, const InvariantLoop &loop,
                                            unsigned count) {
  uint64_t state = 0xD1B54A32D192ED03ull;
  auto next = [&]() {
    state ^= state << 13;
    state ^= state >> 7;
    state ^= state << 17;
    return state;
  };
  std::vector<std::vector<APInt>> rows;
  for (unsigned r = 0; r < count; ++r) {
    std::vector<APInt> row;
    for (unsigned v : loop.variables) {
      unsigned w = program.state[v].sort.width;
      APInt value(w, 0);
      for (unsigned bit = 0; bit < w; bit += 64)
        value |= APInt(w, next(), false, /*implicitTrunc=*/true).shl(bit);
      row.push_back(value);
    }
    for (size_t i = 0; i < loop.variables.size(); ++i)
      row.push_back(APInt(1, 0));
    for (unsigned p : loop.parameters) {
      unsigned w = program.parameters[p].sort.width;
      APInt value(w, 0);
      for (unsigned bit = 0; bit < w; bit += 64)
        value |= APInt(w, next(), false, /*implicitTrunc=*/true).shl(bit);
      row.push_back(value);
    }
    rows.push_back(std::move(row));
  }
  return rows;
}

/// The template's leaves: header variables, then parameters, with their column.
struct Leaf {
  Term term;
  size_t column;
};

std::vector<Leaf> templateLeaves(const SemanticProgram &program, const InvariantLoop &loop) {
  TermContext &terms = *program.terms;
  std::vector<Leaf> leaves;
  size_t nv = loop.variables.size();
  for (size_t i = 0; i < nv; ++i)
    leaves.push_back({terms.state(loop.variables[i], program.state[loop.variables[i]].sort), i});
  for (size_t j = 0; j < loop.parameters.size(); ++j)
    leaves.push_back({terms.param(loop.parameters[j], program.parameters[loop.parameters[j]].sort),
                      2 * nv + j});
  return leaves;
}

/// Monomials of degree 1 to `degree` over `leaves` (one width), in order: the leaves,
/// then the products a·b with a ≤ b, then a·b·c with a ≤ b ≤ c.
void monomials(TermContext &terms, ArrayRef<Leaf> leaves, unsigned degree,
               ArrayRef<std::vector<APInt>> rows, std::vector<Term> &features,
               std::vector<std::vector<APInt>> &values) {
  auto column = [&](size_t c) {
    std::vector<APInt> out;
    for (const auto &row : rows)
      out.push_back(row[c]);
    return out;
  };
  auto times = [](std::vector<APInt> a, const std::vector<APInt> &b) {
    for (size_t r = 0; r < a.size(); ++r)
      a[r] *= b[r];
    return a;
  };
  for (const Leaf &leaf : leaves) {
    features.push_back(leaf.term);
    values.push_back(column(leaf.column));
  }
  if (degree >= 2)
    for (size_t a = 0; a < leaves.size(); ++a)
      for (size_t b = a; b < leaves.size(); ++b) {
        features.push_back(terms.binary(TermOp::Mul, leaves[a].term, leaves[b].term));
        values.push_back(times(column(leaves[a].column), column(leaves[b].column)));
      }
  if (degree >= 3)
    for (size_t a = 0; a < leaves.size(); ++a)
      for (size_t b = a; b < leaves.size(); ++b)
        for (size_t c = b; c < leaves.size(); ++c) {
          Term ab = terms.binary(TermOp::Mul, leaves[a].term, leaves[b].term);
          features.push_back(terms.binary(TermOp::Mul, ab, leaves[c].term));
          values.push_back(times(times(column(leaves[a].column), column(leaves[b].column)),
                                 column(leaves[c].column)));
        }
}

class Search {
public:
  Search(const SemanticProgram &program, const InvariantSearchOptions &options, SmtBackend &backend,
         LoopInvariantSearch &out)
      : program(program), options(options), backend(backend), terms(*program.terms), out(out) {}

  void run(const LoopFunction *given, const InvariantCertificate *givenCertificate);

private:
  void sample();
  bool refuteOnSamples(const LoopFunction &function);
  std::optional<LoopFunction> synthesizeFunction(std::string &why);
  void addPoint(const std::vector<APInt> &point);
  std::vector<APInt> smallInitialState(const std::vector<Term> &atoms,
                                       const std::vector<APInt> &model);
  std::vector<Term> equalities(unsigned degree, unsigned size);
  std::vector<Term> bounds();
  std::vector<Term> poisonAtoms();
  CheckedLoopInvariant check(const std::vector<Term> &atoms, const LoopFunction &function,
                             bool final);
  bool searchCertificate(const LoopFunction &function);
  void minimize(std::vector<Term> &atoms, const LoopFunction &function, const std::string &poison);
  void fallback(const LoopFunction &function);

  const SemanticProgram &program;
  const InvariantSearchOptions &options;
  SmtBackend &backend;
  TermContext &terms;
  LoopInvariantSearch &out;
  std::vector<std::vector<APInt>> witness;
  std::vector<std::vector<APInt>> points;
};

void Search::sample() {
  bool exhaustive = false;
  points = loopPoints(program, options.check.scope, options.samplePoints, exhaustive);
  out.samplePoints = points.size();
  out.samples = sampleLoop(program, out.loop, points, options.sampleRun, options.maxSamples);
  witness = witnessRows(program, out.loop, options.witnessRows);
}

void Search::addPoint(const std::vector<APInt> &point) {
  points.push_back(point);
  // One run adds at most 64 states, so a long run (a model with a large bound) cannot
  // take the whole sample budget and starve the next refinement. The run itself always
  // happens (sampleLoop skips a point when its cap is 0): its return value is what the
  // function CEGIS refits to, whether or not the state budget has room left.
  size_t room = options.maxSamples > out.samples.states.size()
                    ? std::min<size_t>(64, options.maxSamples - out.samples.states.size())
                    : 0;
  room = std::max<size_t>(room, 1);
  LoopSamples more = sampleLoop(program, out.loop, {point}, options.sampleRun, room);
  for (auto &row : more.states)
    if (out.samples.states.size() < options.maxSamples)
      out.samples.states.push_back(row);
  out.samples.runParameters.insert(out.samples.runParameters.end(), more.runParameters.begin(),
                                   more.runParameters.end());
  out.samples.runReturns.insert(out.samples.runReturns.end(), more.runReturns.begin(),
                                more.runReturns.end());
  out.samples.runs += more.runs;
  out.samples.returned += more.returned;
  out.samples.steps += more.steps;
  out.samples.arrivals += more.arrivals;
  out.samples.maxArrivals = std::max(out.samples.maxArrivals, more.maxArrivals);
  ++out.samplePoints;
}

/// The null space is taken over Q, so its rows must be states of runs that did not
/// wrap. A solver's INV-init model may have any parameters; when one is large, ask (as a
/// proposer: the answer is data, not a verdict) for an initial state that falsifies the
/// same conjunction with every parameter in [-64, 64]. Nothing found: the model as it is.
std::vector<APInt> Search::smallInitialState(const std::vector<Term> &atoms,
                                             const std::vector<APInt> &model) {
  bool small = true;
  for (const APInt &v : model)
    small = small && v.getSignificantBits() <= 7;
  if (small)
    return model;
  std::string why;
  Term scope = scopeFormula(program, options.check.scope, why);
  if (!scope)
    return model;
  Term inv = terms.boolean(true);
  for (Term atom : atoms)
    inv = terms.logicalAnd(inv, atom);
  Term bounded = terms.boolean(true);
  for (const Parameter &p : program.parameters) {
    unsigned w = p.sort.width;
    if (w < 8)
      continue;
    Term v = terms.param(p.id, p.sort);
    bounded = terms.logicalAnd(bounded,
                               terms.logicalAnd(terms.icmp(Predicate::Sge, v, terms.bv(APInt(w, -64, true))),
                                                terms.icmp(Predicate::Sle, v, terms.bv(APInt(w, 64, true)))));
  }
  for (unsigned id : out.loop.entry) {
    const CutPath &path = out.loop.set.paths[id];
    Term formula = terms.logicalAnd(invariantViolation(program, scope, path, terms.boolean(true), inv), bounded);
    SmtResponse response = backend.check(obligationScript("proposal: small initial state", formula),
                                         options.check.solver, /*wantModel=*/true);
    ++out.queries;
    if (response.answer != SmtAnswer::Sat)
      continue;
    std::vector<APInt> parameters;
    for (const Parameter &p : program.parameters) {
      auto value = response.model.find("p" + std::to_string(p.id));
      parameters.push_back(value == response.model.end() ? APInt::getZero(p.sort.width)
                                                         : APInt(p.sort.width, value->second, 10));
    }
    if (parametersInScope(options.check.scope, parameters))
      return parameters;
  }
  return model;
}

/// A sampled run that returns another value refutes the candidate: it is a run of
/// the program itself.
bool Search::refuteOnSamples(const LoopFunction &function) {
  Configuration empty;
  empty.state.assign(program.state.size(), std::nullopt);
  empty.shadow.assign(program.state.size(), false);
  for (size_t r = 0; r < out.samples.runParameters.size(); ++r) {
    if (!out.samples.runReturns[r])
      continue;
    EntryState entry;
    entry.parameters = out.samples.runParameters[r];
    const std::vector<APInt> &returned = *out.samples.runReturns[r];
    LoopCounterexample cex;
    bool differs = false;
    for (unsigned j = 0; j < function.results.size() && j < returned.size(); ++j) {
      std::optional<APInt> expected;
      if (function.results[j])
        expected = evaluateTerm(program, entry, empty, function.results[j]);
      cex.expected.push_back(expected);
      differs = differs || (expected && *expected != returned[j]);
    }
    if (!differs)
      continue;
    cex.parameters = entry.parameters;
    cex.returned = returned;
    cex.source = "a sampled run";
    out.finite.counterexample = cex;
    out.finite.status = "REFUTED";
    return true;
  }
  return false;
}

/// The closed-form grammar over the parameters and their monomials, fitted to the
/// values the sampled runs returned.
std::optional<LoopFunction> Search::synthesizeFunction(std::string &why) {
  std::vector<size_t> runs;
  for (size_t r = 0; r < out.samples.runParameters.size(); ++r)
    if (out.samples.runReturns[r])
      runs.push_back(r);
  if (runs.empty()) {
    why = "no sampled run returned";
    return std::nullopt;
  }
  LoopFunction function;
  function.origin = "synthesized: closed-form grammar over the parameters and their monomials of "
                    "degree <= " + std::to_string(options.functionDegree) + ", size <= " +
                    std::to_string(options.functionTermSize) + ", fitted to " +
                    std::to_string(runs.size()) + " returned run(s)";
  for (unsigned j = 0; j < out.loop.resultSorts.size(); ++j) {
    unsigned width = out.loop.resultSorts[j].width;
    std::vector<Leaf> leaves;
    for (const Parameter &p : program.parameters)
      if (p.sort.width == width)
        leaves.push_back({terms.param(p.id, p.sort), p.id});
    std::vector<std::vector<APInt>> rows;
    for (size_t r : runs)
      rows.push_back(out.samples.runParameters[r]);
    std::vector<Term> features;
    std::vector<std::vector<APInt>> values;
    monomials(terms, leaves, options.functionDegree, rows, features, values);
    TermGrammar grammar;
    grammar.maxTermSize = options.functionTermSize;
    grammar.maxTerms = options.maxTerms;
    GrammarEnumeration e = enumerateGrammar(program, features, values, rows.size(), width, grammar);
    std::vector<uint64_t> key;
    for (size_t r : runs)
      key.push_back((*out.samples.runReturns[r])[j].getZExtValue());
    auto found = e.seen.find(key);
    if (found == e.seen.end()) {
      why = "no term of the grammar returns result " + std::to_string(j) +
            " at every sampled run within the limits";
      return std::nullopt;
    }
    function.results.push_back(e.pool[found->second].term);
  }
  return function;
}

std::vector<Term> Search::equalities(unsigned degree, unsigned size) {
  std::vector<Term> atoms;
  const std::vector<std::vector<APInt>> &samples = out.samples.states;
  if (samples.empty())
    return atoms;
  std::vector<std::vector<APInt>> rows = samples;
  rows.insert(rows.end(), witness.begin(), witness.end());
  std::vector<Leaf> leaves = templateLeaves(program, out.loop);
  std::set<unsigned> widths;
  for (const Leaf &leaf : leaves)
    if (leaf.term->sort.width <= 64)
      widths.insert(leaf.term->sort.width);
  DenseSet<Term> seen;
  for (unsigned width : widths) {
    std::vector<Leaf> level;
    for (const Leaf &leaf : leaves)
      if (leaf.term->sort.width == width)
        level.push_back(leaf);
    std::vector<Term> features;
    std::vector<std::vector<APInt>> values;
    monomials(terms, level, degree, rows, features, values);
    TermGrammar grammar;
    grammar.maxTermSize = size;
    grammar.maxTerms = options.maxTerms;
    grammar.division = false;
    grammar.polynomial = true;
    grammar.collisions = true;
    grammar.collisionsPerTerm = options.collisionsPerTerm;
    GrammarEnumeration e = enumerateGrammar(program, features, values, samples.size(), width, grammar);
    // Each term is a sum of integer multiples of the features and 1, so each
    // equality is a vector of coefficients. One equality per new direction: the
    // same relation shifted by a constant or rearranged adds nothing to the
    // conjunction but solver work.
    DenseMap<Term, unsigned> column;
    for (unsigned k = 0; k < features.size(); ++k)
      column[features[k]] = k + 1; // Column 0 is the constant.
    std::function<bool(Term, int64_t, std::vector<int64_t> &)> linear =
        [&](Term t, int64_t scale, std::vector<int64_t> &v) {
          if (auto found = column.find(t); found != column.end()) {
            v[found->second] += scale;
            return true;
          }
          switch (t->op) {
          case TermOp::Const:
            v[0] += scale * t->value.getSExtValue();
            return true;
          case TermOp::Add:
            return linear(t->args[0], scale, v) && linear(t->args[1], scale, v);
          case TermOp::Sub:
            return linear(t->args[0], scale, v) && linear(t->args[1], -scale, v);
          case TermOp::Mul:
            if (t->args[0]->op != TermOp::Const)
              return false;
            return linear(t->args[1], scale * t->args[0]->value.getSExtValue(), v);
          default:
            return false;
          }
        };
    std::vector<std::vector<int64_t>> basis; // Rows with distinct leading columns.
    auto independent = [&](std::vector<int64_t> v) {
      for (const std::vector<int64_t> &row : basis) {
        size_t lead = 0;
        while (row[lead] == 0)
          ++lead;
        if (v[lead] == 0)
          continue;
        int64_t a = row[lead], b = v[lead];
        int64_t g = 0;
        for (size_t c = 0; c < v.size(); ++c) {
          v[c] = v[c] * a - row[c] * b;
          g = std::gcd(g, v[c] < 0 ? -v[c] : v[c]);
        }
        if (g > 1)
          for (int64_t &x : v)
            x /= g;
      }
      size_t lead = 0;
      while (lead < v.size() && v[lead] == 0)
        ++lead;
      if (lead == v.size())
        return false;
      basis.push_back(v);
      // Keep the rows ordered by leading column, so each reduction sees them in order.
      std::stable_sort(basis.begin(), basis.end(), [](const auto &x, const auto &y) {
        size_t i = 0, j = 0;
        while (x[i] == 0) ++i;
        while (y[j] == 0) ++j;
        return i < j;
      });
      return true;
    };
    for (const auto &[earlier, later] : e.collisions) {
      if (atoms.size() >= options.maxEqualities)
        break;
      std::vector<int64_t> v(features.size() + 1, 0);
      if (!linear(e.pool[earlier].term, 1, v) || !linear(later.term, -1, v) || !independent(v))
        continue;
      Term atom = terms.icmp(Predicate::Eq, e.pool[earlier].term, later.term);
      if (seen.insert(atom).second)
        atoms.push_back(atom);
    }
  }
  return atoms;
}

std::vector<Term> Search::bounds() {
  std::vector<Term> atoms;
  const std::vector<std::vector<APInt>> &samples = out.samples.states;
  if (samples.empty())
    return atoms;
  std::vector<RowView> sampleViews, witnessViews;
  for (const auto &row : samples)
    sampleViews.push_back(rowView(program, out.loop, row));
  for (const auto &row : witness)
    witnessViews.push_back(rowView(program, out.loop, row));
  DenseSet<Term> seen;
  auto offer = [&](Term atom) {
    if (!seen.insert(atom).second)
      return;
    for (const RowView &view : sampleViews)
      if (!holds(program, view, atom))
        return;
    bool identity = true;
    for (const RowView &view : witnessViews)
      identity = identity && holds(program, view, atom);
    if (!identity)
      atoms.push_back(atom);
  };
  std::vector<Leaf> leaves = templateLeaves(program, out.loop);
  std::set<unsigned> widths;
  for (const Leaf &leaf : leaves)
    widths.insert(leaf.term->sort.width);
  for (unsigned width : widths) {
    std::vector<Term> operands{terms.bv(APInt::getZero(width))};
    for (const Leaf &leaf : leaves)
      if (leaf.term->sort.width == width)
        operands.push_back(leaf.term);
    for (Term x : operands)
      for (Term y : operands) {
        if (x == y || (x->op == TermOp::Const && y->op == TermOp::Const))
          continue;
        for (Predicate p : {Predicate::Sle, Predicate::Slt, Predicate::Ule, Predicate::Ult})
          offer(terms.icmp(p, x, y));
      }
  }
  // The loop's own comparisons over header state and parameters, and their
  // non-strict forms.
  DenseSet<Term> visited;
  std::function<void(Term)> comparisons = [&](Term t) {
    if (!visited.insert(t).second)
      return;
    if (t->op == TermOp::ICmp && !containsOp(t, TermOp::Shadow)) {
      bool inside = true;
      for (unsigned v : stateSupport(t))
        inside = inside && llvm::is_contained(out.loop.variables, v);
      if (inside) {
        offer(t);
        Predicate p = (Predicate)t->aux;
        Predicate relaxed = p == Predicate::Slt   ? Predicate::Sle
                            : p == Predicate::Ult ? Predicate::Ule
                            : p == Predicate::Sgt ? Predicate::Sge
                            : p == Predicate::Ugt ? Predicate::Uge
                                                  : p;
        if (relaxed != p)
          offer(terms.icmp(relaxed, t->args[0], t->args[1]));
      }
    }
    for (Term a : t->args)
      comparisons(a);
  };
  for (unsigned id : out.loop.iterate)
    comparisons(out.loop.set.paths[id].condition);
  for (unsigned id : out.loop.leave)
    comparisons(out.loop.set.paths[id].condition);
  return atoms;
}

std::vector<Term> Search::poisonAtoms() {
  std::vector<Term> atoms;
  size_t nv = out.loop.variables.size();
  for (size_t i = 0; i < nv; ++i) {
    bool clear = true;
    for (const auto &row : out.samples.states)
      clear = clear && row[nv + i].isZero();
    if (clear && !out.samples.states.empty())
      atoms.push_back(terms.logicalNot(terms.shadow(out.loop.variables[i])));
  }
  return atoms;
}

CheckedLoopInvariant Search::check(const std::vector<Term> &atoms, const LoopFunction &function,
                                   bool final) {
  InvariantCheckOptions checking = options.check;
  checking.failFast = !final;
  if (!final)
    checking.scripts = false;
  InvariantCertificate certificate;
  certificate.atoms = atoms;
  certificate.origin = "search";
  CheckedLoopInvariant result = checkLoopInvariant(program, out.loop, certificate, function, checking, backend);
  ++out.checks;
  out.queries += result.queries;
  return result;
}

/// Houdini over the template's levels; minimization of a proof; the final check.
/// Houdini over the template's levels; minimization of a proof; the final check.
/// Null-space levels also turn INV-init models into new sampled runs (an initial state
/// is reachable, so its run is data; Sharma et al., Section 4, Theorem 3) and offer the
/// recomputed candidates again. An INV-step model is only an implication pair (ICE,
/// Section 1): it drops atoms and never becomes data.
bool Search::searchCertificate(const LoopFunction &function) {
  // "nullspace": the null-space levels; "collisions": the term-collision levels; "both"
  // (the default): the null-space levels, then the collision levels. The null space is
  // taken over Q from runs that did not wrap; at small widths most runs wrap, and the
  // collision template, which works modulo 2^w, still finds what the null space cannot.
  struct Level {
    unsigned degree, size;
    bool nullSpace;
  };
  std::vector<Level> plan;
  if (options.equalityCandidates != "collisions")
    for (unsigned degree : options.nullSpaceDegrees)
      plan.push_back({degree, 0, true});
  if (options.equalityCandidates != "nullspace")
    for (auto [degree, size] : options.levels)
      plan.push_back({degree, size, false});
  std::vector<Term> equalitySoFar; // The collision template accumulates its levels.
  DenseSet<Term> known;
  CheckedLoopInvariant last;
  std::vector<Term> lastAtoms;
  for (const Level &step : plan) {
    bool nullSpace = step.nullSpace;
    InvariantSearchLevel level;
    level.method = nullSpace ? "nullspace" : "collisions";
    level.degree = step.degree;
    level.size = step.size;
    auto offer = [&]() {
      std::vector<Term> equalities;
      if (nullSpace) {
        NullSpaceStats stats;
        equalities = nullSpaceEqualities(program, out.loop, out.samples.states, step.degree,
                                         options.maxColumns, stats);
        if (equalities.size() > options.maxEqualities)
          equalities.resize(options.maxEqualities);
        level.columns = stats.columns;
        level.rows = stats.rows;
        level.rank = stats.rank;
        level.basis = stats.basis;
        level.skipped = stats.skipped;
        level.folded = stats.folded;
      } else {
        for (Term atom : this->equalities(step.degree, step.size))
          if (known.insert(atom).second)
            equalitySoFar.push_back(atom);
        equalities = equalitySoFar;
      }
      std::vector<Term> bound = bounds(), poison = poisonAtoms();
      level.equalities = equalities.size();
      level.bounds = bound.size();
      level.poison = poison.size();
      std::vector<Term> atoms = bound;
      atoms.insert(atoms.end(), equalities.begin(), equalities.end());
      atoms.insert(atoms.end(), poison.begin(), poison.end());
      level.offered = atoms.size();
      return atoms;
    };
    // Atoms an INV-init model falsified are false at a reachable state: never offered
    // again, whatever the samples say.
    DenseSet<Term> refuted;
    auto fresh = [&](std::vector<Term> atoms) {
      std::vector<Term> kept;
      for (Term atom : atoms)
        if (!refuted.count(atom))
          kept.push_back(atom);
      return kept;
    };
    std::vector<Term> atoms = offer();
    CheckedLoopInvariant result;
    for (unsigned round = 0;; ++round) {
      if (round >= options.maxHoudiniChecks) {
        level.outcome = "Houdini check limit";
        break;
      }
      result = check(atoms, function, /*final=*/false);
      ++level.checks;
      if (getenv("GG_DEBUG_INVARIANTS")) {
        fprintf(stderr, "[degree %u round %u] %s by %s:", step.degree, round,
                stringifyInvariantVerdict(result.invariant), result.falsifiedBy.c_str());
        for (unsigned k : result.falsified)
          fprintf(stderr, " drop %s;", formatLoopTerm(atoms[k]).c_str());
        fprintf(stderr, "\n");
        if (round == 0)
          for (Term atom : atoms)
            fprintf(stderr, "  offered %s\n", formatLoopTerm(atom).c_str());
      }
      if (result.invariant == InvariantVerdict::Inductive)
        break;
      if (nullSpace && result.falsifiedBy == "INV-init" && !result.initialParameters.empty() &&
          level.refinements < options.maxRefinements) {
        // The model is an initial state the samples missed: run from it, and offer
        // the candidates the larger sample supports.
        ++level.refinements;
        for (unsigned k : result.falsified)
          refuted.insert(atoms[k]);
        addPoint(smallInitialState(atoms, result.initialParameters));
        atoms = fresh(offer());
        continue;
      }
      std::set<unsigned> drop(result.falsified.begin(), result.falsified.end());
      drop.insert(result.undecided.begin(), result.undecided.end());
      if (drop.empty()) {
        level.outcome = "no atom to drop: " + result.invariantReason;
        break;
      }
      std::vector<Term> kept;
      for (unsigned k = 0; k < atoms.size(); ++k)
        if (!drop.count(k))
          kept.push_back(atoms[k]);
      atoms = std::move(kept);
    }
    level.kept = atoms.size();
    last = result;
    lastAtoms = atoms;
    if (result.invariant == InvariantVerdict::Inductive) {
      if (result.function == LoopFunctionVerdict::Proved)
        level.outcome = "proved with " + std::to_string(atoms.size()) + " atom(s)";
      else if (result.function == LoopFunctionVerdict::Refuted)
        level.outcome = "refuted by a run";
      else
        level.outcome = "inductive with " + std::to_string(atoms.size()) +
                        " atom(s), not enough: " + result.functionReason;
    }
    out.levels.push_back(level);
    if (result.function == LoopFunctionVerdict::Proved) {
      out.minimizedFrom = atoms.size();
      if (options.minimize)
        minimize(atoms, function, result.poison);
      CheckedLoopInvariant proof = check(atoms, function, /*final=*/true);
      std::string where = nullSpace ? "null space of the sample matrix, degree " + std::to_string(step.degree)
                                    : "term-collision template, degree " + std::to_string(step.degree) +
                                          " size " + std::to_string(step.size);
      out.certificate = InvariantCertificate{
          atoms, "search: Houdini over the " + where + (options.minimize ? ", minimized" : "")};
      out.check = proof;
      return proof.function == LoopFunctionVerdict::Proved;
    }
    if (result.function == LoopFunctionVerdict::Refuted) {
      out.certificate = InvariantCertificate{atoms, "search"};
      out.check = result;
      return false;
    }
  }
  // Nothing proved: the last level's strongest inductive conjunction, checked again
  // with every obligation recorded.
  out.certificate = InvariantCertificate{lastAtoms, "search: the last level's conjunction, which "
                                                    "proves nothing"};
  out.check = check(lastAtoms, function, /*final=*/true);
  return false;
}

void Search::minimize(std::vector<Term> &atoms, const LoopFunction &function, const std::string &poison) {
  auto rank = [&](Term atom) {
    // Larger equalities first, then bounds, then poison atoms (they only matter for
    // the poison report).
    if (atom->op == TermOp::Not)
      return std::make_pair(2, 0u);
    if (atom->op == TermOp::ICmp && (Predicate)atom->aux != Predicate::Eq)
      return std::make_pair(1, 0u);
    return std::make_pair(0, -(unsigned)formatLoopTerm(atom).size());
  };
  std::vector<Term> order = atoms;
  std::stable_sort(order.begin(), order.end(), [&](Term a, Term b) { return rank(a) < rank(b); });
  unsigned checks = 0;
  for (Term atom : order) {
    if (checks++ >= options.maxMinimizeChecks)
      break;
    std::vector<Term> trial;
    for (Term t : atoms)
      if (t != atom)
        trial.push_back(t);
    CheckedLoopInvariant result = check(trial, function, /*final=*/false);
    if (result.function != LoopFunctionVerdict::Proved)
      continue;
    if (poison == "excluded") {
      // The poison answer is part of what the proof reports: keep it.
      InvariantCheckOptions checking = options.check;
      checking.failFast = false;
      checking.scripts = false;
      CheckedLoopInvariant full =
          checkLoopInvariant(program, out.loop, InvariantCertificate{trial, "minimize"}, function, checking, backend);
      ++out.checks;
      out.queries += full.queries;
      if (full.poison != "excluded")
        continue;
    }
    atoms = std::move(trial);
  }
}

void Search::fallback(const LoopFunction &function) {
  bool exhaustive = false;
  std::vector<std::vector<APInt>> finitePoints =
      loopPoints(program, options.check.scope, options.finitePoints, exhaustive);
  out.finite = checkLoopFunctionOnRuns(program, out.loop, function, finitePoints, exhaustive,
                                       options.finiteRun);
}

void Search::run(const LoopFunction *given, const InvariantCertificate *givenCertificate) {
  out.loop = invariantLoop(program, options.slice);
  if (!out.loop.supported()) {
    out.status = "UNSUPPORTED";
    out.reason = out.loop.reason;
    return;
  }
  if (given) {
    out.function = *given;
    out.functionGiven = true;
  }
  if (givenCertificate) {
    // The certificate alone: no sample is taken for it.
    out.certificateGiven = true;
    if (!given) {
      sample();
      std::string why;
      auto synthesized = synthesizeFunction(why);
      if (synthesized)
        out.function = *synthesized;
    }
    out.certificate = *givenCertificate;
    InvariantCheckOptions checking = options.check;
    checking.failFast = false;
    out.check = checkLoopInvariant(program, out.loop, *givenCertificate, out.function, checking, backend);
    ++out.checks;
    out.queries += out.check.queries;
  } else {
    sample();
    for (unsigned round = 0;; ++round) {
      if (!given) {
        std::string why;
        auto synthesized = synthesizeFunction(why);
        if (!synthesized) {
          out.reason = "no function candidate: " + why;
          out.function = LoopFunction();
          break;
        }
        out.function = *synthesized;
      }
      bool anyResult = llvm::any_of(out.function.results, [](Term t) { return t != nullptr; });
      if (!anyResult) {
        out.reason = "no function candidate";
        break;
      }
      if (refuteOnSamples(out.function)) {
        // A synthesized candidate fits every sample, so this is a given one.
        break;
      }
      searchCertificate(out.function);
      if (given || out.check.function != LoopFunctionVerdict::Refuted || !out.check.counterexample ||
          round + 1 >= options.maxFunctionRounds)
        break;
      // CEGIS over the function: the refuting run's parameters join the samples.
      std::string text;
      for (unsigned j = 0; j < out.function.results.size(); ++j)
        if (out.function.results[j])
          text += (text.empty() ? "" : "; ") + ("r" + std::to_string(j) + " = ") +
                  formatLoopTerm(out.function.results[j]);
      out.refutedFunctions.push_back(text);
      addPoint(out.check.counterexample->parameters);
      out.check = CheckedLoopInvariant();
      out.certificate.reset();
    }
  }
  bool anyResult = llvm::any_of(out.function.results, [](Term t) { return t != nullptr; });
  if (out.check.function == LoopFunctionVerdict::Proved) {
    out.status = "PROVED_BY_INVARIANT";
    out.reason = out.check.functionReason;
    return;
  }
  if (out.check.function == LoopFunctionVerdict::Refuted || out.finite.status == "REFUTED") {
    out.status = "REFUTED";
    out.reason = out.check.function == LoopFunctionVerdict::Refuted
                     ? out.check.functionReason
                     : "a run of the program at a sampled point returns another value";
    return;
  }
  // The fallback: complete runs at the scope's points.
  if (anyResult)
    fallback(out.function);
  out.status = out.finite.status;
  std::string invariantPart = out.check.functionReason.empty() ? out.reason : out.check.functionReason;
  if (out.finite.status == "REFUTED")
    out.reason = "a run of the program returns another value (" + out.finite.counterexample->source + ")";
  else
    out.reason = "the invariant path proved nothing (" + invariantPart + ")";
}

json::Value counterexampleJSON(const LoopCounterexample &cex) {
  json::Array parameters, returned, expected;
  for (const APInt &v : cex.parameters)
    parameters.push_back(decimal(v));
  for (const APInt &v : cex.returned)
    returned.push_back(decimal(v));
  for (const auto &v : cex.expected)
    expected.push_back(v ? json::Value(decimal(*v)) : json::Value(nullptr));
  return json::Object{{"parameters", std::move(parameters)},
                      {"returned_by_the_run", std::move(returned)},
                      {"candidate", std::move(expected)},
                      {"steps", (int64_t)cex.steps},
                      {"source", cex.source}};
}

} // namespace

std::vector<Term> nullSpaceEqualities(const SemanticProgram &program, const InvariantLoop &loop,
                                      ArrayRef<std::vector<APInt>> rows, unsigned degree,
                                      size_t maxColumns, NullSpaceStats &stats) {
  using Big = DynamicAPInt;
  TermContext &terms = *program.terms;
  std::vector<Term> atoms;
  stats = NullSpaceStats();
  if (rows.empty())
    return atoms;
  size_t nv = loop.variables.size();
  // Folding: a header variable with one value at every row is that constant. Its atom
  // goes to the checker like any other; the variable leaves the monomials, where it
  // would only add multiples of the same fact.
  std::vector<bool> folded(nv, false);
  for (size_t i = 0; i < nv; ++i) {
    bool same = true;
    for (const auto &row : rows)
      same = same && row[i] == rows.front()[i];
    if (!same)
      continue;
    folded[i] = true;
    ++stats.folded;
    unsigned v = loop.variables[i];
    atoms.push_back(terms.icmp(Predicate::Eq, terms.state(v, program.state[v].sort),
                               terms.bv(rows.front()[i])));
  }
  // Leaves per width: parameters first, then the unfolded header variables, so that a
  // variable the others determine is a late (free) column and its basis vector reads
  // "variable = combination of earlier columns".
  struct Leaf {
    Term term;
    size_t column;
    bool state;
  };
  std::map<unsigned, std::vector<Leaf>> byWidth;
  for (size_t j = 0; j < loop.parameters.size(); ++j) {
    const Parameter &p = program.parameters[loop.parameters[j]];
    if (p.sort.width <= 64)
      byWidth[p.sort.width].push_back({terms.param(p.id, p.sort), 2 * nv + j, false});
  }
  for (size_t i = 0; i < nv; ++i) {
    unsigned v = loop.variables[i];
    if (!folded[i] && program.state[v].sort.width <= 64)
      byWidth[program.state[v].sort.width].push_back(
          {terms.state(v, program.state[v].sort), i, true});
  }
  for (auto &[width, leaves] : byWidth) {
    // Monomials of degree 0..degree as non-decreasing index tuples; the ones with only
    // parameters first, then those that read a header variable, the linear header
    // variables last.
    std::vector<std::vector<size_t>> monomials{{}};
    std::function<void(std::vector<size_t> &, size_t, unsigned)> grow =
        [&](std::vector<size_t> &tuple, size_t from, unsigned left) {
          if (!tuple.empty())
            monomials.push_back(tuple);
          if (!left)
            return;
          for (size_t k = from; k < leaves.size(); ++k) {
            tuple.push_back(k);
            grow(tuple, k, left - 1);
            tuple.pop_back();
          }
        };
    std::vector<size_t> tuple;
    grow(tuple, 0, degree);
    auto readsState = [&](const std::vector<size_t> &m) {
      for (size_t k : m)
        if (leaves[k].state)
          return true;
      return false;
    };
    auto linearState = [&](const std::vector<size_t> &m) {
      return m.size() == 1 && leaves[m[0]].state;
    };
    std::stable_sort(monomials.begin(), monomials.end(), [&](const auto &a, const auto &b) {
      auto key = [&](const std::vector<size_t> &m) { return linearState(m) ? 2 : readsState(m) ? 1 : 0; };
      return key(a) < key(b);
    });
    size_t columns = monomials.size();
    stats.columns += columns;
    if (columns > maxColumns)
      continue; // Too many monomials for this level: nothing offered, and the level says so.
    // The rows of the matrix, each value read as a signed integer, products exact.
    std::set<std::vector<Big>> distinct;
    for (const auto &row : rows) {
      std::vector<Big> line;
      line.reserve(columns);
      for (const auto &m : monomials) {
        Big value(1);
        for (size_t k : m)
          value *= Big(row[leaves[k].column]);
        line.push_back(value);
      }
      distinct.insert(std::move(line));
    }
    stats.rows += distinct.size();
    // Fraction-free Gauss-Jordan elimination, rows kept primitive (gcd 1).
    auto primitive = [](std::vector<Big> &v) {
      Big g(0);
      for (const Big &x : v)
        g = gcd(g, abs(x));
      if (g > Big(1))
        for (Big &x : v)
          x /= g;
    };
    std::vector<std::vector<Big>> echelon;
    std::vector<size_t> pivots;
    for (std::vector<Big> line : distinct) {
      for (size_t r = 0; r < echelon.size(); ++r) {
        size_t c = pivots[r];
        if (line[c] == Big(0))
          continue;
        Big a = echelon[r][c], b = line[c];
        for (size_t k = 0; k < columns; ++k)
          line[k] = line[k] * a - echelon[r][k] * b;
        primitive(line);
      }
      size_t lead = 0;
      while (lead < columns && line[lead] == Big(0))
        ++lead;
      if (lead == columns)
        continue;
      // Clear the new pivot column from the rows above.
      for (size_t r = 0; r < echelon.size(); ++r) {
        if (echelon[r][lead] == Big(0))
          continue;
        Big a = line[lead], b = echelon[r][lead];
        for (size_t k = 0; k < columns; ++k)
          echelon[r][k] = echelon[r][k] * a - line[k] * b;
        primitive(echelon[r]);
      }
      primitive(line);
      echelon.push_back(std::move(line));
      pivots.push_back(lead);
    }
    stats.rank += echelon.size();
    // One basis vector of the null space per free column f: x_f = L and, for each row
    // r with pivot p, x_p = -A[r][f] * L / A[r][p], with L a common multiple of the pivots.
    std::vector<bool> isPivot(columns, false);
    for (size_t c : pivots)
      isPivot[c] = true;
    Big limit(int64_t(1));
    for (unsigned b = 0; b + 1 < width; ++b)
      limit *= Big(2);
    for (size_t f = 0; f < columns; ++f) {
      if (isPivot[f])
        continue;
      Big L(1);
      for (size_t r = 0; r < echelon.size(); ++r)
        if (echelon[r][f] != Big(0))
          L = lcm(L, abs(echelon[r][pivots[r]]));
      std::vector<Big> x(columns, Big(0));
      x[f] = L;
      for (size_t r = 0; r < echelon.size(); ++r)
        if (echelon[r][f] != Big(0))
          x[pivots[r]] = -(echelon[r][f] * (L / echelon[r][pivots[r]]));
      primitive(x);
      if (x[f] < Big(0))
        for (Big &v : x)
          v = -v;
      ++stats.basis;
      bool fits = true;
      for (const Big &c : x)
        fits = fits && abs(c) < limit;
      if (!fits) {
        ++stats.skipped;
        continue;
      }
      // sum(c_k * m_k) = 0 as  (positive part) == (negative part), with the free
      // column last on its side when it is a header variable with coefficient 1, so
      // the checker can solve the atom for it.
      auto monomialTerm = [&](size_t k) -> Term {
        if (monomials[k].empty())
          return nullptr;
        Term t = leaves[monomials[k][0]].term;
        for (size_t i = 1; i < monomials[k].size(); ++i)
          t = terms.binary(TermOp::Mul, t, leaves[monomials[k][i]].term);
        return t;
      };
      auto scaled = [&](size_t k, const Big &magnitude) -> Term {
        APInt value(width, int64_t(magnitude), /*isSigned=*/true);
        Term m = monomialTerm(k);
        if (!m)
          return terms.bv(value);
        if (magnitude == Big(1))
          return m;
        return terms.binary(TermOp::Mul, terms.bv(value), m);
      };
      Term positive = nullptr, negative = nullptr;
      auto add = [&](Term &side, Term t) { side = side ? terms.binary(TermOp::Add, side, t) : t; };
      for (size_t k = 0; k < columns; ++k) {
        if (k == f || x[k] == Big(0))
          continue;
        if (x[k] > Big(0))
          add(positive, scaled(k, x[k]));
        else
          add(negative, scaled(k, -x[k]));
      }
      add(positive, scaled(f, x[f])); // x[f] > 0 by the sign normalization.
      if (!negative)
        negative = terms.bv(APInt::getZero(width));
      atoms.push_back(terms.icmp(Predicate::Eq, positive, negative));
    }
  }
  return atoms;
}

LoopInvariantSearch proveLoopFunction(const SemanticProgram &program, const LoopFunction *given,
                                      const InvariantCertificate *givenCertificate,
                                      const InvariantSearchOptions &options, SmtBackend &backend) {
  LoopInvariantSearch out;
  Search search(program, options, backend, out);
  try {
    search.run(given, givenCertificate);
  } catch (const TermError &error) {
    out.status = "UNKNOWN";
    out.reason = std::string("a term could not be built: ") + error.what();
  }
  // A proof and a refuting run in the scope cannot both stand: that is a defect.
  if (out.status == "PROVED_BY_INVARIANT" && out.finite.counterexample &&
      parametersInScope(options.check.scope, out.finite.counterexample->parameters)) {
    out.conflict = "the invariant proof and a run of the program disagree";
    out.status = "UNKNOWN";
  }
  return out;
}

std::string loopInvariantJSON(const SemanticProgram &program, const LoopInvariantSearch &result,
                              const InvariantSearchOptions &options, const std::string &termination,
                              const std::string &terminationReason, bool scripts) {
  json::Object root;
  root["status"] = result.status;
  root["reason"] = result.reason;
  if (!result.conflict.empty())
    root["conflict"] = result.conflict;
  bool proved = result.status == "PROVED_BY_INVARIANT";
  root["unrolling_omitted"] = proved;
  unsigned width = 0;
  for (const Sort &s : result.loop.resultSorts)
    width = std::max(width, s.width);
  root["semantics"] =
      "bit vectors at the program's widths: values wrap modulo 2^w; nsw, nuw and exact only set "
      "poison bits; a branch on poison is a stop (undefined behaviour); Post compares the bits "
      "of the returned values. No statement over the mathematical integers is made.";
  std::string claim;
  if (proved) {
    std::string w = std::to_string(width);
    if (termination == "PROVEN_TERMINATING")
      claim = "total correctness over the scope: every run returns, and returns the candidate's "
              "value modulo 2^" + w + " (the invariant proof and the G2a termination proof)";
    else if (termination == "PROVEN_TERMINATING_IF_DEFINED")
      claim = "every run either returns the candidate's value modulo 2^" + w +
              " or stops on a side condition (undefined behaviour); none diverges";
    else
      claim = "partial correctness only: a run that returns, returns the candidate's value modulo 2^" +
              w + "; termination is not established" +
              (termination.empty() ? std::string(" (not analyzed)") : " (" + termination + ")");
    if (result.check.poison != "excluded")
      claim += ". A returned value is not shown free of poison: where one is poison (an overflow "
               "under nsw or nuw, undefined behaviour in C), its bits equal the candidate's and it "
               "has no defined value";
  } else if (result.status == "REFUTED") {
    claim = "the candidate is wrong: a run of the program returns another value";
  } else if (result.status == "SCOPE_ENUMERATED") {
    claim = "complete bounded check only: every point of the finite scope ran to completion, "
            "every iteration executed, and returned the candidate's value; nothing outside the scope";
  } else if (result.status == "SAMPLE_CONSISTENT") {
    claim = "sample tests only: equal on the runs made; not a proof";
  } else {
    claim = "no claim";
  }
  root["claim"] = claim;
  root["termination"] = json::Object{{"verdict", termination.empty() ? "NOT_ANALYZED" : termination},
                                     {"reason", terminationReason},
                                     {"role", "an invariant proves partial correctness only; "
                                              "termination is the G2a analysis's, joined here"}};
  if (!result.loop.supported()) {
    std::string text;
    raw_string_ostream os(text);
    os << json::Value(std::move(root));
    return os.str();
  }
  {
    auto spec = json::parse(invariantSpecificationJSON(program, result.loop, result.function,
                                                       options.check.scope));
    if (spec)
      root["specification"] = std::move(*spec);
    else
      consumeError(spec.takeError());
  }
  json::Array results;
  for (unsigned j = 0; j < result.function.results.size(); ++j)
    if (result.function.results[j])
      results.push_back(json::Object{{"result", (int64_t)j},
                                     {"candidate", formatLoopTerm(result.function.results[j])}});
  json::Array refuted;
  for (const std::string &text : result.refutedFunctions)
    refuted.push_back(text);
  root["function"] = json::Object{
      {"origin", result.functionGiven ? std::string("given") : result.function.origin},
      {"results", std::move(results)},
      {"refuted_on_the_way", std::move(refuted)}};
  if (result.certificate) {
    json::Array atoms;
    for (Term atom : result.certificate->atoms)
      atoms.push_back(formatLoopTerm(atom));
    root["certificate"] = json::Object{
        {"origin", result.certificateGiven ? std::string("given") : result.certificate->origin},
        {"atoms", std::move(atoms)},
        {"minimized_from", (int64_t)result.minimizedFrom}};
  } else {
    root["certificate"] = nullptr;
  }
  const CheckedLoopInvariant &check = result.check;
  json::Array falsified;
  for (unsigned k : check.falsified)
    falsified.push_back((int64_t)k);
  root["invariant"] = json::Object{{"verdict", stringifyInvariantVerdict(check.invariant)},
                                   {"reason", check.invariantReason},
                                   {"falsified_by", check.falsifiedBy},
                                   {"falsified_atoms", std::move(falsified)}};
  root["verdict"] = json::Object{{"function", stringifyLoopFunctionVerdict(check.function)},
                                 {"reason", check.functionReason}};
  if (check.counterexample)
    root["counterexample"] = counterexampleJSON(*check.counterexample);
  else if (result.finite.counterexample)
    root["counterexample"] = counterexampleJSON(*result.finite.counterexample);
  else
    root["counterexample"] = nullptr;
  if (!check.unconfirmedExitModel.empty())
    root["unconfirmed_exit_model"] = check.unconfirmedExitModel;
  json::Array eliminated, lemmas, records;
  for (const std::string &e : check.eliminated)
    eliminated.push_back(e);
  for (const std::string &l : check.lemmas)
    lemmas.push_back(l);
  for (const ObligationRecord &r : check.records) {
    json::Object record{{"id", r.id},           {"location", r.location}, {"expected", r.expected},
                        {"result", r.result},   {"evidence", r.evidence}, {"discharged", r.discharged}};
    if (scripts)
      record["script"] = r.script;
    records.push_back(std::move(record));
  }
  root["eliminated"] = std::move(eliminated);
  root["lemmas"] = std::move(lemmas);
  root["poison"] = check.poison;
  root["obligations"] = std::move(records);
  root["proof"] = json::Object{
      {"queries", (int64_t)check.queries},
      {"script_bytes", (int64_t)check.scriptBytes},
      {"program_runs", (int64_t)check.confirmationRuns},
      {"program_steps", (int64_t)check.confirmationSteps},
      {"note", "the obligations of the certificate above; a proof runs no iteration of the program "
               "(runs here only confirm an INV-exit model)"}};
  json::Array levels;
  for (const InvariantSearchLevel &level : result.levels)
    levels.push_back(json::Object{{"method", level.method},
                                  {"degree", (int64_t)level.degree},
                                  {"size", (int64_t)level.size},
                                  {"equalities", (int64_t)level.equalities},
                                  {"bounds", (int64_t)level.bounds},
                                  {"poison", (int64_t)level.poison},
                                  {"offered", (int64_t)level.offered},
                                  {"kept", (int64_t)level.kept},
                                  {"checks", (int64_t)level.checks},
                                  {"columns", (int64_t)level.columns},
                                  {"rows", (int64_t)level.rows},
                                  {"rank", (int64_t)level.rank},
                                  {"basis", (int64_t)level.basis},
                                  {"skipped_large_coefficients", (int64_t)level.skipped},
                                  {"folded_constants", (int64_t)level.folded},
                                  {"refinements", (int64_t)level.refinements},
                                  {"outcome", level.outcome}});
  std::string levelText;
  for (auto [degree, size] : options.levels)
    levelText += (levelText.empty() ? "" : ", ") + std::string("(degree ") + std::to_string(degree) +
                 ", size " + std::to_string(size) + ")";
  std::string degreeText;
  for (unsigned d : options.nullSpaceDegrees)
    degreeText += (degreeText.empty() ? "" : ", ") + std::to_string(d);
  std::string equalityText =
      options.equalityCandidates != "collisions"
          ? "equalities: first a basis of the null space over Q of the matrix of all monomials of degree <= d "
            "(d in " + degreeText + ", at most " + std::to_string(options.maxColumns) +
            " monomials) over the header variables and parameters at the sampled states, each "
            "vector scaled to coprime integers and stated at the program's width (a candidate "
            "only), after folding variables that hold one constant at every sample; then, "
            "unless only the null space is asked for, the term-collision template below; "
          : "";
  root["search"] = json::Object{
      {"equality_candidates", options.equalityCandidates},
      {"template",
       equalityText + "conjunctions of: equalities E = E between sums of monomials over the header variables and "
       "parameters (+, -, constant *, constants 0..8 and the program's literals), offered when equal "
       "at every sampled header state and not an identity; comparisons <=s <s <=u <u between two of "
       "them or 0, and the loop's own comparisons; not-poison per variable. Levels " + levelText +
       "; at most " + std::to_string(options.maxEqualities) + " equalities, " +
       std::to_string(options.maxTerms) + " terms. No disjunction, no quantifier, no division."},
      {"levels", std::move(levels)},
      {"checks", (int64_t)result.checks},
      {"queries", (int64_t)result.queries}};
  root["sampling"] = json::Object{
      {"points", (int64_t)result.samplePoints},
      {"runs", (int64_t)result.samples.runs},
      {"returned", (int64_t)result.samples.returned},
      {"header_arrivals", (int64_t)result.samples.arrivals},
      {"max_arrivals_per_run", (int64_t)result.samples.maxArrivals},
      {"steps", (int64_t)result.samples.steps},
      {"states", (int64_t)result.samples.states.size()},
      {"role", "candidate generation only: no obligation reads a sample"}};
  json::Object fallback{{"status", result.finite.status},
                        {"points", (int64_t)result.finite.points},
                        {"returned", (int64_t)result.finite.returned},
                        {"equal", (int64_t)result.finite.equal},
                        {"out_of_fuel", (int64_t)result.finite.outOfFuel},
                        {"stopped", (int64_t)result.finite.stopped},
                        {"exhaustive", result.finite.exhaustive},
                        {"steps", (int64_t)result.finite.steps}};
  fallback["meaning"] =
      result.finite.points == 0
          ? "not run"
          : "complete runs, every iteration executed: SCOPE_ENUMERATED is a complete bounded check of "
            "a finite scope, SAMPLE_CONSISTENT only tests, REFUTED a concrete counterexample";
  root["fallback"] = std::move(fallback);
  std::string text;
  raw_string_ostream os(text);
  os << json::Value(std::move(root));
  return os.str();
}

} // namespace mlir::generalized_gemm
