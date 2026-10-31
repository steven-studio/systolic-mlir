#include "Systolic/MLIRPolynomialTermination.h"
#include "PolynomialTerminationInternal.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlow.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Parser/Parser.h"
#include <iostream>
#include <set>
#include <stdexcept>

using namespace systolic::polynomial;
namespace term = systolic::polynomial::termination;
namespace {
unsigned checks = 0, terminating = 0, nonterminating = 0, unknown = 0, mutations = 0;
void check(bool ok, const std::string &why) { ++checks; if (!ok) throw std::runtime_error(why); }
void status(const term::Result &r, term::Status expected) {
  check(r.status == expected, "unexpected result: " + r.reason);
  if (expected == term::Status::Unknown) {
    ++unknown; check(!r.certificate, "UNKNOWN must not contain a partial certificate");
    check(term::formatResult(r).find("\"certificate\":null") != std::string::npos,"UNKNOWN report has no certificate");
  } else {
    if(expected == term::Status::ProvenTerminating) ++terminating; else ++nonterminating;
    check(r.certificate.has_value() && r.definedTotalPathVerified,"positive result requires certificate and totality");
    check(!r.sourceFingerprint.empty() && !r.solverVersion.empty(),"proof provenance");
    check(!r.obligations.empty(),"solver obligations recorded");
  }
}
}
int main(int argc, char **argv) {
  try {
    check(argc==2,"expected fixture path");
    mlir::DialectRegistry registry;
    registry.insert<mlir::arith::ArithDialect,mlir::cf::ControlFlowDialect,mlir::func::FuncDialect,mlir::memref::MemRefDialect>();
    mlir::MLIRContext context(registry);
    auto module=mlir::parseSourceFile<mlir::ModuleOp>(argv[1],&context);
    check(bool(module) && succeeded(mlir::verify(*module)),"fixtures verify");
    std::string before=term::detail::snapshot(module.get());
    const std::set<std::string> positive{"inc_s","dec_s","inc_u","dec_u","always_false","swapped","reversed_polarity","forwarded","signed_one_bit"};
    unsigned functions=0;
    for(auto f:module->getOps<mlir::func::FuncOp>()) {
      ++functions;
      auto expected=positive.count(f.getSymName().str()) ? term::Status::ProvenTerminating :
                    f.getSymName()=="always_true" ? term::Status::ProvenNonterminating : term::Status::Unknown;
      std::string old=cfg::formatResult(cfg::analyzeSymbolic(f));
      auto r=term::analyze(f);
      try { status(r,expected); } catch (const std::exception &e) { throw std::runtime_error(f.getSymName().str()+": "+e.what()); }
      check(term::formatResult(r)==term::formatResult(term::analyze(f)),"deterministic report: "+f.getSymName().str());
      check(old==cfg::formatResult(cfg::analyzeSymbolic(f)),"P2a result unchanged");
      if(r.certificate) status(term::verifyCertificate(f,*r.certificate),expected);
    }
    check(functions==36,"all fixture functions checked");
    auto function=[&](const char *name) { return module->lookupSymbol<mlir::func::FuncOp>(name); };
    term::Options bounded; bounded.parameterRanges[1]={-128,126};
    status(term::analyze(function("inclusive"),bounded),term::Status::ProvenTerminating);
    auto overshoot=term::analyze(function("step_two"),bounded);
    status(overshoot,term::Status::ProvenTerminating);
    check(overshoot.certificate->offset==0,"overshoot rank is un-clamped bound minus counter");
    // At i=125,bound=126 the successor i=127 has rank -1. This MUST be
    // accepted: the successor guard is false, so nonnegativity is not required.
    check(Integer(126)-125==1 && Integer(126)-127==-1,"independent negative exiting rank example");
    term::Options max; max.parameterRanges[1]={127,127};
    status(term::analyze(function("inclusive"),max),term::Status::ProvenNonterminating);
    status(term::analyze(function("step_two"),max),term::Status::Unknown); // mixed parity behavior
    // Guard equivalence is not a termination argument. Quantify over symbolic
    // counters/bounds, including endpoints, in widened mathematical arithmetic.
    for (unsigned w : {1u, 8u, 64u}) for (bool sign : {false, true}) for (bool up : {false, true}) {
      const auto bits=std::to_string(w), wide=std::to_string(w+2);
      const std::string extension=sign ? "sign_extend" : "zero_extend";
      const std::string strict=up ? "lt" : "gt", inclusive=up ? "le" : "ge";
      const std::string adjustment=up ? "bvsub" : "bvadd";
      const std::string prelude="(set-logic QF_BV)\n(declare-const i (_ BitVec "+bits+"))\n"
        "(declare-const n (_ BitVec "+bits+"))\n"
        "(define-fun wi () (_ BitVec "+wide+") ((_ "+extension+" 2) i))\n"
        "(define-fun wn () (_ BitVec "+wide+") ((_ "+extension+" 2) n))\n";
      const auto shifted="("+adjustment+" wn (_ bv1 "+wide+"))";
      const auto gap=up ? "(bvsub wn wi)" : "(bvsub wi wn)";
      const auto normalizedGap=up ? "(bvsub "+shifted+" wi)" : "(bvsub wi "+shifted+")";
      const auto equivalence=prelude+"(assert (or (distinct (bvs"+strict+" wi wn) (bvs"+inclusive+" wi "+shifted+"))"
        " (distinct "+gap+" (bvadd "+normalizedGap+" (_ bv1 "+wide+")))))\n";
      check(term::detail::solve("widened-symbolic-guard-and-rank-equivalence",equivalence,"unsat",term::Limits{}).outcome=="unsat",
            "strict/inclusive guard and rank equivalence, including signed/unsigned endpoints");
      Integer edge=up ? (sign ? Integer(1)<<(w-1) : Integer(0)) :
                         (sign ? (Integer(1)<<(w-1))-1 : (Integer(1)<<w)-1);
      const std::string p=sign ? "bvs" : "bvu";
      const auto wrapping=prelude+"(assert (= n (_ bv"+edge.convert_to<std::string>()+" "+bits+")))\n"
        "(assert (distinct ("+p+strict+" i n) ("+p+inclusive+" i ("+adjustment+" n (_ bv1 "+bits+")))))\n";
      check(term::detail::solve("unchecked-source-bound-adjustment-is-invalid",wrapping,"sat",term::Limits{}).outcome=="sat",
            "source-width adjustment at an extremum must not be used as normalization");
    }
    for (const auto &boundary : std::vector<std::pair<const char *,int>>{
           {"inc_s",-128},{"dec_s",127},{"inc_u",0},{"dec_u",-1}}) {
      term::Options edge; edge.parameterRanges[1]={boundary.second,boundary.second};
      status(term::analyze(function(boundary.first),edge),term::Status::ProvenTerminating);
    }
    term::Options even; even.parameterRanges[0]={0,0}; even.parameterRanges[1]={127,127};
    auto wraps=term::analyze(function("step_two"),even);
    status(wraps,term::Status::Unknown); // current recurrent set cannot express parity
    bool descentCounterexample=false;
    for (const auto &q : wraps.obligations) if(q.name=="strict_descent") {
      descentCounterexample=q.outcome=="sat";
      check(q.smtlib.find("(bvslt s0 p1)")!=std::string::npos,"original strict guard retained");
      check(q.smtlib.find("(_ BitVec 10)")!=std::string::npos,"rank arithmetic widened");
    }
    check(descentCounterexample,"actual source-width wraparound defeats the ranking candidate");
    mlir::OwningOpRef<mlir::func::FuncOp> normalized(function("step_two").clone());
    normalized->walk([](mlir::arith::CmpIOp op) { op.setPredicate(mlir::arith::CmpIPredicate::sle); });
    even.parameterRanges[1]={126,126};
    check(succeeded(mlir::verify(normalized.get())),"equivalent inclusive fixture verifies");
    status(term::analyze(normalized.get(),even),term::Status::Unknown);
    // An exact closed cycle, not a timeout: all 128 distinct even residues
    // satisfy BOTH guards, and the final transition returns to the initial state.
    unsigned residue=0; std::set<unsigned> cycle;
    for(unsigned t=0;t<128;++t) {
      int value=residue<128 ? int(residue) : int(residue)-256;
      check(value<127 && value<=126,"both equivalent guards continue along the cycle");
      check(cycle.insert(residue).second,"independent full-state cycle has distinct states");
      residue=(residue+2)%256;
    }
    check(residue==0 && cycle.size()==128,"defined modular execution repeats its initial state");
    term::Options point; point.parameterRanges[0]={0,0}; point.parameterRanges[1]={1,1};
    status(term::analyze(function("stutter"),point),term::Status::ProvenNonterminating);
    status(term::analyze(function("invariant_guard"),point),term::Status::ProvenNonterminating);
    point.parameterRanges[0]={1,1};
    status(term::analyze(function("stutter"),point),term::Status::ProvenTerminating);
    status(term::analyze(function("invariant_guard"),point),term::Status::ProvenTerminating);
    term::Options small; small.parameterRanges[0]={0,5}; small.parameterRanges[1]={10,10};
    auto original=term::analyze(function("inc_s"),small);
    status(original,term::Status::ProvenTerminating);
    auto mutate=[&](term::Certificate c) {
      ++mutations; status(term::verifyCertificate(function("inc_s"),c,small),term::Status::Unknown);
    };
    auto c=*original.certificate;
    c.ascending=false; mutate(c); // candidate is not a proof
    c=*original.certificate; c.sourceFingerprint="wrong"; mutate(c);
    c=*original.certificate; c.guard="(sge s0 p1)"; mutate(c);
    c=*original.certificate; c.domain[0].upper=0; mutate(c); // silent domain strengthening
    c=*original.certificate; c.domain.clear(); mutate(c);
    c=*original.certificate; c.counter=99; mutate(c);
    c=*original.certificate; c.signedValues=false; mutate(c);
    c=*original.certificate; c.offset=100; mutate(c);
    c=*original.certificate; c.invariant={{0,1,127}}; mutate(c); // fails initial condition
    c=*original.certificate; c.invariant={{0,0,5}}; mutate(c); // fails inductiveness at 5
    c=*original.certificate; c.invariant={{0,5,1}}; mutate(c); // empty invariant
    c=*original.certificate; c.kind=term::CertificateKind::ZeroIterations; mutate(c);
    c=*original.certificate; c.kind=term::CertificateKind::RecurrentGuard; mutate(c); // reachable exit
    c=*original.certificate; c.kind=static_cast<term::CertificateKind>(77); mutate(c);
    c=*original.certificate; c.invariant={{0,0,127}};
    status(term::verifyCertificate(function("inc_s"),c,small),term::Status::ProvenTerminating);
    status(term::verifyCertificate(function("dec_s"),*original.certificate,small),term::Status::Unknown);
    for (unsigned which=0;which<10;++which) {
      term::Options limited;
      switch(which) {
      case 0:limited.limits.maxQueries=1;break;
      case 1:limited.limits.maxCandidates=1;break;
      case 2:limited.limits.maxNodes=1;break;
      case 3:limited.limits.maxOperations=1;break;
      case 4:limited.limits.maxBlocks=1;break;
      case 5:limited.limits.maxSMTBytes=1;break;
      case 6:limited.limits.maxParameters=1;break;
      case 7:limited.limits.timeoutMilliseconds=0;break;
      case 8:limited.parameterRanges[0]={1,0};break;
      case 9:limited.parameterRanges[0]={-129,0};break;
      }
      status(term::analyze(function("inc_s"),limited),term::Status::Unknown);
    }
    term::Options resource; resource.limits.solverResourceLimit=1;
    auto exhausted=term::analyze(function("inc_s"),resource);
    status(exhausted,term::Status::Unknown);
    check(!exhausted.obligations.empty() && exhausted.obligations.back().outcome=="unknown","real Z3 resource UNKNOWN");
    term::Limits timeout; timeout.timeoutMilliseconds=1;
    std::string hard="(set-logic QF_BV)\n";
    for(unsigned i=0;i<100;++i) {
      hard+="(declare-const x"+std::to_string(i)+" (_ BitVec 32))\n";
      hard+="(assert (bvult x"+std::to_string(i)+" (_ bv99 32)))\n";
    }
    hard+="(assert (distinct"; for(unsigned i=0;i<100;++i) hard+=" x"+std::to_string(i); hard+="))\n";
    auto timed=term::detail::solve("timeout-test",hard,"unsat",timeout);
    check(timed.outcome=="unknown","real Z3 timeout produces no proof");
    check(timed.detail.find("timeout")!=std::string::npos || timed.detail.find("canceled")!=std::string::npos,"timeout reason recorded");
    auto invalid=term::detail::solve("syntax-test","(invalid syntax)","unsat",term::Limits{});
    check(invalid.outcome=="error","unsupported SMT syntax fails closed");
    status(term::analyze(mlir::func::FuncOp{}),term::Status::Unknown);
    check(term::detail::snapshot(module.get())==before,"all analyses and mutations preserve IR");
    std::cout<<"PASS: "<<checks<<" P2b checks; "<<functions<<" verified fixtures; "<<terminating<<" PROVEN_TERMINATING; "
             <<nonterminating<<" PROVEN_NONTERMINATING; "<<unknown<<" expected UNKNOWN; "<<mutations
             <<" rejected certificate mutations; real solver resource/timeout/syntax rejection checks passed\n";
  } catch(const std::exception &e) { std::cerr<<"FAIL after "<<checks<<" checks: "<<e.what()<<'\n'; return 1; }
}
