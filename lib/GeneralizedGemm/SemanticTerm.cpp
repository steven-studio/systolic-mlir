#include "GeneralizedGemm/SemanticTerm.h"

#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringExtras.h"

#include <algorithm>

using namespace mlir::generalized_gemm;
using llvm::APInt;

Sort Sort::bv(unsigned width) {
  if (width == 0)
    throw TermError("a bitvector needs a nonzero width");
  Sort s;
  s.kind = BV;
  s.width = width;
  return s;
}
Sort Sort::flt(unsigned width) {
  if (width != 32 && width != 64)
    throw TermError("only binary32 and binary64 are modelled");
  Sort s;
  s.kind = Float;
  s.width = width;
  return s;
}
Sort Sort::mem(unsigned root, unsigned rank, unsigned indexWidth, Sort element) {
  if (element.kind == Mem || indexWidth == 0)
    throw TermError("ill-formed memory sort");
  Sort s;
  s.kind = Mem;
  s.width = element.width;
  s.root = root;
  s.rank = rank;
  s.indexWidth = indexWidth;
  s.elementIsFloat = element.kind == Float;
  return s;
}
Sort Sort::element() const {
  if (kind != Mem)
    throw TermError("only memory has an element sort");
  return elementIsFloat ? flt(width) : bv(width);
}
std::string Sort::str() const {
  switch (kind) {
  case BV: return "bv" + std::to_string(width);
  case Float: return "f" + std::to_string(width);
  case Mem:
    return "mem" + std::to_string(root) + "<" + std::to_string(rank) + ",i" +
           std::to_string(indexWidth) + "," + element().str() + ">";
  }
  return "?";
}

Term TermContext::intern(TermOp op, Sort sort, unsigned aux, unsigned index,
                         const APInt &value, llvm::ArrayRef<Term> args) {
  std::string key = std::to_string((int)op) + "|" + sort.str() + "|" +
                    std::to_string(aux) + "|" + std::to_string(index) + "|";
  if (op == TermOp::Const || op == TermOp::FConst) {
    llvm::SmallString<32> text;
    value.toStringUnsigned(text, 16);
    key += text.str().str();
  }
  for (Term arg : args)
    key += "," + std::to_string(arg->id);
  auto found = table.find(key);
  if (found != table.end())
    return found->second;
  if (nodes.size() >= limit)
    throw TermError("term resource limit");
  auto node = std::make_unique<TermNode>(TermNode{
      op, sort, (unsigned)nodes.size(), aux, index, value,
      llvm::SmallVector<Term, 3>(args.begin(), args.end())});
  Term result = node.get();
  nodes.push_back(std::move(node));
  table[key] = result;
  return result;
}

static void require(bool condition, const char *message) {
  if (!condition)
    throw TermError(message);
}
static bool isBV(Term t) { return t->sort.kind == Sort::BV; }
static bool isBool(Term t) { return isBV(t) && t->sort.width == 1; }

Term TermContext::bv(const APInt &value) {
  return intern(TermOp::Const, Sort::bv(value.getBitWidth()), 0, 0, value, {});
}
Term TermContext::boolean(bool value) { return bv(APInt(1, value ? 1 : 0)); }
Term TermContext::fconst(const APInt &bits) {
  return intern(TermOp::FConst, Sort::flt(bits.getBitWidth()), 0, 0, bits, {});
}
Term TermContext::param(unsigned id, Sort sort) {
  require(sort.kind != Sort::Mem, "a parameter is scalar");
  return intern(TermOp::Param, sort, 0, id, APInt(), {});
}
Term TermContext::state(unsigned id, Sort sort) {
  require(sort.kind != Sort::Mem, "a state variable is scalar");
  return intern(TermOp::State, sort, 0, id, APInt(), {});
}
Term TermContext::shadow(unsigned id) {
  return intern(TermOp::Shadow, Sort::bv(1), 0, id, APInt(), {});
}
Term TermContext::memVar(Sort sort) {
  require(sort.kind == Sort::Mem, "memVar needs a memory sort");
  return intern(TermOp::MemVar, sort, 0, sort.root, APInt(), {});
}

Term TermContext::binary(TermOp op, Term a, Term b, unsigned flags) {
  require(op >= TermOp::Add && op <= TermOp::Xor, "not a binary integer operation");
  require(isBV(a) && isBV(b), "integer operation on a non-integer");
  require(a->sort.width == b->sort.width, "operand widths differ");
  unsigned allowed = 0;
  if (op == TermOp::Add || op == TermOp::Sub || op == TermOp::Mul || op == TermOp::Shl)
    allowed = FlagNSW | FlagNUW;
  if (op == TermOp::SDiv || op == TermOp::UDiv || op == TermOp::LShr || op == TermOp::AShr)
    allowed = FlagExact;
  require((flags & ~allowed) == 0, "flag not defined for this operation");
  return intern(op, a->sort, flags, 0, APInt(), {a, b});
}

