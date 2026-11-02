//===- System.cpp - Linear systems over the integers or rationals ---------===//

#include "Systolic/Exact/System.h"

namespace systolic::exact {

LinearExpr linearExpr(std::initializer_list<std::int64_t> coefficients,
                      std::int64_t constant) {
  LinearExpr expr;
  expr.coefficients.reserve(coefficients.size());
  for (std::int64_t c : coefficients)
    expr.coefficients.push_back(Int(c));
  expr.constant = Int(constant);
  return expr;
}

System System::ofSize(unsigned count) {
  return System(std::vector<Variable>(count));
}

RowError System::addInequality(LinearExpr expr) {
  if (expr.coefficients.size() != vars.size())
    return RowError::WrongCoefficientCount;
  ineqs.push_back(Inequality{std::move(expr)});
  return RowError::None;
}

RowError System::addStrictInequality(LinearExpr expr) {
  if (expr.coefficients.size() != vars.size())
    return RowError::WrongCoefficientCount;
  stricts.push_back(StrictInequality{std::move(expr)});
  return RowError::None;
}

RowError System::addEquality(LinearExpr expr) {
  if (expr.coefficients.size() != vars.size())
    return RowError::WrongCoefficientCount;
  eqs.push_back(Equality{std::move(expr)});
  return RowError::None;
}

RowError System::addCongruence(LinearExpr expr, Int modulus) {
  if (expr.coefficients.size() != vars.size())
    return RowError::WrongCoefficientCount;
  if (modulus < Int(1))
    return RowError::NonPositiveModulus;
  congs.push_back(Congruence{std::move(expr), std::move(modulus)});
  return RowError::None;
}

static void appendExpr(std::string &text, const LinearExpr &expr) {
  for (const Int &c : expr.coefficients) {
    text += ' ';
    text += toDecimal(c);
  }
  text += " ; ";
  text += toDecimal(expr.constant);
  text += '\n';
}

std::string canonicalText(const System &system) {
  std::string text = "exact-system 1\nvariables ";
  text += std::to_string(system.numVariables());
  for (const Variable &v : system.variables())
    text += v.kind == VarKind::Problem ? " p" : " e";
  text += '\n';
  for (const Inequality &row : system.inequalities()) {
    text += "ge";
    appendExpr(text, row.expr);
  }
  for (const StrictInequality &row : system.strictInequalities()) {
    text += "gt";
    appendExpr(text, row.expr);
  }
  for (const Equality &row : system.equalities()) {
    text += "eq";
    appendExpr(text, row.expr);
  }
  for (const Congruence &row : system.congruences()) {
    text += "cong ";
    text += toDecimal(row.modulus);
    appendExpr(text, row.expr);
  }
  return text;
}

} // namespace systolic::exact
