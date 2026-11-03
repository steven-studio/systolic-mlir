// G4a: the replay relation, its obligations, and what must never pass.
// test/fuzz/computation_oracle.py recomputes the results with no library code.
#include "GeneralizedGemm/ComputationAnalysis.h"
#include "GeneralizedGemm/SymbolicDomain.h"
#include "GeneralizedGemm/TerminationAnalysis.h"
#include "GeneralizedGemm/TerminationObligations.h"
#include "GeneralizedGemm/Z3Backend.h"

#include "TestSupport.h"

#include "llvm/ADT/bit.h"

#include <chrono>
#include <functional>
#include <map>
#include <random>
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

static ParameterScope range(const SemanticProgram &p, int64_t low, int64_t high) {
  ParameterScope scope;
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

// Every query here has both limits. The resource limit is deterministic and small:
// float stop obligations use it up, which is an unknown, and not a slow pass.
static SmtLimits testSolver() {
  SmtLimits limits;
  limits.timeoutMilliseconds = 2000;
  limits.resourceLimit = 400000;
  return limits;
}

static ComputationOptions computationOptions(const ParameterScope &scope,
                                             const Authorizations &authorized) {
  ComputationOptions options;
  options.scope = scope;
  options.authorized = authorized;
  options.termination.solver = testSolver();
  return options;
}

struct Prepared {
  std::optional<TerminationCertificate> certificate;
  DomainEnumeration enumeration;
  const TerminationCertificate *proof() const { return certificate ? &*certificate : nullptr; }
};

static Prepared prepare(const SemanticProgram &p, const ParameterScope &scope,
                        const Authorizations &authorized,
                        std::vector<std::vector<APInt>> only = {}) {
  Prepared out;
  TerminationOptions termination;
  termination.scope = scope;
  termination.authorized = authorized;
  termination.limits.replayFuel = 1u << 12;
  termination.limits.solver = testSolver();
  out.certificate = analyzeTermination(p, z3Backend, termination).certificate;
  EnumerationOptions enumeration;
  enumeration.scope = scope;
  enumeration.authorized = authorized;
  enumeration.termination.solver = testSolver();
  enumeration.onlyPoints = std::move(only);
  out.enumeration = enumerateDomain(p, out.proof(), enumeration, z3Backend);
  return out;
}

static std::string nest(const std::string &body) {
  return "  scf.for %i = %c0 to %n step %c1 {\n  scf.for %j = %c0 to %c2 step %c1 {\n"
         "  scf.for %k = %c0 to %c2 step %c1 {\n" +
         body + "  }\n  }\n  }\n";
}

static std::string mac(const std::string &a = "%a[%i, %k]", const std::string &b = "%b[%k, %j]",
                       const std::string &c = "%c[%i, %j]", const std::string &type = "4x4xf64",
                       const std::string &add = "arith.addf", const std::string &mul = "arith.mulf",
                       const std::string &suffix = "", const std::string &element = "f64",
                       const std::string &addFlags = "", const std::string &mulFlags = "") {
  std::string t = "memref<" + type + ">";
  return "  %x" + suffix + " = memref.load " + a + " : " + t + "\n  %y" + suffix + " = memref.load " + b +
         " : " + t + "\n  %z" + suffix + " = memref.load " + c + " : " + t + "\n  %m" + suffix + " = " + mul +
         " %x" + suffix + ", %y" + suffix + mulFlags + " : " + element + "\n  %s" + suffix + " = " + add + " %z" +
         suffix + ", %m" + suffix + addFlags + " : " + element + "\n  memref.store %s" + suffix + ", " + c + " : " + t + "\n";
}

static std::string imac(const std::string &a = "%a[%i, %k]", const std::string &b = "%b[%k, %j]",
                        const std::string &c = "%c[%i, %j]", const std::string &addFlags = "") {
  return mac(a, b, c, "4x4xi32", "arith.addi", "arith.muli", "", "i32", addFlags);
}

static const char *header =
    "  %c0 = arith.constant 0 : index\n  %c1 = arith.constant 1 : index\n"
    "  %c2 = arith.constant 2 : index\n  %c3 = arith.constant 3 : index\n"
    "  %fz = arith.constant 0.0 : f64\n  %iz = arith.constant 0 : i32\n"
    "  %ione = arith.constant 1 : i32\n";

static std::string function(const std::string &name, const std::string &body,
                            const std::string &element = "f64", const std::string &result = "") {
  std::string t = "memref<4x4x" + element + ">";
  // The element type is the last dimension of the printed type.
  t = "memref<4x4x" + element + ">";
  return "func.func @" + name + "(%a: " + t + ", %b: " + t + ", %c: " + t + ", %d: " + t +
         ", %n: index)" + (result.empty() ? "" : " -> " + result) + " {\n" + header + body +
         (result.empty() ? "  return\n" : "") + "}\n";
}

static std::string catalogue() {
  std::string F64 = "memref<4x4xf64>", I32 = "memref<4x4xi32>";
  std::string out;
  out += function("clean", nest(mac()));
  out += function("lost_update", nest(R"(
    %x = memref.load %a[%i, %k] : memref<4x4xf64>
    %y = memref.load %b[%k, %j] : memref<4x4xf64>
    %z = memref.load %c[%i, %j] : memref<4x4xf64>
    %m = arith.mulf %x, %y : f64
    %s = arith.addf %z, %m : f64
    memref.store %s, %c[%i, %j] : memref<4x4xf64>
    %t = arith.addf %z, %m : f64
    memref.store %t, %c[%i, %j] : memref<4x4xf64>
)"));
  out += function("overwritten", nest(mac()) + "  memref.store %fz, %c[%c0, %c0] : " + F64 + "\n");
  out += function("reset_inside", "  scf.for %i = %c0 to %n step %c1 {\n  scf.for %j = %c0 to %c2 step %c1 {\n"
                                  "  scf.for %k = %c0 to %c2 step %c1 {\n" + mac() +
                                  "  }\n  memref.store %fz, %c[%i, %j] : " + F64 + "\n  }\n  }\n");
  out += function("overlap", nest(mac("%c[%k, %i]")));
  out += function("chain", nest(mac()) + nest(mac("%c[%i, %k]", "%b[%k, %j]", "%d[%i, %j]")));
  out += function("repeated", "  scf.for %t = %c0 to %c2 step %c1 {\n" + nest(mac()) + "  }\n");
  out += function("prefix", "  scf.for %i = %c0 to %n step %c1 {\n  scf.for %j = %c0 to %c2 step %c1 {\n"
                            "  memref.store %fz, %c[%i, %j] : " + F64 +
                            "\n  scf.for %k = %c0 to %c2 step %c1 {\n" + mac() + "  }\n  }\n  }\n");
  out += function("contract", nest(mac("%a[%i, %k]", "%b[%k, %j]", "%c[%i, %j]", "4x4xf64", "arith.addf",
                                       "arith.mulf", "", "f64", " fastmath<contract>",
                                       " fastmath<contract>")));
  out += function("integer", nest(imac()), "i32");
  out += function("overflow", nest(imac("%a[%i, %k]", "%b[%k, %j]", "%c[%i, %j]", " overflow<nsw>")), "i32");
  out += function("returned", nest(mac()) + "  %r = memref.load %c[%c0, %c0] : " + F64 +
                                  "\n  return %r : f64\n", "f64", "f64");
  out += function("also_stored", nest(R"(
    %x = memref.load %a[%i, %k] : memref<4x4xf64>
    %y = memref.load %b[%k, %j] : memref<4x4xf64>
    %z = memref.load %c[%i, %j] : memref<4x4xf64>
    %m = arith.mulf %x, %y : f64
    %s = arith.addf %z, %m : f64
    memref.store %s, %d[%i, %j] : memref<4x4xf64>
    memref.store %s, %c[%i, %j] : memref<4x4xf64>
)"));
  out += function("also_in_output", nest(R"(
    %x = memref.load %a[%i, %k] : memref<4x4xf64>
    %y = memref.load %b[%k, %j] : memref<4x4xf64>
    %z = memref.load %c[%i, %j] : memref<4x4xf64>
    %m = arith.mulf %x, %y : f64
    %s = arith.addf %z, %m : f64
    memref.store %s, %c[%j, %i] : memref<4x4xf64>
    memref.store %s, %c[%i, %j] : memref<4x4xf64>
)"));
  // Equal in value and different as nodes: x * 1 is not simplified.
  out += function("identity_write", nest(imac()) + "  %q = memref.load %c[%c0, %c0] : " + I32 +
                                        "\n  %r = arith.muli %q, %ione : i32\n  memref.store %r, %c[%c0, %c0] : " + I32 + "\n", "i32");
  out += function("collapsed", nest(mac("%a[%i, %k]", "%b[%k, %j]", "%c[%c0, %c0]")));
  out += function("observed", nest(mac()) + "  %q = memref.load %c[%c0, %c0] : " + F64 +
                                  "\n  memref.store %q, %d[%c0, %c0] : " + F64 + "\n");
  out += function("foreign_elsewhere", nest(mac()) + "  memref.store %fz, %d[%c0, %c0] : " + F64 + "\n");
  out += function("foreign_same_root", nest(mac()) + "  memref.store %fz, %c[%c3, %c3] : " + F64 + "\n");
  // One root holds the operand and the output, in rows that never meet.
  out += function("same_root", nest(mac("%c[%c3, %k]")) + "  memref.store %fz, %c[%c3, %c3] : " + F64 + "\n");
  out += function("late_overlap", nest(mac()) + "  %big = arith.cmpi uge, %n, %c3 : index\n  scf.if %big {\n"
                                                "    memref.store %fz, %c[%c0, %c0] : " + F64 + "\n  }\n");
  out += "func.func @fresh_prefix(%a: memref<4x4xf64>, %b: memref<4x4xf64>, %n: index) {\n" +
         std::string(header) + "  %c = memref.alloc() : memref<4x4xf64>\n"
         "  scf.for %i = %c0 to %n step %c1 {\n  scf.for %j = %c0 to %c2 step %c1 {\n"
         "  memref.store %fz, %c[%i, %j] : memref<4x4xf64>\n  scf.for %k = %c0 to %c2 step %c1 {\n" +
         mac() + "  }\n  }\n  }\n  return\n}\n";
  out += "func.func @fresh_operand(%b: memref<4x4xf64>, %c: memref<4x4xf64>, %n: index) {\n" +
         std::string(header) + "  %a = memref.alloc() : memref<4x4xf64>\n"
         "  scf.for %i = %c0 to %n step %c1 {\n  scf.for %k = %c0 to %c2 step %c1 {\n"
         "  memref.store %fz, %a[%i, %k] : memref<4x4xf64>\n  }\n  }\n" + nest(mac()) + "  return\n}\n";
  out += "func.func @fresh_unset(%a: memref<4x4xf64>, %b: memref<4x4xf64>, %n: index) {\n" +
         std::string(header) + "  %c = memref.alloc() : memref<4x4xf64>\n" + nest(mac()) + "  return\n}\n";
  // A value goes through a third root and back to C. If r and w overlap, C starts
  // from another value, so that pair is needed although the group never touches them.
  out += R"(func.func @relay(%a: memref<4x4xf64>, %b: memref<4x4xf64>, %c: memref<4x4xf64>,
                             %r: memref<4x4xf64>, %w: memref<4x4xf64>, %n: index) {
)" + std::string(header) + R"(  %five = arith.constant 5.0 : f64
  %six = arith.constant 6.0 : f64
  memref.store %five, %r[%c0, %c0] : memref<4x4xf64>
  memref.store %six, %w[%c0, %c0] : memref<4x4xf64>
  %v = memref.load %r[%c0, %c0] : memref<4x4xf64>
  memref.store %v, %c[%c0, %c0] : memref<4x4xf64>
)" + nest(mac()) + "  return\n}\n";
  // A constant and an event in one block are still one before the other.
  out += function("same_block_prefix", R"(  scf.for %i = %c0 to %n step %c1 {
    memref.store %fz, %c[%i, %c0] : memref<4x4xf64>
)" + mac("%a[%i, %c0]", "%b[%i, %c0]", "%c[%i, %c0]") + "  }\n");
  // The read and the store use different cells that hold equal values: no value
  // differs, so this must not be called refuted.
  out += function("equal_cells", R"(  memref.store %fz, %c[%c0, %c0] : memref<4x4xf64>
  memref.store %fz, %c[%c0, %c1] : memref<4x4xf64>
  cf.br ^next
^next:
  %x = memref.load %a[%c0, %c0] : memref<4x4xf64>
  %y = memref.load %b[%c0, %c0] : memref<4x4xf64>
  %z = memref.load %c[%c0, %c1] : memref<4x4xf64>
  %m = arith.mulf %x, %y : f64
  %s = arith.addf %z, %m : f64
  memref.store %s, %c[%c0, %c0] : memref<4x4xf64>
)");
  // No event happens, or no run completes: nothing to claim.
  out += function("dead_site", R"(  %f = arith.constant false
  cf.cond_br %f, ^site, ^end
^site:
)" + mac("%a[%c0, %c0]", "%b[%c0, %c0]", "%c[%c0, %c0]") + R"(  cf.br ^end
^end:
)");
  out += function("all_stop", R"(  %big = arith.constant 9 : index
  memref.store %fz, %d[%big, %c0] : memref<4x4xf64>
  cf.br ^site
^site:
)" + mac("%a[%c0, %c0]", "%b[%c0, %c0]", "%c[%c0, %c0]"));
  out += function("site_out_of_bounds", R"(  %big = arith.constant 9 : index
  scf.for %i = %c0 to %n step %c1 {
)" + mac("%a[%i, %c0]", "%b[%i, %c0]", "%c[%big, %c0]") + "  }\n");
  // A constant product at 128 bits in its poison term, which is -15 and no overflow.
  out += function("wide_constant", R"(  %p = arith.constant -5 : i64
  %q = arith.constant 3 : i64
  %t = arith.muli %p, %q overflow<nsw> : i64
  cf.br ^next
^next:
  %u = arith.addi %t, %q : i64
)" + nest(imac()), "i32");
  // A stop carried in state, and a store of a value from an earlier block.
  out += function("state_stop", R"(  %x0 = memref.load %a[%c0, %c0] : memref<4x4xi32>
  cf.br ^next
^next:
  %q = arith.divsi %iz, %x0 : i32
)" + nest(imac()), "i32");
  out += function("state_store", R"(  %x0 = memref.load %a[%c0, %c0] : memref<4x4xf64>
  cf.br ^next
^next:
)" + nest(mac()) + "  memref.store %x0, %d[%c0, %c0] : memref<4x4xf64>\n");
  out += function("equal_loads", nest(R"(
    %x = memref.load %a[%i, %k] : memref<4x4xf64>
    %y = memref.load %b[%k, %j] : memref<4x4xf64>
    %z = memref.load %c[%i, %j] : memref<4x4xf64>
    %again = memref.load %a[%i, %k] : memref<4x4xf64>
    %m = arith.mulf %x, %y : f64
    %s = arith.addf %z, %m : f64
    memref.store %s, %c[%i, %j] : memref<4x4xf64>
)"));
  out += "func.func @noalias(%a: memref<4x4xf64> {llvm.noalias}, %b: memref<4x4xf64> {llvm.noalias},"
         " %c: memref<4x4xf64> {llvm.noalias}, %d: memref<4x4xf64> {llvm.noalias}, %n: index) {\n" +
         std::string(header) + nest(mac()) + "  return\n}\n";
  out += "func.func @scalar(%a: memref<4xf64>, %b: memref<4xf64>, %c: memref<4x4xf64>, %d: memref<4x4xf64>, %n: index) {\n" +
         std::string(header) + R"(  scf.for %i = %c0 to %n step %c1 {
    %s = scf.for %k = %c0 to %c2 step %c1 iter_args(%v = %fz) -> (f64) {
      %x = memref.load %a[%k] : memref<4xf64>
      %y = memref.load %b[%k] : memref<4xf64>
      %m = arith.mulf %x, %y : f64
      %t = arith.addf %v, %m : f64
      scf.yield %t : f64
    }
    memref.store %s, %c[%i, %c0] : memref<4x4xf64>
  }
  return
}
)";
  return out;
}

