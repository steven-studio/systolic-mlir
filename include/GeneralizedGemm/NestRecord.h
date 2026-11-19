//===- NestRecord.h - One record per loop nest ------------------*- C++ -*-===//
//
// Stage 1 of docs/GENERALIZED_GEMM_SCOPE.md. A record says, of one loop nest of a function,
// what the existing checkers proved about the nest's copy (NestOutline.h), in the terms a
// partitioner reads: which loops are I, J and K and how far each runs, which root and
// which view each operand has, what the update is, and under which assumptions.
//
// Nothing is judged here. The importer, the termination analysis and the domain analysis
// run on the copy as they run on any function, with the options their own driver uses,
// and the record reads their verdicts. Every field that states a fact names the claim it
// comes from, and a claim that is not Verified, or not at the symbolic tier, is a reason
// in `refusals` and never a fact.
//
// Two things are kept apart, because they are different statements:
//
//   ordered fold verified    For every memory at the nest's entry and every value in scope
//                            of the scalars it reads, a run of the nest that completes
//                            leaves each cell of C with the fold of its contributions in
//                            the nest's own order, and writes nothing else (Theorem 13 at
//                            the symbolic tier, with an empty frame of writes).
//   reassociation permitted  Whether the source lets that order change: by a proof (no
//                            cell has two contributions, or a wrapping integer addition
//                            with no flag), or by the `reassoc` flag of the additions,
//                            which is the programmer's permission and no proof.
//
// A record authorizes nothing: `authorizes_lowering` is false in every one of them.
//
//===----------------------------------------------------------------------===//
#ifndef GENERALIZEDGEMM_NESTRECORD_H
#define GENERALIZEDGEMM_NESTRECORD_H

#include "GeneralizedGemm/DomainAnalysis.h"
#include "GeneralizedGemm/NestOutline.h"

