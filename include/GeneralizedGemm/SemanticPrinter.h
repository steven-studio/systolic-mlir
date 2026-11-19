//===- SemanticPrinter.h - Stable serialization -----------------*- C++ -*-===//
#ifndef GENERALIZEDGEMM_SEMANTICPRINTER_H
#define GENERALIZEDGEMM_SEMANTICPRINTER_H

#include "GeneralizedGemm/ConcreteEvaluator.h"
#include "GeneralizedGemm/SemanticProgram.h"

#include <string>

namespace mlir::generalized_gemm {

/// Deterministic JSON: no addresses, no hash order (invariant I7). An
/// unsupported program prints its reason and nothing else.
std::string toJSON(const SemanticProgram &program);
std::string toJSON(const SemanticProgram &program, const RunResult &run);

} // namespace mlir::generalized_gemm
#endif
