//===- IndexInvariant.h - Inductive invariants for GEMM subscripts -*- C++ -*-===//
//
// The inductive invariant path, for the scalar state that the subscripts of a MAC
// site read (docs/GENERALIZED_GEMM_STATEMENT_BV.md, Section 14). The program is a
// whole GEMM: it has memory, floats or integers in the cells, and nested loops, so
// it has one cut per loop header. An untrusted certificate gives, per cut c, a
// conjunction Inv_c of atoms over the integer state, its poison bits and the
// parameters. Three kinds of counterexample queries, all of which must be unsat:
//
//   IDX-init  scope ∧ cond(π) ∧ ¬Inv_b(post π)             per path entry → cut b
//   IDX-step  scope ∧ Inv_a ∧ cond(π) ∧ ¬Inv_b(post π)     per path cut a → cut b
//   IDX-use   scope ∧ Inv_a ∧ premise(ρ) ∧ index(end ρ) ≠ R(end ρ)
//                                                          per arrival ρ at a site
//
// IDX-init and IDX-step are the initiation and consecution of G2a's O3 and O4
// (TerminationObligations.h) over the same cut paths, so cond, post and the frame
// are the program's own semantics: bit vectors at the program's widths, wrapping,
// with a flag such as nsw only setting a poison bit. A path that enters an inner
// loop from an outer header is a path between cuts like any other: what the inner
// loop may assume on entry is what the outer cut's invariant and that path give,
// and nothing else. No obligation iterates a loop: there is one query per path
// between cuts, whatever the trip counts.
//
// IDX-use is the conclusion at the place a subscript is used. ρ ranges over the
// arrivals at the site's block from the entry or a cut (SymbolicDomain.h): its
// premise holds the guards and the no-stop conditions up to and including the
// site's block, since a block that stops contributes no event. R is a claimed
// index relation: a term over the state at the entry of the site's block and the
// parameters. With a coordinate family, the checker also says whether R reads
// state only through the coordinates of the operand's axes (P, I, K for A; P, K, J
// for B; P, I, J for C); only then does the proved equality say that the address is
// that function of those coordinates.
//
// What is proved, and what is not. An inductive Inv_c holds at every arrival at c
// of every run from the scope. A proved relation holds at every event of the site
// in every such run. Neither is a statement about the cells' values, the domain's
// coverage, aliasing, bounds or a lowering; those belong to the other checkers.
// A model of IDX-init or IDX-step refutes the invariant candidate, never the
// program: the state of an IDX-step model need not be reachable. A model of
// IDX-use refutes the relation only when a run of the program from the model's
// parameters returns and has an event whose recorded subscript differs from the
// relation's value there. Unknown, a timeout and a limit prove and refute nothing.
//
// A consumer is never handed a flag saying "proved". The invariants reach
// checkFamilySymbolic and the other checkers as the invariants of a termination
// certificate (strengthenCertificate), and each of those checkers runs
// checkCertificate on the certificate it is given: O3 and O4 are asked again there,
// by code that links nothing of this file's callers.
//
//===----------------------------------------------------------------------===//
#ifndef GENERALIZEDGEMM_INDEXINVARIANT_H
#define GENERALIZEDGEMM_INDEXINVARIANT_H

#include "GeneralizedGemm/CoordinateFamily.h"
#include "GeneralizedGemm/CutPaths.h"
#include "GeneralizedGemm/FiniteDomain.h"
#include "GeneralizedGemm/SmtBackend.h"
#include "GeneralizedGemm/TerminationCertificate.h"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace mlir::generalized_gemm {

struct CutInvariant {
  unsigned cut = 0;
  std::vector<Term> atoms; // Each BV1, over state, shadows and parameters.
};

/// Untrusted. A cut that is not listed has the invariant `true`.
struct IndexInvariantCertificate {
  std::vector<CutInvariant> cuts;
  std::string origin; // Which search proposed it, or "given". Not evidence.
};

/// Untrusted claim: at every event of `site`, subscript `dimension` of the operand
/// (0: A, 1: B, 2: the cell written) equals `relation`, a term over the state at the
/// entry of the site's block and the parameters.
struct IndexRelation {
  unsigned site = 0, operand = 0, dimension = 0;
  Term relation = nullptr;
  std::string origin; // Not evidence.
};

enum class IndexInvariantVerdict { Inductive, NotInductive, Unknown };
const char *stringifyIndexInvariantVerdict(IndexInvariantVerdict verdict);

enum class IndexRelationVerdict { Proved, Refuted, Unknown };
const char *stringifyIndexRelationVerdict(IndexRelationVerdict verdict);

