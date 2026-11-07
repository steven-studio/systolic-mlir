#include "GeneralizedGemm/FiniteDomain.h"

#include "GeneralizedGemm/CutPaths.h"
#include "GeneralizedGemm/TerminationObligations.h"

#include "llvm/ADT/DenseSet.h"

#include <functional>
#include <map>
#include <tuple>

using namespace llvm;

namespace mlir::generalized_gemm {

struct DomainAuthority {
  static AccessDomain make(const SemanticProgram &program,
                           std::vector<APInt> parameters, DomainEvidence evidence,
                           AccessTable table) {
    return AccessDomain(program.fingerprint, std::move(parameters),
                        std::move(evidence), std::move(table));
  }
};

const char *stringifyGateStatus(GateStatus status) {
  switch (status) {
  case GateStatus::Open: return "OPEN";
  case GateStatus::Unsupported: return "UNSUPPORTED";
  case GateStatus::Unknown: return "UNKNOWN";
  }
  return "UNKNOWN";
}

namespace {

std::vector<SiteGroup> groupSites(const SemanticProgram &program) {
  std::vector<SiteGroup> groups;
  std::map<std::tuple<unsigned, unsigned, unsigned>, unsigned> known;
  for (const MacSite &site : program.sites) {
    if (site.form != ReductionForm::Direct || !site.output || !site.outputRead) {
      SiteGroup group;
      group.first = site.a.root;
      group.second = site.b.root;
      group.sites = {site.id};
      group.unsupported = "a scalar accumulator has no exact output view";
      groups.push_back(std::move(group));
      continue;
    }
    // A read of another root makes this C := D + A * B, which is no reduction at
    // any index.
    if (site.outputRead->root != site.output->root) {
      SiteGroup group;
      group.output = site.output->root;
      group.first = site.a.root;
      group.second = site.b.root;
      group.sites = {site.id};
      group.unsupported = "the update reads one root and writes another";
      groups.push_back(std::move(group));
      continue;
    }
    auto key = std::make_tuple(site.output->root, site.a.root, site.b.root);
    auto [slot, added] = known.try_emplace(key, (unsigned)groups.size());
    if (added) {
      SiteGroup group;
      group.output = site.output->root;
      group.first = site.a.root;
      group.second = site.b.root;
      groups.push_back(std::move(group));
    }
    groups[slot->second].sites.push_back(site.id);
  }
  return groups;
}

/// Premise 4 of the record: float parameters are not enumerated, so nothing that
/// decides the control path or an address may read one.
bool controlReadsFloat(const SemanticProgram &program) {
  for (const Transition &t : program.transitions) {
    if (containsFloat(t.guard))
      return true;
    for (const auto &[id, term] : t.assignments)
      if (term->sort.kind == Sort::BV && containsFloat(term))
        return true;
    for (const MemoryEffect &effect : t.effects)
      for (Term index : effect.index)
        if (containsFloat(index))
          return true;
  }
  return false;
}

/// The closed signed range of each parameter, or nothing when too many points.
struct Ranges {
  std::vector<std::pair<int64_t, int64_t>> bounds; // Float parameters: {0, 0}.
  APInt points = APInt(128, 1); // Saturating: only compared against a limit.
};

Ranges parameterRanges(const SemanticProgram &program, const ParameterScope &scope) {
  Ranges ranges;
  for (const Parameter &p : program.parameters) {
    if (p.sort.kind != Sort::BV) {
      ranges.bounds.push_back({0, 0});
      continue;
    }
    int64_t low = APInt::getSignedMinValue(p.sort.width).getSExtValue();
    int64_t high = APInt::getSignedMaxValue(p.sort.width).getSExtValue();
    if (!scope.allParameters)
      for (const ParameterRange &range : scope.ranges)
        if (range.parameter == p.id) {
          low = std::max(low, range.lower.getSExtValue());
          high = std::min(high, range.upper.getSExtValue());
        }
    ranges.bounds.push_back({low, high});
    APInt count(128, 0);
    if (low <= high)
      count = APInt(128, (uint64_t)high - (uint64_t)low) + 1;
    ranges.points = ranges.points.umul_sat(count);
  }
  return ranges;
}

std::optional<EntryState> referenceEntry(const SemanticProgram &program,
                                         const std::vector<APInt> &point,
                                         uint64_t fill, uint64_t maxCells) {
  EntryState entry;
  entry.parameters = point;
  for (const MemoryRoot &root : program.roots) {
    RootData data;
    uint64_t cells = 1;
    for (unsigned d = 0; d < root.extents.size(); ++d) {
      uint64_t extent = root.extents[d] >= 0
                            ? (uint64_t)root.extents[d]
                            : point[root.dimensionParameter[d]].getZExtValue();
      data.shape.push_back(extent);
      if (extent && cells > maxCells / extent)
        return std::nullopt;
      cells *= extent;
    }
    // A fresh allocation is uninitialized, whatever the fill.
    for (uint64_t c = 0; c < cells; ++c)
      if (root.origin == MemoryRoot::FunctionArgument)
        data.cells.emplace_back(APInt(root.sort.width, fill, /*isSigned=*/false,
                                      /*implicitTrunc=*/true));
      else
        data.cells.emplace_back(std::nullopt);
    entry.memory.push_back(std::move(data));
  }
  return entry;
}

bool hasNegativeExtent(const SemanticProgram &program, const std::vector<APInt> &point) {
  for (const Parameter &p : program.parameters)
    if (p.kind == Parameter::Dimension && point[p.id].isNegative())
      return true;
  return false;
}

bool hasLoad(Term term, llvm::DenseSet<Term> &seen) {
  if (!seen.insert(term).second)
    return false;
  if (term->op == TermOp::Load)
    return true;
  return llvm::any_of(term->args, [&](Term arg) { return hasLoad(arg, seen); });
}

/// runConcrete, keeping the state at block entry of each event.
RunResult runKeepingState(const SemanticProgram &program, const EntryState &entry,
                          const RunLimits &limits,
                          std::vector<std::vector<std::optional<APInt>>> &states) {
  RunResult result;
  auto config = initialConfiguration(program, entry, limits, result.reason);
  if (!config) {
    result.status = RunStatus::InvalidEntry;
    return result;
  }
  while (true) {
    if (config->location == program.exitLocation()) {
      result.status = RunStatus::Returned;
      return result;
    }
    if (result.steps >= limits.fuel) {
      result.status = RunStatus::OutOfFuel;
      result.reason = "fuel exhausted: this proves nothing";
      return result;
    }
    auto before = config->state;
    StepOutcome step = stepConfiguration(program, entry, *config);
    if (step.stopped) {
      result.status = *step.stopped;
      result.reason = step.reason;
      return result;
    }
    for (MacEvent &event : step.events) {
      event.step = result.steps;
      result.events.push_back(std::move(event));
      states.push_back(before);
    }
    ++result.steps;
  }
}

class Enumerator {
public:
  Enumerator(const SemanticProgram &program, const EnumerationOptions &options,
             DomainEnumeration &out)
      : program(program), options(options), out(out) {
    for (unsigned g = 0; g < out.groups.size(); ++g)
      for (unsigned site : out.groups[g].sites)
        groupOfSite[site] = g;
  }

