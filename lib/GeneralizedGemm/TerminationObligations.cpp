#include "GeneralizedGemm/TerminationObligations.h"

#include "GeneralizedGemm/RelationalEncoding.h"
#include "GeneralizedGemm/SmtBackend.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"

#include <cctype>
#include <functional>

using namespace mlir::generalized_gemm;
using llvm::APInt;

namespace mlir::generalized_gemm {

const char *stringifySmtAnswer(SmtAnswer answer) {
  switch (answer) {
  case SmtAnswer::Sat: return "sat";
  case SmtAnswer::Unsat: return "unsat";
  case SmtAnswer::Unknown: return "unknown";
  }
  return "unknown";
}

bool isPlainScript(const std::string &script, std::string &why) {
  size_t i = 0, n = script.size();
  bool checked = false;
  while (i < n) {
    char c = script[i];
    if (c == ';') {
      while (i < n && script[i] != '\n')
        ++i;
      continue;
    }
    if (std::isspace((unsigned char)c)) {
      ++i;
      continue;
    }
    if (c != '(') {
      why = "text outside a command";
      return false;
    }
    if (checked) {
      why = "a command follows check-sat";
      return false;
    }
    size_t start = ++i;
    while (i < n && !std::isspace((unsigned char)script[i]) && script[i] != '(' &&
           script[i] != ')')
      ++i;
    std::string command = script.substr(start, i - start);
    if (command == "check-sat")
      checked = true;
    else if (command != "declare-const" && command != "define-fun" &&
             command != "assert") {
      why = "command not allowed: " + command;
      return false;
    }
    int depth = 1;
    while (i < n && depth) {
      char d = script[i++];
      if (d == ';') {
        why = "a comment inside a command";
        return false;
      }
      if (d == '"' || d == '|') {
        why = "a quoted token";
        return false;
      }
      depth += d == '(' ? 1 : d == ')' ? -1 : 0;
    }
    if (depth) {
      why = "unbalanced parentheses";
      return false;
    }
  }
  if (!checked) {
    why = "no check-sat";
    return false;
  }
  return true;
}

Term scopeFormula(const SemanticProgram &program, const ParameterScope &scope,
                  std::string &why) {
  TermContext &terms = *program.terms;
  Term all = terms.boolean(true);
  // No entry has a negative extent: initialConfiguration refuses it.
  for (const Parameter &p : program.parameters)
    if (p.kind == Parameter::Dimension)
      all = terms.logicalAnd(
          all, terms.icmp(Predicate::Sge, terms.param(p.id, p.sort),
                          terms.bv(APInt::getZero(p.sort.width))));
  if (scope.allParameters) {
    if (!scope.ranges.empty()) {
      why = "a scope over all parameters lists ranges";
      return nullptr;
    }
    return all;
  }
  for (const ParameterRange &range : scope.ranges) {
    if (range.parameter >= program.parameters.size()) {
      why = "a range names no parameter";
      return nullptr;
    }
    const Parameter &p = program.parameters[range.parameter];
    if (p.sort.kind != Sort::BV) {
      why = "a range is on a parameter that is not an integer";
      return nullptr;
    }
    if (range.lower.getBitWidth() != p.sort.width ||
        range.upper.getBitWidth() != p.sort.width) {
      why = "a range has the wrong width";
      return nullptr;
    }
    Term value = terms.param(p.id, p.sort);
    all = terms.logicalAnd(
        all, terms.logicalAnd(terms.icmp(Predicate::Sge, value, terms.bv(range.lower)),
                              terms.icmp(Predicate::Sle, value, terms.bv(range.upper))));
  }
  return all;
}

bool parametersInScope(const ParameterScope &scope,
                       const std::vector<APInt> &parameters) {
  DomainEvidence evidence;
  evidence.scope = scope;
  return scopeContains(evidence, parameters);
}

Term continues(const SemanticProgram &program, const PathSet &set,
               llvm::ArrayRef<unsigned> candidates, unsigned location) {
  TermContext &terms = *program.terms;
  Term any = terms.boolean(false);
  for (unsigned id : candidates)
    if (set.paths[id].from == location)
      any = terms.logicalOr(any, set.paths[id].condition);
  return any;
}

Term guardViolation(const SemanticProgram &program, unsigned location) {
  TermContext &terms = *program.terms;
  Term none = terms.boolean(true), overlap = terms.boolean(false);
  bool found = false;
  Term seen = terms.boolean(false);
  for (const Transition &t : program.transitions) {
    if (t.from != location)
      continue;
    found = true;
    overlap = terms.logicalOr(overlap, terms.logicalAnd(seen, t.guard));
    seen = terms.logicalOr(seen, t.guard);
    none = terms.logicalAnd(none, terms.logicalNot(t.guard));
  }
  return found ? terms.logicalOr(none, overlap) : nullptr;
}

Term invariantViolation(const SemanticProgram &program, Term scope,
                        const CutPath &path, Term source, Term target) {
  TermContext &terms = *program.terms;
  Term premise = terms.logicalAnd(terms.logicalAnd(scope, source), path.condition);
  return terms.logicalAnd(premise,
                          terms.logicalNot(postTerm(program, path, target)));
}

Term descentViolation(const SemanticProgram &program, const PathSet &set,
                      Term scope, const CutPath &path, Term invariant,
                      llvm::ArrayRef<unsigned> candidates, Term rankAtSource,
                      Term rankAtTarget, bool strict) {
  TermContext &terms = *program.terms;
  Term premise = terms.logicalAnd(terms.logicalAnd(scope, invariant), path.condition);
  // A run that leaves the SCC, or is stuck at the target, owes no descent. An
  // infinite run inside it satisfies this at every step.
  premise = terms.logicalAnd(
      premise,
      postTerm(program, path, continues(program, set, candidates, path.to)));
  Term next = postTerm(program, path, rankAtTarget);
  Term decreased = terms.icmp(strict ? Predicate::Ult : Predicate::Ule, next,
                              rankAtSource);
  return terms.logicalAnd(premise, terms.logicalNot(decreased));
}

Term stopViolation(const SemanticProgram &program, Term scope,
                   const StopSite &site, Term invariant) {
  TermContext &terms = *program.terms;
  return terms.logicalAnd(
      terms.logicalAnd(terms.logicalAnd(scope, invariant), site.reached),
      site.violated);
}

std::string obligationScript(const std::string &comment, Term formula,
                             llvm::ArrayRef<std::pair<std::string, Term>> aux) {
  std::string out = "; ";
  for (char c : comment)
    out += c == '\n' ? ' ' : c;
  out += "\n";
  llvm::SmallVector<Term> roots{formula};
  for (const auto &[name, term] : aux)
    roots.push_back(term);
  out += encodeTerms(roots);
  for (const auto &[name, term] : aux) {
    out += "(declare-const " + name + " " + smtSortText(term->sort) + ")\n";
    out += "(assert (= " + name + " " + smtTermName(term) + "))\n";
  }
  out += "(assert (= " + smtTermName(formula) + " #b1))\n(check-sat)\n";
  return out;
}

Term adoptProofTerm(const SemanticProgram &program, Term term, std::string &why) {
  if (!term) {
    why = "a missing term";
    return nullptr;
  }
  TermContext &terms = *program.terms;
  llvm::DenseMap<Term, Term> memo;
  std::function<Term(Term)> visit = [&](Term n) -> Term {
    auto known = memo.find(n);
    if (known != memo.end())
      return known->second;
    Term result = nullptr;
    if (n->sort.kind != Sort::BV) {
      why = "a proof term may hold only integers";
      return nullptr;
    }
    switch (n->op) {
    case TermOp::Const:
      result = terms.bv(n->value);
      break;
    case TermOp::Param:
      if (n->index >= program.parameters.size() ||
          program.parameters[n->index].sort != n->sort) {
        why = "a parameter leaf does not exist";
        return nullptr;
      }
      result = terms.param(n->index, n->sort);
      break;
    case TermOp::State:
      if (n->index >= program.state.size() ||
          program.state[n->index].sort != n->sort) {
        why = "a state leaf does not exist";
        return nullptr;
      }
      result = terms.state(n->index, n->sort);
      break;
    case TermOp::Shadow:
      if (n->index >= program.state.size()) {
        why = "a shadow leaf does not exist";
        return nullptr;
      }
      result = terms.shadow(n->index);
      break;
    case TermOp::FConst: case TermOp::MemVar: case TermOp::Load:
    case TermOp::Store: case TermOp::FAdd: case TermOp::FMul:
    case TermOp::FIsNaN: case TermOp::FIsInf: case TermOp::FIsZero:
      why = "a proof term may not read memory or floats";
      return nullptr;
    default: {
      llvm::SmallVector<Term, 4> args;
      for (Term a : n->args) {
        Term adopted = visit(a);
        if (!adopted)
          return nullptr;
        args.push_back(adopted);
      }
      result = terms.rebuild(n, args);
      break;
    }
    }
    memo[n] = result;
    return result;
  };
  try {
    return visit(term);
  } catch (const TermError &error) {
    why = error.what();
    return nullptr;
  }
}

bool readsMemory(Term term) {
  llvm::DenseSet<Term> seen;
  std::function<bool(Term)> visit = [&](Term n) {
    if (!seen.insert(n).second)
      return false;
    if (n->op == TermOp::MemVar)
      return true;
    for (Term a : n->args)
      if (visit(a))
        return true;
    return false;
  };
  return visit(term);
}

} // namespace mlir::generalized_gemm
