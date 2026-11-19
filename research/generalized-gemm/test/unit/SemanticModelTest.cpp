//===- SemanticModelTest.cpp - Invariants I1..I9 of the G1 model ----------===//
//
// Each section tests one invariant by trying to break it. Expected values come
// from arithmetic written here, on plain integers, not from the library.
//
//===----------------------------------------------------------------------===//
#include "TestSupport.h"

#include "mlir/IR/Verifier.h"

#include <functional>
#include <map>
#include <set>

using namespace gg;

static std::string wrapBody(const std::string &signature,
                            const std::string &result, const std::string &body) {
  return "func.func @f(" + signature + ") -> " + result + " {\n" + body + "\n}\n";
}

static RunResult run(const SemanticProgram &p, const std::vector<int64_t> &scalars,
                     const std::vector<Root> &roots = {}, uint64_t fuel = 100000) {
  RunLimits limits;
  limits.fuel = fuel;
  return runConcrete(p, entryFor(p, scalars, roots), limits);
}

//===----------------------------------------------------------------------===//
// Independent reference for the integer table.
//===----------------------------------------------------------------------===//

static int64_t toSigned(uint64_t v, unsigned w) {
  return (v >> (w - 1)) & 1 ? (int64_t)v - ((int64_t)1 << w) : (int64_t)v;
}
static uint64_t mask(int64_t v, unsigned w) {
  return (uint64_t)v & (((uint64_t)1 << w) - 1);
}

struct Expected {
  RunStatus status = RunStatus::Returned;
  uint64_t value = 0;
  bool poison = false;
};

static Expected reference(const std::string &op, const std::string &flag,
                          uint64_t a, uint64_t b, unsigned w) {
  Expected e;
  int64_t sa = toSigned(a, w), sb = toSigned(b, w);
  int64_t fits = (int64_t)1 << (w - 1);
  auto inSigned = [&](int64_t v) { return v >= -fits && v < fits; };
  auto inUnsigned = [&](int64_t v) { return v >= 0 && v < ((int64_t)1 << w); };
  if (op == "addi" || op == "subi" || op == "muli") {
    int64_t s = op == "addi" ? sa + sb : op == "subi" ? sa - sb : sa * sb;
    int64_t u = op == "addi" ? (int64_t)a + b
                             : op == "subi" ? (int64_t)a - b : (int64_t)a * b;
    e.value = mask(u, w);
    e.poison = (flag.find("nsw") != std::string::npos && !inSigned(s)) ||
               (flag.find("nuw") != std::string::npos && !inUnsigned(u));
  } else if (op == "divsi" || op == "remsi") {
    if (b == 0) {
      e.status = RunStatus::UndefinedBehavior;
    } else if (sa == -fits && sb == -1) {
      e.status = op == "divsi" ? RunStatus::UndefinedBehavior
                               : RunStatus::Underspecified;
    } else {
      e.value = mask(op == "divsi" ? sa / sb : sa % sb, w);
      e.poison = flag == "exact" && sa % sb != 0;
    }
  } else if (op == "divui" || op == "remui") {
    if (b == 0) {
      e.status = RunStatus::UndefinedBehavior;
    } else {
      e.value = op == "divui" ? a / b : a % b;
      e.poison = flag == "exact" && a % b != 0;
    }
  } else if (op == "shli" || op == "shrui" || op == "shrsi") {
    if (b >= w) {
      e.poison = true;
      return e;
    }
    if (op == "shli") {
      e.value = mask((int64_t)(a << b), w);
      int64_t mathematical = sa * ((int64_t)1 << b);
      int64_t unsignedResult = (int64_t)a << b;
      e.poison = (flag.find("nsw") != std::string::npos && !inSigned(mathematical)) ||
                 (flag.find("nuw") != std::string::npos && !inUnsigned(unsignedResult));
    } else if (op == "shrui") {
      e.value = a >> b;
      e.poison = flag == "exact" && (a & ((1u << b) - 1));
    } else {
      e.value = mask(sa >> b, w);
      e.poison = flag == "exact" && (a & ((1u << b) - 1));
    }
  } else if (op == "andi") {
    e.value = a & b;
  } else if (op == "ori") {
    e.value = a | b;
  } else if (op == "xori") {
    e.value = a ^ b;
  } else if (op == "minsi") {
    e.value = sa < sb ? a : b;
  } else if (op == "maxsi") {
    e.value = sa > sb ? a : b;
  } else if (op == "minui") {
    e.value = a < b ? a : b;
  } else if (op == "maxui") {
    e.value = a > b ? a : b;
  }
  return e;
}

/// Every operation, flag, width 1..4 and input pair.
static void testIntegerTable() {
  struct Row {
    const char *op;
    std::vector<std::string> flags;
  };
  const std::vector<std::string> overflow = {"", "overflow<nsw>", "overflow<nuw>",
                                             "overflow<nsw, nuw>"};
  const std::vector<Row> table = {
      {"addi", overflow}, {"subi", overflow}, {"muli", overflow},
      {"shli", overflow}, {"divsi", {"", "exact"}}, {"divui", {"", "exact"}},
      {"shrsi", {"", "exact"}}, {"shrui", {"", "exact"}}, {"remsi", {""}},
      {"remui", {""}}, {"andi", {""}}, {"ori", {""}}, {"xori", {""}},
      {"minsi", {""}}, {"maxsi", {""}}, {"minui", {""}}, {"maxui", {""}}};
  unsigned cases = 0;
  for (unsigned w = 1; w <= 4; ++w) {
    std::string t = "i" + std::to_string(w);
    for (const Row &row : table)
      for (const std::string &flag : row.flags) {
        auto p = importText(wrapBody(
            "%a: " + t + ", %b: " + t, t,
            "  %r = arith." + std::string(row.op) + " %a, %b " + flag + " : " + t +
                "\n  return %r : " + t));
        CHECK(p.isSupported());
        std::string flagName = flag.find("exact") != std::string::npos ? "exact" : flag;
        for (uint64_t a = 0; a < (1u << w); ++a)
          for (uint64_t b = 0; b < (1u << w); ++b) {
            Expected e = reference(row.op, flagName, a, b, w);
            RunResult r = run(p, {(int64_t)a, (int64_t)b});
            ++cases;
            bool ok = r.status == e.status;
            if (ok && e.status == RunStatus::Returned) {
              ok = r.returnPoison[0] == e.poison;
              // A poison value's bits are not semantics; only the flag is.
              if (ok && !e.poison)
                ok = r.returns[0].getZExtValue() == e.value;
            }
            if (!ok)
              fprintf(stderr, "table: %s %s i%u a=%llu b=%llu got %s\n", row.op,
                      flag.c_str(), w, (unsigned long long)a,
                      (unsigned long long)b, stringifyRunStatus(r.status));
            CHECK(ok);
          }
      }
  }
  printf("  integer table: %u cases\n", cases);
}

static void testCompareTable() {
  const char *names[] = {"eq", "ne", "slt", "sle", "sgt", "sge", "ult", "ule", "ugt", "uge"};
  for (unsigned w = 1; w <= 4; ++w) {
    std::string t = "i" + std::to_string(w);
    for (const char *name : names) {
      auto p = importText(wrapBody("%a: " + t + ", %b: " + t, "i1",
                                   std::string("  %r = arith.cmpi ") + name +
                                       ", %a, %b : " + t + "\n  return %r : i1"));
      for (uint64_t a = 0; a < (1u << w); ++a)
        for (uint64_t b = 0; b < (1u << w); ++b) {
          int64_t sa = toSigned(a, w), sb = toSigned(b, w);
          std::map<std::string, bool> truth = {
              {"eq", a == b}, {"ne", a != b}, {"slt", sa < sb}, {"sle", sa <= sb},
              {"sgt", sa > sb}, {"sge", sa >= sb}, {"ult", a < b}, {"ule", a <= b},
              {"ugt", a > b}, {"uge", a >= b}};
          RunResult r = run(p, {(int64_t)a, (int64_t)b});
          CHECK(r.status == RunStatus::Returned &&
                r.returns[0].isOne() == truth[name]);
        }
    }
  }
}

static void testCastTable() {
  // i3 -> i5 and i5 -> i3, every value, every flag.
  auto oneCast = [](const std::string &op, const std::string &flag, unsigned from,
                    unsigned to) {
    std::string a = "i" + std::to_string(from), b = "i" + std::to_string(to);
    return importText(wrapBody("%a: " + a, b, "  %r = arith." + op + " %a " + flag +
                                                  " : " + a + " to " + b +
                                                  "\n  return %r : " + b));
  };
  auto sext = oneCast("extsi", "", 3, 5), zext = oneCast("extui", "", 3, 5),
       nneg = oneCast("extui", "nneg", 3, 5), plain = oneCast("trunci", "", 5, 3),
       nsw = oneCast("trunci", "overflow<nsw>", 5, 3),
       nuw = oneCast("trunci", "overflow<nuw>", 5, 3);
  for (int64_t v = 0; v < 8; ++v) {
    CHECK(run(sext, {v}).returns[0].getZExtValue() == mask(toSigned(v, 3), 5));
    CHECK(run(zext, {v}).returns[0].getZExtValue() == (uint64_t)v);
    CHECK(run(nneg, {v}).returnPoison[0] == (v >= 4));
  }
  for (int64_t v = 0; v < 32; ++v) {
    CHECK(run(plain, {v}).returns[0].getZExtValue() == (uint64_t)(v & 7));
    CHECK(!run(plain, {v}).returnPoison[0]);
    CHECK(run(nuw, {v}).returnPoison[0] == (v >= 8));
    int64_t s = toSigned(v, 5);
    CHECK(run(nsw, {v}).returnPoison[0] == !(s >= -4 && s < 4));
  }
  for (const char *op : {"index_cast", "index_castui"}) {
    auto w = importText(wrapBody("%a: i8", "index", std::string("  %r = arith.") + op +
                                 " %a : i8 to index\n  return %r : index"));
    uint64_t expected = std::string(op) == "index_cast" ? ~(uint64_t)0x7f : 0x80;
    CHECK(run(w, {0x80}).returns[0] == APInt(64, expected));
  }
}