struct Expected {
  const char *name;
  unsigned group;
  S finiteStatus;
  T finiteTier;
  S symbolicStatus;
};

static void testClaims() {
  Module m(catalogue());
  const Expected table[] = {
      {"clean", 0, S::Verified, T::ScopeEnumerated, S::Verified},
      {"lost_update", 0, S::Refuted, T::Proposed, S::Unknown},
      {"overwritten", 0, S::Refuted, T::Proposed, S::Unknown},
      {"reset_inside", 0, S::Refuted, T::Proposed, S::Unknown},
      {"overlap", 0, S::Refuted, T::Proposed, S::Unknown},
      {"chain", 0, S::Verified, T::ScopeEnumerated, S::Verified},
      {"chain", 1, S::Refuted, T::Proposed, S::Unknown},
      {"repeated", 0, S::Verified, T::ScopeEnumerated, S::Verified},
      {"prefix", 0, S::Verified, T::ScopeEnumerated, S::Unknown},
      {"contract", 0, S::Verified, T::ScopeEnumerated, S::Verified},
      {"integer", 0, S::Verified, T::ScopeEnumerated, S::Verified},
      {"overflow", 0, S::Verified, T::ScopeEnumerated, S::Verified},
      {"returned", 0, S::Unknown, T::Proposed, S::Unknown},
      {"also_stored", 0, S::Verified, T::ScopeEnumerated, S::Verified},
      {"also_in_output", 0, S::Refuted, T::Proposed, S::Unknown},
      {"identity_write", 0, S::Unknown, T::Proposed, S::Unknown},
      {"collapsed", 0, S::Verified, T::ScopeEnumerated, S::Verified},
      {"observed", 0, S::Verified, T::ScopeEnumerated, S::Verified},
      {"foreign_elsewhere", 0, S::Verified, T::ScopeEnumerated, S::Verified},
      {"foreign_same_root", 0, S::Verified, T::ScopeEnumerated, S::Verified},
      {"same_root", 0, S::Verified, T::ScopeEnumerated, S::Verified},
      {"late_overlap", 0, S::Verified, T::ScopeEnumerated, S::Verified},
      {"fresh_prefix", 0, S::Verified, T::ScopeEnumerated, S::Unknown},
      {"relay", 0, S::Verified, T::ScopeEnumerated, S::Unknown},
      {"same_block_prefix", 0, S::Verified, T::ScopeEnumerated, S::Unknown},
      {"equal_cells", 0, S::Unknown, T::Proposed, S::Unknown},
      {"wide_constant", 0, S::Verified, T::ScopeEnumerated, S::Verified},
      {"state_stop", 0, S::Unknown, T::Proposed, S::Verified}, // Fill 0 stops every enumerated run.
      {"state_store", 0, S::Verified, T::ScopeEnumerated, S::Verified},
      {"equal_loads", 0, S::Verified, T::ScopeEnumerated, S::Verified},
      {"fresh_operand", 0, S::Unknown, T::Proposed, S::Unknown},
  };
  for (const Expected &e : table) {
    SemanticProgram p = m.import(e.name);
    CHECK(p.isSupported());
    if (!p.isSupported())
      continue;
    ParameterScope scope = range(p, 1, 2);
    Authorizations authorized = everyPair(p);
    Prepared ready = prepare(p, scope, authorized);
    CHECK(ready.enumeration.gate == GateStatus::Open);
    ComputationOptions options = computationOptions(scope, authorized);
    CheckedComputation finite = checkComputationFinite(p, ready.enumeration, e.group, options);
    CheckedComputation symbolic = checkComputationSymbolic(
        p, ready.proof(), ready.enumeration.groups[e.group], options, z3Backend);
    bool ok = finite.status() == e.finiteStatus && finite.tier() == e.finiteTier &&
              symbolic.status() == e.symbolicStatus;
    if (!ok)
      printf("%s group %u: finite %s %s (%s), symbolic %s (%s)\n", e.name, e.group,
             stringifyComputationStatus(finite.status()), stringifyComputationTier(finite.tier()),
             finite.reason().c_str(), stringifyComputationStatus(symbolic.status()),
             symbolic.reason().c_str());
    CHECK(ok);
    // A refutation is a concrete run, judged again here.
    CHECK((finite.status() == S::Refuted) == finite.witness().has_value());
    if (finite.witness()) {
      RunResult run = runConcrete(p, *finite.witness());
      CHECK(run.status == RunStatus::Returned);
      ReplayResult replay = replayGroup(p, ready.enumeration.groups[e.group],
                                        finite.witness()->parameters, HerbrandLimits());
      CHECK(replay.outcome == ReplayOutcome::Mismatch);
      CHECK(replayDifference(p, ready.enumeration.groups[e.group], *finite.witness(), run,
                             replay.prefix).has_value());
    }
    // Later phases own these, whatever the claim.
    for (const CheckedComputation *c : {&finite, &symbolic}) {
      CHECK(c->domainAndOrder().kind() == ProofStatus::Kind::Unknown);
      CHECK(c->transformationEquivalence().kind() == ProofStatus::Kind::Unknown);
    }
    CHECK(symbolic.status() != S::Refuted);
  }

  // Evidence at a sample says nothing of the scope. n = 3 overlaps.
  SemanticProgram late = m.import("late_overlap");
  ParameterScope wide = range(late, 1, 3);
  Authorizations authorized = everyPair(late);
  ComputationOptions options = computationOptions(wide, authorized);
  Prepared sampled = prepare(late, wide, authorized, {{APInt(16, 1)}, {APInt(16, 2)}});
  CHECK(sampled.enumeration.sampled);
  CheckedComputation sample = checkComputationFinite(late, sampled.enumeration, 0, options);
  CHECK(sample.status() == S::Verified && sample.tier() == T::Point);
  Prepared whole = prepare(late, wide, authorized);
  CHECK(checkComputationFinite(late, whole.enumeration, 0, options).status() == S::Refuted);
  CheckedComputation failed =
      checkComputationSymbolic(late, whole.proof(), whole.enumeration.groups[0], options, z3Backend);
  CHECK(failed.status() == S::Unknown && failed.tier() == T::Proposed);
  CHECK(failed.failingPoint().has_value());

  // Facts that must stay visible.
  auto symbolicOf = [&](const char *name) {
    SemanticProgram p = m.import(name);
    ParameterScope scope = range(p, 1, 2);
    Authorizations all = everyPair(p);
    Prepared ready = prepare(p, scope, all);
    return checkComputationSymbolic(p, ready.proof(), ready.enumeration.groups[0],
                                    computationOptions(scope, all), z3Backend);
  };
  CheckedComputation contract = symbolicOf("contract");
  CHECK(!contract.contracts().empty() &&
        (contract.contracts()[0].reductionFlags & FlagContract) &&
        (contract.contracts()[0].multiplyFlags & FlagContract));
  CHECK(contract.fold() == FoldForm::OrderedFloat);
  CHECK(!contract.holdsForEveryRun()); // A NaN stops a run.
  CHECK(symbolicOf("integer").fold() == FoldForm::ModularInteger);
  CHECK(symbolicOf("integer").holdsForEveryRun());
  CheckedComputation overflow = symbolicOf("overflow");
  CHECK(!overflow.holdsForEveryRun() && !overflow.memoryStops().empty());
  CHECK(symbolicOf("clean").frame().empty());
  auto frame = symbolicOf("also_stored").frame();
  CHECK(frame.size() == 1 && frame[0].kind == MemoryEffect::Write && frame[0].root == 3);
  frame = symbolicOf("observed").frame();
  bool sawObserver = false, sawCopy = false;
  for (const ResidualEffect &effect : frame) {
    sawObserver = sawObserver || effect.observesOutput;
    sawCopy = sawCopy || effect.valueReadsMemory;
  }
  CHECK(sawObserver && sawCopy);

  // No initial cell, no claim: every run stops, and that must not read as a proof.
  SemanticProgram unset = m.import("fresh_unset");
  Prepared noCell = prepare(unset, range(unset, 1, 2), everyPair(unset));
  CheckedComputation vacuous = checkComputationSymbolic(
      unset, noCell.proof(), noCell.enumeration.groups[0],
      computationOptions(range(unset, 1, 2), everyPair(unset)), z3Backend);
  CHECK(vacuous.status() == S::Unknown);

  // A site that no run reaches, and a program no run of which completes, have a
  // universal claim that holds of anything. Neither may read as a proof.
  for (const char *name : {"dead_site", "all_stop", "site_out_of_bounds"}) {
    SemanticProgram idle = m.import(name);
    ParameterScope scope = range(idle, 1, 2);
    Authorizations all = everyPair(idle);
    Prepared ready = prepare(idle, scope, all);
    CHECK(ready.proof() != nullptr);
    SiteGroup group;
    group.output = idle.sites[0].output->root;
    group.first = idle.sites[0].a.root;
    group.second = idle.sites[0].b.root;
    group.sites = {0};
    CheckedComputation out = checkComputationSymbolic(idle, ready.proof(), group,
                                                      computationOptions(scope, all), z3Backend);
    if (out.status() != S::Unknown || out.reason().find("NV") == std::string::npos)
      printf("%s: %s\n", name, out.reason().c_str());
    CHECK(out.status() == S::Unknown);
    CHECK(out.reason().find("NV") != std::string::npos);
  }

  // A stop through state is named, a frame sees through state, and an equal second
  // load is its own effect.
  CHECK(!symbolicOf("state_stop").memoryStops().empty());
  bool fromMemory = false;
  CheckedComputation stored = symbolicOf("state_store");
  for (const ResidualEffect &effect : stored.frame())
    fromMemory = fromMemory || (effect.kind == MemoryEffect::Write && effect.valueReadsMemory);
  CHECK(fromMemory);
  auto extra = symbolicOf("equal_loads").frame();
  CHECK(extra.size() == 1 && extra[0].kind == MemoryEffect::Read && extra[0].root == 0);

  // The accumulator form stays unsupported, and its store does not vanish.
  SemanticProgram scalar = m.import("scalar");
  Prepared scalars = prepare(scalar, range(scalar, 1, 2), everyPair(scalar));
  CHECK(scalars.enumeration.gate == GateStatus::Unsupported);
}

