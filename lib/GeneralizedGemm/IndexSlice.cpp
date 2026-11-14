#include "GeneralizedGemm/IndexSlice.h"

#include "GeneralizedGemm/ControlStructure.h"
#include "GeneralizedGemm/RelationalEncoding.h"
#include "GeneralizedGemm/TerminationObligations.h"

#include "llvm/ADT/DenseSet.h"

#include <algorithm>
#include <map>
#include <set>

using namespace llvm;

namespace mlir::generalized_gemm {

const char *stringifyClosedFormStatus(ClosedFormStatus status) {
  switch (status) {
  case ClosedFormStatus::Proved: return "PROVED";
  case ClosedFormStatus::Refuted: return "REFUTED";
  case ClosedFormStatus::SampleConsistent: return "SAMPLE_CONSISTENT";
  case ClosedFormStatus::Unknown: return "UNKNOWN";
  }
  return "?";
}

namespace {

/// The slice's variables: the subscript's support, closed under the assignments
/// to them and the guards of the transitions that carry those assignments.
void closeSlice(const SemanticProgram &program, IndexSlice &slice) {
  std::set<unsigned> vars(slice.variables.begin(), slice.variables.end());
  std::set<unsigned> used;
  for (bool changed = true; changed;) {
    changed = false;
    for (unsigned t = 0; t < program.transitions.size(); ++t) {
      const Transition &tr = program.transitions[t];
      bool assigns = false;
      for (const auto &[id, term] : tr.assignments)
        assigns = assigns || vars.count(id);
      if (!assigns)
        continue;
      used.insert(t);
      for (const auto &[id, term] : tr.assignments)
        if (vars.count(id))
          for (unsigned v : stateSupport(term))
            changed = vars.insert(v).second || changed;
      for (unsigned v : stateSupport(tr.guard))
        changed = vars.insert(v).second || changed;
    }
  }
  slice.variables.assign(vars.begin(), vars.end());
  slice.transitions.assign(used.begin(), used.end());
  DenseSet<Term> seen;
  for (unsigned t : used)
    if (seen.insert(program.transitions[t].guard).second)
      slice.guards.push_back(program.transitions[t].guard);
}

/// Dominators over the locations, the exit included; the entry dominates all.
std::vector<std::vector<bool>> dominators(const SemanticProgram &program) {
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

LoopSpecification loopSpecification(const SemanticProgram &program, const IndexSlice &slice,
                                    const PathSet &set, unsigned header,
                                    ArrayRef<unsigned> cuts,
                                    const std::vector<std::vector<bool>> &dom) {
  TermContext &terms = *program.terms;
  LoopSpecification spec;
  spec.header = header;
  // The natural loop: the nodes the header dominates that reach it again.
  {
    unsigned n = program.locationCount + 1;
    std::vector<std::vector<unsigned>> predecessors(n);
    for (const Transition &t : program.transitions)
      predecessors[t.to].push_back(t.from);
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
        spec.component.push_back(node);
  }
  // Innermost: no path from the header reaches a cut it dominates, which would be
  // an inner loop's header. A path to a cut that dominates this one is the exit.
  spec.innermost = true;
  for (const CutPath &path : set.paths)
    if (!path.fromEntry && path.from == header && path.to != header &&
        llvm::is_contained(cuts, path.to) && dom[path.to][header])
      spec.innermost = false;
  for (const CutPath &path : set.paths)
    if (!path.fromEntry && path.from == header && path.to == header)
      spec.paths.push_back(path.id);
  if (!spec.innermost) {
    spec.reason = "the loop contains another loop";
    return spec;
  }
  if (spec.paths.empty()) {
    spec.reason = "no path returns to the header";
    return spec;
  }
  auto assigned = [&](unsigned v) {
    for (unsigned id : spec.paths)
      if (set.paths[id].state[v])
        return true;
    return false;
  };
  // Carried: the slice's variables the loop assigns, closed under what the
  // condition and their updates read among the loop's other assigned variables,
  // as the thesis closes the loop's state under its dependences (F.7).
  std::set<unsigned> carried;
  for (unsigned v : slice.variables)
    if (assigned(v))
      carried.insert(v);
  if (carried.empty()) {
    spec.reason = "the loop assigns no variable of the slice";
    return spec;
  }
  Term condition = terms.boolean(false);
  for (unsigned id : spec.paths)
    condition = terms.logicalOr(condition, set.paths[id].condition);
  auto stepOf = [&](unsigned v) {
    Term current = terms.state(v, program.state[v].sort);
    Term tau = current;
    for (auto it = spec.paths.rbegin(); it != spec.paths.rend(); ++it) {
      const CutPath &path = set.paths[*it];
      Term after = path.state[v] ? path.state[v] : current;
      tau = terms.ite(path.condition, after, tau);
    }
    return tau;
  };
  for (bool changed = true; changed;) {
    changed = false;
    SmallVector<unsigned> conditionReads = stateSupport(condition);
    std::vector<unsigned> reads(conditionReads.begin(), conditionReads.end());
    for (unsigned v : carried)
      for (unsigned r : stateSupport(stepOf(v)))
        reads.push_back(r);
    for (unsigned r : reads)
      if (!carried.count(r) && assigned(r)) {
        carried.insert(r);
        changed = true;
      }
  }
  spec.carried.assign(carried.begin(), carried.end());
  spec.condition = condition;
  for (unsigned v : spec.carried)
    spec.step.push_back(stepOf(v));
  // Fixed: what the condition, the steps and the subscript read besides.
  std::set<unsigned> fixed;
  auto gather = [&](Term term) {
    for (unsigned v : stateSupport(term))
      if (!carried.count(v))
        fixed.insert(v);
  };
  gather(spec.condition);
  for (Term tau : spec.step)
    gather(tau);
  gather(slice.subscript);
  spec.fixed.assign(fixed.begin(), fixed.end());
  // Init: when every path into the header from outside the loop gives v the same
  // term over parameters alone, that term (the thesis's closed term for a loop no
  // loop precedes, extended to a loop entered the same way from an outer one);
  // else the symbolic entry value.
  std::vector<const CutPath *> entryPaths;
  for (const CutPath &path : set.paths)
    if (path.to == header && (path.fromEntry || !llvm::is_contained(spec.component, path.from)))
      entryPaths.push_back(&path);
  // A fixed variable the entry path defines over parameters and constants alone
  // holds that value at every arrival: a materialized constant, or a bound
  // computed before the loop. It is read as its definition, so that the
  // obligations do not quantify over values it never takes.
  // Such a variable is assigned on no path between cuts, and every path from the
  // function's entry to a cut gives it the same term over parameters alone.
  // Phase 5 widened this from the fixed variables to every state variable, so that
  // an init such as f := c0, with c0 a materialized constant, is read as the
  // constant and not as a symbolic entry value.
  std::map<unsigned, Term> definitions;
  for (const StateVar &var : program.state) {
    unsigned v = var.id;
    if (var.sort.kind != Sort::BV)
      continue;
    Term defined = nullptr;
    bool constant = true, any = false;
    for (const CutPath &path : set.paths) {
      if (!path.fromEntry) {
        constant = constant && !path.state[v];
        continue;
      }
      any = true;
      Term here = path.state[v];
      if (!here || !stateSupport(here).empty() || readsMemory(here) || containsFloat(here) ||
          (defined && defined != here))
        constant = false;
      defined = here;
    }
    if (any && constant && defined)
      definitions[v] = defined;
  }
  if (!definitions.empty()) {
    auto substitute = [&](Term term) {
      return terms.substitute(term, [&](Term leaf) -> Term {
        if (leaf->op != TermOp::State)
          return nullptr;
        auto found = definitions.find(leaf->index);
        return found == definitions.end() ? nullptr : found->second;
      });
    };
    spec.condition = substitute(spec.condition);
    for (Term &tau : spec.step)
      tau = substitute(tau);
    std::vector<unsigned> remaining;
    for (unsigned v : spec.fixed)
      if (definitions.count(v))
        spec.defined.push_back(v);
      else
        remaining.push_back(v);
    spec.fixed = remaining;
  }
  auto defined = [&](Term term) {
    if (definitions.empty() || !term)
      return term;
    return terms.substitute(term, [&](Term leaf) -> Term {
      if (leaf->op != TermOp::State)
        return nullptr;
      auto found = definitions.find(leaf->index);
      return found == definitions.end() ? nullptr : found->second;
    });
  };
  for (unsigned v : spec.carried) {
    Term init = nullptr;
    bool same = !entryPaths.empty();
    for (const CutPath *path : entryPaths) {
      Term here = defined(path->state[v]);
      if (!here || !stateSupport(here).empty() || readsMemory(here) || containsFloat(here) ||
          (init && init != here))
        same = false;
      init = here;
    }
    spec.init.push_back(same ? init : nullptr);
  }
  // Synthetic leaves.
  unsigned base = program.state.size();
  unsigned width = program.indexBitWidth ? program.indexBitWidth : 64;
  spec.iteration = terms.state(base, Sort::bv(width));
  for (unsigned k = 0; k < spec.carried.size(); ++k)
    spec.entry.push_back(terms.state(base + 1 + k, program.state[spec.carried[k]].sort));
  return spec;
}

} // namespace

std::optional<std::vector<IndexSlice>> sliceSubscripts(const SemanticProgram &program,
                                                       const SliceLimits &limits,
                                                       std::string &why) {
  if (!program.isSupported()) {
    why = "the program is unsupported";
    return std::nullopt;
  }
  SmallVector<unsigned> cuts = computeCutSet(program);
  if (!cutSetBreaksCycles(program, cuts, why))
    return std::nullopt;
  PathSet set = enumerateCutPaths(program, cuts, limits.paths);
  if (!set.complete) {
    why = "the paths were not enumerated: " + set.reason;
    return std::nullopt;
  }
  auto dom = dominators(program);

  std::vector<IndexSlice> out;
  std::map<Term, unsigned> bySubscript;
  for (const MacSite &site : program.sites) {
    const MacAccess *accesses[3] = {&site.a, &site.b, site.output ? &*site.output : nullptr};
    for (unsigned operand = 0; operand < 3; ++operand) {
      if (!accesses[operand])
        continue;
      for (unsigned d = 0; d < accesses[operand]->index.size(); ++d) {
        Term subscript = accesses[operand]->index[d];
        auto [slot, fresh] = bySubscript.try_emplace(subscript, out.size());
        if (fresh) {
          IndexSlice slice;
          slice.subscript = subscript;
          auto support = stateSupport(subscript);
          slice.variables.assign(support.begin(), support.end());
          closeSlice(program, slice);
          if (readsMemory(subscript))
            slice.reason = "the subscript reads memory";
          else if (slice.variables.empty())
            slice.reason = "the subscript reads no state";
          else
            for (unsigned cut : cuts)
              slice.loops.push_back(loopSpecification(program, slice, set, cut, cuts, dom));
          out.push_back(std::move(slice));
        }
        out[slot->second].uses.push_back({site.id, operand, d});
      }
    }
  }
  return out;
}

std::map<unsigned, Term> entryDefinedState(const SemanticProgram &program, const SliceLimits &limits,
                                           std::string &why) {
  std::map<unsigned, Term> out;
  if (!program.isSupported()) {
    why = "the program is unsupported";
    return out;
  }
  SmallVector<unsigned> cuts = computeCutSet(program);
  if (!cutSetBreaksCycles(program, cuts, why))
    return out;
  PathSet set = enumerateCutPaths(program, cuts, limits.paths);
  if (!set.complete) {
    why = "the paths were not enumerated: " + set.reason;
    return out;
  }
  for (const StateVar &var : program.state) {
    if (var.sort.kind != Sort::BV)
      continue;
    Term defined = nullptr;
    bool constant = true, any = false;
    for (const CutPath &path : set.paths) {
      if (!path.fromEntry) {
        constant = constant && !path.state[var.id];
        continue;
      }
      any = true;
      Term here = path.state[var.id];
      if (!here || !stateSupport(here).empty() || readsMemory(here) || containsFloat(here) ||
          containsOp(here, TermOp::Shadow) || (defined && defined != here))
        constant = false;
      defined = here;
    }
    if (any && constant && defined)
      out[var.id] = defined;
  }
  return out;
}

std::string closedFormLeafName(const SemanticProgram &program, const LoopSpecification &spec,
                               Term leaf) {
  if (leaf == spec.iteration)
    return "m";
  for (unsigned k = 0; k < spec.entry.size(); ++k)
    if (leaf == spec.entry[k])
      return "entry_s" + std::to_string(spec.carried[k]);
  (void)program;
  if (leaf->op == TermOp::State)
    return "s" + std::to_string(leaf->index);
  if (leaf->op == TermOp::Param)
    return "p" + std::to_string(leaf->index);
  return formatTerm(leaf, 4);
}

namespace {

/// Evaluates a term over the program's state plus the synthetic leaves.
struct LeafEvaluator {
  LeafEvaluator(const SemanticProgram &program, const LoopSpecification &spec)
      : program(program), spec(spec) {
    config.state.assign(program.state.size() + 1 + spec.carried.size(), std::nullopt);
    config.shadow.assign(config.state.size(), false);
  }
  void setParameters(const std::vector<APInt> &parameters) { entry.parameters = parameters; }
  void setState(unsigned id, const APInt &value) { config.state[id] = value; }
  void setIteration(const APInt &m) { config.state[spec.iteration->index] = m; }
  void setEntry(unsigned k, const APInt &value) { config.state[spec.entry[k]->index] = value; }
  std::optional<APInt> value(Term term) { return evaluateTerm(program, entry, config, term); }
  const SemanticProgram &program;
  const LoopSpecification &spec;
  EntryState entry;
  Configuration config;
};

} // namespace

std::optional<LoopTrace> runSpecification(const SemanticProgram &program,
                                          const LoopSpecification &spec,
                                          const std::vector<APInt> &parameters,
                                          const std::vector<APInt> &entry,
                                          const std::vector<APInt> &fixedValues,
                                          uint64_t iterations) {
  if (entry.size() != spec.carried.size() || fixedValues.size() != spec.fixed.size())
    return std::nullopt;
  LeafEvaluator eval(program, spec);
  eval.setParameters(parameters);
  for (unsigned k = 0; k < spec.fixed.size(); ++k)
    eval.setState(spec.fixed[k], fixedValues[k]);
  LoopTrace trace;
  trace.parameters = parameters;
  trace.entry = entry;
  trace.fixedValues = fixedValues;
  std::vector<APInt> current = entry;
  trace.states.push_back(current);
  for (uint64_t m = 0; m < iterations; ++m) {
    for (unsigned k = 0; k < spec.carried.size(); ++k)
      eval.setState(spec.carried[k], current[k]);
    auto c = eval.value(spec.condition);
    if (!c)
      return std::nullopt;
    if (c->isZero())
      break;
    std::vector<APInt> next;
    for (Term tau : spec.step) {
      auto v = eval.value(tau);
      if (!v)
        return std::nullopt;
      next.push_back(*v);
    }
    current = std::move(next);
    trace.states.push_back(current);
  }
  return trace;
}

LoopReplay replayLoop(const SemanticProgram &program, const IndexSlice &slice,
                      const LoopSpecification &spec, const EntryState &entry,
                      const RunLimits &limits) {
  LoopReplay out;
  auto config = initialConfiguration(program, entry, limits, out.reason);
  if (!config) {
    out.status = RunStatus::InvalidEntry;
    return out;
  }
  std::set<unsigned> body(spec.component.begin(), spec.component.end());
  body.insert(spec.header);
  bool active = false;
  std::optional<Configuration> lastHeader;
  LoopTrace trace;
  auto carriedValues = [&](const Configuration &c, std::vector<APInt> &values) {
    for (unsigned v : spec.carried) {
      if (!c.state[v]) {
        ++out.undefined;
        return false;
      }
      values.push_back(*c.state[v]);
    }
    return true;
  };
  auto finish = [&]() {
    if (!active)
      return;
    if (lastHeader) {
      auto c = evaluateTerm(program, entry, *lastHeader, spec.condition);
      if (!c)
        ++out.undefined;
      else if (!c->isZero())
        ++out.conditionMismatches;
    }
    out.traces.push_back(std::move(trace));
    trace = LoopTrace();
    active = false;
    lastHeader.reset();
  };
  uint64_t steps = 0;
  while (true) {
    if (config->location == program.exitLocation()) {
      finish();
      out.status = RunStatus::Returned;
      break;
    }
    if (steps >= limits.fuel) {
      out.status = RunStatus::OutOfFuel;
      out.reason = "fuel exhausted: this proves nothing";
      break;
    }
    if (config->location == spec.header) {
      std::vector<APInt> values;
      bool defined = carriedValues(*config, values);
      if (active && lastHeader && defined) {
        ++out.iterations;
        auto c = evaluateTerm(program, entry, *lastHeader, spec.condition);
        if (!c)
          ++out.undefined;
        else if (c->isZero())
          ++out.conditionMismatches;
        for (unsigned k = 0; k < spec.carried.size(); ++k) {
          auto predicted = evaluateTerm(program, entry, *lastHeader, spec.step[k]);
          if (!predicted)
            ++out.undefined;
          else if (*predicted != values[k])
            ++out.stepMismatches;
        }
      }
      if (!active) {
        trace = LoopTrace();
        trace.parameters = entry.parameters;
        trace.entry = values;
        for (unsigned v : spec.fixed) {
          if (!config->state[v]) {
            ++out.undefined;
            trace.fixedValues.push_back(APInt::getZero(program.state[v].sort.width));
          } else {
            trace.fixedValues.push_back(*config->state[v]);
          }
        }
        active = true;
      }
      if (defined)
        trace.states.push_back(values);
      // The state alone: c and τ read no memory, and a term that did has no value.
      Configuration header;
      header.location = config->location;
      header.state = config->state;
      header.shadow = config->shadow;
      lastHeader = std::move(header);
    } else if (active && !body.count(config->location)) {
      finish();
    }
    // The subscript is over the configuration at the entry of the block (I5), so
    // it is evaluated before the step and compared with the events the step emits.
    std::optional<APInt> subscript = evaluateTerm(program, entry, *config, slice.subscript);
    std::vector<APInt> returns;
    std::vector<bool> poison;
    StepOutcome step = stepConfiguration(program, entry, *config, &returns, &poison);
    if (step.stopped) {
      out.status = *step.stopped;
      out.reason = step.reason;
      break;
    }
    ++steps;
    for (const MacEvent &event : step.events)
      for (const SliceUse &use : slice.uses) {
        if (event.site != use.site)
          continue;
        const std::vector<APInt> &index = use.operand == 0 ? event.f : use.operand == 1 ? event.g : event.h;
        if (use.dimension < index.size() && subscript && *subscript == index[use.dimension])
          ++out.subscriptMatches;
        else
          ++out.subscriptMismatches;
      }
  }
  return out;
}

CheckedClosedForm checkProgramOnTraces(const SemanticProgram &program,
                                       const LoopSpecification &spec, unsigned k,
                                       const IndexProgram &candidate, ArrayRef<LoopTrace> traces,
                                       uint64_t fuel) {
  (void)program;
  CheckedClosedForm out;
  out.variable = k < spec.carried.size() ? spec.carried[k] : UINT32_MAX;
  if (k >= spec.carried.size()) {
    out.reason = "no such carried variable";
    return out;
  }
  unsigned width = spec.iteration->sort.width;
  uint64_t compared = 0, cut = 0;
  IndexRunLimits limits;
  limits.fuel = fuel;
  for (const LoopTrace &trace : traces)
    for (size_t m = 0; m < trace.states.size(); ++m) {
      std::vector<APInt> inputs;
      bool known = true;
      for (Term leaf : candidate.inputs) {
        if (leaf == spec.iteration) {
          inputs.push_back(APInt(width, m, /*isSigned=*/false, /*implicitTrunc=*/true));
          continue;
        }
        bool found = false;
        for (unsigned e = 0; e < spec.entry.size() && !found; ++e)
          if (leaf == spec.entry[e]) {
            inputs.push_back(trace.entry[e]);
            found = true;
          }
        for (unsigned f = 0; f < spec.fixed.size() && !found; ++f)
          if (leaf->op == TermOp::State && leaf->index == spec.fixed[f]) {
            inputs.push_back(trace.fixedValues[f]);
            found = true;
          }
        if (!found && leaf->op == TermOp::Param && leaf->index < trace.parameters.size()) {
          inputs.push_back(trace.parameters[leaf->index]);
          found = true;
        }
        known = known && found;
      }
      if (!known) {
        out.status = ClosedFormStatus::Unknown;
        out.reason = "the program reads a leaf the traces do not value";
        return out;
      }
      IndexRun run;
      try {
        run = runIndexProgram(candidate, inputs, limits);
      } catch (const TermError &error) {
        out.status = ClosedFormStatus::Unknown;
        out.reason = error.what();
        return out;
      }
      if (run.status == IndexRunStatus::OutOfFuel) {
        ++cut;
        continue;
      }
      ++compared;
      // NoRule (G.5): the semantics has no output at this sample, which the
      // specification's totality refuses; the same inputs reproduce it.
      if (run.status == IndexRunStatus::NoRule || run.value != trace.states[m][k]) {
        out.status = ClosedFormStatus::Refuted;
        out.reason = run.status == IndexRunStatus::NoRule
                         ? "no rule applies at the sampled iteration m = " + std::to_string(m) + ": " + run.reason
                         : "differs from the sampled iteration m = " + std::to_string(m);
        out.model.push_back({"m", APInt(width, m, false, true)});
        for (unsigned e = 0; e < spec.entry.size(); ++e)
          out.model.push_back({"entry_s" + std::to_string(spec.carried[e]), trace.entry[e]});
        for (unsigned f = 0; f < spec.fixed.size(); ++f)
          out.model.push_back({"s" + std::to_string(spec.fixed[f]), trace.fixedValues[f]});
        for (unsigned p = 0; p < trace.parameters.size(); ++p)
          out.model.push_back({"p" + std::to_string(p), trace.parameters[p]});
        return out;
      }
    }
  if (cut > 0) {
    out.status = ClosedFormStatus::Unknown;
    out.reason = std::to_string(cut) + " of " + std::to_string(cut + compared) +
                 " runs exhausted the fuel: no value there, and no claim about termination";
    return out;
  }
  if (compared == 0) {
    out.status = ClosedFormStatus::Unknown;
    out.reason = "no sampled iteration to compare";
    return out;
  }
  out.status = ClosedFormStatus::SampleConsistent;
  out.reason = "equal to all " + std::to_string(compared) +
               " sampled iterations of " + std::to_string(traces.size()) +
               " activations at the finite tier; not a proof";
  return out;
}

std::vector<CheckedClosedForm> checkClosedForms(const SemanticProgram &program,
                                                const LoopSpecification &spec,
                                                ArrayRef<ClosedForm> forms,
                                                const ParameterScope &scope,
                                                const SmtLimits &limits, SmtBackend &backend) {
  TermContext &terms = *program.terms;
  std::vector<CheckedClosedForm> out;
  std::string why;
  Term scopeTerm = scopeFormula(program, scope, why);
  // The first form listed for each variable stands for it in the others'
  // obligations; a form under check stands for its own variable, so several
  // candidates for one variable may be checked in one call. A while candidate
  // has no term: its variable stays a free leaf.
  std::map<unsigned, Term> others, formOf;
  for (const ClosedForm &form : forms)
    if (form.term)
      others.try_emplace(form.variable, form.term);
  std::vector<unsigned> substituted, freed;
  auto substituteForms = [&](Term term) {
    return terms.substitute(term, [&](Term leaf) -> Term {
      if (leaf->op != TermOp::State)
        return nullptr;
      auto found = formOf.find(leaf->index);
      if (found == formOf.end()) {
        if (llvm::is_contained(spec.carried, leaf->index) && !llvm::is_contained(freed, leaf->index))
          freed.push_back(leaf->index);
        return nullptr;
      }
      if (!llvm::is_contained(substituted, leaf->index))
        substituted.push_back(leaf->index);
      return found->second;
    });
  };
  auto shiftIteration = [&](Term term, Term replacement) {
    return terms.substitute(term, [&](Term leaf) -> Term {
      return leaf == spec.iteration ? replacement : nullptr;
    });
  };
  unsigned width = spec.iteration->sort.width;
  Term zero = terms.bv(APInt::getZero(width));
  Term one = terms.bv(APInt(width, 1));
  Term next = terms.binary(TermOp::Add, spec.iteration, one);
  Term notLast = terms.icmp(Predicate::Ne, spec.iteration, terms.bv(APInt::getAllOnes(width)));
  // The counter r of a while candidate, for W-term: a synthetic leaf past m and the entries.
  Term counterLeaf = terms.state(program.state.size() + 1 + spec.carried.size(), Sort::bv(width));
  for (const ClosedForm &form : forms) {
    CheckedClosedForm checked;
    checked.variable = form.variable;
    formOf = others;
    substituted.clear();
    freed.clear();
    auto k = std::find(spec.carried.begin(), spec.carried.end(), form.variable);
    if (k == spec.carried.end()) {
      checked.reason = "not a carried variable of this loop";
      out.push_back(std::move(checked));
      continue;
    }
    unsigned index = k - spec.carried.begin();
    if (!scopeTerm) {
      checked.reason = "malformed scope: " + why;
      out.push_back(std::move(checked));
      continue;
    }
    Sort sort = program.state[form.variable].sort;
    if (!form.term && !form.program) {
      checked.reason = "no candidate";
      out.push_back(std::move(checked));
      continue;
    }
    if (form.term && form.term->sort != sort) {
      checked.reason = "the form has the wrong sort";
      out.push_back(std::move(checked));
      continue;
    }
    if (form.term)
      formOf[form.variable] = form.term;
    else
      formOf.erase(form.variable); // The while candidate's own variable is free in its step.
    std::vector<std::pair<std::string, Term>> aux{{"m", spec.iteration}};
    for (unsigned e = 0; e < spec.entry.size(); ++e)
      aux.push_back({"entry_s" + std::to_string(spec.carried[e]), spec.entry[e]});
    if (form.program)
      aux.push_back({"r", counterLeaf});
    auto ask = [&](const std::string &id, Term violation) {
      ObligationRecord record;
      record.id = id;
      record.location = "loop " + std::to_string(spec.header) + " variable s" +
                        std::to_string(form.variable);
      record.expected = "unsat";
      record.script = obligationScript(id + " " + record.location, violation, aux);
      SmtResponse response = backend.check(record.script, limits, /*wantModel=*/true);
      record.result = stringifySmtAnswer(response.answer);
      record.discharged = response.answer == SmtAnswer::Unsat;
      if (response.answer == SmtAnswer::Sat) {
        checked.model.clear();
        for (const auto &[name, text] : response.model) {
          // The script declares every leaf of the obligation, the synthetic ones (m, the
          // entries, r, the history) under their s<id> names as well as their aux names;
          // only the program's own state and parameters are read back by s<id>/p<id>.
          // (Found by the sanitizer build: the unguarded lookup read past the table.)
          unsigned w = 64;
          if (name == "m" || name == "r")
            w = width;
          else if (name.rfind("entry_s", 0) == 0) {
            unsigned id = std::stoul(name.substr(7));
            if (id >= program.state.size())
              continue;
            w = program.state[id].sort.width;
          } else if (name.size() > 1 && name[0] == 's' && isdigit((unsigned char)name[1])) {
            unsigned id = std::stoul(name.substr(1));
            if (id >= program.state.size())
              continue;
            w = program.state[id].sort.width;
          } else if (name.size() > 1 && name[0] == 'p' && isdigit((unsigned char)name[1])) {
            unsigned id = std::stoul(name.substr(1));
            if (id >= program.parameters.size())
              continue;
            w = program.parameters[id].sort.width;
          } else
            continue;
          checked.model.push_back({name, APInt(w, text, 10)});
        }
        record.evidence = "model";
      } else if (response.answer == SmtAnswer::Unknown) {
        record.evidence = response.detail;
      }
      checked.records.push_back(record);
      return response.answer;
    };
    Term initial = spec.init[index] ? spec.init[index] : spec.entry[index];
    // The obligations to ask, in order: (id, violation). All unsat is discharged.
    std::vector<std::pair<std::string, Term>> obligations;
    bool shapeOk = true;
    std::string what, lemma; // What was discharged; the coupling lemma the proof rests on.
    Term stepPremise = terms.logicalAnd(terms.logicalAnd(scopeTerm, notLast), substituteForms(spec.condition));
    auto inputsAreLeaves = [&](const IndexProgram &program, std::string &reason) {
      for (Term input : program.inputs) {
        bool known = input == spec.iteration || llvm::is_contained(spec.entry, input) ||
                     input->op == TermOp::Param ||
                     (input->op == TermOp::State && llvm::is_contained(spec.fixed, input->index));
        if (!known) {
          reason = "an input is not a leaf of the specification";
          return false;
        }
      }
      return true;
    };
    if (form.term) {
      what = "init and step";
      // Init: f(0) = init, or f(0) = entry when the entry is symbolic.
      obligations.push_back({"CF-init", terms.logicalAnd(scopeTerm, terms.logicalNot(terms.icmp(
                                            Predicate::Eq, shiftIteration(form.term, zero), initial)))});
      // Step: scope ∧ m ≠ 2^w-1 ∧ c(F(m)) ∧ f(m+1) ≠ τ(F(m)).
      obligations.push_back({"CF-step", terms.logicalAnd(stepPremise, terms.logicalNot(terms.icmp(
                                            Predicate::Eq, shiftIteration(form.term, next),
                                            substituteForms(spec.step[index]))))});
    } else {
      // Which template? The fixed-counter one (phase 4) or the history table (phase 5).
      std::string counterReason, tableReason;
      auto counter = matchCounterTemplate(*form.program, counterReason);
      auto table = counter ? std::nullopt : matchTableTemplate(*form.program, tableReason);
      ObligationRecord shape;
      shape.location = "loop " + std::to_string(spec.header) + " variable s" + std::to_string(form.variable);
      shape.expected = "structural";
      std::string reason;
      if (counter) {
        what = "shape, init, step and termination";
        lemma = "; the program is the fixed-counter template, whose run on m is the first m iterations "
                "of its run on m + 1 (coupling lemma W1 of the record)";
        shape.id = "W-shape";
        shape.evidence = "fixed-counter template (CounterTemplate), not the general while of G.1";
        if (form.program->inputs.at(counter->bound) != spec.iteration) {
          counter.reset();
          reason = "the bound is not the iteration count m";
        }
        if (counter && !inputsAreLeaves(*form.program, reason))
          counter.reset();
        if (counter && form.program->localWidths.at(counter->value) != sort.width) {
          counter.reset();
          reason = "the value local has the wrong width";
        }
        if (counter && form.program->localWidths.at(counter->counter) != width) {
          counter.reset();
          reason = "the counter has a width other than m's";
        }
        std::optional<Term> bodyTerm;
        Term initW = nullptr;
        if (counter) {
          // Locals: r is m, v is the free leaf s_v, every other local its start value 0.
          std::vector<Term> locals;
          for (unsigned v = 0; v < form.program->localWidths.size(); ++v)
            locals.push_back(terms.bv(APInt::getZero(form.program->localWidths[v])));
          locals[counter->counter] = spec.iteration;
          locals[counter->value] = terms.state(form.variable, sort);
          initW = indexExprTerm(terms, *form.program, counter->init, locals);
          bodyTerm = indexStatementTerm(terms, *form.program, counter->body, locals, counter->value);
          if (!initW || !bodyTerm) {
            counter.reset();
            reason = "the body does not lower";
          }
        }
        shape.result = counter ? "matched" : "not matched: " + reason;
        shape.discharged = counter.has_value();
        checked.records.push_back(shape);
        if (!counter) {
          shapeOk = false;
        } else {
          obligations.push_back({"W-init", terms.logicalAnd(scopeTerm, terms.logicalNot(terms.icmp(Predicate::Eq, initW, initial)))});
          obligations.push_back({"W-step", terms.logicalAnd(stepPremise, terms.logicalNot(terms.icmp(
                                               Predicate::Eq, *bodyTerm, substituteForms(spec.step[index]))))});
          // W-term: the variant m − r falls by one at every iteration while r < m.
          Term before = terms.binary(TermOp::Sub, spec.iteration, counterLeaf);
          Term after = terms.binary(TermOp::Sub, spec.iteration, terms.binary(TermOp::Add, counterLeaf, one));
          obligations.push_back({"W-term", terms.logicalAnd(terms.icmp(Predicate::Ult, counterLeaf, spec.iteration),
                                                             terms.logicalNot(terms.icmp(Predicate::Ult, after, before)))});
          if (!llvm::is_contained(freed, form.variable))
            freed.push_back(form.variable);
        }
      } else if (table) {
        // Phase 5: the history table template. T[m] = v(m) by induction on m, with the
        // table's elements T[m-1], ..., T[m-d+1] the carried variables that trail v by
        // one, two, ... iterations (the history chain), proved by TT-shift.
        what = "shape, init, step, shift, bounds and termination";
        lemma = "; the program is the history table template of G.5, whose table after the run on m "
                "holds T[j] = v(j) for j <= m (coupling lemma T1 of the record); for m < 2^" +
                std::to_string(width) + " - " + std::to_string(table->depth()) + ", where the extent m + d does not wrap";
        shape.id = "TT-shape";
        shape.evidence = "history table template (TableTemplate), not the general array grammar of G.5";
        unsigned d = table->depth();
        if (form.program->inputs.at(table->bound) != spec.iteration) {
          table.reset();
          reason = "the bound is not the iteration count m";
        }
        if (table && !inputsAreLeaves(*form.program, reason))
          table.reset();
        if (table && (form.program->arrays[0].width != sort.width || form.program->arrays[0].indexWidth != width ||
                      form.program->localWidths.at(table->counter) != width)) {
          table.reset();
          reason = "the table's widths differ from the variable's or m's";
        }
        // The history chain: v_0 = the variable; v_j the carried variable with v_j(m) =
        // v_{j-1}(m-1), proposed by the caller (form.history) or sought here.
        std::vector<unsigned> chain{form.variable};
        if (table) {
          std::vector<unsigned> proposed = form.history;
          for (unsigned j = 1; j < d && table; ++j) {
            std::optional<unsigned> pick;
            if (j - 1 < proposed.size()) {
              if (llvm::is_contained(spec.carried, proposed[j - 1]) && !llvm::is_contained(chain, proposed[j - 1]))
                pick = proposed[j - 1];
            } else {
              // Seek one syntactically: a step that is exactly the previous link.
              Term previous = terms.state(chain.back(), program.state[chain.back()].sort);
              for (unsigned u = 0; u < spec.carried.size() && !pick; ++u)
                if (!llvm::is_contained(chain, spec.carried[u]) && spec.step[u] == previous)
                  pick = spec.carried[u];
            }
            if (!pick) {
              table.reset();
              reason = "no carried variable trails the chain at depth " + std::to_string(j) +
                       " (the history is not in the loop's state)";
            } else {
              chain.push_back(*pick);
            }
          }
        }
        std::vector<Term> initTerms;
        Term stepTerm = nullptr;
        if (table) {
          std::vector<Term> locals;
          for (unsigned v = 0; v < form.program->localWidths.size(); ++v)
            locals.push_back(terms.bv(APInt::getZero(form.program->localWidths[v])));
          for (unsigned j = 0; j < d; ++j) {
            Term k = indexExprTerm(terms, *form.program, table->inits[j], locals);
            if (!k) {
              table.reset();
              reason = "an init value does not lower";
              break;
            }
            initTerms.push_back(k);
          }
        }
        if (table) {
          // E at r = m + 1 with T[r - j] read as the chain variable v_{j-1} at m.
          std::vector<Term> locals;
          for (unsigned v = 0; v < form.program->localWidths.size(); ++v)
            locals.push_back(terms.bv(APInt::getZero(form.program->localWidths[v])));
          locals[table->counter] = next;
          ReadLowering reads = [&](unsigned array, ArrayRef<Term> indices) -> Term {
            if (array != 0 || indices.size() != 1)
              return nullptr;
            // The index is (m + 1) - j for a constant j in [1, d].
            Term ix = indices[0];
            if (ix->op != TermOp::Sub || ix->args[0] != next || ix->args[1]->op != TermOp::Const)
              return nullptr;
            uint64_t j = ix->args[1]->value.getZExtValue();
            if (j < 1 || j > d || j - 1 >= chain.size())
              return nullptr;
            unsigned v = chain[j - 1];
            return terms.state(v, program.state[v].sort);
          };
          stepTerm = indexExprTerm(terms, *form.program, table->value, locals, reads);
          if (!stepTerm) {
            table.reset();
            reason = "E does not lower over the history chain";
          }
        }
        shape.result = table ? "matched; history chain" : "not matched: " + reason;
        if (table)
          for (unsigned v : chain)
            shape.result += " s" + std::to_string(v);
        shape.discharged = table.has_value();
        checked.records.push_back(shape);
        if (!table) {
          shapeOk = false;
        } else {
          // TT-init_j: the specification unrolled j steps from the entry equals k_j,
          // while the loop continues up to j.
          std::map<unsigned, Term> state; // Carried variable -> term at iteration i.
          for (unsigned u = 0; u < spec.carried.size(); ++u)
            state[spec.carried[u]] = spec.init[u] ? spec.init[u] : spec.entry[u];
          Term continued = scopeTerm;
          for (unsigned j = 0; j < d; ++j) {
            auto at = [&](Term t) {
              return terms.substitute(t, [&](Term leaf) -> Term {
                if (leaf->op != TermOp::State)
                  return nullptr;
                auto found = state.find(leaf->index);
                return found == state.end() ? nullptr : found->second;
              });
            };
            obligations.push_back({"TT-init" + std::to_string(j),
                                   terms.logicalAnd(continued, terms.logicalNot(terms.icmp(
                                                                   Predicate::Eq, state.at(form.variable), initTerms[j])))});
            continued = terms.logicalAnd(continued, at(spec.condition));
            std::map<unsigned, Term> nextState;
            for (unsigned u = 0; u < spec.carried.size(); ++u)
              nextState[spec.carried[u]] = at(spec.step[u]);
            state = std::move(nextState);
          }
          // The table's extent m + d must not wrap: every TT obligation below takes
          // m < 2^w − d as a premise, and the proof is for those m (the reason says so).
          Term noWrap = terms.icmp(Predicate::Ult, spec.iteration,
                                   terms.bv(APInt::getAllOnes(width) - APInt(width, d - 1)));
          stepPremise = terms.logicalAnd(stepPremise, noWrap);
          // TT-step: scope ∧ m ≠ 2^w−1 ∧ m ≥ d−1 ∧ c(F(m)) ∧ E(m+1, chain) ≠ τ_v(F(m)).
          Term pastInits = terms.icmp(Predicate::Uge, spec.iteration, terms.bv(APInt(width, d - 1)));
          obligations.push_back({"TT-step", terms.logicalAnd(terms.logicalAnd(stepPremise, pastInits),
                                                             terms.logicalNot(terms.icmp(Predicate::Eq, stepTerm,
                                                                                         substituteForms(spec.step[index]))))});
          // TT-shift_j: τ_{v_j}(F(m)) = v_{j-1}(m).
          for (unsigned j = 1; j < chain.size(); ++j) {
            unsigned u = std::find(spec.carried.begin(), spec.carried.end(), chain[j]) - spec.carried.begin();
            Term previous = terms.state(chain[j - 1], program.state[chain[j - 1]].sort);
            obligations.push_back({"TT-shift" + std::to_string(j),
                                   terms.logicalAnd(stepPremise, terms.logicalNot(terms.icmp(
                                                                     Predicate::Eq, substituteForms(spec.step[u]), previous)))});
          }
          // TT-bounds: d ≤ r < m + 1 (and m ≠ 2^w−1) keeps every write in the extent
          // m + d and every read T[r − j] at an index written before: r − j < r.
          Term dTerm = terms.bv(APInt(width, d));
          Term inLoop = terms.logicalAnd(terms.logicalAnd(terms.icmp(Predicate::Ule, dTerm, counterLeaf),
                                                          terms.icmp(Predicate::Ult, counterLeaf, next)),
                                         terms.logicalAnd(notLast, noWrap));
          Term safe = terms.icmp(Predicate::Ult, counterLeaf, terms.binary(TermOp::Add, spec.iteration, dTerm));
          for (unsigned j = 1; j <= d; ++j) {
            Term back = terms.binary(TermOp::Sub, counterLeaf, terms.bv(APInt(width, j)));
            safe = terms.logicalAnd(safe, terms.icmp(Predicate::Ult, back, counterLeaf));
          }
          obligations.push_back({"TT-bounds", terms.logicalAnd(inLoop, terms.logicalNot(safe))});
          // TT-term: the variant (m + 1) − r falls by one while r < m + 1.
          Term before = terms.binary(TermOp::Sub, next, counterLeaf);
          Term after = terms.binary(TermOp::Sub, next, terms.binary(TermOp::Add, counterLeaf, one));
          obligations.push_back({"TT-term", terms.logicalAnd(terms.icmp(Predicate::Ult, counterLeaf, next),
                                                              terms.logicalNot(terms.icmp(Predicate::Ult, after, before)))});
          for (unsigned v : chain)
            if (!llvm::is_contained(freed, v))
              freed.push_back(v);
        }
      } else {
        shape.id = "W-shape";
        shape.result = "not matched: " + counterReason + "; as a table: " + tableReason;
        shape.discharged = false;
        shape.evidence = "neither the fixed-counter template nor the history table template";
        checked.records.push_back(shape);
        shapeOk = false;
      }
    }
    if (!shapeOk) {
      checked.status = ClosedFormStatus::Unknown;
      checked.reason = "not of a supported template: " + checked.records.back().result;
      out.push_back(std::move(checked));
      continue;
    }
    // Ask each obligation; keep the first model.
    bool allUnsat = true, anySat = false, satHasFree = false;
    std::string satId;
    std::vector<std::pair<std::string, APInt>> satModel;
    for (const auto &[id, violation] : obligations) {
      SmtAnswer answer = ask(id, violation);
      allUnsat = allUnsat && answer == SmtAnswer::Unsat;
      if (answer == SmtAnswer::Sat && !anySat) {
        anySat = true;
        satId = id;
        satModel = checked.model;
        // A step-like obligation reads the free leaves; init and termination do not.
        satHasFree = !freed.empty() && id.find("init") == std::string::npos && id.find("term") == std::string::npos &&
                     id.find("bounds") == std::string::npos;
      }
    }
    for (unsigned v : substituted)
      if (v != form.variable)
        checked.dependsOn.push_back(v);
    checked.freeLeaves = freed;
    checked.discharged = allUnsat;
    std::string tail = " at width " + std::to_string(width) + " for m < 2^" + std::to_string(width) +
                       " - 1 under the scope";
    if (checked.discharged) {
      // Provisional: the fixpoint below demotes a form whose substituted forms fail.
      checked.status = ClosedFormStatus::Proved;
      checked.reason = what + " discharged" + tail + lemma;
    } else if (anySat) {
      checked.model = satModel;
      if (satHasFree) {
        checked.status = ClosedFormStatus::Unknown;
        std::string names;
        for (unsigned v : freed)
          names += (names.empty() ? "" : ", ") + std::string("s") + std::to_string(v);
        checked.reason = satId + " has a model with " + names +
                         " free: it may be unreachable, so this is neither a proof nor a refutation";
      } else {
        checked.status = ClosedFormStatus::Refuted;
        // "init", "step", "term", "init0", ...: the part after the obligation family.
        std::string which = satId.substr(satId.find('-') + 1);
        if (which.rfind("term", 0) == 0)
          which = "termination";
        checked.reason = which + " has a counterexample";
      }
    } else {
      checked.status = ClosedFormStatus::Unknown;
      checked.reason = "the solver did not decide an obligation";
    }
    out.push_back(std::move(checked));
  }
  // Simultaneous induction: a form stands only with the forms it substituted. The
  // first listed form of each variable is the one the others read.
  std::map<unsigned, const CheckedClosedForm *> standing;
  for (const ClosedForm &form : forms)
    for (const CheckedClosedForm &c : out)
      if (c.variable == form.variable && form.term && !standing.count(form.variable)) {
        standing[form.variable] = &c;
        break;
      }
  for (bool changed = true; changed;) {
    changed = false;
    for (CheckedClosedForm &c : out) {
      if (c.status != ClosedFormStatus::Proved)
        continue;
      for (unsigned v : c.dependsOn) {
        auto found = standing.find(v);
        if (found == standing.end() || found->second->status != ClosedFormStatus::Proved) {
          c.status = ClosedFormStatus::Unknown;
          c.reason = "obligations discharged, but the substituted form of s" + std::to_string(v) +
                     " is not proved";
          changed = true;
          break;
        }
      }
    }
  }
  return out;
}

} // namespace mlir::generalized_gemm
