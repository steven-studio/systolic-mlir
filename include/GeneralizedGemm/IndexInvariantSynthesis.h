//===- IndexInvariantSynthesis.h - Index invariants by search ------*- C++ -*-===//
//
// The proposer side of IndexInvariant.h (docs/GENERALIZED_GEMM_STATEMENT_BV.md,
// Section 14). It proposes; every verdict comes from checkIndexProof, from
// checkCertificate, or from a run of the program.
//
// Invariant candidates, per cut of the termination certificate's cut set, from the
// header states that sampled runs reach there (a run is a candidate source only):
//
//   v == c                 a variable with one value at every sample (folded)
//   Σ c_k · m_k == 0       a basis of the null space over Q of the matrix of the
//                          monomials m_k of degree <= d over the remaining integer
//                          state and the parameters (M1, InvariantSynthesis.h),
//                          written at the program's width
//   ¬poison(v)             for a variable some transition can poison
//
// Houdini over all cuts at once: the conjunctions are checked together, the atoms a
// model of IDX-init or IDX-step falsifies at the end of its path are dropped, and the
// check runs again, until it is inductive. An IDX-init model is an initial state, so
// its parameters become one more sampled run and the candidates are derived again
// (the falsified atoms stay excluded). The termination certificate's own invariant
// is the first atom of each cut: the new atoms are proved together with it, and the
// search never drops it.
//
// Relation candidates, per subscript of a site and per coordinate family: the
// subscript rewritten by the proved equalities of the cut its site is reached from,
// until it reads only the state the coordinates of the operand's axes read. A proposal
// like any other: IDX-use decides.
//
// How a result reaches the GEMM index checker: the proved atoms are conjoined into a
// copy of the termination certificate, checkCertificate is asked about the copy, and
// the copy is what synthesizeProgram, checkFamilySymbolic and the later checkers are
// handed, each of which asks checkCertificate again. A certificate the checker does
// not accept is not handed on.
//
//===----------------------------------------------------------------------===//
#ifndef GENERALIZEDGEMM_INDEXINVARIANTSYNTHESIS_H
#define GENERALIZEDGEMM_INDEXINVARIANTSYNTHESIS_H

#include "GeneralizedGemm/IndexInvariant.h"
#include "GeneralizedGemm/InvariantSynthesis.h"
#include "GeneralizedGemm/SynthesisAnalysis.h"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace mlir::generalized_gemm {

struct IndexInvariantSearchOptions {
  IndexProofOptions check;
  // For checkCertificate on the strengthened certificate.
  TerminationLimits termination;
  Authorizations authorized;
  // Runs for candidates only: no obligation reads a sample.
  uint64_t samplePoints = 24;
  size_t maxRowsPerCut = 256;
  // Monomial degrees of the null-space candidates, in order. After a level, a
  // variable that a kept equality determines leaves the next level's monomials.
  std::vector<unsigned> degrees = {1};
  size_t maxColumns = 128;
  unsigned maxRefinements = 8;   // IDX-init models turned into new sampled runs.
  unsigned maxHoudiniChecks = 96;
  bool poisonAtoms = true;
  // Leaves of a narrower width also read the wider state and parameters truncated to
  // it, so that a recurrence carried at another width than the loop counter has
  // candidates relating the two.
  bool truncations = true;
};

struct IndexInvariantLevel {
  unsigned degree = 0;
  size_t offered = 0, kept = 0;
  // Summed over the cuts: the sample matrices' shape, rank, basis vectors, basis
  // vectors whose coefficients do not fit the width, variables folded to constants.
  size_t columns = 0, rows = 0, rank = 0, basis = 0, skipped = 0, folded = 0;
  unsigned checks = 0, refinements = 0;
  std::string outcome;
};

