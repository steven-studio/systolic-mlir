#include "GeneralizedGemm/LoopInvariant.h"

#include "GeneralizedGemm/ControlStructure.h"
#include "GeneralizedGemm/SymbolicEvaluator.h"
#include "GeneralizedGemm/TerminationObligations.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/JSON.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <functional>
#include <set>

using namespace llvm;

namespace mlir::generalized_gemm {

const char *stringifyInvariantVerdict(InvariantVerdict verdict) {
  switch (verdict) {
  case InvariantVerdict::Inductive: return "INDUCTIVE";
  case InvariantVerdict::NotInductive: return "NOT_INDUCTIVE";
  case InvariantVerdict::Unknown: return "UNKNOWN";
  }
  return "UNKNOWN";
}

const char *stringifyLoopFunctionVerdict(LoopFunctionVerdict verdict) {
  switch (verdict) {
  case LoopFunctionVerdict::Proved: return "PROVED";
  case LoopFunctionVerdict::Refuted: return "REFUTED";
  case LoopFunctionVerdict::Unknown: return "UNKNOWN";
  }
  return "UNKNOWN";
}

namespace {

/// Ids of the Shadow leaves of `term`, sorted, each once.
SmallVector<unsigned> shadowSupport(Term root) {
  DenseSet<Term> seen;
  SmallVector<unsigned> ids;
  std::function<void(Term)> visit = [&](Term t) {
    if (!seen.insert(t).second)
      return;
    if (t->op == TermOp::Shadow)
      ids.push_back(t->index);
    for (Term a : t->args)
      visit(a);
  };
  visit(root);
  llvm::sort(ids);
  ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
  return ids;
}

std::string decimal(const APInt &value) {
  SmallString<40> text;
  bool negative = value.getBitWidth() > 1 && value.isNegative();
  value.toString(text, 10, /*Signed=*/negative);
  return std::string(text);
}

const char *predicateText(Predicate predicate) {
  switch (predicate) {
  case Predicate::Eq: return "==";
  case Predicate::Ne: return "!=";
  case Predicate::Slt: return "<s";
  case Predicate::Sle: return "<=s";
  case Predicate::Sgt: return ">s";
  case Predicate::Sge: return ">=s";
  case Predicate::Ult: return "<u";
  case Predicate::Ule: return "<=u";
  case Predicate::Ugt: return ">u";
  case Predicate::Uge: return ">=u";
  }
  return "?";
}

std::string flagText(unsigned flags) {
  std::string out;
  if (flags & FlagNSW)
    out += "nsw";
  if (flags & FlagNUW)
    out += "nuw";
  if (flags & FlagExact)
    out += "exact";
  return out;
}

std::string pathText(const SemanticProgram &program, const CutPath &path) {
  return (path.fromEntry ? std::string("entry ") : std::string()) + describePath(program, path);
}

} // namespace

std::string formatLoopTerm(Term term) {
  DenseMap<Term, std::string> memo;
  std::function<std::string(Term)> text = [&](Term t) -> std::string {
    auto found = memo.find(t);
    if (found != memo.end())
      return found->second;
    auto a = [&](unsigned i) { return text(t->args[i]); };
    bool boolean = t->sort.kind == Sort::BV && t->sort.width == 1;
    auto binary = [&](const char *op) {
      return "(" + a(0) + " " + op + flagText(t->aux) + " " + a(1) + ")";
    };
    std::string out;
    switch (t->op) {
    case TermOp::Const:
      out = boolean ? (t->value.isOne() ? "true" : "false") : decimal(t->value);
      break;
    case TermOp::Param: out = "p" + std::to_string(t->index); break;
    case TermOp::State: out = "s" + std::to_string(t->index); break;
    case TermOp::Shadow: out = "sp" + std::to_string(t->index); break;
    case TermOp::Add: out = binary("+"); break;
    case TermOp::Sub: out = binary("-"); break;
    case TermOp::Mul: out = binary("*"); break;
    case TermOp::UDiv: out = binary("/u"); break;
    case TermOp::SDiv: out = binary("/s"); break;
    case TermOp::URem: out = binary("%u"); break;
    case TermOp::SRem: out = binary("%s"); break;
    case TermOp::Shl: out = binary("<<"); break;
    case TermOp::LShr: out = binary(">>u"); break;
    case TermOp::AShr: out = binary(">>s"); break;
    case TermOp::And: out = "(" + a(0) + (boolean ? " && " : " & ") + a(1) + ")"; break;
    case TermOp::Or: out = "(" + a(0) + (boolean ? " || " : " | ") + a(1) + ")"; break;
    case TermOp::Xor: out = "(" + a(0) + " ^ " + a(1) + ")"; break;
    case TermOp::Not: out = (boolean ? "!" : "~") + a(0); break;
    case TermOp::ICmp:
      out = "(" + a(0) + " " + predicateText((Predicate)t->aux) + " " + a(1) + ")";
      break;
    case TermOp::Ite: out = "ite(" + a(0) + ", " + a(1) + ", " + a(2) + ")"; break;
    case TermOp::ZExt: out = "zext" + std::to_string(t->sort.width) + "(" + a(0) + ")"; break;
    case TermOp::SExt: out = "sext" + std::to_string(t->sort.width) + "(" + a(0) + ")"; break;
    case TermOp::Trunc: out = "trunc" + std::to_string(t->sort.width) + "(" + a(0) + ")"; break;
    default: out = "<" + describeNode(t) + ">"; break;
    }
    memo[t] = out;
    return out;
  };
  return text(term);
}

namespace {

/// The reader of formatLoopTerm's text. An untyped tree first, then terms: a
/// constant takes its width from the other operand.
class LoopTermParser {
public:
  LoopTermParser(const SemanticProgram &program, const std::string &text)
      : program(program), terms(*program.terms), text(text) {}

  Term parse(std::string &why) {
    try {
      tokenize();
      Node root = parseOr();
      if (position != tokens.size())
        fail("unexpected '" + tokens[position].text + "'");
      unsigned width = widthOf(root);
      Term out = build(root, width ? width : 0);
      if (!out)
        fail("cannot infer the width of a constant expression");
      return out;
    } catch (const std::string &error) {
      why = error;
    } catch (const TermError &error) {
      why = error.what();
    }
    return nullptr;
  }

private:
  struct Token {
    enum Kind { Identifier, Number, Operator } kind;
    std::string text;
    unsigned flags = 0;
  };
  struct Node {
    enum Kind { Leaf, Literal, Unary, Binary, Compare, Logical, Ite, Cast } kind = Leaf;
    std::string op;
    Term leaf = nullptr;
    std::string digits;
    bool negative = false;
    unsigned flags = 0, width = 0;
    Predicate predicate = Predicate::Eq;
    std::vector<Node> kids;
  };

  [[noreturn]] void fail(const std::string &why) { throw why; }

  void tokenize() {
    static const char *operators[] = {"<=s", "<=u", ">=s", ">=u", ">>u", ">>s", "==", "!=",
                                      "&&",  "||",  "/u",  "/s",  "%u",  "%s",  "<<", "<s",
                                      "<u",  ">s",  ">u",  "+",   "-",   "*",   "&",  "|",
                                      "^",   "!",   "~",   "(",   ")",   ","};
    size_t i = 0, n = text.size();
    while (i < n) {
      char c = text[i];
      if (std::isspace((unsigned char)c)) {
        ++i;
        continue;
      }
      if (std::isalpha((unsigned char)c) || c == '_') {
        size_t start = i;
        while (i < n && (std::isalnum((unsigned char)text[i]) || text[i] == '_'))
          ++i;
        tokens.push_back({Token::Identifier, text.substr(start, i - start)});
        continue;
      }
      if (std::isdigit((unsigned char)c)) {
        size_t start = i;
        while (i < n && std::isdigit((unsigned char)text[i]))
          ++i;
        tokens.push_back({Token::Number, text.substr(start, i - start)});
        continue;
      }
      bool matched = false;
      for (const char *op : operators) {
        size_t length = strlen(op);
        if (text.compare(i, length, op) == 0) {
          Token token{Token::Operator, op};
          i += length;
          std::string base = op;
          if (base == "+" || base == "-" || base == "*" || base == "<<" || base == "/u" ||
              base == "/s" || base == ">>u" || base == ">>s") {
            for (bool more = true; more;) {
              more = false;
              for (auto [suffix, flag] : {std::pair<const char *, unsigned>{"nsw", FlagNSW},
                                          {"nuw", FlagNUW}, {"exact", FlagExact}})
                if (text.compare(i, strlen(suffix), suffix) == 0) {
                  token.flags |= flag;
                  i += strlen(suffix);
                  more = true;
                }
            }
          }
          tokens.push_back(token);
          matched = true;
          break;
        }
      }
      if (!matched)
        fail(std::string("unexpected character '") + c + "'");
    }
  }

