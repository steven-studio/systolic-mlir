//===- NestRecord.cpp - One record per loop nest -------------------------===//
//
// See NestRecord.h. The pipeline below is the domain driver's, on the nest's copy, and
// the record is read off its results. No status is made here: a field is a fact only
// where a checker's claim says so, and anything else is a refusal in words.
//
//===----------------------------------------------------------------------===//
#include "GeneralizedGemm/NestRecord.h"

#include "GeneralizedGemm/PolynomialAdapter.h"
#include "Systolic/NestBinding.h"
#include "GeneralizedGemm/SemanticImport.h"
#include "GeneralizedGemm/TerminationAnalysis.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <map>

using namespace mlir;
using namespace mlir::generalized_gemm;
using llvm::APInt;
namespace json = llvm::json;

namespace {

std::optional<int64_t> constantOf(Term term, bool isSigned) {
  if (!term || term->op != TermOp::Const || term->value.getBitWidth() > 64)
    return std::nullopt;
  if (isSigned)
    return term->value.getSExtValue();
  if (term->value.getBitWidth() == 64 && term->value.isNegative())
    return std::nullopt;
  return (int64_t)term->value.getZExtValue();
}

bool isTrue(Term guard) {
  return !guard || (guard->op == TermOp::Const && guard->value.isOne());
}

/// constant + sum of coefficient * leaf, at the term's width. Only what
/// affineOverLevels admits is decomposed, so the two never disagree on a form.
struct Affine {
  APInt constant;
  std::map<unsigned, APInt> levels, parameters;
  bool isConstant() const { return levels.empty() && parameters.empty(); }
};

std::optional<Affine> decompose(Term term) {
  if (!term || term->sort.kind != Sort::BV || term->sort.width > 64)
    return std::nullopt;
  unsigned width = term->sort.width;
  Affine out{APInt(width, 0), {}, {}};
  auto add = [&](std::map<unsigned, APInt> &into, const std::map<unsigned, APInt> &from, bool negate) {
    for (const auto &[leaf, coefficient] : from) {
      auto found = into.try_emplace(leaf, APInt(width, 0)).first;
      found->second = negate ? found->second - coefficient : found->second + coefficient;
      if (found->second.isZero())
        into.erase(found);
    }
  };
  switch (term->op) {
  case TermOp::Const:
    out.constant = term->value;
    return out;
  case TermOp::State:
    if (auto level = levelOf(term)) {
      out.levels[*level] = APInt(width, 1);
      return out;
    }
    return std::nullopt;
  case TermOp::Param:
    out.parameters[term->index] = APInt(width, 1);
    return out;
  case TermOp::Add:
  case TermOp::Sub: {
    auto a = decompose(term->args[0]), b = decompose(term->args[1]);
    if (!a || !b)
      return std::nullopt;
    bool negate = term->op == TermOp::Sub;
    out = *a;
    out.constant = negate ? a->constant - b->constant : a->constant + b->constant;
    add(out.levels, b->levels, negate);
    add(out.parameters, b->parameters, negate);
    return out;
  }
  case TermOp::Mul: {
    auto a = decompose(term->args[0]), b = decompose(term->args[1]);
    if (!a || !b || (!a->isConstant() && !b->isConstant()))
      return std::nullopt;
    const Affine &scale = a->isConstant() ? *a : *b, &other = a->isConstant() ? *b : *a;
    out.constant = other.constant * scale.constant;
    for (const auto &[leaf, coefficient] : other.levels)
      if (APInt product = coefficient * scale.constant; !product.isZero())
        out.levels[leaf] = product;
    for (const auto &[leaf, coefficient] : other.parameters)
      if (APInt product = coefficient * scale.constant; !product.isZero())
        out.parameters[leaf] = product;
    return out;
  }
  default:
    return std::nullopt;
  }
}

/// The parameters a term reads, in order of first occurrence.
void parametersOf(Term term, std::vector<unsigned> &out) {
  if (!term)
    return;
  if (term->op == TermOp::Param) {
    if (std::find(out.begin(), out.end(), term->index) == out.end())
      out.push_back(term->index);
    return;
  }
  for (Term argument : term->args)
    parametersOf(argument, out);
}

/// An affine form as it reads: 2*level0 + level2 + 3.
std::string affineText(const Affine &form) {
  std::string text;
  auto term = [&](int64_t coefficient, const std::string &leaf) {
    if (!text.empty())
      text += coefficient < 0 ? " - " : " + ";
    else if (coefficient < 0)
      text += "-";
    uint64_t magnitude = coefficient < 0 ? 0 - (uint64_t)coefficient : (uint64_t)coefficient;
    if (magnitude != 1)
      text += std::to_string(magnitude) + "*";
    text += leaf;
  };
  for (const auto &[level, coefficient] : form.levels)
    term(coefficient.getSExtValue(), "level" + std::to_string(level));
  for (const auto &[parameter, coefficient] : form.parameters)
    term(coefficient.getSExtValue(), "parameter" + std::to_string(parameter));
  int64_t constant = form.constant.getSExtValue();
  if (text.empty())
    return std::to_string(constant);
  if (constant)
    text += (constant < 0 ? " - " : " + ") + std::to_string(constant < 0 ? 0 - (uint64_t)constant : (uint64_t)constant);
  return text;
}

json::Object factJSON(const Fact &fact) {
  json::Object out{{"status", stringifyFactStatus(fact.status)},
                   {"tier", stringifyComputationTier((ComputationTier)fact.tier)},
                   {"detail", fact.detail}};
  if (fact.point) {
    json::Array point;
    for (const APInt &value : *fact.point)
      point.push_back(value.getSExtValue());
    out["point"] = std::move(point);
  }
  out["witness_memory"] = fact.memory.has_value();
  return out;
}

bool provenOverScope(const Fact &fact) {
  return fact.status == FactStatus::Proven && fact.tier == (int)ComputationTier::Symbolic;
}

std::string factText(const Fact &fact) {
  return std::string(stringifyFactStatus(fact.status)) + " at " +
         stringifyComputationTier((ComputationTier)fact.tier) +
         (fact.detail.empty() ? std::string() : " (" + fact.detail + ")");
}

const char *axisName(char axis) {
  switch (axis) {
  case 'P':
    return "P";
  case 'I':
    return "I";
  case 'J':
    return "J";
  case 'K':
    return "K";
  case 'R':
    return "R";
  default:
    return "?";
  }
}

json::Array accessJSON(unsigned access) {
  json::Array out;
  if (access & NestReads)
    out.push_back("read");
  if (access & NestWrites)
    out.push_back("write");
  return out;
}

} // namespace

