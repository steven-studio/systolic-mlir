#include "GeneralizedGemm/TerminationProposers.h"

#include "GeneralizedGemm/ControlStructure.h"
#include "GeneralizedGemm/TerminationObligations.h"

#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/SmallString.h"

#include <algorithm>
#include <functional>
#include <set>

using namespace mlir::generalized_gemm;
using llvm::APInt;

namespace {

/// Thrown when the query budget runs out. The search ends with what it has.
struct BudgetExhausted {};

class Proposer {
public:
  Proposer(const ProposalInput &input, ProposalStatistics &statistics)
      : in(input), stats(statistics), program(input.program),
        terms(*input.program.terms) {}

  std::map<unsigned, Term> invariants();
  std::optional<std::vector<RankLevel>>
  levels(const std::map<unsigned, Term> &invariants);

private:
  using Aux = std::vector<std::pair<std::string, Term>>;

  SmtResponse ask(Term formula, const Aux &aux, bool wantModel);
  Term conjunction(llvm::ArrayRef<Term> list);
  Term invariantAt(unsigned location) const;
  void comparisons(Term root, std::vector<Term> &out, llvm::DenseSet<Term> &seen);

  // Houdini.
  std::vector<Term> invariantCandidates(unsigned cut);
  Term guarded(Term fact);
  bool refine(const CutPath &path, std::map<unsigned, std::vector<Term>> &active,
              bool &changed);

  // Ranks.
  bool solve(llvm::ArrayRef<unsigned> current, std::vector<RankLevel> &out);
  bool solveComponent(llvm::ArrayRef<unsigned> component,
                      llvm::ArrayRef<unsigned> inside, RankLevel &out);
  bool attempt(Term rank, const std::string &origin,
               llvm::ArrayRef<unsigned> component,
               llvm::ArrayRef<unsigned> inside, RankLevel &out);
  std::vector<std::pair<Term, std::string>>
  guardDistances(llvm::ArrayRef<unsigned> component);
  std::vector<std::pair<Term, std::string>>
  modularDistances(llvm::ArrayRef<unsigned> component,
                   llvm::ArrayRef<unsigned> inside);
  std::vector<std::pair<Term, std::string>>
  affineRanks(llvm::ArrayRef<unsigned> component,
              llvm::ArrayRef<unsigned> inside);
  std::optional<Term> cegis(llvm::ArrayRef<Term> atoms, bool isSigned,
                            llvm::ArrayRef<unsigned> inside,
                            const std::set<unsigned> &strict);
  std::vector<Term> leavingComparisons(llvm::ArrayRef<unsigned> component);
  void rememberParameters(const SmtResponse &response);

