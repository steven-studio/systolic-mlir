#include "GeneralizedGemm/TerminationCertificate.h"

#include "GeneralizedGemm/ControlStructure.h"
#include "GeneralizedGemm/TerminationObligations.h"

#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"

#include <algorithm>
#include <functional>
#include <map>
#include <set>

using namespace mlir::generalized_gemm;

namespace mlir::generalized_gemm {

const char *stringifyTerminationVerdict(TerminationVerdict verdict) {
  switch (verdict) {
  case TerminationVerdict::ProvenTerminating: return "PROVEN_TERMINATING";
  case TerminationVerdict::ProvenTerminatingIfDefined:
    return "PROVEN_TERMINATING_IF_DEFINED";
  case TerminationVerdict::RefutedNonterminating:
    return "REFUTED_NONTERMINATING";
  case TerminationVerdict::Unknown: return "UNKNOWN";
  }
  return "UNKNOWN";
}

const char *stringifyStopStatus(StopStatus status) {
  switch (status) {
  case StopStatus::Unreachable: return "UNREACHABLE";
  case StopStatus::Unknown: return "UNKNOWN";
  case StopStatus::Reachable: return "REACHABLE";
  }
  return "UNKNOWN";
}

CheckedVerdict CheckedVerdict::unknown(std::string reason) {
  return CheckedVerdict(TerminationVerdict::Unknown, std::move(reason));
}

struct CheckerAuthority {
  static CheckedVerdict proven(std::string reason) {
    return CheckedVerdict(TerminationVerdict::ProvenTerminating, std::move(reason));
  }
  static CheckedVerdict provenIfDefined(std::string reason) {
    return CheckedVerdict(TerminationVerdict::ProvenTerminatingIfDefined,
                          std::move(reason));
  }
  static CheckedVerdict refuted(std::string reason) {
    return CheckedVerdict(TerminationVerdict::RefutedNonterminating,
                          std::move(reason));
  }
};

namespace {

/// Whether a float operation anywhere allows results other than the one
/// computed. Wider than the evaluator's report: a dead result still has a stop.
bool hasPermissiveFloat(const SemanticProgram &program) {
  llvm::DenseSet<Term> seen;
  std::function<bool(Term)> visit = [&](Term t) {
    if (!seen.insert(t).second)
      return false;
    if ((t->op == TermOp::FAdd || t->op == TermOp::FMul) &&
        (t->aux & (FlagReassoc | FlagContract | FlagARCFP | FlagAFN)))
      return true;
    return llvm::any_of(t->args, visit);
  };
  for (const Transition &t : program.transitions) {
    for (const auto &[id, term] : t.assignments)
      if (visit(term))
        return true;
    for (const auto &[id, term] : t.memoryUpdates)
      if (visit(term))
        return true;
    for (const SideCondition &side : t.sideConditions)
      if (visit(side.violated))
        return true;
    for (Term term : t.returns)
      if (visit(term))
        return true;
  }
  return false;
}

constexpr const char *DomainPremise =
    "domain: defined parameters, fully initialized argument memory";

class Checker {
public:
  Checker(const SemanticProgram &program, const TerminationCertificate &certificate,
          const CheckOptions &options, SmtBackend &backend)
      : program(program), certificate(certificate), options(options),
        backend(backend), terms(*program.terms) {}

  CheckOutcome run();

private:
  // O0.
  bool structural(const std::string &what, bool passed, const std::string &detail);
  bool adoptInvariants();
  bool adoptLevels();
  bool checkLevels(llvm::ArrayRef<unsigned> current,
                   const std::vector<RankLevel> &levels);
  // O1 to O7.
  bool discharge(ObligationRecord record, Term formula, SmtResponse *answer = nullptr);
  bool dischargeDomain();
  bool dischargeGuards();
  bool dischargeInvariants();
  bool dischargeLevel(const RankLevel &level);
  void dischargeStops();
  bool assumptionsAuthorized() const;

  Term invariantAt(unsigned location) const;
  std::vector<std::string> premises(const std::vector<unsigned> &cuts) const;
  std::string rankText(const RankLevel &level) const;

