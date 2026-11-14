#include "GeneralizedGemm/CoordinateGrammar.h"

#include "GeneralizedGemm/IndexProgram.h"

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
  std::optional<std::vector<uint64_t>> sample(Term term);
  void leaves();
  void grow(unsigned size);
  void statements();
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
  std::map<Term, std::string> programText; // Terms lowered from statement programs.
};

/// The term's values at the sampled events, after its width; nullopt when it has
/// no value or is undefined at a sample, which no coordinate may be.
std::optional<std::vector<uint64_t>> Builder::sample(Term term) {
  Term defined = definedness(terms, term);
  std::vector<uint64_t> signature{term->sort.width};
  for (unsigned d = 0; d < evaluators.size(); ++d) {
    EventEvaluator &evaluator = evaluators[d];
    for (size_t e = 0; e < evaluator.size(); e += strides[d]) {
      auto value = evaluator.value(term, e);
      auto ok = evaluator.value(defined, e);
      if (!value || !ok || ok->isZero())
        return std::nullopt;
      signature.push_back(value->getZExtValue());
    }
  }
  return signature;
}

bool Builder::offer(Term term, unsigned size) {
  if (operands.size() >= spec.maxOperands) {
    out.truncated = "the operand limit";
    return false;
  }
  auto signature = sample(term);
  if (!signature)
    return true;
  if (seen.try_emplace(std::move(*signature), operands.size()).second)
    operands.push_back({term, size});
  return true;
}

