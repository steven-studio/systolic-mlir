#include "GeneralizedGemm/IndexProgram.h"

#include <functional>
#include <map>
#include <set>

using namespace llvm;

namespace mlir::generalized_gemm {

static void require(bool ok, const char *what) {
  if (!ok)
    throw TermError(what);
}

unsigned IndexProgram::input(Term term) {
  require(term && term->sort.kind == Sort::BV && term->sort.width > 1,
          "an index program input is an integer term");
  inputs.push_back(term);
  return inputs.size() - 1;
}

unsigned IndexProgram::local(unsigned width) {
  require(width > 1, "a local is an integer");
  localWidths.push_back(width);
  return localWidths.size() - 1;
}

unsigned IndexProgram::constant(const APInt &value) {
  require(value.getBitWidth() > 1, "a constant is an integer");
  exprs.push_back({IndexExprKind::Const, value.getBitWidth(), 0, value, 0, 0, {}});
  return exprs.size() - 1;
}

unsigned IndexProgram::inputExpr(unsigned n) {
  require(n < inputs.size(), "no such input");
  exprs.push_back({IndexExprKind::Input, inputs[n]->sort.width, n, APInt(), 0, 0, {}});
  return exprs.size() - 1;
}

unsigned IndexProgram::localExpr(unsigned n) {
  require(n < localWidths.size(), "no such local");
  exprs.push_back({IndexExprKind::Local, localWidths[n], n, APInt(), 0, 0, {}});
  return exprs.size() - 1;
}

static unsigned binaryExpr(IndexProgram &p, IndexExprKind kind, unsigned a, unsigned b) {
  require(a < p.exprs.size() && b < p.exprs.size(), "no such expression");
  require(p.exprs[a].width == p.exprs[b].width, "operand widths differ");
  p.exprs.push_back({kind, p.exprs[a].width, 0, APInt(), a, b, {}});
  return p.exprs.size() - 1;
}

unsigned IndexProgram::add(unsigned a, unsigned b) { return binaryExpr(*this, IndexExprKind::Add, a, b); }
unsigned IndexProgram::sub(unsigned a, unsigned b) { return binaryExpr(*this, IndexExprKind::Sub, a, b); }
unsigned IndexProgram::mul(unsigned a, unsigned b) { return binaryExpr(*this, IndexExprKind::Mul, a, b); }

unsigned IndexProgram::neg(unsigned a) {
  require(a < exprs.size(), "no such expression");
  exprs.push_back({IndexExprKind::Neg, exprs[a].width, 0, APInt(), a, 0, {}});
  return exprs.size() - 1;
}

static unsigned divisionExpr(IndexProgram &p, IndexExprKind kind, unsigned a, const APInt &divisor) {
  require(a < p.exprs.size(), "no such expression");
  require(divisor.getBitWidth() == p.exprs[a].width, "divisor width differs");
  require(!divisor.isZero(), "a divisor is a nonzero constant");
  p.exprs.push_back({kind, p.exprs[a].width, 0, divisor, a, 0, {}});
  return p.exprs.size() - 1;
}

unsigned IndexProgram::divConst(unsigned a, const APInt &d) { return divisionExpr(*this, IndexExprKind::DivConst, a, d); }
unsigned IndexProgram::modConst(unsigned a, const APInt &d) { return divisionExpr(*this, IndexExprKind::ModConst, a, d); }

static unsigned comparison(IndexProgram &p, IndexCondKind kind, unsigned a, unsigned b) {
  require(a < p.exprs.size() && b < p.exprs.size(), "no such expression");
  require(p.exprs[a].width == p.exprs[b].width, "compared widths differ");
  p.conds.push_back({kind, a, b});
  return p.conds.size() - 1;
}

unsigned IndexProgram::eq(unsigned a, unsigned b) { return comparison(*this, IndexCondKind::Eq, a, b); }
unsigned IndexProgram::ult(unsigned a, unsigned b) { return comparison(*this, IndexCondKind::Ult, a, b); }
unsigned IndexProgram::ule(unsigned a, unsigned b) { return comparison(*this, IndexCondKind::Ule, a, b); }

unsigned IndexProgram::lnot(unsigned a) {
  require(a < conds.size(), "no such condition");
  conds.push_back({IndexCondKind::Not, a, 0});
  return conds.size() - 1;
}

static unsigned connective(IndexProgram &p, IndexCondKind kind, unsigned a, unsigned b) {
  require(a < p.conds.size() && b < p.conds.size(), "no such condition");
  p.conds.push_back({kind, a, b});
  return p.conds.size() - 1;
}

unsigned IndexProgram::land(unsigned a, unsigned b) { return connective(*this, IndexCondKind::And, a, b); }
unsigned IndexProgram::lor(unsigned a, unsigned b) { return connective(*this, IndexCondKind::Or, a, b); }

unsigned IndexProgram::skip() {
  stmts.push_back({IndexStmtKind::Skip, 0, 0, 0, 0, 0, {}});
  return stmts.size() - 1;
}

unsigned IndexProgram::assign(unsigned v, unsigned e) {
  require(v < localWidths.size(), "no such local");
  require(e < exprs.size(), "no such expression");
  require(localWidths[v] == exprs[e].width, "assigned width differs");
  stmts.push_back({IndexStmtKind::Assign, v, e, 0, 0, 0, {}});
  return stmts.size() - 1;
}

unsigned IndexProgram::seq(unsigned a, unsigned b) {
  require(a < stmts.size() && b < stmts.size(), "no such statement");
  stmts.push_back({IndexStmtKind::Seq, 0, 0, 0, a, b, {}});
  return stmts.size() - 1;
}

unsigned IndexProgram::ifThenElse(unsigned c, unsigned a, unsigned b) {
  require(c < conds.size(), "no such condition");
  require(a < stmts.size() && b < stmts.size(), "no such statement");
  stmts.push_back({IndexStmtKind::If, 0, 0, c, a, b, {}});
  return stmts.size() - 1;
}

unsigned IndexProgram::whileDo(unsigned c, unsigned a) {
  require(c < conds.size(), "no such condition");
  require(a < stmts.size(), "no such statement");
  stmts.push_back({IndexStmtKind::While, 0, 0, c, a, 0, {}});
  return stmts.size() - 1;
}

static bool readsLocal(const IndexProgram &p, unsigned n) {
  const IndexProgram::Expr &x = p.exprs.at(n);
  switch (x.kind) {
  case IndexExprKind::Local: return true;
  case IndexExprKind::Input: case IndexExprKind::Const: return false;
  case IndexExprKind::Add: case IndexExprKind::Sub: case IndexExprKind::Mul:
    return readsLocal(p, x.a) || readsLocal(p, x.b);
  case IndexExprKind::Neg: case IndexExprKind::DivConst: case IndexExprKind::ModConst:
    return readsLocal(p, x.a);
  case IndexExprKind::Read:
    return true; // An array's contents are state, not an input.
  }
  return true;
}

unsigned IndexProgram::array(unsigned width, unsigned indexWidth, ArrayRef<unsigned> extents, bool input) {
  require(width > 1 && indexWidth > 1, "an array holds integers and is indexed by integers");
  require(!extents.empty(), "an array has at least one dimension");
  for (unsigned e : extents) {
    require(e < exprs.size(), "no such extent expression");
    require(exprs[e].width == indexWidth, "an extent has the index width");
    require(!readsLocal(*this, e), "an extent is over the inputs alone");
  }
  arrays.push_back({width, indexWidth, std::vector<unsigned>(extents.begin(), extents.end()), input});
  return arrays.size() - 1;
}

static void checkIndices(const IndexProgram &p, unsigned a, ArrayRef<unsigned> indices) {
  require(a < p.arrays.size(), "no such array");
  const IndexProgram::Array &array = p.arrays[a];
  require(indices.size() == array.extents.size(), "an array is indexed to its rank exactly");
  for (unsigned e : indices) {
    require(e < p.exprs.size(), "no such index expression");
    require(p.exprs[e].width == array.indexWidth, "an index has the array's index width");
  }
}

unsigned IndexProgram::read(unsigned a, ArrayRef<unsigned> indices) {
  checkIndices(*this, a, indices);
  Expr x{IndexExprKind::Read, arrays[a].width, a, APInt(), 0, 0, {}};
  x.indices.assign(indices.begin(), indices.end());
  exprs.push_back(std::move(x));
  return exprs.size() - 1;
}

unsigned IndexProgram::write(unsigned a, ArrayRef<unsigned> indices, unsigned value) {
  checkIndices(*this, a, indices);
  require(value < exprs.size(), "no such expression");
  require(exprs[value].width == arrays[a].width, "the written value has the element width");
  Stmt st{IndexStmtKind::Write, a, value, 0, 0, 0, {}};
  st.indices.assign(indices.begin(), indices.end());
  stmts.push_back(std::move(st));
  return stmts.size() - 1;
}

bool IndexProgram::usesArrays() const {
  std::set<unsigned> e, c, s;
  bool found = false;
  std::function<void(unsigned)> expr = [&](unsigned n) {
    if (!e.insert(n).second)
      return;
    const Expr &x = exprs.at(n);
    switch (x.kind) {
    case IndexExprKind::Read: found = true; for (unsigned i : x.indices) expr(i); break;
    case IndexExprKind::Add: case IndexExprKind::Sub: case IndexExprKind::Mul: expr(x.a); expr(x.b); break;
    case IndexExprKind::Neg: case IndexExprKind::DivConst: case IndexExprKind::ModConst: expr(x.a); break;
    default: break;
    }
  };
  std::function<void(unsigned)> cond = [&](unsigned n) {
    if (!c.insert(n).second)
      return;
    const Cond &x = conds.at(n);
    switch (x.kind) {
    case IndexCondKind::Eq: case IndexCondKind::Ult: case IndexCondKind::Ule: expr(x.a); expr(x.b); break;
    case IndexCondKind::Not: cond(x.a); break;
    case IndexCondKind::And: case IndexCondKind::Or: cond(x.a); cond(x.b); break;
    }
  };
  std::function<void(unsigned)> stmt = [&](unsigned n) {
    if (!s.insert(n).second)
      return;
    const Stmt &x = stmts.at(n);
    switch (x.kind) {
    case IndexStmtKind::Skip: break;
    case IndexStmtKind::Assign: expr(x.expr); break;
    case IndexStmtKind::Write: found = true; expr(x.expr); for (unsigned i : x.indices) expr(i); break;
    case IndexStmtKind::Seq: stmt(x.a); stmt(x.b); break;
    case IndexStmtKind::If: cond(x.cond); stmt(x.a); stmt(x.b); break;
    case IndexStmtKind::While: cond(x.cond); stmt(x.a); break;
    }
  };
  stmt(body);
  expr(result);
  return found;
}

bool IndexProgram::loopFree() const {
  std::function<bool(unsigned)> visit = [&](unsigned s) {
    const Stmt &stmt = stmts.at(s);
    switch (stmt.kind) {
    case IndexStmtKind::While: return false;
    case IndexStmtKind::Seq: case IndexStmtKind::If: return visit(stmt.a) && visit(stmt.b);
    default: return true;
    }
  };
  return visit(body);
}

unsigned IndexProgram::size() const {
  std::set<unsigned> e, c, s;
  std::function<void(unsigned)> expr = [&](unsigned n) {
    if (!e.insert(n).second)
      return;
    const Expr &x = exprs.at(n);
    switch (x.kind) {
    case IndexExprKind::Add: case IndexExprKind::Sub: case IndexExprKind::Mul:
      expr(x.a); expr(x.b); break;
    case IndexExprKind::Neg: case IndexExprKind::DivConst: case IndexExprKind::ModConst:
      expr(x.a); break;
    case IndexExprKind::Read:
      for (unsigned i : x.indices) expr(i);
      break;
    default: break;
    }
  };
  std::function<void(unsigned)> cond = [&](unsigned n) {
    if (!c.insert(n).second)
      return;
    const Cond &x = conds.at(n);
    switch (x.kind) {
    case IndexCondKind::Eq: case IndexCondKind::Ult: case IndexCondKind::Ule:
      expr(x.a); expr(x.b); break;
    case IndexCondKind::Not: cond(x.a); break;
    case IndexCondKind::And: case IndexCondKind::Or: cond(x.a); cond(x.b); break;
    }
  };
  std::function<void(unsigned)> stmt = [&](unsigned n) {
    if (!s.insert(n).second)
      return;
    const Stmt &x = stmts.at(n);
    switch (x.kind) {
    case IndexStmtKind::Skip: break;
    case IndexStmtKind::Assign: expr(x.expr); break;
    case IndexStmtKind::Write: expr(x.expr); for (unsigned i : x.indices) expr(i); break;
    case IndexStmtKind::Seq: stmt(x.a); stmt(x.b); break;
    case IndexStmtKind::If: cond(x.cond); stmt(x.a); stmt(x.b); break;
    case IndexStmtKind::While: cond(x.cond); stmt(x.a); break;
    }
  };
  stmt(body);
  expr(result);
  return e.size() + c.size() + s.size();
}

const char *stringifyIndexRunStatus(IndexRunStatus status) {
  switch (status) {
  case IndexRunStatus::Returned: return "RETURNED";
  case IndexRunStatus::OutOfFuel: return "OUT_OF_FUEL";
  case IndexRunStatus::NoRule: return "NO_RULE";
  }
  return "?";
}

namespace {

/// No rule of G.5 applies: thrown inside the interpreter, reported as NoRule.
struct NoRuleApplies {
  std::string why;
};

/// One case per rule of Appendix G.2 and G.5. Sem_E and Sem_B are total on
/// scalars: every operator is total at its width and divisors are nonzero
/// constants. A read of an array may have no rule (G.5): an index outside the
/// extents or an element without a value. Sem_S is partial through while (the
/// fuel cuts) and through a write outside the extents.
class Interpreter {
public:
  Interpreter(const IndexProgram &program, ArrayRef<APInt> inputs, const IndexRunLimits &limits,
              ArrayRef<IndexTable> inputTables)
      : p(program), inputs(inputs), limits(limits), inputTables(inputTables) {
    for (unsigned width : p.localWidths)
      locals.push_back(APInt::getZero(width));
  }

