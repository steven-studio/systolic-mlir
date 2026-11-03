#include "GeneralizedGemm/SymbolicCandidateChecker.h"

#include "GeneralizedGemm/CoverageSimulation.h"
#include "GeneralizedGemm/SymbolicDomain.h"
#include "GeneralizedGemm/TerminationObligations.h"

#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/StringExtras.h"

#include <algorithm>
#include <set>

using namespace llvm;

namespace mlir::generalized_gemm {

const char *stringifyEvidenceTier(EvidenceTier tier) {
  switch (tier) {
  case EvidenceTier::Proposed: return "PROPOSED";
  case EvidenceTier::Point: return "POINT";
  case EvidenceTier::ScopeEnumerated: return "SCOPE_ENUMERATED";
  case EvidenceTier::SymbolicValid: return "SYMBOLIC_VALID";
  case EvidenceTier::SymbolicExact: return "SYMBOLIC_EXACT";
  }
  return "PROPOSED";
}

const char *stringifyFamilyStatus(FamilyStatus status) {
  switch (status) {
  case FamilyStatus::Verified: return "VERIFIED";
  case FamilyStatus::Refuted: return "REFUTED";
  case FamilyStatus::Unknown: return "UNKNOWN";
  }
  return "UNKNOWN";
}

struct FamilyAuthority {
  static CheckedFamily make(EvidenceTier tier, FamilyStatus status, std::string reason) {
    return CheckedFamily(tier, status, std::move(reason));
  }
  static void facts(CheckedFamily &out, std::vector<PointFacts> facts) {
    out.facts = std::move(facts);
  }
  static void records(CheckedFamily &out, std::vector<ObligationRecord> records) {
    out.obligations = std::move(records);
  }
  static void failing(CheckedFamily &out, std::vector<APInt> point) {
    out.failing = std::move(point);
  }
  static void runs(CheckedFamily &out, bool every) { out.every = every; }
  static void exact(CheckedFamily &out, DomainEvidence evidence, bool box) {
    out.domain = std::move(evidence);
    out.box = box;
  }
};

CheckedFamily CheckedFamily::unknown(std::string reason) {
  return CheckedFamily(EvidenceTier::Proposed, FamilyStatus::Unknown, std::move(reason));
}

namespace {

bool readsShadow(Term term, DenseSet<Term> &seen) {
  if (!seen.insert(term).second)
    return false;
  return term->op == TermOp::Shadow ||
         llvm::any_of(term->args, [&](Term arg) { return readsShadow(arg, seen); });
}

/// Refuses a term that reads anything but parameters and state independent of
/// memory. A value loaded in one iteration can differ in the next, and the
/// symbolic queries share one memory between their two copies: such a term could
/// look like a function of the iteration without being one.
std::optional<CoordinateFamily> adopted(const SemanticProgram &program,
                                        const CoordinateFamily &family, std::string &why) {
  CoordinateFamily out;
  out.origin = family.origin;
  std::vector<bool> independent = memoryIndependentState(program);
  for (unsigned axis = 0; axis < 4; ++axis)
    for (Term term : family.axes[axis]) {
      Term kept = adoptProofTerm(program, term, why);
      if (!kept)
        return std::nullopt;
      DenseSet<Term> seen;
      if (readsShadow(kept, seen)) {
        why = "a coordinate reads a poison shadow";
        return std::nullopt;
      }
      for (unsigned id : stateSupport(kept))
        if (!independent[id]) {
          why = "a coordinate reads a value that depends on memory";
          return std::nullopt;
        }
      out.axes[axis].push_back(kept);
    }
  return out;
}

class Symbolic {
public:
  Symbolic(const SemanticProgram &program, const SiteGroup &group,
           const CoordinateFamily &family, const SymbolicCheckOptions &options,
           SmtBackend &backend)
      : program(program), group(group), family(family), options(options),
        backend(backend), terms(*program.terms) {}

