#include "GeneralizedGemm/RelationalEncoding.h"

#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/SmallString.h"

#include <functional>
#include <map>
#include <set>

using namespace mlir::generalized_gemm;
using llvm::APInt;

namespace {

constexpr unsigned LocationBits = 16;

std::string decimal(const APInt &value) {
  llvm::SmallString<32> text;
  value.toStringUnsigned(text, 10);
  return text.str().str();
}
std::string bvLiteral(const APInt &value) {
  return "(_ bv" + decimal(value) + " " + std::to_string(value.getBitWidth()) + ")";
}
std::string floatSort(unsigned width) {
  return width == 32 ? "(_ FloatingPoint 8 24)" : "(_ FloatingPoint 11 53)";
}
std::string floatLiteral(const APInt &bits) {
  std::string conversion =
      bits.getBitWidth() == 32 ? "(_ to_fp 8 24)" : "(_ to_fp 11 53)";
  return "(" + conversion + " " + bvLiteral(bits) + ")";
}
std::string smtSort(const Sort &sort) {
  switch (sort.kind) {
  case Sort::BV: return "(_ BitVec " + std::to_string(sort.width) + ")";
  case Sort::Float: return floatSort(sort.width);
  case Sort::Mem: {
    unsigned key = sort.rank ? sort.rank * sort.indexWidth : 1;
    return "(Array (_ BitVec " + std::to_string(key) + ") " +
           smtSort(sort.element()) + ")";
  }
  }
  return "?";
}
std::string literal(const Sort &sort, const APInt &bits) {
  return sort.kind == Sort::Float ? floatLiteral(bits) : bvLiteral(bits);
}
std::string location(unsigned id) {
  return "(_ bv" + std::to_string(id) + " " + std::to_string(LocationBits) + ")";
}
std::string name(Term t) { return "t" + std::to_string(t->id); }

std::string key(llvm::ArrayRef<std::string> index) {
  if (index.empty())
    return "#b0";
  std::string out = index.back();
  for (int i = (int)index.size() - 2; i >= 0; --i)
    out = "(concat " + index[i] + " " + out + ")";
  return out;
}

std::string expression(Term t) {
  auto a = [&](unsigned i) { return name(t->args[i]); };
  auto boolean = [](const std::string &text) {
    return "(ite " + text + " #b1 #b0)";
  };
  auto binary = [&](const char *op) {
    return "(" + std::string(op) + " " + a(0) + " " + a(1) + ")";
  };
  auto keyOf = [&](unsigned first) {
    llvm::SmallVector<std::string> index;
    for (unsigned i = first; i < t->args.size(); ++i)
      index.push_back(a(i));
    return key(index);
  };
  switch (t->op) {
  case TermOp::Const: return bvLiteral(t->value);
  case TermOp::FConst: return floatLiteral(t->value);
  case TermOp::Param: return "p" + std::to_string(t->index);
  case TermOp::State: return "s" + std::to_string(t->index);
  case TermOp::Shadow: return "sp" + std::to_string(t->index);
  case TermOp::MemVar: return "m" + std::to_string(t->index);
  case TermOp::Add: return binary("bvadd");
  case TermOp::Sub: return binary("bvsub");
  case TermOp::Mul: return binary("bvmul");
  case TermOp::SDiv: return binary("bvsdiv");
  case TermOp::UDiv: return binary("bvudiv");
  case TermOp::SRem: return binary("bvsrem");
  case TermOp::URem: return binary("bvurem");
  case TermOp::Shl: return binary("bvshl");
  case TermOp::LShr: return binary("bvlshr");
  case TermOp::AShr: return binary("bvashr");
  case TermOp::And: return binary("bvand");
  case TermOp::Or: return binary("bvor");
  case TermOp::Xor: return binary("bvxor");
  case TermOp::Not: return "(bvnot " + a(0) + ")";
  case TermOp::ICmp: {
    static const char *const names[] = {"=",     "distinct", "bvslt", "bvsle",
                                        "bvsgt", "bvsge",    "bvult", "bvule",
                                        "bvugt", "bvuge"};
    return boolean(binary(names[t->aux]));
  }
  case TermOp::Ite:
    return "(ite (= " + a(0) + " #b1) " + a(1) + " " + a(2) + ")";
  case TermOp::ZExt:
    return "((_ zero_extend " +
           std::to_string(t->sort.width - t->args[0]->sort.width) + ") " + a(0) + ")";
  case TermOp::SExt:
    return "((_ sign_extend " +
           std::to_string(t->sort.width - t->args[0]->sort.width) + ") " + a(0) + ")";
  case TermOp::Trunc:
    return "((_ extract " + std::to_string(t->sort.width - 1) + " 0) " + a(0) + ")";
  case TermOp::Load: return "(select " + a(0) + " " + keyOf(1) + ")";
  case TermOp::Store:
    return "(store " + a(0) + " " + keyOf(2) + " " + a(1) + ")";
  case TermOp::FAdd: return "(fp.add RNE " + a(0) + " " + a(1) + ")";
  case TermOp::FMul: return "(fp.mul RNE " + a(0) + " " + a(1) + ")";
  case TermOp::FIsNaN: return boolean("(fp.isNaN " + a(0) + ")");
  case TermOp::FIsInf: return boolean("(fp.isInfinite " + a(0) + ")");
  case TermOp::FIsZero: return boolean("(fp.isZero " + a(0) + ")");
  }
  throw TermError("no encoding for a term");
}

std::string conjunction(const std::vector<std::string> &terms) {
  if (terms.empty())
    return "true";
  if (terms.size() == 1)
    return terms[0];
  std::string out = "(and";
  for (const std::string &t : terms)
    out += " " + t;
  return out + ")";
}
std::string disjunction(const std::vector<std::string> &terms) {
  if (terms.empty())
    return "false";
  if (terms.size() == 1)
    return terms[0];
  std::string out = "(or";
  for (const std::string &t : terms)
    out += " " + t;
  return out + ")";
}
std::string isTrue(Term t) { return "(= " + name(t) + " #b1)"; }

void collectNodes(Term t, std::map<unsigned, Term> &used) {
  if (!used.emplace(t->id, t).second)
    return;
  for (Term a : t->args)
    collectNodes(a, used);
}

// Children have smaller ids, so id order is a definition order.
std::string defineNodes(const std::map<unsigned, Term> &used) {
  std::string out;
  for (const auto &[id, term] : used)
    out += "(define-fun " + name(term) + " () " + smtSort(term->sort) + " " +
           expression(term) + ")\n";
  return out;
}

} // namespace

