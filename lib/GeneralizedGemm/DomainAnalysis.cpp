//===- DomainAnalysis.cpp -------------------------------------------------===//
#include "GeneralizedGemm/DomainAnalysis.h"

#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/raw_ostream.h"

using llvm::APInt;
namespace json = llvm::json;

namespace mlir::generalized_gemm {

DomainResult analyzeDomains(const SemanticProgram &program, const TerminationCertificate *certificate,
                            const DomainAnalysisRequest &request, SmtBackend &backend) {
  DomainResult out;
  SynthesisResult search;
  if (request.family) {
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
      entry.family = candidate.family;
      entry.coordinateOrder = candidate.order;
      const EnumerationOrder *order = candidate.order ? &*candidate.order : nullptr;
      entry.request = proposeDomain(program, out.enumeration, found.group, candidate.family, order,
                                    request.proposal, entry.proposalReason);
      if (entry.request)
        entry.verdict = checkReduction(program, certificate, out.enumeration, found.group,
                                       candidate.family, order, *entry.request, request.domain, backend);
      group.candidates.push_back(std::move(entry));
    }
    out.groups.push_back(std::move(group));
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
                   {"refutation", factJSON(d.refutation())}};
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

json::Object requestJSON(const DomainRequest &request) {
  json::Array levels;
  for (const GuardedLevel &level : request.order.levels)
    levels.push_back(json::Object{{"value", termJSON(level.value)},
                                  {"lower", termJSON(level.lower)},
                                  {"upper", termJSON(level.upper)},
                                  {"descending", level.descending},
                                  {"signed", level.isSigned}});
  json::Array key, access;
  for (unsigned k : request.cellKey)
    key.push_back((int64_t)k);
  for (const auto &form : request.access) {
    json::Array terms;
    for (Term term : form)
      terms.push_back(termJSON(term));
    access.push_back(std::move(terms));
  }
  return json::Object{{"levels", std::move(levels)},
                      {"guard", termJSON(request.order.guard)},
                      {"cell_key", std::move(key)},
                      {"access", std::move(access)},
                      {"origin", request.order.origin}};
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
          for (const AccessEvent &e : table.events()) {
            json::Array x, h;
            for (const APInt &value : table.point(e.point))
              x.push_back((int64_t)value.getZExtValue());
            for (const APInt &value : table.view(e.h))
              h.push_back((int64_t)value.getZExtValue());
            list.push_back(json::Object{{"site", (int64_t)e.site}, {"x", std::move(x)}, {"h", std::move(h)}});
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
  std::string text;
  llvm::raw_string_ostream stream(text);
  stream << llvm::formatv("{0:2}", json::Value(std::move(out)));
  return text;
}

} // namespace mlir::generalized_gemm
