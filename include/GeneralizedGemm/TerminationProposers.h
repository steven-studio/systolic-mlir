//===- TerminationProposers.h - Untrusted candidates ------------*- C++ -*-===//
//
// Everything here only makes suggestions. A wrong invariant or rank costs an
// UNKNOWN: the output goes to checkCertificate, which re-derives the graph and
// discharges every obligation itself. This code can see CheckedVerdict::unknown
// and nothing else that issues a verdict.
//
//   Houdini: guess inductive facts, drop what a model falsifies.
//   T1 guard distance, T2 modular distance, T3 bounded affine by CEGIS.
//   T4 lexicographic: SCC slicing over those three.
//
//===----------------------------------------------------------------------===//
#ifndef GENERALIZEDGEMM_TERMINATIONPROPOSERS_H
#define GENERALIZEDGEMM_TERMINATIONPROPOSERS_H

#include "GeneralizedGemm/TerminationCertificate.h"

#include <map>
#include <optional>
#include <vector>

namespace mlir::generalized_gemm {

/// A rank to try before the templates, valid or not.
struct RankHint {
  Term rank = nullptr;
  std::string origin;
};

struct ProposalInput {
  const SemanticProgram &program;
  const PathSet &set;
  llvm::ArrayRef<unsigned> cuts;
  Term scope;
  const TerminationLimits &limits;
  SmtBackend &backend;
  llvm::ArrayRef<RankHint> hints;
};

struct ProposalStatistics {
  unsigned queries = 0, candidates = 0, invariantsKept = 0;
  std::string failure; // Why no rank was found.
  // Parameter values from failed descent, tried later as replay entries.
  std::vector<std::map<unsigned, std::string>> parameterModels;
};

/// A conjunction per cut. Exhausting the budget gives no invariants at all.
std::map<unsigned, Term> discoverInvariants(const ProposalInput &input,
                                            ProposalStatistics &statistics);

/// Levels for every cyclic SCC, or nullopt. An empty vector means none exist.
std::optional<std::vector<RankLevel>>
proposeRankLevels(const ProposalInput &input,
                  const std::map<unsigned, Term> &invariants,
                  ProposalStatistics &statistics);

} // namespace mlir::generalized_gemm
#endif
