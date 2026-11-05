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
                 unsigned levels, std::string &why);

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

/// A term over parameters and levels at one point. Nullopt for any other leaf or an
/// operation outside the integer fragment.
std::optional<llvm::APInt> evaluateLevelTerm(Term term, llvm::ArrayRef<llvm::APInt> parameters,
                                             llvm::ArrayRef<llvm::APInt> levels);
bool concreteLess(const GuardedOrder &order, llvm::ArrayRef<llvm::APInt> a,
                  llvm::ArrayRef<llvm::APInt> b);

/// S(v) in order, or nullopt with `why` when the box holds more than `limit` points or
/// a term has no value.
std::optional<std::vector<std::vector<llvm::APInt>>>
enumerateGuardedSet(const GuardedOrder &order, llvm::ArrayRef<llvm::APInt> parameters,
                    uint64_t limit, std::string &why);

} // namespace mlir::generalized_gemm
#endif
