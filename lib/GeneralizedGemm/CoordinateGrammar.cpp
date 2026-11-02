#include "GeneralizedGemm/CoordinateGrammar.h"

#include "llvm/ADT/DenseSet.h"

#include <algorithm>
#include <functional>
#include <map>
#include <set>

using namespace llvm;

namespace mlir::generalized_gemm {

GrammarSpec GrammarSpec::atLevel(unsigned level) {
  GrammarSpec spec;
  spec.level = level;
  spec.maxSize = level == 0 ? 1 : 3;
  spec.addition = spec.subtraction = spec.constantProduct = level >= 1;
  spec.product = spec.division = spec.remainder = level >= 2;
  spec.comparison = spec.conditional = level >= 3;
  spec.bits = level >= 4;
  return spec;
}

namespace {

class Builder {
public:
  Builder(const SemanticProgram &program, const SiteGroup &group,
          ArrayRef<const AccessDomain *> domains, const GrammarSpec &spec, AtomBank &out)
      : program(program), group(group), spec(spec), terms(*program.terms), out(out) {
    for (const AccessDomain *domain : domains) {
      evaluators.emplace_back(program, *domain);
      // Enumeration works on a thin sample. Selected atoms use every event.
      size_t count = evaluators.back().size();
      strides.push_back(std::max<size_t>(1, count / 128));
    }
  }

  void run();

private:
  struct Operand {
    Term term;
    unsigned size;
  };
  bool offer(Term term, unsigned size);
  void leaves();
  void grow(unsigned size);
  void select();
  void collectConstants(Term term);

