//===- TerminationTest.cpp - G2a checker, proposers and interface ---------===//
//
// argv[1], argv[2]: the loop catalogue at i4 and i8, from gen_loops.py.
//
// checkProofOnRuns evaluates each obligation formula with G1's evaluator on
// every configuration of real runs. It uses no solver, so an error in a formula
// or in the path composition cannot hide behind Z3's answer.
//
//===----------------------------------------------------------------------===//
#include "TestSupport.h"

#include "GeneralizedGemm/ControlStructure.h"
#include "GeneralizedGemm/CutPaths.h"
#include "GeneralizedGemm/RelationalEncoding.h"
#include "GeneralizedGemm/TerminationAnalysis.h"
#include "GeneralizedGemm/TerminationObligations.h"
#include "GeneralizedGemm/Z3Backend.h"

#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MemoryBuffer.h"

#include <functional>
#include <map>

using namespace gg;
using V = TerminationVerdict;

static mlir::MLIRContext *context;

struct Module {
  mlir::OwningOpRef<mlir::ModuleOp> module;
  explicit Module(const std::string &text)
      : module(mlir::parseSourceString<mlir::ModuleOp>(text, context)) {
    if (!module) {
      fprintf(stderr, "cannot parse:\n%s\n", text.c_str());
      exit(2);
    }
  }
  SemanticProgram import(const std::string &name, ImportOptions options = {}) {
    auto function = module->lookupSymbol<mlir::func::FuncOp>(name);
    if (!function) {
      fprintf(stderr, "no function %s\n", name.c_str());
      exit(2);
    }
    return importFunction(function, options);
  }
};

static std::string readFile(const char *path) {
  auto buffer = llvm::MemoryBuffer::getFile(path);
  if (!buffer) {
    fprintf(stderr, "cannot read %s\n", path);
    exit(2);
  }
  return (*buffer)->getBuffer().str();
}

static Z3Backend z3Backend;

static TerminationOptions fastOptions() {
  TerminationOptions options;
  options.limits.replayFuel = 1u << 14;
  return options;
}

static TerminationResult analyze(const SemanticProgram &p,
                                 TerminationOptions options = fastOptions(),
                                 SmtBackend *backend = nullptr) {
  return analyzeTermination(p, backend ? *backend : z3Backend, options);
}

static ParameterScope upTo(const SemanticProgram &p, int64_t upper) {
  ParameterScope scope;
  scope.allParameters = false;
  unsigned w = p.parameters[0].sort.width;
  scope.ranges.push_back({0, APInt::getSignedMinValue(w),
                          APInt(w, upper, /*isSigned=*/true)});
  return scope;
}

/// Every parameter assignment, last parameter fastest.
static void forEachPoint(const SemanticProgram &p,
                         const std::function<void(std::vector<APInt>)> &visit) {
  std::vector<APInt> point;
  std::function<void(unsigned)> go = [&](unsigned k) {
    if (k == p.parameters.size()) {
      visit(point);
      return;
    }
    unsigned w = p.parameters[k].sort.width;
    for (uint64_t v = 0; v < (uint64_t(1) << w); ++v) {
      point.push_back(APInt(w, v));
      go(k + 1);
      point.pop_back();
    }
  };
  go(0);
}

static EntryState entryOf(const SemanticProgram &p, std::vector<APInt> point) {
  EntryState entry;
  entry.parameters = std::move(point);
  for (const MemoryRoot &root : p.roots) {
    RootData data;
    uint64_t cells = 1;
    for (int64_t extent : root.extents) {
      data.shape.push_back(extent);
      cells *= extent;
    }
    for (uint64_t c = 0; c < cells; ++c)
      data.cells.emplace_back(APInt(root.sort.width, c % 2));
    entry.memory.push_back(std::move(data));
  }
  return entry;
}

static bool evaluatesFalse(const SemanticProgram &p, const EntryState &entry,
                           const Configuration &config, Term formula) {
  auto value = evaluateTerm(p, entry, config, formula);
  return !value || value->isZero(); // Undefined state is universally quantified.
}

/// All paths of a proof node, and of its children, keyed by node.
static void forEachLevel(const std::vector<RankLevel> &levels,
                         const std::function<void(const RankLevel &)> &visit) {
  for (const RankLevel &level : levels) {
    visit(level);
    forEachLevel(level.children, visit);
  }
}

/// The G1/G2a interface and every formula of a proof, on one real run.
static unsigned checkProofOnRun(const SemanticProgram &p, const PathSet &set,
                                const TerminationCertificate *certificate,
                                const EntryState &entry, unsigned fuel) {
  TermContext &terms = *p.terms;
  unsigned segments = 0;
  std::string error;
  auto config = initialConfiguration(p, entry, RunLimits{}, error);
  CHECK(config.has_value());
  std::set<unsigned> cuts;
  for (const CutPath &path : set.paths)
    if (!path.fromEntry)
      cuts.insert(path.from);
  std::map<unsigned, Term> invariants;
  if (certificate)
    for (const auto &[cut, term] : certificate->invariants)
      invariants[cut] = term;
  Term scope = terms.boolean(true);

  Configuration source = *config;
  llvm::SmallVector<unsigned> taken;
  std::vector<Configuration> seen;
  for (unsigned step = 0; step < fuel; ++step) {
    if (config->location == p.exitLocation())
      break;
    bool repeated = false;
    for (const Configuration &old : seen)
      repeated = repeated || sameConfiguration(old, *config);
    if (repeated)
      break;
    seen.push_back(*config);
    StepOutcome outcome = stepConfiguration(p, entry, *config);
    if (outcome.stopped)
      break;
    taken.push_back(outcome.transition);
    if (config->location != p.exitLocation() && !cuts.count(config->location))
      continue;
    // A segment ended. Its path must exist, be enabled, and compute the state.
    const CutPath *path = nullptr;
    for (const CutPath &candidate : set.paths)
      if (candidate.transitions == taken)
        path = &candidate;
    CHECK(path != nullptr);
    if (!path)
      return segments;
    ++segments;
    auto holds = evaluateTerm(p, entry, source, path->condition);
    CHECK(holds && holds->isOne());
    CHECK(path->to == config->location);
    for (const StateVar &s : p.state) {
      if (config->state[s.id]) {
        auto value = evaluateTerm(p, entry, source,
                                  postTerm(p, *path, terms.state(s.id, s.sort)));
        CHECK(value && *value == *config->state[s.id]);
      }
      auto shadow = evaluateTerm(p, entry, source,
                                 postTerm(p, *path, terms.shadow(s.id)));
      CHECK(!shadow || shadow->isOne() == config->shadow[s.id]);
    }
    if (certificate) {
      Term start = path->fromEntry
                       ? terms.boolean(true)
                       : (invariants.count(path->from) ? invariants[path->from]
                                                       : terms.boolean(true));
      if (path->to != p.exitLocation() && invariants.count(path->to))
        CHECK(evaluatesFalse(p, entry, source,
                             invariantViolation(p, scope, *path, start,
                                                invariants[path->to])));
      forEachLevel(certificate->levels, [&](const RankLevel &level) {
        std::vector<unsigned> inside;
        std::set<unsigned> strict;
        for (const CutPath &candidate : set.paths) {
          if (llvm::is_contained(level.paths, candidate.transitions))
            inside.push_back(candidate.id);
          if (llvm::is_contained(level.strict, candidate.transitions))
            strict.insert(candidate.id);
        }
        if (!llvm::is_contained(inside, path->id))
          return;
        std::map<unsigned, Term> rank(level.rank.begin(), level.rank.end());
        CHECK(evaluatesFalse(
            p, entry, source,
            descentViolation(p, set, scope, *path, start, inside,
                             rank.at(path->from), rank.at(path->to),
                             strict.count(path->id) != 0)));
      });
    }
    source = *config;
    taken.clear();
  }
  return segments;
}

//===----------------------------------------------------------------------===//
// Graphs
//===----------------------------------------------------------------------===//

static const char *graphs = R"(
func.func @acyclic(%c: i1) {
  cf.cond_br %c, ^a, ^b
^a:
  cf.br ^b
^b:
  return
}
func.func @self(%c: i1) {
  cf.br ^h
^h:
  cf.cond_br %c, ^h, ^x
^x:
  return
}
func.func @latches(%c: i1, %d: i1) {
  cf.br ^h
^h:
  cf.cond_br %c, ^a, ^x
^a:
  cf.cond_br %d, ^h, ^b
^b:
  cf.br ^h
^x:
  return
}
func.func @irreducible(%c: i1, %d: i1) {
  cf.cond_br %c, ^a, ^b
^a:
  cf.cond_br %d, ^b, ^x
^b:
  cf.br ^a
^x:
  return
}
func.func @parallel(%c: i1, %n: i8) -> i8 {
  %z = arith.constant 0 : i8
  %o = arith.constant 1 : i8
  cf.cond_br %c, ^j(%z : i8), ^j(%o : i8)
^j(%v: i8):
  return %v : i8
}
func.func @switcher(%n: i8) -> i8 {
  %o = arith.constant 1 : i8
  cf.br ^h(%n : i8)
^h(%x: i8):
  cf.switch %x : i8, [default: ^d, 0: ^a(%o : i8), 5: ^h(%o : i8)]
^d:
  %y = arith.subi %x, %o : i8
  cf.br ^h(%y : i8)
^a(%v: i8):
  return %v : i8
}
)";