static bool isTrue(Term t) { return t->op == TermOp::Const && t->value.isOne(); }
static bool isFalse(Term t) { return t->op == TermOp::Const && t->value.isZero(); }

// The only simplifications in the term language: Boolean units. None changes
// a value, and arithmetic is never folded.
Term TermContext::logicalAnd(Term a, Term b) {
  require(isBool(a) && isBool(b), "logical operation needs bv1");
  if (isFalse(a) || isTrue(b)) return a;
  if (isFalse(b) || isTrue(a)) return b;
  return intern(TermOp::And, a->sort, 0, 0, APInt(), {a, b});
}
Term TermContext::logicalOr(Term a, Term b) {
  require(isBool(a) && isBool(b), "logical operation needs bv1");
  if (isTrue(a) || isFalse(b)) return a;
  if (isTrue(b) || isFalse(a)) return b;
  return intern(TermOp::Or, a->sort, 0, 0, APInt(), {a, b});
}
Term TermContext::logicalNot(Term a) {
  require(isBool(a), "logical operation needs bv1");
  if (isTrue(a)) return boolean(false);
  if (isFalse(a)) return boolean(true);
  return intern(TermOp::Not, a->sort, 0, 0, APInt(), {a});
}
Term TermContext::icmp(Predicate predicate, Term a, Term b) {
  require(isBV(a) && isBV(b), "comparison of non-integers");
  require(a->sort.width == b->sort.width, "operand widths differ");
  return intern(TermOp::ICmp, Sort::bv(1), (unsigned)predicate, 0, APInt(), {a, b});
}
Term TermContext::ite(Term condition, Term a, Term b) {
  require(isBool(condition), "a condition is bv1");
  require(a->sort == b->sort, "ite arms have different sorts");
  require(a->sort.kind != Sort::Mem, "ite over memory is not modelled");
  if (isTrue(condition)) return a;
  if (isFalse(condition)) return b;
  return intern(TermOp::Ite, a->sort, 0, 0, APInt(), {condition, a, b});
}
Term TermContext::zext(Term a, unsigned width) {
  require(isBV(a) && width > a->sort.width, "zext must widen an integer");
  return intern(TermOp::ZExt, Sort::bv(width), 0, 0, APInt(), {a});
}
Term TermContext::sext(Term a, unsigned width) {
  require(isBV(a) && width > a->sort.width, "sext must widen an integer");
  return intern(TermOp::SExt, Sort::bv(width), 0, 0, APInt(), {a});
}
Term TermContext::trunc(Term a, unsigned width) {
  require(isBV(a) && width > 0 && width < a->sort.width,
          "trunc must narrow an integer");
  return intern(TermOp::Trunc, Sort::bv(width), 0, 0, APInt(), {a});
}
static void requireIndex(Term memory, llvm::ArrayRef<Term> index) {
  require(memory->sort.kind == Sort::Mem, "memory operation on a non-memory");
  require(index.size() == memory->sort.rank, "index rank mismatch");
  for (Term i : index)
    require(isBV(i) && i->sort.width == memory->sort.indexWidth,
            "index width mismatch");
}
Term TermContext::load(Term memory, llvm::ArrayRef<Term> index) {
  requireIndex(memory, index);
  llvm::SmallVector<Term, 4> args{memory};
  args.append(index.begin(), index.end());
  return intern(TermOp::Load, memory->sort.element(), 0, 0, APInt(), args);
}
Term TermContext::store(Term memory, llvm::ArrayRef<Term> index, Term value) {
  requireIndex(memory, index);
  require(value->sort == memory->sort.element(), "stored value sort mismatch");
  llvm::SmallVector<Term, 4> args{memory, value};
  args.append(index.begin(), index.end());
  return intern(TermOp::Store, memory->sort, 0, 0, APInt(), args);
}
Term TermContext::fbinary(TermOp op, Term a, Term b, unsigned flags) {
  require(op == TermOp::FAdd || op == TermOp::FMul, "not a float operation");
  require(a->sort.kind == Sort::Float && a->sort == b->sort,
          "float operand sorts differ");
  return intern(op, a->sort, flags, 0, APInt(), {a, b});
}
Term TermContext::fclass(TermOp op, Term a) {
  require(op == TermOp::FIsNaN || op == TermOp::FIsInf || op == TermOp::FIsZero,
          "not a float class");
  require(a->sort.kind == Sort::Float, "float class of a non-float");
  return intern(op, Sort::bv(1), 0, 0, APInt(), {a});
}

