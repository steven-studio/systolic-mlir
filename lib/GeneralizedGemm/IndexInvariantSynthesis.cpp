#include "GeneralizedGemm/IndexInvariantSynthesis.h"

#include "GeneralizedGemm/SymbolicDomain.h"
#include "GeneralizedGemm/TerminationObligations.h"

#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/raw_ostream.h"

#include <algorithm>
#include <functional>
#include <set>

using namespace llvm;

namespace mlir::generalized_gemm {

namespace {

std::string decimal(const APInt &value) { return llvm::toString(value, 10, /*Signed=*/true); }

/// A polynomial over opaque leaves with coefficients modulo 2^width: what the proposer
/// uses to solve an equality for a variable and to rewrite a subscript. Nothing here is
/// trusted: a relation it builds is a claim for IDX-use.
struct Poly {
  unsigned width = 0;
  std::map<std::vector<unsigned>, APInt> terms; // Sorted leaf ids -> nonzero coefficient.
};

class Polynomials {
public:
  explicit Polynomials(TermContext &terms) : terms(terms) {}

  std::optional<Poly> from(Term term);
  Term toTerm(const Poly &poly);
  /// lhs - rhs solved for the state variable `variable`: its solution, when it occurs
  /// only as a linear monomial with an odd coefficient (a unit modulo 2^w).
  std::optional<Poly> solve(const Poly &difference, unsigned variable);
  /// State ids the polynomial reads, through leaves and opaque terms alike.
  std::set<unsigned> support(const Poly &poly) const;
  Term leafOf(unsigned id) const { return leaves.at(id); }

private:
  static void add(Poly &into, const std::vector<unsigned> &monomial, const APInt &coefficient) {
    auto [slot, fresh] = into.terms.try_emplace(monomial, coefficient);
    if (!fresh)
      slot->second += coefficient;
    if (slot->second.isZero())
      into.terms.erase(slot);
  }
  std::optional<Poly> product(const Poly &a, const Poly &b);
  std::optional<unsigned> stateLeaf(unsigned variable) const;

  TermContext &terms;
  std::map<unsigned, Term> leaves;
  static constexpr size_t maxMonomials = 256;
};

std::optional<Poly> Polynomials::product(const Poly &a, const Poly &b) {
  Poly out;
  out.width = a.width;
  for (const auto &[ma, ca] : a.terms)
    for (const auto &[mb, cb] : b.terms) {
      std::vector<unsigned> m = ma;
      m.insert(m.end(), mb.begin(), mb.end());
      std::sort(m.begin(), m.end());
      add(out, m, ca * cb);
      if (out.terms.size() > maxMonomials)
        return std::nullopt;
    }
  return out;
}

std::optional<Poly> Polynomials::from(Term term) {
  if (term->sort.kind != Sort::BV)
    return std::nullopt;
  Poly out;
  out.width = term->sort.width;
  if (term->op == TermOp::Const) {
    if (!term->value.isZero())
      out.terms[{}] = term->value;
    return out;
  }
  if ((term->op == TermOp::Add || term->op == TermOp::Sub || term->op == TermOp::Mul) &&
      term->args.size() == 2) {
    auto a = from(term->args[0]), b = from(term->args[1]);
    if (!a || !b)
      return std::nullopt;
    if (term->op == TermOp::Mul)
      return product(*a, *b);
    out = *a;
    for (const auto &[m, c] : b->terms)
      add(out, m, term->op == TermOp::Add ? c : -c);
    if (out.terms.size() > maxMonomials)
      return std::nullopt;
    return out;
  }
  // A leaf, or any other node read as one.
  leaves[term->id] = term;
  out.terms[{term->id}] = APInt(out.width, 1);
  return out;
}

Term Polynomials::toTerm(const Poly &poly) {
  auto monomial = [&](const std::vector<unsigned> &m) {
    Term t = nullptr;
    for (unsigned id : m)
      t = t ? terms.binary(TermOp::Mul, t, leaves.at(id)) : leaves.at(id);
    return t;
  };
  Term sum = nullptr;
  // Positive summands first, so a sum reads "a + b - c" where it can.
  for (bool negative : {false, true})
    for (const auto &[m, c] : poly.terms) {
      if (m.empty() || c.isNegative() != negative)
        continue;
      APInt magnitude = negative ? -c : c;
      Term t = magnitude.isOne() ? monomial(m)
                                 : terms.binary(TermOp::Mul, terms.bv(magnitude), monomial(m));
      if (!sum)
        sum = negative ? terms.binary(TermOp::Sub, terms.bv(APInt::getZero(poly.width)), t) : t;
      else
        sum = terms.binary(negative ? TermOp::Sub : TermOp::Add, sum, t);
    }
  auto constant = poly.terms.find({});
  if (constant != poly.terms.end()) {
    const APInt &c = constant->second;
    if (!sum)
      sum = terms.bv(c);
    else if (c.isNegative())
      sum = terms.binary(TermOp::Sub, sum, terms.bv(-c));
    else
      sum = terms.binary(TermOp::Add, sum, terms.bv(c));
  }
  return sum ? sum : terms.bv(APInt::getZero(poly.width));
}

std::set<unsigned> Polynomials::support(const Poly &poly) const {
  std::set<unsigned> out;
  for (const auto &[m, c] : poly.terms)
    for (unsigned id : m)
      for (unsigned v : stateSupport(leaves.at(id)))
        out.insert(v);
  return out;
}

/// The leaf id of the plain state variable, when some leaf is exactly it.
std::optional<unsigned> Polynomials::stateLeaf(unsigned variable) const {
  for (const auto &[id, term] : leaves)
    if (term->op == TermOp::State && term->index == variable)
      return id;
  return std::nullopt;
}

std::optional<Poly> Polynomials::solve(const Poly &difference, unsigned variable) {
  auto leaf = stateLeaf(variable);
  if (!leaf)
    return std::nullopt;
  auto linear = difference.terms.find({*leaf});
  if (linear == difference.terms.end() || !linear->second[0])
    return std::nullopt; // Absent, or an even coefficient: no unit to divide by.
  for (const auto &[m, c] : difference.terms) {
    if (m == std::vector<unsigned>{*leaf})
      continue;
    for (unsigned id : m)
      if (llvm::is_contained(stateSupport(leaves.at(id)), variable))
        return std::nullopt; // The variable occurs elsewhere: not solved by this monomial.
  }
  APInt scale = -linear->second.multiplicativeInverse();
  Poly out;
  out.width = difference.width;
  for (const auto &[m, c] : difference.terms)
    if (m != std::vector<unsigned>{*leaf})
      add(out, m, c * scale);
  return out;
}

/// The equalities among the conjuncts of a cut's atoms, as differences lhs - rhs.
std::vector<Poly> equalities(Polynomials &polys, ArrayRef<Term> atoms) {
  std::vector<Poly> out;
  for (Term atom : atoms)
    for (Term part : conjuncts(atom)) {
      if (part->op != TermOp::ICmp || (Predicate)part->aux != Predicate::Eq)
        continue;
      auto a = polys.from(part->args[0]), b = polys.from(part->args[1]);
      if (!a || !b)
        continue;
      Poly difference = *a;
      for (const auto &[m, c] : b->terms) {
        auto [slot, fresh] = difference.terms.try_emplace(m, -c);
        if (!fresh)
          slot->second -= c;
        if (slot->second.isZero())
          difference.terms.erase(slot);
      }
      out.push_back(std::move(difference));
    }
  return out;
}

/// Whether some transition can set the variable's poison bit.
bool canBePoison(const SemanticProgram &program, unsigned variable) {
  for (const Transition &t : program.transitions)
    for (const auto &[id, term] : t.shadowAssignments)
      if (id == variable && !(term->op == TermOp::Const && term->value.isZero()))
        return true;
  return false;
}

class IndexSearch {
public:
  IndexSearch(const SemanticProgram &program, const TerminationCertificate &certificate,
              const IndexInvariantSearchOptions &options, SmtBackend &backend,
              IndexInvariantSearch &out)
      : program(program), base(certificate), options(options), backend(backend), out(out),
        terms(*program.terms) {}

