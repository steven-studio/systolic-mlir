#include "GeneralizedGemm/IndexProgramSemgus.h"

#include <functional>
#include <map>
#include <set>

using namespace llvm;

namespace mlir::generalized_gemm {

static std::string literal(const APInt &value) {
  return "(_ bv" + std::to_string(value.getZExtValue()) + " " +
         std::to_string(value.getBitWidth()) + ")";
}

std::string semgusTerm(const IndexProgram &p) {
  std::function<std::string(unsigned)> expr = [&](unsigned n) -> std::string {
    const IndexProgram::Expr &x = p.exprs[n];
    switch (x.kind) {
    case IndexExprKind::Input: return "x" + std::to_string(x.index);
    case IndexExprKind::Local: return "v" + std::to_string(x.index);
    case IndexExprKind::Const: return "(const " + literal(x.value) + ")";
    case IndexExprKind::Add: return "(add " + expr(x.a) + " " + expr(x.b) + ")";
    case IndexExprKind::Sub: return "(sub " + expr(x.a) + " " + expr(x.b) + ")";
    case IndexExprKind::Neg: return "(neg " + expr(x.a) + ")";
    case IndexExprKind::Mul: return "(mul " + expr(x.a) + " " + expr(x.b) + ")";
    case IndexExprKind::DivConst: return "(div " + expr(x.a) + " " + literal(x.value) + ")";
    case IndexExprKind::ModConst: return "(mod " + expr(x.a) + " " + literal(x.value) + ")";
    case IndexExprKind::Read: {
      std::string out = "(read T" + std::to_string(x.index);
      for (unsigned i : x.indices)
        out += " " + expr(i);
      return out + ")";
    }
    }
    return "?";
  };
  std::function<std::string(unsigned)> cond = [&](unsigned n) -> std::string {
    const IndexProgram::Cond &x = p.conds[n];
    switch (x.kind) {
    case IndexCondKind::Eq: return "(eq " + expr(x.a) + " " + expr(x.b) + ")";
    case IndexCondKind::Ult: return "(ult " + expr(x.a) + " " + expr(x.b) + ")";
    case IndexCondKind::Ule: return "(ule " + expr(x.a) + " " + expr(x.b) + ")";
    case IndexCondKind::Not: return "(not " + cond(x.a) + ")";
    case IndexCondKind::And: return "(and " + cond(x.a) + " " + cond(x.b) + ")";
    case IndexCondKind::Or: return "(or " + cond(x.a) + " " + cond(x.b) + ")";
    }
    return "?";
  };
  std::function<std::string(unsigned)> stmt = [&](unsigned n) -> std::string {
    const IndexProgram::Stmt &x = p.stmts[n];
    switch (x.kind) {
    case IndexStmtKind::Skip: return "skip";
    case IndexStmtKind::Assign:
      return "(assign v" + std::to_string(x.local) + " " + expr(x.expr) + ")";
    case IndexStmtKind::Write: {
      std::string out = "(write T" + std::to_string(x.local);
      for (unsigned i : x.indices)
        out += " " + expr(i);
      return out + " " + expr(x.expr) + ")";
    }
    case IndexStmtKind::Seq: return "(seq " + stmt(x.a) + " " + stmt(x.b) + ")";
    case IndexStmtKind::If:
      return "(if " + cond(x.cond) + " " + stmt(x.a) + " " + stmt(x.b) + ")";
    case IndexStmtKind::While: return "(while " + cond(x.cond) + " " + stmt(x.a) + ")";
    }
    return "?";
  };
  return "(return " + stmt(p.body) + " " + expr(p.result) + ")";
}

namespace {

struct SExpr {
  std::string atom;           // Nonempty for an atom.
  std::vector<SExpr> list;    // Otherwise the children.
  bool isAtom() const { return !atom.empty(); }
};

class Reader {
public:
  explicit Reader(const std::string &text) : text(text) {}
  std::optional<SExpr> read(std::string &why) {
    skip();
    auto out = form(why);
    if (!out)
      return std::nullopt;
    skip();
    if (pos != text.size()) {
      why = "text after the term";
      return std::nullopt;
    }
    return out;
  }

private:
  void skip() {
    while (pos < text.size() && isspace((unsigned char)text[pos]))
      ++pos;
  }
  std::optional<SExpr> form(std::string &why) {
    skip();
    if (pos >= text.size()) {
      why = "unexpected end";
      return std::nullopt;
    }
    SExpr out;
    if (text[pos] == '(') {
      ++pos;
      while (true) {
        skip();
        if (pos >= text.size()) {
          why = "unbalanced parenthesis";
          return std::nullopt;
        }
        if (text[pos] == ')') {
          ++pos;
          break;
        }
        auto child = form(why);
        if (!child)
          return std::nullopt;
        out.list.push_back(std::move(*child));
      }
      if (out.list.empty()) {
        why = "empty list";
        return std::nullopt;
      }
      return out;
    }
    if (text[pos] == ')') {
      why = "unexpected parenthesis";
      return std::nullopt;
    }
    size_t start = pos;
    while (pos < text.size() && !isspace((unsigned char)text[pos]) && text[pos] != '(' &&
           text[pos] != ')')
      ++pos;
    out.atom = text.substr(start, pos - start);
    return out;
  }
  const std::string &text;
  size_t pos = 0;
};

class Translator {
public:
  Translator(IndexProgram &p, std::string &why) : p(p), why(why) {}