namespace mlir::generalized_gemm {

const char *stringifyNestStatus(NestRecord::Status status) {
  switch (status) {
  case NestRecord::Unsupported:
    return "UNSUPPORTED";
  case NestRecord::NoGemm:
    return "NO_GEMM";
  case NestRecord::NotVerified:
    return "NOT_VERIFIED";
  case NestRecord::VerifiedNotMapped:
    return "VERIFIED_NOT_MAPPED";
  case NestRecord::Verified:
    return "VERIFIED";
  }
  return "UNSUPPORTED";
}

std::vector<std::string> flagNames(unsigned flags, bool isFloat) {
  std::vector<std::string> out;
  if (isFloat) {
    static const std::pair<unsigned, const char *> names[] = {
        {FlagReassoc, "reassoc"}, {FlagNNaN, "nnan"},        {FlagNInf, "ninf"}, {FlagNSZ, "nsz"},
        {FlagARCFP, "arcp"},      {FlagContract, "contract"}, {FlagAFN, "afn"}};
    for (const auto &[bit, name] : names)
      if (flags & bit)
        out.push_back(name);
  } else {
    static const std::pair<unsigned, const char *> names[] = {
        {FlagNSW, "nsw"}, {FlagNUW, "nuw"}, {FlagExact, "exact"}, {FlagNNeg, "nneg"}};
    for (const auto &[bit, name] : names)
      if (flags & bit)
        out.push_back(name);
  }
  return out;
}

NestRecord analyzeNest(const OutlinedNest &nest, const NestAnalysisOptions &options, SmtBackend &backend) {
  NestRecord r;
  r.function = nest.function;
  r.loop = nest.loop;
  r.location = nest.location;
  r.nest = nest.ordinal;
  r.statement = nest.statement;
  r.conditions = nest.conditions;
  r.line = nest.line;
  r.column = nest.column;
  r.roots = nest.roots;
  r.separations = nest.separations;
  r.separationUsed.assign(nest.separations.size(), false);
  r.bindingSha = nest.bindingSha;
  r.bindingRefusal = nest.bindingRefusal;
  r.indexWidth = options.indexBitWidth;
  if (!nest.unsupported.empty()) {
    r.outlineRefusal = nest.unsupported;
    r.refusals.push_back("the nest has no faithful copy: " + nest.unsupported);
    return r;
  }
  r.outlinedName = nest.outlinedName;
  r.scalars = nest.scalars;
  r.constantsCopied = nest.constantsCopied;

  // The domain driver's pipeline, on the copy.
  func::FuncOp function = nest.outlined();
  ImportOptions importOptions;
  importOptions.indexBitWidth = options.indexBitWidth;
  importOptions.lowerStructured = true;
  SemanticProgram program = importFunction(function, importOptions);
  if (!program.isSupported()) {
    r.modelRefusal = std::string(stringifyUnsupportedKind(program.unsupported->kind)) + ": " +
                     program.unsupported->detail;
    r.refusals.push_back("the importer refuses the nest's copy: " + r.modelRefusal);
    return r;
  }
  r.sites = program.sites.size();

  // The scope, given over the function's arguments, on the copy's parameters.
  ParameterScope scope;
  if (!options.scope.empty()) {
    scope.allParameters = false;
    for (const Parameter &parameter : program.parameters) {
      if (parameter.kind != Parameter::Scalar)
        continue;
      for (const NestScalar &scalar : nest.scalars) {
        if (scalar.outlined != parameter.argument || scalar.source != NestScalar::FunctionArgument)
          continue;
        for (const NestScopeEntry &entry : options.scope) {
          if (entry.argument != scalar.argument)
            continue;
          unsigned width = parameter.sort.width;
          APInt low(64, entry.lower, /*isSigned=*/true), high(64, entry.upper, /*isSigned=*/true);
          if (!low.isSignedIntN(width) || !high.isSignedIntN(width)) {
            r.refusals.push_back("the scope of argument " + std::to_string(entry.argument) +
                                 " does not fit its width");
            return r;
          }
          scope.ranges.push_back({parameter.id, low.trunc(width), high.trunc(width)});
          r.scope.push_back(entry);
          r.assumptions.push_back("argument " + std::to_string(entry.argument) + " of the function is in [" +
                                  std::to_string(entry.lower) + ", " + std::to_string(entry.upper) +
                                  "]: the scope of every claim");
        }
      }
    }
  }

  // Separation, as it stands in the function, handed over by name.
  Authorizations authorized;
  for (const NestSeparation &separation : nest.separations)
    if (separation.apart)
      authorized.insert(separation.outlinedName);

  TerminationOptions termination;
  SynthesisOptions synthesis;
  termination.scope = synthesis.scope = scope;
  termination.limits.solver.timeoutMilliseconds = options.timeoutMs;
  termination.limits.solver.resourceLimit = options.resourceLimit;
  termination.authorized = authorized;
  synthesis.termination = termination.limits;
  synthesis.authorized = authorized;
  synthesis.limits.maxPoints = options.maxPoints;
  synthesis.maxLevel = options.maxLevel;
  synthesis.synthesis.maxCandidates = options.maxCandidates;
  synthesis.synthesis.maxRounds = options.maxRounds;
  synthesis.synthesis.solver = termination.limits.solver;
  synthesis.referenceFill = options.referenceFill;
  std::optional<TerminationCertificate> certificate;
  {
    auto hints = polynomialRankHints(program, function, importOptions);
    certificate = analyzeTermination(program, backend, termination, hints).certificate;
  }
  DomainAnalysisRequest request;
  request.synthesis = synthesis;
  request.domain.scope = scope;
  request.domain.termination = termination.limits;
  request.domain.authorized = authorized;
  request.domain.limits.maxBox = options.maxBox;
  request.domain.limits.maxQueries = options.maxQueries;
  request.domain.computation.replay.maxSteps = options.replaySteps;
  request.domain.computation.replay.maxNodes = options.replayNodes;
  request.proposal.maxBox = options.maxBox;
  request.maxCandidates = options.candidatesPerGroup;
  request.closedForms.scope = scope;
  request.closedForms.solver = termination.limits.solver;
  request.closedForms.referenceFill = options.referenceFill;
  DomainResult result = analyzeDomains(program, certificate ? &*certificate : nullptr, request, backend);

  const DomainEnumeration &enumeration = result.enumeration;
  r.gate = stringifyGateStatus(enumeration.gate);
  r.gateReason = enumeration.reason;
  r.termination = stringifyTerminationVerdict(enumeration.termination.kind());
  r.groups = enumeration.groups.size();

  // A root of the copy is the function's root at the same place of the nest's list.
  auto functionRoot = [&](unsigned outlined) -> int {
    return outlined < nest.roots.size() ? (int)nest.roots[outlined].root : -1;
  };
  auto translate = [&](const std::string &name) {
    unsigned a = 0, b = 0;
    char tail = 0;
    if (std::sscanf(name.c_str(), "roots %u and %u are separate%c", &a, &b, &tail) == 2 &&
        functionRoot(a) >= 0 && functionRoot(b) >= 0)
      return separationName(functionRoot(a), functionRoot(b));
    return name;
  };
  for (const std::string &name : enumeration.assumptionsUsed)
    r.assumptions.push_back("used by the enumeration's gate: " + translate(name));
  // A parameter of the copy, said in the function's terms.
  auto describeParameter = [&](unsigned id) {
    std::string text = "parameter " + std::to_string(id);
    if (id >= program.parameters.size())
      return text;
    const Parameter &parameter = program.parameters[id];
    if (parameter.kind == Parameter::Dimension)
      return text + " (extent " + std::to_string(parameter.dimension) + " of root " +
             std::to_string(functionRoot(parameter.argument)) + ")";
    for (const NestScalar &scalar : nest.scalars)
      if (scalar.outlined == parameter.argument)
        return text + (scalar.source == NestScalar::FunctionArgument
                           ? " (argument " + std::to_string(scalar.argument) + " of the function)"
                           : " (the value of " + scalar.definition + " at " + scalar.location +
                                 ", computed before the nest)");
    return text;
  };
  auto describeTerm = [&](Term term) {
    std::string text = formatTerm(term);
    std::vector<unsigned> parameters;
    parametersOf(term, parameters);
    for (unsigned id : parameters)
      text += "; it reads " + describeParameter(id);
    return text;
  };

  if (!r.sites) {
    r.status = NestRecord::NoGemm;
    r.refusals.push_back("the nest has no multiply-accumulate site");
    return r;
  }
  r.status = NestRecord::NotVerified;
  if (result.groups.empty()) {
    r.refusals.push_back("no claim was made about the nest: the gate is " + r.gate +
                         (r.gateReason.empty() ? std::string() : " (" + r.gateReason + ")"));
    return r;
  }
  if (r.groups != 1 || result.groups.size() != 1) {
    r.refusals.push_back("the nest has " + std::to_string(r.groups) +
                         " groups of multiply-accumulate sites, and a record describes a nest that is one GEMM");
    return r;
  }
  const GroupDomain &groupDomain = result.groups.front();
  const SiteGroup &siteGroup = enumeration.groups[groupDomain.group];
  r.group = groupDomain.group;
  r.candidates = groupDomain.candidates.size();
  r.sitesInGroup = siteGroup.sites.size();
  if (!siteGroup.unsupported.empty()) {
    r.refusals.push_back("the group is not supported: " + siteGroup.unsupported);
    return r;
  }
  if (groupDomain.candidates.empty()) {
    r.refusals.push_back("no candidate for the nest: the search is " +
                         std::string(stringifyRecoveryStatus(groupDomain.search)) +
                         (groupDomain.searchReason.empty() ? std::string() : " (" + groupDomain.searchReason + ")"));
    return r;
  }

  // The first candidate verified over the scope, in the search's order, or the first.
  unsigned chosen = 0;
  for (unsigned c = 0; c < groupDomain.candidates.size(); ++c) {
    const ReductionVerdict &v = groupDomain.candidates[c].verdict;
    if (v.status() == ComputationStatus::Verified && v.tier() == ComputationTier::Symbolic) {
      chosen = c;
      break;
    }
  }
  r.candidate = chosen;
  const CandidateDomain &candidate = groupDomain.candidates[chosen];
  const ReductionVerdict &v = candidate.verdict;
  const CheckedComputation &computation = v.computationSymbolic();
  const CheckedDomain &domain = v.domainSymbolic();

  // The update, from the sites and from the contracts the computation checker read back.
  r.views[0].root = siteGroup.output ? functionRoot(*siteGroup.output) : -1;
  r.views[1].root = functionRoot(siteGroup.first);
  r.views[2].root = functionRoot(siteGroup.second);
  bool isFloat = false;
  {
    const MacSite &first = program.sites[siteGroup.sites.front()];
    isFloat = first.reduction == ReductionOp::FAdd;
    r.reduction = isFloat ? "fadd" : "add";
    r.multiply = isFloat ? "fmul" : "mul";
    r.fold = stringifyFoldForm(isFloat ? FoldForm::OrderedFloat : FoldForm::ModularInteger);
    r.reductionFlags = first.reductionFlags;
    r.multiplyFlags = first.multiplyFlags;
    if (siteGroup.output && *siteGroup.output < program.roots.size())
      r.element = program.roots[*siteGroup.output].sort.element().str();
    for (unsigned site : siteGroup.sites) {
      const MacSite &other = program.sites[site];
      r.flagsAgree = r.flagsAgree && other.reduction == first.reduction &&
                     other.reductionFlags == first.reductionFlags &&
                     other.multiplyFlags == first.multiplyFlags && other.form == ReductionForm::Direct;
    }
    bool firstContract = true;
    for (const SiteContract &contract : computation.contracts()) {
      r.flagsAgree = r.flagsAgree && contract.reduction == first.reduction &&
                     contract.reductionFlags == first.reductionFlags &&
                     contract.multiplyFlags == first.multiplyFlags &&
                     (firstContract || contract.accumulatorFirst == r.accumulatorFirst);
      r.accumulatorFirst = contract.accumulatorFirst;
      firstContract = false;
    }
  }
  for (const ResidualEffect &effect : computation.frame())
    ++(effect.kind == MemoryEffect::Write ? r.otherWrites : r.otherReads);
  for (const SeparationUse &use : computation.separations())
    for (unsigned s = 0; s < r.separations.size(); ++s)
      if (functionRoot(use.first) >= 0 && functionRoot(use.second) >= 0 &&
          separationName(functionRoot(use.first), functionRoot(use.second)) == r.separations[s].name)
        r.separationUsed[s] = true;

  // The order's levels and their axes.
  static const char axes[] = {'P', 'I', 'J', 'K'};
  std::array<unsigned, 4> perAxis{};
  unsigned repeats = 0, ambiguous = 0;
  if (candidate.request && candidate.request->segments.empty()) {
    const GuardedOrder &order = candidate.request->order;
    for (const GuardedLevel &guarded : order.levels) {
      NestLevel level;
      unsigned matches = 0;
      for (unsigned axis = 0; axis < 4; ++axis)
        for (Term term : candidate.family.axes[axis])
          if (term == guarded.value) {
            level.axis = axes[axis];
            ++matches;
          }
      if (!matches) {
        level.axis = 'R';
        ++repeats;
      } else if (matches > 1) {
        level.axis = '?';
        ++ambiguous;
      } else {
        ++perAxis[level.axis == 'P' ? 0 : level.axis == 'I' ? 1 : level.axis == 'J' ? 2 : 3];
      }
      level.descending = guarded.descending;
      level.isSigned = guarded.isSigned;
      level.lower = constantOf(guarded.lower, guarded.isSigned);
      level.upper = constantOf(guarded.upper, guarded.isSigned);
      if (level.lower && level.upper)
        level.extent = *level.upper > *level.lower ? (uint64_t)(*level.upper - *level.lower) : 0;
      level.lowerText = describeTerm(guarded.lower);
      level.upperText = describeTerm(guarded.upper);
      level.valueText = formatTerm(guarded.value);
      r.levels.push_back(std::move(level));
    }
    r.guard = isTrue(order.guard) ? "true" : formatTerm(order.guard);
    r.box = isTrue(order.guard) && !repeats && !ambiguous && !perAxis[0] && perAxis[1] == 1 &&
            perAxis[2] == 1 && perAxis[3] == 1;
    if (r.box) {
      std::array<uint64_t, 3> extents{};
      bool constant = true;
      for (const NestLevel &level : r.levels) {
        if (!level.extent) {
          constant = false;
          continue;
        }
        extents[level.axis == 'I' ? 0 : level.axis == 'J' ? 1 : 2] = *level.extent;
      }
      if (constant)
        r.extents = extents;
    }
    // The views: closed forms of h, f and g over the levels.
    for (unsigned view = 0; view < 3; ++view) {
      NestView &out = r.views[view];
      const Fact &fact = domain.access()[view];
      out.fact = factText(fact);
      out.proved = provenOverScope(fact);
      out.proposed = !candidate.request->access[view].empty();
      out.affine = out.proposed;
      for (Term term : candidate.request->access[view]) {
        NestViewDimension dimension;
        dimension.text = formatTerm(term);
        std::optional<Affine> form = affineOverLevels(term) ? decompose(term) : std::nullopt;
        dimension.affine = form.has_value();
        if (form) {
          dimension.text = affineText(*form);
          for (const auto &[parameter, coefficient] : form->parameters)
            dimension.text += "; it reads " + describeParameter(parameter);
          dimension.constant = form->constant.getSExtValue();
          bool normal = form->parameters.empty();
          int64_t offset = dimension.constant;
          for (const auto &[level, coefficient] : form->levels) {
            dimension.levels.push_back({level, coefficient.getSExtValue()});
            if (level >= r.levels.size() || r.levels[level].descending || !r.levels[level].lower ||
                !std::strchr("IJK", r.levels[level].axis)) {
              normal = false;
              continue;
            }
            offset += coefficient.getSExtValue() * *r.levels[level].lower;
            dimension.roles.push_back({r.levels[level].axis, coefficient.getSExtValue()});
          }
          for (const auto &[parameter, coefficient] : form->parameters)
            dimension.parameters.push_back({parameter, coefficient.getSExtValue()});
          if (normal)
            dimension.offset = offset;
          else
            dimension.roles.clear();
        }
        out.affine = out.affine && dimension.affine;
        out.dimensions.push_back(std::move(dimension));
      }
    }
  }

  // The claims, as the checkers state them.
  r.tiers = json::Object{
      {"termination", r.termination},
      {"theorem13", json::Object{{"status", stringifyComputationStatus(v.status())},
                                 {"tier", stringifyComputationTier(v.tier())},
                                 {"logical_tier", stringifyComputationTier(v.logicalTier())},
                                 {"holds_for", v.everyRun() ? "every run" : "runs that complete"},
                                 {"reason", v.reason()}}},
      {"computation",
       json::Object{{"finite", json::Object{{"status", stringifyComputationStatus(v.computationFinite().status())},
                                            {"tier", stringifyComputationTier(v.computationFinite().tier())},
                                            {"reason", v.computationFinite().reason()}}},
                    {"symbolic", json::Object{{"status", stringifyComputationStatus(computation.status())},
                                              {"tier", stringifyComputationTier(computation.tier())},
                                              {"reason", computation.reason()}}}}},
      {"domain",
       json::Object{{"finite", json::Object{{"status", stringifyComputationStatus(v.domainFinite().status())},
                                            {"tier", stringifyComputationTier(v.domainFinite().tier())},
                                            {"exact_order", v.domainFinite().exactOrder()},
                                            {"reason", v.domainFinite().reason()}}},
                    {"symbolic", json::Object{{"status", stringifyComputationStatus(domain.status())},
                                              {"tier", stringifyComputationTier(domain.tier())},
                                              {"exact_order", domain.exactOrder()},
                                              {"multiplicity", stringifyDomainMultiplicity(domain.multiplicity())},
                                              {"reason", domain.reason()}}}}},
      {"coordinate", json::Object{{"finite", stringifyEvidenceTier(v.coordinateFinite().tier())},
                                  {"symbolic", stringifyEvidenceTier(v.coordinateSymbolic().tier())}}},
      {"facts", json::Object{{"cell_key", factJSON(domain.cellKey())},
                             {"output_injective", factJSON(domain.outputInjective())},
                             {"single_contribution", factJSON(domain.singleContribution())}}}};
  r.reorder = json::Object{{"trivial", factJSON(v.trivialReorder())},
                           {"integer", factJSON(v.integerReorder())},
                           {"float", factJSON(v.floatReorder())}};

  // The ordered fold, for the nest as a whole.
  bool verified = v.status() == ComputationStatus::Verified;
  bool overScope = verified && v.tier() == ComputationTier::Symbolic &&
                   computation.status() == ComputationStatus::Verified &&
                   computation.tier() == ComputationTier::Symbolic;
  if (!verified)
    r.refusals.push_back(std::string("the ordered fold is not verified: Theorem 13 is ") +
                         stringifyComputationStatus(v.status()) + " (" + v.reason() + ")");
  else if (!overScope)
    r.refusals.push_back(std::string("the ordered fold is verified at ") + stringifyComputationTier(v.tier()) +
                         " only, and not over the scope");
  if (overScope && r.otherWrites)
    r.refusals.push_back("the nest writes memory outside the multiply-accumulate: " +
                         std::to_string(r.otherWrites) + " write(s) in the frame of the computation claim");
  r.orderedFoldVerified = overScope && !r.otherWrites;
  if (r.orderedFoldVerified)
    r.orderedFoldReason = std::string("Theorem 13 is VERIFIED at SYMBOLIC (") + v.reason() + "), for " +
                          (v.everyRun() ? "every run" : "the runs that complete") +
                          ", and the frame of the computation claim has no write";
  else
    r.orderedFoldReason = r.refusals.empty() ? std::string("not verified") : r.refusals.back();

  // An operand that may overlap the output, said in the function's own terms.
  for (unsigned view = 1; view < 3; ++view) {
    int c = r.views[0].root, operand = r.views[view].root;
    if (c < 0 || operand < 0 || c == operand)
      continue;
    for (const NestSeparation &separation : r.separations)
      if (separation.name == separationName(c, operand) && !separation.apart)
        r.refusals.push_back(std::string(view == 1 ? "A" : "B") + " (root " + std::to_string(operand) +
                             ") may overlap C (root " + std::to_string(c) +
                             "): the two are not known apart. A fresh allocation proves it; the name \"" +
                             separation.name + "\" assumes it");
  }

  // Whether the source lets the order change.
  bool reassoc = isFloat;
  for (unsigned site : siteGroup.sites)
    reassoc = reassoc && (program.sites[site].reductionFlags & FlagReassoc);
  if (isFloat)
    r.sourceFlagReassoc = reassoc;
  if (provenOverScope(v.trivialReorder())) {
    r.reassociationPermitted = true;
    r.reassociationBasis = "proof";
    r.reassociationDetail = "no cell has two contributions, so there is no order to change: " +
                            v.trivialReorder().detail;
  } else if (!isFloat && provenOverScope(v.integerReorder())) {
    r.reassociationPermitted = true;
    r.reassociationBasis = "proof";
    r.reassociationDetail = v.integerReorder().detail;
  } else if (isFloat && reassoc) {
    r.reassociationPermitted = true;
    r.reassociationBasis = "source flag";
    r.reassociationDetail = "every accumulation of the nest carries reassoc: the programmer's permission, "
                            "and no proof that another order gives the same bits (the float reorder fact is " +
                            std::string(stringifyFactStatus(v.floatReorder().status)) + ")";
  } else {
    r.reassociationBasis = "none";
    r.reassociationDetail = isFloat ? "an accumulation of the nest carries no reassoc flag, and the float reorder fact is " +
                                          std::string(stringifyFactStatus(v.floatReorder().status))
                                    : "the integer reorder fact is " + factText(v.integerReorder());
  }

  // What the checkers assumed.
  for (unsigned s = 0; s < r.separations.size(); ++s)
    if (r.separations[s].apart && !r.separations[s].proof)
      r.assumptions.push_back(r.separations[s].name + ": " + r.separations[s].basis + ", not proved" +
                              (r.separationUsed[s] ? ", and used by the computation claim" : ""));
  if (verified && !v.everyRun()) {
    std::string stops;
    for (const MemoryDependentStop &stop : computation.memoryStops())
      stops += (stops.empty() ? "" : "; ") + stop.reason;
    r.assumptions.push_back("the claims are about the runs of the nest that complete" +
                            (stops.empty() ? std::string() : ": a run may stop on " + stops));
  }
  r.assumptions.push_back("index values are bit vectors of width " + std::to_string(options.indexBitWidth));

  // Whether the record gives a partitioner its input: a constant box and three views.
  NestMappingFacts facts;
  facts.singleOrder = candidate.request && candidate.request->segments.empty();
  facts.logicalSymbolic = v.logicalTier() == ComputationTier::Symbolic;
  facts.logicalTier = stringifyComputationTier(v.logicalTier());
  facts.exactOrder = domain.exactOrder();
  facts.unitMultiplicity = domain.multiplicity() == DomainMultiplicity::Unit;
  facts.multiplicity = stringifyDomainMultiplicity(domain.multiplicity());
  facts.outputInjective = provenOverScope(domain.outputInjective());
  facts.outputInjectiveFact = factText(domain.outputInjective());
  std::vector<std::string> unmapped = mappingRefusals(r, facts);
  bool mapped = unmapped.empty() && r.extents.has_value();
  for (std::string &text : unmapped)
    r.refusals.push_back(std::move(text));
  r.status = !r.orderedFoldVerified ? NestRecord::NotVerified
             : mapped && r.refusals.empty() ? NestRecord::Verified
                                            : NestRecord::VerifiedNotMapped;
  return r;
}

std::vector<std::string> mappingRefusals(const NestRecord &r, const NestMappingFacts &facts) {
  std::vector<std::string> out;
  if (!facts.singleOrder) {
    out.push_back("the candidate has no single order over its family");
    return out;
  }
  if (!facts.logicalSymbolic)
    out.push_back("the coordinates I, J and K are not verified over the scope: the logical tier is " +
                  facts.logicalTier);
  if (!facts.exactOrder)
    out.push_back("the order is not proved exact over the scope");
  if (!facts.unitMultiplicity)
    out.push_back("a point of the order has more than one event: the multiplicity is " + facts.multiplicity);
  if (r.guard != "true")
    out.push_back("the domain has a guard, so it is no box: " + r.guard);
  // The levels, by the axis each is a coordinate of.
  static const char axes[] = {'P', 'I', 'J', 'K'};
  std::array<unsigned, 4> perAxis{};
  unsigned repeats = 0, ambiguous = 0;
  for (const NestLevel &level : r.levels) {
    repeats += level.axis == 'R';
    ambiguous += level.axis == '?';
    for (unsigned axis = 0; axis < 4; ++axis)
      perAxis[axis] += level.axis == axes[axis];
  }
  if (repeats)
    out.push_back(std::to_string(repeats) + " level(s) of the order are no coordinate of the family");
  if (ambiguous)
    out.push_back(std::to_string(ambiguous) + " level(s) of the order are coordinates of two axes");
  if (perAxis[0])
    out.push_back("the family has a batch coordinate (axis P), and batches are not mapped");
  for (unsigned axis = 1; axis < 4; ++axis)
    if (perAxis[axis] != 1)
      out.push_back(std::string("axis ") + axes[axis] + " has " + std::to_string(perAxis[axis]) +
                    " levels, and a box has one");
  for (unsigned l = 0; l < r.levels.size(); ++l) {
    const NestLevel &level = r.levels[l];
    std::string where = "level " + std::to_string(l) + " (axis " + axisName(level.axis) + ")";
    if (!level.lower)
      out.push_back("no constant extent: the lower bound of " + where + " is " + level.lowerText);
    if (!level.upper)
      out.push_back("no constant extent: the upper bound of " + where + " is " + level.upperText);
    if (level.extent && !*level.extent)
      out.push_back(where + " is empty");
    if (level.descending)
      out.push_back(where + " descends, and a descending level is not mapped");
  }
  static const char *const names[] = {"C", "A", "B"};
  static const char *const roles[] = {"IJ", "IK", "KJ"};
  for (unsigned view = 0; view < 3; ++view) {
    const NestView &one = r.views[view];
    if (one.root < 0)
      out.push_back(std::string("the root of ") + names[view] + " is not a root of the function");
    if (!one.proposed) {
      out.push_back(std::string("the view of ") + names[view] + " has no proposed closed form");
      continue;
    }
    if (!one.proved)
      out.push_back(std::string("the view of ") + names[view] + " is not proved over the scope: " + one.fact);
    for (unsigned d = 0; d < one.dimensions.size(); ++d) {
      const NestViewDimension &dimension = one.dimensions[d];
      std::string where = std::string("dimension ") + std::to_string(d) + " of the view of " + names[view];
      if (!dimension.affine) {
        out.push_back(where + " is not affine: " + dimension.text);
        continue;
      }
      if (!dimension.parameters.empty())
        out.push_back(where + " reads a parameter, and a symbolic offset is not mapped: " + dimension.text);
      for (const auto &[level, coefficient] : dimension.levels) {
        char axis = level < r.levels.size() ? r.levels[level].axis : '?';
        if (!std::strchr(roles[view], axis))
          out.push_back(where + " reads level " + std::to_string(level) + " (axis " + axisName(axis) +
                        "), which is no role of " + names[view]);
      }
    }
  }
  if (!facts.outputInjective)
    out.push_back("two logical outputs may share a cell of C: the fact is " + facts.outputInjectiveFact);
  if (!r.flagsAgree)
    out.push_back("the sites of the group do not agree on their operation and flags");
  return out;
}

json::Object toJSON(const NestRecord &r, const std::vector<FunctionRoot> &roots) {
  auto rootJSON = [&](int id) -> json::Value {
    if (id < 0 || (size_t)id >= roots.size())
      return nullptr;
    const FunctionRoot &root = roots[id];
    json::Object out{{"root", (int64_t)root.id}, {"type", root.type}};
    if (root.origin == FunctionRoot::FunctionArgument) {
      out["origin"] = "argument";
      out["argument"] = (int64_t)root.argument;
      out["noalias"] = root.restrictArgument;
    } else {
      out["origin"] = "allocation";
      out["location"] = root.location;
    }
    return out;
  };
  json::Object out{{"function", r.function},
                   {"nest", (int64_t)r.nest},
                   {"loop", r.loop},
                   {"location", r.location},
                   {"line", (int64_t)r.line},
                   {"column", (int64_t)r.column},
                   {"statement", (int64_t)r.statement},
                   {"conditions", (int64_t)r.conditions},
                   {"status", stringifyNestStatus(r.status)},
                   {"authorizes_lowering", false}};

  // Stage 2: which loop of which input the record is about (Systolic/NestBinding.h), the
  // index width of its claims, and the ranges of arguments they are restricted to (empty:
  // every value). A reader that cannot recompute the binding has no record.
  json::Array scopeEntries;
  for (const NestScopeEntry &entry : r.scope)
    scopeEntries.push_back(
        json::Object{{"argument", (int64_t)entry.argument}, {"lower", entry.lower}, {"upper", entry.upper}});
  out["binding"] = json::Object{{"scheme", mlir::systolic::kNestBindingScheme},
                                {"sha256", r.bindingSha.empty() ? json::Value(nullptr) : json::Value(r.bindingSha)},
                                {"reason", r.bindingRefusal},
                                {"index_width", (int64_t)r.indexWidth}};
  out["scope"] = std::move(scopeEntries);

  // The copy and its inputs.
  json::Array inputRoots, inputScalars, separations;
  for (const NestRoot &root : r.roots) {
    json::Value one = rootJSON(root.root);
    if (json::Object *object = one.getAsObject()) {
      (*object)["as"] = (int64_t)root.outlined;
      (*object)["access"] = accessJSON(root.access);
    }
    inputRoots.push_back(std::move(one));
  }
  for (const NestScalar &scalar : r.scalars) {
    json::Object one{{"as", (int64_t)scalar.outlined}, {"type", scalar.type}, {"definition", scalar.definition}};
    if (scalar.source == NestScalar::FunctionArgument) {
      one["source"] = "argument";
      one["argument"] = (int64_t)scalar.argument;
    } else {
      one["source"] = "computed before the nest";
      one["location"] = scalar.location;
      one["note"] = "any value is considered: what computed it is not part of the nest";
    }
    inputScalars.push_back(std::move(one));
  }
  for (unsigned s = 0; s < r.separations.size(); ++s) {
    const NestSeparation &separation = r.separations[s];
    separations.push_back(json::Object{
        {"roots", json::Array{(int64_t)separation.first, (int64_t)separation.second}},
        {"apart", separation.apart},
        {"basis", separation.apart ? separation.basis : "none: the two may overlap"},
        {"kind", !separation.apart ? "unknown" : separation.proof ? "proof" : "assumption"},
        {"name", separation.name},
        {"used", s < r.separationUsed.size() && r.separationUsed[s]}});
  }
  out["outline"] = json::Object{{"status", r.outlineRefusal.empty() ? "OUTLINED" : "UNSUPPORTED"},
                                {"reason", r.outlineRefusal},
                                {"function", r.outlinedName},
                                {"roots", std::move(inputRoots)},
                                {"scalars", std::move(inputScalars)},
                                {"constants_copied", (int64_t)r.constantsCopied},
                                {"entry_memory", "any: what ran before the nest is not assumed"}};
  out["separation"] = std::move(separations);
  out["model"] = json::Object{{"status", !r.outlineRefusal.empty() ? "NOT_IMPORTED"
                                         : r.modelRefusal.empty()  ? "SUPPORTED"
                                                                   : "UNSUPPORTED"},
                              {"reason", r.modelRefusal},
                              {"sites", (int64_t)r.sites},
                              {"groups", (int64_t)r.groups},
                              {"candidates", (int64_t)r.candidates},
                              {"gate", r.gate},
                              {"gate_reason", r.gateReason},
                              {"termination", r.termination}};

  if (r.candidate) {
    json::Array levels;
    json::Object axes{{"P", json::Array()}, {"I", json::Array()}, {"J", json::Array()}, {"K", json::Array()}};
    for (unsigned l = 0; l < r.levels.size(); ++l) {
      const NestLevel &level = r.levels[l];
      json::Object one{{"level", (int64_t)l},
                       {"axis", axisName(level.axis)},
                       {"descending", level.descending},
                       {"signed", level.isSigned},
                       {"value", level.valueText},
                       {"lower", level.lower ? json::Value(*level.lower) : json::Value(level.lowerText)},
                       {"upper", level.upper ? json::Value(*level.upper) : json::Value(level.upperText)},
                       {"extent", level.extent ? json::Value((int64_t)*level.extent) : json::Value(nullptr)}};
      if (json::Array *axis = axes.getArray(axisName(level.axis)))
        axis->push_back((int64_t)l);
      levels.push_back(std::move(one));
    }
    static const char *const names[] = {"C", "A", "B"};
    json::Object views;
    for (unsigned view = 0; view < 3; ++view) {
      const NestView &one = r.views[view];
      json::Array dimensions;
      for (const NestViewDimension &dimension : one.dimensions) {
        json::Array byLevel, byParameter, byRole;
        for (const auto &[level, coefficient] : dimension.levels)
          byLevel.push_back(json::Object{{"level", (int64_t)level}, {"coefficient", coefficient}});
        for (const auto &[parameter, coefficient] : dimension.parameters)
          byParameter.push_back(json::Object{{"parameter", (int64_t)parameter}, {"coefficient", coefficient}});
        for (const auto &[axis, scale] : dimension.roles)
          byRole.push_back(json::Object{{"role", axisName(axis)}, {"scale", scale}});
        dimensions.push_back(json::Object{
            {"form", dimension.text},
            {"affine", dimension.affine},
            {"constant", dimension.affine ? json::Value(dimension.constant) : json::Value(nullptr)},
            {"levels", std::move(byLevel)},
            {"parameters", std::move(byParameter)},
            {"roles", std::move(byRole)},
            {"offset", dimension.offset ? json::Value(*dimension.offset) : json::Value(nullptr)}});
      }
      views[names[view]] = json::Object{{"root", rootJSON(one.root)},
                                        {"proposed", one.proposed},
                                        {"affine", one.proposed ? json::Value(one.affine) : json::Value(nullptr)},
                                        {"proved", one.proved},
                                        {"fact", one.fact},
                                        {"dimensions", std::move(dimensions)}};
    }
    json::Array reductionFlags, multiplyFlags;
    bool isFloat = r.reduction == "fadd";
    for (const std::string &name : flagNames(r.reductionFlags, isFloat))
      reductionFlags.push_back(name);
    for (const std::string &name : flagNames(r.multiplyFlags, isFloat))
      multiplyFlags.push_back(name);
    json::Object gemm{
        {"group", (int64_t)*r.group},
        {"candidate", (int64_t)*r.candidate},
        {"levels", std::move(levels)},
        {"axes", std::move(axes)},
        {"guard", r.guard},
        {"box", r.box},
        {"extents", r.extents ? json::Value(json::Object{{"M", (int64_t)(*r.extents)[0]},
                                                         {"N", (int64_t)(*r.extents)[1]},
                                                         {"K", (int64_t)(*r.extents)[2]}})
                              : json::Value(nullptr)},
        {"views", std::move(views)},
        {"accumulation",
         json::Object{{"statement", r.accumulatorFirst ? "C[h] := C[h] + A[f] * B[g]" : "C[h] := A[f] * B[g] + C[h]"},
                      {"starts_from", "the value of the cell at the nest's entry"},
                      {"order", "the levels, outermost first, each in its direction"},
                      {"reduction", r.reduction},
                      {"multiply", r.multiply},
                      {"element", r.element},
                      {"fold", r.fold},
                      {"accumulator_first", r.accumulatorFirst},
                      {"sites", (int64_t)r.sitesInGroup},
                      {"sites_agree", r.flagsAgree}}},
        {"reduction_flags", std::move(reductionFlags)},
        {"multiply_flags", std::move(multiplyFlags)},
        {"frame", json::Object{{"other_writes", (int64_t)r.otherWrites}, {"other_reads", (int64_t)r.otherReads}}},
        {"tiers", json::Object(r.tiers)},
        {"reorder", json::Object(r.reorder)}};
    out["gemm"] = std::move(gemm);
  } else {
    out["gemm"] = nullptr;
  }

  out["ordered_fold_verified"] = r.orderedFoldVerified;
  out["ordered_fold"] = json::Object{{"verified", r.orderedFoldVerified}, {"reason", r.orderedFoldReason}};
  out["reassociation_permitted"] = r.reassociationPermitted;
  out["reassociation"] = json::Object{
      {"permitted", r.reassociationPermitted},
      {"basis", r.reassociationBasis.empty() ? "none" : r.reassociationBasis},
      {"detail", r.reassociationDetail},
      {"source_flag_reassoc", r.sourceFlagReassoc ? json::Value(*r.sourceFlagReassoc) : json::Value(nullptr)}};
  json::Array assumptions, refusals;
  for (const std::string &text : r.assumptions)
    assumptions.push_back(text);
  for (const std::string &text : r.refusals)
    refusals.push_back(text);
  out["assumptions"] = std::move(assumptions);
  out["refusals"] = std::move(refusals);
  return out;
}

json::Object toJSON(const FunctionOutline &outline, const std::vector<NestRecord> &records) {
  json::Object out{{"function", outline.function},
                   {"schema", "generalized-gemm-nest/2"},
                   {"status", outline.unsupported.empty() ? "OUTLINED" : "UNSUPPORTED"},
                   {"reason", outline.unsupported},
                   {"authorizes_lowering", false},
                   {"composition",
                    "each record is a claim about its nest from any memory at the nest's entry, so the "
                    "statements keep their order and no nest is taken to be independent of another"}};
  json::Array roots, order, dependences, nests;
  for (const FunctionRoot &root : outline.roots) {
    json::Object one{{"root", (int64_t)root.id}, {"type", root.type}};
    if (root.origin == FunctionRoot::FunctionArgument) {
      one["origin"] = "argument";
      one["argument"] = (int64_t)root.argument;
      one["noalias"] = root.restrictArgument;
    } else {
      one["origin"] = "allocation";
      one["location"] = root.location;
      one["inside_loop"] = root.insideLoop;
    }
    roots.push_back(std::move(one));
  }
  for (const OrderEntry &entry : outline.order) {
    json::Object one{{"statement", (int64_t)entry.statement},
                     {"conditions", (int64_t)entry.conditions},
                     {"operation", entry.operation},
                     {"location", entry.location}};
    switch (entry.kind) {
    case OrderEntry::Nest:
      one["kind"] = "nest";
      one["nest"] = (int64_t)entry.nest;
      break;
    case OrderEntry::Access:
      one["kind"] = "access";
      one["root"] = (int64_t)entry.root;
      one["access"] = accessJSON(entry.access);
      break;
    case OrderEntry::Allocation:
      one["kind"] = "allocation";
      one["root"] = (int64_t)entry.root;
      break;
    case OrderEntry::Opaque:
      one["kind"] = "unknown effects";
      break;
    }
    order.push_back(std::move(one));
  }
  for (const NestDependence &dependence : outline.dependences) {
    json::Object one{{"earlier", (int64_t)dependence.earlier}, {"later", (int64_t)dependence.later}};
    switch (dependence.relation) {
    case NestDependence::SameRoot:
      one["relation"] = "same root";
      break;
    case NestDependence::MayAlias:
      one["relation"] = "roots not known apart";
      break;
    case NestDependence::UnknownEffects:
      one["relation"] = "unknown effects";
      break;
    }
    if (dependence.relation != NestDependence::UnknownEffects) {
      one["earlier_root"] = (int64_t)dependence.earlierRoot;
      one["later_root"] = (int64_t)dependence.laterRoot;
      one["earlier_access"] = accessJSON(dependence.earlierAccess);
      one["later_access"] = accessJSON(dependence.laterAccess);
    }
    dependences.push_back(std::move(one));
  }
  for (const NestRecord &record : records)
    nests.push_back(toJSON(record, outline.roots));
  out["roots"] = std::move(roots);
  out["order"] = std::move(order);
  out["dependences"] = std::move(dependences);
  out["dependences_note"] = "by root, so what may be and not what is: never a reason to call two nests independent";
  out["nests"] = std::move(nests);
  return out;
}

} // namespace mlir::generalized_gemm
