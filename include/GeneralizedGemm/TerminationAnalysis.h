//===- TerminationAnalysis.h - G2a entry point ------------------*- C++ -*-===//
//
// Runs the untrusted proposers, hands the result to the checker, and replays
// entries looking for an exact cycle or a stop. Limits, solver failures and
// unsupported programs end in UNKNOWN. A replay that contradicts a proof turns
// the proof into UNKNOWN and sets `conflict`: that would be a defect, so it is
// reported and not resolved in either direction.
//
//===----------------------------------------------------------------------===//
#ifndef GENERALIZEDGEMM_TERMINATIONANALYSIS_H
#define GENERALIZEDGEMM_TERMINATIONANALYSIS_H

#include "GeneralizedGemm/TerminationCertificate.h"
#include "GeneralizedGemm/TerminationProposers.h"

#include <optional>
#include <string>
#include <vector>

namespace mlir::generalized_gemm {

struct TerminationOptions {
  ParameterScope scope;
  TerminationLimits limits;
  Authorizations authorized;
};

struct TerminationResult {
  CheckedVerdict verdict = CheckedVerdict::unknown("not analyzed");
  ProofStatus status = ProofStatus::unknown("not analyzed");
  std::string solver;
  std::optional<TerminationCertificate> certificate; // Only when proven.
  std::optional<NonterminationWitness> witness;      // Only when refuted.
  std::vector<ObligationRecord> records;
  std::array<StopReport, 3> stops;
  std::vector<std::string> assumptionsUsed;
  ProposalStatistics statistics;
  std::string conflict; // Nonempty only for a defect. See the file comment.
};

TerminationResult analyzeTermination(const SemanticProgram &program,
                                     SmtBackend &backend,
                                     const TerminationOptions &options = {},
                                     llvm::ArrayRef<RankHint> hints = {});

/// Deterministic JSON. With `scripts`, every obligation carries its full
/// SMT-LIB, so replay needs nothing else from this library.
std::string toJSON(const SemanticProgram &program, const TerminationResult &result,
                   bool scripts);

} // namespace mlir::generalized_gemm
#endif
