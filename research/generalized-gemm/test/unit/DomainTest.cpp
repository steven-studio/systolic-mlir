// G5a: Theorem 12 and 13, the facts beside them, and what must never pass.
// test/fuzz/domain_oracle.py recomputes the results with no library code.
#include "GeneralizedGemm/DomainAnalysis.h"
#include "GeneralizedGemm/LoopCounters.h"
#include "GeneralizedGemm/ReductionSemantics.h"
#include "GeneralizedGemm/SiteSets.h"
#include "GeneralizedGemm/TerminationAnalysis.h"
#include "GeneralizedGemm/TerminationObligations.h"
#include "GeneralizedGemm/Z3Backend.h"

#include "TestSupport.h"

#include <chrono>
#include <cstring>
#include <functional>
#include <limits>
#include <map>
#include <set>

using namespace mlir::generalized_gemm;
using llvm::APInt;
using S = ComputationStatus;
using T = ComputationTier;

static mlir::MLIRContext *context;
static Z3Backend z3Backend;

struct Module {
  mlir::OwningOpRef<mlir::ModuleOp> module;
  explicit Module(const std::string &text) : module(parse(*context, text)) {
    if (!module) {
      fprintf(stderr, "cannot parse:\n%s\n", text.c_str());
      exit(2);
    }
  }
  SemanticProgram import(const std::string &name, unsigned width = 16) {
    ImportOptions options;
    options.indexBitWidth = width;
    options.lowerStructured = true;
    return importFunction(module->lookupSymbol<mlir::func::FuncOp>(name), options);
  }
};

static SmtLimits testSolver() {
  SmtLimits limits;
  limits.timeoutMilliseconds = 3000;
  limits.resourceLimit = 2000000;
  return limits;
}

static ParameterScope range(const SemanticProgram &p, int64_t low, int64_t high) {
  ParameterScope scope;
  if (p.parameters.empty())
    return scope;
  scope.allParameters = false;
  unsigned w = p.parameters[0].sort.width;
  scope.ranges.push_back({0, APInt(w, low, true), APInt(w, high, true)});
  return scope;
}

static Authorizations everyPair(const SemanticProgram &p) {
  Authorizations out;
  for (unsigned a = 0; a < p.roots.size(); ++a)
    for (unsigned b = a + 1; b < p.roots.size(); ++b)
      out.insert("roots " + std::to_string(a) + " and " + std::to_string(b) + " are separate");
  return out;
}

struct Fixture {
  SemanticProgram program;
  ParameterScope scope;
  Authorizations authorized;
  std::optional<TerminationCertificate> certificate;
  DomainEnumeration enumeration;
  DomainOptions options;
  const TerminationCertificate *proof() const { return certificate ? &*certificate : nullptr; }
};

static Fixture fixture(Module &m, const char *name, int64_t low, int64_t high, unsigned width = 16,
                       std::vector<std::vector<APInt>> only = {}) {
  Fixture f;
  f.program = m.import(name, width);
  CHECK(f.program.isSupported());
  f.scope = range(f.program, low, high);
  f.authorized = everyPair(f.program);
  TerminationOptions termination;
  termination.scope = f.scope;
  termination.authorized = f.authorized;
  termination.limits.replayFuel = 1u << 12;
  termination.limits.solver = testSolver();
  f.certificate = analyzeTermination(f.program, z3Backend, termination).certificate;
  EnumerationOptions enumeration;
  enumeration.scope = f.scope;
  enumeration.authorized = f.authorized;
  enumeration.termination.solver = testSolver();
  enumeration.liveStatePoints = true;
  enumeration.onlyPoints = std::move(only);
  f.enumeration = enumerateDomain(f.program, f.proof(), enumeration, z3Backend);
  f.options.scope = f.scope;
  f.options.authorized = f.authorized;
  f.options.termination.solver = testSolver();
  return f;
}

/// The loop variables, as a family: I = i, J = j, K = k, and anything else in P.
static CoordinateFamily loopFamily(const SemanticProgram &p, std::vector<unsigned> i,
                                   std::vector<unsigned> j, std::vector<unsigned> k,
                                   std::vector<unsigned> batch = {}) {
  CoordinateFamily family;
  family.origin = "the test";
  auto put = [&](unsigned axis, const std::vector<unsigned> &ids) {
    for (unsigned id : ids)
      family.axes[axis].push_back(p.terms->state(id, p.state[id].sort));
  };
  put(AxisP, batch);
  put(AxisI, i);
  put(AxisJ, j);
  put(AxisK, k);
  return family;
}

/// The state ids of the loop variables, outermost first: the block arguments in cycles.
static std::vector<unsigned> loops(const SemanticProgram &p) {
  std::vector<unsigned> out;
  for (const StateVar &s : p.state)
    if (s.kind == StateVar::BlockArgument && s.loopCarried && s.sort.kind == Sort::BV)
      out.push_back(s.id);
  return out;
}

static const char *H = "  %c0 = arith.constant 0 : index\n  %c1 = arith.constant 1 : index\n"
                       "  %c2 = arith.constant 2 : index\n  %c3 = arith.constant 3 : index\n"
                       "  %c4 = arith.constant 4 : index\n";

static std::string mac(const std::string &c = "%c[%i, %j]", const std::string &a = "%a[%i, %k]",
                       const std::string &b = "%b[%k, %j]", const std::string &ind = "      ",
                       const std::string &type = "memref<8x8xi32>", const std::string &element = "i32",
                       const std::string &add = "arith.addi", const std::string &mul = "arith.muli",
                       const std::string &flags = "", const std::string &s = "") {
  return ind + "%x" + s + " = memref.load " + a + " : " + type + "\n" + ind + "%y" + s +
         " = memref.load " + b + " : " + type + "\n" + ind + "%z" + s + " = memref.load " + c + " : " +
         type + "\n" + ind + "%m" + s + " = " + mul + " %x" + s + ", %y" + s + " : " + element + "\n" +
         ind + "%s" + s + " = " + add + " %z" + s + ", %m" + s + flags + " : " + element + "\n" + ind +
         "memref.store %s" + s + ", " + c + " : " + type + "\n";
}

static std::string fn(const std::string &name, const std::string &body,
                      const std::string &type = "memref<8x8xi32>") {
  return "func.func @" + name + "(%a: " + type + ", %b: " + type + ", %c: " + type +
         ", %n: index) {\n" + H + body + "  return\n}\n";
}

static std::string nest(const std::string &inner, const std::string &kb = "%c3",
                        const std::string &jb = "%c3", const std::string &kstep = "%c1") {
  return "  scf.for %i = %c0 to %n step %c1 {\n    scf.for %j = %c0 to " + jb +
         " step %c1 {\n      scf.for %k = %c0 to " + kb + " step " + kstep + " {\n" + inner +
         "      }\n    }\n  }\n";
}

static std::string guarded(const std::string &condition) {
  return condition + "        scf.if %on {\n" + mac("%c[%i, %j]", "%a[%i, %k]", "%b[%k, %j]", "          ") +
         "        }\n";
}

static std::string catalogue() {
  std::string out;
  out += fn("rect", nest(mac()));
  out += fn("triangle", nest(mac(), "%j", "%c4"));
  out += fn("holed", "  %mask = arith.constant 365 : index\n" +
                         nest(guarded("        %sh = arith.muli %j, %c3 : index\n"
                                      "        %pos = arith.addi %sh, %k : index\n"
                                      "        %bit = arith.shrui %mask, %pos : index\n"
                                      "        %one = arith.andi %bit, %c1 : index\n"
                                      "        %on = arith.cmpi eq, %one, %c1 : index\n")));
  out += fn("nonaffine", nest(guarded("        %p = arith.muli %j, %k : index\n"
                                      "        %r = arith.remui %p, %c3 : index\n"
                                      "        %on = arith.cmpi eq, %r, %c0 : index\n")));
  out += fn("band", nest(guarded("        %d = arith.subi %j, %k : index\n"
                                 "        %e = arith.addi %d, %c1 : index\n"
                                 "        %on = arith.cmpi ule, %e, %c2 : index\n"), "%c4", "%c4"));
  out += fn("piecewise", nest("        %on = arith.cmpi ult, %k, %j : index\n        scf.if %on {\n" +
                              mac("%c[%i, %j]", "%a[%i, %k]", "%b[%k, %j]", "          ") +
                              "        } else {\n" +
                              mac("%c[%i, %j]", "%a[%i, %k]", "%b[%k, %j]", "          ", "memref<8x8xi32>",
                                  "i32", "arith.addi", "arith.muli", "", "e") +
                              "        }\n"));
  out += fn("repeated", "  scf.for %t = %c0 to %c2 step %c1 {\n" + nest(mac()) + "  }\n");
  out += fn("reversed", nest("        %kk = arith.subi %c2, %k : index\n" +
                             mac("%c[%i, %j]", "%a[%i, %kk]", "%b[%kk, %j]")));
  out += fn("stepped", nest(mac(), "%c4", "%c3", "%c2"));
  out += fn("collide", nest("        %s0 = arith.addi %i, %j : index\n        %h = arith.remui %s0, %c2 : index\n" +
                            mac("%c[%h, %c0]")));
  out += fn("nsw", nest(mac("%c[%i, %j]", "%a[%i, %k]", "%b[%k, %j]", "      ", "memref<8x8xi32>", "i32",
                            "arith.addi", "arith.muli", " overflow<nsw>")));
  out += fn("floats", nest(mac("%c[%i, %j]", "%a[%i, %k]", "%b[%k, %j]", "      ", "memref<8x8xf64>", "f64",
                               "arith.addf", "arith.mulf")),
            "memref<8x8xf64>");
  out += fn("single", nest(mac("%c[%i, %k]", "%a[%i, %k]", "%b[%k, %j]"), "%c3", "%c1"));
  // Another effect reads the output while it is being summed.
  out += fn("observed", nest(mac() + "      %q = memref.load %c[%c0, %c0] : memref<8x8xi32>\n"
                                     "      memref.store %q, %a[%c4, %c4] : memref<8x8xi32>\n"));
  return out;
}

/// The request the proposer makes from a family of the loop variables.
static std::optional<DomainRequest> propose(const Fixture &f, const CoordinateFamily &family,
                                            std::string &why) {
  return proposeDomain(f.program, f.enumeration, 0, family, nullptr, ProposalLimits(), why);
}

static CoordinateFamily ijk(const SemanticProgram &p) {
  std::vector<unsigned> l = loops(p);
  if (l.size() == 4) // An outer repetition loop goes to P.
    return loopFamily(p, {l[1]}, {l[2]}, {l[3]}, {l[0]});
  CHECK(l.size() == 3);
  return loopFamily(p, {l[0]}, {l[1]}, {l[2]});
}

static void testCatalogue() {
  Module m(catalogue());
  struct Want {
    const char *name;
    bool symbolic;          // Theorem 12 over the scope.
    FactStatus integer;     // The integer reorder fact.
    FactStatus floating;    // The float reorder fact.
    FactStatus injective;   // C1, from the strongest route.
  };
  for (Want w : {Want{"rect", true, FactStatus::Proven, FactStatus::Unknown, FactStatus::Proven},
                 Want{"triangle", true, FactStatus::Proven, FactStatus::Unknown, FactStatus::Proven},
                 Want{"holed", true, FactStatus::Proven, FactStatus::Unknown, FactStatus::Proven},
                 Want{"nonaffine", true, FactStatus::Proven, FactStatus::Unknown, FactStatus::Proven},
                 Want{"band", true, FactStatus::Proven, FactStatus::Unknown, FactStatus::Proven},
                 Want{"piecewise", true, FactStatus::Proven, FactStatus::Unknown, FactStatus::Proven},
                 // With t as P, two batches share each cell: two logical outputs, one cell.
                 Want{"repeated", true, FactStatus::Proven, FactStatus::Unknown, FactStatus::Refuted},
                 Want{"stepped", true, FactStatus::Proven, FactStatus::Unknown, FactStatus::Proven},
                 Want{"collide", true, FactStatus::Proven, FactStatus::Unknown, FactStatus::Refuted},
                 Want{"nsw", true, FactStatus::Refuted, FactStatus::Unknown, FactStatus::Proven},
                 Want{"floats", true, FactStatus::Unknown, FactStatus::Refuted, FactStatus::Proven},
                 Want{"single", true, FactStatus::Proven, FactStatus::Unknown, FactStatus::Proven},
                 Want{"observed", true, FactStatus::Unknown, FactStatus::Unknown, FactStatus::Proven}}) {
    Fixture f = fixture(m, w.name, 1, 2);
    CoordinateFamily family = ijk(f.program);
    std::string why;
    auto request = propose(f, family, why);
    if (!request)
      printf("%s: no proposal: %s\n", w.name, why.c_str());
    CHECK(request.has_value());
    if (!request)
      continue;
    ReductionVerdict v = checkReduction(f.program, f.proof(), f.enumeration, 0, family, nullptr,
                                        *request, f.options, z3Backend);
    bool ok = v.domainFinite().status() == S::Verified &&
              v.domainFinite().tier() == T::ScopeEnumerated &&
              (v.domainSymbolic().status() == S::Verified) == w.symbolic &&
              v.status() == S::Verified && v.integerReorder().status == w.integer &&
              v.floatReorder().status == w.floating;
    Fact injective = v.domainSymbolic().outputInjective().status == FactStatus::Proven
                         ? v.domainSymbolic().outputInjective()
                         : v.domainFinite().outputInjective();
    ok = ok && injective.status == w.injective;
    if (!ok)
      printf("%s: finite %s %s (%s); symbolic %s (%s); T13 %s %s; int %s (%s); float %s; C1 %s\n", w.name,
             stringifyComputationStatus(v.domainFinite().status()), stringifyComputationTier(v.domainFinite().tier()),
             v.domainFinite().reason().c_str(), stringifyComputationStatus(v.domainSymbolic().status()),
             v.domainSymbolic().reason().c_str(), stringifyComputationStatus(v.status()),
             stringifyComputationTier(v.tier()), stringifyFactStatus(v.integerReorder().status),
             v.integerReorder().detail.c_str(), stringifyFactStatus(v.floatReorder().status),
             stringifyFactStatus(injective.status));
    CHECK(ok);
    // Theorem 13 holds over the scope where Theorem 12 does, and nothing is authorized.
    CHECK(v.tier() == (w.symbolic ? T::Symbolic : T::ScopeEnumerated));
    CHECK(v.transformation().kind() == ProofStatus::Kind::Unknown);
    CHECK(v.domainSymbolic().exactOrder() == w.symbolic);
    // Every refuted fact carries what replays it.
    for (const Fact *fact : {&v.integerReorder(), &v.floatReorder()}) {
      if (fact->status != FactStatus::Refuted)
        continue;
      CHECK(fact->memory.has_value() && fact->point.has_value());
      if (fact->memory) {
        ReplayResult replay = replayGroup(f.program, f.enumeration.groups[0], *fact->point, HerbrandLimits());
        std::string detail;
        CHECK(reversalDiffers(f.program, f.enumeration.groups[0], *fact->memory, replay.prefix, detail));
      }
    }
    if (injective.status == FactStatus::Refuted)
      CHECK(injective.point.has_value());
  }

  // Multiplicity. With t outside the family, t is a repeat level: a box of 2.
  {
    Fixture f = fixture(m, "repeated", 1, 2);
    std::vector<unsigned> l = loops(f.program);
    CoordinateFamily family = loopFamily(f.program, {l[1]}, {l[2]}, {l[3]});
    std::string why;
    auto request = propose(f, family, why);
    CHECK(request.has_value());
    if (request) {
      CheckedDomain finite = checkDomainFinite(f.program, f.enumeration, 0, family, *request, f.options);
      CheckedDomain symbolic = checkDomainSymbolic(f.program, f.proof(), f.enumeration.groups[0], family,
                                                   *request, f.options, z3Backend);
      CHECK(finite.status() == S::Verified && finite.multiplicity() == DomainMultiplicity::ConstantBox);
      CHECK(symbolic.status() == S::Verified && symbolic.multiplicity() == DomainMultiplicity::ConstantBox);
      for (const DomainPointFacts &point : finite.points())
        CHECK(point.events == 0 || point.maxMultiplicity == 2);
      auto mu = evaluateLevelTerm(symbolic.multiplicityTerm(), {APInt(16, 2)}, {});
      CHECK(mu && mu->getZExtValue() == 2);
      // Repeats share a cell, and the key must keep t.
      CHECK(llvm::is_contained(request->cellKey, 0u));
      CHECK(symbolic.cellKey().status == FactStatus::Proven);
    }
  }
}

/// A request changed in one place, from a correct one.
static void testAdversarial() {
  Module m(catalogue());
  auto base = [&](const char *name, int64_t low, int64_t high) {
    Fixture f = fixture(m, name, low, high);
    CoordinateFamily family = ijk(f.program);
    std::string why;
    auto request = propose(f, family, why);
    CHECK(request.has_value());
    return std::make_tuple(std::move(f), family, request ? *request : DomainRequest());
  };
  auto judge = [&](const char *what, Fixture &f, const CoordinateFamily &family,
                   const DomainRequest &request, S finiteWant, const char *finiteText,
                   bool symbolicMustFail = true) {
    CheckedDomain finite = checkDomainFinite(f.program, f.enumeration, 0, family, request, f.options);
    CheckedDomain symbolic = checkDomainSymbolic(f.program, f.proof(), f.enumeration.groups[0], family,
                                                 request, f.options, z3Backend);
    bool ok = finite.status() == finiteWant &&
              (!finiteText || finite.reason().find(finiteText) != std::string::npos) &&
              (!symbolicMustFail || symbolic.status() != S::Verified);
    if (!ok)
      printf("%s: finite %s (%s); symbolic %s (%s)\n", what, stringifyComputationStatus(finite.status()),
             finite.reason().c_str(), stringifyComputationStatus(symbolic.status()), symbolic.reason().c_str());
    CHECK(ok);
    CHECK(symbolic.status() != S::Refuted);
    if (finite.status() == S::Refuted)
      CHECK(finite.refutation().point.has_value());
    return std::make_pair(std::move(finite), std::move(symbolic));
  };
  TermContext *terms = nullptr;

  {
    // The guard dropped: holes become points with no event.
    auto [f, family, request] = base("holed", 1, 2);
    terms = f.program.terms.get();
    DomainRequest loose = request;
    loose.order.guard = terms->boolean(true);
    judge("guard dropped", f, family, loose, S::Refuted, "has no event");
    // A guard that removes a point that has an event.
    DomainRequest tight = request;
    Term k = levelVariable(*terms, 2, request.order.levels[2].value->sort);
    tight.order.guard = terms->logicalAnd(request.order.guard,
                                          terms->icmp(Predicate::Ne, k, terms->bv(APInt(16, 0))));
    judge("guard too tight", f, family, tight, S::Refuted, "is no point of the set");
  }
  {
    auto [f, family, request] = base("rect", 1, 2);
    terms = f.program.terms.get();
    // Off by one in each direction.
    for (int delta : {-1, 1}) {
      DomainRequest off = request;
      off.order.levels[2].upper = terms->binary(TermOp::Add, request.order.levels[2].upper,
                                                terms->bv(APInt(16, (uint64_t)(int64_t)delta, true)));
      judge(delta < 0 ? "upper bound short" : "upper bound long", f, family, off, S::Refuted, nullptr);
    }
    // The wrong direction: events are in the other order.
    DomainRequest reversed = request;
    reversed.order.levels[2].descending = !reversed.order.levels[2].descending;
    judge("direction reversed", f, family, reversed, S::Refuted, "not in the order");
    // Levels exchanged: the order is not the execution order.
    DomainRequest swapped = request;
    std::swap(swapped.order.levels[1], swapped.order.levels[2]);
    for (GuardedLevel &level : swapped.order.levels) {
      level.lower = terms->bv(APInt(16, 0));
      level.upper = terms->bv(APInt(16, 3));
    }
    swapped.order.levels[0].upper = terms->param(0, Sort::bv(16));
    judge("levels exchanged", f, family, swapped, S::Refuted, "not in the order");
    // A key too short: two contributions of a cell differ in k, which it leaves out.
    DomainRequest shortKey = request;
    shortKey.cellKey = {0};
    CheckedDomain keyed = checkDomainFinite(f.program, f.enumeration, 0, family, shortKey, f.options);
    CHECK(keyed.status() == S::Verified && keyed.cellKey().status == FactStatus::Refuted);
    CheckedDomain keyedSymbolic = checkDomainSymbolic(f.program, f.proof(), f.enumeration.groups[0], family,
                                                      shortKey, f.options, z3Backend);
    CHECK(keyedSymbolic.cellKey().status != FactStatus::Proven);
    // A closed form that is wrong.
    DomainRequest wrongAccess = request;
    wrongAccess.access[0] = {levelVariable(*terms, 1, Sort::bv(16)), levelVariable(*terms, 0, Sort::bv(16))};
    CheckedDomain accessed = checkDomainFinite(f.program, f.enumeration, 0, family, wrongAccess, f.options);
    CHECK(accessed.access()[0].status == FactStatus::Refuted);
    CHECK(checkDomainSymbolic(f.program, f.proof(), f.enumeration.groups[0], family, wrongAccess, f.options,
                              z3Backend).access()[0].status != FactStatus::Proven);
    // Malformed requests are refused before anything is checked.
    auto refused = [&](const char *what, const DomainRequest &r) {
      CheckedDomain finite = checkDomainFinite(f.program, f.enumeration, 0, family, r, f.options);
      CheckedDomain symbolic = checkDomainSymbolic(f.program, f.proof(), f.enumeration.groups[0], family, r,
                                                   f.options, z3Backend);
      bool ok = finite.status() == S::Unknown && symbolic.status() == S::Unknown &&
                finite.reason().find("refused") != std::string::npos;
      if (!ok)
        printf("%s: %s / %s\n", what, finite.reason().c_str(), symbolic.reason().c_str());
      CHECK(ok);
    };
    DomainRequest inner = request;
    inner.order.levels[0].upper = levelVariable(*terms, 2, Sort::bv(16));
    refused("a bound reads an inner level", inner);
    DomainRequest stateBound = request;
    stateBound.order.levels[1].upper = request.order.levels[2].value;
    refused("a bound reads state", stateBound);
    DomainRequest memoryLevel = request;
    memoryLevel.order.levels[1].value = f.program.sites[0].aValue;
    refused("a level reads memory", memoryLevel);
    DomainRequest missing = request;
    missing.order.levels.pop_back();
    refused("a coordinate is no level", missing);
    DomainRequest badKey = request;
    badKey.cellKey = {2, 1};
    refused("a key out of order", badKey);
  }
  {
    // A repetition the order does not tell apart: duplicates, which no order can hold.
    auto [f, family, request] = base("repeated", 1, 2);
    std::vector<unsigned> l = loops(f.program);
    CoordinateFamily inner = loopFamily(f.program, {l[1]}, {l[2]}, {l[3]});
    DomainRequest flat = request;
    flat.order.levels.erase(flat.order.levels.begin());
    flat.cellKey = {2};
    // The bounds read level 0 no more, but held the indices of the four levels before.
    for (GuardedLevel &level : flat.order.levels) {
      level.lower = f.program.terms->bv(APInt(16, 0));
      level.upper = f.program.terms->bv(APInt(16, 3));
    }
    flat.order.levels[0].upper = f.program.terms->param(0, Sort::bv(16));
    flat.order.guard = f.program.terms->boolean(true);
    flat.access = {};
    judge("repeats not told apart", f, inner, flat, S::Refuted, "not in the order");
  }
  {
    // A point with no event, where the order says there are some.
    auto [f, family, request] = base("rect", 0, 2);
    DomainRequest fixed = request;
    fixed.order.levels[0].upper = f.program.terms->bv(APInt(16, 2));
    judge("events at n = 0", f, family, fixed, S::Refuted, "has no event");
  }
  {
    // A sample is no scope: the same request at one point is Point, and no more.
    Module again(catalogue());
    Fixture f = fixture(again, "rect", 1, 2, 16, {{APInt(16, 1)}});
    CoordinateFamily family = ijk(f.program);
    std::string why;
    auto request = propose(f, family, why);
    CHECK(request.has_value());
    if (request) {
      CheckedDomain finite = checkDomainFinite(f.program, f.enumeration, 0, family, *request, f.options);
      CHECK(finite.status() == S::Verified && finite.tier() == T::Point);
    }
  }
}

/// Wrapping at the width: bounds that pass the top of the range.
static void testWrapping() {
  Module m(R"(func.func @wide(%a: memref<300xi32>, %b: memref<300xi32>, %c: memref<300xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c250 = arith.constant 250 : index
  scf.for %i = %c250 to %n step %c1 {
    %x = memref.load %a[%i] : memref<300xi32>
    %y = memref.load %b[%i] : memref<300xi32>
    %z = memref.load %c[%i] : memref<300xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<300xi32>
  }
  return
}
)");
  // At 16 bits the loop runs from 250 to n. The order says 250 <= i < n + 1 - 1, which
  // is right, and then i < n + 7 at 8 bits would wrap: the 16-bit program has no wrap,
  // so the request is wrong at n = 255 only through the bound written below.
  Fixture f = fixture(m, "wide", 252, 255);
  std::vector<unsigned> l = loops(f.program);
  CHECK(l.size() == 1);
  CoordinateFamily family = loopFamily(f.program, {l[0]}, {}, {});
  std::string why;
  auto request = proposeDomain(f.program, f.enumeration, 0, family, nullptr, ProposalLimits(), why);
  CHECK(request.has_value());
  if (!request)
    return;
  CheckedDomain good = checkDomainSymbolic(f.program, f.proof(), f.enumeration.groups[0], family, *request,
                                           f.options, z3Backend);
  CHECK(good.status() == S::Verified);
  // An upper bound of (n + 65535) + 1 is n at 16 bits; as a mathematical integer it is not.
  DomainRequest wrapped = *request;
  TermContext &terms = *f.program.terms;
  Term n = terms.param(0, Sort::bv(16));
  wrapped.order.levels[0].upper =
      terms.binary(TermOp::Add, terms.binary(TermOp::Add, n, terms.bv(APInt(16, 65535))), terms.bv(APInt(16, 1)));
  CHECK(checkDomainSymbolic(f.program, f.proof(), f.enumeration.groups[0], family, wrapped, f.options, z3Backend)
            .status() == S::Verified);
  // And n + 1 is one too many, which only the width can say.
  DomainRequest over = *request;
  over.order.levels[0].upper = terms.binary(TermOp::Add, n, terms.bv(APInt(16, 1)));
  CHECK(checkDomainFinite(f.program, f.enumeration, 0, family, over, f.options).status() == S::Refuted);
  CHECK(checkDomainSymbolic(f.program, f.proof(), f.enumeration.groups[0], family, over, f.options, z3Backend)
            .status() == S::Unknown);
  // A signed level over the same values: 250 to 255 are positive at 16 bits, and the
  // order holds either way.
  DomainRequest signedOrder = *request;
  signedOrder.order.levels[0].isSigned = true;
  CHECK(checkDomainFinite(f.program, f.enumeration, 0, family, signedOrder, f.options).status() == S::Verified);
}

/// Answers from Z3, except that scripts holding `pattern` get `answer`.
struct Selective : SmtBackend {
  std::string pattern;
  SmtAnswer answer;
  unsigned hits = 0;
  Selective(std::string pattern, SmtAnswer answer) : pattern(std::move(pattern)), answer(answer) {}
  SmtResponse check(const std::string &script, const SmtLimits &limits, bool model) override {
    if (script.find(pattern) != std::string::npos) {
      ++hits;
      SmtResponse response;
      response.answer = answer;
      response.detail = "from the test";
      return response;
    }
    return z3Backend.check(script, limits, model);
  }
  std::string version() const override { return "selective"; }
};

static void testSolverAndLimits() {
  Module m(catalogue());
  Fixture f = fixture(m, "holed", 1, 2);
  CoordinateFamily family = ijk(f.program);
  std::string why;
  auto request = propose(f, family, why);
  CHECK(request.has_value());
  if (!request)
    return;
  auto run = [&](SmtBackend &backend, const DomainOptions &options) {
    return checkDomainSymbolic(f.program, f.proof(), f.enumeration.groups[0], family, *request, options,
                               backend);
  };
  CHECK(run(z3Backend, f.options).status() == S::Verified);
  for (const char *id : {"; D1 ", "; D2 ", "; D3 ", "; prune "})
    for (SmtAnswer answer : {SmtAnswer::Unknown, SmtAnswer::Sat}) {
      Selective solver(id, answer);
      CheckedDomain out = run(solver, f.options);
      if (solver.hits == 0)
        printf("no %s query\n", id);
      CHECK(solver.hits > 0);
      CHECK(out.status() == S::Unknown && !out.exactOrder());
    }
  // Facts: an unknown or sat answer leaves the fact unproven, and the order stands.
  for (const char *id : {"; K1 ", "; C1 ", "; T1 ", "; A1 "}) {
    Selective solver(id, SmtAnswer::Unknown);
    DomainRequest shortKey = *request;
    shortKey.cellKey = {2};
    CheckedDomain out = checkDomainSymbolic(f.program, f.proof(), f.enumeration.groups[0], family, shortKey,
                                            f.options, solver);
    CHECK(out.status() == S::Verified);
    std::string name(id);
    if (name == "; K1 ")
      CHECK(out.cellKey().status == FactStatus::Unknown);
    if (name == "; C1 ")
      CHECK(out.outputInjective().status == FactStatus::Unknown);
    if (name == "; A1 ")
      CHECK(out.access()[0].status == FactStatus::Unknown);
  }
  Selective nothing("check-sat", SmtAnswer::Unknown);
  CHECK(run(nothing, f.options).status() == S::Unknown);
  // Limits.
  DomainOptions zero = f.options;
  zero.termination.solver.timeoutMilliseconds = 0;
  CHECK(run(z3Backend, zero).reason().find("limit of zero") != std::string::npos);
  DomainOptions few = f.options;
  few.limits.maxQueries = 3;
  CHECK(run(z3Backend, few).status() == S::Unknown);
  DomainOptions shallow = f.options;
  shallow.limits.maxUnfolding = 1;
  CHECK(run(z3Backend, shallow).status() == S::Unknown);
  DomainOptions pairs = f.options;
  pairs.limits.maxPairs = 0;
  CHECK(run(z3Backend, pairs).status() == S::Unknown);
  DomainOptions box = f.options;
  box.limits.maxBox = 2;
  CheckedDomain boxed = checkDomainFinite(f.program, f.enumeration, 0, family, *request, box);
  CHECK(boxed.status() == S::Unknown);
  // No certificate, or one that is broken.
  CHECK(checkDomainSymbolic(f.program, nullptr, f.enumeration.groups[0], family, *request, f.options,
                            z3Backend).status() == S::Unknown);
  if (f.certificate) {
    TerminationCertificate broken = *f.certificate;
    broken.levels.clear();
    CHECK(checkDomainSymbolic(f.program, &broken, f.enumeration.groups[0], family, *request, f.options,
                              z3Backend).reason().find("not accepted") != std::string::npos);
  }
  // A group that is not the whole of its roots.
  Fixture p = fixture(m, "piecewise", 1, 2);
  SiteGroup part = p.enumeration.groups[0];
  CHECK(part.sites.size() == 2);
  part.sites.pop_back();
  CoordinateFamily pf = ijk(p.program);
  auto pr = propose(p, pf, why);
  CHECK(pr.has_value());
  if (pr)
    CHECK(checkDomainSymbolic(p.program, p.proof(), part, pf, *pr, p.options, z3Backend).reason().find("not the sites") !=
          std::string::npos);
}

static void testBinding() {
  Module m(catalogue());
  Fixture f = fixture(m, "rect", 1, 2);
  CoordinateFamily family = ijk(f.program);
  std::string why;
  auto request = propose(f, family, why);
  CHECK(request.has_value());
  if (!request)
    return;
  CheckedDomain a = checkDomainFinite(f.program, f.enumeration, 0, family, *request, f.options);
  DomainOptions wider = f.options;
  wider.scope = range(f.program, 1, 3);
  DomainOptions fewer = f.options;
  fewer.authorized.erase(fewer.authorized.begin());
  DomainRequest other = *request;
  other.cellKey = {0, 1, 2};
  CHECK(!a.binding().empty());
  CHECK(a.binding() != checkDomainFinite(f.program, f.enumeration, 0, family, *request, wider).binding());
  CHECK(a.binding() != checkDomainFinite(f.program, f.enumeration, 0, family, *request, fewer).binding());
  CHECK(a.binding() != checkDomainFinite(f.program, f.enumeration, 0, family, other, f.options).binding());
  CHECK(a.binding().find(f.program.fingerprint) == 0);
  // No public way makes a verified claim.
  CHECK(CheckedDomain::unknown("x").status() == S::Unknown);
}

/// All 512 holes of a 3 x 3 box. Each program against its own guard and against each
/// guard one bit away: only its own may pass, on either route.
static std::string maskProgram(unsigned mask) {
  return fn("m", "  %mask = arith.constant " + std::to_string(mask) + " : index\n" +
                     nest(guarded("        %sh = arith.muli %j, %c3 : index\n"
                                  "        %pos = arith.addi %sh, %k : index\n"
                                  "        %bit = arith.shrui %mask, %pos : index\n"
                                  "        %one = arith.andi %bit, %c1 : index\n"
                                  "        %on = arith.cmpi eq, %one, %c1 : index\n")));
}

static Term maskGuard(TermContext &terms, unsigned mask) {
  Sort w = Sort::bv(16);
  Term j = levelVariable(terms, 1, w), k = levelVariable(terms, 2, w);
  Term pos = terms.binary(TermOp::Add, terms.binary(TermOp::Mul, j, terms.bv(APInt(16, 3))), k);
  Term bit = terms.binary(TermOp::And, terms.binary(TermOp::LShr, terms.bv(APInt(16, mask)), pos),
                          terms.bv(APInt(16, 1)));
  return terms.icmp(Predicate::Eq, bit, terms.bv(APInt(16, 1)));
}

static void testExhaustiveHoles() {
  bool full = getenv("DOMAIN_FULL") != nullptr;
  uint64_t programs = 0, checks = 0, verified = 0, refuted = 0, symbolicVerified = 0, falseClaims = 0;
  uint64_t symbolicRun = 0, symbolicRight = 0;
  for (unsigned mask = 0; mask < 512; ++mask) {
    Module m(maskProgram(mask));
    Fixture f = fixture(m, "m", 1, 2);
    if (f.enumeration.gate != GateStatus::Open)
      continue;
    ++programs;
    std::vector<unsigned> l = loops(f.program);
    CoordinateFamily family = loopFamily(f.program, {l[0]}, {l[1]}, {l[2]});
    TermContext &terms = *f.program.terms;
    DomainRequest request;
    for (unsigned m2 = 0; m2 < 3; ++m2) {
      GuardedLevel level;
      level.value = terms.state(l[m2], Sort::bv(16));
      level.lower = terms.bv(APInt(16, 0));
      level.upper = m2 == 0 ? terms.param(0, Sort::bv(16)) : terms.bv(APInt(16, 3));
      request.order.levels.push_back(level);
    }
    request.cellKey = {2};
    std::vector<unsigned> guards{mask};
    for (unsigned bit = 0; bit < 9; ++bit)
      guards.push_back(mask ^ (1u << bit));
    for (unsigned g : guards) {
      request.order.guard = maskGuard(terms, g);
      CheckedDomain finite = checkDomainFinite(f.program, f.enumeration, 0, family, request, f.options);
      ++checks;
      bool right = g == mask;
      // A program with no event at all has nothing to claim: Unknown is right there.
      bool empty = mask == 0;
      if (finite.status() == S::Verified) {
        ++verified;
        falseClaims += !right;
      } else if (finite.status() == S::Refuted) {
        ++refuted;
        falseClaims += right;
      } else if (!empty || right) {
        CHECK(empty);
      }
      // The symbolic route: in the full run for every guard; otherwise for the right
      // guard of one program in four and every guard of one in 32. The finite route
      // judges every request either way.
      if (full || (right && mask % 4 == 0) || mask % 32 == 0) {
        ++symbolicRun;
        CheckedDomain symbolic = checkDomainSymbolic(f.program, f.proof(), f.enumeration.groups[0], family,
                                                     request, f.options, z3Backend);
        if (symbolic.status() == S::Verified) {
          ++symbolicVerified;
          falseClaims += !right;
        }
        CHECK(symbolic.status() != S::Refuted);
        if (right && !empty && symbolic.status() != S::Verified)
          printf("mask %u: %s\n", mask, symbolic.reason().c_str());
        CHECK(!right || empty || symbolic.status() == S::Verified);
        symbolicRight += right && !empty;
      }
    }
  }
  printf("holes: %llu programs, %llu requests: %llu verified, %llu refuted; symbolic %llu run, %llu verified, "
         "%llu false\n",
         (unsigned long long)programs, (unsigned long long)checks, (unsigned long long)verified,
         (unsigned long long)refuted, (unsigned long long)symbolicRun, (unsigned long long)symbolicVerified,
         (unsigned long long)falseClaims);
  CHECK(falseClaims == 0);
  // Every right guard the symbolic route saw was verified; the mask 0 program has no event.
  CHECK(programs == 512 && symbolicVerified == symbolicRight && symbolicRight > 0);
  CHECK(verified == 511);
}

/// Every order of three loops, each either way, against every order and direction.
static void testExhaustiveOrders() {
  std::vector<std::array<int, 3>> permutations;
  std::array<int, 3> p{0, 1, 2};
  do
    permutations.push_back(p);
  while (std::next_permutation(p.begin(), p.end()));
  const char *names[] = {"i", "j", "k"};
  uint64_t checks = 0, verified = 0, falseClaims = 0, symbolicVerified = 0;
  for (const auto &perm : permutations)
    for (unsigned dirs = 0; dirs < 8; ++dirs) {
      // Loops in the order perm, and a level descends when its loop variable is
      // read as 2 - v.
      std::string body;
      std::string open, close;
      for (unsigned d = 0; d < 3; ++d) {
        std::string v = names[perm[d]];
        open += "  scf.for %r" + v + " = %c0 to %c3 step %c1 {\n";
        close += "  }\n";
      }
      for (unsigned d = 0; d < 3; ++d) {
        std::string v = names[d];
        body += (dirs >> d & 1) ? "    %" + v + " = arith.subi %c2, %r" + v + " : index\n"
                                : "    %" + v + " = arith.addi %r" + v + ", %c0 : index\n";
      }
      Module m(fn("o", open + body + mac("%c[%i, %j]", "%a[%i, %k]", "%b[%k, %j]", "    ") + close));
      Fixture f = fixture(m, "o", 0, 0);
      if (f.enumeration.gate != GateStatus::Open)
        continue;
      // The loop variables, in program order: their states, as levels.
      std::vector<unsigned> l = loops(f.program);
      CHECK(l.size() == 3);
      TermContext &terms = *f.program.terms;
      // The coordinates are i, j and k as the site reads them.
      CoordinateFamily family;
      const MacSite &site = f.program.sites[0];
      family.axes[AxisI] = {site.output->index[0]};
      family.axes[AxisJ] = {site.output->index[1]};
      family.axes[AxisK] = {site.a.index[1]};
      Term coordinate[3] = {site.output->index[0], site.output->index[1], site.a.index[1]};
      for (const auto &claim : permutations)
        for (unsigned claimDirs = 0; claimDirs < 8; ++claimDirs) {
          DomainRequest request;
          for (unsigned d = 0; d < 3; ++d) {
            GuardedLevel level;
            level.value = coordinate[claim[d]];
            level.lower = terms.bv(APInt(16, 0));
            level.upper = terms.bv(APInt(16, 3));
            level.descending = claimDirs >> claim[d] & 1;
            request.order.levels.push_back(level);
          }
          request.order.guard = terms.boolean(true);
          request.cellKey = {0, 1, 2};
          CheckedDomain finite = checkDomainFinite(f.program, f.enumeration, 0, family, request, f.options);
          ++checks;
          bool right = claim == perm && claimDirs == dirs;
          if (finite.status() == S::Verified) {
            ++verified;
            falseClaims += !right;
          }
          if (right) {
            CHECK(finite.status() == S::Verified);
            CheckedDomain symbolic = checkDomainSymbolic(f.program, f.proof(), f.enumeration.groups[0], family,
                                                         request, f.options, z3Backend);
            if (symbolic.status() != S::Verified)
              printf("order %d%d%d/%u: %s\n", perm[0], perm[1], perm[2], dirs, symbolic.reason().c_str());
            CHECK(symbolic.status() == S::Verified);
            symbolicVerified += symbolic.status() == S::Verified;
          } else {
            CHECK(finite.status() == S::Refuted);
          }
        }
    }
  printf("orders: %llu requests, %llu verified, %llu symbolic, %llu false\n", (unsigned long long)checks,
         (unsigned long long)verified, (unsigned long long)symbolicVerified, (unsigned long long)falseClaims);
  CHECK(falseClaims == 0 && verified == 48 && symbolicVerified == 48);
}

/// The independent review's reproducers, each with what it must give now.
static const char *reviewed = R"(func.func @two(%a: memref<8x8xi32>, %b: memref<8x8xi32>, %c: memref<8x8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c3 = arith.constant 3 : index
  scf.for %i = %c0 to %n step %c1 {
    scf.for %j = %c0 to %c3 step %c1 {
      scf.for %k = %c0 to %c3 step %c1 {
      %x = memref.load %a[%i, %k] : memref<8x8xi32>
      %y = memref.load %b[%k, %j] : memref<8x8xi32>
      %z = memref.load %c[%i, %j] : memref<8x8xi32>
      %m = arith.muli %x, %y : i32
      %s = arith.addi %z, %m : i32
      memref.store %s, %c[%i, %j] : memref<8x8xi32>
      %x2 = memref.load %a[%i, %k] : memref<8x8xi32>
      %y2 = memref.load %b[%k, %j] : memref<8x8xi32>
      %z2 = memref.load %c[%i, %j] : memref<8x8xi32>
      %m2 = arith.muli %x2, %y2 : i32
      %s2 = arith.addi %z2, %m2 : i32
      memref.store %s2, %c[%i, %j] : memref<8x8xi32>
      }
    }
  }
  return
}
func.func @flstop(%a: memref<8x8xf64>, %b: memref<8x8xf64>, %c: memref<8x8xf64>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  %c3 = arith.constant 3 : index
  %one = arith.constant 1.0 : f64
  %inf = arith.constant 0x7FF0000000000000 : f64
  scf.for %i = %c0 to %c2 step %c1 {
    scf.for %j = %c0 to %c3 step %c1 {
      %r = scf.for %k = %c0 to %n step %c1 iter_args(%acc = %one) -> (f64) {
      %q = arith.mulf %inf, %acc : f64
      %x = memref.load %a[%i, %k] : memref<8x8xf64>
      %y = memref.load %b[%k, %j] : memref<8x8xf64>
      %z = memref.load %c[%i, %j] : memref<8x8xf64>
      %m = arith.mulf %x, %y : f64
      %s = arith.addf %z, %m : f64
      memref.store %s, %c[%i, %j] : memref<8x8xf64>
      scf.yield %s : f64
      }
    }
  }
  return
}
func.func @stop(%a: memref<8x8xi32>, %b: memref<8x8xi32>, %c: memref<8x8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c3 = arith.constant 3 : index
  %one = arith.constant 1 : i32
  %h = arith.constant 100 : i32
  scf.for %i = %c0 to %n step %c1 {
    scf.for %j = %c0 to %c3 step %c1 {
      %r = scf.for %k = %c0 to %c3 step %c1 iter_args(%acc = %one) -> (i32) {
      %kk = arith.constant 23130 : i32
      %ax = arith.xori %acc, %kk : i32
      %d = arith.divsi %h, %ax : i32
      %x = memref.load %a[%i, %k] : memref<8x8xi32>
      %y = memref.load %b[%k, %j] : memref<8x8xi32>
      %z = memref.load %c[%i, %j] : memref<8x8xi32>
      %m = arith.muli %x, %y : i32
      %s = arith.addi %z, %m : i32
      memref.store %s, %c[%i, %j] : memref<8x8xi32>
      scf.yield %s : i32
      }
    }
  }
  return
}
func.func @wide_repeats(%a: memref<8x8xi32>, %b: memref<8x8xi32>, %c: memref<8x8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  %c3 = arith.constant 3 : index
  scf.for %t = %c0 to %c2 step %c1 {
    scf.for %u = %c0 to %c2 step %c1 {
      scf.for %i = %c0 to %n step %c1 {
        scf.for %j = %c0 to %c3 step %c1 {
          scf.for %k = %c0 to %c3 step %c1 {
            %x = memref.load %a[%i, %k] : memref<8x8xi32>
            %y = memref.load %b[%k, %j] : memref<8x8xi32>
            %z = memref.load %c[%i, %j] : memref<8x8xi32>
            %m = arith.muli %x, %y : i32
            %s = arith.addi %z, %m : i32
            memref.store %s, %c[%i, %j] : memref<8x8xi32>
          }
        }
      }
    }
  }
  return
}
)";

static DomainRequest boxRequest(const SemanticProgram &p, const std::vector<unsigned> &levels,
                                const std::vector<Term> &uppers) {
  DomainRequest request;
  TermContext &terms = *p.terms;
  for (unsigned m = 0; m < levels.size(); ++m) {
    GuardedLevel level;
    level.value = terms.state(levels[m], p.state[levels[m]].sort);
    level.lower = terms.bv(APInt(level.value->sort.width, 0));
    level.upper = uppers[m];
    request.order.levels.push_back(level);
  }
  request.order.guard = terms.boolean(true);
  request.cellKey = {(unsigned)levels.size() - 1};
  return request;
}

static void testReviewed() {
  Module m(reviewed);
  {
    // Two sites in one block: two events a step, which the walk takes for one.
    Fixture f = fixture(m, "two", 1, 2);
    CHECK(f.program.sites.size() == 2 && f.program.sites[0].location == f.program.sites[1].location);
    std::vector<unsigned> l = loops(f.program);
    CoordinateFamily family = loopFamily(f.program, {l[0]}, {l[1]}, {l[2]});
    TermContext &terms = *f.program.terms;
    DomainRequest request = boxRequest(f.program, l, {terms.param(0, Sort::bv(16)), terms.bv(APInt(16, 3)),
                                                      terms.bv(APInt(16, 3))});
    CheckedDomain symbolic = checkDomainSymbolic(f.program, f.proof(), f.enumeration.groups[0], family,
                                                 request, f.options, z3Backend);
    CHECK(symbolic.status() == S::Unknown);
    CHECK(symbolic.reason().find("share a block") != std::string::npos);
    CHECK(checkDomainFinite(f.program, f.enumeration, 0, family, request, f.options).status() == S::Refuted);
    // Even with no finite route, nothing over the scope.
    DomainOptions tiny = f.options;
    tiny.limits.maxBox = 2;
    ReductionVerdict v = checkReduction(f.program, f.proof(), f.enumeration, 0, family, nullptr, request, tiny,
                                        z3Backend);
    CHECK(v.tier() != T::Symbolic);
    // The integer fact rests on G4a's fold of the events alone, which holds here: two
    // contributions a step still commute in wrapping arithmetic.
  }
  {
    // A fact from points stays a fact about those points.
    Fixture f = fixture(m, "flstop", 1, 2);
    std::vector<unsigned> l = loops(f.program);
    CHECK(l.size() >= 3);
    CoordinateFamily family = loopFamily(f.program, {l[0]}, {l[1]}, {l[2]});
    std::string why;
    auto request = propose(f, family, why);
    if (request) {
      ReductionVerdict v = checkReduction(f.program, f.proof(), f.enumeration, 0, family, nullptr, *request,
                                          f.options, z3Backend);
      const Fact &floating = v.floatReorder();
      if (floating.status == FactStatus::Proven)
        CHECK(floating.tier != (int)T::Symbolic &&
              v.domainSymbolic().singleContribution().status != FactStatus::Proven);
      CHECK(v.trivialReorder().status != FactStatus::Proven || v.trivialReorder().tier != (int)T::Symbolic ||
            v.domainSymbolic().singleContribution().status == FactStatus::Proven);
    }
  }
  {
    // A partial sum carried in state decides a stop: no integer fact.
    Fixture f = fixture(m, "stop", 1, 2);
    std::vector<unsigned> l = loops(f.program);
    CoordinateFamily family = loopFamily(f.program, {l[0]}, {l[1]}, {l[2]});
    std::string why;
    auto request = propose(f, family, why);
    CHECK(request.has_value());
    if (request) {
      ReductionVerdict v = checkReduction(f.program, f.proof(), f.enumeration, 0, family, nullptr, *request,
                                          f.options, z3Backend);
      if (v.integerReorder().status == FactStatus::Proven)
        printf("stop: %s\n", v.integerReorder().detail.c_str());
      CHECK(v.integerReorder().status != FactStatus::Proven);
    }
  }
  {
    // Two repeat levels at 64 bits: a product that could pass 2^128 is no constant box.
    Fixture f = fixture(m, "wide_repeats", 1, 1, 64);
    std::vector<unsigned> l = loops(f.program);
    CHECK(l.size() == 5);
    CoordinateFamily family = loopFamily(f.program, {l[2]}, {l[3]}, {l[4]});
    TermContext &terms = *f.program.terms;
    Sort w = Sort::bv(64);
    DomainRequest request = boxRequest(f.program, l, {terms.bv(APInt(64, 2)), terms.bv(APInt(64, 2)),
                                                      terms.param(0, w), terms.bv(APInt(64, 3)), terms.bv(APInt(64, 3))});
    request.cellKey = {0, 1, 4};
    CheckedDomain finite = checkDomainFinite(f.program, f.enumeration, 0, family, request, f.options);
    CHECK(finite.status() == S::Verified);
    CHECK(finite.multiplicity() != DomainMultiplicity::ConstantBox);
    for (const DomainPointFacts &point : finite.points())
      CHECK(point.events == 0 || point.maxMultiplicity == 4);
  }
  {
    // A claim is about the family and the closed forms too.
    Module cat(catalogue());
    Fixture f = fixture(cat, "rect", 1, 2);
    std::vector<unsigned> l = loops(f.program);
    CoordinateFamily family = loopFamily(f.program, {l[0]}, {l[1]}, {l[2]});
    CoordinateFamily swapped = loopFamily(f.program, {l[1]}, {l[0]}, {l[2]});
    TermContext &terms = *f.program.terms;
    DomainRequest request = boxRequest(f.program, l, {terms.param(0, Sort::bv(16)), terms.bv(APInt(16, 3)),
                                                      terms.bv(APInt(16, 3))});
    DomainRequest other = request;
    other.access[0] = {levelVariable(terms, 1, Sort::bv(16)), levelVariable(terms, 0, Sort::bv(16))};
    std::string a = checkDomainFinite(f.program, f.enumeration, 0, family, request, f.options).binding();
    CHECK(a != checkDomainFinite(f.program, f.enumeration, 0, swapped, request, f.options).binding());
    CHECK(a != checkDomainFinite(f.program, f.enumeration, 0, family, other, f.options).binding());
    // An enumeration of n = 1 is no enumeration of 1..4, whatever scope it is handed.
    Fixture one = fixture(cat, "rect", 1, 1);
    DomainOptions wider = one.options;
    wider.scope = range(one.program, 1, 4);
    std::vector<unsigned> l1 = loops(one.program);
    CoordinateFamily f1 = loopFamily(one.program, {l1[0]}, {l1[1]}, {l1[2]});
    DomainRequest r1 = boxRequest(one.program, l1, {one.program.terms->param(0, Sort::bv(16)),
                                                    one.program.terms->bv(APInt(16, 3)),
                                                    one.program.terms->bv(APInt(16, 3))});
    CheckedDomain narrow = checkDomainFinite(one.program, one.enumeration, 0, f1, r1, wider);
    CHECK(narrow.status() == S::Verified && narrow.tier() == T::Point);
    CheckedDomain exact = checkDomainFinite(one.program, one.enumeration, 0, f1, r1, one.options);
    CHECK(exact.tier() == T::ScopeEnumerated);
    // The same hole in G4a's finite route, closed there too.
    ComputationOptions computation;
    computation.scope = wider.scope;
    computation.authorized = one.authorized;
    computation.termination.solver = testSolver();
    CheckedComputation g4 = checkComputationFinite(one.program, one.enumeration, 0, computation);
    CHECK(g4.status() == S::Verified && g4.tier() == T::Point);
    computation.scope = one.scope;
    CHECK(checkComputationFinite(one.program, one.enumeration, 0, computation).tier() == T::ScopeEnumerated);
    CHECK(!enumerationCoversScope(one.program, one.enumeration, wider.scope));
    CHECK(enumerationCoversScope(one.program, one.enumeration, one.scope));
  }
}

/// Programs for the second round of gaps that mutation testing showed.
static const char *gapsSource = R"(func.func @shifted(%a: memref<8x8xi32>, %b: memref<8x8xi32>, %c: memref<8x8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c3 = arith.constant 3 : index
  scf.for %i = %c0 to %n step %c1 {
    scf.for %k = %c0 to %c3 step %c1 {
      %kk = arith.subi %k, %c1 : index
      %e = arith.addi %kk, %c1 : index
      %x = memref.load %a[%i, %e] : memref<8x8xi32>
      %y = memref.load %b[%e, %i] : memref<8x8xi32>
      %z = memref.load %c[%i, %c0] : memref<8x8xi32>
      %m = arith.muli %x, %y : i32
      %s = arith.addi %z, %m : i32
      memref.store %s, %c[%i, %c0] : memref<8x8xi32>
    }
  }
  return
}
func.func @tainted(%a: memref<8x8xi32>, %b: memref<8x8xi32>, %c: memref<8x8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c3 = arith.constant 3 : index
  %w = memref.load %a[%c0, %c0] : memref<8x8xi32>
  %t = arith.index_cast %w : i32 to index
  cf.br ^go
^go:
  scf.for %i = %c0 to %n step %c1 {
    scf.for %k = %c0 to %c3 step %c1 {
      %u = arith.addi %t, %c0 : index
      %x = memref.load %a[%i, %k] : memref<8x8xi32>
      %y = memref.load %b[%k, %i] : memref<8x8xi32>
      %z = memref.load %c[%i, %c0] : memref<8x8xi32>
      %m = arith.muli %x, %y : i32
      %s = arith.addi %z, %m : i32
      memref.store %s, %c[%i, %c0] : memref<8x8xi32>
    }
  }
  return
}
func.func @entry_site(%a: memref<8x8xi32>, %b: memref<8x8xi32>, %c: memref<8x8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %x = memref.load %a[%c0, %c0] : memref<8x8xi32>
  %y = memref.load %b[%c0, %c0] : memref<8x8xi32>
  %z = memref.load %c[%c0, %c0] : memref<8x8xi32>
  %m = arith.muli %x, %y : i32
  %s = arith.addi %z, %m : i32
  memref.store %s, %c[%c0, %c0] : memref<8x8xi32>
  scf.for %i = %c0 to %n step %c1 {
    %q = memref.load %a[%i, %c0] : memref<8x8xi32>
    memref.store %q, %b[%i, %c1] : memref<8x8xi32>
  }
  return
}
func.func @memory_guard(%a: memref<8x8xi32>, %b: memref<8x8xi32>, %c: memref<8x8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c3 = arith.constant 3 : index
  %zero = arith.constant 0 : i32
  scf.for %i = %c0 to %n step %c1 {
    scf.for %k = %c0 to %c3 step %c1 {
      %g = memref.load %a[%i, %k] : memref<8x8xi32>
      %p = arith.cmpi sgt, %g, %zero : i32
      scf.if %p {
        %x = memref.load %a[%i, %k] : memref<8x8xi32>
        %y = memref.load %b[%k, %i] : memref<8x8xi32>
        %z = memref.load %c[%i, %c0] : memref<8x8xi32>
        %m = arith.muli %x, %y : i32
        %s = arith.addi %z, %m : i32
        memref.store %s, %c[%i, %c0] : memref<8x8xi32>
      }
    }
  }
  return
}
func.func @partly_repeated(%a: memref<8x8xi32>, %b: memref<8x8xi32>, %c: memref<8x8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  %c3 = arith.constant 3 : index
  scf.for %t = %c0 to %c2 step %c1 {
    scf.for %i = %c0 to %n step %c1 {
      scf.for %k = %c0 to %c3 step %c1 {
        %t0 = arith.cmpi eq, %t, %c0 : index
        %k0 = arith.cmpi eq, %k, %c0 : index
        %on = arith.ori %t0, %k0 : i1
        scf.if %on {
          %x = memref.load %a[%i, %k] : memref<8x8xi32>
          %y = memref.load %b[%k, %i] : memref<8x8xi32>
          %z = memref.load %c[%i, %c0] : memref<8x8xi32>
          %m = arith.muli %x, %y : i32
          %s = arith.addi %z, %m : i32
          memref.store %s, %c[%i, %c0] : memref<8x8xi32>
        }
      }
    }
  }
  return
}
func.func @growing_repeat(%a: memref<8x8xi32>, %b: memref<8x8xi32>, %c: memref<8x8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c3 = arith.constant 3 : index
  scf.for %i = %c0 to %n step %c1 {
    scf.for %k = %c0 to %c3 step %c1 {
      %top = arith.addi %k, %c1 : index
      scf.for %t = %c0 to %top step %c1 {
        %x = memref.load %a[%i, %k] : memref<8x8xi32>
        %y = memref.load %b[%k, %i] : memref<8x8xi32>
        %z = memref.load %c[%i, %c0] : memref<8x8xi32>
        %m = arith.muli %x, %y : i32
        %s = arith.addi %z, %m : i32
        memref.store %s, %c[%i, %c0] : memref<8x8xi32>
      }
    }
  }
  return
}
func.func @stop_on_output(%a: memref<8x8xi32>, %b: memref<8x8xi32>, %c: memref<8x8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c3 = arith.constant 3 : index
  %one = arith.constant 1 : i32
  scf.for %i = %c0 to %n step %c1 {
    scf.for %k = %c0 to %c3 step %c1 {
      %w = memref.load %c[%c0, %c1] : memref<8x8xi32>
      %q = arith.divui %one, %w : i32
      %x = memref.load %a[%i, %k] : memref<8x8xi32>
      %y = memref.load %b[%k, %i] : memref<8x8xi32>
      %z = memref.load %c[%i, %c0] : memref<8x8xi32>
      %m = arith.muli %x, %y : i32
      %s = arith.addi %z, %m : i32
      memref.store %s, %c[%i, %c0] : memref<8x8xi32>
    }
  }
  return
}
func.func @stop_on_accumulator(%a: memref<8x8xi32>, %b: memref<8x8xi32>, %c: memref<8x8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c3 = arith.constant 3 : index
  %one = arith.constant 1 : i32
  scf.for %i = %c0 to %n step %c1 {
    scf.for %k = %c0 to %c3 step %c1 {
      %x = memref.load %a[%i, %k] : memref<8x8xi32>
      %y = memref.load %b[%k, %i] : memref<8x8xi32>
      %z = memref.load %c[%i, %c0] : memref<8x8xi32>
      %odd = arith.ori %z, %one : i32
      %q = arith.divui %one, %odd : i32
      %m = arith.muli %x, %y : i32
      %s = arith.addi %z, %m : i32
      memref.store %s, %c[%i, %c0] : memref<8x8xi32>
    }
  }
  return
}
func.func @mixed_flags(%a: memref<8x8xi32>, %b: memref<8x8xi32>, %c: memref<8x8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c3 = arith.constant 3 : index
  scf.for %i = %c0 to %n step %c1 {
    scf.for %k = %c0 to %c3 step %c1 {
      %on = arith.cmpi eq, %k, %c0 : index
      scf.if %on {
        %x = memref.load %a[%i, %k] : memref<8x8xi32>
        %y = memref.load %b[%k, %i] : memref<8x8xi32>
        %z = memref.load %c[%i, %c0] : memref<8x8xi32>
        %m = arith.muli %x, %y : i32
        %s = arith.addi %z, %m overflow<nsw> : i32
        memref.store %s, %c[%i, %c0] : memref<8x8xi32>
      } else {
        %x2 = memref.load %a[%i, %k] : memref<8x8xi32>
        %y2 = memref.load %b[%k, %i] : memref<8x8xi32>
        %z2 = memref.load %c[%i, %c0] : memref<8x8xi32>
        %m2 = arith.muli %x2, %y2 : i32
        %s2 = arith.addi %z2, %m2 : i32
        memref.store %s2, %c[%i, %c0] : memref<8x8xi32>
      }
    }
  }
  return
}
func.func @open_at_two(%a: memref<8x8xi32>, %b: memref<8x8xi32>, %c: memref<8x8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  %c3 = arith.constant 3 : index
  %one = arith.constant 1 : i32
  scf.for %i = %c0 to %n step %c1 {
    scf.for %k = %c0 to %c3 step %c1 {
      %x = memref.load %a[%i, %k] : memref<8x8xi32>
      %y = memref.load %b[%k, %i] : memref<8x8xi32>
      %z = memref.load %c[%i, %c0] : memref<8x8xi32>
      %m = arith.muli %x, %y : i32
      %s = arith.addi %z, %m : i32
      memref.store %s, %c[%i, %c0] : memref<8x8xi32>
    }
  }
  %wide = arith.cmpi uge, %n, %c2 : index
  scf.if %wide {
    %q = memref.load %c[%c0, %c0] : memref<8x8xi32>
    %r = arith.muli %q, %one : i32
    memref.store %r, %c[%c0, %c0] : memref<8x8xi32>
  }
  return
}
)";

/// A two-level box over i and a second level, with optional guard.
static DomainRequest twoLevels(const SemanticProgram &p, Term second, Term lower, Term upper,
                               bool isSigned = false) {
  DomainRequest request;
  TermContext &terms = *p.terms;
  std::vector<unsigned> l = loops(p);
  GuardedLevel i;
  i.value = terms.state(l[0], Sort::bv(16));
  i.lower = terms.bv(APInt(16, 0));
  i.upper = terms.param(0, Sort::bv(16));
  GuardedLevel k;
  k.value = second;
  k.lower = lower;
  k.upper = upper;
  k.isSigned = isSigned;
  request.order.levels = {i, k};
  request.order.guard = terms.boolean(true);
  request.cellKey = {1};
  return request;
}

static void testGapsG5() {
  Module m(gapsSource);
  auto both = [&](Fixture &f, const CoordinateFamily &family, const DomainRequest &request) {
    return std::make_pair(checkDomainFinite(f.program, f.enumeration, 0, family, request, f.options),
                          checkDomainSymbolic(f.program, f.proof(), f.enumeration.groups[0], family, request,
                                              f.options, z3Backend));
  };
  auto describe = [](const char *what, const CheckedDomain &finite, const CheckedDomain &symbolic) {
    printf("%s: finite %s %s (%s); symbolic %s (%s)\n", what, stringifyComputationStatus(finite.status()),
           stringifyComputationTier(finite.tier()), finite.reason().c_str(),
           stringifyComputationStatus(symbolic.status()), symbolic.reason().c_str());
  };
  Fixture f = fixture(m, "shifted", 1, 2);
  std::vector<unsigned> l = loops(f.program);
  TermContext &terms = *f.program.terms;
  Sort w = Sort::bv(16);
  Term kk = f.program.sites[0].a.index[1]; // (k - 1) + 1, as the site reads it: k.
  // The level k - 1 runs -1, 0, 1: in order only when compared signed.
  Term km1 = terms.binary(TermOp::Sub, terms.state(l[1], w), terms.bv(APInt(16, 1)));
  CoordinateFamily family;
  family.axes[AxisI] = {terms.state(l[0], w)};
  family.axes[AxisK] = {km1};
  {
    DomainRequest signedK = twoLevels(f.program, km1, terms.bv(APInt(16, (uint64_t)-1, true)),
                                      terms.bv(APInt(16, 2)), true);
    auto [finite, symbolic] = both(f, family, signedK);
    if (finite.status() != S::Verified || symbolic.status() != S::Verified)
      describe("signed level", finite, symbolic);
    CHECK(finite.status() == S::Verified && symbolic.status() == S::Verified);
    // The same values compared unsigned are out of order, and below their lower bound.
    DomainRequest unsignedK = signedK;
    unsignedK.order.levels[1].isSigned = false;
    auto [u1, u2] = both(f, family, unsignedK);
    CHECK(u1.status() == S::Refuted && u2.status() != S::Verified);
  }
  (void)kk;
  // The family's k as a level, and one box per test below.
  CoordinateFamily plain;
  plain.axes[AxisI] = {terms.state(l[0], w)};
  plain.axes[AxisK] = {terms.state(l[1], w)};
  DomainRequest box = twoLevels(f.program, terms.state(l[1], w), terms.bv(APInt(16, 0)), terms.bv(APInt(16, 3)));
  CHECK(both(f, plain, box).second.status() == S::Verified);
  {
    // As many points as events, and each a different one: k in [1, 4) for k in [0, 3).
    DomainRequest moved = box;
    moved.order.levels[1].lower = terms.bv(APInt(16, 1));
    moved.order.levels[1].upper = terms.bv(APInt(16, 4));
    auto [finite, symbolic] = both(f, plain, moved);
    CHECK(finite.status() == S::Refuted && finite.reason().find("is not the set's point") != std::string::npos);
    CHECK(symbolic.status() != S::Verified);
  }
  {
    // At n = 0 there is no event, and this set is not empty there: only D1's exit says so.
    Fixture zero = fixture(m, "shifted", 0, 2);
    DomainRequest at0 = box;
    Term n = terms.param(0, w);
    at0.order.levels[0].upper =
        terms.ite(terms.icmp(Predicate::Eq, n, terms.bv(APInt(16, 0))), terms.bv(APInt(16, 1)), n);
    auto [finite, symbolic] = both(zero, plain, at0);
    if (symbolic.status() == S::Verified)
      describe("set at n = 0", finite, symbolic);
    CHECK(symbolic.status() != S::Verified);
    CHECK(finite.status() == S::Refuted);
  }
  {
    // One point before the first event, signed: only D1's "nothing earlier" says so.
    DomainRequest early = box;
    early.order.levels[0].isSigned = true;
    early.order.levels[0].lower = terms.bv(APInt(16, (uint64_t)-1, true));
    Term i = levelVariable(terms, 0, w), k = levelVariable(terms, 1, w);
    Term minusOne = terms.icmp(Predicate::Eq, i, terms.bv(APInt(16, (uint64_t)-1, true)));
    early.order.guard = terms.logicalOr(terms.logicalNot(minusOne),
                                        terms.icmp(Predicate::Eq, k, terms.bv(APInt(16, 0))));
    auto [finite, symbolic] = both(f, plain, early);
    if (symbolic.status() == S::Verified)
      describe("a point before the first event", finite, symbolic);
    CHECK(symbolic.status() != S::Verified);
    CHECK(finite.status() == S::Refuted);
  }
  {
    // A bound that reads its own level is refused.
    DomainRequest self = box;
    self.order.levels[1].upper = levelVariable(terms, 1, w);
    CHECK(both(f, plain, self).first.reason().find("refused") != std::string::npos);
    // A closed form over state, not levels, is refused: it would equal the index trivially.
    DomainRequest overState = box;
    overState.access[0] = {f.program.sites[0].output->index.begin(), f.program.sites[0].output->index.end()};
    auto [sf, ss] = both(f, plain, overState);
    CHECK(sf.reason().find("refused") != std::string::npos && ss.reason().find("refused") != std::string::npos);
    // A family coordinate that no level holds is refused, with a key that is well formed.
    DomainRequest missing = box;
    missing.order.levels.pop_back();
    missing.cellKey = {0};
    auto [finite, symbolic] = both(f, plain, missing);
    CHECK(finite.status() == S::Unknown && finite.reason().find("no level") != std::string::npos);
    CHECK(symbolic.status() == S::Unknown);
  }
  {
    // n = 9 leaves an 8-wide root, so its run stops: that point is open.
    Fixture far = fixture(m, "shifted", 1, 9);
    CheckedDomain finite = checkDomainFinite(far.program, far.enumeration, 0, plain, box, far.options);
    CHECK(finite.status() == S::Verified && finite.tier() == T::Point);
    // A box too large at one point leaves that point open too.
    DomainOptions small = f.options;
    small.limits.maxBox = 4;
    CheckedDomain boxed = checkDomainFinite(f.program, f.enumeration, 0, plain, box, small);
    CHECK(boxed.status() == S::Verified && boxed.tier() == T::Point);
    // An enumeration of another scope is no enumeration of this one.
    DomainOptions shifted = f.options;
    shifted.scope = range(f.program, 2, 3);
    CHECK(checkDomainFinite(f.program, f.enumeration, 0, plain, box, shifted).tier() != T::ScopeEnumerated);
  }
  {
    // A level with no value at the events: without live state, t has none, since no
    // address reads it.
    Fixture p = fixture(m, "partly_repeated", 1, 2);
    std::vector<unsigned> pl = loops(p.program);
    EnumerationOptions plainEnumeration;
    plainEnumeration.scope = p.scope;
    plainEnumeration.authorized = p.authorized;
    plainEnumeration.termination.solver = testSolver();
    DomainEnumeration narrow = enumerateDomain(p.program, p.proof(), plainEnumeration, z3Backend);
    CHECK(narrow.gate == GateStatus::Open);
    CoordinateFamily pf;
    pf.axes[AxisI] = {p.program.terms->state(pl[1], w)};
    pf.axes[AxisK] = {p.program.terms->state(pl[2], w)};
    std::string why;
    auto r = proposeDomain(p.program, p.enumeration, 0, pf, nullptr, ProposalLimits(), why);
    CHECK(r.has_value());
    if (r) {
      CHECK(checkDomainFinite(p.program, p.enumeration, 0, pf, *r, p.options).status() == S::Verified);
      CheckedDomain finite = checkDomainFinite(p.program, narrow, 0, pf, *r, p.options);
      CHECK(finite.status() == S::Unknown && finite.reason().find("live state") != std::string::npos);
    }
  }
  {
    // A level from memory is refused: its value differs from one memory to the next.
    Fixture t = fixture(m, "tainted", 1, 2);
    std::vector<unsigned> tl = loops(t.program);
    unsigned taint = 0;
    for (const StateVar &s : t.program.state)
      if (s.kind == StateVar::CrossBlockResult && s.sort == Sort::bv(16) &&
          !memoryIndependentState(t.program)[s.id])
        taint = s.id;
    CHECK(taint != 0);
    CoordinateFamily tf;
    tf.axes[AxisI] = {t.program.terms->state(tl[0], w)};
    tf.axes[AxisK] = {t.program.terms->state(tl[1], w)};
    DomainRequest r = twoLevels(t.program, t.program.terms->state(tl[1], w), t.program.terms->bv(APInt(16, 0)),
                                t.program.terms->bv(APInt(16, 3)));
    GuardedLevel extra;
    extra.value = t.program.terms->state(taint, w);
    extra.lower = t.program.terms->bv(APInt(16, 0));
    extra.upper = t.program.terms->bv(APInt(16, 1));
    r.order.levels.push_back(extra);
    r.cellKey = {1};
    CheckedDomain finite = checkDomainFinite(t.program, t.enumeration, 0, tf, r, t.options);
    CHECK(finite.status() == S::Unknown && finite.reason().find("depends on memory") != std::string::npos);
  }
  {
    // A site in the entry block: the walk does not start before it.
    Fixture e = fixture(m, "entry_site", 1, 2);
    std::vector<unsigned> el = loops(e.program);
    CoordinateFamily ef;
    ef.axes[AxisI] = {e.program.terms->state(el[0], w)};
    DomainRequest r;
    GuardedLevel only;
    only.value = e.program.terms->state(el[0], w);
    only.lower = e.program.terms->bv(APInt(16, 0));
    only.upper = e.program.terms->bv(APInt(16, 0));
    r.order.levels = {only};
    r.order.guard = e.program.terms->boolean(true);
    r.cellKey = {0};
    CheckedDomain symbolic = checkDomainSymbolic(e.program, e.proof(), e.enumeration.groups[0], ef, r,
                                                 e.options, z3Backend);
    CHECK(symbolic.status() == S::Unknown && symbolic.reason().find("entry block") != std::string::npos);
  }
  {
    // A guard from memory: the trace is no function of the parameters.
    Fixture g = fixture(m, "memory_guard", 1, 2);
    std::vector<unsigned> gl = loops(g.program);
    CoordinateFamily gf;
    gf.axes[AxisI] = {g.program.terms->state(gl[0], w)};
    gf.axes[AxisK] = {g.program.terms->state(gl[1], w)};
    DomainRequest r = twoLevels(g.program, g.program.terms->state(gl[1], w), g.program.terms->bv(APInt(16, 0)),
                                g.program.terms->bv(APInt(16, 3)));
    SiteGroup group;
    group.output = g.program.sites[0].output->root;
    group.first = g.program.sites[0].a.root;
    group.second = g.program.sites[0].b.root;
    group.sites = {0};
    CheckedDomain symbolic = checkDomainSymbolic(g.program, g.proof(), group, gf, r, g.options, z3Backend);
    CHECK(symbolic.status() == S::Unknown &&
          symbolic.reason().find("depends on memory contents") != std::string::npos);
    CHECK(checkDomainFinite(g.program, g.enumeration, 0, gf, r, g.options).reason().find("gate is closed") !=
          std::string::npos);
  }
  {
    // Multiplicity that depends on k: no constant box, whichever way the repeat is read.
    for (const char *name : {"partly_repeated", "growing_repeat"}) {
      Fixture p = fixture(m, name, 1, 2);
      std::vector<unsigned> pl = loops(p.program);
      CHECK(pl.size() == 3);
      bool outer = std::string(name) == "partly_repeated";
      unsigned ti = outer ? pl[0] : pl[2], ii = outer ? pl[1] : pl[0], ki = outer ? pl[2] : pl[1];
      CoordinateFamily pf;
      pf.axes[AxisI] = {p.program.terms->state(ii, w)};
      pf.axes[AxisK] = {p.program.terms->state(ki, w)};
      std::string why;
      auto r = proposeDomain(p.program, p.enumeration, 0, pf, nullptr, ProposalLimits(), why);
      if (!r)
        printf("%s: %s\n", name, why.c_str());
      CHECK(r.has_value());
      if (!r)
        continue;
      (void)ti;
      auto [finite, symbolic] = both(p, pf, *r);
      if (finite.status() != S::Verified || finite.multiplicity() == DomainMultiplicity::ConstantBox ||
          symbolic.multiplicity() == DomainMultiplicity::ConstantBox)
        describe(name, finite, symbolic);
      CHECK(finite.status() == S::Verified);
      CHECK(finite.multiplicity() != DomainMultiplicity::ConstantBox);
      CHECK(symbolic.multiplicity() != DomainMultiplicity::ConstantBox);
      // The guard of partly_repeated reads t, so no box; growing_repeat's t is a box
      // over k, which G5b gives a closed form (testMultiplicity).
      if (outer) {
        CHECK(finite.multiplicity() == DomainMultiplicity::Counted);
        CHECK(symbolic.multiplicity() == DomainMultiplicity::Unknown);
      } else {
        CHECK(finite.multiplicity() == DomainMultiplicity::LogicalBox);
        CHECK(symbolic.multiplicity() == DomainMultiplicity::LogicalBox);
      }
    }
  }
  {
    // A repeat level whose bound reads another level: μ varies, whatever the guard.
    Fixture g = fixture(m, "growing_repeat", 1, 2);
    std::vector<unsigned> gl = loops(g.program);
    CHECK(gl.size() == 3);
    TermContext &gt = *g.program.terms;
    CoordinateFamily gf;
    gf.axes[AxisI] = {gt.state(gl[0], w)};
    gf.axes[AxisK] = {gt.state(gl[1], w)};
    DomainRequest r = twoLevels(g.program, gt.state(gl[1], w), gt.bv(APInt(16, 0)), gt.bv(APInt(16, 3)));
    GuardedLevel repeat;
    repeat.value = gt.state(gl[2], w);
    repeat.lower = gt.bv(APInt(16, 0));
    repeat.upper = gt.binary(TermOp::Add, levelVariable(gt, 1, w), gt.bv(APInt(16, 1)));
    r.order.levels.push_back(repeat);
    r.cellKey = {1, 2};
    auto [finite, symbolic] = both(g, gf, r);
    if (finite.status() != S::Verified)
      describe("growing repeat", finite, symbolic);
    // G5b: that bound reads k, a logical level, so the repeat is a box over the
    // logical point and μ(i, k) = k + 1 has a closed form.
    CHECK(finite.status() == S::Verified && finite.multiplicity() == DomainMultiplicity::LogicalBox);
    CHECK(symbolic.multiplicity() == DomainMultiplicity::LogicalBox);
  }
  {
    // The binding tells requests apart by their levels too.
    DomainRequest other = box;
    other.order.levels[1].upper = terms.bv(APInt(16, 4));
    CHECK(checkDomainFinite(f.program, f.enumeration, 0, plain, box, f.options).binding() !=
          checkDomainFinite(f.program, f.enumeration, 0, plain, other, f.options).binding());
  }
  {
    // Theorem 13 needs G4a. Without the names of separation, there is none.
    DomainOptions unnamed = f.options;
    unnamed.authorized.clear();
    ReductionVerdict v = checkReduction(f.program, f.proof(), f.enumeration, 0, plain, nullptr, box, unnamed,
                                        z3Backend);
    CHECK(v.domainSymbolic().status() == S::Verified);
    CHECK(v.status() == S::Unknown);
    CHECK(v.integerReorder().status != FactStatus::Proven);
    // With them, and with no coordinate order, the logical form has no symbolic tier.
    ReductionVerdict named = checkReduction(f.program, f.proof(), f.enumeration, 0, plain, nullptr, box,
                                            f.options, z3Backend);
    CHECK(named.status() == S::Verified && named.tier() == T::Symbolic);
    CHECK(named.logicalTier() != T::Symbolic);
  }
  {
    // A lying solver says unsat to everything. A concrete refutation still wins.
    Selective liar("check-sat", SmtAnswer::Unsat);
    DomainRequest moved = box;
    moved.order.levels[1].lower = terms.bv(APInt(16, 1));
    moved.order.levels[1].upper = terms.bv(APInt(16, 4));
    ReductionVerdict v = checkReduction(f.program, f.proof(), f.enumeration, 0, plain, nullptr, moved, f.options,
                                        liar);
    CHECK(v.domainFinite().status() == S::Refuted);
    CHECK(v.status() == S::Refuted);
  }
  {
    // The computation open at one point: Theorem 13 at the other only.
    Fixture o = fixture(m, "open_at_two", 1, 2);
    std::vector<unsigned> ol = loops(o.program);
    CoordinateFamily of;
    of.axes[AxisI] = {o.program.terms->state(ol[0], w)};
    of.axes[AxisK] = {o.program.terms->state(ol[1], w)};
    DomainRequest r = twoLevels(o.program, o.program.terms->state(ol[1], w), o.program.terms->bv(APInt(16, 0)),
                                o.program.terms->bv(APInt(16, 3)));
    ReductionVerdict v = checkReduction(o.program, o.proof(), o.enumeration, 0, of, nullptr, r, o.options, z3Backend);
    CHECK(v.computationFinite().tier() == T::Point);
    CHECK(v.status() == S::Verified && v.tier() == T::Point && v.points().size() == 1);
  }
  {
    // A stop that loads the output, and sites whose flags differ: no integer fact.
    Fixture s = fixture(m, "stop_on_output", 1, 2);
    std::vector<unsigned> sl = loops(s.program);
    CoordinateFamily sf;
    sf.axes[AxisI] = {s.program.terms->state(sl[0], w)};
    sf.axes[AxisK] = {s.program.terms->state(sl[1], w)};
    DomainRequest r = twoLevels(s.program, s.program.terms->state(sl[1], w), s.program.terms->bv(APInt(16, 0)),
                                s.program.terms->bv(APInt(16, 3)));
    // The reference fill makes the divisor 0, so give the output a start of 1 by the enumeration.
    ReductionVerdict v = checkReduction(s.program, s.proof(), s.enumeration, 0, sf, nullptr, r, s.options, z3Backend);
    CHECK(v.integerReorder().status != FactStatus::Proven);
    Fixture x = fixture(m, "mixed_flags", 1, 2);
    std::vector<unsigned> xl = loops(x.program);
    CoordinateFamily xf;
    xf.axes[AxisI] = {x.program.terms->state(xl[0], w)};
    xf.axes[AxisK] = {x.program.terms->state(xl[1], w)};
    DomainRequest xr = twoLevels(x.program, x.program.terms->state(xl[1], w), x.program.terms->bv(APInt(16, 0)),
                                 x.program.terms->bv(APInt(16, 3)));
    ReductionVerdict mixed = checkReduction(x.program, x.proof(), x.enumeration, 0, xf, nullptr, xr, x.options,
                                            z3Backend);
    CHECK(mixed.integerReorder().status == FactStatus::Unknown);
    // A stop that reads the group's own accumulator load: no other effect observes it,
    // so only the check of the stops' loads refuses the fact.
    Fixture acc = fixture(m, "stop_on_accumulator", 1, 2);
    std::vector<unsigned> al = loops(acc.program);
    CoordinateFamily af;
    af.axes[AxisI] = {acc.program.terms->state(al[0], w)};
    af.axes[AxisK] = {acc.program.terms->state(al[1], w)};
    DomainRequest ar = twoLevels(acc.program, acc.program.terms->state(al[1], w),
                                 acc.program.terms->bv(APInt(16, 0)), acc.program.terms->bv(APInt(16, 3)));
    ReductionVerdict onAcc = checkReduction(acc.program, acc.proof(), acc.enumeration, 0, af, nullptr, ar,
                                            acc.options, z3Backend);
    CHECK(onAcc.computationSymbolic().status() == S::Verified);
    CHECK(onAcc.computationSymbolic().frame().empty());
    CHECK(onAcc.integerReorder().status != FactStatus::Proven);
    // Evidence for "integer flags ignored": an nsw add stores a value whose poison is a stop
    // that reads the output, so the stop check already refuses it.
    bool stopReadsOutput = false;
    for (const Transition &t : x.program.transitions)
      for (const SideCondition &condition : t.sideConditions)
        stopReadsOutput = stopReadsOutput || (condition.reason == "poison reaches memory" &&
                                              containsOp(condition.violated, TermOp::Load));
    CHECK(stopReadsOutput);
  }
  {
    // A run that stops is no witness, whatever its memory.
    Module cat(catalogue());
    Fixture fl = fixture(cat, "floats", 1, 1);
    EntryState entry;
    entry.parameters = {APInt(16, 1)};
    for (unsigned r = 0; r < 3; ++r) {
      RootData data;
      data.shape = {8, 8};
      for (unsigned c = 0; c < 64; ++c)
        data.cells.emplace_back(APInt(64, 0x7ff8000000000000ull)); // NaN.
      entry.memory.push_back(std::move(data));
    }
    std::string detail;
    CHECK(!reversalDiffers(fl.program, fl.enumeration.groups[0], entry, {}, detail));
  }
}

/// G5b, first milestone: multiplicity as a box over the logical levels (Theorem 14),
/// and what must stay Counted.
static const char *multiplicitySource = R"(func.func @signed_repeat(%a: memref<8x8xi32>, %b: memref<8x8xi32>, %c: memref<8x8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c3 = arith.constant 3 : index
  %m1 = arith.constant -1 : index
  scf.for %i = %c0 to %n step %c1 {
    scf.for %k = %c0 to %c3 step %c1 {
      %top = arith.addi %k, %c1 : index
      scf.for %t = %m1 to %top step %c1 {
        %x = memref.load %a[%i, %k] : memref<8x8xi32>
        %y = memref.load %b[%k, %i] : memref<8x8xi32>
        %z = memref.load %c[%i, %c0] : memref<8x8xi32>
        %m = arith.muli %x, %y : i32
        %s = arith.addi %z, %m : i32
        memref.store %s, %c[%i, %c0] : memref<8x8xi32>
      }
    }
  }
  return
}
func.func @nested_repeats(%a: memref<8x8xi32>, %b: memref<8x8xi32>, %c: memref<8x8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  %c3 = arith.constant 3 : index
  scf.for %i = %c0 to %n step %c1 {
    scf.for %k = %c0 to %c3 step %c1 {
      scf.for %t = %c0 to %c2 step %c1 {
        %top = arith.addi %t, %c1 : index
        scf.for %u = %c0 to %top step %c1 {
          %x = memref.load %a[%i, %k] : memref<8x8xi32>
          %y = memref.load %b[%k, %i] : memref<8x8xi32>
          %z = memref.load %c[%i, %c0] : memref<8x8xi32>
          %m = arith.muli %x, %y : i32
          %s = arith.addi %z, %m : i32
          memref.store %s, %c[%i, %c0] : memref<8x8xi32>
        }
      }
    }
  }
  return
}
func.func @two_repeats(%a: memref<8x8xi32>, %b: memref<8x8xi32>, %c: memref<8x8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  %c3 = arith.constant 3 : index
  scf.for %i = %c0 to %n step %c1 {
    scf.for %k = %c0 to %c3 step %c1 {
      %top = arith.addi %k, %c1 : index
      scf.for %t = %c0 to %top step %c1 {
        scf.for %u = %c0 to %c2 step %c1 {
          %x = memref.load %a[%i, %k] : memref<8x8xi32>
          %y = memref.load %b[%k, %i] : memref<8x8xi32>
          %z = memref.load %c[%i, %c0] : memref<8x8xi32>
          %m = arith.muli %x, %y : i32
          %s = arith.addi %z, %m : i32
          memref.store %s, %c[%i, %c0] : memref<8x8xi32>
        }
      }
    }
  }
  return
}
func.func @float_repeat(%a: memref<8x8xf64>, %b: memref<8x8xf64>, %c: memref<8x8xf64>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c3 = arith.constant 3 : index
  scf.for %i = %c0 to %n step %c1 {
    scf.for %k = %c0 to %c3 step %c1 {
      %top = arith.addi %k, %c1 : index
      scf.for %t = %c0 to %top step %c1 {
        %x = memref.load %a[%i, %k] : memref<8x8xf64>
        %y = memref.load %b[%k, %i] : memref<8x8xf64>
        %z = memref.load %c[%i, %c0] : memref<8x8xf64>
        %m = arith.mulf %x, %y : f64
        %s = arith.addf %z, %m : f64
        memref.store %s, %c[%i, %c0] : memref<8x8xf64>
      }
    }
  }
  return
}
)";

static void describeRequest(const char *what, const DomainRequest &r) {
  printf("%s:", what);
  for (const GuardedLevel &level : r.order.levels)
    printf(" [%s in %s..%s%s%s]", formatTerm(level.value, 4).c_str(), formatTerm(level.lower, 4).c_str(),
           formatTerm(level.upper, 4).c_str(), level.descending ? " desc" : "", level.isSigned ? " signed" : "");
  printf(" guard %s\n", formatTerm(r.order.guard, 6).c_str());
}

static const char *multiplicitySource2 = R"(func.func @descending_repeat(%a: memref<8x8xi32>, %b: memref<8x8xi32>, %c: memref<8x8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  %c3 = arith.constant 3 : index
  scf.for %i = %c0 to %n step %c1 {
    scf.for %k = %c0 to %c3 step %c1 {
      %top = arith.addi %k, %c1 : index
      scf.for %t = %c0 to %top step %c1 {
        %tt = arith.subi %c2, %t : index
        %x = memref.load %a[%i, %k] : memref<8x8xi32>
        %y = memref.load %b[%k, %tt] : memref<8x8xi32>
        %z = memref.load %c[%i, %c0] : memref<8x8xi32>
        %m = arith.muli %x, %y : i32
        %s = arith.addi %z, %m : i32
        memref.store %s, %c[%i, %c0] : memref<8x8xi32>
      }
    }
  }
  return
}
func.func @empty_repeat(%a: memref<8x8xi32>, %b: memref<8x8xi32>, %c: memref<8x8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  %c3 = arith.constant 3 : index
  scf.for %i = %c0 to %n step %c1 {
    scf.for %k = %c0 to %c3 step %c1 {
      scf.for %t = %k to %c2 step %c1 {
        %x = memref.load %a[%i, %k] : memref<8x8xi32>
        %y = memref.load %b[%k, %i] : memref<8x8xi32>
        %z = memref.load %c[%i, %c0] : memref<8x8xi32>
        %m = arith.muli %x, %y : i32
        %s = arith.addi %z, %m : i32
        memref.store %s, %c[%i, %c0] : memref<8x8xi32>
      }
    }
  }
  return
}
)";

static void testMultiplicity() {
  Module m(std::string(gapsSource) + multiplicitySource + multiplicitySource2);
  Sort w = Sort::bv(16);
  auto family = [&](const SemanticProgram &p, unsigned i, unsigned k) {
    CoordinateFamily f;
    f.axes[AxisI] = {p.terms->state(i, w)};
    f.axes[AxisK] = {p.terms->state(k, w)};
    return f;
  };
  auto evalMu = [&](Term mu, uint64_t n, const std::vector<uint64_t> &levels) {
    if (!mu)
      return (int64_t)-1;
    std::vector<APInt> ls;
    for (uint64_t v : levels)
      ls.push_back(APInt(16, v, /*isSigned=*/true));
    auto value = evaluateLevelTerm(mu, {APInt(16, n)}, ls);
    return value ? (int64_t)value->getSExtValue() : (int64_t)-1;
  };
  {
    // t in [0, k + 1): μ(i, k) = k + 1, over the scope and at each point.
    Fixture g = fixture(m, "growing_repeat", 1, 2);
    std::vector<unsigned> l = loops(g.program);
    CoordinateFamily f = family(g.program, l[0], l[1]);
    std::string why;
    auto r = proposeDomain(g.program, g.enumeration, 0, f, nullptr, ProposalLimits(), why);
    CHECK(r.has_value());
    if (!r)
      return;
    CHECK(r->order.levels.size() == 3 && r->order.guard->op == TermOp::Const);
    CheckedDomain finite = checkDomainFinite(g.program, g.enumeration, 0, f, *r, g.options);
    CheckedDomain symbolic = checkDomainSymbolic(g.program, g.proof(), g.enumeration.groups[0], f, *r,
                                                 g.options, z3Backend);
    CHECK(finite.status() == S::Verified && finite.multiplicity() == DomainMultiplicity::LogicalBox);
    CHECK(symbolic.status() == S::Verified && symbolic.multiplicity() == DomainMultiplicity::LogicalBox);
    for (uint64_t k = 0; k < 3; ++k)
      CHECK(evalMu(symbolic.multiplicityTerm(), 2, {1, k, 0}) == (int64_t)k + 1);
    for (const DomainPointFacts &point : finite.points())
      CHECK(point.maxMultiplicity == 3 && point.events == 6 * point.parameters[0].getZExtValue());
    // Theorem 13 stands with the varying repeat, and the integer fold commutes.
    ReductionVerdict v = checkReduction(g.program, g.proof(), g.enumeration, 0, f, nullptr, *r, g.options,
                                        z3Backend);
    CHECK(v.status() == S::Verified && v.tier() == T::Symbolic);
    CHECK(v.integerReorder().status == FactStatus::Proven);
    // The same request declared ConstantBox-shaped by a wrong constant bound is refuted.
    DomainRequest flat = *r;
    flat.order.levels[2].upper = g.program.terms->bv(APInt(16, 3));
    CHECK(checkDomainFinite(g.program, g.enumeration, 0, f, flat, g.options).status() == S::Refuted);
    CHECK(checkDomainSymbolic(g.program, g.proof(), g.enumeration.groups[0], f, flat, g.options, z3Backend)
              .status() != S::Verified);
    // A logical level's bound that reads the repeat level is no box (Theorem 14's C2),
    // and the finite route still judges the set.
    DomainRequest tangled = *r;
    Term t = levelVariable(*g.program.terms, 2, w);
    tangled.order.levels[1].upper = g.program.terms->binary(
        TermOp::Add, g.program.terms->bv(APInt(16, 3)), g.program.terms->binary(TermOp::Mul, t, g.program.terms->bv(APInt(16, 0))));
    CheckedDomain refused = checkDomainFinite(g.program, g.enumeration, 0, f, tangled, g.options);
    CHECK(refused.reason().find("refused") != std::string::npos); // k is outer to t.
  }
  {
    // t in [-1, k + 1), signed: μ = k + 2 only when the level is compared signed.
    Fixture s = fixture(m, "signed_repeat", 1, 2);
    std::vector<unsigned> l = loops(s.program);
    CoordinateFamily f = family(s.program, l[0], l[1]);
    std::string why;
    auto r = proposeDomain(s.program, s.enumeration, 0, f, nullptr, ProposalLimits(), why);
    if (!r)
      printf("no proposal: %s\n", why.c_str());
    CHECK(r.has_value());
    if (r) {
      CHECK(r->order.levels[2].isSigned);
      CheckedDomain finite = checkDomainFinite(s.program, s.enumeration, 0, f, *r, s.options);
      CheckedDomain symbolic = checkDomainSymbolic(s.program, s.proof(), s.enumeration.groups[0], f, *r,
                                                   s.options, z3Backend);
      if (finite.status() != S::Verified || symbolic.status() != S::Verified)
        printf("signed_repeat: %s / %s\n", finite.reason().c_str(), symbolic.reason().c_str());
      CHECK(finite.status() == S::Verified && finite.multiplicity() == DomainMultiplicity::LogicalBox);
      CHECK(symbolic.status() == S::Verified && symbolic.multiplicity() == DomainMultiplicity::LogicalBox);
      for (uint64_t k = 0; k < 3; ++k)
        CHECK(evalMu(symbolic.multiplicityTerm(), 2, {1, k, 0}) == (int64_t)k + 2);
      // Read unsigned, -1 is 65535: the events are out of the order, and refuted.
      DomainRequest unsignedT = *r;
      unsignedT.order.levels[2].isSigned = false;
      CHECK(checkDomainFinite(s.program, s.enumeration, 0, f, unsignedT, s.options).status() == S::Refuted);
      CHECK(checkDomainSymbolic(s.program, s.proof(), s.enumeration.groups[0], f, unsignedT, s.options,
                                z3Backend).status() != S::Verified);
    }
  }
  {
    // u in [0, t + 1): a repeat bound reading a repeat level is no box (C3). Counted
    // exactly: μ = 1 + 2 = 3 at every logical point.
    Fixture s = fixture(m, "nested_repeats", 1, 2);
    std::vector<unsigned> l = loops(s.program);
    CoordinateFamily f = family(s.program, l[0], l[1]);
    std::string why;
    auto r = proposeDomain(s.program, s.enumeration, 0, f, nullptr, ProposalLimits(), why);
    if (!r)
      printf("no proposal: %s\n", why.c_str());
    CHECK(r.has_value());
    if (r) {
      CheckedDomain finite = checkDomainFinite(s.program, s.enumeration, 0, f, *r, s.options);
      CHECK(finite.status() == S::Verified && finite.multiplicity() == DomainMultiplicity::Counted);
      for (const DomainPointFacts &point : finite.points())
        CHECK(point.maxMultiplicity == 3);
      CheckedDomain symbolic = checkDomainSymbolic(s.program, s.proof(), s.enumeration.groups[0], f, *r,
                                                   s.options, z3Backend);
      CHECK(symbolic.multiplicity() == DomainMultiplicity::Unknown);
      CHECK(symbolic.multiplicityTerm() == nullptr);
    }
  }
  {
    // Two repeat levels, t in [0, k + 1) and u in [0, 2): μ = 2 (k + 1).
    Fixture s = fixture(m, "two_repeats", 1, 2);
    std::vector<unsigned> l = loops(s.program);
    CoordinateFamily f = family(s.program, l[0], l[1]);
    std::string why;
    auto r = proposeDomain(s.program, s.enumeration, 0, f, nullptr, ProposalLimits(), why);
    if (!r)
      printf("no proposal: %s\n", why.c_str());
    CHECK(r.has_value());
    if (r) {
      CheckedDomain symbolic = checkDomainSymbolic(s.program, s.proof(), s.enumeration.groups[0], f, *r,
                                                   s.options, z3Backend);
      if (symbolic.status() != S::Verified || symbolic.multiplicity() != DomainMultiplicity::LogicalBox) {
        describeRequest("two_repeats", *r);
        printf("  %s %s\n", stringifyComputationStatus(symbolic.status()), symbolic.reason().c_str());
      }
      CHECK(symbolic.status() == S::Verified && symbolic.multiplicity() == DomainMultiplicity::LogicalBox);
      for (uint64_t k = 0; k < 3; ++k)
        CHECK(evalMu(symbolic.multiplicityTerm(), 2, {1, k, 0, 0}) == 2 * ((int64_t)k + 1));
    }
  }
  {
    // Floats: the repeats stay in order, the fold is ordered, and reversing changes bits.
    Fixture s = fixture(m, "float_repeat", 1, 2);
    std::vector<unsigned> l = loops(s.program);
    CoordinateFamily f = family(s.program, l[0], l[1]);
    std::string why;
    auto r = proposeDomain(s.program, s.enumeration, 0, f, nullptr, ProposalLimits(), why);
    if (!r)
      printf("no proposal: %s\n", why.c_str());
    CHECK(r.has_value());
    if (r) {
      ReductionVerdict v = checkReduction(s.program, s.proof(), s.enumeration, 0, f, nullptr, *r, s.options,
                                          z3Backend);
      if (v.status() != S::Verified || v.domainSymbolic().multiplicity() != DomainMultiplicity::LogicalBox) {
        describeRequest("float_repeat", *r);
        printf("  %s %s\n", stringifyComputationStatus(v.domainSymbolic().status()), v.domainSymbolic().reason().c_str());
      }
      CHECK(v.status() == S::Verified);
      CHECK(v.domainSymbolic().multiplicity() == DomainMultiplicity::LogicalBox);
      CHECK(v.floatReorder().status == FactStatus::Refuted);
      CHECK(v.integerReorder().status != FactStatus::Proven);
    }
  }
  {
    // A repeat level read the other way round: the level is 2 - t, descending, with
    // bounds [2 - k, 3), and μ = k + 1 still. The direction must be right, or the
    // events are out of order and refuted.
    Fixture d = fixture(m, "descending_repeat", 1, 2);
    std::vector<unsigned> l = loops(d.program);
    CoordinateFamily f = family(d.program, l[0], l[1]);
    TermContext &terms = *d.program.terms;
    DomainRequest r;
    GuardedLevel i, k, tt;
    i.value = terms.state(l[0], w);
    i.lower = terms.bv(APInt(16, 0));
    i.upper = terms.param(0, w);
    k.value = terms.state(l[1], w);
    k.lower = terms.bv(APInt(16, 0));
    k.upper = terms.bv(APInt(16, 3));
    tt.value = terms.binary(TermOp::Sub, terms.bv(APInt(16, 2)), terms.state(l[2], w));
    tt.lower = terms.binary(TermOp::Sub, terms.bv(APInt(16, 2)), levelVariable(terms, 1, w));
    tt.upper = terms.bv(APInt(16, 3));
    tt.descending = true;
    r.order.levels = {i, k, tt};
    r.order.guard = terms.boolean(true);
    r.cellKey = {1, 2};
    CheckedDomain finite = checkDomainFinite(d.program, d.enumeration, 0, f, r, d.options);
    CheckedDomain symbolic = checkDomainSymbolic(d.program, d.proof(), d.enumeration.groups[0], f, r, d.options,
                                                 z3Backend);
    if (finite.status() != S::Verified || symbolic.status() != S::Verified)
      printf("descending: %s / %s\n", finite.reason().c_str(), symbolic.reason().c_str());
    CHECK(finite.status() == S::Verified && finite.multiplicity() == DomainMultiplicity::LogicalBox);
    CHECK(symbolic.status() == S::Verified && symbolic.multiplicity() == DomainMultiplicity::LogicalBox);
    for (uint64_t kk = 0; kk < 3; ++kk)
      CHECK(evalMu(symbolic.multiplicityTerm(), 2, {1, kk, 0}) == (int64_t)kk + 1);
    DomainRequest ascending = r;
    ascending.order.levels[2].descending = false;
    CHECK(checkDomainFinite(d.program, d.enumeration, 0, f, ascending, d.options).status() == S::Refuted);
    CHECK(checkDomainSymbolic(d.program, d.proof(), d.enumeration.groups[0], f, ascending, d.options, z3Backend)
              .status() != S::Verified);
  }
  {
    // t in [k, 2): empty at k = 2, so that logical point has no event and μ = 0 there.
    // Its events are two at k = 0, one at k = 1.
    Fixture e = fixture(m, "empty_repeat", 1, 2);
    std::vector<unsigned> l = loops(e.program);
    CoordinateFamily f = family(e.program, l[0], l[1]);
    std::string why;
    auto r = proposeDomain(e.program, e.enumeration, 0, f, nullptr, ProposalLimits(), why);
    if (!r)
      printf("empty_repeat: %s\n", why.c_str());
    CHECK(r.has_value());
    if (r) {
      CheckedDomain finite = checkDomainFinite(e.program, e.enumeration, 0, f, *r, e.options);
      CheckedDomain symbolic = checkDomainSymbolic(e.program, e.proof(), e.enumeration.groups[0], f, *r,
                                                   e.options, z3Backend);
      if (finite.status() != S::Verified || symbolic.status() != S::Verified)
        describeRequest("empty_repeat", *r);
      CHECK(finite.status() == S::Verified && finite.multiplicity() == DomainMultiplicity::LogicalBox);
      CHECK(symbolic.status() == S::Verified && symbolic.multiplicity() == DomainMultiplicity::LogicalBox);
      CHECK(evalMu(symbolic.multiplicityTerm(), 2, {1, 0, 0}) == 2);
      CHECK(evalMu(symbolic.multiplicityTerm(), 2, {1, 1, 1}) == 1);
      CHECK(evalMu(symbolic.multiplicityTerm(), 2, {1, 2, 2}) == 0); // The clamp, not -1.
      for (const DomainPointFacts &point : finite.points())
        CHECK(point.logicalPoints == 2 * point.parameters[0].getZExtValue());
    }
  }
}

/// Sibling loops: what G5a gives today, pinned so that nothing false appears while the
/// representation they need (G5b, second milestone) is not there.
static const char *siblingSource = R"(func.func @sib_same(%a: memref<8xi32>, %b: memref<8xi32>, %c: memref<8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  scf.for %i = %c0 to %n step %c1 {
    %x = memref.load %a[%i] : memref<8xi32>
    %y = memref.load %b[%i] : memref<8xi32>
    %z = memref.load %c[%i] : memref<8xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<8xi32>
  }
  scf.for %j = %c0 to %n step %c1 {
    %x2 = memref.load %a[%j] : memref<8xi32>
    %y2 = memref.load %b[%j] : memref<8xi32>
    %z2 = memref.load %c[%j] : memref<8xi32>
    %m2 = arith.muli %x2, %y2 : i32
    %s2 = arith.addi %z2, %m2 : i32
    memref.store %s2, %c[%j] : memref<8xi32>
  }
  return
}
func.func @sib_halves(%a: memref<8xi32>, %b: memref<8xi32>, %c: memref<8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c4 = arith.constant 4 : index
  scf.for %i = %c0 to %n step %c1 {
    %x = memref.load %a[%i] : memref<8xi32>
    %y = memref.load %b[%i] : memref<8xi32>
    %z = memref.load %c[%i] : memref<8xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<8xi32>
  }
  scf.for %j = %c0 to %n step %c1 {
    %jj = arith.addi %j, %c4 : index
    %x2 = memref.load %a[%jj] : memref<8xi32>
    %y2 = memref.load %b[%jj] : memref<8xi32>
    %z2 = memref.load %c[%jj] : memref<8xi32>
    %m2 = arith.muli %x2, %y2 : i32
    %s2 = arith.addi %z2, %m2 : i32
    memref.store %s2, %c[%jj] : memref<8xi32>
  }
  return
}
func.func @sib_reversed(%a: memref<8xi32>, %b: memref<8xi32>, %c: memref<8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c3 = arith.constant 3 : index
  scf.for %i = %c0 to %n step %c1 {
    %x = memref.load %a[%i] : memref<8xi32>
    %y = memref.load %b[%i] : memref<8xi32>
    %z = memref.load %c[%i] : memref<8xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<8xi32>
  }
  scf.for %j = %c0 to %n step %c1 {
    %jj = arith.subi %c3, %j : index
    %x2 = memref.load %a[%jj] : memref<8xi32>
    %y2 = memref.load %b[%jj] : memref<8xi32>
    %z2 = memref.load %c[%jj] : memref<8xi32>
    %m2 = arith.muli %x2, %y2 : i32
    %s2 = arith.addi %z2, %m2 : i32
    memref.store %s2, %c[%jj] : memref<8xi32>
  }
  return
}
func.func @sib_conditional(%a: memref<8xi32>, %b: memref<8xi32>, %c: memref<8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  scf.for %i = %c0 to %n step %c1 {
    %x = memref.load %a[%i] : memref<8xi32>
    %y = memref.load %b[%i] : memref<8xi32>
    %z = memref.load %c[%i] : memref<8xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<8xi32>
  }
  %big = arith.cmpi ugt, %n, %c1 : index
  scf.if %big {
    scf.for %j = %c0 to %n step %c1 {
      %x2 = memref.load %a[%j] : memref<8xi32>
      %y2 = memref.load %b[%j] : memref<8xi32>
      %z2 = memref.load %c[%j] : memref<8xi32>
      %m2 = arith.muli %x2, %y2 : i32
      %s2 = arith.addi %z2, %m2 : i32
      memref.store %s2, %c[%j] : memref<8xi32>
    }
  }
  return
}
func.func @sib_roots(%a: memref<8xi32>, %b: memref<8xi32>, %c: memref<8xi32>, %d: memref<8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  scf.for %i = %c0 to %n step %c1 {
    %x = memref.load %a[%i] : memref<8xi32>
    %y = memref.load %b[%i] : memref<8xi32>
    %z = memref.load %c[%i] : memref<8xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<8xi32>
  }
  scf.for %j = %c0 to %n step %c1 {
    %x2 = memref.load %a[%j] : memref<8xi32>
    %y2 = memref.load %d[%j] : memref<8xi32>
    %z2 = memref.load %c[%j] : memref<8xi32>
    %m2 = arith.muli %x2, %y2 : i32
    %s2 = arith.addi %z2, %m2 : i32
    memref.store %s2, %c[%j] : memref<8xi32>
  }
  return
}
)";

static void testSiblings() {
  Module m(siblingSource);
  Sort w = Sort::bv(16);
  for (const char *name : {"sib_same", "sib_halves", "sib_reversed", "sib_conditional"}) {
    Fixture f = fixture(m, name, 1, 3);
    CHECK(f.enumeration.gate == GateStatus::Open);
    CHECK(f.program.sites.size() == 2 && f.enumeration.groups.size() == 1);
    // The second loop's variable is undefined during the first loop, so it is no live
    // coordinate; the first loop's is constant during the second. No family of one
    // loop variable can tell the second loop's events apart.
    std::vector<unsigned> l = loops(f.program);
    CHECK(l.size() == 2);
    const AccessDomain *domain = nullptr;
    for (const PointEvidence &point : f.enumeration.points)
      if (point.domains[0])
        domain = &*point.domains[0];
    CHECK(domain != nullptr);
    if (domain) {
      CHECK(!llvm::is_contained(domain->table().coordinateNames, l[1]));
      CHECK(llvm::is_contained(domain->table().coordinateNames, l[0]));
    }
    for (unsigned variable : l) {
      CoordinateFamily family;
      family.axes[AxisI] = {f.program.terms->state(variable, w)};
      CheckedFamily finite = checkFamilyFinite(f.program, f.enumeration, 0, family);
      CHECK(finite.status() != FamilyStatus::Verified);
      std::string why;
      auto r = proposeDomain(f.program, f.enumeration, 0, family, nullptr, ProposalLimits(), why);
      if (r) {
        CheckedDomain fin = checkDomainFinite(f.program, f.enumeration, 0, family, *r, f.options);
        CheckedDomain sym = checkDomainSymbolic(f.program, f.proof(), f.enumeration.groups[0], family, *r,
                                                f.options, z3Backend);
        CHECK(fin.status() != S::Verified && sym.status() != S::Verified);
      }
    }
    // The search finds nothing either: Unknown, with no false claim on any route.
    SynthesisOptions search;
    search.scope = f.scope;
    search.authorized = f.authorized;
    search.maxLevel = 1;
    search.termination.solver = testSolver();
    search.synthesis.solver = testSolver();
    SynthesisResult found = synthesizeProgram(f.program, f.proof(), search, z3Backend);
    CHECK(found.groups.size() == 1 && found.groups[0].candidates.empty());
    CHECK(found.groups[0].status == RecoveryStatus::Unknown);
    // The computation, by contrast, is one group and is verified: the limit is the
    // coordinates and the order, not the values.
    ComputationOptions computation;
    computation.scope = f.scope;
    computation.authorized = f.authorized;
    computation.termination.solver = testSolver();
    CheckedComputation values = checkComputationSymbolic(f.program, f.proof(), f.enumeration.groups[0],
                                                         computation, z3Backend);
    if (values.status() != S::Verified)
      printf("%s: %s\n", name, values.reason().c_str());
    CHECK(values.status() == S::Verified);
  }
  {
    // Different operand roots make two groups, and each group's own relation is false:
    // the other group writes its cells. Refuted, with a witness, and no Theorem 13.
    Fixture f = fixture(m, "sib_roots", 1, 2);
    CHECK(f.enumeration.groups.size() == 2);
    std::vector<unsigned> l = loops(f.program);
    for (unsigned g = 0; g < 2; ++g) {
      CoordinateFamily family;
      family.axes[AxisP] = {f.program.terms->state(l[g], w)};
      std::string why;
      auto r = proposeDomain(f.program, f.enumeration, g, family, nullptr, ProposalLimits(), why);
      CHECK(r.has_value());
      if (!r)
        continue;
      ReductionVerdict v = checkReduction(f.program, f.proof(), f.enumeration, g, family, nullptr, *r,
                                          f.options, z3Backend);
      CHECK(v.domainSymbolic().status() == S::Verified);
      CHECK(v.computationFinite().status() == S::Refuted && v.computationFinite().witness().has_value());
      CHECK(v.status() == S::Refuted);
      CHECK(v.integerReorder().status != FactStatus::Proven);
    }
  }
}

/// G5b-M2: segmented families and orders, Theorem 15.
static const char *segmentSource = R"(func.func @sib_counter(%a: memref<8xi32>, %b: memref<8xi32>, %c: memref<8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %p1 = scf.for %i = %c0 to %n step %c1 iter_args(%p = %c0) -> (index) {
    %x = memref.load %a[%p] : memref<8xi32>
    %y = memref.load %b[%p] : memref<8xi32>
    %z = memref.load %c[%p] : memref<8xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%p] : memref<8xi32>
    %q = arith.addi %p, %c1 : index
    scf.yield %q : index
  }
  scf.for %j = %c0 to %n step %c1 iter_args(%p2 = %p1) -> (index) {
    %x2 = memref.load %a[%p2] : memref<8xi32>
    %y2 = memref.load %b[%p2] : memref<8xi32>
    %z2 = memref.load %c[%p2] : memref<8xi32>
    %m2 = arith.muli %x2, %y2 : i32
    %s2 = arith.addi %z2, %m2 : i32
    memref.store %s2, %c[%p2] : memref<8xi32>
    %q2 = arith.addi %p2, %c1 : index
    scf.yield %q2 : index
  }
  return
}
func.func @sib_bounds(%a: memref<8xi32>, %b: memref<8xi32>, %c: memref<8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c3 = arith.constant 3 : index
  scf.for %i = %c0 to %n step %c1 {
    %x = memref.load %a[%i] : memref<8xi32>
    %y = memref.load %b[%i] : memref<8xi32>
    %z = memref.load %c[%i] : memref<8xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<8xi32>
  }
  scf.for %j = %c0 to %c3 step %c1 {
    %x2 = memref.load %a[%j] : memref<8xi32>
    %y2 = memref.load %b[%j] : memref<8xi32>
    %z2 = memref.load %c[%j] : memref<8xi32>
    %m2 = arith.muli %x2, %y2 : i32
    %s2 = arith.addi %z2, %m2 : i32
    memref.store %s2, %c[%j] : memref<8xi32>
  }
  return
}
func.func @sib_empty_first(%a: memref<8xi32>, %b: memref<8xi32>, %c: memref<8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  %c3 = arith.constant 3 : index
  scf.for %i = %n to %c3 step %c1 {
    %x = memref.load %a[%i] : memref<8xi32>
    %y = memref.load %b[%i] : memref<8xi32>
    %z = memref.load %c[%i] : memref<8xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<8xi32>
  }
  scf.for %j = %c0 to %c2 step %c1 {
    %x2 = memref.load %a[%j] : memref<8xi32>
    %y2 = memref.load %b[%j] : memref<8xi32>
    %z2 = memref.load %c[%j] : memref<8xi32>
    %m2 = arith.muli %x2, %y2 : i32
    %s2 = arith.addi %z2, %m2 : i32
    memref.store %s2, %c[%j] : memref<8xi32>
  }
  return
}
func.func @sib_empty_second(%a: memref<8xi32>, %b: memref<8xi32>, %c: memref<8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  %c3 = arith.constant 3 : index
  scf.for %i = %c0 to %c2 step %c1 {
    %x = memref.load %a[%i] : memref<8xi32>
    %y = memref.load %b[%i] : memref<8xi32>
    %z = memref.load %c[%i] : memref<8xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<8xi32>
  }
  scf.for %j = %n to %c3 step %c1 {
    %x2 = memref.load %a[%j] : memref<8xi32>
    %y2 = memref.load %b[%j] : memref<8xi32>
    %z2 = memref.load %c[%j] : memref<8xi32>
    %m2 = arith.muli %x2, %y2 : i32
    %s2 = arith.addi %z2, %m2 : i32
    memref.store %s2, %c[%j] : memref<8xi32>
  }
  return
}
func.func @sib_empty_single(%a: memref<8xi32>, %b: memref<8xi32>, %c: memref<8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  scf.for %i = %c0 to %c2 step %c1 {
    %x = memref.load %a[%i] : memref<8xi32>
    %y = memref.load %b[%i] : memref<8xi32>
    %z = memref.load %c[%i] : memref<8xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<8xi32>
  }
  scf.for %j = %n to %c2 step %c1 {
    %x2 = memref.load %a[%j] : memref<8xi32>
    %y2 = memref.load %b[%j] : memref<8xi32>
    %z2 = memref.load %c[%j] : memref<8xi32>
    %m2 = arith.muli %x2, %y2 : i32
    %s2 = arith.addi %z2, %m2 : i32
    memref.store %s2, %c[%j] : memref<8xi32>
  }
  return
}
func.func @sib_empty_both(%a: memref<8xi32>, %b: memref<8xi32>, %c: memref<8xi32>, %n: index) {
  %c1 = arith.constant 1 : index
  %c3 = arith.constant 3 : index
  scf.for %i = %n to %c3 step %c1 {
    %x = memref.load %a[%i] : memref<8xi32>
    %y = memref.load %b[%i] : memref<8xi32>
    %z = memref.load %c[%i] : memref<8xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<8xi32>
  }
  scf.for %j = %n to %c3 step %c1 {
    %x2 = memref.load %a[%j] : memref<8xi32>
    %y2 = memref.load %b[%j] : memref<8xi32>
    %z2 = memref.load %c[%j] : memref<8xi32>
    %m2 = arith.muli %x2, %y2 : i32
    %s2 = arith.addi %z2, %m2 : i32
    memref.store %s2, %c[%j] : memref<8xi32>
  }
  return
}
func.func @sib_three(%a: memref<8xi32>, %b: memref<8xi32>, %c: memref<8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  scf.for %i = %c0 to %n step %c1 {
    %x = memref.load %a[%i] : memref<8xi32>
    %y = memref.load %b[%i] : memref<8xi32>
    %z = memref.load %c[%i] : memref<8xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<8xi32>
  }
  scf.for %j = %c1 to %n step %c1 {
    %x2 = memref.load %a[%j] : memref<8xi32>
    %y2 = memref.load %b[%j] : memref<8xi32>
    %z2 = memref.load %c[%j] : memref<8xi32>
    %m2 = arith.muli %x2, %y2 : i32
    %s2 = arith.addi %z2, %m2 : i32
    memref.store %s2, %c[%j] : memref<8xi32>
  }
  scf.for %k = %c0 to %n step %c1 {
    %x3 = memref.load %a[%k] : memref<8xi32>
    %y3 = memref.load %b[%k] : memref<8xi32>
    %z3 = memref.load %c[%k] : memref<8xi32>
    %m3 = arith.muli %x3, %y3 : i32
    %s3 = arith.addi %z3, %m3 : i32
    memref.store %s3, %c[%k] : memref<8xi32>
  }
  return
}
func.func @sib_repeat(%a: memref<8xi32>, %b: memref<8xi32>, %c: memref<8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  scf.for %i = %c0 to %n step %c1 {
    %x = memref.load %a[%i] : memref<8xi32>
    %y = memref.load %b[%i] : memref<8xi32>
    %z = memref.load %c[%i] : memref<8xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<8xi32>
  }
  scf.for %j = %c0 to %n step %c1 {
    %top = arith.addi %j, %c1 : index
    scf.for %t = %c0 to %top step %c1 {
      %x2 = memref.load %a[%j] : memref<8xi32>
      %y2 = memref.load %b[%j] : memref<8xi32>
      %z2 = memref.load %c[%j] : memref<8xi32>
      %m2 = arith.muli %x2, %y2 : i32
      %s2 = arith.addi %z2, %m2 : i32
      memref.store %s2, %c[%j] : memref<8xi32>
    }
  }
  return
}
func.func @sib_diag(%a: memref<8xi32>, %b: memref<8xi32>, %c: memref<8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  %c3 = arith.constant 3 : index
  scf.for %i = %c0 to %n step %c1 {
    %x = memref.load %a[%i] : memref<8xi32>
    %y = memref.load %b[%i] : memref<8xi32>
    %z = memref.load %c[%i] : memref<8xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<8xi32>
  }
  scf.for %p = %c0 to %c2 step %c1 {
    scf.for %q = %c0 to %c3 step %c1 {
      %d = arith.addi %p, %q : index
      %x2 = memref.load %a[%d] : memref<8xi32>
      %y2 = memref.load %b[%d] : memref<8xi32>
      %z2 = memref.load %c[%d] : memref<8xi32>
      %m2 = arith.muli %x2, %y2 : i32
      %s2 = arith.addi %z2, %m2 : i32
      memref.store %s2, %c[%d] : memref<8xi32>
    }
  }
  return
}
func.func @sib_shuffle(%a: memref<8xi32>, %b: memref<8xi32>, %c: memref<8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  %c4 = arith.constant 4 : index
  scf.for %i = %c0 to %n step %c1 {
    %x = memref.load %a[%i] : memref<8xi32>
    %y = memref.load %b[%i] : memref<8xi32>
    %z = memref.load %c[%i] : memref<8xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<8xi32>
  }
  scf.for %j = %c0 to %c4 step %c1 {
    %lo = arith.remui %j, %c2 : index
    %hi = arith.divui %j, %c2 : index
    %sh = arith.muli %lo, %c2 : index
    %jj = arith.addi %sh, %hi : index
    %x2 = memref.load %a[%jj] : memref<8xi32>
    %y2 = memref.load %b[%jj] : memref<8xi32>
    %z2 = memref.load %c[%jj] : memref<8xi32>
    %m2 = arith.muli %x2, %y2 : i32
    %s2 = arith.addi %z2, %m2 : i32
    memref.store %s2, %c[%jj] : memref<8xi32>
  }
  return
}
)";

/// A segment of one site with levels given as (value, lower, upper, descending).
struct LevelSpec {
  Term value, lower, upper;
  bool descending = false;
  bool isSigned = false;
};
static Segment segmentOf(const SemanticProgram &p, std::vector<unsigned> sites,
                         std::vector<LevelSpec> levels, Term guard = nullptr,
                         std::vector<unsigned> key = {}, unsigned logical = ~0u) {
  Segment out;
  out.sites = std::move(sites);
  for (unsigned m = 0; m < levels.size(); ++m) {
    GuardedLevel level;
    level.value = levels[m].value;
    level.lower = levels[m].lower;
    level.upper = levels[m].upper;
    level.descending = levels[m].descending;
    level.isSigned = levels[m].isSigned;
    out.order.levels.push_back(level);
    if (logical == ~0u || m < logical)
      out.family.axes[AxisI].push_back(levels[m].value);
  }
  out.order.guard = guard ? guard : p.terms->boolean(true);
  out.cellKey = key.empty() ? std::vector<unsigned>{(unsigned)levels.size() - 1} : key;
  return out;
}

/// Each obligation of Theorem 15 alone, and every gate of the segmented route.
static void testSegmentGates() {
  Module m(std::string(siblingSource) + segmentSource);
  Sort w = Sort::bv(16);
  CoordinateFamily none;
  auto both = [&](Fixture &f, const DomainRequest &r) {
    return std::make_pair(checkDomainFinite(f.program, f.enumeration, 0, none, r, f.options),
                          checkDomainSymbolic(f.program, f.proof(), f.enumeration.groups[0], none, r,
                                              f.options, z3Backend));
  };
  auto describe = [](const char *what, const CheckedDomain &a, const CheckedDomain &b) {
    printf("%s: finite %s %s (%s); symbolic %s (%s)\n", what, stringifyComputationStatus(a.status()),
           stringifyComputationTier(a.tier()), a.reason().c_str(), stringifyComputationStatus(b.status()),
           b.reason().c_str());
  };
  auto expectVerified = [&](const char *what, Fixture &f, const DomainRequest &r) {
    auto [fin, sym] = both(f, r);
    bool ok = fin.status() == S::Verified && fin.tier() == T::ScopeEnumerated && sym.status() == S::Verified;
    if (!ok)
      describe(what, fin, sym);
    CHECK(ok);
  };
  // A finite refutation, and no symbolic claim: the one obligation named is what stands
  // between the request and a false proof.
  auto expectRefuted = [&](const char *what, Fixture &f, const DomainRequest &r, const char *obligation) {
    auto [fin, sym] = both(f, r);
    bool ok = fin.status() == S::Refuted && fin.refutation().point.has_value() && sym.status() == S::Unknown &&
              sym.reason().find(obligation) != std::string::npos;
    if (!ok)
      describe(what, fin, sym);
    CHECK(ok);
  };
  auto plain = [&](const SemanticProgram &p, unsigned var, Term lower, Term upper, bool desc = false) {
    return LevelSpec{p.terms->state(var, w), lower, upper, desc, false};
  };
  {
    Fixture f = fixture(m, "sib_same", 1, 3);
    std::vector<unsigned> l = loops(f.program);
    TermContext &terms = *f.program.terms;
    Term zero = terms.bv(APInt(16, 0)), n = terms.param(0, w), one = terms.bv(APInt(16, 1));
    DomainRequest r;
    r.segments = {segmentOf(f.program, {0}, {plain(f.program, l[0], zero, n)}),
                  segmentOf(f.program, {1}, {plain(f.program, l[1], zero, n)})};
    auto run = [&](SmtBackend &backend, const DomainOptions &options, const DomainRequest &req) {
      return checkDomainSymbolic(f.program, f.proof(), f.enumeration.groups[0], none, req, options, backend);
    };
    CHECK(run(z3Backend, f.options, r).status() == S::Verified);
    // Every obligation is asked, and an unknown or sat answer to any one is Unknown. (D5
    // is asked where a first loop can reach the exit: below, on sib_empty_second.)
    for (const char *id : {"; D1 ", "; D2 ", "; D3 ", "; D4 "})
      for (SmtAnswer answer : {SmtAnswer::Unknown, SmtAnswer::Sat}) {
        Selective solver(id, answer);
        CheckedDomain out = run(solver, f.options, r);
        if (solver.hits == 0)
          printf("no %s query\n", id);
        CHECK(solver.hits > 0);
        CHECK(out.status() == S::Unknown && !out.exactOrder());
      }
    // The facts: unknown leaves them unproven and the order stands. (K1 and A1 are not
    // asked here: a one-level key is every level, and no access terms were proposed.)
    for (const char *id : {"; C1 ", "; T1 ", "; SH "}) {
      Selective solver(id, SmtAnswer::Unknown);
      CheckedDomain out = run(solver, f.options, r);
      CHECK(solver.hits > 0);
      CHECK(out.status() == S::Verified);
      std::string name(id);
      if (name == "; T1 ")
        CHECK(out.singleContribution().status == FactStatus::Unknown);
      if (name == "; SH ")
        CHECK(out.sharedCells().status == FactStatus::Unknown);
      if (name == "; C1 ") // A claim that is not Verified has no segment to read: the check then fails.
        CHECK(!out.segments().empty() && out.segments()[0].outputInjective.status == FactStatus::Unknown &&
              out.outputInjective().status == FactStatus::Unknown);
    }
    // Limits and certificates, as for one order.
    DomainOptions zero_ = f.options;
    zero_.termination.solver.timeoutMilliseconds = 0;
    CHECK(run(z3Backend, zero_, r).reason().find("limit of zero") != std::string::npos);
    DomainOptions few = f.options;
    few.limits.maxQueries = 3;
    CHECK(run(z3Backend, few, r).status() == S::Unknown);
    DomainOptions shallow = f.options;
    shallow.limits.maxUnfolding = 1;
    CHECK(run(z3Backend, shallow, r).status() == S::Unknown);
    DomainOptions pairs = f.options;
    pairs.limits.maxPairs = 0;
    CHECK(run(z3Backend, pairs, r).reason().find("arrival pairs") != std::string::npos);
    DomainOptions box = f.options;
    box.limits.maxBox = 1; // Only n = 1 fits.
    CheckedDomain boxed = checkDomainFinite(f.program, f.enumeration, 0, none, r, box);
    CHECK(boxed.status() == S::Unknown || (boxed.tier() == T::Point && boxed.points().size() == 1));
    CHECK(checkDomainSymbolic(f.program, nullptr, f.enumeration.groups[0], none, r, f.options, z3Backend).status() ==
          S::Unknown);
    if (f.certificate) {
      TerminationCertificate broken = *f.certificate;
      broken.levels.clear();
      CHECK(checkDomainSymbolic(f.program, &broken, f.enumeration.groups[0], none, r, f.options, z3Backend)
                .reason().find("not accepted") != std::string::npos);
    }
    // A sample is no scope; a point the run outgrew is no point.
    {
      Fixture one = fixture(m, "sib_same", 1, 3, 16, {{APInt(16, 1)}});
      CheckedDomain finite = checkDomainFinite(one.program, one.enumeration, 0, none, r, one.options);
      CHECK(finite.status() == S::Verified && finite.tier() == T::Point);
    }
    {
      EnumerationOptions tight;
      tight.scope = f.scope;
      tight.authorized = f.authorized;
      tight.termination.solver = testSolver();
      tight.liveStatePoints = true;
      tight.limits.maxEvents = 5; // n = 3 has six events.
      DomainEnumeration limited = enumerateDomain(f.program, f.proof(), tight, z3Backend);
      unsigned stopped = 0;
      for (const PointEvidence &point : limited.points)
        stopped += !point.reason.empty();
      CHECK(stopped == 1);
      CheckedDomain finite = checkDomainFinite(f.program, limited, 0, none, r, f.options);
      CHECK(finite.status() == S::Verified && finite.tier() == T::Point && finite.points().size() == 2);
    }
    // One obligation at a time.
    DomainRequest firstLong = r;
    firstLong.segments[0].order.levels[0].upper = terms.binary(TermOp::Add, n, one);
    expectRefuted("first segment too long", f, firstLong, "D4");
    DomainRequest shifted = r;
    shifted.segments[1] = segmentOf(f.program, {1}, {LevelSpec{terms.binary(TermOp::Add, terms.state(l[1], w), one),
                                                               zero, terms.binary(TermOp::Add, n, one), false, false}});
    expectRefuted("second segment shifted by one", f, shifted, "D4");
    DomainRequest backward;
    backward.segments = {segmentOf(f.program, {1}, {plain(f.program, l[1], zero, n)}, terms.boolean(false)),
                         segmentOf(f.program, {0}, {plain(f.program, l[0], zero, n)})};
    expectRefuted("the later loop as an empty earlier segment", f, backward, "D5");
    DomainRequest tooLong = r;
    tooLong.segments[1].order.levels[0].upper = terms.binary(TermOp::Add, n, one);
    expectRefuted("second segment too long", f, tooLong, "D3");
    // The same size, in order, and other elements: [1, n + 1) for events 0 .. n - 1.
    DomainRequest offset = r;
    offset.segments[1].order.levels[0].lower = one;
    offset.segments[1].order.levels[0].upper = terms.binary(TermOp::Add, n, one);
    expectRefuted("second segment offset by one", f, offset, "D4");
    // A scope wider than the enumeration: no claim over it.
    {
      DomainOptions wider = f.options;
      wider.scope = range(f.program, 1, 4);
      CheckedDomain finite = checkDomainFinite(f.program, f.enumeration, 0, none, r, wider);
      CHECK(finite.status() == S::Verified && finite.tier() == T::Point);
    }
    DomainRequest undefinedLevel;
    undefinedLevel.segments = {segmentOf(f.program, {0, 1}, {plain(f.program, l[1], zero, n)})};
    CheckedDomain refused = checkDomainFinite(f.program, f.enumeration, 0, none, undefinedLevel, f.options);
    CHECK(refused.status() == S::Unknown && refused.reason().find("refused") != std::string::npos);
    CHECK(checkDomainSymbolic(f.program, f.proof(), f.enumeration.groups[0], none, undefinedLevel, f.options,
                              z3Backend).status() != S::Verified);
  }
  {
    // A level that goes back without skipping: p + q over p in [0, 2), q in [0, 3) runs
    // 0 1 2 1 2 3, and only D2's increase stands against the set [0, 4).
    Fixture f = fixture(m, "sib_diag", 1, 2);
    std::vector<unsigned> l = loops(f.program);
    CHECK(l.size() == 3);
    TermContext &terms = *f.program.terms;
    Term zero = terms.bv(APInt(16, 0)), n = terms.param(0, w);
    Term d = terms.binary(TermOp::Add, terms.state(l[1], w), terms.state(l[2], w));
    DomainRequest r;
    r.segments = {segmentOf(f.program, {0}, {plain(f.program, l[0], zero, n)}),
                  segmentOf(f.program, {1}, {LevelSpec{d, zero, terms.bv(APInt(16, 4)), false, false}})};
    expectRefuted("a diagonal level that steps back", f, r, "D2");
    // By (p, q) it is a box, and the cells are shared with the first loop.
    DomainRequest box = r;
    box.segments[1] = segmentOf(f.program, {1}, {plain(f.program, l[1], zero, terms.bv(APInt(16, 2))),
                                                 plain(f.program, l[2], zero, terms.bv(APInt(16, 3)))});
    expectVerified("the diagonal nest by its loop variables", f, box);
  }
  {
    Fixture f = fixture(m, "sib_empty_first", 1, 4);
    std::vector<unsigned> l = loops(f.program);
    TermContext &terms = *f.program.terms;
    Term zero = terms.bv(APInt(16, 0)), n = terms.param(0, w), three = terms.bv(APInt(16, 3));
    Term y0 = levelVariable(terms, 0, w);
    // A first segment right only in scope: [1, 3) with i >= n. Out of scope, at n = 0,
    // the first event would not be its minimum.
    DomainRequest scoped;
    scoped.segments = {segmentOf(f.program, {0}, {plain(f.program, l[0], terms.bv(APInt(16, 1)), three)},
                                 terms.icmp(Predicate::Uge, y0, n)),
                       segmentOf(f.program, {1}, {plain(f.program, l[1], zero, terms.bv(APInt(16, 2)))})};
    expectVerified("a first segment that needs the scope", f, scoped);
    // S_0 nonempty where the first event is the second loop's: D1' alone.
    DomainRequest before = scoped;
    before.segments[0].order.guard =
        terms.logicalOr(terms.icmp(Predicate::Uge, y0, n), terms.icmp(Predicate::Uge, n, three));
    before.segments[0].order.levels[0].lower = zero;
    expectRefuted("a nonempty set before the first event", f, before, "D1");
  }
  {
    Fixture f = fixture(m, "sib_empty_both", 1, 4);
    std::vector<unsigned> l = loops(f.program);
    TermContext &terms = *f.program.terms;
    Term zero = terms.bv(APInt(16, 0)), n = terms.param(0, w), three = terms.bv(APInt(16, 3));
    DomainRequest r;
    r.segments = {segmentOf(f.program, {0}, {plain(f.program, l[0], n, three)}),
                  segmentOf(f.program, {1}, {plain(f.program, l[1], n, three)})};
    expectVerified("both segments empty at n >= 3", f, r);
    // A run with no event, and a set that is not empty: D1' at the exit alone.
    DomainRequest atExit = r;
    atExit.segments[1] = segmentOf(f.program, {1}, {plain(f.program, l[1], zero, terms.binary(TermOp::Sub, n, terms.bv(APInt(16, 2))))},
                                   terms.icmp(Predicate::Uge, n, three));
    expectRefuted("a nonempty set when the run has no event", f, atExit, "D1");
    // D5 is asked here: the first loop reaches the exit where the second is empty.
    {
      Fixture s = fixture(m, "sib_empty_second", 1, 4);
      std::vector<unsigned> sl = loops(s.program);
      DomainRequest second;
      TermContext &st = *s.program.terms;
      Term szero = st.bv(APInt(16, 0)), sn = st.param(0, w), sthree = st.bv(APInt(16, 3));
      second.segments = {segmentOf(s.program, {0}, {LevelSpec{st.state(sl[0], w), szero, st.bv(APInt(16, 2)), false, false}}),
                         segmentOf(s.program, {1}, {LevelSpec{st.state(sl[1], w), sn, sthree, false, false}})};
      CHECK(checkDomainSymbolic(s.program, s.proof(), s.enumeration.groups[0], none, second, s.options, z3Backend)
                .status() == S::Verified);
      for (SmtAnswer answer : {SmtAnswer::Unknown, SmtAnswer::Sat}) {
        Selective solver("; D5 ", answer);
        CheckedDomain out = checkDomainSymbolic(s.program, s.proof(), s.enumeration.groups[0], none, second,
                                                s.options, solver);
        if (solver.hits == 0)
          printf("no D5 query on sib_empty_second\n");
        CHECK(solver.hits > 0);
        CHECK(out.status() == S::Unknown && !out.exactOrder());
      }
    }
    // No point with events is no evidence.
    Fixture g = fixture(m, "sib_empty_both", 3, 4);
    CheckedDomain none_ = checkDomainFinite(g.program, g.enumeration, 0, none, r, g.options);
    CHECK(none_.status() == S::Unknown && none_.reason().find("no point has events") != std::string::npos);
  }
  {
    Fixture f = fixture(m, "sib_three", 1, 2);
    std::vector<unsigned> l = loops(f.program);
    CHECK(l.size() == 3);
    TermContext &terms = *f.program.terms;
    Term zero = terms.bv(APInt(16, 0)), n = terms.param(0, w), one = terms.bv(APInt(16, 1));
    DomainRequest r;
    r.segments = {segmentOf(f.program, {0}, {plain(f.program, l[0], zero, n)}),
                  segmentOf(f.program, {1}, {plain(f.program, l[1], one, n)}),
                  segmentOf(f.program, {2}, {plain(f.program, l[2], zero, n)})};
    expectVerified("three segments, the middle empty at n = 1", f, r);
    // The proposer needs two events of the middle loop at some point: n up to 3.
    Fixture wide = fixture(m, "sib_three", 1, 3);
    expectVerified("three segments, n up to 3", wide, r);
    std::string why;
    auto proposed = proposeSegmentedDomain(wide.program, wide.enumeration, 0, nullptr, ProposalLimits(), why);
    if (!proposed)
      printf("sib_three proposal: %s\n", why.c_str());
    CHECK(proposed && proposed->segments.size() == 3);
    if (proposed)
      expectVerified("three segments proposed", wide, *proposed);
    // The middle set nonempty at n = 1, where the first hands over to the third: D4 alone.
    DomainRequest between = r;
    between.segments[1] = segmentOf(f.program, {1}, {plain(f.program, l[1], terms.binary(TermOp::Sub, n, one), n)});
    expectRefuted("a nonempty set between a hand-over", f, between, "D4");
  }
  // The gates of the one-order route, on the segmented one.
  {
    Module rm(reviewed);
    Module gm(gapsSource);
    {
      Fixture e = fixture(gm, "entry_site", 1, 2);
      std::vector<unsigned> el = loops(e.program);
      TermContext &terms = *e.program.terms;
      DomainRequest r;
      r.segments = {segmentOf(e.program, e.enumeration.groups[0].sites,
                              {plain(e.program, el[0], terms.bv(APInt(16, 0)), terms.bv(APInt(16, 0)))})};
      CheckedDomain symbolic = checkDomainSymbolic(e.program, e.proof(), e.enumeration.groups[0], none, r,
                                                   e.options, z3Backend);
      CHECK(symbolic.status() == S::Unknown && symbolic.reason().find("entry block") != std::string::npos);
    }
    {
      Fixture f = fixture(rm, "two", 1, 2);
      std::vector<unsigned> l = loops(f.program);
      TermContext &terms = *f.program.terms;
      Term zero = terms.bv(APInt(16, 0)), three = terms.bv(APInt(16, 3));
      DomainRequest r;
      r.segments = {segmentOf(f.program, f.enumeration.groups[0].sites,
                              {plain(f.program, l[0], zero, terms.param(0, w)), plain(f.program, l[1], zero, three),
                               plain(f.program, l[2], zero, three)})};
      CheckedDomain symbolic = checkDomainSymbolic(f.program, f.proof(), f.enumeration.groups[0], none, r,
                                                   f.options, z3Backend);
      CHECK(symbolic.status() == S::Unknown && symbolic.reason().find("share a block") != std::string::npos);
      CHECK(checkDomainFinite(f.program, f.enumeration, 0, none, r, f.options).status() == S::Refuted);
    }
    {
      Fixture g = fixture(gm, "memory_guard", 1, 2);
      std::vector<unsigned> gl = loops(g.program);
      TermContext &terms = *g.program.terms;
      DomainRequest r;
      r.segments = {segmentOf(g.program, {0}, {plain(g.program, gl[0], terms.bv(APInt(16, 0)), terms.param(0, w)),
                                                plain(g.program, gl[1], terms.bv(APInt(16, 0)), terms.bv(APInt(16, 3)))})};
      CHECK(checkDomainFinite(g.program, g.enumeration, 0, none, r, g.options).reason().find("gate is closed") !=
            std::string::npos);
    }
  }
}

static void testSegments() {
  Module m(std::string(siblingSource) + segmentSource);
  Sort w = Sort::bv(16);
  CoordinateFamily none;
  auto both = [&](Fixture &f, const DomainRequest &r) {
    return std::make_pair(checkDomainFinite(f.program, f.enumeration, 0, none, r, f.options),
                          checkDomainSymbolic(f.program, f.proof(), f.enumeration.groups[0], none, r,
                                              f.options, z3Backend));
  };
  auto describe = [](const char *what, const CheckedDomain &a, const CheckedDomain &b) {
    printf("%s: finite %s %s (%s); symbolic %s (%s)\n", what, stringifyComputationStatus(a.status()),
           stringifyComputationTier(a.tier()), a.reason().c_str(), stringifyComputationStatus(b.status()),
           b.reason().c_str());
  };
  auto expectVerified = [&](const char *what, Fixture &f, const DomainRequest &r, bool symbolicToo = true) {
    auto [fin, sym] = both(f, r);
    bool ok = fin.status() == S::Verified && fin.tier() == T::ScopeEnumerated &&
              (!symbolicToo || sym.status() == S::Verified);
    if (!ok)
      describe(what, fin, sym);
    CHECK(ok);
    CHECK(fin.exactOrder() && (!symbolicToo || sym.exactOrder()));
    CHECK(fin.segments().size() == r.segments.size() && (!symbolicToo || sym.segments().size() == r.segments.size()));
    return std::make_pair(std::move(fin), std::move(sym));
  };
  auto expectNotVerified = [&](const char *what, Fixture &f, const DomainRequest &r, S finiteWant,
                               const char *text = nullptr) {
    auto [fin, sym] = both(f, r);
    bool ok = fin.status() == finiteWant && sym.status() != S::Verified &&
              (!text || fin.reason().find(text) != std::string::npos);
    if (!ok)
      describe(what, fin, sym);
    CHECK(ok);
    CHECK(sym.status() != S::Refuted);
    if (fin.status() == S::Refuted)
      CHECK(fin.refutation().point.has_value());
  };
  auto plain = [&](const SemanticProgram &p, unsigned var, Term lower, Term upper, bool desc = false) {
    return LevelSpec{p.terms->state(var, w), lower, upper, desc, false};
  };

  // Two loops over the same roots and cells: one group, two segments. Theorem 15 holds
  // on both routes; the cells are shared, which is a fact, and Theorem 13 holds over the
  // segmented key.
  {
    Fixture f = fixture(m, "sib_same", 1, 3);
    std::vector<unsigned> l = loops(f.program);
    TermContext &terms = *f.program.terms;
    Term zero = terms.bv(APInt(16, 0)), n = terms.param(0, w);
    DomainRequest r;
    r.segments = {segmentOf(f.program, {0}, {plain(f.program, l[0], zero, n)}),
                  segmentOf(f.program, {1}, {plain(f.program, l[1], zero, n)})};
    auto [fin, sym] = expectVerified("sib_same", f, r);
    CHECK(fin.sharedCells().status == FactStatus::Refuted && fin.sharedCells().point.has_value());
    CHECK(sym.sharedCells().status != FactStatus::Proven);
    CHECK(fin.singleContribution().status == FactStatus::Refuted);
    CHECK(fin.multiplicity() == DomainMultiplicity::Unit && sym.multiplicity() == DomainMultiplicity::Unit);
    for (const SegmentFacts &s : sym.segments())
      CHECK(s.cellKey.status == FactStatus::Proven && s.outputInjective.status == FactStatus::Proven);
    ReductionVerdict v = checkReduction(f.program, f.proof(), f.enumeration, 0, none, nullptr, r, f.options,
                                        z3Backend);
    CHECK(v.status() == S::Verified && v.tier() == T::Symbolic);
    CHECK(v.logicalTier() == T::Proposed); // No G3b claim over segments.
    CHECK(v.computationSymbolic().status() == S::Verified);
    CHECK(v.integerReorder().status == FactStatus::Proven); // Two wrapping adds commute.
    CHECK(v.trivialReorder().status != FactStatus::Proven);
    // The proposer finds the same shape on its own.
    std::string why;
    auto proposed = proposeSegmentedDomain(f.program, f.enumeration, 0, nullptr, ProposalLimits(), why);
    if (!proposed)
      printf("sib_same proposal: %s\n", why.c_str());
    CHECK(proposed && proposed->segments.size() == 2);
    if (proposed)
      expectVerified("sib_same proposed", f, *proposed);

    // Adversaries on the same program.
    DomainRequest swapped = r;
    std::swap(swapped.segments[0], swapped.segments[1]);
    expectNotVerified("segments swapped", f, swapped, S::Refuted, "follows an event of segment");
    DomainRequest missing = r;
    missing.segments.pop_back();
    expectNotVerified("a site in no segment", f, missing, S::Unknown, "is in no segment");
    DomainRequest twice = r;
    twice.segments[1].sites = {0, 1};
    expectNotVerified("a site in two segments", f, twice, S::Unknown, "two segments");
    DomainRequest outside = r;
    outside.segments[1].sites = {1, 7};
    expectNotVerified("a site outside the group", f, outside, S::Unknown, "outside the group");
    DomainRequest withFamily = r;
    CoordinateFamily extra;
    extra.axes[AxisI] = {terms.state(l[0], w)};
    CheckedDomain fam = checkDomainFinite(f.program, f.enumeration, 0, extra, withFamily, f.options);
    CHECK(fam.status() == S::Unknown && fam.reason().find("own families") != std::string::npos);
    DomainRequest longer = r;
    longer.segments[1].order.levels[0].upper = terms.binary(TermOp::Add, n, terms.bv(APInt(16, 1)));
    expectNotVerified("second segment too long", f, longer, S::Refuted, "has no event");
    DomainRequest shorter = r;
    shorter.segments[1].order.levels[0].upper = terms.binary(TermOp::Sub, n, terms.bv(APInt(16, 1)));
    expectNotVerified("second segment too short", f, shorter, S::Refuted, "no point of the set");
    DomainRequest descending = r;
    descending.segments[1].order.levels[0].descending = true;
    expectNotVerified("second segment reversed", f, descending, S::Refuted, "increasing order");
    DomainRequest merged;
    merged.segments = {segmentOf(f.program, {0, 1}, {plain(f.program, l[0], zero, n)})};
    // One segment for both sites: the first loop's variable is frozen in the second.
    expectNotVerified("one segment for two loops", f, merged, S::Refuted);
    // Binding tells segmented requests apart.
    CHECK(fin.binding() != checkDomainFinite(f.program, f.enumeration, 0, none, longer, f.options).binding());
    CHECK(fin.binding() != checkDomainFinite(f.program, f.enumeration, 0, none, swapped, f.options).binding());
  }
  // Disjoint halves: no shared cell, on both routes.
  {
    Fixture f = fixture(m, "sib_halves", 1, 3);
    std::vector<unsigned> l = loops(f.program);
    TermContext &terms = *f.program.terms;
    DomainRequest r;
    r.segments = {segmentOf(f.program, {0}, {plain(f.program, l[0], terms.bv(APInt(16, 0)), terms.param(0, w))}),
                  segmentOf(f.program, {1}, {plain(f.program, l[1], terms.bv(APInt(16, 0)), terms.param(0, w))})};
    auto [fin, sym] = expectVerified("sib_halves", f, r);
    CHECK(fin.sharedCells().status == FactStatus::Proven && sym.sharedCells().status == FactStatus::Proven);
    CHECK(fin.singleContribution().status == FactStatus::Proven && sym.singleContribution().status == FactStatus::Proven);
    ReductionVerdict v = checkReduction(f.program, f.proof(), f.enumeration, 0, none, nullptr, r, f.options, z3Backend);
    CHECK(v.status() == S::Verified && v.trivialReorder().status == FactStatus::Proven);
  }
  // The second loop reversed: the level may be j ascending, or 3 - j descending; 3 - j
  // declared ascending is refuted.
  {
    Fixture f = fixture(m, "sib_reversed", 1, 3);
    std::vector<unsigned> l = loops(f.program);
    TermContext &terms = *f.program.terms;
    Term zero = terms.bv(APInt(16, 0)), n = terms.param(0, w);
    DomainRequest r;
    r.segments = {segmentOf(f.program, {0}, {plain(f.program, l[0], zero, n)}),
                  segmentOf(f.program, {1}, {plain(f.program, l[1], zero, n)})};
    expectVerified("sib_reversed, j", f, r);
    Term jj = terms.binary(TermOp::Sub, terms.bv(APInt(16, 3)), terms.state(l[1], w));
    DomainRequest flipped = r;
    flipped.segments[1] = segmentOf(f.program, {1},
                                    {LevelSpec{jj, terms.binary(TermOp::Sub, terms.bv(APInt(16, 4)), n),
                                               terms.bv(APInt(16, 4)), true, false}});
    expectVerified("sib_reversed, 3 - j descending", f, flipped);
    DomainRequest wrong = flipped;
    wrong.segments[1].order.levels[0].descending = false;
    expectNotVerified("3 - j declared ascending", f, wrong, S::Refuted, "increasing order");
  }
  // A guard around the second loop: its set is empty where the guard is false.
  {
    Fixture f = fixture(m, "sib_conditional", 1, 3);
    std::vector<unsigned> l = loops(f.program);
    TermContext &terms = *f.program.terms;
    Term zero = terms.bv(APInt(16, 0)), n = terms.param(0, w);
    Term guard = terms.icmp(Predicate::Ugt, n, terms.bv(APInt(16, 1)));
    DomainRequest r;
    r.segments = {segmentOf(f.program, {0}, {plain(f.program, l[0], zero, n)}),
                  segmentOf(f.program, {1}, {plain(f.program, l[1], zero, n)}, guard)};
    expectVerified("sib_conditional", f, r);
    DomainRequest unguarded = r;
    unguarded.segments[1].order.guard = terms.boolean(true);
    expectNotVerified("guard dropped", f, unguarded, S::Refuted, "has no event");
    // Also as the proposer sees it.
    std::string why;
    auto proposed = proposeSegmentedDomain(f.program, f.enumeration, 0, nullptr, ProposalLimits(), why);
    if (!proposed)
      printf("sib_conditional proposal: %s\n", why.c_str());
    CHECK(proposed.has_value());
    if (proposed)
      expectVerified("sib_conditional proposed", f, *proposed);
  }
  // An empty first segment, and an empty second one, inside the scope.
  for (const char *name : {"sib_empty_first", "sib_empty_second"}) {
    bool firstEmpty = std::string(name) == "sib_empty_first";
    Fixture f = fixture(m, name, 1, 4);
    std::vector<unsigned> l = loops(f.program);
    TermContext &terms = *f.program.terms;
    Term zero = terms.bv(APInt(16, 0)), n = terms.param(0, w);
    DomainRequest r;
    if (firstEmpty)
      r.segments = {segmentOf(f.program, {0}, {plain(f.program, l[0], n, terms.bv(APInt(16, 3)))}),
                    segmentOf(f.program, {1}, {plain(f.program, l[1], zero, terms.bv(APInt(16, 2)))})};
    else
      r.segments = {segmentOf(f.program, {0}, {plain(f.program, l[0], zero, terms.bv(APInt(16, 2)))}),
                    segmentOf(f.program, {1}, {plain(f.program, l[1], n, terms.bv(APInt(16, 3)))})};
    auto [fin, sym] = expectVerified(name, f, r);
    // At n = 3 or 4 the first set is empty; at n >= 3 the second. Both routes agree.
    bool sawEmpty = false;
    for (const DomainPointFacts &point : fin.points())
      sawEmpty = sawEmpty || point.points < 5;
    CHECK(sawEmpty);
    std::string why;
    auto proposed = proposeSegmentedDomain(f.program, f.enumeration, 0, nullptr, ProposalLimits(), why);
    if (!proposed)
      printf("%s proposal: %s\n", name, why.c_str());
    CHECK(proposed.has_value());
    if (proposed)
      expectVerified(name, f, *proposed);
  }
  // A second segment with one event in the whole scope: the checker judges the request,
  // the proposer has nothing to fit and says so.
  {
    Fixture f = fixture(m, "sib_empty_single", 1, 4);
    std::vector<unsigned> l = loops(f.program);
    TermContext &terms = *f.program.terms;
    Term zero = terms.bv(APInt(16, 0)), n = terms.param(0, w);
    DomainRequest r;
    r.segments = {segmentOf(f.program, {0}, {plain(f.program, l[0], zero, terms.bv(APInt(16, 2)))}),
                  segmentOf(f.program, {1}, {plain(f.program, l[1], n, terms.bv(APInt(16, 2)))})};
    expectVerified("sib_empty_single", f, r);
    std::string why;
    auto proposed = proposeSegmentedDomain(f.program, f.enumeration, 0, nullptr, ProposalLimits(), why);
    CHECK(!proposed && why.find("no state tells apart") != std::string::npos);
  }
  // A counter carried from the first loop into the second through SSA.
  {
    Fixture f = fixture(m, "sib_counter", 1, 3);
    std::vector<unsigned> l = loops(f.program);
    CHECK(l.size() == 4); // i, p, j, p2.
    TermContext &terms = *f.program.terms;
    Term zero = terms.bv(APInt(16, 0)), n = terms.param(0, w);
    // By the loop variables.
    DomainRequest r;
    r.segments = {segmentOf(f.program, {0}, {plain(f.program, l[0], zero, n)}),
                  segmentOf(f.program, {1}, {plain(f.program, l[2], zero, n)})};
    expectVerified("sib_counter, loop variables", f, r);
    // By the counter itself: p in [0, n), then p2 in [n, 2n).
    DomainRequest counted;
    counted.segments = {segmentOf(f.program, {0}, {plain(f.program, l[1], zero, n)}),
                        segmentOf(f.program, {1}, {plain(f.program, l[3], n, terms.binary(TermOp::Add, n, n))})};
    {
      auto [fin, sym] = both(f, counted);
      bool ok = fin.status() == S::Verified && fin.tier() == T::ScopeEnumerated && fin.exactOrder() &&
                sym.status() == S::Unknown && sym.reason().find("D2") != std::string::npos;
      if (!ok)
        describe("sib_counter, counter", fin, sym);
      CHECK(ok);
      CHECK(fin.sharedCells().status == FactStatus::Proven);
    }
    std::string why;
    auto proposed = proposeSegmentedDomain(f.program, f.enumeration, 0, nullptr, ProposalLimits(), why);
    if (!proposed)
      printf("sib_counter proposal: %s\n", why.c_str());
    CHECK(proposed.has_value());
    if (proposed)
      expectVerified("sib_counter proposed", f, *proposed);
  }
  // Different bounds, and the repeat of M1 inside a segment.
  {
    Fixture f = fixture(m, "sib_bounds", 1, 3);
    std::vector<unsigned> l = loops(f.program);
    TermContext &terms = *f.program.terms;
    DomainRequest r;
    r.segments = {segmentOf(f.program, {0}, {plain(f.program, l[0], terms.bv(APInt(16, 0)), terms.param(0, w))}),
                  segmentOf(f.program, {1}, {plain(f.program, l[1], terms.bv(APInt(16, 0)), terms.bv(APInt(16, 3)))})};
    expectVerified("sib_bounds", f, r);
  }
  {
    Fixture f = fixture(m, "sib_repeat", 1, 3);
    std::vector<unsigned> l = loops(f.program);
    TermContext &terms = *f.program.terms;
    Term zero = terms.bv(APInt(16, 0)), n = terms.param(0, w);
    // Segment 2: j logical, t repeat in [0, j + 1): μ(j) = j + 1 within the segment.
    Term jVar = levelVariable(terms, 0, w);
    DomainRequest r;
    r.segments = {segmentOf(f.program, {0}, {plain(f.program, l[0], zero, n)}),
                  segmentOf(f.program, {1}, {plain(f.program, l[1], zero, n),
                                             plain(f.program, l[2], zero, terms.binary(TermOp::Add, jVar, terms.bv(APInt(16, 1))))},
                            nullptr, {0, 1}, /*logical=*/1)};
    auto [fin, sym] = expectVerified("sib_repeat", f, r);
    // The checks below read the claims' segments by index. A claim that failed
    // expectVerified has none, and reading one was undefined: a sanitized run of 2026-11-13
    // stopped here, on a finite claim that the solver's limit had left Unknown. Each check
    // names the size it needs and fails as a check without it; the term is read only where
    // there is one.
    CHECK(fin.segments().size() == 2 && fin.segments()[0].multiplicity == DomainMultiplicity::Unit);
    CHECK(fin.segments().size() == 2 && fin.segments()[1].multiplicity == DomainMultiplicity::LogicalBox);
    CHECK(sym.segments().size() == 2 && sym.segments()[1].multiplicity == DomainMultiplicity::LogicalBox);
    CHECK(fin.multiplicity() == DomainMultiplicity::Unknown && fin.multiplicityTerm() == nullptr);
    std::optional<APInt> mu;
    if (sym.segments().size() == 2 && sym.segments()[1].multiplicityTerm)
      mu = evaluateLevelTerm(sym.segments()[1].multiplicityTerm, {APInt(16, 3)}, {APInt(16, 2), APInt(16, 0)});
    CHECK(mu && mu->getZExtValue() == 3);
    // The proposer alone takes t for a coordinate (Unit over (j, t)); told that i and j
    // are the logical variables, it proposes the box, and the checker certifies it.
    std::string why;
    auto flat = proposeSegmentedDomain(f.program, f.enumeration, 0, nullptr, ProposalLimits(), why);
    CHECK(flat && flat->segments.size() == 2 && flat->segments[1].order.levels.size() == 2 &&
          flat->segments[1].family.axes[AxisI].size() == 2);
    if (flat)
      expectVerified("sib_repeat proposed, t logical", f, *flat);
    CoordinateFamily logical;
    logical.axes[AxisI] = {terms.state(l[0], w), terms.state(l[1], w)};
    auto boxed = proposeSegmentedDomain(f.program, f.enumeration, 0, &logical, ProposalLimits(), why);
    if (!boxed)
      printf("sib_repeat logical proposal: %s\n", why.c_str());
    CHECK(boxed && boxed->segments.size() == 2 && boxed->segments[1].family.axes[AxisI].size() == 1);
    if (boxed) {
      auto [bf, bs] = expectVerified("sib_repeat proposed, t repeat", f, *boxed);
      CHECK(bs.segments().size() == 2 && bs.segments()[1].multiplicity == DomainMultiplicity::LogicalBox);
    }
  }
  // A level that is not monotone in the second loop: no lexicographic order, refuted;
  // and the loop variable itself, which is monotone, verified.
  {
    Fixture f = fixture(m, "sib_shuffle", 1, 2);
    std::vector<unsigned> l = loops(f.program);
    TermContext &terms = *f.program.terms;
    Term zero = terms.bv(APInt(16, 0)), n = terms.param(0, w);
    Term j = terms.state(l[1], w), two = terms.bv(APInt(16, 2));
    Term jj = terms.binary(TermOp::Add, terms.binary(TermOp::Mul, terms.binary(TermOp::URem, j, two), two),
                           terms.binary(TermOp::UDiv, j, two));
    DomainRequest r;
    r.segments = {segmentOf(f.program, {0}, {plain(f.program, l[0], zero, n)}),
                  segmentOf(f.program, {1}, {LevelSpec{jj, zero, terms.bv(APInt(16, 4)), false, false}})};
    expectNotVerified("a shuffled level", f, r, S::Refuted, "increasing order");
    DomainRequest byJ = r;
    byJ.segments[1] = segmentOf(f.program, {1}, {plain(f.program, l[1], zero, terms.bv(APInt(16, 4)))});
    expectVerified("the loop variable instead", f, byJ);
  }
  // Different operand roots: two groups, each segmented and each with its own order
  // verified, and G4a still refutes each group's computation: the segments do not go
  // around it.
  {
    Fixture f = fixture(m, "sib_roots", 1, 2);
    CHECK(f.enumeration.groups.size() == 2);
    for (unsigned g = 0; g < 2; ++g) {
      std::string why;
      auto proposed = proposeSegmentedDomain(f.program, f.enumeration, g, nullptr, ProposalLimits(), why);
      CHECK(proposed.has_value());
      if (!proposed)
        continue;
      ReductionVerdict v = checkReduction(f.program, f.proof(), f.enumeration, g, none, nullptr, *proposed,
                                          f.options, z3Backend);
      CHECK(v.domainSymbolic().status() == S::Verified && v.domainSymbolic().exactOrder());
      CHECK(v.computationFinite().status() == S::Refuted);
      CHECK(v.status() == S::Refuted);
    }
  }
  // A request with one segment is the G5a request in another coat: the same answers.
  {
    Module cat(catalogue());
    Fixture f = fixture(cat, "rect", 1, 2);
    std::vector<unsigned> l = loops(f.program);
    CoordinateFamily family = loopFamily(f.program, {l[0]}, {l[1]}, {l[2]});
    std::string why;
    auto flat = proposeDomain(f.program, f.enumeration, 0, family, nullptr, ProposalLimits(), why);
    CHECK(flat.has_value());
    if (flat) {
      DomainRequest one;
      Segment segment;
      segment.sites = f.enumeration.groups[0].sites;
      segment.family = family;
      segment.order = flat->order;
      segment.cellKey = flat->cellKey;
      segment.access = flat->access;
      one.segments = {segment};
      CheckedDomain a = checkDomainSymbolic(f.program, f.proof(), f.enumeration.groups[0], family, *flat, f.options, z3Backend);
      CheckedDomain b = checkDomainSymbolic(f.program, f.proof(), f.enumeration.groups[0], none, one, f.options, z3Backend);
      CHECK(a.status() == S::Verified && b.status() == S::Verified);
      CHECK(b.segments().size() == 1 && b.segments()[0].cellKey.status == a.cellKey().status &&
            b.segments()[0].outputInjective.status == a.outputInjective().status);
      CHECK(b.singleContribution().status == a.singleContribution().status);
    }
  }
  testSegmentGates();
}

// G5b-M4-0: one group gate for G3b, G4a and G5a, cells as a root and an index, a
// whole-scope tier that needs every point, and a candidate's refutation kept apart from
// the program's.
static const char *identitySource = R"(func.func @two_groups(%a: memref<8xi32>, %b: memref<8xi32>, %c: memref<8xi32>, %d: memref<8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  scf.for %i = %c0 to %n step %c1 {
    %x = memref.load %a[%i] : memref<8xi32>
    %y = memref.load %b[%i] : memref<8xi32>
    %z = memref.load %c[%i] : memref<8xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<8xi32>
  }
  scf.for %j = %c0 to %n step %c1 {
    %x2 = memref.load %a[%j] : memref<8xi32>
    %y2 = memref.load %b[%j] : memref<8xi32>
    %z2 = memref.load %c[%j] : memref<8xi32>
    %m2 = arith.muli %x2, %y2 : i32
    %s2 = arith.addi %z2, %m2 : i32
    memref.store %s2, %c[%j] : memref<8xi32>
  }
  scf.for %k = %c0 to %n step %c1 {
    %x3 = memref.load %a[%k] : memref<8xi32>
    %y3 = memref.load %b[%k] : memref<8xi32>
    %z3 = memref.load %d[%k] : memref<8xi32>
    %m3 = arith.muli %x3, %y3 : i32
    %s3 = arith.addi %z3, %m3 : i32
    memref.store %s3, %d[%k] : memref<8xi32>
  }
  return
}
func.func @fresh_cells(%a: memref<8xi32>, %b: memref<8xi32>, %c: memref<8xi32>, %d: memref<8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %f = memref.alloc() : memref<8xi32>
  %v = arith.constant 7 : i32
  memref.store %v, %f[%c0] : memref<8xi32>
  scf.for %i = %c0 to %n step %c1 {
    %x = memref.load %a[%i] : memref<8xi32>
    %y = memref.load %b[%i] : memref<8xi32>
    %z = memref.load %c[%i] : memref<8xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<8xi32>
  }
  %q = memref.load %c[%c0] : memref<8xi32>
  memref.store %q, %d[%c0] : memref<8xi32>
  return
}
func.func @dyn_extent(%a: memref<4xi32>, %b: memref<4xi32>, %c: memref<?xi32>) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %e = memref.dim %c, %c0 : memref<?xi32>
  scf.for %i = %c0 to %e step %c1 {
    %x = memref.load %a[%c0] : memref<4xi32>
    %y = memref.load %b[%c0] : memref<4xi32>
    %z = memref.load %c[%i] : memref<?xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<?xi32>
  }
  return
}
func.func @c0_shift(%a: memref<8xi32>, %b: memref<8xi32>, %c: memref<8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  scf.for %i = %c0 to %n step %c1 {
    %i1 = arith.addi %i, %c1 : index
    %x = memref.load %a[%i] : memref<8xi32>
    %y = memref.load %b[%i] : memref<8xi32>
    %z = memref.load %c[%i1] : memref<8xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<8xi32>
  }
  return
}
)";

static void testIdentity() {
  Module m(std::string(identitySource) + siblingSource);
  Sort w = Sort::bv(16);
  CoordinateFamily none;
  DomainRequest nothing;

  // 1. The group gate, at site id 0, at the last id and at the first id past it, the same
  // for G3b, G4a and G5a. A refused group gets the gate's own words from each checker; a
  // whole one never does.
  {
    Fixture f = fixture(m, "two_groups", 1, 2);
    const SemanticProgram &p = f.program;
    CHECK(p.sites.size() == 3 && f.enumeration.groups.size() == 2);
    unsigned last = p.sites.size() - 1, past = p.sites.size();
    const SiteGroup &first = f.enumeration.groups[0], &second = f.enumeration.groups[1];
    CHECK(first.sites == std::vector<unsigned>({0, 1}) && second.sites == std::vector<unsigned>({last}));
    auto with = [&](const SiteGroup &roots, std::vector<unsigned> sites) {
      SiteGroup out = roots;
      out.sites = std::move(sites);
      return out;
    };
    SymbolicCheckOptions coordinates;
    coordinates.scope = f.options.scope;
    coordinates.termination = f.options.termination;
    coordinates.authorized = f.options.authorized;
    ComputationOptions computation;
    computation.scope = f.options.scope;
    computation.termination = f.options.termination;
    computation.authorized = f.options.authorized;
    computation.limits.maxQueries = 4000;
    auto reasons = [&](const SiteGroup &group) {
      return std::vector<std::string>{
          checkFamilySymbolic(p, f.proof(), group, none, nullptr, coordinates, z3Backend).reason(),
          checkComputationSymbolic(p, f.proof(), group, computation, z3Backend).reason(),
          checkDomainSymbolic(p, f.proof(), group, none, nothing, f.options, z3Backend).reason()};
    };
    const char *gateWords[] = {"the group has no output in memory", "the group has no sites",
                               "the group names a site the program does not have",
                               "a site is listed twice", "the group is not the sites of its roots"};
    struct Case {
      const char *what;
      SiteGroup group;
      const char *refusal; // Null: whole.
    };
    SiteGroup unsupported = first, noOutput = first;
    unsupported.unsupported = "a scalar accumulator has no exact output view";
    noOutput.output.reset();
    std::vector<Case> cases = {
        {"the first group", first, nullptr},
        {"the last site's group", second, nullptr},
        {"site 0 alone", with(first, {0}), "the group is not the sites of its roots"},
        {"the last site under the first group's roots", with(first, {0, 1, last}),
         "the group is not the sites of its roots"},
        {"the first id past the program", with(first, {0, 1, past}),
         "the group names a site the program does not have"},
        {"only an id past the program", with(second, {past}),
         "the group names a site the program does not have"},
        {"site 0 twice", with(first, {0, 1, 0}), "a site is listed twice"},
        {"the last site twice", with(second, {last, last}), "a site is listed twice"},
        {"an id past the program before a site twice", with(first, {0, 0, past}),
         "the group names a site the program does not have"},
        {"no site", with(first, {}), "the group has no sites"},
        {"no output", noOutput, "the group has no output in memory"},
        {"unsupported", unsupported, "a scalar accumulator has no exact output view"},
    };
    for (const Case &c : cases) {
      std::string why;
      bool whole = wholeGroup(p, c.group, why);
      bool ok = c.refusal ? !whole && why == c.refusal : whole && why.empty();
      if (!ok)
        printf("%s: whole %d, why '%s'\n", c.what, whole, why.c_str());
      CHECK(ok);
      if (!c.refusal) {
        for (const std::string &reason : reasons(c.group))
          for (const char *words : gateWords)
            CHECK(reason.find(words) == std::string::npos);
        continue;
      }
      // An id past the program reaches the checkers only once the gate has refused it.
      if (whole)
        continue;
      for (const std::string &reason : reasons(c.group)) {
        if (reason != c.refusal)
          printf("%s: a checker says '%s'\n", c.what, reason.c_str());
        CHECK(reason == c.refusal);
      }
    }
    // A set over a group with an id past the program, or with a site twice, is refused
    // the same way.
    std::vector<SiteGroup> listed{with(first, {0, 1, past}), second};
    CHECK(checkGroupSetSymbolic(p, f.proof(), listed, computation, z3Backend).reason() ==
          "the group names a site the program does not have");
    std::vector<SiteGroup> twice{with(first, {0, 1, 1}), second};
    CHECK(checkGroupSetSymbolic(p, f.proof(), twice, computation, z3Backend).reason() ==
          "a site is listed twice");

    // The finite routes take a group by its index in an enumeration. An index past the
    // groups, or a group there that is not whole, gets the same words from all three.
    auto finiteReasons = [&](const DomainEnumeration &e, unsigned g) {
      return std::vector<std::string>{checkFamilyFinite(p, e, g, none).reason(),
                                      checkComputationFinite(p, e, g, computation).reason(),
                                      checkDomainFinite(p, e, g, none, nothing, f.options).reason()};
    };
    auto altered = [&](unsigned g, std::function<void(SiteGroup &)> change) {
      DomainEnumeration e = f.enumeration;
      change(e.groups[g]);
      return e;
    };
    struct FiniteCase {
      const char *what;
      DomainEnumeration enumeration;
      unsigned group;
      const char *refusal;
    };
    unsigned groupsPast = f.enumeration.groups.size();
    std::vector<FiniteCase> finiteCases = {
        {"the first index past the groups", f.enumeration, groupsPast, "no such group"},
        {"site 0 alone", altered(0, [](SiteGroup &g) { g.sites = {0}; }), 0,
         "the group is not the sites of its roots"},
        {"an id past the program", altered(0, [&](SiteGroup &g) { g.sites.push_back(past); }), 0,
         "the group names a site the program does not have"},
        {"a site twice", altered(0, [](SiteGroup &g) { g.sites.push_back(1); }), 0,
         "a site is listed twice"},
        {"unsupported", altered(1, [](SiteGroup &g) { g.unsupported = "marked unsupported"; }), 1,
         "marked unsupported"},
    };
    for (const FiniteCase &c : finiteCases) {
      // A group the gate wrongly admitted would reach the checkers with an id past the
      // program: they are asked only once the gate has refused it.
      if (c.group < c.enumeration.groups.size()) {
        std::string why;
        bool whole = wholeGroup(p, c.enumeration.groups[c.group], why);
        CHECK(!whole && why == c.refusal);
        if (whole)
          continue;
      }
      for (const std::string &reason : finiteReasons(c.enumeration, c.group)) {
        if (reason != c.refusal)
          printf("finite, %s: a checker says '%s'\n", c.what, reason.c_str());
        CHECK(reason == c.refusal);
      }
    }
    for (unsigned g = 0; g < groupsPast; ++g)
      for (const std::string &reason : finiteReasons(f.enumeration, g)) {
        CHECK(reason != "no such group");
        for (const char *words : gateWords)
          CHECK(reason.find(words) == std::string::npos);
      }
  }

  // 2. Cells. C[0] and D[0]: two roots, one index, two cells only when the roots are known
  // apart. One root and one index: one cell. A fresh allocation is apart from everything.
  {
    Fixture f = fixture(m, "fresh_cells", 1, 2);
    const SemanticProgram &p = f.program;
    CHECK(p.roots.size() == 5);
    unsigned c = 2, d = 3, fresh = 4;
    CHECK(p.roots[fresh].origin == MemoryRoot::FreshAllocation && p.roots[c].origin == MemoryRoot::FunctionArgument);
    auto at = [](unsigned root, std::vector<uint64_t> index) {
      CellRef out;
      out.root = root;
      for (uint64_t v : index)
        out.index.push_back(APInt(16, v));
      return out;
    };
    Authorizations apart{"roots 2 and 3 are separate"}, silent;
    CHECK(relateCells(p, apart, at(c, {0}), at(d, {0})) == CellRelation::Distinct);
    CHECK(relateCells(p, silent, at(c, {0}), at(d, {0})) == CellRelation::MayAlias);
    CHECK(relateCells(p, silent, at(d, {0}), at(c, {0})) == CellRelation::MayAlias);
    CHECK(relateCells(p, Authorizations{"roots 3 and 2 are separate"}, at(c, {0}), at(d, {0})) ==
          CellRelation::MayAlias); // The name is the lower root first, as G4a writes it.
    CHECK(relateCells(p, silent, at(c, {0}), at(fresh, {0})) == CellRelation::Distinct);
    CHECK(relateCells(p, silent, at(c, {0}), at(c, {0})) == CellRelation::Same);
    CHECK(relateCells(p, silent, at(c, {0}), at(c, {1})) == CellRelation::Distinct);
    CHECK(relateCells(p, silent, at(c, {0}), at(c, {0, 0})) == CellRelation::MayAlias);
    CellRef wide = at(c, {0});
    wide.index[0] = APInt(32, 0);
    CHECK(relateCells(p, silent, at(c, {0}), wide) == CellRelation::MayAlias);
    CHECK(relateCells(p, apart, at(c, {0}), at(9, {0})) == CellRelation::MayAlias);
    CHECK(rootRelation(p, silent, c, c) == CellRelation::Same);
    CHECK(rootRelation(p, apart, c, d) == CellRelation::Distinct && rootRelation(p, silent, c, d) == CellRelation::MayAlias);
    CHECK(cellKey(c, {APInt(16, 0)}) != cellKey(d, {APInt(16, 0)}));
    CHECK(cellKey(c, {APInt(16, 0)}) == cellKey(c, {APInt(16, 0)}));
    CHECK(cellKey(c, {APInt(16, 1)}) != cellKey(c, {APInt(16, 0)}));
    // The separation the cells read is G4a's gate, word for word.
    RootSeparation byName = rootSeparation(p, apart, d, c), byFresh = rootSeparation(p, silent, c, fresh);
    CHECK(byName.apart && byName.assumption && byName.basis == "authorized" &&
          byName.name == "roots 2 and 3 are separate");
    CHECK(byFresh.apart && !byFresh.assumption && byFresh.basis == "fresh allocation");
    CHECK(!rootSeparation(p, silent, c, d).apart && rootSeparation(p, silent, c, d).basis.empty());
    ComputationOptions computation;
    computation.scope = f.options.scope;
    computation.termination = f.options.termination;
    computation.authorized = f.options.authorized;
    CheckedComputation g4 = checkComputationFinite(p, f.enumeration, 0, computation);
    CHECK(g4.status() == S::Verified);
    unsigned matched = 0;
    for (const SeparationUse &use : g4.separations()) {
      RootSeparation same = rootSeparation(p, f.options.authorized, use.first, use.second);
      matched += same.apart && same.assumption == use.assumption && same.basis == use.basis && same.name == use.name;
    }
    CHECK(matched == g4.separations().size() && matched > 0);
    // The formula: one root, the index equality under the premise; apart, never; may
    // alias, whenever both happen.
    TermContext &terms = *p.terms;
    Term premise = terms.icmp(Predicate::Ult, terms.param(0, w), terms.bv(APInt(16, 3)));
    Term equal = terms.icmp(Predicate::Eq, terms.param(0, w), terms.bv(APInt(16, 1)));
    CHECK(sameCellFormula(terms, CellRelation::Same, premise, equal) == terms.logicalAnd(premise, equal));
    CHECK(sameCellFormula(terms, CellRelation::Distinct, premise, equal) == terms.boolean(false));
    CHECK(sameCellFormula(terms, CellRelation::MayAlias, premise, equal) == premise);
    CHECK(std::string(stringifyCellRelation(CellRelation::MayAlias)) == "MAY_ALIAS");
  }
  // One root and one index across two segments of one group: a shared cell, seen on the
  // finite route and never claimed apart on the symbolic one.
  {
    Fixture f = fixture(m, "sib_same", 1, 3);
    std::vector<unsigned> l = loops(f.program);
    TermContext &terms = *f.program.terms;
    Term zero = terms.bv(APInt(16, 0)), n = terms.param(0, w);
    DomainRequest r;
    r.segments = {segmentOf(f.program, {0}, {LevelSpec{terms.state(l[0], w), zero, n, false, false}}),
                  segmentOf(f.program, {1}, {LevelSpec{terms.state(l[1], w), zero, n, false, false}})};
    CheckedDomain finite = checkDomainFinite(f.program, f.enumeration, 0, none, r, f.options);
    CheckedDomain symbolic = checkDomainSymbolic(f.program, f.proof(), f.enumeration.groups[0], none, r,
                                                 f.options, z3Backend);
    CHECK(finite.status() == S::Verified && finite.sharedCells().status == FactStatus::Refuted);
    CHECK(symbolic.status() == S::Verified && symbolic.sharedCells().status == FactStatus::Unknown);
    CHECK(finite.singleContribution().status == FactStatus::Refuted);
  }

  // 3. The whole scope needs every point. A sample, a list with a point removed, and one
  // with a point removed and another repeated, are no whole scope for G3b, G4a or G5a.
  {
    Module cat(catalogue());
    Fixture f = fixture(cat, "rect", 1, 3);
    std::vector<unsigned> l = loops(f.program);
    CoordinateFamily family = loopFamily(f.program, {l[0]}, {l[1]}, {l[2]});
    CHECK(enumerationRanItsScope(f.enumeration) && f.enumeration.scopePoints == 3);
    CHECK(checkFamilyFinite(f.program, f.enumeration, 0, family).tier() == EvidenceTier::ScopeEnumerated);
    ComputationOptions o;
    o.scope = f.options.scope;
    o.termination = f.options.termination;
    o.authorized = f.options.authorized;
    CHECK(checkComputationFinite(f.program, f.enumeration, 0, o).tier() == T::ScopeEnumerated);
    std::string why;
    auto request = proposeDomain(f.program, f.enumeration, 0, family, nullptr, ProposalLimits(), why);
    CHECK(request.has_value());
    if (request)
      CHECK(checkDomainFinite(f.program, f.enumeration, 0, family, *request, f.options).tier() ==
            T::ScopeEnumerated);
    DomainEnumeration trimmed = f.enumeration;
    trimmed.points.pop_back();
    DomainEnumeration repeated = trimmed;
    repeated.points.push_back(repeated.points.front());
    Fixture sample = fixture(cat, "rect", 1, 3, 16, {{APInt(16, 2)}});
    for (const DomainEnumeration *e : {&trimmed, &repeated, &sample.enumeration}) {
      CHECK(!enumerationRanItsScope(*e));
      CheckedFamily g3 = checkFamilyFinite(f.program, *e, 0, family);
      CHECK(g3.status() == FamilyStatus::Verified && g3.tier() == EvidenceTier::Point);
      CheckedComputation g4 = checkComputationFinite(f.program, *e, 0, o);
      CHECK(g4.status() == S::Verified && g4.tier() == T::Point);
      if (request) {
        CheckedDomain g5 = checkDomainFinite(f.program, *e, 0, family, *request, f.options);
        CHECK(g5.status() == S::Verified && g5.tier() == T::Point);
      }
    }
    // The recorded scope is the one the enumerator ran; a point outside it is no point of it.
    DomainEnumeration moved = f.enumeration;
    moved.points.back().parameters = {APInt(16, 9)};
    CHECK(!enumerationRanItsScope(moved));
    // Each condition on its own: a list marked as a sample, or behind a closed gate, is no
    // run of the scope, whatever it holds.
    DomainEnumeration marked = f.enumeration, closed = f.enumeration;
    marked.sampled = true;
    closed.gate = GateStatus::Unknown;
    CHECK(!enumerationRanItsScope(marked) && !enumerationRanItsScope(closed));
    CHECK(!enumerationRanItsScope(DomainEnumeration()));
  }
  // A point with a negative extent lies outside the domain and is counted as run: the scope
  // here is four points, one of them outside.
  {
    Fixture f = fixture(m, "dyn_extent", -1, 2);
    CHECK(f.enumeration.gate == GateStatus::Open && f.enumeration.outsideDomain == 1 &&
          f.enumeration.points.size() == 3 && f.enumeration.scopePoints == 4);
    CHECK(enumerationRanItsScope(f.enumeration));
    DomainEnumeration trimmed = f.enumeration;
    trimmed.points.pop_back();
    CHECK(!enumerationRanItsScope(trimmed));
  }

  // 4. A flat candidate over sibling loops fails, and says only that this family fails:
  // the program's computation, domain and Theorem 13 all stand. C0 alone refutes every
  // family; a verified family refutes nothing.
  {
    Fixture f = fixture(m, "sib_same", 1, 3);
    std::vector<unsigned> l = loops(f.program);
    CoordinateFamily flat = loopFamily(f.program, {l[0]}, {}, {});
    CheckedFamily candidate = checkFamilyFinite(f.program, f.enumeration, 0, flat);
    CHECK(candidate.status() == FamilyStatus::Refuted && candidate.failingPoint().has_value());
    CHECK(candidate.refutes() == Refutes::ThisFamily);
    ComputationOptions o;
    o.scope = f.options.scope;
    o.termination = f.options.termination;
    o.authorized = f.options.authorized;
    CHECK(checkComputationFinite(f.program, f.enumeration, 0, o).status() == S::Verified);
    CHECK(checkComputationSymbolic(f.program, f.proof(), f.enumeration.groups[0], o, z3Backend).status() ==
          S::Verified);
    TermContext &terms = *f.program.terms;
    Term zero = terms.bv(APInt(16, 0)), n = terms.param(0, w);
    DomainRequest r;
    r.segments = {segmentOf(f.program, {0}, {LevelSpec{terms.state(l[0], w), zero, n, false, false}}),
                  segmentOf(f.program, {1}, {LevelSpec{terms.state(l[1], w), zero, n, false, false}})};
    ReductionVerdict verdict = checkReduction(f.program, f.proof(), f.enumeration, 0, none, nullptr, r,
                                              f.options, z3Backend);
    CHECK(verdict.status() == S::Verified && verdict.tier() == T::Symbolic);
    CHECK(verdict.domainSymbolic().status() == S::Verified);
    CHECK(std::string(stringifyRefutes(candidate.refutes())) == "THIS_FAMILY");

    Fixture shifted = fixture(m, "c0_shift", 1, 3);
    std::vector<unsigned> sl = loops(shifted.program);
    CheckedFamily every = checkFamilyFinite(shifted.program, shifted.enumeration, 0,
                                            loopFamily(shifted.program, {}, {}, {}, {sl[0]}));
    CHECK(every.status() == FamilyStatus::Refuted && every.refutes() == Refutes::EveryFamily);
    CHECK(every.reason().find("output_read_differs") != std::string::npos);

    Module cat(catalogue());
    Fixture rect = fixture(cat, "rect", 1, 2);
    std::vector<unsigned> rl = loops(rect.program);
    CheckedFamily good = checkFamilyFinite(rect.program, rect.enumeration, 0, loopFamily(rect.program, {rl[0]}, {rl[1]}, {rl[2]}));
    CHECK(good.status() == FamilyStatus::Verified && good.refutes() == Refutes::Nothing);
    // A family that reads memory is refused on both routes as this family only.
    CoordinateFamily loaded = loopFamily(rect.program, {rl[0]}, {rl[1]}, {rl[2]});
    loaded.axes[AxisI][0] = rect.program.terms->load(rect.program.terms->memVar(rect.program.roots[0].sort),
                                                     {loaded.axes[AxisI][0], loaded.axes[AxisI][0]});
    CHECK(checkFamilyFinite(rect.program, rect.enumeration, 0, loaded).refutes() == Refutes::ThisFamily);
    SymbolicCheckOptions coordinates;
    coordinates.scope = rect.options.scope;
    coordinates.termination = rect.options.termination;
    coordinates.authorized = rect.options.authorized;
    // An order, so that the route gets as far as reading the family.
    EnumerationOrder order;
    TermContext &rt = *rect.program.terms;
    order.levels.push_back({rt.state(rl[0], w), rt.bv(APInt(16, 0)), rt.param(0, w), -1});
    CheckedFamily symbolicLoaded = checkFamilySymbolic(rect.program, rect.proof(), rect.enumeration.groups[0], loaded,
                                                       &order, coordinates, z3Backend);
    CHECK(symbolicLoaded.status() == FamilyStatus::Refuted && symbolicLoaded.refutes() == Refutes::ThisFamily);
    CHECK(CheckedFamily::unknown("x").refutes() == Refutes::Nothing);
    // A coordinate undefined at an event fails this family alone, before G3a is asked.
    CoordinateFamily undefinedAt = loopFamily(rect.program, {rl[0]}, {rl[1]}, {rl[2]});
    undefinedAt.axes[AxisP].push_back(rt.binary(TermOp::UDiv, rt.state(rl[0], w), rt.bv(APInt(16, 0))));
    CheckedFamily undefinedFamily = checkFamilyFinite(rect.program, rect.enumeration, 0, undefinedAt);
    CHECK(undefinedFamily.status() == FamilyStatus::Refuted && undefinedFamily.refutes() == Refutes::ThisFamily);
    CHECK(undefinedFamily.reason().rfind("a coordinate is undefined at event", 0) == 0);
    CHECK(undefinedFamily.failingPoint().has_value());
  }
}


/// G5b-M4a: programs of several groups, run one after the other.
static const char *setSource = R"(func.func @case_b(%a: memref<8xi32>, %b: memref<8xi32>, %c: memref<8xi32>, %d: memref<8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  scf.for %i = %c0 to %n step %c1 {
    %xi = memref.load %a[%i] : memref<8xi32>
    %yi = memref.load %b[%i] : memref<8xi32>
    %zi = memref.load %c[%i] : memref<8xi32>
    %mi = arith.muli %xi, %yi : i32
    %si = arith.addi %zi, %mi : i32
    memref.store %si, %c[%i] : memref<8xi32>
  }
  scf.for %j = %c0 to %n step %c1 {
    %xj = memref.load %a[%j] : memref<8xi32>
    %yj = memref.load %b[%j] : memref<8xi32>
    %zj = memref.load %d[%j] : memref<8xi32>
    %mj = arith.muli %xj, %yj : i32
    %sj = arith.addi %zj, %mj : i32
    memref.store %sj, %d[%j] : memref<8xi32>
  }
  return
}
func.func @case_b_fresh(%a: memref<8xi32>, %b: memref<8xi32>, %c: memref<8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c8 = arith.constant 8 : index
  %zero = arith.constant 0 : i32
  %f = memref.alloc() : memref<8xi32>
  scf.for %w = %c0 to %c8 step %c1 {
    memref.store %zero, %f[%w] : memref<8xi32>
  }
  scf.for %i = %c0 to %n step %c1 {
    %xi = memref.load %a[%i] : memref<8xi32>
    %yi = memref.load %b[%i] : memref<8xi32>
    %zi = memref.load %c[%i] : memref<8xi32>
    %mi = arith.muli %xi, %yi : i32
    %si = arith.addi %zi, %mi : i32
    memref.store %si, %c[%i] : memref<8xi32>
  }
  scf.for %j = %c0 to %n step %c1 {
    %xj = memref.load %a[%j] : memref<8xi32>
    %yj = memref.load %b[%j] : memref<8xi32>
    %zj = memref.load %f[%j] : memref<8xi32>
    %mj = arith.muli %xj, %yj : i32
    %sj = arith.addi %zj, %mj : i32
    memref.store %sj, %f[%j] : memref<8xi32>
  }
  return
}
func.func @set_interleaved(%a: memref<8xi32>, %b: memref<8xi32>, %c: memref<8xi32>, %d: memref<8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  scf.for %i = %c0 to %n step %c1 {
    %x = memref.load %a[%i] : memref<8xi32>
    %y = memref.load %b[%i] : memref<8xi32>
    %z = memref.load %c[%i] : memref<8xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<8xi32>
    %x2 = memref.load %a[%i] : memref<8xi32>
    %y2 = memref.load %b[%i] : memref<8xi32>
    %z2 = memref.load %d[%i] : memref<8xi32>
    %m2 = arith.muli %x2, %y2 : i32
    %s2 = arith.addi %z2, %m2 : i32
    memref.store %s2, %d[%i] : memref<8xi32>
  }
  return
}
func.func @set_empty_second(%a: memref<8xi32>, %b: memref<8xi32>, %c: memref<8xi32>, %d: memref<8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %k = arith.subi %n, %c1 : index
  scf.for %i = %c0 to %n step %c1 {
    %xi = memref.load %a[%i] : memref<8xi32>
    %yi = memref.load %b[%i] : memref<8xi32>
    %zi = memref.load %c[%i] : memref<8xi32>
    %mi = arith.muli %xi, %yi : i32
    %si = arith.addi %zi, %mi : i32
    memref.store %si, %c[%i] : memref<8xi32>
  }
  scf.for %j = %c0 to %k step %c1 {
    %xj = memref.load %a[%j] : memref<8xi32>
    %yj = memref.load %b[%j] : memref<8xi32>
    %zj = memref.load %d[%j] : memref<8xi32>
    %mj = arith.muli %xj, %yj : i32
    %sj = arith.addi %zj, %mj : i32
    memref.store %sj, %d[%j] : memref<8xi32>
  }
  return
}
func.func @set_never(%a: memref<8xi32>, %b: memref<8xi32>, %c: memref<8xi32>, %d: memref<8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  scf.for %i = %c0 to %n step %c1 {
    %xi = memref.load %a[%i] : memref<8xi32>
    %yi = memref.load %b[%i] : memref<8xi32>
    %zi = memref.load %c[%i] : memref<8xi32>
    %mi = arith.muli %xi, %yi : i32
    %si = arith.addi %zi, %mi : i32
    memref.store %si, %c[%i] : memref<8xi32>
  }
  scf.for %j = %c0 to %c0 step %c1 {
    %xj = memref.load %a[%j] : memref<8xi32>
    %yj = memref.load %b[%j] : memref<8xi32>
    %zj = memref.load %d[%j] : memref<8xi32>
    %mj = arith.muli %xj, %yj : i32
    %sj = arith.addi %zj, %mj : i32
    memref.store %sj, %d[%j] : memref<8xi32>
  }
  return
}
)";

/// G5b-M4a: one domain claim over the union of several groups. Theorem 15 over the union's
/// sites; the set gate (XS); segments each inside one group (XP); cells of two roots
/// compared only through their relation (CI); the scope; the solver's answers; and the
/// claim of a set of one group against the group's own.
static void testSetDomain() {
  Module m(std::string(setSource) + siblingSource);
  Sort w = Sort::bv(16);
  CoordinateFamily none;
  auto level = [&](const SemanticProgram &p, unsigned var, Term lower, Term upper) {
    return LevelSpec{p.terms->state(var, w), lower, upper, false, false};
  };
  auto finite = [&](Fixture &f, std::vector<unsigned> groups, const DomainRequest &r, const DomainOptions &o) {
    return checkSetDomainFinite(f.program, f.enumeration, groups, r, o);
  };
  auto symbolic = [&](Fixture &f, std::vector<unsigned> groups, const DomainRequest &r, const DomainOptions &o,
                      SmtBackend &backend) {
    std::vector<SiteGroup> listed;
    for (unsigned g : groups)
      listed.push_back(f.enumeration.groups[g]);
    return checkSetDomainSymbolic(f.program, f.proof(), listed, r, o, backend);
  };
  auto say = [](const char *what, const CheckedSetDomain &d) {
    printf("%s: %s %s (%s)\n", what, stringifyComputationStatus(d.status()), stringifyComputationTier(d.tier()),
           d.reason().c_str());
  };
  auto expect = [&](const char *what, const CheckedSetDomain &d, ComputationStatus status, const std::string &words) {
    bool ok = d.status() == status && d.reason().find(words) != std::string::npos;
    if (!ok)
      say(what, d);
    CHECK(ok);
  };
  // Two segments, each one site and one loop variable over [0, upper).
  auto twoSegments = [&](Fixture &f, unsigned firstSite, unsigned firstVar, unsigned secondSite,
                         unsigned secondVar, Term secondUpper = nullptr) {
    TermContext &terms = *f.program.terms;
    Term zero = terms.bv(APInt(16, 0)), n = terms.param(0, w);
    DomainRequest r;
    r.segments = {segmentOf(f.program, {firstSite}, {level(f.program, firstVar, zero, n)}),
                  segmentOf(f.program, {secondSite}, {level(f.program, secondVar, zero, secondUpper ? secondUpper : n)})};
    return r;
  };

  // Case A, the brief's first numeric case: two groups write the same cells of C. At n = 2
  // the union has exactly four events, two of each group; the events are disjoint and the
  // cells are not, and the claim says each for what it is.
  {
    Fixture f = fixture(m, "sib_roots", 1, 3);
    std::vector<unsigned> l = loops(f.program);
    CHECK(f.enumeration.groups.size() == 2 && l.size() == 2);
    CHECK(f.enumeration.groups[0].sites == std::vector<unsigned>({0}) &&
          f.enumeration.groups[1].sites == std::vector<unsigned>({1}));
    DomainRequest r = twoSegments(f, 0, l[0], 1, l[1]);
    CheckedSetDomain fin = finite(f, {0, 1}, r, f.options), sym = symbolic(f, {0, 1}, r, f.options, z3Backend);
    if (fin.status() != S::Verified || sym.status() != S::Verified) {
      say("case A finite", fin);
      say("case A symbolic", sym);
    }
    CHECK(fin.status() == S::Verified && fin.tier() == T::ScopeEnumerated && fin.exactOrder());
    CHECK(sym.status() == S::Verified && sym.tier() == T::Symbolic && sym.exactOrder());
    unsigned atTwo = 0;
    for (const DomainPointFacts &point : fin.points())
      if (point.parameters[0] == 2) {
        ++atTwo;
        CHECK(point.events == 4 && point.points == 4 && point.cells == 2);
      }
    CHECK(atTwo == 1 && fin.points().size() == 3);
    CHECK(fin.groups().size() == 2 && fin.segmentGroups() == std::vector<unsigned>({0, 1}));
    CHECK(fin.groups()[0].sites == std::vector<unsigned>({0}) && fin.groups()[1].sites == std::vector<unsigned>({1}));
    CHECK(fin.rootPairs().empty() && sym.rootPairs().empty());
    CHECK(fin.sharedCells().status == FactStatus::Refuted && fin.sharedCells().point.has_value());
    CHECK(fin.singleContribution().status == FactStatus::Refuted);
    CHECK(sym.sharedCells().status == FactStatus::Unknown && sym.singleContribution().status == FactStatus::Unknown);
    for (const CheckedSetDomain *d : {&fin, &sym}) {
      CHECK(d->segments().size() == 2);
      for (const SegmentFacts &segment : d->segments())
        CHECK(segment.cellKey.status == FactStatus::Proven && segment.outputInjective.status == FactStatus::Proven);
    }
    CHECK(fin.binding().find(f.program.fingerprint + "|set|") == 0 && fin.binding() == sym.binding());
    // Two sets of other groups are two claims, whatever the request.
    CHECK(finite(f, {0}, r, f.options).binding() != finite(f, {1}, r, f.options).binding());

    // The listing's order changes nothing.
    CheckedSetDomain back = finite(f, {1, 0}, r, f.options), backSym = symbolic(f, {1, 0}, r, f.options, z3Backend);
    CHECK(back.binding() == fin.binding() && back.status() == fin.status() && back.tier() == fin.tier() &&
          back.reason() == fin.reason() && back.segmentGroups() == fin.segmentGroups() &&
          back.sharedCells().status == fin.sharedCells().status);
    CHECK(backSym.binding() == sym.binding() && backSym.status() == sym.status() &&
          backSym.records().size() == sym.records().size());

    // The groups run in the other order than the segments say: refuted at a point, and no
    // symbolic claim.
    TermContext &terms = *f.program.terms;
    Term zero = terms.bv(APInt(16, 0)), n = terms.param(0, w), one = terms.bv(APInt(16, 1));
    DomainRequest backward;
    backward.segments = {segmentOf(f.program, {1}, {level(f.program, l[1], zero, n)}),
                         segmentOf(f.program, {0}, {level(f.program, l[0], zero, n)})};
    CheckedSetDomain wrong = finite(f, {0, 1}, backward, f.options);
    expect("case A backwards", wrong, S::Refuted, "follows an event of segment");
    CHECK(wrong.refutation().point.has_value() && !wrong.exactOrder());
    CheckedSetDomain wrongSym = symbolic(f, {0, 1}, backward, f.options, z3Backend);
    CHECK(wrongSym.status() == S::Unknown && !wrongSym.exactOrder());
    // A segment whose set misses an event, and one whose set has a point with no event.
    DomainRequest missing = twoSegments(f, 0, l[0], 1, l[1], terms.binary(TermOp::Sub, n, one));
    expect("an event outside its set", finite(f, {0, 1}, missing, f.options), S::Refuted, "an event is no point of the set");
    CHECK(symbolic(f, {0, 1}, missing, f.options, z3Backend).status() == S::Unknown);
    DomainRequest extra = twoSegments(f, 0, l[0], 1, l[1], terms.binary(TermOp::Add, n, one));
    expect("a point with no event", finite(f, {0, 1}, extra, f.options), S::Refuted, "a point of the set has no event");
    CHECK(symbolic(f, {0, 1}, extra, f.options, z3Backend).status() == S::Unknown);

    // The scope: a sample and a list with a point removed are no whole scope.
    DomainEnumeration trimmed = f.enumeration;
    trimmed.points.pop_back();
    CheckedSetDomain part = checkSetDomainFinite(f.program, trimmed, {0, 1}, r, f.options);
    CHECK(part.status() == S::Verified && part.tier() == T::Point);
    Fixture sample = fixture(m, "sib_roots", 1, 3, 16, {{APInt(16, 2)}});
    std::vector<unsigned> sl = loops(sample.program);
    CheckedSetDomain sampled = finite(sample, {0, 1}, twoSegments(sample, 0, sl[0], 1, sl[1]), sample.options);
    CHECK(sampled.status() == S::Verified && sampled.tier() == T::Point);

    // The solver: an unknown or sat answer to any obligation of Theorem 15 is no claim; an
    // unknown on a fact leaves that fact open and the claim standing; a limit is Unknown.
    for (const char *id : {"; D1 ", "; D2 ", "; D3 ", "; D4 "})
      for (SmtAnswer answer : {SmtAnswer::Unknown, SmtAnswer::Sat}) {
        Selective solver(id, answer);
        CheckedSetDomain out = symbolic(f, {0, 1}, r, f.options, solver);
        if (solver.hits == 0)
          printf("no %s query on the union\n", id);
        CHECK(solver.hits > 0 && out.status() == S::Unknown && !out.exactOrder());
      }
    for (const char *id : {"; SH ", "; T1 "}) {
      Selective solver(id, SmtAnswer::Unknown);
      CheckedSetDomain out = symbolic(f, {0, 1}, r, f.options, solver);
      CHECK(solver.hits > 0 && out.status() == S::Verified);
      CHECK(out.sharedCells().status == FactStatus::Unknown && out.singleContribution().status == FactStatus::Unknown);
    }
    {
      Selective liar("; SH ", SmtAnswer::Unsat);
      CheckedSetDomain out = symbolic(f, {0, 1}, r, f.options, liar);
      CHECK(liar.hits > 0 && out.status() == S::Verified);
    }
    DomainOptions few = f.options;
    few.limits.maxQueries = 1;
    expect("query limit", symbolic(f, {0, 1}, r, few, z3Backend), S::Unknown, "query limit");
    DomainOptions narrow = f.options;
    narrow.limits.maxPairs = 1;
    expect("pair limit", symbolic(f, {0, 1}, r, narrow, z3Backend), S::Unknown, "more arrival pairs than the limit");
    DomainOptions zero0 = f.options;
    zero0.termination.solver.timeoutMilliseconds = 0;
    expect("zero limit", symbolic(f, {0, 1}, r, zero0, z3Backend), S::Unknown, "a solver limit of zero is no limit");
    std::vector<SiteGroup> listed{f.enumeration.groups[0], f.enumeration.groups[1]};
    expect("no certificate", checkSetDomainSymbolic(f.program, nullptr, listed, r, f.options, z3Backend), S::Unknown,
           "no termination certificate");

    // The gates, on both routes and in their words.
    auto refusedBoth = [&](const char *what, std::vector<unsigned> groups, const DomainRequest &req,
                           const std::string &words) {
      expect(what, finite(f, groups, req, f.options), S::Unknown, words);
      expect(what, symbolic(f, groups, req, f.options, z3Backend), S::Unknown, words);
    };
    DomainRequest mixed;
    mixed.segments = {segmentOf(f.program, {0, 1}, {level(f.program, l[0], zero, n)})};
    refusedBoth("a segment of two groups", {0, 1}, mixed, "the request is refused: segment 0 mixes groups");
    CHECK(finite(f, {0, 1}, mixed, f.options).segmentGroups().empty());
    refusedBoth("a site outside the set", {0}, r, "the request is refused: a segment names a site outside the set");
    DomainRequest firstOnly;
    firstOnly.segments = {r.segments[0]};
    refusedBoth("a site in no segment", {0, 1}, firstOnly, "the request is refused: a site of the set is in no segment");
    DomainRequest emptySegment = r;
    emptySegment.segments.push_back(segmentOf(f.program, {}, {level(f.program, l[1], zero, n)}));
    refusedBoth("an empty segment", {0, 1}, emptySegment, "the request is refused: a segment with no site");
    refusedBoth("a group twice", {0, 0}, r, "a group is listed twice");
    refusedBoth("no group", {}, r, "no group is listed");
    refusedBoth("a flat request", {0, 1}, DomainRequest(), "a claim over a set needs a segmented request");
    expect("a group past the enumeration", finite(f, {0, 2}, r, f.options), S::Unknown, "no such group");
    DomainEnumeration closed = f.enumeration;
    closed.gate = GateStatus::Unknown;
    closed.reason = "closed by the test";
    expect("a closed gate", checkSetDomainFinite(f.program, closed, {0, 1}, r, f.options), S::Unknown,
           "the domain gate is closed: closed by the test");
    SiteGroup twice = f.enumeration.groups[0];
    twice.sites.push_back(twice.sites.front());
    expect("a site twice", checkSetDomainSymbolic(f.program, f.proof(), {twice, f.enumeration.groups[1]}, r, f.options,
                                                  z3Backend), S::Unknown, "a site is listed twice");
    SiteGroup past = f.enumeration.groups[0];
    past.sites.push_back(77);
    expect("an id past the program", checkSetDomainSymbolic(f.program, f.proof(), {past}, r, f.options, z3Backend),
           S::Unknown, "the group names a site the program does not have");
    std::vector<SiteGroup> many(65, f.enumeration.groups[0]);
    expect("65 groups", checkSetDomainSymbolic(f.program, f.proof(), many, r, f.options, z3Backend), S::Unknown,
           "more than 64 groups are listed");
  }

  // Case B: the second group writes D. Index for index, C[i] and D[i] are two cells only
  // when C and D are known apart. Authorized: no cell is common, on both routes, and no
  // query is asked for it. Not authorized, or under the reversed name: the facts stay open
  // and the claim about events still stands.
  {
    Fixture f = fixture(m, "case_b", 1, 3);
    std::vector<unsigned> l = loops(f.program);
    CHECK(f.enumeration.groups.size() == 2 && l.size() == 2);
    DomainRequest r = twoSegments(f, 0, l[0], 1, l[1]);
    CheckedSetDomain fin = finite(f, {0, 1}, r, f.options), sym = symbolic(f, {0, 1}, r, f.options, z3Backend);
    CHECK(fin.status() == S::Verified && fin.tier() == T::ScopeEnumerated && sym.status() == S::Verified);
    for (const CheckedSetDomain *d : {&fin, &sym}) {
      CHECK(d->rootPairs().size() == 1);
      if (d->rootPairs().size() == 1) {
        const RootPair &pair = d->rootPairs()[0];
        CHECK(pair.first == 2 && pair.second == 3 && pair.relation == CellRelation::Distinct &&
              pair.basis == "authorized" && pair.assumption);
      }
      CHECK(d->sharedCells().status == FactStatus::Proven && d->singleContribution().status == FactStatus::Proven);
    }
    Selective quiet("; SH ", SmtAnswer::Sat);
    CheckedSetDomain asked = symbolic(f, {0, 1}, r, f.options, quiet);
    CHECK(quiet.hits == 0 && asked.sharedCells().status == FactStatus::Proven);
    for (Authorizations names : {Authorizations{}, Authorizations{"roots 3 and 2 are separate"}}) {
      DomainOptions o = f.options;
      o.authorized = names;
      CheckedSetDomain open = finite(f, {0, 1}, r, o), openSym = symbolic(f, {0, 1}, r, o, z3Backend);
      if (openSym.status() != S::Verified)
        say("case B without the name", openSym);
      CHECK(open.status() == S::Verified && open.tier() == T::ScopeEnumerated && openSym.status() == S::Verified);
      for (const CheckedSetDomain *d : {&open, &openSym}) {
        CHECK(d->rootPairs().size() == 1 && d->rootPairs()[0].relation == CellRelation::MayAlias &&
              d->rootPairs()[0].basis.empty());
        CHECK(d->sharedCells().status == FactStatus::Unknown && d->singleContribution().status == FactStatus::Unknown);
        CHECK(d->sharedCells().detail.find("roots 2 and 3 may alias") != std::string::npos);
      }
    }
  }
  // A fresh allocation is apart from every argument, as a proof, with no name given.
  {
    Fixture f = fixture(m, "case_b_fresh", 1, 3);
    std::vector<unsigned> l = loops(f.program);
    CHECK(f.enumeration.groups.size() == 2 && l.size() == 3);
    DomainRequest r = twoSegments(f, 0, l[1], 1, l[2]);
    DomainOptions o = f.options;
    o.authorized.clear();
    CheckedSetDomain fin = finite(f, {0, 1}, r, o), sym = symbolic(f, {0, 1}, r, o, z3Backend);
    if (fin.status() != S::Verified)
      say("fresh", fin);
    CHECK(fin.status() == S::Verified && sym.status() == S::Verified);
    for (const CheckedSetDomain *d : {&fin, &sym}) {
      CHECK(d->rootPairs().size() == 1 && d->rootPairs()[0].relation == CellRelation::Distinct &&
            d->rootPairs()[0].basis == "fresh allocation" && !d->rootPairs()[0].assumption);
      CHECK(d->sharedCells().status == FactStatus::Proven);
    }
  }
  // Two groups in one loop: their events interleave, so no segment order holds. A run
  // shows it; the symbolic route has no order for two sites of one block.
  {
    Fixture f = fixture(m, "set_interleaved", 1, 3);
    std::vector<unsigned> l = loops(f.program);
    CHECK(f.enumeration.groups.size() == 2 && l.size() == 1);
    DomainRequest r = twoSegments(f, 0, l[0], 1, l[0]);
    CheckedSetDomain fin = finite(f, {0, 1}, r, f.options);
    expect("interleaved", fin, S::Refuted, "follows an event of segment");
    CHECK(fin.refutation().point.has_value());
    expect("interleaved, symbolic", symbolic(f, {0, 1}, r, f.options, z3Backend), S::Unknown,
           "two sites of the set share a block");
  }
  // A segment empty at a point, and a group with no event at any point.
  {
    Fixture f = fixture(m, "set_empty_second", 1, 3);
    std::vector<unsigned> l = loops(f.program);
    TermContext &terms = *f.program.terms;
    Term less = terms.binary(TermOp::Sub, terms.param(0, w), terms.bv(APInt(16, 1)));
    DomainRequest r = twoSegments(f, 0, l[0], 1, l[1], less);
    CheckedSetDomain fin = finite(f, {0, 1}, r, f.options), sym = symbolic(f, {0, 1}, r, f.options, z3Backend);
    if (fin.status() != S::Verified || sym.status() != S::Verified) {
      say("empty second", fin);
      say("empty second, symbolic", sym);
    }
    CHECK(fin.status() == S::Verified && fin.tier() == T::ScopeEnumerated && sym.status() == S::Verified);
    for (const DomainPointFacts &point : fin.points())
      if (point.parameters[0] == 1)
        CHECK(point.events == 1 && point.points == 1);
    for (SmtAnswer answer : {SmtAnswer::Unknown, SmtAnswer::Sat}) {
      Selective solver("; D5 ", answer);
      CheckedSetDomain out = symbolic(f, {0, 1}, r, f.options, solver);
      if (solver.hits == 0)
        printf("no D5 query on the union\n");
      CHECK(solver.hits > 0 && out.status() == S::Unknown);
    }
  }
  {
    Fixture f = fixture(m, "set_never", 1, 3);
    std::vector<unsigned> l = loops(f.program);
    TermContext &terms = *f.program.terms;
    Term zero = terms.bv(APInt(16, 0));
    CHECK(f.enumeration.groups.size() == 2);
    DomainRequest never;
    never.segments = {segmentOf(f.program, {1}, {level(f.program, l[1], zero, zero)})};
    expect("a group with no event", finite(f, {1}, never, f.options), S::Unknown, "no point has events");
    DomainRequest r = twoSegments(f, 0, l[0], 1, l[1], zero);
    CheckedSetDomain fin = finite(f, {0, 1}, r, f.options);
    if (fin.status() != S::Verified)
      say("never", fin);
    CHECK(fin.status() == S::Verified && fin.tier() == T::ScopeEnumerated);
    CHECK(symbolic(f, {0, 1}, r, f.options, z3Backend).status() == S::Verified);
  }
  // A set of one group is that group's segmented claim, under its own binding.
  {
    Fixture f = fixture(m, "sib_same", 1, 3);
    std::vector<unsigned> l = loops(f.program);
    CHECK(f.enumeration.groups.size() == 1);
    DomainRequest r = twoSegments(f, 0, l[0], 1, l[1]);
    CheckedSetDomain set = finite(f, {0}, r, f.options), setSym = symbolic(f, {0}, r, f.options, z3Backend);
    CheckedDomain one = checkDomainFinite(f.program, f.enumeration, 0, none, r, f.options);
    CheckedDomain oneSym = checkDomainSymbolic(f.program, f.proof(), f.enumeration.groups[0], none, r, f.options, z3Backend);
    auto same = [&](const CheckedSetDomain &a, const CheckedDomain &b) {
      bool ok = a.status() == b.status() && a.tier() == b.tier() && a.reason() == b.reason() &&
                a.exactOrder() == b.exactOrder() && a.points().size() == b.points().size() &&
                a.segments().size() == b.segments().size() && a.records().size() == b.records().size() &&
                a.sharedCells().status == b.sharedCells().status && a.sharedCells().detail == b.sharedCells().detail &&
                a.singleContribution().status == b.singleContribution().status &&
                a.singleContribution().detail == b.singleContribution().detail && a.binding() != b.binding();
      for (size_t k = 0; ok && k < a.points().size(); ++k)
        ok = a.points()[k].events == b.points()[k].events && a.points()[k].cells == b.points()[k].cells;
      for (size_t s = 0; ok && s < a.segments().size(); ++s)
        ok = a.segments()[s].cellKey.status == b.segments()[s].cellKey.status &&
             a.segments()[s].outputInjective.status == b.segments()[s].outputInjective.status &&
             a.segments()[s].sites == b.segments()[s].sites;
      for (size_t k = 0; ok && k < a.records().size(); ++k)
        ok = a.records()[k].id == b.records()[k].id && a.records()[k].result == b.records()[k].result;
      return ok;
    };
    CHECK(same(set, one) && same(setSym, oneSym));
    CHECK(set.status() == S::Verified && set.rootPairs().empty() && set.segmentGroups() == std::vector<unsigned>({0, 0}));
  }
}


/// A loop with a step, for the congruence atoms the proposer adds at widths that hold them.
static const char *steppedSource = R"(func.func @stepped(%a: memref<8xi32>, %b: memref<8xi32>, %c: memref<8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  scf.for %i = %c0 to %n step %c2 {
    %x = memref.load %a[%i] : memref<8xi32>
    %y = memref.load %b[%i] : memref<8xi32>
    %z = memref.load %c[%i] : memref<8xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<8xi32>
  }
  return
}
)";

/// Every constant under `term` fits its width: a bit pattern wider than the sort is an
/// APInt no checker may see. Read without APInt's own assertions, so that a release build
/// checks it too.
static bool constantsFit(Term term, std::set<Term> &seen) {
  if (!term || !seen.insert(term).second)
    return true;
  if (term->op == TermOp::Const) {
    unsigned width = term->sort.width;
    if (term->value.getBitWidth() != width)
      return false;
    if (width < 64 && (term->value.getZExtValue() >> width) != 0)
      return false;
  }
  for (Term arg : term->args)
    if (!constantsFit(arg, seen))
      return false;
  return true;
}
static bool constantsFit(const DomainRequest &request) {
  std::set<Term> seen;
  bool ok = constantsFit(request.order.guard, seen);
  for (const GuardedLevel &level : request.order.levels)
    ok = ok && constantsFit(level.value, seen) && constantsFit(level.lower, seen) && constantsFit(level.upper, seen);
  for (const auto &view : request.access)
    for (Term term : view)
      ok = ok && constantsFit(term, seen);
  for (const Segment &segment : request.segments) {
    ok = ok && constantsFit(segment.order.guard, seen);
    for (const GuardedLevel &level : segment.order.levels)
      ok = ok && constantsFit(level.value, seen) && constantsFit(level.lower, seen) && constantsFit(level.upper, seen);
  }
  return ok;
}

/// The congruence atoms among `atoms`: `(y urem c) == r`, with the moduli seen.
static unsigned congruences(const std::vector<Term> &atoms, std::set<uint64_t> &moduli, bool &fit) {
  unsigned count = 0;
  std::set<Term> seen;
  for (Term atom : atoms) {
    fit = fit && constantsFit(atom, seen);
    if (atom->op != TermOp::ICmp || atom->aux != (unsigned)Predicate::Eq || atom->args.size() != 2)
      continue;
    Term left = atom->args[0];
    if (left->op != TermOp::URem)
      continue;
    if (left->args.size() == 2 && left->args[1]->op == TermOp::Const && atom->args[1]->op == TermOp::Const) {
      ++count;
      moduli.insert(left->args[1]->value.getZExtValue());
    }
  }
  return count;
}

/// The proposer's congruence atoms at widths 1, 2, 8 and 32: none where no modulus fits,
/// 2 and 3 at two bits, all three from three bits on, every constant fitting its width;
/// and the end-to-end program that once aborted a debug build, through the search.
static void testProposer() {
  Module m(std::string(siblingSource) + steppedSource);
  // The atoms, directly, for one level of each width.
  {
    Fixture f = fixture(m, "sib_roots", 1, 2);
    TermContext &terms = *f.program.terms;
    std::vector<unsigned> l = loops(f.program);
    Sort w16 = Sort::bv(16);
    Term bit = terms.icmp(Predicate::Eq, terms.state(l[0], w16), terms.bv(APInt(16, 0))); // one bit
    Term two = terms.zext(bit, 2);                                                        // two bits
    Term eight = terms.trunc(terms.state(l[0], w16), 8);
    Term wide = terms.zext(terms.state(l[0], w16), 32);
    struct Case {
      Term level;
      unsigned width;
      unsigned expected;
      std::set<uint64_t> moduli;
    };
    std::vector<Case> cases = {{bit, 1, 0, {}}, {two, 2, 5, {2, 3}}, {eight, 8, 9, {2, 3, 4}}, {wide, 32, 9, {2, 3, 4}}};
    for (const Case &c : cases) {
      CHECK(c.level->sort.width == c.width);
      GuardedOrder order;
      GuardedLevel level;
      level.value = c.level;
      level.lower = terms.bv(APInt(c.width, 0));
      level.upper = terms.bv(APInt(c.width, 1));
      order.levels.push_back(level);
      order.guard = terms.boolean(true);
      std::vector<Term> atoms = proposalConditionAtoms(f.program, order, {}, 64);
      std::set<uint64_t> moduli;
      bool fit = true;
      unsigned count = congruences(atoms, moduli, fit);
      if (count != c.expected || moduli != c.moduli || !fit)
        printf("width %u: %u congruence atoms, %zu moduli, fit %d\n", c.width, count, moduli.size(), fit);
      CHECK(count == c.expected && moduli == c.moduli && fit);
      // No modulus a width cannot hold, whatever the count says.
      for (uint64_t modulus : moduli)
        CHECK(c.width >= 64 || (modulus >> c.width) == 0);
    }
    // Two one-bit levels and a sixteen-bit one together: the wide level keeps its nine.
    GuardedOrder mixed;
    for (Term value : {bit, terms.state(l[0], w16), terms.icmp(Predicate::Ne, terms.state(l[1], w16), terms.bv(APInt(16, 0)))}) {
      GuardedLevel level;
      level.value = value;
      level.lower = terms.bv(APInt(value->sort.width, 0));
      level.upper = terms.bv(APInt(value->sort.width, 1));
      mixed.levels.push_back(level);
    }
    mixed.guard = terms.boolean(true);
    std::set<uint64_t> moduli;
    bool fit = true;
    CHECK(congruences(proposalConditionAtoms(f.program, mixed, {}, 64), moduli, fit) == 9 && fit);
    // A limit stops the atoms, with or without a one-bit level.
    CHECK(proposalConditionAtoms(f.program, mixed, {}, 0).empty());
  }
  // A proposal whose family holds a one-bit and a two-bit coordinate: a request or a
  // refusal, never an abort, and every constant of a request fits.
  {
    Fixture f = fixture(m, "sib_roots", 1, 3);
    TermContext &terms = *f.program.terms;
    std::vector<unsigned> l = loops(f.program);
    Sort w16 = Sort::bv(16);
    for (unsigned width : {1u, 2u}) {
      CoordinateFamily family = loopFamily(f.program, {l[0]}, {}, {});
      Term bit = terms.icmp(Predicate::Eq, terms.binary(TermOp::URem, terms.state(l[0], w16), terms.bv(APInt(16, 2))),
                            terms.bv(APInt(16, 0)));
      family.axes[AxisP].push_back(width == 1 ? bit : terms.zext(bit, 2));
      std::string why;
      auto request = proposeDomain(f.program, f.enumeration, 0, family, nullptr, ProposalLimits(), why);
      if (request)
        CHECK(constantsFit(*request));
      CHECK(request.has_value() || !why.empty());
    }
  }
  // The search itself, as the driver runs it: the program and scope that aborted.
  {
    Fixture f = fixture(m, "sib_roots", 1, 3);
    SynthesisOptions search;
    search.scope = f.scope;
    search.termination = f.options.termination;
    search.authorized = f.authorized;
    search.limits.maxPoints = 256;
    search.maxLevel = 3;
    search.synthesis.maxCandidates = 5;
    search.synthesis.maxRounds = 300;
    search.synthesis.solver = testSolver();
    DomainAnalysisRequest request;
    request.synthesis = search;
    request.domain = f.options;
    request.maxCandidates = 2;
    DomainResult result = analyzeDomains(f.program, f.proof(), request, z3Backend);
    CHECK(result.enumeration.gate == GateStatus::Open && result.groups.size() == 2);
    // The search finds families of comparison terms here, and the proposer refuses each
    // ("no guard from the program's conditions is exact at the samples"), before and after
    // the fix: what changed is that it now says so instead of aborting on the way.
    unsigned candidates = 0;
    for (const GroupDomain &group : result.groups)
      for (const CandidateDomain &candidate : group.candidates) {
        ++candidates;
        if (candidate.request)
          CHECK(constantsFit(*candidate.request));
        else
          CHECK(!candidate.proposalReason.empty());
      }
    CHECK(candidates > 0);
  }
  // Steps at eight and thirty-two bits: the congruence is what makes the box exact, and
  // the finite checker verifies the proposal that uses it.
  for (unsigned width : {8u, 32u}) {
    Fixture f = fixture(m, "stepped", 1, 5, width);
    std::vector<unsigned> l = loops(f.program);
    CHECK(l.size() == 1);
    CoordinateFamily family = loopFamily(f.program, {l[0]}, {}, {});
    std::string why;
    auto request = proposeDomain(f.program, f.enumeration, 0, family, nullptr, ProposalLimits(), why);
    if (!request)
      printf("stepped at %u bits: no proposal: %s\n", width, why.c_str());
    CHECK(request.has_value());
    if (!request)
      continue;
    CHECK(constantsFit(*request));
    std::set<uint64_t> moduli;
    bool fit = true;
    unsigned used = congruences({request->order.guard}, moduli, fit);
    // The guard is a conjunction or disjunction holding the atom: count congruences inside.
    std::vector<Term> leaves;
    std::function<void(Term)> flatten = [&](Term t) {
      if (t->op == TermOp::And || t->op == TermOp::Or) {
        for (Term a : t->args)
          flatten(a);
      } else {
        leaves.push_back(t);
      }
    };
    flatten(request->order.guard);
    used += congruences(leaves, moduli, fit);
    CHECK(used > 0 && moduli.count(2) && fit);
    for (uint64_t modulus : moduli)
      CHECK(modulus >> width == 0);
    CheckedDomain finite = checkDomainFinite(f.program, f.enumeration, 0, family, *request, f.options);
    if (finite.status() != S::Verified)
      printf("stepped at %u bits: %s (%s)\n", width, stringifyComputationStatus(finite.status()), finite.reason().c_str());
    CHECK(finite.status() == S::Verified && finite.tier() == T::ScopeEnumerated);
  }
}


/// G5b-M4b-1: programs for Theorem 17 over a set.
static const char *reductionSource = R"(func.func @set_repeat(%a: memref<8xi32>, %b: memref<8xi32>, %c: memref<8xi32>, %d: memref<8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  scf.for %i = %c0 to %n step %c1 {
    %xi = memref.load %a[%i] : memref<8xi32>
    %yi = memref.load %b[%i] : memref<8xi32>
    %zi = memref.load %c[%i] : memref<8xi32>
    %mi = arith.muli %xi, %yi : i32
    %si = arith.addi %zi, %mi : i32
    memref.store %si, %c[%i] : memref<8xi32>
  }
  scf.for %j = %c0 to %n step %c1 {
    scf.for %r = %c0 to %c2 step %c1 {
      %xr = memref.load %a[%j] : memref<8xi32>
      %yr = memref.load %d[%j] : memref<8xi32>
      %zr = memref.load %c[%j] : memref<8xi32>
      %mr = arith.muli %xr, %yr : i32
      %sr = arith.addi %zr, %mr : i32
      memref.store %sr, %c[%j] : memref<8xi32>
    }
  }
  return
}
func.func @set_float(%a: memref<8xf64>, %b: memref<8xf64>, %c: memref<8xf64>, %d: memref<8xf64>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  scf.for %i = %c0 to %n step %c1 {
    %xi = memref.load %a[%i] : memref<8xf64>
    %yi = memref.load %b[%i] : memref<8xf64>
    %zi = memref.load %c[%i] : memref<8xf64>
    %mi = arith.mulf %xi, %yi : f64
    %si = arith.addf %zi, %mi : f64
    memref.store %si, %c[%i] : memref<8xf64>
  }
  scf.for %j = %c0 to %n step %c1 {
    %xj = memref.load %a[%j] : memref<8xf64>
    %yj = memref.load %d[%j] : memref<8xf64>
    %zj = memref.load %c[%j] : memref<8xf64>
    %mj = arith.mulf %xj, %yj : f64
    %sj = arith.addf %zj, %mj : f64
    memref.store %sj, %c[%j] : memref<8xf64>
  }
  return
}
func.func @set_reads(%a: memref<8xi32>, %b: memref<8xi32>, %c: memref<8xi32>, %d: memref<8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  scf.for %i = %c0 to %n step %c1 {
    %xi = memref.load %a[%i] : memref<8xi32>
    %yi = memref.load %b[%i] : memref<8xi32>
    %zi = memref.load %c[%i] : memref<8xi32>
    %mi = arith.muli %xi, %yi : i32
    %si = arith.addi %zi, %mi : i32
    memref.store %si, %c[%i] : memref<8xi32>
  }
  scf.for %j = %c0 to %n step %c1 {
    %xj = memref.load %c[%j] : memref<8xi32>
    %yj = memref.load %b[%j] : memref<8xi32>
    %zj = memref.load %d[%j] : memref<8xi32>
    %mj = arith.muli %xj, %yj : i32
    %sj = arith.addi %zj, %mj : i32
    memref.store %sj, %d[%j] : memref<8xi32>
  }
  return
}
func.func @set_third(%a: memref<8xi32>, %b: memref<8xi32>, %c: memref<8xi32>, %d: memref<8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  scf.for %i = %c0 to %n step %c1 {
    %xi = memref.load %a[%i] : memref<8xi32>
    %yi = memref.load %b[%i] : memref<8xi32>
    %zi = memref.load %c[%i] : memref<8xi32>
    %mi = arith.muli %xi, %yi : i32
    %si = arith.addi %zi, %mi : i32
    memref.store %si, %c[%i] : memref<8xi32>
  }
  scf.for %j = %c0 to %n step %c1 {
    %xj = memref.load %a[%j] : memref<8xi32>
    %yj = memref.load %d[%j] : memref<8xi32>
    %zj = memref.load %c[%j] : memref<8xi32>
    %mj = arith.muli %xj, %yj : i32
    %sj = arith.addi %zj, %mj : i32
    memref.store %sj, %c[%j] : memref<8xi32>
  }
  scf.for %k = %c0 to %n step %c1 {
    %xk = memref.load %b[%k] : memref<8xi32>
    %yk = memref.load %d[%k] : memref<8xi32>
    %zk = memref.load %c[%k] : memref<8xi32>
    %mk = arith.muli %xk, %yk : i32
    %sk = arith.addi %zk, %mk : i32
    memref.store %sk, %c[%k] : memref<8xi32>
  }
  return
}
)";

/// G5b-M4b-1: Theorem 17 over a set of groups, from the composer's own claims. The ten
/// cases of the brief, the obligations B, RA, TT and VR each alone, the listing, a set of
/// one group against Theorem 13, and per-group claims never standing in for the set's.
static void testSetReduction() {
  Module m(std::string(setSource) + siblingSource + reductionSource);
  Sort w = Sort::bv(16);
  CoordinateFamily none;
  auto level = [&](const SemanticProgram &p, unsigned var, Term lower, Term upper) {
    return LevelSpec{p.terms->state(var, w), lower, upper, false, false};
  };
  // Two segments, one site and one loop variable each, over [0, upper).
  auto twoSegments = [&](Fixture &f, unsigned firstSite, unsigned firstVar, unsigned secondSite,
                         unsigned secondVar, Term secondUpper = nullptr) {
    TermContext &terms = *f.program.terms;
    Term zero = terms.bv(APInt(16, 0)), n = terms.param(0, w);
    DomainRequest r;
    r.segments = {segmentOf(f.program, {firstSite}, {level(f.program, firstVar, zero, n)}),
                  segmentOf(f.program, {secondSite}, {level(f.program, secondVar, zero, secondUpper ? secondUpper : n)})};
    return r;
  };
  // The computation options the composer derives from the domain's, as it does.
  auto compOf = [](const DomainOptions &o) {
    ComputationOptions out;
    out.scope = o.scope;
    out.termination = o.termination;
    out.authorized = o.authorized;
    out.limits = o.computation;
    return out;
  };
  auto verdict = [&](Fixture &f, std::vector<unsigned> groups, const DomainRequest &r, const DomainOptions &o,
                     SmtBackend &backend) {
    return checkSetReduction(f.program, f.proof(), f.enumeration, groups, r, o, backend);
  };
  auto say = [](const char *what, const SetReductionVerdict &v) {
    printf("%s: %s %s (%s) refuted by %s\n", what, stringifyComputationStatus(v.status()),
           stringifyComputationTier(v.tier()), v.reason().c_str(), stringifyRefutedBy(v.refutedBy()));
  };
  auto expect = [&](const char *what, const SetReductionVerdict &v, ComputationStatus status, ComputationTier tier,
                    RefutedBy by = RefutedBy::Nothing) {
    bool ok = v.status() == status && v.tier() == tier && v.refutedBy() == by;
    if (!ok)
      say(what, v);
    CHECK(ok);
  };

  // 1. Two groups write the same cells (case A): Theorem 17 over the union, symbolic. The
  // fold is modular, the trivial fact is refuted at a point (two contributions per cell),
  // and no reorder is claimed.
  {
    Fixture f = fixture(m, "sib_roots", 1, 3);
    std::vector<unsigned> l = loops(f.program);
    DomainRequest r = twoSegments(f, 0, l[0], 1, l[1]);
    SetReductionVerdict v = verdict(f, {0, 1}, r, f.options, z3Backend);
    expect("case A", v, S::Verified, T::Symbolic);
    CHECK(v.fold() == FoldForm::ModularInteger && v.points().empty() && v.logicalTier() == T::Proposed);
    CHECK(v.trivialReorder().status == FactStatus::Refuted && v.trivialReorder().point.has_value());
    // G5b-M4b-2a: the integer fact by Theorem 13's rule over the union, at the verdict's
    // tier; the float fact stays Unknown (M4b-2b). Before M4b-2a both were Unknown here.
    CHECK(v.integerReorder().status == FactStatus::Proven && v.integerReorder().tier == (int)T::Symbolic &&
          v.floatReorder().status == FactStatus::Unknown);
    CHECK(!v.witness() && !v.failingPoint() && v.transformation().kind() == ProofStatus::Kind::Unknown);
    CHECK(v.groups().size() == 2 && v.binding().find("reduction|" + f.program.fingerprint + "|set|") == 0);
    CHECK(v.computationSymbolic().status() == S::Verified && v.domainSymbolic().status() == S::Verified);
    // Per-group claims are not what the set rests on: each group alone is refuted by M3.
    ComputationOptions c = compOf(f.options);
    CHECK(checkGroupSetFinite(f.program, f.enumeration, {0}, c).status() == S::Refuted);
    CHECK(checkGroupSetFinite(f.program, f.enumeration, {1}, c).status() == S::Refuted);
    // The listing changes nothing.
    SetReductionVerdict back = verdict(f, {1, 0}, r, f.options, z3Backend);
    CHECK(back.status() == v.status() && back.tier() == v.tier() && back.binding() == v.binding() &&
          back.reason() == v.reason());

    // TT: a symbolic claim lost on either side gives the finite tier, never Symbolic.
    Selective noD1("; D1 ", SmtAnswer::Unknown);
    SetReductionVerdict finite = verdict(f, {0, 1}, r, f.options, noD1);
    CHECK(noD1.hits > 0);
    expect("domain symbolic lost", finite, S::Verified, T::ScopeEnumerated);
    CHECK(finite.points().size() == 3 && finite.domainSymbolic().status() == S::Unknown &&
          finite.computationSymbolic().status() == S::Verified);
    // A list with a point removed is Point.
    DomainEnumeration trimmed = f.enumeration;
    trimmed.points.pop_back();
    SetReductionVerdict part = checkSetReduction(f.program, f.proof(), trimmed, {0, 1}, r, f.options, noD1);
    expect("trimmed", part, S::Verified, T::Point);
    CHECK(part.points().size() == 2);
    // The enumeration of another scope than the claim's: never the whole scope.
    Fixture narrow = fixture(m, "sib_roots", 1, 2);
    std::vector<unsigned> nl = loops(narrow.program);
    DomainOptions wider = narrow.options;
    wider.scope = f.options.scope;
    SetReductionVerdict other = checkSetReduction(narrow.program, narrow.proof(), narrow.enumeration, {0, 1},
                                                  twoSegments(narrow, 0, nl[0], 1, nl[1]), wider, noD1);
    CHECK(other.tier() != T::ScopeEnumerated && other.tier() != T::Symbolic);

    // 8. The request lists the segments backwards: refuted by this request's Theorem 15 at
    // a point, which says nothing about the program.
    TermContext &terms = *f.program.terms;
    Term zero = terms.bv(APInt(16, 0)), n = terms.param(0, w), one = terms.bv(APInt(16, 1));
    DomainRequest backward;
    backward.segments = {segmentOf(f.program, {1}, {level(f.program, l[1], zero, n)}),
                         segmentOf(f.program, {0}, {level(f.program, l[0], zero, n)})};
    SetReductionVerdict wrong = verdict(f, {0, 1}, backward, f.options, z3Backend);
    expect("backwards", wrong, S::Refuted, T::Proposed, RefutedBy::Domain);
    CHECK(wrong.failingPoint().has_value() && !wrong.witness() &&
          wrong.domainFinite().status() == S::Refuted && wrong.computationFinite().status() == S::Verified);
    // 7. Events missing from a segment's set: the same refutation, of the request.
    DomainRequest missing = twoSegments(f, 0, l[0], 1, l[1], terms.binary(TermOp::Sub, n, one));
    expect("a missing event", verdict(f, {0, 1}, missing, f.options, z3Backend), S::Refuted, T::Proposed, RefutedBy::Domain);
    // 10. A set that is not whole groups, and a flat request: Unknown, nothing refuted.
    expect("a group twice", verdict(f, {0, 0}, r, f.options, z3Backend), S::Unknown, T::Proposed);
    expect("a flat request", verdict(f, {0, 1}, DomainRequest(), f.options, z3Backend), S::Unknown, T::Proposed);
    expect("no such group", verdict(f, {0, 5}, r, f.options, z3Backend), S::Unknown, T::Proposed);
    CHECK(verdict(f, {0, 5}, r, f.options, z3Backend).reason() == "no such group");

    // 10. B: claims about something else never join. The composer's own claims for this
    // set, joined under another scope, another request, another site set and another
    // program, each refused in words; the same claims joined as they are stand.
    ComputationOptions co = compOf(f.options);
    CheckedComputation cf = checkGroupSetFinite(f.program, f.enumeration, {0, 1}, co);
    std::vector<SiteGroup> both{f.enumeration.groups[0], f.enumeration.groups[1]};
    CheckedComputation cs = checkGroupSetSymbolic(f.program, f.proof(), both, co, z3Backend);
    CheckedSetDomain dfin = checkSetDomainFinite(f.program, f.enumeration, {0, 1}, r, f.options);
    CheckedSetDomain dsym = checkSetDomainSymbolic(f.program, f.proof(), both, r, f.options, z3Backend);
    SetReductionVerdict joined = joinSetReduction(f.program, f.enumeration, {0, 1}, r, f.options, cf, cs, dfin, dsym);
    expect("joined as they are", joined, S::Verified, T::Symbolic);
    // TT on the computation's side: this program asks no symbolic query, so its claim is
    // lost by hand; the verdict is the finite tier, never Symbolic.
    SetReductionVerdict finite2 = joinSetReduction(f.program, f.enumeration, {0, 1}, r, f.options, cf,
                                                   CheckedComputation::unknown("lost"), dfin, dsym);
    expect("computation symbolic lost", finite2, S::Verified, T::ScopeEnumerated);
    CHECK(finite2.points().size() == 3 && finite2.computationSymbolic().status() == S::Unknown &&
          finite2.domainSymbolic().status() == S::Verified);
    DomainOptions otherScope = f.options;
    otherScope.scope = range(f.program, 1, 2);
    SetReductionVerdict scoped = joinSetReduction(f.program, f.enumeration, {0, 1}, r, otherScope, cf, cs, dfin, dsym);
    expect("another scope", scoped, S::Unknown, T::Proposed);
    CHECK(scoped.reason().find("a claim about something else") != std::string::npos);
    SetReductionVerdict requested = joinSetReduction(f.program, f.enumeration, {0, 1}, backward, f.options, cf, cs, dfin, dsym);
    expect("another request", requested, S::Unknown, T::Proposed);
    CHECK(requested.reason() == "the finite domain claim is a claim about something else");
    DomainRequest firstOnly;
    firstOnly.segments = {r.segments[0]};
    CheckedComputation cf1 = checkGroupSetFinite(f.program, f.enumeration, {0}, co);
    SetReductionVerdict sites = joinSetReduction(f.program, f.enumeration, {0, 1}, r, f.options, cf1, cs, dfin, dsym);
    expect("another site set", sites, S::Unknown, T::Proposed);
    CHECK(sites.reason() == "the finite computation claim is a claim about something else");
    Fixture g = fixture(m, "sib_same", 1, 3);
    std::vector<unsigned> gl = loops(g.program);
    DomainRequest gr = twoSegments(g, 0, gl[0], 1, gl[1]);
    CheckedComputation gcf = checkGroupSetFinite(g.program, g.enumeration, {0}, compOf(g.options));
    CheckedSetDomain gdf = checkSetDomainFinite(g.program, g.enumeration, {0}, gr, g.options);
    SetReductionVerdict foreign = joinSetReduction(f.program, f.enumeration, {0, 1}, r, f.options, gcf, cs, gdf, dsym);
    expect("another program", foreign, S::Unknown, T::Proposed);
    CHECK(foreign.reason().find("a claim about something else") != std::string::npos);
    // Unknown claims of something else are not even looked at: the symbolic claims alone carry it.
    SetReductionVerdict unknowns = joinSetReduction(f.program, f.enumeration, {0, 1}, r, f.options,
                                                    CheckedComputation::unknown("x"), cs,
                                                    CheckedSetDomain::unknown("y"), dsym);
    expect("unknown finite claims", unknowns, S::Verified, T::Symbolic);
    // A refuted claim of something else refutes nothing here.
    Fixture third = fixture(m, "set_third", 1, 3);
    CheckedComputation refutedElsewhere = checkGroupSetFinite(third.program, third.enumeration, {0, 1},
                                                              compOf(third.options));
    CHECK(refutedElsewhere.status() == S::Refuted);
    SetReductionVerdict notHere = joinSetReduction(f.program, f.enumeration, {0, 1}, r, f.options,
                                                   refutedElsewhere, cs, dfin, dsym);
    expect("a refutation of something else", notHere, S::Unknown, T::Proposed);
  }

  // 2, 3. Different cells on one root (case B, authorized) and on a fresh root: Theorem 17
  // holds, every cell has one contribution, and the integer fold may be reordered by that
  // fact alone.
  {
    Fixture f = fixture(m, "case_b", 1, 3);
    std::vector<unsigned> l = loops(f.program);
    DomainRequest r = twoSegments(f, 0, l[0], 1, l[1]);
    SetReductionVerdict v = verdict(f, {0, 1}, r, f.options, z3Backend);
    expect("case B", v, S::Verified, T::Symbolic);
    CHECK(v.trivialReorder().status == FactStatus::Proven && v.integerReorder().status == FactStatus::Proven &&
          v.floatReorder().status == FactStatus::Unknown);
    CHECK(v.domainSymbolic().rootPairs().size() == 1 &&
          v.domainSymbolic().rootPairs()[0].relation == CellRelation::Distinct);
    // 4. The same without the name: the roots may alias, and nothing is claimed.
    DomainOptions silent = f.options;
    silent.authorized.clear();
    SetReductionVerdict open = verdict(f, {0, 1}, r, silent, z3Backend);
    expect("may alias", open, S::Unknown, T::Proposed);
    CHECK(open.reason().find("may alias") != std::string::npos && open.trivialReorder().status != FactStatus::Proven);
    CHECK(open.integerReorder().status == FactStatus::Unknown && open.floatReorder().status == FactStatus::Unknown);
    // The fresh root (3) is apart from everything with no name; the argument roots keep
    // theirs, which M3 needs for operands against outputs.
    Fixture fresh = fixture(m, "case_b_fresh", 1, 3);
    std::vector<unsigned> fl = loops(fresh.program);
    DomainOptions noNames = fresh.options;
    noNames.authorized = {"roots 0 and 1 are separate", "roots 0 and 2 are separate", "roots 1 and 2 are separate"};
    SetReductionVerdict byFresh = verdict(fresh, {0, 1}, twoSegments(fresh, 0, fl[1], 1, fl[2]), noNames, z3Backend);
    // The allocation's initializing stores keep M3's symbolic claim Unknown (a write to an
    // output outside the sites), so the verdict is the finite tier at the whole scope.
    expect("fresh root", byFresh, S::Verified, T::ScopeEnumerated);
    if (byFresh.computationSymbolic().status() != S::Unknown)
      printf("fresh root: M3 symbolic %s\n", byFresh.computationSymbolic().reason().c_str());
    CHECK(byFresh.computationSymbolic().status() == S::Unknown && byFresh.domainSymbolic().status() == S::Verified);
    CHECK(byFresh.trivialReorder().status == FactStatus::Proven && byFresh.points().size() == 3);
    CHECK(byFresh.domainSymbolic().rootPairs().size() == 1 && !byFresh.domainSymbolic().rootPairs()[0].assumption);
  }
  // 5. An empty segment at a point, and a group with no event anywhere.
  {
    Fixture f = fixture(m, "set_empty_second", 1, 3);
    std::vector<unsigned> l = loops(f.program);
    TermContext &terms = *f.program.terms;
    Term less = terms.binary(TermOp::Sub, terms.param(0, w), terms.bv(APInt(16, 1)));
    expect("empty second", verdict(f, {0, 1}, twoSegments(f, 0, l[0], 1, l[1], less), f.options, z3Backend),
           S::Verified, T::Symbolic);
    Fixture g = fixture(m, "set_never", 1, 3);
    std::vector<unsigned> gl = loops(g.program);
    Term zero = g.program.terms->bv(APInt(16, 0));
    SetReductionVerdict never = verdict(g, {0, 1}, twoSegments(g, 0, gl[0], 1, gl[1], zero), g.options, z3Backend);
    expect("never", never, S::Verified, T::Symbolic);
  }
  // 6. Multiplicity: the second group contributes twice to each cell, in its key's order.
  {
    Fixture f = fixture(m, "set_repeat", 1, 3);
    std::vector<unsigned> l = loops(f.program);
    CHECK(l.size() == 3 && f.enumeration.groups.size() == 2);
    TermContext &terms = *f.program.terms;
    Term zero = terms.bv(APInt(16, 0)), n = terms.param(0, w), two = terms.bv(APInt(16, 2));
    DomainRequest r;
    r.segments = {segmentOf(f.program, {0}, {level(f.program, l[0], zero, n)}),
                  segmentOf(f.program, {1}, {level(f.program, l[1], zero, n), level(f.program, l[2], zero, two)})};
    SetReductionVerdict v = verdict(f, {0, 1}, r, f.options, z3Backend);
    expect("repeat", v, S::Verified, T::Symbolic);
    CHECK(v.trivialReorder().status == FactStatus::Refuted);
    CHECK(v.domainSymbolic().segments().size() == 2 &&
          v.domainSymbolic().segments()[1].cellKey.status == FactStatus::Proven);
  }
  // 7. A writer left out: the set {0, 1} of set_third leaves the third group's writes to
  // C unaccounted for, and M3 refutes with a witness. In set_reads the second group reads
  // what the first wrote: the union is refuted by the computation with a witness.
  {
    Fixture f = fixture(m, "set_third", 1, 3);
    std::vector<unsigned> l = loops(f.program);
    CHECK(f.enumeration.groups.size() == 3);
    SetReductionVerdict v = verdict(f, {0, 1}, twoSegments(f, 0, l[0], 1, l[1]), f.options, z3Backend);
    expect("a writer left out", v, S::Refuted, T::Proposed, RefutedBy::Computation);
    CHECK(v.witness().has_value() && v.failingPoint().has_value() && v.computationFinite().status() == S::Refuted);
    Fixture g = fixture(m, "set_reads", 1, 3);
    std::vector<unsigned> gl = loops(g.program);
    CHECK(g.enumeration.groups.size() == 2);
    SetReductionVerdict reads = verdict(g, {0, 1}, twoSegments(g, 0, gl[0], 1, gl[1]), g.options, z3Backend);
    expect("a cross-group read", reads, S::Refuted, T::Proposed, RefutedBy::Computation);
    CHECK(reads.witness().has_value());
  }
  // 9. Floats: the fold's order is the claim. With C[0] = 1e16, A[0] = B[0] = 1 and
  // D[0] = -1e16 the program computes (1e16 + 1) + (-1e16) = 0 in segment order; the other
  // order gives 1. Nothing reorders: trivial is refuted, and (G5b-M4b-2b-1; M4b-1 left it
  // Unknown) the float fact is refuted by the fixed fill sequence with a replayed witness.
  {
    Fixture f = fixture(m, "set_float", 1, 3);
    std::vector<unsigned> l = loops(f.program);
    SetReductionVerdict v = verdict(f, {0, 1}, twoSegments(f, 0, l[0], 1, l[1]), f.options, z3Backend);
    expect("float", v, S::Verified, T::Symbolic);
    CHECK(v.fold() == FoldForm::OrderedFloat && v.trivialReorder().status == FactStatus::Refuted &&
          v.floatReorder().status == FactStatus::Refuted && v.integerReorder().status == FactStatus::Unknown);
    CHECK(v.floatReorder().point && v.floatReorder().memory &&
          v.floatReorder().detail.find("changes its final bits") != std::string::npos);
    if (v.floatReorder().memory)
      CHECK(runConcrete(f.program, *v.floatReorder().memory).status == RunStatus::Returned);
    EntryState entry;
    entry.parameters = {APInt(16, 1)};
    auto bitsOf = [](double value) {
      uint64_t raw;
      std::memcpy(&raw, &value, sizeof raw);
      return APInt(64, raw);
    };
    for (unsigned root = 0; root < 4; ++root) {
      RootData data;
      data.shape = {8};
      double first = root == 0 ? 1.0 : root == 1 ? 1.0 : root == 2 ? 1e16 : -1e16;
      for (unsigned i = 0; i < 8; ++i)
        data.cells.push_back(bitsOf(i == 0 ? first : 0.0));
      entry.memory.push_back(data);
    }
    RunResult run = runConcrete(f.program, entry);
    CHECK(run.status == RunStatus::Returned && run.memory.size() == 4 && run.memory[2].cells[0].has_value());
    if (run.status == RunStatus::Returned && run.memory[2].cells[0])
      CHECK(*run.memory[2].cells[0] == bitsOf((1e16 + 1.0) + -1e16) && *run.memory[2].cells[0] != bitsOf((1e16 + -1e16) + 1.0));
    CHECK(bitsOf((1e16 + 1.0) + -1e16) != bitsOf((1e16 + -1e16) + 1.0));
  }
  // The interleaved union: each group alone is Verified by M3, and the union's computation
  // too, yet the set verdict for the two-segment request is refuted by the domain at a
  // point. No per-group verdict stands in for the set's.
  {
    Fixture f = fixture(m, "set_interleaved", 1, 3);
    std::vector<unsigned> l = loops(f.program);
    ComputationOptions c = compOf(f.options);
    CHECK(checkGroupSetFinite(f.program, f.enumeration, {0}, c).status() == S::Verified);
    CHECK(checkGroupSetFinite(f.program, f.enumeration, {1}, c).status() == S::Verified);
    SetReductionVerdict v = verdict(f, {0, 1}, twoSegments(f, 0, l[0], 1, l[0]), f.options, z3Backend);
    expect("interleaved", v, S::Refuted, T::Proposed, RefutedBy::Domain);
    CHECK(v.failingPoint().has_value() && v.computationFinite().status() == S::Verified);
  }
  // A set of one group is Theorem 13 on the same request: status, tier, points, trivial.
  {
    Fixture f = fixture(m, "sib_same", 1, 3);
    std::vector<unsigned> l = loops(f.program);
    DomainRequest r = twoSegments(f, 0, l[0], 1, l[1]);
    SetReductionVerdict set = verdict(f, {0}, r, f.options, z3Backend);
    ReductionVerdict one = checkReduction(f.program, f.proof(), f.enumeration, 0, none, nullptr, r, f.options, z3Backend);
    bool same = set.status() == one.status() && set.tier() == one.tier() && set.points().size() == one.points().size() &&
                set.everyRun() == one.everyRun() && set.trivialReorder().status == one.trivialReorder().status;
    if (!same)
      printf("one group: set %s %s trivial %s; T13 %s %s trivial %s\n", stringifyComputationStatus(set.status()),
             stringifyComputationTier(set.tier()), stringifyFactStatus(set.trivialReorder().status),
             stringifyComputationStatus(one.status()), stringifyComputationTier(one.tier()),
             stringifyFactStatus(one.trivialReorder().status));
    CHECK(same);
    CHECK(set.status() == S::Verified);
    // G5b-M4b-2a: the integer fact too, by the same rule at the same tier (M4b-1 left it
    // Unknown here); the float fact as well.
    CHECK(set.integerReorder().status == one.integerReorder().status &&
          set.integerReorder().tier == one.integerReorder().tier &&
          set.integerReorder().detail == one.integerReorder().detail);
    CHECK(set.floatReorder().status == one.floatReorder().status);
  }
}

/// G5b-M4b-2a: programs for the integer reorder fact over a set. Each is two or three
/// loops writing cells of one or two output roots, as sib_roots is, with one thing the
/// integer rule must see: the element width, a flag, an observer, a stop on an accumulate
/// load of the second root, a stop through loop-carried state, a float group, flags that
/// differ between the groups, or distinct cells of one root.
static const char *reorderSource = R"(func.func @set_i8(%a: memref<8xi8>, %b: memref<8xi8>, %c: memref<8xi8>, %d: memref<8xi8>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  scf.for %i = %c0 to %n step %c1 {
    %xi = memref.load %a[%i] : memref<8xi8>
    %yi = memref.load %b[%i] : memref<8xi8>
    %zi = memref.load %c[%i] : memref<8xi8>
    %mi = arith.muli %xi, %yi : i8
    %si = arith.addi %zi, %mi : i8
    memref.store %si, %c[%i] : memref<8xi8>
  }
  scf.for %j = %c0 to %n step %c1 {
    %xj = memref.load %a[%j] : memref<8xi8>
    %yj = memref.load %d[%j] : memref<8xi8>
    %zj = memref.load %c[%j] : memref<8xi8>
    %mj = arith.muli %xj, %yj : i8
    %sj = arith.addi %zj, %mj : i8
    memref.store %sj, %c[%j] : memref<8xi8>
  }
  return
}
func.func @set_nsw(%a: memref<8xi32>, %b: memref<8xi32>, %c: memref<8xi32>, %d: memref<8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  scf.for %i = %c0 to %n step %c1 {
    %xi = memref.load %a[%i] : memref<8xi32>
    %yi = memref.load %b[%i] : memref<8xi32>
    %zi = memref.load %c[%i] : memref<8xi32>
    %mi = arith.muli %xi, %yi : i32
    %si = arith.addi %zi, %mi overflow<nsw> : i32
    memref.store %si, %c[%i] : memref<8xi32>
  }
  scf.for %j = %c0 to %n step %c1 {
    %xj = memref.load %a[%j] : memref<8xi32>
    %yj = memref.load %d[%j] : memref<8xi32>
    %zj = memref.load %c[%j] : memref<8xi32>
    %mj = arith.muli %xj, %yj : i32
    %sj = arith.addi %zj, %mj overflow<nsw> : i32
    memref.store %sj, %c[%j] : memref<8xi32>
  }
  return
}
func.func @set_nuw(%a: memref<8xi32>, %b: memref<8xi32>, %c: memref<8xi32>, %d: memref<8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  scf.for %i = %c0 to %n step %c1 {
    %xi = memref.load %a[%i] : memref<8xi32>
    %yi = memref.load %b[%i] : memref<8xi32>
    %zi = memref.load %c[%i] : memref<8xi32>
    %mi = arith.muli %xi, %yi : i32
    %si = arith.addi %zi, %mi overflow<nuw> : i32
    memref.store %si, %c[%i] : memref<8xi32>
  }
  scf.for %j = %c0 to %n step %c1 {
    %xj = memref.load %a[%j] : memref<8xi32>
    %yj = memref.load %d[%j] : memref<8xi32>
    %zj = memref.load %c[%j] : memref<8xi32>
    %mj = arith.muli %xj, %yj : i32
    %sj = arith.addi %zj, %mj overflow<nuw> : i32
    memref.store %sj, %c[%j] : memref<8xi32>
  }
  return
}
func.func @set_observer(%a: memref<8xi32>, %b: memref<8xi32>, %c: memref<8xi32>, %d: memref<8xi32>, %e: memref<8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  scf.for %i = %c0 to %n step %c1 {
    %xi = memref.load %a[%i] : memref<8xi32>
    %yi = memref.load %b[%i] : memref<8xi32>
    %zi = memref.load %c[%i] : memref<8xi32>
    %mi = arith.muli %xi, %yi : i32
    %si = arith.addi %zi, %mi : i32
    memref.store %si, %c[%i] : memref<8xi32>
  }
  %o = memref.load %c[%c0] : memref<8xi32>
  memref.store %o, %e[%c0] : memref<8xi32>
  scf.for %j = %c0 to %n step %c1 {
    %xj = memref.load %a[%j] : memref<8xi32>
    %yj = memref.load %d[%j] : memref<8xi32>
    %zj = memref.load %c[%j] : memref<8xi32>
    %mj = arith.muli %xj, %yj : i32
    %sj = arith.addi %zj, %mj : i32
    memref.store %sj, %c[%j] : memref<8xi32>
  }
  return
}
func.func @set_stop_second(%a: memref<8xi32>, %b: memref<8xi32>, %c: memref<8xi32>, %d: memref<8xi32>, %e: memref<8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %one = arith.constant 1 : i32
  scf.for %i = %c0 to %n step %c1 {
    %xi = memref.load %a[%i] : memref<8xi32>
    %yi = memref.load %b[%i] : memref<8xi32>
    %zi = memref.load %c[%i] : memref<8xi32>
    %mi = arith.muli %xi, %yi : i32
    %si = arith.addi %zi, %mi : i32
    memref.store %si, %c[%i] : memref<8xi32>
  }
  scf.for %j = %c0 to %n step %c1 {
    %xj = memref.load %a[%j] : memref<8xi32>
    %yj = memref.load %d[%j] : memref<8xi32>
    %zj = memref.load %c[%j] : memref<8xi32>
    %mj = arith.muli %xj, %yj : i32
    %sj = arith.addi %zj, %mj : i32
    memref.store %sj, %c[%j] : memref<8xi32>
  }
  scf.for %k = %c0 to %n step %c1 {
    %xk = memref.load %a[%k] : memref<8xi32>
    %yk = memref.load %b[%k] : memref<8xi32>
    %zk = memref.load %e[%k] : memref<8xi32>
    %odd = arith.ori %zk, %one : i32
    %q = arith.divui %one, %odd : i32
    %mk = arith.muli %xk, %yk : i32
    %sk = arith.addi %zk, %mk : i32
    memref.store %sk, %e[%k] : memref<8xi32>
  }
  return
}
func.func @set_carried_stop(%a: memref<8xi32>, %b: memref<8xi32>, %c: memref<8xi32>, %d: memref<8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %one = arith.constant 1 : i32
  %h = arith.constant 100 : i32
  %kk = arith.constant 23130 : i32
  scf.for %i = %c0 to %n step %c1 {
    %xi = memref.load %a[%i] : memref<8xi32>
    %yi = memref.load %b[%i] : memref<8xi32>
    %zi = memref.load %c[%i] : memref<8xi32>
    %mi = arith.muli %xi, %yi : i32
    %si = arith.addi %zi, %mi : i32
    memref.store %si, %c[%i] : memref<8xi32>
  }
  %r = scf.for %j = %c0 to %n step %c1 iter_args(%acc = %one) -> (i32) {
    %ax = arith.xori %acc, %kk : i32
    %dv = arith.divsi %h, %ax : i32
    %xj = memref.load %a[%j] : memref<8xi32>
    %yj = memref.load %d[%j] : memref<8xi32>
    %zj = memref.load %c[%j] : memref<8xi32>
    %mj = arith.muli %xj, %yj : i32
    %sj = arith.addi %zj, %mj : i32
    memref.store %sj, %c[%j] : memref<8xi32>
    scf.yield %sj : i32
  }
  return
}
func.func @set_mixed(%a: memref<8xi32>, %b: memref<8xi32>, %c: memref<8xi32>, %d: memref<8xi32>, %e: memref<8xf64>, %g: memref<8xf64>, %h: memref<8xf64>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  scf.for %i = %c0 to %n step %c1 {
    %xi = memref.load %a[%i] : memref<8xi32>
    %yi = memref.load %b[%i] : memref<8xi32>
    %zi = memref.load %c[%i] : memref<8xi32>
    %mi = arith.muli %xi, %yi : i32
    %si = arith.addi %zi, %mi : i32
    memref.store %si, %c[%i] : memref<8xi32>
  }
  scf.for %j = %c0 to %n step %c1 {
    %xj = memref.load %a[%j] : memref<8xi32>
    %yj = memref.load %d[%j] : memref<8xi32>
    %zj = memref.load %c[%j] : memref<8xi32>
    %mj = arith.muli %xj, %yj : i32
    %sj = arith.addi %zj, %mj : i32
    memref.store %sj, %c[%j] : memref<8xi32>
  }
  scf.for %k = %c0 to %n step %c1 {
    %xk = memref.load %g[%k] : memref<8xf64>
    %yk = memref.load %h[%k] : memref<8xf64>
    %zk = memref.load %e[%k] : memref<8xf64>
    %mk = arith.mulf %xk, %yk : f64
    %sk = arith.addf %zk, %mk : f64
    memref.store %sk, %e[%k] : memref<8xf64>
  }
  return
}
func.func @set_mixed_flags(%a: memref<8xi32>, %b: memref<8xi32>, %c: memref<8xi32>, %d: memref<8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  scf.for %i = %c0 to %n step %c1 {
    %xi = memref.load %a[%i] : memref<8xi32>
    %yi = memref.load %b[%i] : memref<8xi32>
    %zi = memref.load %c[%i] : memref<8xi32>
    %mi = arith.muli %xi, %yi : i32
    %si = arith.addi %zi, %mi overflow<nsw> : i32
    memref.store %si, %c[%i] : memref<8xi32>
  }
  scf.for %j = %c0 to %n step %c1 {
    %xj = memref.load %a[%j] : memref<8xi32>
    %yj = memref.load %d[%j] : memref<8xi32>
    %zj = memref.load %c[%j] : memref<8xi32>
    %mj = arith.muli %xj, %yj : i32
    %sj = arith.addi %zj, %mj : i32
    memref.store %sj, %c[%j] : memref<8xi32>
  }
  return
}
func.func @set_guard(%a: memref<8xi32>, %b: memref<8xi32>, %c: memref<8xi32>, %d: memref<8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %zero = arith.constant 0 : i32
  scf.for %i = %c0 to %n step %c1 {
    %xi = memref.load %a[%i] : memref<8xi32>
    %yi = memref.load %b[%i] : memref<8xi32>
    %zi = memref.load %c[%i] : memref<8xi32>
    %mi = arith.muli %xi, %yi : i32
    %si = arith.addi %zi, %mi : i32
    memref.store %si, %c[%i] : memref<8xi32>
  }
  scf.for %j = %c0 to %n step %c1 {
    %g = memref.load %c[%c0] : memref<8xi32>
    %on = arith.cmpi ne, %g, %zero : i32
    scf.if %on {
      %xj = memref.load %a[%j] : memref<8xi32>
      %yj = memref.load %d[%j] : memref<8xi32>
      %zj = memref.load %c[%j] : memref<8xi32>
      %mj = arith.muli %xj, %yj : i32
      %sj = arith.addi %zj, %mj : i32
      memref.store %sj, %c[%j] : memref<8xi32>
    }
  }
  return
}
func.func @set_guard_state(%a: memref<8xi32>, %b: memref<8xi32>, %c: memref<8xi32>, %d: memref<8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %zero = arith.constant 0 : i32
  scf.for %i = %c0 to %n step %c1 {
    %xi = memref.load %a[%i] : memref<8xi32>
    %yi = memref.load %b[%i] : memref<8xi32>
    %zi = memref.load %c[%i] : memref<8xi32>
    %mi = arith.muli %xi, %yi : i32
    %si = arith.addi %zi, %mi : i32
    memref.store %si, %c[%i] : memref<8xi32>
  }
  %r = scf.for %j = %c0 to %n step %c1 iter_args(%acc = %zero) -> (i32) {
    %on = arith.cmpi ne, %acc, %zero : i32
    scf.if %on {
      %xj = memref.load %a[%j] : memref<8xi32>
      %yj = memref.load %d[%j] : memref<8xi32>
      %zj = memref.load %c[%j] : memref<8xi32>
      %mj = arith.muli %xj, %yj : i32
      %sj = arith.addi %zj, %mj : i32
      memref.store %sj, %c[%j] : memref<8xi32>
    }
    %zk = memref.load %c[%j] : memref<8xi32>
    scf.yield %zk : i32
  }
  return
}
func.func @set_cells(%a: memref<8xi32>, %b: memref<8xi32>, %c: memref<8xi32>, %d: memref<8xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c4 = arith.constant 4 : index
  scf.for %i = %c0 to %n step %c1 {
    %xi = memref.load %a[%i] : memref<8xi32>
    %yi = memref.load %b[%i] : memref<8xi32>
    %zi = memref.load %c[%i] : memref<8xi32>
    %mi = arith.muli %xi, %yi : i32
    %si = arith.addi %zi, %mi : i32
    memref.store %si, %c[%i] : memref<8xi32>
  }
  scf.for %j = %c0 to %n step %c1 {
    %j4 = arith.addi %j, %c4 : index
    %xj = memref.load %a[%j] : memref<8xi32>
    %yj = memref.load %d[%j] : memref<8xi32>
    %zj = memref.load %c[%j4] : memref<8xi32>
    %mj = arith.muli %xj, %yj : i32
    %sj = arith.addi %zj, %mj : i32
    memref.store %sj, %c[%j4] : memref<8xi32>
  }
  return
}
)";

/// G5b-M4b-2a: the integer reorder fact over a set of groups, by Theorem 13's rule read
/// over the union (every listed output root), after the verdict is final. The brief's
/// cases: A, 8-bit modular addition in both orders; B, distinct cells; C, roots that may
/// alias; D, overflow flags; E, wrong listings. Then each condition alone, the tiers, the
/// witness replayed by the judge, bindings, scopes, limits, and a set of one group against
/// Theorem 13.
static void testSetReorder() {
  Module m(std::string(setSource) + siblingSource + reductionSource + reorderSource);
  Sort w = Sort::bv(16);
  auto level = [&](const SemanticProgram &p, unsigned var, Term lower, Term upper) {
    return LevelSpec{p.terms->state(var, w), lower, upper, false, false};
  };
  // One segment per listed site, one loop variable each, over [0, n).
  auto segments = [&](Fixture &f, std::vector<std::pair<unsigned, unsigned>> siteVars) {
    TermContext &terms = *f.program.terms;
    Term zero = terms.bv(APInt(16, 0)), n = terms.param(0, w);
    DomainRequest r;
    for (auto [site, var] : siteVars)
      r.segments.push_back(segmentOf(f.program, {site}, {level(f.program, var, zero, n)}));
    return r;
  };
  auto verdict = [&](Fixture &f, std::vector<unsigned> groups, const DomainRequest &r, const DomainOptions &o,
                     SmtBackend &backend) {
    return checkSetReduction(f.program, f.proof(), f.enumeration, groups, r, o, backend);
  };
  auto say = [](const char *what, const SetReductionVerdict &v) {
    printf("%s: %s %s (%s) refuted by %s; integer %s %s \"%s\"; float %s \"%s\"\n", what,
           stringifyComputationStatus(v.status()), stringifyComputationTier(v.tier()), v.reason().c_str(),
           stringifyRefutedBy(v.refutedBy()), stringifyFactStatus(v.integerReorder().status),
           stringifyComputationTier((ComputationTier)v.integerReorder().tier), v.integerReorder().detail.c_str(),
           stringifyFactStatus(v.floatReorder().status), v.floatReorder().detail.c_str());
  };
  auto expect = [&](const char *what, const SetReductionVerdict &v, ComputationStatus status, ComputationTier tier,
                    RefutedBy by = RefutedBy::Nothing) {
    bool ok = v.status() == status && v.tier() == tier && v.refutedBy() == by;
    if (!ok)
      say(what, v);
    CHECK(ok);
  };
  // The integer fact as expected, in words. A verdict refused or refuted before the facts
  // are computed carries the default fact, Unknown with no detail, as M4b-1's does.
  auto integer = [&](const char *what, const SetReductionVerdict &v, FactStatus status, ComputationTier tier,
                     const std::string &detail) {
    const Fact &fact = v.integerReorder();
    bool ok = fact.status == status &&
              (fact.detail == detail || (status == FactStatus::Unknown && fact.detail.empty())) &&
              (status != FactStatus::Proven || fact.tier == (int)tier);
    if (!ok)
      say(what, v);
    CHECK(ok);
  };
  const std::string byRule = "wrapping addition with no flag, unobserved, at the computation's tier";
  const std::string noClaim = "not an integer fold, or no claim";
  const std::string flagged = "the reduction carries nsw or nuw, so a partial sum can stop the run";
  const std::string observed = "another effect reads the output, so it can see a partial sum";
  const std::string stopped = "a stop reads the output or memory-derived state, so a partial sum can decide it";
  // An entry at n = `point` with every root's first cells as given, the rest zero.
  auto entry = [&](const Fixture &f, unsigned point, std::vector<std::vector<uint64_t>> firstCells) {
    EntryState out;
    out.parameters = {APInt(16, point)};
    for (unsigned r = 0; r < f.program.roots.size(); ++r) {
      RootData data;
      data.shape = {8};
      unsigned width = f.program.roots[r].sort.width;
      for (unsigned i = 0; i < 8; ++i) {
        uint64_t value = r < firstCells.size() && i < firstCells[r].size() ? firstCells[r][i] : 0;
        data.cells.push_back(APInt(width, value));
      }
      out.memory.push_back(data);
    }
    return out;
  };
  // The judge over the union, from the set's own replay at `point`.
  auto judge = [&](Fixture &f, std::vector<unsigned> groups, unsigned point, const EntryState &e, std::string &detail,
                   bool &ok) {
    std::vector<const SiteGroup *> listed;
    for (unsigned g : groups)
      listed.push_back(&f.enumeration.groups[g]);
    std::vector<int> groupOf;
    std::string why;
    std::optional<SiteGroup> united = unitedSiteGroup(f.program, listed, groupOf, why);
    ok = united.has_value();
    if (!united)
      return false;
    ReplayResult replay = replayGroupSet(f.program, *united, groupOf, listed.size(), {APInt(16, point)},
                                         HerbrandLimits());
    ok = replay.outcome == ReplayOutcome::Equal;
    return reversalDiffers(f.program, *united, e, replay.prefix, detail);
  };
  auto cellOf = [](const RunResult &run, unsigned root, unsigned index) -> std::optional<APInt> {
    if (run.status != RunStatus::Returned || run.memory.size() <= root || run.memory[root].cells.size() <= index)
      return std::nullopt;
    return run.memory[root].cells[index];
  };

  // Case A. Two groups write the same cells of C with 8-bit wrapping addition: Theorem 17
  // symbolic, the fold modular, and the integer fact Proven at the symbolic tier by the
  // rule. With C[0] = 250, A[0] = 10, B[0] = 1 and D[0] = 2 the program gives (250 + 10) +
  // 20 = 24 mod 256; the other order (250 + 20) + 10 is 24 as well, and the judge finds
  // no reversal that differs. The listing changes no fact.
  {
    Fixture f = fixture(m, "set_i8", 1, 3);
    std::vector<unsigned> l = loops(f.program);
    CHECK(l.size() == 2 && f.enumeration.groups.size() == 2);
    DomainRequest r = segments(f, {{0, l[0]}, {1, l[1]}});
    SetReductionVerdict v = verdict(f, {0, 1}, r, f.options, z3Backend);
    expect("case A", v, S::Verified, T::Symbolic);
    CHECK(v.fold() == FoldForm::ModularInteger && v.trivialReorder().status == FactStatus::Refuted);
    integer("case A", v, FactStatus::Proven, T::Symbolic, byRule);
    CHECK(v.floatReorder().status == FactStatus::Unknown && v.floatReorder().detail == "not a float fold, or no claim");
    CHECK(!v.integerReorder().point && !v.integerReorder().memory);
    EntryState e = entry(f, 1, {{10}, {1}, {250}, {2}});
    RunResult run = runConcrete(f.program, e);
    std::optional<APInt> c0 = cellOf(run, 2, 0);
    CHECK(c0 && *c0 == APInt(8, 24));
    APInt z(8, 250), p1(8, 10), p2(8, 20);
    CHECK((z + p1 + p2) == APInt(8, 24) && (z + p2 + p1) == APInt(8, 24));
    std::string detail;
    bool ok = false;
    CHECK(!judge(f, {0, 1}, 1, e, detail, ok) && ok);
    SetReductionVerdict back = verdict(f, {1, 0}, r, f.options, z3Backend);
    integer("case A, other listing", back, FactStatus::Proven, T::Symbolic, byRule);
    CHECK(back.status() == v.status() && back.tier() == v.tier() && back.binding() == v.binding());
    // The same at 32 bits: sib_roots with C[0] = -10, A[0] = 10, B[0] = 1, D[0] = 2 ends at 20.
    Fixture g = fixture(m, "sib_roots", 1, 3);
    std::vector<unsigned> gl = loops(g.program);
    SetReductionVerdict s32 = verdict(g, {0, 1}, segments(g, {{0, gl[0]}, {1, gl[1]}}), g.options, z3Backend);
    integer("sib_roots", s32, FactStatus::Proven, T::Symbolic, byRule);
    EntryState e32 = entry(g, 1, {{10}, {1}, {0xFFFFFFF6u}, {2}});
    std::optional<APInt> g0 = cellOf(runConcrete(g.program, e32), 2, 0);
    CHECK(g0 && *g0 == APInt(32, 20));
  }

  // Tiers. The fact holds at the verdict's tier and never above it: with the domain's
  // symbolic claim lost the verdict is the whole scope, and so is the fact; with a point
  // removed both are Point.
  {
    Fixture f = fixture(m, "sib_roots", 1, 3);
    std::vector<unsigned> l = loops(f.program);
    DomainRequest r = segments(f, {{0, l[0]}, {1, l[1]}});
    Selective noD1("; D1 ", SmtAnswer::Unknown);
    SetReductionVerdict whole = verdict(f, {0, 1}, r, f.options, noD1);
    CHECK(noD1.hits > 0);
    expect("whole scope", whole, S::Verified, T::ScopeEnumerated);
    integer("whole scope", whole, FactStatus::Proven, T::ScopeEnumerated, byRule);
    DomainEnumeration trimmed = f.enumeration;
    trimmed.points.pop_back();
    SetReductionVerdict part = checkSetReduction(f.program, f.proof(), trimmed, {0, 1}, r, f.options, noD1);
    expect("points", part, S::Verified, T::Point);
    integer("points", part, FactStatus::Proven, T::Point, byRule);
    // A solver limit of zero is no limit for the symbolic routes: both symbolic claims are
    // Unknown, the verdict is the finite tier, which asks no solver, and so is the fact.
    DomainOptions zero = f.options;
    zero.termination.solver.timeoutMilliseconds = 0;
    SetReductionVerdict none = verdict(f, {0, 1}, r, zero, z3Backend);
    expect("zero limit", none, S::Verified, T::ScopeEnumerated);
    CHECK(none.domainSymbolic().status() == S::Unknown && none.computationSymbolic().status() == S::Unknown);
    integer("zero limit", none, FactStatus::Proven, T::ScopeEnumerated, byRule);
  }

  // Case D. Flags. With nsw on both adds the fact is never Proven: a partial sum can
  // overflow and stop the run in one order and not the other. The verdict stands (the
  // claim is about runs that complete). The search's fill sequence is fixed (Theorem 13's
  // seed), so its outcome is a fact of the tree: it finds a memory at which the program
  // completes and the reversal of a cell overflows, and the fact is Refuted with it, the
  // witness judged again here over the union. A memory built by hand as well: C[0] =
  // INT_MAX, A[0] = -1, B[0] = 1, D[0] = -1, so the contributions are -1 then +1: the
  // program completes at INT_MAX, the reversal overflows at its first step. With nuw no
  // reversal can overflow (unsigned partial sums only grow), and the fact stays Unknown.
  {
    Fixture f = fixture(m, "set_nsw", 1, 3);
    std::vector<unsigned> l = loops(f.program);
    DomainRequest r = segments(f, {{0, l[0]}, {1, l[1]}});
    SetReductionVerdict v = verdict(f, {0, 1}, r, f.options, z3Backend);
    if (v.status() != S::Verified || v.integerReorder().status != FactStatus::Refuted)
      say("nsw", v);
    CHECK(v.status() == S::Verified && v.tier() == T::Symbolic && v.trivialReorder().status == FactStatus::Refuted);
    // Refuted, by the search over the union: a search over one group alone finds nothing
    // here, since each group writes each cell once (and each group alone has no claim).
    CHECK(v.integerReorder().status == FactStatus::Refuted);
    const Fact &fact = v.integerReorder();
    CHECK(fact.point && fact.memory && fact.detail.find("overflows a flagged addition") != std::string::npos);
    if (fact.point && fact.memory) {
      CHECK(runConcrete(f.program, *fact.memory).status == RunStatus::Returned);
      std::string detail;
      bool ok = false;
      CHECK(judge(f, {0, 1}, (*fact.point)[0].getZExtValue(), *fact.memory, detail, ok) && ok);
    }
    printf("nsw: integer %s: %s\n", stringifyFactStatus(v.integerReorder().status), v.integerReorder().detail.c_str());
    EntryState e = entry(f, 1, {{0xFFFFFFFFu}, {1}, {0x7FFFFFFFu}, {0xFFFFFFFFu}});
    RunResult run = runConcrete(f.program, e);
    std::optional<APInt> c0 = cellOf(run, 2, 0);
    CHECK(c0 && *c0 == APInt(32, 0x7FFFFFFFu));
    std::string detail;
    bool ok = false;
    CHECK(judge(f, {0, 1}, 1, e, detail, ok) && ok && detail.find("overflows a flagged addition") != std::string::npos);
    // The float fact is untouched by any of it.
    CHECK(v.floatReorder().status == FactStatus::Unknown);
    Fixture g = fixture(m, "set_nuw", 1, 3);
    std::vector<unsigned> gl = loops(g.program);
    SetReductionVerdict u = verdict(g, {0, 1}, segments(g, {{0, gl[0]}, {1, gl[1]}}), g.options, z3Backend);
    CHECK(u.status() == S::Verified);
    integer("nuw", u, FactStatus::Unknown, T::Proposed, flagged);
  }

  // Each condition alone. An observer: a load of C outside the groups, stored elsewhere.
  {
    Fixture f = fixture(m, "set_observer", 1, 3);
    std::vector<unsigned> l = loops(f.program);
    SetReductionVerdict v = verdict(f, {0, 1}, segments(f, {{0, l[0]}, {1, l[1]}}), f.options, z3Backend);
    if (v.status() != S::Verified)
      say("observer", v);
    CHECK(v.integerReorder().status != FactStatus::Proven);
    if (v.status() == S::Verified)
      integer("observer", v, FactStatus::Unknown, T::Proposed, observed);
  }
  // A stop on the accumulate load of the second output root: the union {C, E} refuses by
  // that root, which a check of the first root alone would miss; the union {C} of the
  // first two groups, whose stops read no root of theirs, is Proven.
  {
    Fixture f = fixture(m, "set_stop_second", 1, 3);
    std::vector<unsigned> l = loops(f.program);
    CHECK(l.size() == 3 && f.enumeration.groups.size() == 3);
    SetReductionVerdict three = verdict(f, {0, 1, 2}, segments(f, {{0, l[0]}, {1, l[1]}, {2, l[2]}}), f.options,
                                        z3Backend);
    expect("stop on the second root", three, S::Verified, T::Symbolic);
    integer("stop on the second root", three, FactStatus::Unknown, T::Proposed, stopped);
    CHECK(three.trivialReorder().status == FactStatus::Refuted);
    SetReductionVerdict two = verdict(f, {0, 1}, segments(f, {{0, l[0]}, {1, l[1]}}), f.options, z3Backend);
    expect("the first root alone", two, S::Verified, T::Symbolic);
    integer("the first root alone", two, FactStatus::Proven, T::Symbolic, byRule);
  }
  // A partial sum carried in state decides a stop (the R3 regression, over a set).
  {
    Fixture f = fixture(m, "set_carried_stop", 1, 3);
    std::vector<unsigned> l = loops(f.program);
    CHECK(l.size() >= 2);
    SetReductionVerdict v = verdict(f, {0, 1}, segments(f, {{0, l[0]}, {1, l[1]}}), f.options, z3Backend);
    if (v.status() != S::Verified)
      say("carried stop", v);
    CHECK(v.integerReorder().status != FactStatus::Proven);
    if (v.status() == S::Verified)
      integer("carried stop", v, FactStatus::Unknown, T::Proposed, stopped);
  }
  // IR-5 is a defence: a guard that reads the output root, directly or through loop-carried
  // state, makes the trace depend on memory, and the obliviousness gate closes before any
  // claim: the verdict is Unknown and so is every fact.
  for (const char *name : {"set_guard", "set_guard_state"}) {
    Fixture f = fixture(m, name, 1, 3);
    CHECK(f.enumeration.gate != GateStatus::Open);
    std::vector<unsigned> l = loops(f.program);
    DomainRequest r;
    if (l.size() >= 2)
      r = segments(f, {{0, l[0]}, {1, l[1]}});
    SetReductionVerdict v = verdict(f, {0, 1}, r, f.options, z3Backend);
    expect(name, v, S::Unknown, T::Proposed);
    integer(name, v, FactStatus::Unknown, T::Proposed, noClaim);
    CHECK(v.floatReorder().status == FactStatus::Unknown && v.trivialReorder().status == FactStatus::Unknown);
  }
  // A float group in the union makes the whole fold ordered: no integer fact; the two
  // integer groups alone are Proven.
  {
    Fixture f = fixture(m, "set_mixed", 1, 3);
    std::vector<unsigned> l = loops(f.program);
    CHECK(l.size() == 3 && f.enumeration.groups.size() == 3);
    SetReductionVerdict three = verdict(f, {0, 1, 2}, segments(f, {{0, l[0]}, {1, l[1]}, {2, l[2]}}), f.options,
                                        z3Backend);
    expect("mixed", three, S::Verified, T::Symbolic);
    CHECK(three.fold() == FoldForm::OrderedFloat);
    integer("mixed", three, FactStatus::Unknown, T::Proposed, noClaim);
    CHECK(three.floatReorder().status == FactStatus::Unknown);
    SetReductionVerdict two = verdict(f, {0, 1}, segments(f, {{0, l[0]}, {1, l[1]}}), f.options, z3Backend);
    integer("the integer groups alone", two, FactStatus::Proven, T::Symbolic, byRule);
  }
  // Flags that differ between the groups: not Proven (a flag), and the judge refuses to
  // reverse a fold of two operations, so no refutation either.
  {
    Fixture f = fixture(m, "set_mixed_flags", 1, 3);
    std::vector<unsigned> l = loops(f.program);
    SetReductionVerdict v = verdict(f, {0, 1}, segments(f, {{0, l[0]}, {1, l[1]}}), f.options, z3Backend);
    CHECK(v.status() == S::Verified);
    integer("mixed flags", v, FactStatus::Unknown, T::Proposed, flagged);
    EntryState e = entry(f, 1, {{0xFFFFFFFFu}, {1}, {0x7FFFFFFFu}, {0xFFFFFFFFu}});
    std::string detail;
    bool ok = false;
    CHECK(!judge(f, {0, 1}, 1, e, detail, ok) && ok);
  }

  // Case B. Distinct cells of one root: every cell has one contribution, the trivial fact
  // proves the integer one, and the run shows the two cells apart.
  {
    Fixture f = fixture(m, "set_cells", 1, 3);
    std::vector<unsigned> l = loops(f.program);
    SetReductionVerdict v = verdict(f, {0, 1}, segments(f, {{0, l[0]}, {1, l[1]}}), f.options, z3Backend);
    if (v.status() != S::Verified)
      say("cells", v);
    CHECK(v.status() == S::Verified && v.trivialReorder().status == FactStatus::Proven);
    CHECK(v.integerReorder().status == FactStatus::Proven && v.integerReorder().detail == v.trivialReorder().detail);
    EntryState e = entry(f, 1, {{2}, {3}, {5, 0, 0, 0, 7}, {10}});
    RunResult run = runConcrete(f.program, e);
    std::optional<APInt> c0 = cellOf(run, 2, 0), c4 = cellOf(run, 2, 4);
    CHECK(c0 && *c0 == APInt(32, 11) && c4 && *c4 == APInt(32, 27));
  }

  // Case C. Roots that may alias: without the names nothing is claimed, and no fact.
  {
    Fixture f = fixture(m, "case_b", 1, 3);
    std::vector<unsigned> l = loops(f.program);
    DomainOptions silent = f.options;
    silent.authorized.clear();
    SetReductionVerdict v = verdict(f, {0, 1}, segments(f, {{0, l[0]}, {1, l[1]}}), silent, z3Backend);
    expect("may alias", v, S::Unknown, T::Proposed);
    integer("may alias", v, FactStatus::Unknown, T::Proposed, noClaim);
    Fixture g = fixture(m, "set_i8", 1, 3);
    std::vector<unsigned> gl = loops(g.program);
    DomainOptions quiet = g.options;
    quiet.authorized.clear();
    SetReductionVerdict u = verdict(g, {0, 1}, segments(g, {{0, gl[0]}, {1, gl[1]}}), quiet, z3Backend);
    CHECK(u.status() != S::Verified);
    integer("operands may alias", u, FactStatus::Unknown, T::Proposed, noClaim);
  }

  // Case E. Wrong listings: the segments backwards, an event missing, a group twice, a
  // writer left out, two groups in one block. Every verdict is as M4b-1 says, and no fact
  // is Proven: a refuted order is never rescued by the algebra of wrapping addition.
  {
    Fixture f = fixture(m, "sib_roots", 1, 3);
    std::vector<unsigned> l = loops(f.program);
    TermContext &terms = *f.program.terms;
    Term zero = terms.bv(APInt(16, 0)), n = terms.param(0, w), one = terms.bv(APInt(16, 1));
    DomainRequest backward;
    backward.segments = {segmentOf(f.program, {1}, {level(f.program, l[1], zero, n)}),
                         segmentOf(f.program, {0}, {level(f.program, l[0], zero, n)})};
    SetReductionVerdict wrong = verdict(f, {0, 1}, backward, f.options, z3Backend);
    expect("backwards", wrong, S::Refuted, T::Proposed, RefutedBy::Domain);
    integer("backwards", wrong, FactStatus::Unknown, T::Proposed, noClaim);
    CHECK(wrong.floatReorder().status == FactStatus::Unknown && wrong.trivialReorder().status != FactStatus::Proven);
    DomainRequest missing;
    missing.segments = {segmentOf(f.program, {0}, {level(f.program, l[0], zero, n)}),
                        segmentOf(f.program, {1}, {level(f.program, l[1], zero, terms.binary(TermOp::Sub, n, one))})};
    SetReductionVerdict lost = verdict(f, {0, 1}, missing, f.options, z3Backend);
    expect("a missing event", lost, S::Refuted, T::Proposed, RefutedBy::Domain);
    integer("a missing event", lost, FactStatus::Unknown, T::Proposed, noClaim);
    DomainRequest r = segments(f, {{0, l[0]}, {1, l[1]}});
    SetReductionVerdict twice = verdict(f, {0, 0}, r, f.options, z3Backend);
    expect("a group twice", twice, S::Unknown, T::Proposed);
    integer("a group twice", twice, FactStatus::Unknown, T::Proposed, noClaim);
    Fixture third = fixture(m, "set_third", 1, 3);
    std::vector<unsigned> tl = loops(third.program);
    SetReductionVerdict left = verdict(third, {0, 1}, segments(third, {{0, tl[0]}, {1, tl[1]}}), third.options, z3Backend);
    expect("a writer left out", left, S::Refuted, T::Proposed, RefutedBy::Computation);
    integer("a writer left out", left, FactStatus::Unknown, T::Proposed, noClaim);
    SetReductionVerdict all = verdict(third, {0, 1, 2}, segments(third, {{0, tl[0]}, {1, tl[1]}, {2, tl[2]}}),
                                      third.options, z3Backend);
    expect("all three writers", all, S::Verified, T::Symbolic);
    integer("all three writers", all, FactStatus::Proven, T::Symbolic, byRule);
    Fixture inter = fixture(m, "set_interleaved", 1, 3);
    std::vector<unsigned> il = loops(inter.program);
    SetReductionVerdict mixed = verdict(inter, {0, 1}, segments(inter, {{0, il[0]}, {1, il[0]}}), inter.options, z3Backend);
    expect("interleaved", mixed, S::Refuted, T::Proposed, RefutedBy::Domain);
    integer("interleaved", mixed, FactStatus::Unknown, T::Proposed, noClaim);
    // Bindings and scopes: claims about something else join to nothing, and no fact.
    ComputationOptions co;
    co.scope = f.options.scope;
    co.termination = f.options.termination;
    co.authorized = f.options.authorized;
    co.limits = f.options.computation;
    CheckedComputation cf = checkGroupSetFinite(f.program, f.enumeration, {0, 1}, co);
    std::vector<SiteGroup> both{f.enumeration.groups[0], f.enumeration.groups[1]};
    CheckedComputation cs = checkGroupSetSymbolic(f.program, f.proof(), both, co, z3Backend);
    CheckedSetDomain dfin = checkSetDomainFinite(f.program, f.enumeration, {0, 1}, r, f.options);
    CheckedSetDomain dsym = checkSetDomainSymbolic(f.program, f.proof(), both, r, f.options, z3Backend);
    SetReductionVerdict joined = joinSetReduction(f.program, f.enumeration, {0, 1}, r, f.options, cf, cs, dfin, dsym);
    expect("joined", joined, S::Verified, T::Symbolic);
    integer("joined", joined, FactStatus::Proven, T::Symbolic, byRule);
    DomainOptions otherScope = f.options;
    otherScope.scope = range(f.program, 1, 2);
    SetReductionVerdict scoped = joinSetReduction(f.program, f.enumeration, {0, 1}, r, otherScope, cf, cs, dfin, dsym);
    expect("another scope", scoped, S::Unknown, T::Proposed);
    integer("another scope", scoped, FactStatus::Unknown, T::Proposed, noClaim);
    CheckedComputation cf1 = checkGroupSetFinite(f.program, f.enumeration, {0}, co);
    SetReductionVerdict sites = joinSetReduction(f.program, f.enumeration, {0, 1}, r, f.options, cf1, cs, dfin, dsym);
    expect("another site set", sites, S::Unknown, T::Proposed);
    integer("another site set", sites, FactStatus::Unknown, T::Proposed, noClaim);
    // The computation's symbolic claim lost: the finite tier, and the fact at it.
    SetReductionVerdict finite = joinSetReduction(f.program, f.enumeration, {0, 1}, r, f.options, cf,
                                                  CheckedComputation::unknown("lost"), dfin, dsym);
    expect("computation symbolic lost", finite, S::Verified, T::ScopeEnumerated);
    integer("computation symbolic lost", finite, FactStatus::Proven, T::ScopeEnumerated, byRule);
  }

  // NB-2 where the composer does compute facts on a verdict that is not Verified: the claims
  // meet at no tier (here the domain claims are Unknown, for a flat request, or lost by hand,
  // while the computation claims are Verified), the verdict is Unknown, and no fact may rest
  // on the computation claim alone. The frozen run of 2026-11-13 let the mutant `set integer
  // fact on an unverified claim` survive for want of this assertion.
  {
    Fixture f = fixture(m, "sib_roots", 1, 3);
    std::vector<unsigned> l = loops(f.program);
    DomainRequest r = segments(f, {{0, l[0]}, {1, l[1]}});
    SetReductionVerdict flat = verdict(f, {0, 1}, DomainRequest(), f.options, z3Backend);
    expect("a flat request", flat, S::Unknown, T::Proposed);
    CHECK(flat.computationFinite().status() == S::Verified);
    integer("a flat request", flat, FactStatus::Unknown, T::Proposed, noClaim);
    CHECK(flat.floatReorder().status == FactStatus::Unknown && flat.trivialReorder().status == FactStatus::Unknown);
    CHECK(flat.floatReorder().detail == "not a float fold, or no claim");
    ComputationOptions co;
    co.scope = f.options.scope;
    co.termination = f.options.termination;
    co.authorized = f.options.authorized;
    co.limits = f.options.computation;
    CheckedComputation cf = checkGroupSetFinite(f.program, f.enumeration, {0, 1}, co);
    std::vector<SiteGroup> both{f.enumeration.groups[0], f.enumeration.groups[1]};
    CheckedComputation cs = checkGroupSetSymbolic(f.program, f.proof(), both, co, z3Backend);
    CHECK(cf.status() == S::Verified && cs.status() == S::Verified);
    SetReductionVerdict lost = joinSetReduction(f.program, f.enumeration, {0, 1}, r, f.options, cf, cs,
                                                CheckedSetDomain::unknown("lost"), CheckedSetDomain::unknown("lost"));
    expect("domain claims lost", lost, S::Unknown, T::Proposed);
    integer("domain claims lost", lost, FactStatus::Unknown, T::Proposed, noClaim);
    CHECK(lost.floatReorder().status == FactStatus::Unknown && lost.trivialReorder().status == FactStatus::Unknown);
    CHECK(lost.floatReorder().detail == "not a float fold, or no claim");
  }

  // A set of one group is Theorem 13 on the same request, facts included.
  {
    Fixture f = fixture(m, "sib_same", 1, 3);
    std::vector<unsigned> l = loops(f.program);
    DomainRequest r = segments(f, {{0, l[0]}, {1, l[1]}});
    CoordinateFamily none;
    SetReductionVerdict set = verdict(f, {0}, r, f.options, z3Backend);
    ReductionVerdict one = checkReduction(f.program, f.proof(), f.enumeration, 0, none, nullptr, r, f.options, z3Backend);
    CHECK(set.status() == S::Verified && one.status() == S::Verified);
    bool same = set.integerReorder().status == one.integerReorder().status &&
                set.integerReorder().tier == one.integerReorder().tier &&
                set.integerReorder().detail == one.integerReorder().detail &&
                set.floatReorder().status == one.floatReorder().status &&
                set.trivialReorder().status == one.trivialReorder().status;
    if (!same)
      printf("one group: set integer %s \"%s\"; T13 integer %s \"%s\"\n", stringifyFactStatus(set.integerReorder().status),
             set.integerReorder().detail.c_str(), stringifyFactStatus(one.integerReorder().status),
             one.integerReorder().detail.c_str());
    CHECK(same);
    CHECK(one.integerReorder().status == FactStatus::Proven);
  }
}

static const char *phase4Source = R"(func.func @stepped(%a: memref<1x64xi32>, %b: memref<64x2xi32>, %c: memref<1x2xi32>, %n: index) {
  %zero = arith.constant 0 : index
  cf.br ^outer(%zero : index)
^outer(%j: index):
  %two = arith.constant 2 : index
  %g = arith.cmpi ult, %j, %two : index
  cf.cond_br %g, ^start, ^exit
^start:
  %z0 = arith.constant 0 : index
  cf.br ^inner(%z0 : index)
^inner(%i: index):
  %h = arith.cmpi ult, %i, %n : index
  cf.cond_br %h, ^body, ^next
^body:
  %c0 = arith.constant 0 : index
  %x = memref.load %a[%c0, %i] : memref<1x64xi32>
  %y = memref.load %b[%i, %j] : memref<64x2xi32>
  %w = memref.load %c[%c0, %j] : memref<1x2xi32>
  %m = arith.muli %x, %y : i32
  %s = arith.addi %w, %m : i32
  memref.store %s, %c[%c0, %j] : memref<1x2xi32>
  %c2 = arith.constant 2 : index
  %c3 = arith.constant 3 : index
  %c7 = arith.constant 7 : index
  %r = arith.remui %i, %c2 : index
  %even = arith.cmpi eq, %r, %c0 : index
  %step = arith.select %even, %c7, %c3 : index
  %ni = arith.addi %i, %step : index
  cf.br ^inner(%ni : index)
^next:
  %one = arith.constant 1 : index
  %nj = arith.addi %j, %one : index
  cf.br ^outer(%nj : index)
^exit:
  return
}
func.func @rotated(%a: memref<9x9xi32>, %b: memref<9x2xi32>, %c: memref<9x2xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  %two_n = arith.muli %n, %c2 : index
  scf.for %i = %c0 to %c2 step %c1 {
    scf.for %t = %c0 to %two_n step %c1 {
      %p = arith.cmpi ult, %t, %n : index
      scf.if %p {
        %x = memref.load %a[%i, %t] : memref<9x9xi32>
        %y = memref.load %b[%t, %c0] : memref<9x2xi32>
        %z = memref.load %c[%i, %c0] : memref<9x2xi32>
        %m = arith.muli %x, %y : i32
        %s = arith.addi %z, %m : i32
        memref.store %s, %c[%i, %c0] : memref<9x2xi32>
      } else {
        %u = arith.addi %t, %c1 : index
        %v = arith.subi %u, %n : index
        %r = arith.remui %v, %n : index
        %x = memref.load %a[%i, %r] : memref<9x9xi32>
        %y = memref.load %b[%r, %c1] : memref<9x2xi32>
        %z = memref.load %c[%i, %c1] : memref<9x2xi32>
        %m = arith.muli %x, %y : i32
        %s = arith.addi %z, %m : i32
        memref.store %s, %c[%i, %c1] : memref<9x2xi32>
      }
    }
  }
  return
})";

/// Phase 4 (I1 to I3, docs/GENERALIZED_GEMM_STATEMENT_BV.md Section 11): the closed
/// forms and the counters through the whole pipeline, with the three questions kept
/// apart: decomposition verified, views proved and affine, lowering eligible.
static void testPhase4Views() {
  // affineOverLevels is a syntactic test.
  {
    TermContext terms;
    Term x = levelVariable(terms, 0, Sort::bv(8)), y = levelVariable(terms, 1, Sort::bv(8));
    Term n = terms.param(0, Sort::bv(8)), two = terms.bv(APInt(8, 2));
    CHECK(affineOverLevels(x));
    CHECK(affineOverLevels(terms.binary(TermOp::Add, terms.binary(TermOp::Mul, two, x), n)));
    CHECK(affineOverLevels(terms.binary(TermOp::Sub, terms.binary(TermOp::Mul, x, two), y)));
    CHECK(!affineOverLevels(terms.binary(TermOp::Mul, x, y)));
    CHECK(!affineOverLevels(terms.binary(TermOp::URem, x, two)));
    CHECK(!affineOverLevels(terms.binary(TermOp::UDiv, x, two)));
    CHECK(!affineOverLevels(terms.ite(terms.icmp(Predicate::Ult, x, n), x, y)));
    CHECK(!affineOverLevels(terms.zext(x, 16)));
  }
  Module m(phase4Source);
  // stepped with counters: K is the inner counter; the proved closed form i = f(c)
  // becomes an identity, the certificate is strengthened with it and the counter's
  // bounds, the domain and Theorem 13 are verified over the scope, the views A and B
  // are proved and not affine, and lowering is not eligible. Without the option, the
  // same family stops at the proposer.
  {
    SemanticProgram original = m.import("stepped", 8);
    SemanticProgram p = original;
    CHECK(addIterationCounters(p).reason.empty());
    ParameterScope scope = range(p, 8, 40);
    TerminationOptions termination;
    termination.scope = scope;
    termination.limits.solver = testSolver();
    auto certificate = analyzeTermination(original, z3Backend, termination).certificate;
    CHECK(certificate.has_value());
    if (!certificate)
      return;
    DomainAnalysisRequest request;
    request.synthesis.scope = scope;
    request.synthesis.maxLevel = 0;
    request.synthesis.synthesis.maxCandidates = 2;
    request.synthesis.synthesis.solver = testSolver();
    request.synthesis.termination.solver = testSolver();
    request.synthesis.authorized = everyPair(p);
    request.domain.scope = scope;
    request.domain.termination.solver = testSolver();
    request.domain.authorized = everyPair(p);
    request.maxCandidates = 1;
    for (bool views : {false, true}) {
      request.closedFormViews = views;
      request.closedForms.scope = scope;
      request.closedForms.solver = testSolver();
      DomainResult result = analyzeDomains(p, &*certificate, request, z3Backend);
      CHECK(result.enumeration.gate == GateStatus::Open && result.groups.size() == 1 &&
            result.groups[0].candidates.size() == 1);
      if (result.groups.empty() || result.groups[0].candidates.empty())
        continue;
      const CandidateDomain &c = result.groups[0].candidates[0];
      Term inner = p.terms->state(p.iterationCounters.back(), Sort::bv(8));
      CHECK(llvm::is_contained(c.family.axes[AxisK], inner));
      if (!views) {
        CHECK(!result.closedForms.has_value() && result.identities.empty());
        CHECK(!c.certificateStrengthened && c.invariantsAdded.empty());
        // The guard needs the identity: the loop condition reads i, which is no level.
        CHECK(!c.request.has_value());
        CHECK(c.proposalReason.find("no guard") != std::string::npos);
        CHECK(c.verdict.status() != ComputationStatus::Verified);
        continue;
      }
      CHECK(result.closedForms.has_value() && result.identities.size() == 1);
      if (!result.identities.empty()) {
        CHECK(result.identities[0].state == 1 && result.identities[0].cut == 3);
        CHECK(!affineOverLevels(result.identities[0].term));
        CHECK(llvm::is_contained(stateSupport(result.identities[0].term), p.iterationCounters.back()));
      }
      CHECK(c.request.has_value());
      if (!c.request)
        continue; // A failed proposal: the checks below read it, and a mutant must die by a CHECK, not a crash.
      CHECK(c.certificateStrengthened && c.invariantsAdded.size() == 2);
      CHECK(c.certificateNote.find("accepts") != std::string::npos);
      const ReductionVerdict &v = c.verdict;
      CHECK(v.coordinateFinite().tier() == EvidenceTier::ScopeEnumerated);
      CHECK(v.coordinateSymbolic().tier() != EvidenceTier::SymbolicExact); // No order bounds the counter.
      CHECK(v.domainSymbolic().status() == ComputationStatus::Verified && v.domainSymbolic().exactOrder());
      CHECK(v.status() == ComputationStatus::Verified && v.tier() == ComputationTier::Symbolic);
      for (unsigned view = 0; view < 3; ++view) {
        CHECK(!c.request->access[view].empty());
        CHECK(v.domainSymbolic().access()[view].status == FactStatus::Proven);
        CHECK(v.domainFinite().access()[view].status == FactStatus::Proven);
      }
      // C = (0, j) is affine; A = (0, f(c)) and B = (f(c), j) are not.
      bool affineC = true, affineA = true, affineB = true;
      for (Term t : c.request->access[0]) affineC = affineC && affineOverLevels(t);
      for (Term t : c.request->access[1]) affineA = affineA && affineOverLevels(t);
      for (Term t : c.request->access[2]) affineB = affineB && affineOverLevels(t);
      CHECK(affineC && !affineA && !affineB);
      std::string json = toJSON(p, result, false);
      CHECK(json.find("\"lowering_eligible\": false") != std::string::npos);
      CHECK(json.find("a view is not affine") != std::string::npos);
      CHECK(json.find("\"decomposition\": \"VERIFIED at SYMBOLIC\"") != std::string::npos);
      CHECK(json.find("\"authorizes_lowering\": false") != std::string::npos);
      CHECK(json.find("\"authorizes_lowering\": true") == std::string::npos);
      CHECK(json.find("\"strengthened\": true") != std::string::npos);
    }
  }
  // rotated with the statement family (I2): the entry definition two_n = 2n lets the
  // proposer bound t, the sample-checked views are the level variables (I, K), (K, J),
  // (I, J), all affine, and the integer reorder fact holds: eligible under (V).
  {
    SemanticProgram p = m.import("rotated", 8);
    ParameterScope scope = range(p, 2, 3);
    TerminationOptions termination;
    termination.scope = scope;
    termination.limits.solver = testSolver();
    auto certificate = analyzeTermination(p, z3Backend, termination).certificate;
    CHECK(certificate.has_value());
    if (!certificate)
      return;
    DomainAnalysisRequest request;
    request.synthesis.scope = scope;
    request.synthesis.maxLevel = 3;
    request.synthesis.statementGrammar = true;
    request.synthesis.synthesis.maxCandidates = 1;
    request.synthesis.synthesis.solver = testSolver();
    request.synthesis.termination.solver = testSolver();
    request.synthesis.authorized = everyPair(p);
    request.domain.scope = scope;
    request.domain.termination.solver = testSolver();
    request.domain.authorized = everyPair(p);
    request.maxCandidates = 1;
    request.closedFormViews = true;
    request.closedForms.scope = scope;
    request.closedForms.solver = testSolver();
    DomainResult result = analyzeDomains(p, &*certificate, request, z3Backend);
    CHECK(result.groups.size() == 1 && result.groups[0].candidates.size() == 1);
    if (result.groups.empty() || result.groups[0].candidates.empty())
      return;
    const CandidateDomain &c = result.groups[0].candidates[0];
    bool twoN = false;
    for (const StateIdentity &identity : result.identities)
      twoN = twoN || (identity.cut < 0 && identity.term->op == TermOp::Mul);
    CHECK(twoN);
    CHECK(c.request.has_value() && c.certificateStrengthened);
    if (!c.request)
      return;
    const ReductionVerdict &v = c.verdict;
    CHECK(v.status() == ComputationStatus::Verified && v.tier() == ComputationTier::Symbolic);
    CHECK(v.domainSymbolic().exactOrder());
    for (unsigned view = 0; view < 3; ++view) {
      CHECK(c.request->access[view].size() == 2);
      for (Term t : c.request->access[view])
        CHECK(levelOf(t).has_value()); // Each dimension is a level variable.
      CHECK(v.domainSymbolic().access()[view].status == FactStatus::Proven);
    }
    CHECK(v.integerReorder().status == FactStatus::Proven);
    std::string json = toJSON(p, result, false);
    CHECK(json.find("\"lowering_eligible\": true") != std::string::npos);
    CHECK(json.find("\"views_affine\": true") != std::string::npos);
    CHECK(json.find("\"authorizes_lowering\": true") == std::string::npos);
  }
}

/// G5b-M4b-2b-1: programs for the float reorder fact over a set: binary32, two operand
/// roots of their own, a repeat (three contributions to a cell), and one group over two
/// loops.
static const char *floatSource = R"(func.func @set_float32(%a: memref<8xf32>, %b: memref<8xf32>, %c: memref<8xf32>, %d: memref<8xf32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  scf.for %i = %c0 to %n step %c1 {
    %xi = memref.load %a[%i] : memref<8xf32>
    %yi = memref.load %b[%i] : memref<8xf32>
    %zi = memref.load %c[%i] : memref<8xf32>
    %mi = arith.mulf %xi, %yi : f32
    %si = arith.addf %zi, %mi : f32
    memref.store %si, %c[%i] : memref<8xf32>
  }
  scf.for %j = %c0 to %n step %c1 {
    %xj = memref.load %a[%j] : memref<8xf32>
    %yj = memref.load %d[%j] : memref<8xf32>
    %zj = memref.load %c[%j] : memref<8xf32>
    %mj = arith.mulf %xj, %yj : f32
    %sj = arith.addf %zj, %mj : f32
    memref.store %sj, %c[%j] : memref<8xf32>
  }
  return
}
func.func @set_float_roots(%a: memref<8xf64>, %b: memref<8xf64>, %c: memref<8xf64>, %d: memref<8xf64>, %e: memref<8xf64>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  scf.for %i = %c0 to %n step %c1 {
    %xi = memref.load %a[%i] : memref<8xf64>
    %yi = memref.load %b[%i] : memref<8xf64>
    %zi = memref.load %c[%i] : memref<8xf64>
    %mi = arith.mulf %xi, %yi : f64
    %si = arith.addf %zi, %mi : f64
    memref.store %si, %c[%i] : memref<8xf64>
  }
  scf.for %j = %c0 to %n step %c1 {
    %xj = memref.load %d[%j] : memref<8xf64>
    %yj = memref.load %e[%j] : memref<8xf64>
    %zj = memref.load %c[%j] : memref<8xf64>
    %mj = arith.mulf %xj, %yj : f64
    %sj = arith.addf %zj, %mj : f64
    memref.store %sj, %c[%j] : memref<8xf64>
  }
  return
}
func.func @set_float_repeat(%a: memref<8xf64>, %b: memref<8xf64>, %c: memref<8xf64>, %d: memref<8xf64>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  scf.for %i = %c0 to %n step %c1 {
    %xi = memref.load %a[%i] : memref<8xf64>
    %yi = memref.load %b[%i] : memref<8xf64>
    %zi = memref.load %c[%i] : memref<8xf64>
    %mi = arith.mulf %xi, %yi : f64
    %si = arith.addf %zi, %mi : f64
    memref.store %si, %c[%i] : memref<8xf64>
  }
  scf.for %j = %c0 to %n step %c1 {
    scf.for %k = %c0 to %c2 step %c1 {
      %xj = memref.load %a[%j] : memref<8xf64>
      %yj = memref.load %d[%j] : memref<8xf64>
      %zj = memref.load %c[%j] : memref<8xf64>
      %mj = arith.mulf %xj, %yj : f64
      %sj = arith.addf %zj, %mj : f64
      memref.store %sj, %c[%j] : memref<8xf64>
    }
  }
  return
}
func.func @set_float_same(%a: memref<8xf64>, %b: memref<8xf64>, %c: memref<8xf64>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  scf.for %i = %c0 to %n step %c1 {
    %xi = memref.load %a[%i] : memref<8xf64>
    %yi = memref.load %b[%i] : memref<8xf64>
    %zi = memref.load %c[%i] : memref<8xf64>
    %mi = arith.mulf %xi, %yi : f64
    %si = arith.addf %zi, %mi : f64
    memref.store %si, %c[%i] : memref<8xf64>
  }
  scf.for %j = %c0 to %n step %c1 {
    %xj = memref.load %a[%j] : memref<8xf64>
    %yj = memref.load %b[%j] : memref<8xf64>
    %zj = memref.load %c[%j] : memref<8xf64>
    %mj = arith.mulf %xj, %yj : f64
    %sj = arith.addf %zj, %mj : f64
    memref.store %sj, %c[%j] : memref<8xf64>
  }
  return
}
)";

/// G5b-M4b-2b-1: the float reorder fact over a set, Refuted by a witness and never Proven
/// but by `trivial`. binary64 in both orders (0 and 1), binary32 at 2^24, infinity in one
/// order only, a NaN reversal (that run would stop), the sign of a zero (no difference under
/// round to nearest even), two operand roots of their own, three contributions to a cell,
/// the listing, no witness within the fills, a fold of two operations, a refuted order, a set
/// of one group against Theorem 13, and the verdict untouched by a refuted fact.
static void testSetFloat() {
  Module m(std::string(setSource) + siblingSource + reductionSource + reorderSource + floatSource);
  Sort w = Sort::bv(16);
  auto level = [&](const SemanticProgram &p, unsigned var, Term lower, Term upper) {
    return LevelSpec{p.terms->state(var, w), lower, upper, false, false};
  };
  auto segments = [&](Fixture &f, std::vector<std::pair<unsigned, unsigned>> siteVars) {
    TermContext &terms = *f.program.terms;
    Term zero = terms.bv(APInt(16, 0)), n = terms.param(0, w);
    DomainRequest r;
    for (auto [site, var] : siteVars)
      r.segments.push_back(segmentOf(f.program, {site}, {level(f.program, var, zero, n)}));
    return r;
  };
  auto verdict = [&](Fixture &f, std::vector<unsigned> groups, const DomainRequest &r, const DomainOptions &o,
                     SmtBackend &backend) {
    return checkSetReduction(f.program, f.proof(), f.enumeration, groups, r, o, backend);
  };
  auto say = [](const char *what, const SetReductionVerdict &v) {
    printf("%s: %s %s (%s); float %s \"%s\"; integer %s \"%s\"\n", what, stringifyComputationStatus(v.status()),
           stringifyComputationTier(v.tier()), v.reason().c_str(), stringifyFactStatus(v.floatReorder().status),
           v.floatReorder().detail.c_str(), stringifyFactStatus(v.integerReorder().status),
           v.integerReorder().detail.c_str());
  };
  auto expect = [&](const char *what, const SetReductionVerdict &v, ComputationStatus status, ComputationTier tier) {
    bool ok = v.status() == status && v.tier() == tier;
    if (!ok)
      say(what, v);
    CHECK(ok);
  };
  // The bits of a value at a root's width: binary64, or binary32 where the root is 32 bits.
  auto bitsAt = [](unsigned width, double value) {
    if (width == 32) {
      float narrow = (float)value;
      uint32_t raw;
      std::memcpy(&raw, &narrow, sizeof raw);
      return APInt(32, raw);
    }
    uint64_t raw;
    std::memcpy(&raw, &value, sizeof raw);
    return APInt(64, raw);
  };
  // An entry at n = `point` with every root's first cells as given (as doubles), the rest 0.
  auto entry = [&](const Fixture &f, unsigned point, std::vector<std::vector<double>> firstCells) {
    EntryState out;
    out.parameters = {APInt(16, point)};
    for (unsigned r = 0; r < f.program.roots.size(); ++r) {
      RootData data;
      data.shape = {8};
      unsigned width = f.program.roots[r].sort.width;
      for (unsigned i = 0; i < 8; ++i) {
        double value = r < firstCells.size() && i < firstCells[r].size() ? firstCells[r][i] : 0.0;
        data.cells.push_back(bitsAt(width, value));
      }
      out.memory.push_back(data);
    }
    return out;
  };
  // The judge over the union, from the set's own replay at `point`; `ok` says the union and
  // its replay were there to judge against.
  auto judge = [&](Fixture &f, std::vector<unsigned> groups, unsigned point, const EntryState &e, std::string &detail,
                   bool &ok) {
    std::vector<const SiteGroup *> listed;
    for (unsigned g : groups)
      listed.push_back(&f.enumeration.groups[g]);
    std::vector<int> groupOf;
    std::string why;
    std::optional<SiteGroup> united = unitedSiteGroup(f.program, listed, groupOf, why);
    ok = united.has_value();
    if (!united)
      return false;
    ReplayResult replay = replayGroupSet(f.program, *united, groupOf, listed.size(), {APInt(16, point)},
                                         HerbrandLimits());
    ok = replay.outcome == ReplayOutcome::Equal;
    return reversalDiffers(f.program, *united, e, replay.prefix, detail);
  };
  // The program's final value of C[0], as bits, when the run completes.
  auto final0 = [&](Fixture &f, const EntryState &e) -> std::optional<APInt> {
    RunResult run = runConcrete(f.program, e);
    if (run.status != RunStatus::Returned || run.memory.size() <= 2 || run.memory[2].cells.empty())
      return std::nullopt;
    return run.memory[2].cells[0];
  };
  // A hand-built witness: the program completes with `programValue` in C[0]; the judge over
  // the union finds the reversal different, with `words` in its detail.
  auto witness = [&](const char *what, Fixture &f, std::vector<unsigned> groups, unsigned point, const EntryState &e,
                     double programValue, const char *words) {
    std::optional<APInt> c0 = final0(f, e);
    unsigned width = f.program.roots[2].sort.width;
    bool valueOk = c0 && *c0 == bitsAt(width, programValue);
    std::string detail;
    bool ok = false;
    bool differs = judge(f, groups, point, e, detail, ok);
    bool said = detail.find(words) != std::string::npos;
    if (!(valueOk && ok && differs && said))
      printf("%s: value %s, replay %s, differs %s, detail \"%s\"\n", what, valueOk ? "ok" : "WRONG", ok ? "ok" : "NOT EQUAL",
             differs ? "yes" : "no", detail.c_str());
    CHECK(valueOk && ok && differs && said);
  };
  const double inf = std::numeric_limits<double>::infinity(), big = std::numeric_limits<double>::max();
  const std::string noFloat = "not a float fold, or no claim", noInteger = "not an integer fold, or no claim";

  // binary64. The search's fixed fill sequence refutes set_float (as the setreduction section
  // asserts); the witness is replayed and judged again over the union. Then the brief's two
  // memories, in both orders: C[0] = 1e16 with A·B = 1 then A·D = −1e16 gives (1e16 ⊕ 1) ⊕
  // −1e16 = 0, the other order 1; with the contributions exchanged the program gives
  // (1e16 ⊕ −1e16) ⊕ 1 = 1, the other order 0.
  {
    Fixture f = fixture(m, "set_float", 1, 3);
    std::vector<unsigned> l = loops(f.program);
    DomainRequest r = segments(f, {{0, l[0]}, {1, l[1]}});
    SetReductionVerdict v = verdict(f, {0, 1}, r, f.options, z3Backend);
    expect("binary64", v, S::Verified, T::Symbolic);
    CHECK(v.fold() == FoldForm::OrderedFloat && v.trivialReorder().status == FactStatus::Refuted);
    CHECK(v.floatReorder().status == FactStatus::Refuted && v.floatReorder().point && v.floatReorder().memory);
    CHECK(v.integerReorder().status == FactStatus::Unknown && v.integerReorder().detail == noInteger);
    if (v.floatReorder().point && v.floatReorder().memory) {
      CHECK(runConcrete(f.program, *v.floatReorder().memory).status == RunStatus::Returned);
      std::string detail;
      bool ok = false;
      CHECK(judge(f, {0, 1}, (*v.floatReorder().point)[0].getZExtValue(), *v.floatReorder().memory, detail, ok) && ok);
    }
    // The verdict is what M4b-1 says, fact or no fact: the computation claims stand.
    CHECK(v.computationSymbolic().status() == S::Verified && v.computationFinite().status() == S::Verified &&
          v.domainSymbolic().status() == S::Verified && v.points().empty() && !v.witness() && !v.failingPoint());
    SetReductionVerdict back = verdict(f, {1, 0}, r, f.options, z3Backend);
    CHECK(back.status() == v.status() && back.tier() == v.tier() && back.binding() == v.binding() &&
          back.floatReorder().status == v.floatReorder().status && back.floatReorder().detail == v.floatReorder().detail &&
          back.floatReorder().point == v.floatReorder().point);
    // (1e16 ⊕ 1) ⊕ −1e16 = 0; reversed 1.
    witness("1e16 + 1 - 1e16", f, {0, 1}, 1, entry(f, 1, {{1.0}, {1.0}, {1e16}, {-1e16}}), 0.0, "changes its final bits");
    CHECK((1e16 + 1.0) + -1e16 == 0.0 && (1e16 + -1e16) + 1.0 == 1.0);
    // (1e16 ⊕ −1e16) ⊕ 1 = 1; reversed 0.
    witness("1e16 - 1e16 + 1", f, {0, 1}, 1, entry(f, 1, {{1.0}, {-1e16}, {1e16}, {1.0}}), 1.0, "changes its final bits");
    // Infinity in one order only: (max ⊕ max) ⊕ −max = +∞ completes; (max ⊕ −max) ⊕ max = max.
    witness("infinity", f, {0, 1}, 1, entry(f, 1, {{big}, {1.0}, {big}, {-1.0}}), inf, "changes its final bits");
    CHECK((big + big) + -big == inf && (big + -big) + big == big);
    // A NaN reversal: (1e308 ⊕ −∞) ⊕ 1e308 = −∞ completes; (1e308 ⊕ 1e308) ⊕ −∞ is NaN, and that
    // run would stop. A = 1e308, B = −∞, D = 1.
    witness("NaN reversal", f, {0, 1}, 1, entry(f, 1, {{1e308}, {-inf}, {1e308}, {1.0}}), -inf, "gives NaN, so that run would stop");
    // The sign of a zero: with A = −1, B = D = 0 both contributions are −0; (+0 ⊕ −0) ⊕ −0 = +0 in
    // either order, and from −0 both orders give −0. No difference, so no witness.
    for (double start : {0.0, -0.0}) {
      EntryState e = entry(f, 1, {{-1.0}, {0.0}, {start}, {0.0}});
      std::optional<APInt> c0 = final0(f, e);
      CHECK(c0 && *c0 == bitsAt(64, start));
      std::string detail;
      bool ok = false;
      CHECK(!judge(f, {0, 1}, 1, e, detail, ok) && ok);
    }
    // No witness within the fills: Unknown, with what the search found, never Proven.
    DomainOptions none = f.options;
    none.limits.witnessFills = 0;
    SetReductionVerdict open = verdict(f, {0, 1}, r, none, z3Backend);
    expect("no fills", open, S::Verified, T::Symbolic);
    CHECK(open.floatReorder().status == FactStatus::Unknown &&
          open.floatReorder().detail == "no memory found at which a reversal differs");
    // A refuted order: the verdict is refuted by the domain, and no fact is made.
    TermContext &terms = *f.program.terms;
    Term zero = terms.bv(APInt(16, 0)), n = terms.param(0, w);
    DomainRequest backward;
    backward.segments = {segmentOf(f.program, {1}, {level(f.program, l[1], zero, n)}),
                         segmentOf(f.program, {0}, {level(f.program, l[0], zero, n)})};
    SetReductionVerdict wrong = verdict(f, {0, 1}, backward, f.options, z3Backend);
    CHECK(wrong.status() == S::Refuted && wrong.refutedBy() == RefutedBy::Domain &&
          wrong.floatReorder().status == FactStatus::Unknown && wrong.integerReorder().status == FactStatus::Unknown);
  }
  // binary32: 2^24 + 1 rounds to 2^24, so (2^24 ⊕ 1) ⊕ −2^24 = 0 and the other order 1; with
  // the contributions exchanged, 1 and 0. The search's outcome is a fact of the fill
  // sequence; whatever it is, the fact is never Proven, and a refutation is replayed.
  {
    Fixture f = fixture(m, "set_float32", 1, 3);
    std::vector<unsigned> l = loops(f.program);
    DomainRequest r = segments(f, {{0, l[0]}, {1, l[1]}});
    SetReductionVerdict v = verdict(f, {0, 1}, r, f.options, z3Backend);
    expect("binary32", v, S::Verified, T::Symbolic);
    CHECK(v.fold() == FoldForm::OrderedFloat && v.floatReorder().status != FactStatus::Proven);
    printf("binary32: float %s: %s\n", stringifyFactStatus(v.floatReorder().status), v.floatReorder().detail.c_str());
    if (v.floatReorder().status == FactStatus::Refuted) {
      CHECK(v.floatReorder().point && v.floatReorder().memory);
      if (v.floatReorder().point && v.floatReorder().memory) {
        CHECK(runConcrete(f.program, *v.floatReorder().memory).status == RunStatus::Returned);
        std::string detail;
        bool ok = false;
        CHECK(judge(f, {0, 1}, (*v.floatReorder().point)[0].getZExtValue(), *v.floatReorder().memory, detail, ok) && ok);
      }
    }
    const double two24 = 16777216.0;
    witness("2^24 + 1 - 2^24", f, {0, 1}, 1, entry(f, 1, {{1.0}, {1.0}, {two24}, {-two24}}), 0.0, "changes its final bits");
    witness("2^24 - 2^24 + 1", f, {0, 1}, 1, entry(f, 1, {{1.0}, {-two24}, {two24}, {1.0}}), 1.0, "changes its final bits");
    CHECK((float)((float)(two24 + 1.0f) + -two24) == 0.0f && (float)((float)(two24 + -two24) + 1.0f) == 1.0f);
  }
  // Two operand roots of their own (A·B then D·E), and three contributions to a cell (the
  // second group's repeat): the same memories refute.
  {
    Fixture f = fixture(m, "set_float_roots", 1, 3);
    std::vector<unsigned> l = loops(f.program);
    SetReductionVerdict v = verdict(f, {0, 1}, segments(f, {{0, l[0]}, {1, l[1]}}), f.options, z3Backend);
    expect("roots of their own", v, S::Verified, T::Symbolic);
    CHECK(v.floatReorder().status != FactStatus::Proven);
    witness("roots of their own", f, {0, 1}, 1, entry(f, 1, {{1.0}, {1.0}, {1e16}, {1.0}, {-1e16}}), 0.0,
            "changes its final bits");
    Fixture g = fixture(m, "set_float_repeat", 1, 3);
    std::vector<unsigned> gl = loops(g.program);
    CHECK(gl.size() == 3);
    TermContext &terms = *g.program.terms;
    Term zero = terms.bv(APInt(16, 0)), n = terms.param(0, w), two = terms.bv(APInt(16, 2));
    DomainRequest r;
    r.segments = {segmentOf(g.program, {0}, {level(g.program, gl[0], zero, n)}),
                  segmentOf(g.program, {1}, {level(g.program, gl[1], zero, n), level(g.program, gl[2], zero, two)})};
    SetReductionVerdict u = verdict(g, {0, 1}, r, g.options, z3Backend);
    expect("repeat", u, S::Verified, T::Symbolic);
    CHECK(u.floatReorder().status != FactStatus::Proven);
    // (1e16 ⊕ 1) ⊕ −5e15 ⊕ −5e15 = 0; reversed (1e16 ⊕ −5e15 ⊕ −5e15) ⊕ 1 = 1.
    witness("repeat", g, {0, 1}, 1, entry(g, 1, {{1.0}, {1.0}, {1e16}, {-5e15}}), 0.0, "changes its final bits");
  }
  // A fold of two operations (set_mixed: two integer groups and a float group): the judge
  // refuses to reverse it, so no witness and no fact; the verdict stands.
  {
    Fixture f = fixture(m, "set_mixed", 1, 3);
    std::vector<unsigned> l = loops(f.program);
    SetReductionVerdict v = verdict(f, {0, 1, 2}, segments(f, {{0, l[0]}, {1, l[1]}, {2, l[2]}}), f.options, z3Backend);
    expect("mixed", v, S::Verified, T::Symbolic);
    CHECK(v.fold() == FoldForm::OrderedFloat && v.floatReorder().status == FactStatus::Unknown &&
          v.floatReorder().detail == "no memory found at which a reversal differs" &&
          v.integerReorder().status == FactStatus::Unknown && v.integerReorder().detail == noInteger);
  }
  // Integer sets keep their float fact as it was.
  {
    Fixture f = fixture(m, "sib_roots", 1, 3);
    std::vector<unsigned> l = loops(f.program);
    SetReductionVerdict v = verdict(f, {0, 1}, segments(f, {{0, l[0]}, {1, l[1]}}), f.options, z3Backend);
    CHECK(v.status() == S::Verified && v.integerReorder().status == FactStatus::Proven &&
          v.floatReorder().status == FactStatus::Unknown && v.floatReorder().detail == noFloat);
  }
  // A set of one float group is Theorem 13 on the same request, facts included.
  {
    Fixture f = fixture(m, "set_float_same", 1, 3);
    std::vector<unsigned> l = loops(f.program);
    DomainRequest r = segments(f, {{0, l[0]}, {1, l[1]}});
    CoordinateFamily none;
    SetReductionVerdict set = verdict(f, {0}, r, f.options, z3Backend);
    ReductionVerdict one = checkReduction(f.program, f.proof(), f.enumeration, 0, none, nullptr, r, f.options, z3Backend);
    bool same = set.status() == one.status() && set.tier() == one.tier() &&
                set.floatReorder().status == one.floatReorder().status &&
                set.floatReorder().detail == one.floatReorder().detail &&
                set.floatReorder().point == one.floatReorder().point &&
                set.integerReorder().status == one.integerReorder().status &&
                set.trivialReorder().status == one.trivialReorder().status;
    if (!same)
      printf("one float group: set %s float %s \"%s\"; T13 %s float %s \"%s\"\n", stringifyComputationStatus(set.status()),
             stringifyFactStatus(set.floatReorder().status), set.floatReorder().detail.c_str(),
             stringifyComputationStatus(one.status()), stringifyFactStatus(one.floatReorder().status),
             one.floatReorder().detail.c_str());
    CHECK(same);
    CHECK(set.status() == S::Verified);
    printf("one float group: %s: %s\n", stringifyFactStatus(set.floatReorder().status), set.floatReorder().detail.c_str());
  }
}

struct DesignedNest {
  std::string out, a, b;
  bool repeat = false;
  std::string bIndex;   // The second operand's index; the loop variable when empty.
  std::string outIndex; // The output's index; the loop variable when empty.
};

/// G5b-M4b-2b-2. A program of nests run one after the other over roots of eight cells. Each
/// nest is `for v in [0, n)`, with `repeat` around an inner loop of two, and holds the site
/// out[v or outIndex] += a[v] * b[v or bIndex]; `flag` goes on the multiplication and on the addition,
/// as signed arithmetic puts nsw on both. The integer programs are sixteen bits wide: the
/// solver's multiplications are then small, and the search is the same at every width.
static std::string designedProgram(const std::string &name, const std::string &element, const std::string &flag,
                                   const std::vector<DesignedNest> &nests, const std::string &before = "") {
  bool isFloat = element[0] == 'f';
  std::string type = "memref<8x" + element + ">", mul = isFloat ? "arith.mulf" : "arith.muli",
              add = isFloat ? "arith.addf" : "arith.addi";
  std::string out = "func.func @" + name + "(%a: " + type + ", %b: " + type + ", %c: " + type + ", %d: " + type +
                    ", %n: index) {\n  %c0 = arith.constant 0 : index\n  %c1 = arith.constant 1 : index\n"
                    "  %c2 = arith.constant 2 : index\n" + before;
  for (size_t k = 0; k < nests.size(); ++k) {
    const DesignedNest &nest = nests[k];
    std::string s = std::to_string(k), v = "%v" + s, in = nest.repeat ? "      " : "    ";
    std::string h = nest.outIndex.empty() ? v : nest.outIndex;
    out += "  scf.for " + v + " = %c0 to %n step %c1 {\n";
    if (nest.repeat)
      out += "    scf.for %r" + s + " = %c0 to %c2 step %c1 {\n";
    out += in + "%x" + s + " = memref.load %" + nest.a + "[" + v + "] : " + type + "\n" + in + "%y" + s +
           " = memref.load %" + nest.b + "[" + (nest.bIndex.empty() ? v : nest.bIndex) + "] : " + type + "\n" + in +
           "%z" + s + " = memref.load %" + nest.out + "[" + h + "] : " + type + "\n" + in + "%m" + s + " = " + mul +
           " %x" + s + ", %y" + s + flag + " : " + element + "\n" + in + "%s" + s + " = " + add + " %z" + s + ", %m" +
           s + flag + " : " + element + "\n" + in + "memref.store %s" + s + ", %" + nest.out + "[" + h + "] : " +
           type + "\n";
    if (nest.repeat)
      out += "    }\n";
    out += "  }\n";
  }
  return out + "  return\n}\n";
}

static std::string designedSource() {
  const std::string nsw = " overflow<nsw>";
  std::string out;
  // Two groups write C[i]: A*B, then A*D.
  out += designedProgram("d_set", "i16", nsw, {{"c", "a", "b"}, {"c", "a", "d"}});
  // The second group's first operand is the first group's second: A*B, then B*D.
  out += designedProgram("d_shared", "i16", nsw, {{"c", "a", "b"}, {"c", "b", "d"}});
  // The second group runs twice: A*B, then A*D and A*D again.
  out += designedProgram("d_repeat", "i16", nsw, {{"c", "a", "b"}, {"c", "a", "d", true}});
  // One group, two sites: A[i]*B[0], then A[j]*B[j]. Cell 0 gets one product twice.
  out += designedProgram("d_second", "i16", nsw, {{"c", "a", "b", false, "%c0"}, {"c", "a", "b"}});
  // The second group adds every product to C[3]: at n = 4 cells 0, 1 and 2 get one
  // contribution each, and cell 3 five.
  out += designedProgram("d_late", "i16", nsw, {{"c", "a", "b"}, {"c", "a", "d", false, "", "%c3"}},
                         "  %c3 = arith.constant 3 : index\n");
  // C[0] is set to a constant before any event.
  out += designedProgram("d_constant", "i16", nsw, {{"c", "a", "b"}, {"c", "a", "d"}},
                         "  %five = arith.constant 5 : i16\n  memref.store %five, %c[%c0] : memref<8xi16>\n");
  // One float group of two sites: A[i]*B[0], then A[j]*B[j].
  out += designedProgram("d_floats", "f64", "", {{"c", "a", "b", false, "%c0"}, {"c", "a", "b"}});
  // nsw and nuw on both operations.
  out += designedProgram("d_both", "i16", " overflow<nsw, nuw>", {{"c", "a", "b"}, {"c", "a", "d"}});
  // The first group's second operand is a root the program allocates and fills with ones.
  out += designedProgram("d_fresh_operand", "i16", nsw, {{"c", "a", "f"}, {"c", "a", "d"}},
                         "  %f = memref.alloc() : memref<8xi16>\n  %one = arith.constant 1 : i16\n"
                         "  %c8 = arith.constant 8 : index\n  scf.for %w = %c0 to %c8 step %c1 {\n"
                         "    memref.store %one, %f[%w] : memref<8xi16>\n  }\n");
  // One group, one site, three contributions to a cell: a matrix product, C[i, j] += A[i, k] * B[k, j].
  out += R"(func.func @d_gemm(%a: memref<8x8xi16>, %b: memref<8x8xi16>, %c: memref<8x8xi16>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c3 = arith.constant 3 : index
  scf.for %i = %c0 to %n step %c1 {
    scf.for %j = %c0 to %c3 step %c1 {
      scf.for %k = %c0 to %c3 step %c1 {
        %x = memref.load %a[%i, %k] : memref<8x8xi16>
        %y = memref.load %b[%k, %j] : memref<8x8xi16>
        %z = memref.load %c[%i, %j] : memref<8x8xi16>
        %m = arith.muli %x, %y overflow<nsw> : i16
        %s = arith.addi %z, %m overflow<nsw> : i16
        memref.store %s, %c[%i, %j] : memref<8x8xi16>
      }
    }
  }
  return
}
)";
  return out;
}

/// G5b-M4b-2b-2: designed memories. They are judged only where the fill sequence found
/// nothing at any point looked at, and only by the judge: signed C arithmetic over two
/// groups, which the fill sequence cannot refute and the first designed memory does; the
/// family as documented, member by member; an operand cell two contributions share; a cell
/// the family cannot aim at and the next that it can; a matrix product in one group, by
/// Theorem 13; binary64 and binary32 with one fill; a family none of whose members is a
/// witness, which leaves Unknown; no fills, which asks for no search; what the fill
/// sequence finds, which stays what it found; and where nothing is designed.
static void testDesignedFills() {
  Module m(std::string(setSource) + siblingSource + reductionSource + reorderSource + floatSource + designedSource());
  Sort w = Sort::bv(16);
  auto level = [&](const SemanticProgram &p, unsigned var, Term lower, Term upper) {
    return LevelSpec{p.terms->state(var, w), lower, upper, false, false};
  };
  auto segments = [&](Fixture &f, std::vector<std::pair<unsigned, unsigned>> siteVars) {
    TermContext &terms = *f.program.terms;
    Term zero = terms.bv(APInt(16, 0)), n = terms.param(0, w);
    DomainRequest r;
    for (auto [site, var] : siteVars)
      r.segments.push_back(segmentOf(f.program, {site}, {level(f.program, var, zero, n)}));
    return r;
  };
  auto verdict = [&](Fixture &f, std::vector<unsigned> groups, const DomainRequest &r, const DomainOptions &o) {
    return checkSetReduction(f.program, f.proof(), f.enumeration, groups, r, o, z3Backend);
  };
  auto say = [](const char *what, const SetReductionVerdict &v) {
    printf("%s: %s %s (%s); integer %s \"%s\"; float %s \"%s\"\n", what, stringifyComputationStatus(v.status()),
           stringifyComputationTier(v.tier()), v.reason().c_str(), stringifyFactStatus(v.integerReorder().status),
           v.integerReorder().detail.c_str(), stringifyFactStatus(v.floatReorder().status),
           v.floatReorder().detail.c_str());
  };
  // The union of the listed groups and its replay at n = `point`, as the search takes them.
  struct Union {
    std::optional<SiteGroup> united;
    ReplayResult replay;
    std::vector<const SiteGroup *> listed;
  };
  auto unionAt = [&](Fixture &f, std::vector<unsigned> groups, unsigned point) {
    Union out;
    for (unsigned g : groups)
      out.listed.push_back(&f.enumeration.groups[g]);
    std::vector<int> groupOf;
    std::string why;
    out.united = unitedSiteGroup(f.program, out.listed, groupOf, why);
    if (out.united)
      out.replay = replayGroupSet(f.program, *out.united, groupOf, out.listed.size(), {APInt(16, point)},
                                  HerbrandLimits());
    return out;
  };
  const uint64_t cellLimit = DomainLimits().witnessCells;
  // The designed memories over the union at n = `point`.
  auto family = [&](Fixture &f, std::vector<unsigned> groups, unsigned point) {
    Union u = unionAt(f, groups, point);
    CHECK(u.united.has_value());
    if (!u.united)
      return std::vector<EntryState>();
    return designedReversalFills(f.program, *u.united, u.replay.prefix, {APInt(16, point)}, cellLimit);
  };
  // The judge over the union at n = `point`.
  auto judge = [&](Fixture &f, std::vector<unsigned> groups, unsigned point, const EntryState &e, std::string &detail) {
    Union u = unionAt(f, groups, point);
    if (!u.united || u.replay.outcome != ReplayOutcome::Equal)
      return false;
    return reversalDiffers(f.program, *u.united, e, u.replay.prefix, detail);
  };
  // What the fill sequence alone finds over the union at n = `point`, with `fills` fills.
  auto drawn = [&](Fixture &f, std::vector<unsigned> groups, unsigned point, unsigned fills) {
    std::vector<const SiteGroup *> listed;
    for (unsigned g : groups)
      listed.push_back(&f.enumeration.groups[g]);
    return reorderWitnessOver(f.program, listed, {APInt(16, point)}, fills, cellLimit);
  };
  // The cells of a memory that are set and not zero: root, row-major offset, bits.
  using Cells = std::vector<std::tuple<unsigned, uint64_t, uint64_t>>;
  auto cellsOf = [](const EntryState &e) {
    Cells out;
    for (unsigned r = 0; r < e.memory.size(); ++r)
      for (uint64_t i = 0; i < e.memory[r].cells.size(); ++i)
        if (e.memory[r].cells[i] && !e.memory[r].cells[i]->isZero())
          out.emplace_back(r, i, e.memory[r].cells[i]->getZExtValue());
    return out;
  };
  auto sameMemory = [&](const EntryState &a, const EntryState &b) {
    if (a.parameters != b.parameters || a.memory.size() != b.memory.size())
      return false;
    for (unsigned r = 0; r < a.memory.size(); ++r)
      if (a.memory[r].shape != b.memory[r].shape || a.memory[r].cells != b.memory[r].cells)
        return false;
    return true;
  };
  auto show = [&](const char *what, const std::vector<EntryState> &entries) {
    for (size_t k = 0; k < entries.size(); ++k) {
      printf("%s: designed memory %zu:", what, k);
      for (auto [root, at, bits] : cellsOf(entries[k]))
        printf(" %u[%llu]=%llx", root, (unsigned long long)at, (unsigned long long)bits);
      printf("\n");
    }
  };
  auto bits64 = [](double value) {
    uint64_t raw;
    std::memcpy(&raw, &value, sizeof raw);
    return raw;
  };
  auto bits32 = [](float value) {
    uint32_t raw;
    std::memcpy(&raw, &value, sizeof raw);
    return (uint64_t)raw;
  };
  const uint64_t intMax = 0x7FFF, intMin = 0x8000, minusOne = 0xFFFF;
  const std::string flagged = "the reduction carries nsw or nuw, so a partial sum can stop the run";
  const std::string overflows = " overflows a flagged addition, at a designed memory";
  const std::string changes = " changes its final bits, at a designed memory";
  const std::string noMemory = "no memory found at which a reversal differs";
  // A fact refuted by the designed memory `expected` at n = `point`: its words, its point,
  // its memory cell for cell, the program completing on it, and the judge again.
  auto designed = [&](const char *what, Fixture &f, std::vector<unsigned> groups, const Fact &fact, unsigned point,
                      const std::string &words, const Cells &expected) {
    bool ok = fact.status == FactStatus::Refuted && fact.detail == words && fact.point && fact.memory &&
              *fact.point == std::vector<APInt>{APInt(16, point)} && cellsOf(*fact.memory) == expected;
    std::string detail;
    bool judged = ok && runConcrete(f.program, *fact.memory).status == RunStatus::Returned &&
                  judge(f, groups, point, *fact.memory, detail) && detail + ", at a designed memory" == fact.detail;
    if (!ok || !judged) {
      printf("%s: %s \"%s\"%s%s\n", what, stringifyFactStatus(fact.status), fact.detail.c_str(),
             fact.point ? "" : ", no point", judged ? "" : ", not judged a witness");
      if (fact.memory)
        show(what, {*fact.memory});
    }
    CHECK(ok && judged);
  };

  // Signed arithmetic: nsw on the multiplication and on the addition, two groups writing
  // C[i]. A product of two drawn values overflows unless one of them is small, so the fill
  // sequence finds no memory at any point of the scope: before designed memories this fact
  // was Unknown. The first designed memory is C[0] = INT_MAX, A[0] = 1, B[0] = -1, D[0] = 1:
  // the program adds -1 then +1 and completes at INT_MAX; reversed, INT_MAX + 1 overflows.
  {
    Fixture f = fixture(m, "d_set", 1, 3);
    std::vector<unsigned> l = loops(f.program);
    CHECK(l.size() == 2 && f.enumeration.groups.size() == 2);
    DomainRequest r = segments(f, {{0, l[0]}, {1, l[1]}});
    for (unsigned n : {1u, 2u, 3u}) {
      Fact alone = drawn(f, {0, 1}, n, DomainLimits().witnessFills);
      CHECK(alone.status == FactStatus::Unknown && alone.detail == noMemory && !alone.memory);
    }
    SetReductionVerdict v = verdict(f, {0, 1}, r, f.options);
    if (v.status() != S::Verified || v.integerReorder().status != FactStatus::Refuted)
      say("signed", v);
    CHECK(v.status() == S::Verified && v.tier() == T::Symbolic && v.fold() == FoldForm::ModularInteger &&
          v.trivialReorder().status == FactStatus::Refuted);
    designed("signed", f, {0, 1}, v.integerReorder(), 1, "reversing cell 0 of root 2" + overflows,
             {{0, 0, 1}, {1, 0, minusOne}, {2, 0, intMax}, {3, 0, 1}});
    CHECK(v.floatReorder().status == FactStatus::Unknown && v.floatReorder().detail == "not a float fold, or no claim");
    // The verdict is what it was: a refuted fact refutes nothing, and no claim moved.
    CHECK(v.computationSymbolic().status() == S::Verified && v.domainSymbolic().status() == S::Verified &&
          v.refutedBy() == RefutedBy::Nothing && !v.witness() && !v.failingPoint());
    // The family at n = 1, member by member: INT_MAX with -1 then +1, INT_MIN with +1 then
    // -1. The other operand choices give a cell two values (A[0] is read by both
    // contributions) or a memory already listed. Each member is a witness here.
    std::vector<EntryState> members = family(f, {0, 1}, 1);
    CHECK(members.size() == 2 && members.size() <= designedFillsAtMost);
    if (members.size() == 2) {
      CHECK(cellsOf(members[0]) == Cells({{0, 0, 1}, {1, 0, minusOne}, {2, 0, intMax}, {3, 0, 1}}));
      CHECK(cellsOf(members[1]) == Cells({{0, 0, 1}, {1, 0, 1}, {2, 0, intMin}, {3, 0, minusOne}}));
      CHECK(v.integerReorder().memory && sameMemory(*v.integerReorder().memory, members[0]));
      for (const EntryState &member : members) {
        std::string detail;
        RunResult run = runConcrete(f.program, member);
        CHECK(run.status == RunStatus::Returned && judge(f, {0, 1}, 1, member, detail) &&
              detail == "reversing cell 0 of root 2 overflows a flagged addition");
        // The program ends where it started: -1 then +1, or +1 then -1.
        CHECK(run.status == RunStatus::Returned && run.memory[2].cells[0] == member.memory[2].cells[0]);
      }
    } else {
      show("signed", members);
    }
    // At n = 2 there are two targets, each with its two members.
    CHECK(family(f, {0, 1}, 2).size() == 4);
    // No fills asks for no search, designed or drawn: Unknown, with the rule's reason.
    DomainOptions none = f.options;
    none.limits.witnessFills = 0;
    SetReductionVerdict open = verdict(f, {0, 1}, r, none);
    CHECK(open.status() == S::Verified && open.integerReorder().status == FactStatus::Unknown &&
          open.integerReorder().detail == flagged && !open.integerReorder().point && !open.integerReorder().memory);
    Union u = unionAt(f, {0, 1}, 1);
    Fact unasked = designedWitnessOver(f.program, u.listed, {APInt(16, 1)}, 0, cellLimit);
    CHECK(unasked.status == FactStatus::Unknown && !unasked.point && !unasked.memory);
    // One fill is enough to ask, and the listing changes nothing: the same memory either way.
    Fact asked = designedWitnessOver(f.program, u.listed, {APInt(16, 1)}, 1, cellLimit);
    Union reversed = unionAt(f, {1, 0}, 1);
    Fact back = designedWitnessOver(f.program, reversed.listed, {APInt(16, 1)}, 1, cellLimit);
    for (const Fact *fact : {&asked, &back})
      CHECK(fact->status == FactStatus::Refuted && fact->detail == v.integerReorder().detail && fact->memory &&
            v.integerReorder().memory && sameMemory(*fact->memory, *v.integerReorder().memory));
  }

  // An operand cell two contributions share: A*B then B*D. With B[0] holding the first
  // contribution it cannot be 1 for the last, so the first goes into A[0]: C[0] = INT_MAX,
  // A[0] = -1, B[0] = 1, D[0] = 1.
  {
    Fixture f = fixture(m, "d_shared", 1, 2);
    std::vector<unsigned> l = loops(f.program);
    CHECK(l.size() == 2 && f.enumeration.groups.size() == 2);
    SetReductionVerdict v = verdict(f, {0, 1}, segments(f, {{0, l[0]}, {1, l[1]}}), f.options);
    if (v.status() != S::Verified || v.integerReorder().status != FactStatus::Refuted)
      say("shared operand", v);
    CHECK(v.status() == S::Verified);
    designed("shared operand", f, {0, 1}, v.integerReorder(), 1, "reversing cell 0 of root 2" + overflows,
             {{0, 0, minusOne}, {1, 0, 1}, {2, 0, intMax}, {3, 0, 1}});
    std::vector<EntryState> members = family(f, {0, 1}, 1);
    CHECK(members.size() == 2);
    if (members.size() == 2)
      CHECK(cellsOf(members[1]) == Cells({{0, 0, 1}, {1, 0, 1}, {2, 0, intMin}, {3, 0, minusOne}}));
    else
      show("shared operand", members);
  }

  // A family none of whose members is a witness. The second group runs twice, so the last
  // two contributions are one product: -1, +1, +1 from INT_MAX overflows in the program's
  // own order, and +1, -1, -1 from INT_MIN does too, so both runs stop and show nothing. A
  // witness exists (C[0] = INT_MAX with -2, +1, +1) and is not in the family: the fact
  // stays Unknown, with the rule's reason, and never becomes Proven.
  {
    Fixture f = fixture(m, "d_repeat", 1, 2);
    std::vector<unsigned> l = loops(f.program);
    CHECK(l.size() == 3 && f.enumeration.groups.size() == 2);
    TermContext &terms = *f.program.terms;
    Term zero = terms.bv(APInt(16, 0)), n = terms.param(0, w), two = terms.bv(APInt(16, 2));
    DomainRequest r;
    r.segments = {segmentOf(f.program, {0}, {level(f.program, l[0], zero, n)}),
                  segmentOf(f.program, {1}, {level(f.program, l[1], zero, n), level(f.program, l[2], zero, two)})};
    SetReductionVerdict v = verdict(f, {0, 1}, r, f.options);
    if (v.status() != S::Verified || v.integerReorder().status != FactStatus::Unknown)
      say("repeat", v);
    CHECK(v.status() == S::Verified && v.trivialReorder().status == FactStatus::Refuted);
    CHECK(v.integerReorder().status == FactStatus::Unknown && v.integerReorder().detail == flagged &&
          !v.integerReorder().point && !v.integerReorder().memory);
    std::vector<EntryState> members = family(f, {0, 1}, 1);
    CHECK(members.size() == 2);
    for (const EntryState &member : members) {
      std::string detail;
      CHECK(runConcrete(f.program, member).status != RunStatus::Returned && !judge(f, {0, 1}, 1, member, detail));
    }
    // The witness the family does not hold, judged: the fact could be refuted, and is not claimed.
    EntryState e = members.empty() ? EntryState() : members[0];
    if (!members.empty()) {
      e.memory[0].cells[0] = APInt(16, 1);
      e.memory[1].cells[0] = APInt(16, 0xFFFE);
      e.memory[2].cells[0] = APInt(16, intMax);
      e.memory[3].cells[0] = APInt(16, 1);
      std::string detail;
      CHECK(runConcrete(f.program, e).status == RunStatus::Returned && judge(f, {0, 1}, 1, e, detail));
    }
  }

  // One group, two sites, by Theorem 13 and as a set of one group. Cell 0 gets A[0]*B[0]
  // twice, one product, so no memory is designed for it; cell 1 gets A[1]*B[0] then
  // A[1]*B[1]: C[1] = INT_MAX, A[1] = 1, B[0] = -1, B[1] = 1, at n = 2, the first point of
  // the scope. A[0] stays zero, so cell 0 adds nothing.
  {
    Fixture f = fixture(m, "d_second", 2, 3);
    std::vector<unsigned> l = loops(f.program);
    CHECK(l.size() == 2 && f.enumeration.groups.size() == 1);
    DomainRequest r = segments(f, {{0, l[0]}, {1, l[1]}});
    CoordinateFamily none;
    ReductionVerdict one = checkReduction(f.program, f.proof(), f.enumeration, 0, none, nullptr, r, f.options, z3Backend);
    SetReductionVerdict set = verdict(f, {0}, r, f.options);
    if (one.status() != S::Verified || set.status() != S::Verified)
      say("second target", set);
    CHECK(one.status() == S::Verified && set.status() == S::Verified);
    const Cells expected{{0, 1, 1}, {1, 0, minusOne}, {1, 1, 1}, {2, 1, intMax}};
    designed("second target, Theorem 13", f, {0}, one.integerReorder(), 2, "reversing cell 1 of root 2" + overflows,
             expected);
    designed("second target, a set of one", f, {0}, set.integerReorder(), 2, "reversing cell 1 of root 2" + overflows,
             expected);
    // What Theorem 13's own search judges is the group itself, with the group's replay.
    if (one.integerReorder().memory) {
      ReplayResult replay = replayGroup(f.program, f.enumeration.groups[0], {APInt(16, 2)}, HerbrandLimits());
      std::string detail;
      CHECK(reversalDiffers(f.program, f.enumeration.groups[0], *one.integerReorder().memory, replay.prefix, detail));
      Fact direct = designedWitness(f.program, f.enumeration.groups[0], {APInt(16, 2)}, 1, cellLimit);
      CHECK(direct.status == FactStatus::Refuted && direct.detail == one.integerReorder().detail && direct.memory &&
            sameMemory(*direct.memory, *one.integerReorder().memory));
      CHECK(reorderWitness(f.program, f.enumeration.groups[0], {APInt(16, 2)}, DomainLimits().witnessFills, cellLimit)
                .status == FactStatus::Unknown);
    }
    // Two members at n = 2, both for cell 1: cell 0 has none.
    std::vector<EntryState> members = family(f, {0}, 2);
    CHECK(members.size() == 2);
    for (const EntryState &member : members)
      CHECK(member.memory[2].cells[0] && member.memory[2].cells[0]->isZero() && member.memory[2].cells[1] &&
            !member.memory[2].cells[1]->isZero());
  }

  // Cells of one contribution are no targets and do not count as one. At n = 4 the first
  // group writes cells 0 to 3 once each and the second adds four products to C[3]: the
  // fourth cell written is the first target. C[3] = INT_MAX, A[3] = 1, B[3] = -1 (the first
  // contribution) and D[3] = 1 (the last, j = 3; the three before it add A[j] * D[j] = 0).
  {
    Fixture f = fixture(m, "d_late", 4, 4);
    CHECK(f.enumeration.groups.size() == 2);
    std::vector<EntryState> members = family(f, {0, 1}, 4);
    CHECK(members.size() == 2);
    if (!members.empty()) {
      CHECK(cellsOf(members[0]) == Cells({{0, 3, 1}, {1, 3, minusOne}, {2, 3, intMax}, {3, 3, 1}}));
      std::string detail;
      CHECK(runConcrete(f.program, members[0]).status == RunStatus::Returned &&
            judge(f, {0, 1}, 4, members[0], detail) && detail == "reversing cell 3 of root 2 overflows a flagged addition");
    } else {
      show("late target", members);
    }
  }

  // A cell that holds a constant before its first event starts its fold there, whatever
  // the entry holds: it is no target. C[0] is 5, so at n = 1 nothing is designed; at n = 2
  // cell 1 is aimed at, and the witness is at the second point looked at.
  {
    Fixture f = fixture(m, "d_constant", 1, 2);
    std::vector<unsigned> l = loops(f.program);
    CHECK(l.size() == 2 && f.enumeration.groups.size() == 2);
    CHECK(family(f, {0, 1}, 1).empty());
    Union u = unionAt(f, {0, 1}, 1);
    CHECK(u.replay.outcome == ReplayOutcome::Equal && u.replay.prefix.count({2, 0}) == 1 &&
          u.replay.prefix.at({2, 0}) == APInt(16, 5));
    SetReductionVerdict v = verdict(f, {0, 1}, segments(f, {{0, l[0]}, {1, l[1]}}), f.options);
    if (v.status() != S::Verified || v.integerReorder().status != FactStatus::Refuted)
      say("constant start", v);
    CHECK(v.status() == S::Verified);
    designed("constant start", f, {0, 1}, v.integerReorder(), 2, "reversing cell 1 of root 2" + overflows,
             {{0, 1, 1}, {1, 1, minusOne}, {2, 1, intMax}, {3, 1, 1}});
  }

  // A matrix product in one group, by Theorem 13 with the proposer's request: three
  // contributions to every cell. The first target is C[0, 0]: it at INT_MAX, A[0, 0] = 1
  // and B[0, 0] = -1 for k = 0, A[0, 2] = 1 and B[2, 0] = 1 for k = 2; k = 1 adds 0.
  {
    Fixture f = fixture(m, "d_gemm", 1, 2);
    CoordinateFamily ijkFamily = ijk(f.program);
    std::string why;
    auto request = propose(f, ijkFamily, why);
    CHECK(request.has_value());
    if (request) {
      ReductionVerdict v = checkReduction(f.program, f.proof(), f.enumeration, 0, ijkFamily, nullptr, *request,
                                          f.options, z3Backend);
      CHECK(v.status() == S::Verified && v.tier() == T::Symbolic);
      designed("matrix product", f, {0}, v.integerReorder(), 1, "reversing cell 0 of root 2" + overflows,
               {{0, 0, 1}, {0, 2, 1}, {1, 0, minusOne}, {1, 16, 1}, {2, 0, intMax}});
      CHECK(reorderWitness(f.program, f.enumeration.groups[0], {APInt(16, 1)}, DomainLimits().witnessFills, cellLimit)
                .status == FactStatus::Unknown);
    }
    // Three targets, C[0, 0], C[0, 1] and C[0, 2], four members each: the bound is never passed.
    std::vector<EntryState> members = family(f, {0}, 1);
    CHECK(members.size() == 12 && members.size() <= designedFillsAtMost);
    CHECK(family(f, {0}, 2).size() == 12);
  }

  // Floats. With one fill the drawn memory shows nothing, and the first designed memory
  // does: C[0] = 2^53, A[0] = B[0] = 1, D[0] = -2^53, where (2^53 + 1) - 2^53 = 0 and
  // (2^53 - 2^53) + 1 = 1. The second member is the largest finite value: (max + max) - max
  // is +infinity, (max - max) + max is max. With the default fills the fill sequence refutes
  // first, and its witness is the one it found before designed memories existed.
  {
    Fixture f = fixture(m, "set_float", 1, 2);
    std::vector<unsigned> l = loops(f.program);
    DomainRequest r = segments(f, {{0, l[0]}, {1, l[1]}});
    const double big = 9007199254740992.0, largest = std::numeric_limits<double>::max();
    for (unsigned n : {1u, 2u})
      CHECK(drawn(f, {0, 1}, n, 1).status == FactStatus::Unknown);
    DomainOptions one = f.options;
    one.limits.witnessFills = 1;
    SetReductionVerdict v = verdict(f, {0, 1}, r, one);
    CHECK(v.status() == S::Verified && v.fold() == FoldForm::OrderedFloat);
    designed("binary64", f, {0, 1}, v.floatReorder(), 1, "reversing cell 0 of root 2" + changes,
             {{0, 0, bits64(1.0)}, {1, 0, bits64(1.0)}, {2, 0, bits64(big)}, {3, 0, bits64(-big)}});
    CHECK(v.integerReorder().status == FactStatus::Unknown && v.integerReorder().detail == "not an integer fold, or no claim");
    std::vector<EntryState> members = family(f, {0, 1}, 1);
    CHECK(members.size() == 2);
    if (members.size() == 2) {
      CHECK(cellsOf(members[1]) == Cells({{0, 0, bits64(1.0)}, {1, 0, bits64(largest)}, {2, 0, bits64(largest)},
                                           {3, 0, bits64(-largest)}}));
      std::string detail;
      RunResult run = runConcrete(f.program, members[1]);
      CHECK(run.status == RunStatus::Returned &&
            run.memory[2].cells[0] == APInt(64, bits64(std::numeric_limits<double>::infinity())) &&
            judge(f, {0, 1}, 1, members[1], detail) && detail == "reversing cell 0 of root 2 changes its final bits");
    } else {
      show("binary64", members);
    }
    // No fills: no search of either kind (the verdict of it is in the setfloat section).
    Fact unasked = designedWitnessOver(f.program, unionAt(f, {0, 1}, 1).listed, {APInt(16, 1)}, 0, cellLimit);
    CHECK(unasked.status == FactStatus::Unknown && !unasked.memory);
    // The default: what the fill sequence finds is the fact, memory for memory, and it is
    // not a designed one.
    SetReductionVerdict full = verdict(f, {0, 1}, r, f.options);
    const Fact &fact = full.floatReorder();
    CHECK(fact.status == FactStatus::Refuted && fact.point && fact.memory &&
          fact.detail.find("designed") == std::string::npos);
    if (fact.point && fact.memory) {
      Fact alone = drawn(f, {0, 1}, (*fact.point)[0].getZExtValue(), DomainLimits().witnessFills);
      CHECK(alone.status == FactStatus::Refuted && alone.detail == fact.detail && alone.memory &&
            sameMemory(*alone.memory, *fact.memory));
      for (const EntryState &member : family(f, {0, 1}, (*fact.point)[0].getZExtValue()))
        CHECK(!sameMemory(member, *fact.memory));
    }
  }
  // binary32: 2^24 in place of 2^53, and the largest finite binary32.
  {
    Fixture f = fixture(m, "set_float32", 1, 2);
    std::vector<unsigned> l = loops(f.program);
    DomainRequest r = segments(f, {{0, l[0]}, {1, l[1]}});
    const float big = 16777216.0f, largest = std::numeric_limits<float>::max();
    for (unsigned n : {1u, 2u})
      CHECK(drawn(f, {0, 1}, n, 1).status == FactStatus::Unknown);
    DomainOptions one = f.options;
    one.limits.witnessFills = 1;
    SetReductionVerdict v = verdict(f, {0, 1}, r, one);
    CHECK(v.status() == S::Verified && v.fold() == FoldForm::OrderedFloat);
    designed("binary32", f, {0, 1}, v.floatReorder(), 1, "reversing cell 0 of root 2" + changes,
             {{0, 0, bits32(1.0f)}, {1, 0, bits32(1.0f)}, {2, 0, bits32(big)}, {3, 0, bits32(-big)}});
    std::vector<EntryState> members = family(f, {0, 1}, 1);
    CHECK(members.size() == 2);
    if (members.size() == 2) {
      CHECK(cellsOf(members[1]) == Cells({{0, 0, bits32(1.0f)}, {1, 0, bits32(largest)}, {2, 0, bits32(largest)},
                                           {3, 0, bits32(-largest)}}));
      std::string detail;
      CHECK(judge(f, {0, 1}, 1, members[1], detail) && detail == "reversing cell 0 of root 2 changes its final bits");
    } else {
      show("binary32", members);
    }
  }

  // What the fill sequence finds stays what it found: set_nsw, whose multiplication carries
  // no flag, is refuted by a drawn memory, the same one, and no designed memory is judged.
  {
    Fixture f = fixture(m, "set_nsw", 1, 2);
    std::vector<unsigned> l = loops(f.program);
    SetReductionVerdict v = verdict(f, {0, 1}, segments(f, {{0, l[0]}, {1, l[1]}}), f.options);
    const Fact &fact = v.integerReorder();
    CHECK(fact.status == FactStatus::Refuted && fact.point && fact.memory &&
          fact.detail.find("designed") == std::string::npos);
    if (fact.point && fact.memory) {
      Fact alone = drawn(f, {0, 1}, (*fact.point)[0].getZExtValue(), DomainLimits().witnessFills);
      CHECK(alone.status == FactStatus::Refuted && alone.detail == fact.detail && alone.memory &&
            sameMemory(*alone.memory, *fact.memory));
      // Designed memories exist here and are witnesses too; the search never reached them.
      std::vector<EntryState> members = family(f, {0, 1}, (*fact.point)[0].getZExtValue());
      CHECK(!members.empty());
      for (const EntryState &member : members)
        CHECK(!sameMemory(member, *fact.memory));
    }
  }

  // The same for Theorem 13's own search: one float group of two sites, A[i]*B[0] then
  // A[j]*B[j], whose cell 1 the fill sequence refutes at n = 2.
  {
    Fixture f = fixture(m, "d_floats", 2, 2);
    std::vector<unsigned> l = loops(f.program);
    CHECK(l.size() == 2 && f.enumeration.groups.size() == 1);
    CoordinateFamily none;
    ReductionVerdict v = checkReduction(f.program, f.proof(), f.enumeration, 0, none, nullptr,
                                        segments(f, {{0, l[0]}, {1, l[1]}}), f.options, z3Backend);
    const Fact &fact = v.floatReorder();
    CHECK(v.status() == S::Verified && fact.status == FactStatus::Refuted && fact.point && fact.memory &&
          fact.detail.find("designed") == std::string::npos);
    if (fact.point && fact.memory) {
      Fact alone = reorderWitness(f.program, f.enumeration.groups[0], *fact.point, DomainLimits().witnessFills, cellLimit);
      CHECK(alone.status == FactStatus::Refuted && alone.detail == fact.detail && alone.memory &&
            sameMemory(*alone.memory, *fact.memory));
      std::vector<EntryState> members = family(f, {0}, 2);
      CHECK(!members.empty());
      for (const EntryState &member : members)
        CHECK(!sameMemory(member, *fact.memory));
    }
  }

  // Where nothing is designed. An integer fold with no nsw (no reversal overflows), and
  // with nsw and nuw together (both patterns would stop the program: -1 is the largest
  // unsigned value); sites of two operations, and of two sets of flags, which the judge
  // does not reverse; two contributions that are one product; an operand in a root the
  // program allocates, which no entry sets.
  {
    for (const char *name : {"sib_roots", "set_mixed_flags"}) {
      Fixture f = fixture(m, name, 1, 2);
      CHECK(f.enumeration.groups.size() == 2);
      CHECK(family(f, {0, 1}, 1).empty() && family(f, {0, 1}, 2).empty());
    }
    Fixture mixed = fixture(m, "set_mixed", 1, 2);
    CHECK(mixed.enumeration.groups.size() == 3 && family(mixed, {0, 1, 2}, 1).empty());
    Fixture same = fixture(m, "set_float_same", 1, 2);
    CHECK(same.enumeration.groups.size() == 1 && family(same, {0}, 1).empty() && family(same, {0}, 2).empty());
    Fixture fresh = fixture(m, "d_fresh_operand", 1, 2);
    CHECK(fresh.enumeration.groups.size() == 2 && family(fresh, {0, 1}, 1).empty());
    // nsw and nuw together: nothing designed, and the fact Unknown, as with nuw alone.
    Fixture both = fixture(m, "d_both", 1, 2);
    CHECK(both.enumeration.groups.size() == 2 && family(both, {0, 1}, 1).empty() && family(both, {0, 1}, 2).empty());
    std::vector<unsigned> l = loops(both.program);
    SetReductionVerdict v = verdict(both, {0, 1}, segments(both, {{0, l[0]}, {1, l[1]}}), both.options);
    CHECK(v.status() == S::Verified && v.integerReorder().status == FactStatus::Unknown &&
          v.integerReorder().detail == flagged);
  }
}

int main(int argc, char **argv) {
  setvbuf(stdout, nullptr, _IOLBF, 0);
  mlir::MLIRContext local;
  configure(local);
  context = &local;
  std::string only = argc > 1 ? argv[1] : "";
  // "core" is every section but the 512-program sweep, which CTest runs on its own.
  auto run = [&](const char *name, void (*test)()) {
    bool core = only == "core" && std::string(name) != "holes";
    if (only.empty() || only == name || core) {
      auto begin = std::chrono::steady_clock::now();
      test();
      printf("%s: %.1f s, %d checks, %d failures\n", name,
             std::chrono::duration<double>(std::chrono::steady_clock::now() - begin).count(), checks,
             failures);
    }
  };
  run("phase4", testPhase4Views);
  run("identity", testIdentity);
  run("setdomain", testSetDomain);
  run("setreduction", testSetReduction);
  run("setreorder", testSetReorder);
  run("setfloat", testSetFloat);
  run("designed", testDesignedFills);
  run("proposer", testProposer);
  run("catalogue", testCatalogue);
  run("adversarial", testAdversarial);
  run("wrapping", testWrapping);
  run("limits", testSolverAndLimits);
  run("binding", testBinding);
  run("reviewed", testReviewed);
  run("gaps", testGapsG5);
  run("multiplicity", testMultiplicity);
  run("siblings", testSiblings);
  run("segments", testSegments);
  run("orders", testExhaustiveOrders);
  run("holes", testExhaustiveHoles);
  printf("%d checks, %d failures\n", checks, failures);
  return failures ? 1 : 0;
}
