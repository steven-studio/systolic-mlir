//===- ExactSolverTest.cpp - Tests of the project-owned exact solver ------===//
//
// Links the solver and LLVMSupport only, so that no LLVM solver can be what
// answers here. Expectations come from outside the code under test: literals
// worked out by hand, Python's integers through --arithmetic-vectors, brute
// force over a box, and an exact Fourier-Motzkin elimination written here.
//
//===----------------------------------------------------------------------===//

#include "Systolic/Exact/Budget.h"
#include "Systolic/Exact/Checker.h"
#include "Systolic/Exact/Decide.h"
#include "Systolic/Exact/Int.h"
#include "Systolic/Exact/Lattice.h"
#include "Systolic/Exact/Normalize.h"
#include "Systolic/Exact/Provenance.h"
#include "Systolic/Exact/Rational.h"
#include "Systolic/Exact/System.h"
#include "Systolic/Exact/Verdict.h"

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <type_traits>
#include <unistd.h>
#include <vector>

using namespace systolic::exact;

// An answer can only be read by naming it.
static_assert(!std::is_convertible_v<Decision, bool>);
static_assert(!std::is_constructible_v<bool, Decision>);
static_assert(!std::is_convertible_v<Verdict, bool>);
// A Decision cannot be made up: nothing outside the solver can construct one.
static_assert(!std::is_default_constructible_v<Decision>);
static_assert(!std::is_constructible_v<Decision, Verdict>);

