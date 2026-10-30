#include "PolynomialTerminationInternal.h"
#include "llvm/Support/JSON.h"
#include <z3++.h>
#include <functional>
#include <cctype>
#include <set>

namespace systolic::polynomial::termination {
namespace detail {
namespace {
std::string bv(Integer n, unsigned width) {
  Integer modulus = Integer(1) << width;
  n %= modulus; if (n < 0) n += modulus;
  return "(_ bv" + n.convert_to<std::string>() + " " + std::to_string(width) + ")";
}
std::string conjunction(const std::vector<std::string> &terms) {
  if (terms.empty()) return "true";
  std::string s = "(and"; for (auto &t : terms) s += " " + t; return s + ")";
}
std::string state(unsigned i, const std::string &prefix = "s") { return prefix + std::to_string(i); }
std::string encode(const Expression &e, unsigned width, unsigned maxBytes) {
  std::map<const Expression::Node *, std::string> memo;
  std::function<std::string(const Expression::Node *)> visit = [&](const auto *n) -> std::string {
    if (auto it = memo.find(n); it != memo.end()) return it->second;
    std::string result;
    using K = Expression::Kind;
    if (n->kind == K::Constant) {
      if (n->constant.denominator() != 1) throw Rejection("nonintegral source expression");
      result = bv(n->constant.numerator(), width);
    } else if (n->kind == K::Variable) result = n->name;
    else {
      const char *op = n->kind == K::Add ? "bvadd" : n->kind == K::Subtract ? "bvsub" : n->kind == K::Multiply ? "bvmul" : nullptr;
      if (!op) throw Rejection("unsupported SMT source expression");
      result = "(" + std::string(op) + " " + visit(n->left.get()) + " " + visit(n->right.get()) + ")";
    }
    if (result.size() > maxBytes) throw Rejection("SMT expression byte resource limit");
    memo.emplace(n, result); return result;
  };
  return visit(e.root().get());
}
struct Encoding {
  const Model &m;
  const Certificate &c;
  const Limits &limits;
  std::string base, domain;
  Encoding(const Model &model, const Certificate &cert, const Limits &l) : m(model), c(cert), limits(l) {
    base = "; Source SHA256: " + m.fingerprint + "\n(set-logic QF_BV)\n";
    base += "(set-option :timeout " + std::to_string(l.timeoutMilliseconds) + ")\n";
    base += "(set-option :rlimit " + std::to_string(l.solverResourceLimit) + ")\n";
    std::string type = "(_ BitVec " + std::to_string(m.width) + ")";
    for (const auto &p : m.domain) base += "(declare-const p" + std::to_string(p.argument) + " " + type + ")\n";
    for (unsigned i = 0; i < m.initial.size(); ++i) base += "(declare-const " + state(i) + " " + type + ")\n";
    for (unsigned i = 0; i < m.initial.size(); ++i) {
      base += "(define-fun " + state(i,"e") + " () " + type + " " + encode(m.initial[i],m.width,l.maxSMTBytes) + ")\n";
      base += "(define-fun " + state(i,"n") + " () " + type + " " + encode(m.next[i],m.width,l.maxSMTBytes) + ")\n";
    }
    std::vector<std::string> ranges;
    for (const auto &p : m.domain) {
      auto name = "p"+std::to_string(p.argument);
      ranges.push_back("(bvsge "+name+" "+bv(p.lower,m.width)+")");
      ranges.push_back("(bvsle "+name+" "+bv(p.upper,m.width)+")");
    }
    domain = conjunction(ranges);
    base += "(define-fun D () Bool " + domain + ")\n";
    // A parallel SMT let implements simultaneous state substitution. No next
    // value is substituted into another next value's right-hand side.
    for (auto prefix : {std::string("s"), std::string("e"), std::string("n")}) {
      base += "(define-fun G_"+prefix+" () Bool "+at(guard(),prefix)+")\n";
      base += "(define-fun I_"+prefix+" () Bool "+at(invariant(),prefix)+")\n";
      if (c.kind == CertificateKind::LinearRanking)
        base += "(define-fun R_"+prefix+" () (_ BitVec "+std::to_string(m.width+2)+") "+at(rank(),prefix)+")\n";
    }
    if (base.size() > l.maxSMTBytes) throw Rejection("SMT model byte resource limit");
  }
  std::string at(const std::string &text, const std::string &prefix) const {
    if (prefix == "s") return text;
    std::string out = "(let (";
    for (unsigned i = 0; i < m.initial.size(); ++i) out += "("+state(i)+" "+state(i,prefix)+")";
    return out+") "+text+")";
  }
  std::string guard() const {
    if (m.constantGuard) return *m.constantGuard ? "true" : "false";
    return "(bv"+std::string(predicateName(m.predicate))+" "+encode(m.lhs,m.width,limits.maxSMTBytes)+" "+encode(m.rhs,m.width,limits.maxSMTBytes)+")";
  }
  std::string invariant() const {
    std::vector<std::string> terms;
    for (auto &i : c.invariant) {
      terms.push_back("(bvsge "+state(i.state)+" "+bv(i.lower,m.width)+")");
      terms.push_back("(bvsle "+state(i.state)+" "+bv(i.upper,m.width)+")");
    }
    return conjunction(terms);
  }
  std::string rank() const {
    std::string extension = c.signedValues ? "sign_extend" : "zero_extend";
    std::string counter = "((_ "+extension+" 2) "+state(c.counter)+")";
    std::string bound = "((_ "+extension+" 2) "+encode(m.rhs,m.width,limits.maxSMTBytes)+")";
    std::string gap = c.ascending ? "(bvsub "+bound+" "+counter+")" : "(bvsub "+counter+" "+bound+")";
    // w+2 bits represent the full mathematical difference of two signed OR
    // unsigned w-bit values, plus offset -1..1. Signed rank comparisons cannot
    // wrap. In particular, an exiting successor rank is allowed to be negative.
    return "(bvadd "+gap+" "+bv(c.offset,m.width+2)+")";
  }
};
void validateCertificate(const Model &m, const Certificate &c) {
  if (c.sourceFingerprint != m.fingerprint || c.guard != m.guardText || c.domain != m.domain)
    throw Rejection("certificate source, guard, or domain mismatch");
  Integer half = Integer(1) << (m.width-1);
  std::set<unsigned> states;
  for (auto &i : c.invariant)
    if (i.state >= m.initial.size() || !states.insert(i.state).second || i.lower > i.upper || i.lower < -half || i.upper >= half)
      throw Rejection("invalid certificate invariant");
  switch (c.kind) {
  case CertificateKind::LinearRanking:
    if (!m.counter || c.counter != *m.counter || !m.step || *m.step == 0 ||
        c.signedValues != signedPredicate(m.predicate) || c.offset < -1 || c.offset > 1)
      throw Rejection("unsupported ranking certificate");
    break;
  case CertificateKind::ZeroIterations: case CertificateKind::RecurrentGuard: break;
  default: throw Rejection("invalid certificate kind");
  }
}
} // namespace

Obligation solve(const std::string &name, const std::string &smtlib,
                 const std::string &expected, const Limits &limits) {
  Obligation out{name,smtlib,expected,"error",""};
  try {
    if (smtlib.size() > limits.maxSMTBytes) throw Rejection("SMT query byte resource limit");
    // Z3's script reader warns and ignores unknown top-level commands. Never
    // turn such a warning into a successful proof of a different script.
    // This deliberately accepts only the command vocabulary we emit.
    unsigned depth = 0;
    const std::set<std::string> commands{"set-logic", "set-option", "declare-const",
      "define-fun", "assert", "check-sat"};
    for (size_t i = 0; i < smtlib.size(); ++i) {
      char ch = smtlib[i];
      if (ch == ';') { while (i < smtlib.size() && smtlib[i] != '\n') ++i; continue; }
      if (ch == '"' || ch == '|') throw Rejection("unsupported quoted SMT syntax");
      if (ch == '(') {
        if (depth++ == 0) {
          size_t begin = i + 1;
          while (begin < smtlib.size() && std::isspace(static_cast<unsigned char>(smtlib[begin]))) ++begin;
          size_t end = begin;
          while (end < smtlib.size() && !std::isspace(static_cast<unsigned char>(smtlib[end])) && smtlib[end] != ')' && smtlib[end] != '(') ++end;
          if (!commands.count(smtlib.substr(begin, end - begin))) throw Rejection("unsupported SMT command");
        }
      } else if (ch == ')') {
        if (!depth) throw Rejection("unbalanced SMT syntax");
        --depth;
      } else if (!depth && !std::isspace(static_cast<unsigned char>(ch))) throw Rejection("unsupported SMT syntax");
    }
    if (depth) throw Rejection("unbalanced SMT syntax");
    z3::context context;
    z3::solver solver(context, "QF_BV");
    solver.set("timeout", limits.timeoutMilliseconds);
    solver.set("rlimit", limits.solverResourceLimit);
    solver.set("random_seed", 0u);
    solver.from_string(smtlib.c_str());
    auto result = solver.check();
    if (result == z3::sat) out.outcome = "sat";
    else if (result == z3::unsat) out.outcome = "unsat";
    else { out.outcome = "unknown"; out.detail = solver.reason_unknown(); }
  } catch (const std::exception &e) { out.detail = e.what(); }
  return out;
}
Result verify(mlir::func::FuncOp function, const Certificate &c, const Options &options, unsigned &queryCount) {
  Result result; result.options = options; result.solverVersion = Z3_get_full_version();
  try {
    Model m = extract(function, options); // reconstruction, never supplied E/G/T
    result.sourceFingerprint = m.fingerprint; result.bitWidth = m.width;
    result.domain = m.domain; result.guard = m.guardText;
    for (auto &e : m.initial) result.initial.push_back(cfg::formatExpression(e));
    for (auto &e : m.next) result.transition.push_back(cfg::formatExpression(e));
    // Constructive totality: every original reachable operation was accepted
    // as a total unflagged operation, all branches were verified, P2a proved
    // single-path coverage, and every header argument has exactly one next
    // expression. T is the graph of this total function, not a partial filter.
    result.definedTotalPathVerified = true;
    validateCertificate(m,c);
    Encoding e(m,c,options.limits);
    auto check = [&](const std::string &name, const std::string &bad, const std::string &expected = "unsat") {
      if (++queryCount > options.limits.maxQueries) throw Rejection("solver query resource limit");
      std::string script = e.base + "\n; Obligation: "+name+"\n(assert "+bad+")\n(check-sat)\n";
      auto q = solve(name,script,expected,options.limits);
      bool passed = q.outcome == expected;
      auto why = q.outcome + (q.detail.empty() ? "" : ": " + q.detail);
      result.obligations.push_back(std::move(q));
      if (!passed) throw Rejection(name + " not established (" + why + ")");
    };
    check("nonempty_domain", "D", "sat");
    // One UNSAT query checks the disjunction of independent violations. It
    // proves each obligation; it is not a sampled equality or an assumption.
    std::string disagreement = "(or false";
    for (unsigned i = 0; i < m.initial.size(); ++i) {
      disagreement += " (distinct e"+std::to_string(i)+" "+encode(m.recurrenceInitial[i],m.width,options.limits.maxSMTBytes)+")";
      disagreement += " (distinct n"+std::to_string(i)+" "+encode(m.recurrenceNext[i],m.width,options.limits.maxSMTBytes)+")";
    }
    disagreement += ")";
    check("source_agreement_initial_and_inductive_invariant",
          "(and D (or "+disagreement+" (not I_e) (and I_s G_s (not I_n))))");
    if (c.kind == CertificateKind::ZeroIterations) {
      check("all_entries_exit_immediately", "(and D G_e)");
      result.status = Status::ProvenTerminating;
      result.reason = "all permitted entry guards are false; zero backedges";
    } else if (c.kind == CertificateKind::LinearRanking) {
      check("well_founded_on_continuing_states", "(and D I_s G_s (bvslt R_s "+bv(0,m.width+2)+"))");
      check("strict_descent", "(and D I_s G_s (not (bvslt R_n R_s)))");
      check("well_founded_on_continuing_successors", "(and D I_s G_s G_n (bvslt R_n "+bv(0,m.width+2)+"))");
      result.status = Status::ProvenTerminating;
      result.reason = "verified natural-valued ranking on continuing states and strict transition descent";
    } else {
      // R = I AND G. These checks are independent of ranking discovery.
      check("all_entries_in_recurrent_set", "(and D (not G_e))");
      check("recurrent_set_closed", "(and D I_s G_s (not G_n))");
      result.status = Status::ProvenNonterminating;
      result.reason = "all permitted entries lie in the defined, total, closed recurrent set I AND G";
    }
    result.certificate = c;
  } catch (const std::exception &error) {
    result.status = Status::Unknown; result.certificate.reset(); result.reason = error.what();
  }
  return result;
}
} // namespace detail

Result verifyCertificate(mlir::func::FuncOp f, const Certificate &c, const Options &options) {
  unsigned queries = 0;
  return detail::verify(f,c,options,queries);
}

std::string formatResult(const Result &r) {
  using namespace llvm;
  auto integer = [](const Integer &n) { return n.convert_to<std::string>(); };
  json::Object out;
  out["status"] = r.status == Status::ProvenTerminating ? "PROVEN_TERMINATING" : r.status == Status::ProvenNonterminating ? "PROVEN_NONTERMINATING" : "UNKNOWN";
  out["reason"] = r.reason;
  out["scope"] = "selected loop, all defined inputs in reported domain; no exact trip count, requested-visit reachability, or exit-value claim";
  out["program_semantics"] = "unflagged source-width bitvectors; signedness belongs to each comparison";
  out["source_sha256"] = r.sourceFingerprint; out["solver_version"] = r.solverVersion;
  out["bit_width"] = r.bitWidth;
  json::Object limits;
#define REPORT_LIMIT(field) limits[#field] = r.options.limits.field
  REPORT_LIMIT(maxBlocks); REPORT_LIMIT(maxOperations); REPORT_LIMIT(maxStates);
  REPORT_LIMIT(maxParameters); REPORT_LIMIT(maxNodes); REPORT_LIMIT(maxDepth);
  REPORT_LIMIT(maxCandidates); REPORT_LIMIT(maxQueries); REPORT_LIMIT(timeoutMilliseconds);
  REPORT_LIMIT(solverResourceLimit); REPORT_LIMIT(maxSMTBytes);
#undef REPORT_LIMIT
  out["limits"] = std::move(limits);
  json::Array domain, initial, next, queries;
  for (auto &p : r.domain) {
    json::Object d; d["argument"] = p.argument; d["bit_width"] = p.bitWidth;
    d["lower"] = integer(p.lower); d["upper"] = integer(p.upper); domain.push_back(std::move(d));
  }
  out["parameter_domain"] = std::move(domain);
  for (auto &e : r.initial) initial.push_back(e);
  for (auto &e : r.transition) next.push_back(e);
  json::Object model;
  model["initial"] = std::move(initial); model["transition"] = std::move(next); model["guard"] = r.guard;
  model["defined_total_path_verified"] = r.definedTotalPathVerified;
  out["model"] = std::move(model);
  out["certificate"] = nullptr;
  if (r.status != Status::Unknown && r.certificate) {
    const auto &c = *r.certificate;
    json::Object cert;
    cert["kind"] = c.kind == CertificateKind::LinearRanking ? "linear-ranking" : c.kind == CertificateKind::ZeroIterations ? "zero-iterations" : "recurrent-guard";
    cert["source_sha256"] = c.sourceFingerprint; cert["guard"] = c.guard;
    json::Array inv;
    for (auto &i : c.invariant) {
      json::Object bounds; bounds["state"] = i.state; bounds["lower"] = integer(i.lower); bounds["upper"] = integer(i.upper); inv.push_back(std::move(bounds));
    }
    cert["invariant_intervals"] = std::move(inv);
    if (c.kind == CertificateKind::LinearRanking) {
      cert["counter"] = c.counter; cert["ascending"] = c.ascending; cert["signed_values"] = c.signedValues;
      cert["offset"] = c.offset; cert["ranking_bit_width"] = r.bitWidth+2;
      cert["ranking_domain"] = "mathematical naturals on continuing states; exiting successor may be negative";
    } else if (c.kind == CertificateKind::RecurrentGuard) cert["recurrent_set"] = "I AND G";
    out["certificate"] = std::move(cert);
  }
  for (auto &q : r.obligations) {
    json::Object item; item["name"] = q.name; item["expected"] = q.expected; item["outcome"] = q.outcome;
    item["detail"] = q.detail; item["smtlib"] = q.smtlib; queries.push_back(std::move(item));
  }
  out["obligations"] = std::move(queries);
  std::string text; raw_string_ostream stream(text); stream << json::Value(std::move(out)); return text;
}
} // namespace systolic::polynomial::termination