static void testGraphs() {
  // Components: self edges count, a chain does not, sorted output.
  CHECK(cyclicComponents(3, {{0, 1}, {1, 2}}).empty());
  auto self = cyclicComponents(3, {{0, 1}, {1, 1}});
  CHECK(self.size() == 1 && self[0].size() == 1 && self[0][0] == 1);
  auto two = cyclicComponents(5, {{3, 4}, {4, 3}, {0, 1}, {1, 0}, {1, 3}});
  CHECK(two.size() == 2 && two[0][0] == 0 && two[1][0] == 3);

  Module m(graphs);
  std::string why;
  for (const char *name : {"acyclic", "self", "latches", "irreducible", "parallel",
                           "switcher"}) {
    SemanticProgram p = m.import(name);
    CHECK(p.isSupported());
    auto cuts = computeCutSet(p);
    CHECK(cutSetBreaksCycles(p, cuts, why));
    PathSet set = enumerateCutPaths(p, cuts);
    CHECK(set.complete);
    // Paths of the same source have distinct transition lists.
    std::set<llvm::SmallVector<unsigned>> distinct;
    for (const CutPath &path : set.paths)
      CHECK(distinct.insert(path.transitions).second);
  }
  SemanticProgram acyclic = m.import("acyclic");
  CHECK(computeCutSet(acyclic).empty());
  CHECK(enumerateCutPaths(acyclic, {}).paths.size() == 2);

  // Parallel edges with different operands are separate paths.
  SemanticProgram parallel = m.import("parallel");
  CHECK(enumerateCutPaths(parallel, {}).paths.size() == 2);

  // The checker rejects cut sets that are wrong in every listed way.
  SemanticProgram latches = m.import("latches");
  CHECK(!cutSetBreaksCycles(latches, {}, why));
  auto good = computeCutSet(latches);
  CHECK(good.size() == 1);
  CHECK(!cutSetBreaksCycles(latches, {latches.entryLocation}, why));
  // The same refusal when the rest of the set would break every cycle.
  llvm::SmallVector<unsigned> withEntry{latches.entryLocation, good[0]};
  llvm::sort(withEntry);
  CHECK(!cutSetBreaksCycles(latches, withEntry, why));
  CHECK(why.find("entry") != std::string::npos);
  CHECK(!cutSetBreaksCycles(latches, {good[0], good[0]}, why));
  CHECK(!cutSetBreaksCycles(latches, {latches.locationCount}, why));
  SemanticProgram irreducible = m.import("irreducible");
  auto twoEntry = computeCutSet(irreducible);
  CHECK(!twoEntry.empty());
  CHECK(cutSetBreaksCycles(irreducible, twoEntry, why));
  // Each single location of the 2-cycle breaks it; removing all does not.
  CHECK(!cutSetBreaksCycles(irreducible, {}, why));

  // Limits give nothing at all, never a partial list.
  SemanticProgram sw = m.import("switcher");
  PathLimits tight;
  tight.maxPaths = 1;
  PathSet cut = enumerateCutPaths(sw, computeCutSet(sw), tight);
  CHECK(!cut.complete && cut.paths.empty() && cut.sites.empty());
}

//===----------------------------------------------------------------------===//
// Solver boundary
//===----------------------------------------------------------------------===//

static void testScripts() {
  std::string why;
  CHECK(isPlainScript("(assert true)\n(check-sat)\n", why));
  CHECK(isPlainScript("; note (with parens\n(declare-const a (_ BitVec 2))\n(define-fun b () (_ BitVec 2) a)\n(check-sat)", why));
  CHECK(!isPlainScript("(assert true)", why));
  CHECK(!isPlainScript("(check-sat)(check-sat)", why));
  CHECK(!isPlainScript("(check-sat)(assert false)", why));
  CHECK(!isPlainScript("(assert true", why));
  CHECK(!isPlainScript("(push)(check-sat)", why));
  CHECK(!isPlainScript("(set-option :timeout 1)(check-sat)", why));
  CHECK(!isPlainScript("(get-model)(check-sat)", why));
  CHECK(!isPlainScript("assert (check-sat)", why));
  CHECK(!isPlainScript("(assert |odd name|)(check-sat)", why));
  CHECK(!isPlainScript("(assert true ; hidden )\n)(check-sat)", why));
  CHECK(!isPlainScript("", why));
}

static void testBackend() {
  SmtLimits limits;
  auto answer = [&](const std::string &script, SmtLimits l) {
    return z3Backend.check(script, l, true).answer;
  };
  CHECK(answer("(assert true)(check-sat)", limits) == SmtAnswer::Sat);
  CHECK(answer("(assert false)(check-sat)", limits) == SmtAnswer::Unsat);

  // Errors come back as Unknown and the process survives. The `false` first
  // would give unsat if a bad command were skipped.
  CHECK(answer("(assert false)(assert undefined_name)(check-sat)", limits) == SmtAnswer::Unknown);
  CHECK(answer("(assert (bvadd 1))(check-sat)", limits) == SmtAnswer::Unknown);
  CHECK(answer("(declare-const a (_ BitVec 0))(check-sat)", limits) == SmtAnswer::Unknown);
  CHECK(answer("(declare-const a Int)(declare-const a Int)(assert false)(check-sat)", limits) == SmtAnswer::Unknown);
  CHECK(answer("(assert false)(push)(check-sat)", limits) == SmtAnswer::Unknown);
  CHECK(answer("(assert false)", limits) == SmtAnswer::Unknown);

  // A hard query under real limits. i*i overflow at 64 bits does not finish.
  std::string hard =
      "(declare-const a (_ BitVec 64))(declare-const b (_ BitVec 64))"
      "(assert (= (bvmul a b) (_ bv1000000007000000063 64)))"
      "(assert (bvugt a (_ bv1 64)))(assert (bvugt b (_ bv1 64)))"
      "(assert (bvult a (_ bv4294967296 64)))(assert (bvult b (_ bv4294967296 64)))"
      "(assert (= (bvurem a (_ bv7 64)) (_ bv3 64)))(check-sat)";
  SmtLimits tiny;
  tiny.resourceLimit = 1;
  CHECK(answer(hard, tiny) == SmtAnswer::Unknown);
  SmtLimits instant;
  instant.timeoutMilliseconds = 1;
  CHECK(answer(hard, instant) != SmtAnswer::Unsat);

  SmtResponse model = z3Backend.check(
      "(declare-const a (_ BitVec 8))(assert (= a (_ bv200 8)))(declare-const b Int)"
      "(assert (= b (- 3)))(check-sat)", limits, true);
  CHECK(model.answer == SmtAnswer::Sat && model.model["a"] == "200" && model.model["b"] == "-3");
  CHECK(z3Backend.version().rfind("z3 ", 0) == 0);
}

/// Answers from a function of the script and the query number.
struct ScriptedBackend : SmtBackend {
  std::function<std::optional<SmtAnswer>(const std::string &, unsigned)> rule;
  unsigned count = 0;
  SmtResponse check(const std::string &script, const SmtLimits &limits,
                    bool wantModel) override {
    auto forced = rule(script, count++);
    if (!forced)
      return z3Backend.check(script, limits, wantModel);
    SmtResponse response;
    response.answer = *forced;
    response.detail = "forced";
    return response;
  }
  std::string version() const override { return "scripted"; }
};

//===----------------------------------------------------------------------===//
// Verdicts on the catalogue
//===----------------------------------------------------------------------===//

struct Expected {
  const char *name;
  V verdict;
};

