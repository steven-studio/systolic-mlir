#include "GeneralizedGemm/SynthesisAnalysis.h"

#include "GeneralizedGemm/CoordinateAnalysis.h"
#include "GeneralizedGemm/TerminationObligations.h"

#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/raw_ostream.h"

#include <algorithm>
#include <set>
#include <tuple>

using namespace llvm;

namespace mlir::generalized_gemm {

namespace {

/// Sample points when the scope is too large to run: small values in range.
std::vector<std::vector<APInt>> samplePoints(const SemanticProgram &program,
                                             const ParameterScope &scope, unsigned limit) {
  std::vector<std::vector<APInt>> values;
  for (const Parameter &p : program.parameters) {
    std::vector<APInt> list;
    if (p.sort.kind == Sort::BV) {
      int64_t low = APInt::getSignedMinValue(p.sort.width).getSExtValue();
      int64_t high = APInt::getSignedMaxValue(p.sort.width).getSExtValue();
      if (!scope.allParameters)
        for (const ParameterRange &range : scope.ranges)
          if (range.parameter == p.id) {
            low = std::max(low, range.lower.getSExtValue());
            high = std::min(high, range.upper.getSExtValue());
          }
      int64_t start = std::max<int64_t>(low, 2);
      for (int64_t v = start; v <= high && v < start + 3; ++v)
        list.push_back(APInt(p.sort.width, (uint64_t)v, true, true));
      if (list.empty() && low <= high)
        list.push_back(APInt(p.sort.width, (uint64_t)high, true, true));
    } else {
      list.push_back(APInt(p.sort.width, 0));
    }
    values.push_back(std::move(list));
  }
  std::vector<std::vector<APInt>> out{{}};
  for (const auto &list : values) {
    std::vector<std::vector<APInt>> next;
    for (const auto &prefix : out)
      for (const APInt &value : list) {
        auto longer = prefix;
        longer.push_back(value);
        next.push_back(std::move(longer));
      }
    out = std::move(next);
    if (out.size() > limit)
      out.resize(limit);
  }
  return out;
}

/// A few spread points with events, for the search. Evidence never comes from
/// this choice: the checker reads the whole enumeration.
std::vector<const AccessDomain *> searchDomains(const DomainEnumeration &enumeration,
                                                unsigned group, unsigned limit,
                                                ArrayRef<std::vector<APInt>> must) {
  std::vector<const AccessDomain *> all;
  for (const PointEvidence &point : enumeration.points)
    if (point.domains[group])
      all.push_back(&*point.domains[group]);
  std::vector<const AccessDomain *> out;
  for (const AccessDomain *domain : all)
    if (llvm::is_contained(must, domain->parameters()))
      out.push_back(domain);
  if (all.size() <= limit)
    return all;
  // The largest last: it has the most to say.
  for (unsigned n = 1; n <= limit; ++n) {
    const AccessDomain *domain = all[all.size() * n / limit - 1];
    if (!llvm::is_contained(out, domain))
      out.push_back(domain);
  }
  return out;
}

/// Full at some point. A parameter value may be too small for any axis to be long.
bool fullSomewhere(const VerifiedCandidate &candidate) {
  return llvm::any_of(candidate.finite.points(), [](const PointFacts &point) {
    return point.report.structurallyFull;
  });
}

/// Search order only: it decides which candidates are looked at first.
std::tuple<bool, bool, bool, bool, int, int> preference(const VerifiedCandidate &candidate) {
  const auto &points = candidate.finite.points();
  bool box = llvm::all_of(points, [](const PointFacts &point) {
    return point.report.cartesian == CartesianForm::Box;
  });
  bool unit = llvm::all_of(points, [](const PointFacts &point) {
    return point.report.multiplicity == MultiplicityForm::Unit;
  });
  int batch = candidate.family.axes[AxisP].size(), total = batch;
  for (unsigned axis : {AxisI, AxisJ, AxisK})
    total += candidate.family.axes[axis].size();
  const auto &axes = candidate.family.axes;
  bool every = !axes[AxisI].empty() && !axes[AxisJ].empty() && !axes[AxisK].empty();
  return {fullSomewhere(candidate), every, box, unit, -batch, -total};
}

class Driver {
public:
  Driver(const SemanticProgram &program, const TerminationCertificate *certificate,
         const SynthesisOptions &options, SmtBackend &backend)
      : program(program), certificate(certificate), options(options), backend(backend) {}

