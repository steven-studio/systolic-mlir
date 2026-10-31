#include "GeneralizedGemm/ConcreteEvaluator.h"

#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/SmallString.h"

#include <functional>
#include <set>

using namespace mlir::generalized_gemm;
using llvm::APInt;

namespace {

enum class Failure { None, UndefinedState, Uninitialized, OutOfRange };

/// One evaluation of terms against one configuration.
class Evaluation {
public:
  Evaluation(const EntryState &entry, const Configuration &config)
      : entry(entry), config(config) {}

  std::optional<APInt> value(Term t);
  Failure failure = Failure::None;

private:
  std::optional<APInt> fail(Failure f) {
    failure = f;
    return std::nullopt;
  }
  std::optional<APInt> loadFrom(Term memory, llvm::ArrayRef<APInt> key);
  std::optional<APInt> compute(Term t);

  const EntryState &entry;
  const Configuration &config;
  llvm::DenseMap<Term, APInt> memo;
};

std::optional<APInt> Evaluation::value(Term t) {
  auto it = memo.find(t);
  if (it != memo.end())
    return it->second;
  auto result = compute(t);
  if (result)
    memo.try_emplace(t, *result);
  return result;
}

std::optional<APInt> Evaluation::loadFrom(Term memory,
                                          llvm::ArrayRef<APInt> key) {
  // Walk the store chain, newest first. The last write to a cell wins.
  while (memory->op == TermOp::Store) {
    bool same = true;
    for (unsigned i = 0; i < key.size(); ++i) {
      auto index = value(memory->args[i + 2]);
      if (!index)
        return std::nullopt;
      same = same && *index == key[i];
    }
    if (same)
      return value(memory->args[1]);
    memory = memory->args[0];
  }
  const RootData &root = config.memory[memory->index];
  uint64_t offset = 0;
  for (unsigned i = 0; i < key.size(); ++i) {
    if (key[i].uge(root.shape[i]))
      return fail(Failure::OutOfRange);
    offset = offset * root.shape[i] + key[i].getZExtValue();
  }
  const auto &cell = root.cells[offset];
  if (!cell)
    return fail(Failure::Uninitialized);
  return *cell;
}

std::optional<APInt> Evaluation::compute(Term t) {
  auto arg = [&](unsigned i) { return value(t->args[i]); };
  switch (t->op) {
  case TermOp::Const:
  case TermOp::FConst:
    return t->value;
  case TermOp::Param:
    return entry.parameters[t->index];
  case TermOp::State: {
    const auto &v = config.state[t->index];
    if (!v)
      return fail(Failure::UndefinedState);
    return *v;
  }
  case TermOp::Shadow:
    return APInt(1, config.shadow[t->index] ? 1 : 0);
  case TermOp::MemVar:
  case TermOp::Store:
    return fail(Failure::UndefinedState); // Memory has no scalar value.
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
    if (!a)
      return std::nullopt;
    return ~*a;
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
    return arg(c->isOne() ? 1 : 2); // Lazy: the other arm may be undefined.
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
  case TermOp::Load: {
    llvm::SmallVector<APInt, 3> key;
    for (unsigned i = 1; i < t->args.size(); ++i) {
      auto index = arg(i);
      if (!index)
        return std::nullopt;
      key.push_back(*index);
    }
    return loadFrom(t->args[0], key);
  }
  case TermOp::FAdd: case TermOp::FMul: {
    auto a = arg(0), b = arg(1);
    if (!a || !b)
      return std::nullopt;
    return evalFloatBinary(t->op, *a, *b);
  }
  case TermOp::FIsNaN: case TermOp::FIsInf: case TermOp::FIsZero: {
    auto a = arg(0);
    if (!a)
      return std::nullopt;
    return APInt(1, evalFloatClass(t->op, *a) ? 1 : 0);
  }
  }
  return fail(Failure::UndefinedState);
}

const char *describe(Failure failure) {
  switch (failure) {
  case Failure::UndefinedState: return "read of an undefined state variable";
  case Failure::Uninitialized: return "read of uninitialized memory";
  case Failure::OutOfRange: return "read outside the root";
  case Failure::None: break;
  }
  return "evaluation failed";
}

uint64_t rowMajor(const RootData &root, llvm::ArrayRef<APInt> key) {
  uint64_t offset = 0;
  for (unsigned i = 0; i < key.size(); ++i)
    offset = offset * root.shape[i] + key[i].getZExtValue();
  return offset;
}

std::string hex(const APInt &value) {
  llvm::SmallString<32> text;
  value.toStringUnsigned(text, 16);
  return text.str().str();
}

std::string configurationKey(const Configuration &c) {
  std::string key = std::to_string(c.location);
  for (const auto &v : c.state)
    key += "," + (v ? hex(*v) : std::string("?"));
  for (bool bit : c.shadow)
    key += bit ? "1" : "0";
  for (const RootData &root : c.memory)
    for (const auto &cell : root.cells)
      key += ";" + (cell ? hex(*cell) : std::string("?"));
  return key;
}

bool usesPermissiveFloat(const SemanticProgram &program) {
  llvm::DenseSet<Term> seen;
  std::function<bool(Term)> visit = [&](Term t) {
    if (!t || !seen.insert(t).second)
      return false;
    if ((t->op == TermOp::FAdd || t->op == TermOp::FMul) &&
        (t->aux & (FlagReassoc | FlagContract | FlagARCFP | FlagAFN)))
      return true;
    for (Term a : t->args)
      if (visit(a))
        return true;
    return false;
  };
  for (const Transition &t : program.transitions) {
    for (const auto &[id, term] : t.assignments)
      if (visit(term)) return true;
    for (const auto &[id, term] : t.memoryUpdates)
      if (visit(term)) return true;
    for (Term term : t.returns)
      if (visit(term)) return true;
    if (visit(t.guard)) return true;
  }
  return false;
}

} // namespace