namespace {

unsigned passed = 0;
unsigned failed = 0;

void expect(bool condition, const std::string &what) {
  if (condition) {
    ++passed;
    return;
  }
  ++failed;
  std::cout << "FAIL: " << what << "\n";
}

Int big(const char *text) { return *parseDecimal(text); }

/// A row with the test's own 64-bit representation, kept apart from the solver.
struct Raw {
  std::vector<std::int64_t> a;
  std::int64_t c = 0;
};
struct RawCongruence {
  Raw row;
  std::int64_t modulus = 1;
};
struct RawSystem {
  unsigned n = 0;
  std::vector<Raw> ge, eq;
  std::vector<RawCongruence> cong;
};

LinearExpr toExpr(const Raw &row) {
  LinearExpr expr;
  for (std::int64_t c : row.a)
    expr.coefficients.push_back(Int(c));
  expr.constant = Int(row.c);
  return expr;
}

System toSystem(const RawSystem &raw) {
  System system = System::ofSize(raw.n);
  for (const Raw &row : raw.ge)
    expect(system.addInequality(toExpr(row)) == RowError::None, "add ge");
  for (const Raw &row : raw.eq)
    expect(system.addEquality(toExpr(row)) == RowError::None, "add eq");
  for (const RawCongruence &row : raw.cong)
    expect(system.addCongruence(toExpr(row.row), Int(row.modulus)) ==
               RowError::None,
           "add cong");
  return system;
}

std::int64_t value(const Raw &row, const std::vector<std::int64_t> &x) {
  std::int64_t total = row.c;
  for (std::size_t j = 0; j < x.size(); ++j)
    total += row.a[j] * x[j];
  return total;
}

bool holds(const RawSystem &raw, const std::vector<std::int64_t> &x) {
  for (const Raw &row : raw.ge)
    if (value(row, x) < 0)
      return false;
  for (const Raw &row : raw.eq)
    if (value(row, x) != 0)
      return false;
  for (const RawCongruence &row : raw.cong)
    if (value(row.row, x) % row.modulus != 0)
      return false;
  return true;
}

/// Brute force. The caller guarantees every solution lies in the box.
bool hasIntegerPoint(const RawSystem &raw, int box) {
  std::vector<std::int64_t> x(raw.n, -box);
  while (true) {
    if (holds(raw, x))
      return true;
    unsigned j = 0;
    for (; j < raw.n; ++j) {
      if (++x[j] <= box)
        break;
      x[j] = -box;
    }
    if (j == raw.n)
      return false;
  }
}

/// Fourier-Motzkin with no tightening, which is exact over the rationals.
/// std::nullopt when the rows grow too far to finish.
std::optional<bool> rationallyEmpty(const RawSystem &raw) {
  using Row = std::vector<__int128>;
  const unsigned n = raw.n;
  std::vector<Row> rows;
  auto push = [&](const Raw &row, int sign) {
    Row r(n + 1);
    for (unsigned j = 0; j < n; ++j)
      r[j] = sign * row.a[j];
    r[n] = sign * row.c;
    rows.push_back(r);
  };
  for (const Raw &row : raw.ge)
    push(row, 1);
  for (const Raw &row : raw.eq) {
    push(row, 1);
    push(row, -1);
  }
  auto reduced = [&](Row r) {
    __int128 g = 0;
    for (__int128 v : r) {
      __int128 a = v < 0 ? -v : v;
      while (a != 0) {
        const __int128 t = g % a;
        g = a;
        a = t;
      }
    }
    if (g > 1)
      for (__int128 &v : r)
        v /= g;
    return r;
  };
  for (int column = int(n) - 1; column >= 0; --column) {
    std::vector<Row> lower, upper;
    std::set<Row> next;
    for (const Row &r : rows) {
      if (r[column] > 0)
        lower.push_back(r);
      else if (r[column] < 0)
        upper.push_back(r);
      else
        next.insert(reduced(r));
    }
    for (const Row &l : lower)
      for (const Row &u : upper) {
        Row sum(n + 1);
        for (unsigned j = 0; j <= n; ++j)
          sum[j] = -u[column] * l[j] + l[column] * u[j];
        next.insert(reduced(sum));
      }
    if (next.size() > 3000)
      return std::nullopt;
    rows.assign(next.begin(), next.end());
  }
  for (const Row &r : rows)
    if (r[n] < 0)
      return true;
  return false;
}

Rational rationalValue(const Raw &row, const std::vector<Rational> &x) {
  Rational total{Int(row.c)};
  for (std::size_t j = 0; j < x.size(); ++j)
    total = total + Rational(Int(row.a[j])) * x[j];
  return total;
}

//===----------------------------------------------------------------------===//
// Integers
//===----------------------------------------------------------------------===//

void testIntegers() {
  const Int max64(INT64_MAX), min64(INT64_MIN);
  // Each result leaves the 64-bit range, and each literal is worked out by hand.
  expect(toDecimal(max64 + Int(1)) == "9223372036854775808", "max64 + 1");
  expect(toDecimal(min64 - Int(1)) == "-9223372036854775809", "min64 - 1");
  expect(toDecimal(-min64) == "9223372036854775808", "-min64");
  expect(toDecimal(magnitude(min64)) == "9223372036854775808", "|min64|");
  expect(toDecimal(max64 * max64) ==
             "85070591730234615847396907784232501249",
         "max64 squared");
  expect(toDecimal(min64 * min64) ==
             "85070591730234615865843651857942052864",
         "min64 squared");
  expect(toDecimal(min64 * Int(-1)) == "9223372036854775808", "min64 * -1");
  expect(toDecimal(*divFloor(min64, Int(-1))) == "9223372036854775808",
         "min64 / -1 floor");
  expect(toDecimal(*divCeil(min64, Int(-1))) == "9223372036854775808",
         "min64 / -1 ceil");
  // gcd must not hand INT64_MIN to a 64-bit routine: 2^63 has no such form.
  expect(toDecimal(gcdOf(min64, Int(0))) == "9223372036854775808",
         "gcd(min64, 0)");
  expect(toDecimal(gcdOf(Int(0), min64)) == "9223372036854775808",
         "gcd(0, min64)");
  expect(toDecimal(gcdOf(min64, min64)) == "9223372036854775808",
         "gcd(min64, min64)");
  expect(toDecimal(gcdOf(min64, Int(6))) == "2", "gcd(min64, 6)");

  // Total functions.
  expect(gcdOf(Int(0), Int(0)) == Int(0), "gcd(0, 0) is 0");
  expect(gcdOf(Int(-12), Int(18)) == Int(6), "gcd sign");
  expect(lcmOf(Int(0), Int(0)) == Int(0), "lcm(0, 0) is 0");
  expect(lcmOf(Int(0), Int(7)) == Int(0), "lcm(0, 7) is 0");
  expect(lcmOf(Int(-4), Int(6)) == Int(12), "lcm sign");
  expect(contentOf({}) == Int(0), "empty content");
  expect(contentOf({Int(0), Int(0)}) == Int(0), "zero content");
  expect(contentOf({Int(-6), Int(0), Int(9)}) == Int(3), "content");
  expect(!divFloor(Int(5), Int(0)), "floor by zero");
  expect(!divCeil(Int(5), Int(0)), "ceil by zero");
  expect(!modFloor(Int(5), Int(0)), "mod by zero");

  // Rounding, all four sign cases, by hand.
  expect(*divFloor(Int(7), Int(2)) == Int(3), "7/2 floor");
  expect(*divCeil(Int(7), Int(2)) == Int(4), "7/2 ceil");
  expect(*divFloor(Int(-7), Int(2)) == Int(-4), "-7/2 floor");
  expect(*divCeil(Int(-7), Int(2)) == Int(-3), "-7/2 ceil");
  expect(*divFloor(Int(7), Int(-2)) == Int(-4), "7/-2 floor");
  expect(*divCeil(Int(7), Int(-2)) == Int(-3), "7/-2 ceil");
  expect(*divFloor(Int(-7), Int(-2)) == Int(3), "-7/-2 floor");
  expect(*divCeil(Int(-7), Int(-2)) == Int(4), "-7/-2 ceil");
  expect(*modFloor(Int(-7), Int(2)) == Int(1), "-7 mod 2");
  expect(*modFloor(Int(7), Int(-2)) == Int(-1), "7 mod -2");

  expect(bitLength(Int(0)) == 0, "bits of 0");
  expect(bitLength(Int(1)) == 1, "bits of 1");
  expect(bitLength(Int(-8)) == 4, "bits of -8");
  expect(bitLength(min64) == 64, "bits of min64");
  expect(bitLength(max64) == 63, "bits of max64");
  expect(bitLength(big("18446744073709551616")) == 65, "bits of 2^64");
  expect(bitLength(big("4294967295")) == 32, "bits of 2^32 - 1");
  expect(bitLength(big("4294967296")) == 33, "bits of 2^32");

  // Text.
  expect(toDecimal(Int(0)) == "0", "text of 0");
  expect(toDecimal(Int(-1)) == "-1", "text of -1");
  const char *longValue = "-1000000000000000000000000000000000000000000000000001";
  expect(toDecimal(big(longValue)) == longValue, "zero digits inside the text");
  expect(toDecimal(*parseDecimal("0007")) == "7", "leading zeros");
  expect(toDecimal(*parseDecimal("-0")) == "0", "negative zero");
  expect(!parseDecimal(""), "empty text");
  expect(!parseDecimal("-"), "sign alone");
  expect(!parseDecimal("12a"), "not a number");
  expect(!parseDecimal("+5"), "plus sign");
  expect(!parseDecimal(" 5"), "space");
}

/// Rows from arithmetic_vectors.py. Returns the number of cases.
unsigned testArithmeticVectors(const char *path) {
  std::ifstream in(path);
  expect(in.good(), "vector file opens");
  std::string line;
  unsigned cases = 0;
  while (std::getline(in, line)) {
    std::istringstream fields(line);
    std::vector<std::string> f;
    for (std::string field; fields >> field;)
      f.push_back(field);
    if (f.empty())
      continue;
    ++cases;
    if (f[0] == "int") {
      expect(f.size() == 12, "int vector shape");
      const Int a = big(f[1].c_str()), b = big(f[2].c_str());
      expect(toDecimal(a + b) == f[3], "sum " + line);
      expect(toDecimal(a - b) == f[4], "difference " + line);
      expect(toDecimal(a * b) == f[5], "product " + line);
      expect(toDecimal(*divFloor(a, b)) == f[6], "floor " + line);
      expect(toDecimal(*divCeil(a, b)) == f[7], "ceil " + line);
      expect(toDecimal(*modFloor(a, b)) == f[8], "mod " + line);
      expect(toDecimal(gcdOf(a, b)) == f[9], "gcd " + line);
      expect(toDecimal(lcmOf(a, b)) == f[10], "lcm " + line);
      expect(std::to_string(bitLength(a)) == f[11], "bits " + line);
      expect(toDecimal(big(f[1].c_str())) == f[1], "text round trip " + line);
    } else {
      expect(f[0] == "rat" && f.size() == 14, "rat vector shape");
      const auto x = Rational::of(big(f[1].c_str()), big(f[2].c_str()));
      const auto y = Rational::of(big(f[3].c_str()), big(f[4].c_str()));
      expect(toDecimal(x->numerator()) == f[5] &&
                 toDecimal(x->denominator()) == f[6],
             "canonical " + line);
      const Rational sum = *x + *y, product = *x * *y;
      expect(toDecimal(sum.numerator()) == f[7] &&
                 toDecimal(sum.denominator()) == f[8],
             "rational sum " + line);
      expect(toDecimal(product.numerator()) == f[9] &&
                 toDecimal(product.denominator()) == f[10],
             "rational product " + line);
      expect((*x < *y) == (f[11] == "1"), "rational order " + line);
      expect(toDecimal(x->floor()) == f[12], "rational floor " + line);
      expect(toDecimal(x->ceil()) == f[13], "rational ceil " + line);
    }
  }
  return cases;
}

//===----------------------------------------------------------------------===//
// Rationals
//===----------------------------------------------------------------------===//

Rational q(std::int64_t n, std::int64_t d) { return *Rational::of(Int(n), Int(d)); }

void testRationals() {
  expect(!Rational::of(Int(1), Int(0)), "zero denominator");
  expect(q(2, 4).toString() == "1/2", "lowest terms");
  expect(q(-2, 4).toString() == "-1/2", "negative numerator");
  expect(q(2, -4).toString() == "-1/2", "negative denominator");
  expect(q(-2, -4).toString() == "1/2", "both negative");
  expect(q(0, -5).toString() == "0", "zero has one form");
  expect(q(0, -5).denominator() == Int(1), "zero is 0/1");
  expect(q(6, 3).isInteger() && q(6, 3).numerator() == Int(2), "integer");
  expect(q(-6, 3).toString() == "-2", "negative integer");
  expect(q(1, 2) == q(-3, -6), "equal forms");
  expect(q(1, 2) != q(1, 3), "different values");
  expect(Rational().sign() == 0 && q(-1, 9).sign() == -1 && q(1, 9).sign() == 1,
         "sign");

  // The order has to hold across signs and through a negative denominator.
  expect(q(-1, 2) < q(1, 3), "negative below positive");
  expect(q(1, -2) < q(-1, 3), "negative denominators");
  expect(q(-1, 2) < q(-1, 3), "negatives order");
  expect(q(2, 3) > q(3, 5), "cross multiplication");
  expect(q(1, 2) <= q(2, 4) && q(1, 2) >= q(2, 4), "equal is both");

  expect(q(-7, 2).floor() == Int(-4), "-7/2 floor");
  expect(q(-7, 2).ceil() == Int(-3), "-7/2 ceil");
  expect(q(7, 2).floor() == Int(3), "7/2 floor");
  expect(q(7, 2).ceil() == Int(4), "7/2 ceil");
  expect(q(-4, 2).floor() == Int(-2) && q(-4, 2).ceil() == Int(-2),
         "an integer rounds to itself");

  expect((q(1, 3) + q(1, 6)).toString() == "1/2", "sum reduces");
  expect((q(1, 2) - q(1, 2)).toString() == "0", "difference reaches zero");
  expect((q(-2, 3) * q(3, 4)).toString() == "-1/2", "product");
  expect((-q(1, 2)).toString() == "-1/2", "negate");
  expect(q(1, 2).dividedBy(q(-1, 4))->toString() == "-2", "negative divisor");
  expect(!q(1, 2).dividedBy(Rational()), "division by zero");

  // Beyond 64 bits.
  const Int two64 = big("18446744073709551616");
  const Rational huge = *Rational::of(two64, two64 * Int(2));
  expect(huge.toString() == "1/2", "big reduction");
  const Rational small = *Rational::of(Int(1), two64);
  expect((small * small).denominator() ==
             big("340282366920938463463374607431768211456"),
         "2^-64 squared");
  expect((*Rational::of(Int(INT64_MIN), Int(-1))).toString() ==
             "9223372036854775808",
         "INT64_MIN / -1");
}

//===----------------------------------------------------------------------===//
// Systems and text
//===----------------------------------------------------------------------===//

void testSystems() {
  System system = System::ofSize(2);
  expect(system.addInequality(linearExpr({1}, 0)) ==
             RowError::WrongCoefficientCount,
         "short row refused");
  expect(system.addEquality(linearExpr({1, 2, 3}, 0)) ==
             RowError::WrongCoefficientCount,
         "long row refused");
  expect(system.addCongruence(linearExpr({1, 0}, 0), Int(0)) ==
             RowError::NonPositiveModulus,
         "zero modulus refused");
  expect(system.addCongruence(linearExpr({1, 0}, 0), Int(-3)) ==
             RowError::NonPositiveModulus,
         "negative modulus refused");
  expect(system.numRows() == 0, "a refused row changes nothing");

  expect(system.addInequality(linearExpr({1, -2}, 3)) == RowError::None, "ge");
  expect(system.addEquality(linearExpr({0, 4}, -1)) == RowError::None, "eq");
  expect(system.addCongruence(linearExpr({1, 1}, 0), Int(5)) == RowError::None,
         "cong");
  expect(canonicalText(system) ==
             "exact-system 1\nvariables 2 p p\nge 1 -2 ; 3\neq 0 4 ; -1\n"
             "cong 5 1 1 ; 0\n",
         "canonical system text");

  // Names are labels, and kinds are mathematics.
  System named({Variable{VarKind::Problem, "x"},
                Variable{VarKind::Existential, "q"}});
  System renamed({Variable{VarKind::Problem, "other"},
                  Variable{VarKind::Existential, "name"}});
  expect(canonicalText(named) == canonicalText(renamed), "names do not matter");
  expect(canonicalText(named) == "exact-system 1\nvariables 2 p e\n",
         "kinds are recorded");

  LinearExpr large;
  large.coefficients = {big("-340282366920938463463374607431768211456")};
  large.constant = big("18446744073709551617");
  System wide = System::ofSize(1);
  expect(wide.addInequality(large) == RowError::None, "large row");
  expect(canonicalText(wide) ==
             "exact-system 1\nvariables 1 p\n"
             "ge -340282366920938463463374607431768211456 ; "
             "18446744073709551617\n",
         "large values keep every digit");
}

//===----------------------------------------------------------------------===//
// Normalization
//===----------------------------------------------------------------------===//

NormalizeResult normalized(std::vector<AffineRow> rows, Domain domain) {
  WorkMeter meter{WorkLimits()};
  return normalizeRows(rows, domain, meter);
}

AffineRow row(std::vector<std::int64_t> a, Rational c, std::size_t origin = 0) {
  AffineRow out;
  for (std::int64_t v : a)
    out.coefficients.push_back(Int(v));
  out.constant = c;
  out.origin = origin;
  return out;
}

void testNormalize() {
  using Status = NormalizeResult::Status;
  // 4x + 6y + 7 >= 0 divides by 2. The integer form floors 7/2; the rational
  // form keeps it.
  auto i = normalized({row({4, 6}, q(7, 1))}, Domain::Integer);
  expect(i.status == Status::Normalized && i.rows.size() == 1, "one row");
  expect(i.rows[0].direction == std::vector<Int>{Int(2), Int(3)}, "primitive");
  expect(i.rows[0].lower->value == q(-3, 1), "integer tightening floors");
  auto r = normalized({row({4, 6}, q(7, 1))}, Domain::Rational);
  expect(r.rows[0].lower->value == q(-7, 2), "rational keeps the fraction");

  // The floor rounds toward minus infinity for a negative constant too.
  auto n = normalized({row({2}, q(-1, 1))}, Domain::Integer);
  expect(n.rows[0].lower->value == q(1, 1), "2x - 1 >= 0 is x >= 1");
  auto n2 = normalized({row({-2}, q(1, 1))}, Domain::Integer);
  expect(n2.rows[0].upper->value == q(0, 1), "-2x + 1 >= 0 is x <= 0");

  // Direction: the first nonzero entry becomes positive, leading zeros stay.
  auto d = normalized({row({0, -2, 4}, q(0, 1))}, Domain::Integer);
  expect(d.rows[0].direction == std::vector<Int>{Int(0), Int(1), Int(-2)},
         "direction sign");
  expect(!d.rows[0].lower && d.rows[0].upper->value == q(0, 1),
         "a flipped row is an upper bound");

  // Constant rows.
  auto ok = normalized({row({0, 0}, q(0, 1)), row({0, 0}, q(5, 1))},
                       Domain::Integer);
  expect(ok.status == Status::Normalized && ok.rows.empty(),
         "a true constant row disappears");
  auto bad = normalized({row({0, 0}, q(-1, 2), 7)}, Domain::Rational);
  expect(bad.status == Status::ConstantConflict && bad.lowerOrigin == 7,
         "a false constant row is a conflict");

  // Merging keeps the tightest bound and its origin.
  auto m = normalized({row({1}, q(0, 1), 0), row({2}, q(-6, 1), 1),
                       row({3}, q(-3, 1), 2)},
                      Domain::Integer);
  expect(m.rows.size() == 1 && m.rows[0].lower->value == q(3, 1) &&
             m.rows[0].lower->origin == 1,
         "tightest lower bound");

  // Opposite rows. 2x >= 1 and 2x <= 1 meet at 1/2 over the rationals only.
  auto half = normalized({row({2}, q(-1, 1), 0), row({-2}, q(1, 1), 1)},
                         Domain::Integer);
  expect(half.status == Status::OppositeConflict && half.lowerOrigin == 0 &&
             half.upperOrigin == 1,
         "no integer in [1/2, 1/2]");
  auto halfR = normalized({row({2}, q(-1, 1), 0), row({-2}, q(1, 1), 1)},
                          Domain::Rational);
  expect(halfR.status == Status::Normalized &&
             halfR.rows[0].lower->value == q(1, 2) &&
             halfR.rows[0].upper->value == q(1, 2),
         "a rational point in [1/2, 1/2]");

  expect(normalized({row({1, 1}, q(0, 1))}, Domain::Integer)
             .rows[0].isSingleVariable() == false,
         "a coupled row");
  expect(normalized({row({0, 3}, q(0, 1))}, Domain::Integer)
             .rows[0].isSingleVariable(),
         "a single-variable row");

  // First appearance fixes the order, whatever the values are.
  auto order = normalized({row({5, 1}, q(0, 1)), row({1, 0}, q(0, 1)),
                           row({0, 1}, q(0, 1))},
                          Domain::Integer);
  expect(order.rows[0].direction[0] == Int(5) &&
             order.rows[1].direction[0] == Int(1) &&
             order.rows[2].direction[0] == Int(0),
         "input order is kept");

  // Normalizing what normalization produced changes nothing.
  auto once = normalized({row({4, 6}, q(7, 1)), row({-2, -3}, q(9, 1))},
                         Domain::Integer);
  std::vector<AffineRow> again;
  for (const NormalizedRow &entry : once.rows) {
    AffineRow low, high;
    if (entry.lower) {
      low.coefficients = entry.direction;
      low.constant = -entry.lower->value;
      again.push_back(low);
    }
    if (entry.upper) {
      for (const Int &c : entry.direction)
        high.coefficients.push_back(-c);
      high.constant = entry.upper->value;
      again.push_back(high);
    }
  }
  auto twice = normalized(again, Domain::Integer);
  expect(twice.rows.size() == once.rows.size() &&
             twice.rows[0].direction == once.rows[0].direction &&
             twice.rows[0].lower->value == once.rows[0].lower->value &&
             twice.rows[0].upper->value == once.rows[0].upper->value,
         "normalization is idempotent");
}

//===----------------------------------------------------------------------===//
// Equalities
//===----------------------------------------------------------------------===//

/// The determinant by cofactors: slow, and nothing in common with the solver's
/// elimination, which is the point.
Int cofactorDeterminant(const std::vector<std::vector<Int>> &m) {
  const std::size_t n = m.size();
  if (n == 0)
    return Int(1);
  Int total(0);
  for (std::size_t c = 0; c < n; ++c) {
    std::vector<std::vector<Int>> minor;
    for (std::size_t r = 1; r < n; ++r) {
      std::vector<Int> row;
      for (std::size_t j = 0; j < n; ++j)
        if (j != c)
          row.push_back(m[r][j]);
      minor.push_back(std::move(row));
    }
    const Int term = m[0][c] * cofactorDeterminant(minor);
    total += c % 2 == 0 ? term : -term;
  }
  return total;
}

LatticeResult solved(unsigned n, const std::vector<Raw> &rows, Domain domain) {
  std::vector<EqualityRow> input;
  for (const Raw &raw : rows) {
    EqualityRow r;
    for (std::int64_t c : raw.a)
      r.coefficients.push_back(Int(c));
    r.constant = Int(raw.c);
    input.push_back(r);
  }
  WorkMeter meter{WorkLimits()};
  return solveEqualities(n, input, domain, meter);
}

void testUnitDeterminant() {
  using Matrix = std::vector<std::vector<Int>>;
  auto unit = [](Matrix matrix) {
    WorkMeter meter{WorkLimits()};
    return hasUnitDeterminant(std::move(matrix), meter);
  };
  auto ints = [](std::initializer_list<std::initializer_list<int>> rows) {
    Matrix matrix;
    for (const auto &row : rows) {
      matrix.emplace_back();
      for (int value : row)
        matrix.back().push_back(Int(value));
    }
    return matrix;
  };
  // Determinants worked by hand. The empty product is 1.
  expect(unit({}) == true, "the empty matrix");
  expect(unit(ints({{1}})) == true, "det 1");
  expect(unit(ints({{-1}})) == true, "det -1");
  expect(unit(ints({{2}})) == false, "det 2");
  expect(unit(ints({{0}})) == false, "det 0");
  expect(unit(ints({{0, 1}, {1, 0}})) == true, "a swap, det -1");
  expect(unit(ints({{2, 1}, {1, 1}})) == true, "det 1, off the identity");
  expect(unit(ints({{1, 1}, {1, 1}})) == false, "singular");
  expect(unit(ints({{2, 0}, {0, 1}})) == false, "a scaled column");
  // Needs a row swap on its second step, and has determinant -1.
  expect(unit(ints({{1, 1, 0}, {1, 1, 1}, {0, 1, 1}})) == true,
         "a pivot that appears only after elimination");
  // Its last step divides by the pivot 2 of the step before: det 1.
  expect(unit(ints({{2, 1, 0}, {1, 1, 0}, {0, 0, 1}})) == true,
         "division by the previous pivot");

  // Beyond 64 bits: (a+1)(a-1) - a*a = -1, and (a+1)a - a*a = a, for a = 2^70.
  const Int a = *parseDecimal("1180591620717411303424");
  expect(unit({{a + Int(1), a}, {a, a - Int(1)}}) == true, "|det| 1 at 2^140");
  expect(unit({{a + Int(1), a}, {a, a}}) == false, "det 2^70");

  // Random matrices, against cofactors.
  std::mt19937_64 rng(31);
  unsigned units = 0, others = 0;
  for (int trial = 0; trial < 20000; ++trial) {
    const unsigned n = 1 + rng() % 4;
    Matrix matrix(n, std::vector<Int>(n, Int(0)));
    for (auto &row : matrix)
      for (Int &value : row)
        value = Int(std::int64_t(rng() % 5) - 2);
    const bool expected = magnitude(cofactorDeterminant(matrix)) == Int(1);
    (expected ? units : others) += 1;
    expect(unit(matrix) == expected, "the determinant of a random matrix");
  }
  expect(units > 2000 && others > 2000, "both outcomes occurred");

  // It obeys the meter, and says that it could not tell.
  WorkLimits fewSteps;
  fewSteps.maxSteps = 3;
  WorkMeter stepMeter(fewSteps);
  expect(!hasUnitDeterminant(ints({{2, 1}, {1, 1}}), stepMeter).has_value() &&
             stepMeter.reason() == UnknownReason::StepLimit,
         "the determinant is charged: 2 squared is over 3");
  fewSteps.maxSteps = 5;
  WorkMeter enoughSteps(fewSteps);
  expect(hasUnitDeterminant(ints({{2, 1}, {1, 1}}), enoughSteps) == true &&
             enoughSteps.steps() == 5,
         "4 and then 1");
  // Entries of 2 bits with a minor of 6.
  WorkLimits fewBits;
  fewBits.maxCoefficientBits = 2;
  WorkMeter bitMeter(fewBits);
  expect(!hasUnitDeterminant(ints({{3, 3}, {1, 3}}), bitMeter).has_value() &&
             bitMeter.reason() == UnknownReason::BitLimit,
         "the minors are admitted");
}

void testLattice() {
  using Status = LatticeResult::Status;
  // 2x + 4y = 3: the gcd 2 does not divide 3.
  auto noInteger = solved(2, {{{2, 4}, -3}}, Domain::Integer);
  expect(noInteger.status == Status::Conflict &&
             noInteger.conflict.kind == EqualityConflict::Kind::NotIntegral,
         "2x + 4y = 3 has no integer point");
  expect(noInteger.conflict.multipliers == std::vector<Rational>{q(1, 2)},
         "its certificate is 1/2");
  auto rational = solved(2, {{{2, 4}, -3}}, Domain::Rational);
  expect(rational.status == Status::Solved &&
             rational.solution.numFree() == 1,
         "2x + 4y = 3 has rational points");

  // x = 1 and x = 2.
  auto clash = solved(1, {{{1}, -1}, {{1}, -2}}, Domain::Rational);
  expect(clash.status == Status::Conflict &&
             clash.conflict.kind == EqualityConflict::Kind::Inconsistent,
         "x = 1 and x = 2");
  expect(clash.conflict.multipliers == std::vector<Rational>{q(-1, 1), q(1, 1)},
         "the second row minus the first");
  // A redundant row is not a conflict: x + y = 1 twice, once doubled.
  auto repeated = solved(2, {{{1, 1}, -1}, {{2, 2}, -2}}, Domain::Integer);
  expect(repeated.status == Status::Solved &&
             repeated.solution.numFree() == 1,
         "a dependent consistent row");
  // A zero row.
  expect(solved(2, {{{0, 0}, 0}}, Domain::Integer).status == Status::Solved,
         "0 = 0");
  auto zero = solved(2, {{{0, 0}, 5}}, Domain::Integer);
  expect(zero.status == Status::Conflict &&
             zero.conflict.multipliers == std::vector<Rational>{q(1, 1)},
         "0 = -5");
  auto none = solved(3, {}, Domain::Integer);
  expect(none.status == Status::Solved && none.solution.numFree() == 3,
         "no rows");

  // Solutions in the integers, in a box, against brute force.
  std::mt19937_64 rng(7);
  auto pick = [&](int lo, int hi) {
    return lo + int(rng() % std::uint64_t(hi - lo + 1));
  };
  unsigned solvedCases = 0, conflictCases = 0;
  for (int trial = 0; trial < 3000; ++trial) {
    const unsigned n = pick(1, 3);
    const unsigned m = pick(1, 3);
    std::vector<Raw> rows;
    for (unsigned r = 0; r < m; ++r) {
      Raw raw;
      for (unsigned j = 0; j < n; ++j)
        raw.a.push_back(pick(-6, 6));
      raw.c = pick(-9, 9);
      rows.push_back(raw);
    }
    const LatticeResult result = solved(n, rows, Domain::Integer);
    const int box = 12;
    RawSystem raw;
    raw.n = n;
    raw.eq = rows;
    std::vector<std::vector<std::int64_t>> solutions;
    std::vector<std::int64_t> x(n, -box);
    while (true) {
      if (holds(raw, x))
        solutions.push_back(x);
      unsigned j = 0;
      for (; j < n; ++j) {
        if (++x[j] <= box)
          break;
        x[j] = -box;
      }
      if (j == n)
        break;
    }
    if (result.status == Status::Conflict) {
      ++conflictCases;
      expect(solutions.empty(), "a conflict means no integer point");
      continue;
    }
    ++solvedCases;
    const EqualitySolution &s = result.solution;
    // x = U z loses no integer point, and the free coordinates reach no row.
    expect(magnitude(cofactorDeterminant(s.transform())) == Int(1),
           "U is unimodular");
    for (const Raw &r : rows)
      for (const Int &c : s.freeCoefficients(toExpr(r).coefficients))
        expect(c == Int(0), "no free coordinate reaches an equality");
    // Every y gives a solution.
    const unsigned k = s.numFree();
    std::vector<Rational> free(k);
    for (int sample = 0; sample < 12; ++sample) {
      for (unsigned t = 0; t < k; ++t)
        free[t] = q(pick(-3, 3), 1);
      const std::vector<Rational> point = s.point(free);
      for (const Raw &r : rows)
        expect(rationalValue(r, point).sign() == 0,
               "a solution of the parameterization solves the row");
      for (const Rational &v : point)
        expect(v.isInteger(), "an integer y gives an integer point");
    }
    // Every solution has an integer y: the form a . x over the free
    // coordinates and at the origin reconstructs it exactly.
    for (const auto &solution : solutions) {
      const std::vector<Rational> base = s.point(std::vector<Rational>(k));
      // Solve basis * y = x - base by elimination over the rationals.
      std::vector<std::vector<Rational>> m2(n, std::vector<Rational>(k + 1));
      for (unsigned t = 0; t < k; ++t) {
        std::vector<Rational> unit(k);
        unit[t] = q(1, 1);
        const std::vector<Rational> column = s.point(unit);
        for (unsigned i = 0; i < n; ++i)
          m2[i][t] = column[i] - base[i];
      }
      for (unsigned i = 0; i < n; ++i)
        m2[i][k] = Rational(Int(solution[i])) - base[i];
      unsigned rank = 0;
      for (unsigned t = 0; t < k && rank < n; ++t) {
        unsigned pivot = rank;
        while (pivot < n && m2[pivot][t].sign() == 0)
          ++pivot;
        if (pivot == n)
          continue;
        std::swap(m2[pivot], m2[rank]);
        for (unsigned i = 0; i < n; ++i) {
          if (i == rank || m2[i][t].sign() == 0)
            continue;
          const Rational f = *m2[i][t].dividedBy(m2[rank][t]);
          for (unsigned c = t; c <= k; ++c)
            m2[i][c] = m2[i][c] - f * m2[rank][c];
        }
        ++rank;
      }
      expect(rank == k, "the basis has full column rank");
      bool consistent = true, integral = true;
      for (unsigned i = 0; i < n; ++i) {
        bool zeroRow = true;
        for (unsigned t = 0; t < k; ++t)
          zeroRow = zeroRow && m2[i][t].sign() == 0;
        if (zeroRow) {
          consistent = consistent && m2[i][k].sign() == 0;
        } else {
          unsigned t = 0;
          while (m2[i][t].sign() == 0)
            ++t;
          integral = integral && m2[i][k].dividedBy(m2[i][t])->isInteger();
        }
      }
      expect(consistent && integral, "a solution has an integer y");
    }
  }
  expect(solvedCases > 200 && conflictCases > 200,
         "both outcomes occurred in the equality trial");

  // The check runs on a query with an equality. For x + y = 1: 3 to admit the
  // row, 3 for its one column step, 1 for the residual, and 5 for the check.
  const System line = toSystem({2, {}, {{{1, 1}, -1}}, {}});
  expect(decide(line, Domain::Integer).receipt().steps == 12,
         "the determinant check ran");
}

//===----------------------------------------------------------------------===//
// Decisions
//===----------------------------------------------------------------------===//

void testIntegerAgainstRational() {
  struct Case {
    const char *name;
    RawSystem system;
    Verdict integer, rational;
  };
  const std::vector<Case> cases = {
      {"2x = 1", {1, {}, {{{2}, -1}}, {}}, Verdict::Infeasible,
       Verdict::Feasible},
      {"2x >= 1, 2x <= 1", {1, {{{2}, -1}, {{-2}, 1}}, {}, {}},
       Verdict::Infeasible, Verdict::Feasible},
      {"2x >= 1, 3x <= 2", {1, {{{2}, -1}, {{-3}, 2}}, {}, {}},
       Verdict::Infeasible, Verdict::Feasible},
      {"x >= 1, x <= 0", {1, {{{1}, -1}, {{-1}, 0}}, {}, {}},
       Verdict::Infeasible, Verdict::Infeasible},
      {"x = 1, x = 2", {1, {}, {{{1}, -1}, {{1}, -2}}, {}},
       Verdict::Infeasible, Verdict::Infeasible},
      {"0 >= 1", {2, {{{0, 0}, -1}}, {}, {}}, Verdict::Infeasible,
       Verdict::Infeasible},
      {"0 = 1", {2, {}, {{{0, 0}, -1}}, {}}, Verdict::Infeasible,
       Verdict::Infeasible},
      {"no rows, no variables", {0, {}, {}, {}}, Verdict::Feasible,
       Verdict::Feasible},
      {"no rows", {3, {}, {}, {}}, Verdict::Feasible, Verdict::Feasible},
      {"0 = 0", {2, {}, {{{0, 0}, 0}}, {}}, Verdict::Feasible,
       Verdict::Feasible},
      {"0 >= 0", {2, {{{0, 0}, 0}}, {}, {}}, Verdict::Feasible,
       Verdict::Feasible},
      {"box, negative", {2, {{{1, 0}, 5}, {{-1, 0}, -3}, {{0, -1}, -2}}, {}, {}},
       Verdict::Feasible, Verdict::Feasible},
      {"x + y = 3, x - y = 1", {2, {}, {{{1, 1}, -3}, {{1, -1}, -1}}, {}},
       Verdict::Feasible, Verdict::Feasible},
      {"x + y = 3, x - y = 0", {2, {}, {{{1, 1}, -3}, {{1, -1}, 0}}, {}},
       Verdict::Infeasible, Verdict::Feasible},
      {"x = y, x >= 1, y <= 0",
       {2, {{{1, 0}, -1}, {{0, -1}, 0}}, {{{1, -1}, 0}}, {}},
       Verdict::Infeasible, Verdict::Infeasible},
      {"a coupled row survives", {2, {{{1, 1}, -1}}, {}, {}}, Verdict::Unknown,
       Verdict::Unknown},
  };
  for (const Case &c : cases) {
    const System system = toSystem(c.system);
    const Decision integer = decide(system, Domain::Integer);
    const Decision rational = decide(system, Domain::Rational);
    expect(integer.verdict() == c.integer,
           std::string(c.name) + " over the integers");
    expect(rational.verdict() == c.rational,
           std::string(c.name) + " over the rationals");
    expect(integer.domain() == Domain::Integer &&
               rational.domain() == Domain::Rational,
           std::string(c.name) + " records its domain");
  }

  // Tightening is what separates them, so a proof that used it must be refused
  // in the rational domain, and accepted in the integer one.
  const System half = toSystem({1, {{{2}, -1}, {{-2}, 1}}, {}, {}});
  const Decision proved = decide(half, Domain::Integer);
  expect(proved.provedInfeasible(), "2x = 1 style system");
  expect(checkInfeasibility(half, Domain::Integer, *proved.proof()).accepted,
         "the integer proof checks over the integers");
  expect(!checkInfeasibility(half, Domain::Rational, *proved.proof()).accepted,
         "the integer proof is refused over the rationals");

  // A rational proof through an equality with a pivot of 2: its rows carry a
  // scale of 2, which the proof has to cancel. 2x + 2y = 1, x + z >= 1 and
  // x + z <= 0 clash whatever y is.
  const System scaled = toSystem(
      {3, {{{1, 0, 1}, -1}, {{-1, 0, -1}, 0}}, {{{2, 2, 0}, -1}}, {}});
  expect(decide(scaled, Domain::Rational).provedInfeasible(),
         "a proof with scaled rows");

  // Every rational proof is a pure combination, so it holds over the integers.
  const System apart = toSystem({1, {{{1}, -1}, {{-1}, 0}}, {}, {}});
  const Decision narrow = decide(apart, Domain::Rational);
  expect(checkInfeasibility(apart, Domain::Integer, *narrow.proof()).accepted,
         "a rational proof also holds over the integers");
}

void testCongruences() {
  // x = 1 (mod 2) and x = 0 (mod 2).
  RawSystem clash;
  clash.n = 1;
  clash.cong = {{{{1}, -1}, 2}, {{{1}, 0}, 2}};
  const System clashing = toSystem(clash);
  const Decision answer = decide(clashing, Domain::Integer);
  expect(answer.provedInfeasible(), "odd and even");
  expect(decide(clashing, Domain::Rational).why() ==
             UnknownReason::UnsupportedConstruct,
         "a congruence has no rational answer");

  // x = 1 (mod 3), x in [4, 6]: only x = 4.
  RawSystem only;
  only.n = 1;
  only.cong = {{{{1}, -1}, 3}};
  only.eq = {{{1}, -4}};
  const Decision four = decide(toSystem(only), Domain::Integer);
  expect(four.provedFeasible() &&
             four.integerWitness()->values == std::vector<Int>{Int(4)},
         "the witness has no quotient in it");

  // x = 2 (mod 4) and x = 1: 1 - 2 = -1 is not a multiple of 4.
  RawSystem off;
  off.n = 1;
  off.cong = {{{{1}, -2}, 4}};
  off.eq = {{{1}, -1}};
  expect(decide(toSystem(off), Domain::Integer).provedInfeasible(),
         "a pinned value off the lattice");

  // A negative value satisfies a congruence by the floor rule: -3 = 1 (mod 4).
  System negative = System::ofSize(1);
  expect(negative.addCongruence(linearExpr({1}, -1), Int(4)) == RowError::None,
         "negative witness system");
  expect(checkWitness(negative, IntegerWitness{{Int(-3)}}).accepted,
         "-3 is 1 mod 4");
  expect(!checkWitness(negative, IntegerWitness{{Int(-2)}}).accepted,
         "-2 is not 1 mod 4");
}

void testLargeValues() {
  const Int two64 = big("18446744073709551616");
  // 2^64 x = 2^65: x = 2.
  System scaled = System::ofSize(1);
  LinearExpr equality;
  equality.coefficients = {two64};
  equality.constant = -(two64 * Int(2));
  expect(scaled.addEquality(equality) == RowError::None, "large equality");
  const Decision two = decide(scaled, Domain::Integer);
  expect(two.provedFeasible() &&
             two.integerWitness()->values == std::vector<Int>{Int(2)},
         "2^64 x = 2^65 gives x = 2");

  // 2^64 x = 2^64 + 1 has no integer point, by a margin no 64-bit value holds.
  System off = System::ofSize(1);
  LinearExpr odd;
  odd.coefficients = {two64};
  odd.constant = -(two64 + Int(1));
  expect(off.addEquality(odd) == RowError::None, "odd large equality");
  expect(decide(off, Domain::Integer).provedInfeasible(), "off by one in 2^64");
  const Decision rational = decide(off, Domain::Rational);
  expect(rational.provedFeasible(), "a rational point");
  expect(rational.rationalWitness()->values[0].toString() ==
             "18446744073709551617/18446744073709551616",
         "the exact rational point");

  // Bounds around 2^63: x >= 2^63 and x <= 2^63 - 1.
  System tight = System::ofSize(1);
  LinearExpr low, high;
  low.coefficients = {Int(1)};
  low.constant = Int(INT64_MIN);
  high.coefficients = {Int(-1)};
  high.constant = Int(INT64_MAX);
  expect(tight.addInequality(low) == RowError::None, "low");
  expect(tight.addInequality(high) == RowError::None, "high");
  expect(decide(tight, Domain::Integer).provedInfeasible(),
         "an empty interval at the 64-bit edge");
  // x = 2^63 exactly, a value with no int64_t form.
  System edge = System::ofSize(1);
  LinearExpr pin;
  pin.coefficients = {Int(1)};
  pin.constant = Int(INT64_MIN);
  expect(edge.addEquality(pin) == RowError::None, "pin");
  const Decision at = decide(edge, Domain::Integer);
  expect(at.provedFeasible() && toDecimal(at.integerWitness()->values[0]) ==
                                    "9223372036854775808",
         "a witness beyond int64_t");

  // Elimination with a coefficient far above 64 bits, and a negative one.
  System wide = System::ofSize(2);
  LinearExpr tie;
  tie.coefficients = {two64 * two64, Int(-1)}; // 2^128 x - y = 0
  expect(wide.addEquality(tie) == RowError::None, "tie");
  LinearExpr bound;
  bound.coefficients = {Int(1), Int(0)}; // x = 3
  bound.constant = Int(-3);
  expect(wide.addEquality(bound) == RowError::None, "bound");
  const Decision far = decide(wide, Domain::Integer);
  expect(far.provedFeasible() &&
             toDecimal(far.integerWitness()->values[1]) ==
                 "1020847100762815390390123822295304634368",
         "y = 3 * 2^128");
}

void testCheckerRefusals() {
  const System system = toSystem({1, {{{1}, -1}, {{-1}, 3}}, {}, {}}); // 1..3
  const Decision wrongPoint = certifyFeasible(system, IntegerWitness{{Int(9)}});
  expect(wrongPoint.isUnknown() &&
             wrongPoint.why() == UnknownReason::CertificateRejected,
         "a false witness cannot become a Feasible decision");
  expect(wrongPoint.integerWitness() == nullptr, "and it keeps no witness");
  const Decision shortPoint = certifyFeasible(system, IntegerWitness{});
  expect(shortPoint.isUnknown(), "a witness of the wrong size");
  const Decision point = certifyFeasible(system, IntegerWitness{{Int(2)}});
  expect(point.provedFeasible(), "a true witness is accepted");
  const Decision rational =
      certifyFeasible(system, RationalWitness{{q(5, 2)}});
  expect(rational.provedFeasible() && rational.domain() == Domain::Rational,
         "a rational witness");

  // False rational witnesses, for each kind of row.
  expect(certifyFeasible(system, RationalWitness{{q(9, 2)}}).why() ==
             UnknownReason::CertificateRejected,
         "a rational witness outside an inequality");
  const System half = toSystem({1, {}, {{{2}, -1}}, {}});
  expect(certifyFeasible(half, RationalWitness{{q(1, 3)}}).why() ==
             UnknownReason::CertificateRejected,
         "a rational witness off an equality");
  // An equality refuses a point on either side of it.
  expect(checkWitness(half, RationalWitness{{q(1, 3)}}).why ==
             "the witness violates an equality",
         "a rational witness below an equality");
  expect(checkWitness(half, RationalWitness{{q(1, 1)}}).why ==
             "the witness violates an equality",
         "a rational witness above an equality");
  expect(certifyFeasible(half, RationalWitness{{q(1, 2)}}).provedFeasible(),
         "the rational point on it");
  expect(certifyFeasible(half, RationalWitness{}).why() ==
             UnknownReason::CertificateRejected,
         "a rational witness of the wrong size");

  // A congruence has no rational meaning, so nothing rational can be said of a
  // system that has one, even when the rest of the certificate is sound.
  System mixed = System::ofSize(1);
  expect(mixed.addInequality(linearExpr({0}, -1)) == RowError::None, "false row");
  expect(mixed.addCongruence(linearExpr({1}, 0), Int(2)) == RowError::None,
         "congruence");
  InfeasibilityProof falseRow;
  falseRow.steps.push_back(
      ProofStep::input({RowRef::Kind::Inequality, 0, false}));
  expect(checkInfeasibility(mixed, Domain::Integer, falseRow).accepted,
         "the false row proves it over the integers");
  expect(checkInfeasibility(mixed, Domain::Rational, falseRow).why ==
             "a congruence has no rational meaning",
         "a rational proof on a congruence");
  expect(checkWitness(mixed, RationalWitness{{q(0, 1)}}).why ==
             "a congruence has no rational meaning",
         "a rational witness on a congruence");

  // Over a common denominator: 6/3 + 12/4 = 5, and 6/3 + 4/2 = 4.
  const System sixFour = toSystem({2, {}, {{{6, 4}, -5}}, {}});
  expect(checkWitness(sixFour, RationalWitness{{q(1, 3), q(3, 4)}}).accepted,
         "mixed denominators on an equality");
  expect(checkWitness(sixFour, RationalWitness{{q(1, 3), q(1, 2)}}).why ==
             "the witness violates an equality",
         "mixed denominators off an equality");
  const System sixFourRow = toSystem({2, {{{6, 4}, -5}}, {}, {}});
  expect(checkWitness(sixFourRow, RationalWitness{{q(1, 3), q(3, 4)}}).accepted,
         "mixed denominators on the boundary of a row");
  expect(checkWitness(sixFourRow, RationalWitness{{q(1, 3), q(1, 2)}}).why ==
             "the witness violates an inequality",
         "mixed denominators under a row");
  // A denominator past 64 bits: 2^70 x = 1.
  const Int huge = *parseDecimal("1180591620717411303424");
  System tiny = System::ofSize(1);
  expect(tiny.addEquality(LinearExpr{{huge}, Int(-1)}) == RowError::None,
         "a coefficient of 2^70");
  expect(checkWitness(tiny, RationalWitness{{*Rational::of(Int(1), huge)}})
             .accepted,
         "x = 1 / 2^70");
  expect(checkWitness(tiny,
                      RationalWitness{{*Rational::of(Int(1), huge + Int(1))}})
                 .why == "the witness violates an equality",
         "x = 1 / (2^70 + 1)");

  // A feasible system cannot be proved empty.
  InfeasibilityProof empty;
  expect(certifyInfeasible(system, Domain::Integer, empty).isUnknown(),
         "an empty proof");
  InfeasibilityProof sum;
  const std::size_t a = 0, b = 1;
  sum.steps.push_back(ProofStep::input({RowRef::Kind::Inequality, a, false}));
  sum.steps.push_back(ProofStep::input({RowRef::Kind::Inequality, b, false}));
  sum.steps.push_back(ProofStep::combine({{0, Int(1)}, {1, Int(1)}}));
  // The sum is 0 >= -2: true, so not a contradiction.
  expect(certifyInfeasible(system, Domain::Integer, sum).why() ==
             UnknownReason::CertificateRejected,
         "a true final row proves nothing");

  auto refused = [&](ProofStep step, const char *reason) {
    InfeasibilityProof proof;
    proof.steps.push_back(ProofStep::input({RowRef::Kind::Inequality, 0, false}));
    proof.steps.push_back(std::move(step));
    expect(checkInfeasibility(system, Domain::Integer, proof).why == reason,
           reason);
  };
  refused(ProofStep::input({RowRef::Kind::Inequality, 9, false}),
          "an input step names no usable row");
  refused(ProofStep::combine({}), "a combine step has no terms");
  refused(ProofStep::combine({{5, Int(1)}}), "a combine step reads a later step");
  refused(ProofStep::combine({{0, Int(0)}}),
          "a combine step has a multiplier that is not positive");
  refused(ProofStep::tighten(7), "a tighten step reads a later step");

  // Each of these would be a sound-looking proof that a feasible system is
  // empty, if the rule it breaks were missing: { x >= 1, x >= -5 } has x = 1,
  // and x - 1 plus -(x + 5) is the false row 0 >= 6.
  const System open = toSystem({1, {{{1}, -1}, {{1}, 5}}, {}, {}});
  InfeasibilityProof negated;
  negated.steps.push_back(ProofStep::input({RowRef::Kind::Inequality, 0, false}));
  negated.steps.push_back(ProofStep::input({RowRef::Kind::Inequality, 1, true}));
  negated.steps.push_back(ProofStep::combine({{0, Int(1)}, {1, Int(1)}}));
  expect(checkInfeasibility(open, Domain::Integer, negated).why ==
             "an input step names no usable row",
         "an inequality cannot be negated");
  InfeasibilityProof subtracted;
  subtracted.steps.push_back(
      ProofStep::input({RowRef::Kind::Inequality, 0, false}));
  subtracted.steps.push_back(
      ProofStep::input({RowRef::Kind::Inequality, 1, false}));
  subtracted.steps.push_back(ProofStep::combine({{0, Int(1)}, {1, Int(-1)}}));
  expect(checkInfeasibility(open, Domain::Integer, subtracted).why ==
             "a combine step has a multiplier that is not positive",
         "an inequality cannot be subtracted");

  // The false row has to be false: x >= 0 and x <= 0 sum to 0 >= 0, which holds.
  const System pinned = toSystem({1, {{{1}, 0}, {{-1}, 0}}, {}, {}});
  InfeasibilityProof zero;
  zero.steps.push_back(ProofStep::input({RowRef::Kind::Inequality, 0, false}));
  zero.steps.push_back(ProofStep::input({RowRef::Kind::Inequality, 1, false}));
  zero.steps.push_back(ProofStep::combine({{0, Int(1)}, {1, Int(1)}}));
  expect(checkInfeasibility(pinned, Domain::Integer, zero).why ==
             "the last step is not a false row",
         "0 >= 0 is not a contradiction");

  // It also has to have no variables: x - 1 >= 0 has a negative constant.
  InfeasibilityProof variable;
  variable.steps.push_back(ProofStep::input({RowRef::Kind::Inequality, 0, false}));
  expect(checkInfeasibility(system, Domain::Integer, variable).why ==
             "the last step still has variables",
         "a negative constant alone is not a contradiction");

  // Tightening divides by the gcd of the variables, so it needs some.
  const System constant = toSystem({1, {{{0}, -1}}, {}, {}});
  InfeasibilityProof tightened;
  tightened.steps.push_back(ProofStep::input({RowRef::Kind::Inequality, 0, false}));
  tightened.steps.push_back(ProofStep::tighten(0));
  expect(checkInfeasibility(constant, Domain::Integer, tightened).why ==
             "a tighten step reads a row with no variables",
         "tightening a constant row");
}

void testAgainstBruteForce() {
  std::mt19937_64 rng(20261102);
  auto pick = [&](int lo, int hi) {
    return lo + int(rng() % std::uint64_t(hi - lo + 1));
  };
  unsigned feasible = 0, infeasible = 0, unknown = 0, rationalDecided = 0,
           rationalOnly = 0;
  for (int trial = 0; trial < 30000; ++trial) {
    RawSystem raw;
    raw.n = pick(1, 4);
    const int box = 3;
    for (unsigned j = 0; j < raw.n; ++j) {
      Raw lo, hi;
      lo.a.assign(raw.n, 0);
      hi.a.assign(raw.n, 0);
      lo.a[j] = 1;
      lo.c = box;
      hi.a[j] = -1;
      hi.c = box;
      raw.ge.push_back(lo);
      raw.ge.push_back(hi);
    }
    const int family = pick(0, 3);
    const int extra = pick(0, 3);
    for (int r = 0; r < extra; ++r) {
      Raw row;
      const int scale = family == 2 ? pick(2, 5) : 1;
      const int nonzero = family == 3 ? int(pick(0, raw.n - 1)) : -1;
      for (unsigned j = 0; j < raw.n; ++j) {
        // Family 3 keeps rows on one variable, which S1 can finish.
        const int c = nonzero >= 0 && int(j) != nonzero ? 0 : pick(-4, 4);
        row.a.push_back(c * scale);
      }
      row.c = pick(-9, 9);
      raw.ge.push_back(row);
    }
    for (int r = pick(0, 2); r > 0; --r) {
      Raw row;
      for (unsigned j = 0; j < raw.n; ++j)
        row.a.push_back(pick(-4, 4));
      row.c = pick(-6, 6);
      raw.eq.push_back(row);
    }
    if (pick(0, 2) == 0) {
      Raw row;
      for (unsigned j = 0; j < raw.n; ++j)
        row.a.push_back(pick(-3, 3));
      row.c = pick(-4, 4);
      raw.cong.push_back({row, pick(1, 5)});
    }
    const System system = toSystem(raw);
    const bool truth = hasIntegerPoint(raw, box);
    const Decision integer = decide(system, Domain::Integer);
    if (integer.provedFeasible()) {
      ++feasible;
      expect(truth, "a Feasible answer needs an integer point");
      std::vector<std::int64_t> x;
      for (const Int &v : integer.integerWitness()->values)
        x.push_back(static_cast<std::int64_t>(v));
      expect(holds(raw, x), "the witness holds by the test's own arithmetic");
    } else if (integer.provedInfeasible()) {
      ++infeasible;
      expect(!truth, "an Infeasible answer needs no integer point");
      expect(integer.proof() != nullptr, "an Infeasible answer has a proof");
    } else {
      ++unknown;
      expect(integer.why() == UnknownReason::RequiresSearch,
             "S1 refuses only because a search is missing");
      expect(!integer.integerWitness() && !integer.proof(),
             "Unknown carries no certificate");
    }

    // The rational question, without the congruence it does not define.
    RawSystem plain = raw;
    plain.cong.clear();
    const std::optional<bool> empty = rationallyEmpty(plain);
    const Decision rational = decide(toSystem(plain), Domain::Rational);
    if (rational.isUnknown() || !empty)
      continue;
    ++rationalDecided;
    expect(rational.provedInfeasible() == *empty,
           "the rational answer matches Fourier-Motzkin");
    if (rational.provedFeasible()) {
      const auto &x = rational.rationalWitness()->values;
      for (const Raw &row : plain.ge)
        expect(rationalValue(row, x).sign() >= 0, "rational witness, ge");
      for (const Raw &row : plain.eq)
        expect(rationalValue(row, x).sign() == 0, "rational witness, eq");
      rationalOnly += !hasIntegerPoint(plain, box);
    }
  }
  std::cout << "brute force: " << feasible << " feasible, " << infeasible
            << " infeasible, " << unknown << " unknown; rational decided "
            << rationalDecided << ", of which " << rationalOnly
            << " have no integer point\n";
  // The comparison means nothing unless all of these really occurred.
  expect(feasible > 1000, "feasible answers occurred");
  expect(infeasible > 1000, "infeasible answers occurred");
  expect(unknown > 1000, "unknown answers occurred");
  expect(rationalDecided > 1000, "rational answers occurred");
  expect(rationalOnly > 10, "the domains really differed");
}

void testUnknownIsHonest() {
  // These have clear answers, which the solver cannot reach yet. It must say so
  // rather than answer. x + y >= 1 is feasible; the second is empty by a
  // combination of rows that no pairwise rule sees.
  const System feasible = toSystem({2, {{{1, 1}, -1}}, {}, {}});
  const Decision a = decide(feasible, Domain::Integer);
  expect(a.isUnknown() && a.why() == UnknownReason::RequiresSearch,
         "a feasible coupled system is not claimed");
  const System empty =
      toSystem({2, {{{1, 1}, -1}, {{-1, 1}, 0}, {{0, -1}, 0}}, {}, {}});
  const Decision b = decide(empty, Domain::Rational);
  expect(b.isUnknown() && b.why() == UnknownReason::RequiresSearch,
         "an infeasible coupled system is not claimed");
  expect(!a.provedFeasible() && !a.provedInfeasible(), "Unknown is neither");
  expect(a.integerWitness() == nullptr && a.rationalWitness() == nullptr &&
             a.proof() == nullptr,
         "Unknown carries nothing");
}

//===----------------------------------------------------------------------===//
// Limits
//===----------------------------------------------------------------------===//

void testBudget() {
  // Work is exact: the limit at the count succeeds, and one below stops.
  const std::vector<RawSystem> systems = {
      {2, {{{1, 0}, 2}, {{-1, 0}, 2}}, {{{1, 1}, -3}}, {}},
      {3, {{{2, 0, 0}, -1}, {{-2, 0, 0}, 1}}, {{{1, 1, 1}, -1}, {{1, -1, 0}, 0}}, {}},
      {2, {{{1, 1}, -1}}, {}, {}},
      {1, {}, {{{2}, -1}}, {}},
  };
  for (const RawSystem &raw : systems) {
    const System system = toSystem(raw);
    const Decision open = decide(system, Domain::Integer);
    const std::uint64_t steps = open.receipt().steps;
    expect(steps > 0, "a query does some work");
    WorkLimits exact;
    exact.maxSteps = steps;
    const Decision same = decide(system, Domain::Integer, exact);
    expect(canonicalText(same) == canonicalText(open),
           "the exact budget gives the same answer");
    expect(same.receipt().steps == steps, "and the same work");
    WorkLimits below;
    below.maxSteps = steps - 1;
    const Decision cut = decide(system, Domain::Integer, below);
    expect(cut.isUnknown() && cut.why() == UnknownReason::StepLimit,
           "one step short stops");
    for (std::uint64_t limit = 0; limit < steps; ++limit) {
      WorkLimits l;
      l.maxSteps = limit;
      const Decision d = decide(system, Domain::Integer, l);
      expect(d.isUnknown() && d.why() == UnknownReason::StepLimit,
             "every budget below the cost stops, and none invents an answer");
    }
  }

  // Shape limits are checked before any work.
  const System two = toSystem({2, {{{1, 0}, 0}, {{0, 1}, 0}}, {}, {}});
  WorkLimits rows;
  rows.maxRows = 1;
  expect(decide(two, Domain::Integer, rows).why() == UnknownReason::RowLimit,
         "row limit");
  rows.maxRows = 2;
  expect(decide(two, Domain::Integer, rows).provedFeasible(),
         "exactly at the row limit passes");
  WorkLimits vars;
  vars.maxVariables = 1;
  expect(decide(two, Domain::Integer, vars).why() ==
             UnknownReason::VariableLimit,
         "variable limit");
  vars.maxVariables = 2;
  expect(decide(two, Domain::Integer, vars).provedFeasible(),
         "exactly at the variable limit passes");
  vars.maxVariables = 1;
  // A congruence's quotient is a variable of the query.
  RawSystem withCongruence{1, {}, {}, {{{{1}, 0}, 2}}};
  expect(decide(toSystem(withCongruence), Domain::Integer, vars).why() ==
             UnknownReason::VariableLimit,
         "a quotient counts as a variable");

  // The coefficient limit sees input and computed values.
  WorkLimits bits;
  bits.maxCoefficientBits = 4;
  System wide = System::ofSize(1);
  expect(wide.addInequality(linearExpr({100}, 0)) == RowError::None, "wide");
  expect(decide(wide, Domain::Integer, bits).why() == UnknownReason::BitLimit,
         "an input coefficient over the limit");
  expect(decide(toSystem({1, {{{15}, 0}}, {}, {}}), Domain::Integer, bits)
             .provedFeasible(),
         "exactly at the limit passes");
  // Euclid grows a column of U past the input size, here 3 bits against 5.
  WorkLimits grow;
  grow.maxCoefficientBits = 3;
  const System tall =
      toSystem({3, {}, {{{5, 7, 0}, 0}, {{1, 1, 7}, 0}, {{7, 1, 1}, 0}}, {}});
  expect(decide(tall, Domain::Integer).provedFeasible(), "it is feasible");
  expect(decide(tall, Domain::Integer, grow).why() == UnknownReason::BitLimit,
         "a computed value over the limit");
  // Every input fits in 3 bits and the row shrinks as Euclid runs, so only an
  // entry of the unimodular matrix can pass the limit here.
  const System onlyU = toSystem({3, {}, {{{7, -6, 4}, 0}}, {}});
  expect(decide(onlyU, Domain::Integer).provedFeasible(), "it is feasible too");
  expect(decide(onlyU, Domain::Integer, grow).why() == UnknownReason::BitLimit,
         "growth of the unimodular matrix is limited");

  // A meter stops once, for good, and keeps the first cause.
  WorkLimits one;
  one.maxSteps = 5;
  WorkMeter meter(one);
  expect(meter.charge(5), "within the limit");
  expect(!meter.charge(1), "over the limit");
  expect(meter.reason() == UnknownReason::StepLimit, "first cause");
  meter.stop(UnknownReason::RowLimit);
  expect(meter.reason() == UnknownReason::StepLimit, "the first cause stays");
  expect(!meter.charge(0), "a stopped meter stays stopped");

  // A charge near 2^64 must not wrap.
  WorkLimits huge;
  huge.maxSteps = UINT64_MAX;
  WorkMeter wrap(huge);
  expect(wrap.charge(UINT64_MAX - 1), "almost everything");
  expect(!wrap.charge(2), "the sum would wrap past the limit");

  // Cancellation.
  const System system = toSystem(systems[0]);
  const Decision open = decide(system, Domain::Integer);
  CancellationToken counter;
  const Decision counted = decide(system, Domain::Integer, WorkLimits(), &counter);
  const std::uint64_t polls = counter.pollCount();
  expect(polls > 0, "a query polls");
  expect(canonicalText(counted) == canonicalText(open),
         "an untouched token changes nothing");
  for (std::uint64_t k = 0; k < polls; ++k) {
    CancellationToken token;
    token.cancelAfterPolls(k);
    const Decision d = decide(system, Domain::Integer, WorkLimits(), &token);
    expect(d.isUnknown() && d.why() == UnknownReason::Cancelled,
           "a cancelled query is Unknown");
    expect(token.pollCount() == k + 1, "it stops at the requested poll");
  }
  CancellationToken late;
  late.cancelAfterPolls(polls);
  expect(canonicalText(decide(system, Domain::Integer, WorkLimits(), &late)) ==
             canonicalText(open),
         "a cancellation that never comes changes nothing");
  CancellationToken at0;
  at0.requestCancellation();
  expect(decide(system, Domain::Integer, WorkLimits(), &at0).why() ==
             UnknownReason::Cancelled,
         "cancelled before the start");
}

void testDeterminism() {
  RawSystem raw{3,
                {{{1, 0, 0}, 3}, {{-1, 0, 0}, 3}, {{0, 2, 0}, 5}, {{0, -2, 0}, -5}},
                {{{1, 1, 1}, -2}},
                {}};
  const System system = toSystem(raw);
  const Decision first = decide(system, Domain::Integer);
  for (int i = 0; i < 20; ++i) {
    const Decision next = decide(system, Domain::Integer);
    expect(canonicalText(next) == canonicalText(first), "the same answer");
    expect(next.receipt().digest == first.receipt().digest, "the same receipt");
  }
  System copy = system;
  expect(canonicalText(decide(copy, Domain::Integer)) == canonicalText(first),
         "a copy of the system decides the same");

  // A decision owns what it holds, so it outlives the system.
  std::optional<Decision> kept;
  {
    const System local = toSystem({1, {{{1}, -3}, {{-1}, 3}}, {}, {}});
    kept.emplace(decide(local, Domain::Integer));
  }
  expect(kept->provedFeasible() &&
             kept->integerWitness()->values == std::vector<Int>{Int(3)},
         "the decision outlives its system");

  // Hand derivation of the proof text: x >= 1 and x <= 0 add up to 0 >= 1,
  // after each row is tightened, which leaves it unchanged here.
  const System apart = toSystem({1, {{{1}, -1}, {{-1}, 0}}, {}, {}});
  expect(canonicalText(*decide(apart, Domain::Integer).proof()) ==
             "exact-proof 1\n0 input ge 0 pos\n1 input ge 1 pos\n2 tighten 0\n"
             "3 tighten 1\n4 combine 1*2 1*3\n",
         "the exact proof");
  expect(canonicalText(*decide(apart, Domain::Rational).proof()) ==
             "exact-proof 1\n0 input ge 0 pos\n1 input ge 1 pos\n"
             "2 combine 1*0 1*1\n",
         "the exact rational proof has no tighten step");
}

//===----------------------------------------------------------------------===//
// Provenance
//===----------------------------------------------------------------------===//

std::uint64_t ownFnv1a(const std::string &text) {
  std::uint64_t hash = 0xcbf29ce484222325ULL;
  for (unsigned char c : text)
    hash = (hash ^ c) * 0x100000001b3ULL;
  return hash;
}

void testProvenance() {
  // Reading the declaration is not execution: no counter moves.
  const RuntimeEvidence quiet = runtimeEvidence();
  const BuildDeclaration &declared = buildDeclaration();
  const std::string report = provenanceJson();
  expect(implementationName(declared.implementation) ==
             std::string("PROJECT_OWNED"),
         "this library declares itself project-owned");
  expect(declared.productionSolver == SolverImplementation::LlvmUpstream,
         "production has not moved");
  // Project-owned is not the same as free of LLVM.
  expect(declared.llvmDependency == "ARITHMETIC_ONLY",
         "the dependency on LLVM is declared");
  expect(report.find("\"llvm_dependency\": \"ARITHMETIC_ONLY\"") !=
             std::string::npos,
         "and reported");
  const RuntimeEvidence untouched = runtimeEvidence();
  expect(untouched.decideCalls == quiet.decideCalls &&
             untouched.checkerRuns == quiet.checkerRuns,
         "a declaration is not evidence");
  expect(report.find("\"compile_time_declaration\"") != std::string::npos &&
             report.find("\"runtime_evidence\"") != std::string::npos,
         "the two kinds of claim stay apart");

  // A query moves exactly the counters of the stages it ran.
  const System system = toSystem({1, {{{1}, -1}, {{-1}, 0}}, {}, {}});
  const RuntimeEvidence before = runtimeEvidence();
  const Decision decision = decide(system, Domain::Integer);
  const RuntimeEvidence after = runtimeEvidence();
  expect(after.decideCalls == before.decideCalls + 1, "decide ran once");
  expect(after.latticeRuns == before.latticeRuns + 1, "the lattice ran once");
  expect(after.normalizeRuns == before.normalizeRuns + 1, "normalize ran once");
  expect(after.checkerRuns == before.checkerRuns + 1, "the checker ran once");
  expect(after.checkerRefusals == before.checkerRefusals, "nothing was refused");

  // A stage that did not run does not count.
  const RuntimeEvidence s0 = runtimeEvidence();
  WorkLimits none;
  none.maxRows = 0;
  const Decision refused = decide(system, Domain::Integer, none);
  const RuntimeEvidence s1 = runtimeEvidence();
  expect(s1.decideCalls == s0.decideCalls + 1 &&
             s1.latticeRuns == s0.latticeRuns &&
             s1.normalizeRuns == s0.normalizeRuns &&
             s1.checkerRuns == s0.checkerRuns,
         "a refused query ran no stage");
  expect(!refused.receipt().ranLattice && !refused.receipt().ranNormalize &&
             !refused.receipt().ranChecker,
         "and its receipt says so");

  // The receipt is bound to this input and this outcome. The steps are worked
  // out by hand: two rows admitted at 2 each, two reduced at 2 each, and two
  // normalized at 2 each.
  expect(decision.receipt().steps == 12, "the work of this query");
  const std::string expected = "exact-system 1\nvariables 1 p\n"
                               "ge 1 ; -1\nge -1 ; 0\n"
                               "domain=integer verdict=INFEASIBLE reason=none "
                               "steps=12\n"
                               "stages=lattice,normalize,checker\n";
  expect(receiptText(system, decision) == expected, "the receipt text");
  expect(decision.receipt().digest == ownFnv1a(expected),
         "the digest, recomputed here");
  expect(fnv1a64("") == 0xcbf29ce484222325ULL, "FNV-1a of nothing");
  expect(fnv1a64("a") == 0xaf63dc4c8601ec8cULL, "FNV-1a of a");

  // A different input gives a different receipt, so one cannot be replayed.
  const System other = toSystem({1, {{{1}, -2}, {{-1}, 0}}, {}, {}});
  expect(decide(other, Domain::Integer).receipt().digest !=
             decision.receipt().digest,
         "another system, another receipt");
  expect(decide(system, Domain::Rational).receipt().digest !=
             decision.receipt().digest,
         "another domain, another receipt");

  // A Decision from outside a query has no receipt.
  expect(certifyFeasible(system, IntegerWitness{{Int(0)}}).receipt().digest == 0,
         "certifying is not a query");

  // A refused certificate shows up as evidence.
  const RuntimeEvidence c0 = runtimeEvidence();
  const Decision forged = certifyFeasible(system, IntegerWitness{{Int(0)}});
  const RuntimeEvidence c1 = runtimeEvidence();
  expect(forged.isUnknown() && c1.checkerRefusals == c0.checkerRefusals + 1,
         "a refusal is counted");

  // The system that loops forever in the LLVM 24 Simplex, from the root cause
  // report: 7 rows over 3 variables. This binary has no such code, and this
  // answer differs from the library's, so what ran cannot have been the library.
  // An alarm turns a hang into a failure.
  const System stall = toSystem({3,
                                 {{{-2, 0, 2}, 0},
                                  {{0, 2, 1}, 0},
                                  {{1, -2, -1}, 0},
                                  {{3, -1, 0}, 2},
                                  {{0, 1, 0}, -1},
                                  {{-1, 0, 0}, -1},
                                  {{1, 1, 1}, 0}},
                                 {},
                                 {}});
  alarm(30);
  const Decision stalled = decide(stall, Domain::Integer);
  alarm(0);
  expect(stalled.isUnknown() && stalled.why() == UnknownReason::RequiresSearch,
         "the 7-row system comes back at once and is not claimed");
}

} // namespace

int main(int argc, char **argv) {
  if (argc == 3 && std::string(argv[1]) == "--arithmetic-vectors") {
    const unsigned cases = testArithmeticVectors(argv[2]);
    std::cout << "arithmetic vectors: " << cases << " cases, " << passed
              << " passed, " << failed << " failed\n";
    return failed != 0 || cases == 0;
  }
  if (argc == 2 && std::string(argv[1]) == "--print-provenance") {
    std::cout << provenanceJson();
    return 0;
  }
  if (argc == 2 && std::string(argv[1]) == "--print-provenance-after-query") {
    const System system = System::ofSize(1);
    const Decision decision = decide(system, Domain::Integer);
    std::cout << provenanceJson();
    return decision.provedFeasible() ? 0 : 1;
  }

  testIntegers();
  testRationals();
  testSystems();
  testNormalize();
  testUnitDeterminant();
  testLattice();
  testIntegerAgainstRational();
  testCongruences();
  testLargeValues();
  testCheckerRefusals();
  testAgainstBruteForce();
  testUnknownIsHonest();
  testBudget();
  testDeterminism();
  testProvenance();

  std::cout << "exact solver foundation: " << passed << " passed, " << failed
            << " failed\n";
  return failed != 0;
}
