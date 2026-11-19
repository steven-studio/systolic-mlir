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

/// G5b-M3: one claim over several groups, by their indices, in the order the claim's
/// groupOrder() judges. `all`: every supported group, in index order, or reversed.
struct GroupSetRequest {
  std::vector<unsigned> groups;
  bool all = false, reversed = false;
};

struct ComputationRequest {
  SynthesisOptions synthesis;
  ComputationOptions computation;
  bool search = true; // False: the group claims alone, with no candidates.
  std::vector<GroupSetRequest> groupSets; // G5b-M3. Empty: no claim over a set.
};

struct GroupComputation {
  unsigned group = 0;
  CheckedComputation finite = CheckedComputation::unknown("not checked");
  CheckedComputation symbolic = CheckedComputation::unknown("not checked");
  RecoveryStatus search = RecoveryStatus::Unknown;
  std::string searchReason;
  std::vector<CandidateComputation> candidates;
};

/// G5b-M3: the claims over one set of groups.
struct GroupSetComputation {
  std::vector<unsigned> groups;
  CheckedComputation finite = CheckedComputation::unknown("not checked");
  CheckedComputation symbolic = CheckedComputation::unknown("not checked");
};

struct ComputationResult {
  DomainEnumeration enumeration;
  std::vector<GroupComputation> groups;
  std::vector<GroupSetComputation> sets;
};

ComputationResult analyzeComputation(const SemanticProgram &program,
                                     const TerminationCertificate *certificate,
                                     const ComputationRequest &request, SmtBackend &backend);

/// Deterministic JSON, with `authorizes_lowering` false at every level that has it.
std::string toJSON(const SemanticProgram &program, const ComputationResult &result);

} // namespace mlir::generalized_gemm
#endif
