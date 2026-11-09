//===- DomainChecker.cpp --------------------------------------------------===//
#include "GeneralizedGemm/DomainChecker.h"
#include "GeneralizedGemm/SiteSets.h"

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
  static void shared(CheckedDomain &out, Fact fact) { out.shared = std::move(fact); }
  static void segments(CheckedDomain &out, std::vector<SegmentFacts> list) {
    out.segmentList = std::move(list);
  }
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
  for (const Segment &segment : request.segments) {
    out += "|S";
    for (unsigned site : segment.sites)
      out += std::to_string(site) + ",";
    for (const auto &axis : segment.family.axes) {
      out += "|f";
      for (Term term : axis)
        out += formatTerm(term) + ";";
    }
    for (const GuardedLevel &level : segment.order.levels)
      out += "|" + formatTerm(level.value) + ";" + formatTerm(level.lower) + ";" +
             formatTerm(level.upper) + ";" + (level.descending ? "d" : "a") + (level.isSigned ? "s" : "u");
    if (segment.order.guard)
      out += "|g" + formatTerm(segment.order.guard);
    out += "|k";
    for (unsigned k : segment.cellKey)
      out += std::to_string(k) + ",";
    for (const auto &form : segment.access) {
      out += "|a";
      for (Term term : form)
        out += formatTerm(term) + ";";
    }
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

/// Theorem 14: when the guard and every logical level's bounds read no repeat level,
/// and no repeat level's bounds read a repeat level, S(v) is, for each logical point x,
/// a box in the repeat levels: μ(x) = Π max(hi_r(v, x) - lo_r(v, x), 0), each span in its
/// level's own signedness, at 128 bits so the product cannot wrap (the widths sum below
/// 128). ConstantBox when μ reads no level, LogicalBox when it reads logical ones. Null
/// when the conditions fail: then only an enumeration counts.
Term boxMultiplicity(const SemanticProgram &program, const GuardedOrder &order,
                     const std::vector<bool> &repeat, bool &readsLogical) {
  TermContext &terms = *program.terms;
  readsLogical = false;
  std::set<unsigned> repeatLevels;
  for (unsigned m = 0; m < repeat.size(); ++m)
    if (repeat[m])
      repeatLevels.insert(m);
  auto reads = [&](Term term, bool wantRepeat) {
    llvm::DenseSet<Term> seen;
    std::function<bool(Term)> visit = [&](Term n) {
      if (!seen.insert(n).second)
        return false;
      auto level = levelOf(n);
      if (level && (repeatLevels.count(*level) != 0) == wantRepeat)
        return true;
      return llvm::any_of(n->args, visit);
    };
    return visit(term);
  };
  if (reads(order.guard, true))
    return nullptr;
  // Each span is below 2^w, so the product is below 2^(sum of widths): at most 127
  // bits, or this is no value at 128 bits.
  unsigned bitsUsed = 0;
  for (unsigned m : repeatLevels)
    bitsUsed += order.levels[m].value->sort.width;
  if (bitsUsed >= 128)
    return nullptr;
  Term product = terms.bv(APInt(128, 1));
  for (unsigned m = 0; m < order.levels.size(); ++m) {
    const GuardedLevel &level = order.levels[m];
    if (reads(level.lower, true) || reads(level.upper, true))
      return nullptr;
    if (!repeatLevels.count(m))
      continue;
    readsLogical = readsLogical || reads(level.lower, false) || reads(level.upper, false);
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
  bool anyRepeat = llvm::is_contained(out.repeat, true), readsLogical = false;
  if (!anyRepeat) {
    out.form = DomainMultiplicity::Unit;
  } else if ((out.mu = boxMultiplicity(program, out.order, out.repeat, readsLogical))) {
    out.form = readsLogical ? DomainMultiplicity::LogicalBox : DomainMultiplicity::ConstantBox;
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
  if (!wholeGroup(program, group, why) || !traceIsFixed(program, why))
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
    std::vector<unsigned> roots; // The output root of each event: a cell is a root and an index.
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
        roots.push_back(program.sites[event.site].output->root);
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
    std::map<std::vector<uint64_t>, size_t> firstEvent;
    for (size_t e = 0; e < logicals.size(); ++e) {
      std::vector<uint64_t> key = keyOf(logicals[e]);
      here.maxMultiplicity = std::max(here.maxMultiplicity, ++counts[key]);
      firstEvent.try_emplace(key, e);
    }
    here.logicalPoints = counts.size();
    if (structure->form == DomainMultiplicity::ConstantBox ||
        structure->form == DomainMultiplicity::LogicalBox) {
      // μ reads no repeat level, so any event of the point gives its value.
      for (const auto &[point, count] : counts) {
        auto mu = evaluateLevelTerm(structure->mu, v, lambdas[firstEvent.at(point)]);
        if (!mu || *mu != APInt(128, count))
          return CheckedDomain::unknown("the multiplicity contradicts the order's box: a defect");
      }
    }

    // Cells: the order within each, the key, collisions, and single contributions.
    std::map<std::vector<uint64_t>, std::vector<size_t>> cells;
    for (size_t e = 0; e < lambdas.size(); ++e)
      cells[cellKey(roots[e], views[0][e])].push_back(e);
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


// ---- G5b: segmented requests -------------------------------------------------------

struct SegmentStructure {
  Structure core;
  std::vector<Term> logical, output; // The segment family's (P,I,J,K) and (P,I,J).
  GuardedOrder keyed;
};

bool familyIsEmpty(const CoordinateFamily &family) {
  for (const auto &axis : family.axes)
    if (!axis.empty())
      return false;
  return true;
}

/// Every segment adopted, the sites a partition of the group's, and the family given
/// beside the request empty. Nullopt with `why` otherwise.
std::optional<std::vector<SegmentStructure>> adoptSegments(const SemanticProgram &program,
                                                           const SiteGroup &group,
                                                           const CoordinateFamily &family,
                                                           const DomainRequest &request,
                                                           std::string &why) {
  if (!familyIsEmpty(family)) {
    why = "a segmented request carries its own families";
    return std::nullopt;
  }
  std::set<unsigned> listed(group.sites.begin(), group.sites.end()), seen;
  std::vector<SegmentStructure> out;
  for (const Segment &segment : request.segments) {
    if (segment.sites.empty()) {
      why = "a segment with no site";
      return std::nullopt;
    }
    for (unsigned site : segment.sites) {
      if (!listed.count(site)) {
        why = "a segment names a site outside the group";
        return std::nullopt;
      }
      if (!seen.insert(site).second) {
        why = "a site in two segments";
        return std::nullopt;
      }
    }
    DomainRequest one;
    one.order = segment.order;
    one.cellKey = segment.cellKey;
    one.access = segment.access;
    auto core = adoptRequest(program, segment.family, one, why);
    if (!core)
      return std::nullopt;
    SegmentStructure structure;
    structure.core = std::move(*core);
    std::string ignored;
    structure.logical = axesOf(segment.family, {AxisP, AxisI, AxisJ, AxisK}, program, ignored);
    structure.output = axesOf(segment.family, {AxisP, AxisI, AxisJ}, program, ignored);
    structure.keyed = keyOrder(structure.core.order, structure.core.key);
    out.push_back(std::move(structure));
  }
  if (seen.size() != listed.size()) {
    why = "a site of the group is in no segment";
    return std::nullopt;
  }
  return out;
}

std::map<unsigned, unsigned> segmentOfSite(const DomainRequest &request) {
  std::map<unsigned, unsigned> out;
  for (unsigned s = 0; s < request.segments.size(); ++s)
    for (unsigned site : request.segments[s].sites)
      out[site] = s;
  return out;
}

/// The facts of every segment, and the joined ones, on a result.
void recordSegments(CheckedDomain &out, const std::vector<SegmentStructure> &structures,
                    const DomainRequest &request, std::vector<SegmentFacts> facts, ComputationTier tier,
                    bool symbolic) {
  bool allUnit = true, keysProven = true, injectiveAll = true;
  std::optional<Fact> keyBroken, collisionBroken;
  for (unsigned s = 0; s < facts.size(); ++s) {
    facts[s].sites = request.segments[s].sites;
    DomainMultiplicity form = structures[s].core.form;
    if (symbolic && form == DomainMultiplicity::Counted)
      form = DomainMultiplicity::Unknown;
    facts[s].multiplicity = form;
    facts[s].multiplicityTerm = structures[s].core.mu;
    allUnit = allUnit && form == DomainMultiplicity::Unit;
    if (facts[s].cellKey.status != FactStatus::Proven) {
      keysProven = false;
      if (!keyBroken)
        keyBroken = facts[s].cellKey;
    }
    if (facts[s].outputInjective.status != FactStatus::Proven) {
      injectiveAll = false;
      if (!collisionBroken)
        collisionBroken = facts[s].outputInjective;
    }
  }
  DomainAuthority::multiplicity(out, allUnit ? DomainMultiplicity::Unit : DomainMultiplicity::Unknown, nullptr);
  DomainAuthority::key(out, keysProven ? proven("every segment's key", tier) : *keyBroken);
  DomainAuthority::collision(out, injectiveAll ? proven("every segment's outputs", tier) : *collisionBroken);
  for (unsigned view = 0; view < 3; ++view)
    DomainAuthority::access(out, view, unknownFact("per segment: see segments()"));
  DomainAuthority::segments(out, std::move(facts));
}

CheckedDomain finiteSegmented(const SemanticProgram &program, const DomainEnumeration &enumeration,
                              unsigned g, const CoordinateFamily &family, const DomainRequest &request,
                              const DomainOptions &options) {
  if (enumeration.gate != GateStatus::Open)
    return CheckedDomain::unknown("the domain gate is closed: " + enumeration.reason);
  if (g >= enumeration.groups.size())
    return CheckedDomain::unknown("no such group");
  const SiteGroup &group = enumeration.groups[g];
  std::string why;
  if (!wholeGroup(program, group, why) || !traceIsFixed(program, why))
    return CheckedDomain::unknown(why);
  auto structures = adoptSegments(program, group, family, request, why);
  if (!structures)
    return CheckedDomain::unknown("the request is refused: " + why);
  std::map<unsigned, unsigned> segmentOf = segmentOfSite(request);
  unsigned m = structures->size();

  std::vector<DomainPointFacts> facts;
  std::vector<SegmentFacts> segmentFacts(m);
  bool complete = enumerationCoversScope(program, enumeration, options.scope);
  std::vector<bool> keyHolds(m, true), injective(m, true);
  std::vector<std::array<bool, 3>> accessHolds(m, {true, true, true});
  std::vector<std::optional<Fact>> keyBroken(m), collisionSeen(m);
  std::vector<std::array<std::optional<Fact>, 3>> accessBroken(m);
  bool single = true, sharedNone = true;
  std::optional<Fact> multipleSeen, sharedSeen;
  std::string open;
  bool anyEvent = false;
  auto refute = [&](const std::vector<APInt> &point, std::string detail) {
    CheckedDomain out = DomainAuthority::make(ComputationTier::Proposed, ComputationStatus::Refuted,
                                              "the segments do not describe the events: " + detail);
    DomainAuthority::counter(out, refutedAt(point, detail));
    return out;
  };
  auto keyOf = [](const std::vector<APInt> &values) {
    std::vector<uint64_t> out;
    for (const APInt &value : values)
      out.push_back(value.getZExtValue());
    return out;
  };
  std::set<unsigned> members(group.sites.begin(), group.sites.end());

  for (const PointEvidence &point : enumeration.points) {
    if (!point.reason.empty()) {
      complete = false;
      continue;
    }
    const std::vector<APInt> &v = point.parameters;
    // The run again, with the state at every event: a segment's terms may read what the
    // live table left out.
    auto states = eventStates(program, v, 0, options.limits.events);
    if (!states || states->run.status != RunStatus::Returned) {
      complete = false;
      if (open.empty())
        open = states ? "the run at a point did not return: " + states->run.reason : "no entry at a point";
      continue;
    }
    std::vector<std::vector<std::vector<APInt>>> sets(m);
    bool boxed = true;
    for (unsigned s = 0; s < m && boxed; ++s) {
      auto set = enumerateGuardedSet((*structures)[s].core.order, v, options.limits.maxBox, why);
      if (!set) {
        boxed = false;
        break;
      }
      sets[s] = std::move(*set);
    }
    if (!boxed) {
      complete = false;
      if (open.empty())
        open = why;
      continue;
    }
    // Every event of the group, with its segment and its terms' values.
    struct Row {
      unsigned segment;
      std::vector<APInt> lambda, logical, output;
      std::vector<uint64_t> cell;
    };
    std::vector<Row> rows;
    Configuration config;
    config.memory = states->entry.memory;
    config.shadow.assign(program.state.size(), false);
    bool valued = true;
    std::string unvalued;
    for (size_t e = 0; e < states->run.events.size() && valued; ++e) {
      const MacEvent &event = states->run.events[e];
      if (!members.count(event.site))
        continue;
      Row row;
      auto placed = segmentOf.find(event.site);
      if (placed == segmentOf.end())
        return CheckedDomain::unknown("the request is refused: an event at a site of no segment");
      row.segment = placed->second;
      config.state = states->states[e];
      auto tuple = [&](ArrayRef<Term> list, std::vector<APInt> &out) {
        for (Term term : list) {
          auto value = evaluateTerm(program, states->entry, config, term);
          if (!value) {
            valued = false;
            unvalued = "a term of segment " + std::to_string(row.segment) + " has no value at an event";
            return;
          }
          out.push_back(*value);
        }
      };
      std::vector<Term> levelTerms;
      for (const GuardedLevel &level : (*structures)[row.segment].core.order.levels)
        levelTerms.push_back(level.value);
      tuple(levelTerms, row.lambda);
      tuple((*structures)[row.segment].logical, row.logical);
      tuple((*structures)[row.segment].output, row.output);
      row.cell = cellKey(program.sites[event.site].output->root, event.h);
      rows.push_back(std::move(row));
    }
    if (!valued)
      return CheckedDomain::unknown("the request is refused: " + unvalued);
    DomainPointFacts here;
    here.parameters = v;
    here.events = rows.size();
    for (const auto &set : sets)
      here.points += set.size();
    anyEvent = anyEvent || !rows.empty();

    // Theorem 15 at this point: segments in order, each exactly its set, in order.
    for (size_t e = 1; e < rows.size(); ++e)
      if (rows[e].segment < rows[e - 1].segment)
        return refute(v, "event " + std::to_string(e) + " of segment " + std::to_string(rows[e].segment) +
                             " follows an event of segment " + std::to_string(rows[e - 1].segment));
    std::vector<std::vector<size_t>> ofSegment(m);
    for (size_t e = 0; e < rows.size(); ++e)
      ofSegment[rows[e].segment].push_back(e);
    for (unsigned s = 0; s < m; ++s) {
      const GuardedOrder &order = (*structures)[s].core.order;
      const auto &mine = ofSegment[s];
      std::string where = "segment " + std::to_string(s) + ": ";
      for (size_t n = 1; n < mine.size(); ++n)
        if (!concreteLess(order, rows[mine[n - 1]].lambda, rows[mine[n]].lambda))
          return refute(v, where + "events " + std::to_string(n - 1) + " and " + std::to_string(n) +
                               " are not in the order's increasing order");
      if (mine.size() != sets[s].size())
        return refute(v, where + (mine.size() < sets[s].size() ? "a point of the set has no event ("
                                                                : "an event is no point of the set (") +
                             std::to_string(sets[s].size()) + " points, " + std::to_string(mine.size()) +
                             " events)");
      for (size_t n = 0; n < mine.size(); ++n)
        if (rows[mine[n]].lambda != sets[s][n])
          return refute(v, where + "event " + std::to_string(n) + " is not the set's point " + std::to_string(n));

      // Multiplicity, exactly, per segment.
      std::map<std::vector<uint64_t>, uint64_t> counts;
      std::map<std::vector<uint64_t>, size_t> firstEvent;
      for (size_t e : mine) {
        std::vector<uint64_t> key = keyOf(rows[e].logical);
        here.maxMultiplicity = std::max(here.maxMultiplicity, ++counts[key]);
        firstEvent.try_emplace(key, e);
      }
      here.logicalPoints += counts.size();
      const Structure &core = (*structures)[s].core;
      if (core.form == DomainMultiplicity::ConstantBox || core.form == DomainMultiplicity::LogicalBox)
        for (const auto &[point, count] : counts) {
          auto mu = evaluateLevelTerm(core.mu, v, rows[firstEvent.at(point)].lambda);
          if (!mu || *mu != APInt(128, count))
            return CheckedDomain::unknown("the multiplicity contradicts a segment's box: a defect");
        }
      // Closed forms.
      for (unsigned view = 0; view < 3; ++view) {
        const auto &form = request.segments[s].access[view];
        if (form.empty() || !accessHolds[s][view])
          continue;
        // The views of this segment's events, from the run.
        size_t at = 0;
        for (size_t e = 0; e < states->run.events.size() && accessHolds[s][view]; ++e) {
          const MacEvent &event = states->run.events[e];
          if (!members.count(event.site))
            continue;
          size_t row = at++;
          if (rows[row].segment != s)
            continue;
          const std::vector<APInt> &index = view == 0 ? event.h : view == 1 ? event.f : event.g;
          bool same = form.size() == index.size();
          for (unsigned n = 0; same && n < form.size(); ++n) {
            auto value = evaluateLevelTerm(form[n], v, rows[row].lambda);
            same = value && *value == index[n];
          }
          if (!same) {
            accessHolds[s][view] = false;
            accessBroken[s][view] = refutedAt(v, "a proposed closed form differs from an index");
          }
        }
      }
    }

    // Cells, over the whole group: the key within a segment, collisions within a
    // segment, cells two segments share, and single contributions.
    std::map<std::vector<uint64_t>, std::vector<size_t>> cells;
    for (size_t e = 0; e < rows.size(); ++e)
      cells[rows[e].cell].push_back(e);
    here.cells = cells.size();
    for (const auto &[cell, list] : cells) {
      here.maxPerCell = std::max<uint64_t>(here.maxPerCell, list.size());
      std::set<unsigned> segmentsHere;
      std::map<unsigned, std::set<std::vector<uint64_t>>> outputsHere;
      for (size_t n = 0; n < list.size(); ++n) {
        const Row &row = rows[list[n]];
        segmentsHere.insert(row.segment);
        outputsHere[row.segment].insert(keyOf(row.output));
        if (n && rows[list[n - 1]].segment == row.segment) {
          const Structure &core = (*structures)[row.segment].core;
          std::vector<APInt> a, b;
          for (unsigned k : core.key) {
            a.push_back(rows[list[n - 1]].lambda[k]);
            b.push_back(row.lambda[k]);
          }
          if (!concreteLess((*structures)[row.segment].keyed, a, b) && keyHolds[row.segment]) {
            keyHolds[row.segment] = false;
            keyBroken[row.segment] = refutedAt(v, "two contributions of one cell are not in the key's order");
          }
        }
      }
      for (const auto &[s, outputs] : outputsHere)
        if (outputs.size() > 1) {
          ++here.collisions;
          if (injective[s]) {
            injective[s] = false;
            collisionSeen[s] = refutedAt(v, "two logical outputs of segment " + std::to_string(s) + " share a cell");
          }
        }
      if (segmentsHere.size() > 1 && sharedNone) {
        sharedNone = false;
        sharedSeen = refutedAt(v, "two segments write one cell");
      }
      if (list.size() > 1 && single) {
        single = false;
        multipleSeen = refutedAt(v, "a cell has more than one contribution");
      }
    }
    facts.push_back(std::move(here));
  }
  if (!anyEvent)
    return CheckedDomain::unknown(open.empty() ? "no point has events" : open);
  ComputationTier tier = complete ? ComputationTier::ScopeEnumerated : ComputationTier::Point;
  CheckedDomain out = DomainAuthority::make(
      tier, ComputationStatus::Verified,
      open.empty() ? "each listed point enumerated exactly, segment by segment, in order"
                   : "each listed point enumerated exactly, segment by segment, in order; another is open: " + open);
  DomainAuthority::orbit(out, true);
  DomainAuthority::facts(out, std::move(facts));
  std::string where = "at the listed points";
  for (unsigned s = 0; s < m; ++s) {
    segmentFacts[s].cellKey = keyHolds[s] ? proven(where, tier) : *keyBroken[s];
    segmentFacts[s].outputInjective = injective[s] ? proven(where, tier) : *collisionSeen[s];
    for (unsigned view = 0; view < 3; ++view)
      segmentFacts[s].access[view] = request.segments[s].access[view].empty() ? unknownFact("nothing proposed")
                                     : accessHolds[s][view]                   ? proven(where, tier)
                                                                              : *accessBroken[s][view];
  }
  recordSegments(out, *structures, request, std::move(segmentFacts), tier, false);
  DomainAuthority::single(out, single ? proven(where, tier) : *multipleSeen);
  DomainAuthority::shared(out, sharedNone ? proven(where, tier) : *sharedSeen);
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
  if (!wholeGroup(program, group, why) || !traceIsFixed(program, why))
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
      Term sameCell = sameCellFormula(
          *program.terms,
          rootRelation(program, options.authorized, first.output->root, second.output->root), both,
          proof.equal(proof.at(a, first.output->index), proof.second(proof.at(b, second.output->index))));
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


CheckedDomain symbolicSegmented(const SemanticProgram &program, const TerminationCertificate *certificate,
                                const SiteGroup &group, const CoordinateFamily &family,
                                const DomainRequest &request, const DomainOptions &options,
                                SmtBackend &backend) {
  if (!program.isSupported())
    return CheckedDomain::unknown("the program is unsupported");
  std::string why;
  if (!options.termination.solver.timeoutMilliseconds || !options.termination.solver.resourceLimit)
    return CheckedDomain::unknown("a solver limit of zero is no limit");
  if (!wholeGroup(program, group, why) || !traceIsFixed(program, why))
    return CheckedDomain::unknown(why);
  auto structures = adoptSegments(program, group, family, request, why);
  if (!structures)
    return CheckedDomain::unknown("the request is refused: " + why);
  if (!acceptCertificate(program, certificate, options, backend, why))
    return CheckedDomain::unknown(why);
  {
    std::set<unsigned> places;
    for (unsigned id : group.sites)
      if (!places.insert(program.sites[id].location).second)
        return CheckedDomain::unknown("two sites of the group share a block");
  }
  std::map<unsigned, unsigned> ofSite = segmentOfSite(request), ofLocation;
  for (const auto &[site, s] : ofSite)
    ofLocation[program.sites[site].location] = s;
  unsigned m = structures->size();
  std::vector<GuardedOrder> orders;
  for (const SegmentStructure &s : *structures)
    orders.push_back(s.core.order);

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
    // Theorem 15.
    std::string orbitWhy;
    AskUnsat ask = [&](const std::string &id, const std::string &where, Term formula) {
      return proof.ask(id, where, formula);
    };
    if (!proveSegmentedOrbit(program, orders, ofLocation, proof.arrivals, proof.locations, proof.scope,
                             options.limits.maxUnfolding, ask, orbitWhy))
      return unknownWith(proof.failure.empty() ? orbitWhy : proof.failure + "; " + orbitWhy);

    size_t n = proof.arrivals.arrivals.size();
    if (n * n > options.limits.maxPairs)
      return unknownWith("more arrival pairs than the limit");

    // Within a segment: K1 and C1. Over the group: T1, and whether two segments share
    // a cell.
    std::vector<bool> keyOk(m, true), injective(m, true);
    bool single = true, sharedNone = true;
    TermContext &terms = *program.terms;
    proof.pairs([&](const SiteArrival &a, const SiteArrival &b) {
      unsigned sa = ofSite.at(a.site), sb = ofSite.at(b.site);
      const MacSite &first = program.sites[a.site], &second = program.sites[b.site];
      Term both = terms.logicalAnd(a.premise, secondCopy(program, b.premise));
      Term sameCell = sameCellFormula(
          terms, rootRelation(program, options.authorized, first.output->root, second.output->root), both,
          proof.equal(proof.at(a, first.output->index), proof.second(proof.at(b, second.output->index))));
      std::vector<Term> la = proof.values(orders[sa], a), lb = proof.second(proof.values(orders[sb], b));
      if (sa != sb) {
        if (sharedNone && proof.ask("SH", "cells of two segments", sameCell) != SmtAnswer::Unsat)
          sharedNone = false;
        if (single && proof.ask("T1", "one contribution per cell, across segments", sameCell) != SmtAnswer::Unsat)
          single = false;
        return true;
      }
      const SegmentStructure &s = (*structures)[sa];
      if (keyOk[sa] && !s.core.keyAll) {
        std::vector<Term> ka, kb;
        for (unsigned k : s.core.key) {
          ka.push_back(la[k]);
          kb.push_back(lb[k]);
        }
        Term bad = terms.logicalAnd(sameCell, terms.logicalAnd(guardedLess(program, orders[sa], la, lb),
                                                               terms.logicalNot(guardedLess(program, s.keyed, ka, kb))));
        if (proof.ask("K1", "the cell key, segment " + std::to_string(sa), bad) != SmtAnswer::Unsat)
          keyOk[sa] = false;
      }
      if (injective[sa]) {
        Term bad = terms.logicalAnd(sameCell, terms.logicalNot(proof.equal(proof.at(a, s.output),
                                                                            proof.second(proof.at(b, s.output)))));
        if (proof.ask("C1", "one logical output per cell, segment " + std::to_string(sa), bad) != SmtAnswer::Unsat)
          injective[sa] = false;
      }
      if (single) {
        Term bad = terms.logicalAnd(sameCell, terms.logicalNot(proof.equal(la, lb)));
        if (proof.ask("T1", "one contribution per cell", bad) != SmtAnswer::Unsat)
          single = false;
      }
      return true;
    });
    std::vector<SegmentFacts> segmentFacts(m);
    const ComputationTier symbolic = ComputationTier::Symbolic;
    for (unsigned s = 0; s < m; ++s) {
      const SegmentStructure &structure = (*structures)[s];
      segmentFacts[s].cellKey = structure.core.keyAll ? proven("the key is every level", symbolic)
                                : keyOk[s]            ? proven("K1 over the scope", symbolic)
                                                      : unknownFact("K1 not discharged");
      segmentFacts[s].outputInjective = injective[s] ? proven("C1 over the scope", symbolic)
                                                     : unknownFact("C1 not discharged; a point may show it");
      for (unsigned view = 0; view < 3; ++view) {
        const auto &form = request.segments[s].access[view];
        if (form.empty()) {
          segmentFacts[s].access[view] = unknownFact("nothing proposed");
          continue;
        }
        bool ok = true;
        for (const SiteArrival &a : proof.arrivals.arrivals) {
          if (ofSite.at(a.site) != s)
            continue;
          ArrayRef<Term> index = viewOf(program.sites[a.site], view);
          if (index.size() != form.size()) {
            ok = false;
            break;
          }
          std::vector<Term> la = proof.values(orders[s], a);
          Term differs = terms.boolean(false);
          for (unsigned k = 0; k < form.size(); ++k)
            differs = terms.logicalOr(differs, terms.icmp(Predicate::Ne, atPathEnd(program, a.path, index[k]),
                                                          bindLevels(program, form[k], la)));
          if (proof.ask("A1", "a closed form of a view, segment " + std::to_string(s),
                        terms.logicalAnd(a.premise, differs)) != SmtAnswer::Unsat) {
            ok = false;
            break;
          }
        }
        segmentFacts[s].access[view] = ok ? proven("over the scope", symbolic) : unknownFact("not discharged");
      }
    }
    CheckedDomain out = DomainAuthority::make(ComputationTier::Symbolic, ComputationStatus::Verified,
                                              "D1 to D5 discharged over the scope");
    DomainAuthority::records(out, std::move(proof.records));
    DomainAuthority::orbit(out, true);
    recordSegments(out, *structures, request, std::move(segmentFacts), symbolic, true);
    DomainAuthority::single(out, single ? proven("T1 over the scope", symbolic) : unknownFact("T1 not discharged"));
    DomainAuthority::shared(out, sharedNone ? proven("no cell of two segments, over the scope", symbolic)
                                            : unknownFact("not discharged; a point may show it"));
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
  CheckedDomain out = request.segments.empty() ? finiteClaim(program, enumeration, g, family, request, options)
                                               : finiteSegmented(program, enumeration, g, family, request, options);
  if (g < enumeration.groups.size())
    DomainAuthority::bind(out, bindingOf(program, enumeration.groups[g], options, family, request));
  return out;
}

CheckedDomain checkDomainSymbolic(const SemanticProgram &program,
                                  const TerminationCertificate *certificate, const SiteGroup &group,
                                  const CoordinateFamily &family, const DomainRequest &request,
                                  const DomainOptions &options, SmtBackend &backend) {
  CheckedDomain out = request.segments.empty()
                          ? symbolicClaim(program, certificate, group, family, request, options, backend)
                          : symbolicSegmented(program, certificate, group, family, request, options, backend);
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

  // Every claim below is made here, for this program, group, scope and set of names. A
  // segmented request carries one family per segment, which G3b, with its one family
  // over every event, does not judge: its logical form has no tier here.
  if (request.segments.empty()) {
    out.coordFinite = checkFamilyFinite(program, enumeration, g, family);
    if (coordinateOrder)
      out.coordSymbolic = checkFamilySymbolic(program, certificate, group, family, coordinateOrder,
                                              coordinates, backend);
  } else {
    out.coordFinite = CheckedFamily::unknown("a segmented request: per-segment coordinates are not judged by G3b");
    out.coordSymbolic = out.coordFinite;
  }
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
