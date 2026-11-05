//===- DomainChecker.cpp --------------------------------------------------===//
#include "GeneralizedGemm/DomainChecker.h"

#include "GeneralizedGemm/ReductionSemantics.h"
#include "GeneralizedGemm/SymbolicDomain.h"
#include "GeneralizedGemm/TerminationObligations.h"

#include "llvm/ADT/DenseSet.h"

#include <algorithm>
#include <functional>
#include <map>
#include <set>

using llvm::APInt;
using llvm::ArrayRef;

namespace mlir::generalized_gemm {

struct DomainAuthority {
  static CheckedDomain make(ComputationTier tier, ComputationStatus status, std::string reason) {
    return CheckedDomain(tier, status, std::move(reason));
  }
  static CheckedDomain &records(CheckedDomain &out, std::vector<ObligationRecord> list) {
    out.obligations = std::move(list);
    return out;
  }
  static void bind(CheckedDomain &out, std::string binding) { out.bound = std::move(binding); }
  static void facts(CheckedDomain &out, std::vector<DomainPointFacts> list) { out.facts = std::move(list); }
  static void orbit(CheckedDomain &out, bool value) { out.orbit = value; }
  static void multiplicity(CheckedDomain &out, DomainMultiplicity form, Term mu) {
    out.form = form;
    out.mu = mu;
  }
  static void key(CheckedDomain &out, Fact fact) { out.key = std::move(fact); }
  static void collision(CheckedDomain &out, Fact fact) { out.collision = std::move(fact); }
  static void access(CheckedDomain &out, unsigned view, Fact fact) {
    out.accessFacts[view] = std::move(fact);
  }
  static void single(CheckedDomain &out, Fact fact) { out.single = std::move(fact); }
  static void counter(CheckedDomain &out, Fact fact) { out.counter = std::move(fact); }
  static void failing(CheckedDomain &out, std::vector<APInt> point) { out.failing = std::move(point); }
};

CheckedDomain CheckedDomain::unknown(std::string reason) {
  return CheckedDomain(ComputationTier::Proposed, ComputationStatus::Unknown, std::move(reason));
}

namespace {

Fact proven(std::string detail, ComputationTier tier) {
  Fact f;
  f.status = FactStatus::Proven;
  f.detail = std::move(detail);
  f.tier = (int)tier;
  return f;
}

Fact refutedAt(const std::vector<APInt> &point, std::string detail) {
  Fact f;
  f.status = FactStatus::Refuted;
  f.detail = std::move(detail);
  f.point = point;
  return f;
}

Fact unknownFact(std::string detail) {
  Fact f;
  f.detail = std::move(detail);
  return f;
}

bool groupIsWhole(const SemanticProgram &program, const SiteGroup &group, std::string &why) {
  if (!group.unsupported.empty()) {
    why = group.unsupported;
    return false;
  }
  if (!group.output || group.sites.empty()) {
    why = "the group has no output in memory";
    return false;
  }
  std::set<unsigned> listed(group.sites.begin(), group.sites.end());
  for (const MacSite &site : program.sites) {
    bool direct = site.form == ReductionForm::Direct && site.output && site.outputRead &&
                  site.outputRead->root == site.output->root;
    bool same = direct && site.output->root == *group.output && site.a.root == group.first &&
                site.b.root == group.second;
    if (same != (listed.count(site.id) != 0)) {
      why = "the group is not the sites of its roots";
      return false;
    }
  }
  return true;
}

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

/// What a claim is about: program, group, scope, names and the request itself.
std::string bindingOf(const SemanticProgram &program, const SiteGroup &group,
                      const DomainOptions &options, const CoordinateFamily &family,
                      const DomainRequest &request) {
  std::string out = program.fingerprint + "|" +
                    std::to_string(group.output ? (int64_t)*group.output : -1) + "," +
                    std::to_string(group.first) + "," + std::to_string(group.second) + "|";
  for (unsigned site : group.sites)
    out += std::to_string(site) + ",";
  out += options.scope.allParameters ? "|all|" : "|some|";
  for (const ParameterRange &range : options.scope.ranges)
    out += std::to_string(range.parameter) + ":" + std::to_string(range.lower.getSExtValue()) +
           ":" + std::to_string(range.upper.getSExtValue()) + ",";
  for (const std::string &name : options.authorized)
    out += "|" + name;
  for (const GuardedLevel &level : request.order.levels)
    out += "|" + formatTerm(level.value) + ";" + formatTerm(level.lower) + ";" +
           formatTerm(level.upper) + ";" + (level.descending ? "d" : "a") + (level.isSigned ? "s" : "u");
  if (request.order.guard)
    out += "|g" + formatTerm(request.order.guard);
  out += "|k";
  for (unsigned k : request.cellKey)
    out += std::to_string(k) + ",";
  for (const auto &form : request.access) {
    out += "|a";
    for (Term term : form)
      out += formatTerm(term) + ";";
  }
  for (const auto &axis : family.axes) {
    out += "|f";
    for (Term term : axis)
      out += formatTerm(term) + ";";
  }
  return out;
}

bool keyIsWellFormed(const GuardedOrder &order, const std::vector<unsigned> &key) {
  if (key.empty())
    return false;
  for (unsigned n = 0; n < key.size(); ++n)
    if (key[n] >= order.levels.size() || (n && key[n] <= key[n - 1]))
      return false;
  return true;
}

/// The key as an order of its own, for comparisons of keys alone.
GuardedOrder keyOrder(const GuardedOrder &order, const std::vector<unsigned> &key) {
  GuardedOrder out;
  for (unsigned k : key)
    out.levels.push_back(order.levels[k]);
  return out;
}

/// The view index terms of a site: 0 is h, 1 is f, 2 is g.
ArrayRef<Term> viewOf(const MacSite &site, unsigned view) {
  if (view == 0)
    return site.output->index;
  return view == 1 ? ArrayRef<Term>(site.a.index) : ArrayRef<Term>(site.b.index);
}

/// μ as Π max(hi - lo, 0) over the repeat levels, at 128 bits so it cannot wrap. Only for
/// a box: repeat bounds read parameters alone, and nothing else reads a repeat level.
Term constantBox(const SemanticProgram &program, const GuardedOrder &order,
                 const std::vector<bool> &repeat) {
  TermContext &terms = *program.terms;
  std::set<unsigned> repeatLevels;
  for (unsigned m = 0; m < repeat.size(); ++m)
    if (repeat[m])
      repeatLevels.insert(m);
  auto readsRepeat = [&](Term term) {
    llvm::DenseSet<Term> seen;
    std::function<bool(Term)> visit = [&](Term n) {
      if (!seen.insert(n).second)
        return false;
      auto level = levelOf(n);
      if (level && repeatLevels.count(*level))
        return true;
      return llvm::any_of(n->args, visit);
    };
    return visit(term);
  };
  auto readsAnyLevel = [&](Term term) {
    llvm::DenseSet<Term> seen;
    std::function<bool(Term)> visit = [&](Term n) {
      if (!seen.insert(n).second)
        return false;
      return levelOf(n).has_value() || llvm::any_of(n->args, visit);
    };
    return visit(term);
  };
  if (readsRepeat(order.guard))
    return nullptr;
  // Each span is below 2^w, so the product is below 2^(sum of widths): at most 127
  // bits, or this is no constant at 128 bits.
  unsigned bitsUsed = 0;
  for (unsigned m : repeatLevels)
    bitsUsed += order.levels[m].value->sort.width;
  if (bitsUsed >= 128)
    return nullptr;
  Term product = terms.bv(APInt(128, 1));
  for (unsigned m = 0; m < order.levels.size(); ++m) {
    const GuardedLevel &level = order.levels[m];
    if (!repeatLevels.count(m)) {
      if (readsRepeat(level.lower) || readsRepeat(level.upper))
        return nullptr;
      continue;
    }
    if (readsAnyLevel(level.lower) || readsAnyLevel(level.upper))
      return nullptr;
    Term lo = level.isSigned ? terms.sext(level.lower, 128) : terms.zext(level.lower, 128);
    Term hi = level.isSigned ? terms.sext(level.upper, 128) : terms.zext(level.upper, 128);
    Term span = terms.ite(terms.icmp(Predicate::Slt, lo, hi), terms.binary(TermOp::Sub, hi, lo),
                          terms.bv(APInt(128, 0)));
    product = terms.binary(TermOp::Mul, product, span);
  }
  return product;
}

struct Structure {
  GuardedOrder order;
  std::vector<bool> repeat;
  DomainMultiplicity form = DomainMultiplicity::Counted;
  Term mu = nullptr;
  std::vector<unsigned> key;
  bool keyAll = false;
};

std::optional<Structure> adoptRequest(const SemanticProgram &program, const CoordinateFamily &family,
                                      const DomainRequest &request, std::string &why) {
  Structure out;
  auto order = adoptGuardedOrder(program, family, request.order, out.repeat, why);
  if (!order)
    return std::nullopt;
  out.order = std::move(*order);
  out.key = request.cellKey.empty() ? std::vector<unsigned>() : request.cellKey;
  if (out.key.empty())
    for (unsigned m = 0; m < out.order.levels.size(); ++m)
      out.key.push_back(m);
  if (!keyIsWellFormed(out.order, out.key)) {
    why = "a malformed cell key";
    return std::nullopt;
  }
  out.keyAll = out.key.size() == out.order.levels.size();
  for (unsigned view = 0; view < 3; ++view)
    for (Term term : request.access[view])
      if (!isLevelTerm(program, out.order, term, out.order.levels.size(), why))
        return std::nullopt;
  bool anyRepeat = llvm::is_contained(out.repeat, true);
  if (!anyRepeat) {
    out.form = DomainMultiplicity::Unit;
  } else if ((out.mu = constantBox(program, out.order, out.repeat))) {
    out.form = DomainMultiplicity::ConstantBox;
  }
  return out;
}

/// The logical point of an event, (P, I, J, K), and its output key (P, I, J).
std::vector<Term> axesOf(const CoordinateFamily &family, std::initializer_list<unsigned> axes,
                         const SemanticProgram &program, std::string &why) {
  std::vector<Term> out;
  for (unsigned axis : axes)
    for (Term term : family.axes[axis])
      if (Term kept = adoptProofTerm(program, term, why))
        out.push_back(kept);
  return out;
}

CheckedDomain finiteClaim(const SemanticProgram &program, const DomainEnumeration &enumeration,
                          unsigned g, const CoordinateFamily &family, const DomainRequest &request,
                          const DomainOptions &options) {
  if (enumeration.gate != GateStatus::Open)
    return CheckedDomain::unknown("the domain gate is closed: " + enumeration.reason);
  if (g >= enumeration.groups.size())
    return CheckedDomain::unknown("no such group");
  const SiteGroup &group = enumeration.groups[g];
  std::string why;
  if (!groupIsWhole(program, group, why) || !traceIsFixed(program, why))
    return CheckedDomain::unknown(why);
  auto structure = adoptRequest(program, family, request, why);
  if (!structure)
    return CheckedDomain::unknown("the request is refused: " + why);
  const GuardedOrder &order = structure->order;
  std::vector<Term> logical = axesOf(family, {AxisP, AxisI, AxisJ, AxisK}, program, why);
  std::vector<Term> output = axesOf(family, {AxisP, AxisI, AxisJ}, program, why);
  GuardedOrder keyed = keyOrder(order, structure->key);

  std::vector<DomainPointFacts> facts;
  bool complete = enumerationCoversScope(program, enumeration, options.scope);
  bool keyHolds = true, injective = true, single = true;
  std::array<bool, 3> accessHolds{true, true, true};
  std::optional<Fact> keyBroken, collisionSeen, multipleSeen;
  std::array<std::optional<Fact>, 3> accessBroken;
  std::string open;
  bool anyEvent = false;
  auto refute = [&](const std::vector<APInt> &point, std::string detail) {
    CheckedDomain out = DomainAuthority::make(ComputationTier::Proposed, ComputationStatus::Refuted,
                                              "the order does not describe the events: " + detail);
    DomainAuthority::counter(out, refutedAt(point, detail));
    return out;
  };
  for (const PointEvidence &point : enumeration.points) {
    if (!point.reason.empty()) {
      complete = false;
      continue;
    }
    const std::vector<APInt> &v = point.parameters;
    auto set = enumerateGuardedSet(order, v, options.limits.maxBox, why);
    if (!set) {
      complete = false;
      if (open.empty())
        open = why;
      continue;
    }
    DomainPointFacts here;
    here.parameters = v;
    here.points = set->size();
    std::vector<std::vector<APInt>> lambdas, logicals, outputs;
    std::vector<IndexTuple> cellsOf;
    std::array<std::vector<IndexTuple>, 3> views;
    const auto &domain = point.domains[g];
    if (domain) {
      const AccessTable &table = domain->table();
      EventEvaluator values(program, *domain);
      bool valued = true;
      auto tuple = [&](ArrayRef<Term> list, size_t e) {
        std::vector<APInt> out;
        for (Term term : list) {
          auto value = values.value(term, e);
          if (!value) {
            valued = false;
            return out;
          }
          out.push_back(*value);
        }
        return out;
      };
      std::vector<Term> levelTerms;
      for (const GuardedLevel &level : order.levels)
        levelTerms.push_back(level.value);
      for (size_t e = 0; e < table.events().size() && valued; ++e) {
        lambdas.push_back(tuple(levelTerms, e));
        logicals.push_back(tuple(logical, e));
        outputs.push_back(tuple(output, e));
        const AccessEvent &event = table.events()[e];
        views[0].push_back(table.view(event.h));
        views[1].push_back(table.view(event.f));
        views[2].push_back(table.view(event.g));
      }
      if (!valued) {
        complete = false;
        if (open.empty())
          open = "a level has no value at an event: the enumeration needs live state";
        continue;
      }
    }
    here.events = lambdas.size();
    anyEvent = anyEvent || !lambdas.empty();

    // Theorem 12 at this point: the events, in order, are S(v), in order.
    for (size_t e = 1; e < lambdas.size(); ++e)
      if (!concreteLess(order, lambdas[e - 1], lambdas[e]))
        return refute(v, "events " + std::to_string(e - 1) + " and " + std::to_string(e) +
                             " are not in the order's increasing order");
    if (lambdas.size() != set->size()) {
      size_t n = 0;
      while (n < lambdas.size() && n < set->size() && lambdas[n] == (*set)[n])
        ++n;
      return refute(v, lambdas.size() < set->size()
                           ? "a point of the set has no event (" + std::to_string(set->size()) +
                                 " points, " + std::to_string(lambdas.size()) + " events)"
                           : "an event is no point of the set (" + std::to_string(set->size()) +
                                 " points, " + std::to_string(lambdas.size()) + " events)");
    }
    for (size_t n = 0; n < lambdas.size(); ++n)
      if (lambdas[n] != (*set)[n])
        return refute(v, "event " + std::to_string(n) + " is not the set's point " + std::to_string(n));

    // Multiplicity, exactly.
    std::map<std::vector<uint64_t>, uint64_t> counts;
    auto keyOf = [](const std::vector<APInt> &values) {
      std::vector<uint64_t> out;
      for (const APInt &value : values)
        out.push_back(value.getZExtValue());
      return out;
    };
    for (const auto &point : logicals)
      here.maxMultiplicity = std::max(here.maxMultiplicity, ++counts[keyOf(point)]);
    here.logicalPoints = counts.size();
    if (structure->form == DomainMultiplicity::ConstantBox) {
      auto mu = evaluateLevelTerm(structure->mu, v, {});
      for (const auto &[point, count] : counts)
        if (!mu || *mu != APInt(128, count))
          return CheckedDomain::unknown("the multiplicity contradicts the order's box: a defect");
    }

    // Cells: the order within each, the key, collisions, and single contributions.
    std::map<std::vector<uint64_t>, std::vector<size_t>> cells;
    for (size_t e = 0; e < lambdas.size(); ++e) {
      std::vector<uint64_t> cell;
      for (const APInt &value : views[0][e])
        cell.push_back(value.getZExtValue());
      cells[cell].push_back(e);
    }
    here.cells = cells.size();
    for (const auto &[cell, members] : cells) {
      here.maxPerCell = std::max<uint64_t>(here.maxPerCell, members.size());
      std::set<std::vector<uint64_t>> outputsHere;
      for (size_t n = 0; n < members.size(); ++n) {
        outputsHere.insert(keyOf(outputs[members[n]]));
        if (n) {
          std::vector<APInt> a, b;
          for (unsigned k : structure->key) {
            a.push_back(lambdas[members[n - 1]][k]);
            b.push_back(lambdas[members[n]][k]);
          }
          if (!concreteLess(keyed, a, b) && keyHolds) {
            keyHolds = false;
            keyBroken = refutedAt(v, "two contributions of one cell are not in the key's order");
          }
        }
      }
      if (outputsHere.size() > 1) {
        ++here.collisions;
        if (injective) {
          injective = false;
          collisionSeen = refutedAt(v, "two logical outputs share a cell");
        }
      }
      if (members.size() > 1 && single) {
        single = false;
        multipleSeen = refutedAt(v, "a cell has more than one contribution");
      }
    }

    // Proposed closed forms of the views.
    for (unsigned view = 0; view < 3; ++view) {
      const auto &form = request.access[view];
      if (form.empty() || !accessHolds[view])
        continue;
      for (size_t e = 0; e < lambdas.size() && accessHolds[view]; ++e) {
        bool same = form.size() == views[view][e].size();
        for (unsigned n = 0; same && n < form.size(); ++n) {
          auto value = evaluateLevelTerm(form[n], v, lambdas[e]);
          same = value && *value == views[view][e][n];
        }
        if (!same) {
          accessHolds[view] = false;
          accessBroken[view] = refutedAt(v, "a proposed closed form differs from an index");
        }
      }
    }
    facts.push_back(std::move(here));
  }
  if (!anyEvent)
    return CheckedDomain::unknown(open.empty() ? "no point has events" : open);
  ComputationTier tier = complete ? ComputationTier::ScopeEnumerated : ComputationTier::Point;
  CheckedDomain out = DomainAuthority::make(
      tier, ComputationStatus::Verified,
      open.empty() ? "each listed point enumerated exactly, in order"
                   : "each listed point enumerated exactly, in order; another is open: " + open);
  DomainAuthority::orbit(out, true);
  DomainAuthority::facts(out, std::move(facts));
  DomainAuthority::multiplicity(out, structure->form, structure->mu);
  std::string where = "at the listed points";
  DomainAuthority::key(out, keyHolds ? proven(where, tier) : *keyBroken);
  DomainAuthority::collision(out, injective ? proven(where, tier) : *collisionSeen);
  DomainAuthority::single(out, single ? proven(where, tier) : *multipleSeen);
  for (unsigned view = 0; view < 3; ++view)
    DomainAuthority::access(out, view,
                            request.access[view].empty() ? unknownFact("nothing proposed")
                            : accessHolds[view]          ? proven(where, tier)
                                                         : *accessBroken[view]);
  return out;
}

std::optional<CheckOutcome> acceptCertificate(const SemanticProgram &program,
                                              const TerminationCertificate *certificate,
                                              const DomainOptions &options, SmtBackend &backend,
                                              std::string &why) {
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

class Proof {
public:
  Proof(const SemanticProgram &program, const SiteGroup &group, const DomainOptions &options,
        SmtBackend &backend)
      : program(program), group(group), options(options), backend(backend), terms(*program.terms) {}

  SmtAnswer ask(const std::string &id, const std::string &where, Term formula, bool record = true) {
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
    ObligationRecord entry;
    entry.id = id;
    entry.location = where;
    entry.expected = "unsat";
    entry.script = obligationScript(id + " " + where, formula, aux);
    SmtResponse response = backend.check(entry.script, options.termination.solver, true);
    entry.result = stringifySmtAnswer(response.answer);
    entry.discharged = response.answer == SmtAnswer::Unsat;
    if (response.answer == SmtAnswer::Unknown)
      entry.evidence = response.detail;
    if (response.answer == SmtAnswer::Sat && id != "prune" && !failing) {
      std::vector<APInt> point;
      for (const Parameter &p : program.parameters) {
        auto found = response.model.find("v" + std::to_string(p.id));
        point.push_back(found == response.model.end() ? APInt(p.sort.width, 0)
                                                      : APInt(p.sort.width, found->second, 10));
      }
      failing = std::move(point);
    }
    if (record && id != "prune")
      records.push_back(std::move(entry));
    return response.answer;
  }

  bool prepare(const TerminationCertificate &certificate, std::string &why) {
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
    std::vector<unsigned> ids(group.sites.begin(), group.sites.end());
    arrivals = reachSites(program, premises.invariants, certificate.cuts, ids, scope,
                          options.termination.paths);
    if (!arrivals.complete) {
      why = arrivals.reason;
      return false;
    }
    for (unsigned id : group.sites)
      locations.push_back(program.sites[id].location);
    return true;
  }

  std::vector<Term> values(const GuardedOrder &order, const SiteArrival &a) {
    std::vector<Term> out;
    for (const GuardedLevel &level : order.levels)
      out.push_back(atPathEnd(program, a.path, level.value));
    return out;
  }
  std::vector<Term> second(const std::vector<Term> &list) {
    std::vector<Term> out;
    for (Term term : list)
      out.push_back(secondCopy(program, term));
    return out;
  }
  std::vector<Term> at(const SiteArrival &a, ArrayRef<Term> list) {
    std::vector<Term> out;
    for (Term term : list)
      out.push_back(atPathEnd(program, a.path, term));
    return out;
  }
  Term equal(ArrayRef<Term> a, ArrayRef<Term> b) {
    Term all = terms.boolean(a.size() == b.size());
    for (unsigned n = 0; n < a.size() && n < b.size(); ++n)
      all = terms.logicalAnd(all, terms.icmp(Predicate::Eq, a[n], b[n]));
    return all;
  }

  /// Every pair of arrivals, each with itself too, over two copies of the state.
  template <typename Body> bool pairs(Body body) {
    for (const SiteArrival &a : arrivals.arrivals)
      for (const SiteArrival &b : arrivals.arrivals)
        if (!body(a, b))
          return false;
    return true;
  }

  const SemanticProgram &program;
  const SiteGroup &group;
  const DomainOptions &options;
  SmtBackend &backend;
  TermContext &terms;
  Term scope = nullptr;
  SiteArrivals arrivals;
  std::vector<unsigned> locations;
  std::vector<ObligationRecord> records;
  std::string failure;
  std::optional<std::vector<APInt>> failing;
  unsigned queries = 0;
};

CheckedDomain symbolicClaim(const SemanticProgram &program, const TerminationCertificate *certificate,
                            const SiteGroup &group, const CoordinateFamily &family,
                            const DomainRequest &request, const DomainOptions &options,
                            SmtBackend &backend) {
  if (!program.isSupported())
    return CheckedDomain::unknown("the program is unsupported");
  std::string why;
  if (!options.termination.solver.timeoutMilliseconds || !options.termination.solver.resourceLimit)
    return CheckedDomain::unknown("a solver limit of zero is no limit");
  if (!groupIsWhole(program, group, why) || !traceIsFixed(program, why))
    return CheckedDomain::unknown(why);
  auto structure = adoptRequest(program, family, request, why);
  if (!structure)
    return CheckedDomain::unknown("the request is refused: " + why);
  if (!acceptCertificate(program, certificate, options, backend, why))
    return CheckedDomain::unknown(why);
  const GuardedOrder &order = structure->order;

  // The walk takes one arrival at a block as one event, and λ is read at the block's
  // entry: two sites of the group in one block would be two events it cannot tell
  // apart. The finite route sees them, and refutes an order that cannot.
  {
    std::set<unsigned> places;
    for (unsigned id : group.sites)
      if (!places.insert(program.sites[id].location).second)
        return CheckedDomain::unknown("two sites of the group share a block");
  }
  Proof proof(program, group, options, backend);
  try {
    if (!proof.prepare(*certificate, why))
      return CheckedDomain::unknown(why);
    auto unknownWith = [&](std::string reason) {
      CheckedDomain out = DomainAuthority::make(ComputationTier::Proposed, ComputationStatus::Unknown,
                                                std::move(reason));
      DomainAuthority::records(out, std::move(proof.records));
      if (proof.failing)
        DomainAuthority::failing(out, *proof.failing);
      return out;
    };
    // Theorem 12.
    std::string orbitWhy;
    AskUnsat ask = [&](const std::string &id, const std::string &where, Term formula) {
      return proof.ask(id, where, formula);
    };
    if (!proveGuardedOrbit(program, order, proof.arrivals, proof.locations, proof.scope,
                           options.limits.maxUnfolding, ask, orbitWhy))
      return unknownWith(proof.failure.empty() ? orbitWhy : proof.failure + "; " + orbitWhy);

    size_t n = proof.arrivals.arrivals.size();
    if (n * n > options.limits.maxPairs)
      return unknownWith("more arrival pairs than the limit");

    // K1, C1, T1 over pairs of events of one cell; A1 over each event.
    std::string why2;
    std::vector<Term> output = axesOf(family, {AxisP, AxisI, AxisJ}, program, why2);
    GuardedOrder keyed = keyOrder(order, structure->key);
    bool keyOk = true, injective = true, single = true;
    bool keyUnknown = false;
    proof.pairs([&](const SiteArrival &a, const SiteArrival &b) {
      const MacSite &first = program.sites[a.site], &second = program.sites[b.site];
      Term both = program.terms->logicalAnd(a.premise, secondCopy(program, b.premise));
      Term sameCell = program.terms->logicalAnd(
          both, proof.equal(proof.at(a, first.output->index),
                            proof.second(proof.at(b, second.output->index))));
      std::vector<Term> la = proof.values(order, a), lb = proof.second(proof.values(order, b));
      TermContext &terms = *program.terms;
      if (keyOk && !structure->keyAll) {
        std::vector<Term> ka, kb;
        for (unsigned k : structure->key) {
          ka.push_back(la[k]);
          kb.push_back(lb[k]);
        }
        Term bad = terms.logicalAnd(sameCell, terms.logicalAnd(guardedLess(program, order, la, lb),
                                                               terms.logicalNot(guardedLess(program, keyed, ka, kb))));
        SmtAnswer answer = proof.ask("K1", "the cell key", bad);
        if (answer != SmtAnswer::Unsat) {
          keyOk = false;
          keyUnknown = true;
        }
      }
      if (injective) {
        Term bad = terms.logicalAnd(sameCell, terms.logicalNot(proof.equal(proof.at(a, output),
                                                                            proof.second(proof.at(b, output)))));
        if (proof.ask("C1", "one logical output per cell", bad) != SmtAnswer::Unsat)
          injective = false;
      }
      if (single) {
        Term bad = terms.logicalAnd(sameCell, terms.logicalNot(proof.equal(la, lb)));
        if (proof.ask("T1", "one contribution per cell", bad) != SmtAnswer::Unsat)
          single = false;
      }
      return true;
    });
    (void)keyUnknown;
    std::array<Fact, 3> accessFacts;
    for (unsigned view = 0; view < 3; ++view) {
      const auto &form = request.access[view];
      if (form.empty()) {
        accessFacts[view] = unknownFact("nothing proposed");
        continue;
      }
      bool ok = true;
      for (const SiteArrival &a : proof.arrivals.arrivals) {
        ArrayRef<Term> index = viewOf(program.sites[a.site], view);
        if (index.size() != form.size()) {
          ok = false;
          break;
        }
        std::vector<Term> la = proof.values(order, a);
        Term differs = program.terms->boolean(false);
        for (unsigned m = 0; m < form.size(); ++m)
          differs = program.terms->logicalOr(
              differs, program.terms->icmp(Predicate::Ne, atPathEnd(program, a.path, index[m]),
                                           bindLevels(program, form[m], la)));
        if (proof.ask("A1", "a closed form of a view", program.terms->logicalAnd(a.premise, differs)) !=
            SmtAnswer::Unsat) {
          ok = false;
          break;
        }
      }
      accessFacts[view] = ok ? proven("over the scope", ComputationTier::Symbolic) : unknownFact("not discharged");
    }

    CheckedDomain out = DomainAuthority::make(ComputationTier::Symbolic, ComputationStatus::Verified,
                                              "D1 to D3 discharged over the scope");
    DomainAuthority::records(out, std::move(proof.records));
    DomainAuthority::orbit(out, true);
    DomainMultiplicity form = structure->form == DomainMultiplicity::Counted ? DomainMultiplicity::Unknown
                                                                         : structure->form;
    DomainAuthority::multiplicity(out, form, structure->mu);
    const ComputationTier symbolic = ComputationTier::Symbolic;
    DomainAuthority::key(out, structure->keyAll ? proven("the key is every level", symbolic)
                              : keyOk           ? proven("K1 over the scope", symbolic)
                                                : unknownFact("K1 not discharged"));
    DomainAuthority::collision(out, injective ? proven("C1 over the scope", symbolic)
                                              : unknownFact("C1 not discharged; a point may show it"));
    DomainAuthority::single(out, single ? proven("T1 over the scope", symbolic) : unknownFact("T1 not discharged"));
    for (unsigned view = 0; view < 3; ++view)
      DomainAuthority::access(out, view, accessFacts[view]);
    return out;
  } catch (const TermError &error) {
    return CheckedDomain::unknown(std::string("term limit: ") + error.what());
  }
}

ComputationOptions computationOf(const DomainOptions &options) {
  ComputationOptions out;
  out.scope = options.scope;
  out.termination = options.termination;
  out.authorized = options.authorized;
  out.limits = options.computation;
  return out;
}

std::vector<uint64_t> pointKey(const std::vector<APInt> &point) {
  std::vector<uint64_t> out;
  for (const APInt &value : point)
    out.push_back(value.getZExtValue());
  return out;
}

} // namespace

CheckedDomain checkDomainFinite(const SemanticProgram &program, const DomainEnumeration &enumeration,
                                unsigned g, const CoordinateFamily &family,
                                const DomainRequest &request, const DomainOptions &options) {
  CheckedDomain out = finiteClaim(program, enumeration, g, family, request, options);
  if (g < enumeration.groups.size())
    DomainAuthority::bind(out, bindingOf(program, enumeration.groups[g], options, family, request));
  return out;
}

CheckedDomain checkDomainSymbolic(const SemanticProgram &program,
                                  const TerminationCertificate *certificate, const SiteGroup &group,
                                  const CoordinateFamily &family, const DomainRequest &request,
                                  const DomainOptions &options, SmtBackend &backend) {
  CheckedDomain out = symbolicClaim(program, certificate, group, family, request, options, backend);
  DomainAuthority::bind(out, bindingOf(program, group, options, family, request));
  return out;
}

ReductionVerdict checkReduction(const SemanticProgram &program,
                                const TerminationCertificate *certificate,
                                const DomainEnumeration &enumeration, unsigned g,
                                const CoordinateFamily &family, const EnumerationOrder *coordinateOrder,
                                const DomainRequest &request, const DomainOptions &options,
                                SmtBackend &backend) {
  ReductionVerdict out;
  if (g >= enumeration.groups.size()) {
    out.why = "no such group";
    return out;
  }
  const SiteGroup &group = enumeration.groups[g];
  ComputationOptions computation = computationOf(options);
  SymbolicCheckOptions coordinates;
  coordinates.scope = options.scope;
  coordinates.termination = options.termination;
  coordinates.authorized = options.authorized;
  coordinates.limits.paths = options.termination.paths;

  // Every claim below is made here, for this program, group, scope and set of names.
  out.coordFinite = checkFamilyFinite(program, enumeration, g, family);
  if (coordinateOrder)
    out.coordSymbolic = checkFamilySymbolic(program, certificate, group, family, coordinateOrder,
                                                 coordinates, backend);
  out.compFinite = checkComputationFinite(program, enumeration, g, computation);
  out.compSymbolic = checkComputationSymbolic(program, certificate, group, computation, backend);
  out.domFinite = checkDomainFinite(program, enumeration, g, family, request, options);
  out.domSymbolic = checkDomainSymbolic(program, certificate, group, family, request, options, backend);

  bool refuted = false;
  for (const CheckedComputation *c : {&out.compFinite, &out.compSymbolic})
    refuted = refuted || c->status() == ComputationStatus::Refuted;
  for (const CheckedDomain *d : {&out.domFinite, &out.domSymbolic})
    refuted = refuted || d->status() == ComputationStatus::Refuted;
  bool coordinateSymbolicOk = out.coordSymbolic.status() == FamilyStatus::Verified &&
                              out.coordSymbolic.tier() >= EvidenceTier::SymbolicValid;
  bool coordinateFiniteOk = out.coordFinite.status() == FamilyStatus::Verified;

  // Theorem 13.
  if (refuted) {
    out.claim = ComputationStatus::Refuted;
    out.why = "a claim it rests on is refuted";
  } else if (out.compSymbolic.status() == ComputationStatus::Verified &&
             out.domSymbolic.status() == ComputationStatus::Verified &&
             out.domSymbolic.exactOrder()) {
    out.claim = ComputationStatus::Verified;
    out.claimTier = ComputationTier::Symbolic;
    out.every = out.compSymbolic.holdsForEveryRun();
    out.logical = coordinateSymbolicOk ? ComputationTier::Symbolic : ComputationTier::Proposed;
    out.why = "G4a and Theorem 12 over the scope";
  } else if (out.compFinite.status() == ComputationStatus::Verified &&
             out.domFinite.status() == ComputationStatus::Verified) {
    std::set<std::vector<uint64_t>> computed;
    for (const ReplayFacts &facts : out.compFinite.points())
      computed.insert(pointKey(facts.parameters));
    for (const DomainPointFacts &facts : out.domFinite.points())
      if (facts.events && computed.count(pointKey(facts.parameters)))
        out.at.push_back(facts.parameters);
    if (out.at.empty()) {
      out.why = "the computation and domain points do not meet";
    } else {
      bool whole = out.compFinite.tier() == ComputationTier::ScopeEnumerated &&
                   out.domFinite.tier() == ComputationTier::ScopeEnumerated;
      out.claim = ComputationStatus::Verified;
      out.claimTier = whole ? ComputationTier::ScopeEnumerated : ComputationTier::Point;
      out.why = "G4a and Theorem 12 at the listed points";
      bool logicalWhole = coordinateSymbolicOk ||
                          (coordinateFiniteOk && out.coordFinite.tier() == EvidenceTier::ScopeEnumerated);
      out.logical = coordinateSymbolicOk || coordinateFiniteOk
                            ? (whole && logicalWhole ? ComputationTier::ScopeEnumerated : ComputationTier::Point)
                            : ComputationTier::Proposed;
    }
  } else {
    out.why = "no tier of the computation meets a tier of the domain";
  }

  // Reorder facts, each from the claims it needs.
  const CheckedDomain &domain = out.domSymbolic.status() == ComputationStatus::Verified
                                    ? out.domSymbolic
                                    : out.domFinite;
  const CheckedComputation &values = out.compSymbolic.status() == ComputationStatus::Verified
                                         ? out.compSymbolic
                                         : out.compFinite;
  // Proven over the scope when T1 holds; otherwise what the points show, refuted by a
  // point with two contributions to one cell.
  if (out.domSymbolic.status() == ComputationStatus::Verified &&
      out.domSymbolic.singleContribution().status == FactStatus::Proven)
    out.trivial = out.domSymbolic.singleContribution();
  else if (out.domFinite.status() == ComputationStatus::Verified)
    out.trivial = out.domFinite.singleContribution();
  else if (domain.status() == ComputationStatus::Verified)
    out.trivial = domain.singleContribution();
  else
    out.trivial = unknownFact("no domain claim");
  bool isFloat = program.sites[group.sites.front()].reduction == ReductionOp::FAdd;
  auto witness = [&]() {
    Fact found = unknownFact("no point to look at");
    unsigned looked = 0;
    for (const PointEvidence &point : enumeration.points) {
      if (!point.reason.empty() || !point.domains[g] || looked++ >= 4)
        continue;
      found = reorderWitness(program, group, point.parameters, options.limits.witnessFills,
                             options.limits.witnessCells);
      if (found.status == FactStatus::Refuted)
        return found;
    }
    return found;
  };
  if (out.claim == ComputationStatus::Verified && out.trivial.status == FactStatus::Proven) {
    out.integer = isFloat ? unknownFact("not an integer fold") : out.trivial;
    out.floating = isFloat ? out.trivial : unknownFact("not a float fold");
  } else if (isFloat) {
    out.integer = unknownFact("not an integer fold");
    out.floating = values.status() == ComputationStatus::Verified
                           ? witness()
                           : unknownFact("the computation is not verified");
  } else {
    out.floating = unknownFact("not a float fold");
    std::string why;
    if (integerReorderConditions(program, group, values, why)) {
      out.integer = proven("wrapping addition with no flag, unobserved, at the computation's tier",
                           values.tier());
    } else if (values.status() == ComputationStatus::Verified) {
      Fact found = witness();
      out.integer = found.status == FactStatus::Refuted ? found : unknownFact(why);
    } else {
      out.integer = unknownFact(why);
    }
  }
  return out;
}

} // namespace mlir::generalized_gemm