  bool peek(const char *op) const {
    return position < tokens.size() && tokens[position].kind == Token::Operator &&
           tokens[position].text == op;
  }
  void expect(const char *op) {
    if (!peek(op))
      fail(std::string("expected '") + op + "'");
    ++position;
  }

  Node parseOr() {
    Node left = parseAnd();
    while (peek("||")) {
      ++position;
      Node node;
      node.kind = Node::Logical;
      node.op = "||";
      node.kids = {std::move(left), parseAnd()};
      left = std::move(node);
    }
    return left;
  }
  Node parseAnd() {
    Node left = parseCompare();
    while (peek("&&")) {
      ++position;
      Node node;
      node.kind = Node::Logical;
      node.op = "&&";
      node.kids = {std::move(left), parseCompare()};
      left = std::move(node);
    }
    return left;
  }
  Node parseCompare() {
    Node left = parseBits();
    static const std::pair<const char *, Predicate> predicates[] = {
        {"==", Predicate::Eq},   {"!=", Predicate::Ne},   {"<s", Predicate::Slt},
        {"<=s", Predicate::Sle}, {">s", Predicate::Sgt},  {">=s", Predicate::Sge},
        {"<u", Predicate::Ult},  {"<=u", Predicate::Ule}, {">u", Predicate::Ugt},
        {">=u", Predicate::Uge}};
    for (const auto &[op, predicate] : predicates)
      if (peek(op)) {
        ++position;
        Node node;
        node.kind = Node::Compare;
        node.predicate = predicate;
        node.kids = {std::move(left), parseBits()};
        return node;
      }
    return left;
  }
  Node parseBits() {
    Node left = parseShift();
    while (peek("&") || peek("|") || peek("^")) {
      Node node;
      node.kind = Node::Binary;
      node.op = tokens[position++].text;
      node.kids = {std::move(left), parseShift()};
      left = std::move(node);
    }
    return left;
  }
  Node parseShift() {
    Node left = parseSum();
    while (peek("<<") || peek(">>u") || peek(">>s")) {
      Node node;
      node.kind = Node::Binary;
      node.op = tokens[position].text;
      node.flags = tokens[position++].flags;
      node.kids = {std::move(left), parseSum()};
      left = std::move(node);
    }
    return left;
  }
  Node parseSum() {
    Node left = parseProduct();
    while (peek("+") || peek("-")) {
      Node node;
      node.kind = Node::Binary;
      node.op = tokens[position].text;
      node.flags = tokens[position++].flags;
      node.kids = {std::move(left), parseProduct()};
      left = std::move(node);
    }
    return left;
  }
  Node parseProduct() {
    Node left = parseUnary();
    while (peek("*") || peek("/u") || peek("/s") || peek("%u") || peek("%s")) {
      Node node;
      node.kind = Node::Binary;
      node.op = tokens[position].text;
      node.flags = tokens[position++].flags;
      node.kids = {std::move(left), parseUnary()};
      left = std::move(node);
    }
    return left;
  }
  Node parseUnary() {
    if (peek("!") || peek("~")) {
      Node node;
      node.kind = Node::Unary;
      node.op = tokens[position++].text;
      node.kids = {parseUnary()};
      return node;
    }
    if (peek("-")) {
      ++position;
      if (position < tokens.size() && tokens[position].kind == Token::Number) {
        Node node;
        node.kind = Node::Literal;
        node.digits = tokens[position++].text;
        node.negative = true;
        return node;
      }
      Node node;
      node.kind = Node::Unary;
      node.op = "-";
      node.kids = {parseUnary()};
      return node;
    }
    return parsePrimary();
  }
  Node parsePrimary() {
    if (position >= tokens.size())
      fail("unexpected end of text");
    const Token &token = tokens[position];
    if (token.kind == Token::Operator && token.text == "(") {
      ++position;
      Node inner = parseOr();
      expect(")");
      return inner;
    }
    if (token.kind == Token::Number) {
      ++position;
      Node node;
      node.kind = Node::Literal;
      node.digits = token.text;
      return node;
    }
    if (token.kind != Token::Identifier)
      fail("unexpected '" + token.text + "'");
    ++position;
    const std::string &name = token.text;
    auto number = [&](size_t prefix, unsigned &value) {
      if (name.size() <= prefix)
        return false;
      for (size_t i = prefix; i < name.size(); ++i)
        if (!std::isdigit((unsigned char)name[i]))
          return false;
      value = std::stoul(name.substr(prefix));
      return true;
    };
    Node node;
    unsigned id = 0;
    if (name == "true" || name == "false") {
      node.kind = Node::Leaf;
      node.leaf = terms.boolean(name == "true");
      return node;
    }
    if (name == "ite") {
      expect("(");
      node.kind = Node::Ite;
      node.kids.push_back(parseOr());
      expect(",");
      node.kids.push_back(parseOr());
      expect(",");
      node.kids.push_back(parseOr());
      expect(")");
      return node;
    }
    for (const char *cast : {"zext", "sext", "trunc"}) {
      size_t length = strlen(cast);
      if (name.compare(0, length, cast) == 0 && number(length, id)) {
        node.kind = Node::Cast;
        node.op = cast;
        node.width = id;
        expect("(");
        node.kids.push_back(parseOr());
        expect(")");
        return node;
      }
    }
    node.kind = Node::Leaf;
    if (name.compare(0, 2, "sp") == 0 && number(2, id)) {
      if (id >= program.state.size())
        fail("no state variable s" + std::to_string(id) + " for " + name);
      node.leaf = terms.shadow(id);
      return node;
    }
    if (name[0] == 's' && number(1, id)) {
      if (id >= program.state.size())
        fail("no state variable " + name);
      node.leaf = terms.state(id, program.state[id].sort);
      return node;
    }
    if (name[0] == 'p' && number(1, id)) {
      if (id >= program.parameters.size())
        fail("no parameter " + name);
      node.leaf = terms.param(id, program.parameters[id].sort);
      return node;
    }
    fail("unknown name '" + name + "'");
  }

  /// 0 for a width the node does not fix by itself.
  unsigned widthOf(const Node &node) {
    switch (node.kind) {
    case Node::Leaf: return node.leaf->sort.width;
    case Node::Literal: return 0;
    case Node::Compare:
    case Node::Logical: return 1;
    case Node::Cast: return node.width;
    case Node::Unary: return node.op == "!" ? 1 : widthOf(node.kids[0]);
    case Node::Binary: {
      unsigned a = widthOf(node.kids[0]), b = widthOf(node.kids[1]);
      if (a && b && a != b)
        fail("operands of '" + node.op + "' have widths " + std::to_string(a) + " and " +
             std::to_string(b));
      return a ? a : b;
    }
    case Node::Ite: {
      unsigned a = widthOf(node.kids[1]), b = widthOf(node.kids[2]);
      if (a && b && a != b)
        fail("arms of ite have different widths");
      return a ? a : b;
    }
    }
    return 0;
  }

