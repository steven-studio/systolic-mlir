#include "Systolic/MLIRPolynomialRecurrence.h"

#include "mlir/Analysis/CFGLoopInfo.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include "mlir/IR/Dominance.h"
#include "mlir/IR/RegionKindInterface.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Interfaces/ControlFlowInterfaces.h"
#include "mlir/Interfaces/SideEffectInterfaces.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/SCCIterator.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/JSON.h"

#include <functional>
#include <set>
#include <sstream>
#include <stdexcept>

namespace systolic::polynomial::cfg {
namespace {
using namespace mlir;
using Node = Expression::Node;
using Kind = Expression::Kind;
using Flags = arith::IntegerOverflowFlags;

struct Unsupported : std::runtime_error {
  using std::runtime_error::runtime_error;
};

unsigned width(Type type) {
  auto integer = dyn_cast<IntegerType>(type);
  if (!integer || !integer.isSignless() || integer.getWidth() == 0 ||
      integer.getWidth() > 4096)
    throw Unsupported("expected a scalar signless integer of width 1..4096");
  return integer.getWidth();
}

Integer signedInteger(const llvm::APInt &value) {
  llvm::SmallString<64> text;
  value.toString(text, 10, true);
  return Integer(text.str().str());
}

struct Edge {
  Block *from;
  Block *to;
  unsigned index;
  Value argument(unsigned i) const {
    auto branch = dyn_cast<BranchOpInterface>(from->getTerminator());
    if (!branch)
      throw Unsupported("terminator has no BranchOpInterface");
    auto operands = branch.getSuccessorOperands(index);
    if (operands.getProducedOperandCount() != 0 ||
        operands.size() != to->getNumArguments() || i >= operands.size())
      throw Unsupported("unsupported produced or mismatched successor operands");
    return operands[i];
  }
};

// Structural removal of exactly one additive occurrence of the old state.
// Source IR intermediates are checked separately, BEFORE relying on this
// reassociation. No polynomial algebra is implemented here.
Expression incrementOf(const Expression &next, const std::string &self) {
  std::map<const Node *, bool> mentionsMemo;
  std::function<bool(const Node *)> mentions = [&](const Node *n) {
    if (auto it = mentionsMemo.find(n); it != mentionsMemo.end())
      return it->second;
    bool result = n->kind == Kind::Variable && n->name == self;
    if (n->left)
      result |= mentions(n->left.get()) || mentions(n->right.get());
    mentionsMemo.emplace(n, result);
    return result;
  };
  std::map<const Node *, Expression> copied;
  std::function<Expression(const Node *)> copy = [&](const Node *n) -> Expression {
    if (auto it = copied.find(n); it != copied.end())
      return it->second;
    Expression e;
    switch (n->kind) {
    case Kind::Constant: e = Expression(n->constant); break;
    case Kind::Variable: e = Expression::variable(n->name); break;
    case Kind::Add: e = copy(n->left.get()) + copy(n->right.get()); break;
    case Kind::Subtract: e = copy(n->left.get()) - copy(n->right.get()); break;
    case Kind::Multiply: e = copy(n->left.get()) * copy(n->right.get()); break;
    case Kind::Unsupported: throw Unsupported("unsupported transition expression");
    }
    copied.emplace(n, e);
    return e;
  };
  auto isSelf = [&](const Node *n) {
    return n->kind == Kind::Variable && n->name == self;
  };
  std::function<Expression(const Node *)> remove = [&](const Node *n) -> Expression {
    if (isSelf(n))
      return Expression(Rational(0));
    if (n->kind == Kind::Add) {
      bool left = mentions(n->left.get()), right = mentions(n->right.get());
      if (left == right)
        throw Unsupported("backedge must contain exactly one additive old-state occurrence");
      if (left)
        return isSelf(n->left.get()) ? copy(n->right.get())
                                    : remove(n->left.get()) + copy(n->right.get());
      return isSelf(n->right.get()) ? copy(n->left.get())
                                   : copy(n->left.get()) + remove(n->right.get());
    }
    if (n->kind == Kind::Subtract && mentions(n->left.get()) &&
        !mentions(n->right.get()))
      return remove(n->left.get()) - copy(n->right.get());
    throw Unsupported("non-additive or nonlinear self recurrence");
  };
  return remove(next.root().get());
}

Polynomial substitute(const Expression &expression,
                      const std::map<std::string, Polynomial> &states) {
  std::map<const Node *, Polynomial> memo;
  std::function<Polynomial(const Node *)> visit = [&](const Node *n) -> Polynomial {
    if (auto found = memo.find(n); found != memo.end())
      return found->second;
    Polynomial p;
    switch (n->kind) {
    case Kind::Constant: p = Polynomial::constant(n->constant); break;
    case Kind::Variable: p = states.at(n->name); break;
    case Kind::Add: p = visit(n->left.get()) + visit(n->right.get()); break;
    case Kind::Subtract: p = visit(n->left.get()) - visit(n->right.get()); break;
    case Kind::Multiply: p = visit(n->left.get()) * visit(n->right.get()); break;
    case Kind::Unsupported: throw Unsupported("unsupported polynomial substitution");
    }
    memo.emplace(n, p);
    return p;
  };
  return visit(expression.root().get());
}

struct Extractor {
  Options options;
  Block *header = nullptr;
  unsigned bits = 0;
  llvm::DenseSet<Block *> reachable, loopBlocks, prelude;
  llvm::DenseMap<Block *, std::vector<Edge>> incoming;
  llvm::DenseMap<Value, Expression> expressions;
  llvm::DenseMap<Value, llvm::APInt> constants, castConstants;

