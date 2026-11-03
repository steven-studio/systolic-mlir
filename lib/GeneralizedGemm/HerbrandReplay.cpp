//===- HerbrandReplay.cpp -------------------------------------------------===//
#include "GeneralizedGemm/HerbrandReplay.h"

#include "GeneralizedGemm/TerminationObligations.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/StringMap.h"

#include <algorithm>
#include <functional>
#include <optional>

using llvm::APInt;

namespace mlir::generalized_gemm {

const char *stringifyReplayOutcome(ReplayOutcome outcome) {
  switch (outcome) {
  case ReplayOutcome::Equal: return "EQUAL";
  case ReplayOutcome::NoCompletingRun: return "NO_COMPLETING_RUN";
  case ReplayOutcome::Mismatch: return "MISMATCH";
  case ReplayOutcome::Unobservable: return "UNOBSERVABLE";
  case ReplayOutcome::Limit: return "LIMIT";
  case ReplayOutcome::Refused: return "REFUSED";
  }
  return "REFUSED";
}

namespace {

using NodeId = uint32_t;
constexpr NodeId None = 0;

struct LimitReached {
  std::string reason;
};

struct Node {
  bool constant = false;
  APInt value; // For a constant: the exact bits, at the exact width.
};

/// Nodes are identified by their key, so equal keys are one node.
class NodeTable {
public:
  explicit NodeTable(uint64_t limit) : limit(limit) { nodes.emplace_back(); }

  NodeId constant(const APInt &value, bool isFloat) {
    // Poison terms compute at widths past 64 bits, so every word is part of the key.
    std::vector<uint64_t> key{1, (uint64_t)isFloat, value.getBitWidth()};
    key.insert(key.end(), value.getRawData(), value.getRawData() + value.getNumWords());
    return intern(key, [&](Node &n) {
      n.constant = true;
      n.value = value;
    });
  }
  NodeId leaf(uint64_t kind, uint64_t a, uint64_t b) {
    return intern({2, kind, a, b}, [](Node &) {});
  }
  /// An operation on values, with nothing simplified.
  NodeId apply(TermOp op, unsigned aux, Sort sort, llvm::ArrayRef<NodeId> args) {
    std::vector<uint64_t> key{3, (uint64_t)op, aux, (uint64_t)sort.kind, sort.width};
    key.insert(key.end(), args.begin(), args.end());
    return intern(key, [](Node &) {});
  }
  const Node &operator[](NodeId id) const { return nodes[id]; }
  bool isConstant(NodeId id) const { return nodes[id].constant; }
  size_t size() const { return nodes.size() - 1; }

private:
  NodeId intern(const std::vector<uint64_t> &key, const std::function<void(Node &)> &fill) {
    llvm::StringRef bytes(reinterpret_cast<const char *>(key.data()),
                          key.size() * sizeof(uint64_t));
    auto found = ids.find(bytes);
    if (found != ids.end())
      return found->second;
    if (nodes.size() > limit)
      throw LimitReached{"the node limit"};
    NodeId id = nodes.size();
    nodes.emplace_back();
    fill(nodes.back());
    ids[bytes] = id;
    return id;
  }
  uint64_t limit;
  std::vector<Node> nodes;
  llvm::StringMap<NodeId> ids;
};

enum class Failure { None, Stop, Refuse };

class Machine {
public:
  Machine(const SemanticProgram &program, const SiteGroup &group,
          const std::vector<APInt> &parameters, const HerbrandLimits &limits,
          ReplayResult &out)
      : program(program), group(group), parameters(parameters), limits(limits), out(out),
        table(limits.maxNodes) {}

  void run();

private:
  enum LeafKind : uint64_t { InitialCell, FloatParameter };