  Term build(const Node &node, unsigned width) {
    switch (node.kind) {
    case Node::Leaf:
      if (width && node.leaf->sort.width != width)
        fail("a leaf of width " + std::to_string(node.leaf->sort.width) + " where " +
             std::to_string(width) + " is needed");
      return node.leaf;
    case Node::Literal: {
      if (!width)
        return nullptr;
      if (node.digits.size() > 36)
        fail("constant too long");
      APInt value(128, node.digits, 10);
      if (node.negative)
        value = -value;
      bool fits = node.negative ? value.isSignedIntN(width) : value.isIntN(width);
      if (!fits)
        fail("constant " + std::string(node.negative ? "-" : "") + node.digits +
             " does not fit " + std::to_string(width) + " bits");
      return terms.bv(value.trunc(width));
    }
    case Node::Unary: {
      if (node.op == "!") {
        Term a = build(node.kids[0], 1);
        if (!a)
          return nullptr;
        return terms.logicalNot(a);
      }
      Term a = build(node.kids[0], width);
      if (!a)
        return nullptr;
      if (node.op == "-")
        return terms.binary(TermOp::Sub, terms.bv(APInt::getZero(a->sort.width)), a);
      return terms.binary(TermOp::Xor, a, terms.bv(APInt::getAllOnes(a->sort.width)));
    }
    case Node::Binary: {
      unsigned w = widthOf(node);
      if (!w)
        w = width;
      if (!w)
        return nullptr;
      Term a = build(node.kids[0], w), b = build(node.kids[1], w);
      if (!a || !b)
        return nullptr;
      static const std::pair<const char *, TermOp> ops[] = {
          {"+", TermOp::Add},   {"-", TermOp::Sub},    {"*", TermOp::Mul},    {"/u", TermOp::UDiv},
          {"/s", TermOp::SDiv}, {"%u", TermOp::URem},  {"%s", TermOp::SRem},  {"<<", TermOp::Shl},
          {">>u", TermOp::LShr}, {">>s", TermOp::AShr}, {"&", TermOp::And},   {"|", TermOp::Or},
          {"^", TermOp::Xor}};
      for (const auto &[text, op] : ops)
        if (node.op == text)
          return terms.binary(op, a, b, node.flags);
      fail("unknown operator " + node.op);
    }
    case Node::Compare: {
      unsigned a = widthOf(node.kids[0]), b = widthOf(node.kids[1]);
      if (a && b && a != b)
        fail("compared operands have widths " + std::to_string(a) + " and " + std::to_string(b));
      unsigned w = a ? a : b;
      if (!w)
        fail("a comparison of two constants has no width");
      Term x = build(node.kids[0], w), y = build(node.kids[1], w);
      if (!x || !y)
        return nullptr;
      return terms.icmp(node.predicate, x, y);
    }
    case Node::Logical: {
      Term a = build(node.kids[0], 1), b = build(node.kids[1], 1);
      if (!a || !b)
        return nullptr;
      return node.op == "&&" ? terms.logicalAnd(a, b) : terms.logicalOr(a, b);
    }
    case Node::Ite: {
      Term c = build(node.kids[0], 1);
      unsigned w = widthOf(node);
      if (!w)
        w = width;
      if (!c || !w)
        return nullptr;
      Term a = build(node.kids[1], w), b = build(node.kids[2], w);
      if (!a || !b)
        return nullptr;
      return terms.ite(c, a, b);
    }
    case Node::Cast: {
      unsigned inner = widthOf(node.kids[0]);
      if (!inner)
        fail(node.op + " of a constant has no source width");
      Term a = build(node.kids[0], inner);
      if (!a)
        return nullptr;
      if (node.op == "zext")
        return terms.zext(a, node.width);
      if (node.op == "sext")
        return terms.sext(a, node.width);
      if (node.width >= inner)
        fail("trunc must narrow");
      return terms.trunc(a, node.width);
    }
    }
    return nullptr;
  }

  const SemanticProgram &program;
  TermContext &terms;
  const std::string &text;
  std::vector<Token> tokens;
  size_t position = 0;
};

} // namespace

Term parseLoopTerm(const SemanticProgram &program, const std::string &text, std::string &why) {
  LoopTermParser parser(program, text);
  return parser.parse(why);
}

std::vector<Term> conjuncts(Term term) {
  std::vector<Term> out;
  std::function<void(Term)> visit = [&](Term t) {
    if (t->op == TermOp::And && t->sort.kind == Sort::BV && t->sort.width == 1) {
      visit(t->args[0]);
      visit(t->args[1]);
      return;
    }
    if (t->op == TermOp::Const && t->sort.width == 1 && t->value.isOne())
      return; // `true` adds nothing.
    out.push_back(t);
  };
  visit(term);
  return out;
}

InvariantLoop invariantLoop(const SemanticProgram &program, const SliceLimits &limits) {
  auto refuse = [](std::string why) {
    InvariantLoop out;
    out.reason = std::move(why);
    return out;
  };
  if (!program.isSupported())
    return refuse(std::string("the program is unsupported: ") +
                  stringifyUnsupportedKind(program.unsupported->kind) + ": " +
                  program.unsupported->detail);
  if (!program.roots.empty())
    return refuse("the function has " + std::to_string(program.roots.size()) +
                  " memory root(s): arrays and memory are outside stage 1");
  for (const Parameter &p : program.parameters)
    if (p.sort.kind != Sort::BV)
      return refuse("a parameter is not an integer: floats are outside stage 1");
  if (program.returnSorts.empty())
    return refuse("the function returns no value: there is no result for a candidate to describe");
  for (const Sort &sort : program.returnSorts)
    if (sort.kind != Sort::BV)
      return refuse("a returned value is not an integer: floats are outside stage 1");
  SmallVector<unsigned> cuts = computeCutSet(program);
  std::string why;
  if (!cutSetBreaksCycles(program, cuts, why))
    return refuse("no cut set: " + why);
  if (cuts.empty())
    return refuse("the function has no loop");
  if (cuts.size() > 1)
    return refuse("the function has " + std::to_string(cuts.size()) +
                  " loop headers: nested or consecutive loops are outside stage 1");
  InvariantLoop loop;
  loop.header = cuts.front();
  loop.set = enumerateCutPaths(program, cuts, limits.paths);
  if (!loop.set.complete)
    return refuse("the paths were not enumerated: " + loop.set.reason);
  TermContext &terms = *program.terms;
  for (const CutPath &path : loop.set.paths) {
    bool toExit = path.to == program.exitLocation();
    if (path.fromEntry && path.to == loop.header)
      loop.entry.push_back(path.id);
    else if (path.fromEntry && toExit)
      loop.bypass.push_back(path.id);
    else if (!path.fromEntry && path.from == loop.header && path.to == loop.header)
      loop.iterate.push_back(path.id);
    else if (!path.fromEntry && path.from == loop.header && toExit)
      loop.leave.push_back(path.id);
    else
      return refuse("the path " + pathText(program, path) + " does not fit one loop");
  }
  if (loop.entry.empty())
    return refuse("no path from the entry reaches the header");
  if (loop.iterate.empty())
    return refuse("no path returns to the header");
  auto clean = [](Term t) { return !t || (!containsFloat(t) && !readsMemory(t)); };
  try {
    std::vector<unsigned> returning = loop.leave;
    returning.insert(returning.end(), loop.bypass.begin(), loop.bypass.end());
    for (unsigned id : returning) {
      const CutPath &path = loop.set.paths[id];
      SymbolicPath walk = startSymbolicPath(program, path.from);
      if (path.fromEntry)
        walk.shadow.assign(program.state.size(), terms.boolean(false));
      for (unsigned t : path.transitions)
        walk = extendSymbolicPath(program, walk, t);
      if (walk.returns.size() != program.returnSorts.size() ||
          walk.returnPoison.size() != program.returnSorts.size())
        return refuse("the path " + pathText(program, path) + " returns the wrong number of values");
      SmallVector<Term> values(walk.returns.begin(), walk.returns.end());
      values.append(walk.returnPoison.begin(), walk.returnPoison.end());
      for (Term r : values)
        if (!clean(r))
          return refuse("a returned value reads memory or a float");
      loop.returns[id] = std::vector<Term>(walk.returns.begin(), walk.returns.end());
      loop.returnPoison[id] = std::vector<Term>(walk.returnPoison.begin(), walk.returnPoison.end());
    }
  } catch (const TermError &error) {
    return refuse(std::string("a return path could not be composed: ") + error.what());
  }
  for (const CutPath &path : loop.set.paths)
    if (!clean(path.condition))
      return refuse("the condition of " + pathText(program, path) + " reads memory or a float");
  // What the returns, the leaving and continuing conditions read, closed under the
  // updates of the paths around the loop.
  std::set<unsigned> relevant;
  auto need = [&](Term t) {
    if (t)
      for (unsigned v : stateSupport(t))
        relevant.insert(v);
  };
  for (unsigned id : loop.leave) {
    need(loop.set.paths[id].condition);
    for (Term r : loop.returns[id])
      need(r);
  }
  for (unsigned id : loop.iterate)
    need(loop.set.paths[id].condition);
  for (bool changed = true; changed;) {
    changed = false;
    std::vector<unsigned> now(relevant.begin(), relevant.end());
    for (unsigned id : loop.iterate)
      for (unsigned v : now)
        if (Term t = loop.set.paths[id].state[v]) {
          if (!clean(t))
            return refuse("the update of s" + std::to_string(v) + " reads memory or a float");
          for (unsigned u : stateSupport(t))
            changed = relevant.insert(u).second || changed;
        }
  }
  for (unsigned v : relevant) {
    if (program.state[v].sort.kind != Sort::BV)
      return refuse("s" + std::to_string(v) + " is not an integer");
    for (unsigned id : loop.entry)
      if (!loop.set.paths[id].state[v])
        return refuse("s" + std::to_string(v) + " is read at the header but " +
                      pathText(program, loop.set.paths[id]) + " does not define it");
    loop.variables.push_back(v);
  }
  for (const Parameter &p : program.parameters)
    loop.parameters.push_back(p.id);
  loop.resultSorts.assign(program.returnSorts.begin(), program.returnSorts.end());
  return loop;
}

namespace {

using Aux = std::vector<std::pair<std::string, Term>>;

/// The checker of one certificate. Every verdict it gives comes from a solver's
/// unsat or from a run of the program; a proposer only ever sees the result.
class InvariantChecker {
public:
  InvariantChecker(const SemanticProgram &program, const InvariantLoop &loop,
                   const InvariantCertificate &certificate, const LoopFunction &function,
                   const InvariantCheckOptions &options, SmtBackend &backend)
      : program(program), loop(loop), certificate(certificate), function(function),
        options(options), backend(backend), terms(*program.terms) {}

