//===- FiniteDomain.h - Gated exact enumeration -----------------*- C++ -*-===//
//
// The only source of an AccessDomain. A domain is enumerated only when
//
//   1. the program is supported, control and address oblivious, and no integer
//      control or address depends on a float parameter;
//   2. the parameter scope is finite within the limits;
//   3. a G2a certificate passes checkCertificate here, for this program and
//      this scope. A verdict computed elsewhere is not taken on trust;
//   4. the point's run returned, and I8 admits a coverage claim for it.
//
// A failed gate gives no domain. A limit, or a run that does not return, gives
// nothing for that point: there is no partial table, and a sample of runs is
// never treated as the domain. Results hold for runs that complete, for each
// enumerated point, and for no other point.
//
// Sites are grouped by the exact (C, A, B) root triple. Two groups are never
// merged, and a site in a group keeps its identity.
//
//===----------------------------------------------------------------------===//
#ifndef GENERALIZEDGEMM_FINITEDOMAIN_H
#define GENERALIZEDGEMM_FINITEDOMAIN_H

#include "GeneralizedGemm/CoordinateModel.h"
#include "GeneralizedGemm/SemanticProgram.h"
#include "GeneralizedGemm/TerminationCertificate.h"

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace mlir::generalized_gemm {

struct EnumerationLimits {
  uint64_t maxPoints = 4096;
  uint64_t maxEvents = 1u << 22;
  uint64_t fuel = 1u << 24;
  uint64_t maxCells = 1u << 20;
};

struct EnumerationOptions {
  ParameterScope scope;
  TerminationLimits termination;
  Authorizations authorized;
  EnumerationLimits limits;
  // The value of every argument cell. Obliviousness says it cannot matter; the
  // tests compare two fills to hold that to account.
  uint64_t referenceFill = 0;
  // G3b. Off by default, and the default output does not change. With this, x
  // is every integer state variable that is independent of memory and defined at
  // all events of the group, whether or not an address reads it: a coordinate
  // need not appear in any access.
  bool liveStatePoints = false;
  // G3b. When nonempty, only these points run, each of which must be in the
  // scope. The certificate is still checked for the whole scope, and the result
  // is marked as a sample: it is evidence about these points alone.
  std::vector<std::vector<llvm::APInt>> onlyPoints;
};

/// Sites sharing one (C, A, B) root triple. A scalar accumulator has no output
/// root, so its site is alone in an unsupported group.
struct SiteGroup {
  std::optional<unsigned> output;
  unsigned first = 0, second = 0;
  std::vector<unsigned> sites;
  std::string unsupported; // Nonempty: no table is built for this group.
};

enum class GateStatus { Open, Unsupported, Unknown };
const char *stringifyGateStatus(GateStatus status);

struct PointEvidence {
  std::vector<llvm::APInt> parameters;
  RunStatus run = RunStatus::InvalidEntry;
  std::string reason; // Nonempty when this point supports no claim.
  // Parallel to DomainEnumeration::groups. Empty slot: unsupported, or no event.
  std::vector<std::optional<AccessDomain>> domains;
};

struct DomainEnumeration {
  GateStatus gate = GateStatus::Unknown;
  std::string reason;
  CheckedVerdict termination = CheckedVerdict::unknown("not checked");
  ProofStatus terminationStatus = ProofStatus::unknown("not checked");
  std::array<StopReport, 3> stops;
  std::vector<std::string> assumptionsUsed;
  std::vector<SiteGroup> groups;
  std::vector<PointEvidence> points;
  uint64_t outsideDomain = 0; // Scope points with a negative extent.
  bool sampled = false;       // Not every point of the scope ran.
  // G5b-M4-0: the scope the enumerator was given, and, when it ran every point of it, how
  // many it counted, with those outside the domain. A list with a point removed no longer
  // matches its count, and reads as the sample it is.
  ParameterScope scope;
  uint64_t scopePoints = 0;
};

/// Whether every point of the scope the enumeration was made for ran: not a sample, an
/// open gate, every point inside that scope, and as many distinct points, with those
/// outside the domain, as the enumerator counted (G5b-M4-0). It says nothing of another
/// scope: a claim about a given scope compares with that scope (enumerationCoversScope).
bool enumerationRanItsScope(const DomainEnumeration &enumeration);

/// State variables that no value read from memory can reach.
std::vector<bool> memoryIndependentState(const SemanticProgram &program);

/// One point run with the reference memory, keeping the whole state at the entry of
/// each event's block. For G5b, whose segment terms read state the live table omits: a
/// loop variable that is undefined during another loop's events. The trace is the same
/// for every memory, by obliviousness, which the caller must have checked.
struct EventStates {
  EntryState entry;
  RunResult run;
  std::vector<std::vector<std::optional<llvm::APInt>>> states; // Parallel to run.events.
};
std::optional<EventStates> eventStates(const SemanticProgram &program,
                                       const std::vector<llvm::APInt> &point, uint64_t fill,
                                       const EnumerationLimits &limits);

/// `certificate` may be null: then there is no enumeration.
DomainEnumeration enumerateDomain(const SemanticProgram &program,
                                  const TerminationCertificate *certificate,
                                  const EnumerationOptions &options,
                                  SmtBackend &backend);

} // namespace mlir::generalized_gemm
#endif