  Value forwarded(BlockArgument argument) {
    auto &edges = incoming[argument.getOwner()];
    if (edges.size() != 1)
      throw Unsupported("block argument has no unique reachable incoming edge");
    return edges.front().argument(argument.getArgNumber());
  }

  // Exact APInt evaluation of entry constants, or constant-only cast chains.
  // This does not fold or mutate IR. Constant-only casts may change the value;
  // their actual bit semantics, rather than an assumed identity, is used.
  llvm::APInt constant(Value value, bool castsOnly = false, unsigned depth = 0) {
    if (depth > 128)
      throw Unsupported("constant/forwarding expression exceeds depth 128");
    auto &memo = castsOnly ? castConstants : constants;
    if (auto found = memo.find(value); found != memo.end())
      return found->second;
    unsigned targetWidth = width(value.getType());
    llvm::APInt result(targetWidth, 0);
    if (auto arg = dyn_cast<BlockArgument>(value)) {
      if (arg.getOwner() == header)
        throw Unsupported("initial value or cast depends on loop state");
      result = constant(forwarded(arg), castsOnly, depth + 1);
    } else if (auto c = value.getDefiningOp<arith::ConstantOp>()) {
      auto attr = dyn_cast<IntegerAttr>(c.getValue());
      if (!attr)
        throw Unsupported("non-integer constant");
      result = attr.getValue();
    } else if (auto c = value.getDefiningOp<arith::ExtSIOp>()) {
      result = constant(c.getIn(), true, depth + 1).sext(targetWidth);
    } else if (auto c = value.getDefiningOp<arith::ExtUIOp>()) {
      llvm::APInt source = constant(c.getIn(), true, depth + 1);
      // nneg constrains the sign bit at the SOURCE width. Zero extension
      // would erase that evidence and must not turn poison into a constant.
      if (c.getNonNeg() && source.isNegative())
        throw Unsupported("arith.extui nneg precondition violated at source width");
      result = source.zext(targetWidth);
    } else if (auto c = value.getDefiningOp<arith::TruncIOp>()) {
      if (c.getOverflowFlags() != Flags::none)
        throw Unsupported("overflow flags on a cast are unsupported");
      result = constant(c.getIn(), true, depth + 1).trunc(targetWidth);
    } else {
      Operation *op = value.getDefiningOp();
      if (castsOnly || !op || !isa<arith::AddIOp, arith::SubIOp, arith::MulIOp>(op))
        throw Unsupported("initial value or cast is not an exact constant");
      llvm::APInt a = constant(op->getOperand(0), false, depth + 1);
      llvm::APInt b = constant(op->getOperand(1), false, depth + 1);
      bool overflow = false;
      if (isa<arith::AddIOp>(op))
        result = a.sadd_ov(b, overflow);
      else if (isa<arith::SubIOp>(op))
        result = a.ssub_ov(b, overflow);
      else
        result = a.smul_ov(b, overflow);
      if (options.mode != IntegerMode::Modular && overflow)
        throw Unsupported("constant arithmetic overflows before loop entry");
    }
    memo.try_emplace(value, result);
    return result;
  }

