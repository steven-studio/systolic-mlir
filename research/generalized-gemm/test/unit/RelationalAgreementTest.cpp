//===- RelationalAgreementTest.cpp - I6 for the relational encoding -------===//
//
// Runs programs concretely and asks Z3, at every step, about the SMT-LIB text:
// the relation admits the concrete step, admits no other successor, and
// refuses a tampered one; a stopped configuration is stuck for the same kind
// of reason. Z3 is only linked here, never by the library.
//
//===----------------------------------------------------------------------===//
#include "TestSupport.h"

#include "GeneralizedGemm/RelationalEncoding.h"

#include <z3++.h>

using namespace gg;

static unsigned queries = 0, skipped = 0;

static z3::check_result solve(const std::string &text) {
  z3::context context;
  z3::solver solver(context);
  z3::params params(context);
  params.set("timeout", 20000u);
  solver.set(params);
  solver.from_string(text.c_str());
  ++queries;
  return solver.check();
}

/// A float NaN has one SMT value. Those configurations stop concretely anyway.
static void checkRun(const SemanticProgram &p, const EntryState &entry,
                     unsigned maxSteps) {
  std::string error;
  auto config = initialConfiguration(p, entry, RunLimits{}, error);
  CHECK(config.has_value());
  for (unsigned i = 0; i < maxSteps; ++i) {
    if (config->location == p.exitLocation())
      return;
    Configuration before = *config;
    std::vector<APInt> returns;
    std::vector<bool> poison;
    StepOutcome step = stepConfiguration(p, entry, *config, &returns, &poison);
    if (step.stopped) {
      switch (*step.stopped) {
      case RunStatus::UndefinedBehavior:
        CHECK(solve(encodeStepQuery(p, entry, before, nullptr, StepQuery::StuckUB)) == z3::sat);
        CHECK(solve(encodeStepQuery(p, entry, before, nullptr, StepQuery::AdmitsAnyStep)) == z3::unsat);
        break;
      case RunStatus::ModelLimit:
        CHECK(solve(encodeStepQuery(p, entry, before, nullptr, StepQuery::StuckOther)) == z3::sat);
        CHECK(solve(encodeStepQuery(p, entry, before, nullptr, StepQuery::AdmitsAnyStep)) == z3::unsat);
        break;
      case RunStatus::Underspecified:
        // An uninitialized read is found by evaluating, not by a side
        // condition, and the relation leaves such a cell free.
        if (step.reason == "read of uninitialized memory") {
          ++skipped;
          break;
        }
        CHECK(solve(encodeStepQuery(p, entry, before, nullptr, StepQuery::StuckOther)) == z3::sat);
        CHECK(solve(encodeStepQuery(p, entry, before, nullptr, StepQuery::AdmitsAnyStep)) == z3::unsat);
        break;
      default:
        CHECK(false);
      }
      return;
    }
    CHECK(solve(encodeStepQuery(p, entry, before, nullptr, StepQuery::StuckUB)) == z3::unsat);
    CHECK(solve(encodeStepQuery(p, entry, before, config.operator->(), StepQuery::AdmitsStep, &returns, &poison)) == z3::sat);
    CHECK(solve(encodeStepQuery(p, entry, before, config.operator->(), StepQuery::AdmitsOtherStep, &returns, &poison)) == z3::unsat);
    // A wrong result is refused.
    for (unsigned r = 0; r < returns.size(); ++r) {
      if (poison[r])
        continue;
      std::vector<APInt> wrongResults = returns;
      wrongResults[r] += 1;
      CHECK(solve(encodeStepQuery(p, entry, before, config.operator->(), StepQuery::AdmitsStep, &wrongResults, &poison)) == z3::unsat);
      break;
    }
    // A wrong location is refused.
    Configuration wrong = *config;
    wrong.location = (wrong.location + 1) % (p.locationCount + 1);
    CHECK(solve(encodeStepQuery(p, entry, before, &wrong, StepQuery::AdmitsStep)) == z3::unsat);
    // So is a wrong value, in the first assigned scalar variable.
    for (unsigned s = 0; s < p.state.size(); ++s) {
      if (!wrong.state[s] || p.state[s].sort.kind != Sort::BV)
        continue;
      Configuration tampered = *config;
      *tampered.state[s] += 1;
      CHECK(solve(encodeStepQuery(p, entry, before, &tampered, StepQuery::AdmitsStep)) == z3::unsat);
      break;
    }
    // And a wrong memory cell.
    for (unsigned r = 0; r < p.roots.size(); ++r) {
      RootData &data = config->memory[r];
      if (data.cells.empty() || !data.cells[0])
        continue;
      Configuration tampered = *config;
      *tampered.memory[r].cells[0] += 1;
      CHECK(solve(encodeStepQuery(p, entry, before, &tampered, StepQuery::AdmitsStep)) == z3::unsat);
      break;
    }
  }
}