  bool program(const SExpr &s) {
    if (!(s.list.size() == 3 && s.list[0].isAtom() && s.list[0].atom == "return"))
      return fail("a program is (return S E)");
    auto body = stmt(s.list[1]);
    auto result = expr(s.list[2]);
    if (!body || !result)
      return false;
    p.body = *body;
    p.result = *result;
    return true;
  }

private:
  bool fail(const std::string &what) {
    if (why.empty())
      why = what;
    return false;
  }
  std::optional<unsigned> none(const std::string &what) {
    fail(what);
    return std::nullopt;
  }
  template <typename F> std::optional<unsigned> guarded(F &&build) {
    try {
      return build();
    } catch (const TermError &error) {
      return none(error.what());
    }
  }
  std::optional<APInt> lit(const SExpr &s) {
    // (_ bvN W)
    if (s.list.size() != 3 || !s.list[0].isAtom() || s.list[0].atom != "_" ||
        !s.list[1].isAtom() || !s.list[2].isAtom() || s.list[1].atom.rfind("bv", 0) != 0) {
      fail("a literal is (_ bvN W)");
      return std::nullopt;
    }
    std::string digits = s.list[1].atom.substr(2);
    if (digits.empty() || digits.find_first_not_of("0123456789") != std::string::npos ||
        s.list[2].atom.find_first_not_of("0123456789") != std::string::npos) {
      fail("a literal is (_ bvN W)");
      return std::nullopt;
    }
    unsigned width = std::stoul(s.list[2].atom);
    if (width == 0 || width > 64 || digits.size() > 20) {
      fail("literal width or value out of range");
      return std::nullopt;
    }
    uint64_t value = std::stoull(digits);
    if (width < 64 && (value >> width) != 0) {
      fail("literal does not fit its width");
      return std::nullopt;
    }
    return APInt(width, value);
  }
  std::optional<unsigned> expr(const SExpr &s) {
    if (s.isAtom()) {
      if (s.atom.size() > 1 && (s.atom[0] == 'x' || s.atom[0] == 'v') &&
          s.atom.find_first_not_of("0123456789", 1) == std::string::npos) {
        unsigned n = std::stoul(s.atom.substr(1));
        if (s.atom[0] == 'x')
          return n < p.inputs.size() ? guarded([&] { return p.inputExpr(n); }) : none("no such input");
        return n < p.localWidths.size() ? guarded([&] { return p.localExpr(n); }) : none("no such local");
      }
      return none("unknown expression atom " + s.atom);
    }
    const std::string &head = s.list[0].atom;
    if (head == "const" && s.list.size() == 2) {
      auto value = lit(s.list[1]);
      if (!value)
        return std::nullopt;
      return guarded([&] { return p.constant(*value); });
    }
    if ((head == "add" || head == "sub" || head == "mul") && s.list.size() == 3) {
      auto a = expr(s.list[1]), b = expr(s.list[2]);
      if (!a || !b)
        return std::nullopt;
      return guarded([&] { return head == "add" ? p.add(*a, *b) : head == "sub" ? p.sub(*a, *b) : p.mul(*a, *b); });
    }
    if (head == "neg" && s.list.size() == 2) {
      auto a = expr(s.list[1]);
      if (!a)
        return std::nullopt;
      return guarded([&] { return p.neg(*a); });
    }
    if ((head == "div" || head == "mod") && s.list.size() == 3) {
      auto a = expr(s.list[1]);
      auto d = lit(s.list[2]);
      if (!a || !d)
        return std::nullopt;
      return guarded([&] { return head == "div" ? p.divConst(*a, *d) : p.modConst(*a, *d); });
    }
    if (head == "read" && s.list.size() >= 3 && s.list[1].isAtom()) {
      auto array = arrayOf(s.list[1].atom);
      if (!array)
        return std::nullopt;
      std::vector<unsigned> indices;
      for (size_t n = 2; n < s.list.size(); ++n) {
        auto i = expr(s.list[n]);
        if (!i)
          return std::nullopt;
        indices.push_back(*i);
      }
      return guarded([&] { return p.read(*array, indices); });
    }
    return none("unknown expression form " + head);
  }
  std::optional<unsigned> arrayOf(const std::string &name) {
    if (name.size() < 2 || name[0] != 'T' || name.find_first_not_of("0123456789", 1) != std::string::npos)
      return none("not an array name: " + name);
    unsigned a = std::stoul(name.substr(1));
    if (a >= p.arrays.size())
      return none("no such array " + name);
    return a;
  }
  std::optional<unsigned> cond(const SExpr &s) {
    if (s.isAtom())
      return none("a condition is a list");
    const std::string &head = s.list[0].atom;
    if ((head == "eq" || head == "ult" || head == "ule") && s.list.size() == 3) {
      auto a = expr(s.list[1]), b = expr(s.list[2]);
      if (!a || !b)
        return std::nullopt;
      return guarded([&] { return head == "eq" ? p.eq(*a, *b) : head == "ult" ? p.ult(*a, *b) : p.ule(*a, *b); });
    }
    if (head == "not" && s.list.size() == 2) {
      auto a = cond(s.list[1]);
      if (!a)
        return std::nullopt;
      return guarded([&] { return p.lnot(*a); });
    }
    if ((head == "and" || head == "or") && s.list.size() == 3) {
      auto a = cond(s.list[1]), b = cond(s.list[2]);
      if (!a || !b)
        return std::nullopt;
      return guarded([&] { return head == "and" ? p.land(*a, *b) : p.lor(*a, *b); });
    }
    return none("unknown condition form " + head);
  }
  std::optional<unsigned> stmt(const SExpr &s) {
    if (s.isAtom()) {
      if (s.atom == "skip")
        return p.skip();
      return none("unknown statement atom " + s.atom);
    }
    const std::string &head = s.list[0].atom;
    if (head == "assign" && s.list.size() == 3 && s.list[1].isAtom()) {
      const std::string &name = s.list[1].atom;
      if (name.size() < 2 || name[0] != 'v' ||
          name.find_first_not_of("0123456789", 1) != std::string::npos)
        return none("assignment to a non-local");
      unsigned v = std::stoul(name.substr(1));
      auto e = expr(s.list[2]);
      if (!e)
        return std::nullopt;
      return guarded([&] { return p.assign(v, *e); });
    }
    if (head == "write" && s.list.size() >= 4 && s.list[1].isAtom()) {
      auto array = arrayOf(s.list[1].atom);
      if (!array)
        return std::nullopt;
      std::vector<unsigned> indices;
      for (size_t n = 2; n + 1 < s.list.size(); ++n) {
        auto i = expr(s.list[n]);
        if (!i)
          return std::nullopt;
        indices.push_back(*i);
      }
      auto value = expr(s.list.back());
      if (!value)
        return std::nullopt;
      return guarded([&] { return p.write(*array, indices, *value); });
    }
    if (head == "seq" && s.list.size() == 3) {
      auto a = stmt(s.list[1]), b = stmt(s.list[2]);
      if (!a || !b)
        return std::nullopt;
      return guarded([&] { return p.seq(*a, *b); });
    }
    if (head == "if" && s.list.size() == 4) {
      auto c = cond(s.list[1]);
      auto a = stmt(s.list[2]), b = stmt(s.list[3]);
      if (!c || !a || !b)
        return std::nullopt;
      return guarded([&] { return p.ifThenElse(*c, *a, *b); });
    }
    if (head == "while" && s.list.size() == 3) {
      auto c = cond(s.list[1]);
      auto a = stmt(s.list[2]);
      if (!c || !a)
        return std::nullopt;
      return guarded([&] { return p.whileDo(*c, *a); });
    }
    return none("unknown statement form " + head);
  }

