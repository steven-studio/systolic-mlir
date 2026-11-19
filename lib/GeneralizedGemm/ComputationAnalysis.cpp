//===- ComputationAnalysis.cpp --------------------------------------------===//
#include "GeneralizedGemm/ComputationAnalysis.h"

#include "llvm/Support/JSON.h"
#include "llvm/Support/raw_ostream.h"

#include <algorithm>

using llvm::APInt;
namespace json = llvm::json;

namespace mlir::generalized_gemm {

ComputationResult analyzeComputation(const SemanticProgram &program,
                                     const TerminationCertificate *certificate,
                                     const ComputationRequest &request, SmtBackend &backend) {
  ComputationResult out;
  const ComputationOptions &options = request.computation;
  std::vector<GroupSynthesis> searched;
  if (request.search) {
    SynthesisResult synthesis = synthesizeProgram(program, certificate, request.synthesis, backend);
    out.enumeration = std::move(synthesis.enumeration);
    searched = std::move(synthesis.groups);
  } else {
    EnumerationOptions enumeration;
    enumeration.scope = options.scope;
    enumeration.termination = options.termination;
    enumeration.authorized = options.authorized;
    enumeration.limits = request.synthesis.limits;
    enumeration.referenceFill = request.synthesis.referenceFill;
    out.enumeration = enumerateDomain(program, certificate, enumeration, backend);
  }

  for (unsigned g = 0; g < out.enumeration.groups.size(); ++g) {
    const SiteGroup &siteGroup = out.enumeration.groups[g];
    GroupComputation group;
    group.group = g;
    group.finite = checkComputationFinite(program, out.enumeration, g, options);
    group.symbolic = checkComputationSymbolic(program, certificate, siteGroup, options, backend);

    // A lead from a failed query is run exactly, and is nothing until then.
    if (group.finite.status() != ComputationStatus::Refuted && group.symbolic.failingPoint()) {
      EnumerationOptions exact;
      exact.scope = options.scope;
      exact.termination = options.termination;
      exact.authorized = options.authorized;
      exact.limits = request.synthesis.limits;
      exact.onlyPoints = {*group.symbolic.failingPoint()};
      DomainEnumeration point = enumerateDomain(program, certificate, exact, backend);
      CheckedComputation lead = checkComputationFinite(program, point, g, options);
      if (lead.status() == ComputationStatus::Refuted)
        group.finite = std::move(lead);
    }

    for (const GroupSynthesis &result : searched) {
      if (result.group != g)
        continue;
      group.search = result.status;
      group.searchReason = result.reason;
      for (const VerifiedCandidate &candidate : result.candidates)
        group.candidates.push_back(checkCandidateComputation(
            program, certificate, out.enumeration, g, candidate.family,
            candidate.order ? &*candidate.order : nullptr, group.finite, group.symbolic, options,
            backend));
    }
    out.groups.push_back(std::move(group));
  }

  // G5b-M3: each set is its own claim, and a lead is run exactly as for a group.
  for (const GroupSetRequest &asked : request.groupSets) {
    GroupSetComputation set;
    set.groups = asked.groups;
    if (asked.all) {
      set.groups.clear();
      for (unsigned g = 0; g < out.enumeration.groups.size(); ++g)
        if (out.enumeration.groups[g].unsupported.empty())
          set.groups.push_back(g);
      if (asked.reversed)
        std::reverse(set.groups.begin(), set.groups.end());
    }
    std::vector<SiteGroup> listed;
    bool known = true;
    for (unsigned g : set.groups) {
      known = known && g < out.enumeration.groups.size();
      if (known)
        listed.push_back(out.enumeration.groups[g]);
    }
    set.finite = checkGroupSetFinite(program, out.enumeration, set.groups, options);
    set.symbolic = known ? checkGroupSetSymbolic(program, certificate, listed, options, backend)
                         : CheckedComputation::unknown("no such group");
    if (set.finite.status() != ComputationStatus::Refuted && set.symbolic.failingPoint()) {
      EnumerationOptions exact;
      exact.scope = options.scope;
      exact.termination = options.termination;
      exact.authorized = options.authorized;
      exact.limits = request.synthesis.limits;
      exact.onlyPoints = {*set.symbolic.failingPoint()};
      DomainEnumeration point = enumerateDomain(program, certificate, exact, backend);
      CheckedComputation lead = checkGroupSetFinite(program, point, set.groups, options);
      if (lead.status() == ComputationStatus::Refuted)
        set.finite = std::move(lead);
    }
    out.sets.push_back(std::move(set));
  }
  return out;
}

namespace {

std::string hex(const APInt &value) {
  llvm::SmallString<32> text;
  value.toString(text, 16, /*Signed=*/false);
  return text.str().str();
}

json::Array parametersJSON(const std::vector<APInt> &point) {
  json::Array out;
  for (const APInt &value : point)
    out.push_back((int64_t)value.getSExtValue());
  return out;
}

json::Object checkedJSON(const CheckedComputation &c) {
  json::Object out{{"tier", stringifyComputationTier(c.tier())},
                   {"status", stringifyComputationStatus(c.status())},
                   {"reason", c.reason()},
                   {"holds_for", c.holdsForEveryRun() ? "every run" : "runs that complete"},
                   {"fold", stringifyFoldForm(c.fold())},
                   {"domain_and_order", c.domainAndOrder().name()},
                   {"transformation", c.transformationEquivalence().name()}};
  json::Array contracts;
  for (const SiteContract &contract : c.contracts())
    contracts.push_back(json::Object{{"site", (int64_t)contract.site},
                                     {"reduction_flags", (int64_t)contract.reductionFlags},
                                     {"multiply_flags", (int64_t)contract.multiplyFlags},
                                     {"accumulator_first", contract.accumulatorFirst}});
  out["contracts"] = std::move(contracts);
  json::Array separations;
  for (const SeparationUse &use : c.separations())
    separations.push_back(json::Object{{"first", (int64_t)use.first},
                                       {"second", (int64_t)use.second},
                                       {"assumption", use.assumption},
                                       {"name", use.name},
                                       {"basis", use.basis}});
  out["separations"] = std::move(separations);
  json::Array frame;
  for (const ResidualEffect &effect : c.frame())
    frame.push_back(json::Object{
        {"location", (int64_t)effect.location},
        {"op", (int64_t)effect.opIndex},
        {"root", (int64_t)effect.root},
        {"kind", effect.kind == MemoryEffect::Write ? "write" : "read"},
        {"value_reads_memory", effect.valueReadsMemory},
        {"observes_output", effect.observesOutput}});
  out["frame"] = std::move(frame);
  json::Array stops;
  for (const MemoryDependentStop &stop : c.memoryStops())
    stops.push_back(json::Object{{"kind", stringifySideKind(stop.kind)}, {"reason", stop.reason}});
  out["memory_stops"] = std::move(stops);
  json::Array points;
  for (const ReplayFacts &facts : c.points())
    points.push_back(json::Object{{"parameters", parametersJSON(facts.parameters)},
                                  {"steps", (int64_t)facts.steps},
                                  {"events", (int64_t)facts.events},
                                  {"output_cells", (int64_t)facts.outputCells},
                                  {"prefix_cells", (int64_t)facts.prefixCells},
                                  {"residual_cells", (int64_t)facts.residualCells},
                                  {"max_contributions", (int64_t)facts.maxContributions}});
  out["points"] = std::move(points);
  json::Array records;
  for (const ObligationRecord &record : c.records())
    records.push_back(json::Object{{"id", record.id},
                                   {"location", record.location},
                                   {"result", record.result},
                                   {"discharged", record.discharged}});
  out["obligations"] = std::move(records);
  if (c.failingPoint())
    out["failing_point"] = parametersJSON(*c.failingPoint());
  if (c.witness()) {
    const EntryState &entry = *c.witness();
    json::Array memory;
    for (const RootData &root : entry.memory) {
      json::Array cells;
      for (const auto &cell : root.cells)
        cells.push_back(cell ? json::Value(hex(*cell)) : json::Value(nullptr));
      memory.push_back(std::move(cells));
    }
    out["witness"] = json::Object{{"parameters", parametersJSON(entry.parameters)},
                                  {"memory", std::move(memory)}};
  }
  return out;
}

const char *injectivityName(ViewInjectivity answer) {
  return answer == ViewInjectivity::Proven ? "PROVEN" : "NOT_PROVEN";
}

json::Object setFactJSON(const SetFact &fact) {
  json::Object out{{"status", stringifySetFactStatus(fact.status)},
                   {"tier", stringifyComputationTier(fact.tier)},
                   {"detail", fact.detail}};
  if (fact.point)
    out["point"] = parametersJSON(*fact.point);
  return out;
}

/// A claim over a set: a claim's fields, and the set's facts.
json::Object setClaimJSON(const CheckedComputation &c) {
  json::Object out = checkedJSON(c);
  json::Array listed;
  for (const std::vector<unsigned> &sites : c.groupSites()) {
    json::Array list;
    for (unsigned site : sites)
      list.push_back((int64_t)site);
    listed.push_back(std::move(list));
  }
  out["group_sites"] = std::move(listed);
  out["group_order"] = setFactJSON(c.groupOrder());
  out["shared_cells"] = setFactJSON(c.sharedCells());
  json::Array points;
  for (const GroupSetPoint &point : c.groupPoints()) {
    json::Array events;
    for (uint64_t count : point.events)
      events.push_back((int64_t)count);
    points.push_back(json::Object{{"parameters", parametersJSON(point.parameters)},
                                  {"events", std::move(events)},
                                  {"shared_cells", (int64_t)point.sharedCells},
                                  {"listed_order", point.listedOrder}});
  }
  out["group_points"] = std::move(points);
  return out;
}

} // namespace

std::string toJSON(const SemanticProgram &program, const ComputationResult &result) {
  const DomainEnumeration &enumeration = result.enumeration;
  json::Object out{{"function", program.functionName},
                   {"fingerprint", program.fingerprint},
                   {"gate", stringifyGateStatus(enumeration.gate)},
                   {"reason", enumeration.reason},
                   {"authorizes_lowering", false},
                   {"sampled", enumeration.sampled},
                   {"termination", stringifyTerminationVerdict(enumeration.termination.kind())}};
  json::Array groups;
  for (const GroupComputation &group : result.groups) {
    const SiteGroup &sites = enumeration.groups[group.group];
    json::Array siteList, candidates;
    for (unsigned id : sites.sites)
      siteList.push_back((int64_t)id);
    for (const CandidateComputation &candidate : group.candidates) {
      static const char *names[] = {"p", "i", "j", "k"};
      json::Object axes;
      for (unsigned axis = 0; axis < 4; ++axis) {
        json::Array list;
        for (Term term : candidate.family.axes[axis])
          list.push_back(formatTerm(term));
        axes[names[axis]] = std::move(list);
      }
      json::Array points;
      for (const std::vector<APInt> &point : candidate.points)
        points.push_back(parametersJSON(point));
      candidates.push_back(json::Object{
          {"axes", std::move(axes)},
          {"status", stringifyComputationStatus(candidate.status)},
          {"tier", stringifyComputationTier(candidate.tier)},
          {"reason", candidate.reason},
          {"coordinate_finite", stringifyEvidenceTier(candidate.coordinateFinite.tier())},
          {"coordinate_symbolic", stringifyEvidenceTier(candidate.coordinateSymbolic.tier())},
          {"holds_for", candidate.everyRun ? "every run" : "runs that complete"},
          {"points", std::move(points)},
          {"injective_h", injectivityName(candidate.symbolicInjective[0])},
          {"injective_f", injectivityName(candidate.symbolicInjective[1])},
          {"injective_g", injectivityName(candidate.symbolicInjective[2])},
          {"domain_and_order", "UNKNOWN"},
          {"transformation", "UNKNOWN"},
          {"authorizes_lowering", false}});
    }
    json::Object entry{{"group", (int64_t)group.group},
                       {"unsupported", sites.unsupported},
                       {"output", sites.output ? json::Value((int64_t)*sites.output)
                                               : json::Value(nullptr)},
                       {"first", (int64_t)sites.first},
                       {"second", (int64_t)sites.second},
                       {"sites", std::move(siteList)},
                       {"finite", checkedJSON(group.finite)},
                       {"symbolic", checkedJSON(group.symbolic)},
                       {"search", stringifyRecoveryStatus(group.search)},
                       {"search_reason", group.searchReason},
                       {"candidates", std::move(candidates)}};
    groups.push_back(std::move(entry));
  }
  out["groups"] = std::move(groups);
  // G5b-M3, only when asked: the output above is unchanged without it.
  if (!result.sets.empty()) {
    json::Array sets;
    for (const GroupSetComputation &set : result.sets) {
      json::Array indices, roots;
      for (unsigned g : set.groups) {
        indices.push_back((int64_t)g);
        if (g < enumeration.groups.size()) {
          const SiteGroup &group = enumeration.groups[g];
          roots.push_back(json::Array{group.output ? json::Value((int64_t)*group.output)
                                                   : json::Value(nullptr),
                                      (int64_t)group.first, (int64_t)group.second});
        }
      }
      sets.push_back(json::Object{{"groups", std::move(indices)},
                                  {"roots", std::move(roots)},
                                  {"authorizes_lowering", false},
                                  {"finite", setClaimJSON(set.finite)},
                                  {"symbolic", setClaimJSON(set.symbolic)}});
    }
    out["group_sets"] = std::move(sets);
  }
  std::string text;
  llvm::raw_string_ostream stream(text);
  stream << llvm::formatv("{0:2}", json::Value(std::move(out)));
  return text;
}

} // namespace mlir::generalized_gemm
