//===- LoopInvariant.h - Inductive invariant path for one scalar loop -*- C++ -*-===//
//
// The inductive invariant path of the bit-vector adaptation
// (docs/GENERALIZED_GEMM_STATEMENT_BV.md, Section 13). A function candidate F for
// the values a scalar function returns is proved, for every parameter in the
// scope, by an invariant certificate Inv at the header of the function's one
// loop, through three kinds of counterexample queries that all must be unsat:
//
//   INV-init   scope ∧ Pre(s) ∧ ¬Inv(s)               per path entry → header
//   INV-step   scope ∧ Inv(s) ∧ G(s) ∧ T(s,s') ∧ ¬Inv(s')  per path header → header
//   INV-exit   scope ∧ Inv(s) ∧ X(s) ∧ ¬Post(s)        per path header → return
//
// plus RET-bypass, scope ∧ X(s) ∧ ¬Post(s), for a path from the entry to a
// return that never reaches the header. Pre, G, T, X and Post are read off the
// program's cut paths (CutPaths.h), so they are the program's own semantics:
// the operations of a block in their order (x++ reads x before the update),
// the counter's update on the back edge, a variable no path assigns kept as it
// was (the frame), the guards, and the no-stop conditions of every block passed.
//
// G is the disjunction of the conditions of the paths that return to the header
// and X the condition of a path that leaves it for a return. ¬G alone also holds
// at a state stuck on a side condition (undefined behaviour); such a run returns
// nothing, and partial correctness asks nothing of it, so the exit query uses X.
//
// Every term is a bit vector at the program's width, values wrap, and a flag
// such as nsw only sets a poison bit (SemanticTerm.h, invariant I2). The proof is
// therefore about the program as written: Post compares bits, modulo 2^w. A
// mathematical-integer statement is not made. Whether a returned value can be
// poison is a separate, informational query (INV-poison).
//
// All three unsat proves partial correctness only: a run that returns, returns
// F. That every run returns is the termination analysis's question (G2a); the
// caller joins the two, and an invariant alone is never called a termination
// proof.
//
// A model of INV-init or INV-step refutes the invariant candidate, not the
// function candidate. A model of INV-exit may sit at a state no run reaches (the
// invariant may be too weak): it refutes the function candidate only when a run
// of the program itself, from the model's parameters, returns another value.
// Unknown, a timeout and a resource limit refute nothing and prove nothing.
//
//===----------------------------------------------------------------------===//
#ifndef GENERALIZEDGEMM_LOOPINVARIANT_H
#define GENERALIZEDGEMM_LOOPINVARIANT_H

#include "GeneralizedGemm/ConcreteEvaluator.h"
#include "GeneralizedGemm/CutPaths.h"
#include "GeneralizedGemm/IndexSlice.h"
#include "GeneralizedGemm/SmtBackend.h"
#include "GeneralizedGemm/TerminationCertificate.h"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace mlir::generalized_gemm {

/// The function's one loop and the paths around it. Stage 1 supports a function
/// with integer parameters and results, no memory root, no float and exactly one
/// cut; anything else leaves `reason` set and nothing else filled in.
struct InvariantLoop {
  unsigned header = 0;
  PathSet set;                      // Complete.
  std::vector<unsigned> entry;      // entry → header: Pre.
  std::vector<unsigned> iterate;    // header → header: G and T.
  std::vector<unsigned> leave;      // header → return: X and the returned values.
  std::vector<unsigned> bypass;     // entry → return without the header.
  // Per path of `leave` and `bypass`: the returned values and their poison bits,
  // over the configuration at the start of the path.
  std::map<unsigned, std::vector<Term>> returns, returnPoison;
  // The header state an invariant may read: integer variables every entry path
  // assigns that reach a return, a loop condition or each other's updates.
  std::vector<unsigned> variables;
  std::vector<unsigned> parameters; // Integer parameters, in order.
  std::vector<Sort> resultSorts;
  std::string reason;               // Nonempty: not supported, and why.

  bool supported() const { return reason.empty(); }
};

InvariantLoop invariantLoop(const SemanticProgram &program, const SliceLimits &limits = {});

/// A function candidate: per returned value, a term over the parameters alone, or
/// nullptr for a value the candidate says nothing about.
struct LoopFunction {
  std::vector<Term> results;
  std::string origin; // Where it came from. Not evidence.
};

/// Untrusted: Inv is the conjunction of the atoms, each a BV1 term over the header
/// state `loop.variables`, their poison bits and the parameters.
struct InvariantCertificate {
  std::vector<Term> atoms;
  std::string origin; // Which search proposed it. Not evidence.
};

enum class InvariantVerdict { Inductive, NotInductive, Unknown };
const char *stringifyInvariantVerdict(InvariantVerdict verdict);

enum class LoopFunctionVerdict { Proved, Refuted, Unknown };
const char *stringifyLoopFunctionVerdict(LoopFunctionVerdict verdict);

/// A run of the program itself that returns another value than the candidate.
struct LoopCounterexample {
  std::vector<llvm::APInt> parameters;
  std::vector<llvm::APInt> returned;              // By the run.
  std::vector<std::optional<llvm::APInt>> expected; // The candidate's values.
  uint64_t steps = 0;
  std::string source; // Which query or sample proposed the parameters.
};

