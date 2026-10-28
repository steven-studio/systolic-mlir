#include "Systolic/PolynomialSummation.h"

#include <iostream>
#include <stdexcept>

using namespace systolic::polynomial;

namespace {
unsigned checks = 0;
void require(bool condition, const std::string &message) {
  ++checks;
  if (!condition)
    throw std::runtime_error(message);
}
template <class Exception, class Function>
void throws(Function function, const std::string &message) {
  try {
    function();
  } catch (const Exception &) {
    require(true, message);
    return;
  }
  require(false, message);
}
Rational q(long long n, long long d = 1) {
  return Rational(Integer(n), Integer(d));
}
Polynomial basis(unsigned k) {
  std::vector<Rational> a(k + 1, q(0));
  a[k] = q(1);
  return Polynomial(a);
}
Rational direct(const std::vector<Rational> &a, const Integer &n) {
  Rational result(0);
  for (auto it = a.rbegin(); it != a.rend(); ++it)
    result = result * Rational(n) + *it;
  return result;
}
void identity(const Polynomial &p, const Rational &initial = q(0)) {
  Polynomial sum = p.integrate(initial);
  require(sum.difference() == p, "coefficient discrete difference");
  require(sum.shift(Integer(1)) - sum == p, "shift-based polynomial identity");
  require(sum.evaluate(Integer(0)) == initial, "initial condition");
}

void algebra() {
  require(Polynomial({q(0), q(0)}).degree() == -1, "canonical zero degree");
  require(Polynomial({q(2, 4), q(0)}) == Polynomial({q(1, 2)}), "normalization");
  require(Polynomial({q(1), q(0)}).degree() == 0, "constant degree");
  require(Polynomial().integrate(q(7)) == Polynomial::constant(q(7)), "zero integral");
  Polynomial t({q(0), q(1)});
  for (unsigned k = 0; k <= 16; ++k) {
    std::vector<Rational> monomial(k + 1, q(0));
    monomial[k] = q(1);
    Polynomial power = Polynomial::fromMonomial(monomial);
    require(power == t.pow(k), "general multiplication vs power conversion");
    require(power.toMonomial() == monomial, "monomial conversion inverse");
    identity(power, q(-31));
    for (int n = -11; n <= 32; ++n) {
      require(power.evaluate(Integer(n)) == direct(monomial, Integer(n)), "signed evaluation");
      require(power.shift(Integer(-7)).evaluate(Integer(n)) ==
                  power.evaluate(Integer(n - 7)), "negative shift");
    }
  }
  // Expected textbook formulas are TEST ORACLES, never used to derive sums.
  require(t.sumFrom(Integer(1), true) == (t * (t + basis(0))).scale(q(1, 2)),
          "sum i = n(n+1)/2");
  require(t.pow(2).sumFrom(Integer(1), true) ==
              (t * (t + basis(0)) * (t.scale(q(2)) + basis(0))).scale(q(1, 6)),
          "sum squares");
  require(t.pow(3).sumFrom(Integer(1), true) ==
              (t * (t + basis(0))).scale(q(1, 2)).pow(2), "sum cubes");
  require(t.pow(4).sumFrom(Integer(1), true).toMonomial() ==
              std::vector<Rational>({q(0), q(-1, 30), q(0), q(1, 3), q(1, 2), q(1, 5)}),
          "sum fourth powers");
  require(t.pow(5).sumFrom(Integer(1), true).toMonomial() ==
              std::vector<Rational>({q(0), q(0), q(-1, 12), q(0), q(5, 12), q(1, 2), q(1, 6)}),
          "sum fifth powers");
  for (unsigned i = 0; i <= 10; ++i)
    for (unsigned j = 0; j <= 10; ++j) {
      Polynomial product = basis(i) * basis(j);
      require(product == basis(j) * basis(i), "commutative product");
      // Discrete product rule: delta(pq) = delta(p)*q + p*delta(q)
      // + delta(p)*delta(q). Checks all coefficients, not sampled equality.
      require(product.difference() == basis(i).difference() * basis(j) +
                  basis(i) * basis(j).difference() +
                  basis(i).difference() * basis(j).difference(), "discrete product rule");
      for (int n : {-15, -1, 0, 1, 3, 10, 20, 51})
        require(product.evaluate(Integer(n)) ==
                    basis(i).evaluate(Integer(n)) * basis(j).evaluate(Integer(n)),
                "binomial product adversary");
    }
  require(binomial(Integer(-1), 64) == 1, "negative binomial even");
  require(binomial(Integer(-1), 63) == -1, "negative binomial odd");
  require(binomial(Integer(2), 3) == 0, "below degree binomial");
  require(binomial(Integer(0), 0) == 1, "C(0,0)");
  Polynomial largeProduct = basis(32) * basis(32);
  require(largeProduct.coefficients().at(48).numerator() > (Integer(1) << 64),
          "product coefficients exceed uint64");
  require(largeProduct.evaluate(Integer(100)) ==
              Rational(binomial(Integer(100), 32) * binomial(Integer(100), 32)),
          "large combinatorial intermediates");
  identity(basis(63));
  throws<std::length_error>([&] { basis(64).integrate(); }, "integration cap");
  throws<std::length_error>([&] { basis(33) * basis(32); }, "multiplication cap");
  throws<std::length_error>([&] { t.pow(65); }, "power cap");
  require((basis(64) * Polynomial()).isZero(), "zero avoids degree growth");
  require(basis(64).scale(q(0)).isZero(), "zero scaling normalizes");
  std::vector<Rational> highZeros(100, q(0));
  require(Polynomial::fromMonomial(highZeros).isZero(), "trim before degree check");
}

void sums() {
  std::vector<std::vector<Rational>> cases = {
      {}, {q(0)}, {q(11)}, {q(5), q(-3), q(7)},
      {q(-17), q(12), q(-11), q(2)},
      {q(-13, 7), q(11, 3), q(-7, 5), q(9, 2)},
      {Rational(Integer(1) << 200), q(-1), Rational(-(Integer(1) << 180)), q(19)}};
  for (const auto &coefficients : cases) {
    Polynomial p = Polynomial::fromMonomial(coefficients);
    identity(p, q(-123, 17));
    auto exclusive = p.sumBetween(false), inclusive = p.sumBetween(true);
    require(exclusive.upperTerm.difference() == p &&
                inclusive.upperTerm.difference() == p.shift(Integer(1)),
            "symbolic upper-bound difference identity");
    require(exclusive.lowerTerm.difference() == p.scale(q(-1)),
            "symbolic lower-bound difference identity");
    require((exclusive.lowerTerm + exclusive.upperTerm).isZero() &&
                (inclusive.lowerTerm + inclusive.upperTerm.shift(Integer(-1))).isZero(),
            "symbolic empty-range base identities");
    require((p + p) - p == p, "addition subtraction");
    for (int lower = -12; lower <= 12; ++lower) {
      Rational expected(0);
      for (int count = 0; count <= 35; ++count) {
        Integer upper = lower + count;
        require(p.sumHalfOpen(Integer(lower), upper) == expected, "half-open direct sum");
        require(p.sumInclusive(Integer(lower), upper - 1) == expected, "inclusive off-by-one");
        require(exclusive.evaluate(Integer(lower), upper) == expected &&
                    inclusive.evaluate(Integer(lower), upper - 1) == expected,
                "two independent symbolic bounds");
        require(p.sumFrom(Integer(lower), false).evaluate(upper) == expected, "symbolic exclusive");
        require(p.sumFrom(Integer(lower), true).evaluate(upper - 1) == expected, "symbolic inclusive");
        expected += direct(coefficients, upper);
      }
      require(p.sumHalfOpen(Integer(lower), Integer(lower - 5)) == q(0), "reversed empty");
      require(exclusive.evaluate(Integer(lower), Integer(lower - 5)) == q(0) &&
                  inclusive.evaluate(Integer(lower), Integer(lower - 5)) == q(0),
              "symbolic bounds retain range emptiness");
    }
    for (int start : {-19, 0, 1, 11})
      for (int step : {-3, 0, 1, 2, 5, 13}) {
        Polynomial sampled = p.composeAffine(q(start), q(step));
        identity(sampled, q(47));
        Rational expected(47);
        for (int n = 0; n <= 40; ++n) {
          require(sampled.integrate(q(47)).evaluate(Integer(n)) == expected,
                  "affine stride direct sum");
          expected += direct(coefficients, Integer(start + step * n));
        }
      }
  }
  require(Polynomial::fromMonomial({q(0), q(1, 2)}).isIntegerValued() == false,
          "noninteger sequence classification");
  require(Polynomial::fromMonomial({q(0), q(-1, 2), q(1, 2)}).isIntegerValued(),
          "rational powers can be integer-valued");
  Polynomial p = Polynomial::fromMonomial({q(3, 5), q(-8, 7), q(9, 2)});
  auto composed = p.composeAffine(q(-7, 3), q(5, 11));
  identity(composed, q(8, 3));
  for (int n = -5; n <= 5; ++n) {
    Rational x = q(-7, 3) + q(5, 11) * q(n);
    require(composed.evaluate(Integer(n)) == q(3, 5) - q(8, 7) * x + q(9, 2) * x * x,
            "rational affine composition");
  }
}

void recurrences() {
  Expression x = Expression::variable("x"), y = Expression::variable("y");
  Expression z = Expression::variable("z"), w = Expression::variable("w");
  std::vector<Recurrence> system = {{"w", q(19), y * y + x * z},
                                  {"z", q(-7), y}, {"y", q(11), x * x},
                                  {"x", q(-3), Expression(q(2))}};
  auto result = analyze(system);
  require(result.status == Status::Proven, "nested triangular recognized");
  auto &forms = result.integerLifts;
  identity(forms.at("y").difference(), q(11));
  require(forms.at("z").difference() == forms.at("y"), "old y, not next y");
  require(forms.at("w").difference() == forms.at("y") * forms.at("y") +
              forms.at("x") * forms.at("z"), "nested polynomial increment identity");
  Integer xv = -3, yv = 11, zv = -7, wv = 19;
  for (int t = 0; t <= 120; ++t) {
    require(result.valueAt("x", Integer(t)) == xv &&
                result.valueAt("y", Integer(t)) == yv &&
                result.valueAt("z", Integer(t)) == zv &&
                result.valueAt("w", Integer(t)) == wv, "independent simultaneous simulation");
    wv += yv * yv + xv * zv;
    zv += yv;
    yv += xv * xv;
    xv += 2;
  }
  require(analyze({{"x", q(2), x * x - x}}).status == Status::Unknown,
          "x'=x*x rejected");
  require(analyze({{"x", q(2), x}}).status == Status::Unknown, "doubling rejected");
  require(analyze({{"x", q(0), y}, {"y", q(1), x}}).status == Status::Unknown,
          "mutual recurrence rejected");
  require(analyze({{"x", q(0), Expression::unsupported("division")}}).status == Status::Unknown,
          "unsupported operator rejected");
  require(analyze({{"x", q(0), z}}).status == Status::Unknown, "unknown state rejected");
  require(analyze({{"x", q(0), Expression(q(1, 2))}}).status == Status::Unknown,
          "noninteger recurrence rejected");
  require(analyze({{"x", q(0), Expression(q(1))},
                   {"y", q(0), (x * x - x) * Expression(q(1, 2))}}).status == Status::Proven,
          "integer-valued rational mathematical increment");
  require(analyze({{"x", q(0), Expression(q(1))},
                   {"x", q(0), Expression(q(1))}}).status == Status::Unknown,
          "duplicate state rejected");
  auto tooHigh = analyze({{"x", q(0), Expression(q(1))}, {"y", q(0), x.pow(64)}});
  require(tooHigh.status == Status::Unknown && tooHigh.integerLifts.empty(),
          "resource limit never returns partial result");
  throws<std::logic_error>([&] { tooHigh.valueAt("x", Integer(0)); }, "UNKNOWN evaluation rejected");
  throws<std::out_of_range>([&] { result.valueAt("x", Integer(-1)); }, "negative iteration rejected");
  Expression deep(q(0));
  for (unsigned i = 0; i < 130; ++i)
    deep = deep + Expression(q(1));
  require(analyze({{"x", q(0), deep}}).status == Status::Unknown, "expression depth cap");
}

void semantics() {
  Expression x = Expression::variable("x");
  AnalysisOptions modular;
  modular.mode = IntegerMode::Modular;
  modular.bitWidth = 8;
  auto triangle = analyze({{"x", q(1), Expression(q(1))}, {"s", q(0), x}}, modular);
  // 16*17 wraps to 16 in i8, then /2 gives 8; correct residue is 136.
  require(triangle.valueAt("s", Integer(16)) == 136, "divide AFTER exact product");
  require(triangle.valueAt("s", Integer(256)) == 128, "do not wrap iteration count");
  auto negative = analyze({{"x", q(-300), Expression(q(-19))}}, modular);
  require(negative.valueAt("x", Integer(0)) == 212, "negative modular initial");
  require(analyze({{"x", q(0), Expression(q(1))},
                   {"s", q(0), (x * x - x) * Expression(q(1, 2))}}, modular).status == Status::Unknown,
          "rational modular operation rejected even if integer-valued");
  AnalysisOptions signedOptions;
  signedOptions.mode = IntegerMode::SignedNoOverflow;
  signedOptions.bitWidth = 8;
  require(analyze({{"x", q(0), Expression(q(1))}}, signedOptions).status == Status::Unknown,
          "signed horizon required");
  signedOptions.maxIterations = Integer(10);
  auto safe = analyze({{"x", q(-10), Expression(q(1))},
                        {"s", q(0), x}}, signedOptions);
  require(safe.status == Status::Proven && safe.valueAt("s", Integer(10)) == -55,
          "conservative signed range proof");
  throws<std::out_of_range>([&] { safe.valueAt("s", Integer(11)); }, "proof horizon enforced");
  // Multiplication overflows even though subtraction cancels its result.
  auto hazardSystem = std::vector<Recurrence>{{"x", q(16), Expression(q(0))},
                                             {"s", q(0), x * x - x * x}};
  require(analyze(hazardSystem, signedOptions).status == Status::Unknown,
          "canceled intermediate overflow not hidden");
  signedOptions.maxIterations = Integer(0);
  require(analyze(hazardSystem, signedOptions).status == Status::Proven,
          "zero iterations execute no hazardous intermediate");
  // This safe polynomial's loose binomial enclosure crosses i8 bounds, so
  // no unconditional proof is claimed. Explicit assumptions are retained.
  signedOptions.maxIterations = Integer(10);
  auto loose = std::vector<Recurrence>{{"x", q(0), Expression(q(1))},
                                       {"s", q(0), Expression(q(9)) - x * Expression(q(2))}};
  require(analyze(loose, signedOptions).status == Status::Proven,
          "finite bound proof works with negative coefficients");
  signedOptions.maxIterations = Integer(20);
  auto needsAssumption = std::vector<Recurrence>{{"x", q(0), Expression(q(1))},
    {"s", q(0), Expression(q(19)) - x * Expression(q(2))}};
  require(analyze(needsAssumption, signedOptions).status == Status::Unknown,
          "inconclusive enclosure is UNKNOWN");
  signedOptions.noOverflowAssumption = "For 0<=t<20, source operations fit i8; s(t)=t*(20-t).";
  auto conditional = analyze(needsAssumption, signedOptions);
  require(conditional.status == Status::Conditional &&
              conditional.reason.find(signedOptions.noOverflowAssumption) != std::string::npos,
          "explicit assumption remains conditional");
  require(conditional.valueAt("s", Integer(10)) == 100, "conditional value");
  signedOptions.noOverflowAssumption.clear();
  signedOptions.maxIterations = Integer(1);
  require(analyze({{"x", q(127), Expression(q(1))}}, signedOptions).status == Status::Unknown,
          "last state update overflow checked");
  require(analyze({{"x", q(128), Expression(q(0))}}, signedOptions).status == Status::Unknown,
          "out-of-range initial rejected");
  modular.bitWidth = 0;
  require(analyze({{"x", q(0), Expression(q(1))}}, modular).status == Status::Unknown,
          "invalid width rejected");
}
} // namespace

int main() {
  try {
    algebra();
    sums();
    recurrences();
    semantics();
    std::cout << "PASS: " << checks << " exact algebra, recurrence, range, and rejection checks\n";
  } catch (const std::exception &error) {
    std::cerr << "FAIL after " << checks << " checks: " << error.what() << '\n';
    return 1;
  }
}
