#include "GeneralizedGemm/CoverageSimulation.h"

#include "GeneralizedGemm/CutPaths.h"

#include <algorithm>
#include <functional>
#include <set>

using namespace llvm;

namespace mlir::generalized_gemm {

namespace {

class Order {
public:
  Order(const SemanticProgram &program, const EnumerationOrder &order)
      : terms(*program.terms), levels(order.levels) {}

  Term upper(unsigned m, ArrayRef<Term> values) const {
    Term base = levels[m].upper;
    if (levels[m].upperOuter >= 0)
      base = terms.binary(TermOp::Add, base, values[levels[m].upperOuter]);
    return base;
  }

  Term member(ArrayRef<Term> values) const {
    Term all = terms.boolean(true);
    for (unsigned m = 0; m < levels.size(); ++m) {
      all = terms.logicalAnd(all, terms.icmp(Predicate::Ule, levels[m].lower, values[m]));
      all = terms.logicalAnd(all, terms.icmp(Predicate::Ult, values[m], upper(m, values)));
    }
    return all;
  }

  std::vector<Term> first() const {
    std::vector<Term> out;
    for (const OrderLevel &level : levels)
      out.push_back(level.lower);
    return out;
  }

  Term increment(Term value) const {
    return terms.binary(TermOp::Add, value, terms.bv(APInt(value->sort.width, 1)));
  }

  Term canBump(unsigned m, ArrayRef<Term> values) const {
    return terms.icmp(Predicate::Ult, increment(values[m]), upper(m, values));
  }

  Term hasSuccessor(ArrayRef<Term> values) const {
    Term any = terms.boolean(false);
    for (unsigned m = 0; m < levels.size(); ++m)
      any = terms.logicalOr(any, canBump(m, values));
    return any;
  }

  /// Bump the fastest level that can move, and reset every faster one.
  std::vector<Term> successor(ArrayRef<Term> values) const {
    std::vector<Term> out(values.begin(), values.end());
    for (unsigned m = 0; m < levels.size(); ++m) {
      std::vector<Term> bumped(values.begin(), values.begin() + m);
      bumped.push_back(increment(values[m]));
      for (unsigned r = m + 1; r < levels.size(); ++r)
        bumped.push_back(levels[r].lower);
      Term here = canBump(m, values);
      for (unsigned r = 0; r < levels.size(); ++r)
        out[r] = terms.ite(here, bumped[r], out[r]);
    }
    return out;
  }

  Term equal(ArrayRef<Term> a, ArrayRef<Term> b) const {
    Term all = terms.boolean(true);
    for (unsigned m = 0; m < a.size(); ++m)
      all = terms.logicalAnd(all, terms.icmp(Predicate::Eq, a[m], b[m]));
    return all;
  }

private:
  TermContext &terms;
  const std::vector<OrderLevel> &levels;
};

class Walker {
public:
  Walker(const SemanticProgram &program, ArrayRef<unsigned> locations, unsigned depth,
         const AskUnsat &ask, std::string &why)
      : program(program), terms(*program.terms), ask(ask), why(why), depth(depth) {
    for (unsigned location : locations)
      isSite.insert(location);
    outgoing.resize(program.locationCount);
    for (unsigned i = 0; i < program.transitions.size(); ++i)
      outgoing[program.transitions[i].from].push_back(i);
  }

  using Leaf = std::function<bool(const SymbolicPath &, Term premise, bool exit)>;

  /// Every feasible way from `path` to the next event or the exit.
  bool walk(const SymbolicPath &path, Term assumption, const Leaf &leaf) {
    unsigned steps = path.transitions.size();
    return step(path, assumption, leaf, steps + depth);
  }

private:
  bool failed(const std::string &text) {
    if (why.empty())
      why = text;
    return false;
  }

