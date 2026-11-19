//===- CandidateChecker.h - The G3a coordinate checker ----------*- C++ -*-===//
//
// checkCandidate is the only authority for CandidateFound and CandidateRefuted.
// CheckedCandidate has no public constructor for either, and this code links
// neither the proposers nor union-find: every fact comes from maps over events.
//
// CandidateFound means the accesses of an exactly enumerated domain decompose
// through the candidate. It is a statement about finite data at one parameter
// point. It is no proof of GEMM equivalence and authorizes no lowering: the
// equivalence status stays Unknown, and G4 to G7 own it.
//
// CandidateRefuted says that candidate fails, with exact events. It says nothing
// about any other candidate, except for C0, which no += decomposition can pass.
//
//===----------------------------------------------------------------------===//
#ifndef GENERALIZEDGEMM_CANDIDATECHECKER_H
#define GENERALIZEDGEMM_CANDIDATECHECKER_H

#include "GeneralizedGemm/CoordinateModel.h"

#include <string>
#include <utility>

namespace mlir::generalized_gemm {

struct CandidateAuthority; // Defined only where the checker is implemented.

class CheckedCandidate {
public:
  /// Anyone may say they do not know, or that a form is unsupported.
  static CheckedCandidate unknown(std::string reason);
  static CheckedCandidate unsupported(std::string reason);

  RecoveryStatus kind() const { return status; }
  const std::string &reason() const { return text; }
  const std::string &fingerprint() const { return programFingerprint; }
  const CandidateReport &report() const { return facts; }
  /// Always Unknown in G3a. A candidate is not a proof of equivalence.
  const ProofStatus &equivalence() const { return equivalenceStatus; }

private:
  friend struct CandidateAuthority;
  CheckedCandidate(RecoveryStatus status, std::string reason)
      : status(status), text(std::move(reason)) {}
  RecoveryStatus status;
  std::string text, programFingerprint;
  CandidateReport facts;
  ProofStatus equivalenceStatus =
      ProofStatus::unknown("no equivalence is claimed before G4 to G7");
};

/// The facts of one candidate over one table. Works on any table, and issues no
/// status: a table with no domain evidence supports no claim.
CandidateReport inspectCandidate(const AccessTable &table,
                                 const CoordinateCandidate &candidate);

CheckedCandidate checkCandidate(const AccessDomain &domain,
                                const CoordinateCandidate &candidate);

} // namespace mlir::generalized_gemm
#endif
