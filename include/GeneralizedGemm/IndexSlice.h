//===- IndexSlice.h - The slice of a subscript and its loop spec ---*- C++ -*-===//
//
// Phase 2 of the bit-vector adaptation of the thesis method (Section 4.4,
// Appendix G.3, Appendix F.7; docs/GENERALIZED_GEMM_STATEMENT_BV.md).
//
// The slice of a subscript is the backward slice on the transition system: the
// state variables whose values reach the subscript, the assignments to them and
// the guards that select those assignments, transitively. The transitions of a
// SemanticProgram are the τ_b and γ_e of the thesis's Definition 3.1, so this is
// the slice the thesis describes and did not implement.
//
// For an innermost loop, the slice gives the thesis's specification (F.2):
//
//   f_v(0) = init_v         ∀m: c(F(m)) ⇒ f_v(m + 1) = τ_v(F(m))
//
// where c is the loop condition, τ_v the one-iteration update of v and F the
// vector of closed forms. Here every term is a bit vector at the program's
// width, m ranges over [0, 2^w − 2] and parameters over the stated scope, so a
// discharged obligation proves the closed form AT THAT WIDTH AND SCOPE and nothing
// else. The solver's unsat is the only proof; sat gives a model and unknown, a
// timeout or a resource limit give Unknown.
//
// The specification is checked against the program itself by replay: every
// arrival at the header during a concrete run must satisfy c and τ from the
// previous arrival, and the subscript evaluated at the event's block must equal
// the recorded index. That is test evidence for the stated points.
//
//===----------------------------------------------------------------------===//
#ifndef GENERALIZEDGEMM_INDEXSLICE_H
#define GENERALIZEDGEMM_INDEXSLICE_H

#include "GeneralizedGemm/ConcreteEvaluator.h"
#include "GeneralizedGemm/CutPaths.h"
#include "GeneralizedGemm/IndexProgram.h"
#include "GeneralizedGemm/SmtBackend.h"
#include "GeneralizedGemm/TerminationCertificate.h"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace mlir::generalized_gemm {

/// A loop of the slice, with the specification's ingredients. The iteration
/// count m and the entry values are synthetic State leaves whose ids start at
/// program.state.size(); nothing in the program reads them.
struct LoopSpecification {
  unsigned header = 0;
  bool innermost = false;       // No path from the header reaches a cut it dominates.
  std::vector<unsigned> paths;  // Cut paths from the header back to it.
  std::vector<unsigned> component; // The natural loop: locations the header dominates that reach it.
  std::vector<unsigned> carried;   // Slice variables the loop assigns, closed under what c and τ read.
  std::vector<unsigned> fixed;     // Variables read by c, τ or the subscript, not carried.
  std::vector<unsigned> defined;   // Fixed variables replaced by their entry definitions.
  Term condition = nullptr;        // c, over the state at the header.
  std::vector<Term> step;          // τ_v per carried variable, over the state at the header.
  std::vector<Term> init;          // Per carried variable: over parameters, or nullptr (symbolic).
  Term iteration = nullptr;        // m.
  std::vector<Term> entry;         // entry_v per carried variable.
  std::string reason;              // Why no specification was built, when empty.
};

struct SliceUse {
  unsigned site = 0, operand = 0, dimension = 0; // operand: 0 A, 1 B, 2 C.
};

struct IndexSlice {
  Term subscript = nullptr;
  std::vector<SliceUse> uses;
  std::vector<unsigned> variables;    // The closure.
  std::vector<unsigned> transitions;  // Indices into program.transitions that assign them.
  std::vector<Term> guards;           // Their guards.
  std::vector<LoopSpecification> loops;
  std::string reason;
};

struct SliceLimits {
  PathLimits paths;
};

/// Every distinct subscript of every MAC site, with its slice. Nullopt with `why`
/// when the cut set or the paths cannot be had.
std::optional<std::vector<IndexSlice>> sliceSubscripts(const SemanticProgram &program,
                                                       const SliceLimits &limits,
                                                       std::string &why);