  Expression expression(Value value, unsigned depth = 0) {
    if (depth > 128)
      throw Unsupported("SSA expression exceeds depth 128");
    if (auto found = expressions.find(value); found != expressions.end())
      return found->second;
    if (width(value.getType()) != bits)
      throw Unsupported("mixed-width arithmetic or unproven cast");
    Expression result;
    if (auto arg = dyn_cast<BlockArgument>(value)) {
      if (arg.getOwner() == header)
        result = Expression::variable("s" + std::to_string(arg.getArgNumber()));
      else
        result = expression(forwarded(arg), depth + 1);
    } else {
      Operation *op = value.getDefiningOp();
      if (prelude.count(op->getBlock()) ||
          isa<arith::ConstantOp, arith::ExtSIOp, arith::ExtUIOp, arith::TruncIOp>(op)) {
        bool cast = isa<arith::ExtSIOp, arith::ExtUIOp, arith::TruncIOp>(op);
        result = Expression(Rational(signedInteger(constant(value, cast))));
      } else if (isa<arith::AddIOp>(op)) {
        result = expression(op->getOperand(0), depth + 1) +
                 expression(op->getOperand(1), depth + 1);
      } else if (isa<arith::SubIOp>(op)) {
        result = expression(op->getOperand(0), depth + 1) -
                 expression(op->getOperand(1), depth + 1);
      } else if (isa<arith::MulIOp>(op)) {
        result = expression(op->getOperand(0), depth + 1) *
                 expression(op->getOperand(1), depth + 1);
      } else {
        throw Unsupported("unsupported arithmetic in state transition");
      }
    }
    expressions.try_emplace(value, result);
    return result;
  }
};

const char *statusName(Status status) {
  switch (status) {
  case Status::Proven: return "PROVEN";
  case Status::Conditional: return "CONDITIONAL";
  case Status::Unknown: return "UNKNOWN";
  }
  return "UNKNOWN";
}
const char *modeName(IntegerMode mode) {
  switch (mode) {
  case IntegerMode::Mathematical: return "mathematical";
  case IntegerMode::Modular: return "modular";
  case IntegerMode::SignedNoOverflow: return "signed-no-overflow";
  }
  return "invalid";
}
std::string rationalString(const Rational &q) {
  std::ostringstream stream;
  stream << q;
  return stream.str();
}
} // namespace

Result analyze(mlir::func::FuncOp function, const Options &options) {
  Result result;
  result.options = options;
  auto unknown = [&](const std::string &reason) {
    result.status = Status::Unknown;
    result.reason = reason;
    result.header = nullptr;
    result.states.clear();
    result.solution = {};
    return result;
  };
  if (!function || failed(mlir::verify(function.getOperation())))
    return unknown("MLIR verification failed");
  if (function.isExternal())
    return unknown("external function has no CFG");
  if (options.mode != IntegerMode::Modular && options.mode != IntegerMode::Mathematical &&
      options.mode != IntegerMode::SignedNoOverflow)
    return unknown("invalid integer semantic mode");
  if (options.maxIterations && *options.maxIterations < 0)
    return unknown("negative iteration horizon");
  bool checkSigned = options.mode != IntegerMode::Modular;
  if (checkSigned && !options.maxIterations)
    return unknown("integer interpretation of finite-width MLIR requires an explicit horizon");
  if (!checkSigned && !options.noOverflowAssumption.empty())
    return unknown("no-overflow assumption is not a modular contract");

  try {
    auto &region = function.getBody();
    DominanceInfo dominance(function);
    if (mayBeGraphRegion(region) || !dominance.hasSSADominance(&region))
      throw Unsupported("region is not an SSACFG");
    if (region.hasOneBlock())
      throw Unsupported("no supported CFG loop");
    Extractor extractor;
    extractor.options = options;
    unsigned operationCount = 0, blockCount = 0;
    std::vector<Edge> edges;
    for (Block &block : region) {
      if (++blockCount > 512)
        throw Unsupported("CFG exceeds 512 blocks");
      if (!dominance.isReachableFromEntry(&block))
        continue;
      extractor.reachable.insert(&block);
      for (Operation &op : block) {
        if (++operationCount > 4096)
          throw Unsupported("reachable CFG exceeds 4096 operations");
        if (op.getNumRegions())
          throw Unsupported("structured/nested regions require a separate proven extraction");
        if (!isMemoryEffectFree(&op))
          throw Unsupported("unknown or nonempty memory effects: " + op.getName().getStringRef().str());
        if (!isa<arith::ConstantOp, arith::AddIOp, arith::SubIOp, arith::MulIOp,
                 arith::CmpIOp, arith::ExtSIOp, arith::ExtUIOp, arith::TruncIOp,
                 cf::BranchOp, cf::CondBranchOp, func::ReturnOp>(&op))
          throw Unsupported("unsupported operation: " + op.getName().getStringRef().str());
        Flags flags = Flags::none;
        if (auto arithmetic = dyn_cast<arith::AddIOp>(op)) flags = arithmetic.getOverflowFlags();
        if (auto arithmetic = dyn_cast<arith::SubIOp>(op)) flags = arithmetic.getOverflowFlags();
        if (auto arithmetic = dyn_cast<arith::MulIOp>(op)) flags = arithmetic.getOverflowFlags();
        if (flags != Flags::none && (!checkSigned || flags != Flags::nsw))
          throw Unsupported("unproven arithmetic overflow flags");
      }
    }
    for (Block &block : region) {
      if (!extractor.reachable.count(&block))
        continue;
      Operation *term = block.getTerminator();
      for (unsigned i = 0; i < term->getNumSuccessors(); ++i) {
        Edge edge{&block, term->getSuccessor(i), i};
        auto branch = dyn_cast<BranchOpInterface>(term);
        if (!branch || branch.getSuccessorOperands(i).getProducedOperandCount() != 0)
          throw Unsupported("unsupported branch-produced values");
        edges.push_back(edge);
        extractor.incoming[edge.to].push_back(edge);
      }
    }
    CFGLoopInfo loopInfo(dominance.getDomTree(&region));
    auto loops = loopInfo.getLoopsInPreorder();
    unsigned cycles = 0;
    std::set<Block *> cyclicBlocks;
    for (auto it = llvm::scc_begin(&region.front()); !it.isAtEnd(); ++it) {
      if (!it.hasCycle())
        continue;
      ++cycles;
      cyclicBlocks.insert(it->begin(), it->end());
    }
    if (cycles != 1 || loops.size() != 1 || !loops.front()->getSubLoops().empty())
      throw Unsupported("expected one reducible natural loop; irreducible/multiple/nested cycles unsupported");
    CFGLoop *loop = loops.front();
    Block *header = loop->getHeader();
    extractor.header = header;
    for (Block *block : loop->getBlocks()) {
      if (!dominance.dominates(header, block))
        throw Unsupported("header does not dominate loop");
      extractor.loopBlocks.insert(block);
    }
    if (cyclicBlocks.size() != extractor.loopBlocks.size() ||
        !std::all_of(cyclicBlocks.begin(), cyclicBlocks.end(), [&](Block *b) {
          return extractor.loopBlocks.count(b);
        }))
      throw Unsupported("irreducible or multi-entry CFG cycle");
    std::vector<Edge> entries, backedges, exits;
    for (const Edge &edge : edges) {
      bool sourceInside = loop->contains(edge.from), targetInside = loop->contains(edge.to);
      if (!sourceInside && targetInside) entries.push_back(edge);
      if (sourceInside && edge.to == header) backedges.push_back(edge);
      if (sourceInside && !targetInside) exits.push_back(edge);
    }
    if (entries.size() != 1 || entries.front().to != header)
      throw Unsupported("loop requires one entry edge to its header");
    if (backedges.size() != 1)
      throw Unsupported("multiple backedges are unsupported");
    if (exits.size() != 1 || !isa<cf::CondBranchOp>(exits.front().from->getTerminator()))
      throw Unsupported("loop requires one conditional exit edge");
    // Prove a single unconditional update path around the loop. The only
    // condition chooses continuation versus exit, never different updates.
    llvm::DenseSet<Block *> visited;
    Block *cursor = header;
    do {
      if (!visited.insert(cursor).second)
        throw Unsupported("ambiguous update path");
      Operation *term = cursor->getTerminator();
      if (cursor == exits.front().from) {
        if (!isa<cf::CondBranchOp>(term))
          throw Unsupported("unsupported exit terminator");
      } else if (!isa<cf::BranchOp>(term)) {
        throw Unsupported("loop-carried conditional updates are unsupported");
      }
      Block *next = nullptr;
      for (Block *successor : cursor->getSuccessors()) {
        if (loop->contains(successor)) {
          if (next)
            throw Unsupported("ambiguous internal control dependency");
          next = successor;
        }
      }
      if (!next)
        throw Unsupported("missing cycle continuation");
      cursor = next;
    } while (cursor != header);
    if (visited.size() != extractor.loopBlocks.size())
      throw Unsupported("loop is not a single update path");

    // Constant initialization must lie on a unique, unconditional entry path.
    cursor = &region.front();
    while (cursor != header) {
      if (loop->contains(cursor) || !extractor.prelude.insert(cursor).second)
        throw Unsupported("unsupported entry control flow");
      auto branch = dyn_cast<cf::BranchOp>(cursor->getTerminator());
      if (!branch)
        throw Unsupported("conditional/multiple-entry initialization is unsupported");
      cursor = branch.getDest();
    }
    llvm::DenseSet<Block *> tail;
    cursor = exits.front().to;
    while (true) {
      if (loop->contains(cursor) || extractor.prelude.count(cursor) || !tail.insert(cursor).second)
        throw Unsupported("unsupported exit control flow");
      if (isa<func::ReturnOp>(cursor->getTerminator())) break;
      auto branch = dyn_cast<cf::BranchOp>(cursor->getTerminator());
      if (!branch) throw Unsupported("conditional exit tail is unsupported");
      cursor = branch.getDest();
    }
    if (extractor.prelude.size() + extractor.loopBlocks.size() + tail.size() != extractor.reachable.size())
      throw Unsupported("reachable blocks outside supported loop paths");
    if (header->getNumArguments() == 0 || header->getNumArguments() > 128)
      throw Unsupported("expected 1..128 loop-carried header arguments");
    extractor.bits = width(header->getArgument(0).getType());
    for (BlockArgument arg : header->getArguments())
      if (width(arg.getType()) != extractor.bits)
        throw Unsupported("mixed-width loop-carried state is unsupported");
    std::vector<Recurrence> system;
    for (unsigned i = 0; i < header->getNumArguments(); ++i) {
      State state;
      state.argument = i;
      state.name = "s" + std::to_string(i);
      state.initial = Rational(signedInteger(extractor.constant(entries.front().argument(i))));
      state.next = extractor.expression(backedges.front().argument(i));
      // Bound expansion of shared SSA DAGs before solving or reporting them.
      (void)formatExpression(state.next);
      state.increment = incrementOf(state.next, state.name);
      (void)formatExpression(state.increment);
      std::set<unsigned> deps;
      std::set<const Node *> seen;
      std::function<void(const Node *)> collect = [&](const Node *n) {
        if (!seen.insert(n).second) return;
        if (n->kind == Kind::Variable) deps.insert(std::stoul(n->name.substr(1)));
        if (n->left) { collect(n->left.get()); collect(n->right.get()); }
      };
      collect(state.increment.root().get());
      state.dependencies.assign(deps.begin(), deps.end());
      system.push_back({state.name, state.initial, state.increment});
      result.states.push_back(std::move(state));
    }
    AnalysisOptions solveOptions;
    solveOptions.mode = checkSigned ? IntegerMode::Mathematical : IntegerMode::Modular;
    solveOptions.bitWidth = checkSigned ? 0 : extractor.bits;
    solveOptions.maxIterations = options.maxIterations;
    result.solution = polynomial::analyze(system, solveOptions);
    if (result.solution.status == Status::Unknown)
      throw Unsupported("P0: " + result.solution.reason);
    const auto &forms = result.solution.integerLifts;
    for (const State &state : result.states) {
      if (forms.at(state.name).shift(Integer(1)) != substitute(state.next, forms) ||
          forms.at(state.name).difference() != substitute(state.increment, forms) ||
          forms.at(state.name).evaluate(Integer(0)) != state.initial)
        throw Unsupported("extracted transition identity or initial condition failed");
    }
    bool proven = true;
    Integer half = Integer(1) << (extractor.bits - 1);
    auto proveRange = [&](const Polynomial &p) {
      if (!checkSigned) return;
      auto [lower, upper] = p.bounds(*options.maxIterations);
      proven &= lower >= Rational(-half) && upper < Rational(half);
    };
    // Even dead arithmetic executes in the original IR. Inspect every
    // reachable source operation, including the header's final guard visit.
    for (Block &block : region) {
      if (!extractor.reachable.count(&block)) continue;
      for (Operation &op : block) {
        if (isa<arith::ConstantOp>(op)) {
          (void)extractor.constant(op.getResult(0), true);
        } else if (isa<arith::ExtSIOp, arith::ExtUIOp, arith::TruncIOp>(op)) {
          (void)extractor.constant(op.getResult(0), true);
        } else if (isa<arith::AddIOp, arith::SubIOp, arith::MulIOp>(op)) {
          Expression source = extractor.expression(op.getResult(0));
          (void)formatExpression(source);
          proveRange(substitute(source, forms));
        }
      }
    }
    for (const auto &[name, form] : forms) proveRange(form);
    if (checkSigned && !proven && options.noOverflowAssumption.empty())
      throw Unsupported("original MLIR intermediate/state no-overflow proof inconclusive");
    result.status = checkSigned && !proven ? Status::Conditional : Status::Proven;
    result.reason = result.status == Status::Conditional
        ? "conditional on original MLIR no-overflow assumption: " + options.noOverflowAssumption
        : (checkSigned ? "source no-overflow proved; exact header-state identities"
                       : "modular header-state identities proved");
    result.solution.status = result.status;
    result.solution.reason = result.reason;
    result.solution.options.mode = options.mode;
    result.solution.options.bitWidth = options.mode == IntegerMode::Mathematical ? 0 : extractor.bits;
    result.solution.options.noOverflowAssumption = options.noOverflowAssumption;
    result.bitWidth = extractor.bits;
    result.header = header;
    for (Block &block : region) {
      if (&block == header) break;
      ++result.headerBlock;
    }
  } catch (const Unsupported &error) {
    return unknown(error.what());
  } catch (const std::length_error &error) {
    return unknown(std::string("analysis resource limit: ") + error.what());
  }
  return result;
}

std::string formatExpression(const Expression &expression) {
  std::map<const Node *, std::string> memo;
  std::function<std::string(const Node *, unsigned)> print = [&](const Node *n, unsigned depth) -> std::string {
    if (depth > 128) throw std::length_error("reported SSA expression exceeds depth 128");
    if (auto it = memo.find(n); it != memo.end()) return it->second;
    if (n->kind == Kind::Constant) return rationalString(n->constant);
    if (n->kind == Kind::Variable) return n->name;
    if (n->kind == Kind::Unsupported) return "unsupported";
    const char *op = n->kind == Kind::Add ? "+" : n->kind == Kind::Subtract ? "-" : "*";
    auto left = print(n->left.get(), depth + 1), right = print(n->right.get(), depth + 1);
    if (left.size() + right.size() + 5 > 65536)
      throw std::length_error("expanded SSA expression exceeds 65536 characters");
    std::string text = "(" + std::string(op) + " " + left + " " + right + ")";
    memo.emplace(n, text);
    return text;
  };
  return print(expression.root().get(), 0);
}

std::string formatResult(const Result &result) {
  llvm::json::Object object;
  object["status"] = statusName(result.status);
  object["reason"] = result.reason;
  object["mode"] = modeName(result.options.mode);
  object["scope"] = "header visits after t completed backedges; no trip-count or exit-value claim";
  if (result.options.maxIterations)
    object["horizon"] = result.options.maxIterations->convert_to<std::string>();
  if (!result.options.noOverflowAssumption.empty())
    object["assumption"] = result.options.noOverflowAssumption;
  llvm::json::Array states;
  if (result.status != Status::Unknown) {
    object["header_block"] = result.headerBlock;
    object["bit_width"] = result.bitWidth;
    for (const State &state : result.states) {
      llvm::json::Object item;
      item["argument"] = state.argument;
      item["name"] = state.name;
      item["initial"] = rationalString(state.initial);
      item["next"] = formatExpression(state.next);
      item["increment"] = formatExpression(state.increment);
      llvm::json::Array deps, coefficients;
      for (unsigned dep : state.dependencies) deps.push_back(dep);
      for (const Rational &coefficient : result.solution.integerLifts.at(state.name).coefficients())
        coefficients.push_back(rationalString(coefficient));
      item["dependencies"] = std::move(deps);
      item["binomial_coefficients"] = std::move(coefficients);
      states.push_back(std::move(item));
    }
  }
  object["states"] = std::move(states);
  std::string output;
  llvm::raw_string_ostream stream(output);
  stream << llvm::json::Value(std::move(object));
  return output;
}
} // namespace systolic::polynomial::cfg
