#include "Systolic/MLIRPolynomialRecurrence.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/Support/raw_ostream.h"
#include <functional>
#include <iostream>
#include <random>
#include <set>
#include <stdexcept>

using namespace systolic::polynomial;
using namespace mlir;
using SP = SymbolicPolynomial;
namespace {
size_t checks = 0, executions = 0, comparisons = 0, proven = 0, conditional = 0, unknown = 0;
void check(bool condition, const std::string &message) {
  ++checks;
  if (!condition) throw std::runtime_error(message);
}
template<class F> void throws(F action, const std::string &message) {
  bool caught = false;
  try { action(); } catch (const std::exception &) { caught = true; }
  check(caught, message);
}
std::string snapshot(Operation *op) {
  std::string text;
  llvm::raw_string_ostream stream(text);
  op->print(stream, OpPrintingFlags().printGenericOpForm().enableDebugInfo());
  return text;
}
Integer integer(const llvm::APInt &bits, bool signedValue) {
  llvm::SmallString<128> text;
  bits.toString(text, 10, signedValue);
  return Integer(text.str().str());
}
llvm::APInt bits(const Integer &value, unsigned width) {
  Integer modulus = Integer(1) << width, residue = value % modulus;
  if (residue < 0) residue += modulus;
  return llvm::APInt(width, residue.convert_to<std::string>(), 10);
}
using Trace = std::vector<std::vector<llvm::APInt>>;
// Independent execution of the original verified MLIR. No extracted relation,
// polynomial operation, dependency scheduler, or branch transfer helper used.
Trace execute(func::FuncOp function, Block *observe, const std::vector<Integer> &inputs) {
  ++executions;
  llvm::DenseMap<Value, llvm::APInt> env;
  auto read = [&](Value v) { return env.at(v); };
  auto transfer = [&](Block *dest, ValueRange operands) {
    std::vector<llvm::APInt> next;
    for (Value v : operands) next.push_back(read(v));
    for (unsigned i = 0; i < next.size(); ++i) env[dest->getArgument(i)] = next[i];
  };
  for (unsigned i = 0; i < inputs.size(); ++i)
    env[function.getArgument(i)] = bits(inputs[i], cast<IntegerType>(function.getArgument(i).getType()).getWidth());
  Block *block = &function.getBody().front();
  Trace trace;
  for (unsigned budget = 0; budget < 2000; ++budget) {
    if (block == observe) {
      std::vector<llvm::APInt> state;
      for (auto arg : block->getArguments()) state.push_back(read(arg));
      trace.push_back(std::move(state));
    }
    Block *next = nullptr;
    for (Operation &op : *block) {
      if (auto c = dyn_cast<arith::ConstantOp>(op)) {
        env[c.getResult()] = cast<IntegerAttr>(c.getValue()).getValue();
      } else if (isa<arith::AddIOp, arith::SubIOp, arith::MulIOp>(op)) {
        auto a = read(op.getOperand(0)), b = read(op.getOperand(1));
        bool overflow = false;
        arith::IntegerOverflowFlags flags;
        llvm::APInt value;
        if (auto arithmetic=dyn_cast<arith::AddIOp>(op)) {
          value=a.sadd_ov(b,overflow); flags=arithmetic.getOverflowFlags();
        } else if (auto arithmetic=dyn_cast<arith::SubIOp>(op)) {
          value=a.ssub_ov(b,overflow); flags=arithmetic.getOverflowFlags();
        } else {
          value=a.smul_ov(b,overflow); flags=cast<arith::MulIOp>(op).getOverflowFlags();
        }
        if(flags!=arith::IntegerOverflowFlags::none)
          check(flags==arith::IntegerOverflowFlags::nsw && !overflow, "independent executor checks nsw precondition");
        env[op.getResult(0)] = value;
      } else if (auto c = dyn_cast<arith::CmpIOp>(op)) {
        check(c.getPredicate() == arith::CmpIPredicate::slt, "oracle fixture uses signed less-than");
        env[c.getResult()] = llvm::APInt(1, read(c.getLhs()).slt(read(c.getRhs())));
      } else if (auto b = dyn_cast<cf::BranchOp>(op)) {
        next = b.getDest(); transfer(next, b.getDestOperands());
      } else if (auto b = dyn_cast<cf::CondBranchOp>(op)) {
        bool yes = !read(b.getCondition()).isZero();
        next = yes ? b.getTrueDest() : b.getFalseDest();
        transfer(next, yes ? b.getTrueDestOperands() : b.getFalseDestOperands());
      } else if (isa<func::ReturnOp>(op)) return trace;
      else throw std::runtime_error("unsupported independent interpreter op");
    }
    check(next != nullptr, "interpreter successor");
    block = next;
  }
  throw std::runtime_error("interpreter budget");
}
void accepted(const cfg::SymbolicResult &r, Status status = Status::Proven) {
  check(r.status == status, "expected accepted result: " + r.reason);
  if (status == Status::Proven) ++proven; else ++conditional;
  check(r.header && !r.states.empty() && r.states.size() == r.integerLifts.size(), "complete symbolic proof");
}
void rejected(const cfg::SymbolicResult &r) {
  ++unknown;
  check(r.status == Status::Unknown, "expected UNKNOWN: " + r.reason);
  check(!r.reason.empty() && !r.header && r.parameters.empty() && r.states.empty() && r.integerLifts.empty(),
        "UNKNOWN has reason and no partial proof");
  throws([&] { r.valueAt("s0", 0, {}); }, "UNKNOWN cannot be evaluated");
  check(cfg::formatResult(r).find("binomial_coefficients") == std::string::npos, "no reported polynomial on UNKNOWN");
}
struct Relations { std::vector<std::string> initial, increment; std::vector<std::vector<unsigned>> deps; };
const std::map<std::string, Relations> relations = {
  {"accumulator", {{"0/1", "p1"}, {"1/1", "s0"}, {{}, {0}}}},
  {"shifted_square", {{"0/1", "p2", "p1"}, {"1/1", "1/1", "(* s1 s1)"}, {{}, {}, {1}}}},
  {"negative_step", {{"0/1", "p2", "p1"}, {"1/1", "(- 0/1 2/1)", "(* s1 s1)"}, {{}, {}, {1}}}},
  {"symbolic_step", {{"0/1", "p2", "p1"}, {"1/1", "p3", "(* s1 s1)"}, {{}, {}, {1}}}},
  {"polynomial_offset", {{"0/1", "(- p2 p3)", "(+ p1 (* p2 p2))"},
      {"1/1", "(- 0/1 1/1)", "(+ (* (+ s1 p3) (+ s1 p3)) (* p2 p2))"}, {{}, {}, {1}}}},
  {"wrap8", {{"0/1", "(- p2 p3)", "(+ p1 (* p2 p2))"},
      {"1/1", "(- 0/1 1/1)", "(+ (* (+ s1 p3) (+ s1 p3)) (* p2 p2))"}, {{}, {}, {1}}}},
  {"triangular_old", {{"0/1", "p2", "p1", "p3"}, {"1/1", "1/1", "(* s1 s1)", "s2"}, {{}, {}, {1}, {2}}}},
  {"triangular_next", {{"0/1", "p2", "p1", "p3"}, {"1/1", "1/1", "(* s1 s1)", "(+ s2 (* s1 s1))"}, {{}, {}, {1}, {1,2}}}},
  {"invariant_carried", {{"0/1", "p2", "p1", "p3"}, {"1/1", "1/1", "(* s3 s1)", "0/1"}, {{}, {}, {1,3}, {}}}},
  {"forwarded", {{"0/1", "p2", "(- p1 p2)"}, {"1/1", "1/1", "(+ (* s1 s1) p1)"}, {{}, {}, {1}}}},
};
void algebra() {
  auto a = SP::parameter("a"), b = SP::parameter("b");
  SP t(Polynomial({Rational(0), Rational(1)})), one = SP::constant(Rational(1));
  auto p = (b+t)*(b+t), integral = p.integrate()+a;
  check(integral.atIteration(0) == a, "symbolic initial identity");
  check(integral.difference() == p && integral.shift(1) == integral+p, "symbolic transition identity");
  // Independently specified conventional formula, including rational coefficients.
  SP classical(Polynomial::fromMonomial({Rational(0),Rational(1,6),Rational(-1,2),Rational(1,3)}));
  auto expected = a + t*b*b + b*t*(t-one) + classical;
  check(integral == expected, "shifted square equals independent conventional formula");
  check((a+b)*(a-b) == a*a-b*b, "canonical multivariate cancellation");
  check((a-a).isZero() && (p.scale(Rational(1,7))*SP::constant(Rational(7))) == p, "exact rational normalization");
  for (int av=-7; av<=7; ++av) for (int bv=-9; bv<=9; ++bv) {
    Integer direct = av;
    for (int n=0; n<=24; ++n) {
      check(integral.evaluate(n, {{"a", av}, {"b", bv}}) == Rational(direct), "independent square summation");
      direct += Integer(bv+n)*(bv+n);
    }
  }
  auto enclosure = integral.bounds(8, {{"a", {-2,3}}, {"b", {-3,2}}});
  for (int av=-2; av<=3; ++av) for (int bv=-3; bv<=2; ++bv) for (int n=0;n<=8;++n) {
    auto value=integral.evaluate(n, {{"a",av},{"b",bv}});
    check(value>=enclosure.first && value<=enclosure.second, "symbolic interval enclosure");
  }
  throws([&] { integral.substitute({{"a",0}}); }, "missing parameter rejected");
  throws([&] { integral.bounds(-1, {}); }, "negative range rejected");
  throws([&] { auto power=a; for(unsigned i=0;i<5;++i) power=power*power; }, "parameter degree bound");
  throws([&] { SP p=one; for(unsigned i=0;i<9;++i) p=p*(one+SP::parameter("p"+std::to_string(i))); }, "term growth bound");
  throws([&] { SP p; for(unsigned i=0;i<17;++i) p=p+SP::parameter("p"+std::to_string(i)); }, "parameter count bound");
  throws([&] { SP::constant(Rational(Integer(1)<<SP::MaxCoefficientBits)); }, "coefficient bit bound");
  throws([&] { std::vector<Rational> c(65); c.back()=Rational(1); SP(Polynomial(c)).integrate(); }, "P0 iteration degree bound retained");
}
} // namespace

