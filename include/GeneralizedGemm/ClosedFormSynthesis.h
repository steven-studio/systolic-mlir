//===- ClosedFormSynthesis.h - Closed forms of slice variables -----*- C++ -*-===//
//
// Phase 2 of the bit-vector adaptation (docs/GENERALIZED_GEMM_STATEMENT_BV.md):
// the thesis's "synthesis from the model" (Appendix F.7) with the project's own
// enumerator in place of cvc5. For every innermost loop of every subscript's
// slice and every variable the loop carries, a candidate closed form f_v over the
// iteration count m, the entry values, the fixed variables and the parameters is
// found from the sampled traces, then put to checkClosedForms. A refuting model
// is replayed through the specification into a new trace, and the search runs
// again (CEGIS), up to a round limit.
//
// A candidate is a guess from finitely many samples. Only the checker's PROVED,
// which is the solver's unsat on both obligations at the width and scope, says
// anything; REFUTED carries a model, and UNKNOWN covers the solver's unknown, a
// timeout, a resource limit, a missing candidate and an unsupported width.
//
//===----------------------------------------------------------------------===//
#ifndef GENERALIZEDGEMM_CLOSEDFORMSYNTHESIS_H
#define GENERALIZEDGEMM_CLOSEDFORMSYNTHESIS_H

#include "GeneralizedGemm/IndexSlice.h"

#include <map>
#include <string>
#include <vector>