Term TermContext::rebuild(Term n, llvm::ArrayRef<Term> a) {
  switch (n->op) {
  case TermOp::Const: case TermOp::FConst: case TermOp::Param: case TermOp::State:
  case TermOp::Shadow: case TermOp::MemVar:
    return n;
  case TermOp::Add: case TermOp::Sub: case TermOp::Mul: case TermOp::SDiv: case TermOp::UDiv:
  case TermOp::SRem: case TermOp::URem: case TermOp::Shl: case TermOp::LShr: case TermOp::AShr:
  case TermOp::Xor:
    return binary(n->op, a[0], a[1], n->aux);
  case TermOp::And:
    return n->sort.width == 1 ? logicalAnd(a[0], a[1]) : binary(TermOp::And, a[0], a[1]);
  case TermOp::Or:
    return n->sort.width == 1 ? logicalOr(a[0], a[1]) : binary(TermOp::Or, a[0], a[1]);
  case TermOp::Not: return logicalNot(a[0]);
  case TermOp::ICmp: return icmp((Predicate)n->aux, a[0], a[1]);
  case TermOp::Ite: return ite(a[0], a[1], a[2]);
  case TermOp::ZExt: return zext(a[0], n->sort.width);
  case TermOp::SExt: return sext(a[0], n->sort.width);
  case TermOp::Trunc: return trunc(a[0], n->sort.width);
  case TermOp::Load: return load(a[0], a.drop_front());
  case TermOp::Store: return store(a[0], a.drop_front(2), a[1]);
  case TermOp::FAdd: case TermOp::FMul: return fbinary(n->op, a[0], a[1], n->aux);
  case TermOp::FIsNaN: case TermOp::FIsInf: case TermOp::FIsZero:
    return fclass(n->op, a[0]);
  }
  throw TermError("unknown term");
}

Term TermContext::substitute(Term root, const std::function<Term(Term)> &leaf) {
  llvm::DenseMap<Term, Term> memo;
  std::function<Term(Term)> visit = [&](Term n) -> Term {
    auto it = memo.find(n);
    if (it != memo.end())
      return it->second;
    Term result;
    if (n->args.empty()) {
      result = leaf(n);
      if (!result)
        result = n;
      else if (result->sort != n->sort)
        throw TermError("substitution changes a sort");
    } else {
      llvm::SmallVector<Term, 4> args;
      for (Term a : n->args)
        args.push_back(visit(a));
      result = rebuild(n, args);
    }
    memo[n] = result;
    return result;
  };
  return visit(root);
}

static const char *opName(Term t) {
  switch (t->op) {
  case TermOp::Const: return "const"; case TermOp::FConst: return "fconst";
  case TermOp::Param: return "param"; case TermOp::State: return "state";
  case TermOp::Shadow: return "shadow"; case TermOp::MemVar: return "mem";
  case TermOp::Add: return "add"; case TermOp::Sub: return "sub";
  case TermOp::Mul: return "mul"; case TermOp::SDiv: return "sdiv";
  case TermOp::UDiv: return "udiv"; case TermOp::SRem: return "srem";
  case TermOp::URem: return "urem"; case TermOp::Shl: return "shl";
  case TermOp::LShr: return "lshr"; case TermOp::AShr: return "ashr";
  case TermOp::And: return "and"; case TermOp::Or: return "or";
  case TermOp::Xor: return "xor"; case TermOp::Not: return "not";
  case TermOp::ICmp: return "icmp"; case TermOp::Ite: return "ite";
  case TermOp::ZExt: return "zext"; case TermOp::SExt: return "sext";
  case TermOp::Trunc: return "trunc"; case TermOp::Load: return "load";
  case TermOp::Store: return "store"; case TermOp::FAdd: return "fadd";
  case TermOp::FMul: return "fmul"; case TermOp::FIsNaN: return "fisnan";
  case TermOp::FIsInf: return "fisinf"; case TermOp::FIsZero: return "fiszero";
  }
  return "?";
}

static std::string head(Term t) {
  static const char *const predicates[] = {"eq",  "ne",  "slt", "sle", "sgt",
                                           "sge", "ult", "ule", "ugt", "uge"};
  std::string s = opName(t);
  if (t->op == TermOp::ICmp)
    s += std::string(".") + predicates[t->aux];
  else if (t->op == TermOp::FAdd || t->op == TermOp::FMul) {
    if (t->aux)
      s += ".fm" + std::to_string(t->aux);
  } else if (t->aux) {
    if (t->aux & FlagNSW) s += ".nsw";
    if (t->aux & FlagNUW) s += ".nuw";
    if (t->aux & FlagExact) s += ".exact";
  }
  s += ":" + t->sort.str();
  if (t->op == TermOp::Const || t->op == TermOp::FConst) {
    llvm::SmallString<32> text;
    t->value.toStringUnsigned(text, 16);
    s += " 0x" + text.str().str();
  } else if (t->op == TermOp::Param || t->op == TermOp::State || t->op == TermOp::Shadow ||
             t->op == TermOp::MemVar) {
    s += " " + std::to_string(t->index);
  }
  return s;
}

