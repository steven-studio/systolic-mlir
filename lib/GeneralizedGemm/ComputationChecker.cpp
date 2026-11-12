//===- ComputationChecker.cpp ---------------------------------------------===//
#include "GeneralizedGemm/ComputationChecker.h"

#include "GeneralizedGemm/CoverageSimulation.h"
#include "GeneralizedGemm/SiteSets.h"
#include "GeneralizedGemm/SymbolicDomain.h"
#include "GeneralizedGemm/TerminationObligations.h"

#include "llvm/ADT/DenseSet.h"

#include <algorithm>
#include <functional>
#include <random>
#include <tuple>

using llvm::APInt;
using llvm::ArrayRef;
using llvm::DenseSet;

namespace mlir::generalized_gemm {

const char *stringifyComputationTier(ComputationTier tier) {
  switch (tier) {
  case ComputationTier::Proposed: return "PROPOSED";
  case ComputationTier::Point: return "POINT";
  case ComputationTier::ScopeEnumerated: return "SCOPE_ENUMERATED";
  case ComputationTier::Symbolic: return "SYMBOLIC";
  }
  return "PROPOSED";
}

const char *stringifyComputationStatus(ComputationStatus status) {
  switch (status) {
  case ComputationStatus::Verified: return "VERIFIED";
  case ComputationStatus::Refuted: return "REFUTED";
  case ComputationStatus::Unknown: return "UNKNOWN";
  }
  return "UNKNOWN";
}

const char *stringifyFoldForm(FoldForm form) {
  switch (form) {
  case FoldForm::ModularInteger: return "MODULAR_INTEGER";
  case FoldForm::OrderedFloat: return "ORDERED_FLOAT";
  }
  return "ORDERED_FLOAT";
}

const char *stringifySetFactStatus(SetFactStatus status) {
  switch (status) {
  case SetFactStatus::Proven: return "PROVEN";
  case SetFactStatus::Refuted: return "REFUTED";
  case SetFactStatus::Unknown: return "UNKNOWN";
  }
  return "UNKNOWN";
}

struct ComputationAuthority {
  static CheckedComputation make(ComputationTier tier, ComputationStatus status,
                                 std::string reason) {
    return CheckedComputation(tier, status, std::move(reason));
  }
  static void records(CheckedComputation &out, std::vector<ObligationRecord> list) {
    out.obligations = std::move(list);
  }
  static void contracts(CheckedComputation &out, std::vector<SiteContract> list, FoldForm fold) {
    out.siteContracts = std::move(list);
    out.foldForm = fold;
  }
  static void separations(CheckedComputation &out, std::vector<SeparationUse> list) {
    out.separationUses = std::move(list);
  }
  static void frame(CheckedComputation &out, std::vector<ResidualEffect> list) {
    out.residual = std::move(list);
  }
  static void stops(CheckedComputation &out, std::set<MemoryDependentStop> list) {
    out.stops = std::move(list);
  }
  static void facts(CheckedComputation &out, std::vector<ReplayFacts> list) {
    out.facts = std::move(list);
  }
  static void every(CheckedComputation &out, bool value) { out.every = value; }
  static void bind(CheckedComputation &out, std::string binding) {
    out.bound = std::move(binding);
  }
  static void witness(CheckedComputation &out, EntryState entry) {
    out.counterexample = std::move(entry);
  }
  static void failing(CheckedComputation &out, std::vector<APInt> point) {
    out.failing = std::move(point);
  }
  static void groupSet(CheckedComputation &out, std::vector<std::vector<unsigned>> listed,
                       SetFact order, SetFact shared, std::vector<GroupSetPoint> points) {
    out.listed = std::move(listed);
    out.order = std::move(order);
    out.shared = std::move(shared);
    out.setPoints = std::move(points);
  }
};

CheckedComputation CheckedComputation::unknown(std::string reason) {
  return CheckedComputation(ComputationTier::Proposed, ComputationStatus::Unknown,
                            std::move(reason));
}

namespace {

bool sameTerms(ArrayRef<Term> a, ArrayRef<Term> b) {
  return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
}

bool sameEffect(const MemoryEffect &a, const MemoryEffect &b) {
  return a.kind == b.kind && a.root == b.root && a.opIndex == b.opIndex && a.value == b.value &&
         sameTerms(a.index, b.index);
}

/// Control and addresses read neither memory nor a float parameter. The same gate
/// as the enumeration, since a symbolic claim passes no enumeration.
bool traceIsFixed(const SemanticProgram &program, std::string &why) {
  if (!program.controlOblivious || !program.addressOblivious) {
    why = "the trace depends on memory contents";
    return false;
  }
  for (const Transition &t : program.transitions) {
    bool floats = containsFloat(t.guard);
    for (const auto &[id, term] : t.assignments)
      floats = floats || (term->sort.kind == Sort::BV && containsFloat(term));
    for (const MemoryEffect &effect : t.effects)
      for (Term index : effect.index)
        floats = floats || containsFloat(index);
    if (floats) {
      why = "control or an address reads a float";
      return false;
    }
  }
  return true;
}

/// Every written root against every root the program touches, not only the group's:
/// a value can go through a third root and back into the output, and the model of
/// one array per root is wrong for all of them if any two overlap. Different ids say
/// nothing: only a fresh allocation is apart by construction, and everything else,
/// `llvm.noalias` included, is a promise someone must have authorized by name.
bool separationsFor(const SemanticProgram &program, const SiteGroup &group,
                    const Authorizations &authorized, std::vector<SeparationUse> &uses,
                    std::string &why) {
  std::set<unsigned> written, accessed{*group.output, group.first, group.second};
  for (const Transition &t : program.transitions)
    for (const MemoryEffect &effect : t.effects) {
      accessed.insert(effect.root);
      if (effect.kind == MemoryEffect::Write)
        written.insert(effect.root);
    }
  std::set<std::pair<unsigned, unsigned>> pairs;
  for (unsigned w : written)
    for (unsigned r : accessed)
      if (w != r)
        pairs.insert({std::min(w, r), std::max(w, r)});
  // The separation of each pair is the shared rule (SiteSets.h), so that a checker that
  // compares cells of two roots reads the same answer as this gate.
  for (auto [first, second] : pairs) {
    RootSeparation separation = rootSeparation(program, authorized, first, second);
    if (!separation.apart) {
      why = "not authorized: " + separation.name;
      return false;
    }
    SeparationUse use;
    use.first = first;
    use.second = second;
    use.assumption = separation.assumption;
    use.basis = separation.basis;
    use.name = separation.name;
    uses.push_back(std::move(use));
  }
  return true;
}

/// The sites of one group share their output root and so their kind of addition. The
/// union of several groups may not: one float site makes the whole claim ordered.
FoldForm foldOf(const SemanticProgram &program, const SiteGroup &group) {
  bool modular = llvm::all_of(group.sites, [&](unsigned id) {
    return program.sites[id].reduction == ReductionOp::Add;
  });
  return modular ? FoldForm::ModularInteger : FoldForm::OrderedFloat;
}

/// Whether a term's value can depend on memory, directly or through state.
bool dependsOnMemory(Term term, const std::vector<bool> &independent) {
  DenseSet<Term> seen;
  std::function<bool(Term)> visit = [&](Term t) {
    if (!seen.insert(t).second)
      return false;
    if (t->op == TermOp::Load || t->op == TermOp::MemVar)
      return true;
    if ((t->op == TermOp::State || t->op == TermOp::Shadow) && !independent[t->index])
      return true;
    return llvm::any_of(t->args, visit);
  };
  return visit(term);
}

std::set<MemoryDependentStop> memoryStopsOf(const SemanticProgram &program) {
  std::set<MemoryDependentStop> out;
  std::vector<bool> independent = memoryIndependentState(program);
  for (const Transition &t : program.transitions)
    for (const SideCondition &condition : t.sideConditions)
      if (dependsOnMemory(condition.violated, independent) || containsFloat(condition.violated))
        out.insert({condition.kind, condition.reason});
  return out;
}

/// The update of a site, read back from its terms.
struct UpdateShape {
  bool ok = false, accumulatorFirst = true;
};
UpdateShape updateShape(const MacSite &site) {
  UpdateShape out;
  Term update = site.updateValue;
  bool isFloat = site.reduction == ReductionOp::FAdd;
  TermOp add = isFloat ? TermOp::FAdd : TermOp::Add, mul = isFloat ? TermOp::FMul : TermOp::Mul;
  if (!update || update->op != add || update->aux != site.reductionFlags ||
      update->args.size() != 2 || !site.accumulatedValue)
    return out;
  out.accumulatorFirst = update->args[0] == site.accumulatedValue;
  if (!out.accumulatorFirst && update->args[1] != site.accumulatedValue)
    return out;
  Term product = update->args[out.accumulatorFirst ? 1 : 0];
  out.ok = product->op == mul && product->aux == site.multiplyFlags &&
           product->args.size() == 2 && product->args[0] == site.aValue &&
           product->args[1] == site.bValue;
  return out;
}

std::optional<std::vector<SiteContract>> contractsOf(const SemanticProgram &program,
                                                     const SiteGroup &group) {
  std::vector<SiteContract> out;
  for (unsigned id : group.sites) {
    const MacSite &site = program.sites[id];
    UpdateShape shape = updateShape(site);
    if (!shape.ok)
      return std::nullopt;
    out.push_back({id, site.location, site.effectIndex, site.reduction, site.reductionFlags,
                   site.multiplyFlags, shape.accumulatorFirst});
  }
  return out;
}

const Transition *firstFrom(const SemanticProgram &program, unsigned location) {
  for (const Transition &t : program.transitions)
    if (t.from == location)
      return &t;
  return nullptr;
}

/// The memory of `root` before effect `position`, from the listed writes alone.
Term memoryBefore(const SemanticProgram &program, const Transition &block, unsigned root,
                  unsigned position) {
  TermContext &terms = *program.terms;
  Term memory = terms.memVar(program.roots[root].sort);
  for (unsigned e = 0; e < position; ++e) {
    const MemoryEffect &effect = block.effects[e];
    if (effect.kind == MemoryEffect::Write && effect.root == root)
      memory = terms.store(memory, effect.index, effect.value);
  }
  return memory;
}

/// Where the block reads `value`, or nothing when no read of it has this shape.
std::optional<unsigned> readOf(const SemanticProgram &program, const Transition &block,
                               Term value, const MacAccess &access) {
  TermContext &terms = *program.terms;
  for (unsigned e = 0; e < block.effects.size(); ++e) {
    const MemoryEffect &effect = block.effects[e];
    if (effect.kind != MemoryEffect::Read || effect.value != value || effect.root != access.root ||
        !sameTerms(effect.index, access.index))
      continue;
    // The term must be that load of the memory the listed writes leave.
    if (terms.load(memoryBefore(program, block, access.root, e), access.index) == value)
      return e;
  }
  return std::nullopt;
}

/// `outputs`: the output roots of a union of groups (G5b-M3). One group's by default.
std::vector<ResidualEffect> frameOf(const SemanticProgram &program, const SiteGroup &group,
                                    const std::set<unsigned> *outputs = nullptr) {
  std::set<unsigned> observed = outputs ? *outputs : std::set<unsigned>{*group.output};
  // Effects of the group, by position: a second, equal load of a cell is not one.
  std::set<std::pair<unsigned, unsigned>> ownEffects;
  std::vector<bool> independent = memoryIndependentState(program);
  for (unsigned id : group.sites) {
    const MacSite &site = program.sites[id];
    ownEffects.insert({site.location, site.effectIndex});
    const Transition *block = firstFrom(program, site.location);
    if (!block)
      continue;
    auto own = [&](Term value, const MacAccess &access) {
      if (auto read = readOf(program, *block, value, access))
        ownEffects.insert({site.location, *read});
    };
    own(site.aValue, site.a);
    own(site.bValue, site.b);
    if (site.outputRead)
      own(site.accumulatedValue, *site.outputRead);
  }
  std::vector<ResidualEffect> out;
  std::set<unsigned> seen;
  for (const Transition &t : program.transitions) {
    if (!seen.insert(t.from).second)
      continue;
    for (unsigned e = 0; e < t.effects.size(); ++e) {
      const MemoryEffect &effect = t.effects[e];
      if (ownEffects.count({t.from, e}))
        continue;
      ResidualEffect residual;
      residual.location = t.from;
      residual.opIndex = effect.opIndex;
      residual.root = effect.root;
      residual.kind = effect.kind;
      if (effect.kind == MemoryEffect::Write)
        residual.valueReadsMemory = dependsOnMemory(effect.value, independent);
      else
        residual.observesOutput = observed.count(effect.root) != 0;
      out.push_back(residual);
    }
  }
  return out;
}

/// Every fill below is a means to find a witness. None of them is evidence of
/// anything when no difference appears.
class FillSource {
public:
  std::optional<EntryState> entry(const SemanticProgram &program,
                                  const std::vector<APInt> &parameters, uint64_t maxCells) {
    EntryState out;
    out.parameters = parameters;
    for (const MemoryRoot &root : program.roots) {
      RootData data;
      uint64_t cells = 1;
      for (unsigned d = 0; d < root.extents.size(); ++d) {
        int64_t extent = root.extents[d];
        if (extent < 0) {
          if (parameters[root.dimensionParameter[d]].isNegative())
            return std::nullopt;
          extent = parameters[root.dimensionParameter[d]].getZExtValue();
        }
        data.shape.push_back(extent);
        if (extent && cells > maxCells / extent)
          return std::nullopt;
        cells *= extent;
      }
      for (uint64_t c = 0; c < cells; ++c)
        if (root.origin == MemoryRoot::FunctionArgument)
          data.cells.emplace_back(cell(root));
        else
          data.cells.emplace_back(std::nullopt);
      out.memory.push_back(std::move(data));
    }
    return out;
  }

private:
  APInt cell(const MemoryRoot &root) {
    unsigned width = root.sort.width;
    if (!root.sort.elementIsFloat)
      return APInt(width, random(), /*isSigned=*/false, /*implicitTrunc=*/true);
    // Rounding and signed zero are where a fold shows, and a NaN stops the run.
    static const double pool[] = {0.0, -0.0, 1.0, -1.0, 0.5, 3.0, 1e16, -1e16, 1e-3, 9007199254740992.0};
    double value = pool[random() % std::size(pool)];
    if (random() % 2)
      value = (double)(random() % 4096) / 64.0 * ((random() % 2) ? 1.0 : -1.0);
    if (width == 32)
      return APInt(32, (uint64_t)llvm::bit_cast<uint32_t>((float)value));
    return APInt(64, llvm::bit_cast<uint64_t>(value));
  }
  uint64_t random() { return generator(); }
  std::mt19937_64 generator{20261102};
};

} // namespace

std::optional<std::string> replayDifference(const SemanticProgram &program,
                                            const SiteGroup &group, const EntryState &entry,
                                            const RunResult &run,
                                            const std::map<CellKey, APInt> &prefix) {
  if (run.status != RunStatus::Returned)
    return std::nullopt;
  auto offsetIn = [&](unsigned root, ArrayRef<APInt> key) {
    uint64_t at = 0;
    for (unsigned i = 0; i < key.size(); ++i)
      at = at * entry.memory[root].shape[i] + key[i].getZExtValue();
    return at;
  };
  std::set<unsigned> members(group.sites.begin(), group.sites.end());
  std::map<CellKey, APInt> replay;
  std::optional<std::string> found;
  auto note = [&](std::string what) {
    if (!found)
      found = std::move(what);
  };
  uint64_t position = 0;
  for (const MacEvent &event : run.events) {
    if (!members.count(event.site))
      continue;
    const MacSite &site = program.sites[event.site];
    UpdateShape shape = updateShape(site);
    if (!shape.ok)
      return std::nullopt;
    const auto &a0 = entry.memory[site.a.root].cells[offsetIn(site.a.root, event.f)];
    const auto &b0 = entry.memory[site.b.root].cells[offsetIn(site.b.root, event.g)];
    CellKey cell{site.output->root, offsetIn(site.output->root, event.h)};
    std::optional<APInt> current;
    auto kept = replay.find(cell);
    auto started = prefix.find(cell);
    if (kept != replay.end())
      current = kept->second;
    else if (started != prefix.end())
      current = started->second;
    else
      current = entry.memory[cell.first].cells[cell.second];
    // With no value to start from, the replay has no meaning here.
    if (!a0 || !b0 || !current)
      return std::nullopt;
    bool isFloat = site.reduction == ReductionOp::FAdd;
    APInt product = isFloat ? evalFloatBinary(TermOp::FMul, *a0, *b0)
                            : evalIntegerBinary(TermOp::Mul, *a0, *b0);
    const APInt &left = shape.accumulatorFirst ? *current : product;
    const APInt &right = shape.accumulatorFirst ? product : *current;
    APInt next = isFloat ? evalFloatBinary(TermOp::FAdd, left, right)
                         : evalIntegerBinary(TermOp::Add, left, right);
    std::string name = "event " + std::to_string(position);
    if (event.aValue != *a0 || event.bValue != *b0)
      note(name + " reads an operand that is not its initial value");
    if (event.updateValue != next)
      note(name + " writes a value other than the replay's");
    replay.insert_or_assign(cell, next);
    ++position;
  }
  for (const auto &[cell, value] : replay) {
    const auto &final = run.memory[cell.first].cells[cell.second];
    if (!final || *final != value)
      note("a final cell differs from the replay");
  }
  return found;
}

namespace {

} // namespace

bool enumerationCoversScope(const SemanticProgram &program, const DomainEnumeration &enumeration,
                            const ParameterScope &scope) {
  if (enumeration.sampled || scope.allParameters)
    return false;
  APInt expected(128, 1);
  for (const Parameter &p : program.parameters) {
    if (p.sort.kind != Sort::BV)
      continue;
    auto found = std::find_if(scope.ranges.begin(), scope.ranges.end(),
                              [&](const ParameterRange &r) { return r.parameter == p.id; });
    if (found == scope.ranges.end() || found->upper.slt(found->lower))
      return false;
    APInt span = (found->upper.sext(128) - found->lower.sext(128)) + 1;
    bool overflow = false;
    expected = expected.umul_ov(span, overflow);
    if (overflow)
      return false;
  }
  std::set<std::vector<uint64_t>> seen;
  for (const PointEvidence &point : enumeration.points) {
    if (!parametersInScope(scope, point.parameters))
      return false;
    std::vector<uint64_t> key;
    for (const APInt &value : point.parameters)
      key.push_back(value.getZExtValue());
    seen.insert(std::move(key));
  }
  return expected == APInt(128, seen.size() + enumeration.outsideDomain);
}

namespace {

/// The scope and the authorizations, in every binding.
std::string scopeBinding(const ComputationOptions &options) {
  std::string out = options.scope.allParameters ? "|all|" : "|some|";
  for (const ParameterRange &range : options.scope.ranges)
    out += std::to_string(range.parameter) + ":" + std::to_string(range.lower.getSExtValue()) +
           ":" + std::to_string(range.upper.getSExtValue()) + ",";
  for (const std::string &name : options.authorized)
    out += "|" + name;
  return out;
}

/// What a claim is about. A candidate joins only claims with its own.
std::string bindingOf(const SemanticProgram &program, const SiteGroup &group,
                      const ComputationOptions &options) {
  std::string out = program.fingerprint + "|" +
                    std::to_string(group.output ? (int64_t)*group.output : -1) + "," +
                    std::to_string(group.first) + "," + std::to_string(group.second) + "|";
  for (unsigned site : group.sites)
    out += std::to_string(site) + ",";
  return out + scopeBinding(options);
}

/// G5b-M3: every listed group, in the listed order, so that no claim over a set can
/// stand in for the claim of one group, or of another set.
std::string setBindingOf(const SemanticProgram &program, ArrayRef<const SiteGroup *> listed,
                         const ComputationOptions &options) {
  std::string out = program.fingerprint + "|groups|";
  for (const SiteGroup *group : listed) {
    out += std::to_string(group->output ? (int64_t)*group->output : -1) + "," +
           std::to_string(group->first) + "," + std::to_string(group->second) + ":";
    for (unsigned site : group->sites)
      out += std::to_string(site) + ",";
    out += ";";
  }
  return out + scopeBinding(options);
}

/// G5b-M3: the listed groups of a claim over several, and what its replays found.
struct SetTrace {
  std::vector<const SiteGroup *> groups; // In the listed order.
  std::vector<int> groupOf;              // The listed index of each program site, or -1.
  std::set<unsigned> outputs;            // The listed groups' output roots.
  // From the finite route: the points with events, and the first point at which the
  // listed order fails, or two groups write one cell.
  std::vector<GroupSetPoint> points;
  std::optional<std::vector<APInt>> orderBroken, cellShared;
  // From the symbolic route.
  SetFact symbolicOrder, symbolicShared;

