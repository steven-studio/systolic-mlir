//===- ComputationModel.h - G4a claims and their parts ----------*- C++ -*-===//
//
// G4a data, with no reference to a solver. See docs/GENERALIZED_GEMM_G4A.md.
//
// A computation claim is about the run's own ordered events. For a group (C, A, B)
// and an initial memory M0 whose run completes,
//
//   Replay:  for e in E, in order:
//              C[h(e)] := C[h(e)] (+)e ( M0[A][f(e)] (*)e M0[B][g(e)] )
//
// with the exact operation, flags and operand order of e's site. Nothing here
// reorders, merges or simplifies: duplicates stay, and each intermediate value is
// compared. Coverage of a domain, the order a lowering would use, and whether any
// transformation preserves semantics are not claimed.
//
//===----------------------------------------------------------------------===//
#ifndef GENERALIZEDGEMM_COMPUTATIONMODEL_H
#define GENERALIZEDGEMM_COMPUTATIONMODEL_H

#include "GeneralizedGemm/SemanticProgram.h"

#include "llvm/ADT/APInt.h"

#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace mlir::generalized_gemm {

/// A separate scale from the coordinate tiers: neither says anything of the other.
enum class ComputationTier { Proposed, Point, ScopeEnumerated, Symbolic };
const char *stringifyComputationTier(ComputationTier tier);

enum class ComputationStatus { Verified, Refuted, Unknown };
const char *stringifyComputationStatus(ComputationStatus status);

/// What the fold of a cell allows. The replay is ordered in both cases. Only the
/// modular case has Theorem 10, and that is about values in completing runs.
enum class FoldForm { ModularInteger, OrderedFloat };
const char *stringifyFoldForm(FoldForm form);

/// The exact operation of one site, read back from its terms.
struct SiteContract {
  unsigned site = 0, location = 0, effect = 0;
  ReductionOp reduction = ReductionOp::FAdd;
  unsigned reductionFlags = 0, multiplyFlags = 0;
  bool accumulatorFirst = true; // z + m, not m + z.
};

/// One use of separation between two roots. `assumption` stays true for every use
/// that rests on a name someone authorized, and only fresh allocation is a proof.
struct SeparationUse {
  unsigned first = 0, second = 0;
  bool assumption = true;
  std::string basis, name;
};

/// A memory effect outside the group. It stays in the certificate.
struct ResidualEffect {
  unsigned location = 0, opIndex = 0, root = 0;
  MemoryEffect::Kind kind = MemoryEffect::Read;
  bool valueReadsMemory = false; // For a write: its value comes from memory.
  bool observesOutput = false;   // A read of the group's output root.
};

/// A condition under which a run stops that depends on memory. Runs that stop are
/// outside the claim, and a transformation may not preserve the set: that is G7's.
struct MemoryDependentStop {
  SideKind kind = SideKind::UndefinedBehavior;
  std::string reason;
  bool operator<(const MemoryDependentStop &o) const {
    return std::make_pair((int)kind, reason) < std::make_pair((int)o.kind, o.reason);
  }
};

/// One exactly replayed parameter point.
struct ReplayFacts {
  std::vector<llvm::APInt> parameters;
  uint64_t steps = 0, events = 0;
  uint64_t outputCells = 0;  // Cells the group writes.
  uint64_t prefixCells = 0;  // Of those, set to a constant before their first event.
  uint64_t residualCells = 0; // Other cells written.
  uint64_t maxContributions = 0; // The longest fold of a cell.
};

/// G5b-M3. A fact about a claim over several groups, and no condition of it: the claim
/// holds or fails whatever its facts say.
enum class SetFactStatus { Proven, Refuted, Unknown };
const char *stringifySetFactStatus(SetFactStatus status);

struct SetFact {
  SetFactStatus status = SetFactStatus::Unknown;
  ComputationTier tier = ComputationTier::Proposed; // Of a Proven fact.
  std::string detail;
  // Of a Refuted fact: a point whose reference run completed and shows it. The trace
  // is one at a point (Theorem 9a), so every completing run there shows it too.
  std::optional<std::vector<llvm::APInt>> point;
};

/// G5b-M3. One replayed point of a claim over several groups, beside its ReplayFacts.
struct GroupSetPoint {
  std::vector<llvm::APInt> parameters;
  std::vector<uint64_t> events;  // Per listed group, in the listed order.
  uint64_t sharedCells = 0;      // Cells written by events of two or more listed groups.
  bool listedOrder = true;       // No event of a group follows one of a later-listed group.
};

} // namespace mlir::generalized_gemm
#endif
