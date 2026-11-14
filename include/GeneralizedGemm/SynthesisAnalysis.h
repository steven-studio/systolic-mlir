//===- SynthesisAnalysis.h - G3b entry point --------------------*- C++ -*-===//
//
// Enumerates sampled domains with live state, expands the grammar level by level,
// searches for coordinates, and hands every proposal to the checkers. A candidate
// that fails at a point adds that point to the samples, and the search runs again.
// All candidates that pass are kept, up to the limit: the best one is first, which
// is a preference and no judgement of the others.
//
// Failure to find coordinates gives Unknown: a grammar not containing a function
// says nothing about the function. Nothing here authorizes a transformation.
//
//===----------------------------------------------------------------------===//
#ifndef GENERALIZEDGEMM_SYNTHESISANALYSIS_H
#define GENERALIZEDGEMM_SYNTHESISANALYSIS_H

#include "GeneralizedGemm/CoordinateSynthesis.h"
#include "GeneralizedGemm/SymbolicCandidateChecker.h"

#include "llvm/ADT/StringRef.h"

#include <string>
#include <vector>

namespace mlir::generalized_gemm {

struct SynthesisOptions {
  ParameterScope scope;
  TerminationLimits termination;
  Authorizations authorized;
  EnumerationLimits limits;
  SymbolicLimits2 symbolic;
  SynthesisLimits synthesis;
  std::optional<GrammarSpec> grammar; // A fixed grammar, instead of the levels.
  // The statement productions of IndexProgram.h at every level tried: the
  // bit-vector adaptation of the thesis's Appendix G grammar, loop-free in this
  // phase. Off by default, and then nothing in the output changes.
  bool statementGrammar = false;
  unsigned maxLevel = 3, maxSamples = 6, maxRefinements = 3;
  uint64_t referenceFill = 0;
};

struct VerifiedCandidate {
  CoordinateFamily family;
  std::optional<EnumerationOrder> order;
  CheckedFamily finite = CheckedFamily::unknown("not checked");
  CheckedFamily symbolic = CheckedFamily::unknown("not checked");
};

struct GroupSynthesis {
  unsigned group = 0;
  RecoveryStatus status = RecoveryStatus::Unknown;
  std::string reason;
  std::vector<VerifiedCandidate> candidates;
  unsigned level = 0, atoms = 0, rounds = 0, clauses = 0, proposals = 0, refuted = 0;
  bool exhausted = false;
};

struct SynthesisResult {
  DomainEnumeration enumeration;
  std::vector<GroupSynthesis> groups;
};

SynthesisResult synthesizeProgram(const SemanticProgram &program,
                                  const TerminationCertificate *certificate,
                                  const SynthesisOptions &options, SmtBackend &backend);

/// Deterministic JSON. With `events`, the exact tables behind each point. Each
/// extra adds a pre-rendered JSON value under its key.
std::string toJSON(const SemanticProgram &program, const SynthesisResult &result, bool events,
                   llvm::ArrayRef<std::pair<std::string, std::string>> extras = {});
std::string toJSON(const SemanticProgram &program, const SynthesisResult &result, bool events,
                   llvm::StringRef extraKey, llvm::StringRef extraJSON);

/// Phase 4 (I1). JSON for the iteration counters of `instrumented` (LoopCounters.h):
/// their state ids and headers, and the side-by-side replay of `original` and
/// `instrumented` from every point (compareRuns), which is evidence for those
/// points that the instrumentation changed no transition, event or return.
std::string iterationCountersJSON(const SemanticProgram &original, const SemanticProgram &instrumented,
                                  llvm::ArrayRef<std::vector<llvm::APInt>> points, uint64_t referenceFill,
                                  const EnumerationLimits &limits);

} // namespace mlir::generalized_gemm
#endif