#include "llvm/Support/JSON.h"

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace mlir::generalized_gemm {

/// A range of one scalar argument of the function, as signed decimals at its width.
struct NestScopeEntry {
  unsigned argument = 0;
  int64_t lower = 0, upper = 0;
};

/// The options of the domain driver that a nest's analysis takes, with its defaults.
struct NestAnalysisOptions {
  unsigned indexBitWidth = 0;
  unsigned timeoutMs = 10000, resourceLimit = 20000000;
  uint64_t maxPoints = 256, maxBox = 1u << 20;
  uint64_t replaySteps = 1u << 22, replayNodes = 1u << 23, referenceFill = 0;
  unsigned maxLevel = 3, maxCandidates = 5, maxRounds = 300;
  unsigned candidatesPerGroup = 2, maxQueries = 4000;
  /// Over the function's arguments. Empty: no restriction, as with the domain driver.
  std::vector<NestScopeEntry> scope;
  /// Section 14 of docs/GENERALIZED_GEMM_STATEMENT_BV.md. Search for inductive invariants
  /// over the integer state at the loop headers of the copy (a subscript carried by
  /// iter_args is a function of the counters only by such an invariant), and hand what
  /// is proved to the domain analysis: the certificate with those invariants, when the
  /// certificate checker accepts it, and the identities for the views
  /// (handIndexInvariants). Off by default, and then a record is what it was.
  bool indexInvariants = false;
};

/// What the search for index invariants gave a nest's analysis. Not a verdict of the
/// record: the record still reads the checkers, which were handed the certificate.
struct NestIndexInvariants {
  bool asked = false;
  std::string status, reason;          // IndexInvariantSearch's.
  bool automatic = true;               // Found by the search; never a given one here.
  std::vector<std::string> invariants; // "L<cut>: atom", beyond the termination certificate's.
  std::vector<std::string> identities; // "s<v> = term at L<cut>", as handed to the proposer.
  bool certificateStrengthened = false; // The certificate checker accepted it, and it was handed on.
  std::string certificateNote;
  unsigned queries = 0, sampleRuns = 0;
};

/// One level of the nest's order, outermost first: lower <= value < upper.
struct NestLevel {
  char axis = '?'; // 'P', 'I', 'J', 'K', or 'R' for a level that is no coordinate.
  bool descending = false, isSigned = false;
  std::optional<int64_t> lower, upper; // Set when the bound is a constant.
  std::optional<uint64_t> extent;      // Set when both are.
  std::string lowerText, upperText, valueText;
};

/// One dimension of a view: constant + the sum of coefficient * level value.
struct NestViewDimension {
  bool affine = false;
  int64_t constant = 0;
  std::vector<std::pair<unsigned, int64_t>> levels;     // Level, coefficient.
  std::vector<std::pair<unsigned, int64_t>> parameters; // Parameter, coefficient.
  std::string text;
  /// Over the logical coordinates, which count from zero along each level: the scale of
  /// each role and the offset. Set when the dimension is affine, reads no parameter, and
  /// every level it reads ascends from a constant.
  std::optional<int64_t> offset;
  std::vector<std::pair<char, int64_t>> roles; // Axis, scale.
};

struct NestView {
  int root = -1; // The function's root id.
  bool proposed = false, affine = false;
  bool proved = false; // The closed forms are Proven at the symbolic tier.
  std::string fact;    // The fact's status and tier, as text.
  std::vector<NestViewDimension> dimensions;
};

struct NestRecord {
  enum Status {
    Unsupported,       // No copy of the nest, or the importer refuses the copy.
    NoGemm,            // The copy has no multiply-accumulate site.
    NotVerified,       // The ordered fold is not verified for the nest as a whole.
    VerifiedNotMapped, // The ordered fold is verified; the nest is no constant box with affine views.
    Verified,          // The ordered fold is verified and the record gives the box and the views.
  };
  Status status = Unsupported;

  // Where.
  std::string function, loop, location;
  unsigned nest = 0, statement = 0, conditions = 0, line = 0, column = 0;

  // The copy.
  std::string outlineRefusal, outlinedName;
  std::vector<NestRoot> roots;
  std::vector<NestScalar> scalars;
  unsigned constantsCopied = 0;
  std::vector<NestSeparation> separations;
  std::vector<bool> separationUsed; // Parallel: a claim of the record used the pair.

  // The model of the copy.
  std::string modelRefusal, gate, gateReason, termination;
  unsigned sites = 0, groups = 0, candidates = 0;
  std::optional<unsigned> group, candidate;

  // The chosen candidate.
  std::vector<NestLevel> levels;
  std::string guard; // "true", or the guard's text.
  bool box = false;  // One level each for I, J and K, no other level, no guard.
  std::optional<std::array<uint64_t, 3>> extents; // M, N, K: set when the box is constant.
  std::array<NestView, 3> views;                  // C, A, B.

  // The update.
  std::string element, reduction, multiply, fold;
  bool accumulatorFirst = true;
  unsigned reductionFlags = 0, multiplyFlags = 0;
  bool flagsAgree = true; // Every site of the group has these flags and this operation.
  unsigned sitesInGroup = 0;
  unsigned otherWrites = 0, otherReads = 0; // The frame of the computation claim.

  // The claims behind it, as the checkers state them.
  llvm::json::Object tiers, reorder;

  // The two summaries.
  bool orderedFoldVerified = false;
  std::string orderedFoldReason;
  bool reassociationPermitted = false;
  std::string reassociationBasis; // "proof", "source flag" or "none".
  std::string reassociationDetail;
  std::optional<bool> sourceFlagReassoc; // Float folds only.

  std::vector<std::string> assumptions, refusals;

  // Section 14: only with NestAnalysisOptions::indexInvariants.
  NestIndexInvariants indexInvariants;

  // Stage 2: what a reader needs to tell that the record is about the loop in front of it,
  // and under which premises its claims hold, as data rather than words.
  std::string bindingSha, bindingRefusal; // Systolic/NestBinding.h; see OutlinedNest.
  unsigned indexWidth = 0;                // The width every claim is made at.
  std::vector<NestScopeEntry> scope;      // Ranges of the function's arguments; empty: every value.
};

const char *stringifyNestStatus(NestRecord::Status status);

/// What the checkers said of the candidate that a mapping rests on, as plain data.
struct NestMappingFacts {
  bool singleOrder = false;      // One order over the candidate's family, and no segments.
  bool logicalSymbolic = false;  // Theorem 13's logical tier is the symbolic one.
  bool exactOrder = false;       // Theorem 12 over the scope.
  bool unitMultiplicity = false; // One event a point.
  bool outputInjective = false;  // No two logical outputs share a cell, over the scope.
  std::string logicalTier, multiplicity, outputInjectiveFact; // As text, for the refusals.
};

/// Every reason why `record` gives a partitioner no input: no constant box with one level
/// each for I, J and K, or no three proved affine views over their own roles. Empty when
/// it does. It reads the record's levels, guard, views and flags, and the facts, and
/// nothing else: each condition can be asked alone.
std::vector<std::string> mappingRefusals(const NestRecord &record, const NestMappingFacts &facts);

/// Names of the bits of a reduction's or a multiplication's flags.
std::vector<std::string> flagNames(unsigned flags, bool isFloat);

/// Runs the importer and the analyses on the nest's copy, and reads the record off
/// their results.
NestRecord analyzeNest(const OutlinedNest &nest, const NestAnalysisOptions &options, SmtBackend &backend);

/// The record of one function: its roots, the order of its statements, how its nests
/// meet on memory, and one record per nest, in program order.
llvm::json::Object toJSON(const FunctionOutline &outline, const std::vector<NestRecord> &records);
llvm::json::Object toJSON(const NestRecord &record, const std::vector<FunctionRoot> &roots);

} // namespace mlir::generalized_gemm
#endif
