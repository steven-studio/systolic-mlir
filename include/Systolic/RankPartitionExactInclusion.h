#pragma once

#include "mlir/Analysis/Presburger/IntegerRelation.h"

#include <optional>

namespace systolic::logical {

// Phase 5.4.3d-3b-2.
//
// Integer-exact a is-a-subset-of b for two conjunctive
// integer sets of one space (no symbols), decided with
// conjunctive integer emptiness only.
//
// The Presburger library's set difference (and with it
// isSubsetOf / isEqual) is unsound in this LLVM build
// when the subtrahend carries locals; see
// docs/AFFINE_GEMM_MAPPING.md section 19.14.
//
// Forwards to mlir::systolic::exactIsSubset. The
// forwarding keeps the mlir::systolic namespace out of
// the rank-partition headers, which live in
// systolic::logical and are used next to
// `using namespace mlir`.
//
// nullopt: undecided (b has a local that is neither
// pinned by an equality nor a floor division of the
// variables). Callers must refuse.
std::optional<bool> exactIntegerSetInclusion(
    const mlir::presburger::IntegerRelation &a,
    const mlir::presburger::IntegerRelation &b);

} // namespace systolic::logical
