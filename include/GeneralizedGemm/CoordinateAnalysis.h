//===- CoordinateAnalysis.h - G3a entry point -------------------*- C++ -*-===//
//
// Enumerates the gated domain, proposes the canonical candidate per point and
// group, and hands it to the checker. Canonical failure ends in Unknown with
// partition hints: it says the injective-view class does not apply, and neither
// that the program is no GEMM nor that some other coordinates fail. Nothing is
// aggregated into a claim over the scope: every point stands alone.
//
//===----------------------------------------------------------------------===//
#ifndef GENERALIZEDGEMM_COORDINATEANALYSIS_H
#define GENERALIZEDGEMM_COORDINATEANALYSIS_H

#include "GeneralizedGemm/CandidateChecker.h"
#include "GeneralizedGemm/CanonicalRecovery.h"
#include "GeneralizedGemm/FiniteDomain.h"

#include "llvm/Support/JSON.h"

#include <optional>
#include <string>
#include <vector>

namespace mlir::generalized_gemm {

struct GroupRecovery {
  unsigned group = 0;
  CheckedCandidate result = CheckedCandidate::unknown("not analyzed");
  std::optional<CoordinateCandidate> candidate;
  // For an Unknown from a failed canonical class.
  std::optional<CandidateReport> attempted;
  std::optional<CanonicalPartitions> hints;
};

struct PointRecovery {
  std::vector<GroupRecovery> groups; // Parallel to the enumeration's groups.
};

struct CoordinateResult {
  DomainEnumeration enumeration;
  std::vector<PointRecovery> points; // Parallel to the enumeration's points.
  uint64_t memoizedJoins = 0;
};

CoordinateResult recoverCoordinates(const SemanticProgram &program,
                                    const TerminationCertificate *certificate,
                                    const EnumerationOptions &options,
                                    SmtBackend &backend);

/// Deterministic JSON.
std::string toJSON(const SemanticProgram &program, const CoordinateResult &result,
                   bool events);

/// A bare table has no domain evidence, so its status stays Unknown. The facts
/// are the checker's. For oracles over finite mappings.
std::string tableToJSON(const AccessTable &table, const CoordinateCandidate *candidate);

/// One report, as G3a prints it. G3b adds what it needs beside this.
llvm::json::Value reportToJSON(const CandidateReport &report);

} // namespace mlir::generalized_gemm
#endif
