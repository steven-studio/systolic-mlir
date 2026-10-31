#pragma once

#include "Systolic/PolynomialSummation.h"
#include "Systolic/SymbolicPolynomial.h"
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

// P2a opt-in boundary. The P1 API and its constant-only contract are unchanged.
struct SymbolicOptions {
  Options integers;
  // Explicit closed domains for function arguments, indexed by argument
  // number. Default: all signed representatives of the argument's bit width.
  // These are caller constraints, not facts inferred from loop guards.
  std::map<unsigned, std::pair<Integer, Integer>> parameterRanges;
};
struct Parameter {
  unsigned argument;
  std::string name; // p<function argument number>
  unsigned bitWidth;
  Integer lower, upper;
};
struct SymbolicState {
  unsigned argument;
  std::string name;
  Expression initial; // parameter-only expression; no current/next states
  Expression next, increment;
  std::vector<unsigned> dependencies; // state dependencies, excluding parameters
};
struct SymbolicResult {
  Status status = Status::Unknown;
  std::string reason;
  SymbolicOptions options;
  mlir::Block *header = nullptr;
  unsigned headerBlock = 0, bitWidth = 0;
  std::vector<Parameter> parameters;
  std::vector<SymbolicState> states;
  std::map<std::string, SymbolicPolynomial> integerLifts;
  // Requires exactly the reported parameters, in their explicit domains.
  // Modular results are residues; integer results depend on the full contract.
  Integer valueAt(const std::string &state, const Integer &iteration,
                  const SymbolicPolynomial::Assignment &values) const;
};
SymbolicResult analyzeSymbolic(mlir::func::FuncOp function,
                              const SymbolicOptions &options = {});
std::string formatResult(const SymbolicResult &result);

} // namespace systolic::polynomial::cfg