static void testSeparation() {
  Module m(catalogue());
  SemanticProgram p = m.import("clean");
  ParameterScope scope = range(p, 1, 2);
  Authorizations all = everyPair(p);
  Prepared ready = prepare(p, scope, all);
  const SiteGroup &group = ready.enumeration.groups[0];

  // No name, no claim, on either route.
  ComputationOptions none = computationOptions(scope, {});
  CheckedComputation finite = checkComputationFinite(p, ready.enumeration, 0, none);
  CheckedComputation symbolic = checkComputationSymbolic(p, ready.proof(), group, none, z3Backend);
  CHECK(finite.status() == S::Unknown && symbolic.status() == S::Unknown);
  CHECK(finite.reason().find("not authorized") != std::string::npos);
  CHECK(symbolic.reason().find("not authorized") != std::string::npos);

  // One name of two is not enough.
  ComputationOptions half = computationOptions(scope, {"roots 0 and 2 are separate"});
  CHECK(checkComputationFinite(p, ready.enumeration, 0, half).status() == S::Unknown);

  // The two needed pairs suffice, and stay in the result as assumptions. A and B are
  // only read, so they need not be apart, and D is not touched.
  ComputationOptions needed = computationOptions(
      scope, {"roots 0 and 2 are separate", "roots 1 and 2 are separate"});
  CheckedComputation held = checkComputationSymbolic(p, ready.proof(), group, needed, z3Backend);
  CHECK(held.status() == S::Verified);
  CHECK(held.separations().size() == 2);
  for (const SeparationUse &use : held.separations())
    CHECK(use.assumption && use.basis == "authorized" && !use.name.empty());

  // llvm.noalias is a promise, not a proof: it needs the name as well.
  SemanticProgram restricted = m.import("noalias");
  CHECK(restricted.assumptions.empty()); // G1 calls these proven.
  Prepared bound = prepare(restricted, range(restricted, 1, 2), everyPair(restricted));
  ComputationOptions unnamed = computationOptions(range(restricted, 1, 2), {});
  CHECK(checkComputationSymbolic(restricted, bound.proof(), bound.enumeration.groups[0], unnamed,
                                 z3Backend).status() == S::Unknown);
  CheckedComputation named = checkComputationSymbolic(
      restricted, bound.proof(), bound.enumeration.groups[0],
      computationOptions(range(restricted, 1, 2), everyPair(restricted)), z3Backend);
  CHECK(named.status() == S::Verified);
  for (const SeparationUse &use : named.separations())
    CHECK(use.assumption && use.basis.find("llvm.noalias") != std::string::npos);

  // A fresh allocation is apart by construction, and that is the only such case.
  SemanticProgram fresh = m.import("fresh_prefix");
  Prepared made = prepare(fresh, range(fresh, 1, 2), {});
  CheckedComputation own = checkComputationFinite(fresh, made.enumeration, 0,
                                                  computationOptions(range(fresh, 1, 2), {}));
  CHECK(own.status() == S::Verified && own.separations().size() == 2);
  for (const SeparationUse &use : own.separations())
    CHECK(!use.assumption && use.basis == "fresh allocation");

  // A value through two roots the group never touches: their overlap matters too.
  SemanticProgram relay = m.import("relay");
  ParameterScope relayScope = range(relay, 1, 2);
  Authorizations partial = everyPair(relay);
  partial.erase("roots 3 and 4 are separate");
  Prepared relayed = prepare(relay, relayScope, everyPair(relay));
  CheckedComputation withoutPair = checkComputationFinite(
      relay, relayed.enumeration, 0, computationOptions(relayScope, partial));
  CHECK(withoutPair.status() == S::Unknown);
  CHECK(withoutPair.reason().find("roots 3 and 4") != std::string::npos);
  CheckedComputation withPair = checkComputationFinite(
      relay, relayed.enumeration, 0, computationOptions(relayScope, everyPair(relay)));
  CHECK(withPair.status() == S::Verified);
  bool listed = false;
  for (const SeparationUse &use : withPair.separations())
    listed = listed || (use.first == 3 && use.second == 4);
  CHECK(listed);

  // A third root written elsewhere could alias C: it is needed too.
  SemanticProgram other = m.import("foreign_elsewhere");
  Prepared third = prepare(other, range(other, 1, 2), everyPair(other));
  CheckedComputation result =
      checkComputationFinite(other, third.enumeration, 0,
                             computationOptions(range(other, 1, 2),
                                                {"roots 0 and 2 are separate", "roots 1 and 2 are separate"}));
  CHECK(result.status() == S::Unknown);
  CHECK(result.reason().find("3") != std::string::npos);
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

static void testSolverLimits() {
  Module m(catalogue());
  SemanticProgram p = m.import("same_root");
  ParameterScope scope = range(p, 1, 2);
  Authorizations all = everyPair(p);
  Prepared ready = prepare(p, scope, all);
  const SiteGroup &group = ready.enumeration.groups[0];
  ComputationOptions options = computationOptions(scope, all);
  CHECK(checkComputationSymbolic(p, ready.proof(), group, options, z3Backend).tier() == T::Symbolic);

  // Each obligation with a query, answered unknown or sat: no tier.
  for (const char *id : {"F1 ", "OP2 "})
    for (SmtAnswer answer : {SmtAnswer::Unknown, SmtAnswer::Sat}) {
      Selective solver(id, answer);
      CheckedComputation out = checkComputationSymbolic(p, ready.proof(), group, options, solver);
      CHECK(solver.hits > 0);
      CHECK(out.status() == S::Unknown && out.tier() == T::Proposed);
    }

  // Every query unknown, including the certificate's.
  Selective nothing("check-sat", SmtAnswer::Unknown);
  CHECK(checkComputationSymbolic(p, ready.proof(), group, options, nothing).tier() == T::Proposed);

  // A limit exhausted is an unknown.
  ComputationOptions few = options;
  few.limits.maxQueries = 1;
  CheckedComputation limited = checkComputationSymbolic(p, ready.proof(), group, few, z3Backend);
  CHECK(limited.status() == S::Unknown);
  ComputationOptions narrow = options;
  narrow.limits.maxPairs = 1;
  CHECK(checkComputationSymbolic(p, ready.proof(), group, narrow, z3Backend).status() == S::Unknown);
  ComputationOptions shallow = options;
  shallow.termination.paths.maxPaths = 1;
  CHECK(checkComputationSymbolic(p, ready.proof(), group, shallow, z3Backend).status() == S::Unknown);

  // A limit of zero is no limit, so it is refused, before any solver call.
  ComputationOptions noTimeout = options, noResource = options;
  noTimeout.termination.solver.timeoutMilliseconds = 0;
  noResource.termination.solver.resourceLimit = 0;
  for (const ComputationOptions &bad : {noTimeout, noResource}) {
    CheckedComputation out = checkComputationSymbolic(p, ready.proof(), group, bad, z3Backend);
    CHECK(out.status() == S::Unknown && out.reason().find("limit of zero") != std::string::npos);
  }

  // The limits take effect in the solver: a hard product, with each limit alone.
  std::string hard = "(declare-const x (_ BitVec 64))\n(declare-const y (_ BitVec 64))\n"
                     "(assert (= (bvmul x y) #x9b2f1c7de3a5f6c9))\n"
                     "(assert (bvugt x #x0000000000000001))\n(assert (bvugt y #x0000000000000001))\n"
                     "(assert (bvult x #x0000000100000000))\n(assert (bvult y #x0000000100000000))\n"
                     "(check-sat)\n";
  auto timed = [&](SmtLimits limits) {
    auto begin = std::chrono::steady_clock::now();
    SmtResponse response = z3Backend.check(hard, limits, false);
    double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - begin).count();
    CHECK(response.answer == SmtAnswer::Unknown);
    CHECK(seconds < 30);
    printf("limit: %.2f s, %s\n", seconds, response.detail.c_str());
  };
  SmtLimits byTime, byResource;
  byTime.timeoutMilliseconds = 200;
  byTime.resourceLimit = 4000000000u;
  byResource.timeoutMilliseconds = 60000;
  byResource.resourceLimit = 20000;
  timed(byTime);
  timed(byResource);

  // Replay limits, likewise.
  ComputationOptions tight = options;
  tight.limits.replay.maxSteps = 2;
  CHECK(checkComputationFinite(p, ready.enumeration, 0, tight).status() == S::Unknown);
  ComputationOptions small = options;
  small.limits.replay.maxNodes = 3;
  CHECK(checkComputationFinite(p, ready.enumeration, 0, small).status() == S::Unknown);
  ComputationOptions cells = options;
  cells.limits.replay.maxCells = 2;
  CHECK(checkComputationFinite(p, ready.enumeration, 0, cells).status() == S::Unknown);
  CHECK(replayGroup(p, group, {APInt(16, 1)}, tight.limits.replay).outcome == ReplayOutcome::Limit);
}