static void testCatalogue(const std::string &text) {
  Module m(text);
  const V PT = V::ProvenTerminating, PD = V::ProvenTerminatingIfDefined,
          RN = V::RefutedNonterminating, UK = V::Unknown;
  // i8. This is the required coverage: a change here must be deliberate. Every
  // entry was first compared with exhaustive exploration.
  const Expected table[] = {
      {"a_wrap", PT}, {"a_nsw", PT}, {"a_le_wrap", RN}, {"a_le_nsw", PD},
      {"b3_wrap", PT}, {"b3_nsw", PD}, {"b2_wrap", RN}, {"b2_nsw", PD},
      {"c_wrap", UK}, {"c_nsw", PD}, {"d_dec", PT}, {"d_dec_ge", PT},
      {"d_dec3", PT}, {"d_dec3_nsw", PT}, {"g_u3", PT}, {"g_u2", RN},
      {"g_ule", RN}, {"g_uge0", RN}, {"g_dec3", PT}, {"g_ne2", RN},
      {"g_ne1", PT}, {"g_nuw", PD}, {"h_even", RN}, {"h_true", RN},
      {"h_dbl_wrap", RN}, {"j_dbl_nsw", PD}, {"j_sq_nsw", PD}, {"j_halve", PT},
      {"j_step_wrap", UK}, {"j_step_nsw", PD}, {"j_collatz", RN}, {"e_rect", PT},
      {"e_tri", PT}, {"e_lex", PT}, {"t3_swap", PT}, {"t3_two_step", PT},
      {"t3_low", PT}, {"f_div", PD},
      {"f_dead_poison", PT}, {"i_external", UK}, {"i_inlined", PT},
      {"i_recursive", UK}, {"m_two_exits", PT}, {"m_switch", PT},
      {"s_step", RN}, {"s_bounds", PT}, {"m_irreducible", PT},
  };
  for (const Expected &row : table) {
    SemanticProgram p = m.import(row.name);
    TerminationResult result = analyze(p);
    if (result.verdict.kind() != row.verdict)
      fprintf(stderr, "%s: got %s: %s\n", row.name,
              stringifyTerminationVerdict(result.verdict.kind()),
              result.verdict.reason().c_str());
    CHECK(result.verdict.kind() == row.verdict);
    CHECK(result.conflict.empty());
    // A proof has a certificate, a refutation a witness, and nothing else.
    bool proven = row.verdict == PT || row.verdict == PD;
    CHECK(result.certificate.has_value() == proven);
    CHECK(result.witness.has_value() == (row.verdict == RN));
    if (row.verdict == UK)
      CHECK(!result.verdict.reason().empty());
    if (proven) {
      CHECK(!result.records.empty());
      for (const ObligationRecord &record : result.records) {
        CHECK(!record.id.empty() && !record.location.empty() && !record.result.empty());
        if (record.id != "O0")
          CHECK(!record.script.empty());
        if (record.id != "O7")
          CHECK(record.discharged);
      }
    }
  }

  // Which family gave the rank. With T1 taken out, T2 proves most of T1's rows, and the
  // least coefficients, the last of the chain, prove the two it does not (b2_nsw and e_tri,
  // which were UNKNOWN without T1 before that search, with Z3 4.13.3): the table above is
  // passed all the same. So the verdicts do not show that T1 is tried, and the origin of
  // the ranks does. T1 comes first and its candidates are fixed terms that the checker
  // judges: no model of the solver decides any of this.
  for (const char *name : {"a_nsw", "b2_nsw", "e_tri", "g_ne1", "j_halve"}) {
    SemanticProgram p = m.import(name);
    TerminationResult result = analyze(p);
    CHECK(result.certificate.has_value());
    if (!result.certificate)
      continue;
    CHECK(!result.certificate->levels.empty());
    forEachLevel(result.certificate->levels, [&](const RankLevel &level) {
      if (level.origin != "T1 guard distance")
        fprintf(stderr, "%s: a rank from %s\n", name, level.origin.c_str());
      CHECK(level.origin == "T1 guard distance");
    });
  }

  // The same source means different things under wrap and nsw.
  CHECK(analyze(m.import("b3_wrap")).verdict.kind() !=
        analyze(m.import("b3_nsw")).verdict.kind());

  // Reachable UB is reported with a replayed entry at the exact boundary.
  TerminationResult b3 = analyze(m.import("b3_nsw"));
  const StopReport &ub = b3.stops[(unsigned)SideKind::UndefinedBehavior];
  CHECK(ub.status == StopStatus::Reachable);
  CHECK(ub.witness && ub.witness->parameters[0] == APInt(8, 127));

  // Scope: exact boundaries of the two loops.
  {
    SemanticProgram p = m.import("b3_nsw");
    auto options = fastOptions();
    options.scope = upTo(p, 126);
    CHECK(analyze(p, options).verdict.kind() == PT);
    SemanticProgram q = m.import("c_wrap");
    options.scope = upTo(q, 121);
    CHECK(analyze(q, options).verdict.kind() == PT);
    // One further and wrap is possible: the proof must not survive.
    options.scope = upTo(q, 122);
    CHECK(analyze(q, options).verdict.kind() == UK);
    // A scope that hides the failing entry changes a refutation into a proof,
    // and it is the scope, not the proof, that says so.
    SemanticProgram r = m.import("a_le_wrap");
    options.scope = upTo(r, 126);
    CHECK(analyze(r, options).verdict.kind() == PT);
  }
  // A symbolic step: zero spins, and a positive scope removes exactly that.
  {
    SemanticProgram p = m.import("s_step");
    auto options = fastOptions();
    options.scope.allParameters = false;
    options.scope.ranges.push_back({1, APInt(8, 1), APInt(8, 127)});
    CHECK(analyze(p, options).verdict.kind() == PD);
  }
  // An empty domain proves nothing.
  {
    SemanticProgram p = m.import("a_wrap");
    auto options = fastOptions();
    options.scope.allParameters = false;
    options.scope.ranges.push_back({0, APInt(8, 5), APInt(8, 1)});
    CHECK(analyze(p, options).verdict.kind() == UK);
    // Malformed scopes are refused.
    options.scope.ranges = {{0, APInt(16, 0), APInt(16, 1)}};
    CHECK(analyze(p, options).verdict.kind() == UK);
    options.scope.ranges = {{9, APInt(8, 0), APInt(8, 1)}};
    CHECK(analyze(p, options).verdict.kind() == UK);
    options.scope.allParameters = true;
    options.scope.ranges = {{0, APInt(8, 0), APInt(8, 1)}};
    CHECK(analyze(p, options).verdict.kind() == UK);
  }
}

/// Exhaustive at i4 with G1's own runs: the verdict against every entry, and
/// each formula of each proof on each run.
static void testExhaustive(const std::string &text) {
  Module m(text);
  unsigned proofs = 0, entries = 0, segments = 0;
  for (auto function : m.module->getOps<mlir::func::FuncOp>()) {
    if (function.isExternal())
      continue;
    SemanticProgram p = importFunction(function, ImportOptions{});
    if (!p.isSupported())
      continue;
    TerminationResult result = analyze(p);
    CHECK(result.conflict.empty());
    V verdict = result.verdict.kind();
    PathSet set;
    if (result.certificate) {
      ++proofs;
      set = enumerateCutPaths(p, result.certificate->cuts);
    } else {
      set = enumerateCutPaths(p, computeCutSet(p));
    }
    CHECK(set.complete);
    bool anyDiverges = false;
    forEachPoint(p, [&](std::vector<APInt> point) {
      EntryState entry = entryOf(p, std::move(point));
      RunLimits limits;
      limits.fuel = 200000;
      limits.maxRecordedConfigurations = 200000;
      RunResult run = runConcrete(p, entry, limits);
      ++entries;
      CHECK(run.status != RunStatus::OutOfFuel);
      if (verdict == V::ProvenTerminating)
        CHECK(run.status == RunStatus::Returned);
      if (verdict == V::ProvenTerminatingIfDefined)
        CHECK(run.status != RunStatus::Diverged);
      anyDiverges = anyDiverges || run.status == RunStatus::Diverged;
      for (const StopReport &report : result.stops)
        if (report.status == StopStatus::Unreachable && result.certificate) {
          bool hit =
              (report.kind == SideKind::UndefinedBehavior &&
               run.status == RunStatus::UndefinedBehavior) ||
              (report.kind == SideKind::Underspecified &&
               run.status == RunStatus::Underspecified) ||
              (report.kind == SideKind::ModelLimit &&
               run.status == RunStatus::ModelLimit);
          CHECK(!hit);
        }
      segments += checkProofOnRun(p, set,
                                  result.certificate ? &*result.certificate : nullptr,
                                  entry, 400);
    });
    if (verdict == V::RefutedNonterminating)
      CHECK(anyDiverges);
  }
  CHECK(proofs >= 15);
  fprintf(stderr, "exhaustive: %u proofs, %u entries, %u path segments\n", proofs,
          entries, segments);
}

//===----------------------------------------------------------------------===//
// The checker refuses bad certificates
//===----------------------------------------------------------------------===//

static bool isProven(const CheckOutcome &outcome) {
  return outcome.verdict.kind() == V::ProvenTerminating ||
         outcome.verdict.kind() == V::ProvenTerminatingIfDefined;
}