  IndexProgram &p;
  std::string &why;
};

} // namespace

std::optional<IndexProgram> parseSemgusTerm(const std::string &text, ArrayRef<Term> inputs,
                                            ArrayRef<unsigned> localWidths, std::string &why) {
  why.clear();
  auto form = Reader(text).read(why);
  if (!form)
    return std::nullopt;
  IndexProgram p;
  try {
    for (Term input : inputs)
      p.input(input);
    for (unsigned width : localWidths)
      p.local(width);
  } catch (const TermError &error) {
    why = error.what();
    return std::nullopt;
  }
  if (!Translator(p, why).program(*form))
    return std::nullopt;
  return p;
}

std::optional<IndexProgram> parseSemgusTerm(const std::string &text, const IndexProgram &skeleton,
                                            std::string &why) {
  why.clear();
  auto form = Reader(text).read(why);
  if (!form)
    return std::nullopt;
  IndexProgram p;
  p.inputs = skeleton.inputs;
  p.localWidths = skeleton.localWidths;
  // The arrays' extent expressions are copied with their nodes; nothing else is.
  std::map<unsigned, unsigned> copied;
  std::function<unsigned(unsigned)> copyExpr = [&](unsigned n) -> unsigned {
    auto found = copied.find(n);
    if (found != copied.end())
      return found->second;
    const IndexProgram::Expr &x = skeleton.exprs.at(n);
    unsigned out;
    switch (x.kind) {
    case IndexExprKind::Input: out = p.inputExpr(x.index); break;
    case IndexExprKind::Const: out = p.constant(x.value); break;
    case IndexExprKind::Add: out = p.add(copyExpr(x.a), copyExpr(x.b)); break;
    case IndexExprKind::Sub: out = p.sub(copyExpr(x.a), copyExpr(x.b)); break;
    case IndexExprKind::Mul: out = p.mul(copyExpr(x.a), copyExpr(x.b)); break;
    case IndexExprKind::Neg: out = p.neg(copyExpr(x.a)); break;
    case IndexExprKind::DivConst: out = p.divConst(copyExpr(x.a), x.value); break;
    case IndexExprKind::ModConst: out = p.modConst(copyExpr(x.a), x.value); break;
    default: throw TermError("an extent reads a local or an array");
    }
    copied[n] = out;
    return out;
  };
  try {
    for (const IndexProgram::Array &array : skeleton.arrays) {
      std::vector<unsigned> extents;
      for (unsigned e : array.extents)
        extents.push_back(copyExpr(e));
      p.array(array.width, array.indexWidth, extents, array.input);
    }
  } catch (const TermError &error) {
    why = error.what();
    return std::nullopt;
  }
  if (!Translator(p, why).program(*form))
    return std::nullopt;
  return p;
}

std::vector<std::string> semgusConstructors(const IndexProgram &p) {
  std::set<std::string> out;
  std::set<unsigned> e, c, s;
  std::function<void(unsigned)> expr = [&](unsigned n) {
    if (!e.insert(n).second)
      return;
    const IndexProgram::Expr &x = p.exprs.at(n);
    switch (x.kind) {
    case IndexExprKind::Input: out.insert("$x" + std::to_string(x.index)); break;
    case IndexExprKind::Local: out.insert("$v" + std::to_string(x.index)); break;
    case IndexExprKind::Const: out.insert("$const_" + std::to_string(x.value.getZExtValue())); break;
    case IndexExprKind::Add: out.insert("$add"); expr(x.a); expr(x.b); break;
    case IndexExprKind::Sub: out.insert("$sub"); expr(x.a); expr(x.b); break;
    case IndexExprKind::Mul: out.insert("$mul"); expr(x.a); expr(x.b); break;
    case IndexExprKind::Neg: out.insert("$neg"); expr(x.a); break;
    case IndexExprKind::DivConst: out.insert("$div_" + std::to_string(x.value.getZExtValue())); expr(x.a); break;
    case IndexExprKind::ModConst: out.insert("$mod_" + std::to_string(x.value.getZExtValue())); expr(x.a); break;
    case IndexExprKind::Read:
      out.insert("$read_T" + std::to_string(x.index));
      for (unsigned i : x.indices) expr(i);
      break;
    }
  };
  std::function<void(unsigned)> cond = [&](unsigned n) {
    if (!c.insert(n).second)
      return;
    const IndexProgram::Cond &x = p.conds.at(n);
    switch (x.kind) {
    case IndexCondKind::Eq: out.insert("$eq"); expr(x.a); expr(x.b); break;
    case IndexCondKind::Ult: out.insert("$ult"); expr(x.a); expr(x.b); break;
    case IndexCondKind::Ule: out.insert("$ule"); expr(x.a); expr(x.b); break;
    case IndexCondKind::Not: out.insert("$not"); cond(x.a); break;
    case IndexCondKind::And: out.insert("$and"); cond(x.a); cond(x.b); break;
    case IndexCondKind::Or: out.insert("$or"); cond(x.a); cond(x.b); break;
    }
  };
  std::function<void(unsigned)> stmt = [&](unsigned n) {
    if (!s.insert(n).second)
      return;
    const IndexProgram::Stmt &x = p.stmts.at(n);
    switch (x.kind) {
    case IndexStmtKind::Skip: out.insert("$skip"); break;
    case IndexStmtKind::Assign: out.insert("$assign_v" + std::to_string(x.local)); expr(x.expr); break;
    case IndexStmtKind::Write:
      out.insert("$write_T" + std::to_string(x.local));
      expr(x.expr);
      for (unsigned i : x.indices) expr(i);
      break;
    case IndexStmtKind::Seq: out.insert("$seq"); stmt(x.a); stmt(x.b); break;
    case IndexStmtKind::If: out.insert("$if"); cond(x.cond); stmt(x.a); stmt(x.b); break;
    case IndexStmtKind::While: out.insert("$while"); cond(x.cond); stmt(x.a); break;
    }
  };
  stmt(p.body);
  expr(p.result);
  out.insert("$return");
  return std::vector<std::string>(out.begin(), out.end());
}

std::vector<std::string> semgusExportMissing(const IndexProgram &candidate, const std::string &text) {
  std::vector<std::string> missing;
  size_t types = text.find("(declare-term-types");
  size_t rules = text.find("(define-funs-rec");
  size_t synth = text.find("(synth-fun");
  if (types == std::string::npos || rules == std::string::npos || synth == std::string::npos)
    return semgusConstructors(candidate);
  std::string typesText = text.substr(types, rules - types), rulesText = text.substr(rules, synth - rules);
  for (const std::string &ctor : semgusConstructors(candidate)) {
    // In the term types: "(ctor" followed by a space or ")"; in the rules, a clause "((ctor".
    auto declared = [&](const std::string &where) {
      size_t at = 0;
      while ((at = where.find("(" + ctor, at)) != std::string::npos) {
        size_t after = at + 1 + ctor.size();
        if (after < where.size() && (where[after] == ' ' || where[after] == ')'))
          return true;
        at = after;
      }
      return false;
    };
    if (!declared(typesText))
      missing.push_back(ctor + " (no production)");
    else if (!declared(rulesText))
      missing.push_back(ctor + " (no rule)");
  }
  return missing;
}

std::optional<std::string> semgusTermText(Term t, const std::function<std::string(Term)> &leafName) {
  if (!t || t->sort.kind != Sort::BV)
    return std::nullopt;
  auto lit = [](const APInt &v) {
    if (v.getBitWidth() == 1)
      return std::string(v.isZero() ? "false" : "true");
    return literal(v);
  };
  std::function<std::optional<std::string>(Term)> go = [&](Term n) -> std::optional<std::string> {
    if (n->sort.kind != Sort::BV)
      return std::nullopt;
    auto bin = [&](const char *op) -> std::optional<std::string> {
      auto a = go(n->args[0]), b = go(n->args[1]);
      if (!a || !b)
        return std::nullopt;
      return "(" + std::string(op) + " " + *a + " " + *b + ")";
    };
    bool boolean = n->sort.width == 1;
    switch (n->op) {
    case TermOp::Const: return lit(n->value);
    case TermOp::Param: case TermOp::State: return leafName(n);
    // The model's poison shadows: false on every run the specification describes
    // (a run that stops on undefined behaviour has no iteration to specify).
    case TermOp::Shadow: return std::string("false");
    case TermOp::Add: return bin("bvadd");
    case TermOp::Sub: return bin("bvsub");
    case TermOp::Mul: return bin("bvmul");
    case TermOp::UDiv: return bin("bvudiv");
    case TermOp::URem: return bin("bvurem");
    case TermOp::And: return bin(boolean ? "and" : "bvand");
    case TermOp::Or: return bin(boolean ? "or" : "bvor");
    case TermOp::Xor: return bin(boolean ? "xor" : "bvxor");
    case TermOp::Not: {
      auto a = go(n->args[0]);
      if (!a)
        return std::nullopt;
      return "(" + std::string(boolean ? "not" : "bvnot") + " " + *a + ")";
    }
    case TermOp::Ite: {
      auto c = go(n->args[0]), a = go(n->args[1]), b = go(n->args[2]);
      if (!c || !a || !b)
        return std::nullopt;
      return "(ite " + *c + " " + *a + " " + *b + ")";
    }
    case TermOp::ICmp: {
      static const char *names[] = {"=", "distinct", "bvslt", "bvsle", "bvsgt", "bvsge", "bvult", "bvule", "bvugt", "bvuge"};
      unsigned predicate = n->aux;
      if (predicate >= 10)
        return std::nullopt;
      auto a = go(n->args[0]), b = go(n->args[1]);
      if (!a || !b)
        return std::nullopt;
      return "(" + std::string(names[predicate]) + " " + *a + " " + *b + ")";
    }
    default:
      return std::nullopt;
    }
  };
  return go(t);
}

std::string exportSemgusProblem(const SemgusProblem &q) {
  std::string w = std::to_string(q.width), bv = "(_ BitVec " + w + ")";
  std::vector<std::string> inputs = q.inputNames;
  std::string out;
  out += ";; Statement-level index program synthesis, bit-vector adaptation of the\n"
         ";; thesis's Appendix G (grammar G.1, semantics G.2, specification G.3";
  out += q.arrays.empty() ? ").\n" : ", arrays G.5).\n";
  out += ";; STATUS: written in the structure the thesis describes for SemGuS-Lib;\n"
         ";; NOT read by any external SemGuS parser and NOT given to any SemGuS solver.\n"
         ";; The examples are the sampled events; they narrow a search and prove nothing.\n";
  if (!q.arrays.empty())
    out += ";; Arrays (G.5): each array T is a nested (Array idx ...) with a written-flag array W\n"
           ";; beside it; a read needs every index below its extent and the flag set, a write\n"
           ";; needs the indices below the extents and sets the flag; no clause otherwise, so\n"
           ";; an out-of-extent or never-written access has no derivation.\n";
  // The state: inputs, locals, then per array its contents T and its flags W.
  struct Var {
    std::string name, sort;
  };
  std::vector<Var> vars;
  for (const std::string &x : inputs)
    vars.push_back({x, bv});
  for (unsigned v = 0; v < q.locals; ++v)
    vars.push_back({"v" + std::to_string(v), bv});
  auto nested = [&](unsigned rank, const std::string &element) {
    std::string sort = element;
    for (unsigned d = 0; d < rank; ++d)
      sort = "(Array " + bv + " " + sort + ")";
    return sort;
  };
  for (unsigned a = 0; a < q.arrays.size(); ++a) {
    vars.push_back({"T" + std::to_string(a), nested(q.arrays[a].rank, bv)});
    vars.push_back({"W" + std::to_string(a), nested(q.arrays[a].rank, "Bool")});
  }
  auto varArgs = [&](const std::string &suffix) {
    std::string text;
    for (const Var &v : vars)
      text += " (" + v.name + suffix + " " + v.sort + ")";
    return text;
  };
  auto varList = [&](const std::string &suffix) {
    std::string text;
    for (const Var &v : vars)
      text += " " + v.name + suffix;
    return text;
  };
  // Grammar: one constructor per production.
  out += "(declare-term-types\n  ((E 0) (B 0) (S 0) (P 0))\n  ((";
  for (const std::string &x : inputs)
    out += " ($" + x + ")";
  for (unsigned v = 0; v < q.locals; ++v)
    out += " ($v" + std::to_string(v) + ")";
  for (const APInt &k : q.constants)
    out += " ($const_" + std::to_string(k.getZExtValue()) + ")";
  out += " ($add E E) ($sub E E) ($neg E) ($mul E E)";
  for (const APInt &d : q.divisors) {
    std::string n = std::to_string(d.getZExtValue());
    out += " ($div_" + n + " E) ($mod_" + n + " E)";
  }
  for (unsigned a = 0; a < q.arrays.size(); ++a) {
    out += " ($read_T" + std::to_string(a);
    for (unsigned d = 0; d < q.arrays[a].rank; ++d)
      out += " E";
    out += ")";
  }
  out += ")\n   (($eq E E) ($ult E E) ($ule E E) ($not B) ($and B B) ($or B B))\n   (($skip)";
  for (unsigned v = 0; v < q.locals; ++v)
    out += " ($assign_v" + std::to_string(v) + " E)";
  for (unsigned a = 0; a < q.arrays.size(); ++a) {
    if (q.arrays[a].input)
      continue; // An input array is read, never written (G.5: a const table).
    out += " ($write_T" + std::to_string(a);
    for (unsigned d = 0; d < q.arrays[a].rank; ++d)
      out += " E";
    out += " E)";
  }
  out += " ($seq S S) ($if B S S) ($while B S))\n   (($return S E))))\n";
  // Semantics: one Boolean function per nonterminal, matching on constructors.
  out += "(define-funs-rec\n";
  out += "  ((E.Sem ((e E)" + varArgs("") + " (r " + bv + ")) Bool)\n";
  out += "   (B.Sem ((b B)" + varArgs("") + " (w Bool)) Bool)\n";
  out += "   (S.Sem ((s S)" + varArgs("") + varArgs("_out") + ") Bool)\n";
  out += "   (P.Sem ((p P)";
  for (const std::string &x : inputs)
    out += " (" + x + " " + bv + ")";
  for (unsigned a = 0; a < q.arrays.size(); ++a)
    if (q.arrays[a].input)
      out += " (T" + std::to_string(a) + " " + nested(q.arrays[a].rank, bv) + ")";
  out += " (r " + bv + ")) Bool))\n  (";
  // Index tuple helpers for the array rules.
  auto indexVars = [&](unsigned rank, const std::string &prefix) {
    std::vector<std::string> names;
    for (unsigned d = 0; d < rank; ++d)
      names.push_back(prefix + std::to_string(d + 1));
    return names;
  };
  auto selectAt = [&](std::string array, const std::vector<std::string> &k) {
    for (const std::string &i : k)
      array = "(select " + array + " " + i + ")";
    return array;
  };
  // (store (... (select a k1) ...) kd v) nested back up.
  std::function<std::string(const std::string &, const std::vector<std::string> &, size_t, const std::string &)> storeAt =
      [&](const std::string &array, const std::vector<std::string> &k, size_t d, const std::string &value) -> std::string {
    if (d + 1 == k.size())
      return "(store " + array + " " + k[d] + " " + value + ")";
    return "(store " + array + " " + k[d] + " " + storeAt("(select " + array + " " + k[d] + ")", k, d + 1, value) + ")";
  };
  auto inBounds = [&](unsigned a, const std::vector<std::string> &k) {
    std::string text;
    for (unsigned d = 0; d < k.size(); ++d)
      text += " (bvult " + k[d] + " " + q.arrays[a].extents.at(d) + ")";
    return text;
  };
  // E.Sem
  out += "(! (match e (";
  for (const Var &v : vars)
    if (v.sort == bv)
      out += "\n      (($" + v.name + ") (= r " + v.name + "))";
  for (const APInt &k : q.constants)
    out += "\n      (($const_" + std::to_string(k.getZExtValue()) + ") (= r " + literal(k) + "))";
  auto binaryRule = [&](const std::string &ctor, const std::string &op) {
    out += "\n      ((" + ctor + " e1 e2) (exists ((r1 " + bv + ") (r2 " + bv + "))\n        (and (E.Sem e1" +
           varList("") + " r1) (E.Sem e2" + varList("") + " r2) (= r (" + op + " r1 r2)))))";
  };
  binaryRule("$add", "bvadd");
  binaryRule("$sub", "bvsub");
  out += "\n      (($neg e1) (exists ((r1 " + bv + ")) (and (E.Sem e1" + varList("") +
         " r1) (= r (bvneg r1)))))";
  binaryRule("$mul", "bvmul");
  for (const APInt &d : q.divisors) {
    std::string n = std::to_string(d.getZExtValue());
    out += "\n      (($div_" + n + " e1) (exists ((r1 " + bv + ")) (and (E.Sem e1" + varList("") +
           " r1) (= r (bvudiv r1 " + literal(d) + ")))))";
    out += "\n      (($mod_" + n + " e1) (exists ((r1 " + bv + ")) (and (E.Sem e1" + varList("") +
           " r1) (= r (bvurem r1 " + literal(d) + ")))))";
  }
  for (unsigned a = 0; a < q.arrays.size(); ++a) {
    std::string T = "T" + std::to_string(a), W = "W" + std::to_string(a);
    std::vector<std::string> k = indexVars(q.arrays[a].rank, "k");
    std::string pattern = "($read_" + T, binders, premises;
    for (unsigned d = 0; d < k.size(); ++d) {
      pattern += " e" + std::to_string(d + 1);
      binders += " (" + k[d] + " " + bv + ")";
      premises += " (E.Sem e" + std::to_string(d + 1) + varList("") + " " + k[d] + ")";
    }
    // G.5 read rule: indices evaluated, each below its extent, the element written.
    out += "\n      (" + pattern + ") (exists (" + binders.substr(1) + ")\n        (and" + premises + inBounds(a, k) +
           " " + selectAt(W, k) + " (= r " + selectAt(T, k) + "))))";
  }
  out += "))\n    :input (" + varList("").substr(1) + ") :output (r))\n";
  // B.Sem
  out += "   (! (match b (";
  auto compareRule = [&](const std::string &ctor, const std::string &op) {
    out += "\n      ((" + ctor + " e1 e2) (exists ((r1 " + bv + ") (r2 " + bv + "))\n        (and (E.Sem e1" +
           varList("") + " r1) (E.Sem e2" + varList("") + " r2) (= w (" + op + " r1 r2)))))";
  };
  compareRule("$eq", "=");
  compareRule("$ult", "bvult");
  compareRule("$ule", "bvule");
  out += "\n      (($not b1) (exists ((w1 Bool)) (and (B.Sem b1" + varList("") + " w1) (= w (not w1)))))";
  out += "\n      (($and b1 b2) (exists ((w1 Bool) (w2 Bool)) (and (B.Sem b1" + varList("") +
         " w1) (B.Sem b2" + varList("") + " w2) (= w (and w1 w2)))))";
  out += "\n      (($or b1 b2) (exists ((w1 Bool) (w2 Bool)) (and (B.Sem b1" + varList("") +
         " w1) (B.Sem b2" + varList("") + " w2) (= w (or w1 w2)))))";
  out += "))\n    :input (" + varList("").substr(1) + ") :output (w))\n";
  // S.Sem
  auto unchanged = [&](const std::set<std::string> &except) {
    std::string text;
    for (const Var &v : vars)
      if (!except.count(v.name))
        text += " (= " + v.name + "_out " + v.name + ")";
    return text;
  };
  out += "   (! (match s (";
  out += "\n      (($skip) (and" + unchanged({}) + "))";
  for (unsigned v = 0; v < q.locals; ++v) {
    std::string name = "v" + std::to_string(v);
    out += "\n      (($assign_" + name + " e1) (and (E.Sem e1" + varList("") + " " + name + "_out)" +
           unchanged({name}) + "))";
  }
  for (unsigned a = 0; a < q.arrays.size(); ++a) {
    if (q.arrays[a].input)
      continue;
    std::string T = "T" + std::to_string(a), W = "W" + std::to_string(a);
    std::vector<std::string> k = indexVars(q.arrays[a].rank, "k");
    std::string pattern = "($write_" + T, binders, premises;
    for (unsigned d = 0; d < k.size(); ++d) {
      pattern += " e" + std::to_string(d + 1);
      binders += " (" + k[d] + " " + bv + ")";
      premises += " (E.Sem e" + std::to_string(d + 1) + varList("") + " " + k[d] + ")";
    }
    pattern += " e" + std::to_string(k.size() + 1);
    binders += " (n " + bv + ")";
    premises += " (E.Sem e" + std::to_string(k.size() + 1) + varList("") + " n)";
    // G.5 write rule: value and indices in the state before the write, indices
    // below the extents, the element and its flag updated, everything else kept.
    out += "\n      (" + pattern + ") (exists (" + binders.substr(1) + ")\n        (and" + premises + inBounds(a, k) +
           " (= " + T + "_out " + storeAt(T, k, 0, "n") + ") (= " + W + "_out " + storeAt(W, k, 0, "true") + ")" +
           unchanged({T, W}) + ")))";
  }
  std::string mids;
  for (const Var &v : vars)
    mids += " (" + v.name + "_mid " + v.sort + ")";
  out += "\n      (($seq s1 s2) (exists (" + mids.substr(1) + ")\n        (and (S.Sem s1" + varList("") +
         varList("_mid") + ") (S.Sem s2" + varList("_mid") + varList("_out") + "))))";
  out += "\n      (($if b1 s1 s2) (exists ((w1 Bool))\n        (and (B.Sem b1" + varList("") +
         " w1) (ite w1 (S.Sem s1" + varList("") + varList("_out") + ") (S.Sem s2" + varList("") +
         varList("_out") + ")))))";
  out += "\n      (($while b1 s1) (exists ((w1 Bool))\n        (and (B.Sem b1" + varList("") +
         " w1)\n             (ite w1 (exists (" + mids.substr(1) + ") (and (S.Sem s1" + varList("") +
         varList("_mid") + ") (S.Sem ($while b1 s1)" + varList("_mid") + varList("_out") +
         ")))\n                     (and" + unchanged({}) + ")))))";
  out += "))\n    :input (" + varList("").substr(1) + ") :output (" + varList("_out").substr(1) + "))\n";
  // P.Sem: locals start at zero; a non-input array has no written element; an input
  // array has every element and is given.
  out += "   (! (match p (\n      (($return s1 e1) (exists (";
  std::string outs;
  for (const Var &v : vars)
    outs += " (" + v.name + "_out " + v.sort + ")";
  for (unsigned a = 0; a < q.arrays.size(); ++a)
    if (!q.arrays[a].input)
      outs += " (T" + std::to_string(a) + " " + nested(q.arrays[a].rank, bv) + ")";
  out += outs.substr(1) + ")\n        (and (S.Sem s1";
  for (const std::string &x : inputs)
    out += " " + x;
  for (unsigned v = 0; v < q.locals; ++v)
    out += " " + literal(APInt::getZero(q.width));
  for (unsigned a = 0; a < q.arrays.size(); ++a) {
    out += " T" + std::to_string(a);
    out += " ((as const " + nested(q.arrays[a].rank, "Bool") + ") " + (q.arrays[a].input ? "true" : "false") + ")";
  }
  out += varList("_out") + ") (E.Sem e1" + varList("_out") + " r))))))\n    :input (";
  std::string pInputs;
  for (const std::string &x : inputs)
    pInputs += x + " ";
  for (unsigned a = 0; a < q.arrays.size(); ++a)
    if (q.arrays[a].input)
      pInputs += "T" + std::to_string(a) + " ";
  if (!pInputs.empty())
    pInputs.pop_back();
  out += pInputs + ") :output (r))))\n";
  out += "(synth-fun f () P)\n";
  for (const std::string &declared : q.declaredVariables)
    out += "(declare-var " + declared + ")\n";
  for (const SemgusExample &example : q.examples) {
    out += "(constraint (P.Sem f";
    for (const APInt &value : example.inputs)
      out += " " + literal(value);
    out += " " + literal(example.output) + "))\n";
  }
  for (const std::string &constraint : q.constraints)
    out += "(constraint " + constraint + ")\n";
  if (q.candidate)
    out += ";; candidate, in constructor form; a proposal, checked elsewhere\n;; " +
           semgusTerm(*q.candidate) + "\n";
  out += "(check-synth)\n";
  return out;
}

bool semgusExportWellFormed(const std::string &text, std::string &why) {
  static const std::set<std::string> commands{"declare-term-types", "define-funs-rec", "synth-fun",
                                              "declare-var", "constraint", "check-synth"};
  int depth = 0;
  size_t i = 0;
  bool sawCheck = false;
  while (i < text.size()) {
    char c = text[i];
    if (c == ';') {
      while (i < text.size() && text[i] != '\n')
        ++i;
      continue;
    }
    if (c == '(') {
      if (depth == 0) {
        size_t start = i + 1, end = start;
        while (end < text.size() && !isspace((unsigned char)text[end]) && text[end] != ')')
          ++end;
        std::string head = text.substr(start, end - start);
        if (!commands.count(head)) {
          why = "unknown top-level command " + head;
          return false;
        }
        sawCheck = sawCheck || head == "check-synth";
      }
      ++depth;
    } else if (c == ')') {
      if (--depth < 0) {
        why = "unbalanced parenthesis";
        return false;
      }
    } else if (depth == 0 && !isspace((unsigned char)c)) {
      why = "text outside a command";
      return false;
    }
    ++i;
  }
  if (depth != 0) {
    why = "unbalanced parenthesis";
    return false;
  }
  if (!sawCheck) {
    why = "no check-synth";
    return false;
  }
  return true;
}

} // namespace mlir::generalized_gemm
