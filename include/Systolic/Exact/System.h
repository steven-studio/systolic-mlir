//===- System.h - Linear systems over the integers or rationals -*- C++ -*-===//
//
// A System is a list of variables and a list of rows over them:
//
//   inequality   expr >= 0
//   strict       expr > 0
//   equality     expr == 0
//   congruence   expr == 0 (mod m), for an integer m >= 1
//
// It is a plain value. It owns all its storage, holds no solver state, and a
// query never changes it or keeps a reference to it after returning.
//
// A congruence is defined over the integers only, as the existence of an
// integer q with expr = m * q. A rational query on a system with a congruence
// has no meaning here and answers Unknown.
//
// A strict row belongs to the rational solver in Session.h. The checker reads
// it too. decide() does not, so it answers Unknown on a system that has one.
//
// Variable kinds do not change feasibility: every variable is simply a point
// coordinate, so an existential variable is decided like any other. The kind
// is recorded for projection, which uses it to tell a point from a witness.
//
//===----------------------------------------------------------------------===//

#ifndef SYSTOLIC_EXACT_SYSTEM_H
#define SYSTOLIC_EXACT_SYSTEM_H

#include "Systolic/Exact/Int.h"

#include <cstdint>
#include <initializer_list>

namespace systolic::exact {

enum class VarKind {
  /// A coordinate of the set being described.
  Problem,
  /// Exists only to define the set, such as the quotient of a stride.
  Existential,
};

struct Variable {
  VarKind kind = VarKind::Problem;
  /// A label for people. It takes no part in any answer or serialization.
  std::string name;
};

/// coefficients[0] * x_0 + ... + constant.
struct LinearExpr {
  std::vector<Int> coefficients;
  Int constant{0};
};

/// A LinearExpr from 64-bit values, for literals.
LinearExpr linearExpr(std::initializer_list<std::int64_t> coefficients,
                      std::int64_t constant);

struct Inequality {
  LinearExpr expr; // expr >= 0
};
struct StrictInequality {
  LinearExpr expr; // expr > 0
};
struct Equality {
  LinearExpr expr; // expr == 0
};
struct Congruence {
  LinearExpr expr; // expr == 0 (mod modulus)
  Int modulus;     // >= 1
};

/// Why a row was refused. A refused row leaves the system unchanged.
enum class RowError {
  None,
  /// The expression does not have one coefficient per variable.
  WrongCoefficientCount,
  /// A congruence needs a modulus of at least 1.
  NonPositiveModulus,
  /// A Session could not take the row without passing its memory limit.
  SessionLimit,
};

class System {
public:
  explicit System(std::vector<Variable> variables)
      : vars(std::move(variables)) {}
  /// `count` variables of kind Problem.
  static System ofSize(unsigned count);

  [[nodiscard]] RowError addInequality(LinearExpr expr);
  [[nodiscard]] RowError addStrictInequality(LinearExpr expr);
  [[nodiscard]] RowError addEquality(LinearExpr expr);
  [[nodiscard]] RowError addCongruence(LinearExpr expr, Int modulus);

  unsigned numVariables() const { return vars.size(); }
  const std::vector<Variable> &variables() const { return vars; }
  const std::vector<Inequality> &inequalities() const { return ineqs; }
  const std::vector<StrictInequality> &strictInequalities() const {
    return stricts;
  }
  const std::vector<Equality> &equalities() const { return eqs; }
  const std::vector<Congruence> &congruences() const { return congs; }
  std::size_t numRows() const {
    return ineqs.size() + stricts.size() + eqs.size() + congs.size();
  }

private:
  std::vector<Variable> vars;
  std::vector<Inequality> ineqs;
  std::vector<StrictInequality> stricts;
  std::vector<Equality> eqs;
  std::vector<Congruence> congs;
};

/// A text form that depends only on the mathematics: the kinds and rows, in
/// order, without variable names. Equal systems give equal text.
std::string canonicalText(const System &system);

} // namespace systolic::exact

#endif // SYSTOLIC_EXACT_SYSTEM_H
