#pragma once

// Standalone P0 analysis: no MLIR/LLVM IR, passes, or lowering dependencies.
// Uses the same exact arithmetic types as TensorLogicalSpaces.h without
// depending on or changing the Affine Partitioning implementation.
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/rational.hpp>

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace systolic::polynomial {

using Integer = boost::multiprecision::cpp_int;
using Rational = boost::rational<Integer>;

// Resource limit, not a table of supported powers. Integration needs one extra
// degree. Coefficients and iteration numbers have arbitrary precision.
inline constexpr unsigned MaxDegree = 64;

// Generalized integer binomial coefficient, including negative n; k >= 0.
Integer binomial(const Integer &n, unsigned k);

struct SymbolicRangeSum;

// Canonical Newton/binomial basis: p(t) = sum coefficients[k] * C(t,k).
// Empty coefficients represent zero, whose degree is -1. All trailing zeros
// are removed. Rational normal forms are supplied by boost::rational.
class Polynomial {
public:
  Polynomial() = default;
  explicit Polynomial(std::vector<Rational> coefficients);
  static Polynomial constant(const Rational &value);
  static Polynomial fromMonomial(std::vector<Rational> coefficients);

  const std::vector<Rational> &coefficients() const { return coeffs; }
  int degree() const { return static_cast<int>(coeffs.size()) - 1; }
  bool isZero() const { return coeffs.empty(); }
  bool isIntegerValued() const;
  Rational evaluate(const Integer &t) const;
  std::vector<Rational> toMonomial() const;

  Polynomial operator+(const Polynomial &other) const;
  Polynomial operator-(const Polynomial &other) const;
  Polynomial operator*(const Polynomial &other) const;
  Polynomial scale(const Rational &factor) const;
  Polynomial pow(unsigned exponent) const;
  Polynomial composeAffine(const Rational &start, const Rational &step) const;
  Polynomial shift(const Integer &offset) const;
  Polynomial difference() const;
  Polynomial integrate(const Rational &initial = Rational(0)) const;

  // Ordinary finite ranges: reversed ranges are empty, not oriented integrals.
  Rational sumHalfOpen(const Integer &lower, const Integer &upper) const;
  Rational sumInclusive(const Integer &lower, const Integer &upper) const;
  // A two-bound symbolic expression, with independent variables a and b:
  // lowerTerm(a) + upperTerm(b), guarded by ordinary range emptiness.
  SymbolicRangeSum sumBetween(bool inclusiveUpper) const;
  // Symbolic upper bound u. Valid for u >= lower (exclusive), or
  // u >= lower-1 (inclusive). Below that domain use the numeric APIs above.
  Polynomial sumFrom(const Integer &lower, bool inclusiveUpper) const;

  bool operator==(const Polynomial &other) const { return coeffs == other.coeffs; }
  bool operator!=(const Polynomial &other) const { return !(*this == other); }

private:
  std::vector<Rational> coeffs;
};

struct SymbolicRangeSum {
  Polynomial lowerTerm; // -S(a)
  Polynomial upperTerm; // S(b), or S(b+1) for inclusive b
  bool inclusiveUpper = false;
  Rational evaluate(const Integer &lower, const Integer &upper) const;
};

// Immutable expression tree. Each variable denotes the OLD state at t;
// updates are simultaneous. Keeping original operations matters for overflow:
// a canceled multiplication can still overflow in the source transition.
class Expression {
public:
  enum class Kind { Constant, Variable, Add, Subtract, Multiply, Unsupported };
  struct Node {
    Kind kind;
    Rational constant;
    std::string name;
    std::shared_ptr<const Node> left, right;
  };

  explicit Expression(const Rational &value = Rational(0));
  static Expression variable(std::string name);
  static Expression unsupported(std::string description);
  Expression operator+(const Expression &other) const;
  Expression operator-(const Expression &other) const;
  Expression operator*(const Expression &other) const;
  Expression pow(unsigned exponent) const;
  const std::shared_ptr<const Node> &root() const { return node; }

private:
  explicit Expression(std::shared_ptr<const Node> node);
  Expression binary(Kind kind, const Expression &other) const;
  std::shared_ptr<const Node> node;
};

// The boundary is an additive transition, not a CFG or arbitrary assignment
// parser: state(t+1) = state(t) + increment(old states).
struct Recurrence {
  std::string name;
  Rational initial;
  Expression increment;
};

enum class IntegerMode { Mathematical, Modular, SignedNoOverflow };
enum class Status { Proven, Conditional, Unknown };

struct AnalysisOptions {
  IntegerMode mode = IntegerMode::Mathematical;
  unsigned bitWidth = 0; // Required only for Modular / SignedNoOverflow.
  // Number of updates, not a loop bound or a termination claim. Required for
  // signed mode, optional in other modes. Returned values cover 0 <= t <= N.
  std::optional<Integer> maxIterations;
  // If automatic range proof fails, accept conditionally under this explicit
  // caller assertion that every original intermediate and update fits signed
  // bitWidth for 0 <= t < maxIterations. Never advertised as a proof.
  std::string noOverflowAssumption;
};

struct AnalysisResult {
  Status status = Status::Unknown;
  std::string reason;
  AnalysisOptions options;
  // Exact integer lifts. In modular mode these are NOT bitvector expressions;
  // use valueAt to obtain the residue. No IR division is emitted.
  std::map<std::string, Polynomial> integerLifts;
  // Checks status, iteration domain, and signed result range. Conditional
  // results still depend on the caller's intermediate-overflow assumption.
  Integer valueAt(const std::string &name, const Integer &iteration) const;
};

// Recognizes acyclic additive polynomial systems in any input order.
// Mathematical mode permits rational expression coefficients only if every
// resulting sequence is integer-valued. For general Q-valued sums use the
// Polynomial algebra. Fixed-width modes require integral constants/initials
// and only +, -, * (no casts, division, comparisons, or saturation).
// Unsupported structure, failed proof, or resource limit => UNKNOWN, with no
// partial result. Does not infer a trip count, termination, or CFG semantics.
AnalysisResult analyze(const std::vector<Recurrence> &system,
                       const AnalysisOptions &options = {});

} // namespace systolic::polynomial
