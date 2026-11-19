//===- PolynomialAdapter.h - P2b candidates as rank hints -------*- C++ -*-===//
//
// A narrow, one-way bridge to the Polynomial Termination research library. It
// reads P2b's unverified candidates only, never a P2b verdict or certificate
// status, and turns each into a G2a rank term. The G2a checker decides.
//
// A candidate names a header argument, a direction, a signedness and an offset.
// The bound is found in the model's own comparisons. Any mismatch between the
// two models therefore costs a hint, and can do nothing worse.
//
//===----------------------------------------------------------------------===//
#ifndef GENERALIZEDGEMM_POLYNOMIALADAPTER_H
#define GENERALIZEDGEMM_POLYNOMIALADAPTER_H

#include "GeneralizedGemm/SemanticImport.h"
#include "GeneralizedGemm/TerminationProposers.h"

#include <vector>

namespace mlir::generalized_gemm {

std::vector<RankHint> polynomialRankHints(const SemanticProgram &program,
                                          func::FuncOp function,
                                          const ImportOptions &options);

} // namespace mlir::generalized_gemm
#endif
