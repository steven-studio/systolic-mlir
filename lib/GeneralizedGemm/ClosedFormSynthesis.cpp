#include "GeneralizedGemm/ClosedFormSynthesis.h"

#include <cstdlib>
#include <cstdio>

#include "GeneralizedGemm/FiniteDomain.h"
#include "GeneralizedGemm/IndexProgram.h"
#include "GeneralizedGemm/RelationalEncoding.h"
#include "GeneralizedGemm/TerminationObligations.h"

#include "llvm/ADT/DenseSet.h"
#include "llvm/Support/JSON.h"

#include <algorithm>
#include <functional>
#include <map>
#include <set>

using namespace llvm;

namespace mlir::generalized_gemm {

namespace {

struct Sample {
  std::vector<APInt> leafValues; // Parallel to Synthesizer::leaves.
  std::vector<APInt> targets;    // Parallel to spec.carried.
};

using Entry = GrammarTerm; // Values per sample.

/// Goal-directed enumeration of one variable's closed form over the samples.
class Synthesizer {
public:
  Synthesizer(const SemanticProgram &program, const LoopSpecification &spec,
              const std::vector<Sample> &samples, const ClosedFormOptions &options,
              std::vector<Term> leaves)
      : program(program), spec(spec), samples(samples), options(options), leaves(std::move(leaves)),
        terms(*program.terms) {}

  std::optional<Term> find(unsigned k, std::string &why);

private:
  void enumerate(unsigned width);
  std::optional<Term> branch(unsigned k, std::string &why);

  const SemanticProgram &program;
  const LoopSpecification &spec;
  const std::vector<Sample> &samples;
  const ClosedFormOptions &options;
  std::vector<Term> leaves;
  TermContext &terms;
  std::vector<Entry> pool;
  std::map<std::vector<uint64_t>, unsigned> seen;
  std::vector<unsigned> constants; // Indices into pool.
  bool enumerated = false;
};

void Synthesizer::enumerate(unsigned width) {
  if (enumerated)
    return;
  enumerated = true;
  std::vector<std::vector<APInt>> leafValues(leaves.size());
  for (unsigned l = 0; l < leaves.size(); ++l)
    for (const Sample &s : samples)
      leafValues[l].push_back(s.leafValues[l]);
  TermGrammar grammar;
  grammar.maxTermSize = options.maxTermSize;
  grammar.maxTerms = options.maxTerms;
  grammar.extraConstants = options.extraConstants;
  GrammarEnumeration enumeration =
      enumerateGrammar(program, leaves, leafValues, samples.size(), width, grammar);
  pool = std::move(enumeration.pool);
  seen = std::move(enumeration.seen);
  constants = std::move(enumeration.constants);
}

std::optional<Term> Synthesizer::find(unsigned k, std::string &why) {
  unsigned width = program.state[spec.carried[k]].sort.width;
  if (width != spec.iteration->sort.width) {
    why = "the variable's width differs from the iteration count's";
    return std::nullopt;
  }
  if (samples.empty()) {
    why = "no sample";
    return std::nullopt;
  }
  enumerate(width);
  std::vector<uint64_t> target;
  for (const Sample &s : samples)
    target.push_back(s.targets[k].getZExtValue());
  if (auto found = seen.find(target); found != seen.end())
    return pool[found->second].term;
  return branch(k, why);
}

/// One branch: a condition over the pool and the constants, a then-term agreeing
/// with the target where it holds and an else-term where it does not.
std::optional<Term> Synthesizer::branch(unsigned k, std::string &why) {
  size_t n = samples.size(), words = (n + 63) / 64;
  using Mask = std::vector<uint64_t>;
  auto bit = [&](Mask &mask, size_t s) { mask[s / 64] |= uint64_t(1) << (s % 64); };
  // Agreement of each pool term with the target, distinct masks only, largest first.
  std::map<Mask, unsigned> agreement;
  for (unsigned e = 0; e < pool.size(); ++e) {
    Mask mask(words, 0);
    bool any = false;
    for (size_t s = 0; s < n; ++s)
      if (pool[e].values[s] == samples[s].targets[k]) {
        bit(mask, s);
        any = true;
      }
    // A branch's arm agrees on every sample of its side; an arm right at one
    // sample only is noise, and the side it would serve is one sample wide.
    unsigned count = 0;
    for (uint64_t w : mask)
      count += __builtin_popcountll(w);
    if (!any || count < 2)
      continue;
    auto [slot, fresh] = agreement.try_emplace(mask, e);
    if (!fresh && pool[e].size < pool[slot->second].size)
      slot->second = e;
  }
  std::vector<std::pair<Mask, unsigned>> masks(agreement.begin(), agreement.end());
  auto popcount = [](const Mask &m) {
    unsigned c = 0;
    for (uint64_t w : m)
      c += __builtin_popcountll(w);
    return c;
  };
  std::stable_sort(masks.begin(), masks.end(), [&](const auto &a, const auto &b) {
    return popcount(a.first) > popcount(b.first);
  });
  auto covers = [&](const Mask &a, const Mask &m) {
    for (size_t w = 0; w < words; ++w)
      if ((a[w] & m[w]) != m[w])
        return false;
    return true;
  };
  auto cover = [&](const Mask &m) -> std::optional<unsigned> {
    unsigned need = popcount(m);
    for (const auto &[a, e] : masks) {
      if (popcount(a) < need)
        break;
      if (covers(a, m))
        return e;
    }
    return std::nullopt;
  };
  Mask all(words, 0);
  for (size_t s = 0; s < n; ++s)
    bit(all, s);
  // Conditions: a term of size at most 3 against a constant or a leaf.
  std::vector<unsigned> lhs, rhs;
  for (unsigned e = 0; e < pool.size(); ++e) {
    if (pool[e].size <= 3)
      lhs.push_back(e);
    if (pool[e].size == 1)
      rhs.push_back(e);
  }
  std::set<Mask> tried;
  size_t tests = 0;
  std::optional<Term> best;
  unsigned bestSize = UINT32_MAX;
  for (unsigned a : lhs)
    for (unsigned b : rhs) {
      if (a == b)
        continue;
      for (Predicate predicate : {Predicate::Eq, Predicate::Ult, Predicate::Ule}) {
        if (tests++ >= options.maxTests) {
          if (!best)
            why = "the condition limit";
          return best;
        }
        Mask mask(words, 0), complement(words, 0);
        bool anyTrue = false, anyFalse = false;
        for (size_t s = 0; s < n; ++s) {
          const APInt &x = pool[a].values[s], &y = pool[b].values[s];
          bool holds = predicate == Predicate::Eq ? x == y
                       : predicate == Predicate::Ult ? x.ult(y) : x.ule(y);
          if (holds) {
            bit(mask, s);
            anyTrue = true;
          } else {
            bit(complement, s);
            anyFalse = true;
          }
        }
        if (!anyTrue || !anyFalse || tried.count(complement) || !tried.insert(mask).second)
          continue;
        auto then = cover(mask), otherwise = cover(complement);
        if (!then || !otherwise)
          continue;
        unsigned size = pool[a].size + pool[b].size + 1 + pool[*then].size + pool[*otherwise].size;
        if (size >= bestSize)
          continue;
        bestSize = size;
        best = terms.ite(terms.icmp(predicate, pool[a].term, pool[b].term), pool[*then].term,
                         pool[*otherwise].term);
      }
    }
  if (!best)
    why = "no term or branch of the grammar fits the samples within the limits";
  return best;
}

/// A term of the closed-form grammar as an expression of `p`. Leaves become inputs,
/// registered on first use, unless `leaf` maps them to an expression already in `p`.
unsigned termExpr(IndexProgram &p, Term t, std::map<Term, unsigned> &inputs,
                  const std::function<std::optional<unsigned>(Term)> &leaf) {
  switch (t->op) {
  case TermOp::Const: return p.constant(t->value);
  case TermOp::Add: return p.add(termExpr(p, t->args[0], inputs, leaf), termExpr(p, t->args[1], inputs, leaf));
  case TermOp::Sub: return p.sub(termExpr(p, t->args[0], inputs, leaf), termExpr(p, t->args[1], inputs, leaf));
  case TermOp::Mul: return p.mul(termExpr(p, t->args[0], inputs, leaf), termExpr(p, t->args[1], inputs, leaf));
  case TermOp::UDiv:
    if (t->args[1]->op == TermOp::Const)
      return p.divConst(termExpr(p, t->args[0], inputs, leaf), t->args[1]->value);
    break;
  case TermOp::URem:
    if (t->args[1]->op == TermOp::Const)
      return p.modConst(termExpr(p, t->args[0], inputs, leaf), t->args[1]->value);
    break;
  default:
    break;
  }
  if (auto mapped = leaf(t))
    return *mapped;
  auto [slot, fresh] = inputs.try_emplace(t, 0);
  if (fresh)
    slot->second = p.input(t);
  return p.inputExpr(slot->second);
}

unsigned termCond(IndexProgram &p, Term t, std::map<Term, unsigned> &inputs,
                  const std::function<std::optional<unsigned>(Term)> &leaf) {
  unsigned a = termExpr(p, t->args[0], inputs, leaf), b = termExpr(p, t->args[1], inputs, leaf);
  switch ((Predicate)t->aux) {
  case Predicate::Eq: return p.eq(a, b);
  case Predicate::Ult: return p.ult(a, b);
  default: return p.ule(a, b);
  }
}

/// The candidate as a statement program, for the record: one branch at most.
std::string programText(const SemanticProgram &program, const LoopSpecification &spec, Term term) {
  IndexProgram p;
  std::map<Term, unsigned> inputs;
  auto none = [](Term) -> std::optional<unsigned> { return std::nullopt; };
  try {
    unsigned v = p.local(term->sort.width);
    if (term->op == TermOp::Ite)
      p.body = p.ifThenElse(termCond(p, term->args[0], inputs, none),
                            p.assign(v, termExpr(p, term->args[1], inputs, none)),
                            p.assign(v, termExpr(p, term->args[2], inputs, none)));
    else
      p.body = p.assign(v, termExpr(p, term, inputs, none));
    p.result = p.localExpr(v);
  } catch (const TermError &error) {
    return std::string("(not a statement program: ") + error.what() + ")";
  }
  return formatIndexProgram(p, [&](Term leaf) { return closedFormLeafName(program, spec, leaf); });
}

/// Phase 3: while programs of one shape for the carried variable k,
///   r := 0 ; v := init ; while r < m do (v := E ; r := r + 1) ; return v,
/// Appendix G.1's P_K is one. The body E is the step relation of the seeds: from
/// every sampled iteration m to m + 1, v(m) and r = m go in and v(m + 1) comes out,
/// a loop-free synthesis problem that the goal-directed enumerator solves, one
/// branch allowed. Every other input (the entries, the fixed variables, the
/// parameters) and the proved loop-free forms of the other carried variables at r
/// may appear in E. The program is then run on the seeds, so that the body found
/// from the steps is judged as a program, by the same finite-tier check the
/// caller uses on every trace.
class WhileSynthesizer {
public:
  WhileSynthesizer(const SemanticProgram &program, const LoopSpecification &spec, unsigned k,
                   ArrayRef<Term> leaves, const std::map<unsigned, Term> &provedForms,
                   const ClosedFormOptions &options)
      : program(program), spec(spec), k(k), leaves(leaves), provedForms(provedForms),
        options(options) {}