  bool layout();
  std::optional<NodeId> value(Term term);
  std::optional<NodeId> compute(Term term);
  std::optional<NodeId> load(Term term);
  std::optional<uint64_t> offset(unsigned root, llvm::ArrayRef<NodeId> key);
  std::optional<std::vector<NodeId>> tuple(llvm::ArrayRef<Term> terms);
  std::optional<NodeId> initial(unsigned root, uint64_t cell);
  NodeId boolean(bool value) { return table.constant(APInt(1, value ? 1 : 0), false); }
  void fail(Failure kind, std::string reason) {
    if (failure == Failure::None) {
      failure = kind;
      failureReason = std::move(reason);
    }
  }
  void finish(ReplayOutcome outcome, std::string reason) {
    out.outcome = outcome;
    out.reason = std::move(reason);
  }
  /// A failed evaluation of the block: a stop at every memory, or no statement.
  void stopped() {
    if (failure == Failure::Stop)
      finish(ReplayOutcome::NoCompletingRun, failureReason);
    else
      finish(ReplayOutcome::Refused, failureReason);
  }
  bool step(bool &done);
  bool event(const MacSite &site, Term writtenTerm, CellKey &cell);
  void foreignWrite(const CellKey &cell, NodeId value);
  bool exitValues(const Transition &taken);

  const SemanticProgram &program;
  const SiteGroup &group;
  const std::vector<APInt> &parameters;
  const HerbrandLimits &limits;
  ReplayResult &out;
  NodeTable table;

  unsigned location = 0;
  std::vector<NodeId> state, shadow;
  std::vector<std::vector<uint64_t>> shapes;
  std::vector<std::vector<NodeId>> memory;
  std::map<unsigned, std::vector<const Transition *>> outgoing;
  std::map<std::pair<unsigned, unsigned>, unsigned> groupEffects; // (location, effect).
  llvm::DenseMap<Term, NodeId> memo;
  Failure failure = Failure::None;
  std::string failureReason;

