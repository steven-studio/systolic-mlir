//===- LoopCounters.cpp ---------------------------------------------------===//
#include "GeneralizedGemm/LoopCounters.h"

#include "GeneralizedGemm/ControlStructure.h"

#include <algorithm>
#include <map>

using llvm::APInt;

namespace mlir::generalized_gemm {

namespace {

/// Dominators over locations plus the exit, iteratively.
std::vector<std::vector<bool>> dominatorsOf(const SemanticProgram &program) {
  unsigned n = program.locationCount + 1;
  std::vector<std::vector<unsigned>> predecessors(n);
  for (const Transition &t : program.transitions)
    predecessors[t.to].push_back(t.from);
  std::vector<std::vector<bool>> dom(n, std::vector<bool>(n, true));
  dom[program.entryLocation].assign(n, false);
  dom[program.entryLocation][program.entryLocation] = true;
  for (bool changed = true; changed;) {
    changed = false;
    for (unsigned node = 0; node < n; ++node) {
      if (node == program.entryLocation)
        continue;
      std::vector<bool> next(n, !predecessors[node].empty());
      for (unsigned p : predecessors[node])
        for (unsigned d = 0; d < n; ++d)
          next[d] = next[d] && dom[p][d];
      next[node] = true;
      if (next != dom[node]) {
        dom[node] = next;
        changed = true;
      }
    }
  }
  return dom;
}

} // namespace

CounterInstrumentation addIterationCounters(SemanticProgram &program) {
  CounterInstrumentation out;
  if (!program.isSupported()) {
    out.reason = "the program is unsupported";
    return out;
  }
  if (!program.iterationCounters.empty()) {
    out.reason = "the program already has counters";
    return out;
  }
  llvm::SmallVector<unsigned> cuts = computeCutSet(program);
  std::string why;
  if (!cutSetBreaksCycles(program, cuts, why)) {
    out.reason = "no cut set: " + why;
    return out;
  }
  if (cuts.empty()) {
    out.reason = "no loop";
    return out;
  }
  TermContext &terms = *program.terms;
  unsigned width = program.indexBitWidth ? program.indexBitWidth : 64;
  auto dom = dominatorsOf(program);
  unsigned n = program.locationCount + 1;
  std::vector<std::vector<unsigned>> predecessors(n);
  for (const Transition &t : program.transitions)
    predecessors[t.to].push_back(t.from);
  for (unsigned header : cuts) {
    LoopCounter counter;
    counter.header = header;
    // The natural loop: the nodes the header dominates that reach it again.
    std::vector<bool> inLoop(n, false);
    std::vector<unsigned> work{header};
    inLoop[header] = true;
    while (!work.empty()) {
      unsigned node = work.back();
      work.pop_back();
      for (unsigned p : predecessors[node])
        if (!inLoop[p] && dom[p][header]) {
          inLoop[p] = true;
          work.push_back(p);
        }
    }
    for (unsigned node = 0; node < n; ++node)
      if (inLoop[node])
        counter.body.push_back(node);
    StateVar var;
    var.kind = StateVar::IterationCounter;
    var.id = program.state.size();
    var.sort = Sort::bv(width);
    var.location = header;
    var.position = program.iterationCounters.size();
    var.loopCarried = true;
    program.state.push_back(var);
    counter.state = var.id;
    Term leaf = terms.state(var.id, var.sort);
    Term zero = terms.bv(APInt::getZero(width));
    Term bumped = terms.binary(TermOp::Add, leaf, terms.bv(APInt(width, 1)));
    for (Transition &t : program.transitions)
      if (t.to == header)
        t.assignments.push_back({var.id, inLoop[t.from] ? bumped : zero});
    program.iterationCounters.push_back(var.id);
    out.counters.push_back(std::move(counter));
  }
  return out;
}

CounterReplay compareRuns(const SemanticProgram &original, const SemanticProgram &instrumented,
                          const CounterInstrumentation &instrumentation, const EntryState &entry,
                          const RunLimits &limits) {
  CounterReplay out;
  std::string error;
  auto a = initialConfiguration(original, entry, limits, error);
  if (!a) {
    out.reason = "original: " + error;
    return out;
  }
  auto b = initialConfiguration(instrumented, entry, limits, error);
  if (!b) {
    out.reason = "instrumented: " + error;
    return out;
  }
  // Arrivals at each header since the loop was entered, counted here from the
  // transitions taken, independently of the state.
  std::map<unsigned, uint64_t> expected;
  std::map<unsigned, const LoopCounter *> byHeader;
  for (const LoopCounter &c : instrumentation.counters)
    byHeader[c.header] = &c;
  auto sameEvent = [](const MacEvent &x, const MacEvent &y) {
    return x.site == y.site && x.step == y.step && x.coordinates == y.coordinates && x.f == y.f &&
           x.g == y.g && x.h == y.h && x.hRead == y.hRead && x.aValue == y.aValue && x.bValue == y.bValue &&
           x.accumulatedValue == y.accumulatedValue && x.updateValue == y.updateValue;
  };
  std::vector<APInt> returnsA, returnsB;
  std::vector<bool> poisonA, poisonB;
  while (true) {
    if (a->location != b->location) {
      out.reason = "the locations differ at step " + std::to_string(out.steps);
      return out;
    }
    if (a->location == original.exitLocation())
      break;
    if (out.steps >= limits.fuel) {
      out.reason = "fuel exhausted: this proves nothing";
      return out;
    }
    StepOutcome sa = stepConfiguration(original, entry, *a, &returnsA, &poisonA);
    StepOutcome sb = stepConfiguration(instrumented, entry, *b, &returnsB, &poisonB);
    ++out.steps;
    if (sa.stopped.has_value() != sb.stopped.has_value() ||
        (sa.stopped && *sa.stopped != *sb.stopped)) {
      out.reason = "the runs stop differently at step " + std::to_string(out.steps);
      return out;
    }
    if (sa.transition != sb.transition) {
      out.reason = "the transitions differ at step " + std::to_string(out.steps);
      return out;
    }
    if (sa.events.size() != sb.events.size()) {
      out.reason = "the event counts differ at step " + std::to_string(out.steps);
      return out;
    }
    for (size_t e = 0; e < sa.events.size(); ++e)
      if (!sameEvent(sa.events[e], sb.events[e])) {
        out.reason = "an event differs at step " + std::to_string(out.steps);
        return out;
      }
    out.events += sa.events.size();
    if (sa.stopped) {
      // Both stopped the same way; nothing further to compare.
      out.equal = true;
      out.reason = std::string("both runs stopped: ") + stringifyRunStatus(*sa.stopped);
      return out;
    }
    // The state the original has must agree; the counters must count.
    for (unsigned id = 0; id < original.state.size(); ++id)
      if (a->state[id] != b->state[id]) {
        out.reason = "state s" + std::to_string(id) + " differs at step " + std::to_string(out.steps);
        return out;
      }
    const Transition &taken = instrumented.transitions[sb.transition];
    auto found = byHeader.find(taken.to);
    if (found != byHeader.end()) {
      const LoopCounter &c = *found->second;
      bool fromInside = std::binary_search(c.body.begin(), c.body.end(), taken.from);
      expected[c.header] = fromInside ? expected[c.header] + 1 : 0;
      ++out.headerArrivals;
      const auto &value = b->state[c.state];
      unsigned width = instrumented.state[c.state].sort.width;
      if (!value || *value != APInt(width, expected[c.header], /*isSigned=*/false, /*implicitTrunc=*/true)) {
        out.reason = "the counter of header " + std::to_string(c.header) + " does not count at step " +
                     std::to_string(out.steps);
        return out;
      }
    }
  }
  if (returnsA != returnsB || poisonA != poisonB) {
    out.reason = "the returns differ";
    return out;
  }
  out.equal = true;
  out.reason = "same transitions, events and returns; every counter counts its header's arrivals";
  return out;
}

} // namespace mlir::generalized_gemm