  /// Nullopt with `why` when nothing of the shape fits the seeds.
  std::optional<IndexProgram> find(ArrayRef<LoopTrace> seeds, std::string &why, uint64_t &fuelCuts);

private:
  std::optional<IndexProgram> build(Term body, Term current) const;

  const SemanticProgram &program;
  const LoopSpecification &spec;
  unsigned k;
  ArrayRef<Term> leaves;
  const std::map<unsigned, Term> &provedForms;
  const ClosedFormOptions &options;
};

std::optional<IndexProgram> WhileSynthesizer::build(Term body, Term current) const {
  try {
    IndexProgram p;
    std::map<Term, unsigned> inputs;
    for (Term leaf : leaves)
      inputs[leaf] = p.input(leaf);
    unsigned width = spec.iteration->sort.width;
    unsigned r = p.local(width), v = p.local(program.state[spec.carried[k]].sort.width);
    auto none = [](Term) -> std::optional<unsigned> { return std::nullopt; };
    unsigned init = spec.init[k] ? termExpr(p, spec.init[k], inputs, none)
                                 : p.inputExpr(inputs.at(spec.entry[k]));
    unsigned start = p.seq(p.assign(r, p.constant(APInt::getZero(width))), p.assign(v, init));
    // In the body, the current value is the local v and the iteration count the
    // local r; the proved forms of other carried variables are read at r.
    std::function<std::optional<unsigned>(Term)> atStep = [&](Term leaf) -> std::optional<unsigned> {
      if (leaf == current)
        return p.localExpr(v);
      if (leaf == spec.iteration)
        return p.localExpr(r);
      if (leaf->op == TermOp::State)
        for (unsigned u = 0; u < spec.carried.size(); ++u)
          if (u != k && spec.carried[u] == leaf->index && provedForms.count(leaf->index))
            return termExpr(p, provedForms.at(leaf->index), inputs, atStep);
      return std::nullopt;
    };
    unsigned next = body->op == TermOp::Ite
                        ? 0
                        : termExpr(p, body, inputs, atStep);
    unsigned update;
    if (body->op == TermOp::Ite)
      update = p.ifThenElse(termCond(p, body->args[0], inputs, atStep),
                            p.assign(v, termExpr(p, body->args[1], inputs, atStep)),
                            p.assign(v, termExpr(p, body->args[2], inputs, atStep)));
    else
      update = p.assign(v, next);
    unsigned step = p.seq(update, p.assign(r, p.add(p.localExpr(r), p.constant(APInt(width, 1)))));
    p.body = p.seq(start, p.whileDo(p.ult(p.localExpr(r), p.inputExpr(inputs.at(spec.iteration))), step));
    p.result = p.localExpr(v);
    return p;
  } catch (const TermError &) {
    return std::nullopt;
  }
}

std::optional<IndexProgram> WhileSynthesizer::find(ArrayRef<LoopTrace> seeds, std::string &why,
                                                   uint64_t &fuelCuts) {
  TermContext &terms = *program.terms;
  unsigned variable = spec.carried[k];
  unsigned width = program.state[variable].sort.width;
  if (width != spec.iteration->sort.width) {
    why = "the variable's width differs from the iteration count's";
    return std::nullopt;
  }
  // The step samples: leaves are the current value, then every input with m read
  // as the counter r, then the proved forms of the other carried variables at r.
  Term current = terms.state(variable, program.state[variable].sort);
  std::vector<Term> stepLeaves{current};
  for (Term leaf : leaves)
    stepLeaves.push_back(leaf);
  std::vector<Term> formLeaves;
  for (unsigned u = 0; u < spec.carried.size(); ++u)
    if (u != k && provedForms.count(spec.carried[u]) &&
        program.state[spec.carried[u]].sort.width == width) {
      Term other = terms.state(spec.carried[u], program.state[spec.carried[u]].sort);
      stepLeaves.push_back(other);
      formLeaves.push_back(other);
    }
  std::vector<Sample> steps;
  for (const LoopTrace &trace : seeds)
    for (size_t m = 0; m + 1 < trace.states.size(); ++m) {
      Sample sample;
      sample.leafValues.push_back(trace.states[m][k]);
      for (Term leaf : leaves) {
        if (leaf == spec.iteration) {
          sample.leafValues.push_back(APInt(width, m, false, true));
          continue;
        }
        bool found = false;
        for (unsigned e = 0; e < spec.entry.size() && !found; ++e)
          if (leaf == spec.entry[e]) {
            sample.leafValues.push_back(trace.entry[e]);
            found = true;
          }
        for (unsigned f = 0; f < spec.fixed.size() && !found; ++f)
          if (leaf->op == TermOp::State && leaf->index == spec.fixed[f]) {
            sample.leafValues.push_back(trace.fixedValues[f]);
            found = true;
          }
        if (!found && leaf->op == TermOp::Param) {
          sample.leafValues.push_back(trace.parameters[leaf->index]);
          found = true;
        }
        if (!found)
          sample.leafValues.push_back(APInt::getZero(leaf->sort.width));
      }
      for (unsigned u = 0; u < spec.carried.size(); ++u)
        if (u != k && provedForms.count(spec.carried[u]) &&
            program.state[spec.carried[u]].sort.width == width)
          sample.leafValues.push_back(trace.states[m][u]);
      sample.targets.assign(spec.carried.size(), APInt::getZero(width));
      sample.targets[k] = trace.states[m + 1][k];
      steps.push_back(std::move(sample));
    }
  if (steps.empty()) {
    why = "no sampled step";
    return std::nullopt;
  }
  Synthesizer synthesizer(program, spec, steps, options, stepLeaves);
  std::string inner;
  auto body = synthesizer.find(k, inner);
  if (!body) {
    why = "no step expression or branch of the grammar fits the seed steps: " + inner;
    return std::nullopt;
  }
  auto candidate = build(*body, current);
  if (!candidate) {
    why = "the step does not build as a program";
    return std::nullopt;
  }
  CheckedClosedForm onSeeds = checkProgramOnTraces(program, spec, k, *candidate, seeds, options.programFuel);
  if (onSeeds.status == ClosedFormStatus::SampleConsistent)
    return candidate;
  if (onSeeds.status == ClosedFormStatus::Unknown && onSeeds.reason.find("fuel") != std::string::npos)
    ++fuelCuts;
  why = "the program built from the step disagrees with a seed: " + onSeeds.reason;
  return std::nullopt;
}

/// The specification's own run from a solver model: the model names m, entries,
/// fixed variables and parameters; what it leaves free keeps the reference trace's
/// values. Nullopt when the parameters leave the scope or a term has no value.
std::optional<LoopTrace> replayModel(const SemanticProgram &program, const LoopSpecification &spec,
                                     const std::vector<std::pair<std::string, APInt>> &model,
                                     const LoopTrace *reference, const ClosedFormOptions &options,
                                     uint64_t &modelIteration) {
  std::vector<APInt> parameters, entryValues, fixedValues;
  if (reference) {
    parameters = reference->parameters;
    entryValues = reference->entry;
    fixedValues = reference->fixedValues;
  }
  if (parameters.size() != program.parameters.size()) {
    parameters.clear();
    for (const Parameter &p : program.parameters)
      parameters.push_back(APInt::getZero(p.sort.width));
  }
  if (entryValues.size() != spec.carried.size()) {
    entryValues.clear();
    for (unsigned v : spec.carried)
      entryValues.push_back(APInt::getZero(program.state[v].sort.width));
  }
  if (fixedValues.size() != spec.fixed.size()) {
    fixedValues.clear();
    for (unsigned v : spec.fixed)
      fixedValues.push_back(APInt::getZero(program.state[v].sort.width));
  }
  modelIteration = 0;
  for (const auto &[name, value] : model) {
    if (name == "m")
      modelIteration = value.getZExtValue();
    else if (name.rfind("entry_s", 0) == 0) {
      // An entry value from the model only where the entry is symbolic: with a known
      // init the entry leaf is an unconstrained auxiliary of the script, and taking
      // its value would start the specification from a state no run has.
      unsigned v = std::stoul(name.substr(7));
      auto at = std::find(spec.carried.begin(), spec.carried.end(), v);
      if (at != spec.carried.end() && !spec.init[at - spec.carried.begin()])
        entryValues[at - spec.carried.begin()] = value;
    } else if (name[0] == 's') {
      unsigned v = std::stoul(name.substr(1));
      auto at = std::find(spec.fixed.begin(), spec.fixed.end(), v);
      if (at != spec.fixed.end())
        fixedValues[at - spec.fixed.begin()] = value;
    } else if (name[0] == 'p') {
      unsigned id = std::stoul(name.substr(1));
      if (id < parameters.size() && parameters[id].getBitWidth() == value.getBitWidth())
        parameters[id] = value;
    }
  }
  if (!parametersInScope(options.scope, parameters))
    return std::nullopt;
  return runSpecification(program, spec, parameters, entryValues, fixedValues,
                          std::min<uint64_t>(modelIteration + 1, 4096));
}

/// The value of a loop-free form at iteration m of a trace; nullopt when a leaf is
/// not valued there.
std::optional<APInt> formValueAt(const SemanticProgram &program, const LoopSpecification &spec, Term form,
                                 const LoopTrace &trace, size_t m) {
  EntryState entry;
  entry.parameters = trace.parameters;
  Configuration config;
  config.state.assign(program.state.size() + 1 + spec.carried.size(), std::nullopt);
  config.shadow.assign(config.state.size(), false);
  config.state[spec.iteration->index] =
      APInt(spec.iteration->sort.width, m, /*isSigned=*/false, /*implicitTrunc=*/true);
  for (unsigned e = 0; e < spec.entry.size() && e < trace.entry.size(); ++e)
    config.state[spec.entry[e]->index] = trace.entry[e];
  for (unsigned f = 0; f < spec.fixed.size() && f < trace.fixedValues.size(); ++f)
    config.state[spec.fixed[f]] = trace.fixedValues[f];
  return evaluateTerm(program, entry, config, form);
}

/// The first iteration of the trace where the form differs from the carried variable
/// k, or nullopt when it agrees everywhere it has a value.
std::optional<size_t> formDisagreement(const SemanticProgram &program, const LoopSpecification &spec,
                                       Term form, unsigned k, const LoopTrace &trace) {
  for (size_t m = 0; m < trace.states.size(); ++m) {
    auto value = formValueAt(program, spec, form, trace, m);
    if (value && *value != trace.states[m][k])
      return m;
  }
  return std::nullopt;
}

/// Phase 5: the history table template (ClosedFormOptions). The inits are read off
/// the seeds (a constant at every seed, or the symbolic entry value at j = 0); the
/// history chain is the carried variables that trail the variable by 1, 2, ...
/// iterations on the seeds (a proposal; TT-shift checks it); the body E is a
/// loop-free synthesis problem over r, the reads T[r-1..r-d], the inputs and the
/// constants, from the sampled steps m >= d. Depth d from 1 to maxHistory, the
/// smallest that fits first.
class TableSynthesizer {
public:
  struct Candidate {
    IndexProgram program;
    std::vector<unsigned> history;
    unsigned depth = 0;
  };
  TableSynthesizer(const SemanticProgram &program, const LoopSpecification &spec, unsigned k,
                   ArrayRef<Term> leaves, const ClosedFormOptions &options)
      : program(program), spec(spec), k(k), leaves(leaves), options(options) {}