  const ProposalInput &in;
  ProposalStatistics &stats;
  const SemanticProgram &program;
  TermContext &terms;
  std::map<unsigned, Term> known;
  unsigned rankAttempts = 0;
};

SmtResponse Proposer::ask(Term formula, const Aux &aux, bool wantModel) {
  if (stats.queries >= in.limits.maxProposerQueries)
    throw BudgetExhausted{};
  ++stats.queries;
  return in.backend.check(obligationScript("proposal", formula, aux),
                          in.limits.solver, wantModel);
}

Term Proposer::conjunction(llvm::ArrayRef<Term> list) {
  Term all = terms.boolean(true);
  for (Term t : list)
    all = terms.logicalAnd(all, t);
  return all;
}

Term Proposer::invariantAt(unsigned location) const {
  auto found = known.find(location);
  return found == known.end() ? terms.boolean(true) : found->second;
}

void Proposer::comparisons(Term root, std::vector<Term> &out,
                           llvm::DenseSet<Term> &seen) {
  if (!seen.insert(root).second)
    return;
  if (root->op == TermOp::ICmp && !readsMemory(root) &&
      !(root->args[0]->op == TermOp::Const && root->args[1]->op == TermOp::Const))
    out.push_back(root);
  for (Term a : root->args)
    comparisons(a, out, seen);
}

/// A fact about values only has to hold where they are not poison.
Term Proposer::guarded(Term fact) {
  Term excused = terms.boolean(false);
  for (unsigned id : stateSupport(fact))
    excused = terms.logicalOr(excused, terms.shadow(id));
  return terms.logicalOr(excused, fact);
}

std::vector<Term> Proposer::invariantCandidates(unsigned cut) {
  (void)cut;
  std::vector<Term> out;
  llvm::DenseSet<Term> unique;
  auto add = [&](Term t) {
    if (t && unique.insert(t).second)
      out.push_back(t);
  };
  auto addFact = [&](Term fact) {
    add(fact);
    add(guarded(fact));
  };
  // Values a state takes that depend on no other state. These are what make
  // an entry-block constant visible inside a loop.
  auto closed = [&](Term t) {
    if (!t || !stateSupport(t).empty() || readsMemory(t) ||
        containsOp(t, TermOp::Shadow))
      return false;
    return true;
  };
  std::vector<std::vector<Term>> values(program.state.size());
  for (const CutPath &path : in.set.paths)
    for (unsigned id = 0; id < program.state.size(); ++id) {
      Term value = path.state[id];
      if (closed(value) && program.state[id].sort.kind == Sort::BV &&
          values[id].size() < 3 && !llvm::is_contained(values[id], value))
        values[id].push_back(value);
    }
  auto stateTerm = [&](unsigned id) {
    return terms.state(id, program.state[id].sort);
  };

  for (unsigned id = 0; id < program.state.size(); ++id)
    for (Term value : values[id])
      addFact(terms.icmp(Predicate::Eq, stateTerm(id), value));
  for (unsigned id = 0; id < program.state.size(); ++id)
    add(terms.logicalNot(terms.shadow(id)));

  std::vector<Term> atoms;
  llvm::DenseSet<Term> seen;
  for (const CutPath &path : in.set.paths)
    comparisons(path.condition, atoms, seen);
  for (Term atom : atoms) {
    addFact(atom);
    addFact(terms.logicalNot(atom));
  }

  for (unsigned id = 0; id < program.state.size(); ++id) {
    const StateVar &s = program.state[id];
    if (s.sort.kind != Sort::BV || !s.loopCarried)
      continue;
    Term x = stateTerm(id);
    Term zero = terms.bv(APInt(s.sort.width, 0));
    addFact(terms.icmp(Predicate::Sgt, x, zero));
    addFact(terms.icmp(Predicate::Sge, x, zero));
    addFact(terms.icmp(Predicate::Ne, x, zero));
    for (Term value : values[id]) {
      addFact(terms.icmp(Predicate::Sge, x, value));
      addFact(terms.icmp(Predicate::Sle, x, value));
      addFact(terms.icmp(Predicate::Uge, x, value));
      addFact(terms.icmp(Predicate::Ule, x, value));
      for (unsigned bits = 1; bits <= 3 && bits < s.sort.width; ++bits) {
        Term mask = terms.bv(APInt::getLowBitsSet(s.sort.width, bits));
        addFact(terms.icmp(Predicate::Eq, terms.binary(TermOp::And, x, mask),
                           terms.binary(TermOp::And, value, mask)));
      }
      for (unsigned modulus : {3u, 5u, 7u})
        if (s.sort.width > 3)
          addFact(terms.icmp(
              Predicate::Eq,
              terms.binary(TermOp::SRem, terms.binary(TermOp::Sub, x, value),
                           terms.bv(APInt(s.sort.width, modulus))),
              zero));
    }
  }

  // Orderings between the loop's variables and the parameters.
  std::vector<Term> ordered;
  for (const StateVar &s : program.state)
    if (s.loopCarried && s.sort.kind == Sort::BV && ordered.size() < 8)
      ordered.push_back(terms.state(s.id, s.sort));
  for (const Parameter &p : program.parameters)
    if (p.sort.kind == Sort::BV && ordered.size() < 10)
      ordered.push_back(terms.param(p.id, p.sort));
  for (Term x : ordered)
    for (Term y : ordered) {
      if (x == y || x->sort != y->sort)
        continue;
      addFact(terms.icmp(Predicate::Sle, x, y));
      addFact(terms.icmp(Predicate::Ule, x, y));
      addFact(terms.icmp(Predicate::Slt, x, y));
      addFact(terms.icmp(Predicate::Ult, x, y));
    }
  if (out.size() > in.limits.maxInvariantCandidates)
    out.resize(in.limits.maxInvariantCandidates);
  return out;
}

/// Removes targets of `path` until none can be falsified along it.
bool Proposer::refine(const CutPath &path,
                      std::map<unsigned, std::vector<Term>> &active,
                      bool &changed) {
  std::vector<Term> &targets = active[path.to];
  Term source = path.fromEntry ? terms.boolean(true)
                               : conjunction(active[path.from]);
  while (!targets.empty()) {
    Aux aux;
    for (unsigned i = 0; i < targets.size(); ++i)
      aux.push_back({"h" + std::to_string(i), postTerm(program, path, targets[i])});
    SmtResponse response =
        ask(invariantViolation(program, in.scope, path, source,
                               conjunction(targets)),
            aux, /*wantModel=*/true);
    if (response.answer == SmtAnswer::Unsat)
      return true;
    std::vector<Term> kept;
    if (response.answer == SmtAnswer::Sat) {
      for (unsigned i = 0; i < targets.size(); ++i) {
        auto value = response.model.find("h" + std::to_string(i));
        if (value != response.model.end() && value->second == "1")
          kept.push_back(targets[i]);
      }
      if (kept.size() == targets.size())
        kept.clear(); // The model excuses nothing: keep no guess.
    } else {
      // Too hard together. One at a time keeps what is easy.
      for (Term target : targets) {
        SmtResponse single = ask(
            invariantViolation(program, in.scope, path, source, target), {}, false);
        if (single.answer == SmtAnswer::Unsat)
          kept.push_back(target);
      }
    }
    changed = changed || kept.size() != targets.size();
    targets = std::move(kept);
  }
  return true;
}

std::map<unsigned, Term> Proposer::invariants() {
  std::map<unsigned, Term> result;
  if (in.cuts.empty())
    return result;
  std::map<unsigned, std::vector<Term>> active;
  std::vector<Term> candidates = invariantCandidates(0);
  for (unsigned cut : in.cuts)
    active[cut] = candidates;
  try {
    bool changed = true;
    while (changed) {
      changed = false;
      for (const CutPath &path : in.set.paths)
        if (path.to != program.exitLocation())
          refine(path, active, changed);
    }
  } catch (const BudgetExhausted &) {
    stats.failure = "invariant search ran out of queries";
    return result;
  }
  for (unsigned cut : in.cuts) {
    stats.invariantsKept += active[cut].size();
    result[cut] = conjunction(active[cut]);
  }
  return result;
}

std::vector<Term>
Proposer::leavingComparisons(llvm::ArrayRef<unsigned> component) {
  std::vector<Term> atoms;
  llvm::DenseSet<Term> seen;
  for (const CutPath &path : in.set.paths)
    if (!path.fromEntry && llvm::is_contained(component, path.from))
      comparisons(path.condition, atoms, seen);
  return atoms;
}

static Term extended(TermContext &terms, Term value, bool isSigned, unsigned width) {
  if (value->sort.width == width)
    return value;
  return isSigned ? terms.sext(value, width) : terms.zext(value, width);
}

std::vector<std::pair<Term, std::string>>
Proposer::guardDistances(llvm::ArrayRef<unsigned> component) {
  std::vector<std::pair<Term, std::string>> out;
  auto add = [&](Term a, Term b, bool isSigned) {
    unsigned width = a->sort.width + 2;
    Term x = extended(terms, a, isSigned, width);
    Term y = extended(terms, b, isSigned, width);
    Term one = terms.bv(APInt(width, 1));
    for (Term rank : {terms.binary(TermOp::Sub, y, x), terms.binary(TermOp::Sub, x, y)}) {
      out.push_back({rank, "T1 guard distance"});
      out.push_back({terms.binary(TermOp::Add, rank, one), "T1 guard distance"});
    }
  };
  for (Term atom : leavingComparisons(component)) {
    Predicate p = (Predicate)atom->aux;
    bool isSignedOnly = p == Predicate::Slt || p == Predicate::Sle ||
                        p == Predicate::Sgt || p == Predicate::Sge;
    bool isUnsignedOnly = p == Predicate::Ult || p == Predicate::Ule ||
                          p == Predicate::Ugt || p == Predicate::Uge;
    if (!isUnsignedOnly)
      add(atom->args[0], atom->args[1], /*isSigned=*/true);
    if (!isSignedOnly)
      add(atom->args[0], atom->args[1], /*isSigned=*/false);
  }
  return out;
}

std::vector<std::pair<Term, std::string>>
Proposer::modularDistances(llvm::ArrayRef<unsigned> component,
                           llvm::ArrayRef<unsigned> inside) {
  std::vector<std::pair<Term, std::string>> out;
  std::vector<Term> atoms = leavingComparisons(component);
  std::vector<Term> variables;
  for (Term atom : atoms)
    for (Term side : atom->args)
      if (side->op == TermOp::State && !llvm::is_contained(variables, side))
        variables.push_back(side);
  if (variables.empty())
    return out;
  Aux aux;
  for (unsigned k = 0; k < variables.size(); ++k)
    aux.push_back({}); // Filled per path.
  // One step of every path, as a sample. A step that is not constant only
  // yields a rank that the checks reject.
  std::vector<std::optional<APInt>> step(variables.size());
  std::vector<bool> consistent(variables.size(), true);
  for (unsigned id : inside) {
    const CutPath &path = in.set.paths[id];
    for (unsigned k = 0; k < variables.size(); ++k)
      aux[k] = {"d" + std::to_string(k),
                terms.binary(TermOp::Sub, postTerm(program, path, variables[k]),
                             variables[k])};
    Term enabled = terms.logicalAnd(
        terms.logicalAnd(in.scope, invariantAt(path.from)), path.condition);
    SmtResponse response = ask(enabled, aux, /*wantModel=*/true);
    if (response.answer == SmtAnswer::Unsat)
      continue; // An infeasible path constrains nothing.
    if (response.answer != SmtAnswer::Sat)
      return out;
    for (unsigned k = 0; k < variables.size(); ++k) {
      auto value = response.model.find("d" + std::to_string(k));
      APInt delta(variables[k]->sort.width, 0);
      if (value != response.model.end())
        delta = APInt(variables[k]->sort.width, value->second, 10);
      if (step[k] && *step[k] != delta)
        consistent[k] = false;
      step[k] = delta;
    }
  }
  for (unsigned k = 0; k < variables.size(); ++k) {
    if (!consistent[k] || !step[k] || !(*step[k])[0])
      continue;
    Term x = variables[k];
    unsigned width = x->sort.width;
    Term inverse = terms.bv(step[k]->multiplicativeInverse());
    std::vector<Term> targets{terms.bv(APInt(width, 0))};
    for (Term atom : atoms) {
      if (atom->args[0] == x)
        targets.push_back(atom->args[1]);
      if (atom->args[1] == x)
        targets.push_back(atom->args[0]);
    }
    for (Term target : targets)
      out.push_back({terms.binary(TermOp::Mul, inverse,
                                  terms.binary(TermOp::Sub, target, x)),
                     "T2 modular distance"});
  }
  return out;
}

static std::string decimal(const APInt &value) {
  llvm::SmallString<32> text;
  value.toStringSigned(text, 10);
  return text.str().str();
}
static std::string smtInteger(const APInt &value) {
  std::string text = decimal(value);
  return text[0] == '-' ? "(- " + text.substr(1) + ")" : text;
}

/// Coefficients from concrete descent failures. Samples prove nothing: a
/// result here is one more candidate for the checks.
std::optional<Term> Proposer::cegis(llvm::ArrayRef<Term> atoms, bool isSigned,
                                    llvm::ArrayRef<unsigned> inside,
                                    const std::set<unsigned> &strict) {
  unsigned sourceWidth = 0;
  for (Term atom : atoms)
    sourceWidth = std::max(sourceWidth, atom->sort.width);
  unsigned width = sourceWidth + 6, count = atoms.size();
  struct Sample {
    unsigned path;
    std::vector<APInt> before, after;
  };
  std::vector<Sample> samples;
  std::vector<APInt> coefficient(count, APInt(width, 0));
  APInt constant(width, 0);
  auto wide = [&](Term t) { return extended(terms, t, isSigned, width); };
  auto rankOf = [&]() {
    Term rank = terms.bv(constant);
    for (unsigned k = 0; k < count; ++k)
      rank = terms.binary(TermOp::Add, rank,
                          terms.binary(TermOp::Mul, terms.bv(coefficient[k]),
                                       wide(atoms[k])));
    return rank;
  };

  for (unsigned round = 0; round < in.limits.maxCegisRounds; ++round) {
    Term rank = rankOf();
    bool failed = false;
    for (unsigned id : inside) {
      const CutPath &path = in.set.paths[id];
      Aux aux;
      for (unsigned k = 0; k < count; ++k) {
        aux.push_back({"u" + std::to_string(k), wide(atoms[k])});
        aux.push_back({"v" + std::to_string(k),
                       postTerm(program, path, wide(atoms[k]))});
      }
      SmtResponse response = ask(
          descentViolation(program, in.set, in.scope, path, invariantAt(path.from),
                           inside, rank, rank, strict.count(id) != 0),
          aux, /*wantModel=*/true);
      if (response.answer == SmtAnswer::Unsat)
        continue;
      if (response.answer != SmtAnswer::Sat)
        return std::nullopt;
      rememberParameters(response);
      Sample sample{id, {}, {}};
      for (unsigned k = 0; k < count; ++k) {
        auto read = [&](const std::string &name) {
          auto value = response.model.find(name);
          APInt bits(width, 0);
          if (value != response.model.end())
            bits = APInt(width, value->second, 10);
          return bits;
        };
        sample.before.push_back(read("u" + std::to_string(k)));
        sample.after.push_back(read("v" + std::to_string(k)));
      }
      samples.push_back(std::move(sample));
      failed = true;
      break;
    }
    if (!failed)
      return rank;

    std::string script;
    auto coefficientName = [](unsigned k) { return "a" + std::to_string(k); };
    for (unsigned k = 0; k < count; ++k) {
      script += "(declare-const " + coefficientName(k) + " Int)\n";
      script += "(assert (and (<= (- 4) " + coefficientName(k) + ") (<= " +
                coefficientName(k) + " 4)))\n";
    }
    APInt limit = APInt::getOneBitSet(width, sourceWidth + 1);
    script += "(declare-const c Int)\n(assert (and (<= " + smtInteger(-limit) +
              " c) (<= c " + smtInteger(limit) + ")))\n";
    auto linear = [&](const std::vector<APInt> &values, bool withConstant) {
      std::string sum = withConstant ? "c" : "0";
      for (unsigned k = 0; k < count; ++k)
        sum = "(+ " + sum + " (* " + coefficientName(k) + " " +
              smtInteger(values[k]) + "))";
      return sum;
    };
    for (const Sample &sample : samples) {
      script += "(assert (>= (- " + linear(sample.before, false) + " " +
                linear(sample.after, false) + ") " +
                (strict.count(sample.path) ? "1" : "0") + "))\n";
      script += "(assert (>= " + linear(sample.before, true) + " 0))\n";
      script += "(assert (>= " + linear(sample.after, true) + " 0))\n";
    }
    script += "(check-sat)\n";
    if (stats.queries >= in.limits.maxProposerQueries)
      throw BudgetExhausted{};
    ++stats.queries;
    SmtResponse solved = in.backend.check(script, in.limits.solver, /*wantModel=*/true);
    if (solved.answer != SmtAnswer::Sat)
      return std::nullopt;
    auto read = [&](const std::string &name) {
      auto value = solved.model.find(name);
      return value == solved.model.end() ? APInt(width, 0)
                                         : APInt(width, value->second, 10);
    };
    for (unsigned k = 0; k < count; ++k)
      coefficient[k] = read(coefficientName(k));
    constant = read("c");
  }
  return std::nullopt;
}

std::vector<std::pair<Term, std::string>>
Proposer::affineRanks(llvm::ArrayRef<unsigned> component,
                      llvm::ArrayRef<unsigned> inside) {
  std::vector<std::pair<Term, std::string>> out;
  // What varies in the SCC, then the values it is compared with.
  std::vector<Term> atoms;
  auto add = [&](Term t) {
    if (t && t->sort.kind == Sort::BV && !llvm::is_contained(atoms, t) &&
        atoms.size() < 6)
      atoms.push_back(t);
  };
  for (unsigned id : inside)
    for (unsigned s = 0; s < program.state.size(); ++s) {
      Term before = program.state[s].sort.kind == Sort::BV
                        ? terms.state(s, program.state[s].sort)
                        : nullptr;
      Term after = in.set.paths[id].state[s];
      if (before && after && after != before)
        add(before);
    }
  for (Term comparison : leavingComparisons(component))
    for (Term side : comparison->args)
      if (side->op == TermOp::State || side->op == TermOp::Param)
        add(side);
  if (atoms.empty())
    return out;
  for (bool isSigned : {true, false}) {
    std::set<unsigned> all(inside.begin(), inside.end());
    if (auto rank = cegis(atoms, isSigned, inside, all))
      out.push_back({*rank, "T3 bounded affine"});
    if (inside.size() > 1)
      for (unsigned pivot : inside) {
        if (auto rank = cegis(atoms, isSigned, inside, {pivot}))
          out.push_back({*rank, "T3 bounded affine"});
      }
  }
  return out;
}

void Proposer::rememberParameters(const SmtResponse &response) {
  if (stats.parameterModels.size() >= in.limits.maxReplayEntries)
    return;
  std::map<unsigned, std::string> values;
  for (const Parameter &p : program.parameters) {
    auto value = response.model.find("p" + std::to_string(p.id));
    if (value != response.model.end())
      values[p.id] = value->second;
  }
  if (std::find(stats.parameterModels.begin(), stats.parameterModels.end(),
                values) == stats.parameterModels.end())
    stats.parameterModels.push_back(std::move(values));
}

bool Proposer::attempt(Term rank, const std::string &origin,
                       llvm::ArrayRef<unsigned> component,
                       llvm::ArrayRef<unsigned> inside, RankLevel &out) {
  if (rankAttempts >= in.limits.maxRankCandidates)
    return false;
  ++rankAttempts;
  ++stats.candidates;
  std::set<unsigned> strict;
  for (unsigned id : inside) {
    const CutPath &path = in.set.paths[id];
    auto violated = [&](bool isStrict) {
      return descentViolation(program, in.set, in.scope, path,
                              invariantAt(path.from), inside, rank, rank, isStrict);
    };
    SmtResponse first = ask(violated(true), {}, /*wantModel=*/true);
    if (first.answer == SmtAnswer::Unsat) {
      strict.insert(id);
      continue;
    }
    if (first.answer == SmtAnswer::Sat)
      rememberParameters(first);
    SmtResponse second = ask(violated(false), {}, /*wantModel=*/true);
    if (second.answer != SmtAnswer::Unsat) {
      if (second.answer == SmtAnswer::Sat)
        rememberParameters(second);
      return false;
    }
  }
  if (strict.empty())
    return false;
  std::vector<unsigned> rest;
  for (unsigned id : inside)
    if (!strict.count(id))
      rest.push_back(id);
  std::vector<RankLevel> children;
  if (!solve(rest, children))
    return false;

  out = RankLevel();
  out.cuts.assign(component.begin(), component.end());
  for (unsigned cut : component)
    out.rank.push_back({cut, rank});
  for (unsigned id : inside) {
    out.paths.push_back(in.set.paths[id].transitions);
    if (strict.count(id))
      out.strict.push_back(in.set.paths[id].transitions);
  }
  out.children = std::move(children);
  out.origin = origin;
  return true;
}

bool Proposer::solveComponent(llvm::ArrayRef<unsigned> component,
                              llvm::ArrayRef<unsigned> inside, RankLevel &out) {
  std::set<Term> tried;
  auto tryAll = [&](const std::vector<std::pair<Term, std::string>> &list) {
    for (const auto &[rank, origin] : list)
      if (rank && tried.insert(rank).second &&
          attempt(rank, origin, component, inside, out))
        return true;
    return false;
  };
  std::vector<std::pair<Term, std::string>> hints;
  for (const RankHint &hint : in.hints)
    if (hint.rank && hint.rank->sort.kind == Sort::BV && !readsMemory(hint.rank))
      hints.push_back({hint.rank, hint.origin});
  return tryAll(hints) || tryAll(guardDistances(component)) ||
         tryAll(modularDistances(component, inside)) ||
         tryAll(affineRanks(component, inside));
}

bool Proposer::solve(llvm::ArrayRef<unsigned> current, std::vector<RankLevel> &out) {
  llvm::SmallVector<std::pair<unsigned, unsigned>> edges;
  for (unsigned id : current)
    edges.push_back({in.set.paths[id].from, in.set.paths[id].to});
  for (const auto &component : cyclicComponents(program.locationCount, edges)) {
    std::vector<unsigned> inside;
    for (unsigned id : current)
      if (llvm::is_contained(component, in.set.paths[id].from) &&
          llvm::is_contained(component, in.set.paths[id].to))
        inside.push_back(id);
    RankLevel level;
    if (!solveComponent(component, inside, level)) {
      if (stats.failure.empty()) {
        stats.failure = "no rank for the SCC at L" + std::to_string(component[0]);
      }
      return false;
    }
    out.push_back(std::move(level));
  }
  return true;
}

std::optional<std::vector<RankLevel>>
Proposer::levels(const std::map<unsigned, Term> &invariants) {
  known = invariants;
  std::vector<unsigned> everyCutPath;
  for (const CutPath &path : in.set.paths)
    if (!path.fromEntry && path.to != program.exitLocation())
      everyCutPath.push_back(path.id);
  std::vector<RankLevel> result;
  try {
    if (solve(everyCutPath, result))
      return result;
  } catch (const BudgetExhausted &) {
    stats.failure = "rank search ran out of queries";
  }
  return std::nullopt;
}

} // namespace

namespace mlir::generalized_gemm {

std::map<unsigned, Term> discoverInvariants(const ProposalInput &input,
                                            ProposalStatistics &statistics) {
  return Proposer(input, statistics).invariants();
}

std::optional<std::vector<RankLevel>>
proposeRankLevels(const ProposalInput &input,
                  const std::map<unsigned, Term> &invariants,
                  ProposalStatistics &statistics) {
  return Proposer(input, statistics).levels(invariants);
}

} // namespace mlir::generalized_gemm
