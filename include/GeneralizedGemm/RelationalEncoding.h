//===- RelationalEncoding.h - One-step SMT-LIB semantics --------*- C++ -*-===//
//
// Stable SMT-LIB text for the one-step relation T(c, c') and the predicates
// Stuck_UB(c), Stuck_Other(c). Text only: this library does not link a solver.
//
// c is (loc, s*, sp*, m*) and c' primes those names. Parameters are shared.
// T holds only for steps with no violated side condition. Integers are bit
// vectors; arrays are keyed by the concatenation of the index bits.
//
// Floats use the SMT FloatingPoint sort, which has one NaN. The encoding
// therefore identifies NaN payloads, which the concrete model does not.
//
//===----------------------------------------------------------------------===//
#ifndef GENERALIZEDGEMM_RELATIONALENCODING_H
#define GENERALIZEDGEMM_RELATIONALENCODING_H

#include "GeneralizedGemm/ConcreteEvaluator.h"
#include "GeneralizedGemm/SemanticProgram.h"

#include <string>

namespace mlir::generalized_gemm {

std::string encodeTransitionRelation(const SemanticProgram &program);

enum class StepQuery {
  // sat iff T admits pre -> post.
  AdmitsStep,
  // sat iff T admits a successor of pre different from post.
  AdmitsOtherStep,
  // sat iff T admits any successor of pre at all.
  AdmitsAnyStep,
  // sat iff pre is stuck on undefined behaviour.
  StuckUB,
  // sat iff pre is stuck on any other side condition.
  StuckOther,
};

/// The relation plus assertions pinning the configurations. `post` is ignored
/// for the Stuck and AdmitsAnyStep queries. Undefined state and uninitialized
/// cells stay free. `returns` pins the results of a step to the exit, as part of
/// post; a poison result pins only its poison bit.
std::string encodeStepQuery(const SemanticProgram &program,
                            const EntryState &entry, const Configuration &pre,
                            const Configuration *post, StepQuery query,
                            const std::vector<llvm::APInt> *returns = nullptr,
                            const std::vector<bool> *returnPoison = nullptr);

} // namespace mlir::generalized_gemm
#endif
