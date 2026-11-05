//===- DomainModel.h - G5a domains, orders and their facts ------*- C++ -*-===//
//
// G5a data. See docs/GENERALIZED_GEMM_G5A.md.
//
// A guarded order describes a set of fixed-width points and an order on it:
//
//   levels  l1 .. lm, outermost first. Each has a value term over state, a direction
//           and a signedness, and bounds lo <= value < hi, which read parameters and
//           the values of outer levels only.
//   guard   a BV1 term over parameters and every level.
//
//   S(v) = { y : lo_l(v, y_<l) <= y_l < hi_l(v, y_<l) for every l, and g(v, y) }
//
// ordered lexicographically, each level in its own direction and signedness. Nothing
// here is a set of mathematical integers: comparisons are of bit vectors at their
// width. Levels hold the coordinates of a family and any further terms that tell
// repeated events apart. A proposal is data; only DomainChecker says anything of it.
//
//===----------------------------------------------------------------------===//
#ifndef GENERALIZEDGEMM_DOMAINMODEL_H
#define GENERALIZEDGEMM_DOMAINMODEL_H

#include "GeneralizedGemm/ConcreteEvaluator.h"
#include "GeneralizedGemm/SemanticTerm.h"

#include "llvm/ADT/APInt.h"

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace mlir::generalized_gemm {

/// Level variables are state leaves with ids from here on, far above every state id
/// and its second copy. They appear only in bounds, guards and proposed closed forms.
constexpr unsigned LevelVariableBase = 1u << 24;
Term levelVariable(TermContext &terms, unsigned level, Sort sort);
/// The level a leaf names, or nullopt for any other leaf.
std::optional<unsigned> levelOf(Term leaf);

struct GuardedLevel {
  Term value = nullptr;               // Over state, at the event.
  Term lower = nullptr, upper = nullptr; // Over parameters and outer level variables.
  bool descending = false, isSigned = false;
};

struct GuardedOrder {
  std::vector<GuardedLevel> levels;
  Term guard = nullptr; // Over parameters and level variables. Null is true.
  std::string origin;   // Who proposed it. Not evidence.
};

/// What a checker is asked about one group. All of it is a proposal.
struct DomainRequest {
  GuardedOrder order;
  // The per-cell order key: level indices, outermost first.
  std::vector<unsigned> cellKey;
  // Closed forms of h, f, g over level variables. An empty list proposes nothing.
  std::array<std::vector<Term>, 3> access;
};

enum class FactStatus { Proven, Refuted, Unknown };
const char *stringifyFactStatus(FactStatus status);

enum class DomainMultiplicity { Unit, ConstantBox, Counted, Unknown };
const char *stringifyDomainMultiplicity(DomainMultiplicity form);

/// One fact with what supports it. Refuted carries a concrete witness: a parameter
/// point at which a run, any run of this oblivious program, shows the fact false.
struct Fact {
  FactStatus status = FactStatus::Unknown;
  // The tier a Proven fact holds at: Symbolic over the scope, or Point and
  // ScopeEnumerated at the listed points only. A refutation's tier is Proposed.
  int tier = 0; // A ComputationTier, kept as an int to keep this header solver-free.
  std::string detail;
  std::optional<std::vector<llvm::APInt>> point;
  std::optional<EntryState> memory; // For facts about values.
};

/// One exactly enumerated point.
struct DomainPointFacts {
  std::vector<llvm::APInt> parameters;
  uint64_t events = 0, points = 0, logicalPoints = 0, cells = 0;
  uint64_t maxMultiplicity = 0, maxPerCell = 0, collisions = 0;
};

} // namespace mlir::generalized_gemm
#endif