  IndexRun run() {
    IndexRun out;
    try {
      initArrays();
      if (!stmt(p.body)) {
        out.status = IndexRunStatus::OutOfFuel;
        out.steps = steps;
        return out;
      }
      out.value = expr(p.result);
    } catch (const NoRuleApplies &stop) {
      out.status = IndexRunStatus::NoRule;
      out.reason = stop.why;
      out.steps = steps;
      out.writes = writes;
      return out;
    }
    out.status = IndexRunStatus::Returned;
    out.steps = steps;
    out.writes = writes;
    out.finalLocals = locals;
    out.finalArrays = arrays;
    return out;
  }

private:
  /// The extents at the inputs, fixed for the run; an input array takes its
  /// table, which must have those extents.
  void initArrays() {
    for (unsigned a = 0; a < p.arrays.size(); ++a) {
      const IndexProgram::Array &decl = p.arrays[a];
      IndexTable table;
      for (unsigned e : decl.extents)
        table.extents.push_back(expr(e).getZExtValue());
      if (decl.input) {
        if (a >= inputTables.size())
          throw NoRuleApplies{"input array T" + std::to_string(a) + " has no table"};
        if (inputTables[a].extents != table.extents)
          throw NoRuleApplies{"input array T" + std::to_string(a) + ": the table's extents differ"};
        table.elements = inputTables[a].elements;
      }
      arrays.push_back(std::move(table));
    }
  }
  std::vector<uint64_t> indices(unsigned a, ArrayRef<unsigned> exprs, const char *what) {
    const IndexTable &table = arrays[a];
    std::vector<uint64_t> k;
    for (unsigned d = 0; d < exprs.size(); ++d) {
      APInt value = expr(exprs[d]);
      // 0 <= k_d < m_d, unsigned at the index width.
      if (value.getActiveBits() > 64 || value.getZExtValue() >= table.extents[d])
        throw NoRuleApplies{std::string(what) + " of T" + std::to_string(a) + " at index " +
                            std::to_string(value.getZExtValue()) + " in dimension " + std::to_string(d) +
                            ", outside the extent " + std::to_string(table.extents[d])};
      k.push_back(value.getZExtValue());
    }
    return k;
  }
  static std::string tuple(const std::vector<uint64_t> &k) {
    std::string out = "(";
    for (size_t d = 0; d < k.size(); ++d)
      out += (d ? ", " : "") + std::to_string(k[d]);
    return out + ")";
  }
  APInt expr(unsigned n) {
    const IndexProgram::Expr &x = p.exprs[n];
    switch (x.kind) {
    case IndexExprKind::Input: return inputs[x.index];
    case IndexExprKind::Local: return locals[x.index];
    case IndexExprKind::Const: return x.value;
    // Operands left to right, so that the first rule that fails is the same in
    // every build (C++ leaves the order of a + b's operands unspecified).
    case IndexExprKind::Add: { APInt a = expr(x.a), b = expr(x.b); return a + b; }
    case IndexExprKind::Sub: { APInt a = expr(x.a), b = expr(x.b); return a - b; }
    case IndexExprKind::Neg: return -expr(x.a);
    case IndexExprKind::Mul: { APInt a = expr(x.a), b = expr(x.b); return a * b; }
    case IndexExprKind::DivConst: return expr(x.a).udiv(x.value);
    case IndexExprKind::ModConst: return expr(x.a).urem(x.value);
    case IndexExprKind::Read: {
      std::vector<uint64_t> k = indices(x.index, x.indices, "read");
      auto found = arrays[x.index].elements.find(k);
      if (found == arrays[x.index].elements.end())
        throw NoRuleApplies{"read of T" + std::to_string(x.index) + tuple(k) + ", an element without a value"};
      return found->second;
    }
    }
    return APInt();
  }
  bool cond(unsigned n) {
    const IndexProgram::Cond &x = p.conds[n];
    switch (x.kind) {
    case IndexCondKind::Eq: { APInt a = expr(x.a), b = expr(x.b); return a == b; }
    case IndexCondKind::Ult: { APInt a = expr(x.a), b = expr(x.b); return a.ult(b); }
    case IndexCondKind::Ule: { APInt a = expr(x.a), b = expr(x.b); return a.ule(b); }
    case IndexCondKind::Not: return !cond(x.a);
    case IndexCondKind::And: return cond(x.a) && cond(x.b);
    case IndexCondKind::Or: return cond(x.a) || cond(x.b);
    }
    return false;
  }
  bool tick() {
    if (steps >= limits.fuel)
      return false;
    ++steps;
    return true;
  }
  /// False when the fuel ran out: the statement has no output state.
  bool stmt(unsigned n) {
    const IndexProgram::Stmt &x = p.stmts[n];
    switch (x.kind) {
    case IndexStmtKind::Skip:
      return tick();
    case IndexStmtKind::Assign:
      if (!tick())
        return false;
      locals[x.local] = expr(x.expr);
      return true;
    case IndexStmtKind::Write: {
      if (!tick())
        return false;
      // The value and the indices are read in the state before the write (G.5).
      APInt value = expr(x.expr);
      std::vector<uint64_t> k = indices(x.local, x.indices, "write");
      arrays[x.local].elements[k] = value;
      ++writes;
      return true;
    }
    case IndexStmtKind::Seq:
      return stmt(x.a) && stmt(x.b);
    case IndexStmtKind::If:
      if (!tick())
        return false;
      return stmt(cond(x.cond) ? x.a : x.b);
    case IndexStmtKind::While:
      while (true) {
        if (!tick())
          return false;
        if (!cond(x.cond))
          return true;
        if (!stmt(x.a))
          return false;
      }
    }
    return false;
  }