  bool step(const SymbolicPath &path, Term assumption, const Leaf &leaf, unsigned limit) {
    const std::vector<unsigned> &leaving = outgoing[path.end];
    if (leaving.empty())
      return failed("a location has no transition");
    for (unsigned index : leaving) {
      SymbolicPath next = extendSymbolicPath(program, path, index);
      Term premise = pathPremise(program, next);
      if (!premise)
        return failed("a guard reads memory");
      Term here = terms.logicalAnd(assumption, premise);
      SmtAnswer feasible = ask("prune", "next event walk", here);
      if (feasible == SmtAnswer::Unsat)
        continue;
      if (feasible != SmtAnswer::Sat)
        return failed("a path could not be decided");
      if (next.end == program.exitLocation()) {
        if (!leaf(next, here, true))
          return false;
      } else if (isSite.count(next.end)) {
        // The site's block must complete for its event to exist.
        if (!leaf(next, here, false))
          return false;
      } else if (next.transitions.size() >= limit) {
        return failed("the walk to the next event is longer than the limit");
      } else if (!step(next, assumption, leaf, limit)) {
        return false;
      }
    }
    return true;
  }

  const SemanticProgram &program;
  TermContext &terms;
  const AskUnsat &ask;
  std::string &why;
  unsigned depth;
  std::set<unsigned> isSite;
  std::vector<std::vector<unsigned>> outgoing;
};

} // namespace

bool walkToNextEvents(const SemanticProgram &program, ArrayRef<unsigned> locations,
                      unsigned depth, const AskUnsat &ask, std::string &why,
                      const SymbolicPath &path, Term assumption, const WalkLeaf &leaf) {
  Walker walker(program, locations, depth, ask, why);
  return walker.walk(path, assumption, leaf);
}

Term inOrderSet(const SemanticProgram &program, const EnumerationOrder &order,
                ArrayRef<Term> values) {
  return Order(program, order).member(values);
}

std::vector<Term> orderValues(const SemanticProgram &program, const EnumerationOrder &order,
                              const SymbolicPath &path) {
  std::vector<Term> out;
  for (const OrderLevel &level : order.levels)
    out.push_back(atPathEnd(program, path, level.value));
  return out;
}

bool proveCoverage(const SemanticProgram &program, const EnumerationOrder &levels,
                   const SiteArrivals &arrivals, ArrayRef<unsigned> locations, Term scope,
                   unsigned maxUnfolding, const AskUnsat &ask, std::string &why) {
  TermContext &terms = *program.terms;
  Order order(program, levels);
  Walker walker(program, locations, maxUnfolding, ask, why);
  auto mustHold = [&](const char *id, const std::string &where, Term formula) {
    SmtAnswer answer = ask(id, where, formula);
    if (answer == SmtAnswer::Unsat)
      return true;
    if (why.empty())
      why = std::string(id) + " at " + where + " was not discharged";
    return false;
  };

  // S1, from the entry to the first event.
  std::vector<Term> first = order.first();
  Term firstInside = order.member(first);
  SymbolicPath begin = startSymbolicPath(program, program.entryLocation);
  begin.shadow.assign(program.state.size(), terms.boolean(false));
  auto firstLeaf = [&](const SymbolicPath &path, Term premise, bool exit) {
    if (exit)
      return mustHold("S1", "the entry reaches the exit",
                      terms.logicalAnd(premise, firstInside));
    std::vector<Term> values = orderValues(program, levels, path);
    Term good = terms.logicalAnd(firstInside, order.equal(values, first));
    return mustHold("S1", "the first event", terms.logicalAnd(premise, terms.logicalNot(good)));
  };
  if (std::find(locations.begin(), locations.end(), program.entryLocation) != locations.end()) {
    why = "a site in the entry block";
    return false;
  }
  if (!walker.walk(begin, scope, firstLeaf))
    return false;

  // S2 and S3, from each state at which an event can happen.
  for (const SiteArrival &arrival : arrivals.arrivals) {
    std::vector<Term> at = orderValues(program, levels, arrival.path);
    Term assumption = terms.logicalAnd(arrival.premise, order.member(at));
    Term more = order.hasSuccessor(at);
    std::vector<Term> next = order.successor(at);
    auto leaf = [&](const SymbolicPath &path, Term premise, bool exit) {
      if (exit)
        return mustHold("S3", "an element with a successor reaches the exit",
                        terms.logicalAnd(premise, more));
      std::vector<Term> values = orderValues(program, levels, path);
      Term good = terms.logicalAnd(more, terms.logicalAnd(order.equal(values, next),
                                                          order.member(values)));
      return mustHold("S2", "the next event", terms.logicalAnd(premise, terms.logicalNot(good)));
    };
    if (!walker.walk(arrival.path, assumption, leaf))
      return false;
  }
  return true;
}

} // namespace mlir::generalized_gemm