static void testCorruption(const std::string &text) {
  Module m(text);
  TermContext foreign;
  auto check = [&](const SemanticProgram &p, const TerminationCertificate &c) {
    return checkCertificate(p, c, CheckOptions{}, z3Backend);
  };
  for (const char *name : {"e_rect", "c_nsw", "b3_wrap", "e_lex", "e_tri"}) {
    SemanticProgram p = m.import(name);
    TerminationResult result = analyze(p);
    CHECK(result.certificate.has_value());
    if (!result.certificate)
      continue;
    const TerminationCertificate good = *result.certificate;
    CHECK(isProven(check(p, good)));
    TermContext &terms = *p.terms;
    Term no = terms.boolean(false), yes = terms.boolean(true);
    auto refused = [&](auto mutate) {
      TerminationCertificate bad = good;
      mutate(bad);
      CheckOutcome outcome = check(p, bad);
      CHECK(!isProven(outcome));
      CHECK(outcome.verdict.kind() == V::Unknown);
    };

    refused([&](auto &c) { c.fingerprint = "other"; });
    refused([&](auto &c) { c.fingerprint.clear(); });
    refused([&](auto &c) { c.cuts.clear(); });
    refused([&](auto &c) { c.cuts.insert(c.cuts.begin(), c.cuts[0]); });
    refused([&](auto &c) { c.cuts.push_back(p.locationCount); });
    refused([&](auto &c) { c.levels.clear(); });
    refused([&](auto &c) { c.levels.push_back(c.levels[0]); });
    refused([&](auto &c) { c.levels[0].cuts.clear(); });
    refused([&](auto &c) { c.levels[0].rank.clear(); });
    refused([&](auto &c) { c.levels[0].strict.clear(); });
    refused([&](auto &c) { c.levels[0].strict.push_back({99999}); });
    refused([&](auto &c) { c.levels[0].strict.push_back(c.levels[0].strict[0]); });
    refused([&](auto &c) { c.levels[0].paths.pop_back(); });
    refused([&](auto &c) { c.levels[0].paths.push_back({99999}); });
    refused([&](auto &c) { c.levels[0].paths.push_back(c.levels[0].paths[0]); });
    refused([&](auto &c) { c.levels[0].rank[0].second = nullptr; });
    // A constant rank cannot decrease.
    refused([&](auto &c) {
      unsigned w = c.levels[0].rank[0].second->sort.width;
      for (auto &rank : c.levels[0].rank)
        rank.second = terms.bv(APInt(w, 7));
    });
    // The reversed rank increases.
    refused([&](auto &c) {
      for (auto &rank : c.levels[0].rank)
        rank.second = terms.logicalNot(rank.second->sort.width == 1
                                           ? rank.second
                                           : terms.icmp(Predicate::Eq, rank.second, rank.second));
    });
    refused([&](auto &c) {
      for (auto &rank : c.levels[0].rank)
        rank.second = terms.binary(
            TermOp::Sub, terms.bv(APInt::getZero(rank.second->sort.width)),
            rank.second);
    });
    refused([&](auto &c) {
      // Mixed widths inside one SCC or a width the other cuts lack.
      c.levels[0].rank[0].second = terms.boolean(true);
      if (c.levels[0].rank.size() == 1)
        c.levels[0].rank[0].second = terms.bv(APInt(3, 1));
    });
    // Terms built in another context are rejected or rebuilt, never mixed.
    refused([&](auto &c) {
      unsigned w = c.levels[0].rank[0].second->sort.width;
      for (auto &rank : c.levels[0].rank)
        rank.second = foreign.bv(APInt(w, 1));
    });
    // Leaves must exist with their sort, and may not read memory or floats.
    refused([&](auto &c) { c.levels[0].rank[0].second = terms.state(9999, Sort::bv(8)); });
    refused([&](auto &c) { c.levels[0].rank[0].second = terms.param(9999, Sort::bv(8)); });
    refused([&](auto &c) { c.levels[0].rank[0].second = terms.shadow(9999); });
    refused([&](auto &c) { c.levels[0].rank[0].second = terms.fconst(APInt(32, 0)); });
    // Invariants: false fails initiation, and a true-looking lie fails too.
    refused([&](auto &c) { c.invariants = {{good.cuts[0], no}}; });
    refused([&](auto &c) { c.invariants.push_back(c.invariants[0]); });
    refused([&](auto &c) { c.invariants = {{p.locationCount, yes}}; });
    refused([&](auto &c) { c.invariants = {{good.cuts[0], nullptr}}; });
    refused([&](auto &c) { c.invariants = {{good.cuts[0], terms.bv(APInt(8, 1))}}; });
    refused([&](auto &c) {
      // True at the entry but not preserved: the loop variable stays zero.
      const StateVar *loop = nullptr;
      for (const StateVar &s : p.state)
        if (s.loopCarried && s.location == good.cuts[0])
          loop = &s;
      c.invariants = {{good.cuts[0],
                       terms.icmp(Predicate::Eq, terms.state(loop->id, loop->sort),
                                  terms.bv(APInt(loop->sort.width, 0)))}};
    });
  }

  // Children: dropping the inner proof of a nest leaves its SCC uncovered.
  {
    SemanticProgram p = m.import("e_rect");
    TerminationResult result = analyze(p);
    TerminationCertificate bad = *result.certificate;
    CHECK(!bad.levels[0].children.empty());
    bad.levels[0].children.clear();
    CHECK(!isProven(check(p, bad)));
    // Claiming strictness on every path of the outer node fails O5, since the
    // inner loop leaves the outer rank unchanged.
    TerminationCertificate all = *result.certificate;
    all.levels[0].strict = all.levels[0].paths;
    all.levels[0].children.clear();
    CHECK(!isProven(check(p, all)));
    // An inner rank at the outer node does not decrease across the outer path.
    TerminationCertificate swapped = *result.certificate;
    auto inner = swapped.levels[0].children[0].rank[0].second;
    for (auto &rank : swapped.levels[0].rank)
      rank.second = inner;
    CHECK(!isProven(check(p, swapped)));
    // The structure is intact and the outer node is true, so only the descent
    // obligations of the child can catch these.
    auto childRank = [&](auto makeRank) {
      TerminationCertificate bad = *result.certificate;
      for (auto &rank : bad.levels[0].children[0].rank)
        rank.second = makeRank(rank.second);
      return check(p, bad);
    };
    CheckOutcome constant = childRank([&](Term rank) {
      return p.terms->bv(APInt(rank->sort.width, 7));
    });
    CHECK(!isProven(constant));
    bool childFailed = false;
    for (const ObligationRecord &record : constant.records)
      childFailed = childFailed || (record.id == "O5" && !record.discharged);
    CHECK(childFailed);
    CHECK(!isProven(childRank([&](Term rank) {
      return p.terms->binary(TermOp::Sub, p.terms->bv(APInt::getZero(rank->sort.width)),
                             rank);
    })));

    // These two are refused later in any case, because the finite list of nodes
    // runs out, or the computed paths are the ones checked. The check at the node
    // at fault is what gives the reason.
    auto refusedFor = [&](auto mutate, const char *reason) {
      TerminationCertificate bad = *result.certificate;
      mutate(bad);
      CheckOutcome outcome = check(p, bad);
      CHECK(!isProven(outcome));
      bool named = false;
      for (const ObligationRecord &record : outcome.records)
        named = named || (record.id == "O0" && !record.discharged &&
                          record.evidence.find(reason) != std::string::npos);
      CHECK(named);
    };
    refusedFor([](auto &c) { c.levels[0].strict.clear(); }, "removes no path");
    // A proof node that leaves out a path it does not claim strict.
    refusedFor(
        [](auto &c) {
          auto &level = c.levels[0];
          for (unsigned i = 0; i < level.paths.size(); ++i)
            if (!llvm::is_contained(level.strict, level.paths[i])) {
              level.paths.erase(level.paths.begin() + i);
              return;
            }
          CHECK(false);
        },
        "does not list exactly");
  }

  // A rank that decreases on one branch proves nothing about the other.
  {
    Module branch(R"(
func.func @one_branch(%n: i8, %c: i1) {
  %z = arith.constant 0 : i8
  %o = arith.constant 1 : i8
  cf.br ^h(%z : i8)
^h(%i: i8):
  %g = arith.cmpi slt, %i, %n : i8
  cf.cond_br %g, ^b, ^x
^b:
  %even = arith.andi %i, %o : i8
  %t = arith.cmpi eq, %even, %z : i8
  cf.cond_br %t, ^up, ^stay
^up:
  %u = arith.addi %i, %o : i8
  cf.br ^h(%u : i8)
^stay:
  cf.br ^h(%i : i8)
^x:
  return
}
)");
    SemanticProgram p = branch.import("one_branch");
    TerminationResult result = analyze(p);
    CHECK(result.verdict.kind() == V::RefutedNonterminating);
    TermContext &terms = *p.terms;
    TerminationCertificate forged;
    forged.fingerprint = p.fingerprint;
    forged.cuts = computeCutSet(p);
    PathSet set = enumerateCutPaths(p, forged.cuts);
    RankLevel level;
    level.cuts = forged.cuts;
    Term i = nullptr, n = terms.param(0, Sort::bv(8));
    for (const StateVar &s : p.state)
      if (s.loopCarried)
        i = terms.state(s.id, s.sort);
    Term rank = terms.binary(TermOp::Sub, terms.sext(n, 10), terms.sext(i, 10));
    level.rank = {{forged.cuts[0], rank}};
    for (const CutPath &path : set.paths)
      if (!path.fromEntry && path.to == forged.cuts[0]) {
        level.paths.push_back(path.transitions);
        // Only the decreasing branch would pass. Both are claimed in turn.
        level.strict.push_back(path.transitions);
      }
    forged.levels = {level};
    CHECK(!isProven(check(p, forged)));
    forged.levels[0].strict.pop_back();
    CHECK(!isProven(check(p, forged)));
    // The structural rule: nothing strict removes no path from the SCC.
    forged.levels[0].strict.clear();
    CHECK(!isProven(check(p, forged)));
  }

  // A hand-written true certificate passes, so the rejections above are real.
  {
    SemanticProgram p = m.import("a_wrap");
    TerminationCertificate c;
    c.fingerprint = p.fingerprint;
    c.cuts = computeCutSet(p);
    PathSet set = enumerateCutPaths(p, c.cuts);
    TermContext &terms = *p.terms;
    Term i = nullptr, n = terms.param(0, Sort::bv(8));
    for (const StateVar &s : p.state)
      if (s.loopCarried)
        i = terms.state(s.id, s.sort);
    RankLevel level;
    level.cuts = c.cuts;
    level.rank = {{c.cuts[0], terms.binary(TermOp::Sub, terms.sext(n, 10), terms.sext(i, 10))}};
    for (const CutPath &path : set.paths)
      if (!path.fromEntry && path.to == c.cuts[0]) {
        level.paths.push_back(path.transitions);
        level.strict.push_back(path.transitions);
      }
    c.levels = {level};
    // The step `i + c1` is opaque without this invariant: c1 is state.
    CHECK(!isProven(check(p, c)));
    Term definitions = terms.boolean(true);
    for (const CutPath &path : set.paths)
      if (path.fromEntry)
        for (const StateVar &s : p.state)
          if (!s.loopCarried && path.state[s.id] &&
              path.state[s.id]->op == TermOp::Const)
            definitions = terms.logicalAnd(
                definitions, terms.icmp(Predicate::Eq, terms.state(s.id, s.sort),
                                        path.state[s.id]));
    c.invariants = {{c.cuts[0], definitions}};
    CHECK(isProven(check(p, c)));
  }
}