  const IndexProgram &p;
  ArrayRef<APInt> inputs;
  const IndexRunLimits &limits;
  ArrayRef<IndexTable> inputTables;
  std::vector<APInt> locals;
  std::vector<IndexTable> arrays;
  uint64_t steps = 0, writes = 0;
};

} // namespace

IndexRun runIndexProgram(const IndexProgram &program, ArrayRef<APInt> inputs,
                         const IndexRunLimits &limits, ArrayRef<IndexTable> inputTables) {
  if (inputs.size() != program.inputs.size())
    throw TermError("input count differs from the program's inputs");
  for (unsigned n = 0; n < inputs.size(); ++n)
    if (inputs[n].getBitWidth() != program.inputs[n]->sort.width)
      throw TermError("input width differs from the program's input");
  return Interpreter(program, inputs, limits, inputTables).run();
}

namespace {

/// The lowering of Appendix G.2's rules to terms, shared by the whole-program and
/// the statement-level entry points.
struct Lowering {
  Lowering(TermContext &terms, const IndexProgram &p, const ReadLowering &reads = {})
      : terms(terms), p(p), reads(reads) {}
  using Env = std::vector<Term>;
  bool refused = false; // An array write, or a read with no lowering.
  Term expr(unsigned n, const Env &env) {
    const IndexProgram::Expr &x = p.exprs[n];
    switch (x.kind) {
    case IndexExprKind::Read: {
      std::vector<Term> indices;
      for (unsigned i : x.indices) {
        Term t = expr(i, env);
        if (!t)
          return nullptr;
        indices.push_back(t);
      }
      Term read = reads ? reads(x.index, indices) : nullptr;
      if (!read || read->sort != Sort::bv(x.width)) {
        refused = true;
        return nullptr;
      }
      return read;
    }
    case IndexExprKind::Input: return p.inputs[x.index];
    case IndexExprKind::Local: return env[x.index];
    case IndexExprKind::Const: return terms.bv(x.value);
    case IndexExprKind::Add: return binary(TermOp::Add, x, env);
    case IndexExprKind::Sub: return binary(TermOp::Sub, x, env);
    case IndexExprKind::Neg: {
      Term a = expr(x.a, env);
      return a ? terms.binary(TermOp::Sub, terms.bv(APInt::getZero(x.width)), a) : nullptr;
    }
    case IndexExprKind::Mul: return binary(TermOp::Mul, x, env);
    case IndexExprKind::DivConst: {
      Term a = expr(x.a, env);
      return a ? terms.binary(TermOp::UDiv, a, terms.bv(x.value)) : nullptr;
    }
    case IndexExprKind::ModConst: {
      Term a = expr(x.a, env);
      return a ? terms.binary(TermOp::URem, a, terms.bv(x.value)) : nullptr;
    }
    }
    return nullptr;
  }
  Term binary(TermOp op, const IndexProgram::Expr &x, const Env &env) {
    Term a = expr(x.a, env), b = expr(x.b, env);
    return a && b ? terms.binary(op, a, b) : nullptr;
  }
  Term cond(unsigned n, const Env &env) {
    const IndexProgram::Cond &x = p.conds[n];
    auto compare = [&](Predicate predicate) -> Term {
      Term a = expr(x.a, env), b = expr(x.b, env);
      return a && b ? terms.icmp(predicate, a, b) : nullptr;
    };
    switch (x.kind) {
    case IndexCondKind::Eq: return compare(Predicate::Eq);
    case IndexCondKind::Ult: return compare(Predicate::Ult);
    case IndexCondKind::Ule: return compare(Predicate::Ule);
    case IndexCondKind::Not: {
      Term a = cond(x.a, env);
      return a ? terms.logicalNot(a) : nullptr;
    }
    case IndexCondKind::And: case IndexCondKind::Or: {
      Term a = cond(x.a, env), b = cond(x.b, env);
      if (!a || !b)
        return nullptr;
      return x.kind == IndexCondKind::And ? terms.logicalAnd(a, b) : terms.logicalOr(a, b);
    }
    }
    return nullptr;
  }
  Env stmt(unsigned n, Env env) {
    const IndexProgram::Stmt &x = p.stmts[n];
    switch (x.kind) {
    case IndexStmtKind::Skip:
      return env;
    case IndexStmtKind::Assign:
      env[x.local] = expr(x.expr, env);
      return env;
    case IndexStmtKind::Seq:
      return stmt(x.b, stmt(x.a, std::move(env)));
    case IndexStmtKind::If: {
      Term c = cond(x.cond, env);
      Env then = stmt(x.a, env), otherwise = stmt(x.b, env);
      if (!c) {
        refused = true;
        return env;
      }
      for (unsigned v = 0; v < env.size(); ++v)
        env[v] = then[v] == otherwise[v] ? then[v]
                 : (then[v] && otherwise[v]) ? terms.ite(c, then[v], otherwise[v]) : nullptr;
      return env;
    }
    case IndexStmtKind::Write:
      refused = true; // A table is state the term language does not carry.
      return env;
    case IndexStmtKind::While:
      refused = true;
      break;
    }
    return env;
  }
  TermContext &terms;
  const IndexProgram &p;
  ReadLowering reads;
};

bool statementLoopFree(const IndexProgram &p, unsigned s) {
  const IndexProgram::Stmt &stmt = p.stmts.at(s);
  switch (stmt.kind) {
  case IndexStmtKind::While: return false;
  case IndexStmtKind::Seq: case IndexStmtKind::If:
    return statementLoopFree(p, stmt.a) && statementLoopFree(p, stmt.b);
  default: return true;
  }
}

} // namespace

std::optional<Term> indexProgramTerm(TermContext &terms, const IndexProgram &p) {
  if (!p.loopFree() || p.usesArrays())
    return std::nullopt;
  Lowering lowering(terms, p);
  Lowering::Env env;
  for (unsigned width : p.localWidths)
    env.push_back(terms.bv(APInt::getZero(width)));
  Term out = lowering.expr(p.result, lowering.stmt(p.body, std::move(env)));
  if (!out || lowering.refused)
    return std::nullopt;
  return out;
}

std::optional<Term> indexStatementTerm(TermContext &terms, const IndexProgram &p, unsigned stmt,
                                       ArrayRef<Term> locals, unsigned local, const ReadLowering &reads) {
  if (!statementLoopFree(p, stmt) || locals.size() != p.localWidths.size() ||
      local >= p.localWidths.size())
    return std::nullopt;
  for (unsigned v = 0; v < locals.size(); ++v)
    if (!locals[v] || locals[v]->sort != Sort::bv(p.localWidths[v]))
      return std::nullopt;
  Lowering lowering(terms, p, reads);
  Lowering::Env env(locals.begin(), locals.end());
  Term out = lowering.stmt(stmt, std::move(env))[local];
  if (!out || lowering.refused)
    return std::nullopt;
  return out;
}

Term indexExprTerm(TermContext &terms, const IndexProgram &p, unsigned expr, ArrayRef<Term> locals,
                   const ReadLowering &reads) {
  Lowering lowering(terms, p, reads);
  Lowering::Env env(locals.begin(), locals.end());
  Term out = lowering.expr(expr, env);
  return lowering.refused ? nullptr : out;
}

std::optional<CounterTemplate> matchCounterTemplate(const IndexProgram &p, std::string &why) {
  auto fail = [&](const char *reason) {
    why = reason;
    return std::optional<CounterTemplate>();
  };
  // What an expression, condition or statement reads and assigns.
  std::function<void(unsigned, std::set<unsigned> &, std::set<unsigned> &)> exprReads =
      [&](unsigned n, std::set<unsigned> &locals, std::set<unsigned> &inputs) {
        const IndexProgram::Expr &x = p.exprs.at(n);
        switch (x.kind) {
        case IndexExprKind::Input: inputs.insert(x.index); break;
        case IndexExprKind::Local: locals.insert(x.index); break;
        case IndexExprKind::Const: break;
        case IndexExprKind::Add: case IndexExprKind::Sub: case IndexExprKind::Mul:
          exprReads(x.a, locals, inputs); exprReads(x.b, locals, inputs); break;
        case IndexExprKind::Neg: case IndexExprKind::DivConst: case IndexExprKind::ModConst:
          exprReads(x.a, locals, inputs); break;
        case IndexExprKind::Read:
          for (unsigned i : x.indices) exprReads(i, locals, inputs);
          break;
        }
      };
  std::function<void(unsigned, std::set<unsigned> &, std::set<unsigned> &)> condReads =
      [&](unsigned n, std::set<unsigned> &locals, std::set<unsigned> &inputs) {
        const IndexProgram::Cond &x = p.conds.at(n);
        switch (x.kind) {
        case IndexCondKind::Eq: case IndexCondKind::Ult: case IndexCondKind::Ule:
          exprReads(x.a, locals, inputs); exprReads(x.b, locals, inputs); break;
        case IndexCondKind::Not: condReads(x.a, locals, inputs); break;
        case IndexCondKind::And: case IndexCondKind::Or:
          condReads(x.a, locals, inputs); condReads(x.b, locals, inputs); break;
        }
      };
  std::function<void(unsigned, std::set<unsigned> &, std::set<unsigned> &, std::set<unsigned> &)>
      stmtUses = [&](unsigned n, std::set<unsigned> &reads, std::set<unsigned> &inputs,
                     std::set<unsigned> &writes) {
        const IndexProgram::Stmt &x = p.stmts.at(n);
        switch (x.kind) {
        case IndexStmtKind::Skip: break;
        case IndexStmtKind::Assign: writes.insert(x.local); exprReads(x.expr, reads, inputs); break;
        case IndexStmtKind::Write:
          exprReads(x.expr, reads, inputs);
          for (unsigned i : x.indices) exprReads(i, reads, inputs);
          break;
        case IndexStmtKind::Seq: stmtUses(x.a, reads, inputs, writes); stmtUses(x.b, reads, inputs, writes); break;
        case IndexStmtKind::If:
          condReads(x.cond, reads, inputs);
          stmtUses(x.a, reads, inputs, writes); stmtUses(x.b, reads, inputs, writes); break;
        case IndexStmtKind::While:
          condReads(x.cond, reads, inputs); stmtUses(x.a, reads, inputs, writes); break;
        }
      };
  if (p.stmts.empty() || p.exprs.empty())
    return fail("an empty program");
  if (p.usesArrays())
    return fail("the program reads or writes an array: not the counter template (see TableTemplate)");
  // P = (r := 0 ; v := init) ; while B do S'.
  const IndexProgram::Stmt &top = p.stmts.at(p.body);
  if (top.kind != IndexStmtKind::Seq)
    return fail("the body is not a sequence");
  const IndexProgram::Stmt &start = p.stmts.at(top.a), &loop = p.stmts.at(top.b);
  if (start.kind != IndexStmtKind::Seq || loop.kind != IndexStmtKind::While)
    return fail("the body is not (init ; while)");
  const IndexProgram::Stmt &zero = p.stmts.at(start.a), &init = p.stmts.at(start.b);
  if (zero.kind != IndexStmtKind::Assign || init.kind != IndexStmtKind::Assign)
    return fail("the start is not two assignments");
  CounterTemplate out;
  out.counter = zero.local;
  out.value = init.local;
  out.init = init.expr;
  if (out.counter == out.value)
    return fail("the counter and the value are one local");
  const IndexProgram::Expr &zeroExpr = p.exprs.at(zero.expr);
  if (zeroExpr.kind != IndexExprKind::Const || !zeroExpr.value.isZero())
    return fail("the counter does not start at 0");
  {
    std::set<unsigned> locals, inputs;
    exprReads(out.init, locals, inputs);
    if (!locals.empty())
      return fail("init reads a local");
  }
  // B = r < m with m an input.
  const IndexProgram::Cond &test = p.conds.at(loop.cond);
  if (test.kind != IndexCondKind::Ult)
    return fail("the loop test is not r < m");
  const IndexProgram::Expr &lhs = p.exprs.at(test.a), &rhs = p.exprs.at(test.b);
  if (lhs.kind != IndexExprKind::Local || lhs.index != out.counter || rhs.kind != IndexExprKind::Input)
    return fail("the loop test is not r < m");
  out.bound = rhs.index;
  // S' = S ; r := r + 1.
  const IndexProgram::Stmt &step = p.stmts.at(loop.a);
  if (step.kind != IndexStmtKind::Seq)
    return fail("the loop body is not (S ; r := r + 1)");
  const IndexProgram::Stmt &bump = p.stmts.at(step.b);
  if (bump.kind != IndexStmtKind::Assign || bump.local != out.counter)
    return fail("the loop body does not end with r := r + 1");
  const IndexProgram::Expr &bumpExpr = p.exprs.at(bump.expr);
  bool plusOne = bumpExpr.kind == IndexExprKind::Add &&
                 ((p.exprs.at(bumpExpr.a).kind == IndexExprKind::Local && p.exprs.at(bumpExpr.a).index == out.counter &&
                   p.exprs.at(bumpExpr.b).kind == IndexExprKind::Const && p.exprs.at(bumpExpr.b).value.isOne()) ||
                  (p.exprs.at(bumpExpr.b).kind == IndexExprKind::Local && p.exprs.at(bumpExpr.b).index == out.counter &&
                   p.exprs.at(bumpExpr.a).kind == IndexExprKind::Const && p.exprs.at(bumpExpr.a).value.isOne()));
  if (!plusOne)
    return fail("the loop body does not end with r := r + 1");
  out.body = step.a;
  if (!statementLoopFree(p, out.body))
    return fail("S has a while");
  std::set<unsigned> reads, inputs, writes;
  stmtUses(out.body, reads, inputs, writes);
  for (unsigned w : writes)
    if (w != out.value)
      return fail("S assigns a local other than v");
  if (inputs.count(out.bound))
    return fail("S reads the bound m");
  // return v.
  const IndexProgram::Expr &result = p.exprs.at(p.result);
  if (result.kind != IndexExprKind::Local || result.index != out.value)
    return fail("the program does not return v");
  return out;
}

std::optional<TableTemplate> matchTableTemplate(const IndexProgram &p, std::string &why) {
  auto fail = [&](const std::string &reason) {
    why = reason;
    return std::optional<TableTemplate>();
  };
  if (p.arrays.size() != 1)
    return fail("the template has exactly one array");
  const IndexProgram::Array &array = p.arrays[0];
  if (array.input || array.extents.size() != 1)
    return fail("the template's array is a non-input array of rank 1");
  if (p.stmts.empty() || p.exprs.empty())
    return fail("an empty program");
  // Flatten the top-level sequence into statements in order.
  std::vector<unsigned> top;
  std::function<void(unsigned)> flatten = [&](unsigned n) {
    const IndexProgram::Stmt &x = p.stmts.at(n);
    if (x.kind == IndexStmtKind::Seq) {
      flatten(x.a);
      flatten(x.b);
    } else {
      top.push_back(n);
    }
  };
  flatten(p.body);
  TableTemplate out;
  out.array = 0;
  // What an expression reads.
  struct Uses {
    std::set<unsigned> locals, inputs;
    std::vector<std::vector<unsigned>> reads; // Index expressions of each array read.
  };
  std::function<void(unsigned, Uses &)> exprUses = [&](unsigned n, Uses &u) {
    const IndexProgram::Expr &x = p.exprs.at(n);
    switch (x.kind) {
    case IndexExprKind::Input: u.inputs.insert(x.index); break;
    case IndexExprKind::Local: u.locals.insert(x.index); break;
    case IndexExprKind::Const: break;
    case IndexExprKind::Add: case IndexExprKind::Sub: case IndexExprKind::Mul:
      exprUses(x.a, u); exprUses(x.b, u); break;
    case IndexExprKind::Neg: case IndexExprKind::DivConst: case IndexExprKind::ModConst:
      exprUses(x.a, u); break;
    case IndexExprKind::Read:
      u.reads.push_back(x.indices);
      for (unsigned i : x.indices) exprUses(i, u);
      break;
    }
  };
  auto isConst = [&](unsigned e, uint64_t value) {
    const IndexProgram::Expr &x = p.exprs.at(e);
    return x.kind == IndexExprKind::Const && x.value == value;
  };
  // Init writes T[0] := k_0, ..., T[d-1] := k_{d-1}, in order, over inputs alone.
  size_t at = 0;
  for (; at < top.size(); ++at) {
    const IndexProgram::Stmt &x = p.stmts[top[at]];
    if (x.kind != IndexStmtKind::Write)
      break;
    if (x.local != 0 || x.indices.size() != 1 || !isConst(x.indices[0], out.inits.size()))
      return fail("init write " + std::to_string(out.inits.size()) + " is not T[" +
                  std::to_string(out.inits.size()) + "] := k");
    Uses u;
    exprUses(x.expr, u);
    if (!u.locals.empty() || !u.reads.empty())
      return fail("an init value reads a local or the table");
    out.inits.push_back(x.expr);
  }
  if (out.inits.empty())
    return fail("no init write");
  unsigned d = out.inits.size();
  // r := d.
  if (at >= top.size() || p.stmts[top[at]].kind != IndexStmtKind::Assign)
    return fail("no r := d after the init writes");
  const IndexProgram::Stmt &start = p.stmts[top[at]];
  if (!isConst(start.expr, d))
    return fail("the counter does not start at the depth d");
  out.counter = start.local;
  ++at;
  // while r < m + 1 do (T[r] := E ; r := r + 1).
  if (at >= top.size() || p.stmts[top[at]].kind != IndexStmtKind::While)
    return fail("no while after r := d");
  const IndexProgram::Stmt &loop = p.stmts[top[at]];
  const IndexProgram::Cond &test = p.conds.at(loop.cond);
  if (test.kind != IndexCondKind::Ult)
    return fail("the loop test is not r < m + 1");
  const IndexProgram::Expr &lhs = p.exprs.at(test.a), &rhs = p.exprs.at(test.b);
  if (lhs.kind != IndexExprKind::Local || lhs.index != out.counter || rhs.kind != IndexExprKind::Add)
    return fail("the loop test is not r < m + 1");
  const IndexProgram::Expr &ra = p.exprs.at(rhs.a), &rb = p.exprs.at(rhs.b);
  bool mPlusOne = (ra.kind == IndexExprKind::Input && isConst(rhs.b, 1)) ||
                  (rb.kind == IndexExprKind::Input && isConst(rhs.a, 1));
  if (!mPlusOne)
    return fail("the loop test is not r < m + 1");
  out.bound = ra.kind == IndexExprKind::Input ? ra.index : rb.index;
  // The extent of T is m + d: room for the d init writes at every m, and for T[m].
  {
    const IndexProgram::Expr &ext = p.exprs.at(array.extents[0]);
    bool same = ext.kind == IndexExprKind::Add &&
                ((p.exprs.at(ext.a).kind == IndexExprKind::Input && p.exprs.at(ext.a).index == out.bound && isConst(ext.b, d)) ||
                 (p.exprs.at(ext.b).kind == IndexExprKind::Input && p.exprs.at(ext.b).index == out.bound && isConst(ext.a, d)));
    if (!same)
      return fail("the extent of T is not m + d");
  }
  const IndexProgram::Stmt &step = p.stmts.at(loop.a);
  if (step.kind != IndexStmtKind::Seq)
    return fail("the loop body is not (T[r] := E ; r := r + 1)");
  const IndexProgram::Stmt &put = p.stmts.at(step.a), &bump = p.stmts.at(step.b);
  if (put.kind != IndexStmtKind::Write || put.local != 0 || put.indices.size() != 1 ||
      p.exprs.at(put.indices[0]).kind != IndexExprKind::Local || p.exprs.at(put.indices[0]).index != out.counter)
    return fail("the loop body does not write T[r]");
  if (bump.kind != IndexStmtKind::Assign || bump.local != out.counter)
    return fail("the loop body does not end with r := r + 1");
  const IndexProgram::Expr &bumpExpr = p.exprs.at(bump.expr);
  bool plusOne = bumpExpr.kind == IndexExprKind::Add &&
                 ((p.exprs.at(bumpExpr.a).kind == IndexExprKind::Local && p.exprs.at(bumpExpr.a).index == out.counter && isConst(bumpExpr.b, 1)) ||
                  (p.exprs.at(bumpExpr.b).kind == IndexExprKind::Local && p.exprs.at(bumpExpr.b).index == out.counter && isConst(bumpExpr.a, 1)));
  if (!plusOne)
    return fail("the loop body does not end with r := r + 1");
  out.value = put.expr;
  // E reads r, inputs other than m, constants, and T[r - j] for 1 <= j <= d only.
  {
    Uses u;
    exprUses(out.value, u);
    for (unsigned l : u.locals)
      if (l != out.counter)
        return fail("E reads a local other than r");
    if (u.inputs.count(out.bound))
      return fail("E reads the bound m");
    for (const std::vector<unsigned> &k : u.reads) {
      if (k.size() != 1)
        return fail("a read has the wrong rank");
      const IndexProgram::Expr &ix = p.exprs.at(k[0]);
      bool history = ix.kind == IndexExprKind::Sub && p.exprs.at(ix.a).kind == IndexExprKind::Local &&
                     p.exprs.at(ix.a).index == out.counter && p.exprs.at(ix.b).kind == IndexExprKind::Const &&
                     !p.exprs.at(ix.b).value.isZero() && p.exprs.at(ix.b).value.ule(d);
      if (!history)
        return fail("E reads T elsewhere than T[r - j] with 1 <= j <= d");
    }
  }
  ++at;
  if (at != top.size())
    return fail("statements after the loop");
  // return T[m].
  const IndexProgram::Expr &result = p.exprs.at(p.result);
  if (result.kind != IndexExprKind::Read || result.index != 0 || result.indices.size() != 1 ||
      p.exprs.at(result.indices[0]).kind != IndexExprKind::Input || p.exprs.at(result.indices[0]).index != out.bound)
    return fail("the program does not return T[m]");
  return out;
}

std::string formatIndexProgram(const IndexProgram &p,
                               const std::function<std::string(Term)> &inputName) {
  std::function<std::string(unsigned)> expr = [&](unsigned n) -> std::string {
    const IndexProgram::Expr &x = p.exprs[n];
    auto decimal = [](const APInt &v) { return std::to_string(v.getZExtValue()); };
    switch (x.kind) {
    case IndexExprKind::Input:
      return inputName ? inputName(p.inputs[x.index]) : "x" + std::to_string(x.index);
    case IndexExprKind::Local: return "v" + std::to_string(x.index);
    case IndexExprKind::Const: return decimal(x.value);
    case IndexExprKind::Add: return "(" + expr(x.a) + " + " + expr(x.b) + ")";
    case IndexExprKind::Sub: return "(" + expr(x.a) + " - " + expr(x.b) + ")";
    case IndexExprKind::Neg: return "(-" + expr(x.a) + ")";
    case IndexExprKind::Mul: return "(" + expr(x.a) + " * " + expr(x.b) + ")";
    case IndexExprKind::DivConst: return "(" + expr(x.a) + " div " + decimal(x.value) + ")";
    case IndexExprKind::ModConst: return "(" + expr(x.a) + " mod " + decimal(x.value) + ")";
    case IndexExprKind::Read: {
      std::string out = "T" + std::to_string(x.index);
      for (unsigned i : x.indices)
        out += "[" + expr(i) + "]";
      return out;
    }
    }
    return "?";
  };
  std::function<std::string(unsigned)> cond = [&](unsigned n) -> std::string {
    const IndexProgram::Cond &x = p.conds[n];
    switch (x.kind) {
    case IndexCondKind::Eq: return expr(x.a) + " = " + expr(x.b);
    case IndexCondKind::Ult: return expr(x.a) + " < " + expr(x.b);
    case IndexCondKind::Ule: return expr(x.a) + " <= " + expr(x.b);
    case IndexCondKind::Not: return "not (" + cond(x.a) + ")";
    case IndexCondKind::And: return "(" + cond(x.a) + ") and (" + cond(x.b) + ")";
    case IndexCondKind::Or: return "(" + cond(x.a) + ") or (" + cond(x.b) + ")";
    }
    return "?";
  };
  std::function<std::string(unsigned)> stmt = [&](unsigned n) -> std::string {
    const IndexProgram::Stmt &x = p.stmts[n];
    switch (x.kind) {
    case IndexStmtKind::Skip: return "skip";
    case IndexStmtKind::Assign: return "v" + std::to_string(x.local) + " := " + expr(x.expr);
    case IndexStmtKind::Write: {
      std::string out = "T" + std::to_string(x.local);
      for (unsigned i : x.indices)
        out += "[" + expr(i) + "]";
      return out + " := " + expr(x.expr);
    }
    case IndexStmtKind::Seq: return stmt(x.a) + " ; " + stmt(x.b);
    case IndexStmtKind::If:
      return "if " + cond(x.cond) + " then (" + stmt(x.a) + ") else (" + stmt(x.b) + ")";
    case IndexStmtKind::While: return "while " + cond(x.cond) + " do (" + stmt(x.a) + ")";
    }
    return "?";
  };
  std::string decls;
  for (unsigned a = 0; a < p.arrays.size(); ++a) {
    decls += std::string(p.arrays[a].input ? "input " : "") + "array T" + std::to_string(a);
    for (unsigned e : p.arrays[a].extents)
      decls += "[" + expr(e) + "]";
    decls += " ; ";
  }
  return decls + stmt(p.body) + " ; return " + expr(p.result);
}

} // namespace mlir::generalized_gemm