  std::optional<Candidate> find(ArrayRef<LoopTrace> seeds, std::string &why, uint64_t &fuelCuts);

private:
  const SemanticProgram &program;
  const LoopSpecification &spec;
  unsigned k;
  ArrayRef<Term> leaves;
  const ClosedFormOptions &options;
};

std::optional<TableSynthesizer::Candidate> TableSynthesizer::find(ArrayRef<LoopTrace> seeds, std::string &why,
                                                                  uint64_t &fuelCuts) {
  TermContext &terms = *program.terms;
  unsigned variable = spec.carried[k];
  unsigned width = program.state[variable].sort.width;
  if (width != spec.iteration->sort.width) {
    why = "the variable's width differs from the iteration count's";
    return std::nullopt;
  }
  std::string last = "no depth fits";
  for (unsigned d = 1; d <= options.maxHistory; ++d) {
    // Inits: T[j] = v(j) at every seed that reaches j.
    std::vector<std::optional<APInt>> inits(d);
    std::vector<bool> entryInit(d, false);
    bool initsOk = true;
    for (unsigned j = 0; j < d && initsOk; ++j) {
      bool allEntry = j == 0 && !spec.init[k];
      for (const LoopTrace &trace : seeds) {
        if (j >= trace.states.size())
          continue;
        const APInt &value = trace.states[j][k];
        if (!inits[j])
          inits[j] = value;
        else if (*inits[j] != value)
          initsOk = false;
        allEntry = allEntry && value == trace.entry[k];
      }
      if (!initsOk && allEntry) {
        initsOk = true;
        entryInit[j] = true;
      }
      if (!inits[j])
        initsOk = false; // No seed reaches j: nothing to read the init off.
    }
    if (!initsOk) {
      last = "the first " + std::to_string(d) + " values are not one constant (or the entry) on the seeds";
      continue;
    }
    // The chain on the seeds: v_j(m) = v(m - j).
    std::vector<unsigned> chain;
    for (unsigned j = 1; j < d; ++j) {
      std::optional<unsigned> pick;
      for (unsigned u = 0; u < spec.carried.size() && !pick; ++u) {
        if (u == k || llvm::is_contained(chain, spec.carried[u]) ||
            program.state[spec.carried[u]].sort.width != width)
          continue;
        bool trails = true;
        for (const LoopTrace &trace : seeds)
          for (size_t m = j; m < trace.states.size() && trails; ++m)
            trails = trace.states[m][u] == trace.states[m - j][k];
        if (trails)
          pick = spec.carried[u];
      }
      if (!pick)
        break; // A partial chain is still a proposal; the checker may not prove the step.
      chain.push_back(*pick);
    }
    // Samples for E: r = m, T[m-1..m-d], the other inputs -> v(m), for m >= d.
    Term rLeaf = spec.iteration;
    std::vector<Term> historyLeaves;
    unsigned base = program.state.size() + 2 + spec.carried.size();
    for (unsigned j = 1; j <= d; ++j)
      historyLeaves.push_back(terms.state(base + j, Sort::bv(width)));
    std::vector<Term> stepLeaves{rLeaf};
    for (Term h : historyLeaves)
      stepLeaves.push_back(h);
    for (Term leaf : leaves)
      if (leaf != spec.iteration)
        stepLeaves.push_back(leaf);
    std::vector<Sample> steps;
    for (const LoopTrace &trace : seeds)
      for (size_t m = d; m < trace.states.size(); ++m) {
        Sample sample;
        sample.leafValues.push_back(APInt(width, m, false, true));
        for (unsigned j = 1; j <= d; ++j)
          sample.leafValues.push_back(trace.states[m - j][k]);
        for (Term leaf : leaves) {
          if (leaf == spec.iteration)
            continue;
          bool found = false;
          for (unsigned e = 0; e < spec.entry.size() && !found; ++e)
            if (leaf == spec.entry[e]) {
              sample.leafValues.push_back(trace.entry[e]);
              found = true;
            }
          for (unsigned f = 0; f < spec.fixed.size() && !found; ++f)
            if (leaf->op == TermOp::State && leaf->index == spec.fixed[f]) {
              sample.leafValues.push_back(trace.fixedValues[f]);
              found = true;
            }
          if (!found && leaf->op == TermOp::Param) {
            sample.leafValues.push_back(trace.parameters[leaf->index]);
            found = true;
          }
          if (!found)
            sample.leafValues.push_back(APInt::getZero(leaf->sort.width));
        }
        sample.targets.assign(spec.carried.size(), APInt::getZero(width));
        sample.targets[k] = trace.states[m][k];
        steps.push_back(std::move(sample));
      }
    if (steps.empty()) {
      last = "no sampled step beyond depth " + std::to_string(d);
      continue;
    }
    Synthesizer synthesizer(program, spec, steps, options, stepLeaves);
    std::string inner;
    auto body = synthesizer.find(k, inner);
    if (!body) {
      last = "no body of the grammar fits the steps at depth " + std::to_string(d) + ": " + inner;
      continue;
    }
    if ((*body)->op == TermOp::Ite) {
      last = "a branching body is outside the table template";
      continue;
    }
    // Build the template program.
    try {
      IndexProgram p;
      std::map<Term, unsigned> inputs;
      for (Term leaf : leaves)
        inputs[leaf] = p.input(leaf);
      unsigned m = inputs.at(spec.iteration);
      unsigned r = p.local(width);
      auto constant = [&](uint64_t v) { return p.constant(APInt(width, v)); };
      unsigned T = p.array(width, width, {p.add(p.inputExpr(m), constant(d))});
      std::optional<unsigned> body0;
      for (unsigned j = 0; j < d; ++j) {
        unsigned value = entryInit[j] ? p.inputExpr(inputs.at(spec.entry[k])) : p.constant(*inits[j]);
        unsigned write = p.write(T, {constant(j)}, value);
        body0 = body0 ? p.seq(*body0, write) : write;
      }
      unsigned start = p.seq(*body0, p.assign(r, constant(d)));
      std::function<std::optional<unsigned>(Term)> atStep = [&](Term leaf) -> std::optional<unsigned> {
        if (leaf == rLeaf)
          return p.localExpr(r);
        for (unsigned j = 1; j <= d; ++j)
          if (leaf == historyLeaves[j - 1])
            return p.read(T, {p.sub(p.localExpr(r), constant(j))});
        return std::nullopt;
      };
      unsigned E = termExpr(p, *body, inputs, atStep);
      unsigned step = p.seq(p.write(T, {p.localExpr(r)}, E), p.assign(r, p.add(p.localExpr(r), constant(1))));
      p.body = p.seq(start, p.whileDo(p.ult(p.localExpr(r), p.add(p.inputExpr(m), constant(1))), step));
      p.result = p.read(T, {p.inputExpr(m)});
      std::string shape;
      if (!matchTableTemplate(p, shape)) {
        last = "the built program is not the template: " + shape;
        continue;
      }
      CheckedClosedForm onSeeds = checkProgramOnTraces(program, spec, k, p, seeds, options.programFuel);
      if (onSeeds.status == ClosedFormStatus::SampleConsistent) {
        Candidate out;
        out.program = std::move(p);
        out.history = chain;
        out.depth = d;
        return out;
      }
      if (onSeeds.status == ClosedFormStatus::Unknown && onSeeds.reason.find("fuel") != std::string::npos)
        ++fuelCuts;
      last = "the program built from the steps disagrees with a seed: " + onSeeds.reason;
    } catch (const TermError &error) {
      last = std::string("the body does not build as a program: ") + error.what();
    }
  }
  why = last;
  return std::nullopt;
}

/// What a program finder returns: the candidate and, for a table, its history chain.
struct FoundProgram {
  IndexProgram program;
  std::vector<unsigned> history;
};
using ProgramFinder = std::function<std::optional<FoundProgram>(ArrayRef<LoopTrace> seeds, std::string &why,
                                                                uint64_t &fuelCuts)>;

/// CEGIS at the finite tier: seeds are the traces of the first point; a candidate
/// that fits them is judged on every trace, a failing trace joins the seeds.
/// Phase 4: a candidate equal to every sampled iteration is then put to the
/// symbolic obligations (W for the counter template, TT for the table template);
/// PROVED needs all of them unsat.
void synthesizeProgramForm(const SemanticProgram &program, const LoopSpecification &spec, unsigned k,
                           const std::map<unsigned, Term> &provedForms, std::vector<LoopTrace> &traces,
                           const ClosedFormOptions &options, SmtBackend &backend, const char *kind,
                           unsigned rounds, bool prove, const ProgramFinder &finder,
                           SynthesizedClosedForm &form) {
  form.kind = kind;
  form.term = nullptr;
  form.text.clear();
  form.program.reset();
  form.rounds = 0;
  form.counterexampleTraces = 0;
  form.symbolicRounds = 0;
  form.records.clear();
  form.model.clear();
  form.history.clear();
  if (traces.empty()) {
    form.status = ClosedFormStatus::Unknown;
    form.reason = "no trace";
    return;
  }
  std::vector<LoopTrace> seeds;
  for (const LoopTrace &trace : traces)
    if (trace.parameters == traces.front().parameters)
      seeds.push_back(trace);
  auto addSeed = [&](const LoopTrace &trace) {
    for (const LoopTrace &seed : seeds)
      if (seed.parameters == trace.parameters && seed.entry == trace.entry && seed.states == trace.states)
        return false;
    seeds.push_back(trace);
    ++form.counterexampleTraces;
    return true;
  };
  for (unsigned round = 0; round < rounds; ++round) {
    std::string why;
    auto candidate = finder(seeds, why, form.fuelCuts);
    if (!candidate) {
      // Nothing of the shape fits the seeds any more: no candidate stands, and a
      // refuted earlier one is not kept as if it did.
      form.status = ClosedFormStatus::Unknown;
      form.reason = (form.rounds ? "after " + std::to_string(form.rounds) + " refuted candidate(s), " : "") + why;
      form.program.reset();
      form.text.clear();
      form.history.clear();
      form.model.clear();
      return;
    }
    ++form.rounds;
    form.program = candidate->program;
    form.history = candidate->history;
    form.text = formatIndexProgram(*form.program, [&](Term leaf) { return closedFormLeafName(program, spec, leaf); });
    CheckedClosedForm checked = checkProgramOnTraces(program, spec, k, *form.program, traces, options.programFuel);
    if (getenv("GG_DEBUG_FORMS")) {
      std::string model;
      for (const auto &[name, value] : checked.model)
        model += " " + name + "=" + std::to_string(value.getZExtValue());
      fprintf(stderr, "[%s s%u round %u seeds %zu traces %zu] %s -> %s: %s |%s\n", kind, spec.carried[k], form.rounds,
              seeds.size(), traces.size(), form.text.c_str(), stringifyClosedFormStatus(checked.status),
              checked.reason.c_str(), model.c_str());
    }
    form.status = checked.status;
    form.reason = checked.reason;
    form.model = checked.model;
    form.records.clear();
    if (checked.status == ClosedFormStatus::Refuted) {
      // The failing trace, by its parameters and entry, joins the seeds.
      std::vector<APInt> parameters(traces.front().parameters.size(), APInt(1, 0)), entry;
      for (const auto &[name, value] : checked.model) {
        if (name[0] == 'p')
          parameters[std::stoul(name.substr(1))] = value;
        if (name.rfind("entry_s", 0) == 0)
          entry.push_back(value);
      }
      bool added = false;
      for (const LoopTrace &trace : traces)
        if (trace.parameters == parameters && trace.entry == entry) {
          added = addSeed(trace);
          break;
        }
      if (!added)
        return; // The refutation stands as the result.
      continue;
    }
    if (checked.status != ClosedFormStatus::SampleConsistent || !prove)
      return;
    // The symbolic obligations, with the proved loop-free forms of the other
    // variables standing for them and this variable free.
    ++form.symbolicRounds;
    std::vector<ClosedForm> list;
    for (const auto &[variable, term] : provedForms)
      if (variable != spec.carried[k])
        list.push_back({variable, term, nullptr, {}});
    list.push_back({spec.carried[k], nullptr, &*form.program, form.history});
    auto symbolic = checkClosedForms(program, spec, list, options.scope, options.solver, backend);
    if (symbolic.empty())
      return;
    const CheckedClosedForm &c = symbolic.back();
    form.records = c.records;
    if (c.status == ClosedFormStatus::Proved) {
      form.status = ClosedFormStatus::Proved;
      form.reason = c.reason + "; equal to all sampled iterations as well";
      form.model.clear();
      return;
    }
    if (c.model.empty()) {
      // Shape refused, solver unknown, a dependence not proved: the finite tier's
      // word stands, with the reason it is no more than that.
      form.reason = checked.reason + "; symbolic: " + c.reason;
      return;
    }
    // A model, on init (genuine) or on the step with a free leaf (maybe unreachable):
    // the specification's own run from it decides.
    uint64_t at = 0;
    auto trace = replayModel(program, spec, c.model, &traces.front(), options, at);
    if (!trace) {
      form.reason = checked.reason + "; symbolic: " + c.reason + "; the model could not be replayed";
      return;
    }
    CheckedClosedForm onTrace = checkProgramOnTraces(program, spec, k, *form.program, *trace, options.programFuel);
    if (onTrace.status == ClosedFormStatus::Refuted) {
      form.status = ClosedFormStatus::Refuted;
      form.reason = "the specification's run from the solver's model refutes it: " + onTrace.reason;
      form.model = onTrace.model;
      traces.push_back(*trace);
      if (!addSeed(*trace))
        return;
      continue;
    }
    form.model = c.model;
    form.reason = checked.reason + "; symbolic: " + c.reason +
                  (onTrace.status == ClosedFormStatus::SampleConsistent
                       ? "; the model does not replay into a disagreement, so it is not a refutation either"
                       : "; the replay from the model was cut by the fuel");
    return;
  }
}

void synthesizeWhileForm(const SemanticProgram &program, const LoopSpecification &spec, unsigned k,
                         ArrayRef<Term> leaves, const std::map<unsigned, Term> &provedForms,
                         std::vector<LoopTrace> &traces, const ClosedFormOptions &options,
                         SmtBackend &backend, SynthesizedClosedForm &form) {
  WhileSynthesizer synthesizer(program, spec, k, leaves, provedForms, options);
  ProgramFinder finder = [&](ArrayRef<LoopTrace> seeds, std::string &why, uint64_t &fuelCuts) -> std::optional<FoundProgram> {
    auto found = synthesizer.find(seeds, why, fuelCuts);
    if (!found)
      return std::nullopt;
    return FoundProgram{*found, {}};
  };
  synthesizeProgramForm(program, spec, k, provedForms, traces, options, backend, "while", options.maxWhileRounds,
                        options.proveWhile, finder, form);
}

void synthesizeTableForm(const SemanticProgram &program, const LoopSpecification &spec, unsigned k,
                         ArrayRef<Term> leaves, const std::map<unsigned, Term> &provedForms,
                         std::vector<LoopTrace> &traces, const ClosedFormOptions &options,
                         SmtBackend &backend, SynthesizedClosedForm &form) {
  TableSynthesizer synthesizer(program, spec, k, leaves, options);
  ProgramFinder finder = [&](ArrayRef<LoopTrace> seeds, std::string &why, uint64_t &fuelCuts) -> std::optional<FoundProgram> {
    auto found = synthesizer.find(seeds, why, fuelCuts);
    if (!found)
      return std::nullopt;
    return FoundProgram{std::move(found->program), std::move(found->history)};
  };
  synthesizeProgramForm(program, spec, k, provedForms, traces, options, backend, "table", options.maxTableRounds,
                        options.proveWhile, finder, form);
}

void addSamples(const LoopSpecification &spec, const LoopTrace &trace, ArrayRef<Term> leaves,
                std::vector<Sample> &samples, size_t limit) {
  unsigned width = spec.iteration->sort.width;
  for (size_t m = 0; m < trace.states.size() && samples.size() < limit; ++m) {
    Sample sample;
    for (Term leaf : leaves) {
      if (leaf == spec.iteration) {
        sample.leafValues.push_back(APInt(width, m, /*isSigned=*/false, /*implicitTrunc=*/true));
        continue;
      }
      bool found = false;
      for (unsigned k = 0; k < spec.entry.size() && !found; ++k)
        if (leaf == spec.entry[k]) {
          sample.leafValues.push_back(trace.entry[k]);
          found = true;
        }
      for (unsigned k = 0; k < spec.fixed.size() && !found; ++k)
        if (leaf->op == TermOp::State && leaf->index == spec.fixed[k]) {
          sample.leafValues.push_back(trace.fixedValues[k]);
          found = true;
        }
      if (!found && leaf->op == TermOp::Param) {
        sample.leafValues.push_back(trace.parameters[leaf->index]);
        found = true;
      }
      if (!found)
        return;
    }
    sample.targets = trace.states[m];
    samples.push_back(std::move(sample));
  }
}

void synthesizeLoop(const SemanticProgram &program, const IndexSlice &slice,
                    ArrayRef<std::vector<APInt>> points, const ClosedFormOptions &options,
                    SmtBackend &backend, LoopSynthesis &out) {
  const LoopSpecification &spec = out.spec;
  TermContext &terms = *program.terms;
  // Replay every point: the specification against the program, and the samples.
  std::vector<LoopTrace> traces;
  EnumerationLimits enumeration;
  enumeration.fuel = options.fuel;
  enumeration.maxCells = options.maxCells;
  for (const std::vector<APInt> &point : points) {
    auto states = eventStates(program, point, options.referenceFill, enumeration);
    if (!states)
      continue;
    RunLimits limits;
    limits.fuel = options.fuel;
    limits.maxCells = options.maxCells;
    LoopReplay replay = replayLoop(program, slice, spec, states->entry, limits);
    ++out.points;
    out.activations += replay.traces.size();
    out.iterations += replay.iterations;
    out.stepMismatches += replay.stepMismatches;
    out.conditionMismatches += replay.conditionMismatches;
    out.subscriptMatches += replay.subscriptMatches;
    out.subscriptMismatches += replay.subscriptMismatches;
    out.undefined += replay.undefined;
    for (LoopTrace &trace : replay.traces)
      traces.push_back(std::move(trace));
  }
  // Leaves: m, the entries, the fixed variables, the integer parameters.
  std::vector<Term> leaves{spec.iteration};
  for (unsigned k = 0; k < spec.entry.size(); ++k)
    if (!spec.init[k]) // A known init leaves nothing for the entry value to say.
      leaves.push_back(spec.entry[k]);
  for (unsigned v : spec.fixed)
    if (program.state[v].sort.kind == Sort::BV)
      leaves.push_back(terms.state(v, program.state[v].sort));
  for (const Parameter &p : program.parameters)
    if (p.sort.kind == Sort::BV)
      leaves.push_back(terms.param(p.id, p.sort));
  std::vector<Sample> samples;
  for (const LoopTrace &trace : traces)
    addSamples(spec, trace, leaves, samples, options.maxSamples);
  out.samples = samples.size();
  for (unsigned v : spec.carried) {
    SynthesizedClosedForm form;
    form.variable = v;
    out.forms.push_back(form);
  }
  if (samples.empty()) {
    for (SynthesizedClosedForm &form : out.forms)
      form.reason = "no sample: the loop was not entered at any point";
    return;
  }
  // CEGIS: propose from the samples, check, replay a refuting model into a sample.
  for (unsigned round = 0; round < options.maxRounds; ++round) {
    Synthesizer synthesizer(program, spec, samples, options, leaves);
    std::vector<ClosedForm> candidates;
    for (unsigned k = 0; k < spec.carried.size(); ++k) {
      SynthesizedClosedForm &form = out.forms[k];
      if (form.status == ClosedFormStatus::Proved)
        candidates.push_back({form.variable, form.term, nullptr, {}});
      else {
        std::string why;
        auto found = synthesizer.find(k, why);
        if (found) {
          form.term = *found;
          form.text = programText(program, spec, *found);
          candidates.push_back({form.variable, *found, nullptr, {}});
        } else {
          form.term = nullptr;
          form.text.clear();
          form.status = ClosedFormStatus::Unknown;
          form.reason = why;
        }
      }
    }
    if (candidates.empty())
      return;
    auto checked = checkClosedForms(program, spec, candidates, options.scope, options.solver, backend);
    bool progress = false;
    for (const CheckedClosedForm &c : checked) {
      auto k = std::find(spec.carried.begin(), spec.carried.end(), c.variable) - spec.carried.begin();
      SynthesizedClosedForm &form = out.forms[k];
      if (form.status == ClosedFormStatus::Proved && c.status == ClosedFormStatus::Proved)
        continue; // Already proved in an earlier round; the check only repeated it.
      ++form.rounds;
      form.status = c.status;
      form.reason = c.reason;
      form.records = c.records;
      form.model = c.model;
      // Phase 4: a form whose own obligations hold but which substituted a form
      // not (yet) proved is checked once more with the proved forms alone, the
      // rest free; what it still needs stays Unknown.
      if (c.status == ClosedFormStatus::Unknown && c.discharged && c.model.empty()) {
        std::vector<ClosedForm> alone;
        for (unsigned u = 0; u < spec.carried.size(); ++u)
          if (u != (unsigned)k && out.forms[u].status == ClosedFormStatus::Proved && out.forms[u].term)
            alone.push_back({spec.carried[u], out.forms[u].term, nullptr, {}});
        alone.push_back({c.variable, form.term, nullptr, {}});
        auto again = checkClosedForms(program, spec, alone, options.scope, options.solver, backend);
        if (!again.empty() && again.back().variable == c.variable) {
          form.status = again.back().status;
          form.reason = again.back().reason;
          form.records = again.back().records;
          form.model = again.back().model;
        }
      }
      if (form.model.empty() ||
          (form.status != ClosedFormStatus::Refuted && form.status != ClosedFormStatus::Unknown))
        continue;
      // A model: the specification's own run from it says whether the form really
      // differs (a model over a free leaf, or under another candidate's form, may
      // not replay), and the run becomes samples.
      uint64_t at = 0;
      const LoopTrace *reference = traces.empty() ? nullptr : &traces.front();
      auto trace = replayModel(program, spec, form.model, reference, options, at);
      if (!trace) {
        if (form.status == ClosedFormStatus::Refuted) {
          form.status = ClosedFormStatus::Unknown;
          form.reason = "the solver's model could not be replayed through the specification; " + c.reason;
        }
        continue;
      }
      auto differs = formDisagreement(program, spec, form.term, k, *trace);
      if (differs) {
        form.status = ClosedFormStatus::Refuted;
        form.reason = "the specification's run from the solver's model differs from the form at m = " +
                      std::to_string(*differs);
      } else {
        form.status = ClosedFormStatus::Unknown;
        form.reason = "the solver's model does not replay into a disagreement, so it refutes nothing; " + c.reason;
      }
      size_t before = samples.size();
      addSamples(spec, *trace, leaves, samples, options.maxSamples);
      traces.push_back(std::move(*trace));
      progress = progress || samples.size() > before;
    }
    out.samples = samples.size();
    bool pending = false;
    for (const SynthesizedClosedForm &form : out.forms)
      pending = pending || form.status == ClosedFormStatus::Refuted;
    if (!pending || !progress)
      break;
  }
  if (!options.whilePrograms)
    return;
  // Phase 3: a while program for every variable without a proved loop-free form,
  // judged at the finite tier only.
  std::map<unsigned, Term> provedForms;
  for (const SynthesizedClosedForm &form : out.forms)
    if (form.status == ClosedFormStatus::Proved && form.term)
      provedForms[form.variable] = form.term;
  for (unsigned k = 0; k < spec.carried.size(); ++k) {
    SynthesizedClosedForm &form = out.forms[k];
    // A loop-free candidate that is only waiting on another variable's form, or
    // that the solver could not decide, stays as it is reported.
    bool none = form.term == nullptr;
    if (!none && form.status != ClosedFormStatus::Refuted)
      continue;
    SynthesizedClosedForm attempt = form;
    synthesizeWhileForm(program, spec, k, leaves, provedForms, traces, options, backend, attempt);
    // Keep the loop-free record when the while search found nothing at all.
    if (attempt.program || none)
      form = std::move(attempt);
    // Phase 5: a history table when nothing is PROVED yet. A proved table replaces
    // anything; a sample-consistent table replaces only a record that has less (a
    // while program consistent with the samples is kept, with the table's word).
    if (options.tablePrograms && form.status != ClosedFormStatus::Proved) {
      SynthesizedClosedForm table = form;
      synthesizeTableForm(program, spec, k, leaves, provedForms, traces, options, backend, table);
      bool haveConsistent = form.status == ClosedFormStatus::SampleConsistent && form.program;
      if (table.status == ClosedFormStatus::Proved)
        form = std::move(table);
      else if (haveConsistent)
        form.reason += "; a table program " + std::string(table.program ? "was " + std::string(stringifyClosedFormStatus(table.status)) + " and not proved either"
                                                                       : "was not found: " + table.reason);
      else if (table.program || table.status != ClosedFormStatus::Unknown)
        form = std::move(table);
      else if (!form.program && form.term == nullptr)
        form.reason += (form.reason.empty() ? "" : "; ") + std::string("table: ") + table.reason;
    }
  }
}

} // namespace

SliceSynthesisResult synthesizeClosedForms(const SemanticProgram &program,
                                           ArrayRef<std::vector<APInt>> points,
                                           const ClosedFormOptions &options, SmtBackend &backend) {
  SliceSynthesisResult out;
  auto slices = sliceSubscripts(program, options.slice, out.reason);
  if (!slices)
    return out;
  for (IndexSlice &slice : *slices) {
    SliceSynthesis entry;
    entry.slice = slice;
    for (const LoopSpecification &spec : slice.loops) {
      LoopSynthesis loop;
      loop.spec = spec;
      if (spec.reason.empty())
        synthesizeLoop(program, slice, points, options, backend, loop);
      entry.loops.push_back(std::move(loop));
    }
    out.slices.push_back(std::move(entry));
  }
  return out;
}

GrammarEnumeration enumerateGrammar(const SemanticProgram &program, ArrayRef<Term> leaves,
                                    ArrayRef<std::vector<APInt>> leafValues, size_t keyRows,
                                    unsigned width, const TermGrammar &grammar) {
  GrammarEnumeration out;
  std::vector<GrammarTerm> &pool = out.pool;
  size_t rows = keyRows;
  for (const std::vector<APInt> &values : leafValues)
    rows = std::max(rows, values.size());
  std::vector<unsigned> recorded; // Collisions reported per pool entry.
  TermContext &terms = *program.terms;
  auto offer = [&](Term term, unsigned size, std::vector<APInt> values) {
    if (pool.size() >= grammar.maxTerms)
      return;
    std::vector<uint64_t> key;
    key.reserve(keyRows);
    for (size_t r = 0; r < keyRows; ++r)
      key.push_back(values[r].getZExtValue());
    auto [slot, fresh] = out.seen.try_emplace(std::move(key), pool.size());
    if (!fresh) {
      unsigned earlier = slot->second;
      if (grammar.collisions && recorded[earlier] < grammar.collisionsPerTerm) {
        bool identity = true;
        for (size_t r = keyRows; r < values.size() && identity; ++r)
          identity = values[r] == pool[earlier].values[r];
        if (!identity) {
          ++recorded[earlier];
          out.collisions.push_back({earlier, GrammarTerm{term, size, std::move(values)}});
        }
      }
      return;
    }
    pool.push_back({term, size, std::move(values)});
    recorded.push_back(0);
  };
  // Constants first: 0 to 8, the program's literals of the width, the extras. A
  // leaf that only ever holds one of them, such as a materialized constant, then
  // falls to the same signature and the constant node stands for it.
  std::set<uint64_t> literal;
  for (uint64_t c = 0; c <= 8; ++c)
    literal.insert(c);
  std::function<void(Term)> collect = [&](Term t) {
    if (t->op == TermOp::Const && t->sort.kind == Sort::BV && t->sort.width == width)
      literal.insert(t->value.getZExtValue());
    for (Term a : t->args)
      collect(a);
  };
  for (const Transition &t : program.transitions) {
    collect(t.guard);
    for (const auto &[id, term] : t.assignments)
      if (term->sort.kind == Sort::BV)
        collect(term);
  }
  for (const APInt &extra : grammar.extraConstants)
    if (extra.getBitWidth() == width)
      literal.insert(extra.getZExtValue());
  for (uint64_t c : literal) {
    APInt value(width, c, /*isSigned=*/false, /*implicitTrunc=*/true);
    size_t before = pool.size();
    offer(terms.bv(value), 1, std::vector<APInt>(rows, value));
    if (pool.size() > before)
      out.constants.push_back(before);
  }
  // Leaves of the width.
  for (unsigned l = 0; l < leaves.size(); ++l) {
    if (leaves[l]->sort.width != width)
      continue;
    offer(leaves[l], 1, leafValues[l]);
  }
  std::set<unsigned> constantSet(out.constants.begin(), out.constants.end());
  auto isConstant = [&](unsigned n) { return constantSet.count(n) != 0; };
  auto combine = [&](TermOp op, unsigned a, unsigned b) {
    std::vector<APInt> values;
    values.reserve(rows);
    for (size_t s = 0; s < rows; ++s) {
      const APInt &x = pool[a].values[s], &y = pool[b].values[s];
      switch (op) {
      case TermOp::Add: values.push_back(x + y); break;
      case TermOp::Sub: values.push_back(x - y); break;
      case TermOp::Mul: values.push_back(x * y); break;
      case TermOp::UDiv: values.push_back(x.udiv(y)); break;
      case TermOp::URem: values.push_back(x.urem(y)); break;
      default: return;
      }
    }
    offer(terms.binary(op, pool[a].term, pool[b].term), pool[a].size + pool[b].size + 1,
          std::move(values));
  };
  // By size: a node of size s joins operands of sizes adding to s - 1.
  std::vector<std::vector<unsigned>> bySize(grammar.maxTermSize + 1);
  for (unsigned n = 0; n < pool.size(); ++n)
    bySize[1].push_back(n);
  for (unsigned size = 3; size <= grammar.maxTermSize; size += 2) {
    size_t start = pool.size();
    for (unsigned left = 1; left + 1 < size; ++left) {
      unsigned right = size - 1 - left;
      if (right > grammar.maxTermSize)
        continue;
      for (unsigned a : bySize[left])
        for (unsigned b : bySize[right]) {
          if (pool.size() >= grammar.maxTerms)
            break;
          if (isConstant(a) && isConstant(b))
            continue;
          if (pool[a].term->sort.width != width || pool[b].term->sort.width != width)
            continue;
          if (!grammar.polynomial) {
            combine(TermOp::Add, a, b);
            combine(TermOp::Sub, a, b);
            combine(TermOp::Mul, a, b);
          } else {
            if (left < right || (left == right && a <= b))
              combine(TermOp::Add, a, b);
            combine(TermOp::Sub, a, b);
            if (isConstant(a) && !isConstant(b))
              combine(TermOp::Mul, a, b);
          }
          // Division and remainder by a constant above one only.
          if (grammar.division && isConstant(b) && pool[b].values[0].ugt(1)) {
            combine(TermOp::UDiv, a, b);
            combine(TermOp::URem, a, b);
          }
        }
    }
    for (unsigned n = start; n < pool.size(); ++n)
      bySize[size].push_back(n);
  }
  return out;
}

std::string closedFormsJSON(const SemanticProgram &program, const SliceSynthesisResult &result) {
  json::Array slices;
  for (const SliceSynthesis &s : result.slices) {
    json::Array uses, variables, loops;
    for (const SliceUse &u : s.slice.uses)
      uses.push_back(json::Object{{"site", (int64_t)u.site},
                                  {"operand", (int64_t)u.operand},
                                  {"dimension", (int64_t)u.dimension}});
    for (unsigned v : s.slice.variables)
      variables.push_back((int64_t)v);
    for (const LoopSynthesis &l : s.loops) {
      json::Array carried, fixed, defined, forms;
      for (unsigned v : l.spec.carried)
        carried.push_back((int64_t)v);
      for (unsigned v : l.spec.fixed)
        fixed.push_back((int64_t)v);
      for (unsigned v : l.spec.defined)
        defined.push_back((int64_t)v);
      auto leafName = [&](Term leaf) { return closedFormLeafName(program, l.spec, leaf); };
      for (const SynthesizedClosedForm &f : l.forms) {
        json::Array obligations;
        for (const ObligationRecord &r : f.records)
          obligations.push_back(json::Object{{"id", r.id}, {"result", r.result},
                                             {"discharged", r.discharged}});
        json::Object model;
        for (const auto &[name, value] : f.model)
          model[name] = (int64_t)value.getZExtValue();
        std::string termText;
        if (f.term) {
          // formatTerm names leaves by their node text; the program text names them.
          termText = formatTerm(f.term, 12);
        }
        forms.push_back(json::Object{{"variable", (int64_t)f.variable},
                                     {"kind", f.kind},
                                     {"status", stringifyClosedFormStatus(f.status)},
                                     {"reason", f.reason},
                                     {"program", f.text},
                                     {"term", termText},
                                     {"rounds", (int64_t)f.rounds},
                                     {"counterexample_traces", (int64_t)f.counterexampleTraces},
                                     {"symbolic_rounds", (int64_t)f.symbolicRounds},
                                     {"history", [&] {
                                        json::Array h;
                                        for (unsigned v : f.history)
                                          h.push_back((int64_t)v);
                                        return h;
                                      }()},
                                     {"fuel_cuts", (int64_t)f.fuelCuts},
                                     {"obligations", std::move(obligations)},
                                     {"model", std::move(model)}});
      }
      (void)leafName;
      json::Object loop{{"header", (int64_t)l.spec.header},
                        {"innermost", l.spec.innermost},
                        {"reason", l.spec.reason},
                        {"carried", std::move(carried)},
                        {"fixed", std::move(fixed)},
                        {"defined", std::move(defined)},
                        {"closed_forms", std::move(forms)}};
      if (l.spec.reason.empty()) {
        loop["condition"] = formatTerm(l.spec.condition, 12);
        loop["replay"] = json::Object{{"points", (int64_t)l.points},
                                      {"activations", (int64_t)l.activations},
                                      {"iterations", (int64_t)l.iterations},
                                      {"step_mismatches", (int64_t)l.stepMismatches},
                                      {"condition_mismatches", (int64_t)l.conditionMismatches},
                                      {"subscript_matches", (int64_t)l.subscriptMatches},
                                      {"subscript_mismatches", (int64_t)l.subscriptMismatches},
                                      {"undefined", (int64_t)l.undefined}};
        loop["samples"] = (int64_t)l.samples;
      }
      loops.push_back(std::move(loop));
    }
    slices.push_back(json::Object{{"subscript", formatTerm(s.slice.subscript, 12)},
                                  {"uses", std::move(uses)},
                                  {"variables", std::move(variables)},
                                  {"reason", s.slice.reason},
                                  {"loops", std::move(loops)}});
  }
  json::Object out{{"reason", result.reason}, {"slices", std::move(slices)}};
  std::string text;
  raw_string_ostream stream(text);
  stream << formatv("{0:2}", json::Value(std::move(out)));
  return text;
}

} // namespace mlir::generalized_gemm
