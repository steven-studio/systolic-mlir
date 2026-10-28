#include "Systolic/PolynomialSummation.h"

#include <algorithm>
#include <functional>
#include <set>
#include <stdexcept>
#include <utility>

namespace systolic::polynomial {
namespace {
void trim(std::vector<Rational> &values) {
  while (!values.empty() && values.back() == Rational(0))
    values.pop_back();
}

Integer residue(const Integer &value, const Integer &modulus) {
  Integer result = value % modulus;
  if (result < 0)
    result += modulus;
  return result;
}

// A sound enclosure on ALL integer t in [0,last]. C(t,k) is nonnegative
// and nondecreasing there. No sampling or floating-point roots are involved.
std::pair<Rational, Rational> range(const Polynomial &p, const Integer &last) {
  Rational lower(0), upper(0);
  const auto &a = p.coefficients();
  if (!a.empty())
    lower = upper = a[0];
  for (unsigned k = 1; k < a.size(); ++k) {
    Rational term = a[k] * Rational(binomial(last, k));
    if (term < Rational(0))
      lower += term;
    else
      upper += term;
  }
  return {lower, upper};
}
} // namespace

Integer binomial(const Integer &n, unsigned k) {
  if (k > MaxDegree)
    throw std::length_error("binomial degree exceeds P0 limit");
  Integer value = 1;
  for (unsigned j = 1; j <= k; ++j) {
    value *= n - (j - 1);
    // Integral at EVERY step, including negative n. Never divide a residue.
    value /= j;
  }
  return value;
}

Polynomial::Polynomial(std::vector<Rational> coefficients)
    : coeffs(std::move(coefficients)) {
  trim(coeffs);
  if (coeffs.size() > MaxDegree + 1)
    throw std::length_error("polynomial degree exceeds P0 limit");
}

Polynomial Polynomial::constant(const Rational &value) {
  return Polynomial({value});
}

Polynomial Polynomial::fromMonomial(std::vector<Rational> coefficients) {
  trim(coefficients);
  if (coefficients.size() > MaxDegree + 1)
    throw std::length_error("monomial degree exceeds P0 limit");
  // Horner, with t*C(t,k) = k*C(t,k) + (k+1)*C(t,k+1).
  // This derives Newton coefficients without a power-sum formula or sampling.
  Polynomial result;
  for (auto it = coefficients.rbegin(); it != coefficients.rend(); ++it) {
    std::vector<Rational> next(result.coeffs.size() + 1, Rational(0));
    for (unsigned k = 0; k < result.coeffs.size(); ++k) {
      next[k] += result.coeffs[k] * Rational(k);
      next[k + 1] += result.coeffs[k] * Rational(k + 1);
    }
    next[0] += *it;
    result = Polynomial(std::move(next));
  }
  return result;
}

bool Polynomial::isIntegerValued() const {
  return std::all_of(coeffs.begin(), coeffs.end(), [](const Rational &a) {
    return a.denominator() == 1;
  });
}

Rational Polynomial::evaluate(const Integer &t) const {
  Rational value(0);
  Integer choose = 1;
  for (unsigned k = 0; k < coeffs.size(); ++k) {
    value += coeffs[k] * Rational(choose);
    choose *= t - k;
    choose /= k + 1;
  }
  return value;
}

std::vector<Rational> Polynomial::toMonomial() const {
  std::vector<Rational> result(coeffs.size(), Rational(0));
  std::vector<Rational> basis{Rational(1)};
  for (unsigned k = 0; k < coeffs.size(); ++k) {
    for (unsigned j = 0; j < basis.size(); ++j)
      result[j] += coeffs[k] * basis[j];
    std::vector<Rational> next(basis.size() + 1, Rational(0));
    for (unsigned j = 0; j < basis.size(); ++j) {
      next[j] -= basis[j] * Rational(k) / Rational(k + 1);
      next[j + 1] += basis[j] / Rational(k + 1);
    }
    basis = std::move(next);
  }
  trim(result);
  return result;
}

Polynomial Polynomial::operator+(const Polynomial &other) const {
  std::vector<Rational> out(std::max(coeffs.size(), other.coeffs.size()),
                            Rational(0));
  for (unsigned k = 0; k < coeffs.size(); ++k)
    out[k] += coeffs[k];
  for (unsigned k = 0; k < other.coeffs.size(); ++k)
    out[k] += other.coeffs[k];
  return Polynomial(std::move(out));
}

Polynomial Polynomial::operator-(const Polynomial &other) const {
  return *this + other.scale(Rational(-1));
}

Polynomial Polynomial::operator*(const Polynomial &other) const {
  if (isZero() || other.isZero())
    return {};
  unsigned d = degree() + other.degree();
  if (d > MaxDegree)
    throw std::length_error("product degree exceeds P0 limit");
  // Pascal triangle uses arbitrary precision, including its intermediates.
  std::vector<std::vector<Integer>> choose(d + 1);
  for (unsigned n = 0; n <= d; ++n) {
    choose[n].resize(n + 1, Integer(1));
    for (unsigned k = 1; k < n; ++k)
      choose[n][k] = choose[n - 1][k - 1] + choose[n - 1][k];
  }
  std::vector<Rational> out(d + 1, Rational(0));
  // Count pairs of subsets by union size k:
  // C(t,i) C(t,j) = sum_k C(k,i) C(i,k-j) C(t,k).
  for (unsigned i = 0; i < coeffs.size(); ++i) {
    if (coeffs[i] == Rational(0))
      continue;
    for (unsigned j = 0; j < other.coeffs.size(); ++j) {
      if (other.coeffs[j] == Rational(0))
        continue;
      Rational factor = coeffs[i] * other.coeffs[j];
      for (unsigned k = std::max(i, j); k <= i + j; ++k)
        out[k] += factor * Rational(choose[k][i] * choose[i][k - j]);
    }
  }
  return Polynomial(std::move(out));
}

Polynomial Polynomial::scale(const Rational &factor) const {
  std::vector<Rational> out = coeffs;
  for (auto &coefficient : out)
    coefficient *= factor;
  return Polynomial(std::move(out));
}

Polynomial Polynomial::pow(unsigned exponent) const {
  if (exponent > MaxDegree)
    throw std::length_error("power exceeds P0 limit");
  Polynomial result = constant(Rational(1)), base = *this;
  while (exponent) {
    if (exponent & 1)
      result = result * base;
    exponent >>= 1;
    if (exponent)
      base = base * base;
  }
  return result;
}

Polynomial Polynomial::composeAffine(const Rational &start,
                                      const Rational &step) const {
  Polynomial argument({start, step}), result;
  auto monomial = toMonomial();
  for (auto it = monomial.rbegin(); it != monomial.rend(); ++it)
    result = result * argument + constant(*it);
  return result;
}

Polynomial Polynomial::shift(const Integer &offset) const {
  std::vector<Rational> out(coeffs.size(), Rational(0));
  std::vector<Integer> choose(coeffs.size());
  for (unsigned k = 0; k < coeffs.size(); ++k)
    choose[k] = binomial(offset, k);
  // Vandermonde holds as a polynomial identity, also for negative offsets.
  for (unsigned k = 0; k < coeffs.size(); ++k)
    for (unsigned j = 0; j <= k; ++j)
      out[j] += coeffs[k] * Rational(choose[k - j]);
  return Polynomial(std::move(out));
}

Polynomial Polynomial::difference() const {
  if (coeffs.empty())
    return {};
  return Polynomial(std::vector<Rational>(coeffs.begin() + 1, coeffs.end()));
}

Polynomial Polynomial::integrate(const Rational &initial) const {
  std::vector<Rational> out{initial};
  out.insert(out.end(), coeffs.begin(), coeffs.end());
  return Polynomial(std::move(out));
}

Rational Polynomial::sumHalfOpen(const Integer &lower,
                                const Integer &upper) const {
  if (upper <= lower)
    return Rational(0);
  Polynomial integral = integrate();
  return integral.evaluate(upper) - integral.evaluate(lower);
}

Rational Polynomial::sumInclusive(const Integer &lower,
                                 const Integer &upper) const {
  if (upper < lower)
    return Rational(0);
  return sumHalfOpen(lower, upper + 1);
}

SymbolicRangeSum Polynomial::sumBetween(bool inclusiveUpper) const {
  Polynomial integral = integrate();
  return {integral.scale(Rational(-1)),
          inclusiveUpper ? integral.shift(Integer(1)) : integral,
          inclusiveUpper};
}

Rational SymbolicRangeSum::evaluate(const Integer &lower,
                                    const Integer &upper) const {
  if (inclusiveUpper ? upper < lower : upper <= lower)
    return Rational(0);
  return lowerTerm.evaluate(lower) + upperTerm.evaluate(upper);
}

Polynomial Polynomial::sumFrom(const Integer &lower, bool inclusiveUpper) const {
  Polynomial integral = integrate();
  return integral.shift(Integer(inclusiveUpper ? 1 : 0)) -
         constant(integral.evaluate(lower));
}

Expression::Expression(const Rational &value)
    : node(std::make_shared<Node>(Node{Kind::Constant, value, {}, {}, {}})) {}

Expression::Expression(std::shared_ptr<const Node> node)
    : node(std::move(node)) {}

Expression Expression::variable(std::string name) {
  return Expression(std::make_shared<Node>(
      Node{Kind::Variable, Rational(0), std::move(name), {}, {}}));
}

Expression Expression::unsupported(std::string description) {
  return Expression(std::make_shared<Node>(
      Node{Kind::Unsupported, Rational(0), std::move(description), {}, {}}));
}

Expression Expression::binary(Kind kind, const Expression &other) const {
  return Expression(std::make_shared<Node>(
      Node{kind, Rational(0), {}, node, other.node}));
}

Expression Expression::operator+(const Expression &other) const {
  return binary(Kind::Add, other);
}
Expression Expression::operator-(const Expression &other) const {
  return binary(Kind::Subtract, other);
}
Expression Expression::operator*(const Expression &other) const {
  return binary(Kind::Multiply, other);
}

Expression Expression::pow(unsigned exponent) const {
  if (exponent > MaxDegree)
    throw std::length_error("expression power exceeds P0 limit");
  Expression result(Rational(1)), base = *this;
  while (exponent) {
    if (exponent & 1)
      result = result * base;
    exponent >>= 1;
    if (exponent)
      base = base * base;
  }
  return result;
}

Integer AnalysisResult::valueAt(const std::string &name,
                                const Integer &iteration) const {
  if (status == Status::Unknown)
    throw std::logic_error("cannot evaluate UNKNOWN analysis");
  if (iteration < 0 ||
      (options.maxIterations && iteration > *options.maxIterations))
    throw std::out_of_range("iteration outside analysis domain");
  Rational exact = integerLifts.at(name).evaluate(iteration);
  if (exact.denominator() != 1)
    throw std::logic_error("nonintegral integer recurrence result");
  Integer value = exact.numerator();
  if (options.mode == IntegerMode::Modular)
    return residue(value, Integer(1) << options.bitWidth);
  if (options.mode == IntegerMode::SignedNoOverflow) {
    Integer half = Integer(1) << (options.bitWidth - 1);
    if (value < -half || value >= half)
      throw std::overflow_error("result contradicts signed no-overflow contract");
  }
  return value;
}

AnalysisResult analyze(const std::vector<Recurrence> &system,
                       const AnalysisOptions &options) {
  AnalysisResult result;
  result.options = options;
  auto unknown = [&](std::string reason) {
    result.status = Status::Unknown;
    result.reason = std::move(reason);
    result.integerLifts.clear();
    return result;
  };
  if (system.empty() || system.size() > 128)
    return unknown("expected 1..128 state variables");
  if (options.mode != IntegerMode::Mathematical &&
      options.mode != IntegerMode::Modular &&
      options.mode != IntegerMode::SignedNoOverflow)
    return unknown("unrecognized integer semantic mode");
  if (options.mode == IntegerMode::Mathematical && options.bitWidth != 0)
    return unknown("mathematical integers must not specify a bit width");
  if (options.mode != IntegerMode::Mathematical &&
      (options.bitWidth == 0 || options.bitWidth > 4096))
    return unknown("fixed-width mode requires bitWidth in 1..4096");
  if (options.maxIterations && *options.maxIterations < 0)
    return unknown("negative iteration horizon");
  bool isSigned = options.mode == IntegerMode::SignedNoOverflow;
  if (isSigned && !options.maxIterations)
    return unknown("signed mode requires an explicit iteration horizon");
  if (!isSigned && !options.noOverflowAssumption.empty())
    return unknown("no-overflow assumption only applies to signed mode");

  Integer signedHalf = 0;
  if (isSigned)
    signedHalf = Integer(1) << (options.bitWidth - 1);
  auto fitsSigned = [&](const Rational &value) {
    return value >= Rational(-signedHalf) && value < Rational(signedHalf);
  };
  std::set<std::string> names;
  for (const auto &rec : system) {
    if (rec.name.empty() || !names.insert(rec.name).second)
      return unknown("empty or duplicate state name");
    if (rec.initial.denominator() != 1)
      return unknown("integer recurrence has nonintegral initial value");
    if (isSigned && !fitsSigned(rec.initial))
      return unknown("initial value is outside signed range");
  }

  using Node = Expression::Node;
  using Kind = Expression::Kind;
  std::vector<std::set<std::string>> dependencies(system.size());
  try {
    for (unsigned i = 0; i < system.size(); ++i) {
      std::set<const Node *> visited;
      std::function<void(const Node *, unsigned)> inspect =
          [&](const Node *node, unsigned depth) {
            if (depth > 128)
              throw std::length_error("expression depth exceeds 128");
            if (!visited.insert(node).second)
              return;
            if (visited.size() > 4096)
              throw std::length_error("expression exceeds 4096 unique nodes");
            if (node->kind == Kind::Unsupported)
              throw std::invalid_argument("unsupported operation: " + node->name);
            if (node->kind == Kind::Variable) {
              if (!names.count(node->name))
                throw std::invalid_argument("unknown state: " + node->name);
              dependencies[i].insert(node->name);
            } else if (node->kind == Kind::Constant) {
              if (options.mode != IntegerMode::Mathematical &&
                  node->constant.denominator() != 1)
                throw std::invalid_argument("fixed-width rational operation");
              if (isSigned && !fitsSigned(node->constant))
                throw std::invalid_argument("constant outside signed range");
            } else {
              inspect(node->left.get(), depth + 1);
              inspect(node->right.get(), depth + 1);
            }
          };
      inspect(system[i].increment.root().get(), 0);
    }

    bool rangesProven = true;
    auto checkRange = [&](const Polynomial &p, const Integer &last) {
      if (!isSigned)
        return;
      auto bounds = range(p, last);
      rangesProven &= fitsSigned(bounds.first) && fitsSigned(bounds.second);
    };
    while (result.integerLifts.size() < system.size()) {
      bool progress = false;
      for (unsigned i = 0; i < system.size(); ++i) {
        const auto &rec = system[i];
        if (result.integerLifts.count(rec.name))
          continue;
        if (!std::all_of(dependencies[i].begin(), dependencies[i].end(),
                         [&](const std::string &name) {
                           return result.integerLifts.count(name);
                         }))
          continue;
        std::map<const Node *, Polynomial> memo;
        std::function<Polynomial(const Node *)> substitute =
            [&](const Node *node) -> Polynomial {
          if (auto found = memo.find(node); found != memo.end())
            return found->second;
          Polynomial p;
          switch (node->kind) {
          case Kind::Constant:
            p = Polynomial::constant(node->constant);
            break;
          case Kind::Variable:
            p = result.integerLifts.at(node->name);
            break;
          case Kind::Add:
            p = substitute(node->left.get()) + substitute(node->right.get());
            break;
          case Kind::Subtract:
            p = substitute(node->left.get()) - substitute(node->right.get());
            break;
          case Kind::Multiply:
            p = substitute(node->left.get()) * substitute(node->right.get());
            break;
          case Kind::Unsupported:
            throw std::invalid_argument("unsupported expression");
          }
          // Check each ORIGINAL operation, including algebraically canceled
          // intermediates, on the executed transition domain [0,N-1].
          if (isSigned && *options.maxIterations > 0)
            checkRange(p, *options.maxIterations - 1);
          memo.emplace(node, p);
          return p;
        };
        Polynomial step = substitute(rec.increment.root().get());
        Polynomial solution = step.integrate(rec.initial);
        if (!solution.isIntegerValued())
          return unknown("sequence is not integer-valued; use rational algebra");
        if (isSigned) {
          // Covers initial state and EVERY state update, including the last.
          checkRange(solution, *options.maxIterations);
        }
        result.integerLifts.emplace(rec.name, std::move(solution));
        progress = true;
      }
      if (!progress)
        return unknown("cyclic or self-dependent increment; not triangular");
    }
    if (isSigned && !rangesProven) {
      if (options.noOverflowAssumption.empty())
        return unknown("signed no-overflow range proof inconclusive");
      result.status = Status::Conditional;
      result.reason = "conditional on caller no-overflow assumption: " +
                      options.noOverflowAssumption;
    } else {
      result.status = Status::Proven;
      result.reason = "acyclic polynomial recurrences; exact identities";
    }
  } catch (const std::length_error &error) {
    return unknown(error.what());
  } catch (const std::invalid_argument &error) {
    return unknown(error.what());
  }
  return result;
}

} // namespace systolic::polynomial