namespace mlir::generalized_gemm {

struct ClosedFormOptions {
  ParameterScope scope;
  SmtLimits solver;
  SliceLimits slice;
  unsigned maxRounds = 6;       // CEGIS rounds per loop.
  unsigned maxTermSize = 5;     // Expression size: leaves and binary nodes.
  size_t maxTerms = 200000;     // Distinct expressions on the samples.
  size_t maxTests = 120000;     // Conditions tried for a branch.
  size_t maxSamples = 2048;
  uint64_t fuel = 1u << 20, maxCells = 1u << 20, referenceFill = 0;
  std::vector<llvm::APInt> extraConstants;
  // Phase 3: a while program when no loop-free form is found or proved:
  //   r := 0 ; v := init ; while r < m do (v := E ; r := r + 1) ; return v
  // with E over v, r, the inputs, the constants and the proved forms of the other
  // carried variables at r. Checked at the finite tier only, run with `programFuel`
  // per sample; a run cut by the fuel is Unknown, never a claim about termination.
  bool whilePrograms = true;
  unsigned maxBodySize = 5, maxWhileRounds = 6;
  size_t maxBodies = 40000;
  uint64_t programFuel = 1u << 12;
  // Phase 4: a while candidate that equals every sampled iteration is put to the
  // W obligations of checkClosedForms (shape, init, step, termination). Only all
  // of them unsat makes it PROVED; a solver's unknown or a model at a free leaf
  // that does not replay leaves it SAMPLE_CONSISTENT; a model that replays into a
  // disagreeing trace refutes it and feeds the search.
  bool proveWhile = true;
  // Phase 5 (G.5): the history table template for a variable no loop-free form or
  // while program covers:
  //   T[0] := k_0 ; ... ; T[d-1] := k_{d-1} ; r := d ;
  //   while r < m + 1 do (T[r] := E(r, T[r-1], ..., T[r-d], inputs) ; r := r + 1) ; return T[m]
  // with d up to maxHistory, the inits read off the traces, E found by the same
  // goal-directed enumerator, the program judged at the finite tier and then put to
  // the TT obligations of checkClosedForms. Only that template: a second array, a
  // higher rank or another write index is not searched (the record says so).
  bool tablePrograms = true;
  unsigned maxHistory = 3, maxTableRounds = 6;
};

struct SynthesizedClosedForm {
  unsigned variable = 0;
  std::string kind = "loop-free"; // Or "while", or "table" (phase 5).
  Term term = nullptr;      // The last loop-free candidate, or nullptr.
  std::optional<IndexProgram> program; // The while candidate, when kind is "while".
  std::string text;         // The candidate in the statement grammar, with named leaves.
  ClosedFormStatus status = ClosedFormStatus::Unknown;
  std::string reason;
  unsigned rounds = 0;      // Candidates checked.
  unsigned counterexampleTraces = 0; // Traces a failing while candidate added to its seeds.
  unsigned symbolicRounds = 0;       // Phase 4: while candidates put to the W obligations.
  std::vector<unsigned> history;     // Phase 5: the carried variables trailing the variable (table).
  uint64_t fuelCuts = 0;    // Runs of while candidates cut by the fuel during the search.
  std::vector<ObligationRecord> records; // Of the last check.
  std::vector<std::pair<std::string, llvm::APInt>> model; // Of the last refutation.
};

struct LoopSynthesis {
  LoopSpecification spec;
  std::vector<SynthesizedClosedForm> forms;
  // Replay over the given points: the specification against the program.
  unsigned points = 0, activations = 0;
  uint64_t iterations = 0, stepMismatches = 0, conditionMismatches = 0;
  uint64_t subscriptMatches = 0, subscriptMismatches = 0, undefined = 0;
  size_t samples = 0;
};

struct SliceSynthesis {
  IndexSlice slice;
  std::vector<LoopSynthesis> loops;
};

struct SliceSynthesisResult {
  std::vector<SliceSynthesis> slices;
  std::string reason; // Nonempty when no slice could be taken.
};

SliceSynthesisResult synthesizeClosedForms(const SemanticProgram &program,
                                           llvm::ArrayRef<std::vector<llvm::APInt>> points,
                                           const ClosedFormOptions &options, SmtBackend &backend);

/// The closed-form grammar's bottom-up enumerator (Section 7), shared with the
/// inductive invariant path (Section 13). Constants first (0 to 8, the program's
/// literals of the width, `extraConstants`), then the leaves of the width, then
/// terms by size with +, − and ×, and udiv and urem by a constant above one, up to
/// `maxTermSize`, keeping one term per distinct value vector on the first `keyRows`
/// rows (observational equivalence), at most `maxTerms` of them. The defaults are
/// the closed-form search's own; nothing below changes it.
struct GrammarTerm {
  Term term = nullptr;
  unsigned size = 0;
  std::vector<llvm::APInt> values; // Per row.
};
struct TermGrammar {
  unsigned maxTermSize = 5;
  size_t maxTerms = 200000;
  std::vector<llvm::APInt> extraConstants;
  bool division = true; // udiv and urem by a constant above one.
  // The invariant path's polynomial template: a product only of a constant (on the
  // left) and another term, so a term's degree is the largest degree of its leaves;
  // and each sum once, not again with its operands swapped.
  bool polynomial = false;
  // Report a later term whose key equals an earlier term's when the two differ on a
  // row past `keyRows` (a witness row): an equality the key rows show that is not an
  // identity. At most `collisionsPerTerm` per earlier term.
  bool collisions = false;
  unsigned collisionsPerTerm = 2;
};
struct GrammarEnumeration {
  std::vector<GrammarTerm> pool;
  std::vector<unsigned> constants;                // Indices into pool.
  std::map<std::vector<uint64_t>, unsigned> seen; // Key -> index into pool.
  std::vector<std::pair<unsigned, GrammarTerm>> collisions; // Earlier index, later term.
};
/// `leafValues[l]` holds one value per row for `leaves[l]`; rows past `keyRows` are
/// witness rows. Widths up to 64.
GrammarEnumeration enumerateGrammar(const SemanticProgram &program, llvm::ArrayRef<Term> leaves,
                                    llvm::ArrayRef<std::vector<llvm::APInt>> leafValues,
                                    size_t keyRows, unsigned width, const TermGrammar &grammar);

/// Deterministic JSON of the result.
std::string closedFormsJSON(const SemanticProgram &program, const SliceSynthesisResult &result);

} // namespace mlir::generalized_gemm
#endif