  const SemanticProgram &program;
  const TerminationCertificate &certificate;
  const CheckOptions &options;
  SmtBackend &backend;
  TermContext &terms;

  CheckOutcome outcome;
  PathSet set;
  Term scope = nullptr;
  std::map<unsigned, Term> invariants;
  std::map<llvm::SmallVector<unsigned>, unsigned> pathIndex;
  // Adopted ranks and resolved path ids, by RankLevel address.
  std::map<const RankLevel *, std::map<unsigned, Term>> adoptedRank;
  std::map<const RankLevel *, std::vector<unsigned>> levelPaths;
  std::map<const RankLevel *, std::set<unsigned>> levelStrict;
  unsigned queries = 0;
  bool stopsUseMemory = false;
};

bool Checker::assumptionsAuthorized() const {
  for (const ProofStatus &assumption : program.assumptions)
    if (!isDischarged(assumption, options.authorized))
      return false;
  return true;
}

bool Checker::structural(const std::string &what, bool passed,
                         const std::string &detail) {
  ObligationRecord record;
  record.id = "O0";
  record.location = what;
  record.expected = "structural";
  record.result = passed ? "pass" : "fail";
  record.evidence = detail;
  record.discharged = passed;
  outcome.records.push_back(std::move(record));
  return passed;
}

Term Checker::invariantAt(unsigned location) const {
  auto found = invariants.find(location);
  return found == invariants.end() ? terms.boolean(true) : found->second;
}

std::vector<std::string> Checker::premises(const std::vector<unsigned> &cuts) const {
  std::vector<std::string> list{DomainPremise};
  if (!options.scope.allParameters)
    list.push_back("parameter scope");
  for (unsigned cut : cuts)
    list.push_back("invariant at L" + std::to_string(cut));
  return list;
}

std::string Checker::rankText(const RankLevel &level) const {
  std::string out;
  for (const auto &[cut, rank] : adoptedRank.at(&level))
    out += "rank at L" + std::to_string(cut) + " = " + formatTerm(rank, 8) + " ";
  return out;
}

bool Checker::discharge(ObligationRecord record, Term formula, SmtResponse *answer) {
  bool usesMemory = readsMemory(formula);
  // O7 speaks only about stops: dischargeStops accounts for it separately.
  if (record.id != "O7")
    outcome.dependsOnMemory = outcome.dependsOnMemory || usesMemory;
  if (usesMemory)
    for (const ProofStatus &assumption : program.assumptions)
      record.assumptions.push_back(assumption.detail());
  std::string comment = record.id + " " + record.location;
  record.script = obligationScript(comment, formula);
  if (record.expected.empty())
    record.expected = "unsat";
  if (queries >= options.limits.maxCheckerQueries) {
    record.result = "not-run";
    record.evidence = "checker query limit";
    outcome.records.push_back(std::move(record));
    return false;
  }
  ++queries;
  SmtResponse response = backend.check(record.script, options.limits.solver,
                                       /*wantModel=*/true);
  record.result = stringifySmtAnswer(response.answer);
  bool wanted = response.answer == (record.expected == "unsat" ? SmtAnswer::Unsat
                                                               : SmtAnswer::Sat);
  // An unknown answer is never expected, and only an exact answer discharges.
  record.discharged = wanted;
  if (response.answer == SmtAnswer::Sat && !wanted) {
    std::string model = "counterexample:";
    for (const auto &[name, value] : response.model)
      model += " " + name + "=" + value;
    record.evidence = model;
  } else if (response.answer == SmtAnswer::Unknown) {
    record.evidence = response.detail;
  }
  outcome.records.push_back(std::move(record));
  if (answer)
    *answer = std::move(response);
  return wanted;
}

bool Checker::adoptInvariants() {
  std::set<unsigned> seen;
  for (const auto &[cut, term] : certificate.invariants) {
    if (!std::binary_search(certificate.cuts.begin(), certificate.cuts.end(), cut) ||
        !seen.insert(cut).second)
      return structural("invariants", false,
                        "an invariant names no cut, or repeats one");
    std::string why;
    Term adopted = adoptProofTerm(program, term, why);
    if (!adopted || adopted->sort != Sort::bv(1))
      return structural("invariants", false,
                        why.empty() ? "an invariant is not boolean" : why);
    invariants[cut] = adopted;
  }
  return structural("invariants", true, "well-sorted, one per named cut");
}

bool Checker::checkLevels(llvm::ArrayRef<unsigned> current,
                          const std::vector<RankLevel> &levels) {
  // The SCCs of the current graph, computed here: never read from the data.
  llvm::SmallVector<std::pair<unsigned, unsigned>> edges;
  for (unsigned id : current)
    edges.push_back({set.paths[id].from, set.paths[id].to});
  auto components = cyclicComponents(program.locationCount, edges);
  if (components.size() != levels.size())
    return structural("levels", false,
                      "the current graph has " + std::to_string(components.size()) +
                          " cyclic SCCs and the certificate covers " +
                          std::to_string(levels.size()));
  std::set<const RankLevel *> used;
  for (const auto &component : components) {
    const RankLevel *match = nullptr;
    for (const RankLevel &level : levels)
      if (level.cuts == component && !used.count(&level))
        match = &level;
    if (!match)
      return structural("levels", false, "an SCC has no proof node");
    used.insert(match);
    const RankLevel &level = *match;

    std::vector<unsigned> inside;
    for (unsigned id : current) {
      const CutPath &path = set.paths[id];
      if (llvm::is_contained(component, path.from) &&
          llvm::is_contained(component, path.to))
        inside.push_back(id);
    }
    std::set<unsigned> listed;
    for (const auto &transitions : level.paths) {
      auto found = pathIndex.find(transitions);
      if (found == pathIndex.end() || !listed.insert(found->second).second)
        return structural("levels", false, "a listed path does not exist");
    }
    if (listed != std::set<unsigned>(inside.begin(), inside.end()))
      return structural("levels", false,
                        "a proof node does not list exactly the paths of its SCC");

    std::set<unsigned> strict;
    for (const auto &transitions : level.strict) {
      auto found = pathIndex.find(transitions);
      if (found == pathIndex.end() || !listed.count(found->second) ||
          !strict.insert(found->second).second)
        return structural("levels", false, "a strict path is not in its SCC");
    }
    // The SCC would otherwise recur unchanged until the nodes run out, and be
    // refused there. Refusing here names the node at fault.
    if (strict.empty())
      return structural("levels", false, "a proof node removes no path");

    std::map<unsigned, Term> ranks;
    unsigned width = 0;
    for (const auto &[cut, term] : level.rank) {
      if (!llvm::is_contained(component, cut) || ranks.count(cut))
        return structural("levels", false, "a rank names no cut of its SCC");
      std::string why;
      Term adopted = adoptProofTerm(program, term, why);
      if (!adopted)
        return structural("levels", false, why);
      if (adopted->sort.kind != Sort::BV || (width && adopted->sort.width != width))
        return structural("levels", false, "ranks of one SCC differ in width");
      width = adopted->sort.width;
      ranks[cut] = adopted;
    }
    if (ranks.size() != component.size())
      return structural("levels", false, "a cut of an SCC has no rank");

    adoptedRank[&level] = std::move(ranks);
    levelPaths[&level] = inside;
    levelStrict[&level] = strict;
    std::vector<unsigned> rest;
    for (unsigned id : inside)
      if (!strict.count(id))
        rest.push_back(id);
    if (!checkLevels(rest, level.children))
      return false;
  }
  return true;
}

bool Checker::adoptLevels() {
  for (const CutPath &path : set.paths)
    pathIndex[path.transitions] = path.id;
  std::vector<unsigned> everyCutPath;
  for (const CutPath &path : set.paths)
    if (!path.fromEntry && path.to != program.exitLocation())
      everyCutPath.push_back(path.id);
  if (!checkLevels(everyCutPath, certificate.levels))
    return false;
  return structural("levels", true,
                    "every cyclic SCC of every residual path graph has a node "
                    "that lists its paths and strictly removes at least one");
}

bool Checker::dischargeDomain() {
  ObligationRecord record;
  record.id = "O1";
  record.location = "parameter domain";
  record.expected = "sat";
  record.assumptions = {DomainPremise};
  return discharge(std::move(record), scope);
}

bool Checker::dischargeGuards() {
  bool all = true;
  for (unsigned location = 0; location < program.locationCount; ++location) {
    Term formula = guardViolation(program, location);
    if (!formula)
      return structural("guards", false,
                        "L" + std::to_string(location) + " has no transition");
    ObligationRecord record;
    record.id = "O2";
    record.location = "L" + std::to_string(location);
    all = discharge(std::move(record), formula) && all;
  }
  return all;
}

bool Checker::dischargeInvariants() {
  bool all = true;
  for (const CutPath &path : set.paths) {
    if (path.to == program.exitLocation())
      continue;
    ObligationRecord record;
    record.id = path.fromEntry ? "O3" : "O4";
    record.location = describePath(program, path);
    record.assumptions = premises(path.fromEntry ? std::vector<unsigned>{}
                                                 : std::vector<unsigned>{path.from});
    Term target = invariantAt(path.to);
    record.evidence = "invariant at L" + std::to_string(path.to) + " = " +
                      formatTerm(target, 8);
    Term source = path.fromEntry ? terms.boolean(true) : invariantAt(path.from);
    all = discharge(std::move(record),
                    invariantViolation(program, scope, path, source, target)) &&
          all;
  }
  return all;
}

bool Checker::dischargeLevel(const RankLevel &level) {
  const auto &ranks = adoptedRank.at(&level);
  const auto &inside = levelPaths.at(&level);
  const auto &strict = levelStrict.at(&level);
  std::string scc = "SCC {";
  for (unsigned cut : level.cuts)
    scc += " L" + std::to_string(cut);
  scc += " }";
  bool all = true;
  for (unsigned id : inside) {
    const CutPath &path = set.paths[id];
    bool isStrict = strict.count(id) != 0;
    ObligationRecord record;
    record.id = isStrict ? "O5" : "O6";
    record.location = scc + " path " + describePath(program, path);
    record.assumptions = premises({path.from});
    record.evidence = rankText(level);
    Term formula = descentViolation(
        program, set, scope, path, invariantAt(path.from), inside,
        ranks.at(path.from), ranks.at(path.to), isStrict);
    all = discharge(std::move(record), formula) && all;
  }
  for (const RankLevel &child : level.children)
    all = dischargeLevel(child) && all;
  return all;
}

void Checker::dischargeStops() {
  for (unsigned k = 0; k < 3; ++k) {
    outcome.stops[k] = StopReport();
    outcome.stops[k].kind = (SideKind)k;
  }
  bool permissive = hasPermissiveFloat(program);
  for (const StopSite &site : set.sites) {
    StopReport &report = outcome.stops[(unsigned)site.kind];
    ++report.sites;
    ObligationRecord record;
    record.id = "O7";
    record.location = "L" + std::to_string(site.location) + " " + site.reason +
                      " (" + stringifySideKind(site.kind) + "), reached from " +
                      (site.fromEntry ? "entry" : "L" + std::to_string(site.from));
    record.assumptions =
        premises(site.fromEntry ? std::vector<unsigned>{}
                                : std::vector<unsigned>{site.from});
    Term source = site.fromEntry ? terms.boolean(true) : invariantAt(site.from);
    std::string where = record.location;
    Term formula = stopViolation(program, scope, site, source);
    bool usesMemory = readsMemory(formula);
    if (usesMemory && !assumptionsAuthorized()) {
      // Without separation this stop cannot be excluded. It stays unknown, and
      // the termination theorem, which does not rest on it, is unaffected.
      record.expected = "unsat";
      record.result = "not-run";
      record.evidence = "needs an unauthorized assumption";
      outcome.records.push_back(std::move(record));
      report.status = StopStatus::Unknown;
      if (report.detail.empty())
        report.detail = where + ": needs an unauthorized assumption";
    } else if (discharge(std::move(record), formula)) {
      ++report.discharged;
      stopsUseMemory = stopsUseMemory || usesMemory;
      // Discharged for the strict evaluation only. Reassociation may produce a
      // value, such as a NaN, that the expression as written does not.
      if (permissive && containsFloat(site.violated)) {
        report.status = StopStatus::Unknown;
        if (report.detail.empty())
          report.detail = where + ": strict evaluation only, the flags allow others";
      }
    } else {
      report.status = StopStatus::Unknown;
      if (report.detail.empty())
        report.detail = where;
    }
  }
  // The relation leaves an uninitialized read free, and it is no side
  // condition, so no obligation above sees it. A fresh root is uninitialized.
  for (const Transition &t : program.transitions)
    for (const MemoryEffect &effect : t.effects)
      if (effect.kind == MemoryEffect::Read &&
          program.roots[effect.root].origin == MemoryRoot::FreshAllocation) {
        StopReport &report = outcome.stops[(unsigned)SideKind::Underspecified];
        report.status = StopStatus::Unknown;
        if (report.detail.empty())
          report.detail = "a read of a fresh allocation: initialization is not "
                          "tracked";
      }
}

CheckOutcome Checker::run() {
  auto stopWith = [&](std::string reason) {
    outcome.verdict = CheckedVerdict::unknown(reason);
    outcome.status = ProofStatus::unknown(std::move(reason));
    return std::move(outcome);
  };
  for (unsigned k = 0; k < 3; ++k) {
    outcome.stops[k].kind = (SideKind)k;
    outcome.stops[k].status = StopStatus::Unknown;
  }
  if (!structural("model", program.isSupported(),
                  program.isSupported() ? "supported" : program.unsupported->detail))
    return stopWith("the program is unsupported");
  if (!structural("fingerprint", certificate.fingerprint == program.fingerprint,
                  "certificate " + certificate.fingerprint))
    return stopWith("the certificate is for another program");
  std::string why;
  scope = scopeFormula(program, options.scope, why);
  if (!structural("scope", scope != nullptr, why))
    return stopWith("malformed scope: " + why);
  if (!structural("cut set", cutSetBreaksCycles(program, certificate.cuts, why), why))
    return stopWith("the cut set does not cover every cycle: " + why);
  set = enumerateCutPaths(program, certificate.cuts, options.limits.paths);
  if (!structural("paths", set.complete, set.reason))
    return stopWith("path enumeration: " + set.reason);
  try {
    if (!adoptInvariants())
      return stopWith("malformed invariants");
    if (!adoptLevels())
      return stopWith("the rank levels do not cover the graph");
    if (!dischargeDomain())
      return stopWith("the parameter domain is empty or unknown");
    bool proof = dischargeGuards();
    proof = dischargeInvariants() && proof;
    for (const RankLevel &level : certificate.levels)
      proof = dischargeLevel(level) && proof;
    if (!proof)
      return stopWith("an obligation was not discharged");
    dischargeStops();
  } catch (const TermError &error) {
    return stopWith(std::string("term limit: ") + error.what());
  }

  // O0 to O6 give: no run is infinite. An unauthorized assumption they use
  // leaves no theorem; one that only a stop needs was refused above.
  bool everyStopUnreachable = true;
  for (const StopReport &report : outcome.stops)
    everyStopUnreachable = everyStopUnreachable &&
                           report.status == StopStatus::Unreachable;

  ProofStatus status = ProofStatus::proven("O0-O7 of " + program.fingerprint);
  if (outcome.dependsOnMemory || stopsUseMemory)
    for (const ProofStatus &assumption : program.assumptions) {
      outcome.assumptionsUsed.push_back(assumption.detail());
      status = conjoin(status, assumption);
      if (!isDischarged(assumption, options.authorized))
        return stopWith("needs the unauthorized assumption " + assumption.detail());
    }
  outcome.status = status;
  outcome.verdict =
      everyStopUnreachable
          ? CheckerAuthority::proven("O0-O7 discharged")
          : CheckerAuthority::provenIfDefined(
                "O0-O6 discharged; a stop was not shown unreachable");
  return std::move(outcome);
}

bool sameCells(const std::vector<RootData> &a, const std::vector<RootData> &b) {
  if (a.size() != b.size())
    return false;
  for (unsigned r = 0; r < a.size(); ++r)
    if (a[r].shape != b[r].shape || a[r].cells != b[r].cells)
      return false;
  return true;
}

} // namespace

bool sameConfiguration(const Configuration &a, const Configuration &b) {
  return a.location == b.location && a.state == b.state && a.shadow == b.shadow &&
         sameCells(a.memory, b.memory);
}

CheckOutcome checkCertificate(const SemanticProgram &program,
                              const TerminationCertificate &certificate,
                              const CheckOptions &options, SmtBackend &backend) {
  Checker checker(program, certificate, options, backend);
  return checker.run();
}

ReplayReport replayEntry(const SemanticProgram &program, const EntryState &entry,
                         uint64_t fuel, uint64_t maxCells) {
  ReplayReport report;
  RunLimits limits;
  limits.maxCells = maxCells;
  auto start = initialConfiguration(program, entry, limits, report.reason);
  if (!start)
    return report;
  uint64_t steps = 0;
  // One step; false when the run ended, with `report` set.
  auto advance = [&](Configuration &config) {
    if (config.location == program.exitLocation()) {
      report.end = ReplayEnd::Returned;
      return false;
    }
    if (steps >= fuel) {
      report.end = ReplayEnd::OutOfFuel;
      report.reason = "fuel exhausted: this proves nothing";
      return false;
    }
    ++steps;
    report.steps = steps;
    StepOutcome step = stepConfiguration(program, entry, config);
    if (step.stopped) {
      report.end = ReplayEnd::Stopped;
      report.stop = *step.stopped;
      report.reason = step.reason;
      return false;
    }
    return true;
  };

  // Brent: the saved configuration moves at powers of two, so memory stays
  // constant and the comparison is always between complete configurations.
  Configuration saved = *start, hare = *start;
  uint64_t power = 1, period = 0;
  while (true) {
    if (!advance(hare))
      return report;
    ++period;
    if (sameConfiguration(saved, hare))
      break;
    if (period == power) {
      saved = hare;
      power *= 2;
      period = 0;
    }
  }
  // The first repeated position: two walkers `period` apart meet there.
  Configuration lead = *start, trail = *start;
  for (uint64_t i = 0; i < period; ++i)
    if (!advance(lead))
      return report;
  uint64_t prefix = 0;
  while (!sameConfiguration(lead, trail)) {
    if (!advance(lead) || !advance(trail))
      return report;
    ++prefix;
  }
  report.end = ReplayEnd::Cycle;
  report.witness = NonterminationWitness{entry, prefix, period};
  return report;
}

CheckedVerdict verifyNonterminationWitness(const SemanticProgram &program,
                                           const ParameterScope &scope,
                                           const NonterminationWitness &witness,
                                           uint64_t maxCells) {
  if (!program.isSupported())
    return CheckedVerdict::unknown("the program is unsupported");
  // Lowering drops preconditions of the structured source, such as a positive
  // scf.for step, so an entry of the lowered CFG may be no entry of the source.
  // A proof covers those entries too, and stands. A refutation would not.
  if (program.structuredLoweringApplied)
    return CheckedVerdict::unknown(
        "a cycle of the lowered CFG, which may break a source precondition");
  if (witness.period == 0)
    return CheckedVerdict::unknown("a witness needs a positive period");
  if (!parametersInScope(scope, witness.entry.parameters))
    return CheckedVerdict::unknown("the witness entry is outside the domain");
  RunLimits limits;
  limits.maxCells = maxCells;
  std::string error;
  auto config = initialConfiguration(program, witness.entry, limits, error);
  if (!config)
    return CheckedVerdict::unknown("the witness entry is invalid: " + error);
  auto advance = [&]() {
    if (config->location == program.exitLocation())
      return false;
    return !stepConfiguration(program, witness.entry, *config).stopped.has_value();
  };
  for (uint64_t i = 0; i < witness.prefix; ++i)
    if (!advance())
      return CheckedVerdict::unknown("the witness run ended in its prefix");
  Configuration anchor = *config;
  for (uint64_t i = 0; i < witness.period; ++i)
    if (!advance())
      return CheckedVerdict::unknown("the witness run ended in its period");
  if (!sameConfiguration(anchor, *config))
    return CheckedVerdict::unknown("the witness does not repeat a configuration");
  return CheckerAuthority::refuted(
      "exact replay repeats a complete configuration after " +
      std::to_string(witness.prefix) + " steps, with period " +
      std::to_string(witness.period));
}

} // namespace mlir::generalized_gemm
