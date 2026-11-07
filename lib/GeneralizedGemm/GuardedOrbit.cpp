//===- GuardedOrbit.cpp ---------------------------------------------------===//
#include "GeneralizedGemm/GuardedOrbit.h"

#include "GeneralizedGemm/FiniteDomain.h"
#include "GeneralizedGemm/SymbolicDomain.h"
#include "GeneralizedGemm/TerminationObligations.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"

#include <algorithm>
#include <functional>
#include <set>

using llvm::APInt;
using llvm::ArrayRef;

namespace mlir::generalized_gemm {

Term levelVariable(TermContext &terms, unsigned level, Sort sort) {
  return terms.state(LevelVariableBase + level, sort);
}

std::optional<unsigned> levelOf(Term leaf) {
  if (leaf->op == TermOp::State && leaf->index >= LevelVariableBase)
    return leaf->index - LevelVariableBase;
  return std::nullopt;
}

const char *stringifyFactStatus(FactStatus status) {
  switch (status) {
  case FactStatus::Proven: return "PROVEN";
  case FactStatus::Refuted: return "REFUTED";
  case FactStatus::Unknown: return "UNKNOWN";
  }
  return "UNKNOWN";
}

const char *stringifyDomainMultiplicity(DomainMultiplicity form) {
  switch (form) {
  case DomainMultiplicity::Unit: return "UNIT";
  case DomainMultiplicity::ConstantBox: return "CONSTANT_BOX";
  case DomainMultiplicity::LogicalBox: return "LOGICAL_BOX";
  case DomainMultiplicity::Counted: return "COUNTED";
  case DomainMultiplicity::Unknown: return "UNKNOWN";
  }
  return "UNKNOWN";
}

namespace {

/// A bound or guard: integer operations over parameters and level variables below
/// `levels`, with nothing from memory, poison or state.
bool levelTermOk(const SemanticProgram &program, Term term, unsigned levels,
                 const std::vector<Sort> &sorts, std::string &why) {
  llvm::DenseSet<Term> seen;
  std::function<bool(Term)> visit = [&](Term n) {
    if (!seen.insert(n).second)
      return true;
    if (n->sort.kind != Sort::BV) {
      why = "a bound or guard holds a non-integer";
      return false;
    }
    switch (n->op) {
    case TermOp::Const:
      return true;
    case TermOp::Param:
      if (n->index >= program.parameters.size() || program.parameters[n->index].sort != n->sort) {
        why = "a bound or guard reads a parameter that does not exist";
        return false;
      }
      return true;
    case TermOp::State: {
      auto level = levelOf(n);
      if (!level || *level >= levels || sorts[*level] != n->sort) {
        why = "a bound or guard reads state, or a level it may not";
        return false;
      }
      return true;
    }
    case TermOp::Add: case TermOp::Sub: case TermOp::Mul: case TermOp::SDiv: case TermOp::UDiv:
    case TermOp::SRem: case TermOp::URem: case TermOp::Shl: case TermOp::LShr: case TermOp::AShr:
    case TermOp::And: case TermOp::Or: case TermOp::Xor: case TermOp::Not: case TermOp::ICmp:
    case TermOp::Ite: case TermOp::ZExt: case TermOp::SExt: case TermOp::Trunc:
      return llvm::all_of(n->args, visit);
    default:
      why = "a bound or guard holds an operation outside the integer fragment";
      return false;
    }
  };
  return visit(term);
}

bool readsShadow(Term term) {
  llvm::DenseSet<Term> seen;
  std::function<bool(Term)> visit = [&](Term n) {
    if (!seen.insert(n).second)
      return false;
    return n->op == TermOp::Shadow || llvm::any_of(n->args, visit);
  };
  return visit(term);
}

Term less(TermContext &terms, const GuardedLevel &level, Term a, Term b) {
  Predicate before = level.isSigned ? Predicate::Slt : Predicate::Ult;
  return level.descending ? terms.icmp(before, b, a) : terms.icmp(before, a, b);
}

} // namespace

std::optional<GuardedOrder> adoptGuardedOrder(const SemanticProgram &program,
                                              const CoordinateFamily &family,
                                              const GuardedOrder &order,
                                              std::vector<bool> &repeat, std::string &why) {
  TermContext &terms = *program.terms;
  if (order.levels.empty()) {
    why = "an order with no level";
    return std::nullopt;
  }
  if (2 * program.state.size() >= LevelVariableBase) {
    why = "too many state variables for level variables";
    return std::nullopt;
  }
  std::vector<bool> independent = memoryIndependentState(program);
  auto value = [&](Term term) -> Term {
    Term kept = adoptProofTerm(program, term, why);
    if (!kept)
      return nullptr;
    if (readsShadow(kept)) {
      why = "a level reads a poison shadow";
      return nullptr;
    }
    for (unsigned id : stateSupport(kept))
      if (!independent[id]) {
        why = "a level reads a value that depends on memory";
        return nullptr;
      }
    return kept;
  };
  GuardedOrder out;
  out.origin = order.origin;
  std::vector<Sort> sorts;
  for (const GuardedLevel &level : order.levels) {
    GuardedLevel copy = level;
    copy.value = value(level.value);
    if (!copy.value)
      return std::nullopt;
    sorts.push_back(copy.value->sort);
    if (!level.lower || !level.upper) {
      why = "a level with no bounds";
      return std::nullopt;
    }
    if (!levelTermOk(program, level.lower, sorts.size() - 1, sorts, why) ||
        !levelTermOk(program, level.upper, sorts.size() - 1, sorts, why))
      return std::nullopt;
    if (level.lower->sort != copy.value->sort || level.upper->sort != copy.value->sort) {
      why = "a bound of another width than its level";
      return std::nullopt;
    }
    out.levels.push_back(copy);
  }
  out.guard = order.guard ? order.guard : terms.boolean(true);
  if (!levelTermOk(program, out.guard, sorts.size(), sorts, why))
    return std::nullopt;
  if (out.guard->sort != Sort::bv(1)) {
    why = "a guard that is not a truth value";
    return std::nullopt;
  }

  // Every coordinate of the family is a level, each once.
  std::multiset<Term> levels;
  for (const GuardedLevel &level : out.levels)
    levels.insert(level.value);
  repeat.assign(out.levels.size(), true);
  for (const auto &axis : family.axes)
    for (Term term : axis) {
      Term kept = value(term);
      if (!kept)
        return std::nullopt;
      auto found = levels.find(kept);
      if (found == levels.end()) {
        why = "a coordinate of the family is no level";
        return std::nullopt;
      }
      levels.erase(found);
      for (unsigned m = 0; m < out.levels.size(); ++m)
        if (out.levels[m].value == kept && repeat[m]) {
          repeat[m] = false;
          break;
        }
    }
  return out;
}

bool isLevelTerm(const SemanticProgram &program, const GuardedOrder &order, Term term,
                 unsigned levels, std::string &why) {
  std::vector<Sort> sorts;
  for (const GuardedLevel &level : order.levels)
    sorts.push_back(level.value->sort);
  return term && levelTermOk(program, term, std::min<unsigned>(levels, sorts.size()), sorts, why);
}

Term bindLevels(const SemanticProgram &program, Term term, ArrayRef<Term> values) {
  TermContext &terms = *program.terms;
  return terms.substitute(term, [&](Term leaf) -> Term {
    auto level = levelOf(leaf);
    return level && *level < values.size() ? values[*level] : nullptr;
  });
}

Term inGuardedSet(const SemanticProgram &program, const GuardedOrder &order,
                  ArrayRef<Term> values) {
  TermContext &terms = *program.terms;
  Term all = bindLevels(program, order.guard, values);
  for (unsigned m = 0; m < order.levels.size(); ++m) {
    const GuardedLevel &level = order.levels[m];
    Predicate le = level.isSigned ? Predicate::Sle : Predicate::Ule;
    Predicate lt = level.isSigned ? Predicate::Slt : Predicate::Ult;
    all = terms.logicalAnd(all, terms.icmp(le, bindLevels(program, level.lower, values), values[m]));
    all = terms.logicalAnd(all, terms.icmp(lt, values[m], bindLevels(program, level.upper, values)));
  }
  return all;
}

Term guardedLess(const SemanticProgram &program, const GuardedOrder &order, ArrayRef<Term> a,
                 ArrayRef<Term> b) {
  TermContext &terms = *program.terms;
  Term any = terms.boolean(false), same = terms.boolean(true);
  for (unsigned m = 0; m < order.levels.size(); ++m) {
    any = terms.logicalOr(any, terms.logicalAnd(same, less(terms, order.levels[m], a[m], b[m])));
    same = terms.logicalAnd(same, terms.icmp(Predicate::Eq, a[m], b[m]));
  }
  return any;
}

std::vector<Term> freeLevels(const SemanticProgram &program, const GuardedOrder &order) {
  std::vector<Term> out;
  for (unsigned m = 0; m < order.levels.size(); ++m)
    out.push_back(levelVariable(*program.terms, m, order.levels[m].value->sort));
  return out;
}

bool proveGuardedOrbit(const SemanticProgram &program, const GuardedOrder &order,
                       const SiteArrivals &arrivals, ArrayRef<unsigned> locations, Term scope,
                       unsigned maxUnfolding, const AskUnsat &ask, std::string &why) {
  TermContext &terms = *program.terms;
  auto values = [&](const SymbolicPath &path) {
    std::vector<Term> out;
    for (const GuardedLevel &level : order.levels)
      out.push_back(atPathEnd(program, path, level.value));
    return out;
  };
  auto mustHold = [&](const char *id, const std::string &where, Term formula) {
    SmtAnswer answer = ask(id, where, formula);
    if (answer == SmtAnswer::Unsat)
      return true;
    if (why.empty())
      why = std::string(id) + " at " + where + ": " + stringifySmtAnswer(answer);
    return false;
  };
  std::vector<Term> y = freeLevels(program, order);
  Term yInside = inGuardedSet(program, order, y);

  // The first event happens inside the entry block, where no walk starts.
  if (std::find(locations.begin(), locations.end(), program.entryLocation) != locations.end()) {
    why = "a site in the entry block";
    return false;
  }

  // D1, from the entry.
  SymbolicPath begin = startSymbolicPath(program, program.entryLocation);
  begin.shadow.assign(program.state.size(), terms.boolean(false));
  auto firstLeaf = [&](const SymbolicPath &path, Term premise, bool exit) {
    if (exit)
      return mustHold("D1", "the entry reaches the exit", terms.logicalAnd(premise, yInside));
    std::vector<Term> x = values(path);
    Term earlier = terms.logicalAnd(yInside, guardedLess(program, order, y, x));
    Term bad = terms.logicalOr(terms.logicalNot(inGuardedSet(program, order, x)), earlier);
    return mustHold("D1", "the first event", terms.logicalAnd(premise, bad));
  };
  if (!walkToNextEvents(program, locations, maxUnfolding, ask, why, begin, scope, firstLeaf))
    return false;

  // D2 and D3, from each state at which an event can happen. The event there is in S
  // by induction, so that is assumed.
  for (const SiteArrival &arrival : arrivals.arrivals) {
    std::vector<Term> b = values(arrival.path);
    Term assumption = terms.logicalAnd(arrival.premise, inGuardedSet(program, order, b));
    Term later = terms.logicalAnd(yInside, guardedLess(program, order, b, y));
    auto leaf = [&](const SymbolicPath &path, Term premise, bool exit) {
      if (exit)
        return mustHold("D3", "an event that is not the last reaches the exit",
                        terms.logicalAnd(premise, later));
      std::vector<Term> x = values(path);
      Term between = terms.logicalAnd(later, guardedLess(program, order, y, x));
      Term bad = terms.logicalOr(terms.logicalNot(inGuardedSet(program, order, x)),
                                 terms.logicalOr(terms.logicalNot(guardedLess(program, order, b, x)),
                                                 between));
      return mustHold("D2", "the next event", terms.logicalAnd(premise, bad));
    };
    if (!walkToNextEvents(program, locations, maxUnfolding, ask, why, arrival.path, assumption, leaf))
      return false;
  }
  return true;
}

bool proveSegmentedOrbit(const SemanticProgram &program, ArrayRef<GuardedOrder> orders,
                         const std::map<unsigned, unsigned> &segmentOfLocation,
                         const SiteArrivals &arrivals, ArrayRef<unsigned> locations, Term scope,
                         unsigned maxUnfolding, const AskUnsat &ask, std::string &why) {
  TermContext &terms = *program.terms;
  unsigned m = orders.size();
  auto values = [&](unsigned s, const SymbolicPath &path) {
    std::vector<Term> out;
    for (const GuardedLevel &level : orders[s].levels)
      out.push_back(atPathEnd(program, path, level.value));
    return out;
  };
  auto mustHold = [&](const char *id, const std::string &where, Term formula) {
    SmtAnswer answer = ask(id, where, formula);
    if (answer == SmtAnswer::Unsat)
      return true;
    if (why.empty())
      why = std::string(id) + " at " + where + ": " + stringifySmtAnswer(answer);
    return false;
  };
  auto yInside = [&](unsigned s) { return inGuardedSet(program, orders[s], freeLevels(program, orders[s])); };
  // S_r is empty under `premise`: each segment its own query, so one y at a time.
  auto empty = [&](const char *id, const std::string &where, Term premise, unsigned from, unsigned to) {
    for (unsigned r = from; r < to; ++r)
      if (!mustHold(id, where + ", segment " + std::to_string(r) + " empty",
                    terms.logicalAnd(premise, yInside(r))))
        return false;
    return true;
  };
  auto segmentAt = [&](unsigned location) -> std::optional<unsigned> {
    auto found = segmentOfLocation.find(location);
    if (found == segmentOfLocation.end())
      return std::nullopt;
    return found->second;
  };
  // The first event of segment s: λ ∈ S_s and nothing of S_s before it.
  auto isFirst = [&](unsigned s, const std::vector<Term> &x) {
    std::vector<Term> y = freeLevels(program, orders[s]);
    Term earlier = terms.logicalAnd(yInside(s), guardedLess(program, orders[s], y, x));
    return terms.logicalOr(terms.logicalNot(inGuardedSet(program, orders[s], x)), earlier);
  };

  if (std::find(locations.begin(), locations.end(), program.entryLocation) != locations.end()) {
    why = "a site in the entry block";
    return false;
  }

  // D1', from the entry.
  SymbolicPath begin = startSymbolicPath(program, program.entryLocation);
  begin.shadow.assign(program.state.size(), terms.boolean(false));
  auto firstLeaf = [&](const SymbolicPath &path, Term premise, bool exit) {
    if (exit)
      return empty("D1", "the entry reaches the exit", premise, 0, m);
    auto s = segmentAt(path.end);
    if (!s) {
      why = "an event at a location of no segment";
      return false;
    }
    if (!mustHold("D1", "the first event, segment " + std::to_string(*s),
                  terms.logicalAnd(premise, isFirst(*s, values(*s, path)))))
      return false;
    return empty("D1", "before the first event", premise, 0, *s);
  };
  if (!walkToNextEvents(program, locations, maxUnfolding, ask, why, begin, scope, firstLeaf))
    return false;

  // D2 to D5, from each state at which an event can happen.
  for (const SiteArrival &arrival : arrivals.arrivals) {
    auto s = segmentAt(arrival.location);
    if (!s) {
      why = "an event at a location of no segment";
      return false;
    }
    const GuardedOrder &order = orders[*s];
    std::vector<Term> b = values(*s, arrival.path);
    std::vector<Term> y = freeLevels(program, order);
    Term assumption = terms.logicalAnd(arrival.premise, inGuardedSet(program, order, b));
    Term later = terms.logicalAnd(inGuardedSet(program, order, y), guardedLess(program, order, b, y));
    std::string here = "segment " + std::to_string(*s);
    auto leaf = [&](const SymbolicPath &path, Term premise, bool exit) {
      if (exit)
        return mustHold("D3", "an event that is not the last of " + here + " reaches the exit",
                        terms.logicalAnd(premise, later)) &&
               empty("D5", "after the last event of " + here, premise, *s + 1, m);
      auto next = segmentAt(path.end);
      if (!next) {
        why = "an event at a location of no segment";
        return false;
      }
      if (*next < *s)
        return mustHold("D5", "an event of segment " + std::to_string(*next) + " after " + here, premise);
      if (*next == *s) {
        std::vector<Term> x = values(*s, path);
        Term between = terms.logicalAnd(later, guardedLess(program, order, y, x));
        Term bad = terms.logicalOr(terms.logicalNot(inGuardedSet(program, order, x)),
                                   terms.logicalOr(terms.logicalNot(guardedLess(program, order, b, x)), between));
        return mustHold("D2", "the next event, " + here, terms.logicalAnd(premise, bad));
      }
      // A hand-over to a later segment: nothing of S_s remains, the segments between
      // are empty, and the event is the first of its segment.
      std::string handover = here + " to segment " + std::to_string(*next);
      return mustHold("D4", "a successor remains at the hand-over " + handover,
                      terms.logicalAnd(premise, later)) &&
             empty("D4", "between " + handover, premise, *s + 1, *next) &&
             mustHold("D4", "the first event after the hand-over " + handover,
                      terms.logicalAnd(premise, isFirst(*next, values(*next, path))));
    };
    if (!walkToNextEvents(program, locations, maxUnfolding, ask, why, arrival.path, assumption, leaf))
      return false;
  }
  return true;
}

std::optional<APInt> evaluateLevelTerm(Term t, ArrayRef<APInt> parameters, ArrayRef<APInt> levels) {
  auto arg = [&](unsigned i) { return evaluateLevelTerm(t->args[i], parameters, levels); };
  switch (t->op) {
  case TermOp::Const:
    return t->value;
  case TermOp::Param:
    if (t->index >= parameters.size() || parameters[t->index].getBitWidth() != t->sort.width)
      return std::nullopt;
    return parameters[t->index];
  case TermOp::State: {
    auto level = levelOf(t);
    if (!level || *level >= levels.size() || levels[*level].getBitWidth() != t->sort.width)
      return std::nullopt;
    return levels[*level];
  }
  case TermOp::Add: case TermOp::Sub: case TermOp::Mul: case TermOp::SDiv: case TermOp::UDiv:
  case TermOp::SRem: case TermOp::URem: case TermOp::Shl: case TermOp::LShr: case TermOp::AShr:
  case TermOp::And: case TermOp::Or: case TermOp::Xor: {
    auto a = arg(0), b = arg(1);
    if (!a || !b)
      return std::nullopt;
    return evalIntegerBinary(t->op, *a, *b);
  }
  case TermOp::Not: {
    auto a = arg(0);
    return a ? std::optional<APInt>(~*a) : std::nullopt;
  }
  case TermOp::ICmp: {
    auto a = arg(0), b = arg(1);
    if (!a || !b)
      return std::nullopt;
    return APInt(1, evalPredicate((Predicate)t->aux, *a, *b) ? 1 : 0);
  }
  case TermOp::Ite: {
    auto c = arg(0);
    if (!c)
      return std::nullopt;
    return arg(c->isOne() ? 1 : 2);
  }
  case TermOp::ZExt: case TermOp::SExt: case TermOp::Trunc: {
    auto a = arg(0);
    if (!a)
      return std::nullopt;
    if (t->op == TermOp::ZExt)
      return a->zext(t->sort.width);
    if (t->op == TermOp::SExt)
      return a->sext(t->sort.width);
    return a->trunc(t->sort.width);
  }
  default:
    return std::nullopt;
  }
}

bool concreteLess(const GuardedOrder &order, ArrayRef<APInt> a, ArrayRef<APInt> b) {
  for (unsigned m = 0; m < order.levels.size(); ++m) {
    if (a[m] == b[m])
      continue;
    const GuardedLevel &level = order.levels[m];
    bool before = level.isSigned ? a[m].slt(b[m]) : a[m].ult(b[m]);
    return level.descending ? !before : before;
  }
  return false;
}

std::optional<std::vector<std::vector<APInt>>>
enumerateGuardedSet(const GuardedOrder &order, ArrayRef<APInt> parameters, uint64_t limit,
                    std::string &why) {
  std::vector<std::vector<APInt>> out;
  std::vector<APInt> current;
  uint64_t visited = 0;
  bool failed = false;
  std::function<void(unsigned)> go = [&](unsigned m) {
    if (failed)
      return;
    if (m == order.levels.size()) {
      auto keep = evaluateLevelTerm(order.guard, parameters, current);
      if (!keep) {
        failed = true;
        why = "the guard has no value at a point";
        return;
      }
      if (keep->isOne())
        out.push_back(current);
      return;
    }
    const GuardedLevel &level = order.levels[m];
    auto low = evaluateLevelTerm(level.lower, parameters, current);
    auto high = evaluateLevelTerm(level.upper, parameters, current);
    if (!low || !high) {
      failed = true;
      why = "a bound has no value at a point";
      return;
    }
    bool empty = level.isSigned ? !low->slt(*high) : !low->ult(*high);
    if (empty)
      return;
    APInt span = *high - *low; // At most 2^w - 1, as the range is not empty.
    if (span.getActiveBits() > 40 || visited + span.getZExtValue() > limit) {
      failed = true;
      why = "the box holds more points than the limit";
      return;
    }
    visited += span.getZExtValue();
    uint64_t count = span.getZExtValue();
    for (uint64_t n = 0; n < count && !failed; ++n) {
      APInt offset(low->getBitWidth(), level.descending ? count - 1 - n : n);
      current.push_back(*low + offset);
      go(m + 1);
      current.pop_back();
    }
  };
  go(0);
  if (failed)
    return std::nullopt;
  return out;
}

} // namespace mlir::generalized_gemm
