#include "GeneralizedGemm/CutPaths.h"

#include "GeneralizedGemm/SymbolicEvaluator.h"

#include "llvm/ADT/DenseSet.h"

#include <functional>

using namespace mlir::generalized_gemm;

namespace mlir::generalized_gemm {

bool containsFloat(Term term) {
  llvm::DenseSet<Term> seen;
  std::function<bool(Term)> visit = [&](Term n) {
    if (!seen.insert(n).second)
      return false;
    if (n->sort.kind == Sort::Float)
      return true;
    for (Term a : n->args)
      if (visit(a))
        return true;
    return false;
  };
  return visit(term);
}

Term postTerm(const SemanticProgram &program, const CutPath &path, Term term) {
  auto leaf = [&](Term n) -> Term {
    if (n->op == TermOp::State)
      return path.state[n->index];
    if (n->op == TermOp::Shadow)
      return path.shadow[n->index];
    if (n->op == TermOp::MemVar)
      return path.memory[n->index];
    return nullptr;
  };
  return program.terms->substitute(term, leaf);
}

std::string describePath(const SemanticProgram &program, const CutPath &path) {
  std::string out = "L" + std::to_string(path.from);
  for (unsigned index : path.transitions) {
    const Transition &t = program.transitions[index];
    out += " -" + std::to_string(t.slot) + "-> ";
    out += t.to == program.exitLocation() ? std::string("exit")
                                          : "L" + std::to_string(t.to);
  }
  return out;
}

PathSet enumerateCutPaths(const SemanticProgram &program,
                          llvm::ArrayRef<unsigned> cuts,
                          const PathLimits &limits) {
  PathSet set;
  TermContext &terms = *program.terms;
  size_t startTerms = terms.size();
  struct LimitReached {
    std::string reason;
  };
  std::vector<bool> isCut(program.locationCount, false);
  for (unsigned cut : cuts)
    isCut[cut] = true;
  std::vector<std::vector<unsigned>> outgoing(program.locationCount);
  for (unsigned i = 0; i < program.transitions.size(); ++i)
    outgoing[program.transitions[i].from].push_back(i);

  // Guards so far, and no retained stop at any block already left.
  auto liveCondition = [&](const SymbolicPath &path) {
    Term condition = path.condition;
    for (const SideCondition &side : path.sideConditions)
      if (!containsFloat(side.violated))
        condition =
            terms.logicalAnd(condition, terms.logicalNot(side.violated));
    return condition;
  };

  auto walkFrom = [&](unsigned source, bool fromEntry) {
    std::function<void(const SymbolicPath &)> walk = [&](const SymbolicPath &path) {
      if (path.transitions.size() >= limits.maxPathLength)
        throw LimitReached{"path length limit"};
      if (terms.size() - startTerms > limits.maxNewTerms)
        throw LimitReached{"term limit"};
      const std::vector<unsigned> &leaving = outgoing[path.end];
      if (leaving.empty())
        throw LimitReached{"location " + std::to_string(path.end) +
                           " has no transition"};
      // Each block's conditions once per way of reaching it. Every transition
      // of a block carries the same list.
      SymbolicPath probe = extendSymbolicPath(program, path, leaving.front());
      Term reached = liveCondition(path);
      for (unsigned i = path.sideConditions.size();
           i < probe.sideConditions.size(); ++i) {
        const SideCondition &side = probe.sideConditions[i];
        StopSite site;
        site.from = source;
        site.fromEntry = fromEntry;
        site.prefix = path.transitions;
        site.location = path.end;
        site.kind = side.kind;
        site.reason = side.reason;
        site.reached = reached;
        site.violated = side.violated;
        set.sites.push_back(std::move(site));
        if (set.sites.size() > limits.maxStopSites)
          throw LimitReached{"stop site limit"};
      }
      for (unsigned index : leaving) {
        SymbolicPath next = extendSymbolicPath(program, path, index);
        unsigned to = next.end;
        if (to == program.exitLocation() || isCut[to]) {
          CutPath done;
          done.id = set.paths.size();
          done.from = source;
          done.to = to;
          done.fromEntry = fromEntry;
          done.transitions = next.transitions;
          done.condition = liveCondition(next);
          done.state = next.state;
          done.shadow = next.shadow;
          done.memory = next.memory;
          set.paths.push_back(std::move(done));
          if (set.paths.size() > limits.maxPaths)
            throw LimitReached{"path limit"};
        } else {
          walk(next);
        }
      }
    };
    SymbolicPath start = startSymbolicPath(program, source);
    if (fromEntry)
      start.shadow.assign(program.state.size(), terms.boolean(false));
    walk(start);
  };

  try {
    walkFrom(program.entryLocation, /*fromEntry=*/true);
    for (unsigned cut : cuts)
      walkFrom(cut, /*fromEntry=*/false);
  } catch (const LimitReached &limit) {
    set.paths.clear();
    set.sites.clear();
    set.reason = limit.reason;
    return set;
  } catch (const TermError &error) {
    set.paths.clear();
    set.sites.clear();
    set.reason = error.what();
    return set;
  }
  set.complete = true;
  return set;
}

} // namespace mlir::generalized_gemm