/// The structural obligations cannot fail on what the importer records, so here the
/// record lies. Each lie must end the claim on both routes.
static void testTampered() {
  Module m(catalogue());
  SemanticProgram original = m.import("foreign_same_root");
  ParameterScope scope = range(original, 1, 2);
  Authorizations all = everyPair(original);
  Prepared ready = prepare(original, scope, all);
  ComputationOptions options = computationOptions(scope, all);
  const SiteGroup &group = ready.enumeration.groups[0];
  CHECK(checkComputationSymbolic(original, ready.proof(), group, options, z3Backend).tier() == T::Symbolic);

  unsigned location = original.sites[0].location;
  TermContext &terms = *original.terms;
  Term other = terms.bv(APInt(16, 3));
  // `transitionsLie`: the finite route reads transitions alone, so a lie in a site
  // record that leaves the run as it was cannot change its answer, and should not.
  auto lie = [&](const char *what, const std::function<void(SemanticProgram &)> &edit,
                 const char *expected, bool finiteToo = true) {
    SemanticProgram p = original;
    edit(p);
    CheckedComputation symbolic = checkComputationSymbolic(p, ready.proof(), group, options, z3Backend);
    CheckedComputation finite = checkComputationFinite(p, ready.enumeration, 0, options);
    if (symbolic.status() == S::Verified || (finiteToo && finite.status() == S::Verified) ||
        symbolic.reason().find(expected) == std::string::npos)
      printf("%s: symbolic %s (%s), finite %s (%s)\n", what,
             stringifyComputationStatus(symbolic.status()), symbolic.reason().c_str(),
             stringifyComputationStatus(finite.status()), finite.reason().c_str());
    CHECK(symbolic.status() != S::Verified);
    CHECK(!finiteToo || finite.status() != S::Verified);
    CHECK(symbolic.reason().find(expected) != std::string::npos);
  };
  lie("store index", [&](SemanticProgram &p) { p.sites[0].output->index[0] = other; }, "E1");
  lie("store position", [&](SemanticProgram &p) { --p.sites[0].effectIndex; }, "E1");
  lie("store past the end", [&](SemanticProgram &p) { p.sites[0].effectIndex = 99; }, "E1");
  lie("a read as the store", [&](SemanticProgram &p) { p.sites[0].effectIndex = 0; }, "E1");
  lie("update", [&](SemanticProgram &p) { p.sites[0].updateValue = p.sites[0].aValue; }, "R1");
  lie("reduction flags", [&](SemanticProgram &p) { p.sites[0].reductionFlags = FlagReassoc; }, "R1");
  lie("multiply flags", [&](SemanticProgram &p) { p.sites[0].multiplyFlags = FlagContract; }, "R1");
  lie("reduction", [&](SemanticProgram &p) { p.sites[0].reduction = ReductionOp::Add; }, "R1");
  lie("operand values", [&](SemanticProgram &p) {
    std::swap(p.sites[0].aValue, p.sites[0].bValue);
  }, "R1");
  lie("accumulator", [&](SemanticProgram &p) { p.sites[0].accumulatedValue = p.sites[0].aValue; }, "R1");
  lie("operand index", [&](SemanticProgram &p) { p.sites[0].a.index[0] = other; }, "OP1");
  lie("read index", [&](SemanticProgram &p) { p.sites[0].outputRead->index[0] = other; }, "W1r");
  lie("a store of another value", [&](SemanticProgram &p) {
    for (Transition &t : p.transitions)
      if (t.from == location)
        t.effects[p.sites[0].effectIndex].value = p.sites[0].bValue;
  }, "R1");
  // A write the terms do not show, before the read, at a cell nothing else touches.
  lie("a write the terms do not show", [&](SemanticProgram &p) {
    MacSite &site = p.sites[0];
    for (Transition &t : p.transitions)
      if (t.from == location) {
        MemoryEffect write = t.effects[site.effectIndex];
        write.index[0] = terms.bv(APInt(16, 3));
        write.index[1] = terms.bv(APInt(16, 3));
        write.value = site.aValue;
        t.effects.insert(t.effects.begin(), write);
      }
    ++site.effectIndex;
  }, "W1r", false); // The finite route reads the list, and that write is harmless.
  // Two sites of the group with one store: a contribution would count twice.
  {
    SemanticProgram p = original;
    MacSite copy = p.sites[0];
    copy.id = p.sites.size();
    p.sites.push_back(copy);
    SiteGroup both = group;
    both.sites = {0, copy.id};
    CheckedComputation out = checkComputationSymbolic(p, ready.proof(), both, options, z3Backend);
    CHECK(out.status() == S::Unknown);
    CHECK(out.reason().find("E2") != std::string::npos);
  }
  lie("edge effects", [&](SemanticProgram &p) {
    // The second way out of a loop head, which lists an effect the first does not.
    std::map<unsigned, unsigned> seen;
    MemoryEffect read = p.transitions[0].effects.empty()
                            ? MemoryEffect()
                            : p.transitions[0].effects[0];
    for (const Transition &t : p.transitions)
      if (t.from == location)
        read = t.effects[0];
    for (Transition &t : p.transitions)
      if (++seen[t.from] == 2)
        t.effects.push_back(read);
  }, "E0", false);
  lie("a second owner", [&](SemanticProgram &p) {
    MacSite copy = p.sites[0];
    copy.id = p.sites.size();
    copy.a.root = 3;
    p.sites.push_back(copy);
  }, "E2", false);
  lie("an unlisted write", [&](SemanticProgram &p) {
    for (Transition &t : p.transitions)
      if (t.from == location) {
        MemoryEffect write = t.effects[p.sites[0].effectIndex];
        write.opIndex += 100;
        write.value = p.sites[0].bValue;
        t.effects.push_back(write);
      }
  }, "F1");
}

static uint64_t bitsOf(double value) { return llvm::bit_cast<uint64_t>(value); }

static EntryState floatEntry(const SemanticProgram &p, unsigned n,
                             const std::function<double(unsigned, unsigned)> &cell) {
  EntryState entry;
  entry.parameters = {APInt(16, n)};
  for (const MemoryRoot &root : p.roots) {
    RootData data;
    data.shape = {4, 4};
    for (unsigned c = 0; c < 16; ++c)
      data.cells.emplace_back(APInt(64, bitsOf(cell(root.id, c))));
    entry.memory.push_back(std::move(data));
  }
  return entry;
}

/// Theorem 11 against real arithmetic, at values chosen to expose a wrong fold.
static void testFloatSemantics() {
  Module m(catalogue());
  SemanticProgram p = m.import("clean");
  CHECK(p.isSupported());
  ReplayResult replay = replayGroup(p, [&] {
    SiteGroup group;
    group.output = 2;
    group.first = 0;
    group.second = 1;
    group.sites = {0};
    return group;
  }(), {APInt(16, 1)}, HerbrandLimits());
  CHECK(replay.outcome == ReplayOutcome::Equal);
  CHECK(replay.facts.events == 4 && replay.facts.outputCells == 2 &&
        replay.facts.maxContributions == 2);
  CHECK(!replay.conditionalStops.empty()); // The NaN stop stays visible.

  SiteGroup group;
  group.output = 2;
  group.first = 0;
  group.second = 1;
  group.sites = {0};
  auto differs = [&](const EntryState &entry, RunStatus expected) {
    RunResult run = runConcrete(p, entry);
    CHECK(run.status == expected);
    return replayDifference(p, group, entry, run, replay.prefix).has_value();
  };
  // Rounding: 1e16 + 1 + 1 stays 1e16 in order, and the sum of the two first would not.
  EntryState rounding = floatEntry(p, 1, [](unsigned root, unsigned) {
    return root == 2 ? 1e16 : 1.0;
  });
  CHECK(!differs(rounding, RunStatus::Returned));
  RunResult folded = runConcrete(p, rounding);
  CHECK(*folded.memory[2].cells[0] == APInt(64, bitsOf(1e16)));
  // Signed zero: -0 + (0 * x) is +0, and a start from +0 would lose nothing here but
  // the fold must still be the exact one.
  EntryState zero = floatEntry(p, 1, [](unsigned root, unsigned) {
    return root == 2 ? -0.0 : root == 0 ? 0.0 : 1.0;
  });
  CHECK(!differs(zero, RunStatus::Returned));
  EntryState negative = floatEntry(p, 1, [](unsigned root, unsigned) {
    return root == 2 ? -0.0 : root == 0 ? -0.0 : 1.0;
  });
  CHECK(!differs(negative, RunStatus::Returned));
  CHECK(*runConcrete(p, negative).memory[2].cells[0] == APInt(64, bitsOf(-0.0)));
  // NaN: the run stops, so the run has no claim and no witness.
  EntryState nan = floatEntry(p, 1, [](unsigned root, unsigned) {
    return root == 0 ? std::numeric_limits<double>::quiet_NaN() : 1.0;
  });
  CHECK(!differs(nan, RunStatus::Underspecified));
  // An infinity meets zero: NaN again. Infinities that stay are ordinary values.
  EntryState infinite = floatEntry(p, 1, [](unsigned root, unsigned) {
    return root == 0 ? std::numeric_limits<double>::infinity() : root == 1 ? 0.0 : 1.0;
  });
  CHECK(!differs(infinite, RunStatus::Underspecified));
  EntryState large = floatEntry(p, 1, [](unsigned root, unsigned) {
    return root == 0 ? std::numeric_limits<double>::infinity() : 1.0;
  });
  CHECK(!differs(large, RunStatus::Returned));

  // Fuzz: the relation holds at every fill, with no reference fill anywhere.
  std::mt19937_64 random(7);
  const double pool[] = {0.0, -0.0, 1.0, -1.0, 1e16, -1e16, 1e308, 1e-308, 0.1, 3.0};
  for (unsigned n = 0; n < 400; ++n) {
    EntryState entry = floatEntry(p, 1 + n % 2, [&](unsigned, unsigned) {
      return pool[random() % std::size(pool)];
    });
    RunResult run = runConcrete(p, entry);
    CHECK(!replayDifference(p, group, entry, run, replay.prefix).has_value() ||
          entry.parameters[0] != APInt(16, 1));
  }
}

/// One-dimensional programs that go wrong only at i == 20. The replay that NV runs
/// looks at the first 16 points of a scope, and sees nothing, so only the obligation
/// named can stop these. With the scope below 20 the same program must pass, or the
/// test would prove nothing about that obligation.
static std::string farProgram(const std::string &name, const std::string &loopBody,
                              const std::string &after = "", const std::string &result = "") {
  return "func.func @" + name + "(%a: memref<64xi32>, %b: memref<64xi32>, %c: memref<64xi32>, "
         "%d: memref<64xi32>, %n: index)" + (result.empty() ? "" : " -> " + result) + R"( {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c20 = arith.constant 20 : index
  %c32 = arith.constant 32 : index
  %iz = arith.constant 0 : i32
  scf.for %i = %c0 to %n step %c1 {
    %at20 = arith.cmpi eq, %i, %c20 : index
    %next = arith.addi %i, %c1 : index
    %far = arith.addi %i, %c32 : index
)" + loopBody + "  }\n" + after + (result.empty() ? "  return\n" : "") + "}\n";
}

static std::string farCatalogue() {
  std::string load = R"(    %x = memref.load %a[%i] : memref<64xi32>
    %y = memref.load %b[%i] : memref<64xi32>
    %m = arith.muli %x, %y : i32
)";
  std::string plain = load + R"(    %z = memref.load %c[%i] : memref<64xi32>
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<64xi32>
)";
  std::string out;
  // W1: at i == 20 the update reads the next cell.
  out += farProgram("far_w1", R"(    %r = arith.select %at20, %next, %i : index
)" + load + R"(    %z = memref.load %c[%r] : memref<64xi32>
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<64xi32>
)");
  // W2: two sites of the group, and at i == 20 the second read is stale.
  out += farProgram("far_w2", R"(    %r = arith.select %at20, %i, %far : index
)" + load + R"(    %z = memref.load %c[%i] : memref<64xi32>
    %w = memref.load %c[%r] : memref<64xi32>
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<64xi32>
    %t = arith.addi %w, %m : i32
    memref.store %t, %c[%r] : memref<64xi32>
)");
  // OP2: another write reaches an operand cell.
  out += farProgram("far_op2", plain + R"(    scf.if %at20 {
      memref.store %iz, %a[%next] : memref<64xi32>
    }
)");
  // F2: memory leaves through a return, by a load, and by a value in state.
  out += farProgram("far_return_load", plain, R"(  %last = arith.cmpi eq, %n, %c20 : index
  cf.cond_br %last, ^late, ^end
^late:
  %r = memref.load %c[%c0] : memref<64xi32>
  return %r : i32
^end:
  return %iz : i32
)", "i32");
  out += farProgram("far_return_state", plain, R"(  %last = arith.cmpi eq, %n, %c20 : index
  %r = memref.load %c[%c0] : memref<64xi32>
  cf.cond_br %last, ^late(%r : i32), ^late(%iz : i32)
^late(%v: i32):
  return %v : i32
)", "i32");
  // Two sites of one group, on cells that never meet.
  out += farProgram("two_parts", plain + R"(    %p = memref.load %a[%i] : memref<64xi32>
    %q = memref.load %b[%i] : memref<64xi32>
    %u = memref.load %c[%far] : memref<64xi32>
    %v = arith.muli %p, %q : i32
    %w = arith.addi %u, %v : i32
    memref.store %w, %c[%far] : memref<64xi32>
)");
  // The update reads one root and writes another.
  out += farProgram("other_root", load + R"(    %z = memref.load %d[%i] : memref<64xi32>
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<64xi32>
)");
  // An address from memory.
  out += farProgram("memory_address", load + R"(    %e = arith.index_cast %x : i32 to index
    %z = memref.load %c[%e] : memref<64xi32>
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%e] : memref<64xi32>
)");
  // A stop that depends on memory and cannot happen: the divisor is odd.
  out += farProgram("cannot_stop", R"(    %p = memref.load %a[%i] : memref<64xi32>
    %one = arith.constant 1 : i32
    %odd = arith.ori %p, %one : i32
    %q = arith.divui %one, %odd : i32
)" + plain);
  // Equal in value and different as nodes, only from n = 2.
  out += farProgram("open_at_two", plain, R"(  %big = arith.cmpi uge, %n, %c1 : index
  %two = arith.addi %c1, %c1 : index
  %wide = arith.cmpi uge, %n, %two : index
  scf.if %wide {
    %q = memref.load %c[%c0] : memref<64xi32>
    %one = arith.constant 1 : i32
    %r = arith.muli %q, %one : i32
    memref.store %r, %c[%c0] : memref<64xi32>
  }
)");
  // In bounds to n = 4, with a 4-cell output.
  out += "func.func @bounded(%a: memref<4xi32>, %b: memref<4xi32>, %c: memref<4xi32>, %n: index) {\n"
         "  %c0 = arith.constant 0 : index\n  %c1 = arith.constant 1 : index\n"
         "  scf.for %i = %c0 to %n step %c1 {\n"
         "    %x = memref.load %a[%i] : memref<4xi32>\n    %y = memref.load %b[%i] : memref<4xi32>\n"
         "    %z = memref.load %c[%i] : memref<4xi32>\n    %m = arith.muli %x, %y : i32\n"
         "    %s = arith.addi %z, %m : i32\n    memref.store %s, %c[%i] : memref<4xi32>\n  }\n  return\n}\n";
  return out;
}