/// One activation of the loop in a concrete run: the values at m = 0, 1, ...
struct LoopTrace {
  std::vector<llvm::APInt> parameters;
  std::vector<llvm::APInt> entry;                 // Carried values at m = 0.
  std::vector<llvm::APInt> fixedValues;           // Parallel to spec.fixed, at m = 0.
  std::vector<std::vector<llvm::APInt>> states;   // Per m: carried values.
};

struct LoopReplay {
  RunStatus status = RunStatus::InvalidEntry;
  std::string reason;
  std::vector<LoopTrace> traces;
  uint64_t iterations = 0;          // Header arrivals after the first of an activation.
  uint64_t stepMismatches = 0;      // τ(previous) differs from the arrival.
  uint64_t conditionMismatches = 0; // c false before a continued iteration, or true at exit.
  uint64_t subscriptMatches = 0, subscriptMismatches = 0; // Against the recorded events.
  uint64_t undefined = 0;           // A fixed or carried variable undefined at a header.
};

/// Replays one entry state against the specification and the subscript.
LoopReplay replayLoop(const SemanticProgram &program, const IndexSlice &slice,
                      const LoopSpecification &spec, const EntryState &entry,
                      const RunLimits &limits);

/// A proposed closed form of a carried variable: a loop-free term over m, the
/// entry leaves, the fixed variables and the parameters; or (phase 4) a while
/// program of the fixed-counter template (IndexProgram.h, CounterTemplate) whose
/// inputs are those same leaves, with `term` null.
struct ClosedForm {
  unsigned variable = 0;
  Term term = nullptr;
  const IndexProgram *program = nullptr;
  // Phase 5, for a history table program: the carried variables proposed to trail
  // the variable by 1, 2, ... iterations (T[m-1], T[m-2], ...). A proposal the
  // TT-shift obligations check; empty, and the checker looks for them itself.
  std::vector<unsigned> history;
};

/// Proved: every obligation unsat at the width and scope, and every form the
/// obligations substituted is itself Proved. Refuted: a model, or a sampled
/// iteration the candidate misses. SampleConsistent: a program checked at the
/// finite tier only, equal to every sampled iteration of every replayed point; it
/// is not a proof of anything beyond those samples. Unknown: everything else, a
/// solver's unknown, a limit, a run cut by its fuel, a missing form, a model at a
/// free leaf (which may be unreachable), a dependence on an unproved form.
enum class ClosedFormStatus { Proved, Refuted, SampleConsistent, Unknown };
const char *stringifyClosedFormStatus(ClosedFormStatus status);

struct CheckedClosedForm {
  unsigned variable = 0;
  ClosedFormStatus status = ClosedFormStatus::Unknown;
  std::string reason;
  std::vector<ObligationRecord> records;
  // A refuting assignment of the leaves, by SMT name: m, entry_s<v>, s<id>, p<id>.
  std::vector<std::pair<std::string, llvm::APInt>> model;
  // Phase 4. Carried variables whose forms the obligations substituted: the
  // induction is simultaneous, so this form is proved only with them.
  std::vector<unsigned> dependsOn;
  // Carried variables the obligations read as free leaves (no form given, or the
  // while candidate's own variable). A model over a free leaf is no refutation.
  std::vector<unsigned> freeLeaves;
  // Every obligation of this form alone came back unsat.
  bool discharged = false;
};