struct InvariantCheckOptions {
  ParameterScope scope;
  SmtLimits solver;
  // Stop at the first obligation a model falsifies (the search's loop). Off, every
  // obligation runs and is recorded.
  bool failFast = true;
  // INV-exit models are confirmed by running the program from their parameters;
  // when that run cannot decide, the query is asked again with every parameter
  // in [-bound, bound] (within the scope) for each bound in turn.
  RunLimits confirmation;
  std::vector<int64_t> smallInputBounds = {8, 64};
  bool scripts = true; // Keep each obligation's full SMT-LIB in its record.
};

struct CheckedLoopInvariant {
  InvariantVerdict invariant = InvariantVerdict::Unknown;
  std::string invariantReason;
  // For NotInductive: the obligation, and the atoms its model makes false where
  // Inv must hold (at an initial state for INV-init, after the step for INV-step).
  std::string falsifiedBy;
  std::vector<unsigned> falsified;
  // Atoms whose own obligation the solver did not decide (unknown or a limit), when
  // the conjunction was split atom by atom: not refuted, not proved.
  std::vector<unsigned> undecided;
  LoopFunctionVerdict function = LoopFunctionVerdict::Unknown;
  std::string functionReason;
  std::optional<LoopCounterexample> counterexample; // Only for Refuted.
  // An INV-exit model no run confirmed, kept as text: it refutes nothing.
  std::string unconfirmedExitModel;
  std::vector<std::string> eliminated; // "s1 := ..." used to state the queries.
  std::vector<std::string> lemmas;     // Proved exit equalities (INV-exit-eq).
  std::string poison = "not checked";  // "excluded", "not excluded", "not checked".
  std::vector<ObligationRecord> records;
  unsigned queries = 0;
  size_t scriptBytes = 0;
  unsigned confirmationRuns = 0;
  uint64_t confirmationSteps = 0;
};

/// INV-init, INV-step, INV-exit (with INV-exit-eq lemmas), RET-bypass and
/// INV-poison for `certificate` and `function`. Every query is stated after
/// replacing each header variable that an atom of Inv (or a proved INV-exit-eq
/// lemma) defines, v = t with v not in t, by t: the substituted query is
/// equisatisfiable with the literal one, since Inv holds in both.
CheckedLoopInvariant checkLoopInvariant(const SemanticProgram &program, const InvariantLoop &loop,
                                        const InvariantCertificate &certificate,
                                        const LoopFunction &function,
                                        const InvariantCheckOptions &options, SmtBackend &backend);

/// Reachable header states from concrete runs, for proposers. Values are parallel
/// to loop.variables, then the poison bits of loop.variables, then loop.parameters.
struct LoopSamples {
  std::vector<std::vector<llvm::APInt>> states;
  // Per run: its parameters and, when it returned, the values it returned.
  std::vector<std::vector<llvm::APInt>> runParameters;
  std::vector<std::optional<std::vector<llvm::APInt>>> runReturns;
  uint64_t runs = 0, returned = 0, steps = 0, arrivals = 0, maxArrivals = 0;
};
LoopSamples sampleLoop(const SemanticProgram &program, const InvariantLoop &loop,
                       llvm::ArrayRef<std::vector<llvm::APInt>> points, const RunLimits &limits,
                       size_t maxStates);

/// Parameter points for runs: every point of the scope when every parameter has a
/// range and their product has at most `maxPoints` points (`exhaustive` true);
/// otherwise a deterministic selection of small values inside the scope.
std::vector<std::vector<llvm::APInt>> loopPoints(const SemanticProgram &program,
                                                 const ParameterScope &scope, uint64_t maxPoints,
                                                 bool &exhaustive);

/// The finite tier, the fallback of the invariant path: the function candidate
/// against complete runs of the program at `points`. Every iteration of every run
/// is executed: this is the unrolled check, never an invariant proof.
struct LoopRunCheck {
  uint64_t points = 0, returned = 0, stopped = 0, outOfFuel = 0, equal = 0;
  uint64_t steps = 0;
  bool exhaustive = false; // `points` is every point of the scope.
  std::optional<LoopCounterexample> counterexample;
  // ScopeEnumerated: exhaustive, every run returned and equal (a complete bounded
  // check of that scope). SampleConsistent: equal wherever a run returned, and
  // not every point of the scope ran or returned. Refuted: a run differs.
  // Unknown: no run returned.
  std::string status = "UNKNOWN";
};
LoopRunCheck checkLoopFunctionOnRuns(const SemanticProgram &program, const InvariantLoop &loop,
                                     const LoopFunction &function,
                                     llvm::ArrayRef<std::vector<llvm::APInt>> points, bool exhaustive,
                                     const RunLimits &limits);

/// Infix text of a term over p<id>, s<id> and sp<id>: + - * (wrapping), /u %u,
/// == != <s <=s >s >=s <u <=u >u >=u, && || !, decimal constants (signed when
/// the top bit is set). Fully parenthesized; parseLoopTerm reads it back.
std::string formatLoopTerm(Term term);

/// Parses the text of formatLoopTerm, and the same with fewer parentheses (the
/// usual precedence: unary, *, + -, comparisons, &&, ||). A constant takes the
/// width of the other operand. Leaves must exist in `program`. Nullptr with `why`.
Term parseLoopTerm(const SemanticProgram &program, const std::string &text, std::string &why);

/// The atoms of a conjunction: splits && at every level.
std::vector<Term> conjuncts(Term term);

/// What the loop's paths say, as text: Pre, G, T with its frame, X and Post.
std::string invariantSpecificationJSON(const SemanticProgram &program, const InvariantLoop &loop,
                                       const LoopFunction &function, const ParameterScope &scope);

} // namespace mlir::generalized_gemm
#endif