  const SemanticProgram &program;
  const SiteGroup &group;
  const GrammarSpec &spec;
  TermContext &terms;
  AtomBank &out;
  std::vector<EventEvaluator> evaluators;
  std::vector<size_t> strides;
  std::vector<Operand> operands;
  std::map<std::vector<uint64_t>, unsigned> seen;
  std::set<std::pair<unsigned, uint64_t>> constants;
  DenseSet<Term> visited;
};

bool Builder::offer(Term term, unsigned size) {
  if (operands.size() >= spec.maxOperands) {
    out.truncated = "the operand limit";
    return false;
  }
  Term defined = definedness(terms, term);
  std::vector<uint64_t> signature{term->sort.width};
  for (unsigned d = 0; d < evaluators.size(); ++d) {
    EventEvaluator &evaluator = evaluators[d];
    for (size_t e = 0; e < evaluator.size(); e += strides[d]) {
      auto value = evaluator.value(term, e);
      auto ok = evaluator.value(defined, e);
      // A term with no value, or undefined at a sample, cannot be a coordinate.
      if (!value || !ok || ok->isZero())
        return true;
      signature.push_back(value->getZExtValue());
    }
  }
  if (seen.try_emplace(std::move(signature), operands.size()).second)
    operands.push_back({term, size});
  return true;
}

void Builder::collectConstants(Term term) {
  if (!visited.insert(term).second)
    return;
  if (term->op == TermOp::Const && term->sort.width > 1)
    constants.insert({term->sort.width, term->value.getZExtValue()});
  for (Term arg : term->args)
    collectConstants(arg);
}

void Builder::leaves() {
  // State first: a coordinate is about the iteration.
  for (const StateVar &s : program.state)
    if (s.sort.kind == Sort::BV)
      offer(terms.state(s.id, s.sort), 1);

  // The subterms of the accesses: what the program itself computes.
  std::function<void(Term)> subterms = [&](Term term) {
    if (term->sort.kind == Sort::BV && term->sort.width > 1 &&
        term->op != TermOp::Const && term->op != TermOp::Param)
      offer(term, 1);
    for (Term arg : term->args)
      subterms(arg);
  };
  for (unsigned id : group.sites) {
    const MacSite &site = program.sites[id];
    for (const MacAccess *access : {&site.a, &site.b, site.output ? &*site.output : nullptr})
      if (access)
        for (Term index : access->index) {
          subterms(index);
          collectConstants(index);
        }
  }
  for (const Transition &t : program.transitions) {
    collectConstants(t.guard);
    for (const auto &[id, term] : t.assignments)
      if (term->sort.kind == Sort::BV)
        collectConstants(term);
  }
  for (const Parameter &p : program.parameters)
    if (p.sort.kind == Sort::BV)
      offer(terms.param(p.id, p.sort), 1);
  std::set<unsigned> widths;
  for (const Operand &operand : operands)
    widths.insert(operand.term->sort.width);
  for (unsigned width : widths)
    constants.insert({width, 1});
  for (const APInt &value : spec.extraConstants)
    constants.insert({value.getBitWidth(), value.getZExtValue()});
  for (const MemoryRoot &root : program.roots)
    for (int64_t extent : root.extents)
      if (extent > 1)
        for (unsigned width : widths)
          if (width >= 63 || (uint64_t)extent >> width == 0)
            constants.insert({width, (uint64_t)extent});
}

void Builder::grow(unsigned size) {
  auto ofSize = [&](unsigned wanted) {
    std::vector<Term> list;
    for (const Operand &operand : operands)
      if (operand.size == wanted)
        list.push_back(operand.term);
    return list;
  };
  std::vector<Term> small = ofSize(1);
  auto constant = [&](unsigned width, uint64_t value) {
    return terms.bv(APInt(width, value, /*isSigned=*/false, /*implicitTrunc=*/true));
  };
  auto binary = [&](TermOp op, Term a, Term b) {
    return offer(terms.binary(op, a, b), size);
  };
  for (unsigned left = 1; left + 1 < size; ++left) {
    std::vector<Term> as = ofSize(left), bs = ofSize(size - 1 - left);
    for (Term a : as) {
      for (const auto &[width, value] : constants) {
        if (width != a->sort.width)
          continue;
        Term c = constant(width, value);
        if (size - 1 - left != 1)
          continue;
        if (spec.addition && !binary(TermOp::Add, a, c))
          return;
        if (spec.subtraction && !binary(TermOp::Sub, a, c))
          return;
        if (spec.constantProduct && value > 1 && !binary(TermOp::Mul, a, c))
          return;
        if (spec.division && value > 1 && !binary(TermOp::UDiv, a, c))
          return;
        if (spec.remainder && value > 1 && !binary(TermOp::URem, a, c))
          return;
        if (spec.bits && value > 0 && value < width) {
          if (!binary(TermOp::Shl, a, c) || !binary(TermOp::LShr, a, c))
            return;
        }
        if (spec.bits && !binary(TermOp::And, a, c))
          return;
      }
      for (Term b : bs) {
        if (a->sort != b->sort)
          continue;
        if (spec.addition && !binary(TermOp::Add, a, b))
          return;
        if (spec.subtraction && !binary(TermOp::Sub, a, b))
          return;
        if (spec.product && !binary(TermOp::Mul, a, b))
          return;
        if (spec.division && !binary(TermOp::UDiv, a, b))
          return;
        if (spec.remainder && !binary(TermOp::URem, a, b))
          return;
        if (spec.bits && !binary(TermOp::Xor, a, b))
          return;
      }
    }
  }
  if (spec.comparison && size == 3) {
    std::vector<Term> tests;
    for (Term a : small)
      for (const auto &[width, value] : constants)
        if (width == a->sort.width)
          tests.push_back(terms.icmp(Predicate::Ult, a, constant(width, value)));
    for (Term test : tests) {
      if (!offer(test, size))
        return;
      if (spec.conditional)
        for (Term a : small)
          for (Term b : small)
            if (a != b && a->sort == b->sort && !offer(terms.ite(test, a, b), size + 1))
              return;
    }
  }
}

void Builder::select() {
  std::vector<Operand> order = operands;
  std::stable_sort(order.begin(), order.end(),
                   [](const Operand &a, const Operand &b) { return a.size < b.size; });
  std::set<std::vector<Partition>> kernels;
  for (const Operand &operand : order) {
    if (operand.term->op == TermOp::Param || stateSupport(operand.term).empty())
      continue;
    Atom atom;
    atom.term = operand.term;
    atom.size = operand.size;
    std::vector<Partition> kernel;
    bool usable = true, separates = false;
    for (EventEvaluator &evaluator : evaluators) {
      std::vector<uint64_t> values;
      std::map<uint64_t, uint32_t> ids;
      Partition labels;
      for (size_t e = 0; e < evaluator.size() && usable; ++e) {
        auto value = evaluator.value(operand.term, e);
        if (!value) {
          usable = false;
          break;
        }
        values.push_back(value->getZExtValue());
        labels.push_back(ids.try_emplace(values.back(), (uint32_t)ids.size()).first->second);
      }
      separates = separates || ids.size() > 1;
      atom.values.push_back(std::move(values));
      kernel.push_back(std::move(labels));
    }
    // Only kernels matter to a label, and a constant separates nothing.
    if (!usable || !separates || !kernels.insert(std::move(kernel)).second)
      continue;
    if (out.atoms.size() >= spec.maxAtoms) {
      if (out.truncated.empty())
        out.truncated = "the atom limit";
      break;
    }
    out.atoms.push_back(std::move(atom));
  }
  out.operands = operands.size();
}

void Builder::run() {
  leaves();
  for (unsigned size = 3; size <= spec.maxSize && out.truncated.empty(); size += 2)
    grow(size);
  select();
}

} // namespace

AtomBank buildAtoms(const SemanticProgram &program, const SiteGroup &group,
                    ArrayRef<const AccessDomain *> domains, const GrammarSpec &spec) {
  AtomBank out;
  try {
    Builder(program, group, domains, spec, out).run();
  } catch (const TermError &error) {
    out.truncated = error.what();
  }
  return out;
}

} // namespace mlir::generalized_gemm