//===----------------------------------------------------------------------===//
// Verdict discipline
//===----------------------------------------------------------------------===//

static void testSolverFailures(const std::string &text) {
  Module m(text);
  SemanticProgram p = m.import("a_nsw");
  ScriptedBackend backend;

  // Solver unknown everywhere.
  backend.rule = [](const std::string &, unsigned) { return SmtAnswer::Unknown; };
  TerminationResult result = analyze(p, fastOptions(), &backend);
  CHECK(result.verdict.kind() == V::Unknown && !result.certificate);

  // A solver that says sat to everything proves nothing and refutes nothing.
  backend.rule = [](const std::string &, unsigned) { return SmtAnswer::Sat; };
  result = analyze(p, fastOptions(), &backend);
  CHECK(result.verdict.kind() == V::Unknown);

  // Unknown from one obligation family blocks exactly what it should.
  for (const char *family : {"; O1", "; O2", "; O3", "; O4", "; O5", "; O6"}) {
    ScriptedBackend b;
    std::string id = family;
    b.rule = [id](const std::string &script, unsigned) -> std::optional<SmtAnswer> {
      if (script.rfind(id, 0) == 0)
        return SmtAnswer::Unknown;
      return std::nullopt;
    };
    for (const char *name : {"e_rect", "e_lex"}) {
      SemanticProgram q = m.import(name);
      TerminationResult r = analyze(q, fastOptions(), &b);
      // e_lex and e_rect have weak paths, so O6 is exercised. Each family has
      // at least one obligation in one of the two programs.
      if (id == "; O6" || id == "; O5" || id == "; O4" || id == "; O3" ||
          id == "; O2" || id == "; O1")
        CHECK(r.verdict.kind() == V::Unknown);
      CHECK(!r.certificate);
    }
  }

  // O7 alone unknown: finite runs are proven, normal return is not. This is
  // the promotion that must never happen.
  {
    ScriptedBackend b;
    b.rule = [](const std::string &script, unsigned) -> std::optional<SmtAnswer> {
      if (script.rfind("; O7", 0) == 0)
        return SmtAnswer::Unknown;
      return std::nullopt;
    };
    TerminationResult r = analyze(m.import("c_nsw"), fastOptions(), &b);
    CHECK(r.verdict.kind() == V::ProvenTerminatingIfDefined);
    CHECK(r.stops[0].status != StopStatus::Unreachable);
  }

  // A solver that lies. The proof passes every obligation, and the exact replay
  // contradicts it. That is a defect: neither verdict stands, and it is reported.
  {
    ScriptedBackend liar;
    liar.rule = [](const std::string &script, unsigned) -> std::optional<SmtAnswer> {
      if (script.rfind("; O1", 0) == 0 || script.rfind("; domain", 0) == 0)
        return SmtAnswer::Sat;
      return SmtAnswer::Unsat;
    };
    TerminationResult cycle = analyze(m.import("a_le_wrap"), fastOptions(), &liar);
    CHECK(cycle.verdict.kind() == V::Unknown);
    CHECK(!cycle.conflict.empty() && !cycle.certificate && !cycle.witness);
    TerminationResult stop = analyze(m.import("b3_nsw"), fastOptions(), &liar);
    CHECK(stop.verdict.kind() == V::Unknown);
    CHECK(!stop.conflict.empty() && !stop.certificate);
  }

  // Failing any single query can only lose information. The result is the
  // baseline or UNKNOWN, except an O7 failure, which may only weaken a proof.
  auto rank = [](V v) {
    return v == V::ProvenTerminating ? 2 : v == V::ProvenTerminatingIfDefined ? 1 : 0;
  };
  for (const char *name : {"a_nsw", "e_lex", "b3_wrap", "c_nsw", "a_le_wrap"}) {
    SemanticProgram q = m.import(name);
    V baseline = analyze(q).verdict.kind();
    unsigned total;
    {
      ScriptedBackend counting;
      counting.rule = [](const std::string &, unsigned) -> std::optional<SmtAnswer> {
        return std::nullopt;
      };
      analyze(q, fastOptions(), &counting);
      total = counting.count;
    }
    for (unsigned k = 0; k < total; ++k) {
      SemanticProgram again = m.import(name);
      ScriptedBackend b;
      b.rule = [k](const std::string &, unsigned i) -> std::optional<SmtAnswer> {
        if (i == k)
          return SmtAnswer::Unknown;
        return std::nullopt;
      };
      V got = analyze(again, fastOptions(), &b).verdict.kind();
      CHECK(rank(got) <= rank(baseline));
      if (baseline == V::RefutedNonterminating)
        CHECK(got == baseline || got == V::Unknown);
    }
  }

  // Resource limits end in UNKNOWN, or in a result that still has a proof.
  {
    SemanticProgram q = m.import("a_nsw");
    unsigned needed = 0;
    for (const ObligationRecord &record : analyze(q).records)
      needed += record.id != "O0";
    CHECK(needed > 4);
    for (unsigned limit = 0; limit <= needed + 1; ++limit) {
      auto options = fastOptions();
      options.limits.maxProposerQueries = limit;
      V got = analyze(q, options).verdict.kind();
      CHECK(got == V::Unknown || got == V::ProvenTerminating);
      options = fastOptions();
      options.limits.maxCheckerQueries = limit;
      got = analyze(q, options).verdict.kind();
      // An O7 cut short could at most lose the normal-return half.
      CHECK(limit >= needed ? got == V::ProvenTerminating
                            : got != V::ProvenTerminating);
    }
    auto options = fastOptions();
    options.limits.paths.maxPaths = 0;
    CHECK(analyze(q, options).verdict.kind() == V::Unknown);
    options = fastOptions();
    options.limits.paths.maxNewTerms = 0;
    options.limits.paths.maxPathLength = 1;
    CHECK(analyze(q, options).verdict.kind() == V::Unknown);
    options = fastOptions();
    options.limits.solver.resourceLimit = 1;
    CHECK(analyze(q, options).verdict.kind() == V::Unknown);
    // Fuel never makes a refutation: a long cycle at low fuel is UNKNOWN.
    SemanticProgram cycle = m.import("h_even");
    options = fastOptions();
    options.limits.replayFuel = 10;
    CHECK(analyze(cycle, options).verdict.kind() == V::Unknown);
  }

  // Term exhaustion is an UNKNOWN, not an exception.
  {
    Module small(text);
    auto function = small.module->lookupSymbol<mlir::func::FuncOp>("a_nsw");
    ImportOptions tight;
    tight.limits.maxTerms = 60;
    SemanticProgram r = importFunction(function, tight);
    V got = analyze(r).verdict.kind();
    CHECK(got == V::Unknown);
  }
}

