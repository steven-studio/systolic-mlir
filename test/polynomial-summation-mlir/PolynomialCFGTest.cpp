#include "Systolic/MLIRPolynomialRecurrence.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlow.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/raw_ostream.h"

#include <functional>
#include <iostream>
#include <stdexcept>

using namespace systolic::polynomial;
using namespace mlir;

namespace {
size_t checks = 0, executions = 0, headerValues = 0;
void check(bool condition, const std::string &message) {
  ++checks;
  if (!condition) throw std::runtime_error(message);
}

std::string snapshot(Operation *op) {
  std::string text;
  llvm::raw_string_ostream stream(text);
  op->print(stream, OpPrintingFlags().printGenericOpForm().enableDebugInfo());
  return text;
}

Integer asInteger(const llvm::APInt &a, bool isSigned) {
  llvm::SmallString<128> text;
  a.toString(text, 10, isSigned);
  return Integer(text.str().str());
}

// Independent execution oracle: execute the actual operations and branches,
// using APInt bitvectors. It never consults extracted transitions, polynomial
// arithmetic, the dependency graph, or BranchOpInterface's transfer helper.
// The analyzer supplies only the block at which observations are requested.
using Trace = std::vector<std::vector<llvm::APInt>>;
Trace execute(func::FuncOp function, Block *observe, unsigned n) {
  llvm::DenseMap<Value, llvm::APInt> values;
  auto read = [&](Value v) -> llvm::APInt {
    auto it = values.find(v);
    if (it == values.end()) throw std::runtime_error("interpreter read undefined SSA value");
    return it->second;
  };
  auto transfer = [&](Block *dest, ValueRange operands) {
    std::vector<llvm::APInt> next;
    for (Value operand : operands) next.push_back(read(operand));
    // Collect ALL operands first, including self-loop forwarding arguments.
    for (unsigned i = 0; i < next.size(); ++i) values[dest->getArgument(i)] = next[i];
  };
  check(function.getNumArguments() == 1, "oracle expects one integer parameter");
  auto argument = function.getArgument(0);
  values[argument] = llvm::APInt(cast<IntegerType>(argument.getType()).getWidth(), n);
  Block *block = &function.getBody().front();
  Trace trace;
  ++executions;
  for (unsigned steps = 0; steps < 20000; ++steps) {
    if (block == observe) {
      std::vector<llvm::APInt> state;
      for (BlockArgument arg : block->getArguments()) state.push_back(read(arg));
      trace.push_back(std::move(state));
    }
    Block *successor = nullptr;
    for (Operation &op : *block) {
      if (auto c = dyn_cast<arith::ConstantOp>(op)) {
        values[c.getResult()] = cast<IntegerAttr>(c.getValue()).getValue();
      } else if (isa<arith::AddIOp, arith::SubIOp, arith::MulIOp>(op)) {
        auto a = read(op.getOperand(0)), b = read(op.getOperand(1));
        values[op.getResult(0)] = isa<arith::AddIOp>(op) ? a + b
            : isa<arith::SubIOp>(op) ? a - b : a * b;
      } else if (auto c = dyn_cast<arith::ExtSIOp>(op)) {
        values[c.getResult()] = read(c.getIn()).sext(cast<IntegerType>(c.getType()).getWidth());
      } else if (auto c = dyn_cast<arith::ExtUIOp>(op)) {
        values[c.getResult()] = read(c.getIn()).zext(cast<IntegerType>(c.getType()).getWidth());
      } else if (auto c = dyn_cast<arith::TruncIOp>(op)) {
        values[c.getResult()] = read(c.getIn()).trunc(cast<IntegerType>(c.getType()).getWidth());
      } else if (auto c = dyn_cast<arith::CmpIOp>(op)) {
        auto a = read(c.getLhs()), b = read(c.getRhs());
        bool predicate = false;
        using Pred = arith::CmpIPredicate;
        switch (c.getPredicate()) {
        case Pred::eq: predicate = a == b; break;
        case Pred::ne: predicate = a != b; break;
        case Pred::slt: predicate = a.slt(b); break;
        case Pred::sle: predicate = a.sle(b); break;
        case Pred::sgt: predicate = a.sgt(b); break;
        case Pred::sge: predicate = a.sge(b); break;
        case Pred::ult: predicate = a.ult(b); break;
        case Pred::ule: predicate = a.ule(b); break;
        case Pred::ugt: predicate = a.ugt(b); break;
        case Pred::uge: predicate = a.uge(b); break;
        }
        values[c.getResult()] = llvm::APInt(1, predicate);
      } else if (auto branch = dyn_cast<cf::BranchOp>(op)) {
        successor = branch.getDest();
        transfer(successor, branch.getDestOperands());
      } else if (auto branch = dyn_cast<cf::CondBranchOp>(op)) {
        bool takeTrue = !read(branch.getCondition()).isZero();
        successor = takeTrue ? branch.getTrueDest() : branch.getFalseDest();
        transfer(successor, takeTrue ? branch.getTrueDestOperands() : branch.getFalseDestOperands());
      } else if (isa<func::ReturnOp>(op)) {
        return trace;
      } else {
        throw std::runtime_error("unsupported operation in independent interpreter");
      }
    }
    check(successor != nullptr, "interpreter block has no successor");
    block = successor;
  }
  throw std::runtime_error("independent interpreter execution budget exceeded");
}

Polynomial substitute(const Expression &expression, const std::map<std::string, Polynomial> &forms) {
  using Kind = Expression::Kind;
  std::function<Polynomial(const Expression::Node *)> visit = [&](const Expression::Node *n) {
    switch (n->kind) {
    case Kind::Constant: return Polynomial::constant(n->constant);
    case Kind::Variable: return forms.at(n->name);
    case Kind::Add: return visit(n->left.get()) + visit(n->right.get());
    case Kind::Subtract: return visit(n->left.get()) - visit(n->right.get());
    case Kind::Multiply: return visit(n->left.get()) * visit(n->right.get());
    default: throw std::runtime_error("unsupported expression in identity check");
    }
  };
  return visit(expression.root().get());
}

void identities(const cfg::Result &result) {
  check(result.status != Status::Unknown, "identities require an accepted result: " + result.reason);
  for (const auto &state : result.states) {
    const auto &form = result.solution.integerLifts.at(state.name);
    check(form.evaluate(0) == state.initial, "initial condition");
    check(form.difference() == substitute(state.increment, result.solution.integerLifts), "finite difference identity");
    check(form.shift(1) == substitute(state.next, result.solution.integerLifts), "full next-state identity");
  }
}

void compareExecution(func::FuncOp f, const cfg::Result &result, unsigned n, bool posttested = false) {
  auto trace = execute(f, result.header, n);
  check(trace.size() == (posttested ? std::max(1u, n) : n + 1), "independent header count: " + f.getSymName().str());
  for (unsigned t = 0; t < trace.size(); ++t) {
    check(trace[t].size() == result.states.size(), "state count");
    for (unsigned j = 0; j < trace[t].size(); ++j) {
      Integer actual = asInteger(trace[t][j], result.options.mode != IntegerMode::Modular);
      check(result.solution.valueAt(result.states[j].name, t) == actual,
            "direct execution mismatch " + f.getSymName().str() + " t=" + std::to_string(t) + " s=" + std::to_string(j));
      ++headerValues;
    }
  }
}

struct Expected {
  std::vector<int> initial;
  std::vector<std::string> increments;
  std::vector<std::vector<unsigned>> dependencies;
};

const std::map<std::string, Expected> expected = {
  {"sum_integers", {{0,0}, {"1/1", "s0"}, {{}, {0}}}},
  {"sum_squares", {{0,0}, {"1/1", "(* s0 s0)"}, {{}, {0}}}},
  {"sum_cubes", {{0,0}, {"1/1", "(* (* s0 s0) s0)"}, {{}, {0}}}},
  {"sum_fifth", {{0,0}, {"1/1", "(* (* (* s0 s0) (* s0 s0)) s0)"}, {{}, {0}}}},
  {"quadratic_shifted", {{0,-5,17}, {"1/1", "2/1", "(+ (- (* 3/1 (* s1 s1)) (* 7/1 s1)) 11/1)"}, {{}, {}, {1}}}},
  {"cubic_negative_step", {{0,19,-31}, {"1/1", "(- 0/1 3/1)", "(+ (+ (- (* 2/1 (* (* s1 s1) s1)) (* s1 s1)) s1) 5/1)"}, {{}, {}, {1}}}},
  {"triangular", {{0,3,7}, {"1/1", "(* s0 s0)", "s1"}, {{}, {0}, {1}}}},
  {"triangular_next_y", {{0,3,7}, {"1/1", "(* s0 s0)", "(+ s1 (* s0 s0))"}, {{}, {0}, {0,1}}}},
  {"sum_squares_next_i", {{0,0}, {"1/1", "(* (+ s0 1/1) (+ s0 1/1))"}, {{}, {0}}}},
  {"forwarded_noncanonical", {{0,0}, {"1/1", "(* s0 s0)"}, {{}, {0}}}},
  {"unreachable_predecessor", {{0,0}, {"1/1", "s0"}, {{}, {0}}}},
  {"posttested", {{0,0}, {"1/1", "s0"}, {{}, {0}}}},
  {"constant_casts", {{0,-3,255}, {"1/1", "1/1", "s1"}, {{}, {}, {1}}}},
  {"modular_overflow", {{0,120,-10}, {"1/1", "7/1", "(* s1 s1)"}, {{}, {}, {1}}}},
};

void expectUnknown(func::FuncOp function, const cfg::Options &options = {}) {
  std::string before = function ? snapshot(function) : "";
  auto result = cfg::analyze(function, options);
  check(result.status == Status::Unknown, "expected UNKNOWN: " + (function ? function.getSymName().str() : "null"));
  check(!result.reason.empty(), "UNKNOWN has explicit reason");
  check(result.states.empty() && result.solution.integerLifts.empty() && !result.header, "UNKNOWN leaks no partial forms");
  check(before == (function ? snapshot(function) : ""), "rejected IR changed");
  check(cfg::formatResult(result) == cfg::formatResult(cfg::analyze(function, options)), "rejection nondeterministic");
}

void boundsTests() {
  // Exercise the newly public P0 helper without altering any P0 regression.
  for (int seed = -5; seed <= 5; ++seed) {
    Polynomial p({Rational(seed), Rational(-3*seed, 2), Rational(7), Rational(seed, 3)});
    for (int last = 0; last <= 30; ++last) {
      auto bounds = p.bounds(last);
      for (int t = 0; t <= last; ++t)
        check(bounds.first <= p.evaluate(t) && p.evaluate(t) <= bounds.second, "range enclosure");
    }
  }
  bool rejected = false;
  try { (void)Polynomial().bounds(-1); } catch (const std::invalid_argument &) { rejected = true; }
  check(rejected, "negative range domain must fail");
}
} // namespace