namespace mlir::generalized_gemm {

std::string smtSortText(const Sort &sort) { return smtSort(sort); }
std::string smtTermName(Term term) { return name(term); }

std::string encodeTerms(llvm::ArrayRef<Term> roots) {
  std::map<unsigned, Term> used;
  for (Term root : roots)
    collectNodes(root, used);
  std::string out;
  for (const auto &[id, term] : used)
    if (term->op == TermOp::Param || term->op == TermOp::State ||
        term->op == TermOp::Shadow || term->op == TermOp::MemVar)
      out += "(declare-const " + expression(term) + " " + smtSort(term->sort) +
             ")\n";
  return out + defineNodes(used);
}

std::string encodeTransitionRelation(const SemanticProgram &program) {
  if (!program.isSupported())
    return "; unsupported: no relation\n";
  std::string out = "; function " + program.functionName + " sha256 " +
                    program.fingerprint + "\n";
  std::string locationSort = "(_ BitVec " + std::to_string(LocationBits) + ")";
  auto declare = [&](const std::string &n, const std::string &sort) {
    out += "(declare-const " + n + " " + sort + ")\n";
  };
  declare("loc", locationSort);
  declare("loc_n", locationSort);
  for (const Parameter &p : program.parameters)
    declare("p" + std::to_string(p.id), smtSort(p.sort));
  for (const StateVar &s : program.state) {
    std::string id = std::to_string(s.id);
    declare("s" + id, smtSort(s.sort));
    declare("s" + id + "_n", smtSort(s.sort));
    declare("sp" + id, "(_ BitVec 1)");
    declare("sp" + id + "_n", "(_ BitVec 1)");
  }
  for (const MemoryRoot &r : program.roots) {
    std::string id = std::to_string(r.id);
    declare("m" + id, smtSort(r.sort));
    declare("m" + id + "_n", smtSort(r.sort));
  }
  for (unsigned i = 0; i < program.returnSorts.size(); ++i) {
    declare("r" + std::to_string(i), smtSort(program.returnSorts[i]));
    declare("rp" + std::to_string(i), "(_ BitVec 1)");
  }

  std::map<unsigned, Term> used;
  for (const Transition &t : program.transitions) {
    collectNodes(t.guard, used);
    for (const auto &[id, term] : t.assignments) collectNodes(term, used);
    for (const auto &[id, term] : t.shadowAssignments) collectNodes(term, used);
    for (const auto &[id, term] : t.memoryUpdates) collectNodes(term, used);
    for (const SideCondition &side : t.sideConditions)
      collectNodes(side.violated, used);
    for (Term term : t.returns) collectNodes(term, used);
    for (Term term : t.returnPoison) collectNodes(term, used);
  }
  out += defineNodes(used);

  std::vector<std::string> stuckUB, stuckOther, edges;
  std::map<unsigned, std::string> notStuck;
  std::set<unsigned> described;
  for (const Transition &t : program.transitions) {
    if (!described.insert(t.from).second)
      continue;
    std::vector<std::string> ub, other, all;
    for (const SideCondition &side : t.sideConditions) {
      (side.kind == SideKind::UndefinedBehavior ? ub : other)
          .push_back(isTrue(side.violated));
      all.push_back(isTrue(side.violated));
    }
    std::string at = "(= loc " + location(t.from) + ")";
    stuckUB.push_back("(and " + at + " " + disjunction(ub) + ")");
    stuckOther.push_back("(and " + at + " " + disjunction(other) + ")");
    notStuck[t.from] = "(not " + disjunction(all) + ")";
  }
  out += "(define-fun Stuck_UB () Bool " + disjunction(stuckUB) + ")\n";
  out += "(define-fun Stuck_Other () Bool " + disjunction(stuckOther) + ")\n";

  for (const Transition &t : program.transitions) {
    std::vector<std::string> parts{"(= loc " + location(t.from) + ")",
                                   isTrue(t.guard), notStuck[t.from],
                                   "(= loc_n " + location(t.to) + ")"};
    std::map<unsigned, Term> assigned, shadowed, written;
    for (const auto &[id, term] : t.assignments) assigned[id] = term;
    for (const auto &[id, term] : t.shadowAssignments) shadowed[id] = term;
    for (const auto &[id, term] : t.memoryUpdates) written[id] = term;
    for (const StateVar &s : program.state) {
      std::string id = std::to_string(s.id);
      auto v = assigned.find(s.id);
      parts.push_back("(= s" + id + "_n " +
                      (v == assigned.end() ? "s" + id : name(v->second)) + ")");
      auto p = shadowed.find(s.id);
      parts.push_back("(= sp" + id + "_n " +
                      (p == shadowed.end() ? "sp" + id : name(p->second)) + ")");
    }
    for (const MemoryRoot &r : program.roots) {
      std::string id = std::to_string(r.id);
      auto m = written.find(r.id);
      parts.push_back("(= m" + id + "_n " +
                      (m == written.end() ? "m" + id : name(m->second)) + ")");
    }
    for (unsigned i = 0; i < t.returns.size(); ++i) {
      parts.push_back("(= r" + std::to_string(i) + " " + name(t.returns[i]) + ")");
      parts.push_back("(= rp" + std::to_string(i) + " " +
                      name(t.returnPoison[i]) + ")");
    }
    edges.push_back(conjunction(parts));
  }
  out += "(define-fun T () Bool " + disjunction(edges) + ")\n";
  return out;
}

static std::vector<std::string> pins(const SemanticProgram &program,
                                     const Configuration &config,
                                     const std::string &suffix) {
  std::vector<std::string> out;
  out.push_back("(= loc" + suffix + " " + location(config.location) + ")");
  for (const StateVar &s : program.state) {
    std::string id = std::to_string(s.id);
    if (config.state[s.id])
      out.push_back("(= s" + id + suffix + " " +
                    literal(s.sort, *config.state[s.id]) + ")");
    out.push_back("(= sp" + id + suffix + " " +
                  (config.shadow[s.id] ? "#b1" : "#b0") + ")");
  }
  for (const MemoryRoot &r : program.roots) {
    const RootData &data = config.memory[r.id];
    std::string array = "m" + std::to_string(r.id) + suffix;
    for (uint64_t offset = 0; offset < data.cells.size(); ++offset) {
      if (!data.cells[offset])
        continue;
      llvm::SmallVector<std::string> index(data.shape.size());
      uint64_t rest = offset;
      for (int d = (int)data.shape.size() - 1; d >= 0; --d) {
        index[d] = bvLiteral(APInt(r.sort.indexWidth, rest % data.shape[d]));
        rest /= data.shape[d];
      }
      out.push_back("(= (select " + array + " " + key(index) + ") " +
                    literal(r.sort.element(), *data.cells[offset]) + ")");
    }
  }
  return out;
}

std::string encodeStepQuery(const SemanticProgram &program,
                            const EntryState &entry, const Configuration &pre,
                            const Configuration *post, StepQuery query,
                            const std::vector<APInt> *returns,
                            const std::vector<bool> *returnPoison) {
  std::string out = encodeTransitionRelation(program);
  auto postPins = [&]() {
    std::vector<std::string> list = pins(program, *post, "_n");
    if (returns)
      for (unsigned i = 0; i < returns->size(); ++i) {
        bool poison = returnPoison && (*returnPoison)[i];
        list.push_back("(= rp" + std::to_string(i) + " " +
                       (poison ? "#b1" : "#b0") + ")");
        if (!poison)
          list.push_back("(= r" + std::to_string(i) + " " +
                         literal(program.returnSorts[i], (*returns)[i]) + ")");
      }
    return list;
  };
  for (const Parameter &p : program.parameters)
    out += "(assert (= p" + std::to_string(p.id) + " " +
           literal(p.sort, entry.parameters[p.id]) + "))\n";
  for (const std::string &pin : pins(program, pre, ""))
    out += "(assert " + pin + ")\n";
  switch (query) {
  case StepQuery::AdmitsStep:
    out += "(assert T)\n";
    out += "(assert " + conjunction(postPins()) + ")\n";
    break;
  case StepQuery::AdmitsOtherStep:
    out += "(assert T)\n";
    out += "(assert (not " + conjunction(postPins()) + "))\n";
    break;
  case StepQuery::AdmitsAnyStep:
    out += "(assert T)\n";
    break;
  case StepQuery::StuckUB:
    out += "(assert Stuck_UB)\n";
    break;
  case StepQuery::StuckOther:
    out += "(assert Stuck_Other)\n";
    break;
  }
  return out;
}

} // namespace mlir::generalized_gemm