/// Each case pins one thing a mutant showed to be unseen by the other tests.
static void testGaps() {
  Module m(farCatalogue());
  Module cat(catalogue());
  auto symbolic = [&](const char *name, int64_t low, int64_t high) {
    SemanticProgram p = m.import(name, 16);
    CHECK(p.isSupported());
    ParameterScope scope = range(p, low, high);
    Authorizations all = everyPair(p);
    Prepared ready = prepare(p, scope, all);
    CheckedComputation out = checkComputationSymbolic(
        p, ready.proof(), ready.enumeration.groups[0], computationOptions(scope, all), z3Backend);
    return out;
  };
  // Below i == 20 each program is correct, and past it only one obligation sees.
  struct Far {
    const char *name, *id;
  };
  for (Far far : {Far{"far_w1", "W1 "}, Far{"far_w2", "W2 "}, Far{"far_op2", "OP2 "}}) {
    CheckedComputation near = symbolic(far.name, 1, 16);
    if (near.status() != S::Verified)
      printf("%s near: %s\n", far.name, near.reason().c_str());
    CHECK(near.status() == S::Verified);
    CheckedComputation beyond = symbolic(far.name, 1, 30);
    if (beyond.status() != S::Unknown || beyond.reason().find(far.id) == std::string::npos)
      printf("%s beyond: %s %s\n", far.name, stringifyComputationStatus(beyond.status()),
             beyond.reason().c_str());
    CHECK(beyond.status() == S::Unknown);
    CHECK(beyond.tier() == T::Proposed);
    CHECK(beyond.reason().find(far.id) != std::string::npos);
    CHECK(beyond.failingPoint().has_value());
  }
  for (const char *name : {"far_return_load", "far_return_state"}) {
    CheckedComputation out = symbolic(name, 1, 30);
    if (out.reason().find("F2") == std::string::npos)
      printf("%s: %s %s\n", name, stringifyComputationStatus(out.status()), out.reason().c_str());
    CHECK(out.status() == S::Unknown);
    CHECK(out.reason().find("F2") != std::string::npos);
  }
  // That far write at i == 20 is real: the finite route refutes it, exactly there.
  {
    SemanticProgram p = m.import("far_op2");
    ParameterScope scope = range(p, 1, 30);
    Authorizations all = everyPair(p);
    Prepared ready = prepare(p, scope, all);
    CheckedComputation out =
        checkComputationFinite(p, ready.enumeration, 0, computationOptions(scope, all));
    CHECK(out.status() == S::Refuted);
    CHECK(out.witness() && out.witness()->parameters[0].getZExtValue() > 21);
  }

  // Finite tiers.
  {
    // A limit reached after events is no pass, and neither is a stop or an open point.
    SemanticProgram p = m.import("bounded");
    ParameterScope scope = range(p, 3, 3);
    Authorizations all = everyPair(p);
    Prepared ready = prepare(p, scope, all);
    ComputationOptions options = computationOptions(scope, all);
    HerbrandLimits few;
    few.maxSteps = 5;
    ReplayResult cut = replayGroup(p, ready.enumeration.groups[0], {APInt(16, 3)}, few);
    CHECK(cut.outcome == ReplayOutcome::Limit && cut.facts.events > 0);
    options.limits.replay = few;
    CheckedComputation limited = checkComputationFinite(p, ready.enumeration, 0, options);
    CHECK(limited.status() == S::Unknown);
    CHECK(limited.reason().find("LIMIT") != std::string::npos);

    // n = 5 leaves the root, so its run stops and that point is open.
    ParameterScope wide = range(p, 3, 5);
    Prepared stops = prepare(p, wide, all);
    CheckedComputation partial =
        checkComputationFinite(p, stops.enumeration, 0, computationOptions(wide, all));
    CHECK(partial.status() == S::Verified && partial.tier() == T::Point);

    // n = 0 has no event, so there is nothing to claim.
    ParameterScope empty = range(p, 0, 0);
    Prepared idle = prepare(p, empty, all);
    CheckedComputation none =
        checkComputationFinite(p, idle.enumeration, 0, computationOptions(empty, all));
    CHECK(none.status() == S::Unknown);
    CHECK(none.reason().find("no point has events") != std::string::npos);
    CHECK(none.points().empty());
  }
  {
    // One point open, by a mismatch with no witness, keeps the rest at Point.
    SemanticProgram p = m.import("open_at_two");
    ParameterScope scope = range(p, 1, 2);
    Authorizations all = everyPair(p);
    Prepared ready = prepare(p, scope, all);
    CheckedComputation out =
        checkComputationFinite(p, ready.enumeration, 0, computationOptions(scope, all));
    CHECK(out.status() == S::Verified && out.tier() == T::Point);
    CHECK(out.points().size() == 1);
    CHECK(out.reason().find("another point is open") != std::string::npos);
  }
  {
    // A closed gate says so, and does not reach the replay.
    SemanticProgram p = m.import("bounded");
    ParameterScope scope = range(p, 1, 2);
    DomainEnumeration closed = enumerateDomain(p, nullptr, EnumerationOptions(), z3Backend);
    CHECK(closed.gate != GateStatus::Open);
    CheckedComputation out =
        checkComputationFinite(p, closed, 0, computationOptions(scope, everyPair(p)));
    CHECK(out.status() == S::Unknown);
    CHECK(out.reason().find("domain gate is closed") != std::string::npos);
  }

  // Gates.
  {
    // A claim about part of a group is none, even when the parts never meet.
    SemanticProgram p = m.import("two_parts");
    CHECK(p.sites.size() == 2);
    ParameterScope scope = range(p, 1, 2);
    Authorizations all = everyPair(p);
    Prepared ready = prepare(p, scope, all);
    ComputationOptions options = computationOptions(scope, all);
    CHECK(checkComputationSymbolic(p, ready.proof(), ready.enumeration.groups[0], options,
                                   z3Backend).status() == S::Verified);
    SiteGroup part = ready.enumeration.groups[0];
    part.sites.pop_back();
    CheckedComputation symbolicPart = checkComputationSymbolic(p, ready.proof(), part, options, z3Backend);
    CHECK(symbolicPart.status() == S::Unknown);
    CHECK(symbolicPart.reason().find("not the sites") != std::string::npos);
    DomainEnumeration narrowed = ready.enumeration;
    narrowed.groups[0].sites.pop_back();
    CheckedComputation finitePart = checkComputationFinite(p, narrowed, 0, options);
    CHECK(finitePart.status() == S::Unknown);
    CHECK(finitePart.reason().find("not the sites") != std::string::npos);
  }
  {
    // The first gate says why, and the group check behind it would only say less.
    SemanticProgram p = m.import("other_root");
    ParameterScope scope = range(p, 1, 2);
    Authorizations all = everyPair(p);
    Prepared ready = prepare(p, scope, all);
    CHECK(!ready.enumeration.groups[0].unsupported.empty());
    CheckedComputation out = checkComputationSymbolic(
        p, ready.proof(), ready.enumeration.groups[0], computationOptions(scope, all), z3Backend);
    CHECK(out.status() == S::Unknown);
    CHECK(out.reason().find("reads one root and writes another") != std::string::npos);
  }
  {
    SemanticProgram p = m.import("memory_address");
    CHECK(p.isSupported() && !p.addressOblivious);
    ParameterScope scope = range(p, 1, 2);
    Authorizations all = everyPair(p);
    Prepared ready = prepare(p, scope, all);
    SiteGroup group;
    group.output = p.sites[0].output->root;
    group.first = p.sites[0].a.root;
    group.second = p.sites[0].b.root;
    group.sites = {0};
    CheckedComputation out = checkComputationSymbolic(p, ready.proof(), group,
                                                      computationOptions(scope, all), z3Backend);
    CHECK(out.status() == S::Unknown);
    CHECK(out.reason().find("depends on memory contents") != std::string::npos);
  }
  {
    // A certificate with its ranks removed is not accepted, whatever else would pass.
    SemanticProgram p = m.import("bounded");
    ParameterScope scope = range(p, 1, 2);
    Authorizations all = everyPair(p);
    Prepared ready = prepare(p, scope, all);
    CHECK(ready.certificate.has_value());
    TerminationCertificate broken = *ready.certificate;
    broken.levels.clear();
    CheckedComputation out = checkComputationSymbolic(
        p, &broken, ready.enumeration.groups[0], computationOptions(scope, all), z3Backend);
    CHECK(out.status() == S::Unknown);
    CHECK(out.reason().find("not accepted") != std::string::npos);
  }
  {
    // F3 goes first: NV would also stop this, but must not be what says so.
    SemanticProgram p = cat.import("fresh_unset");
    ParameterScope scope = range(p, 1, 2);
    Authorizations all = everyPair(p);
    Prepared ready = prepare(p, scope, all);
    SiteGroup group;
    group.output = p.sites[0].output->root;
    group.first = p.sites[0].a.root;
    group.second = p.sites[0].b.root;
    group.sites = {0};
    CheckedComputation out = checkComputationSymbolic(p, ready.proof(), group,
                                                      computationOptions(scope, all), z3Backend);
    CHECK(out.reason().find("F3") != std::string::npos);
  }
  {
    // The fast path of W2 names itself, and the query behind it would also refuse.
    SemanticProgram p = cat.import("lost_update");
    ParameterScope scope = range(p, 1, 2);
    Authorizations all = everyPair(p);
    Prepared ready = prepare(p, scope, all);
    CheckedComputation out = checkComputationSymbolic(
        p, ready.proof(), ready.enumeration.groups[0], computationOptions(scope, all), z3Backend);
    CHECK(out.reason().find("W2i") != std::string::npos);
  }
  {
    // No stop can happen, and the conservative answer stands: a stop that reads memory
    // keeps "every run" off, even when the certificate says that none is reachable.
    SemanticProgram p = m.import("cannot_stop");
    ParameterScope scope = range(p, 1, 2);
    Authorizations all = everyPair(p);
    Prepared ready = prepare(p, scope, all);
    CHECK(ready.enumeration.termination.kind() == TerminationVerdict::ProvenTerminating);
    ComputationOptions options = computationOptions(scope, all);
    CheckedComputation finite = checkComputationFinite(p, ready.enumeration, 0, options);
    CheckedComputation proved = checkComputationSymbolic(p, ready.proof(),
                                                         ready.enumeration.groups[0], options, z3Backend);
    CHECK(finite.status() == S::Verified && proved.status() == S::Verified);
    CHECK(!finite.memoryStops().empty() && !proved.memoryStops().empty());
    CHECK(!finite.holdsForEveryRun());
    CHECK(!proved.holdsForEveryRun());
  }
  {
    // Evidence for the equivalent mutant "output not separated": every root of a group
    // is the root of an effect, so `accessed` holds it without its initializer.
    for (const char *name : {"clean", "overlap", "chain", "same_root", "relay", "collapsed"}) {
      SemanticProgram p = cat.import(name);
      std::set<unsigned> touched;
      for (const Transition &t : p.transitions)
        for (const MemoryEffect &effect : t.effects)
          touched.insert(effect.root);
      for (const MacSite &site : p.sites) {
        CHECK(site.output && touched.count(site.output->root));
        CHECK(touched.count(site.a.root) && touched.count(site.b.root));
      }
    }
  }
}

/// A one-dimensional i32 program with an f64 scalar `alpha`, for the second round of
/// mutation gaps. `size` is the extent of every root.
static std::string line(const std::string &name, const std::string &entry, const std::string &body,
                        const std::string &after = "", const std::string &size = "64") {
  std::string t = "memref<" + size + "xi32>";
  return "func.func @" + name + "(%a: " + t + ", %b: " + t + ", %c: " + t + ", %d: " + t +
         ", %n: index, %alpha: f64) {\n"
         "  %c0 = arith.constant 0 : index\n  %c1 = arith.constant 1 : index\n"
         "  %c20 = arith.constant 20 : index\n  %iz = arith.constant 0 : i32\n"
         "  %ione = arith.constant 1 : i32\n" + entry +
         "  scf.for %i = %c0 to %n step %c1 {\n    %at20 = arith.cmpi eq, %i, %c20 : index\n"
         "    %next = arith.addi %i, %c1 : index\n" + body + "  }\n" + after + "  return\n}\n";
}

