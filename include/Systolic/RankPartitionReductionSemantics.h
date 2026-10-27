#pragma once

#include "Systolic/RankPartitionDependenceLegality.h"

namespace systolic::logical {

// Phase 5.3.5: classify write/write and reduction-order obligations.
// This is a *candidate classifier*, NOT a lowering legality proof.
// In particular, it does not prove alias freedom, operand purity,
// read/write independence, or correctness of a parallel implementation.
enum class ReductionWriteUpdateKind {
  Assign,          // C[...] = rhs (last-writer semantics)
  IntegerAdd,      // C[...] += rhs, integer
  FloatingAdd,     // C[...] += rhs, floating point
  Unknown
};

enum class IntegerAddSemantics {
  Unknown,
  MathematicalExact, // exact/unbounded integer model (only if established)
  ModularWrap,       // defined wrapping modulo 2^w
  MayOverflowOrUB    // e.g. signed overflow may be undefined
};

enum class ReductionOrderDecision {
  NoOutputWriteCollision,  // Only this output-write hazard was ruled out.
  PreserveSourceOrder,     // Do not reorder these writes.
  ReassociationCandidate   // Candidate ONLY; further legality proof needed.
};

struct ReductionOperatorEvidence {
  ReductionWriteUpdateKind update = ReductionWriteUpdateKind::Unknown;
  IntegerAddSemantics integerSemantics = IntegerAddSemantics::Unknown;

  // True only after proving the corresponding semantic properties.
  // A syntactic '+=' match alone is not proof.
  bool updateIsRecognizedReduction = false;
  bool operandsIndependentOfDestination = false;
  bool noOtherObservableEffects = false;

  // For floating add, this represents a separate, explicit permission
  // to change reduction order under the applicable FP environment.
  // Merely detecting float addition must not set it.
  bool floatingReassociationPermitted = false;
};

inline ReductionOrderDecision classifyReductionOrder(
    bool hasFeasibleOrderedWriteConflict,
    const ReductionOperatorEvidence &evidence) {
  if (!hasFeasibleOrderedWriteConflict)
    return ReductionOrderDecision::NoOutputWriteCollision;

  // Required for any reduction reordering candidate.
  if (!evidence.updateIsRecognizedReduction ||
      !evidence.operandsIndependentOfDestination ||
      !evidence.noOtherObservableEffects)
    return ReductionOrderDecision::PreserveSourceOrder;

  switch (evidence.update) {
  case ReductionWriteUpdateKind::Assign:
  case ReductionWriteUpdateKind::Unknown:
    return ReductionOrderDecision::PreserveSourceOrder;
  case ReductionWriteUpdateKind::IntegerAdd:
    switch (evidence.integerSemantics) {
    case IntegerAddSemantics::MathematicalExact:
    case IntegerAddSemantics::ModularWrap:
      return ReductionOrderDecision::ReassociationCandidate;
    case IntegerAddSemantics::Unknown:
    case IntegerAddSemantics::MayOverflowOrUB:
      return ReductionOrderDecision::PreserveSourceOrder;
    }
    return ReductionOrderDecision::PreserveSourceOrder;
  case ReductionWriteUpdateKind::FloatingAdd:
    return evidence.floatingReassociationPermitted
               ? ReductionOrderDecision::ReassociationCandidate
               : ReductionOrderDecision::PreserveSourceOrder;
  }
  return ReductionOrderDecision::PreserveSourceOrder;
}

// Keep Phase 5.3.4's integer feasibility analysis authoritative.
// No source-point enumeration and no assumption that Batch is output-disjoint.
inline ReductionOrderDecision classifyReductionOrder(
    const OrderedWriteConflictAnalysis &conflicts,
    const ReductionOperatorEvidence &evidence) {
  return classifyReductionOrder(!conflicts.feasibleBranches.empty(), evidence);
}

} // namespace systolic::logical
