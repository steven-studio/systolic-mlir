#include "GeneralizedGemm/IndexInvariant.h"

#include "GeneralizedGemm/ControlStructure.h"
#include "GeneralizedGemm/LoopInvariant.h"
#include "GeneralizedGemm/SymbolicDomain.h"
#include "GeneralizedGemm/TerminationObligations.h"

#include "llvm/ADT/DenseSet.h"

#include <algorithm>
#include <functional>
#include <set>

using namespace llvm;

namespace mlir::generalized_gemm {

const char *stringifyIndexInvariantVerdict(IndexInvariantVerdict verdict) {
  switch (verdict) {
  case IndexInvariantVerdict::Inductive: return "INDUCTIVE";
  case IndexInvariantVerdict::NotInductive: return "NOT_INDUCTIVE";
  case IndexInvariantVerdict::Unknown: return "UNKNOWN";
  }
  return "UNKNOWN";
}

const char *stringifyIndexRelationVerdict(IndexRelationVerdict verdict) {
  switch (verdict) {
  case IndexRelationVerdict::Proved: return "PROVED";
  case IndexRelationVerdict::Refuted: return "REFUTED";
  case IndexRelationVerdict::Unknown: return "UNKNOWN";
  }
  return "UNKNOWN";
}

namespace {

using Aux = std::vector<std::pair<std::string, Term>>;

bool readsShadow(Term root) {
  DenseSet<Term> seen;
  std::function<bool(Term)> visit = [&](Term t) {
    if (!seen.insert(t).second)
      return false;
    return t->op == TermOp::Shadow || llvm::any_of(t->args, visit);
  };
  return visit(root);
}

const char *operandName(unsigned operand) {
  return operand == 0 ? "A" : operand == 1 ? "B" : "C";
}

/// The checker of one certificate and its relations. Every verdict it gives comes
/// from a solver's unsat or from a run of the program.
class IndexChecker {
public:
  IndexChecker(const SemanticProgram &program, ArrayRef<unsigned> cuts,
               const IndexInvariantCertificate &certificate, ArrayRef<IndexRelation> relations,
               const CoordinateFamily *family, const IndexProofOptions &options,
               SmtBackend &backend)
      : program(program), cuts(cuts), certificate(certificate), relations(relations),
        family(family), options(options), backend(backend), terms(*program.terms) {}

  CheckedIndexProof run();

private:
  bool adopt();
  void induction();
  void uses();
  Term conjunction(ArrayRef<Term> list) const;
  Term invariantAt(unsigned cut) const;
  bool ask(const std::string &id, const std::string &location, Term formula, const Aux &aux,
           SmtResponse &response, size_t &record);
  std::string modelText(const SmtResponse &response) const;
  std::vector<APInt> modelParameters(const SmtResponse &response) const;
  bool confirm(const SmtResponse &response, const IndexRelation &claim, Term relation,
               CheckedIndexRelation &checked, std::string &note);
  bool overCoordinates(unsigned operand, Term relation) const;
  void fail(const std::string &id, const CutPath &path, std::vector<unsigned> falsified,
            const SmtResponse &response, const std::string &how);

