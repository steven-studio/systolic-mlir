//===- InvariantSynthesis.h - Invariant certificates by search -----*- C++ -*-===//
//
// The proposer side of the inductive invariant path
// (docs/GENERALIZED_GEMM_STATEMENT_BV.md, Section 13). It proposes; every verdict
// comes from checkLoopInvariant (LoopInvariant.h) or from a run of the program.
//
// Function candidates: given, or found by the closed-form grammar's enumerator
// (enumerateGrammar) over the parameters and their monomials, fitted to the
// values sampled runs return. A run that returns another value refutes one, joins
// the samples, and the next candidate is sought (CEGIS over the function).
//
// Invariant candidates, a bounded polynomial template over the loop's header
// variables V and the parameters P (all at one width):
//
//   Inv  ::= Atom ∧ ... ∧ Atom                       (a conjunction; no ∨, no ∃)
//   Atom ::= E = E                                   (polynomial equality)
//          | x ⋈ y,  ⋈ ∈ {≤s, <s, ≤u, <u}, x, y ∈ V ∪ P ∪ {0}
//          | the loop's own comparisons and their non-strict forms
//          | ¬poison(v), v ∈ V
//   E    ::= a sum of at most (size + 1) / 2 summands (+, −), each a monomial of
//            degree ≤ d over V ∪ P, a constant (0 to 8 and the program's literals
//            of the width), or a constant times a smaller sum
//
// with (d, size) taken from `levels` in order. An equality is offered when the
// enumerator finds two distinct terms equal at every sampled reachable header
// state and different at some random witness state (so not an identity). A bound
// is offered when it holds at every sample and fails at some witness state.
// Houdini then drops whatever a model of INV-init or INV-step falsifies, until the
// conjunction is inductive; that is the strongest inductive conjunction of the
// offered atoms. If INV-exit then has a model no run confirms, the next level adds
// atoms; when the levels run out the search ends UNKNOWN, which says nothing about
// invariants outside the template. A proved certificate is minimized, each atom
// dropped if the rest still proves, and the result is checked once more.
//
// The fallback is the finite tier (checkLoopFunctionOnRuns): complete runs at the
// points of the scope, every iteration executed. Its status is reported as what it
// is: a complete bounded check of a finite scope, a refuting run, or samples.
//
//===----------------------------------------------------------------------===//
#ifndef GENERALIZEDGEMM_INVARIANTSYNTHESIS_H
#define GENERALIZEDGEMM_INVARIANTSYNTHESIS_H

#include "GeneralizedGemm/ClosedFormSynthesis.h"
#include "GeneralizedGemm/LoopInvariant.h"

#include <string>
#include <utility>
#include <vector>