static const char *memoryPrograms = R"(
func.func @scan(%a: memref<4xi8>) -> index {
  %z = arith.constant 0 : index
  %o = arith.constant 1 : index
  %f = arith.constant 4 : index
  %zero = arith.constant 0 : i8
  cf.br ^h(%z : index)
^h(%i: index):
  %b = arith.cmpi ult, %i, %f : index
  cf.cond_br %b, ^t, ^x
^t:
  %v = memref.load %a[%i] : memref<4xi8>
  %c = arith.cmpi ne, %v, %zero : i8
  cf.cond_br %c, ^n, ^x
^n:
  %u = arith.addi %i, %o : index
  cf.br ^h(%u : index)
^x:
  return %i : index
}
func.func @unchecked(%a: memref<4xi8>) -> index {
  %z = arith.constant 0 : index
  %o = arith.constant 1 : index
  %zero = arith.constant 0 : i8
  cf.br ^h(%z : index)
^h(%i: index):
  %v = memref.load %a[%i] : memref<4xi8>
  %c = arith.cmpi ne, %v, %zero : i8
  cf.cond_br %c, ^n, ^x
^n:
  %u = arith.addi %i, %o : index
  cf.br ^h(%u : index)
^x:
  return %i : index
}
func.func @two_roots(%a: memref<2xi8>, %b: memref<2xi8>) {
  %z = arith.constant 0 : index
  %zero = arith.constant 0 : i8
  %one = arith.constant 1 : i8
  cf.br ^h
^h:
  %v = memref.load %a[%z] : memref<2xi8>
  %c = arith.cmpi ne, %v, %zero : i8
  cf.cond_br %c, ^n, ^x
^n:
  memref.store %zero, %a[%z] : memref<2xi8>
  memref.store %one, %b[%z] : memref<2xi8>
  cf.br ^h
^x:
  return
}
func.func @two_roots_unused(%a: memref<2xi8>, %b: memref<2xi8>, %n: i8) {
  %z = arith.constant 0 : index
  %i0 = arith.constant 0 : i8
  %one = arith.constant 1 : i8
  cf.br ^h(%i0 : i8)
^h(%i: i8):
  %c = arith.cmpi slt, %i, %n : i8
  cf.cond_br %c, ^n, ^x
^n:
  memref.store %one, %a[%z] : memref<2xi8>
  memref.store %one, %b[%z] : memref<2xi8>
  %u = arith.addi %i, %one : i8
  cf.br ^h(%u : i8)
^x:
  return
}
func.func @fresh_read(%n: i8) {
  %m = memref.alloca() : memref<1xi8>
  %z = arith.constant 0 : index
  %i0 = arith.constant 0 : i8
  %one = arith.constant 1 : i8
  cf.br ^h(%i0 : i8)
^h(%i: i8):
  %c = arith.cmpi slt, %i, %n : i8
  cf.cond_br %c, ^b, ^x
^b:
  %v = memref.load %m[%z] : memref<1xi8>
  %u = arith.addi %i, %one : i8
  cf.br ^h(%u : i8)
^x:
  return
}
func.func @memory_counter(%a: memref<1xi8>) {
  %z = arith.constant 0 : index
  %one = arith.constant 1 : i8
  %zero = arith.constant 0 : i8
  cf.br ^h
^h:
  %v = memref.load %a[%z] : memref<1xi8>
  %u = arith.addi %v, %one : i8
  memref.store %u, %a[%z] : memref<1xi8>
  %c = arith.cmpi eq, %u, %zero : i8
  cf.cond_br %c, ^x, ^h
^x:
  return
}
func.func @memory_spin(%a: memref<1xi8>) {
  %z = arith.constant 0 : index
  %zero = arith.constant 0 : i8
  cf.br ^h
^h:
  %v = memref.load %a[%z] : memref<1xi8>
  memref.store %v, %a[%z] : memref<1xi8>
  %c = arith.cmpi eq, %v, %zero : i8
  cf.cond_br %c, ^h, ^x
^x:
  return
}
func.func @stop_after_loop(%a: memref<2xi8>, %b: memref<2xi8>, %n: i8) {
  %z = arith.constant 0 : index
  %i0 = arith.constant 0 : i8
  %one = arith.constant 1 : i8
  cf.br ^h(%i0 : i8)
^h(%i: i8):
  %c = arith.cmpi slt, %i, %n : i8
  cf.cond_br %c, ^n, ^x
^n:
  memref.store %one, %b[%z] : memref<2xi8>
  %u = arith.addi %i, %one : i8
  cf.br ^h(%u : i8)
^x:
  %v = memref.load %a[%z] : memref<2xi8>
  %w = arith.ori %v, %one : i8
  %q = arith.divui %one, %w : i8
  return
}
func.func @gemm(%a: memref<2x2xf32>, %b: memref<2x2xf32>, %c: memref<2x2xf32>) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c2 = arith.constant 2 : index
  scf.for %i = %c0 to %c2 step %c1 {
    scf.for %j = %c0 to %c2 step %c1 {
      scf.for %k = %c0 to %c2 step %c1 {
        %x = memref.load %a[%i, %k] : memref<2x2xf32>
        %y = memref.load %b[%k, %j] : memref<2x2xf32>
        %z = memref.load %c[%i, %j] : memref<2x2xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m : f32
        memref.store %s, %c[%i, %j] : memref<2x2xf32>
      }
    }
  }
  return
}
func.func @structured(%n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  scf.for %i = %c0 to %n step %c1 {
    scf.for %j = %c0 to %i step %c1 {
    }
  }
  return
}
)";

static void testMemoryAndStops() {
  Module m(memoryPrograms);
  ImportOptions indexOptions;
  indexOptions.indexBitWidth = 8;

  // A bounds check in the guard: the rank is the bound's distance. Not one
  // state rank mentions memory, but the guard does, so the premise is reported.
  SemanticProgram scan = m.import("scan", indexOptions);
  TerminationResult r = analyze(scan);
  CHECK(r.verdict.kind() == V::ProvenTerminating);

  // No bounds check: every run is finite because the access stops it, and
  // that is UB. A "proof" of return would be wrong.
  SemanticProgram unchecked = m.import("unchecked", indexOptions);
  r = analyze(unchecked);
  CHECK(r.verdict.kind() == V::ProvenTerminatingIfDefined);
  CHECK(r.stops[(unsigned)SideKind::UndefinedBehavior].status == StopStatus::Reachable);

  // Memory in the proof brings in the separation assumption, by name.
  {
    SemanticProgram p = m.import("two_roots", indexOptions);
    CHECK(p.isSupported() && p.assumptions.size() == 1);
    if (p.assumptions.empty())
      return;
    TerminationResult first = analyze(p);
    CHECK(first.verdict.kind() == V::Unknown);
    CHECK(!first.certificate);
    auto options = fastOptions();
    options.authorized.insert(p.assumptions[0].detail());
    TerminationResult second = analyze(p, options);
    CHECK(second.verdict.kind() == V::ProvenTerminating);
    CHECK(second.certificate.has_value());
    CHECK(second.status.kind() == ProofStatus::Kind::Assumed);
    CHECK(second.assumptionsUsed.size() == 1);
    // A proof whose terms do not read memory needs no separation.
    SemanticProgram q = m.import("two_roots_unused", indexOptions);
    CHECK(q.assumptions.size() == 1);
    TerminationResult third = analyze(q);
    CHECK(third.verdict.kind() == V::ProvenTerminating);
    CHECK(third.status.kind() == ProofStatus::Kind::Proven);
    CHECK(third.assumptionsUsed.empty());

    // A stop after the loop reads memory, and the loop's proof does not. Without
    // separation that stop stays open and the theorem stands; with it, the stop
    // is excluded and the assumption is reported.
    SemanticProgram s = m.import("stop_after_loop", indexOptions);
    CHECK(s.assumptions.size() == 1);
    TerminationResult open = analyze(s);
    CHECK(open.verdict.kind() == V::ProvenTerminatingIfDefined);
    CHECK(open.stops[(unsigned)SideKind::UndefinedBehavior].status == StopStatus::Unknown);
    CHECK(open.status.kind() == ProofStatus::Kind::Proven);
    CHECK(open.assumptionsUsed.empty());
    auto separated = fastOptions();
    separated.authorized.insert(s.assumptions[0].detail());
    TerminationResult closed = analyze(s, separated);
    CHECK(closed.verdict.kind() == V::ProvenTerminating);
    CHECK(closed.status.kind() == ProofStatus::Kind::Assumed);
    CHECK(closed.assumptionsUsed.size() == 1);
  }

  // A rank that reads memory is refused even when it is true: this counter's
  // cell rises to its wrap. Ranks speak of state alone.
  {
    SemanticProgram p = m.import("memory_counter", indexOptions);
    TermContext &terms = *p.terms;
    Term cell = terms.load(terms.memVar(p.roots[0].sort),
                           {terms.bv(APInt(8, 0))});
    std::string why;
    CHECK(adoptProofTerm(p, cell, why) == nullptr);
    CHECK(why.find("memory") != std::string::npos);
    TerminationCertificate c;
    c.fingerprint = p.fingerprint;
    c.cuts = computeCutSet(p);
    PathSet set = enumerateCutPaths(p, c.cuts);
    RankLevel level;
    level.cuts = c.cuts;
    level.rank = {{c.cuts[0], terms.binary(TermOp::Sub, terms.bv(APInt(8, 255)), cell)}};
    for (const CutPath &path : set.paths)
      if (!path.fromEntry && path.to == c.cuts[0]) {
        level.paths.push_back(path.transitions);
        level.strict.push_back(path.transitions);
      }
    c.levels = {level};
    Term definitions = terms.boolean(true);
    for (const CutPath &path : set.paths)
      if (path.fromEntry)
        for (const StateVar &s : p.state)
          if (path.state[s.id] && path.state[s.id]->op == TermOp::Const)
            definitions = terms.logicalAnd(
                definitions, terms.icmp(Predicate::Eq, terms.state(s.id, s.sort),
                                        path.state[s.id]));
    c.invariants = {{c.cuts[0], definitions}};
    CHECK(!isProven(checkCertificate(p, c, CheckOptions{}, z3Backend)));
  }

  // An uninitialized read is no side condition, and is still not a return.
  r = analyze(m.import("fresh_read", indexOptions));
  CHECK(r.verdict.kind() == V::ProvenTerminatingIfDefined);
  CHECK(r.stops[(unsigned)SideKind::Underspecified].status != StopStatus::Unreachable);

  // Same location, same scalar state, different memory: not a cycle.
  {
    SemanticProgram p = m.import("memory_counter", indexOptions);
    EntryState entry = entryOf(p, {});
    ReplayReport replay = replayEntry(p, entry, 100000, 64);
    CHECK(replay.end == ReplayEnd::Returned);
  }
  // Memory that really repeats is a cycle, and the verdict shows it.
  {
    SemanticProgram p = m.import("memory_spin", indexOptions);
    TerminationResult spin = analyze(p);
    CHECK(spin.verdict.kind() == V::RefutedNonterminating);
    CHECK(spin.witness.has_value());
  }

  // Float arithmetic never enters a termination query, and every NaN site is
  // still reported. The nest is proven without a claim of normal return.
  {
    ImportOptions options;
    options.indexBitWidth = 16;
    options.lowerStructured = true;
    SemanticProgram p = m.import("gemm", options);
    CHECK(p.isSupported());
    auto limits = fastOptions();
    limits.limits.solver.timeoutMilliseconds = 2000;
    TerminationResult gemm = analyze(p, limits);
    CHECK(gemm.verdict.kind() == V::ProvenTerminatingIfDefined);
    CHECK(gemm.stops[(unsigned)SideKind::UndefinedBehavior].status == StopStatus::Unreachable);
    CHECK(gemm.stops[(unsigned)SideKind::Underspecified].status != StopStatus::Unreachable);
    for (const ObligationRecord &record : gemm.records)
      if (record.id != "O7")
        CHECK(record.script.find("FloatingPoint") == std::string::npos);
    CHECK(gemm.status.kind() == ProofStatus::Kind::Proven);
  }
  {
    ImportOptions options;
    options.indexBitWidth = 32;
    options.lowerStructured = true;
    TerminationResult s = analyze(m.import("structured", options));
    CHECK(s.verdict.kind() == V::ProvenTerminating);
    CHECK(s.conflict.empty());
    // A nested region without lowering refuses, and that is not termination.
    ImportOptions plain;
    plain.indexBitWidth = 32;
    TerminationResult refused = analyze(m.import("structured", plain));
    CHECK(refused.verdict.kind() == V::Unknown);
    CHECK(refused.verdict.reason().find("unsupported") != std::string::npos);
  }
}

