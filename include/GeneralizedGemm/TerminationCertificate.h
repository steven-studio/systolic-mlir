//===- TerminationCertificate.h - The G2a proof checker ---------*- C++ -*-===//
//
// checkCertificate is the only authority for a proven verdict, and
// verifyNonterminationWitness the only one for a refuted verdict. CheckedVerdict
// has no public constructor for any of them, so a proposer cannot issue one:
// proposers do not even link against the code that can. See
// docs/GENERALIZED_GEMM_G2A.md for the proof rule.
//
//   PROVEN_TERMINATING: every run from every entry in the domain returns.
//   PROVEN_TERMINATING_IF_DEFINED: no run from such an entry is infinite, but a
//     run may stop on a side condition. Undefined behaviour permits anything,
//     so this says nothing about the compiled program on such a run.
//   REFUTED_NONTERMINATING: one entry in the domain, replayed exactly, repeats
//     a complete configuration.
//   UNKNOWN: anything else. It never implies nontermination.
//
// The domain is the given parameter scope, defined parameters and fully
// initialized argument memory. It is an input, and a certificate cannot change
// it.
//
//===----------------------------------------------------------------------===//
#ifndef GENERALIZEDGEMM_TERMINATIONCERTIFICATE_H
#define GENERALIZEDGEMM_TERMINATIONCERTIFICATE_H

#include "GeneralizedGemm/ConcreteEvaluator.h"
#include "GeneralizedGemm/CutPaths.h"
#include "GeneralizedGemm/ProofStatus.h"
#include "GeneralizedGemm/SmtBackend.h"

#include <array>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace mlir::generalized_gemm {

enum class TerminationVerdict {
  ProvenTerminating,
  ProvenTerminatingIfDefined,
  RefutedNonterminating,
  Unknown,
};
const char *stringifyTerminationVerdict(TerminationVerdict verdict);

struct CheckerAuthority; // Defined only where the checker is implemented.

class CheckedVerdict {
public:
  /// Anyone may say they do not know.
  static CheckedVerdict unknown(std::string reason);
  TerminationVerdict kind() const { return verdict; }
  const std::string &reason() const { return text; }

private:
  friend struct CheckerAuthority;
  CheckedVerdict(TerminationVerdict verdict, std::string reason)
      : verdict(verdict), text(std::move(reason)) {}
  TerminationVerdict verdict;
  std::string text;
};

/// One node of the proof that no run stays forever in a part of the path graph.
/// `paths` are all paths of one SCC of the current graph, each named by its
/// transitions. Every path weakly decreases the rank and `strict` ones strictly
/// decrease it. Those occur finitely often, so a run that stays must end up in
/// an SCC of the remaining paths: the children, one for each such SCC.
struct RankLevel {
  llvm::SmallVector<unsigned> cuts; // Sorted.
  std::vector<std::pair<unsigned, Term>> rank; // Per cut. BV of one width.
  std::vector<llvm::SmallVector<unsigned>> paths, strict;
  std::vector<RankLevel> children;
  std::string origin; // Which template proposed it. Not evidence.
};

/// Untrusted data: everything here is checked. A rank is a BV term ordered by
/// unsigned <, so it lies in a well-founded domain by its sort.
struct TerminationCertificate {
  std::string fingerprint;
  llvm::SmallVector<unsigned> cuts;
  std::vector<std::pair<unsigned, Term>> invariants; // One per cut.
  std::vector<RankLevel> levels;
};

struct ObligationRecord {
  std::string id;       // O0 to O7.
  std::string location; // The CFG path, SCC or block.
  std::string script;   // The complete formula. Empty for O0.
  std::string expected; // "unsat", "sat" or "structural".
  std::string result;   // Solver answer, "pass", "fail" or "not-run".
  std::string evidence; // The certificate part used, or a counterexample.
  std::vector<std::string> assumptions;
  bool discharged = false;
};

enum class StopStatus { Unreachable, Unknown, Reachable };
const char *stringifyStopStatus(StopStatus status);

/// What is known about the stops of one kind in the domain.
struct StopReport {
  SideKind kind = SideKind::UndefinedBehavior;
  StopStatus status = StopStatus::Unreachable;
  unsigned sites = 0, discharged = 0;
  std::string detail;
  std::optional<EntryState> witness; // Replayed, for Reachable.
};

struct TerminationLimits {
  PathLimits paths;
  SmtLimits solver;
  unsigned maxCheckerQueries = 4096, maxProposerQueries = 2500;
  unsigned maxInvariantCandidates = 600, maxRankCandidates = 96;
  unsigned maxCegisRounds = 24;
  // The search of an affine rank by least coefficients (TerminationProposers.cpp,
  // leastAffine): its rounds, one solver query each at least, and the candidates
  // it lists.
  unsigned maxLeastRounds = 24, maxLeastCandidates = 4096;
  uint64_t replayFuel = 1u << 18, replayCells = 1u << 16;
  unsigned maxReplayEntries = 96;
};

struct CheckOptions {
  ParameterScope scope;
  TerminationLimits limits;
  Authorizations authorized;
};

struct CheckOutcome {
  CheckedVerdict verdict = CheckedVerdict::unknown("not checked");
  // Proven only for a verdict that needs no assumption.
  ProofStatus status = ProofStatus::unknown("not checked");
  std::vector<ObligationRecord> records;
  std::array<StopReport, 3> stops; // Indexed by SideKind.
  std::vector<std::string> assumptionsUsed;
  // Whether an obligation of the termination theorem (O1 to O6) reads memory.
  // Only then does that theorem rest on the separation of roots.
  bool dependsOnMemory = false;
};

/// O0 to O7. A proven verdict comes from nowhere else.
CheckOutcome checkCertificate(const SemanticProgram &program,
                              const TerminationCertificate &certificate,
                              const CheckOptions &options, SmtBackend &backend);

/// A run from `entry` that reaches, after `prefix` steps, a configuration it
/// reaches again `period` steps later.
struct NonterminationWitness {
  EntryState entry;
  uint64_t prefix = 0, period = 0;
};

enum class ReplayEnd { Returned, Stopped, Cycle, OutOfFuel, Invalid };

struct ReplayReport {
  ReplayEnd end = ReplayEnd::Invalid;
  RunStatus stop = RunStatus::InvalidEntry; // For Stopped.
  std::string reason;
  uint64_t steps = 0; // Executed, so a caller can share a budget fairly.
  std::optional<NonterminationWitness> witness; // For Cycle.
};

/// Exact execution with cycle detection in constant space. Running out of fuel
/// is OutOfFuel and proves nothing.
ReplayReport replayEntry(const SemanticProgram &program, const EntryState &entry,
                         uint64_t fuel, uint64_t maxCells);

/// Replays the witness from its entry with no help from the search: the
/// configuration after `prefix` steps must equal, in location, every state
/// value, every shadow and every memory cell, the one `period` steps later,
/// with no stop between them. The only source of a refuted verdict.
CheckedVerdict verifyNonterminationWitness(const SemanticProgram &program,
                                           const ParameterScope &scope,
                                           const NonterminationWitness &witness,
                                           uint64_t maxCells);

bool sameConfiguration(const Configuration &a, const Configuration &b);

} // namespace mlir::generalized_gemm
#endif