  /// Only a run that reached the exit has its whole trace. The point's reference run
  /// returned, and the trace is one at a point (Theorem 9a), so a fact seen here is
  /// seen by every completing run there.
  void observe(const std::vector<APInt> &parameters, const ReplayResult &replay) {
    if (replay.outcome != ReplayOutcome::Equal && replay.outcome != ReplayOutcome::Mismatch)
      return;
    if (!replay.listedOrder && !orderBroken)
      orderBroken = parameters;
    if (replay.sharedCells && !cellShared)
      cellShared = parameters;
    if (replay.outcome == ReplayOutcome::Equal && replay.facts.events)
      points.push_back({parameters, replay.groupEvents, replay.sharedCells, replay.listedOrder});
  }
};

std::vector<std::vector<unsigned>> sitesOf(ArrayRef<const SiteGroup *> listed) {
  std::vector<std::vector<unsigned>> out;
  for (const SiteGroup *group : listed)
    out.push_back(group->sites);
  return out;
}

/// G5b-M3: the union of whole groups, each listed once, with the listed index of each
/// site. Nothing about how their cells meet is assumed or needed here. The gate is the
/// one every claim over a set of groups shares (G5b-M4a).
std::optional<SiteGroup> unite(const SemanticProgram &program, ArrayRef<const SiteGroup *> listed,
                               SetTrace &set, std::string &why) {
  GroupUnion gate;
  if (!uniteGroups(program, listed, gate, why))
    return std::nullopt;
  set.groupOf = std::move(gate.listedOf);
  for (const SiteGroup *group : listed) {
    set.groups.push_back(group);
    set.outputs.insert(*group->output);
  }
  SiteGroup united;
  united.sites = std::move(gate.sites);
  united.output = listed.front()->output;
  united.first = listed.front()->first;
  united.second = listed.front()->second;
  return united;
}

SetFact provenFact(ComputationTier tier, std::string detail) {
  SetFact out;
  out.status = SetFactStatus::Proven;
  out.tier = tier;
  out.detail = std::move(detail);
  return out;
}

SetFact refutedFact(std::vector<APInt> point, std::string detail) {
  SetFact out;
  out.status = SetFactStatus::Refuted;
  out.point = std::move(point);
  out.detail = std::move(detail);
  return out;
}

SetFact unknownFact(std::string detail) {
  SetFact out;
  out.detail = std::move(detail);
  return out;
}

CheckedComputation finiteOver(const SemanticProgram &program, const DomainEnumeration &enumeration,
                              const SiteGroup &group, const ComputationOptions &options,
                              SetTrace *set);

CheckedComputation finiteClaim(const SemanticProgram &program,
                               const DomainEnumeration &enumeration, unsigned g,
                               const ComputationOptions &options) {
  if (enumeration.gate != GateStatus::Open)
    return CheckedComputation::unknown("the domain gate is closed: " + enumeration.reason);
  if (g >= enumeration.groups.size())
    return CheckedComputation::unknown("no such group");
  const SiteGroup &group = enumeration.groups[g];
  std::string why;
  if (!wholeGroup(program, group, why) || !traceIsFixed(program, why))
    return CheckedComputation::unknown(why);
  return finiteOver(program, enumeration, group, options, nullptr);
}

/// One group, or the union of several (G5b-M3, with `set`): the events of `group`'s
/// sites, each with its own site's roots, replayed together in execution order.
CheckedComputation finiteOver(const SemanticProgram &program, const DomainEnumeration &enumeration,
                              const SiteGroup &group, const ComputationOptions &options,
                              SetTrace *set) {
  std::string why;
  std::vector<SeparationUse> separations;
  if (!separationsFor(program, group, options.authorized, separations, why))
    return CheckedComputation::unknown(why);
  auto frame = [&] { return frameOf(program, group, set ? &set->outputs : nullptr); };

  std::vector<ReplayFacts> facts;
  std::set<MemoryDependentStop> stops = memoryStopsOf(program);
  // ScopeEnumerated only for an enumeration of exactly this scope.
  bool complete = enumerationCoversScope(program, enumeration, options.scope), stopsSeen = false;
  std::string open;
  for (const PointEvidence &point : enumeration.points) {
    if (!point.reason.empty()) {
      complete = false;
      continue;
    }
    ReplayResult replay =
        set ? replayGroupSet(program, group, set->groupOf, set->groups.size(), point.parameters,
                             options.limits.replay)
            : replayGroup(program, group, point.parameters, options.limits.replay);
    if (set)
      set->observe(point.parameters, replay);
    stopsSeen = stopsSeen || !replay.conditionalStops.empty();
    stops.insert(replay.conditionalStops.begin(), replay.conditionalStops.end());
    if (replay.outcome == ReplayOutcome::Equal) {
      if (replay.facts.events)
        facts.push_back(std::move(replay.facts));
      continue;
    }
    complete = false;
    if (open.empty())
      open = std::string(stringifyReplayOutcome(replay.outcome)) + ": " + replay.reason;
    if (replay.outcome != ReplayOutcome::Mismatch)
      continue;
    // Different nodes prove nothing. Only a concrete run can say the values differ.
    FillSource fills;
    RunLimits limits;
    limits.fuel = options.limits.witnessFuel;
    limits.maxCells = options.limits.witnessCells;
    limits.maxRecordedConfigurations = 0;
    for (unsigned n = 0; n < options.limits.witnessFills; ++n) {
      auto entry = fills.entry(program, point.parameters, options.limits.witnessCells);
      if (!entry)
        break;
      RunResult run = runConcrete(program, *entry, limits);
      auto difference = replayDifference(program, group, *entry, run, replay.prefix);
      if (!difference)
        continue;
      CheckedComputation out = ComputationAuthority::make(
          ComputationTier::Proposed, ComputationStatus::Refuted,
          "the program and the replay differ at a concrete run: " + *difference);
      ComputationAuthority::witness(out, std::move(*entry));
      ComputationAuthority::separations(out, std::move(separations));
      ComputationAuthority::frame(out, frame());
      ComputationAuthority::stops(out, std::move(stops));
      return out;
    }
  }
  std::vector<SiteContract> contracts;
  if (auto list = contractsOf(program, group))
    contracts = std::move(*list);
  if (facts.empty()) {
    CheckedComputation out = CheckedComputation::unknown(
        open.empty() ? "no point has events" : open);
    ComputationAuthority::separations(out, std::move(separations));
    ComputationAuthority::frame(out, frame());
    ComputationAuthority::stops(out, std::move(stops));
    return out;
  }
  ComputationTier tier = complete ? ComputationTier::ScopeEnumerated : ComputationTier::Point;
  CheckedComputation out = ComputationAuthority::make(
      tier, ComputationStatus::Verified,
      open.empty() ? "Herbrand replay equal at each listed point, for all initial memories"
                   : "Herbrand replay equal at each listed point; another point is open: " + open);
  ComputationAuthority::contracts(out, std::move(contracts), foldOf(program, group));
  ComputationAuthority::separations(out, std::move(separations));
  ComputationAuthority::frame(out, frame());
  ComputationAuthority::stops(out, std::move(stops));
  ComputationAuthority::facts(out, std::move(facts));
  ComputationAuthority::every(out, enumeration.termination.kind() ==
                                       TerminationVerdict::ProvenTerminating &&
                                   !stopsSeen);
  return out;
}

namespace {

/// One write or read of a cell, with the arrivals of its block.
struct Access {
  unsigned location = 0, effect = 0, root = 0;
  ArrayRef<Term> index;
};

class Proof {
public:
  /// `set`: the listed groups when `group` is the union of several (G5b-M3).
  Proof(const SemanticProgram &program, const SiteGroup &group,
        const ComputationOptions &options, SmtBackend &backend, const SetTrace *set = nullptr)
      : program(program), group(group), options(options), backend(backend),
        terms(*program.terms), set(set) {}