struct IndexInvariantSearch {
  // INDUCTIVE      the certificate holds an atom beyond the termination certificate's
  //                own invariants, and IDX-init and IDX-step are unsat on every path.
  // NOTHING_FOUND  a search ended with no such atom.
  // NOT_INDUCTIVE  a given certificate has a model (a search drops what has one).
  // UNKNOWN        the solver did not decide, a limit, or no termination certificate.
  // UNSUPPORTED    the program is outside the model.
  std::string status = "UNKNOWN", reason;
  bool given = false;
  llvm::SmallVector<unsigned> cuts;
  // What `check` judged: per cut, the termination certificate's invariant first (when
  // it has one), then the atoms.
  IndexInvariantCertificate certificate;
  std::map<unsigned, unsigned> inherited; // Per cut: how many leading atoms are the certificate's.
  CheckedIndexProof check;
  // The search's sample, empty for a given certificate.
  CutSamples samples;
  uint64_t samplePoints = 0;
  std::vector<IndexInvariantLevel> levels;
  unsigned checks = 0, queries = 0, refinements = 0;
  // The termination certificate with the new atoms as invariants, when
  // checkCertificate accepts it; and what that checker said either way.
  std::optional<TerminationCertificate> strengthened;
  std::string certificateNote, certificateVerdict;
};

/// With `given`, it alone is checked: no search, and no sample is taken. `certificate`
/// may be null: then nothing is found, since no checker could be handed a result.
IndexInvariantSearch findIndexInvariants(const SemanticProgram &program,
                                         const TerminationCertificate *certificate,
                                         const IndexInvariantCertificate *given,
                                         const IndexInvariantSearchOptions &options,
                                         SmtBackend &backend);

/// Untrusted. One claim per subscript of each site of `group` for which the atoms of
/// `search` rewrite the subscript into a term that reads only the state the
/// coordinates of the operand's axes read (the subscript itself when it already does).
std::vector<IndexRelation> proposeIndexRelations(const SemanticProgram &program,
                                                 const IndexInvariantSearch &search,
                                                 const SiteGroup &group,
                                                 const CoordinateFamily &family);

/// Untrusted, for a proposer that wants "v = term" (the domain proposer's views): per
/// proved equality of the search beyond the termination certificate's own, the
/// variable with the largest id that the equality determines, and its solution.
struct IndexIdentity {
  unsigned state = 0, cut = 0;
  Term term = nullptr;
  std::string origin;
};
std::vector<IndexIdentity> indexIdentities(const SemanticProgram &program,
                                           const IndexInvariantSearch &search);

/// "cut:conjunction[;cut:conjunction...]" over s<id>, sp<id> and p<id>, as
/// parseLoopTerm reads them. Nullopt with `why`.
std::optional<IndexInvariantCertificate> parseIndexInvariants(const SemanticProgram &program,
                                                              const std::string &text,
                                                              std::string &why);

/// "site:A|B|C:dimension:term[;...]". Nullopt with `why`.
std::optional<std::vector<IndexRelation>> parseIndexRelations(const SemanticProgram &program,
                                                              const std::string &text,
                                                              std::string &why);

/// One candidate family of a group: its relations and their proof, and the
/// coordinate checker's answer for the same family with the termination certificate
/// as it was before strengthening, so the record shows what the invariants changed.
struct IndexCandidateCheck {
  unsigned group = 0, candidate = 0;
  CheckedIndexProof proof;
  CheckedFamily without = CheckedFamily::unknown("not checked");
  bool comparison = false; // `without` was computed.
};

/// For every candidate of `result`: the proposed relations and `extra` (given claims,
/// which name sites of any group), checked under the search's certificate; and, with
/// `certificate` and a strengthened certificate, the comparison run.
std::vector<IndexCandidateCheck> checkIndexCandidates(const SemanticProgram &program,
                                                      const TerminationCertificate *certificate,
                                                      const IndexInvariantSearch &search,
                                                      const SynthesisResult &result,
                                                      llvm::ArrayRef<IndexRelation> extra,
                                                      const IndexInvariantSearchOptions &options,
                                                      const SymbolicCheckOptions &symbolic,
                                                      SmtBackend &backend);

/// Deterministic JSON for the report. `result` may be null when there are no
/// candidates to show; `identities` are printed as what a proposer was handed.
std::string indexInvariantsJSON(const SemanticProgram &program, const IndexInvariantSearch &search,
                                llvm::ArrayRef<IndexCandidateCheck> candidates,
                                const SynthesisResult *result,
                                const IndexInvariantSearchOptions &options,
                                llvm::ArrayRef<IndexIdentity> identities = {});

} // namespace mlir::generalized_gemm
#endif
