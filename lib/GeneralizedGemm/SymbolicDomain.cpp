#include "GeneralizedGemm/SymbolicDomain.h"

#include "GeneralizedGemm/TerminationObligations.h"

#include <functional>

using namespace llvm;

namespace mlir::generalized_gemm {

Term pathPremise(const SemanticProgram &program, const SymbolicPath &path) {
  TermContext &terms = *program.terms;
  if (readsMemory(path.condition) || containsFloat(path.condition))
    return nullptr;
  Term premise = path.condition;
  for (const SideCondition &side : path.sideConditions)
    if (!containsFloat(side.violated) && !readsMemory(side.violated))
      premise = terms.logicalAnd(premise, terms.logicalNot(side.violated));
  return premise;
}

Term atPathEnd(const SemanticProgram &program, const SymbolicPath &path, Term term) {
  CutPath view;
  view.state = path.state;
  view.shadow = path.shadow;
  view.memory = path.memory;
  return postTerm(program, view, term);
}

Term secondCopy(const SemanticProgram &program, Term term) {
  unsigned offset = program.state.size();
  TermContext &terms = *program.terms;
  return terms.substitute(term, [&](Term n) -> Term {
    if (n->op == TermOp::State)
      return terms.state(n->index + offset, n->sort);
    if (n->op == TermOp::Shadow)
      return terms.shadow(n->index + offset);
    return nullptr;
  });
}

SiteArrivals reachSites(const SemanticProgram &program,
                        const std::map<unsigned, Term> &invariants,
                        ArrayRef<unsigned> cuts, ArrayRef<unsigned> sites, Term scope,
                        const PathLimits &limits) {
  SiteArrivals out;
  TermContext &terms = *program.terms;
  struct Limit {
    std::string reason;
  };
  std::vector<bool> isCut(program.locationCount, false);
  for (unsigned cut : cuts)
    isCut[cut] = true;
  std::vector<std::vector<unsigned>> outgoing(program.locationCount);
  for (unsigned i = 0; i < program.transitions.size(); ++i)
    outgoing[program.transitions[i].from].push_back(i);

  auto walkFrom = [&](unsigned source, Term start) {
    std::function<void(const SymbolicPath &)> walk = [&](const SymbolicPath &path) {
      if (path.transitions.size() >= limits.maxPathLength)
        throw Limit{"path length limit"};
      const std::vector<unsigned> &leaving = outgoing[path.end];
      if (leaving.empty())
        throw Limit{"a location has no transition"};
      // Conditions of the block are the same on all of its transitions.
      SymbolicPath probe = extendSymbolicPath(program, path, leaving.front());
      for (unsigned id : sites) {
        const MacSite &site = program.sites[id];
        if (site.location != path.end)
          continue;
        SiteArrival arrival;
        arrival.site = id;
        arrival.location = path.end;
        arrival.path = path;
        // The guard of `probe` is not the arrival's: only its side conditions.
        SymbolicPath block = path;
        block.sideConditions = probe.sideConditions;
        Term premise = pathPremise(program, block);
        if (!premise)
          throw Limit{"a guard reads memory"};
        arrival.premise = terms.logicalAnd(terms.logicalAnd(scope, start), premise);
        out.arrivals.push_back(std::move(arrival));
        if (out.arrivals.size() > limits.maxPaths)
          throw Limit{"arrival limit"};
      }
      for (unsigned index : leaving) {
        SymbolicPath next = extendSymbolicPath(program, path, index);
        if (next.end != program.exitLocation() && !isCut[next.end])
          walk(next);
      }
    };
    SymbolicPath begin = startSymbolicPath(program, source);
    if (source == program.entryLocation)
      begin.shadow.assign(program.state.size(), terms.boolean(false));
    walk(begin);
  };

  try {
    walkFrom(program.entryLocation, terms.boolean(true));
    for (unsigned cut : cuts) {
      auto found = invariants.find(cut);
      walkFrom(cut, found == invariants.end() ? terms.boolean(true) : found->second);
    }
  } catch (const Limit &limit) {
    out.arrivals.clear();
    out.reason = limit.reason;
    return out;
  } catch (const TermError &error) {
    out.arrivals.clear();
    out.reason = error.what();
    return out;
  }
  out.complete = true;
  return out;
}

std::map<unsigned, std::set<unsigned>> controlLiveState(const SemanticProgram &program) {
  std::map<unsigned, std::set<unsigned>> live;
  auto add = [](std::set<unsigned> &to, Term term) {
    size_t before = to.size();
    for (unsigned id : stateSupport(term))
      to.insert(id);
    return to.size() != before;
  };
  for (bool changed = true; changed;) {
    changed = false;
    for (const Transition &t : program.transitions) {
      std::set<unsigned> &here = live[t.from];
      changed = add(here, t.guard) || changed;
      if (t.to >= program.locationCount)
        continue;
      std::set<unsigned> next = live[t.to];
      for (const auto &[id, term] : t.assignments)
        if (next.erase(id))
          changed = add(here, term) || changed;
      size_t before = here.size();
      here.insert(next.begin(), next.end());
      changed = changed || here.size() != before;
    }
  }
  return live;
}

} // namespace mlir::generalized_gemm
