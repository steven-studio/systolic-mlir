//===- DomainAnalysis.cpp -------------------------------------------------===//
#include "GeneralizedGemm/DomainAnalysis.h"

#include "GeneralizedGemm/IndexSlice.h"

#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/raw_ostream.h"

#include <algorithm>
#include <functional>
#include <set>

using llvm::APInt;
namespace json = llvm::json;

namespace mlir::generalized_gemm {

bool affineOverLevels(Term term) {
  std::function<bool(Term)> constantOnly = [&](Term t) {
    if (t->op == TermOp::Const)
      return true;
    if (t->op == TermOp::Add || t->op == TermOp::Sub || t->op == TermOp::Mul)
      return constantOnly(t->args[0]) && constantOnly(t->args[1]);
    return false;
  };
  std::function<bool(Term)> affine = [&](Term t) -> bool {
    switch (t->op) {
    case TermOp::Const: case TermOp::State: case TermOp::Param:
      return true;
    case TermOp::Add: case TermOp::Sub:
      return affine(t->args[0]) && affine(t->args[1]);
    case TermOp::Mul:
      return (constantOnly(t->args[0]) && affine(t->args[1])) ||
             (constantOnly(t->args[1]) && affine(t->args[0]));
    default:
      return false;
    }
  };
  return term && term->sort.kind == Sort::BV && affine(term);
}

namespace {

/// Phase 4: the identities for the proposer, from proved closed forms over counters
/// and from entry definitions. Fills out.identities, out.closedForms, out.identityReason.
void gatherIdentities(const SemanticProgram &program, const DomainAnalysisRequest &request,
                      SmtBackend &backend, DomainResult &out) {
  TermContext &terms = *program.terms;
  std::vector<std::vector<APInt>> points;
  for (const PointEvidence &point : out.enumeration.points)
    if (point.reason.empty())
      points.push_back(point.parameters);
  out.closedForms = synthesizeClosedForms(program, points, request.closedForms, backend);
  auto counterOf = [&](unsigned header) -> std::optional<unsigned> {
    for (unsigned id : program.iterationCounters)
      if (program.state[id].location == header)
        return id;
    return std::nullopt;
  };
  std::set<unsigned> taken;
  std::string notes;
  auto note = [&](const std::string &text) {
    if (notes.find(text) == std::string::npos)
      notes += (notes.empty() ? "" : "; ") + text;
  };
  for (const SliceSynthesis &slice : out.closedForms->slices)
    for (const LoopSynthesis &loop : slice.loops) {
      if (!loop.spec.reason.empty())
        continue;
      for (const SynthesizedClosedForm &form : loop.forms) {
        if (form.status != ClosedFormStatus::Proved)
          continue;
        if (!form.term) {
          note("a proved " + form.kind + " program of s" + std::to_string(form.variable) +
               " gives no term, so no identity");
          continue;
        }
        auto counter = counterOf(loop.spec.header);
        if (!counter) {
          note("loop " + std::to_string(loop.spec.header) + " has no iteration counter");
          continue;
        }
        bool entryLeaf = false;
        for (Term leaf : loop.spec.entry)
          entryLeaf = entryLeaf || terms.substitute(form.term, [&](Term l) -> Term {
                                     return l == leaf ? terms.bv(APInt::getZero(l->sort.width)) : nullptr;
                                   }) != form.term;
        if (entryLeaf) {
          note("the form of s" + std::to_string(form.variable) + " reads an entry value, which no level holds");
          continue;
        }
        if (!taken.insert(form.variable).second)
          continue;
        Term counterLeaf = terms.state(*counter, program.state[*counter].sort);
        if (counterLeaf->sort != loop.spec.iteration->sort) {
          note("the counter's width differs from m's");
          continue;
        }
        StateIdentity identity;
        identity.state = form.variable;
        identity.term = terms.substitute(form.term, [&](Term l) -> Term {
          return l == loop.spec.iteration ? counterLeaf : nullptr;
        });
        identity.cut = loop.spec.header;
        identity.origin = "proved closed form of s" + std::to_string(form.variable) + " over loop " +
                          std::to_string(loop.spec.header) + ", m read as the counter s" +
                          std::to_string(*counter) + " (" + form.reason + ")";
        out.identities.push_back(std::move(identity));
      }
    }
  std::string why;
  for (const auto &[state, term] : entryDefinedState(program, request.closedForms.slice, why))
    if (taken.insert(state).second) {
      StateIdentity identity;
      identity.state = state;
      identity.term = term;
      identity.origin = "defined by one term over parameters on every path from the entry";
      out.identities.push_back(std::move(identity));
    }
  if (!why.empty())
    note(why);
  out.identityReason = notes;
}

} // namespace

DomainResult analyzeDomains(const SemanticProgram &program, const TerminationCertificate *certificate,
                            const DomainAnalysisRequest &given, SmtBackend &backend) {
  DomainResult out;
  DomainAnalysisRequest request = given;
  SynthesisResult search;
  if (request.family || request.segmented) {
    // No search: the given family stands in as the one candidate of each group.
    EnumerationOptions gate;
    gate.scope = request.domain.scope;
    gate.termination = request.domain.termination;
    gate.authorized = request.domain.authorized;
    gate.limits = request.synthesis.limits;
    gate.liveStatePoints = true;
    search.enumeration = enumerateDomain(program, certificate, gate, backend);
    for (unsigned g = 0; g < search.enumeration.groups.size(); ++g) {
      GroupSynthesis group;
      group.group = g;
      group.status = RecoveryStatus::Unknown;
      group.reason = "a family was given, and not searched for";
      if (search.enumeration.groups[g].unsupported.empty()) {
        VerifiedCandidate candidate;
        if (request.family)
          candidate.family = *request.family;
        group.candidates.push_back(std::move(candidate));
      }
      search.groups.push_back(std::move(group));
    }
  } else {
    search = synthesizeProgram(program, certificate, request.synthesis, backend);
  }
  // The whole scope, with live state, for the finite route and the proposer.
  EnumerationOptions enumeration;
  enumeration.scope = request.domain.scope;
  enumeration.termination = request.domain.termination;
  enumeration.authorized = request.domain.authorized;
  enumeration.limits = request.synthesis.limits;
  enumeration.referenceFill = request.synthesis.referenceFill;
  enumeration.liveStatePoints = true;
  out.enumeration = enumerateDomain(program, certificate, enumeration, backend);
  if (request.closedFormViews && program.isSupported()) {
    gatherIdentities(program, request, backend, out);
    request.proposal.identities = out.identities;
  }
  for (const GroupSynthesis &found : search.groups) {
    GroupDomain group;
    group.group = found.group;
    group.search = found.status;
    group.searchReason = found.reason;
    unsigned taken = 0;
    for (const VerifiedCandidate &candidate : found.candidates) {
      if (taken++ >= request.maxCandidates)
        break;
      CandidateDomain entry;
      // A segmented request carries its families in its segments; a family given with
      // --segments only tells the proposer which loop variables are logical.
      if (!request.segmented)
        entry.family = candidate.family;
      entry.coordinateOrder = candidate.order;
      const EnumerationOrder *order = candidate.order ? &*candidate.order : nullptr;
      entry.request = request.segmented
                          ? proposeSegmentedDomain(program, out.enumeration, found.group,
                                                   request.family ? &*request.family : nullptr,
                                                   request.proposal, entry.proposalReason)
                          : proposeDomain(program, out.enumeration, found.group, candidate.family, order,
                                          request.proposal, entry.proposalReason);
      // Phase 4: the identities and the counter bounds as certificate invariants, if
      // the certificate checker accepts them; the checkers below re-check whatever
      // certificate they are handed.
      const TerminationCertificate *use = certificate;
      TerminationCertificate strengthened;
      if (certificate && entry.request && request.closedFormViews && !out.identities.empty()) {
        TermContext &terms = *program.terms;
        strengthened = *certificate;
        auto addAt = [&](int cut, Term fact, const std::string &text) {
          bool placed = false;
          for (auto &[at, invariant] : strengthened.invariants)
            if (cut < 0 || (int)at == cut) {
              invariant = terms.logicalAnd(invariant, fact);
              placed = true;
            }
          if (placed)
            entry.invariantsAdded.push_back(text);
        };
        for (const StateIdentity &identity : out.identities) {
          Term leaf = terms.state(identity.state, program.state[identity.state].sort);
          if (identity.term->sort != leaf->sort)
            continue;
          addAt(identity.cut, terms.icmp(Predicate::Eq, leaf, identity.term),
                "s" + std::to_string(identity.state) + " = " + formatTerm(identity.term, 12) +
                    (identity.cut < 0 ? " at every cut" : " at cut " + std::to_string(identity.cut)));
        }
        for (const GuardedLevel &level : entry.request->order.levels) {
          Term value = level.value;
          if (value->op != TermOp::State || value->index >= program.state.size() ||
              program.state[value->index].kind != StateVar::IterationCounter)
            continue;
          if (!level.lower || !level.upper || !stateSupport(level.lower).empty() ||
              !stateSupport(level.upper).empty() || level.lower->sort != value->sort ||
              level.upper->sort != value->sort || level.isSigned)
            continue;
          // At the header the counter runs from lower to the exclusive bound itself,
          // which is where the loop is left.
          Term inRange = terms.logicalAnd(terms.icmp(Predicate::Ule, level.lower, value),
                                          terms.icmp(Predicate::Ule, value, level.upper));
          addAt(program.state[value->index].location, inRange,
                formatTerm(level.lower, 12) + " <= s" + std::to_string(value->index) + " <= " +
                    formatTerm(level.upper, 12) + " at cut " + std::to_string(program.state[value->index].location));
        }
        if (!entry.invariantsAdded.empty()) {
          CheckOptions check;
          check.scope = request.domain.scope;
          check.limits = request.domain.termination;
          check.authorized = request.domain.authorized;
          CheckOutcome outcome = checkCertificate(program, strengthened, check, backend);
          bool accepted = outcome.verdict.kind() == TerminationVerdict::ProvenTerminating ||
                          outcome.verdict.kind() == TerminationVerdict::ProvenTerminatingIfDefined;
          if (accepted) {
            use = &strengthened;
            entry.certificateStrengthened = true;
            entry.certificateNote = "the certificate checker accepts the strengthened invariants: " +
                                    outcome.verdict.reason();
          } else {
            entry.certificateNote = "the strengthened certificate was not accepted, the given one is used: " +
                                    outcome.verdict.reason();
          }
        }
      }
      if (entry.request)
        entry.verdict = checkReduction(program, use, out.enumeration, found.group,
                                       entry.family, order, *entry.request, request.domain, backend);
      group.candidates.push_back(std::move(entry));
    }
    out.groups.push_back(std::move(group));
  }
  // G5b-M4a: each set's request from its groups' own proposals, in program order. The
  // checkers run whatever came of it, and their gates speak for a request that is missing.
  for (const DomainSetRequest &asked : request.groupSets) {
    SetDomain set;
    set.groups = asked.groups;
    if (asked.all) {
      set.groups.clear();
      for (unsigned g = 0; g < out.enumeration.groups.size(); ++g)
        if (out.enumeration.groups[g].unsupported.empty())
          set.groups.push_back(g);
      if (asked.reversed)
        std::reverse(set.groups.begin(), set.groups.end());
    }
    const std::vector<unsigned> &listed = set.groups;
    DomainRequest joined;
    bool proposed = true;
    for (unsigned g : listed) {
      if (g >= out.enumeration.groups.size()) {
        proposed = false;
        set.proposalReason = "no such group";
        break;
      }
      std::string why;
      auto mine = proposeSegmentedDomain(program, out.enumeration, g, nullptr, request.proposal, why);
      if (!mine) {
        proposed = false;
        set.proposalReason = "group " + std::to_string(g) + ": " + why;
        break;
      }
      for (Segment &segment : mine->segments)
        joined.segments.push_back(std::move(segment));
    }
    if (proposed) {
      auto first = [](const Segment &segment) {
        return *std::min_element(segment.sites.begin(), segment.sites.end());
      };
      std::stable_sort(joined.segments.begin(), joined.segments.end(),
                       [&](const Segment &a, const Segment &b) { return first(a) < first(b); });
      set.request = std::move(joined);
    }
    DomainRequest given = set.request ? *set.request : DomainRequest();
    // G5b-M4b-1: the verdict makes the four claims itself; the two domain claims printed
    // beside it are its own, so that M4a's output stays what it was.
    set.reduction = checkSetReduction(program, certificate, out.enumeration, listed, given, request.domain, backend);
    bool known = true;
    for (unsigned g : listed)
      known = known && g < out.enumeration.groups.size();
    set.finite = known ? set.reduction.domainFinite()
                       : checkSetDomainFinite(program, out.enumeration, listed, given, request.domain);
    set.symbolic = known ? set.reduction.domainSymbolic() : CheckedSetDomain::unknown("no such group");
    out.sets.push_back(std::move(set));
  }
  return out;
}

namespace {

const char *operatorName(Term term) {
  switch (term->op) {
  case TermOp::Add: return "add";
  case TermOp::Sub: return "sub";
  case TermOp::Mul: return "mul";
  case TermOp::SDiv: return "sdiv";
  case TermOp::UDiv: return "udiv";
  case TermOp::SRem: return "srem";
  case TermOp::URem: return "urem";
  case TermOp::Shl: return "shl";
  case TermOp::LShr: return "lshr";
  case TermOp::AShr: return "ashr";
  case TermOp::And: return "and";
  case TermOp::Or: return "or";
  case TermOp::Xor: return "xor";
  case TermOp::Not: return "not";
  case TermOp::ICmp: return "icmp";
  case TermOp::Ite: return "ite";
  case TermOp::ZExt: return "zext";
  case TermOp::SExt: return "sext";
  case TermOp::Trunc: return "trunc";
  default: return "other";
  }
}

/// Machine-readable terms for the oracle. A level variable is {"level": m}.
json::Value termJSON(Term term) {
  if (!term)
    return nullptr;
  json::Object out{{"width", (int64_t)term->sort.width}};
  if (term->op == TermOp::Const) {
    // A constant past 64 bits, as the 128-bit μ has, goes as decimal text.
    if (term->value.getBitWidth() <= 64) {
      out["const"] = (int64_t)term->value.getZExtValue();
    } else {
      llvm::SmallString<48> text;
      term->value.toString(text, 10, /*Signed=*/false);
      out["const"] = text.str().str();
    }
  } else if (term->op == TermOp::State) {
    if (auto level = levelOf(term))
      out["level"] = (int64_t)*level;
    else
      out["state"] = (int64_t)term->index;
  } else if (term->op == TermOp::Param) {
    out["param"] = (int64_t)term->index;
  } else {
    out["op"] = operatorName(term);
    if (term->op == TermOp::ICmp)
      out["predicate"] = (int64_t)term->aux;
    json::Array args;
    for (Term arg : term->args)
      args.push_back(termJSON(arg));
    out["args"] = std::move(args);
  }
  return out;
}

json::Array pointJSON(const std::vector<APInt> &point) {
  json::Array out;
  for (const APInt &value : point)
    out.push_back((int64_t)value.getSExtValue());
  return out;
}

std::string hex(const APInt &value) {
  llvm::SmallString<32> text;
  value.toString(text, 16, /*Signed=*/false);
  return text.str().str();
}

json::Object factJSON(const Fact &fact) {
  json::Object out{{"status", stringifyFactStatus(fact.status)},
                   {"tier", stringifyComputationTier((ComputationTier)fact.tier)},
                   {"detail", fact.detail}};
  if (fact.point)
    out["point"] = pointJSON(*fact.point);
  if (fact.memory) {
    json::Array memory;
    for (const RootData &root : fact.memory->memory) {
      json::Array cells;
      for (const auto &cell : root.cells)
        cells.push_back(cell ? json::Value(hex(*cell)) : json::Value(nullptr));
      memory.push_back(std::move(cells));
    }
    out["memory"] = std::move(memory);
  }
  return out;
}

json::Object domainJSON(const CheckedDomain &d) {
  json::Object out{{"tier", stringifyComputationTier(d.tier())},
                   {"status", stringifyComputationStatus(d.status())},
                   {"reason", d.reason()},
                   {"exact_order", d.exactOrder()},
                   {"multiplicity", stringifyDomainMultiplicity(d.multiplicity())},
                   {"multiplicity_term", termJSON(d.multiplicityTerm())},
                   {"cell_key", factJSON(d.cellKey())},
                   {"output_injective", factJSON(d.outputInjective())},
                   {"single_contribution", factJSON(d.singleContribution())},
                   {"shared_cells", factJSON(d.sharedCells())},
                   {"refutation", factJSON(d.refutation())}};
  json::Array segments;
  for (const SegmentFacts &s : d.segments()) {
    json::Array sites, access;
    for (unsigned site : s.sites)
      sites.push_back((int64_t)site);
    for (const Fact &fact : s.access)
      access.push_back(factJSON(fact));
    segments.push_back(json::Object{{"sites", std::move(sites)},
                                    {"multiplicity", stringifyDomainMultiplicity(s.multiplicity)},
                                    {"multiplicity_term", termJSON(s.multiplicityTerm)},
                                    {"cell_key", factJSON(s.cellKey)},
                                    {"output_injective", factJSON(s.outputInjective)},
                                    {"access", std::move(access)}});
  }
  out["segments"] = std::move(segments);
  json::Array access;
  for (const Fact &fact : d.access())
    access.push_back(factJSON(fact));
  out["access"] = std::move(access);
  json::Array points;
  for (const DomainPointFacts &p : d.points())
    points.push_back(json::Object{{"parameters", pointJSON(p.parameters)},
                                  {"events", (int64_t)p.events},
                                  {"points", (int64_t)p.points},
                                  {"logical_points", (int64_t)p.logicalPoints},
                                  {"cells", (int64_t)p.cells},
                                  {"max_multiplicity", (int64_t)p.maxMultiplicity},
                                  {"max_per_cell", (int64_t)p.maxPerCell},
                                  {"collisions", (int64_t)p.collisions}});
  out["points"] = std::move(points);
  json::Array records;
  for (const ObligationRecord &r : d.records())
    records.push_back(json::Object{{"id", r.id}, {"location", r.location}, {"result", r.result},
                                   {"discharged", r.discharged}});
  out["obligations"] = std::move(records);
  if (d.failingPoint())
    out["failing_point"] = pointJSON(*d.failingPoint());
  return out;
}

/// G5b-M4a.
json::Object setDomainJSON(const CheckedSetDomain &d) {
  json::Array groups, segmentGroups, segments, pairs, points, records;
  for (const SiteGroup &group : d.groups()) {
    json::Array sites;
    for (unsigned site : group.sites)
      sites.push_back((int64_t)site);
    groups.push_back(json::Object{{"output", group.output ? json::Value((int64_t)*group.output) : json::Value(nullptr)},
                                  {"first", (int64_t)group.first},
                                  {"second", (int64_t)group.second},
                                  {"sites", std::move(sites)}});
  }
  for (unsigned g : d.segmentGroups())
    segmentGroups.push_back((int64_t)g);
  for (const SegmentFacts &s : d.segments()) {
    json::Array sites, access;
    for (unsigned site : s.sites)
      sites.push_back((int64_t)site);
    for (const Fact &fact : s.access)
      access.push_back(factJSON(fact));
    segments.push_back(json::Object{{"sites", std::move(sites)},
                                    {"multiplicity", stringifyDomainMultiplicity(s.multiplicity)},
                                    {"cell_key", factJSON(s.cellKey)},
                                    {"output_injective", factJSON(s.outputInjective)},
                                    {"access", std::move(access)}});
  }
  for (const RootPair &pair : d.rootPairs())
    pairs.push_back(json::Object{{"first", (int64_t)pair.first},
                                 {"second", (int64_t)pair.second},
                                 {"relation", stringifyCellRelation(pair.relation)},
                                 {"basis", pair.basis},
                                 {"assumption", pair.assumption}});
  for (const DomainPointFacts &p : d.points())
    points.push_back(json::Object{{"parameters", pointJSON(p.parameters)},
                                  {"events", (int64_t)p.events},
                                  {"points", (int64_t)p.points},
                                  {"cells", (int64_t)p.cells},
                                  {"max_per_cell", (int64_t)p.maxPerCell}});
  for (const ObligationRecord &r : d.records())
    records.push_back(json::Object{{"id", r.id}, {"location", r.location}, {"result", r.result},
                                   {"discharged", r.discharged}});
  json::Object out{{"tier", stringifyComputationTier(d.tier())},
                   {"status", stringifyComputationStatus(d.status())},
                   {"reason", d.reason()},
                   {"binding", d.binding()},
                   {"exact_order", d.exactOrder()},
                   {"groups", std::move(groups)},
                   {"segment_groups", std::move(segmentGroups)},
                   {"segments", std::move(segments)},
                   {"shared_cells", factJSON(d.sharedCells())},
                   {"single_contribution", factJSON(d.singleContribution())},
                   {"root_pairs", std::move(pairs)},
                   {"refutation", factJSON(d.refutation())},
                   {"points", std::move(points)},
                   {"obligations", std::move(records)},
                   {"authorizes_lowering", false}};
  if (d.failingPoint())
    out["failing_point"] = pointJSON(*d.failingPoint());
  return out;
}

/// G5b-M4b-1.
json::Object reductionJSON(const SetReductionVerdict &v) {
  auto claimJSON = [](ComputationStatus status, ComputationTier tier, const std::string &reason) {
    return json::Object{{"status", stringifyComputationStatus(status)},
                        {"tier", stringifyComputationTier(tier)},
                        {"reason", reason}};
  };
  json::Array points;
  for (const auto &p : v.points())
    points.push_back(pointJSON(p));
  json::Object out{{"status", stringifyComputationStatus(v.status())},
                   {"tier", stringifyComputationTier(v.tier())},
                   {"logical_tier", stringifyComputationTier(v.logicalTier())},
                   {"reason", v.reason()},
                   {"refuted_by", stringifyRefutedBy(v.refutedBy())},
                   {"holds_for", v.everyRun() ? "every run" : "runs that complete"},
                   {"fold", stringifyFoldForm(v.fold())},
                   {"points", std::move(points)},
                   {"binding", v.binding()},
                   {"computation_finite", claimJSON(v.computationFinite().status(), v.computationFinite().tier(),
                                                    v.computationFinite().reason())},
                   {"computation_symbolic", claimJSON(v.computationSymbolic().status(), v.computationSymbolic().tier(),
                                                      v.computationSymbolic().reason())},
                   {"domain_finite", claimJSON(v.domainFinite().status(), v.domainFinite().tier(),
                                               v.domainFinite().reason())},
                   {"domain_symbolic", claimJSON(v.domainSymbolic().status(), v.domainSymbolic().tier(),
                                                 v.domainSymbolic().reason())},
                   {"reorder", json::Object{{"trivial", factJSON(v.trivialReorder())},
                                            {"integer", factJSON(v.integerReorder())},
                                            {"float", factJSON(v.floatReorder())}}},
                   {"transformation", v.transformation().name()},
                   {"authorizes_lowering", false}};
  if (v.failingPoint())
    out["failing_point"] = pointJSON(*v.failingPoint());
  if (v.witness()) {
    json::Array memory;
    for (const RootData &root : v.witness()->memory) {
      json::Array cells;
      for (const auto &cell : root.cells)
        cells.push_back(cell ? json::Value(hex(*cell)) : json::Value(nullptr));
      memory.push_back(std::move(cells));
    }
    out["witness"] = json::Object{{"parameters", pointJSON(v.witness()->parameters)}, {"memory", std::move(memory)}};
  }
  return out;
}

json::Object orderJSON(const GuardedOrder &order, const std::vector<unsigned> &cellKey,
                       const std::array<std::vector<Term>, 3> &accessForms) {
  json::Array levels;
  for (const GuardedLevel &level : order.levels)
    levels.push_back(json::Object{{"value", termJSON(level.value)},
                                  {"lower", termJSON(level.lower)},
                                  {"upper", termJSON(level.upper)},
                                  {"descending", level.descending},
                                  {"signed", level.isSigned}});
  json::Array key, access;
  for (unsigned k : cellKey)
    key.push_back((int64_t)k);
  for (const auto &form : accessForms) {
    json::Array terms;
    for (Term term : form)
      terms.push_back(termJSON(term));
    access.push_back(std::move(terms));
  }
  return json::Object{{"levels", std::move(levels)},
                      {"guard", termJSON(order.guard)},
                      {"cell_key", std::move(key)},
                      {"access", std::move(access)},
                      {"origin", order.origin}};
}

json::Object requestJSON(const DomainRequest &request) {
  json::Object out = orderJSON(request.order, request.cellKey, request.access);
  json::Array segments;
  for (const Segment &segment : request.segments) {
    json::Object entry = orderJSON(segment.order, segment.cellKey, segment.access);
    json::Array sites;
    for (unsigned site : segment.sites)
      sites.push_back((int64_t)site);
    entry["sites"] = std::move(sites);
    static const char *names[] = {"p", "i", "j", "k"};
    json::Object axes;
    for (unsigned axis = 0; axis < 4; ++axis) {
      json::Array list;
      for (Term term : segment.family.axes[axis])
        list.push_back(termJSON(term));
      axes[names[axis]] = std::move(list);
    }
    entry["axes"] = std::move(axes);
    segments.push_back(std::move(entry));
  }
  out["segments"] = std::move(segments);
  return out;
}

} // namespace

std::string toJSON(const SemanticProgram &program, const DomainResult &result, bool events) {
  const DomainEnumeration &enumeration = result.enumeration;
  json::Object out{{"function", program.functionName},
                   {"fingerprint", program.fingerprint},
                   {"gate", stringifyGateStatus(enumeration.gate)},
                   {"reason", enumeration.reason},
                   {"authorizes_lowering", false},
                   {"sampled", enumeration.sampled},
                   {"termination", stringifyTerminationVerdict(enumeration.termination.kind())}};
  json::Array groups;
  for (const GroupDomain &group : result.groups) {
    json::Array candidates;
    for (const CandidateDomain &c : group.candidates) {
      static const char *names[] = {"p", "i", "j", "k"};
      json::Object axes;
      for (unsigned axis = 0; axis < 4; ++axis) {
        json::Array list;
        for (Term term : c.family.axes[axis])
          list.push_back(termJSON(term));
        axes[names[axis]] = std::move(list);
      }
      json::Object entry{{"axes", std::move(axes)}, {"proposal_reason", c.proposalReason}};
      bool anyProgram = false;
      for (const auto &axis : c.family.programs)
        for (const std::string &text : axis)
          anyProgram = anyProgram || !text.empty();
      if (anyProgram) {
        // The statement programs behind the terms (docs/GENERALIZED_GEMM_STATEMENT_BV.md):
        // text of the proposal, judged nowhere; "" marks a term built directly.
        json::Object programs;
        for (unsigned axis = 0; axis < 4; ++axis) {
          json::Array texts;
          for (const std::string &text : c.family.programs[axis])
            texts.push_back(text);
          programs[names[axis]] = std::move(texts);
        }
        entry["index_programs"] = std::move(programs);
        entry["origin"] = c.family.origin;
      }
      if (c.request)
        entry["request"] = requestJSON(*c.request);
      const ReductionVerdict &v = c.verdict;
      json::Array points;
      for (const auto &p : v.points())
        points.push_back(pointJSON(p));
      entry["theorem13"] = json::Object{{"status", stringifyComputationStatus(v.status())},
                                        {"tier", stringifyComputationTier(v.tier())},
                                        {"logical_tier", stringifyComputationTier(v.logicalTier())},
                                        {"points", std::move(points)},
                                        {"holds_for", v.everyRun() ? "every run" : "runs that complete"},
                                        {"reason", v.reason()}};
      entry["domain_finite"] = domainJSON(v.domainFinite());
      entry["domain_symbolic"] = domainJSON(v.domainSymbolic());
      entry["coordinate_finite"] = stringifyEvidenceTier(v.coordinateFinite().tier());
      entry["coordinate_symbolic"] = stringifyEvidenceTier(v.coordinateSymbolic().tier());
      entry["computation_finite"] = json::Object{
          {"status", stringifyComputationStatus(v.computationFinite().status())},
          {"tier", stringifyComputationTier(v.computationFinite().tier())},
          {"reason", v.computationFinite().reason()}};
      entry["computation_symbolic"] = json::Object{
          {"status", stringifyComputationStatus(v.computationSymbolic().status())},
          {"tier", stringifyComputationTier(v.computationSymbolic().tier())},
          {"reason", v.computationSymbolic().reason()}};
      entry["reorder"] = json::Object{{"trivial", factJSON(v.trivialReorder())},
                                      {"integer", factJSON(v.integerReorder())},
                                      {"float", factJSON(v.floatReorder())}};
      entry["transformation"] = v.transformation().name();
      entry["authorizes_lowering"] = false;
      if (!result.identities.empty() || result.closedForms) {
        // Phase 4 (I3): the three views as proposed, whether each is affine, and a
        // summary that keeps three questions apart: is the decomposition verified,
        // are the views proved and affine, and would Definition 4.5 (V) admit a
        // lowering. The last is a statement about eligibility; this build
        // authorizes nothing.
        static const char *viewNames[] = {"C", "A", "B"};
        json::Object views;
        bool anyProposed = false, allProposed = true, allAffine = true, allProvedSymbolic = true, allProvedFinite = true;
        bool anyIndexProgram = false;
        // Phase 5 (G.5): a view dimension that is a slice variable with a while or table
        // index program (no term, so no level identity) is recorded with that program
        // and its status: a description of the view, judged by the closed-form checker
        // at its own tier, not a proposal the domain checker proved.
        auto programOf = [&](Term dimension) -> std::optional<json::Object> {
          if (!result.closedForms || dimension->op != TermOp::State)
            return std::nullopt;
          for (const SliceSynthesis &slice : result.closedForms->slices)
            for (const LoopSynthesis &loop : slice.loops)
              for (const SynthesizedClosedForm &form : loop.forms)
                if (form.variable == dimension->index && form.program && form.kind != "loop-free")
                  return json::Object{{"kind", form.kind},
                                      {"status", stringifyClosedFormStatus(form.status)},
                                      {"program", form.text},
                                      {"reason", form.reason},
                                      {"loop", (int64_t)loop.spec.header}};
          return std::nullopt;
        };
        unsigned siteOfGroup = group.group < enumeration.groups.size() && !enumeration.groups[group.group].sites.empty()
                                   ? enumeration.groups[group.group].sites.front()
                                   : UINT32_MAX;
        for (unsigned view = 0; view < 3; ++view) {
          json::Array forms;
          bool affine = true, proposed = false;
          if (c.request)
            for (Term term : c.request->access[view]) {
              proposed = true;
              forms.push_back(termJSON(term));
              affine = affine && affineOverLevels(term);
            }
          const Fact &symbolic = v.domainSymbolic().access()[view];
          const Fact &finite = v.domainFinite().access()[view];
          json::Object one{{"proposed", proposed}, {"forms", std::move(forms)},
                           {"symbolic", factJSON(symbolic)}, {"finite", factJSON(finite)}};
          one["affine"] = proposed ? json::Value(affine) : json::Value(nullptr);
          if (siteOfGroup != UINT32_MAX) {
            const MacSite &site = program.sites[siteOfGroup];
            llvm::ArrayRef<Term> index = view == 0 ? (site.output ? llvm::ArrayRef<Term>(site.output->index) : llvm::ArrayRef<Term>())
                                       : view == 1 ? llvm::ArrayRef<Term>(site.a.index)
                                                   : llvm::ArrayRef<Term>(site.b.index);
            json::Array programs;
            bool anyProgram = false;
            for (Term dimension : index) {
              auto found = programOf(dimension);
              anyProgram = anyProgram || found.has_value();
              programs.push_back(found ? json::Value(std::move(*found)) : json::Value(nullptr));
            }
            if (anyProgram) {
              anyIndexProgram = true;
              one["index_programs"] = std::move(programs);
              one["index_programs_note"] =
                  "a dimension of this view is a slice variable described by a statement program with a "
                  "loop or a table; such a view has no affine form, and the program's status is the "
                  "closed-form checker's, not the domain checker's";
            }
          }
          views[viewNames[view]] = std::move(one);
          anyProposed = anyProposed || proposed;
          allProposed = allProposed && proposed;
          allAffine = allAffine && (proposed && affine);
          allProvedSymbolic = allProvedSymbolic && symbolic.status == FactStatus::Proven;
          allProvedFinite = allProvedFinite && finite.status == FactStatus::Proven;
        }
        entry["views"] = std::move(views);
        bool decomposed = v.status() == ComputationStatus::Verified;
        std::string decomposition = decomposed ? std::string("VERIFIED at ") + stringifyComputationTier(v.tier())
                                               : stringifyComputationStatus(v.status());
        std::string viewsProved = !anyProposed ? "NOT_PROPOSED"
                                  : allProposed && allProvedSymbolic ? "PROVEN over the scope"
                                  : allProposed && allProvedFinite ? "PROVEN at the enumerated points"
                                                                   : "NOT_PROVEN";
        bool reorder = (v.trivialReorder().status == FactStatus::Proven && v.trivialReorder().tier == (int)ComputationTier::Symbolic) ||
                       (v.integerReorder().status == FactStatus::Proven && v.integerReorder().tier == (int)ComputationTier::Symbolic) ||
                       (v.floatReorder().status == FactStatus::Proven && v.floatReorder().tier == (int)ComputationTier::Symbolic);
        bool eligible = decomposed && v.tier() == ComputationTier::Symbolic && allProposed && allProvedSymbolic &&
                        allAffine && v.domainSymbolic().exactOrder() && reorder;
        std::string reason;
        if (!decomposed)
          reason = "the decomposition is not verified";
        else if (v.tier() != ComputationTier::Symbolic)
          reason = "the decomposition is verified at the enumerated points only";
        else if (!allProposed && anyIndexProgram)
          reason = "a view's dimension is a slice variable described by a statement program with a loop or a "
                   "table (see views.*.index_programs), which has no affine form; Definition 4.5 (V) is not met";
        else if (!allProposed)
          reason = "a view has no proposed closed form";
        else if (!allProvedSymbolic)
          reason = "a view's closed form is not proved over the scope";
        else if (!allAffine)
          reason = "a view is not affine, which Definition 4.5 (V) requires";
        else if (!v.domainSymbolic().exactOrder())
          reason = "the order is not exact";
        else if (!reorder)
          reason = "no reorder fact (trivial, integer or float) is proved over the scope";
        else
          reason = "decomposition verified over the scope, every view proved and affine, a reorder fact proved; "
                   "eligibility under Definition 4.5 (V), not an authorization";
        json::Array added;
        for (const std::string &text : c.invariantsAdded)
          added.push_back(text);
        entry["certificate"] = json::Object{{"strengthened", c.certificateStrengthened},
                                            {"invariants_added", std::move(added)},
                                            {"note", c.certificateNote}};
        entry["recognition"] = json::Object{{"decomposition", decomposition},
                                            {"views", viewsProved},
                                            {"views_affine", allProposed ? json::Value(allAffine) : json::Value(nullptr)},
                                            {"lowering_eligible", eligible},
                                            {"reason", reason},
                                            {"authorizes_lowering", false}};
      }
      candidates.push_back(std::move(entry));
    }
    json::Object entry{{"group", (int64_t)group.group},
                       {"search", stringifyRecoveryStatus(group.search)},
                       {"search_reason", group.searchReason},
                       {"candidates", std::move(candidates)}};
    if (events && group.group < enumeration.groups.size()) {
      json::Array tables;
      for (const PointEvidence &point : enumeration.points) {
        const auto &domain = point.domains[group.group];
        json::Object slot{{"parameters", pointJSON(point.parameters)}, {"reason", point.reason}};
        if (domain) {
          const AccessTable &table = domain->table();
          json::Array names, list;
          for (unsigned id : table.coordinateNames)
            names.push_back((int64_t)id);
          // The state at every event's block entry, for oracles of segmented
          // requests, whose terms may read what the live table leaves out.
          auto states = eventStates(program, point.parameters, 0, EnumerationLimits());
          size_t groupEvent = 0;
          std::vector<std::vector<std::optional<APInt>>> stateOf;
          if (states && states->run.status == RunStatus::Returned)
            for (size_t e = 0; e < states->run.events.size(); ++e)
              if (llvm::is_contained(enumeration.groups[group.group].sites, states->run.events[e].site))
                stateOf.push_back(states->states[e]);
          for (const AccessEvent &e : table.events()) {
            json::Array x, h, state;
            for (const APInt &value : table.point(e.point))
              x.push_back((int64_t)value.getZExtValue());
            for (const APInt &value : table.view(e.h))
              h.push_back((int64_t)value.getZExtValue());
            if (groupEvent < stateOf.size())
              for (const auto &value : stateOf[groupEvent])
                state.push_back(value ? json::Value((int64_t)value->getSExtValue()) : json::Value(nullptr));
            ++groupEvent;
            list.push_back(json::Object{{"site", (int64_t)e.site}, {"x", std::move(x)}, {"h", std::move(h)},
                                        {"state", std::move(state)}});
          }
          slot["names"] = std::move(names);
          slot["events"] = std::move(list);
        }
        tables.push_back(std::move(slot));
      }
      entry["tables"] = std::move(tables);
    }
    groups.push_back(std::move(entry));
  }
  out["groups"] = std::move(groups);
  // Phase 4: printed only with closed-form views, so that no other output changes.
  if (result.closedForms) {
    json::Array identities;
    for (const StateIdentity &identity : result.identities)
      identities.push_back(json::Object{{"state", (int64_t)identity.state},
                                        {"term", termJSON(identity.term)},
                                        {"text", formatTerm(identity.term, 12)},
                                        {"affine", affineOverLevels(identity.term)},
                                        {"origin", identity.origin}});
    out["identities"] = std::move(identities);
    out["identity_notes"] = result.identityReason;
    auto parsed = json::parse(closedFormsJSON(program, *result.closedForms));
    out["closed_forms"] = parsed ? std::move(*parsed) : json::Value("unparsable");
  }
  // G5b-M4a: printed only when sets were asked for, so that no other output changes.
  if (!result.sets.empty()) {
    json::Array sets;
    for (const SetDomain &set : result.sets) {
      json::Array listed;
      for (unsigned g : set.groups)
        listed.push_back((int64_t)g);
      json::Object entry{{"groups", std::move(listed)},
                         {"proposal_reason", set.proposalReason},
                         {"finite", setDomainJSON(set.finite)},
                         {"symbolic", setDomainJSON(set.symbolic)},
                         {"reduction", reductionJSON(set.reduction)}};
      if (set.request)
        entry["request"] = requestJSON(*set.request);
      sets.push_back(std::move(entry));
    }
    out["group_sets"] = std::move(sets);
  }
  std::string text;
  llvm::raw_string_ostream stream(text);
  stream << llvm::formatv("{0:2}", json::Value(std::move(out)));
  return text;
}

} // namespace mlir::generalized_gemm