  CheckedLoopInvariant run();

private:
  bool adopt();
  void solveAtoms();
  bool solve(Term lhs, Term rhs, const std::string &origin);
  bool define(unsigned variable, Term term, const std::string &origin);
  Term sigma(Term term) const;
  Term conjunction(ArrayRef<Term> list) const;
  size_t ask(const std::string &id, const std::string &location, Term formula, const Aux &aux,
             bool wantModel, SmtResponse &response);
  std::string modelText(const SmtResponse &response) const;
  bool induction(bool step);
  void exits();
  void bypasses();
  void poison();
  std::optional<LoopCounterexample> confirm(const std::map<std::string, std::string> &model,
                                            const std::string &source, std::string &note);
  Term post(ArrayRef<Term> returned) const;

  const SemanticProgram &program;
  const InvariantLoop &loop;
  const InvariantCertificate &certificate;
  const LoopFunction &function;
  const InvariantCheckOptions &options;
  SmtBackend &backend;
  TermContext &terms;
  CheckedLoopInvariant out;
  Term scope = nullptr;
  std::vector<Term> atoms;
  std::vector<Term> results; // Adopted candidate terms, or nullptr.
  // The substitution: header variable -> term over parameters and the variables
  // it leaves, each right-hand side already substituted.
  std::map<unsigned, Term> definitions;
};

Term InvariantChecker::conjunction(ArrayRef<Term> list) const {
  Term all = terms.boolean(true);
  for (Term t : list)
    all = terms.logicalAnd(all, t);
  return all;
}

Term InvariantChecker::sigma(Term term) const {
  if (definitions.empty())
    return term;
  return terms.substitute(term, [&](Term leaf) -> Term {
    if (leaf->op != TermOp::State)
      return nullptr;
    auto found = definitions.find(leaf->index);
    return found == definitions.end() ? nullptr : found->second;
  });
}

bool InvariantChecker::define(unsigned variable, Term term, const std::string &origin) {
  if (definitions.count(variable) || !llvm::is_contained(loop.variables, variable))
    return false;
  Term rhs = sigma(term);
  if (llvm::is_contained(stateSupport(rhs), variable))
    return false;
  // Every earlier right-hand side that reads the variable reads its definition now.
  for (auto &[v, t] : definitions)
    t = terms.substitute(t, [&](Term leaf) -> Term {
      if (leaf->op != TermOp::State || leaf->index != variable)
        return nullptr;
      return rhs;
    });
  definitions[variable] = rhs;
  out.eliminated.push_back("s" + std::to_string(variable) + " := " + formatLoopTerm(rhs) + "  (" +
                           origin + ")");
  return true;
}

/// One equation lhs = rhs solved for a header variable, if it has one of the
/// forms v = t, v + u = t, u + v = t, v - u = t, u - v = t (either side), with v
/// read nowhere else. In Z/2^w each of these is equivalent to v = (its solution).
bool InvariantChecker::solve(Term lhs, Term rhs, const std::string &origin) {
  for (unsigned side = 0; side < 2; ++side) {
    Term here = side ? rhs : lhs, other = side ? lhs : rhs;
    auto free = [&](unsigned v, Term t) { return !llvm::is_contained(stateSupport(t), v); };
    if (here->op == TermOp::State) {
      if (free(here->index, other) && define(here->index, other, origin))
        return true;
      continue;
    }
    if ((here->op == TermOp::Add || here->op == TermOp::Sub) && here->args.size() == 2) {
      Term a = here->args[0], b = here->args[1];
      if (a->op == TermOp::State && free(a->index, b) && free(a->index, other)) {
        // a + b = o  =>  a := o - b ;  a - b = o  =>  a := o + b
        Term rhs = here->op == TermOp::Add ? terms.binary(TermOp::Sub, other, b)
                                           : terms.binary(TermOp::Add, other, b);
        if (define(a->index, rhs, origin))
          return true;
      }
      if (b->op == TermOp::State && free(b->index, a) && free(b->index, other)) {
        // a + b = o  =>  b := o - a ;  a - b = o  =>  b := a - o
        Term rhs = here->op == TermOp::Add ? terms.binary(TermOp::Sub, other, a)
                                           : terms.binary(TermOp::Sub, a, other);
        if (define(b->index, rhs, origin))
          return true;
      }
    }
  }
  return false;
}

void InvariantChecker::solveAtoms() {
  for (unsigned k = 0; k < atoms.size(); ++k) {
    Term atom = atoms[k];
    if (atom->op != TermOp::ICmp || (Predicate)atom->aux != Predicate::Eq)
      continue;
    solve(sigma(atom->args[0]), sigma(atom->args[1]), "atom " + std::to_string(k));
  }
}

bool InvariantChecker::adopt() {
  std::string why;
  scope = scopeFormula(program, options.scope, why);
  if (!scope) {
    out.invariantReason = out.functionReason = "the scope is malformed: " + why;
    return false;
  }
  for (unsigned k = 0; k < certificate.atoms.size(); ++k) {
    Term atom = certificate.atoms[k];
    Term adopted = adoptProofTerm(program, atom, why);
    if (!adopted) {
      out.invariantReason = "atom " + std::to_string(k) + " refused: " + why;
      return false;
    }
    if (adopted->sort.width != 1) {
      out.invariantReason = "atom " + std::to_string(k) + " is not a condition";
      return false;
    }
    SmallVector<unsigned> reads = stateSupport(adopted);
    reads.append(shadowSupport(adopted));
    for (unsigned v : reads)
      if (!llvm::is_contained(loop.variables, v)) {
        out.invariantReason = "atom " + std::to_string(k) + " reads s" + std::to_string(v) +
                              ", which is not header state the loop's results depend on";
        return false;
      }
    atoms.push_back(adopted);
  }
  results.assign(loop.resultSorts.size(), nullptr);
  if (function.results.size() > loop.resultSorts.size()) {
    out.functionReason = "the candidate has more results than the function";
    return false;
  }
  for (unsigned j = 0; j < function.results.size(); ++j) {
    Term f = function.results[j];
    if (!f)
      continue;
    Term adopted = adoptProofTerm(program, f, why);
    if (!adopted) {
      out.functionReason = "result " + std::to_string(j) + " refused: " + why;
      return false;
    }
    if (!stateSupport(adopted).empty() || !shadowSupport(adopted).empty()) {
      out.functionReason = "result " + std::to_string(j) +
                           " reads program state: a candidate is a term over the parameters";
      return false;
    }
    if (adopted->sort != loop.resultSorts[j]) {
      out.functionReason = "result " + std::to_string(j) + " has the wrong width";
      return false;
    }
    results[j] = adopted;
  }
  return true;
}

size_t InvariantChecker::ask(const std::string &id, const std::string &location, Term formula,
                             const Aux &aux, bool wantModel, SmtResponse &response) {
  ObligationRecord record;
  record.id = id;
  record.location = location;
  record.expected = "unsat";
  std::string script = obligationScript(id + " " + location, formula, aux);
  ++out.queries;
  out.scriptBytes += script.size();
  response = backend.check(script, options.solver, wantModel);
  record.result = stringifySmtAnswer(response.answer);
  record.discharged = response.answer == SmtAnswer::Unsat;
  if (response.answer == SmtAnswer::Unknown)
    record.evidence = "the solver did not decide: " + response.detail;
  if (options.scripts)
    record.script = std::move(script);
  out.records.push_back(std::move(record));
  return out.records.size() - 1;
}

std::string InvariantChecker::modelText(const SmtResponse &response) const {
  std::string text;
  for (const auto &[name, value] : response.model) {
    if (name.rfind("h", 0) == 0 && name.size() > 1 && std::isdigit((unsigned char)name[1]))
      continue; // Atom values are reported as the falsified list.
    if (!text.empty())
      text += " ";
    text += name + "=" + value;
  }
  return text.empty() ? "(empty model)" : text;
}

/// INV-init (step false) or INV-step (step true) over every path of the kind. The
/// conjunction first; when the solver does not decide it, one query per atom.
bool InvariantChecker::induction(bool step) {
  const char *id = step ? "INV-step" : "INV-init";
  Term inv = conjunction(atoms);
  for (unsigned p : step ? loop.iterate : loop.entry) {
    const CutPath &path = loop.set.paths[p];
    std::string location = pathText(program, path);
    Term source = step ? inv : terms.boolean(true);
    // The substitution may be applied only where Inv is a premise: INV-step. An entry
    // path assigns every header variable, so INV-init reads none of them anyway.
    auto state = [&](Term t) { return step ? sigma(t) : t; };
    Term formula = state(invariantViolation(program, scope, path, source, inv));
    Aux aux;
    for (unsigned k = 0; k < atoms.size(); ++k)
      aux.push_back({"h" + std::to_string(k), state(postTerm(program, path, atoms[k]))});
    SmtResponse response;
    size_t index = ask(id, location, formula, aux, /*wantModel=*/true, response);
    if (response.answer == SmtAnswer::Unsat)
      continue;
    if (response.answer == SmtAnswer::Sat) {
      std::vector<unsigned> falsified;
      for (unsigned k = 0; k < atoms.size(); ++k) {
        auto value = response.model.find("h" + std::to_string(k));
        if (value == response.model.end() || value->second != "1")
          falsified.push_back(k);
      }
      std::string list;
      for (unsigned k : falsified)
        list += (list.empty() ? "" : ", ") + std::to_string(k);
      out.records[index].evidence =
          std::string(step ? "after one step of this path, " : "at an initial state, ") +
          "atom(s) " + list + " false; model " + modelText(response);
      if (out.invariant == InvariantVerdict::NotInductive)
        return false; // The first failure stands as the reason.
      out.invariant = InvariantVerdict::NotInductive;
      out.invariantReason = std::string(id) + " has a model on " + location +
                            (step ? ": the conjunction is not inductive (the model's state need "
                                    "not be reachable)"
                                  : ": an initial state violates the invariant");
      out.falsifiedBy = id;
      out.falsified = falsified;
      if (!step) {
        // An initial state: its parameters, for a proposer that wants a run from them.
        for (const Parameter &param : program.parameters) {
          auto value = response.model.find("p" + std::to_string(param.id));
          out.initialParameters.push_back(value == response.model.end()
                                              ? APInt::getZero(param.sort.width)
                                              : APInt(param.sort.width, value->second, 10));
        }
        if (!parametersInScope(options.scope, out.initialParameters))
          out.initialParameters.clear();
      }
      return false;
    }
    // Undecided together: one atom at a time.
    std::vector<unsigned> falsified, undecided;
    for (unsigned k = 0; k < atoms.size(); ++k) {
      Term single = state(invariantViolation(program, scope, path, source, atoms[k]));
      SmtResponse alone;
      size_t at = ask(std::string(id) + "[atom " + std::to_string(k) + "]", location, single, {},
                      /*wantModel=*/true, alone);
      if (alone.answer == SmtAnswer::Sat) {
        falsified.push_back(k);
        out.records[at].evidence = "model " + modelText(alone);
      } else if (alone.answer == SmtAnswer::Unknown) {
        undecided.push_back(k);
      }
    }
    if (!falsified.empty()) {
      if (out.invariant == InvariantVerdict::NotInductive)
        return false;
      out.invariant = InvariantVerdict::NotInductive;
      out.invariantReason = std::string(id) + " has a model on " + location + " (atom by atom)";
      out.falsifiedBy = id;
      out.falsified = falsified;
      out.undecided = undecided;
      return false;
    }
    if (!undecided.empty()) {
      if (out.invariant == InvariantVerdict::NotInductive)
        return false;
      out.invariant = InvariantVerdict::Unknown;
      out.invariantReason = std::string(id) + " on " + location +
                            ": the solver decided neither the conjunction nor every atom";
      out.undecided = undecided;
      return false;
    }
    // Every atom alone is unsat: their conjunction is (an atom of Inv at the target
    // follows from Inv at the source for each atom, so all of them do).
    out.records[index].evidence = "the conjunction was not decided; every atom alone is unsat";
  }
  return true;
}

Term InvariantChecker::post(ArrayRef<Term> returned) const {
  Term all = terms.boolean(true);
  for (unsigned j = 0; j < results.size(); ++j)
    if (results[j])
      all = terms.logicalAnd(all, terms.icmp(Predicate::Eq, returned[j], results[j]));
  return all;
}

std::optional<LoopCounterexample>
InvariantChecker::confirm(const std::map<std::string, std::string> &model, const std::string &source,
                          std::string &note) {
  EntryState entry;
  for (const Parameter &p : program.parameters) {
    unsigned w = p.sort.width;
    APInt value = APInt::getZero(w);
    auto found = model.find("p" + std::to_string(p.id));
    if (found != model.end()) {
      value = APInt(w, found->second, 10);
    } else {
      // The parameter is unconstrained in the query: the lower end of its range,
      // or 0.
      for (const ParameterRange &range : options.scope.ranges)
        if (range.parameter == p.id)
          value = range.lower;
    }
    entry.parameters.push_back(value);
  }
  if (!parametersInScope(options.scope, entry.parameters)) {
    note = "the model's parameters leave the scope";
    return std::nullopt;
  }
  RunResult run = runConcrete(program, entry, options.confirmation);
  ++out.confirmationRuns;
  out.confirmationSteps += run.steps;
  std::string inputs;
  for (unsigned i = 0; i < entry.parameters.size(); ++i)
    inputs += (i ? ", p" : "p") + std::to_string(i) + "=" + decimal(entry.parameters[i]);
  if (run.status != RunStatus::Returned) {
    note = "the run from " + inputs + " ended " + stringifyRunStatus(run.status) +
           " after " + std::to_string(run.steps) + " steps, so it confirms nothing";
    return std::nullopt;
  }
  Configuration empty;
  empty.state.assign(program.state.size(), std::nullopt);
  empty.shadow.assign(program.state.size(), false);
  LoopCounterexample cex;
  cex.parameters = entry.parameters;
  cex.returned = run.returns;
  cex.steps = run.steps;
  cex.source = source;
  bool differs = false;
  for (unsigned j = 0; j < results.size(); ++j) {
    std::optional<APInt> expected;
    if (results[j])
      expected = evaluateTerm(program, entry, empty, results[j]);
    cex.expected.push_back(expected);
    if (expected && j < run.returns.size() && *expected != run.returns[j])
      differs = true;
  }
  if (!differs) {
    note = "the run from " + inputs + " returns the candidate's value(s) after " +
           std::to_string(run.steps) + " steps: the model's header state is not reached from them";
    return std::nullopt;
  }
  note = "confirmed: the run from " + inputs + " returns another value";
  return cex;
}

void InvariantChecker::exits() {
  Term inv = conjunction(atoms);
  bool anyResult = llvm::any_of(results, [](Term t) { return t != nullptr; });
  bool provedAll = true;
  for (unsigned p : loop.leave) {
    const CutPath &path = loop.set.paths[p];
    std::string location = pathText(program, path);
    // Equalities the leaving condition and Inv imply, read off the comparisons the
    // condition makes: INV-exit-eq, each proved before it is used.
    std::map<unsigned, Term> saved = definitions;
    size_t eliminatedBefore = out.eliminated.size();
    std::vector<std::pair<Term, Term>> pairs;
    DenseSet<Term> seen;
    std::function<void(Term)> comparisons = [&](Term t) {
      if (!seen.insert(t).second)
        return;
      if (t->op == TermOp::ICmp && t->args[0]->sort.kind == Sort::BV) {
        Term a = t->args[0], b = t->args[1];
        bool readsState = !stateSupport(a).empty() || !stateSupport(b).empty();
        if (readsState && !containsOp(a, TermOp::Shadow) && !containsOp(b, TermOp::Shadow))
          pairs.push_back({a, b});
      }
      for (Term arg : t->args)
        comparisons(arg);
    };
    comparisons(path.condition);
    for (auto [a, b] : pairs) {
      Term sa = sigma(a), sb = sigma(b);
      if (sa == sb)
        continue;
      Term formula = sigma(terms.logicalAnd(
          terms.logicalAnd(terms.logicalAnd(scope, inv), path.condition),
          terms.icmp(Predicate::Ne, a, b)));
      SmtResponse response;
      size_t index = ask("INV-exit-eq", location, formula, {}, /*wantModel=*/false, response);
      std::string lemma = formatLoopTerm(a) + " == " + formatLoopTerm(b);
      out.records[index].evidence = "lemma " + lemma;
      if (response.answer != SmtAnswer::Unsat)
        continue;
      bool used = solve(sa, sb, "INV-exit-eq: " + lemma);
      out.lemmas.push_back(lemma + (used ? "" : " (proved, not used)"));
    }
    if (!anyResult) {
      definitions = saved;
      continue;
    }
    std::vector<Term> returned = loop.returns.at(p);
    Term premise = terms.logicalAnd(terms.logicalAnd(scope, inv), path.condition);
    Term formula = sigma(terms.logicalAnd(premise, terms.logicalNot(post(returned))));
    Aux aux;
    for (unsigned v : loop.variables)
      aux.push_back({"hdr_s" + std::to_string(v), sigma(terms.state(v, program.state[v].sort))});
    for (unsigned j = 0; j < results.size(); ++j)
      if (results[j]) {
        aux.push_back({"ret" + std::to_string(j), sigma(returned[j])});
        aux.push_back({"cand" + std::to_string(j), results[j]});
      }
    SmtResponse response;
    size_t index = ask("INV-exit", location, formula, aux, /*wantModel=*/true, response);
    std::string used;
    for (size_t e = eliminatedBefore; e < out.eliminated.size(); ++e)
      used += (used.empty() ? "" : "; ") + out.eliminated[e];
    if (response.answer == SmtAnswer::Unsat) {
      if (!used.empty())
        out.records[index].evidence = "stated with " + used;
      definitions = saved;
      continue;
    }
    provedAll = false;
    if (response.answer == SmtAnswer::Unknown) {
      if (out.function != LoopFunctionVerdict::Refuted) {
        out.functionReason = "INV-exit on " + location + ": the solver did not decide (" +
                             response.detail + ")";
      }
      definitions = saved;
      continue;
    }
    // A model. It refutes the function candidate only through a run.
    std::string note;
    std::string model = modelText(response);
    out.records[index].evidence = "model " + model;
    auto cex = confirm(response.model, "INV-exit model on " + location, note);
    out.records[index].evidence += "; " + note;
    // A run decides only where it ends within its fuel. Ask again with every
    // parameter small, then with one parameter small at a time (an iteration count
    // small, another parameter wherever the model needs it).
    std::vector<std::pair<std::string, Term>> narrowings;
    for (int64_t bound : options.smallInputBounds) {
      std::string range = "[-" + std::to_string(bound) + ", " + std::to_string(bound) + "]";
      auto within = [&](const Parameter &param) -> Term {
        unsigned w = param.sort.width;
        if (w < 2 || !APInt(64, bound).isSignedIntN(w) || !APInt(64, -bound, true).isSignedIntN(w))
          return nullptr;
        Term value = terms.param(param.id, param.sort);
        return terms.logicalAnd(terms.icmp(Predicate::Sge, value, terms.bv(APInt(w, -bound, true))),
                                terms.icmp(Predicate::Sle, value, terms.bv(APInt(w, bound, true))));
      };
      Term all = terms.boolean(true);
      unsigned count = 0;
      for (const Parameter &param : program.parameters)
        if (Term t = within(param)) {
          all = terms.logicalAnd(all, t);
          ++count;
        }
      if (count)
        narrowings.push_back({"every parameter in " + range, all});
      if (count > 1)
        for (const Parameter &param : program.parameters)
          if (Term t = within(param))
            narrowings.push_back({"p" + std::to_string(param.id) + " in " + range, t});
    }
    for (const auto &[text, constraint] : narrowings) {
      if (cex)
        break;
      SmtResponse again;
      size_t at = ask("INV-exit-small", location + " with " + text,
                      terms.logicalAnd(formula, constraint), aux, /*wantModel=*/true, again);
      if (again.answer != SmtAnswer::Sat) {
        if (again.answer == SmtAnswer::Unsat)
          out.records[at].evidence = "no INV-exit model with " + text;
        continue;
      }
      std::string smallNote;
      cex = confirm(again.model, "INV-exit model with " + text + " on " + location, smallNote);
      out.records[at].evidence = "model " + modelText(again) + "; " + smallNote;
      if (!cex)
        note += "; " + smallNote;
    }
    if (cex) {
      out.function = LoopFunctionVerdict::Refuted;
      out.functionReason = "a run of the program returns another value: " + cex->source;
      out.counterexample = cex;
    } else if (out.function != LoopFunctionVerdict::Refuted) {
      out.unconfirmedExitModel = model;
      out.functionReason = "INV-exit on " + location + " has a model no run confirms (" + note +
                           "): the invariant may be too weak, and the model refutes nothing";
    }
    definitions = saved;
  }
  if (anyResult && provedAll && out.function != LoopFunctionVerdict::Refuted)
    out.function = LoopFunctionVerdict::Proved;
}

void InvariantChecker::bypasses() {
  if (!llvm::any_of(results, [](Term t) { return t != nullptr; }))
    return;
  for (unsigned p : loop.bypass) {
    const CutPath &path = loop.set.paths[p];
    std::string location = pathText(program, path);
    Term formula = terms.logicalAnd(terms.logicalAnd(scope, path.condition),
                                    terms.logicalNot(post(loop.returns.at(p))));
    SmtResponse response;
    size_t index = ask("RET-bypass", location, formula, {}, /*wantModel=*/true, response);
    if (response.answer == SmtAnswer::Unsat)
      continue;
    if (out.function == LoopFunctionVerdict::Proved)
      out.function = LoopFunctionVerdict::Unknown;
    if (response.answer == SmtAnswer::Unknown) {
      if (out.function != LoopFunctionVerdict::Refuted)
        out.functionReason = "RET-bypass on " + location + ": the solver did not decide";
      continue;
    }
    std::string note;
    auto cex = confirm(response.model, "RET-bypass model on " + location, note);
    out.records[index].evidence = "model " + modelText(response) + "; " + note;
    if (cex) {
      out.function = LoopFunctionVerdict::Refuted;
      out.functionReason = "a run of the program returns another value: " + cex->source;
      out.counterexample = cex;
    } else if (out.function != LoopFunctionVerdict::Refuted) {
      out.functionReason = "RET-bypass on " + location + " has a model no run confirms (" + note + ")";
    }
  }
}

/// Informational: can a returned value be poison? Only asked for a proved candidate.
void InvariantChecker::poison() {
  Term inv = conjunction(atoms);
  bool excluded = true;
  std::vector<unsigned> returning = loop.leave;
  returning.insert(returning.end(), loop.bypass.begin(), loop.bypass.end());
  for (unsigned p : returning) {
    const CutPath &path = loop.set.paths[p];
    Term any = terms.boolean(false);
    for (unsigned j = 0; j < results.size(); ++j)
      if (results[j])
        any = terms.logicalOr(any, loop.returnPoison.at(p)[j]);
    if (any == terms.boolean(false))
      continue;
    Term source = path.fromEntry ? terms.boolean(true) : inv;
    Term formula = terms.logicalAnd(
        terms.logicalAnd(terms.logicalAnd(scope, source), path.condition), any);
    if (!path.fromEntry)
      formula = sigma(formula); // Inv is a premise only on a path from the header.
    SmtResponse response;
    size_t index = ask("INV-poison", pathText(program, path), formula, {}, /*wantModel=*/true, response);
    out.records[index].expected = "unsat (informational)";
    if (response.answer != SmtAnswer::Unsat) {
      excluded = false;
      if (response.answer == SmtAnswer::Sat)
        out.records[index].evidence =
            "model " + modelText(response) + ": a returned value may be poison here, as far as Inv "
            "says (the state need not be reachable)";
    }
  }
  out.poison = excluded ? "excluded" : "not excluded";
}

CheckedLoopInvariant InvariantChecker::run() {
  if (!loop.supported()) {
    out.invariantReason = out.functionReason = "unsupported: " + loop.reason;
    return out;
  }
  if (!adopt()) {
    if (out.functionReason.empty())
      out.functionReason = "the certificate was refused: " + out.invariantReason;
    return out;
  }
  solveAtoms();
  bool init = induction(/*step=*/false);
  if (!init && options.failFast) {
    out.functionReason = "the invariant failed INV-init, so it proves nothing";
    return out;
  }
  bool step = (init || !options.failFast) ? induction(/*step=*/true) : false;
  if (init && step) {
    out.invariant = InvariantVerdict::Inductive;
    out.invariantReason = "INV-init and INV-step unsat on every path";
  }
  if (out.invariant != InvariantVerdict::Inductive) {
    if (out.functionReason.empty())
      out.functionReason = "the invariant is not shown inductive, so it proves nothing";
    if (options.failFast)
      return out;
  }
  bool anyResult = llvm::any_of(results, [](Term t) { return t != nullptr; });
  if (!anyResult) {
    out.functionReason = "no function candidate: only the invariant was checked";
    return out;
  }
  if (out.invariant == InvariantVerdict::Inductive) {
    exits();
  }
  bypasses();
  if (out.invariant != InvariantVerdict::Inductive && out.function == LoopFunctionVerdict::Proved)
    out.function = LoopFunctionVerdict::Unknown;
  if (out.function == LoopFunctionVerdict::Proved) {
    out.functionReason = "partial correctness: INV-init, INV-step, INV-exit and RET-bypass unsat "
                         "on every path; no iteration of the loop was executed";
    poison();
  }
  return out;
}

} // namespace

CheckedLoopInvariant checkLoopInvariant(const SemanticProgram &program, const InvariantLoop &loop,
                                        const InvariantCertificate &certificate,
                                        const LoopFunction &function,
                                        const InvariantCheckOptions &options, SmtBackend &backend) {
  InvariantChecker checker(program, loop, certificate, function, options, backend);
  try {
    return checker.run();
  } catch (const TermError &error) {
    CheckedLoopInvariant out;
    out.invariantReason = out.functionReason = std::string("a term could not be built: ") + error.what();
    return out;
  }
}

LoopSamples sampleLoop(const SemanticProgram &program, const InvariantLoop &loop,
                       ArrayRef<std::vector<APInt>> points, const RunLimits &limits,
                       size_t maxStates) {
  LoopSamples out;
  if (!loop.supported())
    return out;
  for (const std::vector<APInt> &point : points) {
    if (out.states.size() >= maxStates)
      break;
    EntryState entry;
    entry.parameters = point;
    std::string error;
    auto config = initialConfiguration(program, entry, limits, error);
    if (!config)
      continue;
    ++out.runs;
    out.runParameters.push_back(point);
    out.runReturns.push_back(std::nullopt);
    std::vector<APInt> returns;
    std::vector<bool> returnPoison;
    uint64_t arrivals = 0;
    for (uint64_t step = 0; step <= limits.fuel; ++step) {
      if (config->location == loop.header) {
        ++arrivals;
        ++out.arrivals;
        std::vector<APInt> row;
        bool defined = true;
        for (unsigned v : loop.variables) {
          if (!config->state[v]) {
            defined = false;
            break;
          }
          row.push_back(*config->state[v]);
        }
        if (defined && out.states.size() < maxStates) {
          for (unsigned v : loop.variables)
            row.push_back(APInt(1, config->shadow[v] ? 1 : 0));
          for (unsigned p : loop.parameters)
            row.push_back(point[p]);
          out.states.push_back(std::move(row));
        }
      }
      if (config->location == program.exitLocation()) {
        ++out.returned;
        out.runReturns.back() = returns;
        break;
      }
      if (step == limits.fuel)
        break;
      StepOutcome outcome = stepConfiguration(program, entry, *config, &returns, &returnPoison);
      ++out.steps;
      if (outcome.stopped)
        break;
    }
    out.maxArrivals = std::max(out.maxArrivals, arrivals);
  }
  return out;
}

std::vector<std::vector<APInt>> loopPoints(const SemanticProgram &program, const ParameterScope &scope,
                                           uint64_t maxPoints, bool &exhaustive) {
  exhaustive = false;
  std::vector<std::vector<APInt>> out;
  unsigned n = program.parameters.size();
  std::vector<std::optional<std::pair<APInt, APInt>>> ranges(n);
  if (!scope.allParameters)
    for (const ParameterRange &range : scope.ranges)
      if (range.parameter < n)
        ranges[range.parameter] = {range.lower, range.upper};
  // Every point, when the product is small.
  bool bounded = n > 0 && llvm::all_of(ranges, [](const auto &r) { return r.has_value(); });
  if (bounded) {
    uint64_t product = 1;
    for (const auto &r : ranges) {
      unsigned w = r->first.getBitWidth();
      APInt low = r->first.sext(w + 2), high = r->second.sext(w + 2);
      if (low.sgt(high)) {
        product = 0;
        break;
      }
      APInt size = high - low + 1;
      if (size.ugt(maxPoints) || (product = product * size.getZExtValue()) > maxPoints) {
        product = maxPoints + 1;
        break;
      }
    }
    if (product <= maxPoints) {
      std::vector<APInt> point;
      for (const auto &r : ranges)
        point.push_back(r->first);
      for (uint64_t k = 0; k < product; ++k) {
        out.push_back(point);
        for (unsigned i = 0; i < n; ++i) {
          if (point[i] != ranges[i]->second) {
            ++point[i];
            break;
          }
          point[i] = ranges[i]->first;
        }
      }
      exhaustive = true;
      return out;
    }
  }
  if (n == 0) {
    out.push_back({});
    exhaustive = true;
    return out;
  }
  // Small values inside each range, nearest to zero first.
  static const int64_t small[] = {0, 1, 2, 3, -1, 4, 5, -2, 6, 7, -3, 9, 12, -5};
  std::vector<std::vector<APInt>> values(n);
  for (unsigned i = 0; i < n; ++i) {
    unsigned w = program.parameters[i].sort.width;
    for (int64_t v : small) {
      if (!APInt(64, v, true).isSignedIntN(w))
        continue;
      APInt value(w, v, true);
      if (ranges[i] && (value.slt(ranges[i]->first) || value.sgt(ranges[i]->second)))
        continue;
      if (!llvm::is_contained(values[i], value))
        values[i].push_back(value);
    }
    if (ranges[i]) {
      for (const APInt &end : {ranges[i]->first, ranges[i]->second}) {
        // An end close to zero only: a run at a large bound would cost its fuel.
        if (end.getSignificantBits() <= 7 && !llvm::is_contained(values[i], end))
          values[i].push_back(end);
      }
      if (values[i].empty())
        values[i].push_back(ranges[i]->first);
    }
  }
  // The product, when it fits; else a deterministic pseudo-random selection that
  // starts at the all-smallest point.
  uint64_t product = 1;
  for (const auto &list : values) {
    product *= list.size();
    if (product > maxPoints)
      break;
  }
  std::set<std::vector<unsigned>> chosen;
  auto add = [&](const std::vector<unsigned> &pick) {
    if (!chosen.insert(pick).second)
      return;
    std::vector<APInt> point;
    for (unsigned i = 0; i < n; ++i)
      point.push_back(values[i][pick[i]]);
    out.push_back(point);
  };
  if (product <= maxPoints) {
    std::vector<unsigned> pick(n, 0);
    for (uint64_t k = 0; k < product; ++k) {
      add(pick);
      for (unsigned i = 0; i < n; ++i) {
        if (++pick[i] < values[i].size())
          break;
        pick[i] = 0;
      }
    }
    return out;
  }
  add(std::vector<unsigned>(n, 0));
  uint64_t state = 0x9E3779B97F4A7C15ull;
  auto next = [&]() {
    state ^= state << 13;
    state ^= state >> 7;
    state ^= state << 17;
    return state;
  };
  for (unsigned tries = 0; out.size() < maxPoints && tries < maxPoints * 16; ++tries) {
    std::vector<unsigned> pick(n);
    for (unsigned i = 0; i < n; ++i)
      pick[i] = next() % values[i].size();
    add(pick);
  }
  return out;
}

LoopRunCheck checkLoopFunctionOnRuns(const SemanticProgram &program, const InvariantLoop &loop,
                                     const LoopFunction &function, ArrayRef<std::vector<APInt>> points,
                                     bool exhaustive, const RunLimits &limits) {
  LoopRunCheck out;
  out.exhaustive = exhaustive;
  bool anyResult = llvm::any_of(function.results, [](Term t) { return t != nullptr; });
  if (!loop.supported() || !anyResult) {
    out.status = "UNKNOWN";
    return out;
  }
  Configuration empty;
  empty.state.assign(program.state.size(), std::nullopt);
  empty.shadow.assign(program.state.size(), false);
  for (const std::vector<APInt> &point : points) {
    ++out.points;
    EntryState entry;
    entry.parameters = point;
    RunResult run = runConcrete(program, entry, limits);
    out.steps += run.steps;
    if (run.status == RunStatus::OutOfFuel) {
      ++out.outOfFuel;
      continue;
    }
    if (run.status != RunStatus::Returned) {
      ++out.stopped;
      continue;
    }
    ++out.returned;
    bool same = true;
    LoopCounterexample cex;
    for (unsigned j = 0; j < function.results.size() && j < run.returns.size(); ++j) {
      std::optional<APInt> expected;
      if (function.results[j])
        expected = evaluateTerm(program, entry, empty, function.results[j]);
      cex.expected.push_back(expected);
      if (expected && *expected != run.returns[j])
        same = false;
    }
    if (same) {
      ++out.equal;
      continue;
    }
    if (!out.counterexample) {
      cex.parameters = point;
      cex.returned = run.returns;
      cex.steps = run.steps;
      cex.source = exhaustive ? "a point of the enumerated scope" : "a sampled point";
      out.counterexample = cex;
    }
  }
  if (out.counterexample)
    out.status = "REFUTED";
  else if (out.returned == 0)
    out.status = "UNKNOWN";
  else if (exhaustive && out.returned == out.points)
    out.status = "SCOPE_ENUMERATED";
  else
    out.status = "SAMPLE_CONSISTENT";
  return out;
}

std::string invariantSpecificationJSON(const SemanticProgram &program, const InvariantLoop &loop,
                                       const LoopFunction &function, const ParameterScope &scope) {
  json::Object root;
  if (!loop.supported()) {
    root["reason"] = loop.reason;
    std::string text;
    raw_string_ostream os(text);
    os << json::Value(std::move(root));
    return os.str();
  }
  root["header"] = (int64_t)loop.header;
  json::Array variables;
  for (unsigned v : loop.variables) {
    const StateVar &var = program.state[v];
    std::string where = var.kind == StateVar::BlockArgument
                            ? "argument " + std::to_string(var.position) + " of block " +
                                  std::to_string(var.location)
                        : var.kind == StateVar::IterationCounter
                            ? "iteration counter of block " + std::to_string(var.location)
                            : "result of op " + std::to_string(var.position) + " of block " +
                                  std::to_string(var.location);
    variables.push_back(json::Object{{"name", "s" + std::to_string(v)},
                                     {"sort", var.sort.str()},
                                     {"is", where}});
  }
  root["variables"] = std::move(variables);
  json::Array parameters;
  for (unsigned p : loop.parameters)
    parameters.push_back(json::Object{{"name", "p" + std::to_string(p)},
                                      {"sort", program.parameters[p].sort.str()},
                                      {"is", "function argument " +
                                                 std::to_string(program.parameters[p].argument)}});
  root["parameters"] = std::move(parameters);
  std::string why;
  Term scopeTerm = scopeFormula(program, scope, why);
  root["scope"] = scopeTerm ? formatLoopTerm(scopeTerm) : "malformed: " + why;
  auto updates = [&](const CutPath &path, bool frame) {
    json::Object assigned;
    json::Array unchanged;
    for (unsigned v : loop.variables) {
      if (Term t = path.state[v])
        assigned["s" + std::to_string(v) + "'"] = formatLoopTerm(t);
      else if (frame)
        unchanged.push_back("s" + std::to_string(v) + "' = s" + std::to_string(v));
    }
    json::Object out{{"path", pathText(program, path)},
                     {"condition", formatLoopTerm(path.condition)},
                     {"update", std::move(assigned)}};
    if (frame)
      out["frame"] = std::move(unchanged);
    return out;
  };
  json::Array pre, transition, exits, bypass;
  for (unsigned p : loop.entry)
    pre.push_back(updates(loop.set.paths[p], /*frame=*/false));
  for (unsigned p : loop.iterate)
    transition.push_back(updates(loop.set.paths[p], /*frame=*/true));
  auto returning = [&](unsigned p) {
    const CutPath &path = loop.set.paths[p];
    json::Array values, poison;
    for (Term t : loop.returns.at(p))
      values.push_back(formatLoopTerm(t));
    for (Term t : loop.returnPoison.at(p))
      poison.push_back(formatLoopTerm(t));
    return json::Object{{"path", pathText(program, path)},
                        {"condition", formatLoopTerm(path.condition)},
                        {"returns", std::move(values)},
                        {"poison", std::move(poison)}};
  };
  for (unsigned p : loop.leave)
    exits.push_back(returning(p));
  for (unsigned p : loop.bypass)
    bypass.push_back(returning(p));
  root["pre"] = std::move(pre);
  root["transition"] = std::move(transition);
  root["exit"] = std::move(exits);
  root["bypass"] = std::move(bypass);
  json::Array post;
  for (unsigned j = 0; j < function.results.size(); ++j)
    if (function.results[j])
      post.push_back("r" + std::to_string(j) + " == " + formatLoopTerm(function.results[j]));
  root["post"] = std::move(post);
  root["guard"] = "G = the disjunction of the transition conditions; X = an exit condition";
  std::string text;
  raw_string_ostream os(text);
  os << json::Value(std::move(root));
  return os.str();
}

} // namespace mlir::generalized_gemm
