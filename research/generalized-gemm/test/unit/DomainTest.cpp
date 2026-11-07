// G5a: Theorem 12 and 13, the facts beside them, and what must never pass.
// test/fuzz/domain_oracle.py recomputes the results with no library code.
#include "GeneralizedGemm/DomainAnalysis.h"
#include "GeneralizedGemm/ReductionSemantics.h"
#include "GeneralizedGemm/TerminationAnalysis.h"
#include "GeneralizedGemm/TerminationObligations.h"
#include "GeneralizedGemm/Z3Backend.h"

#include "TestSupport.h"

#include <chrono>
#include <functional>
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
      if (name == "; C1 ")
        CHECK(out.segments()[0].outputInjective.status == FactStatus::Unknown &&
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
    CHECK(fin.segments()[0].multiplicity == DomainMultiplicity::Unit);
    CHECK(fin.segments()[1].multiplicity == DomainMultiplicity::LogicalBox);
    CHECK(sym.segments()[1].multiplicity == DomainMultiplicity::LogicalBox);
    CHECK(fin.multiplicity() == DomainMultiplicity::Unknown && fin.multiplicityTerm() == nullptr);
    auto mu = evaluateLevelTerm(sym.segments()[1].multiplicityTerm, {APInt(16, 3)}, {APInt(16, 2), APInt(16, 0)});
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