static const char *site1 = R"(    %x = memref.load %a[%i] : memref<64xi32>
    %y = memref.load %b[%i] : memref<64xi32>
    %z = memref.load %c[%i] : memref<64xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<64xi32>
)";

static std::string secondCatalogue() {
  std::string out;
  // The second update is stale, and a later write puts the right final value back:
  // only the comparison of each event sees it.
  out += line("repaired", "", std::string(site1) + R"(    %t = arith.addi %z, %m : i32
    memref.store %t, %c[%i] : memref<64xi32>
    %u = arith.addi %s, %m : i32
    memref.store %u, %c[%i] : memref<64xi32>
)");
  // A and B exchange their first cells, so each operand read is the other's initial
  // value, and the integer products are still equal.
  out += line("swapped", R"(  %pa = memref.load %a[%c0] : memref<64xi32>
  %pb = memref.load %b[%c0] : memref<64xi32>
  memref.store %pb, %a[%c0] : memref<64xi32>
  memref.store %pa, %b[%c0] : memref<64xi32>
  cf.br ^go
^go:
)", site1);
  // A cell is wrong between two blocks, and the second block puts it back. When that
  // block stops, the cell stays wrong, which is no counterexample. c[1] adds a mismatch
  // of nodes with equal values, so that a witness is looked for.
  out += line("restored", "", site1, R"(  %saved = memref.load %c[%c0] : memref<64xi32>
  %seven = arith.constant 7 : i32
  memref.store %seven, %c[%c0] : memref<64xi32>
  cf.br ^back
^back:
  %q = memref.load %a[%c1] : memref<64xi32>
  %odd = arith.andi %q, %ione : i32
  %even = arith.xori %odd, %ione : i32
  %r = arith.divui %ione, %even : i32
  memref.store %saved, %c[%c0] : memref<64xi32>
  %w = memref.load %c[%c1] : memref<64xi32>
  %v = arith.muli %w, %ione : i32
  memref.store %v, %c[%c1] : memref<64xi32>
)");
  // The cell is set to 5 and then to what it held: the fold starts from that.
  out += line("restore_before", R"(  %held = memref.load %c[%c0] : memref<64xi32>
  %five = arith.constant 5 : i32
  memref.store %five, %c[%c0] : memref<64xi32>
  memref.store %held, %c[%c0] : memref<64xi32>
  cf.br ^go
^go:
)", site1);
  // The same value by another operation: flags, then operand order.
  // A float scalar stored as the start of each fold.
  out += R"(func.func @float_start(%a: memref<64xf64>, %b: memref<64xf64>, %c: memref<64xf64>,
                       %d: memref<64xf64>, %n: index, %alpha: f64) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  scf.for %i = %c0 to %n step %c1 {
    memref.store %alpha, %c[%i] : memref<64xf64>
    %x = memref.load %a[%i] : memref<64xf64>
    %y = memref.load %b[%i] : memref<64xf64>
    %z = memref.load %c[%i] : memref<64xf64>
    %m = arith.mulf %x, %y : f64
    %s = arith.addf %z, %m : f64
    memref.store %s, %c[%i] : memref<64xf64>
  }
  return
}
)";
  // Every run stops after its events: the claim would hold of nothing.
  out += line("stops_after", "", site1, R"(  %big = arith.constant 99 : index
  memref.store %iz, %d[%big] : memref<64xi32>
)");
  out += line("certain_division", R"(  %q = arith.divui %ione, %iz : i32
  cf.br ^go
^go:
)", site1);
  out += line("constant_overflow", R"(  %p = arith.constant 4611686018427387904 : i64
  %four = arith.constant 4 : i64
  %t = arith.muli %p, %four overflow<nsw> : i64
  %tt = arith.trunci %t : i64 to i32
  memref.store %tt, %d[%c0] : memref<64xi32>
  cf.br ^go
^go:
)", site1);
  out += line("unset_scratch", "  %tmp = memref.alloc() : memref<4xi32>\n", site1,
              R"(  %u = memref.load %tmp[%c0] : memref<4xi32>
  memref.store %u, %d[%c0] : memref<64xi32>
)");
  // A guard that reads memory.
  out += line("memory_guard", "", R"(    %g = memref.load %a[%i] : memref<64xi32>
    %p = arith.cmpi sgt, %g, %iz : i32
    scf.if %p {
)" + std::string(site1) + "    }\n");
  // Two sites of one group, as in two_parts.
  out += line("two_sites", "", std::string(site1) + R"(    %e = arith.addi %i, %c20 : index
    %p = memref.load %a[%i] : memref<64xi32>
    %q = memref.load %b[%i] : memref<64xi32>
    %w = memref.load %c[%e] : memref<64xi32>
    %v = arith.muli %p, %q : i32
    %u = arith.addi %w, %v : i32
    memref.store %u, %c[%e] : memref<64xi32>
)");
  return out;
}

/// The same final value by another operation, after the events: a recomputation that a
/// node without flags or with sorted operands would wrongly match.
static std::string recomputed(const std::string &name, const std::string &operation) {
  return "func.func @" + name + R"((%a: memref<64xi32>, %b: memref<64xi32>, %c: memref<64xi32>,
                   %d: memref<64xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %x = memref.load %a[%c0] : memref<64xi32>
  %y = memref.load %b[%c0] : memref<64xi32>
  %z = memref.load %c[%c0] : memref<64xi32>
  %m = arith.muli %x, %y : i32
  %s = arith.addi %z, %m : i32
  memref.store %s, %c[%c0] : memref<64xi32>
  cf.br ^again
^again:
  %u = )" + operation + R"( : i32
  memref.store %u, %c[%c0] : memref<64xi32>
  return
}
)";
}

/// The far OP2 program on roots wide enough that a model with a large n still runs.
static std::string wideFar() {
  return R"(func.func @wide_far(%a: memref<8192xi32>, %b: memref<8192xi32>, %c: memref<8192xi32>,
                    %d: memref<8192xi32>, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c20 = arith.constant 20 : index
  %iz = arith.constant 0 : i32
  scf.for %i = %c0 to %n step %c1 {
    %at20 = arith.cmpi eq, %i, %c20 : index
    %next = arith.addi %i, %c1 : index
    %x = memref.load %a[%i] : memref<8192xi32>
    %y = memref.load %b[%i] : memref<8192xi32>
    %z = memref.load %c[%i] : memref<8192xi32>
    %m = arith.muli %x, %y : i32
    %s = arith.addi %z, %m : i32
    memref.store %s, %c[%i] : memref<8192xi32>
    scf.if %at20 {
      memref.store %iz, %a[%next] : memref<8192xi32>
    }
  }
  return
}
)";
}