/// The statement productions, loop-free: `if B then v := E else v := E ; return
/// v`, with B a comparison of operands and constants, its negation left out
/// because swapping the arms gives it, and E any operand, a nested program
/// included up to maxConditionals. Each program is lowered to a term by
/// indexProgramTerm and offered like a grown term; the program's text is kept
/// for the record. The interpreter of IndexProgram.h is the meaning, and the
/// lowering is tested against it; neither is evidence of anything.
void Builder::statements() {
  if (!spec.statements)
    return;
  struct Test {
    Term term;
    unsigned size;
  };
  // Conditions over the operands present before any program, by signature on the
  // samples. A condition constant on the samples decides nothing there.
  std::vector<Test> tests;
  std::set<std::vector<uint64_t>> testSeen;
  std::vector<Operand> pool = operands;
  auto constant = [&](unsigned width, uint64_t value) {
    return terms.bv(APInt(width, value, /*isSigned=*/false, /*implicitTrunc=*/true));
  };
  auto addTest = [&](Term test, unsigned size) {
    if (tests.size() >= spec.maxConditions)
      return false;
    auto signature = sample(test);
    if (!signature)
      return true;
    bool allTrue = true, allFalse = true;
    std::vector<uint64_t> complement{1};
    for (size_t n = 1; n < signature->size(); ++n) {
      allTrue = allTrue && (*signature)[n] == 1;
      allFalse = allFalse && (*signature)[n] == 0;
      complement.push_back(1 - (*signature)[n]);
    }
    if (allTrue || allFalse || testSeen.count(complement))
      return true;
    if (testSeen.insert(std::move(*signature)).second)
      tests.push_back({test, size});
    return true;
  };
  // The guards of the program first, as the slice of Section 4.4 holds them: a
  // comparison the program itself branches on, read as a leaf of size 1.
  std::vector<Term> guardTests;
  DenseSet<Term> guardSeen;
  std::function<void(Term)> guards = [&](Term t) {
    if (!guardSeen.insert(t).second)
      return;
    if (t->op == TermOp::ICmp && t->args[0]->sort.width > 1)
      guardTests.push_back(t);
    for (Term arg : t->args)
      guards(arg);
  };
  for (const Transition &t : program.transitions)
    guards(t.guard);
  for (Term test : guardTests)
    if (!addTest(test, 1))
      break;
  for (const Operand &a : pool) {
    for (const auto &[width, value] : constants) {
      if (width != a.term->sort.width)
        continue;
      Term c = constant(width, value);
      for (Predicate predicate : {Predicate::Eq, Predicate::Ult, Predicate::Ule})
        if (!addTest(terms.icmp(predicate, a.term, c), a.size + 2))
          break;
    }
    for (const Operand &b : pool) {
      if (a.term == b.term || a.term->sort != b.term->sort)
        continue;
      for (Predicate predicate : {Predicate::Eq, Predicate::Ult, Predicate::Ule})
        if (!addTest(terms.icmp(predicate, a.term, b.term), a.size + b.size + 1))
          break;
    }
  }
  std::stable_sort(tests.begin(), tests.end(),
                   [](const Test &a, const Test &b) { return a.size < b.size; });
  auto name = [&](Term input) { return formatTerm(input, 6); };
  unsigned tried = 0;
  for (unsigned round = 0; round < spec.maxConditionals; ++round) {
    // Arms: every operand so far, the previous round's programs included.
    std::vector<Operand> arms = operands;
    std::stable_sort(arms.begin(), arms.end(),
                     [](const Operand &a, const Operand &b) { return a.size < b.size; });
    for (const Test &test : tests)
      for (const Operand &a : arms)
        for (const Operand &b : arms) {
          if (a.term == b.term || a.term->sort != b.term->sort)
            continue;
          // Nesting a program under itself says nothing new.
          if (round > 0 && !programText.count(a.term) && !programText.count(b.term))
            continue;
          if (tried++ >= spec.maxPrograms) {
            if (out.truncated.empty())
              out.truncated = "the program limit";
            return;
          }
          IndexProgram program;
          unsigned v = program.local(a.term->sort.width);
          std::vector<Term> leaves;
          std::function<void(Term)> collect = [&](Term t) {
            if (t->op == TermOp::Const)
              return;
            if (!llvm::is_contained(leaves, t))
              leaves.push_back(t);
          };
          // The test's operands and the arms are the inputs: terms the grammar
          // already holds, read by the program as the thesis's t is read.
          for (Term operand : test.term->args)
            collect(operand);
          collect(a.term);
          collect(b.term);
          std::map<Term, unsigned> inputOf;
          for (Term leaf : leaves)
            inputOf[leaf] = program.input(leaf);
          auto leafExpr = [&](Term t) {
            return t->op == TermOp::Const ? program.constant(t->value)
                                          : program.inputExpr(inputOf.at(t));
          };
          unsigned lhs = leafExpr(test.term->args[0]), rhs = leafExpr(test.term->args[1]);
          unsigned cond;
          switch ((Predicate)test.term->aux) {
          case Predicate::Eq: cond = program.eq(lhs, rhs); break;
          case Predicate::Ne: cond = program.lnot(program.eq(lhs, rhs)); break;
          case Predicate::Ult: cond = program.ult(lhs, rhs); break;
          case Predicate::Ule: cond = program.ule(lhs, rhs); break;
          case Predicate::Ugt: cond = program.ult(rhs, lhs); break;
          case Predicate::Uge: cond = program.ule(rhs, lhs); break;
          default: continue; // Signed comparisons are outside the grammar.
          }
          unsigned then = program.assign(v, leafExpr(a.term));
          unsigned otherwise = program.assign(v, leafExpr(b.term));
          program.body = program.ifThenElse(cond, then, otherwise);
          program.result = program.localExpr(v);
          auto lowered = indexProgramTerm(terms, program);
          if (!lowered)
            continue;
          size_t before = operands.size();
          if (!offer(*lowered, test.size + a.size + b.size + 1))
            return;
          if (operands.size() == before)
            continue;
          ++out.programs;
          std::string text = formatIndexProgram(program, [&](Term input) {
            auto found = programText.find(input);
            return found != programText.end() ? "(" + found->second + ")" : name(input);
          });
          programText.emplace(*lowered, std::move(text));
        }
  }
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
  unsigned termAtoms = 0, programAtoms = 0;
  std::vector<Operand> order = operands;
  std::stable_sort(order.begin(), order.end(),
                   [](const Operand &a, const Operand &b) { return a.size < b.size; });
  // Phase 4 (I1): two atoms with one kernel are one label, so the first is kept;
  // except that an iteration counter (LoopCounters.h) takes the place of a term
  // with its kernel, since its unit steps are what the symbolic tier's order
  // needs and the finite verdict on the label is the same either way.
  std::map<std::vector<Partition>, size_t> kernels;
  auto isCounter = [&](Term term) {
    return term->op == TermOp::State && term->index < program.state.size() &&
           program.state[term->index].kind == StateVar::IterationCounter;
  };
  for (const Operand &operand : order) {
    if (operand.term->op == TermOp::Param || stateSupport(operand.term).empty())
      continue;
    Atom atom;
    atom.term = operand.term;
    atom.size = operand.size;
    if (auto found = programText.find(operand.term); found != programText.end())
      atom.program = found->second;
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
    if (!usable || !separates)
      continue;
    auto [slot, fresh] = kernels.try_emplace(std::move(kernel), out.atoms.size());
    if (!fresh) {
      if (slot->second < out.atoms.size() && isCounter(atom.term) && !isCounter(out.atoms[slot->second].term) &&
          out.atoms[slot->second].program.empty())
        out.atoms[slot->second] = std::move(atom);
      continue;
    }
    bool fromProgram = !atom.program.empty();
    unsigned &count = fromProgram ? programAtoms : termAtoms;
    if (count >= (fromProgram ? spec.maxProgramAtoms : spec.maxAtoms)) {
      if (out.truncated.empty())
        out.truncated = fromProgram ? "the program atom limit" : "the atom limit";
      continue;
    }
    ++count;
    out.atoms.push_back(std::move(atom));
  }
  out.operands = operands.size();
}

void Builder::run() {
  leaves();
  for (unsigned size = 3; size <= spec.maxSize && out.truncated.empty(); size += 2)
    grow(size);
  if (out.truncated.empty())
    statements();
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