  SynthesisResult run();

private:
  EnumerationOptions enumerationOptions(const std::vector<std::vector<APInt>> &points) const;
  void synthesizeGroup(unsigned g, GroupSynthesis &out);
  bool addPoint(const std::vector<APInt> &point, std::vector<std::vector<APInt>> &must,
                std::string &closed);
  void verifySymbolic(unsigned g, VerifiedCandidate &candidate,
                      std::vector<std::vector<APInt>> &must, GroupSynthesis &out);

  const SemanticProgram &program;
  const TerminationCertificate *certificate;
  const SynthesisOptions &options;
  SmtBackend &backend;
  SynthesisResult result;
  std::vector<std::vector<APInt>> samples;
  bool sampling = false;
};

EnumerationOptions Driver::enumerationOptions(const std::vector<std::vector<APInt>> &points) const {
  EnumerationOptions out;
  out.scope = options.scope;
  out.termination = options.termination;
  out.authorized = options.authorized;
  out.limits = options.limits;
  out.referenceFill = options.referenceFill;
  out.liveStatePoints = true;
  out.onlyPoints = points;
  // The limit says whether a scope is run whole. Samples have their own budget.
  if (!points.empty())
    out.limits.maxPoints = std::max<uint64_t>(out.limits.maxPoints, points.size());
  return out;
}

/// Makes a point part of the search. False when it adds nothing new.
bool Driver::addPoint(const std::vector<APInt> &point, std::vector<std::vector<APInt>> &must,
                      std::string &closed) {
  if (llvm::is_contained(must, point) || !parametersInScope(options.scope, point))
    return false;
  bool present = false;
  for (const PointEvidence &evidence : result.enumeration.points)
    present = present || evidence.parameters == point;
  if (!present) {
    if (!sampling)
      return false;
    samples.push_back(point);
    result.enumeration = enumerateDomain(program, certificate, enumerationOptions(samples), backend);
    if (result.enumeration.gate != GateStatus::Open) {
      closed = result.enumeration.reason;
      return false;
    }
  }
  must.push_back(point);
  return true;
}

void Driver::verifySymbolic(unsigned g, VerifiedCandidate &candidate,
                            std::vector<std::vector<APInt>> &must, GroupSynthesis &out) {
  SymbolicCheckOptions check;
  check.scope = options.scope;
  check.termination = options.termination;
  check.authorized = options.authorized;
  check.limits = options.symbolic;
  for (unsigned attempt = 0; attempt <= options.maxRefinements; ++attempt) {
    const SiteGroup &group = result.enumeration.groups[g];
    auto domains = searchDomains(result.enumeration, g, options.maxSamples * 3, must);
    candidate.order = proposeOrder(program, candidate.family, domains);
    candidate.symbolic = checkFamilySymbolic(program, certificate, group, candidate.family,
                                             candidate.order ? &*candidate.order : nullptr,
                                             check, backend);
    // A failed query's model may be spurious: only an exact run of that point can
    // tell. It may also show the order wrong, or the family.
    if (candidate.symbolic.tier() == EvidenceTier::SymbolicExact ||
        !candidate.symbolic.failingPoint())
      return;
    std::string closed;
    if (!addPoint(*candidate.symbolic.failingPoint(), must, closed))
      return;
    candidate.finite = checkFamilyFinite(program, result.enumeration, g, candidate.family);
    if (candidate.finite.status() != FamilyStatus::Verified) {
      ++out.refuted;
      return;
    }
  }
}

void Driver::synthesizeGroup(unsigned g, GroupSynthesis &out) {
  out.group = g;
  if (!result.enumeration.groups[g].unsupported.empty()) {
    out.status = RecoveryStatus::Unsupported;
    out.reason = result.enumeration.groups[g].unsupported;
    return;
  }
  std::vector<std::vector<APInt>> must;
  std::vector<VerifiedCandidate> pool;
  std::set<std::array<std::vector<Term>, 4>> known;
  unsigned first = options.grammar ? options.grammar->level : 0;
  unsigned last = options.grammar ? options.grammar->level : options.maxLevel;
  for (unsigned level = first; level <= last; ++level) {
    out.level = level;
    GrammarSpec spec = options.grammar ? *options.grammar : GrammarSpec::atLevel(level);
    for (unsigned attempt = 0; attempt <= options.maxRefinements; ++attempt) {
      auto domains = searchDomains(result.enumeration, g, options.maxSamples, must);
      if (domains.empty()) {
        out.reason = "no sampled point has events for this group";
        return;
      }
      for (const AccessDomain *domain : domains)
        for (const AccessEvent &event : domain->table().events())
          if (event.h != event.hRead) {
            out.status = RecoveryStatus::CandidateRefuted;
            out.reason = "output_read_differs: no += decomposition exists";
            return;
          }
      AtomBank bank = buildAtoms(program, result.enumeration.groups[g], domains, spec);
      out.atoms = bank.atoms.size();
      SynthesisOutcome outcome =
          synthesizeCoordinates(program, bank, domains, options.synthesis, backend);
      out.rounds += outcome.rounds;
      out.clauses += outcome.clauses;
      out.proposals += outcome.candidates.size();
      out.exhausted = outcome.exhausted && bank.truncated.empty();
      bool repeat = false;
      std::string closed;
      for (const CoordinateFamily &family : outcome.candidates) {
        VerifiedCandidate candidate;
        candidate.family = family;
        candidate.finite = checkFamilyFinite(program, result.enumeration, g, family);
        if (candidate.finite.status() == FamilyStatus::Verified) {
          if (known.insert(family.axes).second)
            pool.push_back(std::move(candidate));
          continue;
        }
        ++out.refuted;
        if (candidate.finite.failingPoint())
          repeat = addPoint(*candidate.finite.failingPoint(), must, closed) || repeat;
        if (!closed.empty()) {
          out.reason = closed;
          return;
        }
      }
      if (!repeat)
        break;
    }
    // A wider grammar is tried only while nothing full has been verified.
    if (llvm::any_of(pool, fullSomewhere))
      break;
  }
  if (pool.empty()) {
    out.reason = out.exhausted ? "no assignment of this grammar's atoms passes the checkers"
                               : "no candidate was verified within the limits";
    return;
  }
  std::stable_sort(pool.begin(), pool.end(),
                   [](const VerifiedCandidate &a, const VerifiedCandidate &b) {
                     return preference(a) > preference(b);
                   });
  if (pool.size() > options.synthesis.maxCandidates)
    pool.resize(options.synthesis.maxCandidates);
  for (VerifiedCandidate &candidate : pool)
    verifySymbolic(g, candidate, must, out);
  for (VerifiedCandidate &candidate : pool)
    if (candidate.finite.status() == FamilyStatus::Verified)
      out.candidates.push_back(std::move(candidate));
  if (out.candidates.empty()) {
    out.reason = "no candidate survived the added points";
    return;
  }
  out.status = RecoveryStatus::CandidateFound;
  out.reason = "the checkers accept these candidates at the stated tiers";
}

SynthesisResult Driver::run() {
  // The finite gate first: a bounded scope is run whole, a larger one is sampled.
  std::vector<std::vector<APInt>> none;
  result.enumeration = enumerateDomain(program, certificate, enumerationOptions(none), backend);
  if (result.enumeration.gate == GateStatus::Unknown &&
      result.enumeration.reason.find("more points") != std::string::npos) {
    sampling = true;
    samples = samplePoints(program, options.scope, options.maxSamples * 3);
    result.enumeration = enumerateDomain(program, certificate, enumerationOptions(samples), backend);
  }
  if (result.enumeration.gate != GateStatus::Open)
    return std::move(result);
  result.groups.resize(result.enumeration.groups.size());
  for (unsigned g = 0; g < result.groups.size(); ++g)
    synthesizeGroup(g, result.groups[g]);
  return std::move(result);
}

const char *operatorName(Term term) {
  switch (term->op) {
  case TermOp::Add: return "add";
  case TermOp::Sub: return "sub";
  case TermOp::Mul: return "mul";
  case TermOp::UDiv: return "udiv";
  case TermOp::URem: return "urem";
  case TermOp::SDiv: return "sdiv";
  case TermOp::SRem: return "srem";
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

json::Value termJSON(Term term) {
  json::Object out{{"width", (int64_t)term->sort.width}};
  if (term->op == TermOp::Const) {
    out["const"] = (int64_t)term->value.getZExtValue();
  } else if (term->op == TermOp::State) {
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

json::Array tupleJSON(const IndexTuple &tuple) {
  json::Array out;
  for (const APInt &value : tuple)
    out.push_back((int64_t)value.getSExtValue());
  return out;
}

json::Object familyJSON(const CheckedFamily &family, bool withRecords) {
  json::Object out{{"tier", stringifyEvidenceTier(family.tier())},
                   {"status", stringifyFamilyStatus(family.status())},
                   {"reason", family.reason()},
                   {"equivalence", family.equivalence().name()}};
  if (withRecords) {
    json::Array records;
    for (const ObligationRecord &record : family.records())
      records.push_back(json::Object{{"id", record.id},
                                     {"location", record.location},
                                     {"result", record.result},
                                     {"discharged", record.discharged}});
    out["obligations"] = std::move(records);
    out["domain_form"] = stringifyDomainForm(family.coverage().form);
    out["box"] = family.symbolicBox();
    out["holds_for"] = family.holdsForEveryRun() ? "every run" : "runs that complete";
  }
  return out;
}

} // namespace

SynthesisResult synthesizeProgram(const SemanticProgram &program,
                                  const TerminationCertificate *certificate,
                                  const SynthesisOptions &options, SmtBackend &backend) {
  return Driver(program, certificate, options, backend).run();
}

std::string toJSON(const SemanticProgram &program, const SynthesisResult &result, bool events) {
  const DomainEnumeration &enumeration = result.enumeration;
  json::Object out{{"function", program.functionName},
                   {"fingerprint", program.fingerprint},
                   {"gate", stringifyGateStatus(enumeration.gate)},
                   {"reason", enumeration.reason},
                   {"authorizes_lowering", false},
                   {"sampled", enumeration.sampled},
                   {"termination", stringifyTerminationVerdict(enumeration.termination.kind())}};
  json::Array groups;
  for (const GroupSynthesis &group : result.groups) {
    json::Array candidates;
    for (const VerifiedCandidate &candidate : group.candidates) {
      json::Object entry{{"origin", candidate.family.origin}};
      static const char *names[] = {"p", "i", "j", "k"};
      json::Object axes;
      for (unsigned axis = 0; axis < 4; ++axis) {
        json::Array terms;
        for (Term term : candidate.family.axes[axis])
          terms.push_back(termJSON(term));
        axes[names[axis]] = std::move(terms);
      }
      entry["axes"] = std::move(axes);
      if (candidate.order) {
        json::Array levels;
        for (const OrderLevel &level : candidate.order->levels)
          levels.push_back(json::Object{{"value", termJSON(level.value)},
                                        {"lower", termJSON(level.lower)},
                                        {"upper", termJSON(level.upper)},
                                        {"upper_outer", (int64_t)level.upperOuter}});
        entry["order"] = std::move(levels);
      }
      entry["finite"] = familyJSON(candidate.finite, false);
      entry["symbolic"] = familyJSON(candidate.symbolic, true);
      bool full = !candidate.finite.points().empty();
      json::Array points;
      for (const PointFacts &point : candidate.finite.points()) {
        full = full && point.report.structurallyFull;
        points.push_back(json::Object{{"parameters", tupleJSON(point.parameters)},
                                      {"structurally_full", point.report.structurallyFull},
                                      {"report", reportToJSON(point.report)}});
      }
      entry["points"] = std::move(points);
      entry["structurally_full"] = full;
      candidates.push_back(std::move(entry));
    }
    json::Object entry{{"group", (int64_t)group.group},
                       {"status", stringifyRecoveryStatus(group.status)},
                       {"reason", group.reason},
                       {"level", (int64_t)group.level},
                       {"atoms", (int64_t)group.atoms},
                       {"rounds", (int64_t)group.rounds},
                       {"clauses", (int64_t)group.clauses},
                       {"proposals", (int64_t)group.proposals},
                       {"refuted", (int64_t)group.refuted},
                       {"exhausted", group.exhausted},
                       {"candidates", std::move(candidates)}};
    if (events) {
      json::Array tables;
      for (const PointEvidence &point : enumeration.points) {
        const auto &domain = point.domains[group.group];
        json::Object slot{{"parameters", tupleJSON(point.parameters)},
                          {"run", stringifyRunStatus(point.run)},
                          {"reason", point.reason}};
        if (domain) {
          const AccessTable &table = domain->table();
          json::Array names, list;
          for (unsigned id : table.coordinateNames)
            names.push_back((int64_t)id);
          for (const AccessEvent &e : table.events()) {
            IndexTuple x(table.point(e.point).begin() + 1, table.point(e.point).end());
            list.push_back(json::Object{{"site", (int64_t)e.site},
                                        {"step", (int64_t)e.step},
                                        {"x", tupleJSON(x)},
                                        {"h", tupleJSON(table.view(e.h))},
                                        {"h_read", tupleJSON(table.view(e.hRead))},
                                        {"f", tupleJSON(table.view(e.f))},
                                        {"g", tupleJSON(table.view(e.g))}});
          }
          slot["coordinate_names"] = std::move(names);
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
  raw_string_ostream stream(text);
  stream << formatv("{0:1}", json::Value(std::move(out)));
  return stream.str();
}

} // namespace mlir::generalized_gemm