/// The obligations of (F.2) for each form, at the width and under `scope`.
///
/// Loop-free form f_v:
///   CF-init   scope ∧ f_v(0) ≠ init_v
///   CF-step   scope ∧ m ≠ 2^w−1 ∧ c(F(m)) ∧ f_v(m+1) ≠ τ_v(F(m))
/// While program W_v of the fixed-counter template with body S:
///   W-shape   structural: matchCounterTemplate, inputs among the leaves
///   W-init    scope ∧ init_W ≠ init_v
///   W-step    scope ∧ m ≠ 2^w−1 ∧ c(F(m)) ∧ S[v := s_v, r := m] ≠ τ_v(F(m))
///   W-term    r < m ∧ ¬((m − (r+1)) <u (m − r))      (the variant m − r)
/// Table program of the history template (phase 5, G.5) with inits k_j, body E,
/// depth d and history chain v_0 = v, v_1, ..., v_{d-1} (carried variables):
///   TT-shape   structural: matchTableTemplate, the chain found or proposed
///   TT-init_j  scope ∧ c(F(0)) ∧ ... ∧ c(F(j−1)) ∧ v(j) ≠ k_j  (F unrolled from the entry)
///   TT-step    scope ∧ m ≠ 2^w−1 ∧ m ≥ d−1 ∧ c(F(m)) ∧ E[r := m+1, T[r−j] := v_{j−1}] ≠ τ_v(F(m))
///   TT-shift_j scope ∧ m ≠ 2^w−1 ∧ c(F(m)) ∧ τ_{v_j}(F(m)) ≠ v_{j−1}
///   TT-bounds  d ≤ r < m+1 ∧ ¬(r < m+d ∧ r−j < r for j = 1..d)
///   TT-term    r < m+1 ∧ ¬((m+1 − (r+1)) <u (m+1 − r))
/// where F(m) reads every carried variable with a form as that form and every
/// other one as a free leaf (the actual value at iteration m; sound, since the
/// obligation then holds for every value). The first form listed for a variable
/// stands for it in the others' obligations. A form is Proved only when every
/// form it substituted is Proved as well (one fixpoint over the list). A sat
/// answer on an obligation that reads a free leaf is Unknown with the model kept:
/// the model may sit at an unreachable value, so the caller replays it.
std::vector<CheckedClosedForm> checkClosedForms(const SemanticProgram &program,
                                                const LoopSpecification &spec,
                                                llvm::ArrayRef<ClosedForm> forms,
                                                const ParameterScope &scope,
                                                const SmtLimits &limits, SmtBackend &backend);

/// Runs the specification itself from the given entry for `iterations` steps or
/// until c fails: the trace the spec predicts. Nullopt when a term has no value.
std::optional<LoopTrace> runSpecification(const SemanticProgram &program,
                                          const LoopSpecification &spec,
                                          const std::vector<llvm::APInt> &parameters,
                                          const std::vector<llvm::APInt> &entry,
                                          const std::vector<llvm::APInt> &fixedValues,
                                          uint64_t iterations);

/// A statement program (IndexProgram.h), with inputs among m, the entries, the
/// fixed variables and the parameters, run with `fuel` at every sampled iteration
/// of every trace and compared with the carried variable `k` there. The result is
/// SampleConsistent, Refuted with the failing inputs as the model, or Unknown
/// when a run exhausts its fuel before every sample is judged: a cut run has no
/// value and says nothing about termination.
CheckedClosedForm checkProgramOnTraces(const SemanticProgram &program,
                                       const LoopSpecification &spec, unsigned k,
                                       const IndexProgram &candidate,
                                       llvm::ArrayRef<LoopTrace> traces, uint64_t fuel);

/// Phase 4 (I2). State variables that every path from the entry to a cut defines
/// by one and the same term over parameters and constants, and that no path
/// between cuts assigns: a materialized constant, a bound computed before the
/// loops. Each maps to that term. Empty with `why` when the paths cannot be had.
std::map<unsigned, Term> entryDefinedState(const SemanticProgram &program, const SliceLimits &limits,
                                           std::string &why);

/// Text of a closed-form leaf: m, entry_s<v>, s<id>, p<id>.
std::string closedFormLeafName(const SemanticProgram &program, const LoopSpecification &spec,
                               Term leaf);

} // namespace mlir::generalized_gemm
#endif