namespace mlir::generalized_gemm {

std::string describeNode(Term t) {
  std::string s = head(t);
  for (Term a : t->args)
    s += " t" + std::to_string(a->id);
  return s;
}

std::string formatTerm(Term t, unsigned depthLimit) {
  if (t->args.empty())
    return "(" + head(t) + ")";
  if (depthLimit == 0)
    return "t" + std::to_string(t->id);
  std::string s = "(" + head(t);
  for (Term a : t->args)
    s += " " + formatTerm(a, depthLimit - 1);
  return s + ")";
}

bool containsOp(Term root, TermOp op) {
  llvm::DenseMap<Term, bool> seen;
  std::function<bool(Term)> visit = [&](Term t) {
    if (t->op == op)
      return true;
    auto [it, fresh] = seen.try_emplace(t, false);
    if (!fresh)
      return false;
    for (Term a : t->args)
      if (visit(a))
        return true;
    return false;
  };
  return visit(root);
}

llvm::SmallVector<unsigned> stateSupport(Term root) {
  llvm::DenseMap<Term, bool> seen;
  llvm::SmallVector<unsigned> ids;
  std::function<void(Term)> visit = [&](Term t) {
    if (!seen.try_emplace(t, true).second)
      return;
    if (t->op == TermOp::State)
      ids.push_back(t->index);
    for (Term a : t->args)
      visit(a);
  };
  visit(root);
  std::sort(ids.begin(), ids.end());
  ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
  return ids;
}

APInt evalIntegerBinary(TermOp op, const APInt &a, const APInt &b) {
  unsigned w = a.getBitWidth();
  switch (op) {
  case TermOp::Add: return a + b;
  case TermOp::Sub: return a - b;
  case TermOp::Mul: return a * b;
  case TermOp::And: return a & b;
  case TermOp::Or: return a | b;
  case TermOp::Xor: return a ^ b;
  case TermOp::UDiv: return b.isZero() ? APInt::getAllOnes(w) : a.udiv(b);
  case TermOp::URem: return b.isZero() ? a : a.urem(b);
  case TermOp::SDiv:
    if (b.isZero())
      return a.isNegative() ? APInt(w, 1) : APInt::getAllOnes(w);
    if (a.isMinSignedValue() && b.isAllOnes())
      return a;
    return a.sdiv(b);
  case TermOp::SRem:
    if (b.isZero())
      return a;
    if (a.isMinSignedValue() && b.isAllOnes())
      return APInt::getZero(w);
    return a.srem(b);
  case TermOp::Shl: return b.uge(w) ? APInt::getZero(w) : a.shl(b);
  case TermOp::LShr: return b.uge(w) ? APInt::getZero(w) : a.lshr(b);
  case TermOp::AShr:
    return b.uge(w) ? (a.isNegative() ? APInt::getAllOnes(w) : APInt::getZero(w))
                    : a.ashr(b);
  default:
    throw TermError("not an integer binary operation");
  }
}

bool evalPredicate(Predicate p, const APInt &a, const APInt &b) {
  switch (p) {
  case Predicate::Eq: return a == b;
  case Predicate::Ne: return a != b;
  case Predicate::Slt: return a.slt(b);
  case Predicate::Sle: return a.sle(b);
  case Predicate::Sgt: return a.sgt(b);
  case Predicate::Sge: return a.sge(b);
  case Predicate::Ult: return a.ult(b);
  case Predicate::Ule: return a.ule(b);
  case Predicate::Ugt: return a.ugt(b);
  case Predicate::Uge: return a.uge(b);
  }
  throw TermError("unknown predicate");
}

static const llvm::fltSemantics &semanticsOf(unsigned width) {
  return width == 32 ? llvm::APFloat::IEEEsingle() : llvm::APFloat::IEEEdouble();
}

APInt evalFloatBinary(TermOp op, const APInt &a, const APInt &b) {
  const auto &sem = semanticsOf(a.getBitWidth());
  llvm::APFloat x(sem, a), y(sem, b);
  auto rounding = llvm::RoundingMode::NearestTiesToEven;
  if (op == TermOp::FAdd)
    x.add(y, rounding);
  else if (op == TermOp::FMul)
    x.multiply(y, rounding);
  else
    throw TermError("not a float operation");
  return x.bitcastToAPInt();
}

bool evalFloatClass(TermOp op, const APInt &a) {
  llvm::APFloat x(semanticsOf(a.getBitWidth()), a);
  switch (op) {
  case TermOp::FIsNaN: return x.isNaN();
  case TermOp::FIsInf: return x.isInfinity();
  case TermOp::FIsZero: return x.isZero();
  default: throw TermError("not a float class");
  }
}

} // namespace mlir::generalized_gemm