int main() {
  // Total division matches SMT-LIB on the boundary cases.
  std::string divide = R"(func.func @f(%a: i4, %b: i4) -> i4 {
  %one = arith.constant 1 : i4
  %d = arith.ori %b, %one : i4
  %q = arith.divsi %a, %d : i4
  %r = arith.remsi %a, %d : i4
  %u = arith.divui %a, %d : i4
  %v = arith.remui %a, %d : i4
  %s = arith.shli %a, %b : i4
  %t = arith.shrsi %a, %b : i4
  %w = arith.shrui %a, %b overflow<none> : i4
  %x = arith.addi %q, %r : i4
  %y = arith.addi %u, %v : i4
  %z = arith.xori %x, %y : i4
  return %z : i4
})";
  size_t position = divide.find(" overflow<none>");
  divide.erase(position, 15);
  auto arithmetic = importText(divide);
  CHECK(arithmetic.isSupported());
  for (int64_t a = 0; a < 16; ++a)
    for (int64_t b = 0; b < 16; ++b)
      checkRun(arithmetic, entryFor(arithmetic, {a, b}), 3);

  // The bits of a poison result are internal, but state carries them, so the
  // total definitions of the shifts must agree across an edge as well.
  auto shifts = importText(R"(func.func @f(%a: i4, %b: i4) -> i4 {
  %s = arith.shli %a, %b : i4
  %t = arith.shrsi %a, %b : i4
  %u = arith.shrui %a, %b : i4
  cf.br ^n(%s, %t, %u : i4, i4, i4)
^n(%x: i4, %y: i4, %z: i4):
  return %a : i4
})");
  for (int64_t a = 0; a < 16; ++a)
    for (int64_t b = 0; b < 16; ++b)
      checkRun(shifts, entryFor(shifts, {a, b}), 3);

  // Every undefined path: a zero divisor, overflow, poison branch.
  auto stuck = importText(R"(func.func @f(%a: i4, %b: i4) -> i4 {
  %r = arith.divsi %a, %b : i4
  %one = arith.constant 1 : i4
  %x = arith.addi %a, %one overflow<nsw> : i4
  %t = arith.cmpi eq, %x, %one : i4
  cf.cond_br %t, ^e, ^e
^e:
  return %r : i4
})");
  for (int64_t a = 0; a < 16; ++a)
    for (int64_t b = 0; b < 16; ++b)
      checkRun(stuck, entryFor(stuck, {a, b}), 3);

  // Loops, parallel edges, a switch, the branchy example.
  auto loop = importText(R"(func.func @f(%i0: i4, %n: i4) -> i4 {
  cf.br ^h(%i0 : i4)
^h(%i: i4):
  %c = arith.cmpi slt, %i, %n : i4
  cf.cond_br %c, ^b, ^exit
^b:
  %two = arith.constant 2 : i4
  %r = arith.remsi %i, %two : i4
  %z = arith.constant 0 : i4
  %e = arith.cmpi eq, %r, %z : i4
  %t = arith.constant 3 : i4
  %u = arith.constant 7 : i4
  %d = arith.select %e, %t, %u : i4
  %x = arith.addi %i, %d : i4
  cf.switch %r : i4, [default: ^h(%x : i4), 1: ^h(%x : i4)]
^exit:
  return %i : i4
})");
  for (int64_t i : {-8, -1, 0, 3})
    for (int64_t n : {-8, 0, 5, 7})
      checkRun(loop, entryFor(loop, {i, n}), 12);

  // Multi-dimensional memory, a store chain, dynamic extents and a fresh root.
  auto memory = importText(R"(func.func @f(%m: memref<2x3xi8>, %d: memref<?xi8>, %i: index, %j: index) -> i8 {
  %t = memref.alloca() : memref<2xi8>
  %z = arith.constant 0 : index
  %v = memref.load %m[%i, %j] : memref<2x3xi8>
  memref.store %v, %t[%z] : memref<2xi8>
  %w = memref.load %d[%i] : memref<?xi8>
  %s = arith.addi %v, %w : i8
  memref.store %s, %m[%z, %j] : memref<2x3xi8>
  memref.store %w, %m[%z, %j] : memref<2x3xi8>
  %q = memref.load %t[%z] : memref<2xi8>
  %r = memref.load %m[%z, %j] : memref<2x3xi8>
  %u = arith.addi %q, %r : i8
  return %u : i8
})");
  CHECK(memory.isSupported());
  for (int64_t i : {0, 1, 2})
    for (int64_t j : {0, 2, 3})
      checkRun(memory, entryFor(memory, {i, j},
                                {Root{{2, 3}, {1, 2, 3, 4, 5, 6}}, Root{{2}, {7, 8}}, Root{{2}, {}}}), 2);

  // Floats, as rounded values.
  auto floats = importText(R"(func.func @f(%m: memref<3xf32>, %x: f32) -> f32 {
  %z = arith.constant 0 : index
  %o = arith.constant 1 : index
  %a = memref.load %m[%z] : memref<3xf32>
  %b = memref.load %m[%o] : memref<3xf32>
  %p = arith.mulf %a, %b : f32
  %s = arith.addf %p, %x fastmath<nsz> : f32
  memref.store %s, %m[%o] : memref<3xf32>
  return %s : f32
})");
  CHECK(floats.isSupported());
  int64_t one = 0x3f800000, tiny = 0x33800000, huge = 0x7f7fffff, inf = 0x7f800000;
  for (int64_t a : {one, tiny, huge, inf})
    for (int64_t b : {one, tiny, huge, (int64_t)0})
      for (int64_t x : {one, tiny, (int64_t)0x80000000})
        checkRun(floats, entryFor(floats, {x}, {Root{{3}, {a, b, 0}}}), 2);

  // A MAC loop, with a float accumulator variable.
  auto scalar = importText(R"(func.func @f(%A: memref<3xf32>, %n: index) -> f32 {
  %z = arith.constant 0 : index
  %o = arith.constant 1 : index
  %s0 = arith.constant 0.0 : f32
  cf.br ^h(%z, %s0 : index, f32)
^h(%i: index, %s: f32):
  %t = arith.cmpi ult, %i, %n : index
  cf.cond_br %t, ^b, ^e
^b:
  %a = memref.load %A[%i] : memref<3xf32>
  %b = memref.load %A[%i] : memref<3xf32>
  %m = arith.mulf %a, %b : f32
  %u = arith.addf %s, %m : f32
  %x = arith.addi %i, %o : index
  cf.br ^h(%x, %u : index, f32)
^e:
  return %s : f32
})");
  checkRun(scalar, entryFor(scalar, {3}, {Root{{3}, {one, one, one}}}), 12);
  checkRun(scalar, entryFor(scalar, {4}, {Root{{3}, {one, one, one}}}), 16); // UB

  // An unsupported program has no relation.
  auto refused = importText("func.func @f(%a: f32) -> f32 {\n%r = arith.subf %a, %a : f32\nreturn %r : f32\n}");
  CHECK(encodeTransitionRelation(refused).find("define-fun") == std::string::npos);

  // Stable text.
  CHECK(encodeTransitionRelation(importText(divide)) ==
        encodeTransitionRelation(importText(divide)));

  printf("%d checks, %d failures, %u solver queries, %u uninitialized reads not compared\n",
         checks, failures, queries, skipped);
  return failures != 0;
}
