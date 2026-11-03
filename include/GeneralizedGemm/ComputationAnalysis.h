//===- ComputationAnalysis.h - G4a entry point ------------------*- C++ -*-===//
//
// Runs the G3b search for candidates, then hands each group and each candidate to
// the checker. The search is only a source of proposals. The claim about a group
// does not depend on coordinates, so a group with no candidate still has one.
//
// A failed symbolic query gives a parameter point on an over-approximation. That
// point is run exactly, which may give a concrete counterexample. It is never one by
// itself. Nothing here authorizes a transformation.
//
//===----------------------------------------------------------------------===//
#ifndef GENERALIZEDGEMM_COMPUTATIONANALYSIS_H
#define GENERALIZEDGEMM_COMPUTATIONANALYSIS_H

#include "GeneralizedGemm/ComputationChecker.h"
#include "GeneralizedGemm/SynthesisAnalysis.h"

#include <string>
#include <vector>

namespace mlir::generalized_gemm {

struct ComputationRequest {
  SynthesisOptions synthesis;
  ComputationOptions computation;
  bool search = true; // False: the group claims alone, with no candidates.
};

struct GroupComputation {
  unsigned group = 0;
  CheckedComputation finite = CheckedComputation::unknown("not checked");
  CheckedComputation symbolic = CheckedComputation::unknown("not checked");
  RecoveryStatus search = RecoveryStatus::Unknown;
  std::string searchReason;
  std::vector<CandidateComputation> candidates;
};

struct ComputationResult {
  DomainEnumeration enumeration;
  std::vector<GroupComputation> groups;
};

ComputationResult analyzeComputation(const SemanticProgram &program,
                                     const TerminationCertificate *certificate,
                                     const ComputationRequest &request, SmtBackend &backend);

/// Deterministic JSON, with `authorizes_lowering` false at every level that has it.
std::string toJSON(const SemanticProgram &program, const ComputationResult &result);

} // namespace mlir::generalized_gemm
#endif