int main(int argc, char **argv) {
  try {
    if (argc != 3) throw std::invalid_argument("expected P2a fixture dir and P1 nneg fixture");
    algebra();
    DialectRegistry registry;
    registry.insert<arith::ArithDialect, cf::ControlFlowDialect, func::FuncDialect, memref::MemRefDialect>();
    MLIRContext context(registry);
    auto load = [&](const std::string &path) {
      auto module=parseSourceFile<ModuleOp>(path,&context);
      check(bool(module) && succeeded(verify(*module)), "valid verified fixture: "+path);
      return module;
    };
    auto module=load(std::string(argv[1])+"/accepted.mlir");
    auto before=snapshot(module.get());
    std::mt19937_64 random(0x502a);
    unsigned functions=0;
    for(auto f: module->getOps<func::FuncOp>()) {
      ++functions;
      const auto &spec=relations.at(f.getSymName().str());
      auto r=cfg::analyzeSymbolic(f);
      accepted(r);
      check(r.states.size()==spec.initial.size(), "state count");
      check(cfg::formatResult(r)==cfg::formatResult(cfg::analyzeSymbolic(f)), "deterministic symbolic output");
      check(cfg::analyze(f).status==Status::Unknown, "P1 constant-only contract preserved");
      std::map<std::string,SP> environment=r.integerLifts;
      for(const auto &p:r.parameters) environment.emplace(p.name,SP::parameter(p.name));
      for(unsigned i=0;i<r.states.size();++i) {
        const auto &s=r.states[i];
        check(cfg::formatExpression(s.initial)==spec.initial[i], "independent initial relation "+s.name);
        check(cfg::formatExpression(s.increment)==spec.increment[i], "independent increment relation "+s.name);
        check(s.dependencies==spec.deps[i], "independent state dependencies");
        const auto &p=r.integerLifts.at(s.name);
        check(p.atIteration(0)==substituteSymbolic(s.initial,environment), "exact initial identity");
        check(p.difference()==substituteSymbolic(s.increment,environment), "exact difference identity");
        check(p.shift(1)==substituteSymbolic(s.next,environment), "exact simultaneous transition identity");
      }
      if(f.getSymName()=="shifted_square") {
        auto a=SP::parameter("p1"), b=SP::parameter("p2");
        SP t(Polynomial({Rational(0),Rational(1)}));
        SP conventional(Polynomial::fromMonomial({Rational(0),Rational(1,6),Rational(-1,2),Rational(1,3)}));
        check(r.integerLifts.at("s2")==a+t*b*b+b*t*(t-SP::constant(Rational(1)))+conventional,
              "CFG derives alpha + t beta^2 + beta t(t-1) + square sum");
      }
      for(unsigned sample=0;sample<64;++sample) {
        unsigned n=sample%13;
        std::vector<Integer> inputs(f.getNumArguments()); inputs[0]=n;
        SP::Assignment assignment;
        for(const auto &p:r.parameters) {
          std::vector<Integer> adversarial{p.lower, p.lower+1, -17,-1,0,1,17,p.upper};
          Integer v=sample<8 ? adversarial[(sample+p.argument)%8] : Integer(static_cast<int64_t>(random()%100001)-50000);
          v=integer(bits(v,p.bitWidth),true);
          inputs[p.argument]=v; assignment[p.name]=v;
        }
        auto trace=execute(f,r.header,inputs);
        check(trace.size()==n+1, "original CFG observation count");
        for(unsigned t=0;t<trace.size();++t) for(unsigned i=0;i<trace[t].size();++i) {
          check(r.valueAt(r.states[i].name,t,assignment)==integer(trace[t][i],false), "modular independent MLIR execution");
          ++comparisons;
        }
      }
      for(auto mode:{IntegerMode::SignedNoOverflow,IntegerMode::Mathematical}) {
        cfg::SymbolicOptions options; options.integers.mode=mode; options.integers.maxIterations=5;
        rejected(cfg::analyzeSymbolic(f,options)); // full domains cannot establish no overflow
        options.integers.noOverflowAssumption="all original intermediates and states fit their signed width";
        auto assumed=cfg::analyzeSymbolic(f,options); accepted(assumed,Status::Conditional);
        SP::Assignment zero; for(const auto &p:r.parameters) zero[p.name]=0;
        std::vector<Integer> inputs(f.getNumArguments(),0); inputs[0]=5;
        auto trace=execute(f,assumed.header,inputs); // an assignment satisfying the contract
        for(unsigned t=0;t<trace.size();++t) for(unsigned i=0;i<trace[t].size();++i) {
          check(assumed.valueAt(assumed.states[i].name,t,zero)==integer(trace[t][i],true), "conditional integer execution");
          ++comparisons;
        }
      }
      if(f.getSymName()=="wrap8") continue; // full-range signed wrap was rejected above
      for(auto mode:{IntegerMode::SignedNoOverflow,IntegerMode::Mathematical}) {
        cfg::SymbolicOptions options; options.integers.mode=mode; options.integers.maxIterations=5;
        for(const auto &p:r.parameters) options.parameterRanges[p.argument]={-2,2};
        auto signedResult=cfg::analyzeSymbolic(f,options); accepted(signedResult);
        for(int seed=-2;seed<=2;++seed) {
          std::vector<Integer> inputs(f.getNumArguments()); inputs[0]=5;
          SP::Assignment assignment;
          for(const auto &p:r.parameters) { inputs[p.argument]=seed; assignment[p.name]=seed; }
          auto trace=execute(f,signedResult.header,inputs);
          for(unsigned t=0;t<trace.size();++t) for(unsigned i=0;i<trace[t].size();++i) {
            check(signedResult.valueAt(signedResult.states[i].name,t,assignment)==integer(trace[t][i],true), "proved signed MLIR execution");
            ++comparisons;
          }
        }
        SP::Assignment outside; for(const auto &p:r.parameters) outside[p.name]=3;
        throws([&] { signedResult.valueAt("s0",0,outside); }, "parameter domain enforced");
        throws([&] { signedResult.valueAt("s0",6,{}); }, "horizon enforced");
      }
    }
    check(functions==10 && functions==relations.size(), "all accepted fixtures covered");
    check(snapshot(module.get())==before, "accepted input IR unchanged");
    auto failures=load(std::string(argv[1])+"/rejected.mlir"); before=snapshot(failures.get());
    unsigned rejectedFunctions=0;
    for(auto f:failures->getOps<func::FuncOp>()) {
      ++rejectedFunctions;
      for(auto mode:{IntegerMode::Modular,IntegerMode::SignedNoOverflow,IntegerMode::Mathematical}) {
        cfg::SymbolicOptions options; options.integers.mode=mode; options.integers.maxIterations=5;
        rejected(cfg::analyzeSymbolic(f,options));
        if(mode!=IntegerMode::Modular) {
          options.integers.noOverflowAssumption="all original intermediates fit";
          rejected(cfg::analyzeSymbolic(f,options));
        }
      }
    }
    check(rejectedFunctions==9,"all rejected fixtures covered");
    check(snapshot(failures.get())==before,"rejected input IR unchanged");
    auto semantics=load(std::string(argv[1])+"/semantics.mlir"); before=snapshot(semantics.get());
    for(auto f:semantics->getOps<func::FuncOp>()) {
      for(auto mode:{IntegerMode::SignedNoOverflow,IntegerMode::Mathematical}) {
        cfg::SymbolicOptions options; options.integers.mode=mode; options.integers.maxIterations=5;
        rejected(cfg::analyzeSymbolic(f,options));
        options.integers.noOverflowAssumption="all original intermediates fit";
        auto assumed=cfg::analyzeSymbolic(f,options);
        if(f.getSymName()=="canceled_overflow") accepted(assumed,Status::Conditional);
        else rejected(assumed); // a general caller assertion cannot prove nsw
        options.integers.noOverflowAssumption.clear();
        for(unsigned i=1;i<f.getNumArguments();++i) options.parameterRanges[i]={-2,2};
        if(f.getSymName()=="flag_operand_wrap") options.parameterRanges[1]={-2,-1};
        auto bounded=cfg::analyzeSymbolic(f,options); accepted(bounded);
        for(bool upper:{false,true}) {
          SP::Assignment assignment;
          std::vector<Integer> inputs(f.getNumArguments()); inputs[0]=5;
          for(const auto &p:bounded.parameters) {
            Integer value=upper ? p.upper : p.lower;
            inputs[p.argument]=value; assignment[p.name]=value;
          }
          auto trace=execute(f,bounded.header,inputs);
          for(unsigned t=0;t<trace.size();++t) for(unsigned i=0;i<trace[t].size();++i) {
            check(bounded.valueAt(bounded.states[i].name,t,assignment)==integer(trace[t][i],true), "flagged bounded independent execution");
            ++comparisons;
          }
        }
      }
      auto modular=cfg::analyzeSymbolic(f);
      if(f.getSymName()=="canceled_overflow") accepted(modular); else rejected(modular);
    }
    check(snapshot(semantics.get())==before,"flag/overflow IR unchanged");
    // Replay all P1 nneg fixtures through the NEW API too, without editing them.
    auto casts=load(argv[2]); before=snapshot(casts.get());
    const std::set<std::string> validCasts={"plain_negative","nneg_zero","nneg_positive","nneg_signed_max",
        "nneg_i1_zero","nneg_wide_max","plain_backedge","nneg_backedge_positive","nneg_extui_chain_nonnegative"};
    unsigned castFunctions=0;
    for(auto f:casts->getOps<func::FuncOp>()) {
      ++castFunctions;
      for(auto mode:{IntegerMode::Modular,IntegerMode::SignedNoOverflow,IntegerMode::Mathematical}) {
        cfg::SymbolicOptions options; options.integers.mode=mode; options.integers.maxIterations=10;
        auto r=cfg::analyzeSymbolic(f,options);
        if(validCasts.count(f.getSymName().str())) accepted(r); else rejected(r);
      }
    }
    check(castFunctions==22,"all nneg fixture functions covered");
    check(snapshot(casts.get())==before,"nneg input IR unchanged");
    std::cout<<"PASS: "<<checks<<" P2a C++ checks; "<<executions<<" independent MLIR executions; "
             <<comparisons<<" state comparisons; "<<proven<<" PROVEN, "<<conditional<<" CONDITIONAL, "
             <<unknown<<" UNKNOWN analysis cases\n";
  } catch(const std::exception &error) {
    std::cerr<<"FAIL after "<<checks<<" checks: "<<error.what()<<'\n'; return 1;
  }
}