//===----------------------------------------------------------------------===//
// I1: exact width.
//===----------------------------------------------------------------------===//
static void testI1() {
  auto add8 = importText(wrapBody("%a: i8, %b: i8", "i8",
                                  "  %r = arith.addi %a, %b : i8\n  return %r : i8"));
  CHECK(run(add8, {127, 1}).returns[0].getZExtValue() == 128);
  CHECK(run(add8, {255, 1}).returns[0].isZero());
  CHECK(add8.transitions[0].returns[0]->sort == Sort::bv(8));
  CHECK(add8.transitions[0].returns[0]->aux == 0);
  auto nsw = importText(wrapBody("%a: i8, %b: i8", "i8",
      "  %r = arith.addi %a, %b overflow<nsw> : i8\n  return %r : i8"));
  CHECK(nsw.transitions[0].returns[0]->aux == FlagNSW); // Preserved on the node.
  CHECK(toJSON(nsw).find("add.nsw:bv8") != std::string::npos);

  TermContext c;
  Term a = c.param(0, Sort::bv(8)), b = c.param(1, Sort::bv(16));
  Term f = c.param(2, Sort::flt(32));
  Term m = c.memVar(Sort::mem(0, 1, 64, Sort::bv(8)));
  CHECK_THROWS(c.binary(TermOp::Add, a, b));
  CHECK_THROWS(c.icmp(Predicate::Eq, a, b));
  CHECK_THROWS(c.ite(a, a, a));
  CHECK_THROWS(c.ite(c.boolean(true), a, b));
  CHECK_THROWS(c.zext(a, 8));
  CHECK_THROWS(c.sext(a, 4));
  CHECK_THROWS(c.trunc(a, 8));
  CHECK_THROWS(c.binary(TermOp::Add, f, f));
  CHECK_THROWS(c.fbinary(TermOp::FAdd, a, a));
  CHECK_THROWS(c.load(m, {a}));             // Index width.
  CHECK_THROWS(c.load(m, {}));              // Rank.
  CHECK_THROWS(c.store(m, {c.bv(APInt(64, 0))}, b)); // Value sort.
  CHECK_THROWS(c.binary(TermOp::Add, a, a, FlagExact)); // Flag on the wrong op.
  CHECK_THROWS(c.binary(TermOp::SRem, a, a, FlagNSW));
  CHECK_THROWS(Sort::bv(0));
  CHECK_THROWS(Sort::flt(16));
  CHECK_THROWS(c.substitute(a, [&](Term) { return b; })); // Sort change.
  TermContext tiny(1);
  tiny.boolean(true);
  CHECK_THROWS(tiny.boolean(false));

  // `index` has no default width.
  std::string usesIndex = wrapBody("%a: index", "index", "  return %a : index");
  auto none = importText(usesIndex, 0);
  CHECK(!none.isSupported() &&
        none.unsupported->kind == UnsupportedKind::IndexWidthUnspecified);
  std::string wraps = wrapBody("%a: index", "index",
      "  %c = arith.constant 1 : index\n  %r = arith.addi %a, %c : index\n  return %r : index");
  auto i16 = importText(wraps, 16), i64 = importText(wraps, 64);
  CHECK(i16.indexBitWidth == 16 && i64.indexBitWidth == 64);
  CHECK(run(i16, {65535}).returns[0].isZero());
  CHECK(run(i64, {65535}).returns[0].getZExtValue() == 65536);
}

