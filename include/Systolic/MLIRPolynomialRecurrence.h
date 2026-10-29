#pragma once

#include "Systolic/PolynomialSummation.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"

namespace systolic::polynomial::cfg {

struct Options {
  // arith iW operations are bitvectors. Modular is the only unconditional
  // default. Signed and Mathematical BOTH require a finite horizon and a
  // proof/explicit assumption connecting those bitvectors to integer lifts.
  IntegerMode mode = IntegerMode::Modular;
  std::optional<Integer> maxIterations;
  // Used only if the source range proof is inconclusive. Asserts that all
  // original arithmetic intermediates and states fit their signed width.
  // The conservative proof checks source expressions at every t in [0,N],
  // including a hypothetical final body evaluation. Never a termination claim.
  // Nonempty text cannot authorize unsupported operations or CFG shapes.
  std::string noOverflowAssumption;
};

struct State {
  unsigned argument;
  std::string name; // s0, s1, ... in HEADER argument order; never SSA spelling.
  Rational initial;
  Expression next; // Every variable refers to the same old header state.
  Expression increment;
  std::vector<unsigned> dependencies;
};

struct Result {
  Status status = Status::Unknown;
  std::string reason;
  Options options;
  // Non-owning pointer, valid only while the input IR remains alive/unchanged.
  mlir::Block *header = nullptr;
  unsigned headerBlock = 0;
  unsigned bitWidth = 0;
  std::vector<State> states;
  AnalysisResult solution;
};

// Read-only analysis of a verified func.func SSACFG. Verification is repeated
// at this boundary. Recognizes one reachable natural loop with a unique entry
// edge, backedge, and exit condition, and an unconditional cycle path.
// Unsupported reachable operations or CFG shapes fail closed with no forms.
// Unreachable blocks are verified but excluded from recurrence extraction.
//
// Results describe header state after t completed backedges, conditional on
// reaching that header visit. This API does not infer a trip count, prove
// termination, or equate header states with function return/loop exit values.
Result analyze(mlir::func::FuncOp function, const Options &options = {});

// Stable, structural serialization, independent of SSA names and addresses.
std::string formatExpression(const Expression &expression);
std::string formatResult(const Result &result);

} // namespace systolic::polynomial::cfg