/// The second round: each case is the smallest program that one surviving mutant
/// needs, and the expected answer is the one the relation demands.
static void testGapsTwo() {
  Module m(secondCatalogue() + recomputed("flags_differ", "arith.addi %z, %m overflow<nsw>") +
           recomputed("order_differs", "arith.addi %m, %z") + wideFar());
  auto prepared = [&](const char *name, int64_t low, int64_t high) {
    SemanticProgram p = m.import(name, 16);
    CHECK(p.isSupported());
    ParameterScope scope = range(p, low, high);
    Authorizations all = everyPair(p);
    Prepared ready = prepare(p, scope, all);
    return std::make_tuple(std::move(p), scope, all, std::move(ready));
  };
  auto finiteOf = [&](const char *name, int64_t low, int64_t high) {
    auto [p, scope, all, ready] = prepared(name, low, high);
    return checkComputationFinite(p, ready.enumeration, 0, computationOptions(scope, all));
  };
  auto symbolicOf = [&](const char *name, int64_t low, int64_t high) {
    auto [p, scope, all, ready] = prepared(name, low, high);
    SiteGroup group;
    group.output = p.sites[0].output->root;
    group.first = p.sites[0].a.root;
    group.second = p.sites[0].b.root;
    for (const MacSite &site : p.sites)
      group.sites.push_back(site.id);
    return checkComputationSymbolic(p, ready.proof(), group, computationOptions(scope, all), z3Backend);
  };
  auto report = [](const char *name, const CheckedComputation &c) {
    printf("%s: %s %s (%s)\n", name, stringifyComputationStatus(c.status()),
           stringifyComputationTier(c.tier()), c.reason().c_str());
  };

  // Intermediate values are part of the claim.
  CheckedComputation repaired = finiteOf("repaired", 1, 2);
  if (repaired.status() != S::Refuted)
    report("repaired", repaired);
  CHECK(repaired.status() == S::Refuted);
  CHECK(repaired.reason().find("writes a value other than the replay's") != std::string::npos);
  // So are operand reads, even where every product agrees.
  CheckedComputation swapped = finiteOf("swapped", 1, 1);
  if (swapped.status() != S::Refuted)
    report("swapped", swapped);
  CHECK(swapped.status() == S::Refuted);
  CHECK(swapped.reason().find("reads an operand") != std::string::npos);
  // A run that stops is no counterexample, whatever its memory holds. The reference
  // fill completes, so the witness search runs, and about half its fills stop.
  CheckedComputation restored = finiteOf("restored", 2, 2);
  if (restored.status() != S::Unknown || restored.reason().find("MISMATCH") == std::string::npos)
    report("restored", restored);
  CHECK(restored.status() == S::Unknown);
  CHECK(restored.reason().find("MISMATCH") != std::string::npos);
  // A value put back before the first event is where the fold starts.
  CheckedComputation before = finiteOf("restore_before", 1, 2);
  if (before.status() != S::Verified)
    report("restore_before", before);
  CHECK(before.status() == S::Verified);
  // Equal values by another operation are not the replay's operation: flags and
  // operand order are part of what is compared, and no witness can exist.
  for (const char *name : {"flags_differ", "order_differs"}) {
    CheckedComputation out = finiteOf(name, 0, 1);
    if (out.status() != S::Unknown)
      report(name, out);
    CHECK(out.status() == S::Unknown);
    CHECK(out.reason().find("MISMATCH") != std::string::npos);
  }
  // A float scalar is not part of a point: the start from alpha is no constant.
  {
    SemanticProgram p = m.import("float_start", 16);
    CHECK(p.isSupported() && p.parameters.size() == 2);
    ParameterScope scope = range(p, 1, 2);
    Authorizations all = everyPair(p);
    Prepared ready = prepare(p, scope, all);
    // Each point fixes n alone: alpha stays an unknown, so a store of it starts no fold,
    // and a claim must not rest on the value the point happens to carry for it.
    ReplayResult alpha = replayGroup(p, ready.enumeration.groups[0], {APInt(16, 1), APInt(64, 0)},
                                     HerbrandLimits());
    CHECK(alpha.outcome == ReplayOutcome::Mismatch);
    CHECK(alpha.prefix.empty());
    CheckedComputation out =
        checkComputationFinite(p, ready.enumeration, 0, computationOptions(scope, all));
    CHECK(out.status() != S::Verified);
  }
  // Every run stops: no NV, for each way a stop can be certain.
  for (const char *name : {"stops_after", "certain_division", "constant_overflow", "unset_scratch"}) {
    CheckedComputation out = symbolicOf(name, 1, 2);
    if (out.status() != S::Unknown || out.reason().find("NV") == std::string::npos)
      report(name, out);
    CHECK(out.status() == S::Unknown);
    CHECK(out.reason().find("NV") != std::string::npos);
  }
  // A guard from memory refuses the replay, gate or no gate.
  {
    SemanticProgram p = m.import("memory_guard", 16);
    CHECK(!p.controlOblivious);
    SiteGroup group;
    group.output = p.sites[0].output->root;
    group.first = p.sites[0].a.root;
    group.second = p.sites[0].b.root;
    group.sites = {0};
    ReplayResult out = replayGroup(p, group, {APInt(16, 2), APInt(64, 0)}, HerbrandLimits());
    CHECK(out.outcome == ReplayOutcome::Refused);
    CHECK(out.reason.find("guard depends on memory") != std::string::npos);
  }
  // A site whose effect is a read has no event, and the replay must say so.
  {
    auto [p, scope, all, ready] = prepared("two_sites", 1, 2);
    CHECK(p.sites.size() == 2);
    CHECK(checkComputationFinite(p, ready.enumeration, 0, computationOptions(scope, all)).status() ==
          S::Verified);
    SemanticProgram lying = p;
    lying.sites[1].effectIndex = 0;
    CheckedComputation out = checkComputationFinite(lying, ready.enumeration, 0,
                                                    computationOptions(scope, all));
    CHECK(out.status() != S::Verified);
  }
  // A failed query's point is replayed exactly, past what any enumeration covers.
  {
    SemanticProgram p = m.import("wide_far", 16);
    ParameterScope scope = range(p, 1, 5000);
    Authorizations all = everyPair(p);
    TerminationOptions termination;
    termination.scope = scope;
    termination.authorized = all;
    termination.limits.replayFuel = 1u << 12;
    termination.limits.solver = testSolver();
    auto certificate = analyzeTermination(p, z3Backend, termination).certificate;
    CHECK(certificate.has_value());
    ComputationRequest request;
    request.search = false;
    request.synthesis.scope = scope;
    request.synthesis.authorized = all;
    request.synthesis.termination.solver = testSolver();
    request.computation = computationOptions(scope, all);
    ComputationResult result = analyzeComputation(p, certificate ? &*certificate : nullptr,
                                                  request, z3Backend);
    CHECK(result.enumeration.gate != GateStatus::Open);
    CHECK(result.groups.size() == 1);
    if (!result.groups.empty()) {
      if (result.groups[0].finite.status() != S::Refuted)
        report("wide_far", result.groups[0].finite);
      CHECK(result.groups[0].symbolic.status() == S::Unknown);
      CHECK(result.groups[0].finite.status() == S::Refuted);
    }
  }

  // Candidates, with the order the search proposed, so the symbolic path runs.
  {
    Module cat(catalogue());
    SemanticProgram p = cat.import("clean");
    ParameterScope scope = range(p, 1, 2);
    Authorizations all = everyPair(p);
    Prepared ready = prepare(p, scope, all);
    ComputationOptions options = computationOptions(scope, all);
    SynthesisOptions search;
    search.scope = scope;
    search.authorized = all;
    search.maxLevel = 0;
    search.termination.solver = testSolver();
    search.synthesis.solver = testSolver();
    SynthesisResult found = synthesizeProgram(p, ready.proof(), search, z3Backend);
    CHECK(!found.groups.empty() && !found.groups[0].candidates.empty());
    if (found.groups.empty() || found.groups[0].candidates.empty())
      return;
    const VerifiedCandidate &candidate = found.groups[0].candidates[0];
    CHECK(candidate.order.has_value());
    const EnumerationOrder *order = candidate.order ? &*candidate.order : nullptr;
    CheckedComputation finite = checkComputationFinite(p, ready.enumeration, 0, options);
    CheckedComputation proved =
        checkComputationSymbolic(p, ready.proof(), ready.enumeration.groups[0], options, z3Backend);
    CandidateComputation full = checkCandidateComputation(
        p, ready.proof(), ready.enumeration, 0, candidate.family, order, finite, proved, options, z3Backend);
    CHECK(full.status == S::Verified && full.tier == T::Symbolic);
    // With no order, the coordinates have no symbolic tier, and neither has the join.
    CandidateComputation unordered = checkCandidateComputation(
        p, ready.proof(), ready.enumeration, 0, candidate.family, nullptr, finite, proved, options, z3Backend);
    CHECK(unordered.status == S::Verified && unordered.tier == T::ScopeEnumerated);
    // A symbolic claim about another program joins nothing.
    SemanticProgram other = cat.import("integer");
    ParameterScope otherScope = range(other, 1, 2);
    Authorizations otherAll = everyPair(other);
    Prepared otherReady = prepare(other, otherScope, otherAll);
    CheckedComputation foreign = checkComputationSymbolic(
        other, otherReady.proof(), otherReady.enumeration.groups[0],
        computationOptions(otherScope, otherAll), z3Backend);
    CHECK(foreign.status() == S::Verified);
    CandidateComputation swappedClaim = checkCandidateComputation(
        p, ready.proof(), ready.enumeration, 0, candidate.family, order, finite, foreign, options, z3Backend);
    CHECK(swappedClaim.tier != T::Symbolic);
    // Coordinates at fewer points than the computation: the join has only those.
    Prepared sample = prepare(p, scope, all, {{APInt(16, 1)}});
    CandidateComputation narrow = checkCandidateComputation(
        p, ready.proof(), sample.enumeration, 0, candidate.family, nullptr, finite, proved, options, z3Backend);
    CHECK(narrow.status == S::Verified && narrow.tier == T::Point);
    CHECK(narrow.points.size() == 1);

    // Evidence for two equivalent mutants. "finite candidate unchecked": a family with no
    // verified tier has no points to join. "short key passes": the coordinate checker
    // adopts every term before any tier, so a term that cannot be adopted never gets one.
    DomainEnumeration closed = enumerateDomain(p, nullptr, EnumerationOptions(), z3Backend);
    CandidateComputation none = checkCandidateComputation(
        p, ready.proof(), closed, 0, candidate.family, nullptr, finite, proved, options, z3Backend);
    CHECK(none.status == S::Unknown && none.points.empty());
    CHECK(none.coordinateFinite.points().empty());
    CoordinateFamily unadoptable = candidate.family;
    unadoptable.axes[AxisI].push_back(p.sites[0].aValue);
    CandidateComputation refused = checkCandidateComputation(
        p, ready.proof(), ready.enumeration, 0, unadoptable, order, finite, proved, options, z3Backend);
    CHECK(refused.coordinateSymbolic.status() != FamilyStatus::Verified);
    CHECK(refused.symbolicInjective[0] == ViewInjectivity::NotProven);
  }
  {
    // Evidence for "operands not compared": every imported site stores its update, so
    // equal nodes for the stored value force equal operand nodes.
    Module cat(catalogue());
    for (const char *name : {"clean", "lost_update", "overlap", "chain", "repeated", "contract",
                             "integer", "overflow", "same_root", "collapsed", "equal_loads"}) {
      SemanticProgram p = cat.import(name);
      for (const MacSite &site : p.sites)
        for (const Transition &t : p.transitions)
          if (t.from == site.location)
            CHECK(t.effects[site.effectIndex].value == site.updateValue);
    }
  }
}

/// Short programs, each judged against plain arithmetic over every fill.
struct Statement {
  enum Kind { Site, Set, Copy, Lost } kind = Site;
  unsigned c = 0, p = 0, q = 0, value = 0;
  bool operandInOutput = false;
};

static std::string statementText(const Statement &s, unsigned n, const std::string &element,
                                 bool isFloat) {
  std::string id = std::to_string(n), type = "memref<2x" + element + ">";
  std::string add = isFloat ? "arith.addf" : "arith.addi", mul = isFloat ? "arith.mulf" : "arith.muli";
  auto index = [](unsigned v) { return "%i" + std::to_string(v); };
  auto load = [&](const std::string &name, const std::string &root, unsigned at) {
    return "  %" + name + id + " = memref.load %" + root + "[" + index(at) + "] : " + type + "\n";
  };
  switch (s.kind) {
  case Statement::Site:
    return load("x", s.operandInOutput ? "c" : "a", s.p) + load("y", "a", s.q) + load("z", "c", s.c) +
           "  %m" + id + " = " + mul + " %x" + id + ", %y" + id + " : " + element + "\n  %s" + id + " = " +
           add + " %z" + id + ", %m" + id + " : " + element + "\n  memref.store %s" + id + ", %c[" +
           index(s.c) + "] : " + type + "\n";
  case Statement::Set:
    return "  memref.store %v" + std::to_string(s.value) + ", %" + (s.operandInOutput ? "a" : "c") +
           "[" + index(s.c) + "] : " + type + "\n";
  case Statement::Copy:
    return load("x", "a", s.p) + "  memref.store %x" + id + ", %c[" + index(s.c) + "] : " + type + "\n";
  case Statement::Lost:
    return load("x", "a", s.p) + load("y", "a", s.q) + load("z", "c", s.c) + "  %m" + id + " = " + mul +
           " %x" + id + ", %y" + id + " : " + element + "\n  %s" + id + " = " + add + " %z" + id + ", %m" +
           id + " : " + element + "\n  memref.store %s" + id + ", %c[" + index(s.c) + "] : " + type +
           "\n  %t" + id + " = " + add + " %z" + id + ", %m" + id + " : " + element +
           "\n  memref.store %t" + id + ", %c[" + index(s.c) + "] : " + type + "\n";
  }
  return "";
}

/// Plain interpretation over C[2], A[2] with the program order, and the replay, for
/// one group. True when they differ at this fill. No library code.
template <typename Value>
static bool bruteDiffers(const std::vector<Statement> &program, bool groupInOutput,
                         const std::array<Value, 4> &fill, const std::array<Value, 2> &constants,
                         const std::function<bool(Value, Value)> &same) {
  Value C[2] = {fill[0], fill[1]}, A[2] = {fill[2], fill[3]};
  Value R[2] = {fill[0], fill[1]};
  bool touched[2] = {false, false}, differs = false;
  auto event = [&](const Statement &s, Value read, Value accumulator) -> Value {
    // The program's update, then the replay's next, from the initial operands.
    Value x0 = s.operandInOutput ? fill[s.p] : fill[2 + s.p], y0 = fill[2 + s.q];
    Value product = (Value)(read * A[s.q]);
    Value written = (Value)(accumulator + product);
    Value contribution = (Value)(x0 * y0);
    Value next = (Value)(R[s.c] + contribution);
    if (!same(read, x0) || !same(A[s.q], y0) || !same(written, next))
      differs = true;
    R[s.c] = next;
    touched[s.c] = true;
    return written;
  };
  for (const Statement &s : program) {
    switch (s.kind) {
    case Statement::Site: {
      Value read = s.operandInOutput ? C[s.p] : A[s.p];
      if (s.operandInOutput == groupInOutput) {
        C[s.c] = event(s, read, C[s.c]);
      } else {
        C[s.c] = (Value)(C[s.c] + (Value)(read * A[s.q]));
        if (!touched[s.c])
          R[s.c] = fill[s.c]; // Not a constant: the fold starts from the initial value.
      }
      break;
    }
    case Statement::Set:
      if (s.operandInOutput) {
        A[s.c] = constants[s.value];
      } else {
        C[s.c] = constants[s.value];
        if (!touched[s.c])
          R[s.c] = constants[s.value];
      }
      break;
    case Statement::Copy:
      C[s.c] = A[s.p];
      if (!touched[s.c])
        R[s.c] = fill[s.c];
      break;
    case Statement::Lost: {
      Value z = C[s.c], read = A[s.p];
      if (!groupInOutput) {
        C[s.c] = event(s, read, z);
        C[s.c] = event(s, read, z);
      } else {
        C[s.c] = (Value)(z + (Value)(read * A[s.q]));
        if (!touched[s.c])
          R[s.c] = fill[s.c];
      }
      break;
    }
    }
  }
  for (unsigned c = 0; c < 2; ++c)
    if (touched[c] && !same(C[c], R[c]))
      differs = true;
  return differs;
}

struct Exhaustive {
  uint64_t programs = 0, groups = 0, verifiedFinite = 0, verifiedSymbolic = 0, refuted = 0;
  uint64_t unknownEqual = 0, unknownDifferent = 0, falseVerified = 0, falseRefuted = 0;
  uint64_t fills = 0;
};