  void run(const IndexInvariantCertificate *given);

private:
  using Candidates = std::map<unsigned, std::vector<Term>>;

  bool inherit();
  void sample();
  Candidates offer(unsigned degree, const Candidates &kept, IndexInvariantLevel &level);
  IndexInvariantCertificate assemble(const Candidates &candidates, const std::string &origin) const;
  bool houdini(Candidates &candidates, const Candidates &kept, unsigned degree,
               IndexInvariantLevel &level);
  void strengthen(const Candidates &candidates);
  size_t count(const Candidates &candidates) const {
    size_t n = 0;
    for (const auto &[cut, list] : candidates)
      n += list.size();
    return n;
  }

  const SemanticProgram &program;
  const TerminationCertificate &base;
  const IndexInvariantSearchOptions &options;
  SmtBackend &backend;
  IndexInvariantSearch &out;
  TermContext &terms;
  std::map<unsigned, Term> inheritedAtom;         // The certificate's invariant per cut.
  std::map<unsigned, std::set<Term>> inheritedParts; // Its conjuncts, to skip repeats.
  std::vector<std::vector<APInt>> points;
  // Never offered again: an atom false at an initial state (a state a run reaches), and
  // an atom the solver did not decide. An atom an IDX-step model falsified only leaves
  // the current round: that model's state need not be reachable, and the atom may be
  // inductive next to the atoms a larger sample offers.
  std::set<std::pair<unsigned, Term>> refuted;
};

/// The termination certificate's invariants, adopted: the first atom of each cut.
bool IndexSearch::inherit() {
  for (const auto &[cut, term] : base.invariants) {
    std::string why;
    Term adopted = adoptProofTerm(program, term, why);
    if (!adopted || adopted->sort != Sort::bv(1)) {
      out.reason = "the termination certificate's invariant at L" + std::to_string(cut) +
                   " cannot be read: " + why;
      return false;
    }
    if (adopted->op == TermOp::Const && adopted->value.isOne())
      continue;
    inheritedAtom[cut] = adopted;
    for (Term part : conjuncts(adopted))
      inheritedParts[cut].insert(part);
  }
  return true;
}

void IndexSearch::sample() {
  out.samples = sampleCutStates(program, out.cuts, points, options.check.referenceFill,
                                options.check.confirmation, options.maxRowsPerCut);
  out.samplePoints = points.size();
}

/// The candidates of one level from the current sample: at each cut, the null-space
/// equalities over the variables no kept equality already determines, and the
/// not-poison atoms; minus what was falsified before and what the certificate's own
/// invariant already states.
IndexSearch::Candidates IndexSearch::offer(unsigned degree, const Candidates &kept,
                                           IndexInvariantLevel &level) {
  Candidates fresh;
  for (const CutSamples::Cut &cut : out.samples.cuts) {
    if (cut.rows.empty())
      continue;
    // Variables a kept equality determines carry no new relation at a higher degree.
    std::vector<unsigned> variables = cut.variables;
    std::vector<size_t> columns(variables.size());
    for (size_t i = 0; i < columns.size(); ++i)
      columns[i] = i;
    auto before = kept.find(cut.cut);
    if (before != kept.end() && !before->second.empty()) {
      Polynomials polys(terms);
      std::vector<Poly> solved = equalities(polys, before->second);
      std::set<unsigned> gone;
      for (const Poly &difference : solved) {
        // The last variable of the equality that it determines, by id.
        std::set<unsigned> reads = polys.support(difference);
        for (auto v = reads.rbegin(); v != reads.rend(); ++v) {
          if (gone.count(*v) || !llvm::is_contained(variables, *v) || !polys.solve(difference, *v))
            continue;
          gone.insert(*v);
          break;
        }
      }
      std::vector<unsigned> left;
      std::vector<size_t> leftColumns;
      for (size_t i = 0; i < variables.size(); ++i)
        if (!gone.count(variables[i])) {
          left.push_back(variables[i]);
          leftColumns.push_back(i);
        }
      variables = std::move(left);
      columns = std::move(leftColumns);
    }
    size_t all = cut.variables.size();
    std::vector<std::vector<APInt>> rows;
    for (const auto &row : cut.rows) {
      std::vector<APInt> line;
      for (size_t c : columns)
        line.push_back(row[c]);
      for (size_t c : columns)
        line.push_back(row[all + c]);
      line.insert(line.end(), row.begin() + 2 * all, row.end());
      rows.push_back(std::move(line));
    }
    NullSpaceStats stats;
    std::vector<Term> atoms = nullSpaceEqualities(program, variables, out.samples.parameters, rows,
                                                  degree, options.maxColumns, stats,
                                                  options.truncations);
    level.columns += stats.columns;
    level.rows += stats.rows;
    level.rank += stats.rank;
    level.basis += stats.basis;
    level.skipped += stats.skipped;
    level.folded += stats.folded;
    if (options.poisonAtoms)
      for (size_t i = 0; i < all; ++i) {
        unsigned v = cut.variables[i];
        if (!canBePoison(program, v))
          continue;
        bool clean = llvm::all_of(cut.rows, [&](const auto &row) { return row[all + i].isZero(); });
        if (clean)
          atoms.push_back(terms.logicalNot(terms.shadow(v)));
      }
    std::vector<Term> &list = fresh[cut.cut];
    auto known = [&](Term atom) {
      if (refuted.count({cut.cut, atom}) || llvm::is_contained(list, atom))
        return true;
      if (before != kept.end() && llvm::is_contained(before->second, atom))
        return true;
      auto parts = inheritedParts.find(cut.cut);
      if (parts == inheritedParts.end())
        return false;
      if (parts->second.count(atom))
        return true;
      // The same equality written the other way round.
      return atom->op == TermOp::ICmp && (Predicate)atom->aux == Predicate::Eq &&
             parts->second.count(terms.icmp(Predicate::Eq, atom->args[1], atom->args[0]));
    };
    for (Term atom : atoms)
      if (!known(atom))
        list.push_back(atom);
  }
  return fresh;
}

IndexInvariantCertificate IndexSearch::assemble(const Candidates &candidates,
                                                const std::string &origin) const {
  IndexInvariantCertificate certificate;
  certificate.origin = origin;
  for (unsigned cut : out.cuts) {
    CutInvariant entry;
    entry.cut = cut;
    auto own = inheritedAtom.find(cut);
    if (own != inheritedAtom.end())
      entry.atoms.push_back(own->second);
    auto found = candidates.find(cut);
    if (found != candidates.end())
      entry.atoms.insert(entry.atoms.end(), found->second.begin(), found->second.end());
    if (!entry.atoms.empty())
      certificate.cuts.push_back(std::move(entry));
  }
  return certificate;
}

/// Houdini over all cuts. True when the conjunctions are inductive; false with
/// out.status and out.reason set otherwise.
bool IndexSearch::houdini(Candidates &candidates, const Candidates &kept, unsigned degree,
                          IndexInvariantLevel &level) {
  std::string origin = "search: Houdini over the null space of the sampled states at each cut, "
                       "degree " + std::to_string(degree);
  IndexProofOptions check = options.check;
  check.failFast = true;
  while (true) {
    if (out.checks >= options.maxHoudiniChecks) {
      out.status = "UNKNOWN";
      out.reason = "the search's check limit";
      return false;
    }
    out.certificate = assemble(candidates, origin);
    out.check = checkIndexProof(program, out.cuts, out.certificate, {}, nullptr, check, backend);
    ++out.checks;
    ++level.checks;
    out.queries += out.check.queries;
    if (out.check.invariant == IndexInvariantVerdict::Inductive)
      return true;
    bool falsified = out.check.invariant == IndexInvariantVerdict::NotInductive;
    unsigned cut = falsified ? out.check.falsifiedCut : out.check.undecidedCut;
    const std::vector<unsigned> &positions = falsified ? out.check.falsified : out.check.undecided;
    if (positions.empty()) {
      out.status = "UNKNOWN";
      out.reason = "the checker gave no verdict: " + out.check.invariantReason;
      return false;
    }
    unsigned own = inheritedAtom.count(cut) ? 1 : 0;
    std::vector<Term> &list = candidates[cut];
    std::set<Term> gone;
    for (unsigned k : positions) {
      if (k < own || k - own >= list.size()) {
        // The certificate's own invariant, which G2a proved inductive, has a model here:
        // nothing may be built on that.
        out.status = "UNKNOWN";
        out.reason = "the termination certificate's own invariant at L" + std::to_string(cut) +
                     " was " + (falsified ? "falsified" : "not decided") + ": " +
                     out.check.invariantReason;
        return false;
      }
      gone.insert(list[k - own]);
    }
    bool initial = falsified && out.check.falsifiedBy == "IDX-init";
    if (initial || !falsified)
      for (Term atom : gone)
        refuted.insert({cut, atom});
    // An initial state the samples missed: run from it, and derive the candidates again.
    if (initial && !out.check.initialParameters.empty() &&
        out.refinements < options.maxRefinements &&
        !llvm::is_contained(points, out.check.initialParameters)) {
      points.push_back(out.check.initialParameters);
      sample();
      ++out.refinements;
      ++level.refinements;
      IndexInvariantLevel again;
      candidates = offer(degree, kept, again);
      for (const auto &[at, atoms] : kept)
        candidates[at].insert(candidates[at].begin(), atoms.begin(), atoms.end());
      continue;
    }
    list.erase(std::remove_if(list.begin(), list.end(), [&](Term atom) { return gone.count(atom); }),
               list.end());
  }
}

/// The proved atoms as invariants of a copy of the termination certificate, which the
/// certificate checker must accept before anything is handed on.
void IndexSearch::strengthen(const Candidates &candidates) {
  std::map<unsigned, Term> extra;
  for (const auto &[cut, list] : candidates) {
    if (list.empty())
      continue;
    Term all = terms.boolean(true);
    for (Term atom : list)
      all = terms.logicalAnd(all, atom);
    extra[cut] = all;
  }
  if (extra.empty())
    return;
  TerminationCertificate copy = strengthenCertificate(program, base, extra);
  CheckOptions check;
  check.scope = options.check.scope;
  check.limits = options.termination;
  check.authorized = options.authorized;
  CheckOutcome outcome = checkCertificate(program, copy, check, backend);
  out.certificateVerdict = stringifyTerminationVerdict(outcome.verdict.kind());
  bool accepted = outcome.verdict.kind() == TerminationVerdict::ProvenTerminating ||
                  outcome.verdict.kind() == TerminationVerdict::ProvenTerminatingIfDefined;
  if (accepted) {
    out.strengthened = std::move(copy);
    out.certificateNote =
        "the certificate checker accepts the certificate with these invariants (O3 and O4 "
        "asked again there): " + outcome.verdict.reason();
  } else {
    out.certificateNote = "the certificate checker does not accept the certificate with these "
                          "invariants, so the given one is handed on: " + outcome.verdict.reason();
  }
}

void IndexSearch::run(const IndexInvariantCertificate *given) {
  out.cuts.assign(base.cuts.begin(), base.cuts.end());
  if (!inherit()) {
    out.status = "UNKNOWN";
    return;
  }
  for (unsigned cut : out.cuts)
    out.inherited[cut] = inheritedAtom.count(cut) ? 1 : 0;
  Candidates candidates;
  if (given) {
    out.given = true;
    for (const CutInvariant &entry : given->cuts)
      candidates[entry.cut].insert(candidates[entry.cut].end(), entry.atoms.begin(),
                                   entry.atoms.end());
    // A given cut that is no cut of the certificate reaches the checker as it is, and
    // the checker refuses it.
    out.certificate = assemble(candidates, given->origin.empty() ? "given" : given->origin);
    for (const CutInvariant &entry : given->cuts)
      if (!llvm::is_contained(out.cuts, entry.cut))
        out.certificate.cuts.push_back(entry);
    IndexProofOptions check = options.check;
    check.failFast = false;
    out.check = checkIndexProof(program, out.cuts, out.certificate, {}, nullptr, check, backend);
    ++out.checks;
    out.queries += out.check.queries;
  } else {
    bool exhaustive = false;
    points = loopPoints(program, options.check.scope, options.samplePoints, exhaustive);
    sample();
    if (!out.samples.runs) {
      out.status = "UNKNOWN";
      out.reason = "no sampled point of the scope could be run";
      return;
    }
    Candidates kept;
    for (unsigned degree : options.degrees) {
      IndexInvariantLevel level;
      level.degree = degree;
      candidates = offer(degree, kept, level);
      level.offered = count(candidates);
      for (const auto &[cut, atoms] : kept)
        candidates[cut].insert(candidates[cut].begin(), atoms.begin(), atoms.end());
      bool inductive = houdini(candidates, kept, degree, level);
      level.kept = inductive ? count(candidates) - count(kept) : 0;
      level.outcome = inductive ? "inductive with " + std::to_string(count(candidates)) +
                                      " atom(s) beyond the termination certificate's"
                                : out.reason;
      out.levels.push_back(level);
      if (!inductive)
        return;
      kept = candidates;
    }
    candidates = kept;
  }
  switch (out.check.invariant) {
  case IndexInvariantVerdict::Inductive:
    if (!count(candidates)) {
      out.status = "NOTHING_FOUND";
      out.reason = given ? "the given certificate holds no atom"
                         : "no candidate beyond the termination certificate's own invariants "
                           "survived IDX-init and IDX-step";
      return;
    }
    out.status = "INDUCTIVE";
    out.reason = out.check.invariantReason;
    strengthen(candidates);
    return;
  case IndexInvariantVerdict::NotInductive:
    out.status = "NOT_INDUCTIVE";
    out.reason = out.check.invariantReason;
    return;
  case IndexInvariantVerdict::Unknown:
    out.status = "UNKNOWN";
    out.reason = out.check.invariantReason;
    return;
  }
}

/// The cut each site is reached from, by the first arrival that starts at a cut.
std::map<unsigned, unsigned> startCuts(const SemanticProgram &program, ArrayRef<unsigned> cuts,
                                       ArrayRef<unsigned> sites) {
  std::map<unsigned, unsigned> out;
  TermContext &terms = *program.terms;
  SiteArrivals arrivals = reachSites(program, {}, cuts, sites, terms.boolean(true), PathLimits());
  if (!arrivals.complete)
    return out;
  for (const SiteArrival &arrival : arrivals.arrivals) {
    unsigned start = arrival.path.transitions.empty()
                         ? arrival.location
                         : program.transitions[arrival.path.transitions.front()].from;
    if (llvm::is_contained(cuts, start))
      out.try_emplace(arrival.site, start);
  }
  return out;
}

json::Array parametersJSON(const std::vector<APInt> &values) {
  json::Array out;
  for (const APInt &value : values)
    out.push_back(decimal(value));
  return out;
}

json::Object obligationCounts(ArrayRef<ObligationRecord> records, bool scripts, json::Array *list) {
  std::map<std::string, std::map<std::string, int64_t>> counts;
  for (const ObligationRecord &record : records) {
    // "IDX-step[atom 3]" counts with IDX-step.
    std::string id = record.id.substr(0, record.id.find('['));
    ++counts[id][record.result];
    if (list) {
      json::Object entry{{"id", record.id},
                         {"location", record.location},
                         {"result", record.result},
                         {"discharged", record.discharged}};
      if (!record.evidence.empty())
        entry["evidence"] = record.evidence;
      if (scripts && !record.script.empty())
        entry["script"] = record.script;
      list->push_back(std::move(entry));
    }
  }
  json::Object out;
  for (const auto &[id, results] : counts) {
    json::Object entry;
    for (const auto &[result, n] : results)
      entry[result] = n;
    out[id] = std::move(entry);
  }
  return out;
}

} // namespace

IndexInvariantSearch findIndexInvariants(const SemanticProgram &program,
                                         const TerminationCertificate *certificate,
                                         const IndexInvariantCertificate *given,
                                         const IndexInvariantSearchOptions &options,
                                         SmtBackend &backend) {
  IndexInvariantSearch out;
  if (!program.isSupported()) {
    out.status = "UNSUPPORTED";
    out.reason = "the program is unsupported";
    return out;
  }
  if (!certificate) {
    out.reason = "no termination certificate: no cut set to state invariants at, and no "
                 "checker to hand them to";
    return out;
  }
  try {
    IndexSearch(program, *certificate, options, backend, out).run(given);
  } catch (const TermError &error) {
    out.status = "UNKNOWN";
    out.reason = std::string("term limit: ") + error.what();
    out.strengthened.reset();
  }
  return out;
}

std::vector<IndexRelation> proposeIndexRelations(const SemanticProgram &program,
                                                 const IndexInvariantSearch &search,
                                                 const SiteGroup &group,
                                                 const CoordinateFamily &family) {
  std::vector<IndexRelation> out;
  if (search.check.invariant != IndexInvariantVerdict::Inductive)
    return out;
  TermContext &terms = *program.terms;
  std::vector<unsigned> sites(group.sites.begin(), group.sites.end());
  std::map<unsigned, unsigned> from = startCuts(program, search.cuts, sites);
  static const unsigned axes[3][3] = {
      {AxisP, AxisI, AxisK}, {AxisP, AxisK, AxisJ}, {AxisP, AxisI, AxisJ}};
  for (unsigned id : sites) {
    if (id >= program.sites.size())
      continue;
    const MacSite &site = program.sites[id];
    Polynomials polys(terms);
    std::vector<Poly> known;
    std::string where = "the subscript itself";
    auto start = from.find(id);
    if (start != from.end())
      for (const CutInvariant &entry : search.certificate.cuts)
        if (entry.cut == start->second) {
          known = equalities(polys, entry.atoms);
          where = "the subscript rewritten by the equalities proved at L" + std::to_string(entry.cut);
        }
    for (unsigned operand = 0; operand < 3; ++operand) {
      const MacAccess *access =
          operand == 0 ? &site.a : operand == 1 ? &site.b : site.output ? &*site.output : nullptr;
      if (!access)
        continue;
      std::set<unsigned> allowed;
      for (unsigned axis : axes[operand])
        for (Term term : family.axes[axis])
          for (unsigned v : stateSupport(term))
            allowed.insert(v);
      for (unsigned d = 0; d < access->index.size(); ++d) {
        Term index = access->index[d];
        if (readsMemory(index) || containsFloat(index))
          continue;
        // The subscript with each variable no coordinate reads replaced by its solution
        // from a proved equality, under casts as well, until only coordinates are read.
        Term current = index;
        std::set<unsigned> eliminated;
        bool rewritten = false, stuck = false;
        for (unsigned round = 0; round < 32 && !stuck; ++round) {
          std::vector<unsigned> foreign;
          for (unsigned v : stateSupport(current))
            if (!allowed.count(v))
              foreign.push_back(v);
          if (foreign.empty())
            break;
          stuck = true;
          for (unsigned v : foreign) {
            for (const Poly &difference : known) {
              auto solution = polys.solve(difference, v);
              if (!solution)
                continue;
              // A solution that brings back a variable already replaced would loop.
              std::set<unsigned> brings = polys.support(*solution);
              if (llvm::any_of(brings, [&](unsigned b) { return eliminated.count(b); }))
                continue;
              Term replacement = polys.toTerm(*solution);
              current = terms.substitute(current, [&](Term leaf) -> Term {
                return leaf->op == TermOp::State && leaf->index == v ? replacement : nullptr;
              });
              eliminated.insert(v);
              rewritten = true;
              stuck = false;
              break;
            }
            if (!stuck)
              break;
          }
        }
        if (stuck || llvm::any_of(stateSupport(current), [&](unsigned v) { return !allowed.count(v); }))
          continue; // No relation over these coordinates is proposed for this subscript.
        if (rewritten)
          if (auto normal = polys.from(current))
            current = polys.toTerm(*normal);
        IndexRelation relation;
        relation.site = id;
        relation.operand = operand;
        relation.dimension = d;
        relation.relation = rewritten ? current : index;
        relation.origin = rewritten ? where : "the subscript itself, which reads only the coordinates";
        out.push_back(std::move(relation));
      }
    }
  }
  return out;
}

std::vector<IndexIdentity> indexIdentities(const SemanticProgram &program,
                                           const IndexInvariantSearch &search) {
  std::vector<IndexIdentity> out;
  if (search.check.invariant != IndexInvariantVerdict::Inductive)
    return out;
  TermContext &terms = *program.terms;
  for (const CutInvariant &entry : search.certificate.cuts) {
    unsigned own = 0;
    auto inherited = search.inherited.find(entry.cut);
    if (inherited != search.inherited.end())
      own = inherited->second;
    std::set<unsigned> taken;
    for (unsigned k = own; k < entry.atoms.size(); ++k) {
      Polynomials polys(terms);
      std::vector<Poly> one = equalities(polys, entry.atoms[k]);
      for (const Poly &difference : one) {
        std::set<unsigned> reads = polys.support(difference);
        for (auto v = reads.rbegin(); v != reads.rend(); ++v) {
          if (taken.count(*v))
            continue;
          auto solution = polys.solve(difference, *v);
          if (!solution)
            continue;
          taken.insert(*v);
          IndexIdentity identity;
          identity.state = *v;
          identity.cut = entry.cut;
          identity.term = polys.toTerm(*solution);
          identity.origin = "index invariant " + formatLoopTerm(entry.atoms[k]) + " at L" +
                            std::to_string(entry.cut) + ", solved for s" + std::to_string(*v);
          out.push_back(std::move(identity));
          break;
        }
      }
    }
  }
  return out;
}

std::optional<IndexInvariantCertificate> parseIndexInvariants(const SemanticProgram &program,
                                                              const std::string &text,
                                                              std::string &why) {
  IndexInvariantCertificate out;
  out.origin = "given";
  SmallVector<StringRef> items;
  StringRef(text).split(items, ';', -1, /*KeepEmpty=*/false);
  for (StringRef item : items) {
    auto [where, body] = item.split(':');
    CutInvariant entry;
    if (where.trim().getAsInteger(10, entry.cut)) {
      why = "'" + item.str() + "': expected cut:conjunction";
      return std::nullopt;
    }
    std::string reason;
    Term inv = parseLoopTerm(program, body.str(), reason);
    if (!inv) {
      why = "'" + item.str() + "': " + reason;
      return std::nullopt;
    }
    entry.atoms = conjuncts(inv);
    out.cuts.push_back(std::move(entry));
  }
  return out;
}

std::optional<std::vector<IndexRelation>> parseIndexRelations(const SemanticProgram &program,
                                                              const std::string &text,
                                                              std::string &why) {
  std::vector<IndexRelation> out;
  SmallVector<StringRef> items;
  StringRef(text).split(items, ';', -1, /*KeepEmpty=*/false);
  for (StringRef item : items) {
    SmallVector<StringRef> parts;
    item.split(parts, ':', 3);
    IndexRelation relation;
    relation.origin = "given";
    StringRef name = parts.size() == 4 ? parts[1].trim() : StringRef();
    relation.operand = name == "A" ? 0 : name == "B" ? 1 : name == "C" ? 2 : 3;
    if (parts.size() != 4 || parts[0].trim().getAsInteger(10, relation.site) ||
        parts[2].trim().getAsInteger(10, relation.dimension) || relation.operand > 2) {
      why = "'" + item.str() + "': expected site:A|B|C:dimension:term";
      return std::nullopt;
    }
    std::string reason;
    relation.relation = parseLoopTerm(program, parts[3].str(), reason);
    if (!relation.relation) {
      why = "'" + item.str() + "': " + reason;
      return std::nullopt;
    }
    out.push_back(std::move(relation));
  }
  return out;
}

std::vector<IndexCandidateCheck> checkIndexCandidates(const SemanticProgram &program,
                                                      const TerminationCertificate *certificate,
                                                      const IndexInvariantSearch &search,
                                                      const SynthesisResult &result,
                                                      ArrayRef<IndexRelation> extra,
                                                      const IndexInvariantSearchOptions &options,
                                                      const SymbolicCheckOptions &symbolic,
                                                      SmtBackend &backend) {
  std::vector<IndexCandidateCheck> out;
  if (search.check.invariant != IndexInvariantVerdict::Inductive)
    return out;
  IndexProofOptions check = options.check;
  check.failFast = false;
  for (const GroupSynthesis &group : result.groups) {
    if (group.group >= result.enumeration.groups.size())
      continue;
    const SiteGroup &sites = result.enumeration.groups[group.group];
    for (unsigned c = 0; c < group.candidates.size(); ++c) {
      const VerifiedCandidate &candidate = group.candidates[c];
      IndexCandidateCheck entry;
      entry.group = group.group;
      entry.candidate = c;
      std::vector<IndexRelation> claims =
          proposeIndexRelations(program, search, sites, candidate.family);
      for (const IndexRelation &given : extra)
        if (llvm::is_contained(sites.sites, given.site))
          claims.push_back(given);
      entry.proof = checkIndexProof(program, search.cuts, search.certificate, claims,
                                    &candidate.family, check, backend);
      if (certificate && search.strengthened) {
        entry.without = checkFamilySymbolic(program, certificate, sites, candidate.family,
                                            candidate.order ? &*candidate.order : nullptr, symbolic,
                                            backend);
        entry.comparison = true;
      }
      out.push_back(std::move(entry));
    }
  }
  return out;
}

std::string indexInvariantsJSON(const SemanticProgram &program, const IndexInvariantSearch &search,
                                ArrayRef<IndexCandidateCheck> candidates,
                                const SynthesisResult *result,
                                const IndexInvariantSearchOptions &options,
                                ArrayRef<IndexIdentity> identities) {
  json::Object out{{"status", search.status},
                   {"reason", search.reason},
                   // A given certificate is a debugging comparison: it is never counted as
                   // found by the search.
                   {"automatic", !search.given},
                   {"origin", search.certificate.origin}};
  json::Array cuts;
  size_t added = 0;
  for (unsigned cut : search.cuts) {
    json::Object entry{{"cut", (int64_t)cut}};
    unsigned own = 0;
    auto inherited = search.inherited.find(cut);
    if (inherited != search.inherited.end())
      own = inherited->second;
    json::Array atoms;
    for (const CutInvariant &listed : search.certificate.cuts) {
      if (listed.cut != cut)
        continue;
      for (unsigned k = 0; k < listed.atoms.size(); ++k) {
        if (k < own)
          entry["termination_certificate_invariant"] = formatLoopTerm(listed.atoms[k]);
        else
          atoms.push_back(formatLoopTerm(listed.atoms[k]));
      }
    }
    added += atoms.size();
    entry["atoms"] = std::move(atoms);
    for (const CutSamples::Cut &sampled : search.samples.cuts)
      if (sampled.cut == cut) {
        json::Array variables;
        for (unsigned v : sampled.variables)
          variables.push_back("s" + std::to_string(v));
        entry["sampled_variables"] = std::move(variables);
        entry["sampled_rows"] = (int64_t)sampled.rows.size();
        entry["sampled_arrivals"] = (int64_t)sampled.arrivals;
      }
    cuts.push_back(std::move(entry));
  }
  // Cuts a given certificate names that the termination certificate does not have.
  for (const CutInvariant &listed : search.certificate.cuts)
    if (!llvm::is_contained(search.cuts, listed.cut)) {
      json::Array atoms;
      for (Term atom : listed.atoms)
        atoms.push_back(formatLoopTerm(atom));
      cuts.push_back(json::Object{{"cut", (int64_t)listed.cut},
                                  {"atoms", std::move(atoms)},
                                  {"note", "not a cut of the termination certificate"}});
    }
  out["cuts"] = std::move(cuts);
  out["atoms_beyond_the_termination_certificate"] = (int64_t)added;
  // What each name in the atoms and relations stands for, so that a reader (or an
  // oracle with its own interpreter) can evaluate them against the source.
  json::Array variables;
  for (const StateVar &var : program.state) {
    if (var.sort.kind != Sort::BV)
      continue;
    std::string where =
        var.kind == StateVar::BlockArgument
            ? "argument " + std::to_string(var.position) + " of block " + std::to_string(var.location)
        : var.kind == StateVar::IterationCounter
            ? "iteration counter of block " + std::to_string(var.location)
            : "result of op " + std::to_string(var.position) + " of block " +
                  std::to_string(var.location);
    variables.push_back(json::Object{
        {"name", "s" + std::to_string(var.id)}, {"sort", var.sort.str()}, {"is", where}});
  }
  out["variables"] = std::move(variables);
  json::Array parameters;
  for (const Parameter &parameter : program.parameters)
    parameters.push_back(json::Object{
        {"name", "p" + std::to_string(parameter.id)},
        {"sort", parameter.sort.str()},
        {"is", parameter.kind == Parameter::Scalar
                   ? "function argument " + std::to_string(parameter.argument)
                   : "extent " + std::to_string(parameter.dimension) + " of function argument " +
                         std::to_string(parameter.argument)}});
  out["parameters"] = std::move(parameters);

  json::Array records;
  json::Object counts = obligationCounts(search.check.records, options.check.scripts, &records);
  bool inductive = search.check.invariant == IndexInvariantVerdict::Inductive;
  json::Object proof{
      {"invariant", stringifyIndexInvariantVerdict(search.check.invariant)},
      {"reason", search.check.invariantReason},
      {"queries", (int64_t)search.check.queries},
      {"script_bytes", (int64_t)search.check.scriptBytes},
      {"obligations", std::move(counts)},
      {"records", std::move(records)},
      // IDX-init and IDX-step ask one query per path between cuts and run the program
      // nowhere: this is the checker's own count of runs, which only IDX-use models
      // (a candidate's relations, below) ever cause.
      {"program_runs", (int64_t)search.check.confirmationRuns},
      {"loop_unrolling",
       inductive ? "none: IDX-init and IDX-step are one query per path between cuts, whatever "
                   "the trip counts"
                 : "not applicable: nothing was proved"}};
  if (!search.check.falsifiedBy.empty()) {
    // The atoms the model makes false, as text: positions count the termination
    // certificate's own invariant, which is the first atom of its cut.
    json::Array falsified;
    for (const CutInvariant &listed : search.certificate.cuts)
      if (listed.cut == search.check.falsifiedCut)
        for (unsigned k : search.check.falsified)
          if (k < listed.atoms.size())
            falsified.push_back(formatLoopTerm(listed.atoms[k]));
    proof["falsified"] = json::Object{{"obligation", search.check.falsifiedBy},
                                      {"cut", (int64_t)search.check.falsifiedCut},
                                      {"atoms", std::move(falsified)},
                                      {"model", search.check.model},
                                      // A model of IDX-step may sit at a state no run reaches.
                                      {"refutes", "the invariant candidate, not the program"}};
  }
  out["proof"] = std::move(proof);

  json::Array levels;
  for (const IndexInvariantLevel &level : search.levels)
    levels.push_back(json::Object{{"degree", (int64_t)level.degree},
                                  {"offered", (int64_t)level.offered},
                                  {"kept", (int64_t)level.kept},
                                  {"columns", (int64_t)level.columns},
                                  {"rows", (int64_t)level.rows},
                                  {"rank", (int64_t)level.rank},
                                  {"basis", (int64_t)level.basis},
                                  {"skipped_large_coefficients", (int64_t)level.skipped},
                                  {"folded_constants", (int64_t)level.folded},
                                  {"checks", (int64_t)level.checks},
                                  {"refinements", (int64_t)level.refinements},
                                  {"outcome", level.outcome}});
  out["search"] = json::Object{
      // Runs that only choose candidates, counted apart from the proof.
      {"sample_points", (int64_t)search.samplePoints},
      {"sample_runs", (int64_t)search.samples.runs},
      {"sample_runs_returned", (int64_t)search.samples.returned},
      {"sample_runs_stopped", (int64_t)search.samples.stopped},
      {"sample_steps", (int64_t)search.samples.steps},
      {"levels", std::move(levels)},
      {"checks", (int64_t)search.checks},
      {"queries", (int64_t)search.queries},
      {"refinements", (int64_t)search.refinements},
      {"template",
       search.given
           ? "none: the certificate was given"
           : "per cut: v == c for a variable constant at the samples, a basis of the null space "
             "over Q of the monomials of the stated degree over the remaining integer state "
             "and the parameters, and not-poison atoms; Houdini over all cuts"}};
  out["certificate"] = json::Object{{"strengthened", search.strengthened.has_value()},
                                    {"checker_verdict", search.certificateVerdict},
                                    {"note", search.certificateNote}};

  json::Array list;
  for (const IndexCandidateCheck &entry : candidates) {
    json::Object item{{"group", (int64_t)entry.group}, {"candidate", (int64_t)entry.candidate}};
    json::Array relations;
    for (const CheckedIndexRelation &checked : entry.proof.relations) {
      const MacSite &site = program.sites[checked.claim.site];
      const MacAccess *access = checked.claim.operand == 0   ? &site.a
                                : checked.claim.operand == 1 ? &site.b
                                : site.output                ? &*site.output
                                                             : nullptr;
      json::Object relation{
          {"site", (int64_t)checked.claim.site},
          {"operand", checked.claim.operand == 0 ? "A" : checked.claim.operand == 1 ? "B" : "C"},
          {"dimension", (int64_t)checked.claim.dimension},
          {"relation", checked.claim.relation ? formatLoopTerm(checked.claim.relation) : ""},
          {"origin", checked.claim.origin},
          {"status", stringifyIndexRelationVerdict(checked.verdict)},
          {"reason", checked.reason},
          {"arrivals", (int64_t)checked.arrivals}};
      if (access && checked.claim.dimension < access->index.size() &&
          !readsMemory(access->index[checked.claim.dimension]))
        relation["subscript"] = formatLoopTerm(access->index[checked.claim.dimension]);
      if (checked.overCoordinates)
        relation["over_coordinates"] = *checked.overCoordinates;
      if (checked.witness)
        relation["witness"] = json::Object{
            {"parameters", parametersJSON(checked.witness->parameters)},
            {"event", (int64_t)checked.witness->event},
            {"subscript_in_the_run", decimal(checked.witness->index)},
            {"relation_value", decimal(checked.witness->claimed)},
            {"steps", (int64_t)checked.witness->steps},
            {"refutes", "the relation, by a run of the program; not the program"}};
      if (!checked.unconfirmedModel.empty())
        relation["unconfirmed_model"] = checked.unconfirmedModel;
      relations.push_back(std::move(relation));
    }
    item["relations"] = std::move(relations);
    json::Array useRecords;
    std::vector<ObligationRecord> uses;
    for (const ObligationRecord &record : entry.proof.records)
      if (record.id == "IDX-use")
        uses.push_back(record);
    item["use_obligations"] = obligationCounts(uses, options.check.scripts, &useRecords);
    item["use_records"] = std::move(useRecords);
    item["confirmation_runs"] = (int64_t)entry.proof.confirmationRuns;
    // What the GEMM index checker says for this family: with the strengthened
    // certificate (the candidate's own record) and with the one it was given before.
    auto dependence = [](const CheckedFamily &family) {
      json::Object object{{"tier", stringifyEvidenceTier(family.tier())},
                          {"status", stringifyFamilyStatus(family.status())},
                          {"reason", family.reason()}};
      for (const char *id : {"V1h", "V1f", "V1g"}) {
        std::string result = "not asked";
        bool every = true, any = false;
        for (const ObligationRecord &record : family.records())
          if (record.id == id) {
            any = true;
            every = every && record.discharged;
            if (!record.discharged)
              result = record.result;
          }
        if (any && every)
          result = "unsat";
        object[id] = result;
      }
      return object;
    };
    if (result && entry.group < result->groups.size() &&
        entry.candidate < result->groups[entry.group].candidates.size())
      item["index_checker"] =
          dependence(result->groups[entry.group].candidates[entry.candidate].symbolic);
    if (entry.comparison)
      item["index_checker_without_the_invariants"] = dependence(entry.without);
    list.push_back(std::move(item));
  }
  out["candidates"] = std::move(list);
  if (!identities.empty()) {
    // What a proposer was handed: proposals, checked by whoever uses them.
    json::Array handed;
    for (const IndexIdentity &identity : identities)
      handed.push_back(json::Object{{"state", (int64_t)identity.state},
                                    {"cut", (int64_t)identity.cut},
                                    {"term", formatLoopTerm(identity.term)},
                                    {"origin", identity.origin}});
    out["identities_for_the_proposer"] = std::move(handed);
  }

  std::string text;
  raw_string_ostream stream(text);
  stream << json::Value(std::move(out));
  return text;
}

} // namespace mlir::generalized_gemm