  bool prepare(const TerminationCertificate &certificate, std::string &why);
  bool discharge();
  ViewInjectivity injective(const std::vector<Term> &key, unsigned view);
  /// `keep` false: a fact's query, which is no obligation of the claim.
  SmtAnswer ask(const std::string &id, const std::string &where, Term formula, bool keep = true);
  SetFact sharedFact();
  SetFact orderFact();

  std::vector<ObligationRecord> records;
  std::vector<SiteContract> contracts;
  std::string failure;
  std::optional<std::vector<APInt>> failing;
  std::map<unsigned, std::vector<SiteArrival>> arrivals;

private:
  bool structural(const std::string &id, const std::string &where, bool passed);
  bool require(const std::string &id, const std::string &where, Term formula);
  bool checkSite(const MacSite &site);
  bool returnsAreFree();
  bool hasEventfulPoint();
  Term sameCell(const SiteArrival &a, ArrayRef<Term> first, const SiteArrival &b,
                ArrayRef<Term> second);
  bool apart(const std::string &id, const std::string &where, const Access &a, const Access &b);

  const SemanticProgram &program;
  const SiteGroup &group;
  const ComputationOptions &options;
  SmtBackend &backend;
  TermContext &terms;
  const SetTrace *set;
  Term scope = nullptr;
  unsigned queries = 0;
  std::map<unsigned, Term> invariants;
};

SmtAnswer Proof::ask(const std::string &id, const std::string &where, Term formula, bool keep) {
  if (queries >= options.limits.maxQueries) {
    if (failure.empty())
      failure = "the query limit";
    return SmtAnswer::Unknown;
  }
  ++queries;
  std::vector<std::pair<std::string, Term>> aux;
  for (const Parameter &p : program.parameters)
    if (p.sort.kind == Sort::BV)
      aux.push_back({"v" + std::to_string(p.id), terms.param(p.id, p.sort)});
  ObligationRecord record;
  record.id = id;
  record.location = where;
  record.expected = "unsat";
  record.script = obligationScript(id + " " + where, formula, aux);
  SmtResponse response = backend.check(record.script, options.termination.solver, true);
  record.result = stringifySmtAnswer(response.answer);
  record.discharged = response.answer == SmtAnswer::Unsat;
  if (response.answer == SmtAnswer::Unknown)
    record.evidence = response.detail;
  if (response.answer == SmtAnswer::Sat && !failing) {
    std::vector<APInt> point;
    for (const Parameter &p : program.parameters) {
      auto found = response.model.find("v" + std::to_string(p.id));
      point.push_back(found == response.model.end() ? APInt(p.sort.width, 0)
                                                    : APInt(p.sort.width, found->second, 10));
    }
    failing = std::move(point);
  }
  if (keep)
    records.push_back(std::move(record));
  return response.answer;
}

bool Proof::require(const std::string &id, const std::string &where, Term formula) {
  SmtAnswer answer = ask(id, where, formula);
  if (answer == SmtAnswer::Unsat)
    return true;
  if (failure.empty())
    failure = id + " " + where + ": " + stringifySmtAnswer(answer);
  return false;
}

bool Proof::structural(const std::string &id, const std::string &where, bool passed) {
  ObligationRecord record;
  record.id = id;
  record.location = where;
  record.expected = "structural";
  record.result = passed ? "pass" : "fail";
  record.discharged = passed;
  records.push_back(std::move(record));
  if (!passed && failure.empty())
    failure = id + " " + where;
  return passed;
}

/// All of the block's terms read its entry state, so one arrival fixes them all.
Term Proof::sameCell(const SiteArrival &a, ArrayRef<Term> first, const SiteArrival &b,
                     ArrayRef<Term> second) {
  Term all = terms.logicalAnd(a.premise, secondCopy(program, b.premise));
  for (unsigned n = 0; n < first.size(); ++n)
    all = terms.logicalAnd(all, terms.icmp(Predicate::Eq, atPathEnd(program, a.path, first[n]),
                                           secondCopy(program, atPathEnd(program, b.path, second[n]))));
  return all;
}

/// No state of a and no state of b, at one parameter point, reach one cell. This
/// takes every pair of events of one run, and every event with itself. Two different
/// roots are apart by the separation gate, which has already passed.
bool Proof::apart(const std::string &id, const std::string &where, const Access &a,
                  const Access &b) {
  if (a.root != b.root)
    return true;
  for (const SiteArrival &first : arrivals[a.location])
    for (const SiteArrival &second : arrivals[b.location])
      if (!require(id, where, sameCell(first, a.index, second, b.index)))
        return false;
  return true;
}

bool Proof::checkSite(const MacSite &site) {
  std::string where = "site " + std::to_string(site.id);
  const Transition *block = firstFrom(program, site.location);
  // E1: one store of this root at this cell, holding this update.
  bool storeOk = block && site.effectIndex < block->effects.size() && site.output && site.outputRead;
  if (storeOk) {
    const MemoryEffect &effect = block->effects[site.effectIndex];
    storeOk = effect.kind == MemoryEffect::Write && effect.root == site.output->root &&
              sameTerms(effect.index, site.output->index);
  }
  if (!structural("E1", where, storeOk))
    return false;
  const MemoryEffect &store = block->effects[site.effectIndex];

  // R1: the value stored is exactly acc (+) (a (*) b), flags included.
  if (!structural("R1", where, store.value == site.updateValue && updateShape(site).ok))
    return false;

  // OP1: the operands are the loads of the recorded accesses.
  auto readA = readOf(program, *block, site.aValue, site.a),
       readB = readOf(program, *block, site.bValue, site.b);
  if (!structural("OP1", where, readA && readB))
    return false;

  // W1: the update reads the cell it writes.
  auto readC = readOf(program, *block, site.accumulatedValue, *site.outputRead);
  if (!structural("W1r", where, readC && site.outputRead->root == site.output->root &&
                                    *readC < site.effectIndex))
    return false;
  if (!sameTerms(site.output->index, site.outputRead->index))
    for (const SiteArrival &a : arrivals[site.location]) {
      Term differs = terms.boolean(false);
      for (unsigned n = 0; n < site.output->index.size(); ++n)
        differs = terms.logicalOr(
            differs, terms.icmp(Predicate::Ne, atPathEnd(program, a.path, site.output->index[n]),
                                atPathEnd(program, a.path, site.outputRead->index[n])));
      if (!require("W1", where, terms.logicalAnd(a.premise, differs)))
        return false;
    }

  // W2: no write between the load and the store reaches that cell, or the read is
  // stale. C0 alone cannot see this.
  for (unsigned e = *readC + 1; e < site.effectIndex; ++e) {
    const MemoryEffect &between = block->effects[e];
    if (between.kind != MemoryEffect::Write || between.root != site.output->root)
      continue;
    if (!structural("W2i", where, !sameTerms(between.index, site.output->index)))
      return false;
    for (const SiteArrival &a : arrivals[site.location]) {
      Term same = a.premise;
      for (unsigned n = 0; n < between.index.size(); ++n)
        same = terms.logicalAnd(same,
                                terms.icmp(Predicate::Eq, atPathEnd(program, a.path, between.index[n]),
                                           atPathEnd(program, a.path, site.output->index[n])));
      if (!require("W2", where, same))
        return false;
    }
  }
  return true;
}

/// F2: nothing leaves through a return that this claim does not describe.
bool Proof::returnsAreFree() {
  std::vector<bool> independent = memoryIndependentState(program);
  std::function<bool(Term, DenseSet<Term> &)> free = [&](Term term, DenseSet<Term> &seen) {
    if (!seen.insert(term).second)
      return true;
    if (term->op == TermOp::Load || term->op == TermOp::MemVar)
      return false;
    if ((term->op == TermOp::State || term->op == TermOp::Shadow) && !independent[term->index])
      return false;
    return llvm::all_of(term->args, [&](Term arg) { return free(arg, seen); });
  };
  for (const Transition &t : program.transitions)
    for (Term term : llvm::concat<const Term>(t.returns, t.returnPoison)) {
      DenseSet<Term> seen;
      if (!structural("F2", "return", free(term, seen)))
        return false;
    }
  return true;
}

bool Proof::prepare(const TerminationCertificate &certificate, std::string &why) {
  SymbolicCheckOptions check;
  check.scope = options.scope;
  check.termination = options.termination;
  check.authorized = options.authorized;
  check.limits.paths = options.termination.paths;
  ProvenPremises premises = provenPremises(program, certificate, check, backend);
  records = std::move(premises.records);
  if (!premises.ready) {
    why = premises.why;
    return false;
  }
  scope = premises.scope;
  invariants = std::move(premises.invariants);

  std::set<unsigned> places;
  for (const Transition &t : program.transitions)
    for (const MemoryEffect &effect : t.effects)
      if (effect.kind == MemoryEffect::Write)
        places.insert(t.from);
  for (unsigned id : group.sites)
    places.insert(program.sites[id].location);
  std::vector<unsigned> list(places.begin(), places.end());
  SiteArrivals reached = reachLocations(program, invariants, certificate.cuts, list, scope,
                                        options.termination.paths);
  if (!reached.complete) {
    why = reached.reason;
    return false;
  }
  for (SiteArrival &arrival : reached.arrivals)
    arrivals[arrival.location].push_back(std::move(arrival));
  return true;
}

bool Proof::discharge() {
  // One list of effects per block: every transition out of it carries the same.
  std::map<unsigned, const Transition *> blocks;
  for (const Transition &t : program.transitions) {
    auto [slot, added] = blocks.try_emplace(t.from, &t);
    if (added)
      continue;
    bool same = t.effects.size() == slot->second->effects.size();
    for (unsigned e = 0; same && e < t.effects.size(); ++e)
      same = sameEffect(t.effects[e], slot->second->effects[e]);
    if (!structural("E0", "block " + std::to_string(t.from), same))
      return false;
  }

  // F3: the relation speaks of initial cells. A fresh root has none, so a claim here
  // could be vacuous: every run might stop at its first read. Every listed group's.
  std::vector<unsigned> roots{*group.output, group.first, group.second};
  if (set) {
    roots.clear();
    // By first site, not by the listing, so that no record depends on the listing.
    std::vector<const SiteGroup *> ordered(set->groups.begin(), set->groups.end());
    std::sort(ordered.begin(), ordered.end(), [](const SiteGroup *x, const SiteGroup *y) {
      return x->sites.front() < y->sites.front();
    });
    for (const SiteGroup *listed : ordered)
      roots.insert(roots.end(), {*listed->output, listed->first, listed->second});
  }
  for (unsigned root : roots)
    if (!structural("F3", "root " + std::to_string(root),
                    program.roots[root].origin == MemoryRoot::FunctionArgument))
      return false;

  // E2: each site owns its store, so one block execution is one event.
  std::set<std::pair<unsigned, unsigned>> owned;
  for (unsigned id : group.sites) {
    const MacSite &site = program.sites[id];
    if (!structural("E2", "site " + std::to_string(id),
                    owned.insert({site.location, site.effectIndex}).second))
      return false;
  }
  for (const MacSite &site : program.sites)
    if (site.output && !llvm::is_contained(group.sites, site.id) &&
        !structural("E2", "site " + std::to_string(site.id),
                    !owned.count({site.location, site.effectIndex})))
      return false;

  for (unsigned id : group.sites)
    if (!checkSite(program.sites[id]))
      return false;
  auto list = contractsOf(program, group);
  if (!structural("R1", "group", list.has_value()))
    return false;
  contracts = std::move(*list);

  std::vector<Access> ownWrites, otherWrites, operands;
  for (const auto &[location, block] : blocks)
    for (unsigned e = 0; e < block->effects.size(); ++e) {
      const MemoryEffect &effect = block->effects[e];
      if (effect.kind != MemoryEffect::Write)
        continue;
      Access access{location, e, effect.root, effect.index};
      (owned.count({location, e}) ? ownWrites : otherWrites).push_back(access);
    }
  for (unsigned id : group.sites) {
    const MacSite &site = program.sites[id];
    operands.push_back({site.location, 0, site.a.root, site.a.index});
    operands.push_back({site.location, 0, site.b.root, site.b.index});
  }

  uint64_t pairs = 0;
  auto count = [&](const Access &a, const Access &b) {
    if (a.root == b.root)
      pairs += arrivals[a.location].size() * arrivals[b.location].size();
  };
  for (const Access &operand : operands) {
    for (const Access &write : ownWrites)
      count(write, operand);
    for (const Access &write : otherWrites)
      count(write, operand);
  }
  for (const Access &write : otherWrites)
    for (const Access &own : ownWrites)
      count(write, own);
  if (pairs > options.limits.maxPairs) {
    failure = "more arrival pairs than the limit";
    return false;
  }

  // OP2: no write of the whole program reaches an operand cell.
  for (const std::vector<Access> *writes : {&ownWrites, &otherWrites})
    for (const Access &write : *writes)
      for (const Access &operand : operands)
        if (!apart("OP2", "write at " + std::to_string(write.location) + ", operand at " +
                              std::to_string(operand.location),
                   write, operand))
          return false;
  // F1: no other write reaches a cell of the group.
  for (const Access &write : otherWrites)
    for (const Access &own : ownWrites)
      if (!apart("F1", "write at " + std::to_string(write.location) + ", output at " +
                           std::to_string(own.location),
                 write, own))
        return false;
  if (!returnsAreFree())
    return false;

  // NV: a universal claim over no event would hold for any program, so there must be
  // a run of the scope that completes with one. Not finding it is an unknown.
  return structural("NV", "a completing run with an event", hasEventfulPoint());
}

/// A few points of the scope, each replayed exactly. A search, so it can only miss.
bool Proof::hasEventfulPoint() {
  for (uint64_t k = 0; k < 16; ++k) {
    std::vector<APInt> point;
    for (const Parameter &p : program.parameters) {
      APInt value(p.sort.width, 0);
      if (p.sort.kind == Sort::BV) {
        for (const ParameterRange &range : options.scope.ranges)
          if (range.parameter == p.id) {
            value = range.lower + APInt(p.sort.width, k, /*isSigned=*/false, /*implicitTrunc=*/true);
            if (value.sgt(range.upper))
              value = range.upper;
          }
        if (llvm::none_of(options.scope.ranges,
                          [&](const ParameterRange &range) { return range.parameter == p.id; }))
          value = APInt(p.sort.width, k, /*isSigned=*/false, /*implicitTrunc=*/true);
      }
      point.push_back(value);
    }
    if (!parametersInScope(options.scope, point))
      continue;
    ReplayResult replay = replayGroup(program, group, point, options.limits.replay);
    if (replay.outcome == ReplayOutcome::Equal && replay.facts.events)
      return true;
  }
  return false;
}

/// G5b-M3: no cell is written by stores of two listed groups, over the scope. Each pair
/// of stores of different groups on one root is asked apart, at one parameter point. A
/// fact: an answer other than unsat leaves it Unknown and changes no claim.
SetFact Proof::sharedFact() {
  std::vector<std::pair<const MacSite *, const MacSite *>> pairs;
  uint64_t count = 0;
  for (unsigned x : group.sites)
    for (unsigned y : group.sites) {
      const MacSite &first = program.sites[x], &second = program.sites[y];
      if (set->groupOf[x] >= set->groupOf[y] || first.output->root != second.output->root)
        continue;
      pairs.push_back({&first, &second});
      count += arrivals[first.location].size() * arrivals[second.location].size();
    }
  if (count > options.limits.maxPairs)
    return unknownFact("more arrival pairs than the limit");
  for (const auto &[first, second] : pairs)
    for (const SiteArrival &a : arrivals[first->location])
      for (const SiteArrival &b : arrivals[second->location]) {
        SmtAnswer answer = ask("SH", "stores of sites " + std::to_string(first->id) + " and " +
                                         std::to_string(second->id),
                               sameCell(a, first->output->index, b, second->output->index), false);
        if (answer != SmtAnswer::Unsat)
          return unknownFact(std::string("SH at sites ") + std::to_string(first->id) + " and " +
                             std::to_string(second->id) + ": " + stringifySmtAnswer(answer));
      }
  return provenFact(ComputationTier::Symbolic, "no cell of two listed groups, over the scope");
}

/// G5b-M3: from every arrival at a site of the j-th listed group, every feasible way to
/// the next event of the union arrives at a group listed no earlier than j. Induction on
/// the events: the listed indices never fall, which is the listed order. The walk calls a
/// leaf only for a path the solver found feasible, so reaching an earlier group there is
/// a failure, with no query of its own. One block with sites of two groups holds two
/// events the walk takes for one, so it is not judged.
SetFact Proof::orderFact() {
  std::map<unsigned, int> groupAt;
  for (unsigned id : group.sites) {
    auto [slot, added] = groupAt.try_emplace(program.sites[id].location, set->groupOf[id]);
    if (!added && slot->second != set->groupOf[id])
      return unknownFact("two listed groups have sites in one block");
  }
  std::vector<unsigned> locations;
  for (const auto &[location, listed] : groupAt)
    locations.push_back(location);
  AskUnsat quiet = [&](const std::string &id, const std::string &where, Term formula) {
    return ask(id, where, formula, false);
  };
  for (const auto &[location, listed] : groupAt)
    for (const SiteArrival &arrival : arrivals[location]) {
      std::string why;
      int here = listed;
      auto leaf = [&](const SymbolicPath &path, Term premise, bool exit) {
        if (exit)
          return true;
        auto next = groupAt.find(path.end);
        if (next == groupAt.end()) {
          why = "an event at a block of no listed group";
          return false;
        }
        (void)premise;
        if (next->second >= here)
          return true;
        why = "GO: an event of group " + std::to_string(next->second) + " may follow one of group " +
              std::to_string(here) + " on a feasible path";
        return false;
      };
      if (!walkToNextEvents(program, locations, options.limits.maxUnfolding, quiet, why,
                            arrival.path, arrival.premise, leaf))
        return unknownFact(why.empty() ? "the walk to the next event failed" : why);
    }
  return provenFact(ComputationTier::Symbolic,
                    "no event of a group follows one of a later-listed group, over the scope");
}

ViewInjectivity Proof::injective(const std::vector<Term> &key, unsigned view) {
  auto viewOf = [&](const MacSite &site) -> ArrayRef<Term> {
    if (view == 0)
      return site.output->index;
    return view == 1 ? ArrayRef<Term>(site.a.index) : ArrayRef<Term>(site.b.index);
  };
  auto rootOf = [&](const MacSite &site) {
    return view == 0 ? site.output->root : view == 1 ? site.a.root : site.b.root;
  };
  for (unsigned x : group.sites)
    for (unsigned y : group.sites) {
      const MacSite &first = program.sites[x], &second = program.sites[y];
      if (rootOf(first) != rootOf(second))
        continue;
      for (const SiteArrival &a : arrivals[first.location])
        for (const SiteArrival &b : arrivals[second.location]) {
          Term same = sameCell(a, viewOf(first), b, viewOf(second));
          Term apartKey = terms.boolean(false);
          for (Term t : key)
            apartKey = terms.logicalOr(
                apartKey, terms.icmp(Predicate::Ne, atPathEnd(program, a.path, t),
                                     secondCopy(program, atPathEnd(program, b.path, t))));
          if (ask("X", "view", terms.logicalAnd(same, apartKey)) != SmtAnswer::Unsat)
            return ViewInjectivity::NotProven;
        }
    }
  return ViewInjectivity::Proven;
}

/// Accepts the certificate as G3b does, here, for this program and scope.
std::optional<CheckOutcome> acceptCertificate(const SemanticProgram &program,
                                              const TerminationCertificate *certificate,
                                              const ComputationOptions &options,
                                              SmtBackend &backend, std::string &why) {
  if (!certificate) {
    why = "no termination certificate";
    return std::nullopt;
  }
  CheckOptions check;
  check.scope = options.scope;
  check.limits = options.termination;
  check.authorized = options.authorized;
  CheckOutcome outcome = checkCertificate(program, *certificate, check, backend);
  if (outcome.verdict.kind() != TerminationVerdict::ProvenTerminating &&
      outcome.verdict.kind() != TerminationVerdict::ProvenTerminatingIfDefined) {
    why = "the certificate was not accepted: " + outcome.verdict.reason();
    return std::nullopt;
  }
  return outcome;
}

bool limitsAreEffective(const ComputationOptions &options, std::string &why) {
  const SmtLimits &solver = options.termination.solver;
  if (solver.timeoutMilliseconds && solver.resourceLimit)
    return true;
  why = "a solver limit of zero is no limit";
  return false;
}

} // namespace

/// One group, or the union of several (G5b-M3, with `set`): Theorem 9 over the sites of
/// `group`, and for a set its two facts once the claim stands.
CheckedComputation symbolicOver(const SemanticProgram &program,
                                const TerminationCertificate *certificate,
                                const SiteGroup &group, const ComputationOptions &options,
                                SmtBackend &backend, SetTrace *set) {
  std::string why;
  std::vector<SeparationUse> separations;
  if (!separationsFor(program, group, options.authorized, separations, why))
    return CheckedComputation::unknown(why);
  auto outcome = acceptCertificate(program, certificate, options, backend, why);
  if (!outcome)
    return CheckedComputation::unknown(why);

  Proof proof(program, group, options, backend, set);
  std::set<MemoryDependentStop> stops = memoryStopsOf(program);
  try {
    if (!proof.prepare(*certificate, why))
      return CheckedComputation::unknown(why);
    bool passed = proof.discharge();
    CheckedComputation out =
        passed ? ComputationAuthority::make(ComputationTier::Symbolic, ComputationStatus::Verified,
                                            "E, OP, R, W and F discharged over the scope")
               : ComputationAuthority::make(ComputationTier::Proposed, ComputationStatus::Unknown,
                                            proof.failure);
    // Facts once the claim stands. A term limit in one leaves it Unknown, and the claim
    // as it is.
    if (set && passed) {
      try {
        set->symbolicShared = proof.sharedFact();
      } catch (const TermError &error) {
        set->symbolicShared = unknownFact(std::string("term limit: ") + error.what());
      }
      try {
        set->symbolicOrder = proof.orderFact();
      } catch (const TermError &error) {
        set->symbolicOrder = unknownFact(std::string("term limit: ") + error.what());
      }
    }
    ComputationAuthority::records(out, std::move(proof.records));
    ComputationAuthority::separations(out, std::move(separations));
    ComputationAuthority::frame(out, frameOf(program, group, set ? &set->outputs : nullptr));
    ComputationAuthority::every(out, passed &&
                                         outcome->verdict.kind() ==
                                             TerminationVerdict::ProvenTerminating &&
                                         stops.empty());
    ComputationAuthority::stops(out, std::move(stops));
    if (passed)
      ComputationAuthority::contracts(out, std::move(proof.contracts), foldOf(program, group));
    else if (proof.failing)
      ComputationAuthority::failing(out, *proof.failing);
    return out;
  } catch (const TermError &error) {
    return CheckedComputation::unknown(std::string("term limit: ") + error.what());
  }
}

CheckedComputation symbolicClaim(const SemanticProgram &program,
                                 const TerminationCertificate *certificate,
                                 const SiteGroup &group, const ComputationOptions &options,
                                 SmtBackend &backend) {
  if (!program.isSupported())
    return CheckedComputation::unknown("the program is unsupported");
  std::string why;
  if (!limitsAreEffective(options, why) || !wholeGroup(program, group, why) ||
      !traceIsFixed(program, why))
    return CheckedComputation::unknown(why);
  return symbolicOver(program, certificate, group, options, backend, nullptr);
}

/// The facts of a finite claim over a set: Refuted at the first point that shows a
/// violation, Proven at the claim's own tier when it is verified, and Unknown otherwise.
SetFact finiteFact(const CheckedComputation &claim, const std::optional<std::vector<APInt>> &broken,
                   const char *refuted, const char *proven) {
  if (broken)
    return refutedFact(*broken, refuted);
  if (claim.status() == ComputationStatus::Verified)
    return provenFact(claim.tier(), proven);
  return unknownFact("the claim is not verified");
}

CheckedComputation finiteSetClaim(const SemanticProgram &program,
                                  const DomainEnumeration &enumeration,
                                  ArrayRef<const SiteGroup *> listed,
                                  const ComputationOptions &options) {
  if (enumeration.gate != GateStatus::Open)
    return CheckedComputation::unknown("the domain gate is closed for the set: " + enumeration.reason);
  SetTrace set;
  std::string why;
  auto united = unite(program, listed, set, why);
  if (!united || !traceIsFixed(program, why))
    return CheckedComputation::unknown(why);
  CheckedComputation out = finiteOver(program, enumeration, *united, options, &set);
  SetFact order = finiteFact(out, set.orderBroken, "an event of a group follows one of a later-listed group",
                             "the listed order at each replayed point");
  SetFact shared = finiteFact(out, set.cellShared, "two listed groups write one cell",
                              "no cell of two listed groups at each replayed point");
  ComputationAuthority::groupSet(out, sitesOf(listed), std::move(order), std::move(shared),
                                 std::move(set.points));
  return out;
}

CheckedComputation symbolicSetClaim(const SemanticProgram &program,
                                    const TerminationCertificate *certificate,
                                    ArrayRef<const SiteGroup *> listed,
                                    const ComputationOptions &options, SmtBackend &backend) {
  if (!program.isSupported())
    return CheckedComputation::unknown("the program is unsupported");
  std::string why;
  if (!limitsAreEffective(options, why))
    return CheckedComputation::unknown(why);
  SetTrace set;
  auto united = unite(program, listed, set, why);
  if (!united || !traceIsFixed(program, why))
    return CheckedComputation::unknown(why);
  set.symbolicOrder = set.symbolicShared = unknownFact("the claim is not verified");
  CheckedComputation out = symbolicOver(program, certificate, *united, options, backend, &set);
  ComputationAuthority::groupSet(out, sitesOf(listed), std::move(set.symbolicOrder),
                                 std::move(set.symbolicShared), {});
  return out;
}

/// A set refused before any replay still names its groups, and says why its facts are
/// unknown.
CheckedComputation refusedSet(CheckedComputation out, ArrayRef<const SiteGroup *> listed) {
  if (!out.groupSites().empty())
    return out;
  SetFact unknown = unknownFact("the set is refused: " + out.reason());
  ComputationAuthority::groupSet(out, sitesOf(listed), unknown, unknown, {});
  return out;
}

} // namespace

CheckedComputation checkComputationFinite(const SemanticProgram &program,
                                          const DomainEnumeration &enumeration, unsigned g,
                                          const ComputationOptions &options) {
  CheckedComputation out = finiteClaim(program, enumeration, g, options);
  if (g < enumeration.groups.size())
    ComputationAuthority::bind(out, bindingOf(program, enumeration.groups[g], options));
  return out;
}

CheckedComputation checkComputationSymbolic(const SemanticProgram &program,
                                            const TerminationCertificate *certificate,
                                            const SiteGroup &group,
                                            const ComputationOptions &options,
                                            SmtBackend &backend) {
  CheckedComputation out = symbolicClaim(program, certificate, group, options, backend);
  ComputationAuthority::bind(out, bindingOf(program, group, options));
  return out;
}

std::string groupSetBinding(const SemanticProgram &program, ArrayRef<const SiteGroup *> listed,
                            const ComputationOptions &options) {
  return setBindingOf(program, listed, options);
}

CheckedComputation checkGroupSetFinite(const SemanticProgram &program,
                                       const DomainEnumeration &enumeration,
                                       ArrayRef<unsigned> groups,
                                       const ComputationOptions &options) {
  std::vector<const SiteGroup *> listed;
  for (unsigned g : groups) {
    if (g >= enumeration.groups.size())
      return refusedSet(CheckedComputation::unknown("no such group"), {});
    listed.push_back(&enumeration.groups[g]);
  }
  CheckedComputation out = refusedSet(finiteSetClaim(program, enumeration, listed, options), listed);
  ComputationAuthority::bind(out, setBindingOf(program, listed, options));
  return out;
}

CheckedComputation checkGroupSetSymbolic(const SemanticProgram &program,
                                         const TerminationCertificate *certificate,
                                         ArrayRef<SiteGroup> groups,
                                         const ComputationOptions &options, SmtBackend &backend) {
  std::vector<const SiteGroup *> listed;
  for (const SiteGroup &group : groups)
    listed.push_back(&group);
  CheckedComputation out =
      refusedSet(symbolicSetClaim(program, certificate, listed, options, backend), listed);
  ComputationAuthority::bind(out, setBindingOf(program, listed, options));
  return out;
}

CandidateComputation checkCandidateComputation(
    const SemanticProgram &program, const TerminationCertificate *certificate,
    const DomainEnumeration &enumeration, unsigned g, const CoordinateFamily &family,
    const EnumerationOrder *order, const CheckedComputation &finiteClaimOfGroup,
    const CheckedComputation &symbolicClaimOfGroup, const ComputationOptions &options,
    SmtBackend &backend) {
  CandidateComputation out;
  out.family = family;
  if (g >= enumeration.groups.size()) {
    out.reason = "no such group";
    return out;
  }
  const SiteGroup &group = enumeration.groups[g];

  // A claim about another program, group, scope or set of authorizations counts for
  // nothing here.
  const CheckedComputation unbound = CheckedComputation::unknown("a claim about something else");
  std::string binding = bindingOf(program, group, options);
  const CheckedComputation &finite =
      finiteClaimOfGroup.binding() == binding ? finiteClaimOfGroup : unbound;
  const CheckedComputation &symbolic =
      symbolicClaimOfGroup.binding() == binding ? symbolicClaimOfGroup : unbound;

  // Nothing from the search is believed: both coordinate checks run here.
  out.coordinateFinite = checkFamilyFinite(program, enumeration, g, family);
  SymbolicCheckOptions check;
  check.scope = options.scope;
  check.termination = options.termination;
  check.authorized = options.authorized;
  check.limits.paths = options.termination.paths;
  out.coordinateSymbolic =
      checkFamilySymbolic(program, certificate, group, family, order, check, backend);

  bool coordinateSymbolicOk = out.coordinateSymbolic.status() == FamilyStatus::Verified &&
                              out.coordinateSymbolic.tier() >= EvidenceTier::SymbolicValid;
  bool coordinateFiniteOk = out.coordinateFinite.status() == FamilyStatus::Verified &&
                            out.coordinateFinite.tier() >= EvidenceTier::Point;
  bool coordinateRefuted = out.coordinateFinite.status() == FamilyStatus::Refuted ||
                           out.coordinateSymbolic.status() == FamilyStatus::Refuted;

  if (finite.status() == ComputationStatus::Refuted ||
      symbolic.status() == ComputationStatus::Refuted) {
    out.status = ComputationStatus::Refuted;
    out.reason = "the group's replay relation is refuted";
    return out;
  }
  if (coordinateRefuted) {
    out.status = ComputationStatus::Refuted;
    out.reason = "the coordinates are refuted";
    return out;
  }
  if (coordinateSymbolicOk && symbolic.tier() == ComputationTier::Symbolic &&
      symbolic.status() == ComputationStatus::Verified) {
    out.status = ComputationStatus::Verified;
    out.tier = ComputationTier::Symbolic;
    out.everyRun = symbolic.holdsForEveryRun() && out.coordinateSymbolic.holdsForEveryRun();
    out.reason = "coordinates and computation hold over the scope";
  } else if (finite.status() == ComputationStatus::Verified &&
             (coordinateFiniteOk || coordinateSymbolicOk)) {
    // The least of the two, at the points both have.
    std::set<std::vector<uint64_t>> covered;
    auto key = [](const std::vector<APInt> &point) {
      std::vector<uint64_t> bits;
      for (const APInt &value : point)
        bits.push_back(value.getZExtValue());
      return bits;
    };
    if (coordinateFiniteOk)
      for (const PointFacts &facts : out.coordinateFinite.points())
        covered.insert(key(facts.parameters));
    for (const ReplayFacts &facts : finite.points())
      if (coordinateSymbolicOk || covered.count(key(facts.parameters)))
        out.points.push_back(facts.parameters);
    if (out.points.empty()) {
      out.reason = "the coordinate and computation points do not meet";
      return out;
    }
    bool whole = finite.tier() == ComputationTier::ScopeEnumerated &&
                 (coordinateSymbolicOk ||
                  out.coordinateFinite.tier() == EvidenceTier::ScopeEnumerated);
    out.status = ComputationStatus::Verified;
    out.tier = whole ? ComputationTier::ScopeEnumerated : ComputationTier::Point;
    out.everyRun = false;
    out.reason = "coordinates and computation hold at the listed points";
  } else {
    out.reason = "no tier of the coordinates meets a tier of the computation";
    return out;
  }

  // Facts only. A view that collapses is no reason to doubt anything.
  if (coordinateSymbolicOk && symbolic.status() == ComputationStatus::Verified) {
    Proof proof(program, group, options, backend);
    std::string why;
    try {
      if (certificate && proof.prepare(*certificate, why)) {
        // A term that cannot be adopted leaves the fact open, not the key shorter.
        auto judged = [&](unsigned x, unsigned y, unsigned z, unsigned view) {
          std::vector<Term> list;
          for (unsigned axis : {x, y, z})
            for (Term term : family.axes[axis]) {
              Term kept = adoptProofTerm(program, term, why);
              if (!kept)
                return ViewInjectivity::NotProven;
              list.push_back(kept);
            }
          return proof.injective(list, view);
        };
        out.symbolicInjective[0] = judged(AxisP, AxisI, AxisJ, 0);
        out.symbolicInjective[1] = judged(AxisP, AxisI, AxisK, 1);
        out.symbolicInjective[2] = judged(AxisP, AxisK, AxisJ, 2);
      }
    } catch (const TermError &) {
      // A fact not established stays NotProven.
    }
  }
  return out;
}

} // namespace mlir::generalized_gemm