/// Findings of the independent review, kept as regressions.
static void testReviewFindings() {
  Module m(R"(
func.func @negative_extent(%a: memref<?xi8>) {
  %z = arith.constant 0 : index
  %v = memref.load %a[%z] : memref<?xi8>
  return
}
func.func @step(%st: index) {
  %c0 = arith.constant 0 : index
  %c3 = arith.constant 3 : index
  scf.for %i = %c0 to %c3 step %st {
  }
  return
}
func.func @as_written() {
  %z = arith.constant 0.0 : f32
  %h = arith.constant 1.0e30 : f32
  %a = arith.mulf %z, %h : f32
  %b = arith.mulf %a, %h : f32
  return
}
func.func @reassociable() {
  %z = arith.constant 0.0 : f32
  %h = arith.constant 1.0e30 : f32
  %a = arith.mulf %z, %h fastmath<reassoc> : f32
  %b = arith.mulf %a, %h fastmath<reassoc> : f32
  return
}
func.func @float_parameter(%x: f32) {
  return
}
)");
  ImportOptions options;
  options.indexBitWidth = 4;
  // A negative extent has no valid entry, so that domain is empty.
  SemanticProgram p = m.import("negative_extent", options);
  CHECK(p.isSupported());
  auto scoped = fastOptions();
  scoped.scope.allParameters = false;
  scoped.scope.ranges.push_back({0, APInt(4, -3, true), APInt(4, -1, true)});
  CHECK(analyze(p, scoped).verdict.kind() == V::Unknown);
  // The same program over its real domain still has a result.
  CHECK(analyze(p).verdict.kind() != V::RefutedNonterminating);

  // A zero step cycles in the lowered CFG, and scf.for does not allow it.
  ImportOptions lowered;
  lowered.indexBitWidth = 8;
  lowered.lowerStructured = true;
  SemanticProgram loop = m.import("step", lowered);
  CHECK(loop.isSupported() && loop.structuredLoweringApplied);
  TerminationResult cycle = analyze(loop);
  CHECK(cycle.verdict.kind() == V::Unknown && !cycle.witness);
  CHECK(cycle.verdict.reason().find("precondition") != std::string::npos);
  ReplayReport found = replayEntry(loop, {{APInt(8, 0)}, {}}, 1u << 12, 64);
  CHECK(found.end == ReplayEnd::Cycle);
  CHECK(verifyNonterminationWitness(loop, ParameterScope{}, *found.witness, 64).kind() == V::Unknown);

  // Without flags O7 settles the NaN site. With reassoc it covers one behaviour.
  TerminationResult strict = analyze(m.import("as_written"));
  CHECK(strict.verdict.kind() == V::ProvenTerminating);
  TerminationResult loose = analyze(m.import("reassociable"));
  CHECK(loose.verdict.kind() == V::ProvenTerminatingIfDefined);
  CHECK(loose.stops[(unsigned)SideKind::Underspecified].status == StopStatus::Unknown);

  // A range on a float parameter is refused, not a crash.
  SemanticProgram f = m.import("float_parameter");
  auto bad = fastOptions();
  bad.scope.allParameters = false;
  bad.scope.ranges.push_back({0, APInt(32, 0), APInt(32, 1)});
  TerminationResult refused = analyze(f, bad);
  CHECK(refused.verdict.kind() == V::Unknown);
  CHECK(refused.verdict.reason().find("malformed scope") != std::string::npos);
  CheckOptions check{bad.scope, bad.limits, {}};
  TerminationCertificate certificate;
  certificate.fingerprint = f.fingerprint;
  CHECK(checkCertificate(f, certificate, check, z3Backend).verdict.kind() == V::Unknown);
}

static void testWitnesses(const std::string &text) {
  Module m(text);
  SemanticProgram p = m.import("h_even");
  ParameterScope all;
  ReplayReport replay = replayEntry(p, EntryState{{APInt(8, 0)}, {}}, 1u << 16, 64);
  CHECK(replay.end == ReplayEnd::Cycle && replay.witness.has_value());
  NonterminationWitness good = *replay.witness;
  CHECK(verifyNonterminationWitness(p, all, good, 64).kind() == V::RefutedNonterminating);

  auto forged = [&](auto change) {
    NonterminationWitness bad = good;
    change(bad);
    return verifyNonterminationWitness(p, all, bad, 64).kind();
  };
  CHECK(forged([](auto &w) { w.period = 0; }) == V::Unknown);
  CHECK(forged([&](auto &w) { w.period = good.period - 1; }) == V::Unknown);
  CHECK(forged([&](auto &w) { w.period = good.period + 1; }) == V::Unknown);
  // A multiple of the period is still an exact repetition.
  CHECK(forged([&](auto &w) { w.period = good.period * 2; }) == V::RefutedNonterminating);
  CHECK(forged([](auto &w) { w.entry.parameters.push_back(APInt(8, 0)); }) == V::Unknown);
  CHECK(forged([](auto &w) { w.entry.parameters.clear(); }) == V::Unknown);

  // A witness outside the scope refutes nothing about the scope.
  ParameterScope narrow;
  narrow.allParameters = false;
  narrow.ranges.push_back({0, APInt(8, 1), APInt(8, 2)});
  CHECK(verifyNonterminationWitness(p, narrow, good, 64).kind() == V::Unknown);

  // A run that returns or stops is no witness, whatever the numbers say.
  SemanticProgram returning = m.import("a_wrap");
  CHECK(verifyNonterminationWitness(
            returning, all, NonterminationWitness{{{APInt(8, 3)}, {}}, 0, 2}, 64)
            .kind() == V::Unknown);
  SemanticProgram stopping = m.import("b3_nsw");
  ReplayReport stop = replayEntry(stopping, {{APInt(8, 127)}, {}}, 1u << 12, 64);
  CHECK(stop.end == ReplayEnd::Stopped && stop.stop == RunStatus::UndefinedBehavior);
  // A stopped or finished step leaves the configuration as it was, so a repeat
  // of one configuration is no cycle when the period itself cannot step.
  CHECK(verifyNonterminationWitness(
            stopping, all,
            NonterminationWitness{{{APInt(8, 127)}, {}}, stop.steps - 1, 1}, 64)
            .kind() == V::Unknown);
  ReplayReport finish = replayEntry(returning, {{APInt(8, 3)}, {}}, 1u << 12, 64);
  CHECK(finish.end == ReplayEnd::Returned);
  CHECK(verifyNonterminationWitness(
            returning, all,
            NonterminationWitness{{{APInt(8, 3)}, {}}, finish.steps, 1}, 64)
            .kind() == V::Unknown);

  // Fuel: too little is OutOfFuel, and proves nothing.
  CHECK(replayEntry(p, EntryState{{APInt(8, 0)}, {}}, 8, 64).end == ReplayEnd::OutOfFuel);
  CHECK(replayEntry(p, EntryState{{}, {}}, 8, 64).end == ReplayEnd::Invalid);

  // A repeated partial state is not a repeated configuration. Each field of a
  // configuration must be compared.
  Configuration a, b;
  a.location = b.location = 1;
  a.state = {APInt(8, 3), std::nullopt};
  b.state = a.state;
  a.shadow = b.shadow = {false, false};
  RootData data;
  data.shape = {1};
  data.cells = {APInt(8, 1)};
  a.memory = b.memory = {data};
  CHECK(sameConfiguration(a, b));
  auto differs = [&](auto change) {
    Configuration c = b;
    change(c);
    return !sameConfiguration(a, c);
  };
  CHECK(differs([](auto &c) { c.location = 2; }));
  CHECK(differs([](auto &c) { c.state[0] = APInt(8, 4); }));
  CHECK(differs([](auto &c) { c.state[1] = APInt(8, 4); }));
  CHECK(differs([](auto &c) { c.shadow[1] = true; }));
  CHECK(differs([](auto &c) { c.memory[0].cells[0] = APInt(8, 2); }));
  CHECK(differs([](auto &c) { c.memory[0].cells[0] = std::nullopt; }));
  CHECK(differs([](auto &c) { c.memory.clear(); }));

  // A second counter that never repeats within the first's cycle.
  Module two(R"(
func.func @pair(%n: i8) {
  %z = arith.constant 0 : i8
  %o = arith.constant 1 : i8
  %f = arith.constant 15 : i8
  cf.br ^h(%z, %z : i8, i8)
^h(%i: i8, %k: i8):
  %d = arith.addi %k, %o : i8
  %s = arith.addi %i, %o : i8
  %w = arith.andi %s, %f : i8
  %g = arith.cmpi ne, %d, %z : i8
  cf.cond_br %g, ^h(%w, %d : i8, i8), ^x
^x:
  return
}
)");
  SemanticProgram pair = two.import("pair");
  CHECK(replayEntry(pair, {{APInt(8, 0)}, {}}, 1u << 12, 64).end == ReplayEnd::Returned);
}