  std::map<CellKey, NodeId> replay;       // The replay's current value of a cell.
  std::map<CellKey, uint64_t> folds;      // Contributions per cell so far.
  std::set<CellKey> foreign;              // Written outside the group.
  std::string pending;                    // The first mismatch.
};

bool Machine::layout() {
  for (const MemoryRoot &root : program.roots) {
    std::vector<uint64_t> shape;
    uint64_t cells = 1;
    for (unsigned d = 0; d < root.extents.size(); ++d) {
      int64_t extent = root.extents[d];
      if (extent < 0) {
        const APInt &dim = parameters[root.dimensionParameter[d]];
        if (dim.isNegative()) {
          finish(ReplayOutcome::Refused, "a negative extent is outside the domain");
          return false;
        }
        extent = dim.getZExtValue();
      }
      shape.push_back(extent);
      if (extent && cells > limits.maxCells / extent) {
        finish(ReplayOutcome::Limit, "the cell limit");
        return false;
      }
      cells *= extent;
    }
    shapes.push_back(std::move(shape));
    // A fresh allocation has no value: a read of it stops, as in G1.
    std::vector<NodeId> contents(cells, None);
    if (root.origin == MemoryRoot::FunctionArgument)
      for (uint64_t c = 0; c < cells; ++c)
        contents[c] = table.leaf(InitialCell, root.id, c);
    memory.push_back(std::move(contents));
  }
  return true;
}

std::optional<uint64_t> Machine::offset(unsigned root, llvm::ArrayRef<NodeId> key) {
  const std::vector<uint64_t> &shape = shapes[root];
  if (key.size() != shape.size()) {
    fail(Failure::Refuse, "an index of the wrong rank");
    return std::nullopt;
  }
  uint64_t at = 0;
  for (unsigned i = 0; i < key.size(); ++i) {
    // Obliviousness says this cannot happen. Should it, nothing is known.
    if (!table.isConstant(key[i])) {
      fail(Failure::Refuse, "an address depends on memory");
      return std::nullopt;
    }
    const APInt &index = table[key[i]].value;
    if (index.uge(shape[i])) {
      fail(Failure::Stop, "an index out of range");
      return std::nullopt;
    }
    at = at * shape[i] + index.getZExtValue();
  }
  return at;
}

std::optional<std::vector<NodeId>> Machine::tuple(llvm::ArrayRef<Term> terms) {
  std::vector<NodeId> nodes;
  for (Term term : terms) {
    auto node = value(term);
    if (!node)
      return std::nullopt;
    nodes.push_back(*node);
  }
  return nodes;
}

std::optional<NodeId> Machine::load(Term term) {
  auto key = tuple(llvm::ArrayRef<Term>(term->args).drop_front());
  if (!key)
    return std::nullopt;
  Term chain = term->args[0];
  while (chain->op == TermOp::Store) {
    auto index = tuple(llvm::ArrayRef<Term>(chain->args).drop_front(2));
    if (!index)
      return std::nullopt;
    for (NodeId id : llvm::concat<const NodeId>(*key, *index))
      if (!table.isConstant(id)) {
        fail(Failure::Refuse, "an address depends on memory");
        return std::nullopt;
      }
    bool same = true;
    for (unsigned i = 0; i < key->size(); ++i)
      same = same && table[(*key)[i]].value == table[(*index)[i]].value;
    if (same)
      return value(chain->args[1]);
    chain = chain->args[0];
  }
  if (chain->op != TermOp::MemVar) {
    fail(Failure::Refuse, "a memory term of an unknown form");
    return std::nullopt;
  }
  unsigned root = chain->sort.root;
  auto at = offset(root, *key);
  if (!at)
    return std::nullopt;
  NodeId cell = memory[root][*at];
  if (cell == None)
    fail(Failure::Stop, "a read of an uninitialized cell");
  return cell == None ? std::nullopt : std::optional<NodeId>(cell);
}

std::optional<NodeId> Machine::value(Term term) {
  auto found = memo.find(term);
  if (found != memo.end())
    return found->second;
  auto computed = compute(term);
  if (computed)
    memo[term] = *computed;
  return computed;
}

std::optional<NodeId> Machine::compute(Term t) {
  bool isFloat = t->sort.kind == Sort::Float;
  auto constant = [&](const APInt &bits) { return table.constant(bits, isFloat); };
  switch (t->op) {
  case TermOp::Const:
  case TermOp::FConst:
    return constant(t->value);
  case TermOp::Param:
    if (t->sort.kind == Sort::BV)
      return constant(parameters[t->index]);
    // A float scalar is no part of a point: it stays an unknown.
    return table.leaf(FloatParameter, t->index, 0);
  case TermOp::State:
    if (state[t->index] == None) {
      fail(Failure::Stop, "a read of an undefined state variable");
      return std::nullopt;
    }
    return state[t->index];
  case TermOp::Shadow:
    return shadow[t->index];
  case TermOp::Load:
    return load(t);
  case TermOp::MemVar:
  case TermOp::Store:
    fail(Failure::Refuse, "a memory term has no scalar value");
    return std::nullopt;
  case TermOp::Ite: {
    auto c = value(t->args[0]);
    if (!c)
      return std::nullopt;
    // Lazy, as in G1: the arm not taken may have no value.
    if (table.isConstant(*c))
      return value(t->args[table[*c].value.isOne() ? 1 : 2]);
    auto a = value(t->args[1]), b = value(t->args[2]);
    if (!a || !b) {
      failure = Failure::None;
      fail(Failure::Refuse, "an arm of a memory-dependent choice has no value");
      return std::nullopt;
    }
    return table.apply(TermOp::Ite, t->aux, t->sort, {*c, *a, *b});
  }
  default:
    break;
  }
  llvm::SmallVector<NodeId, 3> args;
  bool concrete = true;
  for (Term arg : t->args) {
    auto node = value(arg);
    if (!node)
      return std::nullopt;
    args.push_back(*node);
    concrete = concrete && table.isConstant(*node);
  }
  if (!concrete)
    return table.apply(t->op, t->aux, t->sort, args);
  auto bits = [&](unsigned i) -> const APInt & { return table[args[i]].value; };
  switch (t->op) {
  case TermOp::Add: case TermOp::Sub: case TermOp::Mul: case TermOp::SDiv: case TermOp::UDiv:
  case TermOp::SRem: case TermOp::URem: case TermOp::Shl: case TermOp::LShr: case TermOp::AShr:
  case TermOp::And: case TermOp::Or: case TermOp::Xor:
    return constant(evalIntegerBinary(t->op, bits(0), bits(1)));
  case TermOp::Not:
    return constant(~bits(0));
  case TermOp::ICmp:
    return boolean(evalPredicate((Predicate)t->aux, bits(0), bits(1)));
  case TermOp::ZExt:
    return constant(bits(0).zext(t->sort.width));
  case TermOp::SExt:
    return constant(bits(0).sext(t->sort.width));
  case TermOp::Trunc:
    return constant(bits(0).trunc(t->sort.width));
  case TermOp::FAdd: case TermOp::FMul:
    return constant(evalFloatBinary(t->op, bits(0), bits(1)));
  case TermOp::FIsNaN: case TermOp::FIsInf: case TermOp::FIsZero:
    return boolean(evalFloatClass(t->op, bits(0)));
  default:
    fail(Failure::Refuse, "an operation this replay does not know");
    return std::nullopt;
  }
}

std::optional<NodeId> Machine::initial(unsigned root, uint64_t cell) {
  if (program.roots[root].origin != MemoryRoot::FunctionArgument)
    return std::nullopt;
  return table.leaf(InitialCell, root, cell);
}

/// One site event: the replay's step against what the program wrote.
bool Machine::event(const MacSite &site, Term writtenTerm, CellKey &cell) {
  auto refuse = [&](std::string why) {
    finish(ReplayOutcome::Refused, std::move(why));
    return false;
  };
  // The run goes on after a mismatch: a witness needs the whole prefix.
  auto mismatch = [&](std::string why) {
    if (pending.empty())
      pending = std::move(why);
  };
  Term update = site.updateValue;
  bool isFloat = site.reduction == ReductionOp::FAdd;
  TermOp add = isFloat ? TermOp::FAdd : TermOp::Add, mul = isFloat ? TermOp::FMul : TermOp::Mul;
  if (update->op != add || update->aux != site.reductionFlags || update->args.size() != 2)
    return refuse("the update is not the recorded addition");
  bool first = update->args[0] == site.accumulatedValue;
  Term product = update->args[first ? 1 : 0];
  if ((!first && update->args[1] != site.accumulatedValue) || product->op != mul ||
      product->aux != site.multiplyFlags || product->args.size() != 2 ||
      product->args[0] != site.aValue || product->args[1] != site.bValue)
    return refuse("the update is not the recorded product");

  auto h = tuple(site.output->index), read = tuple(site.outputRead->index),
       f = tuple(site.a.index), g = tuple(site.b.index);
  if (!h || !read || !f || !g) {
    stopped();
    return false;
  }
  auto cellH = offset(site.output->root, *h), cellRead = offset(site.outputRead->root, *read),
       cellA = offset(site.a.root, *f), cellB = offset(site.b.root, *g);
  if (!cellH || !cellRead || !cellA || !cellB) {
    stopped();
    return false;
  }
  if (site.outputRead->root != site.output->root || *cellRead != *cellH)
    mismatch("the update reads a cell it does not write");
  auto a = value(site.aValue), b = value(site.bValue), written = value(writtenTerm);
  if (!a || !b || !written) {
    stopped();
    return false;
  }

  cell = {site.output->root, *cellH};
  auto a0 = initial(site.a.root, *cellA), b0 = initial(site.b.root, *cellB);
  auto current = replay.find(cell);
  std::optional<NodeId> accumulated =
      current != replay.end() ? std::optional<NodeId>(current->second)
                              : initial(cell.first, cell.second);
  NodeId next = *written;
  if (!a0 || !b0) {
    mismatch("an operand is in a fresh allocation, so it has no initial value");
  } else if (!accumulated) {
    mismatch("the output cell has no value before its first event");
  } else {
    // The operands the program really read must be the initial ones.
    if (*a != *a0 || *b != *b0)
      mismatch("an operand read is not the initial value of its cell");
    NodeId contribution = table.apply(mul, site.multiplyFlags, product->sort, {*a0, *b0});
    next = first ? table.apply(add, site.reductionFlags, update->sort, {*accumulated, contribution})
                 : table.apply(add, site.reductionFlags, update->sort, {contribution, *accumulated});
    // Each intermediate value, not only the last.
    if (next != *written)
      mismatch("event " + std::to_string(out.facts.events) +
               " writes a value that is not the replay's next value");
  }
  replay[cell] = next;
  out.facts.maxContributions = std::max(out.facts.maxContributions, ++folds[cell]);
  ++out.facts.events;
  return true;
}

/// In effect order, with the group's events: a constant and an event in one block
/// are still one before the other.
void Machine::foreignWrite(const CellKey &cell, NodeId value) {
  foreign.insert(cell);
  // A constant set before a cell's first event starts its fold. Any other value
  // does not, so the fold starts from the cell's initial value and the comparison
  // at the next event shows the difference. After the first event the replay does
  // not change, and the comparison at the end shows it.
  if (folds.count(cell))
    return;
  if (table.isConstant(value)) {
    replay[cell] = value;
    out.prefix.insert_or_assign(cell, table[value].value);
  } else {
    replay.erase(cell);
    out.prefix.erase(cell);
  }
}

bool Machine::exitValues(const Transition &taken) {
  for (Term term : llvm::concat<const Term>(taken.returns, taken.returnPoison)) {
    auto node = value(term);
    if (!node) {
      stopped();
      return false;
    }
    if (!table.isConstant(*node)) {
      finish(ReplayOutcome::Unobservable, "a return value depends on memory");
      return false;
    }
  }
  return true;
}

bool Machine::step(bool &done) {
  auto found = outgoing.find(location);
  if (found == outgoing.end()) {
    finish(ReplayOutcome::NoCompletingRun, "no transition from this location");
    return false;
  }
  memo.clear();
  const std::vector<const Transition *> &leaving = found->second;

  for (const SideCondition &condition : leaving.front()->sideConditions) {
    auto violated = value(condition.violated);
    if (!violated) {
      stopped();
      return false;
    }
    if (table.isConstant(*violated)) {
      if (table[*violated].value.isOne()) {
        finish(ReplayOutcome::NoCompletingRun, condition.reason);
        return false;
      }
    } else {
      out.conditionalStops.insert({condition.kind, condition.reason});
    }
  }

  const Transition *taken = nullptr;
  for (const Transition *t : leaving) {
    auto guard = value(t->guard);
    if (!guard) {
      stopped();
      return false;
    }
    if (!table.isConstant(*guard)) {
      finish(ReplayOutcome::Refused, "a guard depends on memory");
      return false;
    }
    if (table[*guard].value.isOne()) {
      if (taken) {
        finish(ReplayOutcome::NoCompletingRun, "guards overlap");
        return false;
      }
      taken = t;
    }
  }
  if (!taken) {
    finish(ReplayOutcome::NoCompletingRun, "no guard holds");
    return false;
  }

  struct Write {
    CellKey cell;
    NodeId value;
  };
  std::vector<Write> writes;
  for (unsigned e = 0; e < taken->effects.size(); ++e) {
    const MemoryEffect &effect = taken->effects[e];
    auto key = tuple(effect.index);
    if (!key) {
      stopped();
      return false;
    }
    auto at = offset(effect.root, *key);
    auto written = value(effect.value);
    if (!at || !written) {
      stopped();
      return false;
    }
    if (effect.kind != MemoryEffect::Write)
      continue;
    auto site = groupEffects.find({location, e});
    if (site == groupEffects.end()) {
      writes.push_back({{effect.root, *at}, *written});
      foreignWrite({effect.root, *at}, *written);
      continue;
    }
    CellKey cell;
    const MacSite &owner = program.sites[site->second];
    if (owner.output->root != effect.root) {
      finish(ReplayOutcome::Refused, "a site's store is on another root");
      return false;
    }
    if (!event(owner, effect.value, cell))
      return false;
    if (cell.second != *at) {
      finish(ReplayOutcome::Refused, "a site's store is at another cell");
      return false;
    }
    writes.push_back({cell, *written});
  }

  std::vector<std::pair<unsigned, NodeId>> nextState, nextShadow;
  for (const auto &[id, term] : taken->assignments) {
    auto node = value(term);
    if (!node) {
      stopped();
      return false;
    }
    nextState.push_back({id, *node});
  }
  for (const auto &[id, term] : taken->shadowAssignments) {
    auto node = value(term);
    if (!node) {
      stopped();
      return false;
    }
    nextShadow.push_back({id, *node});
  }
  if (taken->to == program.exitLocation() && !exitValues(*taken))
    return false;

  for (auto &[id, node] : nextState)
    state[id] = node;
  for (auto &[id, node] : nextShadow)
    shadow[id] = node;
  for (const Write &write : writes)
    memory[write.cell.first][write.cell.second] = write.value;
  location = taken->to;
  done = location == program.exitLocation();
  return true;
}

void Machine::run() {
  if (parameters.size() != program.parameters.size()) {
    finish(ReplayOutcome::Refused, "wrong number of parameters");
    return;
  }
  for (const Parameter &p : program.parameters)
    if (parameters[p.id].getBitWidth() != p.sort.width) {
      finish(ReplayOutcome::Refused, "a parameter of the wrong width");
      return;
    }
  out.facts.parameters = parameters;
  if (!layout())
    return;
  for (const Transition &t : program.transitions)
    outgoing[t.from].push_back(&t);
  for (unsigned id : group.sites) {
    const MacSite &site = program.sites[id];
    if (!site.output || !site.outputRead || !site.accumulatedValue) {
      finish(ReplayOutcome::Refused, "a site with no output in memory");
      return;
    }
    if (!groupEffects.try_emplace({site.location, site.effectIndex}, id).second) {
      finish(ReplayOutcome::Refused, "two sites share a store");
      return;
    }
    for (const Transition *t : outgoing[site.location])
      if (site.effectIndex >= t->effects.size() ||
          t->effects[site.effectIndex].kind != MemoryEffect::Write) {
        finish(ReplayOutcome::Refused, "a site's effect is not a store");
        return;
      }
  }
  location = program.entryLocation;
  state.assign(program.state.size(), None);
  shadow.assign(program.state.size(), boolean(false));

  bool done = location == program.exitLocation();
  while (!done) {
    if (out.facts.steps >= limits.maxSteps) {
      finish(ReplayOutcome::Limit, "the step limit");
      return;
    }
    if (!step(done))
      return;
    ++out.facts.steps;
  }

  // The whole final contents of every cell the group wrote.
  for (const auto &[cell, count] : folds) {
    (void)count;
    if (memory[cell.first][cell.second] != replay[cell] && pending.empty())
      pending = "a cell's final value is not the replay's";
  }
  out.facts.outputCells = folds.size();
  for (auto it = out.prefix.begin(); it != out.prefix.end();)
    it = folds.count(it->first) ? std::next(it) : out.prefix.erase(it);
  out.facts.prefixCells = out.prefix.size();
  for (const CellKey &cell : foreign)
    out.facts.residualCells += !folds.count(cell);
  if (!pending.empty()) {
    finish(ReplayOutcome::Mismatch, pending);
    return;
  }
  finish(ReplayOutcome::Equal, "every event and final cell agrees");
}

} // namespace

ReplayResult replayGroup(const SemanticProgram &program, const SiteGroup &group,
                         const std::vector<APInt> &parameters, const HerbrandLimits &limits) {
  ReplayResult out;
  Machine machine(program, group, parameters, limits, out);
  try {
    machine.run();
  } catch (const LimitReached &limit) {
    out.outcome = ReplayOutcome::Limit;
    out.reason = limit.reason;
  }
  return out;
}

} // namespace mlir::generalized_gemm
