#include "Systolic/SymbolicPolynomial.h"

#include <algorithm>
#include <functional>
#include <set>
#include <stdexcept>

namespace systolic::polynomial {
namespace {
void checkBits(const Integer &value) {
  Integer magnitude = value < 0 ? -value : value;
  if (magnitude != 0 && boost::multiprecision::msb(magnitude) >= SymbolicPolynomial::MaxCoefficientBits)
    throw std::length_error("symbolic coefficient exceeds bit limit");
}
using Interval = std::pair<Rational, Rational>;
Interval multiply(Interval a, Interval b) {
  std::vector<Rational> products{a.first*b.first, a.first*b.second,
                                 a.second*b.first, a.second*b.second};
  return {*std::min_element(products.begin(), products.end()),
          *std::max_element(products.begin(), products.end())};
}
} // namespace

SymbolicPolynomial::SymbolicPolynomial(const Polynomial &coefficient) {
  data.emplace(Monomial{}, coefficient);
  normalize();
}
SymbolicPolynomial::SymbolicPolynomial(Terms terms) : data(std::move(terms)) { normalize(); }
SymbolicPolynomial SymbolicPolynomial::constant(const Rational &value) {
  return SymbolicPolynomial(Polynomial::constant(value));
}
SymbolicPolynomial SymbolicPolynomial::parameter(const std::string &name) {
  if (name.empty()) throw std::invalid_argument("empty parameter name");
  return SymbolicPolynomial(Terms{{{{name, 1}}, Polynomial::constant(Rational(1))}});
}
void SymbolicPolynomial::normalize() {
  std::set<std::string> parameters;
  for (auto it = data.begin(); it != data.end();) {
    if (it->second.isZero()) { it = data.erase(it); continue; }
    unsigned degree = 0;
    for (const auto &[name, power] : it->first) {
      if (power == 0 || power > MaxParameterDegree || degree > MaxParameterDegree-power)
        throw std::length_error("symbolic parameter degree exceeds limit");
      degree += power;
      parameters.insert(name);
    }
    for (const Rational &q : it->second.coefficients()) {
      checkBits(q.numerator()); checkBits(q.denominator());
    }
    ++it;
  }
  if (data.size() > MaxTerms) throw std::length_error("symbolic term count exceeds limit");
  if (parameters.size() > MaxParameters) throw std::length_error("symbolic parameter count exceeds limit");
}
bool SymbolicPolynomial::isIntegerValued() const {
  return std::all_of(data.begin(), data.end(), [](const auto &term) {
    return term.second.isIntegerValued();
  });
}
int SymbolicPolynomial::iterationDegree() const {
  int degree = -1;
  for (const auto &[key, p] : data) degree = std::max(degree, p.degree());
  return degree;
}
SymbolicPolynomial SymbolicPolynomial::operator+(const SymbolicPolynomial &other) const {
  Terms out = data;
  for (const auto &[key, p] : other.data) out[key] = out[key] + p;
  return SymbolicPolynomial(std::move(out));
}
SymbolicPolynomial SymbolicPolynomial::operator-(const SymbolicPolynomial &other) const {
  return *this + other.scale(Rational(-1));
}
SymbolicPolynomial SymbolicPolynomial::operator*(const SymbolicPolynomial &other) const {
  Terms out;
  for (const auto &[a, p] : data) for (const auto &[b, q] : other.data) {
    Monomial key = a;
    unsigned degree = 0;
    for (const auto &[name, power] : b) key[name] += power;
    for (const auto &[name, power] : key) degree += power;
    if (degree > MaxParameterDegree)
      throw std::length_error("symbolic product parameter degree exceeds limit");
    out[key] = out[key] + p*q; // all iteration algebra remains in P0
    if (out[key].isZero()) out.erase(key);
    if (out.size() > MaxTerms) throw std::length_error("symbolic product term count exceeds limit");
  }
  return SymbolicPolynomial(std::move(out));
}
SymbolicPolynomial SymbolicPolynomial::scale(const Rational &factor) const {
  Terms out;
  for (const auto &[key, p] : data) out.emplace(key, p.scale(factor));
  return SymbolicPolynomial(std::move(out));
}
SymbolicPolynomial SymbolicPolynomial::shift(const Integer &offset) const {
  Terms out;
  for (const auto &[key, p] : data) out.emplace(key, p.shift(offset));
  return SymbolicPolynomial(std::move(out));
}
SymbolicPolynomial SymbolicPolynomial::difference() const {
  Terms out;
  for (const auto &[key, p] : data) out.emplace(key, p.difference());
  return SymbolicPolynomial(std::move(out));
}
SymbolicPolynomial SymbolicPolynomial::integrate() const {
  Terms out;
  for (const auto &[key, p] : data) out.emplace(key, p.integrate());
  return SymbolicPolynomial(std::move(out));
}
SymbolicPolynomial SymbolicPolynomial::atIteration(const Integer &t) const {
  Terms out;
  for (const auto &[key, p] : data) out.emplace(key, Polynomial::constant(p.evaluate(t)));
  return SymbolicPolynomial(std::move(out));
}
Polynomial SymbolicPolynomial::substitute(const Assignment &values) const {
  Polynomial out;
  for (const auto &[key, p] : data) {
    Integer factor = 1;
    for (const auto &[name, power] : key) {
      const auto &value = values.at(name);
      for (unsigned i = 0; i < power; ++i) factor *= value;
    }
    out = out + p.scale(Rational(factor));
  }
  return out;
}
Rational SymbolicPolynomial::evaluate(const Integer &t, const Assignment &values) const {
  return substitute(values).evaluate(t);
}
std::pair<Rational, Rational> SymbolicPolynomial::bounds(const Integer &last, const Ranges &ranges) const {
  if (last < 0) throw std::invalid_argument("negative symbolic range horizon");
  Interval out{Rational(0), Rational(0)};
  for (const auto &[key, p] : data) {
    Interval term = p.bounds(last);
    for (const auto &[name, power] : key) {
      const auto &r = ranges.at(name);
      if (r.first > r.second) throw std::invalid_argument("reversed parameter range");
      for (unsigned i = 0; i < power; ++i)
        term = multiply(term, {Rational(r.first), Rational(r.second)});
    }
    out.first += term.first; out.second += term.second;
  }
  return out;
}
SymbolicPolynomial substituteSymbolic(const Expression &expression,
    const std::map<std::string, SymbolicPolynomial> &values) {
  using Node = Expression::Node;
  using Kind = Expression::Kind;
  std::map<const Node *, SymbolicPolynomial> memo;
  std::set<const Node *> visited;
  std::function<SymbolicPolynomial(const Node *, unsigned)> visit =
      [&](const Node *n, unsigned depth) -> SymbolicPolynomial {
    if (depth > 128) throw std::length_error("symbolic expression depth exceeds 128");
    if (auto it = memo.find(n); it != memo.end()) return it->second;
    visited.insert(n);
    if (visited.size() > 4096) throw std::length_error("symbolic expression exceeds 4096 nodes");
    SymbolicPolynomial p;
    switch (n->kind) {
    case Kind::Constant: p = SymbolicPolynomial::constant(n->constant); break;
    case Kind::Variable: p = values.at(n->name); break;
    case Kind::Add: p = visit(n->left.get(), depth+1) + visit(n->right.get(), depth+1); break;
    case Kind::Subtract: p = visit(n->left.get(), depth+1) - visit(n->right.get(), depth+1); break;
    case Kind::Multiply: p = visit(n->left.get(), depth+1) * visit(n->right.get(), depth+1); break;
    case Kind::Unsupported: throw std::invalid_argument("unsupported symbolic expression");
    }
    memo.emplace(n, p);
    return p;
  };
  return visit(expression.root().get(), 0);
}
} // namespace systolic::polynomial
