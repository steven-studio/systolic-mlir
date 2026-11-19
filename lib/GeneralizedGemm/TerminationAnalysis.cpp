#include "GeneralizedGemm/TerminationAnalysis.h"

#include "GeneralizedGemm/ControlStructure.h"
#include "GeneralizedGemm/TerminationObligations.h"

#include "llvm/ADT/SmallString.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/raw_ostream.h"

#include <set>

using namespace mlir::generalized_gemm;
using llvm::APInt;
namespace json = llvm::json;

namespace {

/// An entry with these parameters, or nullopt when it has no valid shape.
std::optional<EntryState> makeEntry(const SemanticProgram &program,
                                    const std::vector<APInt> &parameters,
                                    bool fillOnes, uint64_t maxCells) {
  EntryState entry;
  entry.parameters = parameters;
  uint64_t total = 0;
  for (const MemoryRoot &root : program.roots) {
    RootData data;
    uint64_t cells = 1;
    for (unsigned d = 0; d < root.extents.size(); ++d) {
      uint64_t extent;
      if (root.extents[d] >= 0) {
        extent = root.extents[d];
      } else {
        const APInt &value = parameters[root.dimensionParameter[d]];
        if (value.isNegative() || value.getActiveBits() > 32)
          return std::nullopt;
        extent = value.getZExtValue();
      }
      data.shape.push_back(extent);
      if (extent && cells > maxCells / extent)
        return std::nullopt;
      cells *= extent;
    }
    total += cells;
    if (total > maxCells)
      return std::nullopt;
    bool fresh = root.origin == MemoryRoot::FreshAllocation;
    bool isFloat = root.sort.elementIsFloat;
    for (uint64_t c = 0; c < cells; ++c) {
      if (fresh)
        data.cells.emplace_back(std::nullopt);
      else
        data.cells.emplace_back(APInt(root.sort.width, fillOnes && !isFloat ? 1 : 0));
    }
    entry.memory.push_back(std::move(data));
  }
  return entry;
}

/// Deterministic parameter points to replay: models of failed obligations, then
/// boundary values. Which points are tried only decides what can be found.
std::vector<std::vector<APInt>>
parameterPoints(const SemanticProgram &program, const ParameterScope &scope,
                const TerminationLimits &limits,
                const ProposalStatistics &statistics) {
  std::vector<std::vector<APInt>> points;
  std::set<std::string> seen;
  auto add = [&](const std::vector<APInt> &point) {
    if (points.size() >= limits.maxReplayEntries ||
        !parametersInScope(scope, point))
      return;
    std::string key;
    for (const APInt &value : point) {
      llvm::SmallString<32> text;
      value.toStringUnsigned(text, 16);
      key += text.str().str() + ",";
    }
    if (seen.insert(key).second)
      points.push_back(point);
  };
  std::vector<std::vector<APInt>> values(program.parameters.size());
  for (const Parameter &p : program.parameters) {
    unsigned w = p.sort.width;
    auto &list = values[p.id];
    for (const ParameterRange &range : scope.ranges)
      if (range.parameter == p.id) {
        list.push_back(range.lower);
        list.push_back(range.upper);
      }
    list.push_back(APInt::getSignedMaxValue(w));
    list.push_back(APInt(w, 0));
    list.push_back(APInt(w, 1));
    list.push_back(APInt::getAllOnes(w));
    list.push_back(APInt::getSignedMinValue(w));
    list.push_back(APInt(w, 3 % (uint64_t(1) << std::min(w, 8u))));
    list.push_back(APInt(w, 7 % (uint64_t(1) << std::min(w, 8u))));
  }
  for (const auto &model : statistics.parameterModels) {
    std::vector<APInt> point;
    for (const Parameter &p : program.parameters) {
      auto value = model.find(p.id);
      point.push_back(value == model.end()
                          ? APInt(p.sort.width, 0)
                          : APInt(p.sort.width, value->second, 10));
    }
    add(point);
  }
  // Mixed radix over the boundary values, in a fixed order.
  std::vector<unsigned> digits(values.size(), 0);
  while (points.size() < limits.maxReplayEntries) {
    std::vector<APInt> point;
    for (unsigned k = 0; k < values.size(); ++k)
      point.push_back(values[k][digits[k]]);
    add(point);
    unsigned k = 0;
    for (; k < digits.size(); ++k) {
      if (++digits[k] < values[k].size())
        break;
      digits[k] = 0;
    }
    if (k == digits.size())
      break;
  }
  return points;
}

SideKind kindOfStop(RunStatus status) {
  switch (status) {
  case RunStatus::UndefinedBehavior: return SideKind::UndefinedBehavior;
  case RunStatus::Underspecified: return SideKind::Underspecified;
  default: return SideKind::ModelLimit;
  }
}

json::Value termJSON(Term t, unsigned &budget) {
  if (budget == 0)
    return nullptr;
  --budget;
  static const char *const names[] = {
      "Const", "FConst", "Param", "State", "Shadow", "MemVar", "Add", "Sub",
      "Mul", "SDiv", "UDiv", "SRem", "URem", "Shl", "LShr", "AShr", "And", "Or",
      "Xor", "Not", "ICmp", "Ite", "ZExt", "SExt", "Trunc", "Load", "Store",
      "FAdd", "FMul", "FIsNaN", "FIsInf", "FIsZero"};
  static const char *const predicates[] = {"eq", "ne", "slt", "sle", "sgt",
                                            "sge", "ult", "ule", "ugt", "uge"};
  json::Object object{{"op", names[(unsigned)t->op]}, {"width", (int64_t)t->sort.width}};
  if (t->op == TermOp::Const) {
    llvm::SmallString<32> text;
    t->value.toStringUnsigned(text, 10);
    object["value"] = text.str().str();
  }
  if (t->op == TermOp::Param || t->op == TermOp::State || t->op == TermOp::Shadow)
    object["index"] = (int64_t)t->index;
  if (t->op == TermOp::ICmp)
    object["predicate"] = predicates[t->aux];
  json::Array args;
  for (Term a : t->args)
    args.push_back(termJSON(a, budget));
  object["args"] = std::move(args);
  return object;
}

json::Value rankedTerm(Term t) {
  unsigned budget = 1u << 14;
  json::Value value = termJSON(t, budget);
  if (budget == 0)
    return nullptr; // Too large to print expanded. Consumers skip it.
  return value;
}

json::Array transitionList(const SemanticProgram &program,
                           llvm::ArrayRef<unsigned> transitions) {
  json::Array out;
  for (unsigned index : transitions) {
    const Transition &t = program.transitions[index];
    out.push_back(json::Array{(int64_t)t.from, (int64_t)t.slot, (int64_t)t.to});
  }
  return out;
}

json::Value levelJSON(const SemanticProgram &program, const RankLevel &level) {
  json::Array cuts, ranks, paths, strict, children;
  for (unsigned cut : level.cuts)
    cuts.push_back((int64_t)cut);
  for (const auto &[cut, term] : level.rank)
    ranks.push_back(json::Object{{"cut", (int64_t)cut}, {"term", rankedTerm(term)}});
  for (const auto &path : level.paths)
    paths.push_back(transitionList(program, path));
  for (const auto &path : level.strict)
    strict.push_back(transitionList(program, path));
  for (const RankLevel &child : level.children)
    children.push_back(levelJSON(program, child));
  return json::Object{{"cuts", std::move(cuts)},
                      {"origin", level.origin},
                      {"rank", std::move(ranks)},
                      {"paths", std::move(paths)},
                      {"strict", std::move(strict)},
                      {"children", std::move(children)}};
}

json::Value entryJSON(const EntryState &entry) {
  auto text = [](const APInt &value) {
    llvm::SmallString<32> digits;
    value.toStringUnsigned(digits, 10);
    return digits.str().str();
  };
  json::Array parameters, memory;
  for (const APInt &value : entry.parameters)
    parameters.push_back(text(value));
  for (const RootData &root : entry.memory) {
    json::Array shape, cells;
    for (uint64_t extent : root.shape)
      shape.push_back((int64_t)extent);
    for (const auto &cell : root.cells)
      if (cell)
        cells.push_back(text(*cell));
      else
        cells.push_back(nullptr);
    memory.push_back(json::Object{{"shape", std::move(shape)}, {"cells", std::move(cells)}});
  }
  return json::Object{{"parameters", std::move(parameters)}, {"memory", std::move(memory)}};
}

} // namespace

