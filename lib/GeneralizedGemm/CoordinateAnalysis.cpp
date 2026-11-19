#include "GeneralizedGemm/CoordinateAnalysis.h"

#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/raw_ostream.h"

#include <map>

using namespace llvm;

namespace mlir::generalized_gemm {

namespace {

/// Memoization of joins, by the exact kernels. Two points whose views have the
/// same kernels have the same joins. The checker still runs on each point.
class JoinCache {
public:
  CanonicalPartitions joins(const AccessTable &table, uint64_t &hits) {
    Partition kerH, kerF, kerG;
    viewKernels(table, kerH, kerF, kerG);
    std::array<Partition, 3> key{kerH, kerF, kerG};
    auto found = cache.find(key);
    if (found != cache.end()) {
      ++hits;
      return found->second;
    }
    CanonicalPartitions out =
        joinKernels(std::move(kerH), std::move(kerF), std::move(kerG));
    cache.emplace(std::move(key), out);
    return out;
  }

private:
  std::map<std::array<Partition, 3>, CanonicalPartitions> cache;
};

GroupRecovery recoverGroup(unsigned group, const AccessDomain &domain,
                           JoinCache &cache, uint64_t &hits) {
  GroupRecovery out;
  out.group = group;
  CanonicalPartitions partitions = cache.joins(domain.table(), hits);
  CoordinateCandidate candidate = canonicalCandidate(domain.table(), partitions);
  CheckedCandidate checked = checkCandidate(domain, candidate);
  // C0 fails on an exact event, so it refutes the form, not just this candidate.
  bool formRefuted = checked.kind() == RecoveryStatus::CandidateRefuted &&
                     !checked.report().outputReadMatches;
  if (checked.kind() == RecoveryStatus::CandidateFound || formRefuted) {
    out.candidate = std::move(candidate);
    out.result = std::move(checked);
    return out;
  }
  out.attempted = checked.report();
  out.hints = std::move(partitions);
  out.result = CheckedCandidate::unknown(
      "the injective-view class does not apply: " + checked.reason());
  return out;
}

json::Array tupleJSON(const IndexTuple &tuple) {
  json::Array out;
  for (const APInt &value : tuple)
    out.push_back((int64_t)value.getSExtValue());
  return out;
}

json::Array labelsJSON(ArrayRef<uint32_t> labels) {
  json::Array out;
  for (uint32_t label : labels)
    out.push_back((int64_t)label);
  return out;
}

json::Array namesJSON(ArrayRef<unsigned> names) {
  json::Array out;
  for (unsigned name : names)
    out.push_back((int64_t)name);
  return out;
}

json::Object reportJSON(const CandidateReport &r) {
  json::Object out;
  if (!r.malformed.empty()) {
    out["malformed"] = r.malformed;
    return out;
  }
  out["validity"] = json::Object{{"decomposition_valid", r.decompositionValid},
                                 {"output_read_matches", r.outputReadMatches},
                                 {"function_of_point", r.functionOfPoint},
                                 {"dependence_h", r.dependenceH},
                                 {"dependence_f", r.dependenceF},
                                 {"dependence_g", r.dependenceG}};
  out["cartesian"] = json::Object{{"form", stringifyCartesianForm(r.cartesian)},
                                  {"missing_points", r.missingPoints}};
  out["multiplicity"] =
      json::Object{{"form", stringifyMultiplicityForm(r.multiplicity)},
                   {"min", (int64_t)r.multiplicityMin},
                   {"max", (int64_t)r.multiplicityMax},
                   {"triple_min", (int64_t)r.tripleMultiplicityMin},
                   {"triple_max", (int64_t)r.tripleMultiplicityMax},
                   {"duplicate_iteration_points", (int64_t)r.duplicateIterationPoints}};
  json::Array batches;
  for (const BatchShape &b : r.batches)
    batches.push_back(json::Object{{"batch", (int64_t)b.batch},
                                   {"i", (int64_t)b.i},
                                   {"j", (int64_t)b.j},
                                   {"k", (int64_t)b.k},
                                   {"points", (int64_t)b.points},
                                   {"events", (int64_t)b.events},
                                   {"missing", b.missing}});
  json::Object shape{{"class", stringifyShapeClass(r.shape)},
                     {"events", (int64_t)r.events},
                     {"triples", (int64_t)r.triples},
                     {"points", (int64_t)r.points},
                     {"distinct_p", (int64_t)r.distinctP},
                     {"distinct_i", (int64_t)r.distinctI},
                     {"distinct_j", (int64_t)r.distinctJ},
                     {"distinct_k", (int64_t)r.distinctK},
                     {"batches", std::move(batches)}};
  if (r.support) {
    shape["support"] = json::Object{{"p", namesJSON((*r.support)[0])},
                                    {"i", namesJSON((*r.support)[1])},
                                    {"j", namesJSON((*r.support)[2])},
                                    {"k", namesJSON((*r.support)[3])}};
  }
  out["shape"] = std::move(shape);
  out["injectivity"] = json::Object{{"h", r.injectiveH},
                                    {"f", r.injectiveF},
                                    {"g", r.injectiveG},
                                    {"batches_share_views", r.batchesShareViews}};
  out["strict_non_degenerate"] = r.strictNonDegenerate;
  out["genuine"] = r.genuine;
  out["unique_up_to_relabelling"] = r.uniqueUpToRelabelling;
  json::Array open;
  for (const std::string &item : r.openObligations)
    open.push_back(item);
  out["eligibility"] = json::Object{{"form", stringifyEligibility(r.eligibility)},
                                    {"open_obligations", std::move(open)}};
  json::Array counterexamples;
  for (const Counterexample &c : r.counterexamples) {
    json::Object entry{{"kind", stringifyCounterexampleKind(c.kind)}};
    if (c.kind == CounterexampleKind::MissingPoint)
      entry["labels"] = labelsJSON(c.labels);
    else {
      entry["first"] = (int64_t)c.first;
      entry["second"] = (int64_t)c.second;
    }
    counterexamples.push_back(std::move(entry));
  }
  out["counterexamples"] = std::move(counterexamples);
  return out;
}

json::Object hintsJSON(const CanonicalPartitions &p) {
  return json::Object{{"ker_h", labelsJSON(p.kerH)},
                      {"ker_f", labelsJSON(p.kerF)},
                      {"ker_g", labelsJSON(p.kerG)},
                      {"rho_i", labelsJSON(p.rhoI)},
                      {"rho_j", labelsJSON(p.rhoJ)},
                      {"rho_k", labelsJSON(p.rhoK)},
                      {"rho_p", labelsJSON(p.rhoP)},
                      {"meet_h", p.meetH},
                      {"meet_f", p.meetF},
                      {"meet_g", p.meetG}};
}

json::Object candidateJSON(const CoordinateCandidate &c) {
  return json::Object{{"origin", c.origin},
                      {"p", labelsJSON(c.p)},
                      {"i", labelsJSON(c.i)},
                      {"j", labelsJSON(c.j)},
                      {"k", labelsJSON(c.k)}};
}

json::Array eventsJSON(const AccessTable &table) {
  json::Array out;
  for (const AccessEvent &e : table.events()) {
    IndexTuple x(table.point(e.point).begin() + 1, table.point(e.point).end());
    out.push_back(json::Object{{"site", (int64_t)e.site},
                               {"step", (int64_t)e.step},
                               {"x", tupleJSON(x)},
                               {"h", tupleJSON(table.view(e.h))},
                               {"h_read", tupleJSON(table.view(e.hRead))},
                               {"f", tupleJSON(table.view(e.f))},
                               {"g", tupleJSON(table.view(e.g))}});
  }
  return out;
}

std::string printed(json::Value value) {
  std::string text;
  raw_string_ostream stream(text);
  stream << formatv("{0:1}", std::move(value));
  return stream.str();
}

} // namespace

CoordinateResult recoverCoordinates(const SemanticProgram &program,
                                    const TerminationCertificate *certificate,
                                    const EnumerationOptions &options,
                                    SmtBackend &backend) {
  CoordinateResult result;
  result.enumeration = enumerateDomain(program, certificate, options, backend);
  if (result.enumeration.gate != GateStatus::Open)
    return result;
  JoinCache cache;
  const DomainEnumeration &enumeration = result.enumeration;
  for (const PointEvidence &point : enumeration.points) {
    PointRecovery recovery;
    for (unsigned g = 0; g < enumeration.groups.size(); ++g) {
      const SiteGroup &group = enumeration.groups[g];
      if (!group.unsupported.empty()) {
        GroupRecovery refused;
        refused.group = g;
        refused.result = CheckedCandidate::unsupported(group.unsupported);
        recovery.groups.push_back(std::move(refused));
      } else if (point.domains[g]) {
        recovery.groups.push_back(
            recoverGroup(g, *point.domains[g], cache, result.memoizedJoins));
      } else {
        GroupRecovery empty;
        empty.group = g;
        // An empty run proves nothing about coordinates, and is no failure.
        empty.result = CheckedCandidate::unknown(
            point.reason.empty() ? "this group has no events at this point"
                                 : point.reason);
        recovery.groups.push_back(std::move(empty));
      }
    }
    result.points.push_back(std::move(recovery));
  }
  return result;
}

json::Value reportToJSON(const CandidateReport &report) { return reportJSON(report); }

std::string toJSON(const SemanticProgram &program, const CoordinateResult &result,
                   bool events) {
  const DomainEnumeration &enumeration = result.enumeration;
  json::Object out{{"function", program.functionName},
                   {"fingerprint", program.fingerprint},
                   {"gate", stringifyGateStatus(enumeration.gate)},
                   {"reason", enumeration.reason},
                   {"authorizes_lowering", false},
                   {"outside_domain", (int64_t)enumeration.outsideDomain},
                   {"memoized_joins", (int64_t)result.memoizedJoins}};
  json::Array assumptions;
  for (const std::string &name : enumeration.assumptionsUsed)
    assumptions.push_back(name);
  json::Array stops;
  for (const StopReport &report : enumeration.stops)
    stops.push_back(json::Object{{"kind", stringifySideKind(report.kind)},
                                 {"status", stringifyStopStatus(report.status)}});
  out["termination"] = json::Object{
      {"verdict", stringifyTerminationVerdict(enumeration.termination.kind())},
      {"reason", enumeration.termination.reason()},
      {"status", enumeration.terminationStatus.name()},
      {"assumptions", std::move(assumptions)},
      {"stops", std::move(stops)}};

  json::Array groups;
  for (unsigned g = 0; g < enumeration.groups.size(); ++g) {
    const SiteGroup &group = enumeration.groups[g];
    json::Array sites;
    for (unsigned id : group.sites) {
      const MacSite &site = program.sites[id];
      sites.push_back(json::Object{
          {"id", (int64_t)id},
          {"reduction", site.reduction == ReductionOp::FAdd ? "fadd" : "add"},
          {"coordinates", namesJSON(site.coordinates)}});
    }
    json::Object entry{{"id", (int64_t)g},
                       {"first", (int64_t)group.first},
                       {"second", (int64_t)group.second},
                       {"sites", std::move(sites)},
                       {"unsupported", group.unsupported}};
    if (group.output)
      entry["output"] = (int64_t)*group.output;
    groups.push_back(std::move(entry));
  }
  out["groups"] = std::move(groups);

  std::map<RecoveryStatus, int64_t> counts;
  json::Array points;
  for (unsigned n = 0; n < result.points.size(); ++n) {
    const PointEvidence &point = enumeration.points[n];
    json::Array results;
    for (const GroupRecovery &recovery : result.points[n].groups) {
      ++counts[recovery.result.kind()];
      json::Object entry{{"group", (int64_t)recovery.group},
                         {"status", stringifyRecoveryStatus(recovery.result.kind())},
                         {"reason", recovery.result.reason()},
                         {"equivalence", recovery.result.equivalence().name()}};
      if (recovery.result.kind() == RecoveryStatus::CandidateFound ||
          recovery.result.kind() == RecoveryStatus::CandidateRefuted)
        entry["report"] = reportJSON(recovery.result.report());
      if (recovery.attempted)
        entry["attempted"] = reportJSON(*recovery.attempted);
      if (recovery.hints)
        entry["hints"] = hintsJSON(*recovery.hints);
      if (recovery.candidate)
        entry["candidate"] = candidateJSON(*recovery.candidate);
      const auto &domain = point.domains[recovery.group];
      if (events && domain)
        entry["events"] = eventsJSON(domain->table());
      results.push_back(std::move(entry));
    }
    points.push_back(json::Object{{"parameters", tupleJSON(point.parameters)},
                                  {"run", stringifyRunStatus(point.run)},
                                  {"reason", point.reason},
                                  {"groups", std::move(results)}});
  }
  out["points"] = std::move(points);
  json::Object summary;
  for (RecoveryStatus status :
       {RecoveryStatus::CandidateFound, RecoveryStatus::CandidateRefuted,
        RecoveryStatus::Unsupported, RecoveryStatus::Unknown})
    summary[stringifyRecoveryStatus(status)] = counts[status];
  out["summary"] = std::move(summary);
  return printed(std::move(out));
}

std::string tableToJSON(const AccessTable &table, const CoordinateCandidate *candidate) {
  CanonicalPartitions partitions;
  viewKernels(table, partitions.kerH, partitions.kerF, partitions.kerG);
  partitions = joinKernels(partitions.kerH, partitions.kerF, partitions.kerG);
  CoordinateCandidate canonical = canonicalCandidate(table, partitions);
  const CoordinateCandidate &chosen = candidate ? *candidate : canonical;
  json::Object out{{"status", stringifyRecoveryStatus(RecoveryStatus::Unknown)},
                   {"reason", "a bare table has no domain evidence"},
                   {"report", reportJSON(inspectCandidate(table, chosen))},
                   {"candidate", candidateJSON(chosen)},
                   {"hints", hintsJSON(partitions)}};
  return printed(std::move(out));
}

} // namespace mlir::generalized_gemm