//===----------------------------------------------------------------------===//
// I2: definedness.
//===----------------------------------------------------------------------===//
static void testI2() {
  // Poison that is never used is not undefined behaviour.
  auto unused = importText(wrapBody("%a: i8", "i8",
      "  %c = arith.constant 1 : i8\n"
      "  %x = arith.addi %a, %c overflow<nsw> : i8\n  return %a : i8"));
  CHECK(run(unused, {127}).status == RunStatus::Returned);
  // A branch on it is, also after crossing a block boundary.
  auto branch = importText(wrapBody("%a: i8", "i8", R"(
  %c = arith.constant 1 : i8
  %x = arith.addi %a, %c overflow<nsw> : i8
  cf.br ^b(%x : i8)
^b(%y: i8):
  %z = arith.constant 0 : i8
  %t = arith.cmpi slt, %y, %z : i8
  cf.cond_br %t, ^c, ^d
^c:
  return %y : i8
^d:
  return %z : i8)"));
  CHECK(run(branch, {127}).status == RunStatus::UndefinedBehavior);
  CHECK(run(branch, {126}).status == RunStatus::Returned);
  // Through a cross-block result, not a block argument.
  auto crossing = importText(wrapBody("%a: i8", "i1", R"(
  %c = arith.constant 1 : i8
  %x = arith.addi %a, %c overflow<nuw> : i8
  cf.br ^b
^b:
  %t = arith.cmpi eq, %x, %c : i8
  cf.cond_br %t, ^c, ^c
^c:
  return %t : i1)"));
  CHECK(run(crossing, {255}).status == RunStatus::UndefinedBehavior);
  CHECK(run(crossing, {1}).status == RunStatus::Returned);
  // Only the selected arm's poison reaches a select.
  auto select = importText(wrapBody("%a: i8, %s: i1", "i8", R"(
  %c = arith.constant 1 : i8
  %x = arith.addi %a, %c overflow<nsw> : i8
  %r = arith.select %s, %x, %a : i8
  return %r : i8)"));
  CHECK(run(select, {127, 1}).returnPoison[0]);
  CHECK(!run(select, {127, 0}).returnPoison[0]);
  // A switch on poison.
  auto sw = importText(wrapBody("%a: i8", "i8", R"(
  %c = arith.constant 1 : i8
  %x = arith.addi %a, %c overflow<nsw> : i8
  cf.switch %x : i8, [default: ^b, 3: ^b]
^b:
  return %a : i8)"));
  CHECK(run(sw, {127}).status == RunStatus::UndefinedBehavior);
  // A poison divisor is undefined; a poison dividend gives poison, except by -1.
  auto poisoned = importText(wrapBody("%a: i8, %b: i8", "i8", R"(
  %c = arith.constant 1 : i8
  %x = arith.addi %a, %c overflow<nsw> : i8
  %r = arith.divsi %b, %x : i8
  return %r : i8)"));
  CHECK(run(poisoned, {127, 4}).status == RunStatus::UndefinedBehavior);
  auto dividend = importText(wrapBody("%a: i8, %b: i8", "i8", R"(
  %c = arith.constant 1 : i8
  %x = arith.addi %a, %c overflow<nsw> : i8
  %r = arith.divsi %x, %b : i8
  return %r : i8)"));
  CHECK(run(dividend, {127, 2}).returnPoison[0]);
  CHECK(run(dividend, {127, -1}).status == RunStatus::UndefinedBehavior);

  // The largest supported width: overflow is computed without wrapping again.
  auto mul64 = importText(wrapBody("%a: i64, %b: i64", "i64",
      "  %r = arith.muli %a, %b overflow<nsw> : i64\n  return %r : i64"));
  auto umul64 = importText(wrapBody("%a: i64, %b: i64", "i64",
      "  %r = arith.muli %a, %b overflow<nuw> : i64\n  return %r : i64"));
  auto add64 = importText(wrapBody("%a: i64, %b: i64", "i64",
      "  %r = arith.addi %a, %b overflow<nsw> : i64\n  return %r : i64"));
  int64_t big = INT64_MAX, half = (int64_t)1 << 32;
  CHECK(run(mul64, {big, 2}).returnPoison[0]);
  CHECK(run(mul64, {half, half}).returnPoison[0]);
  // The largest square that fits, and the next one.
  CHECK(!run(mul64, {3037000499, 3037000499}).returnPoison[0]);
  CHECK(run(mul64, {3037000500, 3037000500}).returnPoison[0]);
  CHECK(!run(mul64, {INT64_MIN, 1}).returnPoison[0]);
  CHECK(run(mul64, {INT64_MIN, -1}).returnPoison[0]);
  CHECK(run(umul64, {half, half}).returnPoison[0]);
  CHECK(!run(umul64, {big, 2}).returnPoison[0]); // 2^64 - 2 fits unsigned.
  CHECK(run(add64, {big, 1}).returnPoison[0]);
  CHECK(!run(add64, {big, -1}).returnPoison[0]);

  // Memory.
  std::string oneLoad = wrapBody("%m: memref<4xi8>, %i: index", "i8",
      "  %r = memref.load %m[%i] : memref<4xi8>\n  return %r : i8");
  auto load = importText(oneLoad);
  Root cells{{4}, {10, 20, 30, 40}};
  CHECK(run(load, {3}, {cells}).returns[0].getZExtValue() == 40);
  CHECK(run(load, {4}, {cells}).status == RunStatus::UndefinedBehavior);
  CHECK(run(load, {-1}, {cells}).status == RunStatus::UndefinedBehavior);
  auto store = importText(wrapBody("%m: memref<4xi8>, %i: index", "i8", R"(
  %c = arith.constant 7 : i8
  memref.store %c, %m[%i] : memref<4xi8>
  return %c : i8)"));
  CHECK(run(store, {4}, {cells}).status == RunStatus::UndefinedBehavior);
  CHECK(run(store, {1}, {cells}).memory[0].cells[1]->getZExtValue() == 7);
  auto dynamic = importText(wrapBody("%m: memref<?xi8>, %i: index", "i8",
      "  %r = memref.load %m[%i] : memref<?xi8>\n  return %r : i8"));
  CHECK(dynamic.parameters.size() == 2);
  CHECK(dynamic.parameters[1].kind == Parameter::Dimension);
  CHECK(run(dynamic, {1}, {Root{{2}, {5, 6}}}).returns[0].getZExtValue() == 6);
  CHECK(run(dynamic, {2}, {Root{{2}, {5, 6}}}).status == RunStatus::UndefinedBehavior);
  auto poisonIndex = importText(wrapBody("%m: memref<4xi8>, %i: index", "i8", R"(
  %c = arith.constant 1 : index
  %x = arith.addi %i, %c overflow<nuw> : index
  %r = memref.load %m[%x] : memref<4xi8>
  return %r : i8)"));
  CHECK(run(poisonIndex, {-1}, {cells}).status == RunStatus::UndefinedBehavior);
  auto poisonStored = importText(wrapBody("%m: memref<4xi8>, %a: i8", "i8", R"(
  %c = arith.constant 1 : i8
  %x = arith.addi %a, %c overflow<nsw> : i8
  %i = arith.constant 0 : index
  memref.store %x, %m[%i] : memref<4xi8>
  return %c : i8)"));
  CHECK(run(poisonStored, {127}, {cells}).status == RunStatus::ModelLimit);
  // Uninitialized memory is not a value.
  auto fresh = importText(wrapBody("%i: index", "i8", R"(
  %m = memref.alloca() : memref<2xi8>
  %c = arith.constant 9 : i8
  %z = arith.constant 0 : index
  memref.store %c, %m[%z] : memref<2xi8>
  %r = memref.load %m[%i] : memref<2xi8>
  return %r : i8)"));
  CHECK(run(fresh, {0}, {Root{{2}, {}}}).returns[0].getZExtValue() == 9);
  CHECK(run(fresh, {1}, {Root{{2}, {}}}).status == RunStatus::Underspecified);

  // Floating point, strictly IEEE.
  int64_t one = 0x3f800000, two = 0x40000000, inf = 0x7f800000, nan = 0x7fc00000;
  int64_t negZero = 0x80000000;
  auto arithmetic = [](const std::string &op, const std::string &flags) {
    return importText(wrapBody("%a: f32, %b: f32", "f32", "  %r = arith." + op +
                               " %a, %b " + flags + " : f32\n  return %r : f32"));
  };
  auto addf = arithmetic("addf", ""), mulf = arithmetic("mulf", "");
  CHECK(run(addf, {one, one}).returns[0].getZExtValue() == (uint64_t)two);
  CHECK(run(addf, {negZero, negZero}).returns[0].getZExtValue() == (uint64_t)negZero);
  CHECK(run(addf, {0, negZero}).returns[0].isZero()); // +0 + -0 is +0.
  // 1 + 2^-24 is a tie: to even.
  CHECK(run(addf, {one, 0x33800000}).returns[0].getZExtValue() == (uint64_t)one);
  CHECK(run(mulf, {inf, 0}).status == RunStatus::Underspecified); // NaN result.
  CHECK(run(addf, {nan, one}).status == RunStatus::Underspecified);
  auto nnan = arithmetic("addf", "fastmath<nnan>");
  CHECK(run(nnan, {nan, one}).returnPoison[0]);
  auto ninf = arithmetic("addf", "fastmath<ninf>");
  CHECK(run(ninf, {inf, one}).returnPoison[0]);
  auto nsz = arithmetic("addf", "fastmath<nsz>");
  CHECK(run(nsz, {0, 0}).status == RunStatus::Underspecified);
  CHECK(!run(addf, {one, one}).strictRefinementOfPermissiveFloat);
  auto reassoc = arithmetic("addf", "fastmath<reassoc>");
  RunResult permissive = run(reassoc, {one, one});
  CHECK(permissive.status == RunStatus::Returned &&
        permissive.strictRefinementOfPermissiveFloat);
  auto f64 = importText(wrapBody("%a: f64, %b: f64", "f64",
      "  %r = arith.mulf %a, %b : f64\n  return %r : f64"));
  CHECK(run(f64, {0x4000000000000000, 0x4000000000000000}).returns[0].getZExtValue() ==
        0x4010000000000000u);
}

//===----------------------------------------------------------------------===//
// I3: fail-closed.
//===----------------------------------------------------------------------===//
static void checkRefused(const SemanticProgram &p, UnsupportedKind kind,
                         std::set<UnsupportedKind> &seen) {
  CHECK(!p.isSupported());
  if (p.isSupported())
    return;
  if (p.unsupported->kind != kind)
    fprintf(stderr, "expected %s got %s: %s\n", stringifyUnsupportedKind(kind),
            stringifyUnsupportedKind(p.unsupported->kind),
            p.unsupported->detail.c_str());
  CHECK(p.unsupported->kind == kind);
  seen.insert(kind);
  // No partial model of any kind.
  CHECK(p.parameters.empty() && p.state.empty() && p.roots.empty());
  CHECK(p.transitions.empty() && p.sites.empty() && p.rootPairs.empty());
  CHECK(p.assumptions.empty() && p.locationCount == 0);
  CHECK(!p.controlOblivious && !p.addressOblivious);
  CHECK(runConcrete(p, EntryState{}).status == RunStatus::InvalidEntry);
  CHECK(!summarizeAcyclic(p, 0).complete);
  std::string json = toJSON(p);
  CHECK(json.find("UNSUPPORTED") != std::string::npos &&
        json.find("transitions") == std::string::npos);
}

static void testI3() {
  using K = UnsupportedKind;
  std::set<K> seen;
  auto refused = [&](const std::string &text, K kind, unsigned index = 64,
                     bool structured = false) {
    checkRefused(importText(text, index, structured), kind, seen);
  };
  refused("func.func @f() -> i32 { return }", K::NotVerified);
  refused("func.func private @f()", K::ExternalFunction);
  refused(wrapBody("%c: i1", "i32", R"(
  %r = scf.if %c -> i32 {
    %x = arith.constant 1 : i32
    scf.yield %x : i32
  } else {
    %x = arith.constant 2 : i32
    scf.yield %x : i32
  }
  return %r : i32)"), K::NestedRegion);
  refused(wrapBody("%a: f32", "f32", "  %r = arith.subf %a, %a : f32\n  return %r : f32"),
          K::UnsupportedOperation);
  refused(wrapBody("%a: f32", "i1", "  %r = arith.cmpf olt, %a, %a : f32\n  return %r : i1"),
          K::UnsupportedOperation);
  refused(wrapBody("%a: i32", "i32", R"(  %r = "foo.bar"(%a) : (i32) -> i32
  return %r : i32)"), K::UnsupportedOperation);
  refused(wrapBody("%a: i8", "i8", "  %r = arith.ceildivsi %a, %a : i8\n  return %r : i8"),
          K::UnsupportedOperation);
  refused(wrapBody("%a: i32", "i32", "  %r = arith.addi %a, %a {mystery} : i32\n  return %r : i32"),
          K::UnsupportedAttribute);
  refused("func.func @f(%a: i32) -> i32 attributes {mustprogress} { return %a : i32 }",
          K::UnsupportedAttribute);
  refused("module attributes {dlti.dl_spec = 1} { func.func @f(%a: i32) -> i32 { return %a : i32 } }",
          K::UnsupportedAttribute);
  refused("func.func @f(%a: i32 {llvm.signext}) -> i32 { return %a : i32 }",
          K::UnsupportedAttribute);
  refused(wrapBody("%a: f32", "f32", "  %r = arith.addf %a, %a to_nearest_away : f32\n  return %r : f32"),
          K::UnsupportedAttribute);
  refused(wrapBody("%m: memref<2xi8>, %i: index", "i8",
          "  %r = \"memref.load\"(%m, %i) <{invariant = true}> : (memref<2xi8>, index) -> i8\n  return %r : i8"),
          K::UnsupportedAttribute);
  refused(wrapBody("%a: vector<4xi32>", "i32", R"(
  %c = arith.constant 0 : i32
  return %c : i32)"), K::UnsupportedType);
  refused(wrapBody("%a: i128", "i128", "  return %a : i128"), K::UnsupportedType);
  refused(wrapBody("%a: f16", "f16", "  return %a : f16"), K::UnsupportedType);
  refused(wrapBody("%a: memref<*xi8>", "i8", R"(
  %c = arith.constant 0 : i8
  return %c : i8)"), K::UnsupportedType);
  refused("func.func @f() { llvm.unreachable }", K::UnsupportedTerminator);
  refused(wrapBody("%a: index", "index", "  return %a : index"),
          K::IndexWidthUnspecified, 0);
  refused(wrapBody("%m: memref<2xi8>", "i8", R"(
  %c = arith.constant 0 : i8
  return %c : i8)"), K::IndexWidthUnspecified, 0);
  refused(wrapBody("", "index", "  %c = arith.constant 70000 : index\n  return %c : index"),
          K::ConstantOutOfRange, 16);
  refused(wrapBody("%m: memref<2xi8, affine_map<(d0) -> (d0 + 1)>>", "i8", R"(
  %c = arith.constant 0 : i8
  return %c : i8)"), K::NonIdentityLayout);
  refused(wrapBody("%m: memref<2xi8, 1>", "i8", R"(
  %c = arith.constant 0 : i8
  return %c : i8)"), K::NonDefaultMemorySpace);
  refused(wrapBody("%m: memref<4xi8>", "i8", R"(
  %s = memref.subview %m[1] [2] [1] : memref<4xi8> to memref<2xi8, strided<[1], offset: 1>>
  %z = arith.constant 0 : index
  %r = memref.load %s[%z] : memref<2xi8, strided<[1], offset: 1>>
  return %r : i8)"), K::UnsupportedView);
  refused(wrapBody("%m: memref<?xi8>", "i8", R"(
  %s = memref.cast %m : memref<?xi8> to memref<4xi8>
  %z = arith.constant 0 : index
  %r = memref.load %s[%z] : memref<4xi8>
  return %r : i8)"), K::UnsupportedView);
  refused(wrapBody("%m: memref<2xi8>", "i8", R"(
  cf.br ^b(%m : memref<2xi8>)
^b(%x: memref<2xi8>):
  %z = arith.constant 0 : index
  %r = memref.load %x[%z] : memref<2xi8>
  return %r : i8)"), K::MemrefBlockArgument);
  refused(wrapBody("%m: memref<2xi8>", "memref<2xi8>", "  return %m : memref<2xi8>"),
          K::MemrefReturn);
  refused(wrapBody("%c: i1", "i8", R"(
  cf.br ^b
^b:
  %m = memref.alloc() : memref<2xi8>
  cf.cond_br %c, ^b, ^e
^e:
  %v = arith.constant 0 : i8
  return %v : i8)"), K::AllocationInCycle);
  refused(wrapBody("%n: index", "i8", R"(
  %m = memref.alloc(%n) : memref<?xi8>
  %v = arith.constant 0 : i8
  return %v : i8)"), K::DynamicAllocation);
  refused(wrapBody("%m: memref<2xi8>, %d: index", "index", R"(
  %r = memref.dim %m, %d : memref<2xi8>
  return %r : index)"), K::DynamicDimensionIndex);
  refused(R"(func.func @f(%a: i32) -> i32 {
  %r = call @g(%a) : (i32) -> i32
  return %r : i32
}
func.func @g(%a: i32) -> i32 {
  %r = call @f(%a) : (i32) -> i32
  return %r : i32
})", K::RecursiveCall);
  refused(R"(func.func private @g(i32) -> i32
func.func @f(%a: i32) -> i32 {
  %r = call @g(%a) : (i32) -> i32
  return %r : i32
})", K::ExternalCall);
  refused(R"(func.func @g(%a: i32) -> i32 { return %a : i32 }
func.func @f(%a: i32) -> i32 {
  %h = constant @g : (i32) -> i32
  %r = call_indirect %h(%a) : (i32) -> i32
  return %r : i32
})", K::IndirectCall);
  refused(R"(func.func @g(%a: i1) -> i1 {
  cf.br ^b
^b:
  cf.cond_br %a, ^b, ^e
^e:
  return %a : i1
}
func.func @f(%a: i1) -> i1 {
  %r = call @g(%a) : (i1) -> i1
  return %r : i1
})", K::CyclicCallee);
  // scf.forall with shared outputs has no lowering, so the upstream pass fails.
  refused(wrapBody("%n: index, %t: tensor<4xi8>", "tensor<4xi8>", R"(
  %r = scf.forall (%i) in (%n) shared_outs(%o = %t) -> (tensor<4xi8>) {
    scf.forall.in_parallel {
    }
  }
  return %r : tensor<4xi8>)"), K::LoweringFailed, 64, true);

  std::string small = wrapBody("%a: i32", "i32", R"(
  cf.br ^b
^b:
  %x = arith.addi %a, %a : i32
  cf.br ^c
^c:
  return %x : i32)");
  ImportLimits limits;
  limits.maxBlocks = 2;
  checkRefused(importText(small, 64, false, "f", limits), K::ResourceLimit, seen);
  limits = {};
  limits.maxOperations = 2;
  checkRefused(importText(small, 64, false, "f", limits), K::ResourceLimit, seen);
  limits = {};
  limits.maxStates = 0;
  checkRefused(importText(small, 64, false, "f", limits), K::ResourceLimit, seen);
  limits = {};
  limits.maxTerms = 2;
  checkRefused(importText(small, 64, false, "f", limits), K::ResourceLimit, seen);
  limits = {};
  limits.maxInlinedCalls = 0;
  checkRefused(importText(R"(func.func @g() { return }
func.func @f() {
  call @g() : () -> ()
  return
})", 64, false, "f", limits), K::ResourceLimit, seen);

  refused(wrapBody("%v: i8", "i8", R"(
  cf.switch %v : i8, [default: ^t, 1: ^t, 1: ^t]
^t:
  return %v : i8)"), K::UnsupportedTerminator);

  // Every enumerator has a test above.
  for (int k = (int)K::NotVerified; k <= (int)K::ResourceLimit; ++k) {
    if (!seen.count((K)k))
      fprintf(stderr, "no test refuses with %s\n", stringifyUnsupportedKind((K)k));
    CHECK(seen.count((K)k));
  }

  // The whitelist covers reachable blocks only: a dead block has no semantics.
  auto dead = importText(wrapBody("%a: i32", "i32", R"(
  return %a : i32
^dead:
  %r = "foo.bar"(%a) : (i32) -> i32
  return %r : i32)"));
  CHECK(dead.isSupported() && dead.locationCount == 1);
  // A false hint is no claim.
  auto hint = importText(wrapBody("%m: memref<2xi8>, %i: index", "i8",
      "  %r = memref.load %m[%i] : memref<2xi8>\n  return %r : i8"));
  CHECK(hint.isSupported());
}

//===----------------------------------------------------------------------===//
// I4: obliviousness is computed.
//===----------------------------------------------------------------------===//
static void testI4() {
  std::string mac = R"(
  %c = memref.load %C[%i] : memref<4xf32>
  %a = memref.load %A[%i] : memref<4xf32>
  %b = memref.load %B[%i] : memref<4xf32>
  %m = arith.mulf %a, %b : f32
  %s = arith.addf %c, %m : f32
  memref.store %s, %C[%i] : memref<4xf32>)";
  std::string roots = "%A: memref<4xf32>, %B: memref<4xf32>, %C: memref<4xf32>";
  std::string loop = R"(
  %z = arith.constant 0 : index
  %one = arith.constant 1 : index
  cf.br ^h(%z : index)
^h(%i: index):
  %t = arith.cmpi slt, %i, %n : index
  cf.cond_br %t, ^b, ^e
^b:)" + mac + R"(
  %next = arith.addi %i, %one : index
  cf.br ^h(%next : index)
^e:
  return)";
  auto clean = importText("func.func @f(" + roots + ", %n: index) {" + loop + "\n}");
  CHECK(clean.isSupported() && clean.controlOblivious && clean.addressOblivious);
  CHECK(clean.sites.size() == 1 && clean.sites[0].oblivious);

  // The bound comes from memory, directly in the guard's block.
  auto bound = importText("func.func @f(" + roots + R"(, %L: memref<index>) {
  %z = arith.constant 0 : index
  %one = arith.constant 1 : index
  cf.br ^h(%z : index)
^h(%i: index):
  %n = memref.load %L[] : memref<index>
  %t = arith.cmpi slt, %i, %n : index
  cf.cond_br %t, ^b, ^e
^b:)" + mac + R"(
  %next = arith.addi %i, %one : index
  cf.br ^h(%next : index)
^e:
  return
})");
  CHECK(bound.isSupported() && !bound.controlOblivious);
  CHECK(bound.sites.size() == 1 && !bound.sites[0].oblivious);

  // The dependence reaches the guard only through a block argument.
  auto carried = importText("func.func @f(" + roots + R"(, %L: memref<index>) {
  %n = memref.load %L[] : memref<index>
  %z = arith.constant 0 : index
  %one = arith.constant 1 : index
  cf.br ^h(%z, %n : index, index)
^h(%i: index, %limit: index):
  %t = arith.cmpi slt, %i, %limit : index
  cf.cond_br %t, ^b, ^e
^b:)" + mac + R"(
  %next = arith.addi %i, %one : index
  cf.br ^h(%next, %limit : index, index)
^e:
  return
})");
  CHECK(carried.isSupported() && !carried.controlOblivious);

  // A cross-block result, then used in an address only.
  auto indirect = importText("func.func @f(%A: memref<4xf32>, %B: memref<4xf32>, %C: memref<4xf32>, %L: memref<index>) {" R"(
  %i = memref.load %L[] : memref<index>
  cf.br ^b
^b:)" + mac + R"(
  return
})");
  CHECK(indirect.isSupported() && indirect.controlOblivious);
  CHECK(!indirect.addressOblivious && !indirect.sites[0].oblivious);

  // A data-dependent address outside the MAC still clears every site.
  auto elsewhere = importText("func.func @f(" + roots + R"(, %L: memref<index>, %i: index) {)" + mac + R"(
  %j = memref.load %L[] : memref<index>
  %c0 = arith.constant 0.0 : f32
  memref.store %c0, %A[%j] : memref<4xf32>
  return
})");
  CHECK(elsewhere.isSupported() && !elsewhere.sites[0].oblivious);

  // Poison flows carry the same dependence through the shadow.
  auto shadow = importText(wrapBody("%L: memref<i8>", "i8", R"(
  %x = memref.load %L[] : memref<i8>
  %one = arith.constant 1 : i8
  %y = arith.addi %x, %one overflow<nsw> : i8
  cf.br ^h
^h:
  %t = arith.cmpi eq, %one, %one : i8
  %u = arith.select %t, %y, %one : i8
  %v = arith.cmpi eq, %u, %one : i8
  cf.cond_br %v, ^e, ^e
^e:
  return %one : i8)"));
  CHECK(shadow.isSupported() && !shadow.controlOblivious);

  // A parameter is not memory.
  auto dim = importText(wrapBody("%m: memref<?xi8>", "index", R"(
  %z = arith.constant 0 : index
  %d = memref.dim %m, %z : memref<?xi8>
  %t = arith.cmpi eq, %d, %z : index
  cf.cond_br %t, ^e, ^e
^e:
  return %d : index)"));
  CHECK(dim.isSupported() && dim.controlOblivious);
}

//===----------------------------------------------------------------------===//
// I5: simultaneous edges.
//===----------------------------------------------------------------------===//
static void testI5() {
  // The classic swap: sequential assignment would give (b, b).
  auto swap = importText(wrapBody("%a: i8, %b: i8, %n: i8", "i8", R"(
  %z = arith.constant 0 : i8
  %one = arith.constant 1 : i8
  cf.br ^h(%a, %b, %z : i8, i8, i8)
^h(%x: i8, %y: i8, %i: i8):
  %t = arith.cmpi ult, %i, %n : i8
  cf.cond_br %t, ^b, ^e
^b:
  %next = arith.addi %i, %one : i8
  cf.br ^h(%y, %x, %next : i8, i8, i8)
^e:
  return %x : i8)"));
  CHECK(run(swap, {3, 5, 0}).returns[0].getZExtValue() == 3);
  CHECK(run(swap, {3, 5, 1}).returns[0].getZExtValue() == 5);
  CHECK(run(swap, {3, 5, 2}).returns[0].getZExtValue() == 3);
  CHECK(run(swap, {3, 5, 3}).returns[0].getZExtValue() == 5);

  // Fibonacci: each update reads the old pair.
  auto fib = importText(wrapBody("%n: i8", "i8", R"(
  %z = arith.constant 0 : i8
  %one = arith.constant 1 : i8
  cf.br ^h(%z, %one, %z : i8, i8, i8)
^h(%a: i8, %b: i8, %i: i8):
  %t = arith.cmpi ult, %i, %n : i8
  cf.cond_br %t, ^l, ^e
^l:
  %s = arith.addi %a, %b : i8
  %next = arith.addi %i, %one : i8
  cf.br ^h(%b, %s, %next : i8, i8, i8)
^e:
  return %a : i8)"));
  CHECK(run(fib, {10}).returns[0].getZExtValue() == 55);

  // Parallel edges stay distinct: same target, different operands.
  auto parallel = importText(wrapBody("%c: i1", "i8", R"(
  %x = arith.constant 1 : i8
  %y = arith.constant 2 : i8
  cf.cond_br %c, ^b(%x : i8), ^b(%y : i8)
^b(%v: i8):
  return %v : i8)"));
  CHECK(parallel.transitions.size() == 3);
  CHECK(run(parallel, {1}).returns[0].getZExtValue() == 1);
  CHECK(run(parallel, {0}).returns[0].getZExtValue() == 2);

  // A switch with a duplicate-free table, every arm.
  auto sw = importText(wrapBody("%v: i8", "i8", R"(
  %a = arith.constant 10 : i8
  %b = arith.constant 20 : i8
  %c = arith.constant 30 : i8
  cf.switch %v : i8, [default: ^t(%a : i8), 1: ^t(%b : i8), 2: ^t(%c : i8)]
^t(%r: i8):
  return %r : i8)"));
  CHECK(run(sw, {0}).returns[0].getZExtValue() == 10);
  CHECK(run(sw, {1}).returns[0].getZExtValue() == 20);
  CHECK(run(sw, {2}).returns[0].getZExtValue() == 30);
}

//===----------------------------------------------------------------------===//
// I6: the symbolic evaluator agrees with the concrete one.
//===----------------------------------------------------------------------===//

/// From one configuration: exactly one path holds, and running the concrete
/// evaluator along it reproduces that path's state, memory and result.
static void checkPathsAgree(const SemanticProgram &p, const EntryState &entry,
                            const Configuration &config) {
  SymbolicRegion region = summarizeAcyclic(p, config.location);
  CHECK(region.complete);
  const SymbolicPath *chosen = nullptr;
  unsigned holding = 0;
  for (const SymbolicPath &path : region.paths) {
    auto holds = evaluateTerm(p, entry, config, path.condition);
    if (!holds)
      return; // Depends on a value this configuration does not define.
    if (holds->isOne()) {
      ++holding;
      chosen = &path;
    }
  }
  CHECK(holding == 1); // The path conditions partition.
  if (!chosen)
    return;
  Configuration running = config;
  std::vector<APInt> returns;
  std::vector<bool> poison;
  for (unsigned index : chosen->transitions) {
    StepOutcome step = stepConfiguration(p, entry, running, &returns, &poison);
    if (step.stopped) {
      // The concrete run stopped inside the path: the path's own side
      // conditions must say so.
      bool stuck = false;
      for (const SideCondition &side : chosen->sideConditions) {
        auto v = evaluateTerm(p, entry, config, side.violated);
        if (v && v->isOne())
          stuck = true;
      }
      CHECK(stuck || step.stopped == RunStatus::Underspecified);
      return;
    }
    CHECK(step.transition == index);
  }
  for (const SideCondition &side : chosen->sideConditions) {
    auto v = evaluateTerm(p, entry, config, side.violated);
    CHECK(v && v->isZero());
  }
  CHECK(running.location == chosen->end);
  for (unsigned s = 0; s < p.state.size(); ++s) {
    if (chosen->state[s]) {
      auto v = evaluateTerm(p, entry, config, chosen->state[s]);
      CHECK(v && running.state[s] && *v == *running.state[s]);
    }
    if (chosen->shadow[s]) {
      auto v = evaluateTerm(p, entry, config, chosen->shadow[s]);
      CHECK(v && v->isOne() == running.shadow[s]);
    }
  }
  for (const MemoryRoot &root : p.roots) {
    Term memory = chosen->memory[root.id];
    if (!memory)
      continue;
    const RootData &data = running.memory[root.id];
    for (uint64_t offset = 0; offset < data.cells.size(); ++offset) {
      if (!data.cells[offset])
        continue;
      llvm::SmallVector<Term, 3> key(data.shape.size());
      uint64_t rest = offset;
      for (int d = (int)data.shape.size() - 1; d >= 0; --d) {
        key[d] = p.terms->bv(APInt(root.sort.indexWidth, rest % data.shape[d]));
        rest /= data.shape[d];
      }
      auto v = evaluateTerm(p, entry, config, p.terms->load(memory, key));
      CHECK(v && *v == *data.cells[offset]);
    }
  }
  for (unsigned i = 0; i < chosen->returns.size(); ++i) {
    auto v = evaluateTerm(p, entry, config, chosen->returns[i]);
    auto q = evaluateTerm(p, entry, config, chosen->returnPoison[i]);
    CHECK(v && q && q->isOne() == poison[i]);
    if (!poison[i])
      CHECK(*v == returns[i]);
  }
}

static void walkAndCheck(const SemanticProgram &p, const EntryState &entry,
                         unsigned maxSteps) {
  std::string error;
  auto config = initialConfiguration(p, entry, RunLimits{}, error);
  CHECK(config.has_value());
  for (unsigned i = 0; config && i < maxSteps; ++i) {
    if (config->location == p.exitLocation())
      return;
    checkPathsAgree(p, entry, *config);
    if (stepConfiguration(p, entry, *config).stopped)
      return;
  }
}

static void testI6Symbolic() {
  // i%2 ? +7 : +3, block form: two paths from the header, both revisit it.
  std::string branchy = wrapBody("%i0: i4, %n: i4", "i4", R"(
  cf.br ^h(%i0 : i4)
^h(%i: i4):
  %c = arith.cmpi slt, %i, %n : i4
  cf.cond_br %c, ^b, ^exit
^b:
  %two = arith.constant 2 : i4
  %r = arith.remsi %i, %two : i4
  %z = arith.constant 0 : i4
  %e = arith.cmpi eq, %r, %z : i4
  cf.cond_br %e, ^a, ^o
^a:
  %t = arith.constant 3 : i4
  %x = arith.addi %i, %t : i4
  cf.br ^h(%x : i4)
^o:
  %s = arith.constant 7 : i4
  %y = arith.addi %i, %s : i4
  cf.br ^h(%y : i4)
^exit:
  return %i : i4)");
  auto p = importText(branchy);
  CHECK(p.isSupported());
  SymbolicRegion region = summarizeAcyclic(p, 1);
  CHECK(region.complete && region.paths.size() == 3);
  unsigned revisits = 0, exits = 0;
  for (const SymbolicPath &path : region.paths)
    (path.stop == SymbolicPath::ReachedRevisit ? revisits : exits)++;
  CHECK(revisits == 2 && exits == 1); // Loops stop, they are not summarized.
  for (int64_t a = 0; a < 16; ++a)
    for (int64_t b = 0; b < 16; ++b)
      walkAndCheck(p, entryFor(p, {a, b}), 40);

  // With memory and poison in the path.
  auto mixed = importText(wrapBody("%m: memref<3xi8>, %a: i8", "i8", R"(
  %z = arith.constant 0 : index
  %o = arith.constant 1 : index
  %v = memref.load %m[%z] : memref<3xi8>
  memref.store %a, %m[%z] : memref<3xi8>
  %c = arith.cmpi slt, %v, %a : i8
  cf.cond_br %c, ^l, ^r
^l:
  %w = memref.load %m[%z] : memref<3xi8>
  %s = arith.addi %w, %v overflow<nsw> : i8
  memref.store %s, %m[%o] : memref<3xi8>
  cf.br ^j(%s : i8)
^r:
  %q = arith.divsi %a, %v : i8
  cf.br ^j(%q : i8)
^j(%r: i8):
  %u = memref.load %m[%o] : memref<3xi8>
  return %r : i8)"));
  CHECK(mixed.isSupported());
  for (int64_t a : {0, 1, 2, 100, 127, -1, -128})
    for (int64_t v : {0, 1, 100, 127, -1, -128})
      walkAndCheck(mixed, entryFor(mixed, {a}, {Root{{3}, {v, 5, 6}}}), 5);

  // Limits: incompleteness returns no paths, rather than some.
  std::string diamonds = "func.func @f(%c: i1) {\n";
  for (int i = 0; i < 12; ++i)
    diamonds += "  cf.cond_br %c, ^a" + std::to_string(i) + ", ^b" + std::to_string(i) +
                "\n^a" + std::to_string(i) + ":\n  cf.br ^j" + std::to_string(i) +
                "\n^b" + std::to_string(i) + ":\n  cf.br ^j" + std::to_string(i) +
                "\n^j" + std::to_string(i) + ":\n";
  diamonds += "  return\n}\n";
  auto wide = importText(diamonds);
  CHECK(wide.isSupported());
  SymbolicRegion limited = summarizeAcyclic(wide, 0);
  CHECK(!limited.complete && limited.paths.empty() && !limited.reason.empty());
  SymbolicLimits more;
  more.maxPaths = 5000;
  SymbolicRegion full = summarizeAcyclic(wide, 0, more);
  CHECK(full.complete && full.paths.size() == 4096);
  SymbolicLimits shortPath;
  shortPath.maxPathLength = 2;
  CHECK(!summarizeAcyclic(wide, 0, shortPath).complete);
  SymbolicLimits fewTerms;
  fewTerms.maxNewTerms = 0;
  // Terms are shared across calls, so this needs a program not yet summarized.
  CHECK(!summarizeAcyclic(importText(branchy), 1, fewTerms).complete);
  CHECK(!summarizeAcyclic(p, 99).complete);
}

//===----------------------------------------------------------------------===//
// I7: read-only and deterministic.
//===----------------------------------------------------------------------===//
static std::string print(mlir::Operation *op) {
  std::string text;
  llvm::raw_string_ostream stream(text);
  op->print(stream, mlir::OpPrintingFlags().printGenericOpForm().enableDebugInfo());
  stream.flush();
  return text;
}

static const char *affineGemm = R"(
func.func @f(%A: memref<?x?xf32>, %B: memref<?x?xf32>, %C: memref<?x?xf32>, %m: index, %n: index, %p: index) {
  affine.for %i = 0 to %m {
    affine.for %j = 0 to %n {
      affine.for %k = 0 to %p {
        %a = affine.load %A[%i, %k] : memref<?x?xf32>
        %b = affine.load %B[%k, %j] : memref<?x?xf32>
        %c = affine.load %C[%i, %j] : memref<?x?xf32>
        %d = arith.mulf %a, %b : f32
        %e = arith.addf %c, %d : f32
        affine.store %e, %C[%i, %j] : memref<?x?xf32>
      }
    }
  }
  return
})";

static void testI7() {
  const char *withCall = R"(
func.func @g(%a: i32) -> i32 {
  %c = arith.constant 0 : i32
  %t = arith.cmpi slt, %a, %c : i32
  cf.cond_br %t, ^n, ^p
^n:
  %x = arith.subi %c, %a : i32
  return %x : i32
^p:
  return %a : i32
}
func.func @f(%a: i32) -> i32 {
  %r = call @g(%a) : (i32) -> i32
  return %r : i32
})";
  for (auto [text, structured] : {std::pair<const char *, bool>{affineGemm, true},
                                  {withCall, false}}) {
    mlir::MLIRContext context;
    configure(context);
    auto module = parse(context, text);
    std::string before = print(*module);
    ImportOptions options;
    options.indexBitWidth = 64;
    options.lowerStructured = structured;
    auto function = module->lookupSymbol<mlir::func::FuncOp>("f");
    SemanticProgram program = importFunction(function, options);
    CHECK(program.isSupported());
    CHECK(print(*module) == before);
    CHECK(mlir::succeeded(mlir::verify(*module)));
    CHECK(matchesSource(program, function, options));
    ImportOptions other = options;
    other.indexBitWidth = 32;
    CHECK(!matchesSource(program, function, other));
    // A change to the function is detected.
    function->setAttr("changed", mlir::UnitAttr::get(&context));
    CHECK(!matchesSource(program, function, options));
  }

  // The model owns its data: usable after its module and context are gone.
  auto owned = importText(wrapBody("%a: i8", "i8", "  %r = arith.addi %a, %a : i8\n  return %r : i8"));
  CHECK(run(owned, {4}).returns[0].getZExtValue() == 8);

  // Deterministic across separate contexts, addresses and threading settings.
  std::string first = toJSON(importText(affineGemm, 64, true));
  for (int i = 0; i < 4; ++i) {
    mlir::MLIRContext context;
    if (i % 2)
      context.disableMultithreading();
    configure(context);
    auto module = parse(context, affineGemm);
    ImportOptions options;
    options.indexBitWidth = 64;
    options.lowerStructured = true;
    CHECK(toJSON(importFunction(module->lookupSymbol<mlir::func::FuncOp>("f"),
                                options)) == first);
  }
}

//===----------------------------------------------------------------------===//
// I8: domain polarity.
//===----------------------------------------------------------------------===//
static void testI8() {
  auto admitted = [](DomainForm form, DomainClaim claim) {
    DomainEvidence e;
    e.form = form;
    return admitsClaim(e, claim).admitted;
  };
  using F = DomainForm;
  using C = DomainClaim;
  // Rows: Unknown, OverApproximate, then the exact forms.
  CHECK(!admitted(F::Unknown, C::UniversalOverDomain));
  CHECK(!admitted(F::Unknown, C::Coverage));
  CHECK(!admitted(F::Unknown, C::RefutationWitness));
  CHECK(admitted(F::OverApproximate, C::UniversalOverDomain));
  CHECK(!admitted(F::OverApproximate, C::Coverage));
  CHECK(!admitted(F::OverApproximate, C::RefutationWitness));
  for (F form : {F::ExactEnumerated, F::ExactPresburger, F::ExactClosedForm}) {
    CHECK(admitted(form, C::UniversalOverDomain));
    CHECK(admitted(form, C::Coverage));
  }
  CHECK(admitted(F::ExactEnumerated, C::RefutationWitness));
  CHECK(!admitted(F::ExactPresburger, C::RefutationWitness));
  CHECK(!admitted(F::ExactClosedForm, C::RefutationWitness));

  CHECK(importedDomainEvidence().form == F::Unknown);

  // A run gives evidence for its own parameters only.
  auto count = importText(wrapBody("%n: i8", "i8", R"(
  %z = arith.constant 0 : i8
  %o = arith.constant 1 : i8
  cf.br ^h(%z : i8)
^h(%i: i8):
  %t = arith.cmpi ult, %i, %n : i8
  cf.cond_br %t, ^b, ^e
^b:
  %x = arith.addi %i, %o : i8
  cf.br ^h(%x : i8)
^e:
  return %i : i8)"));
  EntryState entry = entryFor(count, {5});
  DomainEvidence evidence = domainEvidenceFromRun(count, entry, runConcrete(count, entry));
  CHECK(evidence.form == F::ExactEnumerated);
  CHECK(!evidence.scope.allParameters);
  CHECK(scopeContains(evidence, {APInt(8, 5)}));
  CHECK(!scopeContains(evidence, {APInt(8, 6)}));
  CHECK(admitsClaim(evidence, C::Coverage).admitted);

  // Out of fuel proves nothing.
  RunLimits tight;
  tight.fuel = 2;
  RunResult cut = runConcrete(count, entry, tight);
  CHECK(cut.status == RunStatus::OutOfFuel);
  CHECK(domainEvidenceFromRun(count, entry, cut).form == F::Unknown);

  // Any other ending proves nothing either.
  auto bad = importText(wrapBody("%a: i8", "i8", R"(
  %z = arith.constant 0 : i8
  %r = arith.divui %a, %z : i8
  return %r : i8)"));
  EntryState badEntry = entryFor(bad, {1});
  CHECK(domainEvidenceFromRun(bad, badEntry, runConcrete(bad, badEntry)).form == F::Unknown);

  // A trace that depends on memory is not determined by its parameters.
  auto data = importText(wrapBody("%L: memref<i8>", "i8", R"(
  %x = memref.load %L[] : memref<i8>
  %z = arith.constant 0 : i8
  %t = arith.cmpi eq, %x, %z : i8
  cf.cond_br %t, ^e, ^e
^e:
  return %z : i8)"));
  EntryState dataEntry = entryFor(data, {}, {Root{{}, {3}}});
  RunResult done = runConcrete(data, dataEntry);
  CHECK(done.status == RunStatus::Returned);
  CHECK(domainEvidenceFromRun(data, dataEntry, done).form == F::Unknown);

  // Scope with no restriction, and a width mismatch.
  DomainEvidence all;
  CHECK(scopeContains(all, {}));
  DomainEvidence ranged;
  ranged.scope.allParameters = false;
  ranged.scope.ranges.push_back({0, APInt(8, 1), APInt(8, 3)});
  CHECK(scopeContains(ranged, {APInt(8, 2)}));
  CHECK(!scopeContains(ranged, {APInt(16, 2)}));
  CHECK(!scopeContains(ranged, {}));
}

//===----------------------------------------------------------------------===//
// I9: assumptions never become proofs.
//===----------------------------------------------------------------------===//
static void testI9() {
  using K = ProofStatus::Kind;
  CHECK(ProofStatus::proven("").kind() == K::Unknown);
  CHECK(ProofStatus::refuted("").kind() == K::Unknown);
  CHECK(ProofStatus::assumed("").kind() == K::Unknown);
  ProofStatus proof = ProofStatus::proven("certificate");
  ProofStatus assumption = ProofStatus::assumed("no alias");
  ProofStatus refutation = ProofStatus::refuted("x = 3");
  ProofStatus unknown = ProofStatus::unknown("timeout");
  CHECK(proof.kind() == K::Proven && std::string(proof.name()) == "PROVEN");

  CHECK(isDischarged(proof));
  CHECK(!isDischarged(assumption));
  CHECK(!isDischarged(refutation));
  CHECK(!isDischarged(unknown));
  CHECK(isDischarged(assumption, {"no alias"}));
  CHECK(!isDischarged(assumption, {"something else"}));
  CHECK(!isDischarged(unknown, {"timeout"}));
  CHECK(!isDischarged(refutation, {"x = 3"}));
  // Authorization leaves the status Assumed.
  CHECK(assumption.kind() == K::Assumed);

  CHECK(conjoin(proof, proof).kind() == K::Proven);
  CHECK(conjoin(proof, assumption).kind() == K::Assumed);
  CHECK(conjoin(assumption, proof).kind() == K::Assumed);
  CHECK(conjoin(assumption, unknown).kind() == K::Unknown);
  CHECK(conjoin(unknown, refutation).kind() == K::Refuted);
  CHECK(conjoin(refutation, proof).kind() == K::Refuted);

  // Two plain memref arguments: separation is a named assumption.
  auto plain = importText("func.func @f(%a: memref<2xi8>, %b: memref<2xi8>) { return }");
  CHECK(plain.rootPairs.size() == 1);
  CHECK(plain.rootPairs[0].separated.kind() == K::Assumed);
  CHECK(plain.assumptions.size() == 1);
  CHECK(plain.assumptions[0].kind() == K::Assumed);
  CHECK(!isDischarged(plain.rootPairs[0].separated));
  CHECK(toJSON(plain).find("ASSUMED") != std::string::npos);

  auto restricted = importText(
      "func.func @f(%a: memref<2xi8> {llvm.noalias}, %b: memref<2xi8>) { return }");
  CHECK(restricted.rootPairs[0].separated.kind() == K::Proven);
  CHECK(restricted.assumptions.empty());

  auto fresh = importText(R"(func.func @f(%a: memref<2xi8>) {
  %b = memref.alloca() : memref<2xi8>
  return
})");
  CHECK(fresh.roots.size() == 2);
  CHECK(fresh.rootPairs[0].separated.kind() == K::Proven);
  CHECK(fresh.assumptions.empty());

  // Three arguments: every pair is listed, one each.
  auto three = importText(
      "func.func @f(%a: memref<i8>, %b: memref<i8>, %c: memref<i8> {llvm.noalias}) { return }");
  CHECK(three.rootPairs.size() == 3 && three.assumptions.size() == 1);
}

//===----------------------------------------------------------------------===//
// The semantic objects themselves.
//===----------------------------------------------------------------------===//
static void testMacSites() {
  // Direct, commuted, integer, and scalar forms.
  auto direct = importText(R"(func.func @f(%A: memref<2xi32>, %B: memref<2xi32>, %C: memref<2xi32>, %i: index, %j: index, %k: index) {
  %c = memref.load %C[%i] : memref<2xi32>
  %a = memref.load %A[%j] : memref<2xi32>
  %b = memref.load %B[%k] : memref<2xi32>
  %m = arith.muli %a, %b overflow<nsw> : i32
  %s = arith.addi %m, %c overflow<nuw> : i32
  memref.store %s, %C[%i] : memref<2xi32>
  return
})");
  CHECK(direct.sites.size() == 1);
  const MacSite &site = direct.sites[0];
  CHECK(site.form == ReductionForm::Direct && site.reduction == ReductionOp::Add);
  CHECK(site.multiplyFlags == FlagNSW && site.reductionFlags == FlagNUW);
  CHECK(site.a.root == 0 && site.b.root == 1 && site.output->root == 2);
  CHECK(site.outputRead->root == 2);
  RunResult r = run(direct, {0, 1, 0}, {Root{{2}, {0, 3}}, Root{{2}, {5, 0}},
                                         Root{{2}, {100, 0}}});
  CHECK(r.status == RunStatus::Returned && r.events.size() == 1);
  CHECK(r.events[0].aValue.getZExtValue() == 3 && r.events[0].bValue.getZExtValue() == 5);
  CHECK(r.events[0].updateValue.getZExtValue() == 115);
  CHECK(r.memory[2].cells[0]->getZExtValue() == 115);
  CHECK(r.events[0].h.size() == 1 && r.events[0].hRead.size() == 1);

  // h and h' are recorded, not assumed equal.
  auto different = importText(R"(func.func @f(%A: memref<f32>, %B: memref<f32>, %C: memref<2xf32>) {
  %z = arith.constant 0 : index
  %o = arith.constant 1 : index
  %c = memref.load %C[%z] : memref<2xf32>
  %a = memref.load %A[] : memref<f32>
  %b = memref.load %B[] : memref<f32>
  %m = arith.mulf %a, %b : f32
  %s = arith.addf %c, %m : f32
  memref.store %s, %C[%o] : memref<2xf32>
  return
})");
  CHECK(different.sites.size() == 1);
  CHECK(different.sites[0].output->index[0] != different.sites[0].outputRead->index[0]);

  // Not a MAC: the product is of a load and a parameter.
  auto notMac = importText(R"(func.func @f(%A: memref<f32>, %C: memref<f32>, %x: f32) {
  %c = memref.load %C[] : memref<f32>
  %a = memref.load %A[] : memref<f32>
  %m = arith.mulf %a, %x : f32
  %s = arith.addf %c, %m : f32
  memref.store %s, %C[] : memref<f32>
  return
})");
  CHECK(notMac.isSupported() && notMac.sites.empty());

  // Scalar accumulator.
  auto scalar = importText(R"(func.func @f(%A: memref<4xf32>, %B: memref<4xf32>, %n: index) -> f32 {
  %z = arith.constant 0 : index
  %o = arith.constant 1 : index
  %s0 = arith.constant 0.0 : f32
  cf.br ^h(%z, %s0 : index, f32)
^h(%i: index, %s: f32):
  %t = arith.cmpi slt, %i, %n : index
  cf.cond_br %t, ^b, ^e
^b:
  %a = memref.load %A[%i] : memref<4xf32>
  %q = memref.load %B[%i] : memref<4xf32>
  %m = arith.mulf %a, %q : f32
  %u = arith.addf %s, %m : f32
  %x = arith.addi %i, %o : index
  cf.br ^h(%x, %u : index, f32)
^e:
  return %s : f32
})");
  CHECK(scalar.sites.size() == 1);
  CHECK(scalar.sites[0].form == ReductionForm::ScalarAccumulator);
  CHECK(scalar.sites[0].accumulator.has_value());
  CHECK(scalar.state[*scalar.sites[0].accumulator].loopCarried);
  int64_t one = 0x3f800000, two = 0x40000000, three = 0x40400000;
  RunResult sum = run(scalar, {3}, {Root{{4}, {one, two, three, 0}},
                                    Root{{4}, {one, one, one, 0}}});
  CHECK(sum.status == RunStatus::Returned && sum.events.size() == 3);
  CHECK(sum.returns[0].getZExtValue() == 0x40c00000); // 6.0

  // A repeated address stays repeated: order and multiplicity are kept.
  auto repeated = importText(R"(func.func @f(%A: memref<4xi32>, %C: memref<i32>) {
  %z = arith.constant 0 : index
  %o = arith.constant 1 : index
  %two = arith.constant 2 : index
  cf.br ^h(%z : index)
^h(%u: index):
  %t = arith.cmpi slt, %u, %two : index
  cf.cond_br %t, ^b, ^e
^b:
  %c = memref.load %C[] : memref<i32>
  %a = memref.load %A[%z] : memref<4xi32>
  %q = memref.load %A[%z] : memref<4xi32>
  %m = arith.muli %a, %q : i32
  %s = arith.addi %c, %m : i32
  memref.store %s, %C[] : memref<i32>
  %x = arith.addi %u, %o : index
  cf.br ^h(%x : index)
^e:
  return
})");
  RunResult twice = run(repeated, {}, {Root{{4}, {3, 0, 0, 0}}, Root{{}, {1}}});
  CHECK(twice.events.size() == 2);
  CHECK(twice.events[0].f == twice.events[1].f);
  CHECK(twice.events[0].updateValue.getZExtValue() == 10);
  CHECK(twice.events[1].updateValue.getZExtValue() == 19);
  CHECK(twice.events[0].step < twice.events[1].step);
}

static void testExecution() {
  // The G0 example, exhaustively at width 4, against plain integers.
  std::string branchy = wrapBody("%i0: i4, %n: i4", "i4", R"(
  cf.br ^h(%i0 : i4)
^h(%i: i4):
  %c = arith.cmpi slt, %i, %n : i4
  cf.cond_br %c, ^b, ^exit
^b:
  %two = arith.constant 2 : i4
  %r = arith.remsi %i, %two : i4
  %z = arith.constant 0 : i4
  %e = arith.cmpi eq, %r, %z : i4
  cf.cond_br %e, ^a, ^o
^a:
  %t = arith.constant STEP_A : i4
  %x = arith.addi %i, %t : i4
  cf.br ^h(%x : i4)
^o:
  %s = arith.constant STEP_B : i4
  %y = arith.addi %i, %s : i4
  cf.br ^h(%y : i4)
^exit:
  return %i : i4)");
  auto with = [&](int a, int b) {
    std::string text = branchy;
    text.replace(text.find("STEP_A"), 6, std::to_string(a));
    text.replace(text.find("STEP_B"), 6, std::to_string(b));
    return importText(text);
  };
  auto terminating = with(3, 7), diverging = with(2, 4);
  unsigned returned = 0, diverged = 0;
  for (int64_t i = 0; i < 16; ++i)
    for (int64_t n = 0; n < 16; ++n) {
      // Plain integer reference.
      std::set<uint64_t> seen;
      uint64_t v = i;
      bool loops = false;
      while (toSigned(v, 4) < toSigned(n, 4)) {
        if (!seen.insert(v).second) {
          loops = true;
          break;
        }
        v = (v + (v % 2 == 0 ? 2 : 4)) & 15;
      }
      RunResult r = run(diverging, {i, n});
      CHECK(r.status == (loops ? RunStatus::Diverged : RunStatus::Returned));
      diverged += loops;
      returned += !loops;
      CHECK(run(terminating, {i, n}).status == RunStatus::Returned);
    }
  CHECK(diverged > 0 && returned > 0);

  // Multi-block inlining, with memory passed through the call.
  auto call = importText(R"(func.func @put(%m: memref<2xi32>, %v: i32) -> i32 {
  %z = arith.constant 0 : index
  %c = arith.constant 0 : i32
  %t = arith.cmpi slt, %v, %c : i32
  cf.cond_br %t, ^n, ^p
^n:
  %x = arith.subi %c, %v : i32
  memref.store %x, %m[%z] : memref<2xi32>
  return %x : i32
^p:
  memref.store %v, %m[%z] : memref<2xi32>
  return %v : i32
}
func.func @f(%m: memref<2xi32>, %a: i32) -> i32 {
  %r = call @put(%m, %a) : (memref<2xi32>, i32) -> i32
  %s = call @put(%m, %r) : (memref<2xi32>, i32) -> i32
  %t = arith.addi %r, %s : i32
  return %t : i32
})");
  CHECK(call.isSupported());
  RunResult called = run(call, {-3}, {Root{{2}, {0, 0}}});
  CHECK(called.status == RunStatus::Returned);
  CHECK(called.returns[0].getZExtValue() == 6);
  CHECK(called.memory[0].cells[0]->getZExtValue() == 3);

  // Reads see earlier writes in the same block; the last write wins.
  auto order = importText(wrapBody("%m: memref<2xi8>", "i8", R"(
  %z = arith.constant 0 : index
  %a = arith.constant 1 : i8
  %b = arith.constant 2 : i8
  memref.store %a, %m[%z] : memref<2xi8>
  %x = memref.load %m[%z] : memref<2xi8>
  memref.store %b, %m[%z] : memref<2xi8>
  %y = memref.load %m[%z] : memref<2xi8>
  %r = arith.muli %x, %y : i8
  %c = arith.constant 10 : i8
  %s = arith.addi %r, %c : i8
  return %s : i8)"));
  RunResult ordered = run(order, {}, {Root{{2}, {0, 0}}});
  CHECK(ordered.returns[0].getZExtValue() == 12);
  CHECK(ordered.memory[0].cells[0]->getZExtValue() == 2);

  // A structured GEMM end to end: 2x3x2, against a hand product.
  auto gemm = importText(affineGemm, 64, true);
  CHECK(gemm.isSupported());
  int64_t v1 = 0x3f800000, v2 = 0x40000000, v3 = 0x40400000, v4 = 0x40800000;
  Root A{{2, 2}, {v1, v2, v3, v4}};
  Root B{{2, 3}, {v1, v1, v1, v2, v2, v2}};
  Root C{{2, 3}, {0, 0, 0, 0, 0, 0}};
  RunResult product = run(gemm, {2, 3, 2}, {A, B, C});
  CHECK(product.status == RunStatus::Returned);
  CHECK(product.events.size() == 12);
  // A*B rows: [1 2]*[[1 1 1],[2 2 2]] = 5, and [3 4]*... = 11.
  uint64_t five = 0x40a00000, eleven = 0x41300000;
  for (unsigned cell = 0; cell < 6; ++cell)
    CHECK(product.memory[2].cells[cell]->getZExtValue() == (cell < 3 ? five : eleven));
  CHECK(gemm.sites[0].coordinates.size() == 3);
  EntryState point = entryFor(gemm, {2, 3, 2}, {A, B, C});
  DomainEvidence evidence = domainEvidenceFromRun(gemm, point, product);
  CHECK(evidence.form == DomainForm::ExactEnumerated);

  // The guard checks are defensive: the importer cannot produce these models,
  // so they are built by hand. A corrupted model must stop, not pick an edge.
  auto corrupted = importText(wrapBody("%c: i1", "i8", R"(
  %x = arith.constant 1 : i8
  cf.cond_br %c, ^a, ^b
^a:
  return %x : i8
^b:
  return %x : i8)"));
  corrupted.transitions[1].guard = corrupted.transitions[0].guard;
  RunResult overlap = run(corrupted, {1});
  CHECK(overlap.status == RunStatus::ModelLimit && overlap.reason == "guards overlap");
  RunResult neither = run(corrupted, {0});
  CHECK(neither.status == RunStatus::ModelLimit && neither.reason == "no guard holds");

  // Invalid entries.
  auto one = importText(wrapBody("%a: i8", "i8", "  return %a : i8"));
  EntryState wrong;
  wrong.parameters.push_back(APInt(16, 1));
  CHECK(runConcrete(one, wrong).status == RunStatus::InvalidEntry);
  CHECK(runConcrete(one, EntryState{}).status == RunStatus::InvalidEntry);
  auto memory = importText("func.func @f(%m: memref<2xi8>) { return }");
  EntryState partial = entryFor(memory, {}, {Root{{2}, {1, 2}}});
  partial.memory[0].cells[1] = std::nullopt;
  CHECK(runConcrete(memory, partial).status == RunStatus::InvalidEntry);
  EntryState badShape = entryFor(memory, {}, {Root{{3}, {1, 2, 3}}});
  CHECK(runConcrete(memory, badShape).status == RunStatus::InvalidEntry);
  auto allocated = importText(R"(func.func @f() {
  %m = memref.alloca() : memref<2xi8>
  return
})");
  EntryState filled = entryFor(allocated, {}, {Root{{2}, {1, 2}}});
  CHECK(runConcrete(allocated, filled).status == RunStatus::InvalidEntry);
  auto dyn = importText("func.func @f(%m: memref<?xi8>) { return }");
  EntryState badDim = entryFor(dyn, {}, {Root{{2}, {1, 2}}});
  badDim.parameters[0] = APInt(64, 3);
  CHECK(runConcrete(dyn, badDim).status == RunStatus::InvalidEntry);
}

int main() {
  struct Section {
    const char *name;
    void (*run)();
  } sections[] = {
      {"integer table (I1, I2)", testIntegerTable},
      {"comparison table (I1)", testCompareTable},
      {"cast table (I1, I2)", testCastTable},
      {"I1 exact width", testI1},
      {"I2 definedness", testI2},
      {"I3 fail-closed", testI3},
      {"I4 obliviousness", testI4},
      {"I5 simultaneous edges", testI5},
      {"I6 symbolic agreement", testI6Symbolic},
      {"I7 read-only, deterministic", testI7},
      {"I8 domain polarity", testI8},
      {"I9 assumptions", testI9},
      {"MAC sites and events", testMacSites},
      {"execution", testExecution},
  };
  for (const Section &section : sections) {
    int before = failures;
    section.run();
    printf("%s: %s\n", section.name, failures == before ? "pass" : "FAIL");
  }
  printf("%d checks, %d failures\n", checks, failures);
  return failures != 0;
}