//===----------------------------------------------------------------------===//
// G1 is unchanged, and the result is reproducible
//===----------------------------------------------------------------------===//

static void testPreservation(const std::string &text) {
  Module m(text);
  for (const char *name : {"e_rect", "c_nsw", "t3_swap", "h_dbl_wrap"}) {
    SemanticProgram p = m.import(name);
    std::string before = toJSON(p), relation = encodeTransitionRelation(p);
    size_t transitions = p.transitions.size();
    TerminationResult first = analyze(p);
    CHECK(toJSON(p) == before);
    CHECK(encodeTransitionRelation(p) == relation);
    CHECK(p.transitions.size() == transitions);
    // Runs agree before and after: the analysis leaves no trace in the model.
    SemanticProgram fresh = m.import(name);
    CHECK(fresh.fingerprint == p.fingerprint);
    TerminationResult second = analyze(fresh);
    CHECK(toJSON(fresh, second, true) == toJSON(p, first, true));
  }
  // The input IR is untouched.
  std::string before;
  {
    llvm::raw_string_ostream stream(before);
    m.module->print(stream);
  }
  analyze(m.import("e_tri"));
  std::string after;
  {
    llvm::raw_string_ostream stream(after);
    m.module->print(stream);
  }
  CHECK(before == after);

  // The exported encoder prints what the relation prints.
  SemanticProgram p = m.import("c_nsw");
  std::string relation = encodeTransitionRelation(p);
  for (const Transition &t : p.transitions) {
    std::string text = encodeTerms({t.guard});
    size_t start = 0;
    while ((start = text.find("(define-fun", start)) != std::string::npos) {
      size_t end = text.find('\n', start);
      CHECK(relation.find(text.substr(start, end - start)) != std::string::npos);
      start = end;
    }
    CHECK(text.find("(declare-const") == std::string::npos ||
          relation.find(text.substr(0, text.find('\n'))) != std::string::npos);
  }
  // A leaf is declared once, and only a leaf.
  TermContext &terms = *p.terms;
  Term x = terms.state(0, p.state[0].sort);
  std::string two = encodeTerms({terms.binary(TermOp::Add, x, x), x});
  CHECK(two.find("(declare-const s0 ") != std::string::npos);
  CHECK(two.find("(declare-const s0 ", two.find("(declare-const s0 ") + 1) == std::string::npos);
  CHECK(smtTermName(x) == "t" + std::to_string(x->id));

  // The composition export agrees with summarizeAcyclic from every location.
  for (unsigned location = 0; location < p.locationCount; ++location) {
    SymbolicRegion region = summarizeAcyclic(p, location);
    CHECK(region.complete);
    for (const SymbolicPath &path : region.paths) {
      SymbolicPath again = startSymbolicPath(p, location);
      for (unsigned t : path.transitions)
        again = extendSymbolicPath(p, again, t);
      CHECK(again.condition == path.condition);
      CHECK(again.state == path.state && again.shadow == path.shadow);
      CHECK(again.end == path.end);
    }
  }
}

//===----------------------------------------------------------------------===//
// Formulas
//===----------------------------------------------------------------------===//

static void testFormulas() {
  Module m(R"(
func.func @f(%n: i8) {
  %z = arith.constant 0 : i8
  %o = arith.constant 1 : i8
  cf.br ^h(%z : i8)
^h(%i: i8):
  %g = arith.cmpi slt, %i, %n : i8
  cf.cond_br %g, ^b, ^x
^b:
  %u = arith.addi %i, %o overflow<nsw> : i8
  cf.br ^h(%u : i8)
^x:
  return
}
)");
  SemanticProgram p = m.import("f");
  TermContext &terms = *p.terms;
  std::string why;
  ParameterScope all;
  CHECK(scopeFormula(p, all, why) == terms.boolean(true));
  ParameterScope one;
  one.allParameters = false;
  one.ranges = {{0, APInt(8, 1), APInt(8, 3)}};
  CHECK(parametersInScope(one, {APInt(8, 1)}));
  CHECK(parametersInScope(one, {APInt(8, 3)}));
  CHECK(!parametersInScope(one, {APInt(8, 4)}));
  CHECK(!parametersInScope(one, {APInt(8, 0)}));
  CHECK(parametersInScope(all, {APInt(8, 200)}));

  // A location's guards: exactly one holds.
  for (unsigned location = 0; location < p.locationCount; ++location)
    CHECK(z3Backend.check(obligationScript("g", guardViolation(p, location)), {}, false)
              .answer == SmtAnswer::Unsat);
  CHECK(guardViolation(p, p.locationCount) == nullptr);

  // Two transitions with overlapping or missing guards are caught.
  SemanticProgram overlap = m.import("f");
  for (Transition &t : overlap.transitions)
    if (t.from == 1)
      t.guard = terms.boolean(true);
  CHECK(z3Backend.check(obligationScript("g", guardViolation(overlap, 1)), {}, false)
            .answer == SmtAnswer::Sat);
  SemanticProgram missing = m.import("f");
  for (Transition &t : missing.transitions)
    if (t.from == 1)
      t.guard = terms.boolean(false);
  CHECK(z3Backend.check(obligationScript("g", guardViolation(missing, 1)), {}, false)
            .answer == SmtAnswer::Sat);

  // Paths from the entry start poison free, as initialConfiguration does.
  auto cuts = computeCutSet(p);
  PathSet set = enumerateCutPaths(p, cuts);
  CHECK(set.complete);
  for (const CutPath &path : set.paths)
    if (path.fromEntry)
      CHECK(!containsOp(postTerm(p, path, terms.shadow(0)), TermOp::Shadow));
  // The stuck-on-arrival rule: a condition covers source blocks only. The path
  // that overflows into the header has no branch-on-poison term of the header.
  bool sawStop = false;
  for (const StopSite &site : set.sites)
    sawStop = sawStop || site.reason == "branch on poison";
  CHECK(sawStop);
  for (const CutPath &path : set.paths)
    if (!path.fromEntry && path.to == cuts[0] && path.transitions.size() == 2) {
      Configuration c;
      c.location = path.from;
      c.state.assign(p.state.size(), std::nullopt);
      c.shadow.assign(p.state.size(), false);
      unsigned counter = 0;
      for (const StateVar &s : p.state) {
        if (s.loopCarried)
          counter = s.id;
        c.state[s.id] = APInt(s.sort.width, 1);
      }
      c.state[counter] = APInt(8, 127);
      EntryState entry{{APInt(8, 127)}, {}};
      // 127 < 127 is false: use a larger bound through the wrapped compare.
      c.state[counter] = APInt(8, 126);
      auto holds = evaluateTerm(p, entry, c, path.condition);
      CHECK(holds && holds->isOne());
      // Stepping to the state that overflows next is enabled though the next
      // header would be stuck: the condition did not look ahead.
      c.state[counter] = APInt(8, 127);
      c.shadow[counter] = false;
      entry.parameters[0] = APInt(8, 127);
      auto closed = evaluateTerm(p, entry, c, path.condition);
      CHECK(closed && closed->isZero());
    }

  CHECK(containsFloat(terms.fconst(APInt(32, 0))));
  CHECK(!containsFloat(terms.boolean(true)));
  CHECK(readsMemory(terms.memVar(Sort::mem(0, 1, 8, Sort::bv(8)))));
  CHECK(!readsMemory(terms.boolean(true)));
  TermContext other;
  Term foreign = other.binary(TermOp::Add, other.state(0, p.state[0].sort),
                              other.bv(APInt(p.state[0].sort.width, 1)));
  Term adopted = adoptProofTerm(p, foreign, why);
  CHECK(adopted && adopted != foreign);
  CHECK(terms.size() > 0 && adoptProofTerm(p, adopted, why) == adopted);
  CHECK(!adoptProofTerm(p, terms.state(0, Sort::bv(p.state[0].sort.width + 1)), why));
}

int main(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr, "usage: %s loops4.mlir loops8.mlir\n", argv[0]);
    return 2;
  }
  mlir::MLIRContext ctx;
  configure(ctx);
  context = &ctx;
  std::string small = readFile(argv[1]), wide = readFile(argv[2]);

  testGraphs();
  testScripts();
  testBackend();
  testFormulas();
  testWitnesses(wide);
  testPreservation(wide);
  testCorruption(wide);
  testSolverFailures(wide);
  testMemoryAndStops();
  testReviewFindings();
  testCatalogue(wide);
  testExhaustive(small);

  fprintf(stderr, "%d checks, %d failures\n", checks, failures);
  return failures ? 1 : 0;
}