  bool prepare(const TerminationCertificate &certificate, std::string &why);
  bool premises(const TerminationCertificate &certificate,
                std::map<unsigned, Term> &invariants, std::string &why);
  Term scopeTerm() const { return scope; }
  void addDefinitions(const TerminationCertificate &certificate,
                      std::map<unsigned, Term> &invariants);
  bool dischargeValidity(const EnumerationOrder &order);
  bool dischargeCoverage(const EnumerationOrder &order, std::string &why);

  std::vector<ObligationRecord> records;
  std::string failure;
  std::optional<std::vector<APInt>> failing;
  bool injective = true;

private:
  SmtAnswer ask(const std::string &id, const std::string &where, Term formula);
  bool require(const std::string &id, const std::string &where, Term formula);
  std::vector<Term> coordinates() const;
  Term equalOn(const std::vector<Term> &list, const SiteArrival &a, const SiteArrival &b);
  Term differOn(ArrayRef<Term> first, ArrayRef<Term> second, const SiteArrival &a,
                const SiteArrival &b);
  Term pairPremise(const SiteArrival &a, const SiteArrival &b);

  const SemanticProgram &program;
  const SiteGroup &group;
  const CoordinateFamily &family;
  const SymbolicCheckOptions &options;
  SmtBackend &backend;
  TermContext &terms;
  Term scope = nullptr;
  SiteArrivals sites;
  std::vector<unsigned> locations;
  unsigned queries = 0;
};

SmtAnswer Symbolic::ask(const std::string &id, const std::string &where, Term formula) {
  if (queries >= options.limits.maxQueries)
    return SmtAnswer::Unknown;
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
  if (response.answer == SmtAnswer::Sat && id != "prune" && id != "I" && !failing) {
    // A parameter the solver left free may take any value.
    std::vector<APInt> point;
    for (const Parameter &p : program.parameters) {
      auto found = response.model.find("v" + std::to_string(p.id));
      point.push_back(found == response.model.end()
                          ? APInt(p.sort.width, 0)
                          : APInt(p.sort.width, found->second, 10));
    }
    failing = std::move(point);
  }
  // A pruning query is a step of a walk, and its answer is not an obligation.
  if (id != "prune")
    records.push_back(std::move(record));
  return response.answer;
}

bool Symbolic::require(const std::string &id, const std::string &where, Term formula) {
  SmtAnswer answer = ask(id, where, formula);
  if (answer == SmtAnswer::Unsat)
    return true;
  if (failure.empty())
    failure = id + " " + where + ": " + stringifySmtAnswer(answer);
  return false;
}

std::vector<Term> Symbolic::coordinates() const {
  std::vector<Term> all;
  for (const auto &axis : family.axes)
    all.insert(all.end(), axis.begin(), axis.end());
  return all;
}

Term Symbolic::pairPremise(const SiteArrival &a, const SiteArrival &b) {
  return terms.logicalAnd(a.premise, secondCopy(program, b.premise));
}

Term Symbolic::equalOn(const std::vector<Term> &list, const SiteArrival &a,
                       const SiteArrival &b) {
  Term all = terms.boolean(true);
  for (Term t : list)
    all = terms.logicalAnd(all, terms.icmp(Predicate::Eq, atPathEnd(program, a.path, t),
                                           secondCopy(program, atPathEnd(program, b.path, t))));
  return all;
}

Term Symbolic::differOn(ArrayRef<Term> first, ArrayRef<Term> second, const SiteArrival &a,
                        const SiteArrival &b) {
  Term any = terms.boolean(false);
  if (first.size() != second.size())
    return terms.boolean(true);
  for (unsigned n = 0; n < first.size(); ++n)
    any = terms.logicalOr(any, terms.icmp(Predicate::Ne, atPathEnd(program, a.path, first[n]),
                                          secondCopy(program, atPathEnd(program, b.path, second[n]))));
  return any;
}

/// Facts of the form t == T, for a temporary with one definition: a loop bound
/// computed once is unrelated to its inputs in the certificate's invariants. Each
/// is kept only when it holds on every path into its cut, under the kept facts:
/// these are O3 and O4, so they add strength and no trust.
void Symbolic::addDefinitions(const TerminationCertificate &certificate,
                              std::map<unsigned, Term> &invariants) {
  std::map<unsigned, std::set<Term>> defined;
  std::set<unsigned> rejected;
  for (const Transition &t : program.transitions)
    for (const auto &[id, term] : t.assignments) {
      if (program.state[id].sort.kind != Sort::BV || program.state[id].loopCarried)
        rejected.insert(id);
      else if (readsMemory(term) || containsFloat(term) || term->op == TermOp::Const)
        rejected.insert(id);
      else
        defined[id].insert(term);
    }
  std::map<unsigned, std::vector<Term>> facts;
  for (const auto &[id, terms_] : defined)
    if (terms_.size() == 1 && !rejected.count(id))
      for (unsigned cut : certificate.cuts)
        facts[cut].push_back(terms.icmp(Predicate::Eq, terms.state(id, program.state[id].sort),
                                        *terms_.begin()));
  if (facts.empty())
    return;
  PathSet set = enumerateCutPaths(program, certificate.cuts, options.limits.paths);
  if (!set.complete)
    return;
  auto holding = [&](unsigned cut) {
    auto base = invariants.find(cut);
    Term all = base == invariants.end() ? terms.boolean(true) : base->second;
    for (Term fact : facts[cut])
      all = terms.logicalAnd(all, fact);
    return all;
  };
  for (bool changed = true; changed;) {
    changed = false;
    // Only the last pass is the proof of the facts that remain.
    size_t mark = records.size();
    for (const CutPath &path : set.paths) {
      if (path.to == program.exitLocation())
        continue;
      Term source = path.fromEntry ? terms.boolean(true) : holding(path.from);
      std::vector<Term> &targets = facts[path.to];
      for (size_t n = targets.size(); n-- > 0;) {
        if (ask("I", "definition", invariantViolation(program, scope, path, source,
                                                      targets[n])) == SmtAnswer::Unsat)
          continue;
        targets.erase(targets.begin() + n);
        changed = true;
      }
    }
    if (changed)
      records.erase(records.begin() + mark, records.end());
  }
  failing.reset(); // A dropped fact is no failure of the coordinates.
  for (unsigned cut : certificate.cuts)
    invariants[cut] = holding(cut);
}

bool Symbolic::premises(const TerminationCertificate &certificate,
                        std::map<unsigned, Term> &invariants, std::string &why) {
  scope = scopeFormula(program, options.scope, why);
  if (!scope)
    return false;
  for (const auto &[cut, term] : certificate.invariants) {
    Term kept = adoptProofTerm(program, term, why);
    if (!kept)
      return false;
    invariants[cut] = kept;
  }
  addDefinitions(certificate, invariants);
  return true;
}

bool Symbolic::prepare(const TerminationCertificate &certificate, std::string &why) {
  std::map<unsigned, Term> invariants;
  if (!premises(certificate, invariants, why))
    return false;
  std::vector<unsigned> ids(group.sites.begin(), group.sites.end());
  sites = reachSites(program, invariants, certificate.cuts, ids, scope, options.limits.paths);
  if (!sites.complete) {
    why = sites.reason;
    return false;
  }
  for (unsigned id : group.sites)
    locations.push_back(program.sites[id].location);
  return true;
}

bool Symbolic::dischargeValidity(const EnumerationOrder &order) {
  size_t n = sites.arrivals.size();
  if (n * n > options.limits.maxPairs) {
    failure = "more arrival pairs than the limit";
    return false;
  }
  std::vector<Term> all = coordinates();

  // V4: C0 at every event, not only the sampled ones. The update reads the cell it
  // writes, or it is no reduction, whatever the coordinates.
  for (const SiteArrival &a : sites.arrivals) {
    const MacSite &site = program.sites[a.site];
    Term differs = terms.boolean(false);
    for (unsigned n = 0; n < site.output->index.size(); ++n)
      differs = terms.logicalOr(differs,
                                terms.icmp(Predicate::Ne, atPathEnd(program, a.path, site.output->index[n]),
                                           atPathEnd(program, a.path, site.outputRead->index[n])));
    if (!require("V4", "the update reads the cell it writes", terms.logicalAnd(a.premise, differs)))
      return false;
  }

  // V0: each coordinate is defined wherever an event can happen.
  for (const SiteArrival &a : sites.arrivals)
    for (Term term : all) {
      Term defined = atPathEnd(program, a.path, definedness(terms, term));
      if (!require("V0", "definedness", terms.logicalAnd(a.premise, terms.logicalNot(defined))))
        return false;
    }

  auto joined = [&](unsigned x, unsigned y, unsigned z) {
    std::vector<Term> list;
    for (unsigned axis : {x, y, z})
      list.insert(list.end(), family.axes[axis].begin(), family.axes[axis].end());
    return list;
  };
  std::vector<Term> keyH = joined(AxisP, AxisI, AxisJ), keyF = joined(AxisP, AxisI, AxisK),
                    keyG = joined(AxisP, AxisK, AxisJ);
  auto live = controlLiveState(program);

  for (size_t x = 0; x < n; ++x)
    for (size_t y = x; y < n; ++y) {
      const SiteArrival &a = sites.arrivals[x], &b = sites.arrivals[y];
      const MacSite &first = program.sites[a.site], &second = program.sites[b.site];
      Term both = pairPremise(a, b);

      // V1: the three dependences, by two copies with one set of parameters.
      auto dependence = [&](const char *id, const std::vector<Term> &key,
                            ArrayRef<Term> left, ArrayRef<Term> right) {
        return require(id, "dependence",
                       terms.logicalAnd(terms.logicalAnd(both, equalOn(key, a, b)),
                                        differOn(left, right, a, b)));
      };
      if (!dependence("V1h", keyH, first.output->index, second.output->index) ||
          !dependence("V1f", keyF, first.a.index, second.a.index) ||
          !dependence("V1g", keyG, first.b.index, second.b.index))
        return false;

      // V2, by Theorem 7: distinct events differ in control-live state, or are at
      // different sites. So equal coordinates must force the same state.
      Term apart = terms.boolean(false);
      if (a.site != b.site) {
        apart = terms.boolean(true);
      } else {
        for (unsigned id : live[a.location]) {
          Term state = terms.state(id, program.state[id].sort);
          apart = terms.logicalOr(apart, differOn({state}, {state}, a, b));
        }
      }
      // Failure may be spurious, as D-hat can leave a temporary unrelated to the
      // loop that computed it. So it does not stop the check: coverage can prove
      // the bijection on its own. It only withholds SymbolicValid.
      if (!require("V2", "coordinates identify the event",
                   terms.logicalAnd(terms.logicalAnd(both, equalOn(all, a, b)), apart)))
        injective = false;
    }

  // V3: the coordinates stay in the declared set.
  for (const SiteArrival &a : sites.arrivals) {
    std::vector<Term> values = orderValues(program, order, a.path);
    if (!require("V3", "range", terms.logicalAnd(a.premise,
                                                 terms.logicalNot(inOrderSet(program, order, values)))))
      return false;
  }
  return true;
}

bool Symbolic::dischargeCoverage(const EnumerationOrder &order, std::string &why) {
  std::set<unsigned> distinct(locations.begin(), locations.end());
  if (distinct.size() != locations.size()) {
    why = "two sites of the group share a block";
    return false;
  }
  auto question = [&](const std::string &id, const std::string &where, Term formula) {
    return ask(id, where, formula);
  };
  return proveCoverage(program, order, sites, locations, scope,
                       options.limits.maxUnfolding, question, why);
}

} // namespace

ProvenPremises provenPremises(const SemanticProgram &program,
                              const TerminationCertificate &certificate,
                              const SymbolicCheckOptions &options, SmtBackend &backend) {
  static const SiteGroup noGroup;
  static const CoordinateFamily noFamily;
  ProvenPremises out;
  Symbolic symbolic(program, noGroup, noFamily, options, backend);
  try {
    out.ready = symbolic.premises(certificate, out.invariants, out.why);
  } catch (const TermError &error) {
    out.ready = false;
    out.why = std::string("term limit: ") + error.what();
  }
  out.scope = symbolic.scopeTerm();
  out.records = std::move(symbolic.records);
  return out;
}

CheckedFamily checkFamilyFinite(const SemanticProgram &program,
                                const DomainEnumeration &enumeration, unsigned group,
                                const CoordinateFamily &family) {
  if (enumeration.gate != GateStatus::Open)
    return CheckedFamily::unknown("the domain gate is closed: " + enumeration.reason);
  std::string why;
  auto kept = adopted(program, family, why);
  if (!kept)
    return FamilyAuthority::make(EvidenceTier::Proposed, FamilyStatus::Refuted, why);
  std::vector<PointFacts> facts;
  bool complete = !enumeration.sampled;
  for (const PointEvidence &point : enumeration.points) {
    const auto &domain = point.domains[group];
    if (!domain) {
      // A run that stopped or hit a limit leaves this point open.
      if (!point.reason.empty())
        complete = false;
      continue;
    }
    auto refuted = [&](std::string reason) {
      CheckedFamily out = FamilyAuthority::make(EvidenceTier::Proposed, FamilyStatus::Refuted,
                                                std::move(reason));
      FamilyAuthority::failing(out, point.parameters);
      return out;
    };
    auto labels = familyLabels(program, *domain, *kept, why);
    if (!labels)
      return refuted(why);
    CheckedCandidate checked = checkCandidate(*domain, *labels);
    if (checked.kind() != RecoveryStatus::CandidateFound)
      return refuted(checked.reason());
    facts.push_back({point.parameters, checked.report()});
  }
  if (facts.empty())
    return CheckedFamily::unknown("no point has events");
  EvidenceTier tier = complete ? EvidenceTier::ScopeEnumerated : EvidenceTier::Point;
  CheckedFamily out = FamilyAuthority::make(
      tier, FamilyStatus::Verified,
      "the G3a checker accepts these labels at each listed point");
  FamilyAuthority::facts(out, std::move(facts));
  return out;
}

CheckedFamily checkFamilySymbolic(const SemanticProgram &program,
                                  const TerminationCertificate *certificate,
                                  const SiteGroup &group, const CoordinateFamily &family,
                                  const EnumerationOrder *order,
                                  const SymbolicCheckOptions &options, SmtBackend &backend) {
  if (!program.isSupported())
    return CheckedFamily::unknown("the program is unsupported");
  if (!program.controlOblivious || !program.addressOblivious)
    return CheckedFamily::unknown("the trace depends on memory contents");
  if (!group.unsupported.empty() || !group.output)
    return CheckedFamily::unknown(group.unsupported);
  // A claim about part of a group is no claim about its events: the group must be
  // every direct site of this root triple, and nothing else.
  std::set<unsigned> listed(group.sites.begin(), group.sites.end());
  for (const MacSite &site : program.sites) {
    bool direct = site.form == ReductionForm::Direct && site.output && site.outputRead &&
                  site.outputRead->root == site.output->root;
    bool same = direct && site.output->root == *group.output && site.a.root == group.first &&
                site.b.root == group.second;
    if (same != (listed.count(site.id) != 0))
      return CheckedFamily::unknown("the group is not the sites of its roots");
  }
  if (listed.empty())
    return CheckedFamily::unknown("the group has no sites");
  if (!certificate)
    return CheckedFamily::unknown("no termination certificate");
  if (!order || order->levels.empty())
    return CheckedFamily::unknown("no enumeration order to bound the coordinates");
  CheckOptions check;
  check.scope = options.scope;
  check.limits = options.termination;
  check.authorized = options.authorized;
  CheckOutcome outcome = checkCertificate(program, *certificate, check, backend);
  if (outcome.verdict.kind() != TerminationVerdict::ProvenTerminating &&
      outcome.verdict.kind() != TerminationVerdict::ProvenTerminatingIfDefined)
    return CheckedFamily::unknown("the certificate was not accepted: " +
                                  outcome.verdict.reason());
  bool everyRun = outcome.verdict.kind() == TerminationVerdict::ProvenTerminating;

  std::string why;
  auto kept = adopted(program, family, why);
  if (!kept)
    return FamilyAuthority::make(EvidenceTier::Proposed, FamilyStatus::Refuted, why);

  // The order's levels must be exactly the coordinates, or its bijection is
  // about something else.
  EnumerationOrder levels;
  std::multiset<Term> wanted;
  for (const auto &axis : kept->axes)
    wanted.insert(axis.begin(), axis.end());
  for (const OrderLevel &level : order->levels) {
    OrderLevel copy = level;
    copy.value = adoptProofTerm(program, level.value, why);
    copy.lower = adoptProofTerm(program, level.lower, why);
    copy.upper = adoptProofTerm(program, level.upper, why);
    if (!copy.value || !copy.lower || !copy.upper)
      return CheckedFamily::unknown("the order is malformed: " + why);
    if (!stateSupport(copy.lower).empty() || !stateSupport(copy.upper).empty())
      return CheckedFamily::unknown("a bound reads state");
    if (copy.upperOuter >= (int)levels.levels.size())
      return CheckedFamily::unknown("a bound reads a level that is not outer");
    if (copy.lower->sort != copy.value->sort || copy.upper->sort != copy.value->sort ||
        (copy.upperOuter >= 0 &&
         levels.levels[copy.upperOuter].value->sort != copy.value->sort))
      return CheckedFamily::unknown("the order mixes widths");
    auto found = wanted.find(copy.value);
    if (found == wanted.end())
      return CheckedFamily::unknown("a level of the order is no coordinate");
    wanted.erase(found);
    levels.levels.push_back(copy);
  }
  if (!wanted.empty())
    return CheckedFamily::unknown("the order omits a coordinate");

  Symbolic symbolic(program, group, *kept, options, backend);
  try {
    if (!symbolic.prepare(*certificate, why))
      return CheckedFamily::unknown(why);
    if (!symbolic.dischargeValidity(levels)) {
      CheckedFamily out = FamilyAuthority::make(EvidenceTier::Proposed, FamilyStatus::Unknown,
                                                symbolic.failure);
      FamilyAuthority::records(out, std::move(symbolic.records));
      if (symbolic.failing)
        FamilyAuthority::failing(out, *symbolic.failing);
      return out;
    }
    why.clear();
    bool exact = symbolic.dischargeCoverage(levels, why);
    if (!exact && !symbolic.injective) {
      // V2 and S1 to S3 both open: nothing says that coordinates identify events.
      CheckedFamily out = FamilyAuthority::make(EvidenceTier::Proposed, FamilyStatus::Unknown,
                                                symbolic.failure + "; " + why);
      FamilyAuthority::records(out, std::move(symbolic.records));
      if (symbolic.failing)
        FamilyAuthority::failing(out, *symbolic.failing);
      return out;
    }
    EvidenceTier tier = exact ? EvidenceTier::SymbolicExact : EvidenceTier::SymbolicValid;
    CheckedFamily out = FamilyAuthority::make(
        tier, FamilyStatus::Verified,
        exact ? (symbolic.injective
                     ? "V0 to V4 and S1 to S3 discharged"
                     : "V0, V1, V3, V4 and S1 to S3 discharged; the simulation gives the "
                       "bijection that V2 could not")
              : "V0 to V4 discharged; coverage is open: " + why);
    FamilyAuthority::records(out, std::move(symbolic.records));
    FamilyAuthority::runs(out, everyRun);
    if (symbolic.failing && !exact)
      FamilyAuthority::failing(out, *symbolic.failing);
    if (exact) {
      DomainEvidence evidence;
      evidence.form = DomainForm::ExactClosedForm;
      evidence.provenance = "simulation of the order, " + program.fingerprint;
      evidence.scope = options.scope;
      bool admitted = admitsClaim(evidence, DomainClaim::Coverage).admitted;
      // A box needs bounds that depend on batch coordinates alone.
      std::set<Term> batch(kept->axes[AxisP].begin(), kept->axes[AxisP].end());
      bool box = admitted;
      for (const OrderLevel &level : levels.levels)
        if (level.upperOuter >= 0 && !batch.count(levels.levels[level.upperOuter].value))
          box = false;
      FamilyAuthority::exact(out, std::move(evidence), box);
    }
    return out;
  } catch (const TermError &error) {
    return CheckedFamily::unknown(std::string("term limit: ") + error.what());
  }
}

} // namespace mlir::generalized_gemm