  void enumerate(const Ranges &ranges);
  void visit(const std::vector<APInt> &point);

private:
  void fillLive(const RunResult &run,
                const std::vector<std::vector<std::optional<APInt>>> &states,
                const std::vector<APInt> &point, const DomainEvidence &evidence,
                PointEvidence &slot);
  void fill(const RunResult &run, const std::vector<APInt> &point,
            const DomainEvidence &evidence, PointEvidence &evidenceOut);

  const SemanticProgram &program;
  const EnumerationOptions &options;
  DomainEnumeration &out;
  std::map<unsigned, unsigned> groupOfSite;
};

void Enumerator::fill(const RunResult &run, const std::vector<APInt> &point,
                      const DomainEvidence &evidence, PointEvidence &slot) {
  std::vector<AccessTable> tables(out.groups.size());
  for (unsigned g = 0; g < out.groups.size(); ++g) {
    const SiteGroup &group = out.groups[g];
    AccessTable &table = tables[g];
    table.coordinatesUniform = !group.sites.empty();
    for (unsigned site : group.sites) {
      const auto &names = program.sites[site].coordinates;
      if (site == group.sites.front())
        table.coordinateNames.assign(names.begin(), names.end());
      else if (!llvm::equal(table.coordinateNames, names))
        table.coordinatesUniform = false;
    }
  }
  for (const MacEvent &event : run.events) {
    unsigned g = groupOfSite.at(event.site);
    if (out.groups[g].unsupported.empty())
      tables[g].add(event.site, event.step, event.coordinates, event.h, event.hRead,
                    event.f, event.g);
  }
  for (unsigned g = 0; g < out.groups.size(); ++g)
    if (!tables[g].events().empty())
      slot.domains[g] =
          DomainAuthority::make(program, point, evidence, std::move(tables[g]));
}

void Enumerator::fillLive(const RunResult &run,
                          const std::vector<std::vector<std::optional<APInt>>> &states,
                          const std::vector<APInt> &point,
                          const DomainEvidence &evidence, PointEvidence &slot) {
  std::vector<bool> independent = memoryIndependentState(program);
  for (unsigned g = 0; g < out.groups.size(); ++g) {
    if (!out.groups[g].unsupported.empty())
      continue;
    // One list for every site of the group, or x would not mean one thing.
    std::vector<unsigned> ids;
    for (const StateVar &s : program.state)
      if (independent[s.id] && s.sort.kind == Sort::BV)
        ids.push_back(s.id);
    for (size_t e = 0; e < run.events.size(); ++e)
      if (groupOfSite.at(run.events[e].site) == g)
        llvm::erase_if(ids, [&](unsigned id) { return !states[e][id]; });
    AccessTable table;
    table.coordinateNames = ids;
    table.coordinatesUniform = true;
    for (size_t e = 0; e < run.events.size(); ++e) {
      const MacEvent &event = run.events[e];
      if (groupOfSite.at(event.site) != g)
        continue;
      IndexTuple x;
      for (unsigned id : ids)
        x.push_back(*states[e][id]);
      table.add(event.site, event.step, x, event.h, event.hRead, event.f, event.g);
    }
    if (!table.events().empty())
      slot.domains[g] = DomainAuthority::make(program, point, evidence, std::move(table));
  }
}

void Enumerator::visit(const std::vector<APInt> &point) {
  if (!parametersInScope(options.scope, point))
    return;
  if (hasNegativeExtent(program, point)) {
    ++out.outsideDomain;
    return;
  }
  PointEvidence evidence;
  evidence.parameters = point;
  evidence.domains.resize(out.groups.size());
  auto entry = referenceEntry(program, point, options.referenceFill,
                              options.limits.maxCells);
  if (!entry) {
    evidence.reason = "the memory of this point exceeds the cell limit";
    out.points.push_back(std::move(evidence));
    return;
  }
  RunLimits limits;
  limits.fuel = options.limits.fuel;
  limits.maxCells = options.limits.maxCells;
  limits.maxRecordedConfigurations = 0;
  std::vector<std::vector<std::optional<APInt>>> states;
  RunResult run = options.liveStatePoints
                      ? runKeepingState(program, *entry, limits, states)
                      : runConcrete(program, *entry, limits);
  evidence.run = run.status;
  DomainEvidence domain = domainEvidenceFromRun(program, *entry, run);
  DomainAdmission admission = admitsClaim(domain, DomainClaim::Coverage);
  if (!admission.admitted) {
    evidence.reason = run.reason.empty() ? domain.provenance : run.reason;
  } else if (run.events.size() > options.limits.maxEvents) {
    evidence.reason = "the event limit";
  } else if (options.liveStatePoints) {
    fillLive(run, states, point, domain, evidence);
  } else {
    fill(run, point, domain, evidence);
  }
  out.points.push_back(std::move(evidence));
}

void Enumerator::enumerate(const Ranges &ranges) {
  std::vector<APInt> point;
  std::function<void(unsigned)> go = [&](unsigned k) {
    if (k == program.parameters.size()) {
      visit(point);
      return;
    }
    unsigned width = program.parameters[k].sort.width;
    auto [low, high] = ranges.bounds[k];
    if (low > high)
      return;
    // The exit is before the increment, so the top of an i64 range still ends.
    for (int64_t v = low;; ++v) {
      point.push_back(APInt(width, (uint64_t)v, /*isSigned=*/true,
                            /*implicitTrunc=*/true));
      go(k + 1);
      point.pop_back();
      if (v == high)
        break;
    }
  };
  go(0);
}

} // namespace

std::optional<EventStates> eventStates(const SemanticProgram &program,
                                       const std::vector<APInt> &point, uint64_t fill,
                                       const EnumerationLimits &limits) {
  if (point.size() != program.parameters.size() || hasNegativeExtent(program, point))
    return std::nullopt;
  auto entry = referenceEntry(program, point, fill, limits.maxCells);
  if (!entry)
    return std::nullopt;
  EventStates out;
  out.entry = *entry;
  RunLimits run;
  run.fuel = limits.fuel;
  run.maxCells = limits.maxCells;
  run.maxRecordedConfigurations = 0;
  out.run = runKeepingState(program, out.entry, run, out.states);
  return out;
}

std::vector<bool> memoryIndependentState(const SemanticProgram &program) {
  std::vector<bool> tainted(program.state.size(), false);
  for (bool changed = true; changed;) {
    changed = false;
    for (const Transition &t : program.transitions)
      for (const auto &[id, term] : t.assignments) {
        if (tainted[id])
          continue;
        llvm::DenseSet<Term> seen;
        bool depends = hasLoad(term, seen) || term->sort.kind != Sort::BV ||
                       containsFloat(term) ||
                       llvm::any_of(stateSupport(term), [&](unsigned r) { return tainted[r]; });
        if (depends) {
          tainted[id] = true;
          changed = true;
        }
      }
  }
  tainted.flip();
  return tainted;
}

DomainEnumeration enumerateDomain(const SemanticProgram &program,
                                  const TerminationCertificate *certificate,
                                  const EnumerationOptions &options,
                                  SmtBackend &backend) {
  DomainEnumeration out;
  auto close = [&](GateStatus gate, std::string reason) {
    out.gate = gate;
    out.reason = std::move(reason);
    return std::move(out);
  };
  if (!program.isSupported())
    return close(GateStatus::Unsupported,
                 "the program is unsupported: " + program.unsupported->detail);
  out.groups = groupSites(program);
  if (out.groups.empty())
    return close(GateStatus::Unsupported, "the program has no MAC site");
  bool anySupported = false;
  for (const SiteGroup &group : out.groups)
    anySupported = anySupported || group.unsupported.empty();
  if (!anySupported)
    return close(GateStatus::Unsupported, out.groups[0].unsupported);
  // The trace would be a function of memory, so a parameter point fixes nothing.
  if (!program.controlOblivious || !program.addressOblivious)
    return close(GateStatus::Unsupported, "the trace depends on memory contents");
  if (controlReadsFloat(program))
    return close(GateStatus::Unsupported, "control or an address reads a float");

  Ranges ranges = parameterRanges(program, options.scope);
  out.sampled = !options.onlyPoints.empty();
  if (out.sampled) {
    if (options.onlyPoints.size() > options.limits.maxPoints)
      return close(GateStatus::Unknown, "more sample points than the limit");
    for (const auto &point : options.onlyPoints)
      if (point.size() != program.parameters.size() ||
          !parametersInScope(options.scope, point))
        return close(GateStatus::Unknown, "a sample point is outside the scope");
  } else if (ranges.points.ugt(options.limits.maxPoints))
    return close(GateStatus::Unknown, "the scope has more points than the limit");

  if (!certificate)
    return close(GateStatus::Unknown, "no termination certificate");
  CheckOptions check;
  check.scope = options.scope;
  check.limits = options.termination;
  check.authorized = options.authorized;
  CheckOutcome outcome = checkCertificate(program, *certificate, check, backend);
  out.termination = outcome.verdict;
  out.terminationStatus = outcome.status;
  out.stops = outcome.stops;
  out.assumptionsUsed = outcome.assumptionsUsed;
  if (outcome.verdict.kind() != TerminationVerdict::ProvenTerminating &&
      outcome.verdict.kind() != TerminationVerdict::ProvenTerminatingIfDefined)
    return close(GateStatus::Unknown,
                 "the certificate was not accepted: " + outcome.verdict.reason());

  Enumerator enumerator(program, options, out);
  if (out.sampled)
    for (const auto &point : options.onlyPoints)
      enumerator.visit(point);
  else
    enumerator.enumerate(ranges);
  return close(GateStatus::Open, "");
}

} // namespace mlir::generalized_gemm