/// A run of the program itself with an event at which the relation is false.
struct IndexRelationWitness {
  std::vector<llvm::APInt> parameters;
  uint64_t event = 0;         // Position among the run's events.
  llvm::APInt index, claimed; // The recorded subscript, and the relation's value there.
  uint64_t steps = 0;
};

struct CheckedIndexRelation {
  IndexRelation claim;
  IndexRelationVerdict verdict = IndexRelationVerdict::Unknown;
  std::string reason;
  // With a family: whether the relation reads state only through the coordinates of
  // the operand's axes. Structural; it labels the claim and proves nothing.
  std::optional<bool> overCoordinates;
  unsigned arrivals = 0;
  std::optional<IndexRelationWitness> witness; // Only for Refuted.
  std::string unconfirmedModel;                // A model no run confirmed: it refutes nothing.
};

struct IndexProofOptions {
  ParameterScope scope;
  SmtLimits solver;
  PathLimits paths;
  // Stop at the first IDX-init or IDX-step obligation a model falsifies (the search's
  // loop). Off, every obligation runs and is recorded.
  bool failFast = true;
  bool scripts = false; // Keep each obligation's full SMT-LIB in its record.
  unsigned maxQueries = 4096;
  // IDX-use models are confirmed by a run from their parameters with this memory fill.
  EnumerationLimits confirmation;
  uint64_t referenceFill = 0;
};

struct CheckedIndexProof {
  IndexInvariantVerdict invariant = IndexInvariantVerdict::Unknown;
  std::string invariantReason;
  // For NotInductive: the obligation, the cut at the end of its path, and the atoms of
  // that cut (positions in its list) the model makes false there.
  std::string falsifiedBy;
  unsigned falsifiedCut = 0;
  std::vector<unsigned> falsified;
  // Atoms of `undecidedCut` whose own obligation the solver did not decide.
  unsigned undecidedCut = 0;
  std::vector<unsigned> undecided;
  // For an IDX-init model inside the scope: its parameters. An initial state, so one a
  // run reaches; a proposer may run the program from it for more data.
  std::vector<llvm::APInt> initialParameters;
  std::string model; // The first failing obligation's model, as text.
  // Only for Inductive: the adopted conjunction per listed cut.
  std::map<unsigned, Term> invariants;
  std::vector<CheckedIndexRelation> relations; // Parallel to the claims. Empty unless Inductive.
  std::vector<ObligationRecord> records;
  unsigned queries = 0;
  size_t scriptBytes = 0;
  unsigned confirmationRuns = 0;
  uint64_t confirmationSteps = 0;
};

/// IDX-init and IDX-step for `certificate` over the paths between `cuts`, which must be
/// a cut set of the program (checked here); then, only when every one of them is unsat,
/// IDX-use for each relation at every arrival at its site. `family` may be null.
CheckedIndexProof checkIndexProof(const SemanticProgram &program, llvm::ArrayRef<unsigned> cuts,
                                  const IndexInvariantCertificate &certificate,
                                  llvm::ArrayRef<IndexRelation> relations,
                                  const CoordinateFamily *family, const IndexProofOptions &options,
                                  SmtBackend &backend);

/// `certificate` with the invariant of each cut conjoined with `invariants` at that
/// cut. Nothing is proved by building it: it is a proposal for checkCertificate, which
/// every consumer of a certificate runs again on what it is handed.
TerminationCertificate strengthenCertificate(const SemanticProgram &program,
                                             const TerminationCertificate &certificate,
                                             const std::map<unsigned, Term> &invariants);

/// Reachable states at the cuts, from concrete runs, for proposers: no obligation reads
/// one. Per cut, the integer state variables (at most 64 bits, independent of memory)
/// that have a value at every recorded arrival, and one row per kept arrival: those
/// variables, then their poison bits, then `parameters`.
struct CutSamples {
  struct Cut {
    unsigned cut = 0;
    std::vector<unsigned> variables;
    std::vector<std::vector<llvm::APInt>> rows;
    uint64_t arrivals = 0; // Every arrival of every run, kept or not.
  };
  std::vector<Cut> cuts;
  std::vector<unsigned> parameters; // The integer parameters, in order.
  uint64_t runs = 0, returned = 0, stopped = 0, steps = 0;
};
CutSamples sampleCutStates(const SemanticProgram &program, llvm::ArrayRef<unsigned> cuts,
                           llvm::ArrayRef<std::vector<llvm::APInt>> points, uint64_t referenceFill,
                           const EnumerationLimits &limits, size_t maxRowsPerCut);

} // namespace mlir::generalized_gemm
#endif