namespace mlir::generalized_gemm {

const char *stringifyRunStatus(RunStatus status) {
  switch (status) {
  case RunStatus::Returned: return "RETURNED";
  case RunStatus::UndefinedBehavior: return "UB";
  case RunStatus::Underspecified: return "UNDERSPECIFIED";
  case RunStatus::ModelLimit: return "MODEL_LIMIT";
  case RunStatus::OutOfFuel: return "OUT_OF_FUEL";
  case RunStatus::Diverged: return "DIVERGED";
  case RunStatus::InvalidEntry: return "INVALID_ENTRY";
  }
  return "INVALID_ENTRY";
}

std::optional<Configuration>
initialConfiguration(const SemanticProgram &program, const EntryState &entry,
                     const RunLimits &limits, std::string &error) {
  if (!program.isSupported()) {
    error = "the program is unsupported";
    return std::nullopt;
  }
  if (entry.parameters.size() != program.parameters.size()) {
    error = "wrong number of parameters";
    return std::nullopt;
  }
  for (const Parameter &p : program.parameters)
    if (entry.parameters[p.id].getBitWidth() != p.sort.width) {
      error = "parameter width mismatch";
      return std::nullopt;
    }
  if (entry.memory.size() != program.roots.size()) {
    error = "wrong number of roots";
    return std::nullopt;
  }
  uint64_t total = 0;
  for (const MemoryRoot &root : program.roots) {
    const RootData &data = entry.memory[root.id];
    if (data.shape.size() != root.extents.size()) {
      error = "root rank mismatch";
      return std::nullopt;
    }
    uint64_t cells = 1;
    for (unsigned d = 0; d < data.shape.size(); ++d) {
      if (root.extents[d] >= 0) {
        if ((uint64_t)root.extents[d] != data.shape[d]) {
          error = "root shape does not match its type";
          return std::nullopt;
        }
      } else {
        const APInt &dim = entry.parameters[root.dimensionParameter[d]];
        if (dim.isNegative() || dim.getZExtValue() != data.shape[d]) {
          error = "root shape does not match its dimension parameter";
          return std::nullopt;
        }
      }
      if (data.shape[d] && cells > limits.maxCells / data.shape[d]) {
        error = "memory exceeds the cell limit";
        return std::nullopt;
      }
      cells *= data.shape[d];
    }
    if (data.cells.size() != cells) {
      error = "root cell count does not match its shape";
      return std::nullopt;
    }
    total += cells;
    if (total > limits.maxCells) {
      error = "memory exceeds the cell limit";
      return std::nullopt;
    }
    for (const auto &cell : data.cells) {
      if (cell && cell->getBitWidth() != root.sort.width) {
        error = "cell width mismatch";
        return std::nullopt;
      }
      if (!cell && root.origin == MemoryRoot::FunctionArgument) {
        error = "a function argument root needs every cell";
        return std::nullopt;
      }
      // Contents here would turn an undefined read into a defined one.
      if (cell && root.origin == MemoryRoot::FreshAllocation) {
        error = "a fresh allocation starts uninitialized";
        return std::nullopt;
      }
    }
  }
  Configuration config;
  config.location = program.entryLocation;
  config.state.resize(program.state.size());
  config.shadow.assign(program.state.size(), false);
  config.memory = entry.memory;
  return config;
}

StepOutcome stepConfiguration(const SemanticProgram &program,
                              const EntryState &entry, Configuration &config,
                              std::vector<APInt> *returns,
                              std::vector<bool> *returnPoison) {
  StepOutcome outcome;
  auto stop = [&](RunStatus status, std::string reason) {
    outcome.stopped = status;
    outcome.reason = std::move(reason);
    return outcome;
  };
  llvm::SmallVector<const Transition *, 4> outgoing;
  for (const Transition &t : program.transitions)
    if (t.from == config.location)
      outgoing.push_back(&t);
  if (outgoing.empty())
    return stop(RunStatus::ModelLimit, "no transition from this location");

  Evaluation evaluation(entry, config);
  auto failed = [&]() {
    return stop(RunStatus::Underspecified, describe(evaluation.failure));
  };

  // Conditions are per block and in operation order: the first true one wins.
  for (const SideCondition &condition : outgoing.front()->sideConditions) {
    auto violated = evaluation.value(condition.violated);
    if (!violated)
      return failed();
    if (violated->isOne())
      switch (condition.kind) {
      case SideKind::UndefinedBehavior:
        return stop(RunStatus::UndefinedBehavior, condition.reason);
      case SideKind::Underspecified:
        return stop(RunStatus::Underspecified, condition.reason);
      case SideKind::ModelLimit:
        return stop(RunStatus::ModelLimit, condition.reason);
      }
  }

  const Transition *taken = nullptr;
  for (const Transition *t : outgoing) {
    auto guard = evaluation.value(t->guard);
    if (!guard)
      return failed();
    if (guard->isOne()) {
      if (taken)
        return stop(RunStatus::ModelLimit, "guards overlap");
      taken = t;
      outcome.transition = t - program.transitions.begin();
    }
  }
  if (!taken)
    return stop(RunStatus::ModelLimit, "no guard holds");

  struct Write {
    unsigned root;
    uint64_t offset;
    APInt value;
  };
  std::vector<Write> writes;
  auto indices = [&](llvm::ArrayRef<Term> terms, std::vector<APInt> &out) {
    for (Term index : terms) {
      auto v = evaluation.value(index);
      if (!v)
        return false;
      out.push_back(*v);
    }
    return true;
  };

  for (unsigned e = 0; e < taken->effects.size(); ++e) {
    const MemoryEffect &effect = taken->effects[e];
    std::vector<APInt> key;
    if (!indices(effect.index, key))
      return failed();
    auto value = evaluation.value(effect.value);
    if (!value)
      return failed();
    if (effect.kind == MemoryEffect::Write)
      writes.push_back({effect.root, rowMajor(config.memory[effect.root], key),
                        *value});
    for (const MacSite &site : program.sites) {
      if (site.location != config.location || site.effectIndex != e)
        continue;
      MacEvent event;
      event.site = site.id;
      for (unsigned id : site.coordinates) {
        if (!config.state[id])
          return stop(RunStatus::Underspecified,
                      "read of an undefined state variable");
        event.coordinates.push_back(*config.state[id]);
      }
      if (!indices(site.a.index, event.f) || !indices(site.b.index, event.g))
        return failed();
      if (site.output && !indices(site.output->index, event.h))
        return failed();
      if (site.outputRead && !indices(site.outputRead->index, event.hRead))
        return failed();
      auto a = evaluation.value(site.aValue), b = evaluation.value(site.bValue),
           update = evaluation.value(site.updateValue);
      if (!a || !b || !update)
        return failed();
      event.aValue = *a;
      event.bValue = *b;
      event.updateValue = *update;
      if (site.accumulatedValue) {
        auto c = evaluation.value(site.accumulatedValue);
        if (!c)
          return failed();
        event.accumulatedValue = *c;
      }
      outcome.events.push_back(std::move(event));
    }
  }

  // Every right-hand side reads the entry configuration (invariant I5).
  std::vector<std::pair<unsigned, APInt>> assigned;
  std::vector<std::pair<unsigned, bool>> shadowed;
  for (const auto &[id, term] : taken->assignments) {
    auto v = evaluation.value(term);
    if (!v)
      return failed();
    assigned.push_back({id, *v});
  }
  for (const auto &[id, term] : taken->shadowAssignments) {
    auto v = evaluation.value(term);
    if (!v)
      return failed();
    shadowed.push_back({id, v->isOne()});
  }
  std::vector<APInt> results;
  std::vector<bool> resultPoison;
  for (unsigned i = 0; i < taken->returns.size(); ++i) {
    auto v = evaluation.value(taken->returns[i]);
    auto p = evaluation.value(taken->returnPoison[i]);
    if (!v || !p)
      return failed();
    results.push_back(*v);
    resultPoison.push_back(p->isOne());
  }

  for (auto &[id, value] : assigned)
    config.state[id] = value;
  for (auto &[id, poison] : shadowed)
    config.shadow[id] = poison;
  for (Write &write : writes)
    config.memory[write.root].cells[write.offset] = write.value;
  config.location = taken->to;
  if (returns)
    *returns = std::move(results);
  if (returnPoison)
    *returnPoison = std::move(resultPoison);
  return outcome;
}

RunResult runConcrete(const SemanticProgram &program, const EntryState &entry,
                      const RunLimits &limits) {
  RunResult result;
  auto config = initialConfiguration(program, entry, limits, result.reason);
  if (!config) {
    result.status = RunStatus::InvalidEntry;
    return result;
  }
  result.strictRefinementOfPermissiveFloat = usesPermissiveFloat(program);
  std::set<std::string> seen;
  while (true) {
    if (config->location == program.exitLocation()) {
      result.status = RunStatus::Returned;
      break;
    }
    if (result.steps >= limits.fuel) {
      result.status = RunStatus::OutOfFuel;
      result.reason = "fuel exhausted: this proves nothing";
      break;
    }
    if (seen.size() < limits.maxRecordedConfigurations &&
        !seen.insert(configurationKey(*config)).second) {
      result.status = RunStatus::Diverged;
      result.reason = "a complete configuration repeated";
      break;
    }
    StepOutcome step = stepConfiguration(program, entry, *config,
                                         &result.returns, &result.returnPoison);
    if (step.stopped) {
      result.status = *step.stopped;
      result.reason = step.reason;
      break;
    }
    ++result.steps;
    for (MacEvent &event : step.events) {
      event.step = result.steps - 1;
      result.events.push_back(std::move(event));
    }
  }
  result.memory = config->memory;
  return result;
}

DomainEvidence domainEvidenceFromRun(const SemanticProgram &program,
                                     const EntryState &entry,
                                     const RunResult &run) {
  DomainEvidence evidence;
  if (run.status != RunStatus::Returned) {
    evidence.provenance = std::string("the run ended ") +
                          stringifyRunStatus(run.status) +
                          ", so it does not enumerate D";
    return evidence;
  }
  if (!program.controlOblivious || !program.addressOblivious) {
    // The trace depends on memory contents, so a parameter point does not
    // determine it and a parameter scope would overstate the evidence.
    evidence.provenance = "the trace depends on memory contents";
    return evidence;
  }
  evidence.form = DomainForm::ExactEnumerated;
  evidence.provenance = "complete run of " + program.fingerprint;
  evidence.scope.allParameters = false;
  for (const Parameter &p : program.parameters)
    evidence.scope.ranges.push_back(
        {p.id, entry.parameters[p.id], entry.parameters[p.id]});
  return evidence;
}

std::optional<APInt> evaluateTerm(const SemanticProgram &program,
                                  const EntryState &entry,
                                  const Configuration &config, Term term) {
  (void)program;
  Evaluation evaluation(entry, config);
  return evaluation.value(term);
}

} // namespace mlir::generalized_gemm