  const SemanticProgram &program;
  ArrayRef<unsigned> cuts;
  const IndexInvariantCertificate &certificate;
  ArrayRef<IndexRelation> relations;
  const CoordinateFamily *family;
  const IndexProofOptions &options;
  SmtBackend &backend;
  TermContext &terms;
  CheckedIndexProof out;
  Term scope = nullptr;
  PathSet set;
  std::map<unsigned, std::vector<Term>> atoms;
  bool failed = false, open = false;
};

Term IndexChecker::conjunction(ArrayRef<Term> list) const {
  Term all = terms.boolean(true);
  for (Term t : list)
    all = terms.logicalAnd(all, t);
  return all;
}

Term IndexChecker::invariantAt(unsigned cut) const {
  auto found = atoms.find(cut);
  return found == atoms.end() ? terms.boolean(true) : conjunction(found->second);
}

bool IndexChecker::adopt() {
  std::string why;
  if (!program.isSupported()) {
    out.invariantReason = "the program is unsupported";
    return false;
  }
  scope = scopeFormula(program, options.scope, why);
  if (!scope) {
    out.invariantReason = "the scope is malformed: " + why;
    return false;
  }
  // The cut set is checked here, never taken from the caller's word.
  if (!cutSetBreaksCycles(program, cuts, why)) {
    out.invariantReason = "the cuts are no cut set of the program: " + why;
    return false;
  }
  set = enumerateCutPaths(program, cuts, options.paths);
  if (!set.complete) {
    out.invariantReason = "the paths between the cuts were not enumerated: " + set.reason;
    return false;
  }
  for (const CutInvariant &entry : certificate.cuts) {
    if (!std::binary_search(cuts.begin(), cuts.end(), entry.cut) || atoms.count(entry.cut)) {
      out.invariantReason = "an invariant names no cut, or repeats one";
      return false;
    }
    std::vector<Term> &list = atoms[entry.cut];
    for (unsigned k = 0; k < entry.atoms.size(); ++k) {
      std::string where = "atom " + std::to_string(k) + " at L" + std::to_string(entry.cut);
      Term adopted = adoptProofTerm(program, entry.atoms[k], why);
      if (!adopted) {
        out.invariantReason = where + " refused: " + why;
        return false;
      }
      if (adopted->sort != Sort::bv(1)) {
        out.invariantReason = where + " is not a condition";
        return false;
      }
      list.push_back(adopted);
    }
  }
  return true;
}

bool IndexChecker::ask(const std::string &id, const std::string &location, Term formula,
                       const Aux &aux, SmtResponse &response, size_t &record) {
  ObligationRecord entry;
  entry.id = id;
  entry.location = location;
  entry.expected = "unsat";
  record = out.records.size();
  if (out.queries >= options.maxQueries) {
    entry.result = "not-run";
    entry.evidence = "the checker's query limit";
    response = SmtResponse();
    response.detail = "the checker's query limit";
    out.records.push_back(std::move(entry));
    return false;
  }
  std::string script = obligationScript(id + " " + location, formula, aux);
  ++out.queries;
  out.scriptBytes += script.size();
  response = backend.check(script, options.solver, /*wantModel=*/true);
  entry.result = stringifySmtAnswer(response.answer);
  entry.discharged = response.answer == SmtAnswer::Unsat;
  if (response.answer == SmtAnswer::Unknown)
    entry.evidence = "the solver did not decide: " + response.detail;
  if (options.scripts)
    entry.script = std::move(script);
  out.records.push_back(std::move(entry));
  return true;
}

std::string IndexChecker::modelText(const SmtResponse &response) const {
  std::string text;
  for (const auto &[name, value] : response.model) {
    if (name.rfind("h", 0) == 0 && name.size() > 1 && std::isdigit((unsigned char)name[1]))
      continue; // The atoms' values are reported as the falsified list.
    if (!text.empty())
      text += " ";
    text += name + "=" + value;
  }
  return text.empty() ? "(empty model)" : text;
}

/// The parameters of a model. One the solver left out is free: it takes the lower end
/// of its range, or zero when it has none.
std::vector<APInt> IndexChecker::modelParameters(const SmtResponse &response) const {
  std::vector<APInt> point;
  for (const Parameter &p : program.parameters) {
    APInt value = APInt::getZero(p.sort.width);
    if (!options.scope.allParameters)
      for (const ParameterRange &range : options.scope.ranges)
        if (range.parameter == p.id && range.lower.getBitWidth() == p.sort.width)
          value = range.lower;
    auto found = response.model.find("p" + std::to_string(p.id));
    if (found != response.model.end() && p.sort.kind == Sort::BV)
      value = APInt(p.sort.width, found->second, 10);
    point.push_back(value);
  }
  return point;
}

void IndexChecker::fail(const std::string &id, const CutPath &path, std::vector<unsigned> falsified,
                        const SmtResponse &response, const std::string &how) {
  if (failed)
    return; // The first failure stands as the reason.
  failed = true;
  out.invariantReason =
      id + " has a model on " + describePath(program, path) + how +
      (path.fromEntry ? ": an initial state violates the invariant at L"
                      : ": the conjunction is not inductive (the model's state need not be "
                        "reachable), at L") +
      std::to_string(path.to);
  out.falsifiedBy = id;
  out.falsifiedCut = path.to;
  out.falsified = std::move(falsified);
  out.model = modelText(response);
  if (path.fromEntry) {
    // An initial state: its parameters, for a proposer that wants a run from them.
    out.initialParameters = modelParameters(response);
    if (!parametersInScope(options.scope, out.initialParameters))
      out.initialParameters.clear();
  }
}

/// IDX-init and IDX-step over every path into a cut that has atoms. The conjunction
/// first; when the solver does not decide it, one query per atom.
void IndexChecker::induction() {
  for (const CutPath &path : set.paths) {
    if (path.to == program.exitLocation())
      continue;
    auto target = atoms.find(path.to);
    if (target == atoms.end() || target->second.empty())
      continue; // Nothing is claimed at this cut.
    const std::vector<Term> &list = target->second;
    std::string id = path.fromEntry ? "IDX-init" : "IDX-step";
    std::string location = describePath(program, path);
    Term source = path.fromEntry ? terms.boolean(true) : invariantAt(path.from);
    Term formula = invariantViolation(program, scope, path, source, conjunction(list));
    Aux aux;
    for (unsigned k = 0; k < list.size(); ++k)
      aux.push_back({"h" + std::to_string(k), postTerm(program, path, list[k])});
    SmtResponse response;
    size_t index = 0;
    bool asked = ask(id, location, formula, aux, response, index);
    if (asked && response.answer == SmtAnswer::Unsat)
      continue;
    if (asked && response.answer == SmtAnswer::Sat) {
      std::vector<unsigned> falsified;
      for (unsigned k = 0; k < list.size(); ++k) {
        auto value = response.model.find("h" + std::to_string(k));
        if (value == response.model.end() || value->second != "1")
          falsified.push_back(k);
      }
      std::string text;
      for (unsigned k : falsified)
        text += (text.empty() ? "" : ", ") + std::to_string(k);
      out.records[index].evidence =
          std::string(path.fromEntry ? "at an initial state, " : "after this path, ") + "atom(s) " +
          text + " of L" + std::to_string(path.to) + " false; model " + modelText(response);
      fail(id, path, std::move(falsified), response, "");
      if (options.failFast)
        return;
      continue;
    }
    if (!asked) {
      open = true;
      if (out.invariantReason.empty())
        out.invariantReason = id + " on " + location + ": the checker's query limit";
      return;
    }
    // Undecided together: one atom at a time.
    std::vector<unsigned> falsified, undecided;
    SmtResponse witness;
    for (unsigned k = 0; k < list.size(); ++k) {
      Term single = invariantViolation(program, scope, path, source, list[k]);
      SmtResponse alone;
      size_t at = 0;
      bool ran = ask(id + "[atom " + std::to_string(k) + "]", location, single, {}, alone, at);
      if (ran && alone.answer == SmtAnswer::Sat) {
        if (falsified.empty())
          witness = alone;
        falsified.push_back(k);
        out.records[at].evidence = "model " + modelText(alone);
      } else if (!ran || alone.answer == SmtAnswer::Unknown) {
        undecided.push_back(k);
      }
    }
    if (!falsified.empty()) {
      bool first = !failed;
      fail(id, path, std::move(falsified), witness, " (atom by atom)");
      if (first) {
        out.undecidedCut = path.to;
        out.undecided = undecided;
      }
      if (options.failFast)
        return;
      continue;
    }
    if (!undecided.empty()) {
      if (!open && !failed) {
        out.invariantReason = id + " on " + location +
                              ": the solver decided neither the conjunction nor every atom";
        out.undecidedCut = path.to;
        out.undecided = undecided;
      }
      open = true;
      if (options.failFast)
        return;
      continue;
    }
    // Every atom alone is unsat: each atom of Inv at the target follows from Inv at
    // the source, so their conjunction does.
    out.records[index].evidence = "the conjunction was not decided; every atom alone is unsat";
  }
}

/// Whether `relation` reads state only through the coordinates of the operand's axes.
bool IndexChecker::overCoordinates(unsigned operand, Term relation) const {
  static const unsigned axes[3][3] = {
      {AxisP, AxisI, AxisK}, {AxisP, AxisK, AxisJ}, {AxisP, AxisI, AxisJ}};
  std::set<Term> allowed;
  for (unsigned axis : axes[operand])
    for (Term term : family->axes[axis]) {
      std::string why;
      if (Term adopted = adoptProofTerm(program, term, why))
        allowed.insert(adopted);
    }
  DenseSet<Term> seen;
  std::function<bool(Term)> through = [&](Term t) {
    if (allowed.count(t) || !seen.insert(t).second)
      return true;
    if (t->op == TermOp::State || t->op == TermOp::Shadow)
      return false;
    return llvm::all_of(t->args, through);
  };
  return through(relation);
}

/// A model of IDX-use may sit at a state no run reaches. It refutes the relation only
/// when the run from its parameters returns and has an event of the site whose recorded
/// subscript differs from the relation's value at the entry of the event's block.
bool IndexChecker::confirm(const SmtResponse &response, const IndexRelation &claim, Term relation,
                           CheckedIndexRelation &checked, std::string &note) {
  std::vector<APInt> parameters = modelParameters(response);
  if (!parametersInScope(options.scope, parameters)) {
    note = "the model's parameters are outside the scope";
    return false;
  }
  auto states = eventStates(program, parameters, options.referenceFill, options.confirmation);
  if (!states) {
    note = "no run was made from the model's parameters (an extent is negative, or the "
           "memory exceeds the cell limit)";
    return false;
  }
  ++out.confirmationRuns;
  out.confirmationSteps += states->run.steps;
  if (states->run.status != RunStatus::Returned) {
    note = std::string("the run from the model's parameters did not return (") +
           stringifyRunStatus(states->run.status) + "), so it shows no event of a completing run";
    return false;
  }
  uint64_t seen = 0;
  for (size_t e = 0; e < states->run.events.size(); ++e) {
    const MacEvent &event = states->run.events[e];
    if (event.site != claim.site)
      continue;
    const std::vector<APInt> &index =
        claim.operand == 0 ? event.f : claim.operand == 1 ? event.g : event.h;
    if (claim.dimension >= index.size())
      continue;
    Configuration config;
    config.location = program.sites[claim.site].location;
    config.state = states->states[e];
    config.shadow.assign(program.state.size(), false);
    auto value = evaluateTerm(program, states->entry, config, relation);
    if (!value)
      continue; // The relation reads a variable with no value there: nothing to compare.
    ++seen;
    if (*value == index[claim.dimension])
      continue;
    IndexRelationWitness witness;
    witness.parameters = parameters;
    witness.event = e;
    witness.index = index[claim.dimension];
    witness.claimed = *value;
    witness.steps = states->run.steps;
    checked.witness = std::move(witness);
    return true;
  }
  note = "the run from the model's parameters returns, and the relation holds at each of its " +
         std::to_string(seen) + " event(s) of the site";
  return false;
}

/// IDX-use, for each relation at every arrival at its site, under the invariants.
void IndexChecker::uses() {
  out.relations.resize(relations.size());
  std::vector<Term> adopted(relations.size(), nullptr), subscript(relations.size(), nullptr);
  std::vector<unsigned> sites;
  for (size_t r = 0; r < relations.size(); ++r) {
    const IndexRelation &claim = relations[r];
    CheckedIndexRelation &checked = out.relations[r];
    checked.claim = claim;
    if (claim.site >= program.sites.size() || claim.operand > 2) {
      checked.reason = "no such site or operand";
      continue;
    }
    const MacSite &site = program.sites[claim.site];
    const MacAccess *access =
        claim.operand == 0 ? &site.a : claim.operand == 1 ? &site.b
                                                          : site.output ? &*site.output : nullptr;
    if (!access || claim.dimension >= access->index.size()) {
      checked.reason = "the operand has no such subscript";
      continue;
    }
    Term index = access->index[claim.dimension];
    std::string why;
    Term term = adoptProofTerm(program, claim.relation, why);
    if (!term) {
      checked.reason = "the relation is refused: " + why;
      continue;
    }
    if (term->sort != index->sort) {
      checked.reason = "the relation has another width than the subscript";
      continue;
    }
    if (readsShadow(term)) {
      checked.reason = "the relation reads a poison bit";
      continue;
    }
    if (readsMemory(index) || containsFloat(index)) {
      checked.reason = "the subscript reads memory";
      continue;
    }
    adopted[r] = term;
    subscript[r] = index;
    if (family)
      checked.overCoordinates = overCoordinates(claim.operand, term);
    if (!llvm::is_contained(sites, claim.site))
      sites.push_back(claim.site);
  }
  if (sites.empty())
    return;
  SiteArrivals arrivals = reachSites(program, out.invariants, cuts, sites, scope, options.paths);
  for (size_t r = 0; r < relations.size(); ++r) {
    if (!adopted[r])
      continue;
    const IndexRelation &claim = relations[r];
    CheckedIndexRelation &checked = out.relations[r];
    if (!arrivals.complete) {
      checked.reason = "the arrivals at the site were not enumerated: " + arrivals.reason;
      continue;
    }
    bool every = true;
    for (const SiteArrival &arrival : arrivals.arrivals) {
      if (arrival.site != claim.site)
        continue;
      ++checked.arrivals;
      unsigned start = arrival.path.transitions.empty()
                           ? arrival.location
                           : program.transitions[arrival.path.transitions.front()].from;
      std::string location = std::string(operandName(claim.operand)) + "[" +
                             std::to_string(claim.dimension) + "] of site " +
                             std::to_string(claim.site) + " at L" + std::to_string(arrival.location) +
                             ", arriving from L" + std::to_string(start);
      Term differs = terms.icmp(Predicate::Ne, atPathEnd(program, arrival.path, subscript[r]),
                                atPathEnd(program, arrival.path, adopted[r]));
      SmtResponse response;
      size_t index = 0;
      bool asked = ask("IDX-use", location, terms.logicalAnd(arrival.premise, differs), {}, response,
                       index);
      if (asked && response.answer == SmtAnswer::Unsat)
        continue;
      every = false;
      if (!asked || response.answer == SmtAnswer::Unknown) {
        checked.reason = "IDX-use on " + location + ": " +
                         (asked ? "the solver did not decide (" + response.detail + ")"
                                : std::string("the checker's query limit"));
        break;
      }
      std::string note;
      if (confirm(response, claim, adopted[r], checked, note)) {
        checked.verdict = IndexRelationVerdict::Refuted;
        checked.reason = "a run of the program has an event whose subscript is another value "
                         "(IDX-use model on " + location + ")";
        out.records[index].evidence = "model " + modelText(response) + "; confirmed by a run";
      } else {
        checked.unconfirmedModel = modelText(response);
        checked.reason = "IDX-use on " + location + " has a model no run confirms (" + note +
                         "): the relation is not proved, and nothing is refuted";
        out.records[index].evidence = "model " + checked.unconfirmedModel + "; not confirmed: " + note;
      }
      break;
    }
    if (!every)
      continue;
    if (!checked.arrivals) {
      checked.reason = "no path reaches the site";
      continue;
    }
    checked.verdict = IndexRelationVerdict::Proved;
    checked.reason = "IDX-use unsat at " + std::to_string(checked.arrivals) +
                     " arrival(s) at the site, under the invariants";
  }
}

CheckedIndexProof IndexChecker::run() {
  if (!adopt())
    return std::move(out);
  induction();
  if (failed) {
    out.invariant = IndexInvariantVerdict::NotInductive;
    return std::move(out);
  }
  if (open) {
    out.invariant = IndexInvariantVerdict::Unknown;
    return std::move(out);
  }
  out.invariant = IndexInvariantVerdict::Inductive;
  out.invariantReason = "IDX-init and IDX-step unsat on every path into a cut with atoms";
  for (unsigned cut : cuts)
    if (atoms.count(cut))
      out.invariants[cut] = invariantAt(cut);
  uses();
  return std::move(out);
}

} // namespace

CheckedIndexProof checkIndexProof(const SemanticProgram &program, ArrayRef<unsigned> cuts,
                                  const IndexInvariantCertificate &certificate,
                                  ArrayRef<IndexRelation> relations, const CoordinateFamily *family,
                                  const IndexProofOptions &options, SmtBackend &backend) {
  try {
    return IndexChecker(program, cuts, certificate, relations, family, options, backend).run();
  } catch (const TermError &error) {
    CheckedIndexProof out;
    out.invariantReason = std::string("term limit: ") + error.what();
    return out;
  }
}

TerminationCertificate strengthenCertificate(const SemanticProgram &program,
                                             const TerminationCertificate &certificate,
                                             const std::map<unsigned, Term> &invariants) {
  TermContext &terms = *program.terms;
  TerminationCertificate out = certificate;
  for (const auto &[cut, extra] : invariants) {
    if (!std::binary_search(out.cuts.begin(), out.cuts.end(), cut))
      continue; // Not a cut of this certificate: nothing of it can be placed.
    bool placed = false;
    for (auto &[at, invariant] : out.invariants)
      if (at == cut) {
        invariant = terms.logicalAnd(invariant, extra);
        placed = true;
      }
    if (!placed)
      out.invariants.push_back({cut, extra});
  }
  return out;
}

CutSamples sampleCutStates(const SemanticProgram &program, ArrayRef<unsigned> cuts,
                           ArrayRef<std::vector<APInt>> points, uint64_t referenceFill,
                           const EnumerationLimits &limits, size_t maxRowsPerCut) {
  CutSamples out;
  for (const Parameter &p : program.parameters)
    if (p.sort.kind == Sort::BV && p.sort.width <= 64)
      out.parameters.push_back(p.id);
  std::map<unsigned, unsigned> slot;
  for (unsigned cut : cuts) {
    slot[cut] = out.cuts.size();
    CutSamples::Cut entry;
    entry.cut = cut;
    out.cuts.push_back(std::move(entry));
  }
  if (!program.isSupported() || points.empty())
    return out;
  std::vector<bool> independent = memoryIndependentState(program);
  struct Arrival {
    size_t run;
    std::vector<std::optional<APInt>> state;
    std::vector<bool> shadow;
  };
  std::vector<std::vector<Arrival>> kept(out.cuts.size());
  std::vector<std::vector<APInt>> runPoints;
  // Per run and cut: the arrivals at evenly spaced positions, the first one always, so
  // that one long run cannot take the whole budget.
  size_t perRun = std::max<size_t>(8, maxRowsPerCut / points.size());
  for (const std::vector<APInt> &point : points) {
    auto entry = referenceEntryState(program, point, referenceFill, limits.maxCells);
    if (!entry)
      continue;
    RunLimits run;
    run.fuel = limits.fuel;
    run.maxCells = limits.maxCells;
    run.maxRecordedConfigurations = 0;
    std::string error;
    auto config = initialConfiguration(program, *entry, run, error);
    if (!config)
      continue;
    size_t id = runPoints.size();
    runPoints.push_back(point);
    ++out.runs;
    std::vector<std::vector<Arrival>> mine(out.cuts.size());
    std::vector<uint64_t> seen(out.cuts.size(), 0), stride(out.cuts.size(), 1);
    for (uint64_t step = 0;; ++step) {
      auto found = slot.find(config->location);
      if (found != slot.end()) {
        unsigned c = found->second;
        ++out.cuts[c].arrivals;
        if (seen[c] % stride[c] == 0) {
          mine[c].push_back({id, config->state, config->shadow});
          if (mine[c].size() > perRun) {
            // Thin to every other one, and take half as often from here on.
            std::vector<Arrival> half;
            for (size_t k = 0; k < mine[c].size(); k += 2)
              half.push_back(std::move(mine[c][k]));
            mine[c] = std::move(half);
            stride[c] *= 2;
          }
        }
        ++seen[c];
      }
      if (config->location == program.exitLocation()) {
        ++out.returned;
        break;
      }
      if (step >= run.fuel)
        break;
      StepOutcome outcome = stepConfiguration(program, *entry, *config);
      if (outcome.stopped) {
        ++out.stopped;
        break;
      }
      ++out.steps;
    }
    for (size_t c = 0; c < mine.size(); ++c)
      for (Arrival &arrival : mine[c])
        kept[c].push_back(std::move(arrival));
  }
  for (size_t c = 0; c < out.cuts.size(); ++c) {
    CutSamples::Cut &cut = out.cuts[c];
    if (kept[c].empty())
      continue;
    for (const StateVar &var : program.state) {
      if (var.sort.kind != Sort::BV || var.sort.width > 64 || !independent[var.id])
        continue;
      bool always = llvm::all_of(kept[c], [&](const Arrival &a) { return a.state[var.id].has_value(); });
      if (always)
        cut.variables.push_back(var.id);
    }
    size_t take = std::min(kept[c].size(), maxRowsPerCut);
    for (size_t k = 0; k < take; ++k) {
      // Evenly spaced over what was kept, ends included.
      const Arrival &arrival = kept[c][take == 1 ? 0 : k * (kept[c].size() - 1) / (take - 1)];
      std::vector<APInt> row;
      for (unsigned v : cut.variables)
        row.push_back(*arrival.state[v]);
      for (unsigned v : cut.variables)
        row.push_back(APInt(1, arrival.shadow[v] ? 1 : 0));
      for (unsigned p : out.parameters)
        row.push_back(runPoints[arrival.run][p]);
      cut.rows.push_back(std::move(row));
    }
  }
  return out;
}

} // namespace mlir::generalized_gemm
