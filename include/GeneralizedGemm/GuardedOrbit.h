//===- GuardedOrbit.h - Theorem 12 ------------------------------*- C++ -*-===//
//
// The formulas of exact, ordered coverage by a guarded order. With λ the levels' values
// at an event, and y free level variables:
//
//   D1  On every path from the entry to the first event, λ ∈ S and no y ∈ S lies
//       before it. A path to the exit with no event has no y ∈ S.
//   D2  On every path from an event b to the next event, λ ∈ S, b < λ, and no y ∈ S
//       lies strictly between them.
//   D3  On every path from an event b to the exit with no event between, no y ∈ S lies
//       after b.
//
// Theorem 12: then, for every run that completes, λ maps its events in order onto S(v)
// in order. Nothing is missing, nothing repeats, and nothing is made up. Each query is
// QF_BV, with the y only existential, so no quantifier alternates.
//
// The enumerator is the same set, computed at one point, for the finite route.
//
//===----------------------------------------------------------------------===//
#ifndef GENERALIZEDGEMM_GUARDEDORBIT_H
#define GENERALIZEDGEMM_GUARDEDORBIT_H

#include "GeneralizedGemm/CoordinateFamily.h"
#include "GeneralizedGemm/CoverageSimulation.h"
#include "GeneralizedGemm/DomainModel.h"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace mlir::generalized_gemm {

/// The order, rebuilt in the program's context, or nullopt with `why`. Values must be
/// integer terms of state independent of memory, with no poison shadow. Bounds read
/// parameters and outer levels, and the guard parameters and any level. `family` gives
/// the coordinate terms that must each be a level; `repeat` marks the other levels.
std::optional<GuardedOrder> adoptGuardedOrder(const SemanticProgram &program,
                                              const CoordinateFamily &family,
                                              const GuardedOrder &order,
                                              std::vector<bool> &repeat, std::string &why);

/// Whether `term` reads only parameters and level variables below `levels`, with integer
/// operations alone. For bounds, guards and proposed closed forms.
bool isLevelTerm(const SemanticProgram &program, const GuardedOrder &order, Term term,
                 unsigned levels, std::string &why, bool gatherLoads = false);

/// `term` with each level variable replaced by the matching entry of `values`.
Term bindLevels(const SemanticProgram &program, Term term, llvm::ArrayRef<Term> values);

/// Membership in S(v) and the strict lexicographic order, as terms over `values`.
Term inGuardedSet(const SemanticProgram &program, const GuardedOrder &order,
                  llvm::ArrayRef<Term> values);
Term guardedLess(const SemanticProgram &program, const GuardedOrder &order,
                 llvm::ArrayRef<Term> a, llvm::ArrayRef<Term> b);
/// The level variables themselves: the y of D1 to D3.
std::vector<Term> freeLevels(const SemanticProgram &program, const GuardedOrder &order);

/// D1 to D3. True only when every query is unsat; `why` names the first that is not.
bool proveGuardedOrbit(const SemanticProgram &program, const GuardedOrder &order,
                       const SiteArrivals &arrivals, llvm::ArrayRef<unsigned> locations,
                       Term scope, unsigned maxUnfolding, const AskUnsat &ask, std::string &why);

/// Theorem 15: segments s = 1..m, each with its own order, in execution order, with the
/// group's sites partitioned by `segmentOfLocation`. Beyond D1 to D3 within a segment:
///
///   D1'  The first event is at a segment s with λ = min S_s, and S_r is empty for r < s.
///        A run with no event has every S_s empty.
///   D4   From an event of segment s with no successor in S_s, the next event is at a
///        segment s' > s with λ = min S_{s'}, and S_r is empty for s < r < s'. From one
///        with a successor, the next event is at segment s with that successor.
///   D5   No event of a segment follows an event of a later one. From the last event, or
///        from the entry with no event, every later S_r is empty.
///
/// Then the extended points (s, λ_s) of the events, in order, are S_1 ⊎ ... ⊎ S_m in
/// order. True only when every query is unsat; `why` names the first that is not.
bool proveSegmentedOrbit(const SemanticProgram &program, llvm::ArrayRef<GuardedOrder> orders,
                         const std::map<unsigned, unsigned> &segmentOfLocation,
                         const SiteArrivals &arrivals, llvm::ArrayRef<unsigned> locations,
                         Term scope, unsigned maxUnfolding, const AskUnsat &ask, std::string &why);

/// A term over parameters and levels at one point. Nullopt for any other leaf or an
/// operation outside the integer fragment.
/// With `gather`, a gather load of that program (gatherLoadsOnly) has the contents the
/// replays give its root (gatherFillModulus): a sample of the gather, never all of them.
std::optional<llvm::APInt> evaluateLevelTerm(Term term, llvm::ArrayRef<llvm::APInt> parameters,
                                             llvm::ArrayRef<llvm::APInt> levels,
                                             const SemanticProgram *gather = nullptr);
bool concreteLess(const GuardedOrder &order, llvm::ArrayRef<llvm::APInt> a,
                  llvm::ArrayRef<llvm::APInt> b);

/// S(v) in order, or nullopt with `why` when the box holds more than `limit` points or
/// a term has no value.
std::optional<std::vector<std::vector<llvm::APInt>>>
enumerateGuardedSet(const GuardedOrder &order, llvm::ArrayRef<llvm::APInt> parameters,
                    uint64_t limit, std::string &why);

} // namespace mlir::generalized_gemm
#endif