int main(int argc, char **argv) {
  try {
    if (argc != 2) throw std::invalid_argument("expected fixture directory");
    DialectRegistry registry;
    registry.insert<arith::ArithDialect, cf::ControlFlowDialect, func::FuncDialect,
                    memref::MemRefDialect, scf::SCFDialect>();
    MLIRContext context(registry);
    auto load = [&](const char *name) {
      auto module = parseSourceFile<ModuleOp>(std::string(argv[1]) + "/" + name, &context);
      check(bool(module), std::string("fixture parse/verification failed: ") + name);
      return module;
    };
    auto accepted = load("accepted.mlir");
    std::string original = snapshot(accepted.get());
    size_t functions = 0;
    for (auto f : accepted->getOps<func::FuncOp>()) {
      ++functions;
      auto result = cfg::analyze(f);
      identities(result);
      check(result.status == Status::Proven, "modular proof");
      check(cfg::formatResult(result) == cfg::formatResult(cfg::analyze(f)), "deterministic result");
      const auto &spec = expected.at(f.getSymName().str());
      check(result.states.size() == spec.initial.size(), "expected state count");
      for (unsigned i = 0; i < result.states.size(); ++i) {
        check(result.states[i].argument == i && result.states[i].name == "s" + std::to_string(i), "header argument ordering");
        check(result.states[i].initial == Rational(spec.initial[i]), "independent initial relation");
        check(cfg::formatExpression(result.states[i].increment) == spec.increments[i], "independent increment relation: " + f.getSymName().str());
        check(result.states[i].dependencies == spec.dependencies[i], "independent dependency graph");
      }
      unsigned last = f.getSymName() == "modular_overflow" ? 255 : 80;
      for (unsigned n = 0; n <= last; ++n)
        compareExecution(f, result, n, f.getSymName() == "posttested");
      if (f.getSymName() == "modular_overflow") continue;
      for (auto mode : {IntegerMode::Mathematical, IntegerMode::SignedNoOverflow}) {
        cfg::Options options;
        options.mode = mode;
        options.maxIterations = 10;
        auto checked = cfg::analyze(f, options);
        identities(checked);
        check(checked.status == Status::Proven, "safe signed source proof");
        for (unsigned n = 0; n <= 10; ++n)
          compareExecution(f, checked, n, f.getSymName() == "posttested");
      }
    }
    check(functions == 14 && functions == expected.size(), "all preserved acceptance fixtures tested");
    check(snapshot(accepted.get()) == original, "module unchanged by all analyses");

    auto lookup = [&](const char *name) { return accepted->lookupSymbol<func::FuncOp>(name); };
    auto oldI = cfg::analyze(lookup("sum_squares"));
    auto newI = cfg::analyze(lookup("sum_squares_next_i"));
    check(oldI.solution.valueAt("s1", 10) == 285, "old-index sum");
    check(newI.solution.valueAt("s1", 10) == 385, "off-by-one mutation changes result");
    check(oldI.solution.integerLifts.at("s1") != newI.solution.integerLifts.at("s1"), "index mutation distinguished symbolically");
    auto oldY = cfg::analyze(lookup("triangular"));
    auto newY = cfg::analyze(lookup("triangular_next_y"));
    check(oldY.solution.valueAt("s2", 2) == 13, "simultaneous assignment uses old y");
    check(newY.solution.valueAt("s2", 2) == 14, "explicit next-y mutation distinguished");

    // Invalid API input after parsing: mutate a clone, never a fixture.
    OwningOpRef<func::FuncOp> broken(cast<func::FuncOp>(lookup("sum_squares")->clone()));
    auto brokenResult = cfg::analyze(*broken);
    auto branch = cast<cf::BranchOp>(broken->getBody().front().getTerminator());
    branch->setOperand(0, brokenResult.header->getArgument(0)); // does not dominate entry
    {
      ScopedDiagnosticHandler suppress(&context, [](Diagnostic &) { return success(); });
      check(failed(verify(*broken)), "invalid SSA mutation must fail verifier");
      expectUnknown(*broken);
    }
    expectUnknown(func::FuncOp());

    auto rejected = load("rejected.mlir");
    size_t rejections = 0;
    for (auto f : rejected->getOps<func::FuncOp>()) {
      ++rejections;
      expectUnknown(f);
      cfg::Options options;
      options.mode = IntegerMode::SignedNoOverflow;
      options.maxIterations = 10;
      options.noOverflowAssumption = "all original intermediates fit signed range";
      expectUnknown(f, options); // assumptions never enable unsupported shapes
    }
    check(rejections == 14, "all preserved rejection fixtures tested");
    auto structured = load("structured.mlir");
    for (auto f : structured->getOps<func::FuncOp>()) expectUnknown(f);

    auto overflow = load("overflow.mlir");
    for (auto f : overflow->getOps<func::FuncOp>()) {
      auto name = f.getSymName();
      if (name == "signed_flag" || name == "unsigned_flag") {
        expectUnknown(f);
      } else {
        auto result = cfg::analyze(f);
        identities(result);
        for (unsigned n = 0; n <= 80; ++n) compareExecution(f, result, n);
      }
      cfg::Options options;
      options.mode = IntegerMode::SignedNoOverflow;
      options.maxIterations = 20;
      if (name == "signed_flag") {
        auto result = cfg::analyze(f, options);
        identities(result);
        check(result.status == Status::Proven, "nsw checked with a proof");
        for (unsigned n = 0; n <= 20; ++n) compareExecution(f, result, n);
      } else {
        expectUnknown(f, options);
      }
      if (name == "loose_but_safe") {
        options.noOverflowAssumption = "all source intermediates fit signed i8 through header visit 20";
        auto conditional = cfg::analyze(f, options);
        check(conditional.status == Status::Conditional, "explicit assumption remains conditional");
        identities(conditional);
        for (unsigned n = 0; n <= 20; ++n) compareExecution(f, conditional, n);
      }
    }
    for (auto mode : {IntegerMode::Mathematical, IntegerMode::SignedNoOverflow}) {
      cfg::Options options;
      options.mode = mode;
      expectUnknown(lookup("sum_squares"), options); // no horizon
      options.maxIterations = -1;
      expectUnknown(lookup("sum_squares"), options);
      options.maxIterations = 0;
      identities(cfg::analyze(lookup("sum_squares"), options));
      options.maxIterations = 100;
      expectUnknown(lookup("modular_overflow"), options);
    }
    cfg::Options invalid;
    invalid.noOverflowAssumption = "invalid modular contract";
    expectUnknown(lookup("sum_squares"), invalid);
    invalid = {};
    invalid.mode = static_cast<IntegerMode>(99);
    expectUnknown(lookup("sum_squares"), invalid);
    bool outside = false;
    try { (void)oldI.solution.valueAt("s1", -1); } catch (const std::out_of_range &) { outside = true; }
    check(outside, "negative header iteration rejected");
    // P1 must preserve the P0 evaluation domain when it bridges source modes.
    for (auto mode : {IntegerMode::Modular, IntegerMode::Mathematical, IntegerMode::SignedNoOverflow}) {
      cfg::Options options;
      options.mode = mode;
      options.maxIterations = 10;
      auto bounded = cfg::analyze(lookup("sum_squares"), options);
      identities(bounded);
      check(bounded.solution.valueAt("s1", 10) == 285, "inclusive header horizon");
      bool rejectedPastHorizon = false;
      try { (void)bounded.solution.valueAt("s1", 11); }
      catch (const std::out_of_range &) { rejectedPastHorizon = true; }
      check(rejectedPastHorizon, "evaluation cannot escape P1's contract horizon");
    }
    boundsTests();
    std::cout << "PASS: " << checks << " P1 checks; " << executions
              << " independent APInt CFG executions; " << headerValues
              << " header-state value comparisons; 14 accepted and 14 rejected fixture functions\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "FAIL after " << checks << " checks: " << error.what() << '\n';
    return 1;
  }
}