namespace mlir::generalized_gemm {

TerminationResult analyzeTermination(const SemanticProgram &program,
                                     SmtBackend &backend,
                                     const TerminationOptions &options,
                                     llvm::ArrayRef<RankHint> hints) {
  TerminationResult result;
  result.solver = backend.version();
  for (unsigned k = 0; k < 3; ++k) {
    result.stops[k].kind = (SideKind)k;
    result.stops[k].status = StopStatus::Unknown;
  }
  auto unknown = [&](std::string reason) {
    result.verdict = CheckedVerdict::unknown(reason);
    result.status = ProofStatus::unknown(std::move(reason));
    return std::move(result);
  };
  if (!program.isSupported())
    return unknown(std::string("unsupported: ") +
                   stringifyUnsupportedKind(program.unsupported->kind) + ": " +
                   program.unsupported->detail);
  std::string why;
  Term scope = scopeFormula(program, options.scope, why);
  if (!scope)
    return unknown("malformed scope: " + why);

  TerminationCertificate certificate;
  certificate.fingerprint = program.fingerprint;
  certificate.cuts = computeCutSet(program);
  std::optional<CheckOutcome> outcome;
  try {
    if (backend.check(obligationScript("domain", scope), options.limits.solver,
                      /*wantModel=*/false)
            .answer != SmtAnswer::Sat)
      return unknown("the parameter domain is empty or could not be shown nonempty");
    PathSet set = enumerateCutPaths(program, certificate.cuts, options.limits.paths);
    if (!set.complete)
      return unknown("path enumeration: " + set.reason);
    ProposalInput input{program, set, certificate.cuts, scope,
                        options.limits, backend, hints};
    auto invariants = discoverInvariants(input, result.statistics);
    auto levels = proposeRankLevels(input, invariants, result.statistics);
    if (levels) {
      for (const auto &[cut, term] : invariants)
        certificate.invariants.push_back({cut, term});
      certificate.levels = std::move(*levels);
      CheckOptions check{options.scope, options.limits, options.authorized};
      outcome = checkCertificate(program, certificate, check, backend);
      result.records = outcome->records;
      result.assumptionsUsed = outcome->assumptionsUsed;
      result.stops = outcome->stops;
    }
  } catch (const TermError &error) {
    return unknown(std::string("term limit: ") + error.what());
  }

  bool proven = outcome && outcome->verdict.kind() != TerminationVerdict::Unknown;
  CheckedVerdict verdict =
      outcome ? outcome->verdict
              : CheckedVerdict::unknown(result.statistics.failure.empty()
                                            ? std::string("no ranking proof found")
                                            : result.statistics.failure);
  result.status = outcome ? outcome->status : ProofStatus::unknown(verdict.reason());

  uint64_t budget = options.limits.replayFuel * 4;
  for (const auto &point :
       parameterPoints(program, options.scope, options.limits, result.statistics)) {
    for (bool fillOnes : {false, true}) {
      if (budget == 0)
        break;
      if (fillOnes && program.roots.empty())
        continue;
      auto entry = makeEntry(program, point, fillOnes, options.limits.replayCells);
      if (!entry)
        continue;
      uint64_t fuel = std::min(budget, options.limits.replayFuel);
      ReplayReport replay = replayEntry(program, *entry, fuel, options.limits.replayCells);
      budget -= std::min(budget, std::max<uint64_t>(replay.steps, 1));
      if (replay.end == ReplayEnd::Cycle) {
        if (proven) {
          result.conflict = "a proof and an exact cycle disagree";
          verdict = CheckedVerdict::unknown(result.conflict);
          result.status = ProofStatus::unknown(result.conflict);
          proven = false;
        } else if (result.conflict.empty()) {
          CheckedVerdict refuted = verifyNonterminationWitness(
              program, options.scope, *replay.witness, options.limits.replayCells);
          verdict = refuted; // UNKNOWN, with its reason, unless it was verified.
          if (refuted.kind() == TerminationVerdict::RefutedNonterminating) {
            result.status = ProofStatus::refuted(refuted.reason());
            result.witness = replay.witness;
          }
        }
      } else if (replay.end == ReplayEnd::Stopped) {
        SideKind kind = kindOfStop(replay.stop);
        StopReport &report = result.stops[(unsigned)kind];
        if (proven && report.status == StopStatus::Unreachable) {
          result.conflict = "a proof of an unreachable stop and a replayed stop "
                            "disagree: " + replay.reason;
          verdict = CheckedVerdict::unknown(result.conflict);
          result.status = ProofStatus::unknown(result.conflict);
          proven = false;
        }
        if (report.status != StopStatus::Reachable) {
          report.status = StopStatus::Reachable;
          report.detail = replay.reason;
          report.witness = *entry;
        }
      }
    }
    if (result.witness || !result.conflict.empty())
      break;
  }
  result.verdict = verdict;
  if (proven)
    result.certificate = std::move(certificate);
  return result;
}

std::string toJSON(const SemanticProgram &program, const TerminationResult &result,
                   bool scripts) {
  json::Object out{{"function", program.functionName},
                   {"fingerprint", program.fingerprint},
                   {"verdict", stringifyTerminationVerdict(result.verdict.kind())},
                   {"reason", result.verdict.reason()},
                   {"status", result.status.name()},
                   {"solver", result.solver},
                   {"conflict", result.conflict}};
  json::Array stops;
  for (const StopReport &report : result.stops) {
    json::Object stop{{"kind", stringifySideKind(report.kind)},
                      {"status", stringifyStopStatus(report.status)},
                      {"sites", (int64_t)report.sites},
                      {"discharged", (int64_t)report.discharged},
                      {"detail", report.detail}};
    if (report.witness)
      stop["witness"] = entryJSON(*report.witness);
    stops.push_back(std::move(stop));
  }
  out["stops"] = std::move(stops);
  json::Array assumptions;
  for (const std::string &name : result.assumptionsUsed)
    assumptions.push_back(name);
  out["assumptions"] = std::move(assumptions);

  json::Array state;
  for (const StateVar &s : program.state)
    state.push_back(json::Object{
        {"id", (int64_t)s.id},
        {"kind", s.kind == StateVar::BlockArgument ? "argument"
                 : s.kind == StateVar::IterationCounter ? "counter" : "result"},
        {"location", (int64_t)s.location},
        {"position", (int64_t)s.position}});
  out["state"] = std::move(state);

  if (result.certificate) {
    const TerminationCertificate &c = *result.certificate;
    json::Array cuts, invariants, levels;
    for (unsigned cut : c.cuts)
      cuts.push_back((int64_t)cut);
    for (const auto &[cut, term] : c.invariants)
      invariants.push_back(json::Object{{"cut", (int64_t)cut}, {"term", rankedTerm(term)}});
    for (const RankLevel &level : c.levels)
      levels.push_back(levelJSON(program, level));
    out["certificate"] = json::Object{{"cuts", std::move(cuts)},
                                      {"invariants", std::move(invariants)},
                                      {"levels", std::move(levels)}};
  }
  if (result.witness)
    out["witness"] = json::Object{{"entry", entryJSON(result.witness->entry)},
                                  {"prefix", (int64_t)result.witness->prefix},
                                  {"period", (int64_t)result.witness->period}};

  json::Array records;
  for (const ObligationRecord &record : result.records) {
    json::Array premises;
    for (const std::string &name : record.assumptions)
      premises.push_back(name);
    json::Object entry{{"id", record.id},
                       {"location", record.location},
                       {"expected", record.expected},
                       {"result", record.result},
                       {"evidence", record.evidence},
                       {"assumptions", std::move(premises)},
                       {"discharged", record.discharged}};
    if (scripts && !record.script.empty())
      entry["script"] = record.script;
    records.push_back(std::move(entry));
  }
  out["obligations"] = std::move(records);
  out["proposal"] = json::Object{
      {"queries", (int64_t)result.statistics.queries},
      {"rank_candidates", (int64_t)result.statistics.candidates},
      {"invariants_kept", (int64_t)result.statistics.invariantsKept},
      {"failure", result.statistics.failure}};
  std::string text;
  llvm::raw_string_ostream stream(text);
  stream << llvm::formatv("{0:1}", json::Value(std::move(out)));
  return stream.str();
}

} // namespace mlir::generalized_gemm