template <typename Value>
static void exhaust(const std::string &element, bool isFloat, unsigned length,
                    const std::vector<Value> &values, const std::array<Value, 2> &constants,
                    const std::array<std::string, 2> &constantText, unsigned stride,
                    const std::function<bool(Value, Value)> &same, Exhaustive &out) {
  std::vector<Statement> alphabet;
  for (unsigned c = 0; c < 2; ++c) {
    for (unsigned p = 0; p < 2; ++p)
      for (unsigned q = 0; q < 2; ++q)
        for (bool inOutput : {false, true}) {
          Statement s;
          s.kind = Statement::Site;
          s.c = c;
          s.p = p;
          s.q = q;
          s.operandInOutput = inOutput;
          alphabet.push_back(s);
        }
    for (unsigned value = 0; value < 2; ++value)
      for (bool inA : {false, true}) {
        Statement s;
        s.kind = Statement::Set;
        s.c = c;
        s.value = value;
        s.operandInOutput = inA;
        alphabet.push_back(s);
      }
    Statement copy;
    copy.kind = Statement::Copy;
    copy.c = c;
    copy.p = 1 - c;
    alphabet.push_back(copy);
    Statement lost;
    lost.kind = Statement::Lost;
    lost.c = c;
    alphabet.push_back(lost);
  }
  std::vector<unsigned> digits(length, 0);
  uint64_t serial = 0;
  auto advance = [&]() {
    unsigned d = 0;
    for (; d < length; ++d) {
      if (++digits[d] < alphabet.size())
        break;
      digits[d] = 0;
    }
    return d != length;
  };
  while (true) {
    if (++serial % stride) {
      if (!advance())
        break;
      continue;
    }
    std::vector<Statement> program;
    for (unsigned d : digits)
      program.push_back(alphabet[d]);
    std::string type = "memref<2x" + element + ">";
    std::string text = "func.func @f(%c: " + type + ", %a: " + type + ") {\n"
                       "  %i0 = arith.constant 0 : index\n  %i1 = arith.constant 1 : index\n"
                       "  %v0 = arith.constant " + constantText[0] + " : " + element + "\n"
                       "  %v1 = arith.constant " + constantText[1] + " : " + element + "\n";
    for (unsigned n = 0; n < program.size(); ++n)
      text += statementText(program[n], n, element, isFloat);
    text += "  return\n}\n";
    Module m(text);
    SemanticProgram p = m.import("f");
    CHECK(p.isSupported());
    ++out.programs;
    ParameterScope scope;
    Authorizations authorized = {"roots 0 and 1 are separate"};
    Prepared ready = prepare(p, scope, authorized);
    if (ready.enumeration.gate == GateStatus::Open) {
      ComputationOptions options = computationOptions(scope, authorized);
      for (unsigned g = 0; g < ready.enumeration.groups.size(); ++g) {
        const SiteGroup &group = ready.enumeration.groups[g];
        if (!group.unsupported.empty())
          continue;
        ++out.groups;
        bool groupInOutput = group.first == 0;
        CheckedComputation finite = checkComputationFinite(p, ready.enumeration, g, options);
        CheckedComputation symbolic =
            checkComputationSymbolic(p, ready.proof(), group, options, z3Backend);
        bool different = false;
        std::array<Value, 4> fill;
        for (Value a : values)
          for (Value b : values)
            for (Value c : values)
              for (Value d : values) {
                fill = {a, b, c, d};
                ++out.fills;
                different = different || bruteDiffers<Value>(program, groupInOutput, fill, constants, same);
              }
        if (finite.status() == S::Verified) {
          ++out.verifiedFinite;
          out.falseVerified += different;
        } else if (finite.status() == S::Refuted) {
          ++out.refuted;
          out.falseRefuted += !different;
          if (!different)
            printf("no difference over the fills, refuted: %s\n%s\n", finite.reason().c_str(), text.c_str());
        } else {
          (different ? out.unknownDifferent : out.unknownEqual) += 1;
        }
        if (symbolic.status() == S::Verified) {
          ++out.verifiedSymbolic;
          out.falseVerified += different;
        }
        CHECK(symbolic.status() != S::Refuted);
      }
    }
    if (!advance())
      break;
  }
}

static void testExhaustive() {
  Exhaustive integers, floats;
  std::function<bool(uint8_t, uint8_t)> sameInt = [](uint8_t a, uint8_t b) { return a == b; };
  std::function<bool(float, float)> sameFloat = [](float a, float b) {
    return llvm::bit_cast<uint32_t>(a) == llvm::bit_cast<uint32_t>(b);
  };
  // COMPUTATION_FULL widens the sweep for a validation run: every float program of
  // length 2, and one in 37 of length 3. CTest runs the part below.
  bool full = getenv("COMPUTATION_FULL") != nullptr;
  for (unsigned length : {1u, 2u})
    exhaust<uint8_t>("i8", false, length, {0, 1, 2, 3}, {0, 1}, {"0", "1"}, 1, sameInt, integers);
  if (full)
    exhaust<uint8_t>("i8", false, 3, {0, 1, 2, 3}, {0, 1}, {"0", "1"}, 37, sameInt, integers);
  exhaust<float>("f32", true, 1, {0.0f, -0.0f, 1.0f, 3.0f}, {0.0f, 1.0f}, {"0.0", "1.0"}, 1, sameFloat, floats);
  exhaust<float>("f32", true, 2, {0.0f, -0.0f, 1.0f, 3.0f}, {0.0f, 1.0f}, {"0.0", "1.0"}, full ? 1 : 16,
                 sameFloat, floats);
  for (const Exhaustive *e : {&integers, &floats}) {
    printf("exhaustive: %llu programs, %llu groups, %llu fills: %llu verified, %llu symbolic, "
           "%llu refuted, %llu unknown and equal, %llu unknown and different\n",
           (unsigned long long)e->programs, (unsigned long long)e->groups,
           (unsigned long long)e->fills, (unsigned long long)e->verifiedFinite,
           (unsigned long long)e->verifiedSymbolic, (unsigned long long)e->refuted,
           (unsigned long long)e->unknownEqual, (unsigned long long)e->unknownDifferent);
    CHECK(e->falseVerified == 0);
    CHECK(e->falseRefuted == 0);
    CHECK(e->verifiedFinite > 0 && e->verifiedSymbolic > 0 && e->refuted > 0);
  }
}

static void testCandidates() {
  Module m(catalogue());
  auto run = [&](const char *name, bool withAuthorization = true) {
    SemanticProgram p = m.import(name);
    ParameterScope scope = range(p, 1, 2);
    Authorizations authorized = withAuthorization ? everyPair(p) : Authorizations();
    Prepared ready = prepare(p, scope, authorized);
    ComputationRequest request;
    request.synthesis.scope = scope;
    request.synthesis.authorized = authorized;
    request.synthesis.maxLevel = 0;
    request.synthesis.synthesis.maxCandidates = 5;
    request.synthesis.synthesis.solver = testSolver();
    request.synthesis.termination.solver = testSolver();
    request.computation = computationOptions(scope, authorized);
    ComputationResult result = analyzeComputation(p, ready.proof(), request, z3Backend);
    return std::make_pair(std::move(result), std::move(p));
  };
  {
    auto [result, p] = run("clean");
    CHECK(result.groups.size() == 1 && !result.groups[0].candidates.empty());
    for (const CandidateComputation &c : result.groups[0].candidates) {
      CHECK(c.status == S::Verified && c.tier == T::Symbolic);
      CHECK(c.coordinateSymbolic.tier() >= EvidenceTier::SymbolicValid);
    }
    std::string json = toJSON(p, result);
    CHECK(json.find("\"authorizes_lowering\": true") == std::string::npos);
    CHECK(json.find("\"authorizes_lowering\": false") != std::string::npos);
    CHECK(json.find("\"domain_and_order\": \"UNKNOWN\"") != std::string::npos);
    CHECK(json.find("\"transformation\": \"UNKNOWN\"") != std::string::npos);
    CHECK(json.find("\"name\": \"roots 0 and 2 are separate\"") != std::string::npos);
  }
  {
    // A finite computation claim holds a candidate at a finite tier.
    auto [result, p] = run("prefix");
    CHECK(!result.groups[0].candidates.empty());
    for (const CandidateComputation &c : result.groups[0].candidates) {
      CHECK(c.status == S::Verified && c.tier == T::ScopeEnumerated);
      CHECK(c.points.size() == 2);
    }
  }
  {
    // A refuted relation refutes each candidate of the group, and only that claim.
    auto [result, p] = run("overwritten");
    CHECK(result.groups[0].finite.status() == S::Refuted);
    for (const CandidateComputation &c : result.groups[0].candidates)
      CHECK(c.status == S::Refuted && c.tier == T::Proposed);
  }
  {
    // No authorization: coordinates stand and the computation claim does not.
    auto [result, p] = run("clean", false);
    CHECK(result.groups[0].finite.status() == S::Unknown);
    for (const CandidateComputation &c : result.groups[0].candidates) {
      CHECK(c.status == S::Unknown && c.tier == T::Proposed);
      CHECK(c.coordinateFinite.status() == FamilyStatus::Verified);
    }
  }
  {
    // A view that collapses is a fact and nothing worse.
    auto [result, p] = run("collapsed");
    CHECK(!result.groups[0].candidates.empty());
    bool collapsed = false;
    for (const CandidateComputation &c : result.groups[0].candidates) {
      CHECK(c.status == S::Verified);
      collapsed = collapsed || c.symbolicInjective[0] == ViewInjectivity::NotProven;
    }
    CHECK(collapsed);
  }
  {
    // A forged family is judged on its own. Nothing of the search is carried over.
    auto [result, p] = run("clean");
    Term n = p.terms->param(p.parameters[0].id, p.parameters[0].sort);
    CoordinateFamily forged;
    forged.origin = "a forgery";
    forged.axes[AxisI] = {n};
    ParameterScope scope = range(p, 1, 2);
    Authorizations authorized = everyPair(p);
    Prepared ready = prepare(p, scope, authorized);
    ComputationOptions options = computationOptions(scope, authorized);
    CandidateComputation out = checkCandidateComputation(
        p, ready.proof(), ready.enumeration, 0, forged, nullptr, result.groups[0].finite,
        result.groups[0].symbolic, options, z3Backend);
    CHECK(out.status != S::Verified);

    // A good family with claims about something else counts for nothing.
    auto [otherResult, otherProgram] = run("integer");
    (void)otherProgram;
    const CoordinateFamily &family = result.groups[0].candidates[0].family;
    CandidateComputation swapped = checkCandidateComputation(
        p, ready.proof(), ready.enumeration, 0, family, nullptr, otherResult.groups[0].finite,
        otherResult.groups[0].symbolic, options, z3Backend);
    CHECK(swapped.status != S::Verified);
    // The same claims, read under a wider scope or fewer names, are not those claims.
    ComputationOptions wider = computationOptions(range(p, 1, 3), authorized);
    CHECK(checkCandidateComputation(p, ready.proof(), ready.enumeration, 0, family, nullptr,
                                    result.groups[0].finite, result.groups[0].symbolic, wider,
                                    z3Backend).status != S::Verified);
    ComputationOptions named = computationOptions(scope, {"roots 0 and 2 are separate"});
    CHECK(checkCandidateComputation(p, ready.proof(), ready.enumeration, 0, family, nullptr,
                                    result.groups[0].finite, result.groups[0].symbolic, named,
                                    z3Backend).status != S::Verified);
    // And the same inputs, unchanged, do join: the refusals above are about the binding.
    CandidateComputation same = checkCandidateComputation(
        p, ready.proof(), ready.enumeration, 0, family, nullptr, result.groups[0].finite,
        result.groups[0].symbolic, options, z3Backend);
    CHECK(same.status == S::Verified);
  }
}

static void testReach() {
  Module m(catalogue());
  SemanticProgram p = m.import("foreign_same_root");
  ParameterScope scope = range(p, 1, 2);
  Authorizations all = everyPair(p);
  Prepared ready = prepare(p, scope, all);
  std::string why;
  Term formula = scopeFormula(p, scope, why);
  std::vector<unsigned> sites{0};
  SiteArrivals bySite = reachSites(p, {}, ready.certificate->cuts, sites, formula, PathLimits());
  SiteArrivals byPlace = reachLocations(p, {}, ready.certificate->cuts, {p.sites[0].location},
                                        formula, PathLimits());
  CHECK(bySite.complete && byPlace.complete);
  CHECK(bySite.arrivals.size() == byPlace.arrivals.size());
  for (unsigned n = 0; n < bySite.arrivals.size() && n < byPlace.arrivals.size(); ++n) {
    CHECK(bySite.arrivals[n].location == byPlace.arrivals[n].location);
    CHECK(bySite.arrivals[n].premise == byPlace.arrivals[n].premise);
  }
}

int main(int argc, char **argv) {
  setvbuf(stdout, nullptr, _IOLBF, 0);
  mlir::MLIRContext local;
  configure(local);
  context = &local;
  std::string only = argc > 1 ? argv[1] : "";
  auto run = [&](const char *name, void (*test)()) {
    if (only.empty() || only == name) {
      auto begin = std::chrono::steady_clock::now();
      test();
      printf("%s: %.1f s, %d checks, %d failures\n", name,
             std::chrono::duration<double>(std::chrono::steady_clock::now() - begin).count(),
             checks, failures);
    }
  };
  run("claims", testClaims);
  run("separation", testSeparation);
  run("limits", testSolverLimits);
  run("tampered", testTampered);
  run("gaps", testGaps);
  run("gaps2", testGapsTwo);
  run("float", testFloatSemantics);
  run("candidates", testCandidates);
  run("reach", testReach);
  run("exhaustive", testExhaustive);
  printf("%d checks, %d failures\n", checks, failures);
  return failures ? 1 : 0;
}
