#include "GeneralizedGemm/SymbolicEvaluator.h"

#include <algorithm>
#include <functional>

using namespace mlir::generalized_gemm;

namespace mlir::generalized_gemm {

SymbolicPath startSymbolicPath(const SemanticProgram &program, unsigned from) {
  SymbolicPath start;
  start.end = from;
  start.condition = program.terms->boolean(true);
  start.state.assign(program.state.size(), nullptr);
  start.shadow.assign(program.state.size(), nullptr);
  start.memory.assign(program.roots.size(), nullptr);
  return start;
}

SymbolicPath extendSymbolicPath(const SemanticProgram &program,
                                const SymbolicPath &path, unsigned transition) {
  TermContext &terms = *program.terms;
  const Transition &t = program.transitions[transition];
  // All of t reads the configuration at t.from: the path so far.
  auto leaf = [&](Term n) -> Term {
    if (n->op == TermOp::State)
      return path.state[n->index];
    if (n->op == TermOp::Shadow)
      return path.shadow[n->index];
    if (n->op == TermOp::MemVar)
      return path.memory[n->index];
    return nullptr;
  };
  auto apply = [&](Term term) { return terms.substitute(term, leaf); };

  SymbolicPath next = path;
  next.transitions.push_back(transition);
  next.condition = terms.logicalAnd(path.condition, apply(t.guard));
  for (SideCondition side : t.sideConditions) {
    side.violated = apply(side.violated);
    next.sideConditions.push_back(side);
  }
  for (MemoryEffect effect : t.effects) {
    for (Term &i : effect.index)
      i = apply(i);
    effect.value = apply(effect.value);
    next.effects.push_back(effect);
  }
  for (const auto &[id, term] : t.assignments)
    next.state[id] = apply(term);
  for (const auto &[id, term] : t.shadowAssignments)
    next.shadow[id] = apply(term);
  for (const auto &[root, term] : t.memoryUpdates)
    next.memory[root] = apply(term);
  next.end = t.to;
  if (t.to == program.exitLocation()) {
    next.stop = SymbolicPath::ReachedExit;
    for (Term term : t.returns)
      next.returns.push_back(apply(term));
    for (Term term : t.returnPoison)
      next.returnPoison.push_back(apply(term));
  }
  return next;
}

SymbolicRegion summarizeAcyclic(const SemanticProgram &program, unsigned from,
                                const SymbolicLimits &limits) {
  SymbolicRegion region;
  if (!program.isSupported()) {
    region.reason = "the program is unsupported";
    return region;
  }
  if (from >= program.locationCount) {
    region.reason = "no such location";
    return region;
  }
  TermContext &terms = *program.terms;
  size_t startTerms = terms.size();

  struct LimitReached {
    std::string reason;
  };

  std::function<void(SymbolicPath, llvm::SmallVector<unsigned>)> walk =
      [&](SymbolicPath path, llvm::SmallVector<unsigned> visited) {
        if (path.transitions.size() >= limits.maxPathLength)
          throw LimitReached{"path length limit"};
        for (unsigned index = 0; index < program.transitions.size(); ++index) {
          const Transition &t = program.transitions[index];
          if (t.from != path.end)
            continue;
          if (terms.size() - startTerms > limits.maxNewTerms)
            throw LimitReached{"term limit"};
          SymbolicPath next = extendSymbolicPath(program, path, index);

          if (t.to == program.exitLocation()) {
            region.paths.push_back(std::move(next));
          } else if (llvm::is_contained(visited, t.to)) {
            next.stop = SymbolicPath::ReachedRevisit;
            region.paths.push_back(std::move(next));
          } else {
            auto extended = visited;
            extended.push_back(t.to);
            walk(std::move(next), std::move(extended));
          }
          if (region.paths.size() > limits.maxPaths)
            throw LimitReached{"path limit"};
        }
      };

  try {
    walk(startSymbolicPath(program, from), {from});
  } catch (const LimitReached &limit) {
    region.paths.clear();
    region.reason = limit.reason;
    return region;
  } catch (const TermError &error) {
    region.paths.clear();
    region.reason = error.what();
    return region;
  }
  region.complete = true;
  return region;
}

} // namespace mlir::generalized_gemm
