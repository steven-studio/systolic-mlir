#include "GeneralizedGemm/CoordinateFamily.h"

#include "llvm/ADT/DenseSet.h"

#include <functional>
#include <map>

using namespace llvm;

namespace mlir::generalized_gemm {

Term definedness(TermContext &terms, Term term) {
  Term defined = terms.boolean(true);
  DenseSet<Term> seen;
  std::function<void(Term)> visit = [&](Term n) {
    if (!seen.insert(n).second)
      return;
    for (Term arg : n->args)
      visit(arg);
    switch (n->op) {
    case TermOp::UDiv: case TermOp::URem: case TermOp::SDiv: case TermOp::SRem: {
      unsigned width = n->sort.width;
      Term divisor = n->args[1];
      defined = terms.logicalAnd(
          defined, terms.icmp(Predicate::Ne, divisor, terms.bv(APInt::getZero(width))));
      if (n->op == TermOp::SDiv || n->op == TermOp::SRem) {
        Term minimum = terms.icmp(Predicate::Eq, n->args[0],
                                  terms.bv(APInt::getSignedMinValue(width)));
        Term minusOne = terms.icmp(Predicate::Eq, divisor,
                                   terms.bv(APInt::getAllOnes(width)));
        defined = terms.logicalAnd(defined,
                                   terms.logicalNot(terms.logicalAnd(minimum, minusOne)));
      }
      break;
    }
    case TermOp::Shl: case TermOp::LShr: case TermOp::AShr: {
      unsigned width = n->sort.width;
      defined = terms.logicalAnd(
          defined, terms.icmp(Predicate::Ult, n->args[1], terms.bv(APInt(width, width))));
      break;
    }
    default:
      break;
    }
  };
  visit(term);
  return defined;
}

EventEvaluator::EventEvaluator(const SemanticProgram &program, const AccessDomain &domain)
    : program(program), domain(domain) {
  entry.parameters = domain.parameters();
  config.state.assign(program.state.size(), std::nullopt);
  config.shadow.assign(program.state.size(), false);
}

std::optional<APInt> EventEvaluator::value(Term term, size_t event) {
  const AccessTable &table = domain.table();
  if (loaded != event) {
    const IndexTuple &x = table.point(table.events()[event].point);
    for (unsigned n = 0; n < table.coordinateNames.size(); ++n)
      config.state[table.coordinateNames[n]] = x[n + 1];
    loaded = event;
  }
  return evaluateTerm(program, entry, config, term);
}

std::optional<CoordinateCandidate> familyLabels(const SemanticProgram &program,
                                                const AccessDomain &domain,
                                                const CoordinateFamily &family,
                                                std::string &why) {
  EventEvaluator evaluator(program, domain);
  TermContext &terms = *program.terms;
  std::array<std::vector<Term>, 4> defined;
  for (unsigned axis = 0; axis < 4; ++axis)
    for (Term term : family.axes[axis])
      defined[axis].push_back(definedness(terms, term));
  CoordinateCandidate out;
  out.origin = family.origin;
  std::array<std::vector<uint32_t> *, 4> labels{&out.p, &out.i, &out.j, &out.k};
  std::array<TupleTable, 4> tables;
  for (size_t e = 0; e < evaluator.size(); ++e)
    for (unsigned axis = 0; axis < 4; ++axis) {
      IndexTuple tuple;
      for (unsigned n = 0; n < family.axes[axis].size(); ++n) {
        auto value = evaluator.value(family.axes[axis][n], e);
        auto ok = evaluator.value(defined[axis][n], e);
        if (!value || !ok) {
          why = "a coordinate has no value at event " + std::to_string(e);
          return std::nullopt;
        }
        if (ok->isZero()) {
          why = "a coordinate is undefined at event " + std::to_string(e);
          return std::nullopt;
        }
        tuple.push_back(*value);
      }
      labels[axis]->push_back(tables[axis].intern(tuple));
    }
  return out;
}

} // namespace mlir::generalized_gemm