namespace mlir::generalized_gemm {

struct InvariantSearchOptions {
  InvariantCheckOptions check;
  SliceLimits slice;
  // Runs for candidates only: no obligation reads a sample.
  uint64_t samplePoints = 48;
  RunLimits sampleRun;
  size_t maxSamples = 512;
  // Where equality candidates come from. "nullspace" (M1, the default): every
  // polynomial equality of degree <= d over the header variables and parameters that
  // all sampled states satisfy, as a basis of the null space of the sample matrix over
  // Q (Sharma et al., ESOP 2013, Section 4; Nguyen et al., ICSE 2012, Section III-A),
  // with variables that hold one constant at every sample folded first. "collisions":
  // the earlier term-collision template of the closed-form enumerator, at `levels`.
  // Either way the result is a set of candidates for the checker, never a proof.
  // "both" (the default): the null-space levels first, then the collision levels.
  std::string equalityCandidates = "both";
  std::vector<unsigned> nullSpaceDegrees = {1, 2};
  size_t maxColumns = 128;       // Monomials per sample matrix; a level above is skipped.
  unsigned maxRefinements = 8;   // INV-init models turned into new sampled runs.
  // The collision template: (monomial degree, expression size) per level, in order.
  std::vector<std::pair<unsigned, unsigned>> levels = {{1, 5}, {2, 3}, {2, 5}};
  size_t maxTerms = 200000;
  size_t maxEqualities = 400;
  unsigned witnessRows = 8;
  unsigned collisionsPerTerm = 2;
  unsigned maxHoudiniChecks = 64;
  bool minimize = true;
  unsigned maxMinimizeChecks = 96;
  // The fallback.
  uint64_t finitePoints = 256;
  RunLimits finiteRun;
  // Function candidates when none is given.
  bool synthesizeFunction = true;
  unsigned functionTermSize = 5;
  unsigned functionDegree = 2;
  unsigned maxFunctionRounds = 4;
};

struct InvariantSearchLevel {
  std::string method;  // "nullspace" or "collisions"
  unsigned degree = 0, size = 0;
  size_t equalities = 0, bounds = 0, poison = 0, offered = 0, kept = 0;
  unsigned checks = 0;
  // Null-space levels: matrix shape, rank, basis vectors offered, basis vectors whose
  // coefficients do not fit the width, variables folded to constants, INV-init models
  // that became new samples.
  size_t columns = 0, rows = 0, rank = 0, basis = 0, skipped = 0, folded = 0;
  unsigned refinements = 0;
  std::string outcome;
};

/// The null-space generator, exposed for tests. `rows` are sample rows as LoopSamples
/// lays them out. Returns atoms: `v == c` for a header variable holding one constant c
/// at every row (folded out of the monomials), then one equality per basis vector of
/// the null space over Q of the matrix of all monomials of degree <= `degree` over the
/// remaining header variables and the parameters (one width at a time; values read as
/// signed integers, products exact). Each vector, scaled to coprime integers, becomes
/// sum(c_k * m_k) = 0 at the program's width: an integer identity that holds on every
/// sample holds there modulo 2^w as well, but that is all; the checker decides.
struct NullSpaceStats {
  size_t columns = 0, rows = 0, rank = 0, basis = 0, skipped = 0, folded = 0;
};
std::vector<Term> nullSpaceEqualities(const SemanticProgram &program, const InvariantLoop &loop,
                                      llvm::ArrayRef<std::vector<llvm::APInt>> rows,
                                      unsigned degree, size_t maxColumns, NullSpaceStats &stats);

struct LoopInvariantSearch {
  InvariantLoop loop;
  LoopFunction function;            // The candidate judged last.
  bool functionGiven = false;
  std::vector<std::string> refutedFunctions; // Synthesized candidates a run refuted.
  std::optional<InvariantCertificate> certificate; // The one `check` is about.
  bool certificateGiven = false;
  CheckedLoopInvariant check;       // The checker's verdict on `certificate`.
  LoopRunCheck finite;              // The fallback.
  LoopSamples samples;
  uint64_t samplePoints = 0;
  std::vector<InvariantSearchLevel> levels;
  unsigned checks = 0, queries = 0; // Checker calls and solver queries of the search.
  size_t minimizedFrom = 0;
  // PROVED_BY_INVARIANT, REFUTED, SCOPE_ENUMERATED, SAMPLE_CONSISTENT, UNKNOWN or
  // UNSUPPORTED, and why.
  std::string status = "UNKNOWN", reason;
  std::string conflict; // Nonempty only for a defect: a proof and a refuting run.
};

/// `given` and `givenCertificate` may be null. With a certificate, it alone is
/// checked: no search, and no sample is taken for it.
LoopInvariantSearch proveLoopFunction(const SemanticProgram &program, const LoopFunction *given,
                                      const InvariantCertificate *givenCertificate,
                                      const InvariantSearchOptions &options, SmtBackend &backend);

/// Deterministic JSON. `termination` is the G2a verdict name, or empty when the
/// caller did not run it; the claim joins it with the invariant proof.
std::string loopInvariantJSON(const SemanticProgram &program, const LoopInvariantSearch &result,
                              const InvariantSearchOptions &options, const std::string &termination,
                              const std::string &terminationReason, bool scripts);

} // namespace mlir::generalized_gemm
#endif
