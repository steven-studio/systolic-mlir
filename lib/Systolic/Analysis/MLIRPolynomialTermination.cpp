#include "PolynomialTerminationInternal.h"
#include "mlir/Analysis/CFGLoopInfo.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include "mlir/IR/Dominance.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Interfaces/ControlFlowInterfaces.h"
#include "mlir/Interfaces/SideEffectInterfaces.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/SHA256.h"
#include <functional>
#include <set>

namespace systolic::polynomial::termination {
namespace detail {
using namespace mlir;
using Pred = arith::CmpIPredicate;
std::string snapshot(Operation *op) {
  std::string s;
  llvm::raw_string_ostream stream(s);
  op->print(stream, OpPrintingFlags().printGenericOpForm().enableDebugInfo());
  return s;
}
void validateLimits(const Limits &l) {
  Limits cap;
#define CHECK_LIMIT(field) if (!l.field || l.field > cap.field) throw Rejection("invalid or exhausted resource limit: " #field)
  CHECK_LIMIT(maxBlocks); CHECK_LIMIT(maxOperations); CHECK_LIMIT(maxStates);
  CHECK_LIMIT(maxParameters); CHECK_LIMIT(maxNodes); CHECK_LIMIT(maxDepth);
  CHECK_LIMIT(maxCandidates); CHECK_LIMIT(maxQueries); CHECK_LIMIT(timeoutMilliseconds);
  CHECK_LIMIT(solverResourceLimit); CHECK_LIMIT(maxSMTBytes);
#undef CHECK_LIMIT
}
unsigned width(Type t) {
  auto i = dyn_cast<IntegerType>(t);
  if (!i || !i.isSignless() || i.getWidth() < 1 || i.getWidth() > 64)
    throw Rejection("P2b requires scalar signless integers of width 1..64");
  return i.getWidth();
}
void attributes(Operation *op, std::initializer_list<llvm::StringRef> allowed) {
  for (auto attr : op->getAttrs())
    if (std::find(allowed.begin(), allowed.end(), attr.getName().strref()) == allowed.end())
      throw Rejection("unsupported semantic/progress attribute: " + attr.getName().strref().str());
}
const char *predicateName(Predicate p) {
  switch (p) {
  case Predicate::SLT: return "slt"; case Predicate::SLE: return "sle";
  case Predicate::SGT: return "sgt"; case Predicate::SGE: return "sge";
  case Predicate::ULT: return "ult"; case Predicate::ULE: return "ule";
  case Predicate::UGT: return "ugt"; case Predicate::UGE: return "uge";
  }
  throw Rejection("invalid guard predicate");
}
bool signedPredicate(Predicate p) { return p <= Predicate::SGE; }
bool ascendingPredicate(Predicate p) {
  return p == Predicate::SLT || p == Predicate::SLE || p == Predicate::ULT || p == Predicate::ULE;
}
bool inclusivePredicate(Predicate p) {
  return p == Predicate::SLE || p == Predicate::SGE || p == Predicate::ULE || p == Predicate::UGE;
}
Predicate convert(Pred p) {
  switch (p) {
  case Pred::slt: return Predicate::SLT; case Pred::sle: return Predicate::SLE;
  case Pred::sgt: return Predicate::SGT; case Pred::sge: return Predicate::SGE;
  case Pred::ult: return Predicate::ULT; case Pred::ule: return Predicate::ULE;
  case Pred::ugt: return Predicate::UGT; case Pred::uge: return Predicate::UGE;
  default: throw Rejection("unsupported comparison predicate");
  }
}
Pred swapped(Pred p) {
  switch (p) {
  case Pred::slt: return Pred::sgt; case Pred::sle: return Pred::sge;
  case Pred::sgt: return Pred::slt; case Pred::sge: return Pred::sle;
  case Pred::ult: return Pred::ugt; case Pred::ule: return Pred::uge;
  case Pred::ugt: return Pred::ult; case Pred::uge: return Pred::ule;
  default: throw Rejection("unsupported comparison predicate");
  }
}
struct Edge {
  Block *source, *destination;
  unsigned index;
  Value argument(unsigned n) const {
    auto b = dyn_cast<BranchOpInterface>(source->getTerminator());
    if (!b) throw Rejection("missing branch interface");
    auto args = b.getSuccessorOperands(index);
    if (args.getProducedOperandCount() || args.size() != destination->getNumArguments() || n >= args.size())
      throw Rejection("invalid branch argument transfer");
    return args[n];
  }
};
bool containsState(const Expression &e) {
  std::set<const Expression::Node *> seen;
  std::function<bool(const Expression::Node *)> visit = [&](const auto *n) {
    if (!seen.insert(n).second) return false;
    if (n->kind == Expression::Kind::Variable) return n->name.front() == 's';
    return n->left && (visit(n->left.get()) || visit(n->right.get()));
  };
  return visit(e.root().get());
}

Model extract(func::FuncOp function, const Options &options) {
  validateLimits(options.limits);
  if (!function || failed(verify(function.getOperation())) || function.isExternal())
    throw Rejection("invalid or external MLIR function");
  attributes(function, {"sym_name", "sym_visibility", "function_type"});
  for (Operation *p = function->getParentOp(); p; p = p->getParentOp())
    attributes(p, {"sym_name"});
  auto &region = function.getBody();
  DominanceInfo dom(function);
  unsigned blocks = 0, operations = 0;
  llvm::DenseSet<Block *> reachable;
  llvm::DenseMap<Block *, std::vector<Edge>> incoming;
  for (Block &b : region) {
    if (++blocks > options.limits.maxBlocks) throw Rejection("block resource limit");
    if (!dom.isReachableFromEntry(&b)) continue;
    reachable.insert(&b);
    for (Operation &op : b) {
      if (++operations > options.limits.maxOperations) throw Rejection("operation resource limit");
      if (op.getNumRegions() || !isMemoryEffectFree(&op)) throw Rejection("nested region or memory/call effects");
      if (isa<arith::ConstantOp>(op)) {
        attributes(&op, {"value"});
        (void)width(op.getResult(0).getType());
      } else if (isa<arith::AddIOp, arith::SubIOp, arith::MulIOp>(op)) {
        attributes(&op, {"overflowFlags"});
        arith::IntegerOverflowFlags flags;
        if (auto a = dyn_cast<arith::AddIOp>(op)) flags = a.getOverflowFlags();
        else if (auto a = dyn_cast<arith::SubIOp>(op)) flags = a.getOverflowFlags();
        else flags = cast<arith::MulIOp>(op).getOverflowFlags();
        if (flags != arith::IntegerOverflowFlags::none) throw Rejection("poison-producing overflow flags unsupported");
        (void)width(op.getResult(0).getType());
      } else if (isa<arith::CmpIOp>(op)) attributes(&op, {"predicate"});
      else if (isa<cf::CondBranchOp>(op)) attributes(&op, {"operandSegmentSizes"});
      else if (isa<cf::BranchOp, func::ReturnOp>(op)) attributes(&op, {});
      else throw Rejection("unsupported operation: " + op.getName().getStringRef().str());
    }
    for (unsigned i = 0; i < b.getNumSuccessors(); ++i)
      incoming[b.getSuccessor(i)].push_back({&b, b.getSuccessor(i), i});
  }
  // P2a's accepted topology and polynomial recurrence class are prerequisites.
  // Always use its unconditional modular interpretation, never a horizon or
  // caller no-overflow assumption. The returned P2a object is not modified.
  auto recurrence = cfg::analyzeSymbolic(function);
  if (recurrence.status != polynomial::Status::Proven)
    throw Rejection("P2a prerequisite: " + recurrence.reason);
  Model model;
  model.width = recurrence.bitWidth;
  if (model.width > 64 || recurrence.states.size() > options.limits.maxStates)
    throw Rejection("state width/count resource limit");
  if (function.getNumArguments() > options.limits.maxParameters) throw Rejection("parameter resource limit");
  Integer half = Integer(1) << (model.width-1);
  for (auto &[arg, range] : options.parameterRanges)
    if (arg >= function.getNumArguments() || range.first > range.second || range.first < -half || range.second >= half)
      throw Rejection("invalid/empty source-width parameter domain");
  for (unsigned i = 0; i < function.getNumArguments(); ++i) {
    if (width(function.getArgument(i).getType()) != model.width) throw Rejection("mixed-width parameter");
    auto range = std::make_pair(Integer(-half), Integer(half-1));
    if (auto it = options.parameterRanges.find(i); it != options.parameterRanges.end()) range = it->second;
    model.domain.push_back({i, model.width, range.first, range.second});
  }
  Block *header = recurrence.header;
  CFGLoopInfo loops(dom.getDomTree(&region));
  auto *loop = loops.getLoopFor(header);
  auto branch = dyn_cast<cf::CondBranchOp>(header->getTerminator());
  if (!loop || loop->getHeader() != header || !branch) throw Rejection("requires a header-tested natural loop");
  bool trueInside = loop->contains(branch.getTrueDest()), falseInside = loop->contains(branch.getFalseDest());
  if (trueInside == falseInside) throw Rejection("guard does not select unique continuation versus exit");
  std::vector<Edge> entry, back;
  for (const Edge &e : incoming[header]) (loop->contains(e.source) ? back : entry).push_back(e);
  if (entry.size() != 1 || back.size() != 1) throw Rejection("incomplete entry/backedge model");

  llvm::DenseMap<Value, Expression> memo;
  unsigned nodes = 0;
  std::function<Expression(Value, unsigned)> expr = [&](Value value, unsigned depth) -> Expression {
    if (depth > options.limits.maxDepth) throw Rejection("SSA depth resource limit");
    if (auto it = memo.find(value); it != memo.end()) return it->second;
    if (++nodes > options.limits.maxNodes) throw Rejection("SSA node resource limit");
    if (width(value.getType()) != model.width) throw Rejection("mixed-width SSA expression");
    Expression out;
    if (auto arg = dyn_cast<BlockArgument>(value)) {
      if (arg.getOwner() == header) out = Expression::variable("s"+std::to_string(arg.getArgNumber()));
      else if (arg.getOwner() == &region.front()) out = Expression::variable("p"+std::to_string(arg.getArgNumber()));
      else {
        auto &es = incoming[arg.getOwner()];
        if (es.size() != 1) throw Rejection("ambiguous argument forwarding");
        out = expr(es.front().argument(arg.getArgNumber()), depth+1);
      }
    } else if (auto c = value.getDefiningOp<arith::ConstantOp>()) {
      auto attr = dyn_cast<IntegerAttr>(c.getValue());
      if (!attr) throw Rejection("noninteger constant");
      llvm::SmallString<32> text; attr.getValue().toString(text, 10, true);
      out = Expression(Rational(Integer(text.str().str())));
    } else {
      Operation *op = value.getDefiningOp();
      if (!op || !isa<arith::AddIOp, arith::SubIOp, arith::MulIOp>(op)) throw Rejection("unsupported state/guard expression");
      auto a = expr(op->getOperand(0), depth+1), b = expr(op->getOperand(1), depth+1);
      if (isa<arith::AddIOp>(op)) out = a+b;
      else if (isa<arith::SubIOp>(op)) out = a-b;
      else out = a*b;
    }
    (void)cfg::formatExpression(out); // retain P2a's bounded expansion contract
    memo.try_emplace(value, out);
    return out;
  };
  for (unsigned i = 0; i < header->getNumArguments(); ++i) {
    model.initial.push_back(expr(entry.front().argument(i), 0));
    if (containsState(model.initial.back())) throw Rejection("loop-dependent entry value");
    model.next.push_back(expr(back.front().argument(i), 0));
    model.recurrenceInitial.push_back(recurrence.states[i].initial);
    model.recurrenceNext.push_back(recurrence.states[i].next);
  }
  Value condition = branch.getCondition();
  if (auto c = condition.getDefiningOp<arith::ConstantOp>()) {
    auto attr = dyn_cast<IntegerAttr>(c.getValue());
    if (!attr || attr.getValue().getBitWidth() != 1) throw Rejection("invalid Boolean guard");
    model.constantGuard = (!attr.getValue().isZero()) == trueInside;
    model.guardText = *model.constantGuard ? "true" : "false";
  } else {
    auto comparison = condition.getDefiningOp<arith::CmpIOp>();
    if (!comparison) throw Rejection("unsupported guard condition");
    Pred p = comparison.getPredicate();
    if (!trueInside) p = arith::invertPredicate(p);
    model.lhs = expr(comparison.getLhs(), 0); model.rhs = expr(comparison.getRhs(), 0);
    if (!containsState(model.lhs) && containsState(model.rhs)) {
      std::swap(model.lhs, model.rhs); p = swapped(p);
    }
    model.predicate = convert(p);
    if (containsState(model.rhs)) throw Rejection("loop-variant comparison bound");
    if (containsState(model.lhs)) {
      const auto &node = *model.lhs.root();
      if (node.kind != Expression::Kind::Variable || node.name.front() != 's') throw Rejection("nonlinear/noncanonical guard operand");
      unsigned index = std::stoul(node.name.substr(1));
      model.counter = index;
      try {
        auto increment = substituteSymbolic(recurrence.states[index].increment, {});
        if (increment.iterationDegree() > 0) throw Rejection("nonconstant controlling step");
        Rational value = increment.evaluate(0, {});
        if (value.denominator() != 1) throw Rejection("noninteger controlling step");
        model.step = value.numerator();
      } catch (const std::out_of_range &) { throw Rejection("symbolic or state-dependent controlling step"); }
    }
    model.guardText = "("+std::string(predicateName(model.predicate))+" "+cfg::formatExpression(model.lhs)+" "+cfg::formatExpression(model.rhs)+")";
  }
  llvm::SHA256 hash; hash.update(snapshot(function));
  model.fingerprint = llvm::toHex(hash.final(), true);
  return model;
}
std::vector<Certificate> candidates(const Model &model, const Options &options) {
  Certificate c; c.sourceFingerprint = model.fingerprint; c.guard = model.guardText; c.domain = model.domain;
  c.kind = CertificateKind::ZeroIterations;
  std::vector<Certificate> out{c};
  if (model.counter && model.step && *model.step != 0) {
    c.kind = CertificateKind::LinearRanking; c.counter = *model.counter;
    c.ascending = ascendingPredicate(model.predicate); c.signedValues = signedPredicate(model.predicate);
    c.offset = inclusivePredicate(model.predicate) ? 1 : 0;
    out.push_back(c);
  }
  c.kind = CertificateKind::RecurrentGuard; out.push_back(c);
  if (out.size() > options.limits.maxCandidates) throw Rejection("candidate resource limit");
  return out;
}
} // namespace detail

std::vector<Certificate> generateCandidates(mlir::func::FuncOp f, const Options &options) {
  try { return detail::candidates(detail::extract(f, options), options); }
  catch (const std::exception &) { return {}; }
}
Result analyze(mlir::func::FuncOp f, const Options &options) {
  Result result; result.options = options;
  try {
    auto proposals = detail::candidates(detail::extract(f, options), options);
    unsigned queries = 0;
    std::vector<Obligation> attempts;
    for (const auto &c : proposals) {
      result = detail::verify(f, c, options, queries);
      attempts.insert(attempts.end(), result.obligations.begin(), result.obligations.end());
      if (result.status != Status::Unknown) break;
      // Solver failure/resource exhaustion is not a reason to try another
      // proof with an accidentally reset solver budget.
      if (!result.obligations.empty() && (result.obligations.back().outcome == "unknown" ||
                                         result.obligations.back().outcome == "error")) break;
    }
    result.obligations = std::move(attempts);
  } catch (const std::exception &e) { result.status = Status::Unknown; result.certificate.reset(); result.reason = e.what(); }
  return result;
}
} // namespace systolic::polynomial::termination
