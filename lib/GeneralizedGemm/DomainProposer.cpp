//===- DomainProposer.cpp -------------------------------------------------===//
#include "GeneralizedGemm/DomainProposer.h"

#include "GeneralizedGemm/GuardedOrbit.h"

#include "llvm/ADT/DenseSet.h"

#include <algorithm>
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

/// A bound fitted to one value per point: a constant, or a parameter plus a constant.
Term fit(const SemanticProgram &program, const std::vector<std::pair<std::vector<APInt>, APInt>> &want,
         unsigned width) {
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
  return nullptr;
}

/// Truth values of the program's conditions, as terms over level variables, from the
/// guards of every transition and their boolean parts. A state variable that holds one
/// value at every sample event is read as that constant: a guess, which the checker
/// proves or refutes like the rest. Congruences of each level by small moduli are
/// added, for steps.
std::vector<Term> conditionAtoms(const SemanticProgram &program, const GuardedOrder &order,
                                 const std::map<unsigned, APInt> &constants, unsigned limit) {
  TermContext &terms = *program.terms;
  std::map<unsigned, unsigned> levelOfState;
  for (unsigned m = 0; m < order.levels.size(); ++m)
    if (order.levels[m].value->op == TermOp::State)
      levelOfState[order.levels[m].value->index] = m;
  std::vector<Term> out;
  std::set<Term> seen;
  std::function<void(Term)> collect = [&](Term term) {
    if (out.size() >= limit || term->sort != Sort::bv(1) || !seen.insert(term).second)
      return;
    if (term->op == TermOp::And || term->op == TermOp::Or || term->op == TermOp::Not ||
        term->op == TermOp::Xor)
      for (Term arg : term->args)
        collect(arg);
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
    for (uint64_t modulus = 2; modulus <= 4; ++modulus)
      for (uint64_t rest = 0; rest < modulus; ++rest)
        out.push_back(terms.icmp(Predicate::Eq,
                                 terms.binary(TermOp::URem, y, terms.bv(APInt(sort.width, modulus))),
                                 terms.bv(APInt(sort.width, rest))));
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
        std::vector<uint64_t> cell;
        for (const APInt &value : domain.table().view(domain.table().events()[e].h))
          cell.push_back(value.getZExtValue());
        s.cells.push_back(std::move(cell));
      }
    }
    samples.push_back(std::move(s));
  }

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
      Term lower = fit(program, lows, width), upper = fit(program, highs, width);
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
          std::optional<APInt> offset;
          bool all = true;
          for (const auto &[key, value] : extreme) {
            APInt bound = wantUpper ? value + 1 : value;
            APInt c = bound - APInt(width, key.second);
            if (!offset)
              offset = c;
            all = all && *offset == c;
          }
          if (all && offset) {
            Term y = levelVariable(terms, o, order.levels[o].value->sort);
            return offset->isZero() ? y : terms.binary(TermOp::Add, y, terms.bv(*offset));
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
    std::vector<Term> atoms = conditionAtoms(program, order, constants, limits.maxAtoms);
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

  DomainRequest out;
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
  // Access forms: the index terms, when they read only levels held in plain state.
  std::map<unsigned, unsigned> levelOfState;
  for (unsigned m = 0; m < out.order.levels.size(); ++m)
    if (out.order.levels[m].value->op == TermOp::State)
      levelOfState[out.order.levels[m].value->index] = m;
  const SiteGroup &group = enumeration.groups[g];
  for (unsigned view = 0; view < 3; ++view) {
    const MacSite &site = program.sites[group.sites.front()];
    llvm::ArrayRef<Term> index = view == 0 ? llvm::ArrayRef<Term>(site.output->index)
                               : view == 1 ? llvm::ArrayRef<Term>(site.a.index)
                                           : llvm::ArrayRef<Term>(site.b.index);
    std::vector<Term> form;
    bool ok = true;
    for (Term term : index) {
      for (unsigned id : stateSupport(term))
        ok = ok && levelOfState.count(id);
      if (!ok || containsOp(term, TermOp::Load))
        break;
      form.push_back(terms.substitute(term, [&](Term leaf) -> Term {
        if (leaf->op != TermOp::State)
          return nullptr;
        unsigned m = levelOfState[leaf->index];
        return levelVariable(terms, m, out.order.levels[m].value->sort);
      }));
    }
    if (ok)
      out.access[view] = std::move(form);
  }
  return out;
}

} // namespace mlir::generalized_gemm
