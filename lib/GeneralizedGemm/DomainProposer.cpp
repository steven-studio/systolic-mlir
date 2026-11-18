//===- DomainProposer.cpp -------------------------------------------------===//
#include "GeneralizedGemm/DomainProposer.h"

#include "GeneralizedGemm/GuardedOrbit.h"

#include "llvm/ADT/DenseSet.h"

#include <algorithm>
#include <array>
#include <functional>
#include <map>
#include <set>

using llvm::APInt;

namespace mlir::generalized_gemm {

namespace {

/// The sample events of one point: each level candidate's value, in event order.
struct Sample {
  std::vector<APInt> parameters;
  std::vector<std::vector<APInt>> values; // [event][candidate]
  std::vector<std::vector<uint64_t>> cells;
  // Phase 4 (I3): the recorded indices at each event, per view (0 output h, 1 A's f,
  // 2 B's g), so that a proposed view can be checked against every sampled event of
  // every site before it is proposed. Empty when the caller has none.
  std::vector<std::array<std::vector<APInt>, 3>> access;
};

bool increasing(const GuardedOrder &order, const std::vector<unsigned> &use,
                const std::vector<Sample> &samples) {
  for (const Sample &s : samples)
    for (size_t e = 1; e < s.values.size(); ++e) {
      std::vector<APInt> a, b;
      for (unsigned c : use) {
        a.push_back(s.values[e - 1][c]);
        b.push_back(s.values[e][c]);
      }
      if (!concreteLess(order, a, b))
        return false;
    }
  return true;
}

/// A level is compared signed where a sample value has its top bit set and none sits at
/// the signed top: a loop from -1 is such. A guess the checker judges.
bool guessSigned(unsigned candidate, const std::vector<Sample> &samples) {
  bool negative = false, signedTop = false;
  for (const Sample &s : samples)
    for (const auto &row : s.values) {
      negative = negative || row[candidate].isNegative();
      signedTop = signedTop || row[candidate].isMaxSignedValue();
    }
  return negative && !signedTop;
}

/// Directions read from the samples: where every outer level is still, the level moves
/// one way only, in its signedness. Nullopt when it moves both ways.
std::optional<GuardedOrder> directions(const std::vector<Term> &candidates,
                                       const std::vector<unsigned> &use,
                                       const std::vector<Sample> &samples) {
  GuardedOrder order;
  for (unsigned m = 0; m < use.size(); ++m) {
    GuardedLevel level;
    level.value = candidates[use[m]];
    level.isSigned = guessSigned(use[m], samples);
    bool up = false, down = false;
    for (const Sample &s : samples)
      for (size_t e = 1; e < s.values.size(); ++e) {
        bool still = true;
        for (unsigned o = 0; o < m && still; ++o)
          still = s.values[e - 1][use[o]] == s.values[e][use[o]];
        if (!still)
          continue;
        const APInt &a = s.values[e - 1][use[m]], &b = s.values[e][use[m]];
        up = up || (level.isSigned ? a.slt(b) : a.ult(b));
        down = down || (level.isSigned ? b.slt(a) : b.ult(a));
      }
    if (up && down)
      return std::nullopt;
    level.descending = down;
    order.levels.push_back(level);
  }
  return order;
}

/// A bound fitted to one value per point: a constant, a parameter plus a constant,
/// or (phase 4) an identity's term over parameters alone plus a constant.
Term fit(const SemanticProgram &program, const std::vector<std::pair<std::vector<APInt>, APInt>> &want,
         unsigned width, const std::vector<StateIdentity> &identities = {}) {
  TermContext &terms = *program.terms;
  if (want.empty())
    return nullptr;
  bool constant = true;
  for (const auto &[point, value] : want)
    constant = constant && value == want.front().second;
  if (constant)
    return terms.bv(want.front().second);
  for (const Parameter &p : program.parameters) {
    if (p.sort != Sort::bv(width))
      continue;
    APInt offset = want.front().second - want.front().first[p.id];
    bool all = true;
    for (const auto &[point, value] : want)
      all = all && value == point[p.id] + offset;
    if (all)
      return offset.isZero() ? terms.param(p.id, p.sort)
                             : terms.binary(TermOp::Add, terms.param(p.id, p.sort), terms.bv(offset));
  }
  for (const StateIdentity &identity : identities) {
    if (!identity.term || identity.term->sort != Sort::bv(width) || !stateSupport(identity.term).empty() ||
        containsOp(identity.term, TermOp::Load))
      continue;
    std::optional<APInt> offset;
    bool all = true;
    for (const auto &[point, value] : want) {
      auto base = evaluateLevelTerm(identity.term, point, {});
      if (!base) {
        all = false;
        break;
      }
      APInt here = value - *base;
      if (!offset)
        offset = here;
      all = all && *offset == here;
    }
    if (all && offset)
      return offset->isZero() ? identity.term : terms.binary(TermOp::Add, identity.term, terms.bv(*offset));
  }
  return nullptr;
}

/// Phase 4: reads every state variable with an identity as the identity's term, once
/// (an identity's term may read a level, never another identified variable).
Term applyIdentities(TermContext &terms, Term term, const std::vector<StateIdentity> &identities) {
  if (identities.empty())
    return term;
  return terms.substitute(term, [&](Term leaf) -> Term {
    if (leaf->op != TermOp::State)
      return nullptr;
    for (const StateIdentity &identity : identities)
      if (identity.state == leaf->index && identity.term && identity.term->sort == leaf->sort)
        return identity.term;
    return nullptr;
  });
}

/// Truth values of the program's conditions, as terms over level variables, from the
/// guards of every transition and their boolean parts. A state variable that holds one
/// value at every sample event is read as that constant: a guess, which the checker
/// proves or refutes like the rest. Congruences of each level by small moduli are
/// added, for steps, where the level's width holds the modulus.
std::vector<Term> conditionAtoms(const SemanticProgram &program, const GuardedOrder &order,
                                 const std::map<unsigned, APInt> &constants, unsigned limit,
                                 const std::vector<StateIdentity> &identities = {}) {
  TermContext &terms = *program.terms;
  std::map<unsigned, unsigned> levelOfState;
  for (unsigned m = 0; m < order.levels.size(); ++m)
    if (order.levels[m].value->op == TermOp::State)
      levelOfState[order.levels[m].value->index] = m;
  std::vector<Term> out;
  std::set<Term> seen;
  std::function<void(Term)> collect = [&](Term original) {
    if (out.size() >= limit || original->sort != Sort::bv(1) || !seen.insert(original).second)
      return;
    if (original->op == TermOp::And || original->op == TermOp::Or || original->op == TermOp::Not ||
        original->op == TermOp::Xor)
      for (Term arg : original->args)
        collect(arg);
    Term term = applyIdentities(terms, original, identities);
    bool ok = true;
    for (unsigned id : stateSupport(term))
      ok = ok && (levelOfState.count(id) || constants.count(id));
    if (!ok || containsOp(term, TermOp::Load) || containsOp(term, TermOp::Shadow) ||
        term->op == TermOp::Const)
      return;
    Term mapped = terms.substitute(term, [&](Term leaf) -> Term {
      if (leaf->op != TermOp::State)
        return nullptr;
      auto level = levelOfState.find(leaf->index);
      if (level != levelOfState.end())
        return levelVariable(terms, level->second, order.levels[level->second].value->sort);
      return terms.bv(constants.at(leaf->index));
    });
    out.push_back(mapped);
  };
  for (const Transition &t : program.transitions)
    collect(t.guard);
  for (unsigned m = 0; m < order.levels.size() && out.size() < limit; ++m) {
    Sort sort = order.levels[m].value->sort;
    Term y = levelVariable(terms, m, sort);
    for (uint64_t modulus = 2; modulus <= 4; ++modulus) {
      // A modulus the level's width cannot hold is no congruence of that level: the atom
      // is skipped, never built from a truncated constant. A one-bit level, which the
      // search may propose from a comparison, gets none; a two-bit level gets 2 and 3.
      // Every rest below the modulus then fits as well.
      if (sort.width < 64 && (modulus >> sort.width) != 0)
        continue;
      for (uint64_t rest = 0; rest < modulus; ++rest)
        out.push_back(terms.icmp(Predicate::Eq,
                                 terms.binary(TermOp::URem, y, terms.bv(APInt(sort.width, modulus))),
                                 terms.bv(APInt(sort.width, rest))));
    }
  }
  return out;
}

bool exactAt(const GuardedOrder &order, const std::vector<unsigned> &use,
             const std::vector<Sample> &samples, uint64_t limit) {
  for (const Sample &s : samples) {
    std::string why;
    auto set = enumerateGuardedSet(order, s.parameters, limit, why);
    if (!set || set->size() != s.values.size())
      return false;
    for (size_t e = 0; e < s.values.size(); ++e)
      for (unsigned m = 0; m < use.size(); ++m)
        if ((*set)[e][m] != s.values[e][use[m]])
          return false;
  }
  return true;
}

} // namespace

/// What the fitting gives: the levels used (indices into the candidates), the order with
/// its bounds and guard, the cell key, and closed forms of one site's views.
struct Fitted {
  std::vector<unsigned> use;
  GuardedOrder order;
  std::vector<unsigned> cellKey;
  std::array<std::vector<Term>, 3> access;
};

/// Fits an order to `samples` over `candidates`, whose first `coordinates` are the
/// family's and must all be levels; the rest may become repeat levels. `constants` are
/// state variables with one value at every sample event, read as that value in guards.
std::optional<Fitted> fitOrder(const SemanticProgram &program, const std::vector<Term> &candidates,
                               unsigned coordinates, const EnumerationOrder *coordinateOrder,
                               const std::vector<Sample> &samples, const std::map<unsigned, APInt> &constants,
                               const MacSite *site, const ProposalLimits &limits, std::string &why) {
  TermContext &terms = *program.terms;
  // Coordinates slowest first, unless an order is given.
  std::vector<unsigned> use;
  for (unsigned c = 0; c < coordinates; ++c)
    use.push_back(c);
  if (!coordinateOrder) {
    std::vector<uint64_t> changes(coordinates, 0);
    for (const Sample &s : samples)
      for (size_t e = 1; e < s.values.size(); ++e)
        for (unsigned c = 0; c < coordinates; ++c)
          changes[c] += s.values[e - 1][c] != s.values[e][c];
    std::stable_sort(use.begin(), use.end(), [&](unsigned a, unsigned b) {
      bool plainA = candidates[a]->op == TermOp::State, plainB = candidates[b]->op == TermOp::State;
      if (changes[a] != changes[b])
        return changes[a] < changes[b];
      return plainA && !plainB;
    });
  }
  // Repeat levels where the coordinates do not order the events.
  auto attempt = [&](const std::vector<unsigned> &levels) -> std::optional<GuardedOrder> {
    auto order = directions(candidates, levels, samples);
    if (order && increasing(*order, levels, samples))
      return order;
    return std::nullopt;
  };
  // How many adjacent events a candidate set of levels leaves out of order, or nothing
  // when some level moves both ways.
  auto violations = [&](const std::vector<unsigned> &levels) -> std::optional<uint64_t> {
    auto order = directions(candidates, levels, samples);
    if (!order)
      return std::nullopt;
    uint64_t count = 0;
    for (const Sample &s : samples)
      for (size_t e = 1; e < s.values.size(); ++e) {
        std::vector<APInt> a, b;
        for (unsigned c : levels) {
          a.push_back(s.values[e - 1][c]);
          b.push_back(s.values[e][c]);
        }
        count += !concreteLess(*order, a, b);
      }
    return count;
  };
  std::optional<GuardedOrder> order = attempt(use);
  for (unsigned extra = 0; !order && extra < limits.maxRepeatLevels; ++extra) {
    // The repeat level that leaves fewest events out of order, at its best position,
    // even when some remain: the next one may settle them.
    std::optional<uint64_t> best;
    std::vector<unsigned> bestLevels;
    for (unsigned c = coordinates; c < candidates.size(); ++c) {
      if (llvm::is_contained(use, c))
        continue;
      // Innermost first: a repeat level inside the logical ones leaves their bounds free
      // of it, which Theorem 14 needs; an outer position is taken only when it orders
      // more events.
      for (unsigned at = use.size() + 1; at-- > 0;) {
        std::vector<unsigned> next = use;
        next.insert(next.begin() + at, c);
        auto count = violations(next);
        if (count && (!best || *count < *best)) {
          best = count;
          bestLevels = next;
        }
      }
    }
    if (!best)
      break;
    use = bestLevels;
    if (*best == 0)
      order = attempt(use);
  }
  if (!order) {
    why = "no lexicographic order of these terms fits the samples";
    return std::nullopt;
  }

  auto complete = [&](const std::vector<unsigned> &use, GuardedOrder &order,
                      std::string &failure) -> bool {
    // A box around the samples, with bounds that move with an outer level kept aside.
    std::vector<std::pair<Term, Term>> outerBounds(use.size(), {nullptr, nullptr});
    for (unsigned m = 0; m < use.size(); ++m) {
      unsigned width = order.levels[m].value->sort.width;
      // Extremes in the level's own signedness, or -1 would read as the top value.
      bool sgn = guessSigned(use[m], samples);
      auto less = [sgn](const APInt &a, const APInt &b) { return sgn ? a.slt(b) : a.ult(b); };
      std::vector<std::pair<std::vector<APInt>, APInt>> lows, highs;
      APInt globalLow = sgn ? APInt::getSignedMaxValue(width) : APInt::getMaxValue(width);
      APInt globalHigh = sgn ? APInt::getSignedMinValue(width) : APInt(width, 0);
      bool any = false;
      for (const Sample &s : samples) {
        if (s.values.empty())
          continue;
        APInt low = s.values[0][use[m]], high = low;
        for (const auto &row : s.values) {
          low = less(row[use[m]], low) ? row[use[m]] : low;
          high = less(high, row[use[m]]) ? row[use[m]] : high;
        }
        lows.push_back({s.parameters, low});
        highs.push_back({s.parameters, high + 1});
        globalLow = less(low, globalLow) ? low : globalLow;
        globalHigh = less(globalHigh, high + 1) ? high + 1 : globalHigh;
        any = true;
      }
      if (!any) {
        failure = "no sample has events";
        return false;
      }
      Term lower = fit(program, lows, width, limits.identities),
           upper = fit(program, highs, width, limits.identities);
      // A bound that moves with an outer level: for each value of that level the
      // extreme is the level's value plus one constant.
      auto fitOuter = [&](bool wantUpper) -> Term {
        // The nearest outer level first: it is the one most likely to drive this bound.
        for (unsigned o = m; o-- > 0;) {
          std::map<std::pair<std::vector<uint64_t>, uint64_t>, APInt> extreme;
          for (const Sample &s : samples)
            for (const auto &row : s.values) {
              std::vector<uint64_t> point;
              for (const APInt &p : s.parameters)
                point.push_back(p.getZExtValue());
              auto [slot, added] = extreme.try_emplace({point, row[use[o]].getZExtValue()}, row[use[m]]);
              if (!added)
                slot->second = wantUpper ? (less(slot->second, row[use[m]]) ? row[use[m]] : slot->second)
                                         : (less(row[use[m]], slot->second) ? row[use[m]] : slot->second);
            }
          if (extreme.empty() || order.levels[o].value->sort.width != width)
            continue;
          Term y = levelVariable(terms, o, order.levels[o].value->sort);
          // bound = level + c, or bound = level + parameter + c.
          auto constantOffset = [&](const std::function<APInt(const std::pair<std::vector<uint64_t>, uint64_t> &)> &base)
              -> std::optional<APInt> {
            std::optional<APInt> offset;
            for (const auto &[key, value] : extreme) {
              APInt bound = wantUpper ? value + 1 : value;
              APInt c = bound - base(key);
              if (!offset)
                offset = c;
              else if (*offset != c)
                return std::nullopt;
            }
            return offset;
          };
          if (auto c = constantOffset([&](const auto &key) { return APInt(width, key.second); }))
            return c->isZero() ? y : terms.binary(TermOp::Add, y, terms.bv(*c));
          for (const Parameter &p : program.parameters) {
            if (p.sort != Sort::bv(width))
              continue;
            auto c = constantOffset([&](const auto &key) {
              return APInt(width, key.second) + APInt(width, key.first[p.id], /*isSigned=*/false,
                                                      /*implicitTrunc=*/true);
            });
            if (c) {
              Term sum = terms.binary(TermOp::Add, y, terms.param(p.id, p.sort));
              return c->isZero() ? sum : terms.binary(TermOp::Add, sum, terms.bv(*c));
            }
          }
        }
        return nullptr;
      };
      outerBounds[m] = {fitOuter(false), fitOuter(true)};
      order.levels[m].lower = lower ? lower : terms.bv(globalLow);
      order.levels[m].upper = upper ? upper : terms.bv(globalHigh);
      order.levels[m].isSigned = guessSigned(use[m], samples);
      // A level that is a function of outer levels held in plain state has one value
      // for each of them: bound it to exactly that, or the box grows with it.
      Term value = order.levels[m].value;
      if (value->op != TermOp::State) {
        std::map<unsigned, unsigned> outer;
        for (unsigned o = 0; o < m; ++o)
          if (order.levels[o].value->op == TermOp::State)
            outer[order.levels[o].value->index] = o;
        bool inside = true;
        for (unsigned id : stateSupport(value))
          inside = inside && outer.count(id);
        if (inside && !containsOp(value, TermOp::Load) && !containsOp(value, TermOp::Shadow)) {
          Term exact = terms.substitute(value, [&](Term leaf) -> Term {
            if (leaf->op != TermOp::State)
              return nullptr;
            unsigned o = outer.at(leaf->index);
            return levelVariable(terms, o, order.levels[o].value->sort);
          });
          // [x, x + 1) is empty where x + 1 wraps; the signedness chosen above makes
          // the comparison that keeps it nonempty where a sample sits at the top.
          order.levels[m].lower = exact;
          order.levels[m].upper = terms.binary(TermOp::Add, exact, terms.bv(APInt(width, 1)));
        }
      }
    }
    // The guard.
    std::vector<Term> atoms = conditionAtoms(program, order, constants, limits.maxAtoms, limits.identities);
    auto holds = [&](Term atom, const Sample &s, size_t e) {
      std::vector<APInt> levels;
      for (unsigned c : use)
        levels.push_back(s.values[e][c]);
      auto value = evaluateLevelTerm(atom, s.parameters, levels);
      return value && value->isOne();
    };
    order.guard = terms.boolean(true);
    if (exactAt(order, use, samples, limits.maxBox))
      return true;
    // Bounds that move with an outer level, before any guard: a triangle or a repeat
    // that grows with its loop is a box of varying extent, and no guard is needed. Each
    // is taken, innermost first, only where it brings the box nearer the events.
    auto slack = [&](const GuardedOrder &candidate) -> std::optional<uint64_t> {
      uint64_t extra = 0;
      for (const Sample &s : samples) {
        std::string why;
        auto set = enumerateGuardedSet(candidate, s.parameters, limits.maxBox, why);
        if (!set || set->size() < s.values.size())
          return std::nullopt;
        extra += set->size() - s.values.size();
      }
      return extra;
    };
    std::optional<uint64_t> current = slack(order);
    for (unsigned m = use.size(); m-- > 0;)
      for (bool upper : {true, false}) {
        Term bound = upper ? outerBounds[m].second : outerBounds[m].first;
        if (!bound)
          continue;
        GuardedOrder trial = order;
        (upper ? trial.levels[m].upper : trial.levels[m].lower) = bound;
        auto next = slack(trial);
        if (next && (!current || *next < *current)) {
          order = trial;
          current = next;
        }
      }
    if (exactAt(order, use, samples, limits.maxBox))
      return true;
    Term conjunction = terms.boolean(true);
    for (Term atom : atoms)
      for (Term literal : {atom, terms.logicalNot(atom)}) {
        bool always = true;
        for (const Sample &s : samples)
          for (size_t e = 0; e < s.values.size() && always; ++e)
            always = holds(literal, s, e);
        if (always)
          conjunction = terms.logicalAnd(conjunction, literal);
      }
    order.guard = conjunction;
    if (exactAt(order, use, samples, limits.maxBox))
      return true;
    // Piecewise: one conjunction of literals per pattern of truth values.
    std::set<std::vector<bool>> patterns;
    for (const Sample &s : samples)
      for (size_t e = 0; e < s.values.size(); ++e) {
        std::vector<bool> pattern;
        for (Term atom : atoms)
          pattern.push_back(holds(atom, s, e));
        patterns.insert(pattern);
      }
    if (patterns.size() > limits.maxDisjuncts) {
      failure = "too many patterns for a piecewise guard";
      return false;
    }
    Term disjunction = terms.boolean(false);
    for (const auto &pattern : patterns) {
      Term conj = terms.boolean(true);
      for (unsigned a = 0; a < atoms.size(); ++a)
        conj = terms.logicalAnd(conj, pattern[a] ? atoms[a] : terms.logicalNot(atoms[a]));
      disjunction = terms.logicalOr(disjunction, conj);
    }
    order.guard = disjunction;
    if (exactAt(order, use, samples, limits.maxBox))
      return true;
    failure = "no guard from the program's conditions is exact at the samples";
    return false;
  };

  if (!complete(use, *order, why)) {
    // The conditions may read state that is no level: add such state as levels, where
    // the order still fits, and try once more.
    std::set<unsigned> wanted;
    for (const Transition &t : program.transitions)
      if (!containsOp(t.guard, TermOp::Load))
        for (unsigned id : stateSupport(t.guard))
          if (!constants.count(id))
            wanted.insert(id);
    std::vector<unsigned> wider = use;
    for (unsigned c = coordinates; c < candidates.size(); ++c) {
      Term leaf = candidates[c];
      if (leaf->op != TermOp::State || !wanted.count(leaf->index) || llvm::is_contained(wider, c))
        continue;
      bool placed = false;
      for (unsigned at = wider.size() + 1; at-- > 0 && !placed;) {
        std::vector<unsigned> next = wider;
        next.insert(next.begin() + at, c);
        if (auto widened = attempt(next)) {
          wider = next;
          placed = true;
        }
      }
    }
    if (wider.size() == use.size())
      return std::nullopt;
    auto widened = attempt(wider);
    if (!widened)
      return std::nullopt;
    std::string again;
    if (!complete(wider, *widened, again)) {
      why = again;
      return std::nullopt;
    }
    why.clear();
    use = wider;
    order = std::move(widened);
  }

  Fitted out;
  out.use = use;
  out.order = std::move(*order);
  out.order.origin = "fitted to the samples";
  // The key: the levels that change within a cell, if they order each cell.
  std::vector<unsigned> moving;
  for (unsigned m = 0; m < use.size(); ++m) {
    bool moves = false;
    for (const Sample &s : samples) {
      std::map<std::vector<uint64_t>, size_t> last;
      for (size_t e = 0; e < s.values.size(); ++e) {
        auto [it, added] = last.try_emplace(s.cells[e], e);
        if (!added) {
          moves = moves || s.values[it->second][use[m]] != s.values[e][use[m]];
          it->second = e;
        }
      }
    }
    if (moves)
      moving.push_back(m);
  }
  out.cellKey = moving.empty() ? std::vector<unsigned>{(unsigned)use.size() - 1} : moving;
  // Access forms (views). First the index terms of the first site, with the
  // identities applied, when they read only levels held in plain state; phase 4 adds
  // a check of each proposed dimension against the recorded indices of every
  // sampled event of every site, and, where the term fails or cannot be written over
  // levels, the level whose values equal that dimension at every event. A term that
  // reads a state that is no level (an scf.while body's counter) is tried with that
  // state read as each level. A view with a dimension no candidate fits is not
  // proposed. All of it is a proposal.
  std::map<unsigned, unsigned> levelOfState;
  for (unsigned m = 0; m < out.order.levels.size(); ++m)
    if (out.order.levels[m].value->op == TermOp::State)
      levelOfState[out.order.levels[m].value->index] = m;
  bool recorded = false;
  for (const Sample &s : samples)
    recorded = recorded || !s.access.empty();
  auto fitsEvents = [&](unsigned view, unsigned d, Term form) {
    if (!recorded)
      return true;
    for (const Sample &s : samples) {
      if (s.access.size() != s.values.size())
        return false;
      for (size_t e = 0; e < s.values.size(); ++e) {
        const std::vector<APInt> &index = s.access[e][view];
        if (d >= index.size())
          return false;
        std::vector<APInt> levels;
        for (unsigned c : use)
          levels.push_back(s.values[e][c]);
        auto value = evaluateLevelTerm(form, s.parameters, levels);
        if (!value || *value != index[d])
          return false;
      }
    }
    return true;
  };
  for (unsigned view = 0; view < 3 && site; ++view) {
    llvm::ArrayRef<Term> index = view == 0 ? llvm::ArrayRef<Term>(site->output->index)
                               : view == 1 ? llvm::ArrayRef<Term>(site->a.index)
                                           : llvm::ArrayRef<Term>(site->b.index);
    std::vector<Term> form;
    bool ok = true;
    for (unsigned d = 0; d < index.size() && ok; ++d) {
      Term term = applyIdentities(terms, index[d], limits.identities);
      bool overLevels = !containsOp(term, TermOp::Load);
      for (unsigned id : stateSupport(term))
        overLevels = overLevels && levelOfState.count(id);
      Term chosen = nullptr;
      if (overLevels) {
        Term mapped = terms.substitute(term, [&](Term leaf) -> Term {
          if (leaf->op != TermOp::State)
            return nullptr;
          unsigned m = levelOfState[leaf->index];
          return levelVariable(terms, m, out.order.levels[m].value->sort);
        });
        if (fitsEvents(view, d, mapped))
          chosen = mapped;
      } else if (recorded && !containsOp(term, TermOp::Load) && !containsOp(term, TermOp::Shadow)) {
        // A state that is no level, read as a level: the argument of an scf.while's after
        // region holds the counter its before region compares, and the body's indices are
        // written over it. Each such state (three at most) is tried as each level of its sort,
        // and the first reading under which the form is the recorded index at every sampled
        // event of every site is proposed. A proposal like the others: the checker proves it
        // over the scope, or the view is not proved.
        std::vector<unsigned> open;
        for (unsigned id : stateSupport(term))
          if (!levelOfState.count(id))
            open.push_back(id);
        const size_t count = out.order.levels.size();
        uint64_t readings = 1;
        for (size_t n = 0; n < open.size(); ++n)
          readings *= count;
        for (uint64_t r = 0; !chosen && open.size() <= 3 && r < readings; ++r) {
          std::map<unsigned, unsigned> levelOf;
          uint64_t rest = r;
          bool sorts = true;
          for (unsigned id : open) {
            unsigned m = rest % count;
            rest /= count;
            levelOf[id] = m;
            sorts = sorts && out.order.levels[m].value->sort == program.state[id].sort;
          }
          if (!sorts)
            continue;
          Term mapped = terms.substitute(term, [&](Term leaf) -> Term {
            if (leaf->op != TermOp::State)
              return nullptr;
            auto found = levelOf.find(leaf->index);
            unsigned m = found != levelOf.end() ? found->second : levelOfState[leaf->index];
            return levelVariable(terms, m, out.order.levels[m].value->sort);
          });
          if (fitsEvents(view, d, mapped))
            chosen = mapped;
        }
      }
      for (unsigned m = 0; !chosen && recorded && m < out.order.levels.size(); ++m) {
        if (out.order.levels[m].value->sort != index[d]->sort)
          continue;
        Term y = levelVariable(terms, m, out.order.levels[m].value->sort);
        if (fitsEvents(view, d, y))
          chosen = y;
      }
      if (chosen)
        form.push_back(chosen);
      else
        ok = false;
    }
    if (ok)
      out.access[view] = std::move(form);
  }
  return out;
}

std::vector<Term> proposalConditionAtoms(const SemanticProgram &program, const GuardedOrder &order,
                                         const std::map<unsigned, APInt> &constants, unsigned limit) {
  return conditionAtoms(program, order, constants, limit);
}

std::optional<DomainRequest> proposeDomain(const SemanticProgram &program,
                                           const DomainEnumeration &enumeration, unsigned g,
                                           const CoordinateFamily &family,
                                           const EnumerationOrder *coordinateOrder,
                                           const ProposalLimits &limits, std::string &why) {
  TermContext &terms = *program.terms;
  if (enumeration.gate != GateStatus::Open || g >= enumeration.groups.size()) {
    why = "no samples";
    return std::nullopt;
  }
  // Candidates: the family's coordinates, then every live state variable.
  std::vector<Term> candidates;
  std::set<Term> seen;
  auto offer = [&](Term term) {
    if (term && seen.insert(term).second)
      candidates.push_back(term);
  };
  if (coordinateOrder)
    for (const OrderLevel &level : coordinateOrder->levels)
      offer(level.value);
  for (const auto &axis : family.axes)
    for (Term term : axis)
      offer(term);
  unsigned coordinates = candidates.size();
  std::vector<Sample> samples;
  for (const PointEvidence &point : enumeration.points) {
    if (!point.reason.empty() || !point.domains[g])
      continue;
    const AccessDomain &domain = *point.domains[g];
    for (unsigned id : domain.table().coordinateNames)
      offer(terms.state(id, program.state[id].sort));
  }
  for (const PointEvidence &point : enumeration.points) {
    if (!point.reason.empty())
      continue;
    Sample s;
    s.parameters = point.parameters;
    if (point.domains[g]) {
      const AccessDomain &domain = *point.domains[g];
      EventEvaluator values(program, domain);
      for (size_t e = 0; e < domain.table().events().size(); ++e) {
        std::vector<APInt> row;
        for (Term term : candidates) {
          auto value = values.value(term, e);
          row.push_back(value ? *value : APInt(term->sort.width, 0));
        }
        s.values.push_back(std::move(row));
        const AccessEvent &event = domain.table().events()[e];
        std::vector<uint64_t> cell;
        for (const APInt &value : domain.table().view(event.h))
          cell.push_back(value.getZExtValue());
        s.cells.push_back(std::move(cell));
        std::array<std::vector<APInt>, 3> access;
        for (const APInt &value : domain.table().view(event.h))
          access[0].push_back(value);
        for (const APInt &value : domain.table().view(event.f))
          access[1].push_back(value);
        for (const APInt &value : domain.table().view(event.g))
          access[2].push_back(value);
        s.access.push_back(std::move(access));
      }
    }
    samples.push_back(std::move(s));
  }

  // State that holds one value at every sample event.
  std::map<unsigned, APInt> constants;
  {
    std::map<unsigned, std::optional<APInt>> seenValue;
    std::set<unsigned> varies;
    for (const PointEvidence &point : enumeration.points) {
      if (!point.reason.empty() || !point.domains[g])
        continue;
      const AccessDomain &domain = *point.domains[g];
      EventEvaluator values(program, domain);
      for (unsigned id : domain.table().coordinateNames) {
        Term leaf = terms.state(id, program.state[id].sort);
        for (size_t e = 0; e < domain.table().events().size(); ++e) {
          auto value = values.value(leaf, e);
          auto &known = seenValue[id];
          if (!value || (known && *known != *value))
            varies.insert(id);
          else
            known = *value;
        }
      }
    }
    for (const auto &[id, value] : seenValue)
      if (value && !varies.count(id))
        constants.emplace(id, *value);
  }
  const SiteGroup &group = enumeration.groups[g];
  auto fitted = fitOrder(program, candidates, coordinates, coordinateOrder, samples, constants,
                         &program.sites[group.sites.front()], limits, why);
  if (!fitted)
    return std::nullopt;
  DomainRequest out;
  out.order = std::move(fitted->order);
  out.cellKey = std::move(fitted->cellKey);
  out.access = std::move(fitted->access);
  return out;
}

/// Segments of a group: sites whose events never interleave at any sample point, in the
/// order of their first site, which is program order. Two sites interleave at a point when
/// one has an event between two of the other's; such sites share a segment, transitively.
/// (The order of first appearance would misplace a segment that is empty at the first
/// point; the checker judges the order either way.)
static std::vector<std::vector<unsigned>> segmentSites(const std::vector<std::vector<unsigned>> &siteSequences) {
  std::vector<unsigned> order; // Sites by first appearance.
  std::map<unsigned, unsigned> parent;
  auto find = [&](unsigned site) {
    while (parent[site] != site)
      site = parent[site] = parent[parent[site]];
    return site;
  };
  auto unite = [&](unsigned a, unsigned b) { parent[find(a)] = find(b); };
  for (const auto &sequence : siteSequences) {
    std::map<unsigned, std::pair<size_t, size_t>> span; // Within this point.
    for (size_t e = 0; e < sequence.size(); ++e) {
      unsigned site = sequence[e];
      if (!parent.count(site)) {
        parent[site] = site;
        order.push_back(site);
      }
      auto [slot, added] = span.try_emplace(site, std::make_pair(e, e));
      if (!added)
        slot->second.second = e;
    }
    for (const auto &[a, sa] : span)
      for (const auto &[b, sb] : span)
        if (a < b && sa.first <= sb.second && sb.first <= sa.second)
          unite(a, b);
  }
  std::vector<std::vector<unsigned>> segments;
  std::map<unsigned, size_t> indexOfRoot;
  std::sort(order.begin(), order.end());
  for (unsigned site : order) {
    unsigned root = find(site);
    auto [slot, added] = indexOfRoot.try_emplace(root, segments.size());
    if (added)
      segments.push_back({});
    segments[slot->second].push_back(site);
  }
  return segments;
}

/// Whether a state term is named on any axis of a family.
static bool namedIn(const CoordinateFamily &family, Term state) {
  for (const std::vector<Term> &axis : family.axes)
    if (llvm::is_contained(axis, state))
      return true;
  return false;
}

std::optional<DomainRequest> proposeSegmentedDomain(const SemanticProgram &program,
                                                    const DomainEnumeration &enumeration, unsigned g,
                                                    const CoordinateFamily *logical,
                                                    const ProposalLimits &limits, std::string &why) {
  TermContext &terms = *program.terms;
  if (enumeration.gate != GateStatus::Open || g >= enumeration.groups.size()) {
    why = "no samples";
    return std::nullopt;
  }
  const SiteGroup &group = enumeration.groups[g];
  std::set<unsigned> members(group.sites.begin(), group.sites.end());
  // Every event of the group at every sample point, with the state at its block entry.
  struct Event {
    unsigned site;
    std::vector<std::optional<APInt>> state;
    std::vector<uint64_t> cell;
    std::array<std::vector<APInt>, 3> access; // h, f, g.
  };
  struct PointEvents {
    std::vector<APInt> parameters;
    std::vector<Event> events;
  };
  std::vector<PointEvents> points;
  std::vector<std::vector<unsigned>> siteSequences;
  for (const PointEvidence &point : enumeration.points) {
    if (!point.reason.empty())
      continue;
    auto states = eventStates(program, point.parameters, 0, limits.events);
    if (!states || states->run.status != RunStatus::Returned)
      continue;
    PointEvents pe;
    pe.parameters = point.parameters;
    std::vector<unsigned> sequence;
    for (size_t e = 0; e < states->run.events.size(); ++e) {
      const MacEvent &event = states->run.events[e];
      if (!members.count(event.site))
        continue;
      Event row;
      row.site = event.site;
      row.state = states->states[e];
      for (const APInt &value : event.h)
        row.cell.push_back(value.getZExtValue());
      row.access = {std::vector<APInt>(event.h.begin(), event.h.end()),
                    std::vector<APInt>(event.f.begin(), event.f.end()),
                    std::vector<APInt>(event.g.begin(), event.g.end())};
      pe.events.push_back(std::move(row));
      sequence.push_back(event.site);
    }
    siteSequences.push_back(std::move(sequence));
    points.push_back(std::move(pe));
  }
  if (points.empty()) {
    why = "no sample point ran";
    return std::nullopt;
  }
  std::vector<std::vector<unsigned>> segments = segmentSites(siteSequences);
  // Sites with no event at any sample point still need a segment: last, alone.
  for (unsigned site : group.sites) {
    bool placed = false;
    for (const auto &segment : segments)
      placed = placed || llvm::is_contained(segment, site);
    if (!placed)
      segments.push_back({site});
  }
  std::vector<bool> independent = memoryIndependentState(program);

  DomainRequest out;
  for (const std::vector<unsigned> &sites : segments) {
    std::set<unsigned> mine(sites.begin(), sites.end());
    // State defined at every event of the segment, memory-independent and integer.
    // Defined at every event of the segment; varying when two events of one point
    // differ, since a variable frozen at another loop's final value differs only
    // between points; constant when one value serves every event of every point.
    std::vector<bool> defined(program.state.size(), true), varies(program.state.size(), false);
    std::vector<std::optional<APInt>> first(program.state.size());
    bool oneValue = true;
    size_t count = 0;
    for (const PointEvents &pe : points) {
      std::vector<std::optional<APInt>> here(program.state.size());
      for (const Event &event : pe.events) {
        if (!mine.count(event.site))
          continue;
        ++count;
        for (unsigned id = 0; id < program.state.size(); ++id) {
          if (!event.state[id]) {
            defined[id] = false;
            continue;
          }
          if (!here[id])
            here[id] = event.state[id];
          else if (*here[id] != *event.state[id])
            varies[id] = true;
          if (!first[id])
            first[id] = event.state[id];
          else if (*first[id] != *event.state[id])
            oneValue = oneValue && false;
        }
      }
    }
    (void)oneValue;
    std::vector<Term> candidates;
    std::map<unsigned, APInt> constants;
    auto usable = [&](unsigned id) {
      return defined[id] && independent[id] && program.state[id].sort.kind == Sort::BV && count > 0;
    };
    // Loop variables first, as the segment's coordinates, or with `logical` given only
    // the ones it names; then other varying state as repeat candidates; state with one
    // value is a constant for the guards.
    auto coordinate = [&](const StateVar &s) {
      return (s.kind == StateVar::BlockArgument || s.kind == StateVar::IterationCounter) && s.loopCarried &&
             (!logical || namedIn(*logical, terms.state(s.id, s.sort)));
    };
    for (const StateVar &s : program.state)
      if (usable(s.id) && varies[s.id] && coordinate(s))
        candidates.push_back(terms.state(s.id, s.sort));
    unsigned coordinates = candidates.size();
    for (const StateVar &s : program.state)
      if (usable(s.id) && varies[s.id] && !coordinate(s))
        candidates.push_back(terms.state(s.id, s.sort));
    // One value at every event of every point: a constant for the guards. A value fixed
    // within each point and different between them is neither level nor constant.
    for (const StateVar &s : program.state) {
      if (!usable(s.id) || varies[s.id] || !first[s.id])
        continue;
      bool same = true;
      for (const PointEvents &pe : points)
        for (const Event &event : pe.events)
          if (mine.count(event.site) && event.state[s.id] && *event.state[s.id] != *first[s.id])
            same = false;
      if (same)
        constants.emplace(s.id, *first[s.id]);
    }
    if (count == 0) {
      // A segment no sample reaches: a level with an empty box, which the checker
      // will judge against the scope.
      Segment segment;
      segment.sites = sites;
      GuardedLevel level;
      Term zero = terms.bv(APInt(program.indexBitWidth ? program.indexBitWidth : 16, 0));
      level.value = zero;
      level.lower = zero;
      level.upper = zero;
      segment.order.levels.push_back(level);
      segment.order.guard = terms.boolean(false);
      segment.order.origin = "no sample reaches this segment";
      segment.cellKey = {0};
      out.segments.push_back(std::move(segment));
      continue;
    }
    if (coordinates == 0) {
      // No loop variable moves: the one event per point is its own point.
      coordinates = candidates.size();
      if (coordinates == 0) {
        why = "a segment whose events no state tells apart";
        return std::nullopt;
      }
    }
    std::vector<Sample> samples;
    for (const PointEvents &pe : points) {
      Sample s;
      s.parameters = pe.parameters;
      for (const Event &event : pe.events) {
        if (!mine.count(event.site))
          continue;
        std::vector<APInt> row;
        for (Term term : candidates)
          row.push_back(*event.state[term->index]);
        s.values.push_back(std::move(row));
        s.cells.push_back(event.cell);
        s.access.push_back(event.access);
      }
      samples.push_back(std::move(s));
    }
    std::string failure;
    auto fitted = fitOrder(program, candidates, coordinates, nullptr, samples, constants,
                           &program.sites[sites.front()], limits, failure);
    if (!fitted) {
      why = "segment of site " + std::to_string(sites.front()) + ": " + failure;
      return std::nullopt;
    }
    Segment segment;
    segment.sites = sites;
    segment.order = std::move(fitted->order);
    segment.cellKey = std::move(fitted->cellKey);
    segment.access = std::move(fitted->access);
    // The coordinates are this segment's logical point, on one axis: which axis each
    // belongs to is G3's question, which no segment answers here.
    for (unsigned c : fitted->use)
      if (c < coordinates)
        segment.family.axes[AxisI].push_back(candidates[c]);
    segment.family.origin = "the loop variables of the segment";
    out.segments.push_back(std::move(segment));
  }
  return out;
}

} // namespace mlir::generalized_gemm
