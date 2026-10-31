#pragma once
#include "Systolic/MLIRPolynomialTermination.h"
#include <stdexcept>

namespace systolic::polynomial::termination::detail {
struct Rejection : std::runtime_error { using std::runtime_error::runtime_error; };
enum class Predicate { SLT, SLE, SGT, SGE, ULT, ULE, UGT, UGE };
struct Model {
  unsigned width = 0;
  std::string fingerprint, guardText;
  std::vector<Parameter> domain;
  std::vector<Expression> initial, next, recurrenceInitial, recurrenceNext;
  Expression lhs, rhs;
  Predicate predicate = Predicate::SLT;
  std::optional<bool> constantGuard;
  std::optional<unsigned> counter;
  std::optional<Integer> step;
};
Model extract(mlir::func::FuncOp function, const Options &options);
std::vector<Certificate> candidates(const Model &model, const Options &options);
void validateLimits(const Limits &limits);
const char *predicateName(Predicate predicate);
bool signedPredicate(Predicate predicate);
bool ascendingPredicate(Predicate predicate);
bool inclusivePredicate(Predicate predicate);
std::string snapshot(mlir::Operation *operation);
Result verify(mlir::func::FuncOp function, const Certificate &certificate,
              const Options &options, unsigned &queryCount);
// Shared only with unit tests to exercise actual timeout/error handling. This
// function returns observations; it cannot create a positive analysis result.
Obligation solve(const std::string &name, const std::string &smtlib,
                 const std::string &expected, const Limits &limits);
} // namespace systolic::polynomial::termination::detail
